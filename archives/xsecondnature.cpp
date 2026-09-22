/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsecondnature.h"

#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// "Second Nature Software Inc. " followed by a three-letter kind and a NUL
// terminator.  Thirty-two bytes in total on every known sample.
const char SN_BANNER_TEXT[] = "Second Nature Software Inc. ";
const qint32 SN_BANNER_TEXT_SIZE = 28;
const qint64 SN_BANNER_SIZE = 32;
const qint32 SN_KIND_OFFSET = 28;
const qint32 SN_KIND_SIZE = 3;

// Fixed 8.3 name buffer: "LASTGRAT.JIF" fills twelve characters and puts
// its NUL in the thirteenth byte, so the field must be read at full width.
const qint32 SN_NAME_FIELD = 13;

// BMX: u16 count at 0x20, then count entries of name[13] + u32 + u32.
const qint64 SN_BMX_COUNT_OFFSET = 0x20;
const qint64 SN_BMX_DIR_OFFSET = 0x22;
const qint64 SN_BMX_ENTRY_SIZE = 21;
const qint32 SN_BMX_MAX_MEMBERS = 4096;

// REF: two u16 counts (text members, image members) at 0x20 and 0x22, then
// entries of name[13] + u16 kind + u32 offset + u32 size + u16 rect[4].
// The table is padded to six slots on every known sample, which is why
// member data starts at 222; that constant is not assumed here - the
// directory is only required to end at or before the first member.
const qint64 SN_REF_DIR_OFFSET = 0x24;
const qint64 SN_REF_ENTRY_SIZE = 31;
const qint32 SN_REF_MAX_MEMBERS = 64;

// SNX: description block, then u16 sequence id, u16 width, u16 height at
// 0x160 and exactly three BMX-shaped slots at 0x166.  There is no count
// field; empty slots are recognised by an empty name or a zero size.
const qint64 SN_SNX_INFO_OFFSET = 0x160;
const qint64 SN_SNX_DIR_OFFSET = 0x166;
const qint64 SN_SNX_ENTRY_SIZE = 21;
const qint32 SN_SNX_SLOTS = 3;
const qint64 SN_SNX_TITLE_OFFSET = 0x20;
const qint32 SN_SNX_TITLE_SIZE = 64;

const qint64 SN_MAX_MEMBER_SIZE = 0x10000000;  // 256 MB sanity cap
const qint64 SN_MAX_HEADER_READ = 0x4000;

// Second Nature ".JIF" picture header.  It always begins with 0x18 and
// carries the tag 18 30 00 31 at +16; the JFIF stream follows it.  Two
// header sizes exist (42 in .REF/.BMX, 554 in .SNX, the difference being a
// 512-byte 128x96 preview block), so the SOI marker is located inside a
// bounded window instead of being assumed at a fixed distance.
const qint32 SN_JIF_MIN_HEADER = 20;
const qint32 SN_JIF_TAG_OFFSET = 16;
const quint8 SN_JIF_LEAD_BYTE = 0x18U;
const char SN_JIF_TAG[4] = {'\x18', '\x30', '\x00', '\x31'};
const qint32 SN_JIF_SCAN_WINDOW = 1024;

bool snRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Fixed-width NUL-padded field: the name ends at the first terminator and
// whatever follows is stale buffer content.  "2080.JIF\0JIF\0" is a real
// corpus entry - the trailing "JIF" is the tail of a longer previous name.
QByteArray snFixedName(const QByteArray &baField)
{
    const qint32 nIndex = baField.indexOf('\0');
    return (nIndex < 0) ? baField : baField.left(nIndex);
}

