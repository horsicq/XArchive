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
#include "xnerodiscimage.h"

#include <QtEndian>

#include <cstring>
#include <memory>
#include <new>

// Format knowledge: public descriptions of the Nero image layout (footer,
// chunk list, DAOI/DAOX/ETNF/ETN2 track tables).  Sector layout inside raw
// 2352/2448-byte tracks is taken from the sector itself (ECMA-130 sync and
// mode byte), the DAO mode code only decides audio.
namespace {
const qint64 NRG_FOOTER_V1_SIZE = 8;
const qint64 NRG_FOOTER_V2_SIZE = 12;
const qint64 NRG_CHUNK_HEADER_SIZE = 8;
const qint32 NRG_MAX_CHUNKS = 4096;
const qint32 NRG_MAX_TRACKS = 99;
const qint64 NRG_DAO_HEADER_SIZE = 22;
const qint64 NRG_DAOI_ENTRY_SIZE = 30;
const qint64 NRG_DAOX_ENTRY_SIZE = 42;
const qint64 NRG_ETNF_ENTRY_SIZE = 20;
const qint64 NRG_ETN2_ENTRY_SIZE = 32;
const qint32 NRG_USER_SECTOR = 2048;
const qint32 NRG_SECTORS_PER_READ = 512;
const qint32 NRG_MODE_AUDIO = 0x07;
const qint32 NRG_MODE_AUDIO_SUB = 0x10;
const quint8 NRG_SYNC[12] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

quint32 readBE32(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 4 > baData.size())) return 0;
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData()) + nOffset);
}

quint64 readBE64(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 8 > baData.size())) return 0;
    return qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(baData.constData()) + nOffset);
}

quint16 readBE16(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 2 > baData.size())) return 0;
    return qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData()) + nOffset);
}

bool isChunkTag(const QByteArray &baTag)
{
    if (baTag.size() != 4) return false;
    for (qint32 i = 0; i < 4; ++i) {
        const quint8 nValue = static_cast<quint8>(baTag.at(i));
        const bool bUpper = (nValue >= 'A') && (nValue <= 'Z');
        const bool bDigit = (nValue >= '0') && (nValue <= '9');
        if (!bUpper && !bDigit && (nValue != '!')) return false;
    }
    return true;
}

// Sector size implied by a DAO/ETN mode code.  Unknown codes refuse the image.
qint32 sectorSizeForMode(qint32 nMode)
{
    switch (nMode) {
        case 0x00:
        case 0x02: return 2048;
        case 0x03: return 2336;
        case 0x05:
        case 0x06:
        case 0x07: return 2352;
        case 0x0f:
        case 0x10:
        case 0x11: return 2448;
        default: return 0;
    }
}
}  // namespace

XNeroDiscImage::XNeroDiscImage(QIODevice *pDevice) : XArchive(pDevice)
{
}

XNeroDiscImage::~XNeroDiscImage()
{
}

bool XNeroDiscImage::describeTrack(TRACK *pTrack, qint64 nOffset, qint64 nSize, qint32 nSectorSize, qint32 nModeCode, qint64 nChunkListOffset,
                                   PDSTRUCT *pPdStruct)
{
    if (!pTrack) return false;
    if ((nOffset < 0) || (nSize <= 0) || (nOffset + nSize > nChunkListOffset)) return false;
    if ((nSectorSize != 2048) && (nSectorSize != 2336) && (nSectorSize != 2352) && (nSectorSize != 2448)) return false;
    if ((nSize % nSectorSize) != 0) return false;
    const qint64 nSectors = nSize / nSectorSize;

    TRACK track = {};
    track.nDataOffset = nOffset;
    track.nDataSize = nSize;
    track.nSectorSize = nSectorSize;
    track.bAudio = false;
    if (nSectorSize == 2048) {
        track.nUserOffset = 0;
        track.sMethod = QStringLiteral("Store (2048-byte sectors)");
    } else if (nSectorSize == 2336) {
        track.nUserOffset = 8;
        track.sMethod = QStringLiteral("Mode 2 (2336-byte sectors)");
    } else if ((nModeCode == NRG_MODE_AUDIO) || (nModeCode == NRG_MODE_AUDIO_SUB)) {
        track.bAudio = true;
    } else {
        const QByteArray baFirst = read_array_process(nOffset, 16, pPdStruct);
        if ((baFirst.size() != 16)) return false;
        if (memcmp(baFirst.constData(), NRG_SYNC, 12) != 0) {
            // A raw track without the ECMA-130 sync is audio only when the
            // table does not claim it as data; a data mode code over
            // sync-less sectors is an inconsistent image and is refused.
            const bool bDataCode = (nModeCode == 0x05) || (nModeCode == 0x06) || (nModeCode == 0x0f) || (nModeCode == 0x11);
            if (bDataCode) return false;
            track.bAudio = true;
        } else {
            const quint8 nModeByte = static_cast<quint8>(baFirst.at(15));
            if (nModeByte == 1U) {
                track.nUserOffset = 16;
                track.sMethod = QStringLiteral("Mode 1 raw (%1-byte sectors)").arg(nSectorSize);
            } else if (nModeByte == 2U) {
                track.nUserOffset = 24;
                track.sMethod = QStringLiteral("Mode 2 raw (%1-byte sectors)").arg(nSectorSize);
            } else {
                return false;
            }
        }
    }
    if (track.bAudio) {
        track.nUserOffset = 0;
        track.nOutputSize = 0;
        track.sMethod = QStringLiteral("Audio (not extractable)");
    } else {
        track.nOutputSize = nSectors * NRG_USER_SECTOR;
    }
    *pTrack = track;
    return true;
}

