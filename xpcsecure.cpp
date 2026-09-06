/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpcsecure.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xpcsecuredecoder.h"

namespace {
const qint64 PCS_HEADER_SIZE = 68;
const quint32 PCS_SIG_PCT5 = 0x35544350U;  // "PCT5"
const quint32 PCS_SIG_PCT6 = 0x36544350U;  // "PCT6"
const quint32 PCS_SIG_PCT7 = 0x37544350U;  // "PCT7"
const quint32 PCS_SIG_AFOS = 0x536f6641U;  // "AfoS"
const qint64 PCS_MAX_UNCOMPRESSED_SIZE = 0x20000000;  // 512 MB sanity cap
// The key that unwraps a user-password verifier before it is used as the file
// key; U3 keeps it at 0x009f6e58 as a raw 8-byte little-endian quad word.
const quint8 PCS_VERIFIER_KEY[8] = {0xc1, 0xb0, 0xdc, 0x21,
                                    0xb0, 0x96, 0x4e, 0x7f};

bool pcsIsKnownSignature(quint32 nSignature)
{
    return (nSignature == PCS_SIG_PCT5) || (nSignature == PCS_SIG_PCT6) ||
           (nSignature == PCS_SIG_PCT7) || (nSignature == PCS_SIG_AFOS);
}

QString pcsSignatureToString(quint32 nSignature)
{
    if (nSignature == PCS_SIG_PCT5) return QStringLiteral("PCT5");
    if (nSignature == PCS_SIG_PCT6) return QStringLiteral("PCT6");
    if (nSignature == PCS_SIG_PCT7) return QStringLiteral("PCT7");
    if (nSignature == PCS_SIG_AFOS) return QStringLiteral("AfoS");
    return QString();
}

// What the member advertises.  When no key opened the header the member is still
// listed, but the string has to say WHY it cannot be produced: a non-zero
// verifier is a real user password, a zero verifier is a container variant this
// reader does not implement.  Neither ever reaches a decoder - the member is
// published as HANDLE_METHOD_UNKNOWN so the unpack path refuses rather than
// writing ciphertext out as if it were plaintext.
QString pcsMethodString(bool bKeyFound, bool bCompressed, qint32 nRounds, bool bUserPassword)
{
    if (bKeyFound) {
        return bCompressed
                   ? QStringLiteral("DES (%1 rounds) + LZW").arg(nRounds)
                   : QStringLiteral("DES (%1 rounds)").arg(nRounds);
    }
    return bUserPassword ? QStringLiteral("Encrypted (user password)")
                         : QStringLiteral("Encrypted (unsupported key schedule)");
}

quint64 pcsKeyFromBytes(const quint8 *pBytes)
{
    quint64 nResult = 0;
    for (qint32 i = 0; i < 8; ++i) {
        nResult = (nResult << 8) | pBytes[i];
    }
    return nResult;
}
}  // namespace

XPCSecure::XPCSecure(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPCSecure::~XPCSecure()
{
}

QString XPCSecure::memberName(const QByteArray &baExtension)
{
    QPointer<QIODevice> guardedSource(getDevice());
    QString sPath;
    if (guardedSource) {
        sPath = XBinary::getDeviceFileName(guardedSource.data());
    }
    QString sBase;
    if (!sPath.isEmpty()) sBase = QFileInfo(sPath).completeBaseName();
    if (sBase.isEmpty()) sBase = QStringLiteral("pcsecure_data");

    // The stored extension already carries its dot; drop the NUL/space padding
    // and any byte that could escape the output directory.
    QString sExtension;
    for (qint32 i = 0; i < baExtension.size(); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baExtension.at(i));
        if ((nCharacter == 0) || (nCharacter == 0x20)) break;
        if ((nCharacter < 0x21) || (nCharacter > 0x7e)) break;
        if ((nCharacter == '/') || (nCharacter == '\\') ||
            (nCharacter == ':')) {
            break;
        }
        sExtension.append(QLatin1Char(static_cast<char>(nCharacter)));
    }
    if (sExtension == QStringLiteral(".")) sExtension.clear();
    return XBinary::fixFileName(sBase + sExtension);
}

