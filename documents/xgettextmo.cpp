/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Format knowledge: the public description of the GNU gettext .mo layout
 * (GPL documentation; layout only). The .po layout below was derived by
 * measuring msgunfmt's output on real catalogs; no gettext code was used.
 */
#include "xgettextmo.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <limits>
#include <memory>
#include <new>

namespace {

const qint64 MO_MAX_SOURCE = Q_INT64_C(64) * 1024 * 1024;
const qint64 MO_MAX_TEXT = Q_INT64_C(256) * 1024 * 1024;
const quint32 MO_MAX_STRINGS = 4000000;
const qint32 MO_PAGE_WIDTH = 79;

const quint32 MO_MAGIC = 0x950412deU;
const quint32 MO_MAGIC_SWAPPED = 0xde120495U;

quint32 readU32(const QByteArray &baData, qint32 nOffset, bool bBigEndian)
{
    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);
    return bBigEndian ? qFromBigEndian<quint32>(p) : qFromLittleEndian<quint32>(p);
}

// Decodes one UTF-8 sequence at nPos; returns the code point (or the byte
// value for an invalid sequence) and advances *pnLength.
quint32 utf8At(const QByteArray &baText, qint32 nPos, qint32 *pnLength)
{
    const quint8 c = static_cast<quint8>(baText.at(nPos));
    qint32 nLength = 1;
    quint32 nCode = c;
    if ((c & 0xE0U) == 0xC0U) {
        nLength = 2;
        nCode = c & 0x1FU;
    } else if ((c & 0xF0U) == 0xE0U) {
        nLength = 3;
        nCode = c & 0x0FU;
    } else if ((c & 0xF8U) == 0xF0U) {
        nLength = 4;
        nCode = c & 0x07U;
    }
    if ((nLength > 1) && ((nPos + nLength) <= baText.size())) {
        for (qint32 i = 1; i < nLength; ++i) {
            const quint8 t = static_cast<quint8>(baText.at(nPos + i));
            if ((t & 0xC0U) != 0x80U) {
                *pnLength = 1;
                return c;
            }
            nCode = (nCode << 6) | (t & 0x3FU);
        }
        *pnLength = nLength;
        return nCode;
    }
    *pnLength = 1;
    return c;
}

// Display width of one code point: 0 for combining marks, 2 for East Asian
// wide/fullwidth ranges, 1 otherwise.
qint32 codePointWidth(quint32 nCode)
{
    if (nCode == 0) return 0;
    if ((nCode >= 0x0300U && nCode <= 0x036FU) || (nCode >= 0x0483U && nCode <= 0x0489U) || (nCode >= 0x0591U && nCode <= 0x05BDU) ||
        (nCode >= 0x0610U && nCode <= 0x061AU) || (nCode >= 0x064BU && nCode <= 0x065FU) || (nCode >= 0x0E31U && nCode == 0x0E31U) ||
        (nCode >= 0x0E34U && nCode <= 0x0E3AU) || (nCode >= 0x0E47U && nCode <= 0x0E4EU) || (nCode >= 0x0EB1U && nCode == 0x0EB1U) ||
        (nCode >= 0x0EB4U && nCode <= 0x0EBCU) || (nCode >= 0x0EC8U && nCode <= 0x0ECDU) || (nCode >= 0x1AB0U && nCode <= 0x1AFFU) ||
        (nCode >= 0x1DC0U && nCode <= 0x1DFFU) || (nCode >= 0x200BU && nCode <= 0x200FU) || (nCode >= 0x20D0U && nCode <= 0x20FFU) ||
        (nCode >= 0xFE00U && nCode <= 0xFE0FU) || (nCode >= 0xFE20U && nCode <= 0xFE2FU) || (nCode == 0xFEFFU))
        return 0;
    if ((nCode >= 0x1100U && nCode <= 0x115FU) || (nCode >= 0x2E80U && nCode <= 0x303EU) || (nCode >= 0x3041U && nCode <= 0x33FFU) ||
        (nCode >= 0x3400U && nCode <= 0x4DBFU) || (nCode >= 0x4E00U && nCode <= 0x9FFFU) || (nCode >= 0xA000U && nCode <= 0xA4CFU) ||
        (nCode >= 0xAC00U && nCode <= 0xD7A3U) || (nCode >= 0xF900U && nCode <= 0xFAFFU) || (nCode >= 0xFE30U && nCode <= 0xFE4FU) ||
        (nCode >= 0xFF00U && nCode <= 0xFF60U) || (nCode >= 0xFFE0U && nCode <= 0xFFE6U) || (nCode >= 0x20000U && nCode <= 0x2FFFDU) ||
        (nCode >= 0x30000U && nCode <= 0x3FFFDU))
        return 2;
    return 1;
}

