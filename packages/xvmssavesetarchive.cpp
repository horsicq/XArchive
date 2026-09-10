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
#include "xvmssavesetarchive.h"

#include <QPointer>

#include <new>

namespace {
const qint64 VMS_MAX_INPUT_SIZE = Q_INT64_C(512) * 1024 * 1024;
}  // namespace

XVMSSaveSetArchive::XVMSSaveSetArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XVMSSaveSetArchive::~XVMSSaveSetArchive()
{
}

bool XVMSSaveSetArchive::parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XVMSSaveSetArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context;
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < XVMSSaveSetDecoder::BLOCK_HEADER_SIZE) || (context.nInputSize > VMS_MAX_INPUT_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, XVMSSaveSetDecoder::BLOCK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || ((qint64)baHeader.size() != XVMSSaveSetDecoder::BLOCK_HEADER_SIZE)) return false;
    if (!XVMSSaveSetDecoder::isBlockHeaderValid(baHeader)) return false;

    if (bWalkMembers) {
        const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
        if (!guardedThis || !guardedSource || ((qint64)baFile.size() != context.nInputSize)) return false;
        if (!XVMSSaveSetDecoder::walk(baFile, &context.listMembers, pPdStruct)) return false;
        if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.isEmpty()) return false;
    }

    *pContext = context;

    return true;
}

bool XVMSSaveSetArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context;
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XVMSSaveSetArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XVMSSaveSetArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XVMSSaveSetArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XVMSSaveSetArchive(pDevice);
}

QList<QString> XVMSSaveSetArchive::getSearchSignatures()
{
    // the "magic" is a field pattern, not a string
    return QList<QString>();
}

XBinary::FT XVMSSaveSetArchive::getFileType()
{
    return FT_VMSSAVESET;
}

XBinary::MODE XVMSSaveSetArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XVMSSaveSetArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XVMSSaveSetArchive::getArch()
{
    return QString();
}

qint32 XVMSSaveSetArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XVMSSaveSetArchive::getFileFormatExt()
{
    return QStringLiteral("bck");
}

QString XVMSSaveSetArchive::getFileFormatExtsString()
{
    return QStringLiteral("OpenVMS BACKUP save set (*.bck)");
}

QString XVMSSaveSetArchive::getMIMEString()
{
    return QStringLiteral("application/x-vms-saveset");
}

QString XVMSSaveSetArchive::getVersion()
{
    return QString();
}

qint64 XVMSSaveSetArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XVMSSaveSetArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XVMSSaveSetArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QString XVMSSaveSetArchive::methodToString(const XVMSSaveSetDecoder::MEMBER &member)
{
    return member.bVarRec ? QStringLiteral("BACKUP variable records") : QStringLiteral("BACKUP records");
}

bool XVMSSaveSetArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XVMSSaveSetArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context;
    const bool bNeedMembers = ((nFileParts & FILEPART_STREAM) != 0);
    if (!parseContext(&context, bNeedMembers, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = XVMSSaveSetDecoder::BLOCK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; (nFileParts & FILEPART_STREAM) && (i < context.listMembers.size()); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const XVMSSaveSetDecoder::MEMBER &member = context.listMembers.at(i);

        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = member.nBodyOffset;
        part.nFileSize = member.nBodySpanSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = member.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nBodySpanSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nReportedSize);
        if (member.nBodySpanSize > 0) {
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_VMSSAVESET);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XVMSSaveSetDecoder::memberProperties(member));
        } else {
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        }
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member));
        listResult.append(part);
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

QMap<XBinary::UNPACK_PROP, QVariant> XVMSSaveSetArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XVMSSaveSetArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XVMSSaveSetArchive> guardedThis(this);
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
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
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

XBinary::ARCHIVERECORD XVMSSaveSetArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const XVMSSaveSetDecoder::MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nBodyOffset;
    result.nStreamSize = member.nBodySpanSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nBodySpanSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nReportedSize);
    if (member.nBodySpanSize > 0) {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_VMSSAVESET);
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XVMSSaveSetDecoder::memberProperties(member));
    } else {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XVMSSaveSetArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XVMSSaveSetArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XVMSSaveSetArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
