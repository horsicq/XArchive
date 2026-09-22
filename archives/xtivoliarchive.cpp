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
#include "xtivoliarchive.h"

#include "Algos/xtivolidecoder.h"

#include <QFileInfo>

#include <new>

namespace {
const qint64 TIVOLI_HEADER_SIZE = XTivoliDecoder::HEADER_SIZE;
const qint64 TIVOLI_MAX_INPUT_SIZE = 0x10000000;
const qint64 TIVOLI_CPIO_HEADER_SIZE = 110;
const qint32 TIVOLI_MAX_MEMBERS = 100000;
const qint32 TIVOLI_MAX_NAME_SIZE = 4096;
const quint32 TIVOLI_S_IFMT = 0170000;
const quint32 TIVOLI_S_IFDIR = 0040000;

// One ASCII-hex field of a cpio header. Rejects anything that is not hex, so a
// desynchronised walk fails instead of inventing a length.
bool tivoliHexField(const uchar *pData, qint32 nSize, quint32 *pnValue)
{
    quint32 nResult = 0;
    for (qint32 i = 0; i < nSize; ++i) {
        const uchar nByte = pData[i];
        qint32 nDigit = -1;
        if ((nByte >= '0') && (nByte <= '9')) nDigit = nByte - '0';
        else if ((nByte >= 'a') && (nByte <= 'f')) nDigit = 10 + (nByte - 'a');
        else if ((nByte >= 'A') && (nByte <= 'F')) nDigit = 10 + (nByte - 'A');
        if (nDigit < 0) return false;
        nResult = (nResult << 4) | (quint32)nDigit;
    }
    *pnValue = nResult;

    return true;
}

bool tivoliIsCpioMagic(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 6) > baData.size())) return false;
    const QByteArray baMagic = baData.mid((qint32)nOffset, 6);

    return (baMagic == QByteArray("070701", 6)) || (baMagic == QByteArray("070702", 6));
}

}  // namespace

XTivoliArchive::XTivoliArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTivoliArchive::~XTivoliArchive()
{
}

bool XTivoliArchive::parseCpio(const QByteArray &baInner, QList<MEMBER> *plistMembers, PDSTRUCT *pPdStruct)
{
    if (!plistMembers) return false;
    plistMembers->clear();
    if (!tivoliIsCpioMagic(baInner, 0)) return false;

    const uchar *pData = (const uchar *)baInner.constData();
    const qint64 nSize = baInner.size();
    qint64 nOffset = 0;

    // Every malformed condition below ends the walk rather than throwing the
    // stream away: what has already been recovered is real, and an empty list
    // is what tells the caller to fall back to the raw unwrapped stream.
    while ((nOffset + TIVOLI_CPIO_HEADER_SIZE) <= nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (plistMembers->size() >= TIVOLI_MAX_MEMBERS) break;
        if (!tivoliIsCpioMagic(baInner, nOffset)) break;

        quint32 nMode = 0;
        quint32 nFileSize = 0;
        quint32 nNameSize = 0;
        if (!tivoliHexField(pData + nOffset + 14, 8, &nMode)) break;
        if (!tivoliHexField(pData + nOffset + 54, 8, &nFileSize)) break;
        if (!tivoliHexField(pData + nOffset + 94, 8, &nNameSize)) break;

        nOffset += TIVOLI_CPIO_HEADER_SIZE;
        if ((nNameSize == 0) || ((qint64)nNameSize > TIVOLI_MAX_NAME_SIZE) || ((qint64)nNameSize > (nSize - nOffset))) break;

        QByteArray baName = baInner.mid((qint32)nOffset, (qint32)nNameSize);
        nOffset += (qint64)nNameSize;
        // The stored name length includes the NUL terminator.
        if (baName.endsWith((char)0)) baName.chop(1);
        // An archive whose only record is the trailer carries nothing; report
        // that as "not a cpio" so the caller falls back to the raw stream.
        if (baName == QByteArray("TRAILER!!!", 10)) return !plistMembers->isEmpty();

        // No padding after the name and none after the data - this is the one
        // place a stock newc reader would go wrong.
        if ((qint64)nFileSize > (nSize - nOffset)) break;

        MEMBER member = {};
        member.nStreamOffset = nOffset;
        member.nSize = (qint64)nFileSize;
        member.nMode = nMode;
        member.bIsFolder = ((nMode & TIVOLI_S_IFMT) == TIVOLI_S_IFDIR);
        member.sFileName = QString::fromLatin1(baName);
        if (member.bIsFolder) {
            member.nSize = 0;
        }
        plistMembers->append(member);

        nOffset += (qint64)nFileSize;
    }

    // A cpio without its trailer is truncated; keep what was recovered rather
    // than throwing the whole archive away.
    return !plistMembers->isEmpty();
}

