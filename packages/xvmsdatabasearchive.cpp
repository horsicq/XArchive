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
#include "xvmsdatabasearchive.h"

#include "Algos/xvmsdatabasedecoder.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 VMSDB_MIN_SIZE = 0x16;
const qint64 VMSDB_MAGIC_SIZE = 12;
const qint64 VMSDB_ATTRIBUTE_SIZE = 0xab;
const qint64 VMSDB_BLOCK_SIZE = 0x200;
const qint32 VMSDB_MAX_MEMBERS = 200000;
const qint32 VMSDB_MAX_NAME_SIZE = 4096;
const qint32 VMSDB_MAX_DEPTH = 64;
const qint64 VMSDB_BUFFER_SIZE = 0x10000;

const quint8 VMSDB_TAG_EOC = 0x00;
const quint8 VMSDB_TAG_OCTETSTRING = 0x04;
const quint8 VMSDB_TAG_MEMBER = 0x30;
const quint8 VMSDB_TAG_PRODUCT = 0x61;
const quint8 VMSDB_TAG_ROOT = 0x74;
const quint8 VMSDB_TAG_NAME = 0x81;
const quint8 VMSDB_TAG_SKIPPED = 0x80;
const quint8 VMSDB_TAG_KIT = 0xa2;
const quint8 VMSDB_TAG_CONTENT = 0xa3;
const quint8 VMSDB_TAG_FILES = 0xa8;
const quint8 VMSDB_TAG_FILELIST = 0xaf;

// A forward cursor with its own window.  The walk consumes the stream two and
// four bytes at a time and a kit runs to tens of megabytes, so going through
// the device for each of those reads is not an option.
class VmsCursor {
public:
    VmsCursor(QIODevice *pDevice, qint64 nSize) : m_pDevice(pDevice), m_nSize(nSize), m_nPosition(0), m_nBufferOffset(-1)
    {
    }

    qint64 pos() const
    {
        return m_nPosition;
    }

    qint64 size() const
    {
        return m_nSize;
    }

    bool seek(qint64 nOffset)
    {
        if ((nOffset < 0) || (nOffset > m_nSize)) return false;
        m_nPosition = nOffset;
        return true;
    }

    bool skip(qint64 nSize)
    {
        if ((nSize < 0) || (nSize > (m_nSize - m_nPosition))) return false;
        m_nPosition += nSize;
        return true;
    }

    bool read(quint8 *pBuffer, qint64 nSize)
    {
        if ((nSize < 0) || (nSize > (m_nSize - m_nPosition))) return false;
        qint64 nLeft = nSize;
        quint8 *pOut = pBuffer;
        while (nLeft > 0) {
            if (!fill(m_nPosition)) return false;
            const qint64 nAvailable = m_baBuffer.size() - (m_nPosition - m_nBufferOffset);
            if (nAvailable <= 0) return false;
            const qint64 nPortion = qMin(nAvailable, nLeft);
            if (pOut) {
                memcpy(pOut, m_baBuffer.constData() + (m_nPosition - m_nBufferOffset), (size_t)nPortion);
                pOut += nPortion;
            }
            m_nPosition += nPortion;
            nLeft -= nPortion;
        }
        return true;
    }

    // Reads an element header and steps over it.
    bool tag(XVMSDataBaseDecoder::TAG *pTag)
    {
        quint8 nLookahead[4] = {};
        qint64 nAvailable = m_nSize - m_nPosition;
        if (nAvailable > 4) nAvailable = 4;
        if (nAvailable < 2) return false;
        const qint64 nSaved = m_nPosition;
        if (!read(nLookahead, nAvailable)) return false;
        m_nPosition = nSaved;
        if (!XVMSDataBaseDecoder::parseTag(nLookahead, nAvailable, pTag)) return false;
        return skip(pTag->nHeaderSize);
    }

private:
    bool fill(qint64 nOffset)
    {
        if ((m_nBufferOffset >= 0) && (nOffset >= m_nBufferOffset) && (nOffset < (m_nBufferOffset + m_baBuffer.size()))) return true;
        if (!m_pDevice) return false;
        const qint64 nPortion = qMin(VMSDB_BUFFER_SIZE, m_nSize - nOffset);
        if (nPortion <= 0) return false;
        m_baBuffer = XBinary::read_array(m_pDevice, nOffset, nPortion);
        if (m_baBuffer.size() != nPortion) {
            m_nBufferOffset = -1;
            return false;
        }
        m_nBufferOffset = nOffset;
        return true;
    }

