/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmsi.h"

#include "subdevice.h"
#include "xcab.h"
#include "xcfbf.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopedValueRollback>
#include <QSet>
#include <QTemporaryFile>
#include <QtEndian>
#if (QT_VERSION_MAJOR >= 6) && !defined(QT_CORE5COMPAT_LIB)
#include <QStringConverter>
#endif
#include <algorithm>
#include <limits>

namespace {
const qint64 MSI_MAX_TABLE_STREAM_SIZE = 256 * 1024 * 1024;

class MSI_OPERATION_STATE_DELETER {
public:
    explicit MSI_OPERATION_STATE_DELETER(const QSharedPointer<XMSI::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XMSI::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

struct MSI_STRING_POOL {
    QList<QString> listStrings;
    QList<bool> listPresent;
    qint32 nReferenceWidth;
};

struct MSI_COLUMN {
    QString sName;
    qint32 nNumber;
    qint32 nType;
    qint32 nWidth;
    qint64 nDataOffset;
    bool bString;
};

struct MSI_FILE_ROW {
    QString sKey;
    QString sComponent;
    QString sFileName;
    QString sName;
    qint32 nSequence;
    qint32 nAttributes;
    qint32 nTableOrder;
    qint64 nSize;
};

struct MSI_MEDIA_ROW {
    qint32 nDiskId;
    qint32 nLastSequence;
    QString sCabinet;
};

struct MSI_COMPONENT_ROW {
    QString sKey;
    QString sDirectory;
};

struct MSI_DIRECTORY_ROW {
    QString sKey;
    QString sParent;
    QString sTargetName;
    QString sSourceName;
};

enum BUILD_STATUS {
    BUILD_STATUS_NOT_APPLICABLE = 0,
    BUILD_STATUS_OK,
    BUILD_STATUS_ERROR
};

static quint16 readLE16(const QByteArray &baData, qint64 nOffset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

static quint32 readLE24(const QByteArray &baData, qint64 nOffset)
{
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData() + nOffset);
    return (quint32)pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16);
}

static quint32 readLE32(const QByteArray &baData, qint64 nOffset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

static QString decodeMsiStreamName(const QString &sRawName, bool *pbIsTable = nullptr)
{
    static const char szAlphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._";

    bool bIsTable = !sRawName.isEmpty() && (sRawName.at(0).unicode() == 0x4840);
    QString sResult;
    qint32 nIndex = bIsTable ? 1 : 0;

    for (; nIndex < sRawName.size(); nIndex++) {
        quint16 nValue = sRawName.at(nIndex).unicode();

        if ((nValue >= 0x3800) && (nValue <= 0x47FF)) {
            nValue -= 0x3800;
            sResult.append(QChar::fromLatin1(szAlphabet[nValue & 0x3F]));
            sResult.append(QChar::fromLatin1(szAlphabet[(nValue >> 6) & 0x3F]));
        } else if ((nValue >= 0x4800) && (nValue <= 0x483F)) {
            sResult.append(QChar::fromLatin1(szAlphabet[nValue - 0x4800]));
        } else {
            sResult.append(sRawName.at(nIndex));
        }
    }

    if (pbIsTable) {
        *pbIsTable = bIsTable;
    }

    return sResult;
}

static bool containsName(const QSet<QString> &setNames, const QString &sName, Qt::CaseSensitivity caseSensitivity)
{
    if (caseSensitivity == Qt::CaseSensitive) {
        return setNames.contains(sName);
    }

    return setNames.contains(sName.toCaseFolded());
}

static bool extractCfbfStreams(QIODevice *pDevice, const QSet<QString> &setNames, bool bTableStreams, qint64 nMaximumStreamSize, Qt::CaseSensitivity caseSensitivity,
                               QMap<QString, QByteArray> *pMapResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pMapResult) {
        return false;
    }

    XCFBF cfbf(pDevice);
    XBinary::UNPACK_STATE state = {};

    if (!cfbf.initUnpack(&state, cfbf.getDefaultUnpackProperties(), pPdStruct)) {
        return false;
    }

    bool bResult = true;
    qint64 nTotalExtractedSize = 0;

    for (qint32 i = 0; (i < state.nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        XBinary::ARCHIVERECORD record = cfbf.infoCurrent(&state, pPdStruct);
        QString sRawName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
        bool bIsTable = false;
        QString sDecodedName = decodeMsiStreamName(sRawName, &bIsTable);

        if ((bIsTable == bTableStreams) && containsName(setNames, sDecodedName, caseSensitivity)) {
            if ((record.nStreamSize < 0) || (record.nStreamSize > nMaximumStreamSize) || (record.nStreamSize > (qint64)(std::numeric_limits<qint32>::max)()) ||
                (nTotalExtractedSize > (nMaximumStreamSize - record.nStreamSize))) {
                bResult = false;
                break;
            }

            const QString sMapName = (caseSensitivity == Qt::CaseSensitive) ? sDecodedName : sDecodedName.toCaseFolded();
            if (pMapResult->contains(sMapName)) {
                bResult = false;
                break;
            }

            QByteArray baStream;
            QBuffer buffer(&baStream);

            if (!buffer.open(QIODevice::ReadWrite) || !cfbf.unpackCurrent(&state, &buffer, pPdStruct) || (baStream.size() != record.nStreamSize)) {
                bResult = false;
                break;
            }

            pMapResult->insert(sMapName, baStream);
            nTotalExtractedSize += record.nStreamSize;
        }

        if ((i + 1) < state.nNumberOfRecords) {
            if (!cfbf.moveToNext(&state, pPdStruct)) {
                bResult = false;
                break;
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        bResult = false;
    }

    if (!cfbf.finishUnpack(&state, pPdStruct)) {
        bResult = false;
    }

    return bResult;
}

static QString codePageName(quint32 nCodePage)
{
    switch (nCodePage) {
        case 0: return QString("Windows-1252");
        case 65001: return QString("UTF-8");
        case 932: return QString("Shift-JIS");
        case 936: return QString("GBK");
        case 949: return QString("CP949");
        case 950: return QString("Big5");
        default: return QString("Windows-%1").arg(nCodePage);
    }
}

static bool decodePoolString(const QByteArray &baData, quint32 nCodePage, QString *pResult)
{
    if (!pResult) {
        return false;
    }

    // A zero database code page is the Windows "neutral" code page. Such a
    // database is only portable when its strings are 7-bit; accepting extended
    // bytes with an arbitrary host locale silently changes table identifiers.
    if (nCodePage == 0) {
        for (char ch : baData) {
            if ((quint8)ch > 0x7F) {
                return false;
            }
        }
        *pResult = QString::fromLatin1(baData);
        return true;
    }

    if (nCodePage == 1200) {
        if (baData.size() & 1) {
            return false;
        }
        QString sResult;
        sResult.reserve(baData.size() / 2);
        for (qint32 i = 0; i < baData.size(); i += 2) {
            const quint16 nCharacter = (quint8)baData.at(i) | ((quint16)(quint8)baData.at(i + 1) << 8);
            if ((nCharacter >= 0xD800) && (nCharacter <= 0xDBFF)) {
                if ((i + 3) >= baData.size()) return false;
                const quint16 nLow = (quint8)baData.at(i + 2) | ((quint16)(quint8)baData.at(i + 3) << 8);
                if ((nLow < 0xDC00) || (nLow > 0xDFFF)) return false;
                sResult.append(QChar(nCharacter));
                sResult.append(QChar(nLow));
                i += 2;
            } else {
                if ((nCharacter >= 0xDC00) && (nCharacter <= 0xDFFF)) return false;
                sResult.append(QChar(nCharacter));
            }
        }
        *pResult = sResult;
        return true;
    }

#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    QTextCodec *pCodec = QTextCodec::codecForName(codePageName(nCodePage).toLatin1());
    if (!pCodec) {
        return false;
    }
    QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
    *pResult = pCodec->toUnicode(baData.constData(), baData.size(), &state);
    return state.invalidChars == 0;
#else
    if (nCodePage == 65001) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        *pResult = decoder.decode(baData);
        return !decoder.hasError();
    }
    return false;
#endif
}

static bool parseStringPool(const QByteArray &baPool, const QByteArray &baStringData, MSI_STRING_POOL *pResult)
{
    if (!pResult || (baPool.size() < 4)) {
        return false;
    }

    const quint16 nHeaderHigh = readLE16(baPool, 2);
    const quint32 nCodePage = readLE16(baPool, 0) | ((quint32)(nHeaderHigh & 0x7FFF) << 16);
    QList<quint32> listLengths;
    QList<bool> listPresent;
    qint64 nOffset = 4;
    quint64 nTotalStringSize = 0;

    while ((nOffset + 4) <= baPool.size()) {
        quint32 nLengthLow = readLE16(baPool, nOffset);
        quint16 nReferenceCount = readLE16(baPool, nOffset + 2);
        nOffset += 4;
        quint32 nLength = nLengthLow;

        // For a string of at least 64 KiB MSI emits a dummy pool entry whose
        // reference-count word is the high half of the length. The following
        // normal entry carries the low half and the actual reference count.
        // The dummy entry does not consume a string id.
        if ((nLengthLow == 0) && (nReferenceCount != 0)) {
            if ((nOffset + 4) > baPool.size()) {
                return false;
            }
            nLength = readLE16(baPool, nOffset) | ((quint32)nReferenceCount << 16);
            nReferenceCount = readLE16(baPool, nOffset + 2);
            nOffset += 4;
        }

        if (((nLength == 0) && (nReferenceCount != 0)) || ((nLength != 0) && (nReferenceCount == 0))) {
            return false;
        }

        nTotalStringSize += nLength;
        if ((nTotalStringSize > (quint64)baStringData.size()) || (listLengths.size() >= 0xFFFFFF)) {
            return false;
        }

        listLengths.append(nLength);
        listPresent.append(nLength != 0);
    }

    if (nOffset != baPool.size()) {
        return false;
    }

    pResult->listStrings.clear();
    pResult->listPresent.clear();
    pResult->nReferenceWidth = (nHeaderHigh & 0x8000) ? 3 : 2;
    nOffset = 0;

    for (quint32 nLength : listLengths) {
        if ((nOffset + nLength) > baStringData.size()) {
            return false;
        }

        QString sString;
        if (!decodePoolString(baStringData.mid((qint32)nOffset, (qint32)nLength), nCodePage, &sString)) {
            return false;
        }
        pResult->listStrings.append(sString);
        pResult->listPresent.append(listPresent.at(pResult->listPresent.size()));
        nOffset += nLength;
    }

    return nOffset == baStringData.size();
}

static bool readStringReference(const QByteArray &baData, qint64 nOffset, qint32 nWidth, quint32 *pnReference)
{
    if (!pnReference || (nOffset < 0) || ((nOffset + nWidth) > baData.size())) {
        return false;
    }

    if (nWidth == 2) {
        *pnReference = readLE16(baData, nOffset);
        return true;
    }

    if (nWidth == 3) {
        *pnReference = readLE24(baData, nOffset);
        return true;
    }

    return false;
}

static bool poolString(const MSI_STRING_POOL &pool, quint32 nReference, QString *pResult)
{
    if (!pResult) {
        return false;
    }

    if (nReference == 0) {
        pResult->clear();
        return true;
    }

    if ((nReference > (quint32)pool.listStrings.size()) || (pool.listPresent.size() != pool.listStrings.size()) || !pool.listPresent.at((qint32)nReference - 1)) {
        return false;
    }

    *pResult = pool.listStrings.at((qint32)nReference - 1);
    return true;
}

static bool decodeInteger16(const QByteArray &baData, qint64 nOffset, qint64 *pnValue, bool *pbNull = nullptr)
{
    if (!pnValue || (nOffset < 0) || ((nOffset + 2) > baData.size())) {
        return false;
    }

    quint16 nRaw = readLE16(baData, nOffset);
    if (pbNull) {
        *pbNull = (nRaw == 0);
    }
    *pnValue = (nRaw == 0) ? 0 : (qint16)(nRaw ^ 0x8000);
    return true;
}

static bool decodeInteger32(const QByteArray &baData, qint64 nOffset, qint64 *pnValue, bool *pbNull = nullptr)
{
    if (!pnValue || (nOffset < 0) || ((nOffset + 4) > baData.size())) {
        return false;
    }

    quint32 nRaw = readLE32(baData, nOffset);
    if (pbNull) {
        *pbNull = (nRaw == 0);
    }
    *pnValue = (nRaw == 0) ? 0 : (qint32)(nRaw ^ 0x80000000U);
    return true;
}

static bool compareColumnByNumber(const MSI_COLUMN &a, const MSI_COLUMN &b)
{
    return a.nNumber < b.nNumber;
}

static bool parseColumns(const QByteArray &baColumns, const MSI_STRING_POOL &pool, QMap<QString, QList<MSI_COLUMN> > *pMapColumns)
{
    if (!pMapColumns || ((pool.nReferenceWidth != 2) && (pool.nReferenceWidth != 3))) {
        return false;
    }

    const qint32 nFixedRowWidth = pool.nReferenceWidth * 2 + 4;
    if (baColumns.isEmpty() || (baColumns.size() % nFixedRowWidth)) {
        return false;
    }

    const qint32 nRows = baColumns.size() / nFixedRowWidth;
    if ((nRows <= 0) || (nRows > 65536)) {
        return false;
    }

    const qint64 nTableOffset = 0;
    const qint64 nNumberOffset = (qint64)nRows * pool.nReferenceWidth;
    const qint64 nNameOffset = nNumberOffset + (qint64)nRows * 2;
    const qint64 nTypeOffset = nNameOffset + (qint64)nRows * pool.nReferenceWidth;

    for (qint32 i = 0; i < nRows; i++) {
        quint32 nTableReference = 0;
        quint32 nNameReference = 0;
        qint64 nNumber = 0;
        qint64 nType = 0;
        QString sTable;
        QString sName;

        if (!readStringReference(baColumns, nTableOffset + (qint64)i * pool.nReferenceWidth, pool.nReferenceWidth, &nTableReference) ||
            !readStringReference(baColumns, nNameOffset + (qint64)i * pool.nReferenceWidth, pool.nReferenceWidth, &nNameReference) ||
            !decodeInteger16(baColumns, nNumberOffset + (qint64)i * 2, &nNumber) || !decodeInteger16(baColumns, nTypeOffset + (qint64)i * 2, &nType) ||
            !poolString(pool, nTableReference, &sTable) || !poolString(pool, nNameReference, &sName)) {
            return false;
        }

        if (sTable.isEmpty() || sName.isEmpty() || (nNumber <= 0) || (nNumber > 256) || (nType <= 0)) {
            return false;
        }

        MSI_COLUMN column = {};
        column.sName = sName;
        column.nNumber = (qint32)nNumber;
        column.nType = (qint32)nType;
        column.bString = (column.nType & 0x0800) != 0;
        column.nWidth = column.bString ? pool.nReferenceWidth : (((column.nType & 0xFF) == 4) ? 4 : 2);
        column.nDataOffset = 0;
        pMapColumns->operator[](sTable.toCaseFolded().normalized(QString::NormalizationForm_C)).append(column);
    }

    for (QMap<QString, QList<MSI_COLUMN> >::iterator it = pMapColumns->begin(); it != pMapColumns->end(); ++it) {
        std::sort(it.value().begin(), it.value().end(), compareColumnByNumber);

        QSet<QString> setColumnNames;
        for (qint32 i = 0; i < it.value().size(); i++) {
            const MSI_COLUMN &column = it.value().at(i);
            const QString sNormalizedName = column.sName.toCaseFolded().normalized(QString::NormalizationForm_C);
            const qint32 nDeclaredWidth = column.nType & 0xFF;

            // _Columns is column-major and column numbers must form a unique,
            // contiguous 1-based sequence.  Treating a duplicated/gapped
            // schema as a different row layout can otherwise make unrelated
            // bytes look like File/Media rows. A zero declared width is valid
            // for OBJECT/Binary Stream columns in unrelated MSI tables.
            if ((column.nNumber != (i + 1)) || setColumnNames.contains(sNormalizedName) ||
                (!column.bString && (nDeclaredWidth != 0) && (nDeclaredWidth != 2) && (nDeclaredWidth != 4))) {
                return false;
            }
            setColumnNames.insert(sNormalizedName);
        }
    }

    return true;
}

static bool hasColumnShape(const QList<MSI_COLUMN> &listColumns, qint32 nColumn, bool bString, qint32 nIntegerWidth = 0)
{
    if ((nColumn < 0) || (nColumn >= listColumns.size())) {
        return false;
    }

    const MSI_COLUMN &column = listColumns.at(nColumn);
    return (column.bString == bString) && (bString || ((column.nType & 0xFF) == nIntegerWidth));
}

static qint32 findColumn(const QList<MSI_COLUMN> &listColumns, const QString &sName)
{
    for (qint32 i = 0; i < listColumns.size(); i++) {
        if (listColumns.at(i).sName.compare(sName, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }

    return -1;
}

static bool prepareTable(const QByteArray &baTable, QList<MSI_COLUMN> *pListColumns, qint32 *pnRows)
{
    if (!pListColumns || !pnRows || pListColumns->isEmpty()) {
        return false;
    }

    qint64 nRowWidth = 0;
    for (const MSI_COLUMN &column : *pListColumns) {
        nRowWidth += column.nWidth;
    }

    if ((nRowWidth <= 0) || (nRowWidth > 4096) || (baTable.size() % nRowWidth)) {
        return false;
    }

    qint64 nRows = baTable.size() / nRowWidth;
    if (nRows > 65536) {
        return false;
    }

    qint64 nColumnOffset = 0;
    for (qint32 i = 0; i < pListColumns->size(); i++) {
        (*pListColumns)[i].nDataOffset = nColumnOffset;
        nColumnOffset += nRows * pListColumns->at(i).nWidth;
    }

    *pnRows = (qint32)nRows;
    return true;
}

static bool tableString(const QByteArray &baTable, const MSI_STRING_POOL &pool, const MSI_COLUMN &column, qint32 nRow, QString *pResult)
{
    if (!column.bString) {
        return false;
    }

    quint32 nReference = 0;
    if (!readStringReference(baTable, column.nDataOffset + (qint64)nRow * column.nWidth, column.nWidth, &nReference)) {
        return false;
    }

    return poolString(pool, nReference, pResult);
}

static bool tableInteger(const QByteArray &baTable, const MSI_COLUMN &column, qint32 nRow, qint64 *pnValue)
{
    if (column.bString) {
        return false;
    }

    qint64 nOffset = column.nDataOffset + (qint64)nRow * column.nWidth;
    if (column.nWidth == 4) {
        return decodeInteger32(baTable, nOffset, pnValue);
    }

    return decodeInteger16(baTable, nOffset, pnValue);
}

static bool isSafeWindowsBaseName(const QString &sName)
{
    if (sName.isEmpty() || (sName.size() > 255) || (sName == ".") || (sName == "..") || sName.endsWith(' ') || sName.endsWith('.')) {
        return false;
    }

    static const QString sForbidden = QStringLiteral("<>:\"/\\|?*");
    for (QChar character : sName) {
        if (sForbidden.contains(character) || (character == QChar::ReplacementCharacter) || (character.category() == QChar::Other_Control) ||
            (character.category() == QChar::Other_Surrogate)) {
            return false;
        }
    }

    QString sStem = sName.section('.', 0, 0).toUpper();
    sStem.replace(QChar(0x00B9), QLatin1Char('1'));
    sStem.replace(QChar(0x00B2), QLatin1Char('2'));
    sStem.replace(QChar(0x00B3), QLatin1Char('3'));
    static const QSet<QString> setReservedNames = {
        QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),    QStringLiteral("NUL"),     QStringLiteral("COM1"),
        QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),   QStringLiteral("COM5"),    QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),   QStringLiteral("LPT1"),    QStringLiteral("LPT2"),
        QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),   QStringLiteral("LPT6"),    QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9"), QStringLiteral("CONIN$"), QStringLiteral("CONOUT$"), QStringLiteral("CLOCK$")};
    return !setReservedNames.contains(sStem);
}

static bool installedFileName(const QString &sMsiFileName, QString *pResult)
{
    if (!pResult || sMsiFileName.isEmpty()) {
        return false;
    }

    qint32 nSeparator = sMsiFileName.indexOf('|');
    QString sResult = (nSeparator == -1) ? sMsiFileName : sMsiFileName.mid(nSeparator + 1);
    if (!isSafeWindowsBaseName(sResult)) {
        return false;
    }

    *pResult = sResult;
    return true;
}

static bool directoryPartName(const QString &sValue, QString *pResult)
{
    if (!pResult || sValue.isEmpty()) {
        return false;
    }

    qint32 nSeparator = sValue.indexOf('|');
    QString sResult = (nSeparator == -1) ? sValue : sValue.mid(nSeparator + 1);
    if ((sResult != ".") && !isSafeWindowsBaseName(sResult)) {
        return false;
    }

    *pResult = sResult;
    return true;
}

static bool parseDefaultDirectory(const QString &sDefaultDirectory, QString *pTargetName, QString *pSourceName)
{
    if (!pTargetName || !pSourceName || sDefaultDirectory.isEmpty()) {
        return false;
    }

    const qint32 nSeparator = sDefaultDirectory.indexOf(':');
    const QString sTarget = (nSeparator == -1) ? sDefaultDirectory : sDefaultDirectory.left(nSeparator);
    const QString sSource = (nSeparator == -1) ? sTarget : sDefaultDirectory.mid(nSeparator + 1);
    return directoryPartName(sTarget, pTargetName) && directoryPartName(sSource, pSourceName);
}

static bool isSafeRelativeWindowsPath(const QString &sPath)
{
    if (sPath.isEmpty() || QDir::isAbsolutePath(sPath) || sPath.contains('\\') || sPath.contains(':')) {
        return false;
    }

    const QStringList listParts = sPath.split('/', Qt::KeepEmptyParts);
    if (listParts.isEmpty() || (listParts.size() > 256)) {
        return false;
    }
    for (const QString &sPart : listParts) {
        if (!isSafeWindowsBaseName(sPart)) {
            return false;
        }
    }
    return true;
}

static bool compareFileRowBySequence(const MSI_FILE_ROW &a, const MSI_FILE_ROW &b)
{
    if (a.nSequence != b.nSequence) return a.nSequence < b.nSequence;
    return a.nTableOrder < b.nTableOrder;
}

static bool parseFileTable(const QByteArray &baTable, const MSI_STRING_POOL &pool, QList<MSI_COLUMN> listColumns, QList<MSI_FILE_ROW> *pListFiles)
{
    qint32 nRows = 0;
    if (!pListFiles || !prepareTable(baTable, &listColumns, &nRows)) {
        return false;
    }

    qint32 nFileColumn = findColumn(listColumns, "File");
    qint32 nComponentColumn = findColumn(listColumns, "Component_");
    qint32 nFileNameColumn = findColumn(listColumns, "FileName");
    qint32 nFileSizeColumn = findColumn(listColumns, "FileSize");
    qint32 nAttributesColumn = findColumn(listColumns, "Attributes");
    qint32 nSequenceColumn = findColumn(listColumns, "Sequence");
    if ((nFileColumn < 0) || (nComponentColumn < 0) || (nFileNameColumn < 0) || (nFileSizeColumn < 0) || (nAttributesColumn < 0) || (nSequenceColumn < 0) ||
        !hasColumnShape(listColumns, nFileColumn, true) || !hasColumnShape(listColumns, nComponentColumn, true) || !hasColumnShape(listColumns, nFileNameColumn, true) ||
        !hasColumnShape(listColumns, nFileSizeColumn, false, 4) || !hasColumnShape(listColumns, nAttributesColumn, false, 2) ||
        !hasColumnShape(listColumns, nSequenceColumn, false, 4)) {
        return false;
    }

    QSet<QString> setKeys;

    for (qint32 i = 0; i < nRows; i++) {
        MSI_FILE_ROW row = {};
        qint64 nSequence = 0;
        qint64 nFileSize = 0;
        qint64 nAttributes = 0;
        QString sMsiName;

        if (!tableString(baTable, pool, listColumns.at(nFileColumn), i, &row.sKey) || !tableString(baTable, pool, listColumns.at(nComponentColumn), i, &row.sComponent) ||
            !tableString(baTable, pool, listColumns.at(nFileNameColumn), i, &sMsiName) || !tableInteger(baTable, listColumns.at(nFileSizeColumn), i, &nFileSize) ||
            !tableInteger(baTable, listColumns.at(nAttributesColumn), i, &nAttributes) || !tableInteger(baTable, listColumns.at(nSequenceColumn), i, &nSequence) ||
            row.sKey.isEmpty() || row.sComponent.isEmpty() || (nFileSize < 0) || (nFileSize > (std::numeric_limits<qint32>::max)()) || (nAttributes < 0) ||
            (nAttributes > 0x7FFF) || ((nAttributes & 0x6000) == 0x6000) || (nSequence <= 0) || (nSequence > (std::numeric_limits<qint32>::max)()) ||
            !installedFileName(sMsiName, &row.sFileName)) {
            return false;
        }

        row.sName = row.sFileName;
        row.nSequence = (qint32)nSequence;
        row.nAttributes = (qint32)nAttributes;
        row.nTableOrder = i;
        row.nSize = nFileSize;
        QString sNormalizedKey = row.sKey.toCaseFolded().normalized(QString::NormalizationForm_C);
        if (setKeys.contains(sNormalizedKey)) {
            return false;
        }
        setKeys.insert(sNormalizedKey);
        pListFiles->append(row);
    }

    std::stable_sort(pListFiles->begin(), pListFiles->end(), compareFileRowBySequence);
    return true;
}

static bool parseComponentTable(const QByteArray &baTable, const MSI_STRING_POOL &pool, QList<MSI_COLUMN> listColumns, QMap<QString, MSI_COMPONENT_ROW> *pMapComponents)
{
    qint32 nRows = 0;
    if (!pMapComponents || !prepareTable(baTable, &listColumns, &nRows)) {
        return false;
    }

    const qint32 nComponentColumn = findColumn(listColumns, "Component");
    const qint32 nDirectoryColumn = findColumn(listColumns, "Directory_");
    if (!hasColumnShape(listColumns, nComponentColumn, true) || !hasColumnShape(listColumns, nDirectoryColumn, true)) {
        return false;
    }

    for (qint32 i = 0; i < nRows; i++) {
        MSI_COMPONENT_ROW row;
        if (!tableString(baTable, pool, listColumns.at(nComponentColumn), i, &row.sKey) ||
            !tableString(baTable, pool, listColumns.at(nDirectoryColumn), i, &row.sDirectory) || row.sKey.isEmpty() || row.sDirectory.isEmpty()) {
            return false;
        }

        const QString sNormalizedKey = row.sKey.toCaseFolded().normalized(QString::NormalizationForm_C);
        if (pMapComponents->contains(sNormalizedKey)) {
            return false;
        }
        pMapComponents->insert(sNormalizedKey, row);
    }

    return true;
}

static bool parseDirectoryTable(const QByteArray &baTable, const MSI_STRING_POOL &pool, QList<MSI_COLUMN> listColumns, QMap<QString, MSI_DIRECTORY_ROW> *pMapDirectories)
{
    qint32 nRows = 0;
    if (!pMapDirectories || !prepareTable(baTable, &listColumns, &nRows)) {
        return false;
    }

    const qint32 nDirectoryColumn = findColumn(listColumns, "Directory");
    const qint32 nParentColumn = findColumn(listColumns, "Directory_Parent");
    const qint32 nDefaultDirectoryColumn = findColumn(listColumns, "DefaultDir");
    if (!hasColumnShape(listColumns, nDirectoryColumn, true) || !hasColumnShape(listColumns, nParentColumn, true) ||
        !hasColumnShape(listColumns, nDefaultDirectoryColumn, true)) {
        return false;
    }

    for (qint32 i = 0; i < nRows; i++) {
        MSI_DIRECTORY_ROW row;
        QString sDefaultDirectory;
        if (!tableString(baTable, pool, listColumns.at(nDirectoryColumn), i, &row.sKey) || !tableString(baTable, pool, listColumns.at(nParentColumn), i, &row.sParent) ||
            !tableString(baTable, pool, listColumns.at(nDefaultDirectoryColumn), i, &sDefaultDirectory) || row.sKey.isEmpty() ||
            !parseDefaultDirectory(sDefaultDirectory, &row.sTargetName, &row.sSourceName)) {
            return false;
        }

        const QString sNormalizedKey = row.sKey.toCaseFolded().normalized(QString::NormalizationForm_C);
        if (pMapDirectories->contains(sNormalizedKey)) {
            return false;
        }
        pMapDirectories->insert(sNormalizedKey, row);
    }

    qint32 nRootCount = 0;
    for (QMap<QString, MSI_DIRECTORY_ROW>::const_iterator it = pMapDirectories->constBegin(); it != pMapDirectories->constEnd(); ++it) {
        const MSI_DIRECTORY_ROW &row = it.value();
        const bool bRoot = row.sParent.isEmpty() || (row.sParent.compare(row.sKey, Qt::CaseInsensitive) == 0);
        if (bRoot) {
            nRootCount++;
            if (row.sKey.compare(QStringLiteral("TARGETDIR"), Qt::CaseInsensitive) != 0) {
                return false;
            }
        } else if (!pMapDirectories->contains(row.sParent.toCaseFolded().normalized(QString::NormalizationForm_C))) {
            return false;
        }
    }

    return nRootCount == 1;
}

static bool resolveDirectoryParts(const QMap<QString, MSI_DIRECTORY_ROW> &mapDirectories, const QString &sDirectory, bool bSource, QStringList *pListParts)
{
    if (!pListParts || sDirectory.isEmpty()) {
        return false;
    }

    QStringList listReverse;
    QSet<QString> setVisited;
    QString sCurrent = sDirectory;

    while (!sCurrent.isEmpty()) {
        const QString sNormalized = sCurrent.toCaseFolded().normalized(QString::NormalizationForm_C);
        if (setVisited.contains(sNormalized) || !mapDirectories.contains(sNormalized) || (setVisited.size() >= 256)) {
            return false;
        }
        setVisited.insert(sNormalized);

        const MSI_DIRECTORY_ROW &row = mapDirectories.value(sNormalized);
        const bool bRoot = row.sParent.isEmpty() || (row.sParent.compare(row.sKey, Qt::CaseInsensitive) == 0);
        if (bRoot) {
            if (row.sKey.compare(QStringLiteral("TARGETDIR"), Qt::CaseInsensitive) != 0) {
                return false;
            }
            break;
        }

        const QString sPart = bSource ? row.sSourceName : row.sTargetName;
        if (sPart != ".") {
            if (!isSafeWindowsBaseName(sPart)) {
                return false;
            }
            listReverse.append(sPart);
        }
        sCurrent = row.sParent;
    }

    pListParts->clear();
    for (qint32 i = listReverse.size() - 1; i >= 0; i--) {
        pListParts->append(listReverse.at(i));
    }
    return true;
}

static bool assignOutputNames(QList<MSI_FILE_ROW> *pListFiles, const QMap<QString, MSI_COMPONENT_ROW> &mapComponents,
                              const QMap<QString, MSI_DIRECTORY_ROW> &mapDirectories)
{
    if (!pListFiles) {
        return false;
    }

    QMap<QString, qint32> mapBaseNameCounts;
    for (const MSI_FILE_ROW &file : *pListFiles) {
        mapBaseNameCounts[file.sFileName.toCaseFolded().normalized(QString::NormalizationForm_C)]++;
    }

    QSet<QString> setOutputNames;
    for (MSI_FILE_ROW &file : *pListFiles) {
        QString sOutputName = file.sFileName;
        if (mapBaseNameCounts.value(file.sFileName.toCaseFolded().normalized(QString::NormalizationForm_C)) > 1) {
            const QMap<QString, MSI_COMPONENT_ROW>::const_iterator componentIt =
                mapComponents.constFind(file.sComponent.toCaseFolded().normalized(QString::NormalizationForm_C));
            if (componentIt == mapComponents.constEnd()) {
                return false;
            }

            QStringList listParts;
            if (!resolveDirectoryParts(mapDirectories, componentIt->sDirectory, false, &listParts)) {
                return false;
            }
            listParts.append(file.sFileName);
            sOutputName = listParts.join('/');
        }

        QString sNormalizedName = sOutputName.toCaseFolded().normalized(QString::NormalizationForm_C);
        if (setOutputNames.contains(sNormalizedName)) {
            // Distinct conditional components can legally target the same
            // relative name. Keep both archive records without an overwrite by
            // giving the later collision a stable MSI-key namespace.
            const QString sKeyPart = QStringLiteral("id_") + file.sKey;
            if (!isSafeWindowsBaseName(sKeyPart)) {
                return false;
            }
            sOutputName = QStringLiteral("__msi/%1/%2").arg(sKeyPart, file.sFileName);
            sNormalizedName = sOutputName.toCaseFolded().normalized(QString::NormalizationForm_C);
        }
        if ((sOutputName.size() > 4096) || !isSafeRelativeWindowsPath(sOutputName) || setOutputNames.contains(sNormalizedName)) {
            return false;
        }
        setOutputNames.insert(sNormalizedName);
        file.sName = sOutputName;
    }

    return true;
}

static bool compareMediaRowByDiskId(const MSI_MEDIA_ROW &a, const MSI_MEDIA_ROW &b)
{
    return a.nDiskId < b.nDiskId;
}

static bool parseMediaTable(const QByteArray &baTable, const MSI_STRING_POOL &pool, QList<MSI_COLUMN> listColumns, QList<MSI_MEDIA_ROW> *pListMedia)
{
    qint32 nRows = 0;
    if (!pListMedia || !prepareTable(baTable, &listColumns, &nRows)) {
        return false;
    }

    qint32 nDiskIdColumn = findColumn(listColumns, "DiskId");
    qint32 nLastSequenceColumn = findColumn(listColumns, "LastSequence");
    qint32 nCabinetColumn = findColumn(listColumns, "Cabinet");
    if (!hasColumnShape(listColumns, nDiskIdColumn, false, 2) || !hasColumnShape(listColumns, nLastSequenceColumn, false, 4) ||
        !hasColumnShape(listColumns, nCabinetColumn, true)) {
        return false;
    }

    for (qint32 i = 0; i < nRows; i++) {
        MSI_MEDIA_ROW row = {};
        qint64 nDiskId = 0;
        qint64 nLastSequence = 0;

        if (!tableInteger(baTable, listColumns.at(nDiskIdColumn), i, &nDiskId) || !tableInteger(baTable, listColumns.at(nLastSequenceColumn), i, &nLastSequence) ||
            !tableString(baTable, pool, listColumns.at(nCabinetColumn), i, &row.sCabinet) || (nDiskId <= 0) || (nDiskId > 0x7FFFFFFF) || (nLastSequence < 0) ||
            (nLastSequence > 0x7FFFFFFF)) {
            return false;
        }

        row.nDiskId = (qint32)nDiskId;
        row.nLastSequence = (qint32)nLastSequence;
        pListMedia->append(row);
    }

    std::sort(pListMedia->begin(), pListMedia->end(), compareMediaRowByDiskId);

    qint32 nPreviousDiskId = 0;
    qint32 nPreviousLastSequence = 0;
    for (const MSI_MEDIA_ROW &row : *pListMedia) {
        if ((row.nDiskId <= nPreviousDiskId) || (row.nLastSequence <= nPreviousLastSequence)) {
            return false;
        }
        nPreviousDiskId = row.nDiskId;
        nPreviousLastSequence = row.nLastSequence;
    }

    return true;
}

static bool mapValueCaseInsensitive(const QMap<QString, QByteArray> &mapData, const QString &sName, QByteArray *pResult)
{
    for (QMap<QString, QByteArray>::const_iterator it = mapData.constBegin(); it != mapData.constEnd(); ++it) {
        if (it.key().compare(sName, Qt::CaseInsensitive) == 0) {
            if (pResult) {
                *pResult = it.value();
            }
            return true;
        }
    }

    return false;
}

static bool mapValueCaseSensitive(const QMap<QString, QByteArray> &mapData, const QString &sName, QByteArray *pResult)
{
    QMap<QString, QByteArray>::const_iterator it = mapData.constFind(sName);
    if (it == mapData.constEnd()) {
        return false;
    }

    if (pResult) {
        *pResult = it.value();
    }
    return true;
}

static bool pathsEqual(const QString &sFirst, const QString &sSecond)
{
#ifdef Q_OS_WIN
    return QDir::cleanPath(sFirst).compare(QDir::cleanPath(sSecond), Qt::CaseInsensitive) == 0;
#else
    return QDir::cleanPath(sFirst) == QDir::cleanPath(sSecond);
#endif
}

static QByteArray fingerprintExternalFile(const QString &sFileName, qint64 nExpectedSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (sFileName.isEmpty() || (nExpectedSize < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return QByteArray();

    const QFileInfo initialInfo(sFileName);
    if (!initialInfo.isFile() || initialInfo.isSymLink() || (initialInfo.size() != nExpectedSize) || !pathsEqual(initialInfo.canonicalFilePath(), sFileName))
        return QByteArray();

    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly)) return QByteArray();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray baBuffer;
    baBuffer.resize(1 << 20);
    qint64 nReadTotal = 0;
    while ((nReadTotal < nExpectedSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRequest = qMin<qint64>(baBuffer.size(), nExpectedSize - nReadTotal);
        const qint64 nRead = file.read(baBuffer.data(), nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) return QByteArray();
        hash.addData(QByteArray::fromRawData(baBuffer.constData(), nRead));
        nReadTotal += nRead;
    }

    const QFileInfo finalInfo(sFileName);
    if ((nReadTotal != nExpectedSize) || !file.atEnd() || (file.size() != nExpectedSize) || !finalInfo.isFile() || finalInfo.isSymLink() ||
        (finalInfo.size() != nExpectedSize) || !pathsEqual(initialInfo.canonicalFilePath(), finalInfo.canonicalFilePath()) || !XBinary::isPdStructNotCanceled(pPdStruct))
        return QByteArray();
    return hash.result();
}

static QString resolveCaseInsensitiveSibling(const QDir &sourceDirectory, const QString &sName)
{
    QFileInfo matchedInfo;
    qint32 nMatches = 0;
    const QFileInfoList listEntries = sourceDirectory.entryInfoList(QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &entryInfo : listEntries) {
        if (entryInfo.fileName().compare(sName, Qt::CaseInsensitive) == 0) {
            matchedInfo = entryInfo;
            nMatches++;
        }
    }

    // A case-sensitive host can contain two names which Windows would consider
    // identical. Refuse that ambiguity instead of selecting by enumeration
    // order.
    if ((nMatches != 1) || !matchedInfo.isFile() || matchedInfo.isSymLink()) {
        return QString();
    }

    const QString sSourceCanonical = QFileInfo(sourceDirectory.absolutePath()).canonicalFilePath();
    const QString sCandidateCanonical = matchedInfo.canonicalFilePath();
    if (sSourceCanonical.isEmpty() || sCandidateCanonical.isEmpty() || !pathsEqual(QFileInfo(sCandidateCanonical).absolutePath(), sSourceCanonical)) {
        return QString();
    }

    return sCandidateCanonical;
}

static bool pathIsWithinDirectory(const QString &sPath, const QString &sDirectory)
{
    QString sCleanPath = QDir::fromNativeSeparators(QDir::cleanPath(sPath));
    QString sCleanDirectory = QDir::fromNativeSeparators(QDir::cleanPath(sDirectory));
#ifdef Q_OS_WIN
    sCleanPath = sCleanPath.toCaseFolded();
    sCleanDirectory = sCleanDirectory.toCaseFolded();
#endif
    if (sCleanPath == sCleanDirectory) {
        return true;
    }
    if (!sCleanDirectory.endsWith('/')) {
        sCleanDirectory.append('/');
    }
    return sCleanPath.startsWith(sCleanDirectory);
}

static QString resolveCaseInsensitiveDescendant(const QDir &sourceRoot, const QStringList &listParts)
{
    if (listParts.isEmpty() || (listParts.size() > 256)) {
        return QString();
    }

    QString sRootCanonical = QFileInfo(sourceRoot.absolutePath()).canonicalFilePath();
    if (sRootCanonical.isEmpty()) {
        return QString();
    }

    QFileInfo matchedInfo;
    QDir currentDirectory(sRootCanonical);

    for (qint32 nPart = 0; nPart < listParts.size(); nPart++) {
        const QString &sName = listParts.at(nPart);
        if (!isSafeWindowsBaseName(sName)) {
            return QString();
        }

        qint32 nMatches = 0;
        const QFileInfoList listEntries = currentDirectory.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entryInfo : listEntries) {
            if (entryInfo.fileName().compare(sName, Qt::CaseInsensitive) == 0) {
                matchedInfo = entryInfo;
                nMatches++;
            }
        }

        if ((nMatches != 1) || matchedInfo.isSymLink()) {
            return QString();
        }

        const bool bLast = (nPart + 1) == listParts.size();
        if ((!bLast && !matchedInfo.isDir()) || (bLast && !matchedInfo.isFile())) {
            return QString();
        }

        const QString sCanonical = matchedInfo.canonicalFilePath();
        if (sCanonical.isEmpty() || !pathIsWithinDirectory(sCanonical, sRootCanonical)) {
            return QString();
        }
        if (!bLast) {
            currentDirectory = QDir(sCanonical);
        }
    }

    return matchedInfo.canonicalFilePath();
}

static QIODevice *openCabinetDevice(QIODevice *pMsiDevice, const QString &sCabinet, bool bEmbedded, const QMap<QString, QByteArray> &mapEmbeddedData,
                                    const QMap<QString, QByteArray> &mapExternalData)
{
    QPointer<QIODevice> guardedMsiDevice(pMsiDevice);
    QString sStorageName = bEmbedded ? sCabinet.mid(1) : sCabinet;
    QByteArray baCabinet;

    if (!isSafeWindowsBaseName(sStorageName)) {
        return nullptr;
    }

    const bool bHasMemoryCabinet =
        bEmbedded ? mapValueCaseSensitive(mapEmbeddedData, sStorageName, &baCabinet) : mapValueCaseInsensitive(mapExternalData, sStorageName, &baCabinet);
    if (bHasMemoryCabinet) {
        QBuffer *pBuffer = new QBuffer;
        pBuffer->setData(baCabinet);
        if (!pBuffer->open(QIODevice::ReadOnly)) {
            delete pBuffer;
            return nullptr;
        }
        return pBuffer;
    }

    if (bEmbedded || QDir::isAbsolutePath(sStorageName)) {
        return nullptr;
    }

    QFile *pMsiFile = dynamic_cast<QFile *>(guardedMsiDevice.data());
    QPointer<QFile> guardedMsiFile(pMsiFile);
    if (!guardedMsiDevice || !guardedMsiFile) {
        return nullptr;
    }

    const QString sMsiFileName = guardedMsiFile->fileName();
    if (!guardedMsiDevice || !guardedMsiFile || sMsiFileName.isEmpty()) {
        return nullptr;
    }

    const QDir sourceDirectory = QFileInfo(sMsiFileName).absoluteDir();
    const QString sCabinetCanonical = resolveCaseInsensitiveSibling(sourceDirectory, sStorageName);
    if (!guardedMsiDevice || !guardedMsiFile || sCabinetCanonical.isEmpty()) {
        return nullptr;
    }

    QFile *pCabinetFile = new QFile(sCabinetCanonical);
    if (!pCabinetFile->open(QIODevice::ReadOnly)) {
        delete pCabinetFile;
        return nullptr;
    }

    if (!guardedMsiDevice || !guardedMsiFile) {
        delete pCabinetFile;
        return nullptr;
    }

    return pCabinetFile;
}

static bool destroyCabinetContext(XMSI::CAB_CONTEXT *pCabinet)
{
    if (!pCabinet) {
        return true;
    }

    bool bResult = true;
    if (pCabinet->pArchive) {
        if (!pCabinet->pArchive->finishUnpack(&pCabinet->state, nullptr)) {
            bResult = false;
        }
        delete pCabinet->pArchive;
    }

    if (pCabinet->pDevice) {
        pCabinet->pDevice->close();
        delete pCabinet->pDevice;
    }

    delete pCabinet;
    return bResult;
}

static bool clearPayloadContexts(XMSI::UNPACK_CONTEXT *pContext)
{
    if (!pContext) {
        return true;
    }

    bool bResult = true;
    for (XMSI::CAB_CONTEXT *pCabinet : pContext->listCabinets) {
        if (!destroyCabinetContext(pCabinet)) {
            bResult = false;
        }
    }
    pContext->listCabinets.clear();
    pContext->listEntries.clear();
    return bResult;
}

static qint32 matchingCabinetRecord(const QList<QString> &listCabinetNames, const QList<MSI_FILE_ROW> &listFiles, qint32 nFileIndex, const QSet<qint32> &setUsed)
{
    const MSI_FILE_ROW &file = listFiles.at(nFileIndex);
    qint32 nMatch = -1;

    for (qint32 i = 0; i < listCabinetNames.size(); i++) {
        if (setUsed.contains(i)) {
            continue;
        }
        const QString &sCabinetName = listCabinetNames.at(i);
        if ((sCabinetName.compare(file.sKey, Qt::CaseInsensitive) == 0) || (sCabinetName.compare(file.sFileName, Qt::CaseInsensitive) == 0)) {
            if (nMatch != -1) {
                return -1;  // Ambiguous duplicate CAB record names.
            }
            nMatch = i;
        }
    }

    return nMatch;
}

static bool comparePayloadEntryBySequence(const XMSI::PAYLOAD_ENTRY &a, const XMSI::PAYLOAD_ENTRY &b)
{
    if (a.nSequence != b.nSequence) return a.nSequence < b.nSequence;
    return a.nTableOrder < b.nTableOrder;
}

static BUILD_STATUS buildInstalledPayload(QIODevice *pMsiDevice, qint64 nMsiSize, const QMap<QString, QByteArray> &mapExternalData,
                                          const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, XMSI::UNPACK_CONTEXT *pContext, XBinary::PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedMsiDevice(pMsiDevice);
    if (!guardedMsiDevice || !pContext || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return BUILD_STATUS_ERROR;
    }

    QSet<QString> setExternalKeys;
    for (QMap<QString, QByteArray>::const_iterator it = mapExternalData.constBegin(); it != mapExternalData.constEnd(); ++it) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return BUILD_STATUS_ERROR;
        }
        QString sNormalizedKey = it.key().toCaseFolded().normalized(QString::NormalizationForm_C);
        if (!isSafeWindowsBaseName(it.key()) || it.value().isEmpty() || setExternalKeys.contains(sNormalizedKey)) {
            return BUILD_STATUS_ERROR;
        }
        setExternalKeys.insert(sNormalizedKey);
    }

