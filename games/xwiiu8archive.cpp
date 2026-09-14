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
#include "xwiiu8archive.h"

#include <QCryptographicHash>
#include <QHash>
#include <QPointer>
#include <QSet>

#include <limits>
#include <new>

namespace {
const qint64 U8_HEADER_SIZE = 0x20;
const qint64 U8_NODE_SIZE = 12;
const quint32 U8_MAGIC = 0x55AA382DU;  // "U.8-"
const quint32 U8_ROOT_NODE_OFFSET = 0x20U;
const quint8 U8_NODE_TYPE_FILE = 0x00U;
const quint8 U8_NODE_TYPE_DIRECTORY = 0x01U;
// Bomb guards: the node count and the header size come out of the file.  Real
// Wii archives have at most a few thousand nodes; 100000 matches the
// MAX_RECORDS figure of the other store-only readers in this folder.
const qint64 U8_MAX_NODES = 100000;
const qint64 U8_MAX_HEADER_SIZE = Q_INT64_C(16) * 1024 * 1024;
// Minimum value of the header-size field: root node (12) + the root's empty
// name (1).  The 32-byte fixed header is added at the use sites (V1).
const qint64 U8_MIN_HEADER_SIZE = U8_NODE_SIZE + 1;
const qint32 U8_MAX_NAME_BYTES = 255;
const qint32 U8_MAX_PATH_CHARS = 4096;
// The probe covers every wrapper magic the gate looks at: "IMET" at 0x80 is
// the farthest, so 0x84 bytes are enough to choose the shape.
const qint64 U8_PROBE_SIZE = 0x84;
const quint32 IMD5_MAGIC = 0x494D4435U;  // "IMD5"
const qint64 IMD5_HEADER_SIZE = 0x20;
const qint64 IMD5_DIGEST_OFFSET = 0x10;
const quint32 IMET_MAGIC = 0x494D4554U;  // "IMET"
const qint64 IMET_HASHED_SIZE = 0x600;
const quint32 IMET_HASH_SIZE_FIELD = 0x600U;
const qint64 IMET_SHORT_MAGIC_OFFSET = 0x40;  // disc opening.bnr -> U8 at 0x600
const qint64 IMET_LONG_MAGIC_OFFSET = 0x80;   // NAND 00000000.app -> U8 at 0x640
const qint64 IMET_TITLE_COUNT = 10;
const qint64 IMET_TITLE_BYTES = 84;
// Field offsets relative to the "IMET" magic.
const qint64 IMET_FIELD_HASH_SIZE = 0x04;
const qint64 IMET_FIELD_VERSION = 0x08;
const qint64 IMET_FIELD_ICON_SIZE = 0x0C;
const qint64 IMET_FIELD_BANNER_SIZE = 0x10;
const qint64 IMET_FIELD_SOUND_SIZE = 0x14;
const qint64 IMET_FIELD_FLAGS = 0x18;
const qint64 IMET_FIELD_TITLES = 0x1C;
const qint64 IMET_TITLE_INDEX_JAPANESE = 0;
const qint64 IMET_TITLE_INDEX_ENGLISH = 1;
const qint64 IMET_DIGEST_SIZE = 16;
const qint32 U8_CANCEL_CHECK_MASK = 0xFF;

struct DIRECTORY_FRAME {
    qint64 nEndIndex;
    QString sPath;
};

quint32 u8ReadBE32(const uchar *pData)
{
    return (static_cast<quint32>(pData[0]) << 24) |
           (static_cast<quint32>(pData[1]) << 16) |
           (static_cast<quint32>(pData[2]) << 8) |
           static_cast<quint32>(pData[3]);
}

quint32 u8ReadBE24(const uchar *pData)
{
    return (static_cast<quint32>(pData[0]) << 16) |
           (static_cast<quint32>(pData[1]) << 8) |
           static_cast<quint32>(pData[2]);
}

quint16 u8ReadBE16(const uchar *pData)
{
    return static_cast<quint16>((static_cast<quint16>(pData[0]) << 8) |
                                static_cast<quint16>(pData[1]));
}

bool u8RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool u8CheckedAdd(qint64 nLeft, qint64 nRight, qint64 *pnResult)
{
    if (!pnResult || (nLeft < 0) || (nRight < 0) ||
        (nRight > ((std::numeric_limits<qint64>::max)() - nLeft))) {
        return false;
    }
    *pnResult = nLeft + nRight;
    return true;
}

bool u8HasMagicAt(const QByteArray &baProbe, qint64 nOffset, quint32 nMagic)
{
    if ((nOffset < 0) || (baProbe.size() < (nOffset + 4))) return false;
    const uchar *pData =
        reinterpret_cast<const uchar *>(baProbe.constData()) + nOffset;
    return u8ReadBE32(pData) == nMagic;
}

// Strict UTF-8 (no overlongs, no surrogates, nothing above U+10FFFF).  A name
// that fails this is decoded as Latin-1 instead; it is never a reject.
bool u8IsValidUtf8(const uchar *pData, qint64 nLength)
{
    qint64 i = 0;
    while (i < nLength) {
        const uchar nLead = pData[i];
        qint64 nExtra = 0;
        if (nLead < 0x80U) {
            ++i;
            continue;
        } else if ((nLead & 0xE0U) == 0xC0U) {
            if (nLead < 0xC2U) return false;
            nExtra = 1;
        } else if ((nLead & 0xF0U) == 0xE0U) {
            nExtra = 2;
        } else if ((nLead & 0xF8U) == 0xF0U) {
            if (nLead > 0xF4U) return false;
            nExtra = 3;
        } else {
            return false;
        }
        if ((nLength - i) <= nExtra) return false;
        for (qint64 j = 1; j <= nExtra; ++j) {
            if ((pData[i + j] & 0xC0U) != 0x80U) return false;
        }
        const uchar nSecond = pData[i + 1];
        if (nExtra == 2) {
            if ((nLead == 0xE0U) && (nSecond < 0xA0U)) return false;
            if ((nLead == 0xEDU) && (nSecond >= 0xA0U)) return false;
        } else if (nExtra == 3) {
            if ((nLead == 0xF0U) && (nSecond < 0x90U)) return false;
            if ((nLead == 0xF4U) && (nSecond >= 0x90U)) return false;
        }
        i += nExtra + 1;
    }
    return true;
}

// Reads one NUL-terminated pool entry and applies the name rules.  Returns
// false only for the hard rejects (unterminated, too long, illegal bytes,
// "." / ".."); an empty name is returned as an empty string for the caller
// to substitute.
bool u8DecodeName(const uchar *pPool, qint64 nPoolSize, qint64 nNameOffset,
                  QString *psName)
{
    if (!pPool || !psName || (nNameOffset < 0) || (nNameOffset >= nPoolSize)) {
        return false;
    }
    qint64 nLength = 0;
    while (((nNameOffset + nLength) < nPoolSize) &&
           pPool[nNameOffset + nLength]) {
        ++nLength;
        if (nLength > U8_MAX_NAME_BYTES) return false;
    }
    // The terminator has to exist inside the pool.
    if ((nNameOffset + nLength) >= nPoolSize) return false;

    const uchar *pName = pPool + nNameOffset;
    for (qint64 i = 0; i < nLength; ++i) {
        const uchar nByte = pName[i];
        if ((nByte < 0x20U) || (nByte == 0x7FU) || (nByte == '/') ||
            (nByte == '\\')) {
            return false;
        }
    }

    QString sName;
    if (nLength > 0) {
        const char *pChars = reinterpret_cast<const char *>(pName);
        if (u8IsValidUtf8(pName, nLength)) {
            sName = QString::fromUtf8(pChars, static_cast<qint32>(nLength));
        } else {
            sName = QString::fromLatin1(pChars, static_cast<qint32>(nLength));
        }
    }
    if ((sName == QLatin1String(".")) || (sName == QLatin1String(".."))) {
        return false;
    }
    *psName = sName;
    return true;
}

// icon.bin -> icon_2.bin, meta -> meta_2 (the house convention).
QString u8AppendDuplicateSuffix(const QString &sComponent, qint64 nSuffix)
{
    const qint32 nDot = sComponent.lastIndexOf(QLatin1Char('.'));
    const QString sSuffix = QStringLiteral("_%1").arg(nSuffix);
    if (nDot > 0) {
        return sComponent.left(nDot) + sSuffix + sComponent.mid(nDot);
    }
    return sComponent + sSuffix;
}

QString u8JoinPath(const QString &sParent, const QString &sLeaf)
{
    if (sParent.isEmpty()) return sLeaf;
    return sParent + QLatin1Char('/') + sLeaf;
}

// One IMET title: 42 UTF-16BE code units, decoded up to the first 0x0000.
// Control characters (Nintendo separates the two title lines with one) become
// spaces because the result goes into a one-line report.
QString u8DecodeImetTitle(const uchar *pData)
{
    QString sResult;
    const qint64 nUnits = IMET_TITLE_BYTES / 2;
    for (qint64 i = 0; i < nUnits; ++i) {
        const quint16 nUnit = u8ReadBE16(pData + (i * 2));
        if (nUnit == 0) break;
        if (nUnit < 0x20U) {
            sResult.append(QLatin1Char(' '));
        } else {
            sResult.append(QChar(nUnit));
        }
    }
    return sResult.trimmed();
}
}  // namespace

