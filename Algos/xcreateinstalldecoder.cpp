/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xcreateinstalldecoder.h"

#include <QStringList>
#include <QVector>

namespace {
const qint32 CI_SYMBOL_COUNT = 0x275;                  // 629 leaves
const qint32 CI_NODE_COUNT = 2 * CI_SYMBOL_COUNT - 1;  // 1257 nodes, node 1 is the root
const qint32 CI_EOF_SYMBOL = 0x100;
const qint32 CI_LENGTH_COUNT = 62;  // lengths 3..64 share one distance slot
const qint32 CI_MIN_LENGTH = 3;
const qint32 CI_SLOT_COUNT = 6;
const qint32 CI_WINDOW_SIZE = 0x8000;
const qint32 CI_WINDOW_MASK = CI_WINDOW_SIZE - 1;
const qint32 CI_WEIGHT_LIMIT = 2000;

// Extra bits per distance slot, and the running sum of (1 << bits) in front of
// each slot.  The reference builds the base table at run time from the bit
// table; it is written out here because it is constant.
const qint32 g_arrCiSlotBits[CI_SLOT_COUNT] = {4, 6, 8, 10, 12, 14};
const qint32 g_arrCiSlotBase[CI_SLOT_COUNT] = {0, 16, 80, 336, 1360, 5456};

const quint8 g_arrCiSignature[XCreateInstallDecoder::SIGNATURE_SIZE] = {0x61, 0x57, 0x41, 0x57, 0xAE, 0x40, 0x60, 0x1B};

// Record types.
const quint8 CI_RECORD_FILE = 1;
const quint8 CI_RECORD_ENTERDIR = 2;
const quint8 CI_RECORD_LEAVEDIR = 3;
const quint8 CI_RECORD_END = 4;

const qint64 CI_RECORD_HEADER_SIZE = 17;
const qint64 CI_PRELUDE_SHORT = 77;  // 0x12 + 0x0c + 0x2f
const qint64 CI_PRELUDE_LONG = 79;   // 0x12 + 0x0e + 0x2f
const qint64 CI_PRELUDE_SKIP_OFFSET = 8;
const qint32 CI_SCAN_LIMIT = 0x101;  // the reference gives up after this many slides

// A CreateInstall installer is a desktop setup program.  These ceilings only
// exist so a corrupt carrier cannot ask for an unbounded allocation.
const qint64 CI_MAX_MEMBER_SIZE = Q_INT64_C(0x20000000);
const qint64 CI_MAX_RUNTIME_SIZE = Q_INT64_C(0x400000);
const qint32 CI_MAX_RECORDS = 65536;
const qint32 CI_MAX_NAME_LENGTH = 1024;
const qint32 CI_MAX_DIRECTORY_DEPTH = 64;

quint32 ciRead32(const quint8 *pData)
{
    return (quint32)(pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24));
}

quint64 ciRead64(const quint8 *pData)
{
    return (quint64)ciRead32(pData) | ((quint64)ciRead32(pData + 4) << 32);
}

qint32 ciRead16(const quint8 *pData)
{
    return (qint32)(qint16)(quint16)(pData[0] | ((quint16)pData[1] << 8));
}

// The adaptive Huffman tree, the bit reader and the sliding window.  The big
// arrays are Qt containers so that the whole state can live on the stack of the
// caller without a 42 KiB frame.
struct CI_STATE {
    CI_STATE()
    {
        vecLeft.resize(CI_NODE_COUNT + 1);
        vecRight.resize(CI_NODE_COUNT + 1);
        vecParent.resize(CI_NODE_COUNT + 1);
        vecWeight.resize(CI_NODE_COUNT + 1);
        baWindow.resize(CI_WINDOW_SIZE);
        baWindow.fill((char)0);
        pLeft = vecLeft.data();
        pRight = vecRight.data();
        pParent = vecParent.data();
        pWeight = vecWeight.data();
        pWindow = (quint8 *)baWindow.data();
        pData = nullptr;
        nSize = 0;
        nPos = 0;
        nBitBuffer = 0;
        nBitCount = 0;
    }

