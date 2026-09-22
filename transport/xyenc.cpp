/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Format knowledge: the public yEnc 1.2 description (yenc.org). No code was
 * taken from any decoder.
 */
#include "xyenc.h"


#include <limits>
#include <memory>
#include <new>

namespace {

const qint64 YENC_MAX_SOURCE = Q_INT64_C(256) * 1024 * 1024;
const qint64 YENC_MAX_PREAMBLE = Q_INT64_C(128) * 1024;
const qint32 YENC_MAX_ITEMS = 65536;
const qint32 YENC_MAX_PARTS = 65536;

// A multi-part file whose parts are still being joined.
struct PENDING {
    bool bOpen;
    QString sName;
    qint64 nDeclaredSize;
    qint64 nDeclaredTotal;
    qint32 nParts;
    QByteArray baData;
    quint32 nCrcState;
    bool bHaveFileCrc;
    quint32 nFileCrc;
    bool bEveryPartChecked;

    PENDING() : bOpen(false), nDeclaredSize(0), nDeclaredTotal(0), nParts(0), nCrcState(0xFFFFFFFFU), bHaveFileCrc(false), nFileCrc(0), bEveryPartChecked(true)
    {
    }
};

bool parseUnsigned(const QByteArray &baValue, qint64 *pnResult)
{
    if (!pnResult || baValue.isEmpty() || (baValue.size() > 15)) return false;
    qint64 nResult = 0;
    for (qint32 i = 0; i < baValue.size(); ++i) {
        const char c = baValue.at(i);
        if ((c < '0') || (c > '9')) return false;
        nResult = nResult * 10 + (c - '0');
    }
    *pnResult = nResult;
    return true;
}

bool parseHex32(const QByteArray &baValue, quint32 *pnResult)
{
    if (!pnResult || baValue.isEmpty() || (baValue.size() > 8)) return false;
    quint32 nResult = 0;
    for (qint32 i = 0; i < baValue.size(); ++i) {
        const char c = baValue.at(i);
        qint32 nDigit = -1;
        if ((c >= '0') && (c <= '9')) nDigit = c - '0';
        else if ((c >= 'a') && (c <= 'f')) nDigit = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'F')) nDigit = c - 'A' + 10;
        if (nDigit < 0) return false;
        nResult = (nResult << 4) | static_cast<quint32>(nDigit);
    }
    *pnResult = nResult;
    return true;
}

// Reads the line starting at *pnPos (CR/LF stripped) and advances *pnPos past it.
QByteArray takeLine(const QByteArray &baSource, qint32 *pnPos)
{
    const qint32 nSize = baSource.size();
    qint32 nEnd = baSource.indexOf('\n', *pnPos);
    const qint32 nNext = (nEnd < 0) ? nSize : (nEnd + 1);
    if (nEnd < 0) nEnd = nSize;
    if ((nEnd > *pnPos) && (baSource.at(nEnd - 1) == '\r')) --nEnd;
    const QByteArray baLine = baSource.mid(*pnPos, nEnd - *pnPos);
    *pnPos = nNext;
    return baLine;
}

QString memberName(const QByteArray &baName)
{
    QString sName = QString::fromUtf8(baName.trimmed());
    const qint32 nSlash = qMax(sName.lastIndexOf(QLatin1Char('/')), sName.lastIndexOf(QLatin1Char('\\')));
    if (nSlash >= 0) sName = sName.mid(nSlash + 1);
    sName = XBinary::fixFileName(sName);
    if (sName.isEmpty() || sName.contains(QLatin1Char('/'))) sName = QStringLiteral("decoded");
    return sName;
}

bool hasBeginLine(const QByteArray &baProbe)
{
    if (baProbe.startsWith("=ybegin ")) return true;
    return baProbe.indexOf("\n=ybegin ") >= 0;
}

}  // namespace

