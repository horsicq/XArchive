/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xmwavez.h"

#include "Algos/xcompressdecoder.h"

#include <QtEndian>

#include <memory>
#include <new>

namespace {
// +0x00 magic | +0x02 checksum | +0x04 DOS date | +0x06 DOS time |
// +0x08 name[15] | +0x17 compress flags | +0x18 LZW codes.
const qint64 MWAVEZ_HEADER_SIZE = 0x17;
const qint64 MWAVEZ_NAME_OFFSET = 0x08;
// 0x17 - 0x08.  The reader is handed the FIELD size, not size - 1: a name that
// filled the field would put its terminator in the last byte.
const qint32 MWAVEZ_NAME_FIELD_SIZE = 0x0f;
const qint32 MWAVEZ_NAME_MAX = 12;  // 8.3
// Header + flags byte + at least the two bytes of the first nine-bit code.
const qint64 MWAVEZ_MIN_SIZE = MWAVEZ_HEADER_SIZE + 3;
const quint8 MWAVEZ_MAGIC_0 = 0x1fU;
const quint8 MWAVEZ_MAGIC_1 = 0x9dU;
const qint64 MWAVEZ_MAGIC_SIZE = 2;
const quint8 MWAVEZ_FLAGS_RESERVED = 0x60U;
const quint8 MWAVEZ_FLAGS_BLOCKMODE = 0x80U;
const quint8 MWAVEZ_FLAGS_MAXBITS = 0x1fU;
const qint32 MWAVEZ_MINBITS = 9;
const qint32 MWAVEZ_MAXBITS = 16;

const char g_mwaveZMagic[MWAVEZ_MAGIC_SIZE] = {static_cast<char>(MWAVEZ_MAGIC_0),
                                               static_cast<char>(MWAVEZ_MAGIC_1)};

// Read-only view that presents "1F 9D" followed by the container's payload from
// +0x17 on, i.e. a bit-exact ordinary .Z stream.  XCompressDecoder hard-requires
// the magic at its entry point and the two bytes are not adjacent to the flags
// byte in an Mwave container, so this view is what makes reusing the shared
// codec possible instead of forking a second LZW implementation.
class MwaveZStream : public QIODevice {
public:
    MwaveZStream(QIODevice *pDevice, qint64 nPayloadOffset, qint64 nPayloadSize)
        : m_pDevice(pDevice), m_nPayloadOffset(nPayloadOffset),
          m_nPayloadSize(nPayloadSize)
    {
    }

    qint64 size() const override
    {
        return MWAVEZ_MAGIC_SIZE + m_nPayloadSize;
    }

    bool isSequential() const override
    {
        return false;
    }

    bool open(OpenMode mode) override
    {
        QIODevice *guardedDevice = m_pDevice;
        if ((m_nPayloadOffset < 0) || (m_nPayloadSize < 0) ||
            (mode != QIODevice::ReadOnly)) {
            return false;
        }
        const qint64 nDeviceSize = guardedDevice->size();
        if ((nDeviceSize < 0) ||
            (m_nPayloadOffset > nDeviceSize) ||
            (m_nPayloadSize > nDeviceSize - m_nPayloadOffset)) {
            return false;
        }
        const bool bOpen = guardedDevice->isOpen();
        if (!bOpen) return false;
        const bool bReadable = guardedDevice->isReadable();
        if (!bReadable) return false;
        const bool bSequential = guardedDevice->isSequential();
        if (bSequential) return false;
        return QIODevice::open(mode) && QIODevice::seek(0);
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        QIODevice *guardedDevice = m_pDevice;
        const qint64 nTotal = size();
        const qint64 nPosition = pos();
        if (!isOpen() || !isReadable() || (nMaxSize < 0) ||
            ((nMaxSize > 0) && !pData) || (nPosition < 0) ||
            (nPosition > nTotal)) {
            return -1;
        }
        nMaxSize = qMin(nMaxSize, nTotal - nPosition);

        qint64 nDone = 0;
        while ((nDone < nMaxSize) && ((nPosition + nDone) < MWAVEZ_MAGIC_SIZE)) {
            pData[nDone] = g_mwaveZMagic[nPosition + nDone];
            ++nDone;
        }

        if (nDone < nMaxSize) {
            // The backing device is shared with the parser, so its cursor
            // cannot be assumed to still match this view's logical position.
            const qint64 nAbsolute =
                m_nPayloadOffset + (nPosition + nDone) - MWAVEZ_MAGIC_SIZE;
            const qint64 nBackingPosition = guardedDevice->pos();
            if (!guardedDevice) return -1;
            if (nBackingPosition != nAbsolute) {
                const bool bPositioned = guardedDevice->seek(nAbsolute);
                if (!bPositioned) return -1;
            }
            const qint64 nRead =
                guardedDevice->read(pData + nDone, nMaxSize - nDone);
            if (!guardedDevice) return -1;
            if (nRead < 0) return (nDone > 0) ? nDone : -1;
            nDone += nRead;
        }

        return nDone;
    }

    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }

private:
    QIODevice *m_pDevice = nullptr;
    qint64 m_nPayloadOffset;
    qint64 m_nPayloadSize;
};

