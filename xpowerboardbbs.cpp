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

#include "xpowerboardbbs.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// The extension field is a fixed three bytes, blank-filled when the member has
// no extension.  The base name in front of it is unpadded and 1..8 bytes long,
// so the whole name field is 4..11 bytes.
const qint32 PBBBS_EXT_FIELD = 3;
const qint32 PBBBS_MIN_BASE = 1;
const qint32 PBBBS_MAX_BASE = 8;

// Lead byte encoding.  Adding 10 to the base length is the writer's flag for
// "the size field is a single byte"; without it the size is a 2- or 4-byte
// little-endian value.
const quint8 PBBBS_LEAD_BIAS = 10;

// A Turbo Pascal Integer tops out here; anything larger was written as a
// LongInt.  The reading is what tells the two widths apart when combined with
// the chain walk.
const qint64 PBBBS_INTEGER_MAX = 0x7fff;

// The largest header is 1 lead + 11 name + 4 size.
const qint32 PBBBS_MAX_HEADER = 16;
// Smallest possible record: lead + 4-byte name field + 1-byte size + 1 byte of
// payload.
const qint64 PBBBS_MIN_RECORD = 7;

const qint32 PBBBS_MAX_MEMBERS = 65536;
const qint32 PBBBS_MAX_BACKTRACKS = 4096;
const qint64 PBBBS_MAX_MEMBER_SIZE = 0x40000000;  // 1 GB sanity cap

bool pbbbsRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// DOS-legal 8.3 characters.  The corpus only ever uses letters, digits, space
// padding and " ! $ - _ ", but the rest of the DOS set is accepted so a
// legitimate member cannot be rejected over punctuation.
bool pbbbsIsNameChar(quint8 nCharacter)
{
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;

    switch (nCharacter) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '-':
        case '@':
        case '^':
        case '_':
        case '`':
        case '{':
        case '}':
        case '~':
            return true;
        default:
            return false;
    }
}

// One field of the name: printable DOS characters, then blank padding.  A
// space in the middle of a field means this is not a record header at all,
// which is most of what keeps a random byte stream from chaining.
bool pbbbsCheckField(const char *pData, qint32 nSize, qint32 *pnUsed)
{
    qint32 nUsed = 0;
    bool bPadding = false;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = static_cast<quint8>(pData[i]);

        if (nCharacter == ' ') {
            bPadding = true;
            continue;
        }
        if (bPadding) return false;
        if (!pbbbsIsNameChar(nCharacter)) return false;
        nUsed = i + 1;
    }

    if (pnUsed) *pnUsed = nUsed;

    return true;
}
}  // namespace

XPowerBoardBBS::XPowerBoardBBS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPowerBoardBBS::~XPowerBoardBBS()
{
}