bool XTivoliArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TIVOLI_HEADER_SIZE) || (context.nInputSize > TIVOLI_MAX_INPUT_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, TIVOLI_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != TIVOLI_HEADER_SIZE)) return false;
    if (!XTivoliDecoder::checkHeader(baHeader)) return false;

    // The members live in the decoded stream only, so enumerating them means
    // unwrapping the whole block chain right here.
    const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
    if (!guardedSource || (baFile.size() != context.nInputSize)) return false;

    QByteArray baInner;
    if (!XTivoliDecoder::unwrapFile(baFile, &baInner, pPdStruct)) return false;
    if (!guardedSource) return false;
    context.nInnerSize = baInner.size();

    if (!parseCpio(baInner, &context.listMembers, pPdStruct)) {
        // Not a cpio: the whole unwrapped stream is the one member, named after
        // the archive itself, which is what the reference extractor produces.
        context.listMembers.clear();
        MEMBER member = {};
        member.nStreamOffset = 0;
        member.nSize = context.nInnerSize;
        member.nMode = 0;
        member.bIsFolder = false;
        member.sFileName = QFileInfo(getDeviceFileName(guardedSource)).fileName();
        if (!guardedSource) return false;
        if (member.sFileName.isEmpty()) member.sFileName = QStringLiteral("tivoli");
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XTivoliArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTivoliArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTivoliArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTivoliArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTivoliArchive(pDevice);
}

QList<QString> XTivoliArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'    79 TFPB-'");
}

XBinary::FT XTivoliArchive::getFileType()
{
    return FT_TIVOLI;
}

XBinary::MODE XTivoliArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTivoliArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XTivoliArchive::getArch()
{
    return QString();
}

qint32 XTivoliArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTivoliArchive::getFileFormatExt()
{
    return QStringLiteral("pkt");
}

QString XTivoliArchive::getFileFormatExtsString()
{
    return QStringLiteral("Tivoli Filepack (*.pkt)");
}

QString XTivoliArchive::getMIMEString()
{
    return QStringLiteral("application/x-tivoli-filepack");
}

QString XTivoliArchive::getVersion()
{
    return QStringLiteral("2.01");
}

qint64 XTivoliArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTivoliArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTivoliArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XTivoliArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTivoliArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TIVOLI_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (member.bIsFolder) continue;

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            // No member has a file extent; the whole container is the stream.
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = 0;
            part.nFileSize = context.nInputSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nInputSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TIVOLI);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Tivoli native"));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XTivoliDecoder::memberProperties(member.nStreamOffset, member.nSize));
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

QMap<XBinary::UNPACK_PROP, QVariant> XTivoliArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTivoliArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    // Every record shares one file extent, so the cursor is the container's own
    // start for all of them; the record index is what tells members apart.
    pState->nCurrentOffset = 0;
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

XBinary::ARCHIVERECORD XTivoliArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();
    if (pState->nCurrentOffset != 0) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);

    ARCHIVERECORD result = {};
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);

    if (member.bIsFolder) {
        result.nStreamOffset = 0;
        result.nStreamSize = 0;
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Directory"));
        return result;
    }

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nInputSize;
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nInputSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TIVOLI);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Tivoli native"));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XTivoliDecoder::memberProperties(member.nStreamOffset, member.nSize));

    return result;
}

bool XTivoliArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = 0;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XTivoliArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTivoliArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