// One display "cell" of the escaped string: the escaped bytes and their width.
struct CELL {
    QByteArray baBytes;
    qint32 nWidth;
    bool bSpace;   // a break is allowed after this cell
    bool bNewline; // the escaped "\n" that ends a segment
};

QList<CELL> escapeToCells(const QByteArray &baValue, bool bUtf8)
{
    QList<CELL> listCells;
    qint32 nPos = 0;
    while (nPos < baValue.size()) {
        CELL cell;
        cell.nWidth = 1;
        cell.bSpace = false;
        cell.bNewline = false;
        const quint8 c = static_cast<quint8>(baValue.at(nPos));
        qint32 nLength = 1;
        if (c == '\n') {
            cell.baBytes = "\\n";
            cell.nWidth = 2;
            cell.bNewline = true;
        } else if (c == '\t') {
            cell.baBytes = "\\t";
            cell.nWidth = 2;
        } else if (c == '\r') {
            cell.baBytes = "\\r";
            cell.nWidth = 2;
        } else if (c == '"') {
            cell.baBytes = "\\\"";
            cell.nWidth = 2;
        } else if (c == '\\') {
            cell.baBytes = "\\\\";
            cell.nWidth = 2;
        } else if (c == 0x07) {
            cell.baBytes = "\\a";
            cell.nWidth = 2;
        } else if (c == 0x08) {
            cell.baBytes = "\\b";
            cell.nWidth = 2;
        } else if (c == 0x0C) {
            cell.baBytes = "\\f";
            cell.nWidth = 2;
        } else if (c == 0x0B) {
            cell.baBytes = "\\v";
            cell.nWidth = 2;
        } else if (c == ' ') {
            cell.baBytes = " ";
            cell.bSpace = true;
        } else if (c == '/') {
            // GNU gettext allows a line break after a slash as well as after a
            // space, and the greedy wrap takes the LAST opportunity that fits -
            // so "(UDP, HTTP, RTP/RTSP)" breaks as "RTP/" + "RTSP", not at the
            // space before "RTP".  Measured against msgunfmt on vlc.mo, whose
            // two long descriptions were the only difference in the family.
            cell.baBytes = "/";
            cell.bSpace = true;
        } else if (c < 0x20) {
            cell.baBytes = QByteArray(1, static_cast<char>(c));
            cell.nWidth = 0;
        } else if (bUtf8 && (c >= 0x80)) {
            const quint32 nCode = utf8At(baValue, nPos, &nLength);
            cell.baBytes = baValue.mid(nPos, nLength);
            cell.nWidth = (nLength == 1) ? 1 : codePointWidth(nCode);
        } else {
            cell.baBytes = QByteArray(1, static_cast<char>(c));
        }
        listCells.append(cell);
        nPos += nLength;
    }
    return listCells;
}

}  // namespace

