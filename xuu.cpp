/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * UU and begin-base64 recognition/decoding semantics are based on
 * libarchive's archive_read_support_filter_uu.c (Copyright 2009-2011
 * Michihiro NAKAJIMA, BSD-2-Clause).  See THIRD_PARTY_NOTICES.md.
 */
#include "xuu.h"

#include <QBuffer>
#include <QPointer>

#include <new>

#include "../Formats/xformats.h"

namespace {

const qint64 UU_MAX_PREAMBLE = Q_INT64_C(128) * 1024;
const qint64 UU_MAX_LINE = Q_INT64_C(1024) * 1024;
const qint64 UU_MAX_DECODED_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint32 UU_MAX_FILTER_DEPTH = 4;
const qint32 UU_MAX_BLOCKS = 65536;
const char UU_DEPTH_PROPERTY[] = "_xfileunpacker_filter_depth";

bool appendBounded(QByteArray *pOutput, char value, qint64 nOutputLimit)
{
    if (!pOutput || (nOutputLimit < 0) || (pOutput->size() >= nOutputLimit)) return false;
    pOutput->append(value);
    return true;
}

QByteArray stripLineEnding(QByteArray line)
{
    if (line.endsWith('\n')) line.chop(1);
    if (line.endsWith('\r')) line.chop(1);
    return line;
}

}  // namespace

XUU::UU_UNPACK_CONTEXT::UU_UNPACK_CONTEXT() : pDecodedDevice(nullptr), pInnerArchive(nullptr), innerState(), listBlocks(), bDirectPayload(false)
{
}

XUU::UU_UNPACK_CONTEXT::~UU_UNPACK_CONTEXT()
{
    if (pInnerArchive) {
        pInnerArchive->finishUnpack(&innerState, nullptr);
        delete pInnerArchive;
        pInnerArchive = nullptr;
    }
    delete pDecodedDevice;
    pDecodedDevice = nullptr;
}

XUU::XUU(QIODevice *pDevice) : XArchive(pDevice)
{
}

XUU::~XUU()
{
}

bool XUU::parseHeader(const QByteArray &line, bool *pbBase64, QString *pName)
{
    if (!pbBase64 || !pName) return false;

    QByteArray body = stripLineEnding(line);
    qint32 nPrefix = 0;
    bool bBase64 = false;
    if (body.startsWith("begin ")) {
        nPrefix = 6;
    } else if (body.startsWith("begin-base64 ")) {
        nPrefix = 13;
        bBase64 = true;
    } else {
        return false;
    }

    if ((body.size() < (nPrefix + 5)) || (body.at(nPrefix) < '0') || (body.at(nPrefix) > '7') || (body.at(nPrefix + 1) < '0') || (body.at(nPrefix + 1) > '7') ||
        (body.at(nPrefix + 2) < '0') || (body.at(nPrefix + 2) > '7') || (body.at(nPrefix + 3) != ' ')) {
        return false;
    }

    const QByteArray name = body.mid(nPrefix + 4);
    if (name.isEmpty() || name.contains('\0')) return false;
    for (char c : name) {
        const quint8 value = static_cast<quint8>(c);
        if ((value < 0x20) || (value > 0x7e)) return false;
    }

    *pbBase64 = bBase64;
    *pName = QString::fromLatin1(name);
    return true;
}

qint32 XUU::base64Value(quint8 value)
{
    if ((value >= 'A') && (value <= 'Z')) return value - 'A';
    if ((value >= 'a') && (value <= 'z')) return value - 'a' + 26;
    if ((value >= '0') && (value <= '9')) return value - '0' + 52;
    if (value == '+') return 62;
    if (value == '/') return 63;
    return -1;
}