    void reset(const quint8 *pInput, qint64 nInputSize)
    {
        pData = pInput;
        nSize = nInputSize;
        nPos = 0;
        nBitBuffer = 0;
        nBitCount = 0;

        for (qint32 i = 0; i <= CI_NODE_COUNT; i++) {
            pLeft[i] = 0;
            pRight[i] = 0;
            pParent[i] = 0;
            pWeight[i] = 0;
        }
        for (qint32 i = 2; i <= CI_NODE_COUNT; i++) {
            pWeight[i] = 1;
            pParent[i] = (quint16)(i >> 1);
        }
        for (qint32 i = 1; i < CI_SYMBOL_COUNT; i++) {
            pLeft[i] = (quint16)(i * 2);
            pRight[i] = (quint16)(i * 2 + 1);
        }
        baWindow.fill((char)0);
    }

    qint32 readBit()
    {
        if (nBitCount == 0) {
            if (nPos >= nSize) return -1;
            nBitBuffer = (quint32)pData[nPos];
            nPos++;
            nBitCount = 7;
        } else {
            nBitCount--;
        }
        const qint32 nBit = (qint32)((nBitBuffer >> 7) & 1);
        nBitBuffer = (nBitBuffer << 1) & 0xff;
        return nBit;
    }

    // LSB first: the first bit read is bit 0 of the result.
    qint32 readBits(qint32 nCount)
    {
        qint32 nResult = 0;
        qint32 nMask = 1;
        for (qint32 i = 0; i < nCount; i++) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            if (nBit) nResult |= nMask;
            nMask <<= 1;
        }
        return nResult;
    }

    // Recompute the weights from nNode up to the root, then halve everything
    // once the root reaches the limit.
    void propagate(qint32 nNode, qint32 nSibling)
    {
        while (true) {
            const qint32 nChild = nNode;
            nNode = (qint32)pParent[nNode];
            pWeight[nNode] = (quint16)(pWeight[nChild] + pWeight[nSibling]);
            if (nNode == 1) break;
            const qint32 nParent = (qint32)pParent[nNode];
            nSibling = (qint32)pLeft[nParent];
            if (nSibling == nNode) nSibling = (qint32)pRight[nParent];
        }
        if (pWeight[1] == CI_WEIGHT_LIMIT) {
            for (qint32 i = 1; i <= CI_NODE_COUNT; i++) {
                pWeight[i] = (quint16)(pWeight[i] >> 1);
            }
        }
    }

    void update(qint32 nNode)
    {
        pWeight[nNode] = (quint16)(pWeight[nNode] + 1);
        qint32 nParent = (qint32)pParent[nNode];
        if (nParent == 1) return;

        qint32 nSibling = (qint32)pLeft[nParent];
        if (nNode == nSibling) nSibling = (qint32)pRight[nParent];
        propagate(nNode, nSibling);

        qint32 nCurrent = nParent;
        while (true) {
            const qint32 nGrandParent = (qint32)pParent[nCurrent];
            const qint32 nGrandLeft = (qint32)pLeft[nGrandParent];
            qint32 nUncle = nGrandLeft;
            if (nCurrent == nGrandLeft) nUncle = (qint32)pRight[nGrandParent];

            if (pWeight[nUncle] < pWeight[nNode]) {
                if (nCurrent == nGrandLeft) {
                    pRight[nGrandParent] = (quint16)nNode;
                } else {
                    pLeft[nGrandParent] = (quint16)nNode;
                }
                qint32 nOther = (qint32)pLeft[nCurrent];
                if (nNode == nOther) {
                    nOther = (qint32)pRight[nCurrent];
                    pLeft[nCurrent] = (quint16)nUncle;
                } else {
                    pRight[nCurrent] = (quint16)nUncle;
                }
                pParent[nUncle] = (quint16)nCurrent;
                pParent[nNode] = (quint16)nGrandParent;
                propagate(nUncle, nOther);
                nNode = nUncle;
            }

            nNode = (qint32)pParent[nNode];
            nCurrent = (qint32)pParent[nNode];
            if (nCurrent == 1) break;
        }
    }

    qint32 decodeSymbol()
    {
        qint32 nNode = 1;
        while (true) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            nNode = nBit ? (qint32)pRight[nNode] : (qint32)pLeft[nNode];
            if ((nNode < 1) || (nNode > CI_NODE_COUNT)) return -1;
            if (nNode >= CI_SYMBOL_COUNT) break;
        }
        update(nNode);
        return nNode - CI_SYMBOL_COUNT;
    }

    QVector<quint16> vecLeft;
    QVector<quint16> vecRight;
    QVector<quint16> vecParent;
    QVector<quint16> vecWeight;
    QByteArray baWindow;
    quint16 *pLeft;
    quint16 *pRight;
    quint16 *pParent;
    quint16 *pWeight;
    quint8 *pWindow;
    const quint8 *pData;
    qint64 nSize;
    qint64 nPos;
    quint32 nBitBuffer;
    qint32 nBitCount;
};