bool XPowerBoardBBS::readHeader(qint64 nOffset, qint64 nInputSize, HEADER *pHeader, PDSTRUCT *pPdStruct)
{
    if (!pHeader) return false;

    *pHeader = HEADER();

    QPointer<XPowerBoardBBS> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    if (!pbbbsRangeWithin(nInputSize, nOffset, PBBBS_MIN_RECORD)) return false;

    qint64 nAvailable = nInputSize - nOffset;
    if (nAvailable > PBBBS_MAX_HEADER) nAvailable = PBBBS_MAX_HEADER;

    const QByteArray baHeader = read_array_process(nOffset, nAvailable, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != nAvailable)) return false;

    const quint8 nLead = static_cast<quint8>(baHeader.at(0));

    qint32 nBaseLength = 0;
    bool bSingleByteSize = false;

    if ((nLead >= (PBBBS_LEAD_BIAS + PBBBS_MIN_BASE)) && (nLead <= (PBBBS_LEAD_BIAS + PBBBS_MAX_BASE))) {
        nBaseLength = static_cast<qint32>(nLead) - PBBBS_LEAD_BIAS;
        bSingleByteSize = true;
    } else if ((nLead >= PBBBS_MIN_BASE) && (nLead <= PBBBS_MAX_BASE)) {
        nBaseLength = static_cast<qint32>(nLead);
    } else {
        return false;
    }

    const qint32 nNameLength = nBaseLength + PBBBS_EXT_FIELD;
    if ((1 + nNameLength) > baHeader.size()) return false;

    const char *pName = baHeader.constData() + 1;

    qint32 nBaseUsed = 0;
    if (!pbbbsCheckField(pName, nBaseLength, &nBaseUsed)) return false;
    if (nBaseUsed == 0) return false;  // a member with no name at all

    qint32 nExtUsed = 0;
    if (!pbbbsCheckField(pName + nBaseLength, PBBBS_EXT_FIELD, &nExtUsed)) return false;

    QString sName = QString::fromLatin1(pName, nBaseUsed);
    if (nExtUsed > 0) {
        sName += QLatin1Char('.');
        sName += QString::fromLatin1(pName + nBaseLength, nExtUsed);
    }

    pHeader->nNameLength = nNameLength;
    pHeader->sFileName = sName;

    const qint64 nSizeOffset = nOffset + 1 + nNameLength;
    const qint32 nSizeIndex = 1 + nNameLength;

    if (bSingleByteSize) {
        if ((nSizeIndex + 1) > baHeader.size()) return false;
        const qint64 nSize = static_cast<qint64>(static_cast<quint8>(baHeader.at(nSizeIndex)));
        if ((nSize >= 1) && pbbbsRangeWithin(nInputSize, nSizeOffset + 1, nSize)) {
            pHeader->nWidth[0] = 1;
            pHeader->nSize[0] = nSize;
            pHeader->nCount = 1;
        }
        return (pHeader->nCount > 0);
    }

    // Prefer the narrow reading: it is the common case, and the walk in
    // parseContext() falls back to the wide one when the tail stops chaining.
    if ((nSizeIndex + 2) <= baHeader.size()) {
        const qint64 nSize =
            static_cast<qint64>(qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baHeader.constData()) + nSizeIndex));
        if ((nSize >= 1) && (nSize <= PBBBS_INTEGER_MAX) && pbbbsRangeWithin(nInputSize, nSizeOffset + 2, nSize)) {
            pHeader->nWidth[pHeader->nCount] = 2;
            pHeader->nSize[pHeader->nCount] = nSize;
            pHeader->nCount++;
        }
    }

    if ((nSizeIndex + 4) <= baHeader.size()) {
        const qint64 nSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData()) + nSizeIndex));
        if ((nSize > PBBBS_INTEGER_MAX) && (nSize <= PBBBS_MAX_MEMBER_SIZE) && pbbbsRangeWithin(nInputSize, nSizeOffset + 4, nSize)) {
            pHeader->nWidth[pHeader->nCount] = 4;
            pHeader->nSize[pHeader->nCount] = nSize;
            pHeader->nCount++;
        }
    }

    return (pHeader->nCount > 0);
}

