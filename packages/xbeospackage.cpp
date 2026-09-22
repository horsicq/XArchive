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
#include "xbeospackage.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 BEOS_TAG_SIZE = 7;
const qint64 BEOS_CHUNK_DESCRIPTOR_SIZE = 20;
const qint32 BEOS_MAX_MEMBERS = 100000;
const qint32 BEOS_MAX_DEPTH = 64;
const qint32 BEOS_MAX_STRING = 0x400;
const quint32 BEOS_METHOD_ZLIB = 2;

const quint16 BEOS_TYPE_U32 = 0x101;
const quint16 BEOS_TYPE_U64 = 0x102;
const quint16 BEOS_TYPE_STRING = 0x200;
const quint16 BEOS_TYPE_CHUNK = 0x300;
const quint16 BEOS_TYPE_GROUP = 0x400;
const quint16 BEOS_TYPE_BLOB = 0x500;

bool beosRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XBeOSPackage::XBeOSPackage(QIODevice *pDevice) : XArchive(pDevice), m_nParseInputSize(0)
{
}

XBeOSPackage::~XBeOSPackage()
{
}

bool XBeOSPackage::readTag(qint64 nOffset, TAG *pTag, PDSTRUCT *pPdStruct)
{
    if (!pTag || !beosRangeWithin(m_nParseInputSize, nOffset, BEOS_TAG_SIZE)) return false;
    const QByteArray baTag = read_array_process(nOffset, BEOS_TAG_SIZE, pPdStruct);
    if (baTag.size() != BEOS_TAG_SIZE) return false;
    if (baTag.at(4) != (char)0) return false;
    pTag->baId = baTag.left(4);
    pTag->nType = qFromLittleEndian<quint16>((const uchar *)baTag.constData() + 5);
    pTag->nPayloadOffset = nOffset + BEOS_TAG_SIZE;
    return true;
}

bool XBeOSPackage::skipPayload(qint64 nOffset, quint16 nType, qint64 *pnNext, qint32 nDepth, PDSTRUCT *pPdStruct)
{
    if (!pnNext || (nDepth > BEOS_MAX_DEPTH) || !isPdStructNotCanceled(pPdStruct)) return false;

    if (nType == BEOS_TYPE_U32) {
        if (!beosRangeWithin(m_nParseInputSize, nOffset, 4)) return false;
        *pnNext = nOffset + 4;
        return true;
    }
    if (nType == BEOS_TYPE_U64) {
        if (!beosRangeWithin(m_nParseInputSize, nOffset, 8)) return false;
        *pnNext = nOffset + 8;
        return true;
    }
    if ((nType == BEOS_TYPE_STRING) || (nType == BEOS_TYPE_BLOB)) {
        if (!beosRangeWithin(m_nParseInputSize, nOffset, 4)) return false;
        const QByteArray baLength = read_array_process(nOffset, 4, pPdStruct);
        if (baLength.size() != 4) return false;
        const qint64 nLength = (qint32)qFromBigEndian<quint32>((const uchar *)baLength.constData());
        if ((nLength < 0) || !beosRangeWithin(m_nParseInputSize, nOffset + 4, nLength)) return false;
        *pnNext = nOffset + 4 + nLength;
        return true;
    }
    if (nType == BEOS_TYPE_CHUNK) {
        if (!beosRangeWithin(m_nParseInputSize, nOffset, BEOS_CHUNK_DESCRIPTOR_SIZE)) return false;
        const QByteArray baDescriptor = read_array_process(nOffset, BEOS_CHUNK_DESCRIPTOR_SIZE, pPdStruct);
        if (baDescriptor.size() != BEOS_CHUNK_DESCRIPTOR_SIZE) return false;
        const qint64 nCompressed = (qint64)qFromBigEndian<quint64>((const uchar *)baDescriptor.constData());
        if ((nCompressed < 0) || !beosRangeWithin(m_nParseInputSize, nOffset + BEOS_CHUNK_DESCRIPTOR_SIZE, nCompressed)) return false;
        *pnNext = nOffset + BEOS_CHUNK_DESCRIPTOR_SIZE + nCompressed;
        return true;
    }
    if (nType == BEOS_TYPE_GROUP) {
        qint64 nCurrent = nOffset;
        for (qint32 i = 0; i < BEOS_MAX_MEMBERS; ++i) {
            TAG tag = {};
            if (!readTag(nCurrent, &tag, pPdStruct)) return false;
            if ((tag.nType == 0) && (tag.baId == QByteArray(4, '\0'))) {
                *pnNext = tag.nPayloadOffset;
                return true;
            }
            if (!skipPayload(tag.nPayloadOffset, tag.nType, &nCurrent, nDepth + 1, pPdStruct)) return false;
        }
        return false;
    }

    return false;
}