    QSet<QString> setTableNames;
    setTableNames << "_stringpool"
                  << "_stringdata"
                  << "_columns"
                  << "file"
                  << "media"
                  << "component"
                  << "directory";

    QMap<QString, QByteArray> mapTables;
    if (!extractCfbfStreams(guardedMsiDevice.data(), setTableNames, true, qMin(nMsiSize, MSI_MAX_TABLE_STREAM_SIZE), Qt::CaseInsensitive, &mapTables, pPdStruct) ||
        !guardedMsiDevice) {
        return BUILD_STATUS_ERROR;
    }

    const bool bHasFileTable = mapTables.contains("file");
    const bool bHasMediaTable = mapTables.contains("media");
    if (!bHasFileTable && !bHasMediaTable) {
        return mapExternalData.isEmpty() ? BUILD_STATUS_NOT_APPLICABLE : BUILD_STATUS_ERROR;
    }
    if (!bHasFileTable || !bHasMediaTable || !mapTables.contains("_stringpool") || !mapTables.contains("_stringdata") || !mapTables.contains("_columns")) {
        return BUILD_STATUS_ERROR;
    }

    MSI_STRING_POOL stringPool = {};
    QMap<QString, QList<MSI_COLUMN> > mapColumns;
    if (!parseStringPool(mapTables.value("_stringpool"), mapTables.value("_stringdata"), &stringPool) ||
        !parseColumns(mapTables.value("_columns"), stringPool, &mapColumns)) {
        return BUILD_STATUS_ERROR;
    }

