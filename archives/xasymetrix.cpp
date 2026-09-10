/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xasymetrix.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// 60 22 13 63 followed by 6C 00 00 00.  The second dword is the directory
// record stride and is a hard constant of the writer, so the gate is a full
// 64-bit compare - the same signature Detect-It-Easy ships as "Asymetrix".
const quint32 ASYMETRIX_MAGIC = 0x63132260U;
const quint32 ASYMETRIX_RECORD_STRIDE = 0x6cU;

const qint64 ASYMETRIX_HEADER_SIZE = 0x2c;
const qint64 ASYMETRIX_SETNAME_OFFSET = 0x08;
const qint64 ASYMETRIX_SETNAME_SIZE = 9;
const qint64 ASYMETRIX_VOLUME_OFFSET = 0x28;
const qint64 ASYMETRIX_COUNT_OFFSET = 0x2a;

// Directory record layout.  The name field is FIXED at 13 bytes and the bytes
// between its NUL and the next field are uninitialised template leftovers that
// frequently look like a plausible DOS date/time pair - every field below is at
// a fixed record offset regardless of how long the name is.
const qint64 ASYMETRIX_RECORD_NAME_SIZE = 13;
const qint64 ASYMETRIX_RECORD_VOLUME = 0x1e;  // high word = volume, low word unknown
const qint64 ASYMETRIX_RECORD_OFFSET = 0x22;  // offset inside the volume named above
const qint64 ASYMETRIX_RECORD_SIZE = 0x26;
const qint64 ASYMETRIX_RECORD_DATE = 0x62;  // DOS date comes BEFORE DOS time here
const qint64 ASYMETRIX_RECORD_TIME = 0x64;
const qint64 ASYMETRIX_RECORD_ATTRIBUTES = 0x66;
const qint64 ASYMETRIX_RECORD_CRC = 0x68;

const qint64 ASYMETRIX_BLOCK_HEADER_SIZE = 6;
const qint64 ASYMETRIX_BLOCK_UNPACKED_SIZE = 4096;
// Every block in the reference corpus is at most 4096 packed bytes; the wider
// bound only keeps a hypothetical expanding block from being misread as a
// structural error while still refusing an attacker-sized allocation.
const qint64 ASYMETRIX_MAX_BLOCK_SIZE = 0x10000;

const quint16 ASYMETRIX_METHOD_STORED = 0;
const quint16 ASYMETRIX_METHOD_IMPLODE = 1;

const qint32 ASYMETRIX_MAX_MEMBERS = 0x4000;
const quint16 ASYMETRIX_MAX_VOLUME = 999;

bool asymetrixRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool asymetrixIsNameCharacter(quint8 nCharacter)
{
    if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    // DOS 8.3 names only; a separator here would let a member escape the
    // extraction folder and never occurs in the format.
    switch (nCharacter) {
        case '\\':
        case '/':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|': return false;
        default: return true;
    }
}

// Reads a NUL-terminated name out of a fixed-width field.  Returns an empty
// string when the field carries no terminator inside its own width or when any
// character before the terminator is not usable as a DOS file name.
//
// nMaximumLength bounds the NAME, not the scan: a name that exactly fills its
// field stores the terminator in the last byte, so the scan has to be allowed
// to reach index nMaximumLength to see it.  Stopping one byte short rejected
// every archive holding a full-width 8.3 name such as "comptr01.ico" - 51 of
// the 56 Asymetrix volume-1 archives in the corpus.
QString asymetrixReadName(const QByteArray &baField, qint32 nMaximumLength)
{
    for (qint32 i = 0; (i < baField.size()) && (i <= nMaximumLength); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if (nCharacter == 0) {
            if (i == 0) return QString();
            return QString::fromLatin1(baField.constData(), i);
        }
        if (!asymetrixIsNameCharacter(nCharacter)) return QString();
    }
    return QString();
}
}  // namespace

