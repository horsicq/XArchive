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
#include "xtrdosarchive.h"

#include <QPointer>

#include <new>

namespace {
const qint64 TRDOS_CATALOGUE_SIZE = 0x800;
const qint64 TRDOS_RECORD_SIZE = 0x10;
const qint32 TRDOS_RECORD_COUNT = 128;
const qint64 TRDOS_SECTOR_SIZE = 0x100;
const qint64 TRDOS_TRACK_SIZE = 0x1000;
const qint64 TRDOS_PREFIX_SIZE = 17;
const qint64 TRDOS_MIN_SIZE = 0x8e8;    // through the disk descriptor
const qint64 TRDOS_MAX_SIZE = 0xa0000;  // 80 tracks, double sided
const qint64 TRDOS_DESCRIPTOR_OFFSET = 0x8e0;
const qint64 TRDOS_DESCRIPTOR_SIZE = 8;

bool trdRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The reference implementation right-trims the eight name bytes with Python's
// str.rstrip(), which strips ASCII whitespace plus NEL and NBSP - and does NOT
// touch a LEADING space.  Corpus names such as " oberon.$K" keep theirs, so
// QString::trimmed() is the wrong tool here.
bool trdIsTrailingSpace(quint8 nByte)
{
    return (nByte == 0x20) || ((nByte >= 0x09) && (nByte <= 0x0d)) || (nByte == 0x85) || (nByte == 0xa0);
}

QString trdName(const QByteArray &baRecord)
{
    qint32 nEnd = 8;
    while ((nEnd > 0) && trdIsTrailingSpace((quint8)baRecord.at(nEnd - 1))) --nEnd;

    QString sResult = QString::fromLatin1(baRecord.left(nEnd));
    const quint8 nExtension = (quint8)baRecord.at(8);
    sResult += QStringLiteral(".$");
    // A control byte in the extension would produce an unusable file name.
    sResult += (nExtension < 0x20) ? QChar('_') : QChar((ushort)nExtension);

    return sResult;
}

// Hobeta: 13 record bytes, a zero, the sector count, then the 16-bit checksum.
QByteArray trdHobetaPrefix(const QByteArray &baRecord, quint8 nSectors)
{
    QByteArray baPrefix = baRecord.left(13);
    baPrefix.append((char)0);
    baPrefix.append((char)nSectors);

    quint16 nChecksum = 0;
    for (qint32 i = 0; i < baPrefix.size(); ++i) nChecksum = (quint16)(nChecksum + (quint8)baPrefix.at(i));
    nChecksum = (quint16)((nChecksum * 0x101U) + 0x69U);
    baPrefix.append((char)(quint8)(nChecksum & 0xff));
    baPrefix.append((char)(quint8)((nChecksum >> 8) & 0xff));

    return baPrefix;
}

}  // namespace

XTRDOSArchive::XTRDOSArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTRDOSArchive::~XTRDOSArchive()
{
}

bool XTRDOSArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTRDOSArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TRDOS_MIN_SIZE) || (context.nInputSize > TRDOS_MAX_SIZE)) return false;

    // The disk descriptor is what makes this format identifiable at all.
    const QByteArray baDescriptor = read_array_process(TRDOS_DESCRIPTOR_OFFSET, TRDOS_DESCRIPTOR_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baDescriptor.size() != TRDOS_DESCRIPTOR_SIZE)) return false;
    const quint8 nDiskType = (quint8)baDescriptor.at(3);
    const quint8 nIdentifier = (quint8)baDescriptor.at(7);
    if (nIdentifier != 0x10) return false;
    if ((nDiskType < 0x16) || (nDiskType > 0x19)) return false;
    context.nDiskType = nDiskType;

    const QByteArray baCatalogue = read_array_process(0, TRDOS_CATALOGUE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baCatalogue.size() != TRDOS_CATALOGUE_SIZE)) return false;

    for (qint32 i = 0; i < TRDOS_RECORD_COUNT; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecordOffset = (qint64)i * TRDOS_RECORD_SIZE;
        const QByteArray baRecord = baCatalogue.mid((qint32)nRecordOffset, (qint32)TRDOS_RECORD_SIZE);
        if (baRecord.size() != TRDOS_RECORD_SIZE) break;

        // An all-zero name ends the catalogue; 0x00 / 0x01 in byte 0 alone
        // only means free or deleted, so the two tests are NOT the same one.
        if (baRecord.left(8) == QByteArray(8, (char)0)) break;
        if ((quint8)baRecord.at(0) <= 1) continue;

        const quint8 nSectors = (quint8)baRecord.at(0x0d);
        const quint8 nFirstSector = (quint8)baRecord.at(0x0e);
        const quint8 nFirstTrack = (quint8)baRecord.at(0x0f);

        MEMBER member = {};
        member.nHeaderOffset = nRecordOffset;
        member.nDataOffset = ((qint64)nFirstSector * TRDOS_SECTOR_SIZE) + ((qint64)nFirstTrack * TRDOS_TRACK_SIZE);
        member.nDataSize = (qint64)nSectors * TRDOS_SECTOR_SIZE;
        member.baPrefix = trdHobetaPrefix(baRecord, nSectors);
        member.sFileName = trdName(baRecord);

        // Truncated images do occur; clamp rather than drop the catalogue.
        if (!trdRangeWithin(context.nInputSize, member.nDataOffset, member.nDataSize)) {
            if (member.nDataOffset > context.nInputSize) continue;
            member.nDataSize = context.nInputSize - member.nDataOffset;
        }

        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XTRDOSArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTRDOSArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTRDOSArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTRDOSArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTRDOSArchive(pDevice);
}

QList<QString> XTRDOSArchive::getSearchSignatures()
{
    // No magic; the disk descriptor at 0x8e0 is the identification.
    return QList<QString>();
}

XBinary::FT XTRDOSArchive::getFileType()
{
    return FT_TRDOS;
}

XBinary::MODE XTRDOSArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTRDOSArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTRDOSArchive::getArch()
{
    return QString();
}

qint32 XTRDOSArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTRDOSArchive::getFileFormatExt()
{
    return QStringLiteral("trd");
}

QString XTRDOSArchive::getFileFormatExtsString()
{
    return QStringLiteral("TR-DOS image (*.trd *.img)");
}

QString XTRDOSArchive::getMIMEString()
{
    return QStringLiteral("application/x-spectrum-trd");
}

QString XTRDOSArchive::getVersion()
{
    return QString();
}

qint64 XTRDOSArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTRDOSArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTRDOSArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XTRDOSArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTRDOSArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TRDOS_CATALOGUE_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Catalogue");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize + TRDOS_PREFIX_SIZE);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SCL_SECTORS);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored sectors"));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTRDOSArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTRDOSArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTRDOSArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

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

XBinary::ARCHIVERECORD XTRDOSArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize + TRDOS_PREFIX_SIZE);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SCL_SECTORS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored sectors"));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTRDOSArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XTRDOSArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XTRDOSArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