    if (!mapColumns.contains("file") || !mapColumns.contains("media")) {
        return BUILD_STATUS_ERROR;
    }

    QList<MSI_FILE_ROW> listFiles;
    QList<MSI_MEDIA_ROW> listMedia;
    if (!parseFileTable(mapTables.value("file"), stringPool, mapColumns.value("file"), &listFiles) ||
        !parseMediaTable(mapTables.value("media"), stringPool, mapColumns.value("media"), &listMedia)) {
        return BUILD_STATUS_ERROR;
    }

    QMap<QString, MSI_COMPONENT_ROW> mapComponents;
    QMap<QString, MSI_DIRECTORY_ROW> mapDirectories;
    const bool bHasAnyComponentLayout =
        mapTables.contains("component") || mapTables.contains("directory") || mapColumns.contains("component") || mapColumns.contains("directory");
    const bool bHasComponentLayout =
        mapTables.contains("component") && mapTables.contains("directory") && mapColumns.contains("component") && mapColumns.contains("directory");
    if (bHasAnyComponentLayout && !bHasComponentLayout) {
        return BUILD_STATUS_ERROR;
    }
    if (bHasComponentLayout && (!parseComponentTable(mapTables.value("component"), stringPool, mapColumns.value("component"), &mapComponents) ||
                                !parseDirectoryTable(mapTables.value("directory"), stringPool, mapColumns.value("directory"), &mapDirectories))) {
        return BUILD_STATUS_ERROR;
    }
    if (bHasComponentLayout) {
        for (const MSI_COMPONENT_ROW &component : mapComponents) {
            if (!mapDirectories.contains(component.sDirectory.toCaseFolded().normalized(QString::NormalizationForm_C))) {
                return BUILD_STATUS_ERROR;
            }
        }
        for (const MSI_FILE_ROW &file : listFiles) {
            if (!mapComponents.contains(file.sComponent.toCaseFolded().normalized(QString::NormalizationForm_C))) {
                return BUILD_STATUS_ERROR;
            }
        }
    }
    if (!assignOutputNames(&listFiles, mapComponents, mapDirectories)) {
        return BUILD_STATUS_ERROR;
    }