XGettextMO::XGettextMO(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XGettextMO::parseCatalog(const QByteArray &baSource, CATALOG *pCatalog, PDSTRUCT *pPdStruct)
{
    if (!pCatalog) return false;
    *pCatalog = CATALOG();
    const qint32 nSize = baSource.size();
    if (nSize < 28) return false;
    const quint32 nMagicLE = readU32(baSource, 0, false);
    if (nMagicLE == MO_MAGIC) pCatalog->bBigEndian = false;
    else if (nMagicLE == MO_MAGIC_SWAPPED) pCatalog->bBigEndian = true;
    else return false;
    const bool bBE = pCatalog->bBigEndian;
    pCatalog->nRevision = readU32(baSource, 4, bBE);
    if ((pCatalog->nRevision >> 16) > 1) return false;  // major revision 0 or 1 only
    const quint32 nCount = readU32(baSource, 8, bBE);
    const quint32 nOriginalTable = readU32(baSource, 12, bBE);
    const quint32 nTranslationTable = readU32(baSource, 16, bBE);
    if (nCount > MO_MAX_STRINGS) return false;
    const quint64 nTableBytes = static_cast<quint64>(nCount) * 8U;
    if ((nOriginalTable > static_cast<quint32>(nSize)) || (nTranslationTable > static_cast<quint32>(nSize)) ||
        (nTableBytes > (static_cast<quint64>(nSize) - nOriginalTable)) || (nTableBytes > (static_cast<quint64>(nSize) - nTranslationTable)))
        return false;

    for (quint32 i = 0; i < nCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nOriginalEntry = static_cast<qint32>(nOriginalTable + i * 8U);
        const qint32 nTranslationEntry = static_cast<qint32>(nTranslationTable + i * 8U);
        const quint32 nIdLength = readU32(baSource, nOriginalEntry, bBE);
        const quint32 nIdOffset = readU32(baSource, nOriginalEntry + 4, bBE);
        const quint32 nStrLength = readU32(baSource, nTranslationEntry, bBE);
        const quint32 nStrOffset = readU32(baSource, nTranslationEntry + 4, bBE);
        // Every string is followed by its NUL terminator inside the file.
        if ((nIdOffset >= static_cast<quint32>(nSize)) || (nIdLength >= (static_cast<quint32>(nSize) - nIdOffset)) ||
            (nStrOffset >= static_cast<quint32>(nSize)) || (nStrLength >= (static_cast<quint32>(nSize) - nStrOffset)))
            return false;
        if ((baSource.at(static_cast<qint32>(nIdOffset + nIdLength)) != '\0') || (baSource.at(static_cast<qint32>(nStrOffset + nStrLength)) != '\0')) return false;
        const QByteArray baId = baSource.mid(static_cast<qint32>(nIdOffset), static_cast<qint32>(nIdLength));
        const QByteArray baStr = baSource.mid(static_cast<qint32>(nStrOffset), static_cast<qint32>(nStrLength));

        ENTRY entry;
        QByteArray baRest = baId;
        const qint32 nContextEnd = baRest.indexOf('\x04');
        if (nContextEnd >= 0) {
            entry.bHasContext = true;
            entry.baContext = baRest.left(nContextEnd);
            baRest = baRest.mid(nContextEnd + 1);
        }
        const qint32 nPluralStart = baRest.indexOf('\0');
        if (nPluralStart >= 0) {
            entry.bHasPlural = true;
            entry.baId = baRest.left(nPluralStart);
            entry.baPlural = baRest.mid(nPluralStart + 1);
            const qint32 nSecondNul = entry.baPlural.indexOf('\0');
            if (nSecondNul >= 0) entry.baPlural = entry.baPlural.left(nSecondNul);
        } else {
            entry.baId = baRest;
        }
        if (entry.bHasPlural) {
            entry.listStr = baStr.split('\0');
        } else {
            entry.listStr.append(baStr);
        }
        if (!entry.bHasContext && entry.baId.isEmpty() && !entry.bHasPlural && pCatalog->baCharset.isEmpty()) {
            // Header entry: pick up the declared charset.
            const qint32 nCharset = baStr.indexOf("charset=");
            if (nCharset >= 0) {
                qint32 nEnd = nCharset + 8;
                while ((nEnd < baStr.size()) && (baStr.at(nEnd) != '\n') && (baStr.at(nEnd) != '\r') && (baStr.at(nEnd) != ' ') && (baStr.at(nEnd) != ';')) ++nEnd;
                pCatalog->baCharset = baStr.mid(nCharset + 8, nEnd - nCharset - 8).trimmed().toLower();
            }
        }
        pCatalog->listEntries.append(entry);
    }
    return true;
}

void XGettextMO::writeField(QByteArray *pOut, const QByteArray &baKeyword, const QByteArray &baValue, bool bUtf8)
{
    if (!pOut) return;
    if (baValue.isEmpty()) {
        pOut->append(baKeyword);
        pOut->append(" \"\"\n");
        return;
    }
    const QList<CELL> listCells = escapeToCells(baValue, bUtf8);

    // Split into segments after each escaped newline.
    QList<QList<CELL> > listSegments;
    QList<CELL> listCurrent;
    for (qint32 i = 0; i < listCells.size(); ++i) {
        listCurrent.append(listCells.at(i));
        if (listCells.at(i).bNewline) {
            listSegments.append(listCurrent);
            listCurrent.clear();
        }
    }
    if (!listCurrent.isEmpty()) listSegments.append(listCurrent);

    // Total width of a segment (its escaped text) and of its single-line form.
    qint32 nTotalWidth = 0;
    for (qint32 i = 0; i < listCells.size(); ++i) nTotalWidth += listCells.at(i).nWidth;
    const bool bSingleLine = (listSegments.size() == 1) && ((baKeyword.size() + 1 + 2 + nTotalWidth) <= MO_PAGE_WIDTH);
    if (bSingleLine) {
        pOut->append(baKeyword);
        pOut->append(" \"");
        for (qint32 i = 0; i < listCells.size(); ++i) pOut->append(listCells.at(i).baBytes);
        pOut->append("\"\n");
        return;
    }

    pOut->append(baKeyword);
    pOut->append(" \"\"\n");
    const qint32 nLineWidth = MO_PAGE_WIDTH - 2;  // the two quotes
    for (qint32 s = 0; s < listSegments.size(); ++s) {
        const QList<CELL> &listSegment = listSegments.at(s);
        // Greedy wrap: break after the last breakable cell that keeps the line
        // within the width; a run without any break opportunity is emitted whole.
        qint32 nStart = 0;
        while (nStart < listSegment.size()) {
            qint32 nWidth = 0;
            qint32 nLastBreak = -1;
            qint32 nEnd = nStart;
            while (nEnd < listSegment.size()) {
                const qint32 nNextWidth = nWidth + listSegment.at(nEnd).nWidth;
                if ((nNextWidth > nLineWidth) && (nEnd > nStart)) break;
                nWidth = nNextWidth;
                if (listSegment.at(nEnd).bSpace) nLastBreak = nEnd;
                ++nEnd;
            }
            if ((nEnd < listSegment.size()) && (nLastBreak >= nStart)) nEnd = nLastBreak + 1;
            pOut->append('"');
            for (qint32 i = nStart; i < nEnd; ++i) pOut->append(listSegment.at(i).baBytes);
            pOut->append("\"\n");
            nStart = nEnd;
        }
    }
}

QByteArray XGettextMO::renderPO(const CATALOG &catalog)
{
    QByteArray baOut;
    const bool bUtf8 = catalog.baCharset.isEmpty() || (catalog.baCharset == "utf-8") || (catalog.baCharset == "utf8");
    for (qint32 i = 0; i < catalog.listEntries.size(); ++i) {
        const ENTRY &entry = catalog.listEntries.at(i);
        if (i > 0) baOut.append('\n');
        if (entry.bHasContext) writeField(&baOut, "msgctxt", entry.baContext, bUtf8);
        writeField(&baOut, "msgid", entry.baId, bUtf8);
        if (entry.bHasPlural) {
            writeField(&baOut, "msgid_plural", entry.baPlural, bUtf8);
            for (qint32 f = 0; f < entry.listStr.size(); ++f) {
                writeField(&baOut, "msgstr[" + QByteArray::number(f) + "]", entry.listStr.at(f), bUtf8);
            }
        } else {
            writeField(&baOut, "msgstr", entry.listStr.isEmpty() ? QByteArray() : entry.listStr.first(), bUtf8);
        }
    }
    return baOut;
}

bool XGettextMO::readSource(QByteArray *pData, PDSTRUCT *pPdStruct)
{
    if (!pData || !isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XGettextMO> guardedThis(this);
    const qint64 nSize = getSize();
    if (!guardedThis || (nSize < 28) || (nSize > MO_MAX_SOURCE) || (nSize > (std::numeric_limits<int>::max)())) return false;
    const QByteArray baMagic = read_array_process(0, 4, pPdStruct);
    if (!guardedThis || (baMagic.size() != 4)) return false;
    const quint32 nMagic = readU32(baMagic, 0, false);
    if ((nMagic != MO_MAGIC) && (nMagic != MO_MAGIC_SWAPPED)) return false;
    *pData = read_array_process(0, nSize, pPdStruct);
    return guardedThis && (pData->size() == nSize) && isPdStructNotCanceled(pPdStruct);
}

QString XGettextMO::memberName()
{
    const QString sFileName = XBinary::getDeviceFileName(getDevice());
    QString sStem;
    if (!sFileName.isEmpty()) sStem = QFileInfo(sFileName).completeBaseName();
    sStem = XBinary::fixFileName(sStem);
    if (sStem.isEmpty() || sStem.contains(QLatin1Char('/'))) sStem = QStringLiteral("messages");
    return sStem + QStringLiteral(".po");
}

bool XGettextMO::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGettextMO archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XGettextMO::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baSource;
    CATALOG catalog;
    return readSource(&baSource, pPdStruct) && parseCatalog(baSource, &catalog, pPdStruct) && isPdStructNotCanceled(pPdStruct);
}

XBinary::FT XGettextMO::getFileType()
{
    return FT_GETTEXT_MO;
}
XBinary::MODE XGettextMO::getMode()
{
    return MODE_DATA;
}
qint32 XGettextMO::getType()
{
    return TYPE_ARCHIVE;
}
XBinary::ENDIAN XGettextMO::getEndian()
{
    QByteArray baMagic = read_array(0, 4);
    return ((baMagic.size() == 4) && (readU32(baMagic, 0, false) == MO_MAGIC_SWAPPED)) ? ENDIAN_BIG : ENDIAN_LITTLE;
}
QString XGettextMO::getFileFormatExt()
{
    return QStringLiteral("mo");
}
QString XGettextMO::getFileFormatExtsString()
{
    return QStringLiteral("GNU gettext message catalog (*.mo *.gmo)");
}
QString XGettextMO::getMIMEString()
{
    return QStringLiteral("application/x-gettext-translation");
}
qint64 XGettextMO::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}
XBinary::OSNAME XGettextMO::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}
QString XGettextMO::getVersion()
{
    QByteArray baHeader = read_array(0, 8);
    if (baHeader.size() != 8) return QString();
    const bool bBE = (readU32(baHeader, 0, false) == MO_MAGIC_SWAPPED);
    const quint32 nRevision = readU32(baHeader, 4, bBE);
    return QStringLiteral("%1.%2").arg(nRevision >> 16).arg(nRevision & 0xFFFFU);
}
QList<QString> XGettextMO::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("DE120495") << QStringLiteral("950412DE");
}
XBinary *XGettextMO::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGettextMO(pDevice);
}

