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
#include "xzaparchive.h"

#include "Algos/xdcldecoder.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 ZAP_HEADER_SIZE = 0x15;
const qint64 ZAP_NAME_FIELD_SIZE = 12;
const qint64 ZAP_MAX_UNCOMPRESSED_SIZE = 0x10000000;
const qint32 ZAP_MAX_MEMBERS = 100000;

bool zapRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool zapIsDclPrologue(quint8 nLiteralMode, quint8 nDictionary)
{
    return (nLiteralMode <= 1) && (nDictionary >= 4) && (nDictionary <= 6);
}

}  // namespace

XZAPArchive::XZAPArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZAPArchive::~XZAPArchive()
{
}

bool XZAPArchive::scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    if ((pMember->nCompressedSize <= 0) || (pMember->nCompressedSize > ZAP_MAX_UNCOMPRESSED_SIZE)) return false;

    const QByteArray baPacked = read_array_process(pMember->nDataOffset, pMember->nCompressedSize, pPdStruct);
    if (!guardedSource || (baPacked.size() != pMember->nCompressedSize)) return false;

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan((const uchar *)baPacked.constData(), pMember->nCompressedSize, ZAP_MAX_UNCOMPRESSED_SIZE, &nConsumed, &nRawSize)) {
        return false;
    }
    if ((nRawSize <= 0) || (nRawSize > ZAP_MAX_UNCOMPRESSED_SIZE)) return false;

    pMember->nUncompressedSize = nRawSize;
    pMember->bUncompressedSizeKnown = true;

    return true;
}

bool XZAPArchive::parseContext(CONTEXT *pContext, bool bScanSizes, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (ZAP_HEADER_SIZE + 2)) return false;

    const QByteArray baProbe = read_array_process(0, ZAP_HEADER_SIZE + 2, pPdStruct);
    if (!guardedSource || (baProbe.size() != (ZAP_HEADER_SIZE + 2))) return false;
    const quint8 nFirstNameLength = (quint8)baProbe.at(0);
    if ((nFirstNameLength < 1) || (nFirstNameLength > ZAP_NAME_FIELD_SIZE)) return false;
    if (!zapIsDclPrologue((quint8)baProbe.at(0x15), (quint8)baProbe.at(0x16))) return false;

    qint64 nOffset = 0;

    while ((nOffset + ZAP_HEADER_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= ZAP_MAX_MEMBERS) break;

        const QByteArray baHeader = read_array_process(nOffset, ZAP_HEADER_SIZE, pPdStruct);
        if (!guardedSource || (baHeader.size() != ZAP_HEADER_SIZE)) return false;
        const uchar *pHeader = (const uchar *)baHeader.constData();

        const qint32 nNameLength = (qint32)pHeader[0];
        if ((nNameLength < 1) || (nNameLength > (qint32)ZAP_NAME_FIELD_SIZE)) break;

        const qint64 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pHeader + 0x11);
        if (nCompressedSize <= 0) break;
        if (!zapRangeWithin(context.nInputSize, nOffset + ZAP_HEADER_SIZE, nCompressedSize)) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + ZAP_HEADER_SIZE;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = 0;
        member.bUncompressedSizeKnown = false;
        member.nDosTime = qFromLittleEndian<quint16>(pHeader + 0x0d);
        member.nDosDate = qFromLittleEndian<quint16>(pHeader + 0x0f);
        // Only the first nNameLength bytes of the 12-byte field are the name.
        member.sFileName = QString::fromLatin1(baHeader.mid(1, nNameLength));

        if (bScanSizes) {
            // A member whose length cannot be recovered stays flagged unknown
            // rather than failing the whole archive.
            scanMemberSize(&member, pPdStruct);
            if (!guardedSource) return false;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = (nOffset < context.nInputSize) ? nOffset : context.nInputSize;
    *pContext = context;

    return true;
}

bool XZAPArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZAPArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZAPArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZAPArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZAPArchive(pDevice);
}

QList<QString> XZAPArchive::getSearchSignatures()
{
    // No magic; the name-length byte plus the DCL prologue are the whole test.
    return QList<QString>();
}

XBinary::FT XZAPArchive::getFileType()
{
    return FT_ZAP;
}

XBinary::MODE XZAPArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZAPArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZAPArchive::getArch()
{
    return QString();
}

qint32 XZAPArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZAPArchive::getFileFormatExt()
{
    return QStringLiteral("zap");
}

QString XZAPArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZAP (*.zap)");
}

QString XZAPArchive::getMIMEString()
{
    return QStringLiteral("application/x-zap");
}

QString XZAPArchive::getVersion()
{
    return QString();
}

qint64 XZAPArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZAPArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZAPArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XZAPArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZAPArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = ZAP_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            if (member.bUncompressedSizeKnown) {
                part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            } else {
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
            }
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL Implode"));
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XZAPArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZAPArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // bScanSizes = true: the extraction path needs the plaintext length of
    // every member, and it is not in the container.
    if (!parseContext(pContext, true, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XZAPArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    if (member.bUncompressedSizeKnown) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    } else {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL Implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZAPArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XZAPArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZAPArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