    QSet<QString> setDeclaredEmbeddedCabinets;
    QSet<QString> setDeclaredExternalCabinets;
    for (const MSI_MEDIA_ROW &media : listMedia) {
        if (media.sCabinet.isEmpty()) continue;
        const bool bEmbedded = media.sCabinet.startsWith('#');
        const QString sStorageName = bEmbedded ? media.sCabinet.mid(1) : media.sCabinet;
        const QString sKey = bEmbedded ? sStorageName : sStorageName.toCaseFolded().normalized(QString::NormalizationForm_C);
        QSet<QString> &setCabinets = bEmbedded ? setDeclaredEmbeddedCabinets : setDeclaredExternalCabinets;
        if (!isSafeWindowsBaseName(sStorageName) || setCabinets.contains(sKey)) {
            return BUILD_STATUS_ERROR;
        }
        setCabinets.insert(sKey);
    }

    // Media ranges are (previous LastSequence, current LastSequence].  Every
    // File row must belong to exactly one such range before any cabinet is
    // opened.
    for (const MSI_FILE_ROW &file : listFiles) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return BUILD_STATUS_ERROR;
        }
        qint32 nMatches = 0;
        qint32 nPreviousLastSequence = 0;
        for (const MSI_MEDIA_ROW &media : listMedia) {
            if ((file.nSequence > nPreviousLastSequence) && (file.nSequence <= media.nLastSequence)) {
                nMatches++;
            }
            nPreviousLastSequence = media.nLastSequence;
        }
        if (nMatches != 1) {
            return BUILD_STATUS_ERROR;
        }
    }

    for (QMap<QString, QByteArray>::const_iterator it = mapExternalData.constBegin(); it != mapExternalData.constEnd(); ++it) {
        qint32 nMatches = 0;
        for (const MSI_MEDIA_ROW &media : listMedia) {
            if (!media.sCabinet.startsWith('#') && (media.sCabinet.compare(it.key(), Qt::CaseInsensitive) == 0)) {
                nMatches++;
            }
        }
        if (nMatches != 1) {
            return BUILD_STATUS_ERROR;
        }
    }

    QMap<QString, QByteArray> mapEmbeddedData;
    if (!setDeclaredEmbeddedCabinets.isEmpty() &&
        (!extractCfbfStreams(guardedMsiDevice.data(), setDeclaredEmbeddedCabinets, false, nMsiSize, Qt::CaseSensitive, &mapEmbeddedData, pPdStruct) ||
         !guardedMsiDevice)) {
        return BUILD_STATUS_ERROR;
    }

    qint32 nPreviousLastSequence = 0;

    for (const MSI_MEDIA_ROW &media : listMedia) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return BUILD_STATUS_ERROR;
        }
        QList<MSI_FILE_ROW> listMediaFiles;
        for (const MSI_FILE_ROW &file : listFiles) {
            if ((file.nSequence > nPreviousLastSequence) && (file.nSequence <= media.nLastSequence)) {
                listMediaFiles.append(file);
            }
        }
        nPreviousLastSequence = qMax(nPreviousLastSequence, media.nLastSequence);

        if (listMediaFiles.isEmpty()) {
            continue;
        }

        QList<MSI_FILE_ROW> listCompressedFiles;
        QList<MSI_FILE_ROW> listExternalFiles;
        QSet<qint32> setCompressedSequences;
        QMap<qint32, qint32> mapSequenceCounts;
        for (const MSI_FILE_ROW &file : listMediaFiles) {
            mapSequenceCounts[file.nSequence]++;
        }
        for (const MSI_FILE_ROW &file : listMediaFiles) {
            const bool bExplicitExternal = (file.nAttributes & 0x2000) != 0;
            const bool bExplicitCompressed = (file.nAttributes & 0x4000) != 0;
            const bool bCompressed = bExplicitCompressed || (!bExplicitExternal && !media.sCabinet.isEmpty());

            if (bCompressed) {
                // Duplicate sequence values are meaningful for loose source
                // files (their table order remains deterministic), but a
                // sequence participating in a cabinet must identify one File
                // row only.
                if (media.sCabinet.isEmpty() || (mapSequenceCounts.value(file.nSequence) != 1) || setCompressedSequences.contains(file.nSequence)) {
                    return BUILD_STATUS_ERROR;
                }
                setCompressedSequences.insert(file.nSequence);
                listCompressedFiles.append(file);
            } else {
                listExternalFiles.append(file);
            }
        }

        if (!listExternalFiles.isEmpty()) {
            QFile *pMsiFile = dynamic_cast<QFile *>(guardedMsiDevice.data());
            QPointer<QFile> guardedMsiFile(pMsiFile);
            if (!guardedMsiDevice || !guardedMsiFile || !bHasComponentLayout) {
                return BUILD_STATUS_ERROR;
            }

            const QString sMsiFileName = guardedMsiFile->fileName();
            if (!guardedMsiDevice || !guardedMsiFile || sMsiFileName.isEmpty()) return BUILD_STATUS_ERROR;
            const QDir sourceRoot = QFileInfo(sMsiFileName).absoluteDir();
            for (const MSI_FILE_ROW &file : listExternalFiles) {
                if (!guardedMsiDevice || !guardedMsiFile || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return BUILD_STATUS_ERROR;
                }
                QStringList listSourceParts;
                // In a compressed source image an explicitly uncompressed
                // override is stored at the source root. Otherwise resolve the
                // source side of the Component/Directory tree.
                if (!media.sCabinet.isEmpty() && ((file.nAttributes & 0x2000) != 0)) {
                    listSourceParts.clear();
                } else {
                    const QMap<QString, MSI_COMPONENT_ROW>::const_iterator componentIt =
                        mapComponents.constFind(file.sComponent.toCaseFolded().normalized(QString::NormalizationForm_C));
                    if ((componentIt == mapComponents.constEnd()) || !resolveDirectoryParts(mapDirectories, componentIt->sDirectory, true, &listSourceParts)) {
                        return BUILD_STATUS_ERROR;
                    }
                }
                listSourceParts.append(file.sFileName);

                const QString sExternalPath = resolveCaseInsensitiveDescendant(sourceRoot, listSourceParts);
                const QFileInfo externalInfo(sExternalPath);
                if (sExternalPath.isEmpty() || !externalInfo.isFile() || externalInfo.isSymLink() || (externalInfo.size() != file.nSize)) {
                    return BUILD_STATUS_ERROR;
                }
                const QByteArray baFingerprint = fingerprintExternalFile(sExternalPath, file.nSize, pPdStruct);
                if (baFingerprint.isEmpty() || (baFingerprint != fingerprintExternalFile(sExternalPath, file.nSize, pPdStruct))) {
                    return BUILD_STATUS_ERROR;
                }

                XMSI::PAYLOAD_ENTRY entry = {};
                entry.nCabinetIndex = -1;
                entry.nCabinetRecordIndex = -1;
                entry.nSequence = file.nSequence;
                entry.nTableOrder = file.nTableOrder;
                entry.nSize = file.nSize;
                entry.sName = file.sName;
                entry.sExternalPath = sExternalPath;
                entry.baExternalFingerprint = baFingerprint;
                pContext->listEntries.append(entry);
            }
        }

        if (!listCompressedFiles.isEmpty()) {
            QIODevice *pCabinetDevice = openCabinetDevice(guardedMsiDevice.data(), media.sCabinet, media.sCabinet.startsWith('#'), mapEmbeddedData, mapExternalData);
            if (!guardedMsiDevice || !pCabinetDevice) {
                delete pCabinetDevice;
                return BUILD_STATUS_ERROR;
            }

            XMSI::CAB_CONTEXT *pCabinet = new XMSI::CAB_CONTEXT;
            pCabinet->pDevice = pCabinetDevice;
            pCabinet->pArchive = new XCab(pCabinetDevice);
            pCabinet->state = XBinary::UNPACK_STATE();

            if (!pCabinet->pArchive->initUnpack(&pCabinet->state, mapProperties, pPdStruct)) {
                destroyCabinetContext(pCabinet);
                return BUILD_STATUS_ERROR;
            }

            QList<QString> listCabinetNames;
            QList<qint64> listCabinetSizes;
            for (qint32 i = 0; i < pCabinet->state.nNumberOfRecords; i++) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                    destroyCabinetContext(pCabinet);
                    return BUILD_STATUS_ERROR;
                }
                pCabinet->state.nCurrentIndex = i;
                XBinary::ARCHIVERECORD record = pCabinet->pArchive->infoCurrent(&pCabinet->state, pPdStruct);
                listCabinetNames.append(record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString());
                listCabinetSizes.append(record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, -1).toLongLong());
            }

            if (listCabinetNames.size() < listCompressedFiles.size()) {
                destroyCabinetContext(pCabinet);
                return BUILD_STATUS_ERROR;
            }

            QSet<qint32> setUsedRecords;
            QList<qint32> listAssignments;

            for (qint32 i = 0; i < listCompressedFiles.size(); i++) {
                qint32 nRecord = matchingCabinetRecord(listCabinetNames, listCompressedFiles, i, setUsedRecords);
                if ((nRecord == -1) || (listCabinetSizes.at(nRecord) != listCompressedFiles.at(i).nSize)) {
                    destroyCabinetContext(pCabinet);
                    return BUILD_STATUS_ERROR;
                }
                listAssignments.append(nRecord);
                setUsedRecords.insert(nRecord);
            }

            // MSI permits a cabinet (notably one produced from a merge module)
            // to contain records which are not present in the final File table.
            // The retained File rows must nevertheless occur in exactly their
            // authored order inside the cabinet.
            for (qint32 i = 1; i < listAssignments.size(); i++) {
                if (listAssignments.at(i) <= listAssignments.at(i - 1)) {
                    destroyCabinetContext(pCabinet);
                    return BUILD_STATUS_ERROR;
                }
            }

            const qint32 nCabinetIndex = pContext->listCabinets.size();
            pContext->listCabinets.append(pCabinet);

            for (qint32 i = 0; i < listCompressedFiles.size(); i++) {
                XMSI::PAYLOAD_ENTRY entry = {};
                entry.nCabinetIndex = nCabinetIndex;
                entry.nCabinetRecordIndex = listAssignments.at(i);
                entry.nSequence = listCompressedFiles.at(i).nSequence;
                entry.nTableOrder = listCompressedFiles.at(i).nTableOrder;
                entry.nSize = listCompressedFiles.at(i).nSize;
                entry.sName = listCompressedFiles.at(i).sName;
                pContext->listEntries.append(entry);
            }
        }
    }

    if (pContext->listEntries.size() != listFiles.size()) {
        return BUILD_STATUS_ERROR;
    }

    std::sort(pContext->listEntries.begin(), pContext->listEntries.end(), comparePayloadEntryBySequence);

    return BUILD_STATUS_OK;
}

