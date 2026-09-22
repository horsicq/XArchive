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
#include "xvmarcarchive.h"

#include "Algos/xvmarcdecoder.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 VMARC_HEADER_SIZE = 0x26;      // 38
const qint64 VMARC_EXTENDED_SIZE = 12;
const qint64 VMARC_BLOCK = 80;              // members start on 80-byte bounds
const qint64 VMARC_MAX_INPUT_SIZE = 0x10000000;
const qint32 VMARC_MAX_MEMBERS = 100000;
const qint32 VMARC_NAME_FIELD = 8;

const quint8 VMARC_MAGIC[9] = {0x7a, 0xc3, 0xc6, 0xc6, 0x40, 0x40, 0x40, 0x40, 0x01};
const quint8 VMARC_FORMAT_FIXED = 0xc6;     // EBCDIC 'F'

const quint8 VMARC_FLAG_EXTENDED = 0x01;
const quint8 VMARC_FLAG_STORED = 0x40;
const quint8 VMARC_FLAG_OTHER_CODEC = 0x80;

// CP037 (the code page the reference decodes member names with).
const quint16 VMARC_CP037[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x009c, 0x0009, 0x0086, 0x007f, 0x0097, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
    0x0010, 0x0011, 0x0012, 0x0013, 0x009d, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008f, 0x001c, 0x001d, 0x001e, 0x001f,
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000a, 0x0017, 0x001b, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007,
    0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a,
    0x0020, 0x00a0, 0x00e2, 0x00e4, 0x00e0, 0x00e1, 0x00e3, 0x00e5, 0x00e7, 0x00f1, 0x00a2, 0x002e, 0x003c, 0x0028, 0x002b, 0x007c,
    0x0026, 0x00e9, 0x00ea, 0x00eb, 0x00e8, 0x00ed, 0x00ee, 0x00ef, 0x00ec, 0x00df, 0x0021, 0x0024, 0x002a, 0x0029, 0x003b, 0x00ac,
    0x002d, 0x002f, 0x00c2, 0x00c4, 0x00c0, 0x00c1, 0x00c3, 0x00c5, 0x00c7, 0x00d1, 0x00a6, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f,
    0x00f8, 0x00c9, 0x00ca, 0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x0060, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022,
    0x00d8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x00ab, 0x00bb, 0x00f0, 0x00fd, 0x00fe, 0x00b1,
    0x00b0, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x00aa, 0x00ba, 0x00e6, 0x00b8, 0x00c6, 0x00a4,
    0x00b5, 0x007e, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x00a1, 0x00bf, 0x00d0, 0x00dd, 0x00de, 0x00ae,
    0x005e, 0x00a3, 0x00a5, 0x00b7, 0x00a9, 0x00a7, 0x00b6, 0x00bc, 0x00bd, 0x00be, 0x005b, 0x005d, 0x00af, 0x00a8, 0x00b4, 0x00d7,
    0x007b, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00ad, 0x00f4, 0x00f6, 0x00f2, 0x00f3, 0x00f5,
    0x007d, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x00b9, 0x00fb, 0x00fc, 0x00f9, 0x00fa, 0x00ff,
    0x005c, 0x00f7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00b2, 0x00d4, 0x00d6, 0x00d2, 0x00d3, 0x00d5,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x00b3, 0x00db, 0x00dc, 0x00d9, 0x00da, 0x009f};

qint64 vmarcAlignUp(qint64 nValue)
{
    if (nValue < 0) return 0;
    return ((nValue + VMARC_BLOCK - 1) / VMARC_BLOCK) * VMARC_BLOCK;
}

bool vmarcIsHeader(const quint8 *pData, qint64 nSize, qint64 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 9) > nSize)) return false;
    return memcmp(pData + nOffset, VMARC_MAGIC, 9) == 0;
}
}  // namespace

XVMARCArchive::XVMARCArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XVMARCArchive::~XVMARCArchive()
{
}

QString XVMARCArchive::ebcdicToString(const quint8 *pData, qint32 nSize)
{
    QString sResult;
    for (qint32 i = 0; i < nSize; i++) {
        sResult.append(QChar((ushort)VMARC_CP037[pData[i]]));
    }
    // The reference right-strips only; leading blanks would be part of the
    // name if a producer ever emitted one.
    while (!sResult.isEmpty() && sResult.at(sResult.size() - 1).isSpace()) {
        sResult.chop(1);
    }

    return sResult;
}

