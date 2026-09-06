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

#include "xpovlablzh.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// Base header, level 1: the two leading size/checksum bytes plus method[5],
// skipSize, originalSize, mtime, attributes, level, nameLength.  22 bytes get
// us to the start of the name; the CRC-16, the OS byte and the first
// extended-header length word add another five behind it.
const qint32 POVLAB_NAME_OFFSET = 22;
const qint32 POVLAB_MIN_BASE_SIZE = 27;  // 22 + crc16(2) + os(1) + extsize(2)
const qint32 POVLAB_LEVEL = 1;
const qint32 POVLAB_MAX_NAME = 255;
const qint64 POVLAB_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap
const qint32 POVLAB_MAX_MEMBERS = 65536;
const qint64 POVLAB_MAX_EXT_TOTAL = 0x100000;  // 1 MB of extended headers

bool povlabRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The additive header checksum: every base-header byte from index 2 to the
// end, taken modulo 256.  This one byte is what makes a random "-ARA-" pair
// inside unrelated data almost impossible to mistake for a member.
bool povlabChecksumValid(const QByteArray &baHeader)
{
    if (baHeader.size() < 3) return false;
    const qint32 nHeaderSize = static_cast<quint8>(baHeader.at(0));
    if (baHeader.size() < (2 + nHeaderSize)) return false;

    quint32 nSum = 0;
    for (qint32 i = 2; i < (2 + nHeaderSize); ++i) {
        nSum += static_cast<quint8>(baHeader.at(i));
    }

    return ((nSum & 0xFF) == static_cast<quint32>(static_cast<quint8>(baHeader.at(1))));
}

// DOS 8.3 member names.  Path separators are rejected outright: the format is
// flat and a separator here would only ever be a mis-parse walking into
// payload bytes.
bool povlabIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if ((nCharacter < 0x20) || (nCharacter > 0x7E)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':')) return false;
    }
    return true;
}
}  // namespace

XPovlabLzh::XPovlabLzh(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPovlabLzh::~XPovlabLzh()
{
}

// One checked member parser shared by detection, enumeration and the file map.
bool XPovlabLzh::readMember(qint64 nOffset, qint64 nInputSize, MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    QPointer<XPovlabLzh> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pMember || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;
    if (!povlabRangeWithin(nInputSize, nOffset, POVLAB_MIN_BASE_SIZE)) return false;

    const QByteArray baPrefix = read_array_process(nOffset, POVLAB_NAME_OFFSET, pPdStruct);
    if (!guardedThis || !guardedSource || (baPrefix.size() != POVLAB_NAME_OFFSET)) return false;

    const QByteArray baMethod = baPrefix.mid(2, 5);
    const bool bStored = (baMethod == "-ARS-");
    if (!bStored && (baMethod != "-ARA-")) return false;

    if (static_cast<quint8>(baPrefix.at(20)) != POVLAB_LEVEL) return false;

    const qint64 nBaseSize = static_cast<qint64>(static_cast<quint8>(baPrefix.at(0))) + 2;
    const qint32 nNameLength = static_cast<quint8>(baPrefix.at(21));
    if ((nNameLength < 1) || (nNameLength > POVLAB_MAX_NAME)) return false;
    if (nBaseSize < (POVLAB_MIN_BASE_SIZE + nNameLength)) return false;
    if (!povlabRangeWithin(nInputSize, nOffset, nBaseSize)) return false;

    QByteArray baHeader = read_array_process(nOffset, nBaseSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != nBaseSize)) return false;
    if (!povlabChecksumValid(baHeader)) return false;

    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    const qint64 nSkipSize = static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 7));
    const qint64 nOriginalSize = static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 11));
    const quint32 nTime = qFromLittleEndian<quint32>(pHeader + 15);
    const quint8 nAttributes = pHeader[19];

    const QByteArray baName = baHeader.mid(POVLAB_NAME_OFFSET, nNameLength);
    if (!povlabIsValidName(baName)) return false;
    const quint16 nCRC16 = qFromLittleEndian<quint16>(pHeader + POVLAB_NAME_OFFSET + nNameLength);
    const quint8 nOS = pHeader[POVLAB_NAME_OFFSET + nNameLength + 2];

    if (nOriginalSize > POVLAB_MAX_UNCOMPRESSED_SIZE) return false;

    // Level 1 counts the extended headers inside the packed-size field, and
    // the first of their length words is the final word of the base header.
    // Walking the chain is the only way to learn where the payload starts.
    qint64 nExtTotal = 0;
    qint64 nExtOffset = nOffset + nBaseSize;
    QByteArray baLengthWord = baHeader.right(2);
    for (;;) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (baLengthWord.size() != 2) return false;
        const qint64 nNextSize = static_cast<qint64>(qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baLengthWord.constData())));
        if (nNextSize == 0) break;
        if (nNextSize < 3) return false;
        if (nNextSize > (nSkipSize - nExtTotal)) return false;
        if ((nExtTotal + nNextSize) > POVLAB_MAX_EXT_TOTAL) return false;
        if (!povlabRangeWithin(nInputSize, nExtOffset, nNextSize)) return false;
        baLengthWord = read_array_process(nExtOffset + nNextSize - 2, 2, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        nExtTotal += nNextSize;
        nExtOffset += nNextSize;
    }

    const qint64 nCompressedSize = nSkipSize - nExtTotal;
    if (nCompressedSize < 0) return false;
    // A stored member is the one place the container states both sizes for the
    // same bytes; a mismatch means this is not really a "-ARS-" record.
    if (bStored && (nCompressedSize != nOriginalSize)) return false;
    if (!bStored && (nCompressedSize == 0) && (nOriginalSize != 0)) return false;

    const qint64 nDataOffset = nExtOffset;
    if (!povlabRangeWithin(nInputSize, nDataOffset, nCompressedSize)) return false;

    MEMBER member = {};
    member.nHeaderOffset = nOffset;
    member.nHeaderSize = nDataOffset - nOffset;
    member.nDataOffset = nDataOffset;
    member.nCompressedSize = nCompressedSize;
    member.nUncompressedSize = nOriginalSize;
    member.nTime = nTime;
    member.nCRC16 = nCRC16;
    member.nAttributes = nAttributes;
    member.nOS = nOS;
    member.bStored = bStored;
    member.sMethod = QString::fromLatin1(baMethod);
    member.sFileName = QString::fromLatin1(baName);

    *pMember = member;
    return isPdStructNotCanceled(pPdStruct);
}