bool XPCSecure::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPCSecure> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // U3 requires strictly more than 0x44 bytes: a header with no payload is
    // not a PCSECURE file.
    if (context.nInputSize <= PCS_HEADER_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, PCS_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != PCS_HEADER_SIZE)) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    context.nSignature = qFromLittleEndian<quint32>(pHeader);
    if (!pcsIsKnownSignature(context.nSignature)) return false;

    context.nDataOffset = PCS_HEADER_SIZE;
    context.nDataSize = context.nInputSize - PCS_HEADER_SIZE;
    context.nUncompressedSize = context.nDataSize;
    context.nCompressedSize = context.nDataSize;
    context.sFileName = memberName(QByteArray());
    if (!guardedThis || !guardedSource) return false;

    // Key search, in U3's order: a non-zero verifier first (it is itself
    // encrypted with a fixed key and the result IS the file key), then the
    // four built-in product keys.  PCT7 additionally allows a 3-round header.
    QList<quint64> listKeys;
    const quint64 nVerifier = qFromLittleEndian<quint64>(pHeader + 60);
    context.bUserPassword = (nVerifier != 0);
    if (nVerifier != 0) {
        // The verifier is itself one DES block, encrypted with a fixed key at
        // 16 rounds; its plaintext IS the file key.  NOTE: no sample in the
        // corpus carries a non-zero verifier, so this branch is derived from
        // U3's code and is NOT corpus-verified.
        const QByteArray baVerifier(reinterpret_cast<const char *>(pHeader + 60),
                                    8);
        QByteArray baUnwrapProperty(
            reinterpret_cast<const char *>(PCS_VERIFIER_KEY), 8);
        baUnwrapProperty.append(static_cast<char>(16));  // rounds
        baUnwrapProperty.append(static_cast<char>(0));   // flags: not packed
        baUnwrapProperty.append(4, '\0');                // compressed size
        QByteArray baDerived;
        QByteArray baKeyMaterial = baVerifier;
        if (XPCSecureDecoder::decode(baVerifier, 8, baUnwrapProperty, &baDerived,
                                     pPdStruct) &&
            (baDerived.size() == 8)) {
            baKeyMaterial = baDerived;
        }
        listKeys.append(pcsKeyFromBytes(
            reinterpret_cast<const quint8 *>(baKeyMaterial.constData())));
    }
    const quint64 *pBuiltin = XPCSecureDecoder::builtinKeys();
    for (qint32 i = 0; i < XPCSecureDecoder::builtinKeyCount(); ++i) {
        listKeys.append(pBuiltin[i]);
    }

    QByteArray baPlainHeader;
    quint64 nKey = 0;
    for (qint32 i = 0; (i < listKeys.size()) && !context.bKeyFound; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (XPCSecureDecoder::tryHeader(baHeader, listKeys.at(i), 16,
                                        &baPlainHeader)) {
            context.bKeyFound = true;
            nKey = listKeys.at(i);
        } else if ((context.nSignature == PCS_SIG_PCT7) &&
                   XPCSecureDecoder::tryHeader(baHeader, listKeys.at(i), 3,
                                               &baPlainHeader)) {
            context.bKeyFound = true;
            nKey = listKeys.at(i);
        }
    }

    if (context.bKeyFound) {
        const uchar *pPlain =
            reinterpret_cast<const uchar *>(baPlainHeader.constData());
        context.nFlags = qFromLittleEndian<quint32>(pPlain + 8);
        context.nRounds =
            static_cast<qint32>(qFromLittleEndian<quint16>(pPlain + 0x0c));
        context.bCompressed = ((context.nFlags & 0x01U) != 0);
        const quint32 nUncompressed = qFromLittleEndian<quint32>(pPlain + 0x18);
        const quint32 nCompressed = qFromLittleEndian<quint32>(pPlain + 0x20);
        context.nDosTime = qFromLittleEndian<quint32>(pPlain + 0x38);
        if ((nUncompressed == 0) ||
            (static_cast<qint64>(nUncompressed) >
             PCS_MAX_UNCOMPRESSED_SIZE)) {
            return false;
        }
        if ((nCompressed == 0) ||
            (static_cast<qint64>(nCompressed) > context.nDataSize)) {
            return false;
        }
        context.nUncompressedSize = nUncompressed;
        context.nCompressedSize = nCompressed;
        context.sFileName = memberName(baPlainHeader.mid(0x12, 4));
        if (!guardedThis || !guardedSource) return false;

        QByteArray baProperty;
        for (qint32 i = 0; i < 8; ++i) {
            baProperty.append(
                static_cast<char>(static_cast<quint8>(nKey >> (56 - i * 8))));
        }
        baProperty.append(static_cast<char>(static_cast<quint8>(context.nRounds)));
        baProperty.append(static_cast<char>(static_cast<quint8>(context.nFlags & 0xffU)));
        char szSize[4];
        qToLittleEndian<quint32>(nCompressed, reinterpret_cast<uchar *>(szSize));
        baProperty.append(szSize, 4);
        context.baProperty = baProperty;
    }

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XPCSecure::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XPCSecure::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPCSecure archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPCSecure::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPCSecure(pDevice);
}

