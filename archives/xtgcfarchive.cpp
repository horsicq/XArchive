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
#include "xtgcfarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 TGCF_MIN_SIZE = 0x20;
const qint64 TGCF_HEADER_SIZE = 0x1c;
const qint64 TGCF_RECORD_SIZE = 0x24;
const qint64 TGCF_TAIL_SIZE = 0x0b;
const qint32 TGCF_MAX_MEMBERS = 200000;
const qint32 TGCF_MAX_NAME_SIZE = 1024;
const qint64 TGCF_NAME_WINDOW = 0x900;  // two names plus the flag and offsets

const quint16 TGCF_SPLIT_FRAGMENT = 2;
const quint16 TGCF_METHOD_STORE = 0;
const quint16 TGCF_METHOD_ZLIB = 4;

// The stored names are absolute Windows paths with a drive letter
// ("F:\CATraxx32\Doc\License.txt"), which cannot be recreated as such.
QString tgcfNormalizeName(const QString &sName)
{
    QString sResult = sName;
    sResult.replace(QChar('\\'), QChar('/'));
    if ((sResult.size() >= 2) && (sResult.at(1) == QChar(':'))) sResult = sResult.mid(2);
    while (sResult.startsWith(QChar('/'))) sResult = sResult.mid(1);

    return sResult;
}

bool tgcfRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

quint16 tgcfBE16(const uchar *pData, qint64 nOffset)
{
    return (quint16)(((quint16)pData[nOffset] << 8) | pData[nOffset + 1]);
}

quint32 tgcfBE32(const uchar *pData, qint64 nOffset)
{
    return ((quint32)pData[nOffset] << 24) | ((quint32)pData[nOffset + 1] << 16) | ((quint32)pData[nOffset + 2] << 8) | (quint32)pData[nOffset + 3];
}

}  // namespace

XTGCFArchive::XTGCFArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTGCFArchive::~XTGCFArchive()
{
}

bool XTGCFArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTGCFArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < TGCF_MIN_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, TGCF_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != TGCF_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("TGCF", 4)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    context.nVersion = tgcfBE16(pHeader, 6);
    if ((context.nVersion != 0x0130) && (context.nVersion != 0x0140) && (context.nVersion != 0x0160)) return false;
    const qint64 nNameLength = tgcfBE16(pHeader, 0x1a);
    if ((nNameLength == 0) || (nNameLength > 0x400)) return false;
    context.bExtended = (context.nVersion > 0x015f);

    if (!tgcfRangeWithin(context.nInputSize, TGCF_HEADER_SIZE, nNameLength)) return false;
    const QByteArray baVolume = read_array_process(TGCF_HEADER_SIZE, nNameLength, pPdStruct);
    if (!guardedThis || !guardedSource || (baVolume.size() != nNameLength)) return false;
    context.sVolumeName = QString::fromLatin1(baVolume);

    qint64 nOffset = TGCF_HEADER_SIZE + nNameLength;
    qint64 nListStart = -1;
    if (context.bExtended) {
        if (!tgcfRangeWithin(context.nInputSize, nOffset, 4)) return false;
        const QByteArray baList = read_array_process(nOffset, 4, pPdStruct);
        if (!guardedThis || !guardedSource || (baList.size() != 4)) return false;
        nListStart = tgcfBE32((const uchar *)baList.constData(), 0);
        nOffset += 4;
    }
    nOffset += 4;  // header CRC
    context.nHeaderSize = nOffset;
    if (context.bExtended) {
        if ((nListStart < 0) || (nListStart > context.nInputSize)) return false;
        nOffset = nListStart;
    }
    if (nOffset > context.nInputSize) return false;

    qint64 nPayloadEnd = nOffset;
    while ((context.nInputSize - nOffset) >= TGCF_TAIL_SIZE) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= TGCF_MAX_MEMBERS) break;
        if (!tgcfRangeWithin(context.nInputSize, nOffset, TGCF_RECORD_SIZE)) break;

        const qint64 nReadSize = qMin(TGCF_RECORD_SIZE + TGCF_NAME_WINDOW, context.nInputSize - nOffset);
        const QByteArray baRecord = read_array_process(nOffset, nReadSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != nReadSize)) return false;
        if (baRecord.left(4) != QByteArray("TGCF", 4)) break;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        const quint16 nSplit = qFromLittleEndian<quint16>(pRecord + 0x0c);
        const quint16 nMethod = qFromLittleEndian<quint16>(pRecord + 0x0e);
        const quint32 nTime = tgcfBE32(pRecord, 0x10);
        const qint64 nCompressedSize = tgcfBE32(pRecord, 0x14);
        const qint64 nUncompressedSize = tgcfBE32(pRecord, 0x18);
        const quint32 nCRC = tgcfBE32(pRecord, 0x1c);
        const quint32 nAttributes = tgcfBE32(pRecord, 0x20);

        qint64 nCursor = TGCF_RECORD_SIZE;
        QString sShortName;
        QString sLongName;
        bool bNamesOk = true;
        for (qint32 nWhich = 0; nWhich < 2; ++nWhich) {
            const qint64 nStart = nCursor;
            while ((nCursor < nReadSize) && (pRecord[nCursor] != 0)) ++nCursor;
            if ((nCursor >= nReadSize) || ((nCursor - nStart) > TGCF_MAX_NAME_SIZE)) {
                bNamesOk = false;
                break;
            }
            const QString sName = QString::fromLatin1(baRecord.mid((int)nStart, (int)(nCursor - nStart)));
            if (nWhich == 0) sShortName = sName;
            else sLongName = sName;
            ++nCursor;  // the NUL
        }
        if (!bNamesOk) break;
        if (nCursor >= nReadSize) break;
        ++nCursor;  // the flag byte

        qint64 nPayloadOffset = -1;
        if (context.bExtended) {
            if ((nCursor + 4) > nReadSize) break;
            nPayloadOffset = tgcfBE32(pRecord, nCursor);
            nCursor += 4;
        }
        nCursor += 4;  // record CRC
        const qint64 nRecordEnd = nOffset + nCursor;
        if (nRecordEnd > context.nInputSize) break;
        if (!context.bExtended) nPayloadOffset = nRecordEnd;

        if (!tgcfRangeWithin(context.nInputSize, nPayloadOffset, nCompressedSize)) break;

        if (nSplit != TGCF_SPLIT_FRAGMENT) {
            MEMBER member = {};
            member.nHeaderOffset = nOffset;
            member.nHeaderSize = nCursor;
            member.nDataOffset = nPayloadOffset;
            member.nCompressedSize = nCompressedSize;
            member.nUncompressedSize = nUncompressedSize;
            member.nCRC = nCRC;
            member.nTime = nTime;
            member.nAttributes = nAttributes;
            member.nMethod = nMethod;
            member.sFileName = tgcfNormalizeName(sLongName.isEmpty() ? sShortName : sLongName);
            context.listMembers.append(member);
        }

        if ((nPayloadOffset + nCompressedSize) > nPayloadEnd) nPayloadEnd = nPayloadOffset + nCompressedSize;
        nOffset = context.bExtended ? nRecordEnd : (nPayloadOffset + nCompressedSize);
        if (nOffset > nPayloadEnd) nPayloadEnd = nOffset;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(qMax(nPayloadEnd, nOffset), context.nInputSize);
    *pContext = context;

    return true;
}