bool XPovlabLzh::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPovlabLzh> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // The smallest possible archive is one empty stored member with a
    // one-character name, plus the end marker.
    if (context.nInputSize < (POVLAB_MIN_BASE_SIZE + 1 + 1)) return false;

    qint64 nOffset = 0;
    while (isPdStructNotCanceled(pPdStruct)) {
        if (nOffset >= context.nInputSize) return false;

        const QByteArray baLead = read_array_process(nOffset, 1, pPdStruct);
        if (!guardedThis || !guardedSource || (baLead.size() != 1)) return false;
        if (static_cast<quint8>(baLead.at(0)) == 0) break;  // end-of-archive marker

        MEMBER member = {};
        if (!readMember(nOffset, context.nInputSize, &member, pPdStruct)) return false;
        if (!guardedThis || !guardedSource) return false;
        if (context.listMembers.size() >= POVLAB_MAX_MEMBERS) return false;

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (!isPdStructNotCanceled(pPdStruct)) return false;
    if (context.listMembers.isEmpty()) return false;

    // The chain terminates on the 0x00 marker byte; the archive is everything
    // through it.  Anything after that is overlay, not part of the format.
    context.nArchiveSize = nOffset + 1;
    if (context.nArchiveSize > context.nInputSize) return false;

    *pContext = context;
    return guardedThis && guardedSource;
}

bool XPovlabLzh::isValid(PDSTRUCT *pPdStruct)
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

bool XPovlabLzh::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPovlabLzh archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPovlabLzh::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPovlabLzh(pDevice);
}

QList<QString> XPovlabLzh::getSearchSignatures()
{
    QList<QString> listResult;

    // Two skipped bytes (size + checksum), then the renamed method tag.
    listResult.append("....'-ARA-'");
    listResult.append("....'-ARS-'");

    return listResult;
}

XBinary::FT XPovlabLzh::getFileType()
{
    return FT_POVLAB_LZH;
}

XBinary::MODE XPovlabLzh::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPovlabLzh::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPovlabLzh::getArch()
{
    return QString();
}

QString XPovlabLzh::getFileFormatExt()
{
    return QStringLiteral("lzh");
}

QString XPovlabLzh::getFileFormatExtsString()
{
    return QStringLiteral("POVLAB LZH archive (*.*)");
}

QString XPovlabLzh::getMIMEString()
{
    return QStringLiteral("application/x-povlab-lzh");
}

QString XPovlabLzh::getVersion()
{
    // parseContext() rejects anything but level 1, so this is a constant and
    // needs no device access (which would have to snapshot the caller's
    // cursor to stay side-effect free).
    return QStringLiteral("1");
}

qint64 XPovlabLzh::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPovlabLzh::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPovlabLzh::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::HANDLE_METHOD XPovlabLzh::methodToHandleMethod(const MEMBER &member)
{
    // "-ARS-" is LHA's "-lh0-" under another name and "-ARA-" is "-lh5-";
    // both reuse the decoders the shared dispatcher already has.
    return member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_LZH5;
}

bool XPovlabLzh::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPovlabLzh::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.sMethod);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.sMethod.toLatin1());
            // Verified against the corpus: the stored word is CRC-16/ARC
            // (reflected 0xA001, init and xorout 0) over the UNPACKED member.
            part.mapProperties.insert(FPART_PROP_RESULTCRC, static_cast<quint32>(member.nCRC16));
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16ARC);
            if (member.nTime != 0) {
                part.mapProperties.insert(FPART_PROP_DATETIME,
                                          dosDateTimeToQDateTime(static_cast<quint16>(member.nTime >> 16), static_cast<quint16>(member.nTime & 0xFFFF)));
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        result.append(part);
    }

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XPovlabLzh::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPovlabLzh::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XPovlabLzh> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("POVLAB LZH archive; LHA level-1 members retagged \"-ARA-\"/\"-ARS-\""));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
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

XBinary::ARCHIVERECORD XPovlabLzh::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.sMethod);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.sMethod.toLatin1());
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, static_cast<quint32>(member.nCRC16));
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16ARC);
    result.mapProperties.insert(FPART_PROP_FILEMODE, static_cast<quint32>(member.nAttributes));
    if (member.nTime != 0) {
        result.mapProperties.insert(FPART_PROP_DATETIME,
                                    dosDateTimeToQDateTime(static_cast<quint16>(member.nTime >> 16), static_cast<quint16>(member.nTime & 0xFFFF)));
    }
    return result;
}

bool XPovlabLzh::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XPovlabLzh::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