XWiiU8Archive::XWiiU8Archive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWiiU8Archive::~XWiiU8Archive()
{
}

bool XWiiU8Archive::parseImetWrapper(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QPointer<XWiiU8Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nMagicOffset = (pContext->nWrapper == WRAPPER_IMET_SHORT)
                                    ? IMET_SHORT_MAGIC_OFFSET
                                    : IMET_LONG_MAGIC_OFFSET;
    const qint64 nBlockOffset = pContext->nBase - IMET_HASHED_SIZE;
    if ((nBlockOffset < 0) || (nMagicOffset < nBlockOffset)) return false;

    QByteArray baBlock = read_array_process(nBlockOffset, IMET_HASHED_SIZE,
                                            pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baBlock.size() != IMET_HASHED_SIZE)) {
        return false;
    }
    const uchar *pBlock = reinterpret_cast<const uchar *>(baBlock.constData());
    const uchar *pImet = pBlock + (nMagicOffset - nBlockOffset);
    if (u8ReadBE32(pImet) != IMET_MAGIC) return false;
    // The hash size is the one constant that separates a real IMET header
    // from a stray "IMET" string, so it is the wrapper's only hard reject.
    if (u8ReadBE32(pImet + IMET_FIELD_HASH_SIZE) != IMET_HASH_SIZE_FIELD) {
        return false;
    }

    pContext->nImetVersion = u8ReadBE32(pImet + IMET_FIELD_VERSION);
    pContext->nImetIconSize = u8ReadBE32(pImet + IMET_FIELD_ICON_SIZE);
    pContext->nImetBannerSize = u8ReadBE32(pImet + IMET_FIELD_BANNER_SIZE);
    pContext->nImetSoundSize = u8ReadBE32(pImet + IMET_FIELD_SOUND_SIZE);
    pContext->nImetFlags = u8ReadBE32(pImet + IMET_FIELD_FLAGS);

    // English first, then Japanese, then the first non-empty of the ten.
    QString sTitle;
    QString sFirstNonEmpty;
    for (qint64 i = 0; i < IMET_TITLE_COUNT; ++i) {
        const QString sCandidate = u8DecodeImetTitle(
            pImet + IMET_FIELD_TITLES + (i * IMET_TITLE_BYTES));
        if (sCandidate.isEmpty()) continue;
        if (sFirstNonEmpty.isEmpty()) sFirstNonEmpty = sCandidate;
        if (i == IMET_TITLE_INDEX_ENGLISH) {
            sTitle = sCandidate;
            break;
        }
        if ((i == IMET_TITLE_INDEX_JAPANESE) && sTitle.isEmpty()) {
            sTitle = sCandidate;
        }
    }
    if (sTitle.isEmpty()) sTitle = sFirstNonEmpty;
    pContext->sImetTitle = sTitle;

    // The digest sits in the last 16 bytes of the hashed span and is hashed
    // as zeros.  Reported only: homebrew banner tools write bad ones.
    const QByteArray baStoredDigest = baBlock.right(IMET_DIGEST_SIZE);
    QByteArray baHashed = baBlock;
    for (qint64 i = 0; i < IMET_DIGEST_SIZE; ++i) {
        baHashed[static_cast<qint32>(IMET_HASHED_SIZE - IMET_DIGEST_SIZE + i)] =
            '\0';
    }
    const QByteArray baComputed =
        QCryptographicHash::hash(baHashed, QCryptographicHash::Md5);
    pContext->bImetHashValid = (baComputed == baStoredDigest);

    return guardedThis && guardedSource;
}