bool XNeroDiscImage::parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !guardedSource || !guardedSource->isOpen() || !guardedSource->isReadable() ||
        guardedSource->isSequential() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CONTEXT context = {};
    context.nFileSize = getSize();
    if (!guardedSource || (context.nFileSize < NRG_FOOTER_V2_SIZE + NRG_CHUNK_HEADER_SIZE + NRG_USER_SECTOR)) return false;

    const QByteArray baFooter = read_array_process(context.nFileSize - NRG_FOOTER_V2_SIZE, NRG_FOOTER_V2_SIZE, pPdStruct);
    if (!guardedSource || (baFooter.size() != NRG_FOOTER_V2_SIZE)) return false;

    qint64 nFooterOffset = 0;
    if (baFooter.mid(4, 4) == QByteArray("NERO", 4)) {
        context.nFooterVersion = 1;
        context.nChunkListOffset = readBE32(baFooter, 8);
        nFooterOffset = context.nFileSize - NRG_FOOTER_V1_SIZE;
    } else if (baFooter.left(4) == QByteArray("NER5", 4)) {
        context.nFooterVersion = 2;
        const quint64 nOffset = readBE64(baFooter, 4);
        if (nOffset > static_cast<quint64>(context.nFileSize)) return false;
        context.nChunkListOffset = static_cast<qint64>(nOffset);
        nFooterOffset = context.nFileSize - NRG_FOOTER_V2_SIZE;
    } else {
        return false;
    }
    if ((context.nChunkListOffset < NRG_USER_SECTOR) || (context.nChunkListOffset + NRG_CHUNK_HEADER_SIZE > nFooterOffset)) return false;

    // Walk the chunk list up to END!.
    bool bEnd = false;
    qint32 nTrackNumber = 0;
    qint64 nPosition = context.nChunkListOffset;
    for (qint32 nChunk = 0; (nChunk < NRG_MAX_CHUNKS) && !bEnd; ++nChunk) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nPosition + NRG_CHUNK_HEADER_SIZE > nFooterOffset)) return false;
        const QByteArray baChunkHeader = read_array_process(nPosition, NRG_CHUNK_HEADER_SIZE, pPdStruct);
        if (!guardedSource || (baChunkHeader.size() != NRG_CHUNK_HEADER_SIZE)) return false;
        const QByteArray baTag = baChunkHeader.left(4);
        if (!isChunkTag(baTag)) return false;
        const qint64 nChunkSize = readBE32(baChunkHeader, 4);
        if (nPosition + NRG_CHUNK_HEADER_SIZE + nChunkSize > nFooterOffset) return false;
        const qint64 nPayload = nPosition + NRG_CHUNK_HEADER_SIZE;

        if (baTag == QByteArray("END!", 4)) {
            bEnd = true;
        } else if ((baTag == QByteArray("DAOI", 4)) || (baTag == QByteArray("DAOX", 4))) {
            const bool bWide = (baTag == QByteArray("DAOX", 4));
            const qint64 nEntrySize = bWide ? NRG_DAOX_ENTRY_SIZE : NRG_DAOI_ENTRY_SIZE;
            if ((nChunkSize < NRG_DAO_HEADER_SIZE + nEntrySize) || (((nChunkSize - NRG_DAO_HEADER_SIZE) % nEntrySize) != 0)) return false;
            const qint64 nEntries = (nChunkSize - NRG_DAO_HEADER_SIZE) / nEntrySize;
            if (nEntries > NRG_MAX_TRACKS) return false;
            const QByteArray baDao = read_array_process(nPayload, nChunkSize, pPdStruct);
            if (!guardedSource || (baDao.size() != nChunkSize)) return false;
            for (qint64 i = 0; i < nEntries; ++i) {
                const qint64 nEntry = NRG_DAO_HEADER_SIZE + i * nEntrySize;
                const qint32 nSectorSize = readBE16(baDao, nEntry + 12);
                const qint32 nModeCode = static_cast<quint8>(baDao.at(static_cast<int>(nEntry + 14)));
                qint64 nIndex0 = 0;
                qint64 nIndex1 = 0;
                qint64 nEnd = 0;
                if (bWide) {
                    const quint64 nRaw0 = readBE64(baDao, nEntry + 18);
                    const quint64 nRaw1 = readBE64(baDao, nEntry + 26);
                    const quint64 nRaw2 = readBE64(baDao, nEntry + 34);
                    if ((nRaw0 > static_cast<quint64>(context.nFileSize)) || (nRaw1 > static_cast<quint64>(context.nFileSize)) ||
                        (nRaw2 > static_cast<quint64>(context.nFileSize))) {
                        return false;
                    }
                    nIndex0 = static_cast<qint64>(nRaw0);
                    nIndex1 = static_cast<qint64>(nRaw1);
                    nEnd = static_cast<qint64>(nRaw2);
                } else {
                    nIndex0 = readBE32(baDao, nEntry + 18);
                    nIndex1 = readBE32(baDao, nEntry + 22);
                    nEnd = readBE32(baDao, nEntry + 26);
                }
                if ((nIndex0 > nIndex1) || (nIndex1 >= nEnd)) return false;
                if (nTrackNumber >= NRG_MAX_TRACKS) return false;
                TRACK track = {};
                if (!describeTrack(&track, nIndex1, nEnd - nIndex1, nSectorSize, nModeCode, context.nChunkListOffset, pPdStruct)) return false;
                ++nTrackNumber;
                track.sName = QStringLiteral("Track%1.%2").arg(nTrackNumber, 2, 10, QLatin1Char('0')).arg(track.bAudio ? QStringLiteral("audio") : QStringLiteral("iso"));
                context.listTracks.append(track);
            }
        } else if ((baTag == QByteArray("ETNF", 4)) || (baTag == QByteArray("ETN2", 4))) {
            const bool bWide = (baTag == QByteArray("ETN2", 4));
            const qint64 nEntrySize = bWide ? NRG_ETN2_ENTRY_SIZE : NRG_ETNF_ENTRY_SIZE;
            if ((nChunkSize < nEntrySize) || ((nChunkSize % nEntrySize) != 0)) return false;
            const qint64 nEntries = nChunkSize / nEntrySize;
            if (nEntries > NRG_MAX_TRACKS) return false;
            const QByteArray baEtn = read_array_process(nPayload, nChunkSize, pPdStruct);
            if (!guardedSource || (baEtn.size() != nChunkSize)) return false;
            for (qint64 i = 0; i < nEntries; ++i) {
                const qint64 nEntry = i * nEntrySize;
                qint64 nOffset = 0;
                qint64 nSize = 0;
                qint32 nMode = 0;
                if (bWide) {
                    const quint64 nRawOffset = readBE64(baEtn, nEntry);
                    const quint64 nRawSize = readBE64(baEtn, nEntry + 8);
                    if ((nRawOffset > static_cast<quint64>(context.nFileSize)) || (nRawSize > static_cast<quint64>(context.nFileSize))) return false;
                    nOffset = static_cast<qint64>(nRawOffset);
                    nSize = static_cast<qint64>(nRawSize);
                    nMode = static_cast<qint32>(readBE32(baEtn, nEntry + 16));
                } else {
                    nOffset = readBE32(baEtn, nEntry);
                    nSize = readBE32(baEtn, nEntry + 4);
                    nMode = static_cast<qint32>(readBE32(baEtn, nEntry + 8));
                }
                const qint32 nSectorSize = sectorSizeForMode(nMode);
                if (nSectorSize == 0) return false;
                if (nTrackNumber >= NRG_MAX_TRACKS) return false;
                TRACK track = {};
                if (!describeTrack(&track, nOffset, nSize, nSectorSize, nMode, context.nChunkListOffset, pPdStruct)) return false;
                ++nTrackNumber;
                track.sName = QStringLiteral("Track%1.%2").arg(nTrackNumber, 2, 10, QLatin1Char('0')).arg(track.bAudio ? QStringLiteral("audio") : QStringLiteral("iso"));
                context.listTracks.append(track);
            }
        }
        nPosition = nPayload + nChunkSize;
    }

    if (!bEnd || context.listTracks.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pContext = context;
    return true;
}