// One name component, made safe for a host filesystem.  '%' is deliberately
// kept: a CreateInstall script name may hold the builder's "%installpath%"
// variable and folding it away would collide two members that the container
// keeps apart (one carrier ships both "Uninstal.exe" and
// "%installpath%\Uninstal.exe", and they are different files).
QString ciSanitizeComponent(const QString &sComponent)
{
    QString sResult;
    const QString sForbidden = QStringLiteral("<>:\"|?*");
    for (qint32 i = 0; i < sComponent.size(); i++) {
        const QChar character = sComponent.at(i);
        if ((character.unicode() < 0x20) || sForbidden.contains(character)) {
            sResult.append(QLatin1Char('_'));
        } else {
            sResult.append(character);
        }
    }
    while (!sResult.isEmpty() && ((sResult.at(sResult.size() - 1) == QLatin1Char(' ')) || (sResult.at(sResult.size() - 1) == QLatin1Char('.')))) {
        sResult.chop(1);
    }
    if (sResult.isEmpty()) sResult = QStringLiteral("_");
    return sResult;
}
}  // namespace

bool XCreateInstallDecoder::checkSignature(const quint8 *pData, qint64 nSize)
{
    if (!pData || (nSize < SIGNATURE_SIZE)) return false;
    for (qint32 i = 0; i < SIGNATURE_SIZE; i++) {
        if (pData[i] != g_arrCiSignature[i]) return false;
    }
    return true;
}

