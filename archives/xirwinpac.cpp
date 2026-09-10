/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xirwinpac.h"

#include <QFileInfo>
#include <QPointer>

#include <cstring>
#include <new>

namespace {
// "IrwinPac" - 8 ASCII bytes, no NUL.
const char *const IRWINPAC_MAGIC = "IrwinPac";
const qint64 IRWINPAC_MAGIC_SIZE = 8;
const qint64 IRWINPAC_HEADER_SIZE = 20;
const qint32 IRWINPAC_CHUNK_HEADER_SIZE = 10;
const qint32 IRWINPAC_MAX_BLOCK = 65536;
const qint32 IRWINPAC_SHORT_DIST_BITS = 7;
const qint32 IRWINPAC_LONG_DIST_BITS = 11;
// Every sample in the corpus writes 16384-byte blocks; the field is a u16, so
// this is only an upper bound on what a block may claim.
const qint64 IRWINPAC_MAX_BLOCKS = 1 << 20;
const QString IRWINPAC_FALLBACK_NAME = QStringLiteral("irwinpac.bin");

struct IRWINPAC_BITS {
    const quint8 *pData;
    qint64 nSize;
    qint64 nBitPos;
};

bool irwinpacProbeBits(IRWINPAC_BITS *pBits, qint32 nCount, quint32 *pValue)
{
    if ((pBits->nBitPos + nCount) > (pBits->nSize * 8)) return false;

    quint32 nResult = 0;

    for (qint32 i = 0; i < nCount; i++) {
        const quint8 nByte = pBits->pData[pBits->nBitPos >> 3];
        nResult = (nResult << 1) | ((nByte >> (7 - (pBits->nBitPos & 7))) & 1);
        pBits->nBitPos++;
    }

    *pValue = nResult;

    return true;
}

bool irwinpacProbeLength(IRWINPAC_BITS *pBits, qint32 *pLength)
{
    quint32 nValue = 0;

    if (!irwinpacProbeBits(pBits, 2, &nValue)) return false;
    if (nValue < 3) {
        *pLength = 2 + static_cast<qint32>(nValue);
        return true;
    }

    if (!irwinpacProbeBits(pBits, 2, &nValue)) return false;
    if (nValue < 3) {
        *pLength = 5 + static_cast<qint32>(nValue);
        return true;
    }

    qint32 nBase = 8;

    while (true) {
        if (!irwinpacProbeBits(pBits, 4, &nValue)) return false;
        if (nValue < 15) {
            *pLength = nBase + static_cast<qint32>(nValue);
            return true;
        }
        nBase += 15;
        if (nBase > IRWINPAC_MAX_BLOCK) return false;
    }
}

// Structural trial decode of one compressed block.  It does not materialise the
// bytes; it only walks the token grammar and checks that every back-reference
// points strictly behind the write pointer and that the block ends on exactly
// the declared size.  Random data almost never satisfies both at once, which is
// what keeps a chain walk that happens to land on EOF from being published as a
// valid archive.
bool irwinpacProbeBlock(const QByteArray &baPayload, qint32 nBlockSize)
{
    IRWINPAC_BITS bits = {};
    bits.pData = reinterpret_cast<const quint8 *>(baPayload.constData());
    bits.nSize = baPayload.size();
    bits.nBitPos = 0;

    qint32 nProduced = 0;

    while (nProduced < nBlockSize) {
        quint32 nFlag = 0;
        if (!irwinpacProbeBits(&bits, 1, &nFlag)) return false;

        if (nFlag == 0) {
            quint32 nLiteral = 0;
            if (!irwinpacProbeBits(&bits, 8, &nLiteral)) return false;
            nProduced++;
            continue;
        }

        quint32 nShort = 0;
        if (!irwinpacProbeBits(&bits, 1, &nShort)) return false;

        quint32 nDistance = 0;
        if (!irwinpacProbeBits(&bits, (nShort != 0) ? IRWINPAC_SHORT_DIST_BITS : IRWINPAC_LONG_DIST_BITS, &nDistance)) {
            return false;
        }

        qint32 nLength = 0;
        if (!irwinpacProbeLength(&bits, &nLength)) return false;

        if ((nDistance == 0) || (static_cast<qint32>(nDistance) > nProduced)) return false;
        if ((nLength <= 0) || (nLength > (nBlockSize - nProduced))) return false;

        nProduced += nLength;
    }

    return nProduced == nBlockSize;
}
}  // namespace

XIrwinPac::XIrwinPac(QIODevice *pDevice) : XArchive(pDevice)
{
}

QString XIrwinPac::deriveMemberName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return IRWINPAC_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return IRWINPAC_FALLBACK_NAME;

    // The format stores no name at all.  The installer decides the target name
    // from its own script, so the container's file name - "IWCOMPAT.DL_" and
    // friends - is the only label available here.
    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return IRWINPAC_FALLBACK_NAME;

    return sFileName;
}