static bool failExternalOutput(bool bSeekableOutput, QIODevice *pOutputDevice, qint64 *pnWritten)
{
    if (bSeekableOutput) {
        XBinary::resize(pOutputDevice, 0);
        pOutputDevice->seek(0);
        *pnWritten = 0;
    }
    return false;
}

static bool failGuardedExternalOutput(const QPointer<QIODevice> &guardedOutput, qint64 *pnWritten)
{
    if (guardedOutput) {
        XBinary::resize(guardedOutput.data(), 0);
        if (guardedOutput) guardedOutput->seek(0);
        *pnWritten = 0;
    }
    return false;
}

static bool unpackExternalPayloadFile(const XMSI::PAYLOAD_ENTRY &entry, QIODevice *pOutputDevice, qint64 *pnWritten, XBinary::PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedOutput(pOutputDevice);
    if (!guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() || guardedOutput->isSequential() ||
        (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) || !XBinary::isResizeEnable(guardedOutput.data()) || !pnWritten ||
        entry.sExternalPath.isEmpty() || entry.baExternalFingerprint.isEmpty() || (entry.nSize < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QFileInfo fileInfo(entry.sExternalPath);
    if (!fileInfo.isFile() || fileInfo.isSymLink() || (fileInfo.size() != entry.nSize) || !pathsEqual(fileInfo.canonicalFilePath(), entry.sExternalPath)) {
        return false;
    }

    QFile file(entry.sExternalPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    if (XBinary::devicesAlias(&file, guardedOutput.data())) return false;

    QTemporaryFile stage;
    if (!stage.open()) return false;

    QByteArray baBuffer;
    baBuffer.resize((qint32)qMin<qint64>(1 << 20, qMax<qint64>(1, entry.nSize)));
    QCryptographicHash sourceHash(QCryptographicHash::Sha256);
    qint64 nRemaining = entry.nSize;

    while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nToRead = qMin<qint64>(baBuffer.size(), nRemaining);
        qint64 nRead = 0;
        while (nRead < nToRead) {
            const qint64 nResult = file.read(baBuffer.data() + nRead, nToRead - nRead);
            if (nResult <= 0) {
                return failGuardedExternalOutput(guardedOutput, pnWritten);
            }
            nRead += nResult;
        }
        sourceHash.addData(QByteArray::fromRawData(baBuffer.constData(), nRead));

        qint64 nStaged = 0;
        while (nStaged < nRead) {
            const qint64 nToWrite = nRead - nStaged;
            const qint64 nResult = stage.write(baBuffer.constData() + nStaged, nToWrite);
            if ((nResult <= 0) || (nResult > nToWrite)) {
                return false;
            }
            nStaged += nResult;
        }
        nRemaining -= nRead;
    }

    QFileInfo finalInfo(entry.sExternalPath);
    if ((nRemaining != 0) || (file.size() != entry.nSize) || !file.atEnd() || !finalInfo.isFile() || finalInfo.isSymLink() || (finalInfo.size() != entry.nSize) ||
        !pathsEqual(finalInfo.canonicalFilePath(), entry.sExternalPath) || (sourceHash.result() != entry.baExternalFingerprint) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || !guardedOutput || !stage.flush() || (stage.size() != entry.nSize) || !stage.seek(0)) {
        return false;
    }

    *pnWritten = 0;
    if (!guardedOutput->seek(0) || !guardedOutput || !XBinary::resize(guardedOutput.data(), 0) || !guardedOutput || !guardedOutput->seek(0)) {
        return failGuardedExternalOutput(guardedOutput, pnWritten);
    }

    while (*pnWritten < entry.nSize) {
        if (!guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct)) return failGuardedExternalOutput(guardedOutput, pnWritten);
        const qint64 nToRead = qMin<qint64>(baBuffer.size(), entry.nSize - *pnWritten);
        const qint64 nRead = stage.read(baBuffer.data(), nToRead);
        if ((nRead <= 0) || (nRead > nToRead)) return failGuardedExternalOutput(guardedOutput, pnWritten);

        qint64 nPublished = 0;
        while (nPublished < nRead) {
            if (!guardedOutput) return false;
            const qint64 nToWrite = nRead - nPublished;
            const qint64 nResult = guardedOutput->write(baBuffer.constData() + nPublished, nToWrite);
            if ((nResult <= 0) || (nResult > nToWrite)) return failGuardedExternalOutput(guardedOutput, pnWritten);
            nPublished += nResult;
            *pnWritten += nResult;
        }
    }

    if (!guardedOutput || (guardedOutput->size() != entry.nSize) || !guardedOutput->seek(entry.nSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return failGuardedExternalOutput(guardedOutput, pnWritten);
    }
    return true;
}
}  // namespace

XMSI::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) {
        clearPayloadContexts(pContext);
        if (pContext->pArchive) {
            pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
        }
        if (pContext->pSourceValidator) {
            pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
            delete pContext->pSourceValidator;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }
        delete pContext;
    }
}

