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
const char UU_DEPTH_PROPERTY[] = "_xfileunpacker_filter_depth";

bool appendBounded(QByteArray *pOutput, char value)
{
    if (!pOutput || (pOutput->size() >= UU_MAX_DECODED_SIZE)) return false;
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

XUU::UU_UNPACK_CONTEXT::UU_UNPACK_CONTEXT()
    : pDecodedDevice(nullptr), pInnerArchive(nullptr), innerState()
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

    if ((body.size() < (nPrefix + 5)) || (body.at(nPrefix) < '0') || (body.at(nPrefix) > '7') ||
        (body.at(nPrefix + 1) < '0') || (body.at(nPrefix + 1) > '7') ||
        (body.at(nPrefix + 2) < '0') || (body.at(nPrefix + 2) > '7') ||
        (body.at(nPrefix + 3) != ' ')) {
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

bool XUU::decodeTransport(QByteArray *pOutput, QString *pDeclaredName, PDSTRUCT *pPdStruct)
{
    QPointer<XUU> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!pOutput || !pDeclaredName || !guardedDevice || guardedDevice->isSequential() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nOriginalPosition = guardedDevice->pos();
    if ((nOriginalPosition < 0) || !guardedDevice->seek(0)) return false;

    pOutput->clear();
    pDeclaredName->clear();
    bool bHeaderFound = false;
    bool bBase64 = false;
    qint64 nScanned = 0;

    while (!guardedDevice->atEnd() && (nScanned <= UU_MAX_PREAMBLE) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray line = guardedDevice->readLine(UU_MAX_LINE + 2);
        if (!guardedThis || !guardedDevice || line.isEmpty() ||
            ((line.size() > UU_MAX_LINE) && !line.endsWith('\n'))) {
            guardedDevice->seek(nOriginalPosition);
            return false;
        }
        nScanned += line.size();
        if (parseHeader(line, &bBase64, pDeclaredName)) {
            bHeaderFound = true;
            break;
        }
    }

    bool bComplete = false;
    if (bHeaderFound && bBase64) {
        QByteArray carry;
        bool bPaddingSeen = false;
        while (!guardedDevice->atEnd() && XBinary::isPdStructNotCanceled(pPdStruct)) {
            QByteArray line = guardedDevice->readLine(UU_MAX_LINE + 2);
            if (!guardedThis || !guardedDevice || line.isEmpty() ||
                ((line.size() > UU_MAX_LINE) && !line.endsWith('\n'))) {
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
                if ((v0 < 0) || (v1 < 0) || (v2 < 0) || (v3 < 0) ||
                    ((c2 == '=') && (c3 != '='))) {
                    carry.clear();
                    bPaddingSeen = true;
                    bComplete = false;
                    goto decode_finished;
                }
                if (!appendBounded(pOutput, static_cast<char>((v0 << 2) | (v1 >> 4)))) goto decode_finished;
                if (c2 != '=') {
                    if (!appendBounded(pOutput, static_cast<char>((v1 << 4) | (v2 >> 2)))) goto decode_finished;
                    if (c3 != '=') {
                        if (!appendBounded(pOutput, static_cast<char>((v2 << 6) | v3))) goto decode_finished;
                    }
                }
                if ((c2 == '=') || (c3 == '=')) bPaddingSeen = true;
            }
        }
    } else if (bHeaderFound) {
        bool bSawZeroLine = false;
        while (!guardedDevice->atEnd() && XBinary::isPdStructNotCanceled(pPdStruct)) {
            QByteArray line = guardedDevice->readLine(UU_MAX_LINE + 2);
            if (!guardedThis || !guardedDevice || line.isEmpty() ||
                ((line.size() > UU_MAX_LINE) && !line.endsWith('\n'))) {
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
                const char decoded[3] = {
                    static_cast<char>((values[0] << 2) | (values[1] >> 4)),
                    static_cast<char>((values[1] << 4) | (values[2] >> 2)),
                    static_cast<char>((values[2] << 6) | values[3])};
                for (qint32 j = 0; (j < 3) && (nWritten < nDecoded); j++, nWritten++) {
                    if (!appendBounded(pOutput, decoded[j])) goto decode_finished;
                }
            }
        }
    }

decode_finished:
    if (guardedDevice) guardedDevice->seek(nOriginalPosition);
    if (!guardedThis || !guardedDevice || !bComplete || pOutput->isEmpty() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pOutput->clear();
        pDeclaredName->clear();
        return false;
    }
    return true;
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

XBinary::FT XUU::getFileType() { return FT_UU; }
XBinary::MODE XUU::getMode() { return MODE_DATA; }
QString XUU::getMIMEString() { return "application/x-uuencode"; }
QString XUU::getFileFormatExt() { return "uu"; }
QString XUU::getFileFormatExtsString() { return "UUencoded archive (*.uu *.uue)"; }

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

    QByteArray decoded;
    QString declaredName;
    bool bResult = decodeTransport(&decoded, &declaredName, pPdStruct);
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
        pContext->pDecodedDevice->setData(decoded);
        pContext->pDecodedDevice->setProperty(UU_DEPTH_PROPERTY, nDepth + 1);
        bResult = pContext->pDecodedDevice->open(QIODevice::ReadOnly);
    } else {
        bResult = false;
    }

    FT innerType = FT_UNKNOWN;
    if (bResult) innerType = XFormats::getPrefFileType(pContext->pDecodedDevice, FT_FLAG_ARCHIVES, pPdStruct);
    if (!guardedThis || !guardedSource || (innerType == FT_UNKNOWN)) bResult = false;
    if (bResult) {
        XBinary *pBinary = XFormats::createClass(innerType, pContext->pDecodedDevice);
        pContext->pInnerArchive = dynamic_cast<XArchive *>(pBinary);
        if (!pContext->pInnerArchive) delete pBinary;
        bResult = pContext->pInnerArchive != nullptr;
    }
    if (bResult) bResult = pContext->pInnerArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct);
    qint64 nOuterSize = -1;
    if (guardedThis && guardedSource && bResult) {
        nOuterSize = guardedSource->size();
    }
    if (!guardedThis || !guardedSource || !bResult || (nOuterSize < 0) ||
        (pContext->innerState.nNumberOfRecords < 0) ||
        (pContext->innerState.nCurrentIndex != 0) ||
        (pContext->innerState.nCurrentIndex >
         pContext->innerState.nNumberOfRecords)) {
        delete pContext;
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = pContext->innerState.mapUnpackProperties;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->nCurrentOffset = pContext->innerState.nNumberOfRecords == 0
        ? nOuterSize : 0;
    pState->nTotalSize = nOuterSize;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
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
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pState->nTotalSize < 0)) return ARCHIVERECORD();
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pInnerArchive || !pContext->pDecodedDevice ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords !=
         pState->nNumberOfRecords)) return ARCHIVERECORD();
    ARCHIVERECORD record = pContext->pInnerArchive->infoCurrent(
        &pContext->innerState, pPdStruct);
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords !=
         pState->nNumberOfRecords) || record.mapProperties.isEmpty() ||
        !XBinary::markArchiveStreamRecord(
            &record, pState->nCurrentIndex)) {
        return ARCHIVERECORD();
    }
    return record;
}