bool XNeroDiscImage::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseImage(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);
    return bResult;
}

bool XNeroDiscImage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNeroDiscImage image(pDevice);
    return image.isValid(pPdStruct);
}

XBinary *XNeroDiscImage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XNeroDiscImage(pDevice);
}

QList<QString> XNeroDiscImage::getSearchSignatures()
{
    // The footer is at the tail, so there is no fixed-offset signature.
    return QList<QString>();
}

XBinary::FT XNeroDiscImage::getFileType()
{
    return FT_NERO_NRG;
}

XBinary::MODE XNeroDiscImage::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XNeroDiscImage::getEndian()
{
    return ENDIAN_BIG;
}

QString XNeroDiscImage::getArch()
{
    return QString();
}

qint32 XNeroDiscImage::getType()
{
    return TYPE_ARCHIVE;
}

QString XNeroDiscImage::getFileFormatExt()
{
    return QStringLiteral("nrg");
}

QString XNeroDiscImage::getFileFormatExtsString()
{
    return QStringLiteral("Nero disc image (*.nrg)");
}

QString XNeroDiscImage::getMIMEString()
{
    return QStringLiteral("application/x-nrg");
}

QString XNeroDiscImage::getVersion()
{
    CONTEXT context = {};
    if (!parseImage(&context, nullptr)) return QString();
    return QString::number(context.nFooterVersion);
}

