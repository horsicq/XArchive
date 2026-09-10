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
#include "xclparchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 CLP_HEADER_SIZE = 4;
const qint64 CLP_RECORD_SIZE = 0x59;
const qint32 CLP_MAX_MEMBERS = 100000;

// The identifier at +0 is the ONLY signature this format has: 0xC350 for the
// Windows 3.0 clipboard and 0xC351 for the Windows NT one.  Without it the
// reader accepts any file with a small number at +2 followed by a few in-range
// 32-bit pairs, which is common enough that it shadowed 25 Quarterdeck QIP and
// GTU archives that have working readers of their own.
const quint16 CLP_ID_WIN3 = 0xc350;
const quint16 CLP_ID_WINNT = 0xc351;

// Standard clipboard formats stop at CF_DIBV5; anything a program registers by
// name lands at or above CF_PRIVATEFIRST.  Real .CLP files use both (the
// reference set holds ids 1, 2, 3, 7, 8, 9 alongside 50417..52384), but
// NOTHING legitimate falls in the gap between them.
const quint16 CLP_CF_MAX_STANDARD = 17;
const quint16 CLP_CF_REGISTERED_FIRST = 0xc000;

const quint16 CLP_CF_TEXT = 1;
const quint16 CLP_CF_OEMTEXT = 7;
const quint16 CLP_CF_DIB = 8;
const quint16 CLP_CF_UNICODETEXT = 13;
const quint16 CLP_CF_DIBV5 = 17;

bool clpRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XCLPArchive::XCLPArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCLPArchive::~XCLPArchive()
{
}

bool XCLPArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XCLPArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < CLP_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, CLP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != CLP_HEADER_SIZE)) return false;
    const quint16 nIdentifier = qFromLittleEndian<quint16>((const uchar *)baHeader.constData());
    if ((nIdentifier != CLP_ID_WIN3) && (nIdentifier != CLP_ID_WINNT)) return false;
    const qint32 nCount = (qint32)qFromLittleEndian<quint16>((const uchar *)baHeader.constData() + 2);
    if ((nCount <= 0) || (nCount > CLP_MAX_MEMBERS)) return false;
    if (!clpRangeWithin(context.nInputSize, CLP_HEADER_SIZE, (qint64)nCount * CLP_RECORD_SIZE)) return false;

    qint32 nAccepted = 0;
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecordOffset = CLP_HEADER_SIZE + ((qint64)i * CLP_RECORD_SIZE);
        const QByteArray baRecord = read_array_process(nRecordOffset, CLP_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != CLP_RECORD_SIZE)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        const quint16 nFormat = qFromLittleEndian<quint16>(pRecord);
        const qint64 nSize = (qint32)qFromLittleEndian<quint32>(pRecord + 2);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pRecord + 6);
        if ((nSize < 0) || (nDataOffset < 0)) return false;
        // an id in neither the standard nor the registered range is not a
        // clipboard record, so the file is not a clipboard file
        if ((nFormat == 0) || ((nFormat > CLP_CF_MAX_STANDARD) && (nFormat < CLP_CF_REGISTERED_FIRST))) return false;
        if (!clpRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nRecordOffset;
        member.nHeaderSize = CLP_RECORD_SIZE;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nSize;
        member.nUncompressedSize = nSize;
        member.nFormat = nFormat;

        if ((nFormat == CLP_CF_TEXT) || (nFormat == CLP_CF_OEMTEXT) || (nFormat == CLP_CF_UNICODETEXT)) {
            const QByteArray baText = read_array_process(nDataOffset, nSize, pPdStruct);
            if (!guardedThis || !guardedSource || (baText.size() != nSize)) return false;
            qint64 nLength = nSize;
            if (nFormat == CLP_CF_UNICODETEXT) {
                nLength = 0;
                while ((nLength + 1) < nSize) {
                    if ((baText.at(nLength) == (char)0) && (baText.at(nLength + 1) == (char)0)) break;
                    nLength += 2;
                }
            } else {
                const qint32 nZero = baText.indexOf('\0');
                if (nZero >= 0) nLength = nZero;
            }
            ++nAccepted;
            member.nCompressedSize = nLength;
            member.nUncompressedSize = nLength;
            member.sFileName = QString::number(nAccepted) + QStringLiteral(".txt");
            context.listMembers.append(member);
            continue;
        }

        if ((nFormat == CLP_CF_DIB) || (nFormat == CLP_CF_DIBV5)) {
            const qint64 nInfoSize = (nFormat == CLP_CF_DIBV5) ? 0x7c : 0x28;
            if (nSize < nInfoSize) continue;
            const QByteArray baInfo = read_array_process(nDataOffset, nInfoSize, pPdStruct);
            if (!guardedThis || !guardedSource || (baInfo.size() != nInfoSize)) return false;
            const uchar *pInfo = (const uchar *)baInfo.constData();
            const qint32 nWidth = (qint32)qFromLittleEndian<quint32>(pInfo + 4);
            const qint32 nHeight = (qint32)qFromLittleEndian<quint32>(pInfo + 8);
            const quint16 nBitCount = qFromLittleEndian<quint16>(pInfo + 14);
            const qint32 nFileSize = (qint32)(nSize + 14);
            const qint32 nOffBits = nFileSize - (qint32)(((qint64)nWidth * nHeight * nBitCount) / 8);

            QByteArray baPrefix;
            baPrefix.resize(14);
            uchar *pPrefix = (uchar *)baPrefix.data();
            qToLittleEndian<quint16>((quint16)0x4d42, pPrefix);
            qToLittleEndian<quint32>((quint32)nFileSize, pPrefix + 2);
            qToLittleEndian<quint16>((quint16)0, pPrefix + 6);
            qToLittleEndian<quint16>((quint16)0, pPrefix + 8);
            qToLittleEndian<quint32>((quint32)nOffBits, pPrefix + 10);

            ++nAccepted;
            member.baPrefix = baPrefix;
            member.nUncompressedSize = nSize + 14;
            member.sFileName = QString::number(nAccepted) + QStringLiteral(".bmp");
            context.listMembers.append(member);
            continue;
        }

        // no extractable form; listed so the record is at least visible
        member.sFileName = QStringLiteral("record%1.fmt%2.bin").arg(i + 1).arg(nFormat);
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}