bool XBeOSPackage::readFields(qint64 nOffset, QString *psName, qint64 *pnDataOffset, qint64 *pnOriginalSize, qint64 *pnNext, PDSTRUCT *pPdStruct)
{
    qint64 nCurrent = nOffset;
    if (pnDataOffset) *pnDataOffset = -1;
    if (pnOriginalSize) *pnOriginalSize = -1;

    for (qint32 i = 0; i < BEOS_MAX_MEMBERS; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        TAG tag = {};
        if (!readTag(nCurrent, &tag, pPdStruct)) return false;
        if ((tag.nType == 0) && (tag.baId == QByteArray(4, '\0'))) {
            if (pnNext) *pnNext = tag.nPayloadOffset;
            return true;
        }
        if ((tag.baId == QByteArray("Name")) && (tag.nType == BEOS_TYPE_STRING)) {
            if (!beosRangeWithin(m_nParseInputSize, tag.nPayloadOffset, 4)) return false;
            const QByteArray baLength = read_array_process(tag.nPayloadOffset, 4, pPdStruct);
            if (baLength.size() != 4) return false;
            const qint64 nLength = (qint32)qFromBigEndian<quint32>((const uchar *)baLength.constData());
            if ((nLength < 0) || (nLength > BEOS_MAX_STRING) || !beosRangeWithin(m_nParseInputSize, tag.nPayloadOffset + 4, nLength)) return false;
            const QByteArray baName = read_array_process(tag.nPayloadOffset + 4, nLength, pPdStruct);
            if (baName.size() != nLength) return false;
            if (psName) *psName = QString::fromUtf8(baName);
            nCurrent = tag.nPayloadOffset + 4 + nLength;
            continue;
        }
        if ((tag.baId == QByteArray("OffT")) && (tag.nType == BEOS_TYPE_U64)) {
            if (!beosRangeWithin(m_nParseInputSize, tag.nPayloadOffset, 8)) return false;
            const QByteArray baValue = read_array_process(tag.nPayloadOffset, 8, pPdStruct);
            if (baValue.size() != 8) return false;
            if (pnDataOffset) *pnDataOffset = (qint64)qFromBigEndian<quint64>((const uchar *)baValue.constData());
            nCurrent = tag.nPayloadOffset + 8;
            continue;
        }
        if ((tag.baId == QByteArray("OrgS")) && (tag.nType == BEOS_TYPE_U64)) {
            if (!beosRangeWithin(m_nParseInputSize, tag.nPayloadOffset, 8)) return false;
            const QByteArray baValue = read_array_process(tag.nPayloadOffset, 8, pPdStruct);
            if (baValue.size() != 8) return false;
            if (pnOriginalSize) *pnOriginalSize = (qint64)qFromBigEndian<quint64>((const uchar *)baValue.constData());
            nCurrent = tag.nPayloadOffset + 8;
            continue;
        }
        if (!skipPayload(tag.nPayloadOffset, tag.nType, &nCurrent, 1, pPdStruct)) return false;
    }

    return false;
}