QMap<XBinary::UNPACK_PROP, QVariant> XNeroDiscImage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XNeroDiscImage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) goto failed;
    if (!parseImage(pContext, pPdStruct) || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) goto failed;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listTracks.count();
    pState->nCurrentOffset = pContext->listTracks.constFirst().nDataOffset;
    pState->nTotalSize = pContext->nFileSize;
    pState->pContext = pContext;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) goto failed;
    return true;

failed:
    releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XNeroDiscImage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pState->nNumberOfRecords != pContext->listTracks.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    const TRACK track = pContext->listTracks.at(pState->nCurrentIndex);
    if (track.sName.isEmpty() || (track.nDataSize <= 0)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = track.nDataOffset;
    result.nStreamSize = track.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, track.sName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, track.bAudio ? track.nDataSize : track.nOutputSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, track.nDataSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, track.sMethod);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XNeroDiscImage::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput) || devicesAlias(guardedSource, guardedOutput) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pState->nNumberOfRecords != pContext->listTracks.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const TRACK track = pContext->listTracks.at(pState->nCurrentIndex);
    if (track.bAudio) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Audio tracks are not extractable"));
        return false;
    }
    if ((track.nOutputSize <= 0) || (track.nSectorSize <= 0) || (track.nUserOffset < 0) || (track.nUserOffset + NRG_USER_SECTOR > track.nSectorSize) ||
        (track.nDataOffset < 0) || (track.nDataOffset + track.nDataSize > pContext->nFileSize)) {
        return false;
    }
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, track.nOutputSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, track.sName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(track.nOutputSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(track.nOutputSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedOutput || !guardedSource) return false;

    const qint64 nSectors = track.nDataSize / track.nSectorSize;
    qint64 nSector = 0;
    QByteArray baUser;
    while (nSector < nSectors) {
        if (!guardedOutput || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nBatch = qMin<qint64>(NRG_SECTORS_PER_READ, nSectors - nSector);
        const qint64 nReadSize = nBatch * track.nSectorSize;
        const QByteArray baRaw = read_array_process(track.nDataOffset + nSector * track.nSectorSize, nReadSize, pPdStruct);
        if (!guardedOutput || !guardedSource || (baRaw.size() != nReadSize) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
        if (track.nSectorSize == NRG_USER_SECTOR) {
            if (pStage->write(baRaw.constData(), nReadSize) != nReadSize) return false;
        } else {
            baUser.resize(static_cast<int>(nBatch * NRG_USER_SECTOR));
            for (qint64 i = 0; i < nBatch; ++i) {
                memcpy(baUser.data() + i * NRG_USER_SECTOR, baRaw.constData() + i * track.nSectorSize + track.nUserOffset, NRG_USER_SECTOR);
            }
            if (pStage->write(baUser.constData(), baUser.size()) != baUser.size()) return false;
        }
        nSector += nBatch;
    }

    if ((pStage->size() != track.nOutputSize) || !pStage->seek(0) || !guardedOutput || !guardedSource ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
    if (bResult) pState->nCurrentOffset = track.nDataOffset + track.nDataSize;
    return bResult;
}

bool XNeroDiscImage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pState->nNumberOfRecords != pContext->listTracks.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listTracks.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nFileSize;
    return false;
}

bool XNeroDiscImage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XNeroDiscImage::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_ISFOLDER;
}