XYEnc::XYEnc(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XYEnc::parseAttributes(const QByteArray &baLine, qint32 nPrefix, QMap<QByteArray, QByteArray> *pMap)
{
    if (!pMap || (nPrefix < 0) || (nPrefix > baLine.size())) return false;
    pMap->clear();
    const qint32 nSize = baLine.size();
    qint32 i = nPrefix;
    while (i < nSize) {
        while ((i < nSize) && (baLine.at(i) == ' ')) ++i;
        if (i >= nSize) break;
        const qint32 nEqual = baLine.indexOf('=', i);
        if (nEqual < 0) return false;
        const QByteArray baKey = baLine.mid(i, nEqual - i).toLower();
        if (baKey.isEmpty() || baKey.contains(' ')) return false;
        if (baKey == "name") {
            // The name is always the last attribute and may contain spaces.
            pMap->insert(baKey, baLine.mid(nEqual + 1));
            break;
        }
        const qint32 nSpace = baLine.indexOf(' ', nEqual + 1);
        const QByteArray baValue = (nSpace < 0) ? baLine.mid(nEqual + 1) : baLine.mid(nEqual + 1, nSpace - nEqual - 1);
        pMap->insert(baKey, baValue);
        i = (nSpace < 0) ? nSize : (nSpace + 1);
    }
    return true;
}

bool XYEnc::decode(const QByteArray &baSource, QList<ITEM> *pItems, PDSTRUCT *pPdStruct)
{
    if (!pItems) return false;
    pItems->clear();
    quint32 *pCRCTable = XBinary::_getCRC32Table_EDB88320();
    if (!pCRCTable) return false;

    const qint32 nSize = baSource.size();
    qint32 nPos = 0;
    bool bSeenBlock = false;
    PENDING pending;

    while (nPos < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const QByteArray baLine = takeLine(baSource, &nPos);
        if (!baLine.startsWith("=ybegin ")) {
            // Article headers between parts are ordinary text; only the
            // preamble before the first block is bounded.
            if (!bSeenBlock && (nPos > YENC_MAX_PREAMBLE)) return false;
            continue;
        }
        bSeenBlock = true;

        QMap<QByteArray, QByteArray> mapBegin;
        if (!parseAttributes(baLine, 8, &mapBegin)) return false;
        if (!mapBegin.contains("size") || !mapBegin.contains("name")) return false;
        qint64 nFileSize = 0;
        if (!parseUnsigned(mapBegin.value("size"), &nFileSize) || (nFileSize <= 0) || (nFileSize > YENC_MAX_SOURCE)) return false;
        const QString sName = memberName(mapBegin.value("name"));
        qint64 nLineLength = 0;
        if (mapBegin.contains("line") && (!parseUnsigned(mapBegin.value("line"), &nLineLength) || (nLineLength <= 0))) return false;

        const bool bMultipart = mapBegin.contains("part");
        qint64 nPartNumber = 0;
        qint64 nTotal = 0;
        if (bMultipart && (!parseUnsigned(mapBegin.value("part"), &nPartNumber) || (nPartNumber <= 0) || (nPartNumber > YENC_MAX_PARTS))) return false;
        if (mapBegin.contains("total") && (!parseUnsigned(mapBegin.value("total"), &nTotal) || (nTotal <= 0) || (nTotal > YENC_MAX_PARTS))) return false;

        qint64 nPartBegin = 1;
        qint64 nPartEnd = nFileSize;
        if (bMultipart) {
            if (nPos >= nSize) return false;
            const QByteArray baPartLine = takeLine(baSource, &nPos);
            if (!baPartLine.startsWith("=ypart ")) return false;
            QMap<QByteArray, QByteArray> mapPart;
            if (!parseAttributes(baPartLine, 7, &mapPart)) return false;
            if (!parseUnsigned(mapPart.value("begin"), &nPartBegin) || !parseUnsigned(mapPart.value("end"), &nPartEnd)) return false;
            if ((nPartBegin < 1) || (nPartEnd < nPartBegin) || (nPartEnd > nFileSize)) return false;
        }
        const qint64 nExpectedPartSize = nPartEnd - nPartBegin + 1;

        QByteArray baPart;
        baPart.reserve(static_cast<qint32>(qMin<qint64>(nExpectedPartSize, nSize - nPos)));
        bool bEndSeen = false;
        QByteArray baEndLine;
        while (nPos < nSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const QByteArray baBody = takeLine(baSource, &nPos);
            if (baBody.startsWith("=yend")) {
                if ((baBody.size() > 5) && (baBody.at(5) != ' ')) return false;
                bEndSeen = true;
                baEndLine = baBody;
                break;
            }
            const qint32 nBodySize = baBody.size();
            for (qint32 i = 0; i < nBodySize; ++i) {
                quint8 nByte = static_cast<quint8>(baBody.at(i));
                if (nByte == '=') {
                    if ((i + 1) >= nBodySize) return false;  // an escape needs its operand on the same line
                    ++i;
                    nByte = static_cast<quint8>(static_cast<quint8>(baBody.at(i)) - 64U - 42U);
                } else if ((nByte == '\r') || (nByte == '\n')) {
                    continue;
                } else {
                    nByte = static_cast<quint8>(nByte - 42U);
                }
                baPart.append(static_cast<char>(nByte));
            }
            if (baPart.size() > nExpectedPartSize) return false;
        }
        if (!bEndSeen) return false;

        QMap<QByteArray, QByteArray> mapEnd;
        if (!parseAttributes(baEndLine, 5, &mapEnd)) return false;
        qint64 nEndSize = 0;
        if (!parseUnsigned(mapEnd.value("size"), &nEndSize)) return false;
        if ((nEndSize != baPart.size()) || (nEndSize != nExpectedPartSize)) return false;
        if (mapEnd.contains("part")) {
            qint64 nEndPart = 0;
            if (!parseUnsigned(mapEnd.value("part"), &nEndPart) || (nEndPart != nPartNumber)) return false;
        }
        bool bPartChecked = false;
        if (mapEnd.contains("pcrc32")) {
            quint32 nDeclared = 0;
            if (!parseHex32(mapEnd.value("pcrc32"), &nDeclared)) return false;
            const quint32 nActual = XBinary::_getCRC32(baPart.constData(), baPart.size(), 0xFFFFFFFFU, pCRCTable) ^ 0xFFFFFFFFU;
            if (nActual != nDeclared) return false;
            bPartChecked = true;
        }
        bool bHaveFileCrc = false;
        quint32 nFileCrc = 0;
        if (mapEnd.contains("crc32")) {
            if (!parseHex32(mapEnd.value("crc32"), &nFileCrc)) return false;
            bHaveFileCrc = true;
        }

        if (bMultipart) {
            if (pending.bOpen && ((pending.sName != sName) || (pending.nDeclaredSize != nFileSize) || (nPartBegin != (pending.baData.size() + 1)) ||
                                  (nPartNumber != (pending.nParts + 1)))) {
                return false;  // a gap, a reordered part, or another file before this one is complete
            }
            if (!pending.bOpen) {
                if ((nPartBegin != 1) || (nPartNumber != 1)) return false;
                pending = PENDING();
                pending.bOpen = true;
                pending.sName = sName;
                pending.nDeclaredSize = nFileSize;
                pending.nDeclaredTotal = nTotal;
            } else if ((nTotal != 0) && (pending.nDeclaredTotal != 0) && (nTotal != pending.nDeclaredTotal)) {
                return false;
            }
            if ((nTotal != 0) && (pending.nDeclaredTotal == 0)) pending.nDeclaredTotal = nTotal;
            pending.baData.append(baPart);
            pending.nCrcState = XBinary::_getCRC32(baPart.constData(), baPart.size(), pending.nCrcState, pCRCTable);
            ++pending.nParts;
            if (!bPartChecked) pending.bEveryPartChecked = false;
            if (bHaveFileCrc) {
                if (pending.bHaveFileCrc && (pending.nFileCrc != nFileCrc)) return false;
                pending.bHaveFileCrc = true;
                pending.nFileCrc = nFileCrc;
            }
            if ((pending.nDeclaredTotal != 0) && (pending.nParts > pending.nDeclaredTotal)) return false;
            if (pending.baData.size() == pending.nDeclaredSize) {
                if ((pending.nDeclaredTotal != 0) && (pending.nParts != pending.nDeclaredTotal)) return false;
                if (pending.bHaveFileCrc && ((pending.nCrcState ^ 0xFFFFFFFFU) != pending.nFileCrc)) return false;
                ITEM item;
                item.sName = pending.sName;
                item.baData = pending.baData;
                item.nParts = pending.nParts;
                item.bCrcVerified = pending.bHaveFileCrc || pending.bEveryPartChecked;
                pItems->append(item);
                pending = PENDING();
            }
        } else {
            if (pending.bOpen) return false;  // an unfinished multi-part file precedes this block
            if (baPart.size() != nFileSize) return false;
            if (bHaveFileCrc) {
                const quint32 nActual = XBinary::_getCRC32(baPart.constData(), baPart.size(), 0xFFFFFFFFU, pCRCTable) ^ 0xFFFFFFFFU;
                if (nActual != nFileCrc) return false;
            }
            ITEM item;
            item.sName = sName;
            item.baData = baPart;
            item.nParts = 1;
            item.bCrcVerified = bHaveFileCrc || bPartChecked;
            pItems->append(item);
        }
        if (pItems->size() > YENC_MAX_ITEMS) return false;
    }

    if (pending.bOpen) return false;  // the last multi-part file never reached its declared size
    return !pItems->isEmpty();
}

bool XYEnc::readSource(QByteArray *pData, PDSTRUCT *pPdStruct)
{
    if (!pData || !isPdStructNotCanceled(pPdStruct)) return false;
    const qint64 nSize = getSize();
    if ((nSize < 16) || (nSize > YENC_MAX_SOURCE) || (nSize > (std::numeric_limits<int>::max)())) return false;
    // Reject unrelated inputs from a bounded probe before materialising the source.
    const QByteArray baProbe = read_array_process(0, qMin<qint64>(nSize, YENC_MAX_PREAMBLE + 16), pPdStruct);
    if (!hasBeginLine(baProbe) || !isPdStructNotCanceled(pPdStruct)) return false;
    *pData = read_array_process(0, nSize, pPdStruct);
    return (pData->size() == nSize) && isPdStructNotCanceled(pPdStruct);
}

bool XYEnc::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XYEnc archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XYEnc::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baSource;
    QList<ITEM> listItems;
    return readSource(&baSource, pPdStruct) && decode(baSource, &listItems, pPdStruct) && isPdStructNotCanceled(pPdStruct);
}