bool XPowerBoardBBS::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPowerBoardBBS> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PBBBS_MIN_RECORD) return false;

    // Backtrack points: the offset of a record whose size field had a second
    // reading, and how many members were already accepted at that point.
    struct CHOICE {
        qint64 nHeaderOffset;
        qint32 nMemberCount;
        qint32 nWidth;
        qint64 nSize;
        qint32 nNameLength;
        QString sFileName;
    };

    QList<CHOICE> listChoices;
    qint64 nOffset = 0;
    qint32 nBacktracks = 0;

    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        bool bAdvanced = false;

        if (nOffset == context.nInputSize) break;  // the chain landed on EOF

        if (nOffset < context.nInputSize) {
            HEADER header = {};
            if (readHeader(nOffset, context.nInputSize, &header, pPdStruct)) {
                if (!guardedThis || !guardedSource) return false;
                if (context.listMembers.size() >= PBBBS_MAX_MEMBERS) return false;

                if (header.nCount > 1) {
                    CHOICE choice = {};
                    choice.nHeaderOffset = nOffset;
                    choice.nMemberCount = context.listMembers.size();
                    choice.nWidth = header.nWidth[1];
                    choice.nSize = header.nSize[1];
                    choice.nNameLength = header.nNameLength;
                    choice.sFileName = header.sFileName;
                    listChoices.append(choice);
                }

                MEMBER member = {};
                member.nHeaderOffset = nOffset;
                member.nDataOffset = nOffset + 1 + header.nNameLength + header.nWidth[0];
                member.nSize = header.nSize[0];
                member.sFileName = header.sFileName;
                context.listMembers.append(member);

                nOffset = member.nDataOffset + member.nSize;
                bAdvanced = true;
            }
        }

        if (bAdvanced) continue;

        // Dead end - retry the last record that had a second size reading.
        bool bResumed = false;

        while (!listChoices.isEmpty()) {
            if (nBacktracks >= PBBBS_MAX_BACKTRACKS) return false;
            nBacktracks++;

            const CHOICE choice = listChoices.takeLast();

            if (choice.nMemberCount > context.listMembers.size()) return false;
            while (context.listMembers.size() > choice.nMemberCount) {
                context.listMembers.removeLast();
            }

            // The alternative was validated against the file bounds when the
            // header was decoded, so it can be applied directly.
            MEMBER member = {};
            member.nHeaderOffset = choice.nHeaderOffset;
            member.nDataOffset = choice.nHeaderOffset + 1 + choice.nNameLength + choice.nWidth;
            member.nSize = choice.nSize;
            member.sFileName = choice.sFileName;
            context.listMembers.append(member);

            nOffset = member.nDataOffset + member.nSize;
            bResumed = true;
            break;
        }

        if (!bResumed) return false;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return guardedThis && guardedSource;
}

bool XPowerBoardBBS::isValid(PDSTRUCT *pPdStruct)
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

bool XPowerBoardBBS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPowerBoardBBS archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XPowerBoardBBS::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XPowerBoardBBS(pDevice);
}

QList<QString> XPowerBoardBBS::getSearchSignatures()
{
    // Headerless: the first byte is a length, not a magic.  Any signature
    // short enough to match would match half the world, so publish none and
    // let the full-file chain walk in isValid() do the deciding.
    return QList<QString>();
}

XBinary::FT XPowerBoardBBS::getFileType()
{
    return FT_POWERBOARD_BBS;
}

XBinary::MODE XPowerBoardBBS::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPowerBoardBBS::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPowerBoardBBS::getArch()
{
    return QString();
}

QString XPowerBoardBBS::getFileFormatExt()
{
    return QStringLiteral("bbs");
}

QString XPowerBoardBBS::getFileFormatExtsString()
{
    return QStringLiteral("Powerboard BBS library (*.bbs *.dat *.exp *.ltr)");
}

QString XPowerBoardBBS::getMIMEString()
{
    return QStringLiteral("application/x-powerboard-bbs");
}

QString XPowerBoardBBS::getVersion()
{
    // The container carries no version word of any kind.
    return QString();
}

qint64 XPowerBoardBBS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPowerBoardBBS::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPowerBoardBBS::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XPowerBoardBBS::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPowerBoardBBS::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    const qint32 nNumberOfMembers = context.listMembers.size();

    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;

        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nDataOffset - member.nHeaderOffset;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store"));
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = (member.nDataOffset - member.nHeaderOffset) + member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
        // parseContext() only accepts a chain that ends exactly on EOF, so
        // this cannot fire today; it is kept so the part list stays correct
        // if the acceptance rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XPowerBoardBBS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPowerBoardBBS::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XPowerBoardBBS> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Powerboard BBS library; stored members"));
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

XBinary::ARCHIVERECORD XPowerBoardBBS::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XPowerBoardBBS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }

    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XPowerBoardBBS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