bool XUU::decodeTransportAt(qint64 nSearchOffset, QByteArray *pOutput, QString *pDeclaredName, qint64 nOutputLimit, qint64 *pnNextSearchOffset,
                            bool *pbHeaderFound, PDSTRUCT *pPdStruct)
{
    QPointer<XUU> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!pOutput || !pDeclaredName || !pnNextSearchOffset || !pbHeaderFound || (nSearchOffset < 0) || (nOutputLimit < 0) || !guardedDevice ||
        guardedDevice->isSequential() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nOriginalPosition = guardedDevice->pos();
    if ((nOriginalPosition < 0) || !guardedDevice->seek(nSearchOffset)) return false;

    pOutput->clear();
    pDeclaredName->clear();
    *pnNextSearchOffset = nSearchOffset;
    *pbHeaderFound = false;
    bool bHeaderFound = false;
    bool bBase64 = false;
    qint64 nScanned = 0;
    // Keep the initial false-positive surface bounded, but after one complete
    // block the caller advances nSearchOffset and every remaining line belongs
    // to an already-recognized transport. Scan that remainder once so a large
    // inter-block comment cannot make later members disappear silently.
    const qint64 nHeaderScanLimit = (nSearchOffset == 0) ? UU_MAX_PREAMBLE : qMax<qint64>(0, guardedDevice->size() - nSearchOffset);

    while (!guardedDevice->atEnd() && (nScanned <= nHeaderScanLimit) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray line = guardedDevice->readLine(UU_MAX_LINE + 2);
        if (!guardedThis || !guardedDevice || line.isEmpty() || ((line.size() > UU_MAX_LINE) && !line.endsWith('\n'))) {
            guardedDevice->seek(nOriginalPosition);
            return false;
        }
        nScanned += line.size();
        if (parseHeader(line, &bBase64, pDeclaredName)) {
            bHeaderFound = true;
            *pbHeaderFound = true;
            break;
        }
    }

    bool bComplete = false;
    if (bHeaderFound && bBase64) {
        QByteArray carry;
        bool bPaddingSeen = false;
        while (!guardedDevice->atEnd() && XBinary::isPdStructNotCanceled(pPdStruct)) {
            QByteArray line = guardedDevice->readLine(UU_MAX_LINE + 2);
            if (!guardedThis || !guardedDevice || line.isEmpty() || ((line.size() > UU_MAX_LINE) && !line.endsWith('\n'))) {
                break;
            }
            line = stripLineEnding(line);
            if (line == "====") {
                bComplete = carry.isEmpty();
                break;
            }
            if (line.isEmpty() || bPaddingSeen) break;

            carry.append(line);
            while (carry.size() >= 4) {
                const quint8 c0 = static_cast<quint8>(carry.at(0));
                const quint8 c1 = static_cast<quint8>(carry.at(1));
                const quint8 c2 = static_cast<quint8>(carry.at(2));
                const quint8 c3 = static_cast<quint8>(carry.at(3));
                carry.remove(0, 4);
                const qint32 v0 = base64Value(c0);
                const qint32 v1 = base64Value(c1);
                const qint32 v2 = (c2 == '=') ? 0 : base64Value(c2);
                const qint32 v3 = (c3 == '=') ? 0 : base64Value(c3);
                if ((v0 < 0) || (v1 < 0) || (v2 < 0) || (v3 < 0) || ((c2 == '=') && (c3 != '=')) || ((c2 == '=') && ((v1 & 0x0f) != 0)) ||
                    ((c2 != '=') && (c3 == '=') && ((v2 & 0x03) != 0))) {
                    carry.clear();
                    bPaddingSeen = true;
                    bComplete = false;
                    goto decode_finished;
                }
                if (!appendBounded(pOutput, static_cast<char>((v0 << 2) | (v1 >> 4)), nOutputLimit)) goto decode_finished;
                if (c2 != '=') {
                    if (!appendBounded(pOutput, static_cast<char>((v1 << 4) | (v2 >> 2)), nOutputLimit)) goto decode_finished;
                    if (c3 != '=') {
                        if (!appendBounded(pOutput, static_cast<char>((v2 << 6) | v3), nOutputLimit)) goto decode_finished;
                    }
                }
                if ((c2 == '=') || (c3 == '=')) {
                    bPaddingSeen = true;
                    // Padding terminates the Base64 data.  Reject another
                    // quartet on the same line; the next line must be the
                    // transport terminator handled by the outer loop.
                    if (!carry.isEmpty()) {
                        carry.clear();
                        bComplete = false;
                        goto decode_finished;
                    }
                    break;
                }
            }
        }
    } else if (bHeaderFound) {
        bool bSawZeroLine = false;
        while (!guardedDevice->atEnd() && XBinary::isPdStructNotCanceled(pPdStruct)) {
            QByteArray line = guardedDevice->readLine(UU_MAX_LINE + 2);
            if (!guardedThis || !guardedDevice || line.isEmpty() || ((line.size() > UU_MAX_LINE) && !line.endsWith('\n'))) {
                break;
            }
            line = stripLineEnding(line);
            if (bSawZeroLine) {
                bComplete = (line == "end");
                break;
            }
            if (line.isEmpty()) break;
            const quint8 first = static_cast<quint8>(line.at(0));
            if ((first < 0x20) || (first > 0x60)) break;
            const qint32 nDecoded = (first - 0x20) & 0x3f;
            if (nDecoded > 45) break;
            if (nDecoded == 0) {
                bSawZeroLine = true;
                continue;
            }
            const qint32 nEncoded = ((nDecoded + 2) / 3) * 4;
            if (line.size() < (1 + nEncoded)) break;
            qint32 nWritten = 0;
            for (qint32 i = 0; (i < nEncoded) && (nWritten < nDecoded); i += 4) {
                quint8 values[4] = {};
                bool bValid = true;
                for (qint32 j = 0; j < 4; j++) {
                    const quint8 c = static_cast<quint8>(line.at(1 + i + j));
                    if ((c < 0x20) || (c > 0x60)) {
                        bValid = false;
                        break;
                    }
                    values[j] = (c - 0x20) & 0x3f;
                }
                if (!bValid) goto decode_finished;
                const char decoded[3] = {static_cast<char>((values[0] << 2) | (values[1] >> 4)), static_cast<char>((values[1] << 4) | (values[2] >> 2)),
                                         static_cast<char>((values[2] << 6) | values[3])};
                for (qint32 j = 0; (j < 3) && (nWritten < nDecoded); j++, nWritten++) {
                    if (!appendBounded(pOutput, decoded[j], nOutputLimit)) goto decode_finished;
                }
            }
        }
    }

decode_finished:
    if (guardedDevice) *pnNextSearchOffset = guardedDevice->pos();
    if (guardedDevice) guardedDevice->seek(nOriginalPosition);
    if (!guardedThis || !guardedDevice || !bComplete || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pOutput->clear();
        pDeclaredName->clear();
        return false;
    }
    return true;
}

