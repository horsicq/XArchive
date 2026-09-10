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
#include "xromfsarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 ROMFS_HEADER_SIZE = 16;
const qint64 ROMFS_FILE_HEADER = 16;
const qint32 ROMFS_MAX_DEPTH = 64;
const qint32 ROMFS_MAX_MEMBERS = 100000;

qint64 romfsAlign(qint64 nValue)
{
    return (nValue + 15) & ~((qint64)15);
}
const qint32 ROMFS_MAX_NAME_SIZE = 4096;

bool romfsRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The name is a fixed-length field, not a C string; only control bytes make it
// implausible.  Trailing NULs do occur and are trimmed.
bool romfsIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = (quint8)baName.at(i);
        if ((nCharacter < 0x20) && (nCharacter != 0)) return false;
    }
    return true;
}
}  // namespace

XRomfsArchive::XRomfsArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRomfsArchive::~XRomfsArchive()
{
}

bool XRomfsArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XRomfsArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ROMFS_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, ROMFS_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != ROMFS_HEADER_SIZE)) return false;
    if (baHeader.left(8) != QByteArray("-rom1fs-")) return false;

    // skip the volume name to reach the root directory's first entry
    qint64 nNameEnd = 16;
    while ((nNameEnd < context.nInputSize) && (nNameEnd < (16 + ROMFS_MAX_NAME_SIZE))) {
        const QByteArray baByte = read_array_process(nNameEnd, 1, pPdStruct);
        if (!guardedThis || !guardedSource || (baByte.size() != 1)) return false;
        if (baByte.at(0) == (char)0) break;
        ++nNameEnd;
    }
    const qint64 nRoot = romfsAlign(nNameEnd + 1);
    if (nRoot >= context.nInputSize) return false;

    QSet<qint64> stSeen;
    if (!walkDirectory(nRoot, QString(), &context, &stSeen, 0, pPdStruct)) return false;

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}


bool XRomfsArchive::walkDirectory(qint64 nOffset, const QString &sPrefix, CONTEXT *pContext, QSet<qint64> *pstSeen, qint32 nDepth, PDSTRUCT *pPdStruct)
{
    if (!pContext || !pstSeen || (nDepth > ROMFS_MAX_DEPTH)) return false;

    QPointer<XRomfsArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    qint64 nCurrent = nOffset;
    while ((nCurrent > 0) && !pstSeen->contains(nCurrent)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (pContext->listMembers.size() >= ROMFS_MAX_MEMBERS) break;
        pstSeen->insert(nCurrent);
        if (!romfsRangeWithin(pContext->nInputSize, nCurrent, ROMFS_FILE_HEADER)) return false;

        const QByteArray baEntry = read_array_process(nCurrent, ROMFS_FILE_HEADER, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != ROMFS_FILE_HEADER)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();

        const quint32 nRawNext = qFromBigEndian<quint32>(pEntry);
        const qint64 nSpec = (qint32)qFromBigEndian<quint32>(pEntry + 4);
        const qint64 nSize = (qint32)qFromBigEndian<quint32>(pEntry + 8);
        const qint32 nType = (qint32)(nRawNext & 7U);
        const qint64 nNext = (qint64)(nRawNext & ~((quint32)15));

        QByteArray baName;
        qint64 nNamePosition = nCurrent + ROMFS_FILE_HEADER;
        while (baName.size() <= ROMFS_MAX_NAME_SIZE) {
            if (nNamePosition >= pContext->nInputSize) return false;
            const QByteArray baByte = read_array_process(nNamePosition, 1, pPdStruct);
            if (!guardedThis || !guardedSource || (baByte.size() != 1)) return false;
            ++nNamePosition;
            if (baByte.at(0) == (char)0) break;
            baName.append(baByte.at(0));
        }
        const QString sName = QString::fromLatin1(baName);
        const qint64 nDataOffset = romfsAlign(nNamePosition);
        const QString sPath = sPrefix.isEmpty() ? sName : (sPrefix + QChar('/') + sName);

        if (nType == 2) {
            if ((nSize >= 0) && romfsRangeWithin(pContext->nInputSize, nDataOffset, nSize)) {
                MEMBER member = {};
                member.nHeaderOffset = nCurrent;
                member.nHeaderSize = nDataOffset - nCurrent;
                member.nDataOffset = nDataOffset;
                member.nCompressedSize = nSize;
                member.nUncompressedSize = nSize;
                member.sFileName = sPath;
                pContext->listMembers.append(member);
            }
        } else if (nType == 1) {
            // "." and ".." point back up the tree; recursing into them loops
            if ((sName != QStringLiteral(".")) && (sName != QStringLiteral("..")) && (nSpec > 0)) {
                if (!walkDirectory(nSpec & ~((qint64)15), sPath, pContext, pstSeen, nDepth + 1, pPdStruct)) return false;
            }
        }

        nCurrent = nNext;
    }

    return true;
}

bool XRomfsArchive::isValid(PDSTRUCT *pPdStruct)
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

bool XRomfsArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRomfsArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRomfsArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRomfsArchive(pDevice);
}

QList<QString> XRomfsArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'-rom1fs-'");
}

XBinary::FT XRomfsArchive::getFileType()
{
    return FT_ROMFS;
}

XBinary::MODE XRomfsArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRomfsArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XRomfsArchive::getArch()
{
    return QString();
}

qint32 XRomfsArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XRomfsArchive::getFileFormatExt()
{
    return QStringLiteral("romfs");
}

QString XRomfsArchive::getFileFormatExtsString()
{
    return QStringLiteral("romfs (*)");
}

QString XRomfsArchive::getMIMEString()
{
    return QStringLiteral("application/x-romfs");
}

QString XRomfsArchive::getVersion()
{
    return QString();
}

qint64 XRomfsArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XRomfsArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XRomfsArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XRomfsArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRomfsArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XRomfsArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRomfsArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XRomfsArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XRomfsArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XRomfsArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XRomfsArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XRomfsArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