bool XWiiU8Archive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XWiiU8Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (U8_HEADER_SIZE + U8_MIN_HEADER_SIZE)) return false;

    // Step 1: choose the wrapper.  Each shape needs BOTH its wrapper magic and
    // the U8 magic behind it; first match wins.
    const qint64 nProbeSize = qMin(context.nInputSize, U8_PROBE_SIZE);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baProbe.size() != nProbeSize)) {
        return false;
    }

    bool bShapeFound = false;
    if (u8HasMagicAt(baProbe, 0, U8_MAGIC)) {
        context.nWrapper = WRAPPER_NONE;
        context.nBase = 0;
        bShapeFound = true;
    } else if (u8HasMagicAt(baProbe, 0, IMD5_MAGIC) &&
               u8HasMagicAt(baProbe, IMD5_HEADER_SIZE, U8_MAGIC)) {
        context.nWrapper = WRAPPER_IMD5;
        context.nBase = IMD5_HEADER_SIZE;
        bShapeFound = true;
    } else {
        const qint64 arrMagicOffsets[2] = {IMET_SHORT_MAGIC_OFFSET,
                                           IMET_LONG_MAGIC_OFFSET};
        const qint32 arrWrappers[2] = {WRAPPER_IMET_SHORT, WRAPPER_IMET_LONG};
        for (qint32 i = 0; (i < 2) && !bShapeFound; ++i) {
            if (!u8HasMagicAt(baProbe, arrMagicOffsets[i], IMET_MAGIC)) continue;
            // The IMET block is 0x600 bytes starting 0x40 before its magic,
            // so the U8 begins 0x5C0 bytes after the magic: 0x600 or 0x640.
            const qint64 nCandidateBase =
                arrMagicOffsets[i] + (IMET_HASHED_SIZE - IMET_SHORT_MAGIC_OFFSET);
            if (!u8RangeWithin(context.nInputSize, nCandidateBase, 4)) continue;
            const QByteArray baU8Magic =
                read_array_process(nCandidateBase, 4, pPdStruct);
            if (!guardedThis || !guardedSource) return false;
            if (!u8HasMagicAt(baU8Magic, 0, U8_MAGIC)) continue;
            context.nWrapper = arrWrappers[i];
            context.nBase = nCandidateBase;
            bShapeFound = true;
        }
    }
    if (!bShapeFound) return false;

    // V1: header + root node + the root's empty name must exist.
    if (!u8RangeWithin(context.nInputSize, context.nBase,
                       U8_HEADER_SIZE + U8_MIN_HEADER_SIZE)) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(context.nBase, U8_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != U8_HEADER_SIZE)) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (u8ReadBE32(pHeader) != U8_MAGIC) return false;
    // V3: the root node always follows the 32-byte header.
    if (u8ReadBE32(pHeader + 4) != U8_ROOT_NODE_OFFSET) return false;
    const qint64 nHeaderSize = static_cast<qint64>(u8ReadBE32(pHeader + 8));
    const qint64 nDataOffset = static_cast<qint64>(u8ReadBE32(pHeader + 12));
    // V4: node table + pool must be plausible and inside the device.
    if ((nHeaderSize < U8_MIN_HEADER_SIZE) || (nHeaderSize > U8_MAX_HEADER_SIZE)) {
        return false;
    }
    const qint64 nTableOffset = context.nBase + U8_HEADER_SIZE;
    if (!u8RangeWithin(context.nInputSize, nTableOffset, nHeaderSize)) {
        return false;
    }
    context.nHeaderSize = nHeaderSize;
    context.nDataOffset = nDataOffset;

    const QByteArray baTable =
        read_array_process(nTableOffset, nHeaderSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baTable.size() != nHeaderSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const uchar *pTable = reinterpret_cast<const uchar *>(baTable.constData());

    // V5: the root node.
    if (pTable[0] != U8_NODE_TYPE_DIRECTORY) return false;
    if (u8ReadBE24(pTable + 1) != 0) return false;
    const qint64 nNodeCount = static_cast<qint64>(u8ReadBE32(pTable + 8));
    if ((nNodeCount < 1) || (nNodeCount > U8_MAX_NODES)) return false;
    // V6: the pool needs room for at least the root's terminator.
    const qint64 nTableBytes = nNodeCount * U8_NODE_SIZE;
    if (nTableBytes > (nHeaderSize - 1)) return false;
    const uchar *pPool = pTable + nTableBytes;
    const qint64 nPoolBytes = nHeaderSize - nTableBytes;
    // V7: the root's name is empty in every writer; it is never a member.
    if (pPool[0] != 0) return false;
    context.nNodeCount = nNodeCount;

    // V8 .. V15: walk the nodes in pre-order with a directory stack.
    QList<DIRECTORY_FRAME> listStack;
    {
        DIRECTORY_FRAME rootFrame = {};
        rootFrame.nEndIndex = nNodeCount;
        listStack.append(rootFrame);
    }
    QSet<QString> setUsedKeys;
    QHash<QString, qint64> mapNextSuffixes;
    bool bHasNonEmptyFile = false;
    qint64 nDataEnd = 0;

    for (qint64 i = 1; i < nNodeCount; ++i) {
        if (((i & U8_CANCEL_CHECK_MASK) == 0) &&
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        // Directories whose range has ended are closed before this node.  The
        // root's end index is nNodeCount, so the stack never empties here.
        while ((listStack.size() > 1) && (listStack.last().nEndIndex <= i)) {
            listStack.removeLast();
        }
        // Copied, not referenced: the push below may reallocate the list.
        const qint64 nParentEndIndex = listStack.last().nEndIndex;
        const QString sParentPath = listStack.last().sPath;

        const uchar *pNode = pTable + (i * U8_NODE_SIZE);
        const quint8 nType = pNode[0];
        // V8: only files and directories exist.
        if ((nType != U8_NODE_TYPE_FILE) && (nType != U8_NODE_TYPE_DIRECTORY)) {
            return false;
        }
        const qint64 nNameOffset = static_cast<qint64>(u8ReadBE24(pNode + 1));
        const qint64 nField4 = static_cast<qint64>(u8ReadBE32(pNode + 4));
        const qint64 nField8 = static_cast<qint64>(u8ReadBE32(pNode + 8));

        // V9: name rules.  An empty non-root name is not a reject; it gets a
        // synthetic name so that a pathological archive still lists.
        QString sName;
        if (!u8DecodeName(pPool, nPoolBytes, nNameOffset, &sName)) return false;
        if (sName.isEmpty()) sName = QStringLiteral("_node%1").arg(i);

        // V14: compose the path from the resolved (possibly renamed) parents.
        const QString sBasePath = u8JoinPath(sParentPath, sName);
        if (sBasePath.size() > U8_MAX_PATH_CHARS) return false;

        // V15: duplicates (case-folded, one namespace for files and
        // directories) get _2, _3, ... on the leaf; the first stays verbatim.
        const QString sBaseKey = sBasePath.toCaseFolded();
        QString sPath = sBasePath;
        QString sKey = sBaseKey;
        qint64 nSuffix = 1;
        while (setUsedKeys.contains(sKey)) {
            nSuffix = (nSuffix == 1) ? mapNextSuffixes.value(sBaseKey, 2)
                                     : (nSuffix + 1);
            if (nSuffix > (U8_MAX_NODES + 1)) return false;
            sPath = u8JoinPath(sParentPath,
                               u8AppendDuplicateSuffix(sName, nSuffix));
            sKey = sPath.toCaseFolded();
        }
        if (sPath.size() > U8_MAX_PATH_CHARS) return false;
        setUsedKeys.insert(sKey);
        mapNextSuffixes.insert(sBaseKey, qMax(Q_INT64_C(2), nSuffix + 1));

        MEMBER member = {};
        member.nRecordOffset = nTableOffset + (i * U8_NODE_SIZE);
        member.sPath = sPath;

        if (nType == U8_NODE_TYPE_DIRECTORY) {
            // V11: E == i + 1 is an empty directory; E must stay inside the
            // parent.  The +4 "parent" field is a parent index in Nintendo and
            // libWiiPy files but a nesting depth in some homebrew files, so it
            // is deliberately ignored.
            const qint64 nEndIndex = nField8;
            if ((nEndIndex < (i + 1)) || (nEndIndex > nParentEndIndex)) {
                return false;
            }
            member.bIsFolder = true;
            member.nDataOffset = member.nRecordOffset;
            member.nSize = 0;
            context.listEntries.append(member);
            ++context.nDirectoryCount;

            DIRECTORY_FRAME frame = {};
            frame.nEndIndex = nEndIndex;
            frame.sPath = sPath;
            listStack.append(frame);
        } else {
            const qint64 nFileOffset = nField4;
            const qint64 nFileSize = nField8;
            qint64 nAbsoluteOffset = 0;
            qint64 nAbsoluteEnd = 0;
            if (!u8CheckedAdd(context.nBase, nFileOffset, &nAbsoluteOffset) ||
                !u8CheckedAdd(nAbsoluteOffset, nFileSize, &nAbsoluteEnd)) {
                return false;
            }
            if (nFileSize > 0) {
                // V12: a stored payload lies at or past the data region start
                // and inside the device.  Overlaps and order are not checked:
                // harmless for a store-only container.
                if (nFileOffset < nDataOffset) return false;
                if (!u8RangeWithin(context.nInputSize, nAbsoluteOffset,
                                   nFileSize)) {
                    return false;
                }
                bHasNonEmptyFile = true;
                nDataEnd = qMax(nDataEnd, nAbsoluteEnd);
            } else {
                // V13: an empty file only needs its offset inside the device
                // (libWiiPy gives it the next file's offset).
                if (!u8RangeWithin(context.nInputSize, nAbsoluteOffset, 0)) {
                    return false;
                }
            }
            member.bIsFolder = false;
            member.nDataOffset = nAbsoluteOffset;
            member.nSize = nFileSize;
            context.listEntries.append(member);
            ++context.nFileCount;
        }
    }

    // V10: the data offset field.  0 is what some homebrew writers emit for a
    // tree without file payloads; alignment differs between writers and is
    // not validated.
    qint64 nAbsoluteDataOffset = 0;
    if (!u8CheckedAdd(context.nBase, nDataOffset, &nAbsoluteDataOffset)) {
        return false;
    }
    const bool bDataOffsetInRange =
        (nDataOffset >= (U8_HEADER_SIZE + nHeaderSize)) &&
        (nAbsoluteDataOffset <= context.nInputSize);
    if (!bDataOffsetInRange) {
        if (bHasNonEmptyFile || (nDataOffset != 0)) return false;
    }

    // The container end: the farthest of the data region start, the pool end
    // and the last stored byte; the IMD5 size is preferred when it is
    // consistent because it includes the writer's trailing padding.  Trailing
    // bytes beyond that are overlay, not format.
    qint64 nArchiveSize = qMax(nTableOffset + nHeaderSize, nDataEnd);
    if (bDataOffsetInRange) nArchiveSize = qMax(nArchiveSize, nAbsoluteDataOffset);

    if (context.nWrapper == WRAPPER_IMD5) {
        const uchar *pImd5 = reinterpret_cast<const uchar *>(baProbe.constData());
        context.nImd5PayloadSize = static_cast<qint64>(u8ReadBE32(pImd5 + 4));
        context.sImd5Digest = QString::fromLatin1(
            baProbe.mid(static_cast<qint32>(IMD5_DIGEST_OFFSET),
                        static_cast<qint32>(IMET_DIGEST_SIZE))
                .toHex());
        // V16: report only; the size participates in the end offset when it
        // lies between the computed end and the device end.
        qint64 nImd5End = 0;
        if (u8CheckedAdd(IMD5_HEADER_SIZE, context.nImd5PayloadSize, &nImd5End) &&
            (nImd5End >= nArchiveSize) && (nImd5End <= context.nInputSize)) {
            nArchiveSize = nImd5End;
        }
    } else if ((context.nWrapper == WRAPPER_IMET_SHORT) ||
               (context.nWrapper == WRAPPER_IMET_LONG)) {
        // V17 / V18: the hash size field is the hard reject; everything else
        // is reported.
        if (!parseImetWrapper(&context, pPdStruct) || !guardedThis ||
            !guardedSource) {
            return false;
        }
    }

    context.nArchiveSize = qMin(nArchiveSize, context.nInputSize);
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XWiiU8Archive::isValid(PDSTRUCT *pPdStruct)
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

bool XWiiU8Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWiiU8Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWiiU8Archive::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWiiU8Archive(pDevice);
}

QList<QString> XWiiU8Archive::getSearchSignatures()
{
    // Magic plus the fixed root node offset; the IMD5 form pins its zero
    // padding and the wrapped magic too.  The IMET forms have no fixed bytes
    // within the first 0x40 (build tag) and are left to isValid().
    return QList<QString>()
           << QStringLiteral("55AA382D00000020")
           << QStringLiteral("494D4435........0000000000000000"
                             "................................55AA382D00000020");
}

XBinary::FT XWiiU8Archive::getFileType()
{
    return FT_WII_U8;
}

XBinary::MODE XWiiU8Archive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XWiiU8Archive::getEndian()
{
    return ENDIAN_BIG;
}

QString XWiiU8Archive::getArch()
{
    return QString();
}

qint32 XWiiU8Archive::getType()
{
    return TYPE_ARCHIVE;
}

QString XWiiU8Archive::getFileFormatExt()
{
    return QStringLiteral("arc");
}

QString XWiiU8Archive::getFileFormatExtsString()
{
    return QStringLiteral("Nintendo Wii U8 archive (*.arc *.app *.bnr *.u8)");
}

QString XWiiU8Archive::getMIMEString()
{
    return QStringLiteral("application/x-nintendo-u8-archive");
}

qint64 XWiiU8Archive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XWiiU8Archive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XWiiU8Archive::getMemoryMap(MAPMODE mapMode,
                                                 PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XWiiU8Archive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XWiiU8Archive::getFileParts(quint32 nFileParts,
                                                  qint32 nLimit,
                                                  PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        // Wrapper + U8 header + node table + string pool.
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nBase + U8_HEADER_SIZE + context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if (nFileParts & FILEPART_STREAM) {
        const qint32 nCount = context.listEntries.size();
        for (qint32 i = 0; i < nCount; ++i) {
            if (!canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
                break;
            }
            const MEMBER &member = context.listEntries.at(i);
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sPath;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      member.bIsFolder
                                          ? QStringLiteral("Directory")
                                          : QStringLiteral("Stored"));
            part.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);
            listResult.append(part);
        }
    }

    if (nFileParts & FILEPART_REGION) {
        const qint32 nCount = context.listEntries.size();
        for (qint32 i = 0; i < nCount; ++i) {
            const MEMBER &member = context.listEntries.at(i);
            if (member.bIsFolder) continue;
            if (!canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
                break;
            }
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sPath;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XWiiU8Archive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

QString XWiiU8Archive::buildInfoString(const CONTEXT &context)
{
    QString sResult = QStringLiteral("U8 archive; %1 nodes (%2 %3, %4 %5)")
                          .arg(context.nNodeCount)
                          .arg(context.nDirectoryCount)
                          .arg((context.nDirectoryCount == 1)
                                   ? QStringLiteral("directory")
                                   : QStringLiteral("directories"))
                          .arg(context.nFileCount)
                          .arg((context.nFileCount == 1)
                                   ? QStringLiteral("file")
                                   : QStringLiteral("files"));

    if (context.nWrapper == WRAPPER_IMD5) {
        sResult += QStringLiteral("; IMD5 wrapper; payload 0x%1 bytes; MD5 %2")
                       .arg(context.nImd5PayloadSize, 0, 16)
                       .arg(context.sImd5Digest);
    } else if ((context.nWrapper == WRAPPER_IMET_SHORT) ||
               (context.nWrapper == WRAPPER_IMET_LONG)) {
        sResult += QStringLiteral("; IMET wrapper (%1, U8 at 0x%2)")
                       .arg((context.nWrapper == WRAPPER_IMET_SHORT)
                                ? QStringLiteral("disc")
                                : QStringLiteral("NAND"))
                       .arg(context.nBase, 0, 16);
        if (context.nImetVersion != 3) {
            sResult += QStringLiteral("; version %1").arg(context.nImetVersion);
        }
        if (!context.sImetTitle.isEmpty()) {
            sResult += QStringLiteral("; title \"%1\"").arg(context.sImetTitle);
        }
        sResult += QStringLiteral("; icon/banner/sound 0x%1/0x%2/0x%3")
                       .arg(context.nImetIconSize, 0, 16)
                       .arg(context.nImetBannerSize, 0, 16)
                       .arg(context.nImetSoundSize, 0, 16);
        if (context.nImetFlags != 0) {
            sResult += QStringLiteral("; flags 0x%1")
                           .arg(context.nImetFlags, 8, 16, QLatin1Char('0'));
        }
        sResult += context.bImetHashValid ? QStringLiteral("; IMET MD5 ok")
                                          : QStringLiteral("; IMET MD5 mismatch");
    }

    return sResult;
}

bool XWiiU8Archive::initUnpack(UNPACK_STATE *pState,
                               const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XWiiU8Archive> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO,
                                        buildInfoString(*pContext));
    // A root-only archive has no members; the cursor then already sits at the
    // container end.
    pState->nCurrentOffset = pContext->listEntries.isEmpty()
                                 ? pContext->nArchiveSize
                                 : pContext->listEntries.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XWiiU8Archive::infoCurrent(UNPACK_STATE *pState,
                                                  PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listEntries.at(pState->nCurrentIndex);
    // Record offsets are unique per node; data offsets are not (empty files
    // share them), which is why the cursor is keyed on the node record.
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sPath);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                member.bIsFolder ? QStringLiteral("Directory")
                                                 : QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);
    // No checksum and no time stamp property: the format carries neither.
    return result;
}

bool XWiiU8Archive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listEntries.size()) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XWiiU8Archive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XWiiU8Archive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME
                               << FPART_PROP_COMPRESSEDSIZE
                               << FPART_PROP_UNCOMPRESSEDSIZE
                               << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_ISFOLDER;
}