bool XCreateInstallDecoder::decodeStream(const quint8 *pData, qint64 nSize, qint64 nOutputLimit, QByteArray *pOutput, qint64 *pnConsumed, qint64 *pnProduced,
                                         XBinary::PDSTRUCT *pPdStruct)
{
    if (!pData || (nSize <= 0) || (nOutputLimit < 0) || !pnConsumed || !pnProduced) return false;

    CI_STATE state;
    state.reset(pData, nSize);

    qint64 nProduced = 0;
    qint32 nWindowPos = 0;
    qint32 nTick = 0;
    bool bResult = false;

    while (true) {
        nTick++;
        if (((nTick & 0xffff) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) break;

        const qint32 nSymbol = state.decodeSymbol();
        if (nSymbol < 0) break;
        if (nSymbol == CI_EOF_SYMBOL) {
            bResult = true;
            break;
        }

        if (nSymbol < CI_EOF_SYMBOL) {
            if (nProduced >= nOutputLimit) break;
            const quint8 nByte = (quint8)nSymbol;
            state.pWindow[nWindowPos] = nByte;
            if (pOutput) pOutput->append((char)nByte);
            nWindowPos = (nWindowPos + 1) & CI_WINDOW_MASK;
            nProduced++;
        } else {
            const qint32 nCode = nSymbol - (CI_EOF_SYMBOL + 1);
            const qint32 nLength = (nCode % CI_LENGTH_COUNT) + CI_MIN_LENGTH;
            const qint32 nSlot = nCode / CI_LENGTH_COUNT;
            if (nSlot >= CI_SLOT_COUNT) break;
            const qint32 nExtra = state.readBits(g_arrCiSlotBits[nSlot]);
            if (nExtra < 0) break;
            const qint64 nDistance = (qint64)nExtra + (qint64)nLength + (qint64)g_arrCiSlotBase[nSlot];
            // The window starts empty for every stream, so a match can never
            // reach behind the first byte this stream produced.  The reference
            // reads the uninitialised window instead; refusing is the only way
            // to keep a corrupt stream from emitting invented bytes.
            if (nDistance > nProduced) break;
            if ((qint64)nLength > nOutputLimit - nProduced) break;
            qint32 nSource = (qint32)(((qint64)nWindowPos - nDistance) & (qint64)CI_WINDOW_MASK);
            for (qint32 i = 0; i < nLength; i++) {
                const quint8 nByte = state.pWindow[nSource];
                state.pWindow[nWindowPos] = nByte;
                if (pOutput) pOutput->append((char)nByte);
                nSource = (nSource + 1) & CI_WINDOW_MASK;
                nWindowPos = (nWindowPos + 1) & CI_WINDOW_MASK;
            }
            nProduced += nLength;
        }
    }

    *pnConsumed = state.nPos;
    *pnProduced = nProduced;
    return bResult && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XCreateInstallDecoder::decode(const QByteArray &packed, qint64 nUncompressedSize, QByteArray *punpacked, XBinary::PDSTRUCT *pPdStruct)
{
    if (!punpacked || (nUncompressedSize < 0) || (nUncompressedSize > CI_MAX_MEMBER_SIZE)) return false;

    punpacked->clear();
    // A zero-length member carries no stream at all, not even an end symbol.
    if (nUncompressedSize == 0) return true;
    if (packed.isEmpty()) return false;

    punpacked->reserve((qint32)nUncompressedSize);

    const quint8 *pData = (const quint8 *)packed.constData();
    const qint64 nPackedSize = (qint64)packed.size();
    qint64 nOffset = 0;

    while ((qint64)punpacked->size() < nUncompressedSize) {
        if (nOffset >= nPackedSize) return false;
        qint64 nConsumed = 0;
        qint64 nProduced = 0;
        if (!decodeStream(pData + nOffset, nPackedSize - nOffset, nUncompressedSize - (qint64)punpacked->size(), punpacked, &nConsumed, &nProduced, pPdStruct)) {
            return false;
        }
        // A stream that produced nothing would spin the chain forever.
        if ((nProduced <= 0) || (nConsumed <= 0)) return false;
        nOffset += nConsumed;
    }

    return ((qint64)punpacked->size() == nUncompressedSize);
}

bool XCreateInstallDecoder::isRecordHeaderValid(const QByteArray &baPayload, qint64 nOffset)
{
    const qint64 nPayloadSize = (qint64)baPayload.size();
    if ((nOffset < 0) || (nOffset > nPayloadSize - CI_RECORD_HEADER_SIZE)) return false;

    const quint8 *pRecord = (const quint8 *)baPayload.constData() + nOffset;
    const quint8 nType = pRecord[0];
    if ((nType != CI_RECORD_FILE) && (nType != CI_RECORD_ENTERDIR) && (nType != CI_RECORD_LEAVEDIR) && (nType != CI_RECORD_END)) return false;
    if (pRecord[14] > 1) return false;

    const qint32 nNameLength = ciRead16(pRecord + 15);
    if (nType == CI_RECORD_END) return true;
    if (nType == CI_RECORD_LEAVEDIR) return true;
    if ((nNameLength <= 0) || (nNameLength > CI_MAX_NAME_LENGTH)) return false;
    if ((qint64)nNameLength > nPayloadSize - (nOffset + CI_RECORD_HEADER_SIZE)) return false;
    for (qint32 i = 0; i < nNameLength; i++) {
        if (pRecord[CI_RECORD_HEADER_SIZE + i] < 0x20) return false;
    }
    return true;
}

// The record name is relative to the directory the type 2 / type 3 records
// have walked to, so the current directory's components come first.  A type 2
// record REPLACES that list with the result rather than appending to it, which
// is what the reference does when it rebuilds its current-directory string.
void XCreateInstallDecoder::composeParts(const QStringList &listDirectories, const QByteArray &baRawName, QStringList *plistParts)
{
    QByteArray baName = baRawName;
    // Script-derived names are stored quoted.  Both quotes have to be there:
    // anything else is left alone rather than silently shortened.
    if ((baName.size() >= 2) && (baName.at(0) == '"') && (baName.at(baName.size() - 1) == '"')) {
        baName = baName.mid(1, baName.size() - 2);
    }

    QString sRaw = QString::fromLatin1(baName.constData(), baName.size());
    sRaw.replace(QLatin1Char('\\'), QLatin1Char('/'));

    plistParts->clear();
    for (qint32 i = 0; i < listDirectories.size(); i++) {
        plistParts->append(listDirectories.at(i));
    }

    const QStringList listRaw = sRaw.split(QLatin1Char('/'));
    for (qint32 i = 0; i < listRaw.size(); i++) {
        const QString sPart = listRaw.at(i);
        if (sPart.isEmpty() || (sPart == QLatin1String("."))) continue;
        if (sPart == QLatin1String("..")) {
            // Never walk out of the extraction directory.
            plistParts->append(QStringLiteral("__"));
            continue;
        }
        // A leading drive letter is not a name component.
        if ((sPart.size() == 2) && (sPart.at(1) == QLatin1Char(':')) && sPart.at(0).isLetter() && (i == 0) && plistParts->isEmpty()) continue;
        plistParts->append(ciSanitizeComponent(sPart));
    }
}

bool XCreateInstallDecoder::walkContainer(const QByteArray &baPayload, qint64 nContainerOffset, QList<RECORD> *plistRecords, qint64 *pnArchiveSize,
                                          XBinary::PDSTRUCT *pPdStruct)
{
    if (!plistRecords || !pnArchiveSize || (nContainerOffset < 0)) return false;

    plistRecords->clear();
    *pnArchiveSize = 0;

    const qint64 nPayloadSize = (qint64)baPayload.size();
    if (nPayloadSize < SIGNATURE_SIZE + CI_RECORD_HEADER_SIZE) return false;

    const quint8 *pPayload = (const quint8 *)baPayload.constData();
    if (!checkSignature(pPayload, nPayloadSize)) return false;

    // 1. The runtime stream.  Its decoded size is published as a member and its
    //    compressed length is where the prelude search starts.
    QByteArray baRuntime;
    qint64 nRuntimeConsumed = 0;
    qint64 nRuntimeProduced = 0;
    if (!decodeStream(pPayload, nPayloadSize, CI_MAX_RUNTIME_SIZE, &baRuntime, &nRuntimeConsumed, &nRuntimeProduced, pPdStruct)) return false;
    if ((nRuntimeProduced <= 0) || (nRuntimeConsumed <= 0)) return false;
    baRuntime.clear();

    // 2. The prelude.  Prefer the dword that is the size of the carrier file;
    //    fall back to the reference's weaker "top byte is zero" rule.
    const quint32 nFileSize = (quint32)(nContainerOffset + nPayloadSize);
    qint64 nPreludeOffset = -1;
    qint64 nFallbackOffset = -1;
    for (qint32 i = 0; i <= CI_SCAN_LIMIT; i++) {
        const qint64 nProbe = nRuntimeConsumed + i;
        if (nProbe > nPayloadSize - 4) break;
        const quint32 nValue = ciRead32(pPayload + nProbe);
        if (nValue == nFileSize) {
            nPreludeOffset = nProbe;
            break;
        }
        if ((nFallbackOffset < 0) && ((nValue >> 24) == 0)) nFallbackOffset = nProbe;
    }
    if (nPreludeOffset < 0) nPreludeOffset = nFallbackOffset;
    if (nPreludeOffset < 0) return false;
    if (nPreludeOffset > nPayloadSize - 0x12) return false;

    const qint32 nSkip = (qint32)ciRead32(pPayload + nPreludeOffset + CI_PRELUDE_SKIP_OFFSET);
    if (nSkip < 0) return false;

    // 3. The 77/79 prelude length.  Both candidates are tried and the one that
    //    lands on a readable record header wins; the reference's own rule
    //    ("the runtime is 0x10000 bytes long") mis-sizes the oldest builder.
    qint64 nRecordOffset = -1;
    for (qint32 i = 0; i < 2; i++) {
        const qint64 nCandidate = nPreludeOffset + (i == 0 ? CI_PRELUDE_SHORT : CI_PRELUDE_LONG) + (qint64)nSkip;
        if (isRecordHeaderValid(baPayload, nCandidate)) {
            nRecordOffset = nCandidate;
            break;
        }
    }
    if (nRecordOffset < 0) return false;

    // 4. The record chain.
    QStringList listDirectories;
    bool bEndSeen = false;
    qint64 nPosition = nRecordOffset;

    RECORD runtime;
    runtime.sFileName = QStringLiteral("instcrin.dll");
    runtime.nStreamOffset = 0;
    runtime.nStreamSize = nRuntimeConsumed;
    runtime.nUncompressedSize = nRuntimeProduced;
    runtime.nAttributes = 0;
    runtime.nFileTime = 0;
    runtime.bStored = false;
    runtime.nFlag = 0;
    plistRecords->append(runtime);

    while (!bEndSeen) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nPosition > nPayloadSize - CI_RECORD_HEADER_SIZE) return false;
        if (plistRecords->size() > CI_MAX_RECORDS) return false;

        const quint8 *pRecord = pPayload + nPosition;
        const quint8 nType = pRecord[0];
        const quint8 nFlag = pRecord[1];
        const quint32 nAttributes = ciRead32(pRecord + 2);
        const quint64 nFileTime = ciRead64(pRecord + 6);
        const quint8 nMethod = pRecord[14];
        const qint32 nNameLength = ciRead16(pRecord + 15);
        nPosition += CI_RECORD_HEADER_SIZE;

        if (nMethod > 1) return false;

        if (nType == CI_RECORD_END) {
            bEndSeen = true;
            break;
        }
        if (nType == CI_RECORD_LEAVEDIR) {
            if (!listDirectories.isEmpty()) listDirectories.removeLast();
            continue;
        }
        if ((nType != CI_RECORD_FILE) && (nType != CI_RECORD_ENTERDIR)) return false;

        if ((nNameLength <= 0) || (nNameLength > CI_MAX_NAME_LENGTH)) return false;
        if ((qint64)nNameLength > nPayloadSize - nPosition) return false;
        const QByteArray baRawName = baPayload.mid((qint32)nPosition, nNameLength);
        nPosition += nNameLength;

        QStringList listParts;
        composeParts(listDirectories, baRawName, &listParts);
        if (listParts.isEmpty() || (listParts.size() > CI_MAX_DIRECTORY_DEPTH)) return false;
        const QString sName = listParts.join(QStringLiteral("/"));

        if (nType == CI_RECORD_ENTERDIR) {
            listDirectories = listParts;
            continue;
        }

        if (nPosition > nPayloadSize - 4) return false;
        const qint32 nSize = (qint32)ciRead32(pPayload + nPosition);
        nPosition += 4;
        if ((nSize < 0) || ((qint64)nSize > CI_MAX_MEMBER_SIZE)) return false;

        RECORD record;
        record.sFileName = sName;
        record.nStreamOffset = nPosition;
        record.nUncompressedSize = (qint64)nSize;
        record.nAttributes = nAttributes;
        record.nFileTime = nFileTime;
        record.bStored = (nMethod == 1);
        record.nFlag = nFlag;

        if (record.bStored || (nSize == 0)) {
            if ((qint64)nSize > nPayloadSize - nPosition) return false;
            record.nStreamSize = (qint64)nSize;
            nPosition += nSize;
        } else {
            // Nothing declares the compressed length: walk the stream chain to
            // find it.  The decoded bytes are thrown away, only the counts
            // matter here.
            qint64 nRemaining = (qint64)nSize;
            qint64 nStreamSize = 0;
            while (nRemaining > 0) {
                qint64 nConsumed = 0;
                qint64 nProduced = 0;
                if (nPosition + nStreamSize >= nPayloadSize) return false;
                if (!decodeStream(pPayload + nPosition + nStreamSize, nPayloadSize - (nPosition + nStreamSize), nRemaining, nullptr, &nConsumed, &nProduced,
                                  pPdStruct)) {
                    return false;
                }
                if ((nProduced <= 0) || (nConsumed <= 0) || (nProduced > nRemaining)) return false;
                nStreamSize += nConsumed;
                nRemaining -= nProduced;
            }
            record.nStreamSize = nStreamSize;
            nPosition += nStreamSize;
        }

        plistRecords->append(record);
    }

    if (!bEndSeen) return false;
    if (plistRecords->size() < 2) return false;

    *pnArchiveSize = nPosition;
    return true;
}