QList<QString> XPCSecure::getSearchSignatures()
{
    return {QStringLiteral("'PCT5'"), QStringLiteral("'PCT6'"),
            QStringLiteral("'PCT7'"), QStringLiteral("'AfoS'")};
}

XBinary::FT XPCSecure::getFileType()
{
    return FT_PC_SECURE;
}

XBinary::MODE XPCSecure::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPCSecure::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPCSecure::getArch()
{
    return QString();
}

QString XPCSecure::getFileFormatExt()
{
    return QString();
}

QString XPCSecure::getFileFormatExtsString()
{
    return QStringLiteral("PC Tools PCSECURE protected file");
}

QString XPCSecure::getMIMEString()
{
    return QStringLiteral("application/x-pcsecure");
}

QString XPCSecure::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return pcsSignatureToString(context.nSignature);
}

qint64 XPCSecure::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XPCSecure::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPCSecure::getMemoryMap(MAPMODE mapMode,
                                             PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XPCSecure::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XPCSecure::getFileParts(quint32 nFileParts, qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PCS_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        // The whole payload is handed over: the cipher runs over every whole
        // 8-byte block and only then is the stream cut to the compressed size.
        part.nFileSize = context.nDataSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(
            FPART_PROP_HANDLEMETHOD,
            context.bKeyFound ? HANDLE_METHOD_PC_SECURE : HANDLE_METHOD_UNKNOWN);
        part.mapProperties.insert(
            FPART_PROP_REPORTEDMETHOD,
            pcsMethodString(context.bKeyFound, context.bCompressed,
                            context.nRounds, context.bUserPassword));
        part.mapProperties.insert(FPART_PROP_ENCRYPTED, !context.bKeyFound);
        if (context.bKeyFound) {
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                      context.baProperty);
            if (context.nSignature == PCS_SIG_PCT7) {
                const QDateTime dtMTime = dosDateTimeToQDateTime(
                    static_cast<quint16>(context.nDosTime >> 16),
                    static_cast<quint16>(context.nDosTime & 0xffffU));
                if (dtMTime.isValid()) {
                    part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
                }
            }
        }
        result.append(part);
    }
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XPCSecure::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPCSecure::initUnpack(UNPACK_STATE *pState,
                           const QMap<UNPACK_PROP, QVariant> &mapProperties,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XPCSecure> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("PC Tools PCSECURE protected file; DES-encrypted, optionally LZW"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XPCSecure::infoCurrent(UNPACK_STATE *pState,
                                              PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        pContext->bKeyFound ? HANDLE_METHOD_PC_SECURE : HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        pcsMethodString(pContext->bKeyFound, pContext->bCompressed,
                        pContext->nRounds, pContext->bUserPassword));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, !pContext->bKeyFound);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (pContext->bKeyFound) {
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                    pContext->baProperty);
        if (pContext->nSignature == PCS_SIG_PCT7) {
            const QDateTime dtMTime = dosDateTimeToQDateTime(
                static_cast<quint16>(pContext->nDosTime >> 16),
                static_cast<quint16>(pContext->nDosTime & 0xffffU));
            if (dtMTime.isValid()) {
                result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            }
        }
    }
    return result;
}

bool XPCSecure::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nInputSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XPCSecure::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