bool XUU::decodeTransports(QList<UU_BLOCK> *pBlocks, qint64 nEntryLimit, qint64 nAggregateLimit, qint32 nBlockLimit, PDSTRUCT *pPdStruct)
{
    if (!pBlocks || (nEntryLimit < 0) || (nAggregateLimit < 0) || (nBlockLimit < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    pBlocks->clear();

    QPointer<XUU> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedThis || !guardedDevice || guardedDevice->isSequential()) return false;
    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedThis || !guardedDevice || (nDeviceSize < 0)) return false;

    qint64 nSearchOffset = 0;
    qint64 nRemainingLimit = nAggregateLimit;
    while ((nSearchOffset < nDeviceSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        UU_BLOCK block;
        qint64 nNextSearchOffset = nSearchOffset;
        bool bHeaderFound = false;
        // Once the configured member count is reached, continue scanning only
        // to distinguish a harmless footer from an additional block. A zero
        // decode allowance refuses the first byte of any extra non-empty block
        // without allocating it; an extra empty block is rejected below.
        const qint64 nBlockOutputLimit = (pBlocks->count() >= nBlockLimit) ? 0 : qMin(nEntryLimit, nRemainingLimit);
        const bool bDecoded = decodeTransportAt(nSearchOffset, &block.baDecoded, &block.sDeclaredName, nBlockOutputLimit, &nNextSearchOffset, &bHeaderFound, pPdStruct);
        if (!guardedThis || !guardedDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (!bDecoded) {
            // No later valid header is normal termination. Once a header has
            // been accepted, however, a malformed/truncated block must make
            // the whole transport fail instead of silently publishing only
            // the preceding files.
            if (bHeaderFound) return false;
            break;
        }
        if (pBlocks->count() >= nBlockLimit) return false;
        if ((nNextSearchOffset <= nSearchOffset) || block.sDeclaredName.isEmpty() || (block.baDecoded.size() > nRemainingLimit)) return false;
        nRemainingLimit -= block.baDecoded.size();
        pBlocks->append(block);
        nSearchOffset = nNextSearchOffset;
    }

    return guardedThis && guardedDevice && !pBlocks->isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XUU::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    const bool bResult = initUnpack(&state, QMap<UNPACK_PROP, QVariant>(), pPdStruct);
    const bool bFinished = finishUnpack(&state, nullptr);
    return bResult && bFinished;
}

bool XUU::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XUU archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XUU::getFileType()
{
    return FT_UU;
}
XBinary::MODE XUU::getMode()
{
    return MODE_DATA;
}
QString XUU::getMIMEString()
{
    return "application/x-uuencode";
}
QString XUU::getFileFormatExt()
{
    return "uu";
}
QString XUU::getFileFormatExtsString()
{
    return "UUencoded archive (*.uu *.uue)";
}

QList<QString> XUU::getSearchSignatures()
{
    return QList<QString>() << "'begin '" << "'begin-base64 '";
}

XBinary *XUU::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XUU(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XUU::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XUU::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XUU> guardedThis(this);
    if (!pState || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis) return false;

    UU_UNPACK_CONTEXT *pContext = new (std::nothrow) UU_UNPACK_CONTEXT();
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    qint64 nLegacyOutputLimit = -1;
    OUTPUT_POLICY outputPolicy = {};
    if (!getUnpackOutputLimit(mapProperties, &nLegacyOutputLimit) || !resolveUnpackOutputPolicy(mapProperties, &outputPolicy)) {
        setPdStructErrorString(pPdStruct, tr("Invalid unpacked-output limit"));
        delete pContext;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    qint64 nEntryLimit = UU_MAX_DECODED_SIZE;
    qint64 nAggregateLimit = UU_MAX_DECODED_SIZE;
    qint32 nBlockLimit = UU_MAX_BLOCKS;
    if (nLegacyOutputLimit >= 0) nEntryLimit = qMin(nEntryLimit, nLegacyOutputLimit);
    if (outputPolicy.nMaxEntryOutputSize >= 0) nEntryLimit = qMin(nEntryLimit, outputPolicy.nMaxEntryOutputSize);
    if (outputPolicy.nMaxMemoryOutputSize >= 0) nAggregateLimit = qMin(nAggregateLimit, outputPolicy.nMaxMemoryOutputSize);
    if (outputPolicy.nMaxTotalOutputSize >= 0) nAggregateLimit = qMin(nAggregateLimit, outputPolicy.nMaxTotalOutputSize);
    if (outputPolicy.nMaxEntryCount >= 0) nBlockLimit = (qint32)qMin<qint64>(nBlockLimit, outputPolicy.nMaxEntryCount);

    bool bResult = decodeTransports(&pContext->listBlocks, nEntryLimit, nAggregateLimit, nBlockLimit, pPdStruct);
    QPointer<QIODevice> guardedSource(getDevice());
    qint32 nDepth = guardedSource ? guardedSource->property(UU_DEPTH_PROPERTY).toInt() : 0;
    if (!guardedThis || !guardedSource || !bResult || (nDepth >= UU_MAX_FILTER_DEPTH)) {
        delete pContext;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pContext->pDecodedDevice = new (std::nothrow) QBuffer();
    if (pContext->pDecodedDevice) {
        pContext->pDecodedDevice->setData(pContext->listBlocks.isEmpty() ? QByteArray() : pContext->listBlocks.first().baDecoded);
        pContext->pDecodedDevice->setProperty(UU_DEPTH_PROPERTY, nDepth + 1);
        bResult = pContext->pDecodedDevice->open(QIODevice::ReadOnly);
    } else {
        bResult = false;
    }

    FT innerType = FT_UNKNOWN;
    // A single transport block retains the historical filter behaviour: if
    // its decoded bytes are themselves an archive, expose that archive's
    // records. Multiple blocks are a file bundle and their declared names are
    // the member model, so do not collapse the first one into a nested view.
    if (bResult && (pContext->listBlocks.count() == 1)) {
        innerType = XFormats::getPrefFileType(pContext->pDecodedDevice, FT_FLAG_ARCHIVES, pPdStruct);
    }
    if (!guardedThis || !guardedSource) bResult = false;
    if (bResult && (innerType != FT_UNKNOWN)) {
        XBinary *pBinary = XFormats::createClass(innerType, pContext->pDecodedDevice);
        pContext->pInnerArchive = dynamic_cast<XArchive *>(pBinary);
        if (!pContext->pInnerArchive) delete pBinary;
        if (pContext->pInnerArchive) {
            bResult = pContext->pInnerArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct);
            if (!bResult) {
                pContext->pInnerArchive->finishUnpack(&pContext->innerState, nullptr);
                delete pContext->pInnerArchive;
                pContext->pInnerArchive = nullptr;
                pContext->innerState = UNPACK_STATE();
                bResult = guardedThis && guardedSource && XBinary::isPdStructNotCanceled(pPdStruct);
            }
        }
    }
    if (bResult && !pContext->pInnerArchive) {
        pContext->bDirectPayload = true;
    }
    qint64 nOuterSize = -1;
    if (guardedThis && guardedSource && bResult) {
        nOuterSize = guardedSource->size();
    }
    if (!guardedThis || !guardedSource || !bResult || (nOuterSize < 0) || pContext->listBlocks.isEmpty() ||
        (!pContext->bDirectPayload && ((pContext->innerState.nNumberOfRecords < 0) || (pContext->innerState.nCurrentIndex != 0) ||
                                       (pContext->innerState.nCurrentIndex > pContext->innerState.nNumberOfRecords)))) {
        delete pContext;
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = pContext->bDirectPayload ? mapProperties : pContext->innerState.mapUnpackProperties;
    pState->mapArchiveProperties = pContext->bDirectPayload ? QMap<FPART_PROP, QVariant>() : pContext->innerState.mapArchiveProperties;
    const qint32 nRecords = pContext->bDirectPayload ? pContext->listBlocks.count() : pContext->innerState.nNumberOfRecords;
    pState->nCurrentOffset = nRecords == 0 ? nOuterSize : 0;
    pState->nTotalSize = nOuterSize;
    pState->nCurrentIndex = pContext->bDirectPayload ? 0 : pContext->innerState.nCurrentIndex;
    pState->nNumberOfRecords = nRecords;
    pState->pContext = pContext;
    bResult = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !bResult) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XUU::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XUU> guardedThis(this);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nTotalSize < 0))
        return ARCHIVERECORD();
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pDecodedDevice || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) {
        if (!pContext || !pContext->bDirectPayload || !pContext->pDecodedDevice || pContext->pInnerArchive ||
            (pState->nNumberOfRecords != pContext->listBlocks.count()) || (pState->nCurrentIndex >= pContext->listBlocks.count())) {
            return ARCHIVERECORD();
        }
        const UU_BLOCK &block = pContext->listBlocks.at(pState->nCurrentIndex);
        const qint64 nDecodedSize = block.baDecoded.size();
        if ((nDecodedSize < 0) || block.sDeclaredName.isEmpty()) {
            return ARCHIVERECORD();
        }
        ARCHIVERECORD record = {};
        record.nStreamOffset = 0;
        record.nStreamSize = nDecodedSize;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, block.sDeclaredName);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nDecodedSize);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nDecodedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        if (!XBinary::markArchiveStreamRecord(&record, pState->nCurrentIndex)) {
            return ARCHIVERECORD();
        }
        return record;
    }
    if (!pContext->pInnerArchive || pContext->bDirectPayload) {
        return ARCHIVERECORD();
    }
    ARCHIVERECORD record = pContext->pInnerArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct) || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords) ||
        record.mapProperties.isEmpty() || !XBinary::markArchiveStreamRecord(&record, pState->nCurrentIndex)) {
        return ARCHIVERECORD();
    }
    return record;
}

