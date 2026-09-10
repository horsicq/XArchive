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
#include "xwpkarchive.h"

#include "Algos/xwpkdecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 WPK_HEADER_SIZE = 0x0c;
const qint64 WPK_RECORD_SIZE = 0x11;
const quint32 WPK_MAGIC_A = 0x01012403;
const quint32 WPK_MAGIC_B = 0x01332403;
const qint32 WPK_MAX_MEMBERS = 65535;
const qint64 WPK_MAX_DIRECTORY_SIZE = 0x10000;
// How many method A members the sorter probe decodes. One is what the
// reference does; a handful costs little and covers a first member whose code
// lengths carry no ties, where both sorts agree and the first one wins by
// accident.
const qint32 WPK_PROBE_MEMBERS = 3;
const qint32 WPK_SORTER_COUNT = 3;

bool wpkRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString wpkNormalizeName(const QByteArray &baName)
{
    QString sResult = QString::fromLatin1(baName);
    sResult.replace(QChar('\\'), QChar('/'));

    return sResult;
}

}  // namespace

XWPKArchive::XWPKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWPKArchive::~XWPKArchive()
{
}

QByteArray XWPKArchive::decoderProperties(quint32 nSorter, quint32 nCRC)
{
    QByteArray baResult(8, (char)0);
    qToLittleEndian<quint32>(nSorter, (uchar *)baResult.data());
    qToLittleEndian<quint32>(nCRC, (uchar *)baResult.data() + 4);

    return baResult;
}

bool XWPKArchive::parseDecoderProperties(const QByteArray &baProperties, quint32 *pnSorter, quint32 *pnCRC)
{
    if (!pnSorter || !pnCRC || (baProperties.size() != 8)) return false;

    *pnSorter = qFromLittleEndian<quint32>((const uchar *)baProperties.constData());
    *pnCRC = qFromLittleEndian<quint32>((const uchar *)baProperties.constData() + 4);

    return true;
}

bool XWPKArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XWPKArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < WPK_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, WPK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != WPK_HEADER_SIZE)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    const quint32 nMagic = qFromLittleEndian<quint32>(pHeader);
    if ((nMagic != WPK_MAGIC_A) && (nMagic != WPK_MAGIC_B)) return false;

    const qint32 nCount = (qint32)qFromLittleEndian<quint16>(pHeader + 4);
    const qint64 nDirectorySize = (qint64)qFromLittleEndian<quint16>(pHeader + 6);
    const qint64 nDirectoryOffset = (qint64)qFromLittleEndian<quint32>(pHeader + 8);
    if ((nCount == 0) || (nCount > WPK_MAX_MEMBERS)) return false;
    if (nDirectoryOffset <= WPK_HEADER_SIZE) return false;
    if ((nDirectorySize + nDirectoryOffset) != context.nInputSize) return false;
    // The reference detector's own bound: every record costs 0x11 header bytes
    // plus at least one name byte, and the directory has to be strictly larger.
    if (((qint64)nCount * (WPK_RECORD_SIZE + 1)) >= nDirectorySize) return false;
    if (nDirectorySize > WPK_MAX_DIRECTORY_SIZE) return false;
    if (!wpkRangeWithin(context.nInputSize, nDirectoryOffset, nDirectorySize)) return false;

    const QByteArray baDirectory = read_array_process(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize)) return false;

    context.nMagic = nMagic;
    // Only the STARTING candidate; the real answer comes from resolveSorter(),
    // because the magic does not discriminate. See TRAP 3 in the header.
    context.nSorter = (nMagic == WPK_MAGIC_B) ? (quint32)XWPKDecoder::SORTER_B : (quint32)XWPKDecoder::SORTER_A;

    const uchar *pDirectory = (const uchar *)baDirectory.constData();
    qint64 nPosition = 0;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nDirectorySize - nPosition) <= 0x10) return false;

        const qint64 nUncompressed = (qint32)qFromLittleEndian<quint32>(pDirectory + nPosition);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pDirectory + nPosition + 4);
        const quint32 nTime = qFromLittleEndian<quint32>(pDirectory + nPosition + 8);
        const quint32 nCRC = qFromLittleEndian<quint32>(pDirectory + nPosition + 0x0c);
        const quint8 nFlags = pDirectory[nPosition + 0x10];
        const qint64 nNameSize = (qint64)(nFlags & 0x7f);

        const qint64 nRecordOffset = nDirectoryOffset + nPosition;
        nPosition += WPK_RECORD_SIZE;

        if ((nUncompressed < 0) || (nDataOffset < 0) || (nNameSize == 0) || (nNameSize > (nDirectorySize - nPosition))) return false;
        // The payload area is bounded by the directory, never by the file end.
        if ((nDataOffset < WPK_HEADER_SIZE) || (nDataOffset > nDirectoryOffset)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nRecordOffset;
        member.nHeaderSize = WPK_RECORD_SIZE + nNameSize;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nDirectoryOffset - nDataOffset;
        member.nUncompressedSize = nUncompressed;
        member.nCRC = nCRC;
        member.nTime = nTime;
        member.bMethodB = ((nFlags & 0x80) != 0);
        member.sFileName = wpkNormalizeName(baDirectory.mid((qint32)nPosition, (qint32)nNameSize));
        nPosition += nNameSize;

        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}