// Sizing pass sink: the container stores no unpacked length, so the only way to
// learn it is to run the codec once and count.
class MwaveZDiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override
    {
        return -1;
    }
    qint64 writeData(const char *, qint64 nSize) override
    {
        return nSize;
    }
};

bool mwaveZIsNameChar(quint8 nCharacter)
{
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    // The DOS 8.3 punctuation set, minus the wildcard and path characters.
    static const char *pAllowed = "$%'-_@~`!(){}^#&";
    for (const char *p = pAllowed; *p; ++p) {
        if (static_cast<quint8>(*p) == nCharacter) return true;
    }
    return false;
}
}  // namespace

XBinary::XCONVERT _TABLE_XMwaveZ_STRUCTID[] = {
    {XMwaveZ::TYPE_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XMwaveZ::TYPE_Z, "Z", QString("IBM Mwave packed file")}};

XMwaveZ::XMwaveZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMwaveZ::~XMwaveZ()
{
}

bool XMwaveZ::isValidDosName(const QByteArray &baName)
{
    const qint32 nSize = baName.size();
    if ((nSize < 1) || (nSize > MWAVEZ_NAME_MAX)) return false;

    qint32 nDotPosition = -1;
    for (qint32 i = 0; i < nSize; ++i) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (nCharacter == '.') {
            // Exactly one dot at most, never leading, never trailing.
            if ((nDotPosition != -1) || (i == 0) || (i == nSize - 1)) {
                return false;
            }
            nDotPosition = i;
            continue;
        }
        if (!mwaveZIsNameChar(nCharacter)) return false;
    }

    const qint32 nBaseSize = (nDotPosition == -1) ? nSize : nDotPosition;
    const qint32 nExtSize = (nDotPosition == -1) ? 0 : (nSize - nDotPosition - 1);

    return (nBaseSize >= 1) && (nBaseSize <= 8) && (nExtSize <= 3);
}