bool snIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || baName.size() >= SN_NAME_FIELD) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
        if ((c == '/') || (c == '\\') || (c == ':') || (c == '*') ||
            (c == '?') || (c == '"') || (c == '<') || (c == '>') ||
            (c == '|')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XSecondNature::XSecondNature(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSecondNature::~XSecondNature()
{
}

QByteArray XSecondNature::readHeaderBlock(qint64 nOffset, qint64 nSize,
                                          bool bComplemented,
                                          PDSTRUCT *pPdStruct)
{
    if (nSize <= 0 || nSize > SN_MAX_HEADER_READ) return QByteArray();
    QByteArray baResult = read_array_process(nOffset, nSize, pPdStruct);
    if (baResult.size() != nSize) return QByteArray();
    if (bComplemented) {
        // The whole header/directory region of .SNX and .REF is stored as
        // its one's complement; member payloads are not.
        for (qint32 i = 0; i < baResult.size(); ++i) {
            baResult[i] = static_cast<char>(
                static_cast<quint8>(~static_cast<quint8>(baResult.at(i))));
        }
    }
    return baResult;
}

bool XSecondNature::resolveStream(MEMBER *pMember, qint64 nInputSize,
                                  PDSTRUCT *pPdStruct)
{
    if (!pMember) return false;

    pMember->bImage = false;
    if (pMember->nMemberOffset >= nInputSize) {
        // Truncated archive: the reference extractor creates an empty file
        // for such a member and reports a failure for the archive.
        pMember->nStreamOffset = nInputSize;
        pMember->nStreamSize = 0;
        return true;
    }

    qint64 nAvailable = pMember->nMemberSize;
    if (nAvailable > nInputSize - pMember->nMemberOffset) {
        nAvailable = nInputSize - pMember->nMemberOffset;
    }
    pMember->nStreamOffset = pMember->nMemberOffset;
    pMember->nStreamSize = nAvailable;
    if (nAvailable < SN_JIF_MIN_HEADER) return true;

    qint64 nProbeSize = nAvailable;
    if (nProbeSize > SN_JIF_SCAN_WINDOW) nProbeSize = SN_JIF_SCAN_WINDOW;
    const QByteArray baProbe =
        read_array_process(pMember->nMemberOffset, nProbeSize, pPdStruct);
    if (baProbe.size() != nProbeSize) return false;

    if (static_cast<quint8>(baProbe.at(0)) != SN_JIF_LEAD_BYTE) return true;
    if (std::memcmp(baProbe.constData() + SN_JIF_TAG_OFFSET, SN_JIF_TAG,
                    sizeof(SN_JIF_TAG)) != 0) {
        return true;
    }

    const qint32 nSOI = baProbe.indexOf(QByteArray("\xff\xd8\xff", 3));
    if (nSOI < 0) return true;

    pMember->bImage = true;
    pMember->nStreamOffset = pMember->nMemberOffset + nSOI;
    pMember->nStreamSize = nAvailable - nSOI;
    return true;
}

bool XSecondNature::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SN_BANNER_SIZE + SN_BMX_ENTRY_SIZE) return false;

    const QByteArray baBanner =
        read_array_process(0, SN_BANNER_SIZE, pPdStruct);
    if (!guardedSource || baBanner.size() != SN_BANNER_SIZE) {
        return false;
    }

    QByteArray baPlainBanner = baBanner;
    if (std::memcmp(baPlainBanner.constData(), SN_BANNER_TEXT,
                    SN_BANNER_TEXT_SIZE) == 0) {
        context.bComplemented = false;
    } else {
        for (qint32 i = 0; i < baPlainBanner.size(); ++i) {
            baPlainBanner[i] = static_cast<char>(static_cast<quint8>(
                ~static_cast<quint8>(baPlainBanner.at(i))));
        }
        if (std::memcmp(baPlainBanner.constData(), SN_BANNER_TEXT,
                        SN_BANNER_TEXT_SIZE) != 0) {
            return false;
        }
        context.bComplemented = true;
    }
    // The banner is exactly 32 bytes: the literal, a three-letter kind and
    // a terminator.
    if (baPlainBanner.at(SN_BANNER_SIZE - 1) != '\0') return false;

    const QByteArray baKind =
        baPlainBanner.mid(SN_KIND_OFFSET, SN_KIND_SIZE);
    if (baKind == QByteArray("SNX", 3)) {
        context.kind = KIND_SNX;
    } else if (baKind == QByteArray("REF", 3)) {
        context.kind = KIND_REF;
    } else if (baKind == QByteArray("BMX", 3)) {
        context.kind = KIND_BMX;
    } else {
        return false;
    }

    qint64 nDirOffset = 0;
    qint64 nEntrySize = 0;
    qint32 nCount = 0;

    if (context.kind == KIND_BMX) {
        const QByteArray baCount = readHeaderBlock(
            SN_BMX_COUNT_OFFSET, 2, context.bComplemented, pPdStruct);
        if (!guardedSource || baCount.size() != 2) return false;
        nCount = static_cast<qint32>(qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar *>(baCount.constData())));
        if (nCount < 1 || nCount > SN_BMX_MAX_MEMBERS) return false;
        nDirOffset = SN_BMX_DIR_OFFSET;
        nEntrySize = SN_BMX_ENTRY_SIZE;
    } else if (context.kind == KIND_REF) {
        const QByteArray baCounts = readHeaderBlock(
            SN_BMX_COUNT_OFFSET, 4, context.bComplemented, pPdStruct);
        if (!guardedSource || baCounts.size() != 4) return false;
        const uchar *pCounts =
            reinterpret_cast<const uchar *>(baCounts.constData());
        // Two counts: text members first, then image members.  They are
        // stored in that order but the entries themselves follow the same
        // order, so the sum is the entry count.
        const qint32 nTextCount =
            static_cast<qint32>(qFromLittleEndian<quint16>(pCounts));
        const qint32 nImageCount =
            static_cast<qint32>(qFromLittleEndian<quint16>(pCounts + 2));
        if (nTextCount < 0 || nTextCount > SN_REF_MAX_MEMBERS) return false;
        if (nImageCount < 0 || nImageCount > SN_REF_MAX_MEMBERS) return false;
        nCount = nTextCount + nImageCount;
        if (nCount < 1 || nCount > SN_REF_MAX_MEMBERS) return false;
        nDirOffset = SN_REF_DIR_OFFSET;
        nEntrySize = SN_REF_ENTRY_SIZE;
    } else {
        nCount = SN_SNX_SLOTS;
        nDirOffset = SN_SNX_DIR_OFFSET;
        nEntrySize = SN_SNX_ENTRY_SIZE;
    }

    const qint64 nDirSize = static_cast<qint64>(nCount) * nEntrySize;
    if (!snRangeWithin(context.nInputSize, nDirOffset, nDirSize)) return false;

    const QByteArray baDir = readHeaderBlock(nDirOffset, nDirSize,
                                             context.bComplemented, pPdStruct);
    if (!guardedSource || baDir.size() != nDirSize) {
        return false;
    }

    context.nDirectoryOffset = nDirOffset;
    context.nDirectorySize = nDirSize;

    if (context.kind == KIND_SNX) {
        const QByteArray baInfo = readHeaderBlock(
            SN_SNX_INFO_OFFSET, 6, context.bComplemented, pPdStruct);
        if (!guardedSource || baInfo.size() != 6) return false;
        const uchar *pInfo = reinterpret_cast<const uchar *>(baInfo.constData());
        context.nSequenceId = qFromLittleEndian<quint16>(pInfo);
        context.nWidth = qFromLittleEndian<quint16>(pInfo + 2);
        context.nHeight = qFromLittleEndian<quint16>(pInfo + 4);

        const QByteArray baTitle =
            readHeaderBlock(SN_SNX_TITLE_OFFSET, SN_SNX_TITLE_SIZE,
                            context.bComplemented, pPdStruct);
        if (!guardedSource) return false;
        if (baTitle.size() == SN_SNX_TITLE_SIZE) {
            const QByteArray baTrimmed = snFixedName(baTitle);
            bool bPrintable = true;
            for (char c : baTrimmed) {
                const quint8 nCharacter = static_cast<quint8>(c);
                if (nCharacter < 0x20 || nCharacter > 0x7e) bPrintable = false;
            }
            if (bPrintable) context.sTitle = QString::fromLatin1(baTrimmed);
        }
    }

    qint64 nFirstMemberOffset = context.nInputSize;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nEntry = static_cast<qint32>(i * nEntrySize);

        MEMBER member = {};
        member.nRecordOffset = nDirOffset + i * nEntrySize;

        const QByteArray baName =
            snFixedName(baDir.mid(nEntry, SN_NAME_FIELD));
        const uchar *pEntry =
            reinterpret_cast<const uchar *>(baDir.constData()) + nEntry;

        qint32 nValueOffset = SN_NAME_FIELD;
        if (context.kind == KIND_REF) {
            member.nKind = qFromLittleEndian<quint16>(pEntry + SN_NAME_FIELD);
            nValueOffset = SN_NAME_FIELD + 2;
        }
        member.nMemberOffset =
            static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + nValueOffset));
        member.nMemberSize = static_cast<qint64>(
            qFromLittleEndian<quint32>(pEntry + nValueOffset + 4));

        if (baName.isEmpty() && member.nMemberOffset == 0 &&
            member.nMemberSize == 0) {
            // Unused slot.  .SNX pads its table to three; .REF slots that
            // merely name a companion file carry a zero offset and size.
            continue;
        }
        if (!snIsValidName(baName)) {
            // A .SNX slot may name a companion module with no payload; a
            // malformed name anywhere else means this is not our format.
            if (context.kind == KIND_SNX && member.nMemberSize == 0) continue;
            return false;
        }
        if (member.nMemberSize == 0) continue;  // reference-only slot
        if (member.nMemberSize > SN_MAX_MEMBER_SIZE) return false;
        // Offsets are absolute and must land past the directory.  They are
        // deliberately NOT required to land inside the file: truncated
        // corpus samples keep their original directory and the reference
        // extractor lists them before failing on the data.
        if (member.nMemberOffset < nDirOffset + nDirSize) return false;

        member.sFileName = QString::fromLatin1(baName);
        if (member.nMemberOffset < nFirstMemberOffset) {
            nFirstMemberOffset = member.nMemberOffset;
        }
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;
    // The directory must not overlap the payload area.
    if (nFirstMemberOffset < nDirOffset + nDirSize) return false;

    qint64 nArchiveEnd = nDirOffset + nDirSize;
    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER &member = context.listMembers[i];
        if (!resolveStream(&member, context.nInputSize, pPdStruct) ||
            !guardedSource) {
            return false;
        }
        qint64 nEnd = member.nMemberOffset + member.nMemberSize;
        if (nEnd > context.nInputSize) nEnd = context.nInputSize;
        if (nEnd > nArchiveEnd) nArchiveEnd = nEnd;
    }

    context.nArchiveSize = nArchiveEnd;
    *pContext = context;
    return guardedSource != nullptr;
}