XMSI::XMSI(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackDeferredCleanup = QSharedPointer<UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pUnpackDeferredCleanup;
    m_pUnpackOperationState = QSharedPointer<bool>(new bool(false), MSI_OPERATION_STATE_DELETER(pDeferredCleanup));
    setIsArchive(true);
}

XMSI::~XMSI()
{
    if (m_pUnpackOperationState) *m_pUnpackOperationState = true;
    if (m_pUnpackDeferredCleanup) {
        m_pUnpackDeferredCleanup->setContexts.unite(m_setUnpackContexts);
        m_setUnpackContexts.clear();
    }
    m_pUnpackDeferredCleanup.clear();
    m_pUnpackOperationState.clear();
}

bool XMSI::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XMSI> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XMSI::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMSI x(pDevice);
    return x.isValid(pPdStruct);
}

XMSI::INTERNAL_INFO XMSI::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

bool XMSI::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMSI> guardedThis(this);
    const bool bAlreadyHandled = guardedThis->isInternalInfoHandled();
    if (!guardedThis) return false;

    if (!bAlreadyHandled) {
        const quint64 nTransaction = guardedThis->beginInternalInfoTransaction();
        if (!nTransaction) return false;

        // The transaction supplies the recursion sentinel. Keep every
        // source-derived value local until the same binding is revalidated.
        guardedThis->m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = guardedThis->_getInternalInfo(pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap = guardedThis->getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        guardedThis->m_internalInfo = info;
        if (!guardedThis->commitInternalInfoTransaction(nTransaction, static_cast<XBinary::INTERNAL_INFO *>(&guardedThis->m_internalInfo))) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XMSI::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMSI> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XMSI::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        setIsInternalInfoHandled(true);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        setIsInternalInfoHandled(false);
        XBinary::setInternalInfo(nullptr);
    }
}