bool XWPKArchive::resolveSorter(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QPointer<XWPKArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    // The WPK_PROBE_MEMBERS smallest method A members, ascending, by bounded
    // insertion - the smallest ones make the trial decodes cheap.
    qint32 arrProbe[WPK_PROBE_MEMBERS];
    qint32 nProbeCount = 0;

    for (qint32 i = 0; i < pContext->listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const MEMBER &member = pContext->listMembers.at(i);
        if (member.bMethodB || (member.nUncompressedSize <= 0) || (member.nCompressedSize <= 0)) continue;

        qint32 nPosition = nProbeCount;
        while ((nPosition > 0) && (pContext->listMembers.at(arrProbe[nPosition - 1]).nUncompressedSize > member.nUncompressedSize)) {
            --nPosition;
        }
        if (nPosition >= WPK_PROBE_MEMBERS) continue;

        qint32 nLast = (nProbeCount < WPK_PROBE_MEMBERS) ? nProbeCount : (WPK_PROBE_MEMBERS - 1);
        for (qint32 j = nLast; j > nPosition; --j) {
            arrProbe[j] = arrProbe[j - 1];
        }
        arrProbe[nPosition] = i;
        if (nProbeCount < WPK_PROBE_MEMBERS) ++nProbeCount;
    }

    if (nProbeCount == 0) return true;  // nothing but method B members

    QList<QByteArray> listPacked;
    for (qint32 i = 0; i < nProbeCount; ++i) {
        const MEMBER &member = pContext->listMembers.at(arrProbe[i]);
        const QByteArray baPacked = read_array_process(member.nDataOffset, member.nCompressedSize, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baPacked.size() != member.nCompressedSize) return true;
        listPacked.append(baPacked);
    }

    quint32 arrCandidates[WPK_SORTER_COUNT];
    if (pContext->nMagic == WPK_MAGIC_B) {
        arrCandidates[0] = (quint32)XWPKDecoder::SORTER_B;
        arrCandidates[1] = (quint32)XWPKDecoder::SORTER_A;
    } else {
        arrCandidates[0] = (quint32)XWPKDecoder::SORTER_A;
        arrCandidates[1] = (quint32)XWPKDecoder::SORTER_B;
    }
    arrCandidates[2] = (quint32)XWPKDecoder::SORTER_STABLE;

    for (qint32 c = 0; c < WPK_SORTER_COUNT; ++c) {
        bool bAccepted = true;
        for (qint32 i = 0; i < nProbeCount; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            const MEMBER &member = pContext->listMembers.at(arrProbe[i]);
            const QByteArray &baPacked = listPacked.at(i);

            QByteArray baResult;
            qint64 nConsumed = 0;
            if (!XWPKDecoder::decodeMethodA(baPacked, (XWPKDecoder::SORTER)arrCandidates[c], member.nUncompressedSize, &baResult, &nConsumed, pPdStruct)) {
                bAccepted = false;
                break;
            }
            if ((baResult.size() != member.nUncompressedSize) || (nConsumed <= 0) || (nConsumed > baPacked.size())) {
                bAccepted = false;
                break;
            }
            // The stored CRC is the zlib CRC-32 of the CONSUMED compressed
            // prefix, so it needs the final complement the running primitive
            // does not apply.
            const quint32 nCRC = XBinary::_getCRC32(baPacked.constData(), (qint32)nConsumed, 0xffffffffU, XBinary::_getCRC32Table_EDB88320()) ^ 0xffffffffU;
            if (nCRC != member.nCRC) {
                bAccepted = false;
                break;
            }
        }
        if (bAccepted) {
            pContext->nSorter = arrCandidates[c];
            return true;
        }
    }

    return true;
}

bool XWPKArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XWPKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWPKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWPKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWPKArchive(pDevice);
}

QList<QString> XWPKArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("03240101") << QStringLiteral("03243301");
}

XBinary::FT XWPKArchive::getFileType()
{
    return FT_WPK;
}

XBinary::MODE XWPKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XWPKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XWPKArchive::getArch()
{
    return QString();
}

qint32 XWPKArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XWPKArchive::getFileFormatExt()
{
    return QStringLiteral("wpk");
}

QString XWPKArchive::getFileFormatExtsString()
{
    return QStringLiteral("WPK (*.wpk)");
}

QString XWPKArchive::getMIMEString()
{
    return QStringLiteral("application/x-wpk");
}

QString XWPKArchive::getVersion()
{
    return QString();
}

qint64 XWPKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XWPKArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XWPKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QString XWPKArchive::methodToString(bool bMethodB)
{
    return bMethodB ? QStringLiteral("WPK LZSS") : QStringLiteral("WPK Huffman LZSS");
}

XBinary::HANDLE_METHOD XWPKArchive::methodToHandleMethod(bool bMethodB)
{
    return bMethodB ? HANDLE_METHOD_WPK_B : HANDLE_METHOD_WPK_A;
}

bool XWPKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XWPKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;
    // The published FPART_PROP_COMPRESSPROPERTIES carries the sorter, so it has
    // to be the settled one and not the magic's guess.
    if (!resolveSorter(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = WPK_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.bMethodB));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.bMethodB));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, decoderProperties(context.nSorter, member.nCRC));
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

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XWPKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XWPKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XWPKArchive> guardedThis(this);
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
    // Settle the code-length sort once for the whole session; every record then
    // publishes the same, already-validated selector.
    if (!resolveSorter(pContext, pPdStruct) || !guardedThis || !guardedSource) {
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

XBinary::ARCHIVERECORD XWPKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.bMethodB));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.bMethodB));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, decoderProperties(pContext->nSorter, member.nCRC));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XWPKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XWPKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XWPKArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