bool XTGCFArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTGCFArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTGCFArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTGCFArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTGCFArchive(pDevice);
}

QList<QString> XTGCFArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'TGCF'0024");
}

XBinary::FT XTGCFArchive::getFileType()
{
    return FT_TGCF;
}

XBinary::MODE XTGCFArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTGCFArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XTGCFArchive::getArch()
{
    return QString();
}

qint32 XTGCFArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTGCFArchive::getFileFormatExt()
{
    return QStringLiteral("tgcf");
}

QString XTGCFArchive::getFileFormatExtsString()
{
    return QStringLiteral("TGCF archive");
}

QString XTGCFArchive::getMIMEString()
{
    return QStringLiteral("application/x-tgcf");
}

QString XTGCFArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();

    return QString("%1.%2").arg(context.nVersion >> 8).arg(context.nVersion & 0xff, 2, 16, QChar('0'));
}

qint64 XTGCFArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XTGCFArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTGCFArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XTGCFArchive::methodToString(quint16 nMethod)
{
    if (nMethod == TGCF_METHOD_STORE) return QStringLiteral("Stored");
    if (nMethod == TGCF_METHOD_ZLIB) return QStringLiteral("Deflate (zlib)");

    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XTGCFArchive::methodToHandleMethod(quint16 nMethod, qint64 nCompressedSize, qint64 nUncompressedSize)
{
    // the reference refuses a stored member whose two size dwords disagree
    if ((nMethod == TGCF_METHOD_STORE) && (nCompressedSize == nUncompressedSize)) return HANDLE_METHOD_STORE;
    if (nMethod == TGCF_METHOD_ZLIB) return HANDLE_METHOD_ZLIB;

    return HANDLE_METHOD_UNKNOWN;
}

bool XTGCFArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTGCFArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod, member.nCompressedSize, member.nUncompressedSize));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            if (!context.bExtended) {
                // Pre-0x160 volumes seed 0xFFFFFFFF and complement the result,
                // i.e. plain CRC-32 over the member's uncompressed bytes, so the
                // shared verifier can authenticate an extraction instead of
                // trusting the declared size.  The >= 0x160 convention (seed 0,
                // no complement) is deliberately NOT published as CRC-32.
                part.mapProperties.insert(FPART_PROP_RESULTCRC, (quint32)member.nCRC);
                part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            }
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

QMap<XBinary::UNPACK_PROP, QVariant> XTGCFArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTGCFArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTGCFArchive> guardedThis(this);
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
    pState->nTotalSize = pContext->nArchiveSize;
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

XBinary::ARCHIVERECORD XTGCFArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod, member.nCompressedSize, member.nUncompressedSize));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    if (!pContext->bExtended) {
        result.mapProperties.insert(FPART_PROP_RESULTCRC, (quint32)member.nCRC);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    }
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTGCFArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XTGCFArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTGCFArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_RESULTCRC << FPART_PROP_CRC_TYPE << FPART_PROP_ISFOLDER;
}