bool XBeOSPackage::walkFolder(qint64 nOffset, const QString &sPrefix, CONTEXT *pContext, qint64 *pnNext, qint32 nDepth, PDSTRUCT *pPdStruct)
{
    if (!pContext || (nDepth > BEOS_MAX_DEPTH)) return false;

    // A folder opens with an FDat group carrying its own fields.
    TAG tagData = {};
    if (!readTag(nOffset, &tagData, pPdStruct)) return false;
    if (tagData.baId != QByteArray("FDat")) return false;

    QString sName;
    qint64 nCurrent = 0;
    if (!readFields(tagData.nPayloadOffset, &sName, nullptr, nullptr, &nCurrent, pPdStruct)) return false;

    QString sPath = sPrefix;
    if (!sName.isEmpty()) sPath = sPrefix.isEmpty() ? sName : (sPrefix + QChar('/') + sName);

    for (qint32 i = 0; i < BEOS_MAX_MEMBERS; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        TAG tag = {};
        if (!readTag(nCurrent, &tag, pPdStruct)) return false;
        if ((tag.nType == 0) && (tag.baId == QByteArray(4, '\0'))) {
            if (pnNext) *pnNext = tag.nPayloadOffset;
            return true;
        }
        if (tag.baId == QByteArray("FldI")) {
            if (!walkFolder(tag.nPayloadOffset, sPath, pContext, &nCurrent, nDepth + 1, pPdStruct)) return false;
            continue;
        }
        if ((tag.baId == QByteArray("FilI")) || (tag.baId == QByteArray("ScrI"))) {
            QString sEntry;
            qint64 nDataOffset = -1;
            qint64 nOriginalSize = -1;
            if (!readFields(tag.nPayloadOffset, &sEntry, &nDataOffset, &nOriginalSize, &nCurrent, pPdStruct)) return false;
            if ((nDataOffset >= 0) && (pContext->listMembers.size() < BEOS_MAX_MEMBERS)) {
                MEMBER member = {};
                member.nDataOffset = nDataOffset;
                member.nStreamOffset = -1;
                member.nCompressedSize = 0;
                member.nUncompressedSize = (nOriginalSize < 0) ? 0 : nOriginalSize;
                member.sFileName = sPath.isEmpty() ? sEntry : (sPath + QChar('/') + sEntry);
                pContext->listMembers.append(member);
            }
            continue;
        }
        if (!skipPayload(tag.nPayloadOffset, tag.nType, &nCurrent, nDepth + 1, pPdStruct)) return false;
    }

    return false;
}

