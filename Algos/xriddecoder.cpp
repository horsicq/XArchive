/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xriddecoder.h"

#include "xdcldecoder.h"

#include <QtEndian>

namespace {
// The frame length field is a quint16, so this is a hard ceiling of the
// container, not a policy limit.
const qint64 RID_MAX_BLOCK_SIZE = 0xffff;
// A member's uncompressed size lives in a quint32 that the container itself
// caps at 0x00ffffff; give the per-block decoder the same budget so a single
// corrupted block cannot be asked to materialise more than one member.
const qint64 RID_MAX_MEMBER_SIZE = Q_INT64_C(0x00ffffff);
// A chain with more frames than this is not a RID member; the largest member
// of the reference corpus uses 8 frames.
const qint32 RID_MAX_BLOCKS = 100000;

bool ridReadFrame(const uchar *pData, qint64 nSize, qint64 nOffset,
                  qint64 *pnBlockSize, quint8 *pnBlockType)
{
    if ((nOffset < 0) || (nSize < XRidDecoder::BLOCK_FRAME_SIZE) ||
        (nOffset > nSize - XRidDecoder::BLOCK_FRAME_SIZE)) {
        return false;
    }
    *pnBlockSize =
        static_cast<qint64>(qFromLittleEndian<quint16>(pData + nOffset));
    *pnBlockType = pData[nOffset + 2];
    return true;
}
}  // namespace

bool XRidDecoder::measure(const uchar *pData, qint64 nSize, qint64 *pnConsumed,
                          qint32 *pnBlocks)
{
    if (!pData || (nSize < 0)) return false;

    qint64 nOffset = 0;
    qint32 nBlocks = 0;
    while (nBlocks <= RID_MAX_BLOCKS) {
        qint64 nBlockSize = 0;
        quint8 nBlockType = 0;
        if (!ridReadFrame(pData, nSize, nOffset, &nBlockSize, &nBlockType)) {
            return false;
        }
        nOffset += BLOCK_FRAME_SIZE;
        if (nBlockType == BLOCK_TYPE_END) {
            // The terminator is the ONLY frame allowed to be empty, and it is
            // required to be empty: 0xff with a length would mean the walk is
            // reading something that is not a RID frame.
            if (nBlockSize != 0) return false;
            if (pnConsumed) *pnConsumed = nOffset;
            if (pnBlocks) *pnBlocks = nBlocks;
            return true;
        }
        if ((nBlockType != BLOCK_TYPE_STORED) &&
            (nBlockType != BLOCK_TYPE_PACKED)) {
            return false;
        }
        if ((nBlockSize <= 0) || (nBlockSize > RID_MAX_BLOCK_SIZE)) {
            return false;
        }
        if (nBlockSize > nSize - nOffset) return false;
        nOffset += nBlockSize;
        nBlocks++;
    }
    return false;
}

bool XRidDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                         QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;
    pbaUnpacked->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > RID_MAX_MEMBER_SIZE)) {
        return false;
    }

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    const qint64 nSize = baPacked.size();

    QByteArray baResult;
    qint64 nOffset = 0;
    qint32 nBlocks = 0;
    while (nBlocks <= RID_MAX_BLOCKS) {
        qint64 nBlockSize = 0;
        quint8 nBlockType = 0;
        if (!ridReadFrame(pData, nSize, nOffset, &nBlockSize, &nBlockType)) {
            return false;
        }
        nOffset += BLOCK_FRAME_SIZE;
        if (nBlockType == BLOCK_TYPE_END) {
            if (nBlockSize != 0) return false;
            // The caller hands over the member's exact extent, so a chain that
            // terminates early means the member boundary and the frames
            // disagree - refuse rather than publish a truncated member.
            if (nOffset != nSize) return false;
            if (baResult.size() != nUncompressedSize) return false;
            *pbaUnpacked = baResult;
            return true;
        }
        if ((nBlockSize <= 0) || (nBlockSize > RID_MAX_BLOCK_SIZE) ||
            (nBlockSize > nSize - nOffset)) {
            return false;
        }
        if (nBlockType == BLOCK_TYPE_STORED) {
            if (baResult.size() > nUncompressedSize - nBlockSize) return false;
            baResult.append(baPacked.mid(static_cast<int>(nOffset),
                                         static_cast<int>(nBlockSize)));
        } else if (nBlockType == BLOCK_TYPE_PACKED) {
            QByteArray baBlock;
            const qint64 nBudget = nUncompressedSize - baResult.size();
            if (nBudget < 0) return false;
            if (!XDclDecoder::decode(baPacked.mid(static_cast<int>(nOffset),
                                                 static_cast<int>(nBlockSize)),
                                     &baBlock, nBudget, nullptr)) {
                return false;
            }
            if (baBlock.size() > nBudget) return false;
            baResult.append(baBlock);
        } else {
            return false;
        }
        nOffset += nBlockSize;
        nBlocks++;
    }
    return false;
}

bool XRidDecoder::probe(const QByteArray &baPacked, qint64 nMaxOutput,
                        bool *pbComplete, qint64 *pnRawSize)
{
    if (pbComplete) *pbComplete = false;
    if (pnRawSize) *pnRawSize = -1;
    if (nMaxOutput < 0) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    const qint64 nSize = baPacked.size();

    qint64 nOffset = 0;
    qint64 nRawSize = 0;
    qint32 nBlocks = 0;
    while (nBlocks <= RID_MAX_BLOCKS) {
        qint64 nBlockSize = 0;
        quint8 nBlockType = 0;
        if (!ridReadFrame(pData, nSize, nOffset, &nBlockSize, &nBlockType)) {
            return false;
        }
        nOffset += BLOCK_FRAME_SIZE;
        if (nBlockType == BLOCK_TYPE_END) {
            if (nBlockSize != 0) return false;
            if (pbComplete) *pbComplete = true;
            if (pnRawSize) *pnRawSize = nRawSize;
            return true;
        }
        if ((nBlockSize <= 0) || (nBlockSize > RID_MAX_BLOCK_SIZE) ||
            (nBlockSize > nSize - nOffset)) {
            return false;
        }
        if (nBlockType == BLOCK_TYPE_STORED) {
            nRawSize += nBlockSize;
        } else if (nBlockType == BLOCK_TYPE_PACKED) {
            QByteArray baBlock;
            if (!XDclDecoder::decode(baPacked.mid(static_cast<int>(nOffset),
                                                 static_cast<int>(nBlockSize)),
                                     &baBlock, RID_MAX_MEMBER_SIZE, nullptr)) {
                return false;
            }
            nRawSize += baBlock.size();
        } else {
            return false;
        }
        nOffset += nBlockSize;
        nBlocks++;
        // Budget exhausted: everything seen so far was well formed, which is
        // all detection asks for.  *pbComplete stays false so the caller knows
        // the plaintext length is not yet the member's full length.
        if (nRawSize >= nMaxOutput) return true;
    }
    return false;
}