XAsymetrix::XAsymetrix(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAsymetrix::~XAsymetrix()
{
}

bool XAsymetrix::measureMemberChain(const CONTEXT &context, qint64 nDataOffset,
                                    qint64 nRegionEnd,
                                    qint64 nUncompressedSize,
                                    qint64 *pnStreamSize, PDSTRUCT *pPdStruct)
{
    if (!pnStreamSize) return false;
    *pnStreamSize = 0;

    QPointer<XAsymetrix> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    if ((nUncompressedSize < 0) || (nDataOffset < context.nDataOffset) ||
        (nRegionEnd > context.nInputSize) || (nDataOffset > nRegionEnd)) {
        return false;
    }

    qint64 nOffset = nDataOffset;
    qint64 nProduced = 0;
    while (nProduced < nUncompressedSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset > nRegionEnd - ASYMETRIX_BLOCK_HEADER_SIZE) return false;

        const QByteArray baBlockHeader =
            read_array_process(nOffset, ASYMETRIX_BLOCK_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baBlockHeader.size() != ASYMETRIX_BLOCK_HEADER_SIZE) {
            return false;
        }
        const uchar *pBlockHeader =
            reinterpret_cast<const uchar *>(baBlockHeader.constData());
        const quint16 nMethod = qFromLittleEndian<quint16>(pBlockHeader);
        const quint32 nBlockSize = qFromLittleEndian<quint32>(pBlockHeader + 2);

        if ((nMethod != ASYMETRIX_METHOD_STORED) &&
            (nMethod != ASYMETRIX_METHOD_IMPLODE)) {
            return false;
        }
        if ((static_cast<qint64>(nBlockSize) > ASYMETRIX_MAX_BLOCK_SIZE) ||
            !asymetrixRangeWithin(nRegionEnd,
                                  nOffset + ASYMETRIX_BLOCK_HEADER_SIZE,
                                  static_cast<qint64>(nBlockSize))) {
            return false;
        }

        // The block's unpacked size is never stored: it is implied to be
        // min(4096, remaining).  A stored block must therefore carry exactly
        // that many bytes - which is also what rejects the 6-zero-byte
        // end-of-volume trailer that marks a member as continuing on the next
        // disk, so a spanning member fails here instead of being truncated.
        const qint64 nWanted = qMin<qint64>(ASYMETRIX_BLOCK_UNPACKED_SIZE,
                                            nUncompressedSize - nProduced);
        if (nMethod == ASYMETRIX_METHOD_STORED) {
            if (static_cast<qint64>(nBlockSize) != nWanted) return false;
        } else if (nBlockSize == 0) {
            return false;
        }

        nProduced += nWanted;
        nOffset += ASYMETRIX_BLOCK_HEADER_SIZE + static_cast<qint64>(nBlockSize);
    }

    *pnStreamSize = nOffset - nDataOffset;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAsymetrix::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAsymetrix> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <
        ASYMETRIX_HEADER_SIZE + ASYMETRIX_BLOCK_HEADER_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, ASYMETRIX_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baHeader.size() != ASYMETRIX_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if ((qFromLittleEndian<quint32>(pHeader) != ASYMETRIX_MAGIC) ||
        (qFromLittleEndian<quint32>(pHeader + 4) != ASYMETRIX_RECORD_STRIDE)) {
        return false;
    }

    context.sSetName = asymetrixReadName(
        baHeader.mid(static_cast<qint32>(ASYMETRIX_SETNAME_OFFSET),
                     static_cast<qint32>(ASYMETRIX_SETNAME_SIZE)),
        static_cast<qint32>(ASYMETRIX_SETNAME_SIZE - 1));
    if (context.sSetName.isEmpty()) return false;

    context.nVolume =
        qFromLittleEndian<quint16>(pHeader + ASYMETRIX_VOLUME_OFFSET);
    context.nDeclaredCount =
        qFromLittleEndian<quint16>(pHeader + ASYMETRIX_COUNT_OFFSET);
    if ((context.nVolume < 1) || (context.nVolume > ASYMETRIX_MAX_VOLUME)) {
        return false;
    }

    // The member count in the header is the count for the whole SET and is
    // repeated verbatim on every volume, but only volume 1 actually carries the
    // directory.  Trusting the count on a continuation volume would parse raw
    // compressed data as file names, so the volume number is the only valid
    // discriminator.
    if (context.nVolume == 1) {
        if ((context.nDeclaredCount < 1) ||
            (context.nDeclaredCount > ASYMETRIX_MAX_MEMBERS)) {
            return false;
        }
        context.nDirectorySize = static_cast<qint64>(context.nDeclaredCount) *
                                 static_cast<qint64>(ASYMETRIX_RECORD_STRIDE);
        context.nDataOffset = ASYMETRIX_HEADER_SIZE + context.nDirectorySize;
    } else {
        context.nDirectorySize = 0;
        context.nDataOffset = ASYMETRIX_HEADER_SIZE;
    }
    if (!asymetrixRangeWithin(context.nInputSize, context.nDataOffset,
                              ASYMETRIX_BLOCK_HEADER_SIZE)) {
        return false;
    }

    if (context.nVolume == 1) {
        const QByteArray baDirectory = read_array_process(
            ASYMETRIX_HEADER_SIZE, context.nDirectorySize, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baDirectory.size() != context.nDirectorySize) {
            return false;
        }
        const uchar *pDirectory =
            reinterpret_cast<const uchar *>(baDirectory.constData());

        quint16 nPreviousVolume = 0;
        for (qint32 i = 0; i < context.nDeclaredCount; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            const qint64 nRecordOffset =
                static_cast<qint64>(i) *
                static_cast<qint64>(ASYMETRIX_RECORD_STRIDE);
            const uchar *pRecord = pDirectory + nRecordOffset;

            MEMBER member = {};
            member.nRecordOffset = ASYMETRIX_HEADER_SIZE + nRecordOffset;
            member.sFileName = asymetrixReadName(
                baDirectory.mid(static_cast<qint32>(nRecordOffset),
                                static_cast<qint32>(ASYMETRIX_RECORD_NAME_SIZE)),
                static_cast<qint32>(ASYMETRIX_RECORD_NAME_SIZE - 1));
            if (member.sFileName.isEmpty()) return false;

            member.nVolume = static_cast<quint16>(
                qFromLittleEndian<quint32>(pRecord + ASYMETRIX_RECORD_VOLUME) >>
                16);
            member.nDataOffset = static_cast<qint64>(
                qFromLittleEndian<quint32>(pRecord + ASYMETRIX_RECORD_OFFSET));
            member.nUncompressedSize = static_cast<qint64>(
                qFromLittleEndian<quint32>(pRecord + ASYMETRIX_RECORD_SIZE));
            member.nDosDate =
                qFromLittleEndian<quint16>(pRecord + ASYMETRIX_RECORD_DATE);
            member.nDosTime =
                qFromLittleEndian<quint16>(pRecord + ASYMETRIX_RECORD_TIME);
            member.nAttributes = qFromLittleEndian<quint16>(
                pRecord + ASYMETRIX_RECORD_ATTRIBUTES);
            member.nCRC32 =
                qFromLittleEndian<quint32>(pRecord + ASYMETRIX_RECORD_CRC);

            // Records are written sorted by (volume, offset).  The order is what
            // lets the next record on the same volume delimit this member's
            // block region in one pass instead of a quadratic rescan.
            if ((member.nVolume < 1) || (member.nVolume < nPreviousVolume)) {
                return false;
            }
            nPreviousVolume = member.nVolume;

            if (i == 0) {
                // The first member must be the first thing after the directory
                // and must live on this volume; no zero-size member exists.
                if ((member.nVolume != context.nVolume) ||
                    (member.nDataOffset != context.nDataOffset) ||
                    (member.nUncompressedSize == 0)) {
                    return false;
                }
            }
            context.listMembers.append(member);
        }

        for (qint32 i = 0; i < context.listMembers.size(); ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            MEMBER &member = context.listMembers[i];
            if (member.nVolume != context.nVolume) continue;

            const bool bNextOnSameVolume =
                (i + 1 < context.listMembers.size()) &&
                (context.listMembers.at(i + 1).nVolume == context.nVolume);
            const qint64 nRegionEnd =
                bNextOnSameVolume ? context.listMembers.at(i + 1).nDataOffset
                                  : context.nInputSize;

            qint64 nStreamSize = 0;
            if (measureMemberChain(context, member.nDataOffset, nRegionEnd,
                                   member.nUncompressedSize, &nStreamSize,
                                   pPdStruct)) {
                member.nStreamSize = nStreamSize;
                member.bComplete = true;
                ++context.nCompleteCount;
            }
            if (!guardedThis || !guardedSource) return false;
        }
    }

    // Final gate: the first block of the data area must be well formed.  On a
    // continuation volume this is the only structural evidence available, and
    // the 00 05 prefix is the PKWARE DCL (literal mode 0, 2048-byte window)
    // header that every compressed block in this format carries.
    const QByteArray baFirstBlock = read_array_process(
        context.nDataOffset, ASYMETRIX_BLOCK_HEADER_SIZE + 2, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baFirstBlock.size() < ASYMETRIX_BLOCK_HEADER_SIZE) {
        return false;
    }
    const uchar *pFirstBlock =
        reinterpret_cast<const uchar *>(baFirstBlock.constData());
    const quint16 nFirstMethod = qFromLittleEndian<quint16>(pFirstBlock);
    const quint32 nFirstBlockSize = qFromLittleEndian<quint32>(pFirstBlock + 2);
    if ((nFirstMethod != ASYMETRIX_METHOD_STORED) &&
        (nFirstMethod != ASYMETRIX_METHOD_IMPLODE)) {
        return false;
    }
    if ((nFirstBlockSize == 0) ||
        (static_cast<qint64>(nFirstBlockSize) > ASYMETRIX_MAX_BLOCK_SIZE) ||
        !asymetrixRangeWithin(context.nInputSize,
                              context.nDataOffset + ASYMETRIX_BLOCK_HEADER_SIZE,
                              static_cast<qint64>(nFirstBlockSize))) {
        return false;
    }
    if (nFirstMethod == ASYMETRIX_METHOD_IMPLODE) {
        if ((baFirstBlock.size() < ASYMETRIX_BLOCK_HEADER_SIZE + 2) ||
            (static_cast<quint8>(
                 baFirstBlock.at(static_cast<qint32>(
                     ASYMETRIX_BLOCK_HEADER_SIZE))) != 0x00) ||
            (static_cast<quint8>(
                 baFirstBlock.at(static_cast<qint32>(
                     ASYMETRIX_BLOCK_HEADER_SIZE + 1))) != 0x05)) {
            return false;
        }
    }

    // The block chain of every volume of this format runs to exactly EOF, so
    // the container occupies the whole file and there is no overlay to split
    // off.
    context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAsymetrix::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XAsymetrix::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAsymetrix archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAsymetrix::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAsymetrix(pDevice);
}

