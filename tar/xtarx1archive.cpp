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
#include "xtarx1archive.h"

#include "Algos/xtarx1decoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 TARX_MAGIC_SIZE = 4;
const qint64 TARX_KEY_HEADER_SIZE = 100;
const qint64 TARX_MIN_SIZE = 0x68;
const qint64 TARX_BLOCK_SIZE = 512;
const qint32 TARX_MAX_MEMBERS = 100000;
const qint32 TARX_NAME_SIZE = 100;
// The plaintext has to be resident to walk the tar, and it is exactly as long
// as the container.
const qint64 TARX_MAX_INPUT_SIZE = 0x20000000;

// A tar octal field: NUL-terminated, space padded.  -1 marks a field that is
// not a plain octal number, which the reference treats as the end of the walk
// rather than as a zero.
qint64 tarxOctal(const quint8 *pField, qint32 nSize)
{
    qint32 nStart = 0;
    qint32 nEnd = 0;
    while ((nEnd < nSize) && (pField[nEnd] != 0)) ++nEnd;
    while ((nStart < nEnd) && ((pField[nStart] == ' ') || (pField[nStart] == '\t') || (pField[nStart] == '\r') || (pField[nStart] == '\n'))) ++nStart;
    while ((nEnd > nStart) && ((pField[nEnd - 1] == ' ') || (pField[nEnd - 1] == '\t') || (pField[nEnd - 1] == '\r') || (pField[nEnd - 1] == '\n'))) --nEnd;
    if (nStart == nEnd) return 0;

    qint64 nValue = 0;
    for (qint32 i = nStart; i < nEnd; ++i) {
        if ((pField[i] < '0') || (pField[i] > '7')) return -1;
        if (nValue > ((qint64)0x7fffffffffffLL)) return -1;
        nValue = (nValue * 8) + (pField[i] - '0');
    }

    return nValue;
}
}  // namespace

XTARX1Archive::XTARX1Archive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTARX1Archive::~XTARX1Archive()
{
}

QByteArray XTARX1Archive::skipToProperty(qint64 nSkipSize)
{
    QByteArray baResult(8, (char)0);
    qToLittleEndian<quint64>((quint64)nSkipSize, (uchar *)baResult.data());
    return baResult;
}