bool XIrwinPac::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XIrwinPac> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (IRWINPAC_HEADER_SIZE + IRWINPAC_CHUNK_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, IRWINPAC_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != IRWINPAC_HEADER_SIZE)) return false;
    if (std::memcmp(baHeader.constData(), IRWINPAC_MAGIC, static_cast<size_t>(IRWINPAC_MAGIC_SIZE)) != 0) return false;

    const quint32 nDeclaredHeaderSize = static_cast<quint8>(baHeader.at(8)) | (static_cast<quint32>(static_cast<quint8>(baHeader.at(9))) << 8);
    if (nDeclaredHeaderSize != static_cast<quint32>(IRWINPAC_HEADER_SIZE)) return false;

    context.nStreamOffset = IRWINPAC_HEADER_SIZE;
    context.nStreamSize = context.nInputSize - IRWINPAC_HEADER_SIZE;

    // Walk the block chain.  It must consume the file exactly: a chain that
    // overshoots or stops short is not this format.
    qint64 nOffset = context.nStreamOffset;
    qint64 nBlocks = 0;
    qint64 nUncompressed = 0;
    bool bProbed = false;

    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((context.nInputSize - nOffset) < IRWINPAC_CHUNK_HEADER_SIZE) return false;
        if (nBlocks >= IRWINPAC_MAX_BLOCKS) return false;

        const QByteArray baChunk = read_array_process(nOffset, IRWINPAC_CHUNK_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baChunk.size() != IRWINPAC_CHUNK_HEADER_SIZE)) return false;

        const quint32 nFlag = static_cast<quint8>(baChunk.at(0)) | (static_cast<quint32>(static_cast<quint8>(baChunk.at(1))) << 8);
        const quint32 nChunkSize = static_cast<quint8>(baChunk.at(2)) | (static_cast<quint32>(static_cast<quint8>(baChunk.at(3))) << 8);
        const quint32 nUnpackedSize = static_cast<quint8>(baChunk.at(4)) | (static_cast<quint32>(static_cast<quint8>(baChunk.at(5))) << 8);

        if (nFlag > 1) return false;
        if (nChunkSize <= static_cast<quint32>(IRWINPAC_CHUNK_HEADER_SIZE)) return false;

        const qint64 nPayloadSize = static_cast<qint64>(nChunkSize) - IRWINPAC_CHUNK_HEADER_SIZE;

        if ((context.nInputSize - nOffset) < static_cast<qint64>(nChunkSize)) return false;
        if (nUnpackedSize == 0) return false;
        if (nFlag == 0) {
            // A stored block carries its bytes verbatim, so the two sizes must agree.
            if (nPayloadSize != static_cast<qint64>(nUnpackedSize)) return false;
        } else if (!bProbed) {
            const QByteArray baPayload = read_array_process(nOffset + IRWINPAC_CHUNK_HEADER_SIZE, nPayloadSize, pPdStruct);
            if (!guardedThis || !guardedSource || (baPayload.size() != nPayloadSize)) return false;
            if (!irwinpacProbeBlock(baPayload, static_cast<qint32>(nUnpackedSize))) return false;
            bProbed = true;
        }

        nUncompressed += nUnpackedSize;
        nOffset += nChunkSize;
        nBlocks++;
    }

    if ((nOffset != context.nInputSize) || (nBlocks == 0)) return false;

    context.nUncompressedSize = nUncompressed;
    context.nNumberOfBlocks = nBlocks;

    context.sFileName = deriveMemberName();
    if (!guardedThis || !guardedSource) return false;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XIrwinPac::isValid(PDSTRUCT *pPdStruct)
{
    // The probe runs on a device the caller still owns: remember where its
    // cursor was and put it back, whatever the outcome.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XIrwinPac::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIrwinPac archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XIrwinPac::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XIrwinPac(pDevice);
}

QList<QString> XIrwinPac::getSearchSignatures()
{
    return {QStringLiteral("'IrwinPac'1400")};
}

XBinary::FT XIrwinPac::getFileType()
{
    return FT_IRWINPAC;
}

XBinary::MODE XIrwinPac::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIrwinPac::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIrwinPac::getArch()
{
    return QString();
}

qint32 XIrwinPac::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XIrwinPac::getFileFormatExt()
{
    return QStringLiteral("dl_");
}

QString XIrwinPac::getFileFormatExtsString()
{
    return QStringLiteral("IrwinPac compressed install file (*.dl_ *.ex_ *.hl_ *.sy_)");
}

QString XIrwinPac::getMIMEString()
{
    return QStringLiteral("application/x-irwinpac");
}

QString XIrwinPac::getVersion()
{
    // The two u16 fields behind the magic are the header size (always 20) and a
    // constant 64; neither moves between the 1991 and 1999 disks, so there is
    // no version to report.
    return QString();
}

qint64 XIrwinPac::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XIrwinPac::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIrwinPac::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XIrwinPac::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIrwinPac::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = IRWINPAC_HEADER_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        record.nFileOffset = context.nStreamOffset;
        record.nFileSize = context.nStreamSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IRWINPAC);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("IrwinPac LZ"));
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XIrwinPac::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIrwinPac::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XIrwinPac> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
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
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("IrwinPac compressed install file; single blocked LZ member"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source; without this finalize the listing would
    // work and every extraction would silently produce nothing.
    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XIrwinPac::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IRWINPAC);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("IrwinPac LZ"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XIrwinPac::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // The index must move PAST the last record; stopping one short makes both
    // the GUI and the CLI list nothing at all.
    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XIrwinPac::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}