XBinary::FT XMSI::getFileType()
{
    return FT_CFBF_MSI;
}

XMSI::INTERNAL_INFO XMSI::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};

    if (!XCFBF::isValid(getDevice(), pPdStruct)) return result;

    QByteArray baMagic = read_array_process(0, 8, pPdStruct);
    if (baMagic != QByteArray("\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8)) return result;

    QByteArray baShift = read_array_process(0x1E, 2, pPdStruct);
    if (baShift.size() < 2) return result;
    quint16 nShift = (quint16)((quint8)baShift.at(0) | ((quint16)(quint8)baShift.at(1) << 8));
    if ((nShift < 7) || (nShift > 20)) return result;
    quint32 nSectorSize = (quint32)1 << nShift;

    QByteArray baFirstDir = read_array_process(0x30, 4, pPdStruct);
    if (baFirstDir.size() < 4) return result;
    const quint8 *pfd = (const quint8 *)baFirstDir.constData();
    quint32 nFirstDir = (quint32)(pfd[0] | ((quint32)pfd[1] << 8) | ((quint32)pfd[2] << 16) | ((quint32)pfd[3] << 24));

    if (nFirstDir >= (quint64)(getSize() / nSectorSize)) return result;
    qint64 nRootOff = (qint64)(nFirstDir + 1) * (qint64)nSectorSize;
    QByteArray baClsid = read_array_process(nRootOff + 0x50, 16, pPdStruct);
    if (baClsid.size() < 16) return result;
    const quint8 *c = (const quint8 *)baClsid.constData();

    bool bTail = (c[1] == 0x10) && (c[2] == 0x0C) && (c[3] == 0x00) && (c[4] == 0x00) && (c[5] == 0x00) && (c[6] == 0x00) && (c[7] == 0x00) && (c[8] == 0xC0) &&
                 (c[9] == 0x00) && (c[10] == 0x00) && (c[11] == 0x00) && (c[12] == 0x00) && (c[13] == 0x00) && (c[14] == 0x00) && (c[15] == 0x46);
    if (!bTail) return result;

    if (c[0] == 0x84) {
        result.bIsValid = true;
        result.sVersion = "database";
    } else if (c[0] == 0x86) {
        result.bIsValid = true;
        result.sVersion = "patch";
    } else if (c[0] == 0x82) {
        result.bIsValid = true;
        result.sVersion = "transform";
    }

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XMSI::getDefaultUnpackProperties()
{
    return XBinary::getDefaultUnpackProperties();
}

void XMSI::setExternalCabinetData(const QMap<QString, QByteArray> &mapData)
{
    m_mapExternalCabinetData = mapData;
}

bool XMSI::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XMSI> guardedThis(this);
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        m_setUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        bool bFinishOK = clearPayloadContexts(pOldContext);
        if (pOldContext->pArchive) {
            bFinishOK = pOldContext->pArchive->finishUnpack(&pOldContext->innerState, nullptr) && bFinishOK;
            delete pOldContext->pArchive;
        }
        if (pOldContext->pSourceValidator) {
            pOldContext->pSourceValidator->releaseUnpackSource(&pOldContext->sourceValidationState);
            delete pOldContext->pSourceValidator;
        }
        if (pOldContext->pSubDevice) {
            pOldContext->pSubDevice->close();
            delete pOldContext->pSubDevice;
        }
        delete pOldContext;
        *pState = UNPACK_STATE();
        if (!guardedThis || !bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    XArchive initialSourceValidator(guardedSource.data());
    UNPACK_STATE initialSourceState = {};
    if (!initialSourceValidator.bindUnpackSource(&initialSourceState, pPdStruct) || !guardedThis || !guardedSource) return false;
    XMSI detector(guardedSource.data(), isImage(), getModuleAddress());
    const qint64 nTotalSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nTotalSize < 0)) return false;
    pState->nTotalSize = nTotalSize;
    pState->mapUnpackProperties = mapProperties;

    const INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!guardedThis || !guardedSource || !info.bIsValid || !initialSourceValidator.isUnpackSourceCurrent(&initialSourceState, pPdStruct) || !guardedThis ||
        !guardedSource)
        return false;

    const QMap<QString, QByteArray> mapExternalCabinetData = m_mapExternalCabinetData;

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->bPayloadMode = true;
    pContext->pSubDevice = nullptr;
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();
    pContext->pSourceValidator = new XArchive(guardedSource.data());
    pContext->sourceValidationState = UNPACK_STATE();

    if (!pContext->pSourceValidator->bindUnpackSource(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        delete pContext;
        return false;
    }

    BUILD_STATUS status = BUILD_STATUS_NOT_APPLICABLE;
    if (info.sVersion == QStringLiteral("database")) {
        status = buildInstalledPayload(guardedSource.data(), nTotalSize, mapExternalCabinetData, mapProperties, pContext, pPdStruct);
    }

    if (!guardedThis || !guardedSource || (status == BUILD_STATUS_ERROR)) {
        clearPayloadContexts(pContext);
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        delete pContext;
        return false;
    }

    if (status == BUILD_STATUS_OK) {
        if (!pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
            clearPayloadContexts(pContext);
            pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
            delete pContext->pSourceValidator;
            delete pContext;
            return false;
        }
        pState->nNumberOfRecords = pContext->listEntries.size();
        pContext->pOwnerState = pState;
        pState->pContext = pContext;
        m_setUnpackContexts.insert(pContext);
        return true;
    }

    if (!clearPayloadContexts(pContext) || !guardedThis || !guardedSource) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        delete pContext;
        return false;
    }

    // Non-database MSI containers (patches/transforms), or databases without
    // File/Media tables, retain the generic CFBF stream view.
    pContext->bPayloadMode = false;
    pContext->pSubDevice = new SubDevice(guardedSource.data(), 0, nTotalSize);
    if (!pContext->pSubDevice->open(QIODevice::ReadOnly)) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pContext->pArchive = new XCFBF(pContext->pSubDevice);
    if (!pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) || !guardedThis || !guardedSource) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    if (!pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    m_setUnpackContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XMSI::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XMSI> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()))
        return result;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext))
        return result;

    if (pContext->bPayloadMode) {
        if ((pState->nNumberOfRecords != pContext->listEntries.size()) || (pState->nCurrentIndex >= pContext->listEntries.size())) return result;
        const PAYLOAD_ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
        if (entry.nCabinetIndex == -1) {
            if (entry.sExternalPath.isEmpty() || (entry.nSize < 0)) return result;
            result.nStreamOffset = 0;
            result.nStreamSize = entry.nSize;
            result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sName);
            result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nSize);
            result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nSize);
            return result;
        }
        if ((entry.nCabinetIndex < 0) || (entry.nCabinetIndex >= pContext->listCabinets.size())) return result;
        CAB_CONTEXT *pCabinet = pContext->listCabinets.at(entry.nCabinetIndex);
        if (!pCabinet || !pCabinet->pArchive) return result;
        pCabinet->state.nCurrentIndex = entry.nCabinetRecordIndex;
        result = pCabinet->pArchive->infoCurrent(&pCabinet->state, pPdStruct);
        if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
        if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !m_setUnpackContexts.contains(pContext) ||
            (pState->pContext != pContext))
            return ARCHIVERECORD();
        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sName);
        return result;
    }

    if (!pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !m_setUnpackContexts.contains(pContext) ||
        (pState->pContext != pContext))
        return ARCHIVERECORD();
    QString sRawName = result.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, decodeMsiStreamName(sRawName));
    return result;
}