QList<QString> XAsymetrix::getSearchSignatures()
{
    return {QStringLiteral("602213636c000000")};
}

XBinary::FT XAsymetrix::getFileType()
{
    return FT_ASYMETRIX;
}

XBinary::MODE XAsymetrix::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAsymetrix::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAsymetrix::getArch()
{
    return QString();
}

QString XAsymetrix::getFileFormatExt()
{
    return QStringLiteral("001");
}

QString XAsymetrix::getFileFormatExtsString()
{
    return QStringLiteral("Asymetrix Setup (*.001 *.002 *.003)");
}

QString XAsymetrix::getMIMEString()
{
    return QStringLiteral("application/x-asymetrix-setup");
}

QString XAsymetrix::getVersion()
{
    // Deliberately a header-only read: the version string is queried on every
    // identification and must not pay for a full directory and block walk.
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential() ||
        (guardedSource->size() < ASYMETRIX_HEADER_SIZE)) {
        return QString();
    }
    const qint64 nSavedPosition = guardedSource->pos();
    const QByteArray baHeader = read_array(0, ASYMETRIX_HEADER_SIZE);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    if (baHeader.size() != ASYMETRIX_HEADER_SIZE) return QString();
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint32>(pHeader) != ASYMETRIX_MAGIC) return QString();
    return QStringLiteral("Volume %1").arg(
        qFromLittleEndian<quint16>(pHeader + ASYMETRIX_VOLUME_OFFSET));
}