    QPointer<QIODevice> m_pDevice;
    qint64 m_nSize;
    qint64 m_nPosition;
    QByteArray m_baBuffer;
    qint64 m_nBufferOffset;
};

// Steps over one element.  A constructed element has no length, so the only way
// past it is to walk its children to the end-of-contents marker.
bool vmsSkipElement(VmsCursor *pCursor, const XVMSDataBaseDecoder::TAG &tag, qint32 nDepth)
{
    if (nDepth > VMSDB_MAX_DEPTH) return false;
    if (!tag.bConstructed) return pCursor->skip(tag.nLength);

    for (;;) {
        XVMSDataBaseDecoder::TAG inner = {};
        if (!pCursor->tag(&inner)) return false;
        if (inner.nTag == VMSDB_TAG_EOC) return true;
        if (!vmsSkipElement(pCursor, inner, nDepth + 1)) return false;
    }
}

// Finds a constructed element with the wanted tag at the current level.
bool vmsEnterElement(VmsCursor *pCursor, quint8 nWanted)
{
    for (;;) {
        XVMSDataBaseDecoder::TAG tag = {};
        if (!pCursor->tag(&tag)) return false;
        if (tag.nTag == VMSDB_TAG_EOC) return false;
        if (tag.bConstructed && (tag.nTag == nWanted)) return true;
        if (!vmsSkipElement(pCursor, tag, 0)) return false;
    }
}
}  // namespace

XVMSDataBaseArchive::XVMSDataBaseArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XVMSDataBaseArchive::~XVMSDataBaseArchive()
{
}

bool XVMSDataBaseArchive::parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XVMSDataBaseArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < VMSDB_MIN_SIZE) return false;

    const QByteArray baMagic = read_array_process(0, VMSDB_MAGIC_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baMagic.size() != VMSDB_MAGIC_SIZE)) return false;
    const uchar *pMagic = (const uchar *)baMagic.constData();
    if (qFromLittleEndian<quint32>(pMagic) != 0x8074ffff) return false;
    if (qFromLittleEndian<quint32>(pMagic + 4) != 0x018080a0) return false;
    if (qFromLittleEndian<quint32>(pMagic + 8) != 0x00018101) return false;

    if (bWalkMembers) {
        VmsCursor cursor(guardedSource.data(), context.nInputSize);
        // The reference reader opens by discarding two bytes; the first element
        // it looks at is the 0x74 that follows them.
        if (!cursor.seek(2)) return false;
        if (!vmsEnterElement(&cursor, VMSDB_TAG_ROOT)) return false;
        if (!vmsEnterElement(&cursor, VMSDB_TAG_KIT)) return false;
        if (!vmsEnterElement(&cursor, VMSDB_TAG_PRODUCT)) return false;

        // A kit without a file list is well formed and simply holds nothing.
        if (vmsEnterElement(&cursor, VMSDB_TAG_FILELIST) && vmsEnterElement(&cursor, VMSDB_TAG_FILES)) {
            while (context.listMembers.size() < VMSDB_MAX_MEMBERS) {
                if (!isPdStructNotCanceled(pPdStruct)) return false;

                const qint64 nMemberOffset = cursor.pos();
                XVMSDataBaseDecoder::TAG tag = {};
                if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_MEMBER)) break;

                if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_SKIPPED)) break;
                if (!vmsSkipElement(&cursor, tag, 0)) break;

                if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_NAME) || (tag.nLength == 0) || (tag.nLength > VMSDB_MAX_NAME_SIZE)) break;
                QByteArray baName((qint32)tag.nLength, (char)0);
                if (baName.size() != (qint32)tag.nLength) break;
                if (!cursor.read((quint8 *)baName.data(), tag.nLength)) break;

                if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_KIT)) break;
                if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_OCTETSTRING) || (tag.nLength != VMSDB_ATTRIBUTE_SIZE)) break;
                quint8 nAttributes[VMSDB_ATTRIBUTE_SIZE];
                if (!cursor.read(nAttributes, VMSDB_ATTRIBUTE_SIZE)) break;
                if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_EOC)) break;

                const qint64 nBlocks = (qint32)qFromLittleEndian<quint32>(nAttributes + 0x13);
                const qint64 nLastBytes = (qint64)qFromLittleEndian<quint16>(nAttributes + 0x1f);
                const qint64 nSize = ((nBlocks - 1) * VMSDB_BLOCK_SIZE) + nLastBytes;

                MEMBER member = {};
                member.nHeaderOffset = nMemberOffset;
                member.sFileName = QString::fromLatin1(baName).trimmed();

                if (!cursor.tag(&tag)) break;
                if (tag.nTag == VMSDB_TAG_CONTENT) {
                    // A block count of zero makes the declared size negative.
                    // The reference does not reject that: its remaining-bytes
                    // counter simply never goes positive and the member comes
                    // out empty, so the clamp is the faithful reading.
                    const qint64 nDeclared = (nSize > 0) ? nSize : 0;
                    const qint64 nContentOffset = cursor.pos();
                    qint64 nTotal = 0;
                    bool bOk = true;
                    for (;;) {
                        if (!isPdStructNotCanceled(pPdStruct)) return false;
                        XVMSDataBaseDecoder::TAG chunk = {};
                        if (!cursor.tag(&chunk)) {
                            bOk = false;
                            break;
                        }
                        if (chunk.nTag == VMSDB_TAG_EOC) break;
                        if ((chunk.nTag != VMSDB_TAG_OCTETSTRING) || (chunk.nLength == 0) || !cursor.skip(chunk.nLength)) {
                            bOk = false;
                            break;
                        }
                        nTotal += chunk.nLength;
                    }
                    if (!bOk) break;

                    member.nDataOffset = nContentOffset;
                    // The stream deliberately takes in the end-of-contents
                    // marker: it is the decoder's stop signal.
                    member.nDataSize = cursor.pos() - nContentOffset;
                    member.nUncompressedSize = qMin(nDeclared, nTotal);
                    member.bStored = false;
                    context.listMembers.append(member);

                    // TRAP: the enclosing 0x30's own end-of-contents marker.
                    if (!cursor.tag(&tag) || (tag.nTag != VMSDB_TAG_EOC)) break;
                } else if ((tag.nTag == VMSDB_TAG_EOC) && (nSize == 0)) {
                    // An empty file writes no content element at all, so the
                    // marker just read IS the member's own terminator.
                    member.nDataOffset = cursor.pos();
                    member.nDataSize = 0;
                    member.nUncompressedSize = 0;
                    member.bStored = true;
                    context.listMembers.append(member);
                } else {
                    break;
                }
            }
        }
        if (!guardedThis || !guardedSource) return false;
    }

    *pContext = context;

    return true;
}