bool XUU::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XUU> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !pDevice ||
        !isUnpackSourceCurrent(pState, pPdStruct) || devicesAlias(getDevice(), pDevice)) return false;
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pInnerArchive || !pContext->pDecodedDevice ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords !=
         pState->nNumberOfRecords)) return false;
    const bool bResult = pContext->pInnerArchive->unpackCurrent(&pContext->innerState, pDevice, pPdStruct);
    if (!guardedThis || !bResult ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords !=
         pState->nNumberOfRecords) ||
        !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    pState->nCurrentOffset = 0;
    return true;
}

bool XUU::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XUU> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    UU_UNPACK_CONTEXT *pContext = static_cast<UU_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pInnerArchive ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords !=
         pState->nNumberOfRecords)) return false;
    const qint32 nPreviousIndex = pState->nCurrentIndex;
    const bool bResult = pContext->pInnerArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pContext->innerState.nNumberOfRecords !=
         pState->nNumberOfRecords)) return false;
    if (bResult) {
        if ((pContext->innerState.nCurrentIndex !=
             (nPreviousIndex + 1)) ||
            (pContext->innerState.nCurrentIndex >=
             pState->nNumberOfRecords)) return false;
        pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
        pState->nCurrentOffset = 0;
        return true;
    }
    if ((nPreviousIndex + 1) != pState->nNumberOfRecords ||
        ((pContext->innerState.nCurrentIndex != nPreviousIndex) &&
         (pContext->innerState.nCurrentIndex !=
          pState->nNumberOfRecords))) return false;
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