bool XBeOSPackage::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    m_nParseInputSize = context.nInputSize;
    if (context.nInputSize < 16) return false;

    const QByteArray baSignature = read_array_process(0, 8, pPdStruct);
    if (!guardedSource || (baSignature.size() != 8)) return false;
    if (baSignature != QByteArray("AlB\x1a\xff\x0a\x0d\x00", 8)) return false;

    TAG tagRoot = {};
    if (!readTag(8, &tagRoot, pPdStruct)) return false;
    if ((tagRoot.baId != QByteArray("PhIn")) || (tagRoot.nType != BEOS_TYPE_GROUP)) return false;

    qint64 nCurrent = tagRoot.nPayloadOffset;
    qint64 nTreeOffset = -1;
    for (qint32 i = 0; i < BEOS_MAX_MEMBERS; ++i) {
        TAG tag = {};
        if (!readTag(nCurrent, &tag, pPdStruct)) return false;
        if ((tag.nType == 0) && (tag.baId == QByteArray(4, '\0'))) return false;
        if (tag.baId == QByteArray("COff")) {
            if (tag.nType != BEOS_TYPE_U64) return false;
            if (!beosRangeWithin(context.nInputSize, tag.nPayloadOffset, 8)) return false;
            const QByteArray baValue = read_array_process(tag.nPayloadOffset, 8, pPdStruct);
            if (baValue.size() != 8) return false;
            nTreeOffset = (qint64)qFromBigEndian<quint64>((const uchar *)baValue.constData());
            break;
        }
        if (!skipPayload(tag.nPayloadOffset, tag.nType, &nCurrent, 1, pPdStruct)) return false;
    }
    if ((nTreeOffset < 0) || (nTreeOffset >= context.nInputSize)) return false;

    TAG tagTree = {};
    if (!readTag(nTreeOffset, &tagTree, pPdStruct)) return false;
    if (tagTree.baId != QByteArray("FldI")) return false;

    qint64 nAfterTree = 0;
    if (!walkFolder(tagTree.nPayloadOffset, QString(), &context, &nAfterTree, 0, pPdStruct)) return false;
    if (!guardedSource || context.listMembers.isEmpty()) return false;

    // Resolve each member's deflate stream: FiDa group, then the FiMF chunk.
    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER &member = context.listMembers[i];
        TAG tagFiDa = {};
        if (!readTag(member.nDataOffset, &tagFiDa, pPdStruct)) return false;
        if (tagFiDa.baId != QByteArray("FiDa")) return false;
        TAG tagChunk = {};
        if (!readTag(tagFiDa.nPayloadOffset, &tagChunk, pPdStruct)) return false;
        if ((tagChunk.baId != QByteArray("FiMF")) || (tagChunk.nType != BEOS_TYPE_CHUNK)) return false;
        if (!beosRangeWithin(context.nInputSize, tagChunk.nPayloadOffset, BEOS_CHUNK_DESCRIPTOR_SIZE)) return false;
        const QByteArray baDescriptor = read_array_process(tagChunk.nPayloadOffset, BEOS_CHUNK_DESCRIPTOR_SIZE, pPdStruct);
        if (baDescriptor.size() != BEOS_CHUNK_DESCRIPTOR_SIZE) return false;
        const uchar *pDescriptor = (const uchar *)baDescriptor.constData();
        const qint64 nCompressed = (qint64)qFromBigEndian<quint64>(pDescriptor);
        const qint64 nOriginal = (qint64)qFromBigEndian<quint64>(pDescriptor + 8);
        const quint32 nMethod = qFromBigEndian<quint32>(pDescriptor + 16);
        if (nMethod != BEOS_METHOD_ZLIB) return false;
        if ((nCompressed < 0) || (nOriginal < 0)) return false;
        member.nStreamOffset = tagChunk.nPayloadOffset + BEOS_CHUNK_DESCRIPTOR_SIZE;
        if (!beosRangeWithin(context.nInputSize, member.nStreamOffset, nCompressed)) return false;
        member.nCompressedSize = nCompressed;
        member.nUncompressedSize = nOriginal;
        if ((member.nStreamOffset + nCompressed) > context.nArchiveSize) context.nArchiveSize = member.nStreamOffset + nCompressed;
    }

    if (context.nArchiveSize < nAfterTree) context.nArchiveSize = nAfterTree;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;

    return true;
}

bool XBeOSPackage::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XBeOSPackage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBeOSPackage archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBeOSPackage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBeOSPackage(pDevice);
}

QList<QString> XBeOSPackage::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'AlB'1AFF0A0D00");
}

XBinary::FT XBeOSPackage::getFileType()
{
    return FT_BEOS_PACKAGE;
}

XBinary::MODE XBeOSPackage::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBeOSPackage::getEndian()
{
    return ENDIAN_BIG;
}

QString XBeOSPackage::getArch()
{
    return QString();
}

qint32 XBeOSPackage::getType()
{
    return TYPE_ARCHIVE;
}

QString XBeOSPackage::getFileFormatExt()
{
    return QStringLiteral("pkg");
}

QString XBeOSPackage::getFileFormatExtsString()
{
    return QStringLiteral("BeOS package (*.pkg)");
}

QString XBeOSPackage::getMIMEString()
{
    return QStringLiteral("application/x-beos-package");
}

QString XBeOSPackage::getVersion()
{
    return QString();
}

qint64 XBeOSPackage::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XBeOSPackage::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XBeOSPackage::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XBeOSPackage::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBeOSPackage::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = 8;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Signature");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nStreamOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate (zlib)"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XBeOSPackage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBeOSPackage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nDataOffset;
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XBeOSPackage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate (zlib)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XBeOSPackage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XBeOSPackage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XBeOSPackage::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