bool XMwaveZ::readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct)
{
    if (!pHeader || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const bool bSequential = guardedSource->isSequential();
    if (bSequential) return false;

    const qint64 nInputSize = getSize();
    if ((nInputSize < MWAVEZ_MIN_SIZE)) {
        return false;
    }

    // One read covers the whole fixed header plus the flags byte and the two
    // bytes of the first LZW code.
    const QByteArray baHeader =
        read_array_process(0, MWAVEZ_HEADER_SIZE + 3, pPdStruct);
    if ((baHeader.size() != MWAVEZ_HEADER_SIZE + 3)) {
        return false;
    }
    const uchar *pHeaderBytes =
        reinterpret_cast<const uchar *>(baHeader.constData());

    if ((pHeaderBytes[0] != MWAVEZ_MAGIC_0) ||
        (pHeaderBytes[1] != MWAVEZ_MAGIC_1)) {
        return false;
    }

    // The stored 8.3 name is the strongest discriminator the header offers: in
    // a plain Unix compress .Z these fifteen bytes are LZW code data.
    const QByteArray baNameField =
        baHeader.mid(static_cast<qint32>(MWAVEZ_NAME_OFFSET),
                     MWAVEZ_NAME_FIELD_SIZE);
    if (baNameField.size() != MWAVEZ_NAME_FIELD_SIZE) return false;
    const qint32 nTerminator = baNameField.indexOf('\0');
    if ((nTerminator < 1) || (nTerminator > MWAVEZ_NAME_MAX)) return false;
    const QByteArray baName = baNameField.left(nTerminator);
    if (!isValidDosName(baName)) return false;

    const quint8 nFlags = pHeaderBytes[MWAVEZ_HEADER_SIZE];
    if (nFlags & MWAVEZ_FLAGS_RESERVED) return false;
    // Every known writer is compress 4.x in block mode; the reserved-bit test
    // alone would let far too much through.
    if (!(nFlags & MWAVEZ_FLAGS_BLOCKMODE)) return false;
    const qint32 nMaxBits = static_cast<qint32>(nFlags & MWAVEZ_FLAGS_MAXBITS);
    if ((nMaxBits < MWAVEZ_MINBITS) || (nMaxBits > MWAVEZ_MAXBITS)) return false;

    const quint16 nDosDate = qFromLittleEndian<quint16>(pHeaderBytes + 4);
    const quint16 nDosTime = qFromLittleEndian<quint16>(pHeaderBytes + 6);
    const quint32 nMonth = (nDosDate >> 5) & 0x0fU;
    const quint32 nDay = nDosDate & 0x1fU;
    const quint32 nHour = nDosTime >> 11;
    const quint32 nMinute = (nDosTime >> 5) & 0x3fU;
    if ((nMonth < 1) || (nMonth > 12) || (nDay < 1) || (nDay > 31) ||
        (nHour > 23) || (nMinute > 59)) {
        return false;
    }

    // Bounded trial decode of exactly one code: the dictionary is pristine at
    // the head of a compress stream, so the first nine-bit LSB-first code can
    // only be a literal 0..255.  That is the high bit of the second code byte.
    const quint32 nFirstCode =
        static_cast<quint32>(pHeaderBytes[MWAVEZ_HEADER_SIZE + 1]) |
        ((static_cast<quint32>(pHeaderBytes[MWAVEZ_HEADER_SIZE + 2]) & 1U) << 8);
    if (nFirstCode >= 256) return false;

    pHeader->nChecksum = qFromLittleEndian<quint16>(pHeaderBytes + 2);
    pHeader->nDosDate = nDosDate;
    pHeader->nDosTime = nDosTime;
    pHeader->nFlags = nFlags;
    pHeader->sFileName = QString::fromLatin1(baName);

    return isPdStructNotCanceled(pPdStruct);
}

bool XMwaveZ::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QIODevice *guardedSource = getDevice();

    CONTEXT context = {};
    if (!readHeader(&context.header, pPdStruct)) {
        return false;
    }

    context.nInputSize = getSize();
    if ((context.nInputSize < MWAVEZ_MIN_SIZE)) {
        return false;
    }
    context.nStreamOffset = MWAVEZ_HEADER_SIZE;

    // No stored unpacked length: run the shared codec once over the whole
    // payload and take both sizes from the counters.
    MwaveZStream stream(guardedSource, MWAVEZ_HEADER_SIZE,
                        context.nInputSize - MWAVEZ_HEADER_SIZE);
    if (!stream.open(QIODevice::ReadOnly)) return false;

    MwaveZDiscardDevice output;
    bool bResult = false;
    if (output.open(QIODevice::WriteOnly)) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.pDeviceInput = &stream;
        decompressState.pDeviceOutput = &output;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = stream.size();
        decompressState.nProcessedOffset = 0;
        decompressState.nProcessedLimit = -1;

        bResult = XCompressDecoder::decompress(&decompressState, pPdStruct);
        if (!guardedSource) return false;

        if (bResult) {
            // nCountInput counts the two synthetic magic bytes as well.
            context.nCompressedSize =
                (decompressState.nCountInput >= MWAVEZ_MAGIC_SIZE)
                    ? (decompressState.nCountInput - MWAVEZ_MAGIC_SIZE)
                    : 0;
            context.nUncompressedSize = decompressState.nCountOutput;
        }

        output.close();
    }
    stream.close();

    if (!bResult) return false;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XMwaveZ::isValid(PDSTRUCT *pPdStruct)
{
    // Detection probes a device the caller still owns: read_array_process
    // moves the cursor, so snapshot it and put it back.
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();

    HEADER header = {};
    const bool bResult = readHeader(&header, pPdStruct);

    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XMwaveZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMwaveZ archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMwaveZ::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XMwaveZ(pDevice);
}

QList<QString> XMwaveZ::getSearchSignatures()
{
    QList<QString> listResult;

    // Shared with the plain .Z signature; the header gate is what separates
    // the two, so nothing narrower is available here.
    listResult.append(QStringLiteral("1F9D"));

    return listResult;
}

XBinary::FT XMwaveZ::getFileType()
{
    return FT_MWAVE_Z;
}

XBinary::MODE XMwaveZ::getMode()
{
    return MODE_DATA;
}

qint32 XMwaveZ::getType()
{
    return TYPE_Z;
}

QString XMwaveZ::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    if (nType == TYPE_Z) {
        sResult = QString("Z");
    }

    return sResult;
}

XBinary::ENDIAN XMwaveZ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMwaveZ::getArch()
{
    return QString();
}

QString XMwaveZ::getFileFormatExt()
{
    return QStringLiteral("Z");
}

QString XMwaveZ::getFileFormatExtsString()
{
    return QStringLiteral("IBM Mwave packed file (*.Z)");
}

QString XMwaveZ::getMIMEString()
{
    return QStringLiteral("application/x-mwave-compress");
}

XBinary::OSNAME XMwaveZ::getOsName()
{
    return OSNAME_MSDOS;
}

qint64 XMwaveZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return 0;

    return MWAVEZ_HEADER_SIZE + context.nCompressedSize;
}