bool XMSI::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XMSI> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || !guardedThis || !guardedOutput || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()))
        return false;
    if (XBinary::devicesAlias(pContext->pOuterSourceDevice.data(), guardedOutput.data()) || !guardedThis || !guardedOutput) return false;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext))
        return false;

    if (pContext->bPayloadMode) {
        if ((pState->nNumberOfRecords != pContext->listEntries.size()) || (pState->nCurrentIndex >= pContext->listEntries.size())) return false;
        const PAYLOAD_ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
        if (entry.nCabinetIndex == -1) {
            // The external-sidecar route bypasses the base decode chain, so it
            // must charge the operation budget itself: one entry, entry.nSize
            // produced bytes (the copy loop is size- and hash-verified).
            if (pState->spOutputBudget) {
                if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sName)) {
                    if (pState->spOutputBudget->isEnforcing()) {
                        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                        return false;
                    }
                    XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
                }
                if (!pState->spOutputBudget->debit(entry.nSize)) {
                    if (pState->spOutputBudget->isEnforcing()) {
                        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                        return false;
                    }
                    XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
                }
            }
            const bool bResult = unpackExternalPayloadFile(entry, guardedOutput.data(), &pState->nCurrentOffset, pPdStruct);
            if (!guardedThis || !guardedOutput || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
            if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedOutput ||
                !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) {
                if (guardedOutput) {
                    XBinary::resize(guardedOutput.data(), 0);
                    if (guardedOutput) guardedOutput->seek(0);
                }
                return false;
            }
            return bResult;
        }
        if ((entry.nCabinetIndex < 0) || (entry.nCabinetIndex >= pContext->listCabinets.size())) return false;
        CAB_CONTEXT *pCabinet = pContext->listCabinets.at(entry.nCabinetIndex);
        if (!pCabinet || !pCabinet->pArchive) return false;
        pCabinet->state.nCurrentIndex = entry.nCabinetRecordIndex;
        // XFU-015: the inner cabinet route performs its own entry and output
        // accounting; share the operation budget into its state.
        pCabinet->state.spOutputBudget = pState->spOutputBudget;
        const bool bResult = pCabinet->pArchive->unpackCurrent(&pCabinet->state, guardedOutput.data(), pPdStruct);
        if (!guardedThis || !guardedOutput || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
        if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedOutput ||
            !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) {
            if (guardedOutput) {
                XBinary::resize(guardedOutput.data(), 0);
                if (guardedOutput) guardedOutput->seek(0);
            }
            return false;
        }
        pState->nCurrentOffset = pCabinet->state.nCurrentOffset;
        return bResult;
    }

    if (!pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    // XFU-015: the inner CFBF route performs its own entry and output
    // accounting; share the operation budget into its state.
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedOutput ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) {
        if (guardedOutput) {
            XBinary::resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XMSI::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XMSI> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()))
        return false;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext))
        return false;

    if (pContext->bPayloadMode) {
        if ((pState->nNumberOfRecords != pContext->listEntries.size()) || (pState->nCurrentIndex >= pContext->listEntries.size())) return false;
        const qint32 nOldIndex = pState->nCurrentIndex;
        const qint64 nOldOffset = pState->nCurrentOffset;
        pState->nCurrentIndex++;
        pState->nCurrentOffset = 0;
        const bool bSourceCurrent = pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) && guardedThis &&
                                    m_setUnpackContexts.contains(pContext) && (pState->pContext == pContext);
        if (!bSourceCurrent && guardedThis && m_setUnpackContexts.contains(pContext) && (pState->pContext == pContext)) {
            pState->nCurrentIndex = nOldIndex;
            pState->nCurrentOffset = nOldOffset;
        }
        return bSourceCurrent && (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    if (!pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    bool bResult = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !m_setUnpackContexts.contains(pContext) ||
        (pState->pContext != pContext))
        return false;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XMSI::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XMSI> guardedThis(this);

    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        m_setUnpackContexts.remove(pContext);
        pState->pContext = nullptr;
        if (!clearPayloadContexts(pContext)) {
            bResult = false;
        }

        if (pContext->pArchive) {
            if (!pContext->pArchive->finishUnpack(&pContext->innerState, nullptr)) {
                bResult = false;
            }
            delete pContext->pArchive;
        }
        if (pContext->pSourceValidator) {
            pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
            delete pContext->pSourceValidator;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }

        delete pContext;
        if (!guardedThis) return false;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    return bResult;
}