bool XGettextMO::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XGettextMO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !bindUnpackSource(pState, pPdStruct)) return false;

    QByteArray baSource;
    CATALOG catalog;
    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    bool bResult = pContext && readSource(&baSource, pPdStruct) && guardedThis && parseCatalog(baSource, &catalog, pPdStruct);
    if (bResult) {
        pContext->sName = memberName();
        pContext->baText = renderPO(catalog);
        pContext->nEntries = catalog.listEntries.size();
        bResult = guardedThis && (pContext->baText.size() <= MO_MAX_TEXT);
    }
    if (!bResult) {
        delete pContext;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    // msgunfmt writes nothing for an empty catalog: no member is published.
    pState->nNumberOfRecords = (pContext->nEntries > 0) ? 1 : 0;
    pState->nCurrentOffset = (pState->nNumberOfRecords == 0) ? baSource.size() : 0;
    pState->nTotalSize = baSource.size();
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XGettextMO::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XGettextMO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1))
        return ARCHIVERECORD();
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(pContext->baText.size()));
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PO text (synthesised, %1 entries)").arg(pContext->nEntries));
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XGettextMO::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XGettextMO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1) || devicesAlias(getDevice(), pDevice))
        return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nSize = pContext->baText.size();
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nSize)) return false;

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->sName) && pState->spOutputBudget->isEnforcing()) return false;
        if (!pState->spOutputBudget->debit(nSize) && pState->spOutputBudget->isEnforcing()) return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nSize, pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput || ((nSize > 0) && (pStage->write(pContext->baText) != nSize)) || !pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = nSize;
    return bResult && guardedThis;
}

bool XGettextMO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XGettextMO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex == pState->nNumberOfRecords) ? pState->nTotalSize : 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XGettextMO::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XGettextMO::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD};
}
