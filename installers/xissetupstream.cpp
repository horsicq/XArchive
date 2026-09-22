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
#include "xissetupstream.h"

#include <QtEndian>

#include <new>

namespace {
const char ISSETUPSTREAM_TAG[14] = {'I', 'S', 'S', 'e', 't', 'u', 'p', 'S', 't', 'r', 'e', 'a', 'm', '\0'};
const qint64 ISSETUPSTREAM_TAG_SIZE = 14;
const qint64 ISSETUPSTREAM_HEADER_SIZE = 46;
const qint64 ISSETUPSTREAM_RECORD_SIZE = 24;
const qint64 ISSETUPSTREAM_MAX_NAME_SIZE = 0x10000;

// The reference accepts exactly these two container versions: it forms
// (1 << version) and tests it against 0x0c, which is versions 2 and 3.  Every
// container in the reference set is version 3.
const quint32 ISSETUPSTREAM_VERSION_MIN = 2;
const quint32 ISSETUPSTREAM_VERSION_MAX = 3;

// Cipher selectors.  The reference treats 0, 0xffffffff and 0xfffffffd as "no
// filter", wraps 2 and 6 in the name-keyed filter, and refuses everything else
// - it returns its "unsupported member" code rather than emitting the bytes.
const quint32 ISSETUPSTREAM_CIPHER_NONE_A = 0;
const quint32 ISSETUPSTREAM_CIPHER_NONE_B = 0xffffffff;
const quint32 ISSETUPSTREAM_CIPHER_NONE_C = 0xfffffffd;
const quint32 ISSETUPSTREAM_CIPHER_2 = 2;
const quint32 ISSETUPSTREAM_CIPHER_6 = 6;

bool issRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XISSetupStream::XISSetupStream(QIODevice *pDevice) : XArchive(pDevice)
{
}

XISSetupStream::~XISSetupStream()
{
}

bool XISSetupStream::parseAt(qint64 nBaseOffset, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!issRangeWithin(pContext->nInputSize, nBaseOffset, ISSETUPSTREAM_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(nBaseOffset, ISSETUPSTREAM_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != ISSETUPSTREAM_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (memcmp(baHeader.constData(), ISSETUPSTREAM_TAG, (size_t)ISSETUPSTREAM_TAG_SIZE) != 0) return false;

    const qint32 nCount = (qint32)qFromLittleEndian<quint16>(pHeader + 0x0e);
    if (nCount <= 0) return false;

    const quint32 nVersion = (quint32)pHeader[0x10];
    if ((nVersion < ISSETUPSTREAM_VERSION_MIN) || (nVersion > ISSETUPSTREAM_VERSION_MAX)) return false;

    // The 29 bytes behind the version are zero in every container.  Requiring
    // them is what separates the container from the .rdata copy of the same
    // tag that the carrier's own code refers to: that copy is followed by
    // whatever string or pointer table happens to come next.
    for (qint32 i = 0x11; i < ISSETUPSTREAM_HEADER_SIZE; ++i) {
        if (pHeader[i] != 0) return false;
    }

    QList<MEMBER> listMembers;
    qint64 nOffset = nBaseOffset + ISSETUPSTREAM_HEADER_SIZE;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!issRangeWithin(pContext->nInputSize, nOffset, ISSETUPSTREAM_RECORD_SIZE)) return false;

        const QByteArray baRecord = read_array_process(nOffset, ISSETUPSTREAM_RECORD_SIZE, pPdStruct);
        if ((baRecord.size() != ISSETUPSTREAM_RECORD_SIZE)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        const quint32 nNameSize = qFromLittleEndian<quint32>(pRecord);
        const quint32 nCipher = qFromLittleEndian<quint32>(pRecord + 4);
        const quint16 nReserved1 = qFromLittleEndian<quint16>(pRecord + 8);
        const quint32 nStreamSize = qFromLittleEndian<quint32>(pRecord + 0x0a);
        const quint64 nReserved2 = qFromLittleEndian<quint64>(pRecord + 0x0e);
        const quint16 nStorage = qFromLittleEndian<quint16>(pRecord + 0x16);

        if ((nNameSize == 0) || ((nNameSize & 1) != 0) || (nNameSize > ISSETUPSTREAM_MAX_NAME_SIZE)) return false;
        if (nReserved1 != 0) return false;
        if (nReserved2 != 0) return false;
        if (nStreamSize > 0x7fffffff) return false;
        if ((nStorage != 0) && (nStorage != 1)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = ISSETUPSTREAM_RECORD_SIZE + (qint64)nNameSize;
        member.nDataOffset = nOffset + member.nHeaderSize;
        member.nStreamSize = (qint64)nStreamSize;
        member.nStorage = (qint32)nStorage;

        if ((nCipher == ISSETUPSTREAM_CIPHER_NONE_A) || (nCipher == ISSETUPSTREAM_CIPHER_NONE_B) || (nCipher == ISSETUPSTREAM_CIPHER_NONE_C)) {
            member.nCipher = 0;
        } else if (nCipher == ISSETUPSTREAM_CIPHER_2) {
            member.nCipher = 2;
        } else if (nCipher == ISSETUPSTREAM_CIPHER_6) {
            member.nCipher = 6;
        } else {
            member.nCipher = -1;
        }

        if (!issRangeWithin(pContext->nInputSize, nOffset + ISSETUPSTREAM_RECORD_SIZE, (qint64)nNameSize)) return false;
        const QByteArray baName = read_array_process(nOffset + ISSETUPSTREAM_RECORD_SIZE, (qint64)nNameSize, pPdStruct);
        if ((baName.size() != (qint32)nNameSize)) return false;

        const QString sRawName = QString::fromUtf16(reinterpret_cast<const char16_t *>(baName.constData()), (qint32)(nNameSize / 2));

        // The key is the name EXACTLY as the record stores it - nothing is
        // trimmed, because a trimmed key decodes to noise.  Only the displayed
        // name loses a trailing NUL, which no observed container writes.
        member.baName = sRawName.toUtf8();

        QString sFileName = sRawName;
        while (!sFileName.isEmpty() && (sFileName.at(sFileName.size() - 1) == QChar('\0'))) sFileName.chop(1);
        if (sFileName.isEmpty()) return false;
        member.sFileName = sFileName;

        if (!issRangeWithin(pContext->nInputSize, member.nDataOffset, member.nStreamSize)) return false;

        listMembers.append(member);
        nOffset = member.nDataOffset + member.nStreamSize;
    }

    if (listMembers.isEmpty()) return false;

    pContext->nBaseOffset = nBaseOffset;
    pContext->nVersion = (qint32)nVersion;
    pContext->nArchiveSize = nOffset;
    pContext->listMembers = listMembers;

    return true;
}

bool XISSetupStream::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (ISSETUPSTREAM_HEADER_SIZE + ISSETUPSTREAM_RECORD_SIZE)) return false;

    qint64 nScanOffset = 0;
    bool bFound = false;
    while ((nScanOffset + ISSETUPSTREAM_TAG_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nFound = find_array(nScanOffset, context.nInputSize - nScanOffset, ISSETUPSTREAM_TAG, ISSETUPSTREAM_TAG_SIZE, pPdStruct);
        if (nFound == -1) break;
        if (parseAt(nFound, &context, pPdStruct)) {
            bFound = true;
            break;
        }
        nScanOffset = nFound + 1;
    }

    if (!bFound) return false;

    *pContext = context;

    return true;
}

XBinary::HANDLE_METHOD XISSetupStream::methodOf(const MEMBER &member)
{
    // A selector this reader does not model must not be reported as STORE:
    // that would publish enciphered bytes as if they were the file.
    if (member.nCipher < 0) return HANDLE_METHOD_UNKNOWN;
    if ((member.nCipher == 0) && (member.nStorage == 0)) return HANDLE_METHOD_STORE;
    if ((member.nCipher == 0) && (member.nStorage == 1)) return HANDLE_METHOD_ZLIB;

    return HANDLE_METHOD_ISSETUPSTREAM;
}

QString XISSetupStream::describe(const MEMBER &member)
{
    QString sResult;

    if (member.nStorage == 1) sResult = QStringLiteral("zlib");
    else sResult = QStringLiteral("stored");

    if (member.nCipher < 0) return sResult + QStringLiteral(" (unsupported filter)");
    if (member.nCipher == 0) return sResult;

    return sResult + QStringLiteral(" + name filter %1").arg(member.nCipher);
}

QByteArray XISSetupStream::propertyOf(const MEMBER &member)
{
    // The blob decISSetupStream() consumes:
    //     [0]   cipher selector, 0 / 2 / 6
    //     [1]   storage, 0 = stored, 1 = zlib
    //     [2..] the member name, UTF-8, WITHOUT the salt
    QByteArray baResult;
    baResult.append((char)(quint8)member.nCipher);
    baResult.append((char)(quint8)member.nStorage);
    baResult.append(member.baName);

    return baResult;
}

bool XISSetupStream::isValid(PDSTRUCT *pPdStruct)
{
    // getRecords-style probing displaces the caller's cursor, so snapshot it.
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XISSetupStream::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XISSetupStream archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XISSetupStream::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XISSetupStream(pDevice);
}

QList<QString> XISSetupStream::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ISSetupStream'00");
}

XBinary::FT XISSetupStream::getFileType()
{
    return FT_ISSETUPSTREAM;
}

XBinary::MODE XISSetupStream::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XISSetupStream::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XISSetupStream::getArch()
{
    return QString();
}

qint32 XISSetupStream::getType()
{
    return TYPE_ARCHIVE;
}

QString XISSetupStream::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XISSetupStream::getFileFormatExtsString()
{
    return QStringLiteral("InstallShield setup stream (*.exe *.dll)");
}

QString XISSetupStream::getMIMEString()
{
    return QStringLiteral("application/x-installshield");
}

QString XISSetupStream::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();

    return QString::number(context.nVersion);
}

qint64 XISSetupStream::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XISSetupStream::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XISSetupStream::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_DATA, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XISSetupStream::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XISSetupStream::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nBaseOffset;
        part.nFileSize = ISSETUPSTREAM_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
            // A compressed member records no inflated length anywhere in the
            // container, so no size is published for it at all.
            if (member.nStorage == 0) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodOf(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, describe(member));
            if (member.nCipher > 0) part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, propertyOf(member));
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

QMap<XBinary::UNPACK_PROP, QVariant> XISSetupStream::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XISSetupStream::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
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
    if (!bFinalized) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XISSetupStream::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    if (member.nStorage == 0) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodOf(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, describe(member));
    if (member.nCipher > 0) result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, propertyOf(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XISSetupStream::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XISSetupStream::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XISSetupStream::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_COMPRESSPROPERTIES;
}