bool XUU::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XUU> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) || devicesAlias(getDevice(), pDevice)) return false;
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pDecodedDevice || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pContext->bDirectPayload ? (pContext->pInnerArchive || (pState->nNumberOfRecords != pContext->listBlocks.count()))
                                  : (!pContext->pInnerArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
                                     (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))))
        return false;
    if (pContext->bDirectPayload) {
        const UU_BLOCK &block = pContext->listBlocks.at(pState->nCurrentIndex);
        const qint64 nDecodedSize = block.baDecoded.size();
        if ((nDecodedSize < 0) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nDecodedSize)) {
            return false;
        }
        // The direct-payload route bypasses the base decode chain, so it
        // must charge the operation budget itself: one entry, nDecodedSize
        // produced bytes (publishUnpackOutput never debits the copy).
        if (pState->spOutputBudget) {
            if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, block.sDeclaredName)) {
                if (pState->spOutputBudget->isEnforcing()) {
                    XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                    return false;
                }
                XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
            }
            if (!pState->spOutputBudget->debit(nDecodedSize)) {
                if (pState->spOutputBudget->isEnforcing()) {
                    XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                    return false;
                }
                XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
            }
        }
        QBuffer decodedDevice;
        decodedDevice.setData(block.baDecoded);
        if (!decodedDevice.open(QIODevice::ReadOnly)) return false;
        const bool bResult = publishUnpackOutput(&decodedDevice, pDevice, pState, pPdStruct);
        // publishUnpackOutput() finishes with the outer source-authentication
        // check. Do not perform another callback-bearing source read after the
        // caller-owned destination has been committed.
        if (!guardedThis || !bResult) return false;
        pState->nCurrentOffset = 0;
        return true;
    }
    // Delegate to the inner archive; it performs its own entry and
    // produced-byte accounting against the shared operation budget.
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    const bool bResult = pContext->pInnerArchive->unpackCurrent(&pContext->innerState, pDevice, pPdStruct);
    if (!guardedThis || !bResult || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords) || !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    pState->nCurrentOffset = 0;
    return true;
}

bool XUU::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XUU> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pContext->bDirectPayload ? (pContext->pInnerArchive || (pState->nNumberOfRecords != pContext->listBlocks.count()))
                                  : (!pContext->pInnerArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
                                     (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))))
        return false;
    if (pContext->bDirectPayload) {
        ++pState->nCurrentIndex;
        if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
            pState->nCurrentOffset = pState->nTotalSize;
            return false;
        }
        pState->nCurrentOffset = 0;
        return true;
    }
    const qint32 nPreviousIndex = pState->nCurrentIndex;
    const bool bResult = pContext->pInnerArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) return false;
    if (bResult) {
        if ((pContext->innerState.nCurrentIndex != (nPreviousIndex + 1)) || (pContext->innerState.nCurrentIndex >= pState->nNumberOfRecords)) return false;
        pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
        pState->nCurrentOffset = 0;
        return true;
    }
    if ((nPreviousIndex + 1) != pState->nNumberOfRecords ||
        ((pContext->innerState.nCurrentIndex != nPreviousIndex) && (pContext->innerState.nCurrentIndex != pState->nNumberOfRecords)))
        return false;
    pState->nCurrentIndex = pState->nNumberOfRecords;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XUU::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