bool XVMSDataBaseArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XVMSDataBaseArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XVMSDataBaseArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XVMSDataBaseArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XVMSDataBaseArchive(pDevice);
}

QList<QString> XVMSDataBaseArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("ffff7480a0808001018101");
}

XBinary::FT XVMSDataBaseArchive::getFileType()
{
    return FT_VMSDATABASE;
}

XBinary::MODE XVMSDataBaseArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XVMSDataBaseArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XVMSDataBaseArchive::getArch()
{
    return QString();
}

qint32 XVMSDataBaseArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XVMSDataBaseArchive::getFileFormatExt()
{
    return QStringLiteral("pcsi");
}

QString XVMSDataBaseArchive::getFileFormatExtsString()
{
    return QStringLiteral("VMS DataBase (*.pcsi)");
}

QString XVMSDataBaseArchive::getMIMEString()
{
    return QStringLiteral("application/x-vms-database");
}

QString XVMSDataBaseArchive::getVersion()
{
    return QString();
}

qint64 XVMSDataBaseArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XVMSDataBaseArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XVMSDataBaseArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XVMSDataBaseArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XVMSDataBaseArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = VMSDB_MAGIC_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
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
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_VMSDATABASE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bStored ? QStringLiteral("Stored") : QStringLiteral("Chunked"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XVMSDataBaseArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XVMSDataBaseArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XVMSDataBaseArchive> guardedThis(this);
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
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.isEmpty() ? pContext->nInputSize : pContext->listMembers.at(0).nHeaderOffset;
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

XBinary::ARCHIVERECORD XVMSDataBaseArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_VMSDATABASE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bStored ? QStringLiteral("Stored") : QStringLiteral("Chunked"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XVMSDataBaseArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XVMSDataBaseArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XVMSDataBaseArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