qint64 XAsymetrix::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAsymetrix::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAsymetrix::getMemoryMap(MAPMODE mapMode,
                                              PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XAsymetrix::memberInfoString(const CONTEXT &context,
                                     const MEMBER &member) const
{
    if (member.nVolume != context.nVolume) {
        return tr("Member data begins on volume %1 of disk set '%2'")
            .arg(member.nVolume)
            .arg(context.sSetName);
    }
    if (!member.bComplete) {
        return tr("Member data continues on volume %1 of disk set '%2'")
            .arg(member.nVolume + 1)
            .arg(context.sSetName);
    }
    return QString();
}

bool XAsymetrix::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAsymetrix::getFileParts(quint32 nFileParts,
                                               qint32 nLimit,
                                               PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ASYMETRIX_HEADER_SIZE + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        // A member whose data lives on another volume owns no bytes of this
        // file, and an incomplete one owns bytes that cannot be decoded on
        // their own; neither may be published as a decodable stream, or the
        // extraction path would write a truncated file.
        if (member.nVolume != context.nVolume) continue;

        if ((nFileParts & FILEPART_STREAM) && member.bComplete) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_ASYMETRIX_BLOCKS);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("PKWARE DCL implode blocks"));
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                      CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.bComplete
                                 ? member.nStreamSize
                                 : (context.nInputSize - member.nDataOffset);
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XAsymetrix::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAsymetrix::initUnpack(UNPACK_STATE *pState,
                            const QMap<UNPACK_PROP, QVariant> &mapProperties,
                            PDSTRUCT *pPdStruct)
{
    QPointer<XAsymetrix> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
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
    // A continuation volume is a genuine archive of this family that carries no
    // directory at all, so it opens with zero records.  Refusing it outright
    // would lose the identification; reporting it as an empty archive without
    // saying why is the false-success mode this class exists to avoid, hence
    // the explicit info line below.
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    if (pContext->nVolume == 1) {
        pState->mapArchiveProperties.insert(
            FPART_PROP_INFO,
            tr("Asymetrix disk set '%1', volume 1; %2 of %3 members are "
               "complete in this volume")
                .arg(pContext->sSetName)
                .arg(pContext->nCompleteCount)
                .arg(pContext->listMembers.size()));
    } else {
        pState->mapArchiveProperties.insert(
            FPART_PROP_INFO,
            tr("Asymetrix disk set '%1', volume %2; this volume carries "
               "continuation data only - the member directory is on volume 1")
                .arg(pContext->sSetName)
                .arg(pContext->nVolume));
    }
    pState->nCurrentOffset = pContext->nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;
    if (!pContext->listMembers.isEmpty()) {
        pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    }

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XAsymetrix::infoCurrent(UNPACK_STATE *pState,
                                               PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    const bool bExtractable =
        member.bComplete && (member.nVolume == pContext->nVolume);

    // An unextractable member is still listed with its real name, size and
    // timestamp - the directory for it is complete and correct - but it is
    // published with no stream and no handle method, so the decode path refuses
    // it instead of writing the bytes that happen to be present.
    result.nStreamOffset = bExtractable ? member.nDataOffset : 0;
    result.nStreamSize = bExtractable ? member.nStreamSize : 0;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, member.nRecordOffset);
    result.mapProperties.insert(
        FPART_PROP_HEADER_SIZE,
        static_cast<qint64>(ASYMETRIX_RECORD_STRIDE));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_ISREADONLY,
                                (member.nAttributes & 0x0001U) != 0);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, member.nCRC32);

    if (bExtractable) {
        result.mapProperties.insert(FPART_PROP_STREAMOFFSET,
                                    result.nStreamOffset);
        result.mapProperties.insert(FPART_PROP_STREAMSIZE, result.nStreamSize);
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                    member.nStreamSize);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                    HANDLE_METHOD_ASYMETRIX_BLOCKS);
        result.mapProperties.insert(
            FPART_PROP_REPORTEDMETHOD,
            QStringLiteral("PKWARE DCL implode blocks"));
        // The format's own CRC-32 covers the unpacked member, so handing it to
        // the framework turns every extraction into a verified one.
        result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                    CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    } else {
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                    static_cast<qint64>(0));
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                    HANDLE_METHOD_UNKNOWN);
        result.mapProperties.insert(FPART_PROP_INFO,
                                    memberInfoString(*pContext, member));
    }

    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    return result;
}

bool XAsymetrix::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XAsymetrix::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