bool XVMARCArchive::parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < VMARC_HEADER_SIZE) || (context.nInputSize > VMARC_MAX_INPUT_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, VMARC_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != VMARC_HEADER_SIZE)) return false;
    if (memcmp(baHeader.constData(), VMARC_MAGIC, 9) != 0) return false;

    context.nArchiveSize = context.nInputSize;

    if (!bFull) {
        *pContext = context;
        return true;
    }

    const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
    if (!guardedSource || (baFile.size() != context.nInputSize)) return false;
    const quint8 *pData = (const quint8 *)baFile.constData();
    const qint64 nSize = context.nInputSize;

    qint64 nOffset = 0;
    qint64 nEnd = 0;
    while ((nOffset + VMARC_HEADER_SIZE) <= nSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= VMARC_MAX_MEMBERS) break;

        if (vmarcIsHeader(pData, nSize, nOffset)) {
            const quint8 *pHeader = pData + nOffset;
            MEMBER member = {};
            member.nHeaderOffset = nOffset;
            member.nLRECL = qFromBigEndian<quint16>(pHeader + 0x1c);
            member.nRecordFormat = pHeader[0x24];
            member.bFixed = (member.nRecordFormat == VMARC_FORMAT_FIXED);
            member.nFlags = pHeader[0x25];

            qint64 nDataOffset = nOffset + VMARC_HEADER_SIZE;
            if (member.nFlags & VMARC_FLAG_EXTENDED) {
                nDataOffset += VMARC_EXTENDED_SIZE;
                if (nDataOffset > nSize) break;
            }
            member.nHeaderSize = nDataOffset - nOffset;
            member.nDataOffset = nDataOffset;

            QString sName = ebcdicToString(pHeader + 0x0a, VMARC_NAME_FIELD);
            const QString sType = ebcdicToString(pHeader + 0x12, VMARC_NAME_FIELD);
            if (!sType.isEmpty()) sName = sName + QChar('.') + sType;
            if (sName.isEmpty()) sName = QStringLiteral("MEMBER%1").arg(context.listMembers.size());
            member.sFileName = sName;

            if ((!(member.nFlags & VMARC_FLAG_STORED)) && (member.nFlags & VMARC_FLAG_OTHER_CODEC)) {
                // The reference gives up on the WHOLE container here, it does
                // not skip the member.
                return false;
            }

            member.nMode = (member.nFlags & VMARC_FLAG_STORED) ? XVMARCDecoder::MODE_STORED : XVMARCDecoder::MODE_LZW;

            qint64 nMemberEnd = nDataOffset;
            QByteArray baOut;
            const bool bTerminated =
                XVMARCDecoder::run(baFile, nDataOffset, member.nLRECL, member.bFixed, member.nMode, &baOut, &nMemberEnd, pPdStruct);
            if (!guardedSource) return false;

            if (nMemberEnd < nDataOffset) nMemberEnd = nDataOffset;
            if (nMemberEnd > nSize) nMemberEnd = nSize;
            member.nCompressedSize = nMemberEnd - nDataOffset;
            member.nUncompressedSize = baOut.size();
            context.listMembers.append(member);
            nEnd = nMemberEnd;
            nOffset = nMemberEnd;
            // A stored member with no terminating zero-length record ends the
            // walk; an LZW member's own failure does not, exactly as the
            // reference behaves.
            if ((member.nMode == XVMARCDecoder::MODE_STORED) && (!bTerminated)) break;
        } else {
            nOffset += VMARC_HEADER_SIZE;
        }
        nOffset = vmarcAlignUp(nOffset);
    }

    if (context.listMembers.isEmpty()) return false;
    context.nArchiveSize = qMin<qint64>(qMax<qint64>(vmarcAlignUp(nEnd), VMARC_HEADER_SIZE), context.nInputSize);
    *pContext = context;

    return true;
}

bool XVMARCArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XVMARCArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XVMARCArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XVMARCArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XVMARCArchive(pDevice);
}

QList<QString> XVMARCArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("7ac3c6c64040404001");
}

XBinary::FT XVMARCArchive::getFileType()
{
    return FT_VMARC;
}

XBinary::MODE XVMARCArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XVMARCArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XVMARCArchive::getArch()
{
    return QString();
}

qint32 XVMARCArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XVMARCArchive::getFileFormatExt()
{
    return QStringLiteral("vmarc");
}

QString XVMARCArchive::getFileFormatExtsString()
{
    return QStringLiteral("VMARC (*.vmarc *.vma)");
}

QString XVMARCArchive::getMIMEString()
{
    return QStringLiteral("application/x-vmarc");
}

QString XVMARCArchive::getVersion()
{
    return QString();
}

qint64 XVMARCArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XVMARCArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XVMARCArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XVMARCArchive::methodToString(quint8 nMode)
{
    if (nMode == XVMARCDecoder::MODE_STORED) return QStringLiteral("Stored");
    return QStringLiteral("VMARC LZW");
}

XBinary::HANDLE_METHOD XVMARCArchive::methodToHandleMethod(quint8 nMode)
{
    Q_UNUSED(nMode)
    // The stored branch is length-prefixed record framing, not a plain byte
    // range, so it cannot be aliased onto HANDLE_METHOD_STORE.
    return HANDLE_METHOD_VMARC;
}

bool XVMARCArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XVMARCArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = VMARC_HEADER_SIZE;
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMode));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMode));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XVMARCDecoder::packProperties(member.nLRECL, member.bFixed, member.nMode));
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

QMap<XBinary::UNPACK_PROP, QVariant> XVMARCArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XVMARCArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XVMARCArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMode));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMode));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XVMARCDecoder::packProperties(member.nLRECL, member.bFixed, member.nMode));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XVMARCArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XVMARCArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XVMARCArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