XBinary::HANDLE_METHOD XCLPArchive::methodOf(const MEMBER &member)
{
    if (!member.baPrefix.isEmpty()) return HANDLE_METHOD_SCL_SECTORS;  // prefix then copy
    if ((member.nFormat == CLP_CF_TEXT) || (member.nFormat == CLP_CF_OEMTEXT) || (member.nFormat == CLP_CF_UNICODETEXT)) {
        return HANDLE_METHOD_STORE;
    }
    return HANDLE_METHOD_UNKNOWN;
}

QString XCLPArchive::describe(const MEMBER &member)
{
    if (member.nFormat == CLP_CF_TEXT) return QStringLiteral("CF_TEXT");
    if (member.nFormat == CLP_CF_OEMTEXT) return QStringLiteral("CF_OEMTEXT");
    if (member.nFormat == CLP_CF_UNICODETEXT) return QStringLiteral("CF_UNICODETEXT");
    if (member.nFormat == CLP_CF_DIB) return QStringLiteral("CF_DIB");
    if (member.nFormat == CLP_CF_DIBV5) return QStringLiteral("CF_DIBV5");
    return QStringLiteral("format %1 (no file form)").arg(member.nFormat);
}

bool XCLPArchive::isValid(PDSTRUCT *pPdStruct)
{
    // getRecords-style probing displaces the caller's cursor, so snapshot it.
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XCLPArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCLPArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCLPArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCLPArchive(pDevice);
}

QList<QString> XCLPArchive::getSearchSignatures()
{
    return QList<QString>();
}

XBinary::FT XCLPArchive::getFileType()
{
    return FT_CLP;
}

XBinary::MODE XCLPArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCLPArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCLPArchive::getArch()
{
    return QString();
}

qint32 XCLPArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XCLPArchive::getFileFormatExt()
{
    return QStringLiteral("clp");
}

QString XCLPArchive::getFileFormatExtsString()
{
    return QStringLiteral("Windows Clipboard (*.clp)");
}

QString XCLPArchive::getMIMEString()
{
    return QStringLiteral("application/x-clipboard");
}

QString XCLPArchive::getVersion()
{
    return QString();
}

qint64 XCLPArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XCLPArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XCLPArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XCLPArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCLPArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodOf(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, describe(member));
            if (!member.baPrefix.isEmpty()) part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
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

QMap<XBinary::UNPACK_PROP, QVariant> XCLPArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCLPArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCLPArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XCLPArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodOf(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, describe(member));
    if (!member.baPrefix.isEmpty()) result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XCLPArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XCLPArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XCLPArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_COMPRESSPROPERTIES;
}