bool XTARX1Archive::parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTARX1Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TARX_MIN_SIZE) || (context.nInputSize > TARX_MAX_INPUT_SIZE)) return false;
    // The 4-byte magic in front of a 512-byte-blocked tar is what leaves this
    // remainder, and it is the container's only structural invariant.
    if ((context.nInputSize & (TARX_BLOCK_SIZE - 1)) != TARX_MAGIC_SIZE) return false;

    const QByteArray baMagic = read_array_process(0, TARX_MAGIC_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baMagic.size() != TARX_MAGIC_SIZE)) return false;
    if (baMagic != QByteArray("TaRx", 4)) return false;

    const QByteArray baKeyHeader = read_array_process(TARX_MAGIC_SIZE, TARX_KEY_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baKeyHeader.size() != TARX_KEY_HEADER_SIZE)) return false;
    if (!XTARX1Decoder::recoverKey(baKeyHeader, &context.nKey)) return false;

    context.nStreamOffset = TARX_MAGIC_SIZE;
    context.nStreamSize = context.nInputSize - TARX_MAGIC_SIZE;

    if (bWalkMembers) {
        const QByteArray baCipher = read_array_process(context.nStreamOffset, context.nStreamSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baCipher.size() != context.nStreamSize)) return false;

        QByteArray baPlain;
        if (!XTARX1Decoder::decrypt(baCipher, context.nKey, &baPlain, pPdStruct)) return false;
        if (baPlain.size() != context.nStreamSize) return false;

        const quint8 *pPlain = (const quint8 *)baPlain.constData();
        const qint64 nPlainSize = baPlain.size();
        qint64 nPosition = 0;

        while ((nPosition + TARX_BLOCK_SIZE) <= nPlainSize) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (context.listMembers.size() >= TARX_MAX_MEMBERS) break;

            const quint8 *pBlock = pPlain + nPosition;
            bool bEmpty = true;
            for (qint32 i = 0; i < TARX_BLOCK_SIZE; ++i) {
                if (pBlock[i] != 0) {
                    bEmpty = false;
                    break;
                }
            }
            if (bEmpty) break;

            const qint64 nWanted = tarxOctal(pBlock + 148, 8);
            qint64 nSum = 256;
            for (qint32 i = 0; i < 148; ++i) nSum += pBlock[i];
            for (qint32 i = 156; i < TARX_BLOCK_SIZE; ++i) nSum += pBlock[i];
            if ((nWanted < 0) || (nSum != nWanted)) break;

            qint64 nSize = tarxOctal(pBlock + 124, 12);
            if (nSize < 0) break;
            const quint8 nTypeFlag = pBlock[156];

            qint32 nNameLength = 0;
            while ((nNameLength < TARX_NAME_SIZE) && (pBlock[nNameLength] != 0)) ++nNameLength;
            const QByteArray baName((const char *)pBlock, nNameLength);

            const qint64 nHeaderPosition = nPosition;
            nPosition += TARX_BLOCK_SIZE;

            // A hard or symbolic link carries a size field that describes the
            // target, not a payload; taking it at face value walks off the end
            // of the following header.
            if ((nTypeFlag == '1') || (nTypeFlag == '2')) nSize = 0;

            if (((nTypeFlag == 0) || (nTypeFlag == '0')) && !baName.endsWith('/')) {
                // A truncated container yields a SHORT last member rather than
                // no member at all, which is what the reference produces; the
                // walk then ends on the next round because the padded advance
                // below carries the cursor past the end.
                const qint64 nStored = qMin(nSize, nPlainSize - nPosition);
                MEMBER member = {};
                member.nHeaderOffset = context.nStreamOffset + nHeaderPosition;
                member.nHeaderSize = TARX_BLOCK_SIZE;
                member.nSkipSize = nPosition;
                member.nUncompressedSize = nStored;
                member.sFileName = QString::fromLatin1(baName);
                context.listMembers.append(member);
            }

            nPosition += ((nSize + (TARX_BLOCK_SIZE - 1)) / TARX_BLOCK_SIZE) * TARX_BLOCK_SIZE;
        }

        if (context.listMembers.isEmpty()) return false;
    }

    *pContext = context;

    return true;
}

bool XTARX1Archive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTARX1Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTARX1Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTARX1Archive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTARX1Archive(pDevice);
}

QList<QString> XTARX1Archive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'TaRx'");
}

XBinary::FT XTARX1Archive::getFileType()
{
    return FT_TARX1;
}

XBinary::MODE XTARX1Archive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTARX1Archive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTARX1Archive::getArch()
{
    return QString();
}

qint32 XTARX1Archive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTARX1Archive::getFileFormatExt()
{
    return QStringLiteral("tarx");
}

QString XTARX1Archive::getFileFormatExtsString()
{
    return QStringLiteral("TARX (*.tarx)");
}

QString XTARX1Archive::getMIMEString()
{
    return QStringLiteral("application/x-tarx");
}

QString XTARX1Archive::getVersion()
{
    return QStringLiteral("1");
}

qint64 XTARX1Archive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTARX1Archive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTARX1Archive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XTARX1Archive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTARX1Archive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TARX_MAGIC_SIZE;
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
            part.nFileOffset = context.nStreamOffset;
            part.nFileSize = context.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TARX1);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("TARX cipher"));
            part.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, skipToProperty(member.nSkipSize));
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

QMap<XBinary::UNPACK_PROP, QVariant> XTARX1Archive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTARX1Archive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTARX1Archive> guardedThis(this);
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

XBinary::ARCHIVERECORD XTARX1Archive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TARX1);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("TARX cipher"));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, skipToProperty(member.nSkipSize));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTARX1Archive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XTARX1Archive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTARX1Archive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_ENCRYPTED << FPART_PROP_COMPRESSPROPERTIES;
}