QList<XBinary::MAPMODE> XMwaveZ::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XMwaveZ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;
    }

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QList<XBinary::FPART> XMwaveZ::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    const qint64 nArchiveSize = MWAVEZ_HEADER_SIZE + context.nCompressedSize;

    if ((nFileParts & FILEPART_HEADER) &&
        ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = MWAVEZ_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.header.sFileName;
        part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                  context.header.sFileName);
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  HANDLE_METHOD_MWAVE_Z);
        part.mapProperties.insert(
            FPART_PROP_DATETIME,
            XBinary::dosDateTimeToQDateTime(context.header.nDosDate,
                                            context.header.nDosTime));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nArchiveSize < context.nInputSize) &&
        ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = nArchiveSize;
        part.nFileSize = context.nInputSize - nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XMwaveZ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMwaveZ::initUnpack(UNPACK_STATE *pState,
                         const QMap<UNPACK_PROP, QVariant> &mapProperties,
                         PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();

    if (!pState || m_bUnpackOperationInProgress) return false;
    const bool bSequential = guardedSource->isSequential();
    if (bSequential) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    const bool bFinished = finishUnpack(pState, nullptr);
    if (!bFinished ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!bBound) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    const bool bParsed = parseContext(pContext, pPdStruct);
    if (!bParsed) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("IBM Mwave packed file; a single Unix compress (LZW) member"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = MWAVEZ_HEADER_SIZE + pContext->nCompressedSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source; without the finalize, listing works and
    // extraction silently produces nothing.
    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XMwaveZ::infoCurrent(UNPACK_STATE *pState,
                                            PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    if (!pState || !pState->pContext || !isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!bSourceCurrent) return ARCHIVERECORD();

    if ((pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                pContext->header.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MWAVE_Z);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QString("Compress (LZW, %1 bits)")
                                    .arg(pContext->header.nFlags &
                                         MWAVEZ_FLAGS_MAXBITS));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(
        FPART_PROP_DATETIME,
        XBinary::dosDateTimeToQDateTime(pContext->header.nDosDate,
                                        pContext->header.nDosTime));
    // The +0x02 word is a checksum whose algorithm is not identified, so it is
    // deliberately NOT published as FPART_PROP_RESULTCRC: an unverifiable value
    // in a CRC slot would be reported as a failed integrity check.

    return result;
}

bool XMwaveZ::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                            PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext || !pDevice) return false;

    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bOutputSupported = isUnpackOutputSupported(guardedOutput);
    if (!bOutputSupported) return false;
    const bool bAliases =
        XBinary::devicesAlias(guardedSource, guardedOutput);
    if (bAliases) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!bSourceCurrent) return false;

    if ((pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint64 nInputSize = getSize();
    if (!guardedSource) return false;
    if ((nInputSize < MWAVEZ_MIN_SIZE) || (pContext->nUncompressedSize < 0) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            pContext->nUncompressedSize)) {
        return false;
    }

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here. Produced bytes are charged by _writeDevice through
    // decompressState.spOutputBudget.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                pContext->header.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(
        XBinary::createFileBuffer(pContext->nUncompressedSize, pPdStruct));
    if (!pStage) return false;
    const bool bStageSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!bStageSourceCurrent) return false;

    MwaveZStream stream(guardedSource, MWAVEZ_HEADER_SIZE,
                        nInputSize - MWAVEZ_HEADER_SIZE);
    if (!stream.open(QIODevice::ReadOnly)) return false;

    XBinary::DATAPROCESS_STATE decompressState = {};
    decompressState.mapUnpackProperties = pState->mapUnpackProperties;
    decompressState.spOutputBudget = pState->spOutputBudget;
    decompressState.pDeviceInput = &stream;
    decompressState.pDeviceOutput = pStage.get();
    decompressState.nInputOffset = 0;
    decompressState.nInputLimit = stream.size();
    decompressState.nProcessedOffset = 0;
    decompressState.nProcessedLimit = -1;

    bool bResult = XCompressDecoder::decompress(&decompressState, pPdStruct);
    stream.close();
    bResult = bResult && (decompressState.nCountOutput == pContext->nUncompressedSize);

    if (!bResult) return false;
    const bool bFinalSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!bFinalSourceCurrent) {
        return false;
    }

    const bool bPublished =
        publishUnpackOutput(pStage.get(), guardedOutput, pState,
                            pPdStruct);

    return bPublished;
}

bool XMwaveZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    // Guard on >= nNumberOfRecords, never on nNumberOfRecords - 1: the index
    // has to be allowed to advance past the last record or nothing lists.
    if (!bSourceCurrent || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);

    ++pState->nCurrentIndex;
    pState->nCurrentOffset = MWAVEZ_HEADER_SIZE + pContext->nCompressedSize;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XMwaveZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XMwaveZ::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_REPORTEDMETHOD);
    listResult.append(FPART_PROP_DATETIME);
    listResult.append(FPART_PROP_ISFOLDER);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}