XBinary::FT XYEnc::getFileType()
{
    return FT_YENC;
}
XBinary::MODE XYEnc::getMode()
{
    return MODE_DATA;
}
qint32 XYEnc::getType()
{
    return TYPE_ARCHIVE;
}
XBinary::ENDIAN XYEnc::getEndian()
{
    return ENDIAN_LITTLE;
}
QString XYEnc::getFileFormatExt()
{
    return QStringLiteral("yenc");
}
QString XYEnc::getFileFormatExtsString()
{
    return QStringLiteral("yEnc encoded data (*.yenc *.ntx)");
}
QString XYEnc::getMIMEString()
{
    return QStringLiteral("application/x-yenc");
}
qint64 XYEnc::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}
XBinary::OSNAME XYEnc::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}
QString XYEnc::getVersion()
{
    return QStringLiteral("1.2");
}
QList<QString> XYEnc::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'=ybegin '");
}
XBinary *XYEnc::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XYEnc(pDevice);
}

bool XYEnc::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!bindUnpackSource(pState, pPdStruct)) return false;

    QByteArray baSource;
    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext || !readSource(&baSource, pPdStruct) || !decode(baSource, &pContext->listItems, pPdStruct) || pContext->listItems.isEmpty()) {
        delete pContext;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listItems.size();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = baSource.size();
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XYEnc::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return ARCHIVERECORD();

    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->listItems.size() != pState->nNumberOfRecords) return ARCHIVERECORD();
    const ITEM &item = pContext->listItems.at(pState->nCurrentIndex);
    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, item.sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(item.baData.size()));
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    QString sMethod = (item.nParts > 1) ? QStringLiteral("yEnc %1 parts").arg(item.nParts) : QStringLiteral("yEnc");
    if (item.bCrcVerified) sMethod += QStringLiteral(" CRC32");
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sMethod);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XYEnc::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || devicesAlias(getDevice(), pDevice))
        return false;

    QIODevice *guardedOutput = pDevice;
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->listItems.size() != pState->nNumberOfRecords) return false;
    const ITEM &item = pContext->listItems.at(pState->nCurrentIndex);
    const qint64 nSize = item.baData.size();
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nSize)) return false;

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, item.sName) && pState->spOutputBudget->isEnforcing()) return false;
        if (!pState->spOutputBudget->debit(nSize) && pState->spOutputBudget->isEnforcing()) return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nSize, pPdStruct));
    if (!pStage || !guardedOutput || ((nSize > 0) && (pStage->write(item.baData) != nSize)) || !pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
    if (bResult) pState->nCurrentOffset = nSize;
    return bResult;
}

bool XYEnc::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex == pState->nNumberOfRecords) ? pState->nTotalSize : 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XYEnc::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XYEnc::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD};
}