bool XSecondNature::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSecondNature::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSecondNature archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSecondNature::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSecondNature(pDevice);
}

QList<QString> XSecondNature::getSearchSignatures()
{
    // Plain form (.BMX) and the one's-complement form (.SNX / .REF).
    return {QStringLiteral("'Second Nature Software Inc. '"),
            QStringLiteral("AC9A9C90919BDFB19E8B8A8D9ADFAC90998B889E8D9ADFB6919CD1DF")};
}

XBinary::FT XSecondNature::getFileType()
{
    return FT_SECOND_NATURE;
}

XBinary::MODE XSecondNature::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSecondNature::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSecondNature::getArch()
{
    return QString();
}

QString XSecondNature::getFileFormatExt()
{
    return QStringLiteral("snx");
}

QString XSecondNature::getFileFormatExtsString()
{
    return QStringLiteral(
        "Second Nature screen saver archive (*.snx *.ref *.bmx)");
}

QString XSecondNature::getMIMEString()
{
    return QStringLiteral("application/x-second-nature");
}

QString XSecondNature::getVersion()
{
    // The container carries no version field; the three-letter kind in the
    // banner is the only discriminator and it is reported through
    // FPART_PROP_INFO instead.
    return QString();
}

qint64 XSecondNature::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSecondNature::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSecondNature::getMemoryMap(MAPMODE mapMode,
                                                 PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XSecondNature::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XSecondNature::getFileParts(quint32 nFileParts,
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
        part.nFileSize = context.nDirectoryOffset + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nStreamOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = context.kind == KIND_REF ? SN_REF_ENTRY_SIZE
                                                      : SN_BMX_ENTRY_SIZE;
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
    if ((nFileParts & FILEPART_OVERLAY) &&
        context.nArchiveSize < context.nInputSize &&
        canAppendPart(nLimit, result.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XSecondNature::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSecondNature::initUnpack(UNPACK_STATE *pState,
                               const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    QString sInfo = QStringLiteral("Second Nature Software Inc. ");
    if (pContext->kind == KIND_SNX) {
        sInfo += QStringLiteral("SNX picture module");
    } else if (pContext->kind == KIND_REF) {
        sInfo += QStringLiteral("REF reference module");
    } else {
        sInfo += QStringLiteral("BMX splash module");
    }
    if (!pContext->sTitle.isEmpty()) {
        sInfo += QStringLiteral(" (") + pContext->sTitle + QStringLiteral(")");
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XSecondNature::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nRecordOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XSecondNature::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSecondNature::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
