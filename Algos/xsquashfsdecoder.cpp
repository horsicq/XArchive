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
#include "xsquashfsdecoder.h"

#include "algo_utils.h"
#include "xalgo_local.h"

#include "include/zlib.h"

#include <QMap>
#include <QtEndian>

#include <cstring>

namespace {
const qint64 SQFS_META_MAX = 8192;
const qint64 SQFS_MIN_SIZE = 0x80;
const qint32 SQFS_MAX_DEPTH = 64;
const qint32 SQFS_MAX_MEMBERS = 500000;
const qint32 SQFS_MAX_CHUNKS = 200000;
const qint64 SQFS_MAX_BLOCK_SIZE = 8 * 1024 * 1024;
const qint32 SQFS_MAX_NAME = 256;

const quint32 SQFS_DESCRIPTOR_MAGIC = 0x44465153;  // 'SQFD'

const qint32 SQFS_KIND_STORED = 0;
const qint32 SQFS_KIND_COMPRESSED = 1;
const qint32 SQFS_KIND_SPARSE = 2;

const qint32 SQFS_COMP_ZLIB = 0;
const qint32 SQFS_COMP_LZMA = 2;
const qint32 SQFS_COMP_XZ = 4;

quint16 sqfsU16(const quint8 *pData, bool bBigEndian)
{
    return bBigEndian ? qFromBigEndian<quint16>(pData) : qFromLittleEndian<quint16>(pData);
}

quint32 sqfsU32(const quint8 *pData, bool bBigEndian)
{
    return bBigEndian ? qFromBigEndian<quint32>(pData) : qFromLittleEndian<quint32>(pData);
}

quint64 sqfsU64(const quint8 *pData, bool bBigEndian)
{
    return bBigEndian ? qFromBigEndian<quint64>(pData) : qFromLittleEndian<quint64>(pData);
}

bool sqfsInflate(const quint8 *pData, qint64 nSize, qint64 nMaxOut, QByteArray *pbaOut)
{
    if ((nSize <= 0) || (nMaxOut <= 0) || (nSize > 0x7fffffff) || (nMaxOut > 0x7fffffff)) return false;
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, 15) != Z_OK) return false;
    QByteArray baBlock((qint32)nMaxOut, (char)0);
    stream.next_in = (Bytef *)pData;
    stream.avail_in = (uInt)nSize;
    stream.next_out = (Bytef *)baBlock.data();
    stream.avail_out = (uInt)nMaxOut;
    const int nStatus = inflate(&stream, Z_NO_FLUSH);
    const qint32 nProduced = (qint32)nMaxOut - (qint32)stream.avail_out;
    inflateEnd(&stream);
    if ((nStatus != Z_STREAM_END) && (nStatus != Z_OK) && (nStatus != Z_BUF_ERROR)) return false;
    if (nProduced <= 0) return false;
    baBlock.truncate(nProduced);
    *pbaOut = baBlock;

    return true;
}

// The squashfs-lzma variant: five props bytes and NO size field.  The plain
// 13-byte "alone" header is tried second, exactly as the reference does.
bool sqfsLzmaAlone(const quint8 *pData, qint64 nSize, qint64 nMaxOut, QByteArray *pbaOut)
{
    if ((nSize <= 5) || (nMaxOut <= 0) || (nMaxOut > 0x7fffffff)) return false;

    for (qint32 nVariant = 0; nVariant < 2; nVariant++) {
        const qint64 nHeaderSize = (nVariant == 0) ? 5 : 13;
        if (nSize <= nHeaderSize) continue;

        CLzmaDec state;
        X_LzmaDec_Construct(&state);
        if (X_LzmaDec_Allocate(&state, (const Byte *)pData, 5, Algo_utils::lzmaAlloc()) != 0) continue;
        X_LzmaDec_Init(&state);

        QByteArray baBlock((qint32)nMaxOut, (char)0);
        const quint8 *pInput = pData + nHeaderSize;
        qint64 nInputLeft = nSize - nHeaderSize;
        qint64 nProduced = 0;
        bool bOk = true;

        while ((nProduced < nMaxOut) && (nInputLeft > 0)) {
            SizeT nDestLength = (SizeT)(nMaxOut - nProduced);
            SizeT nSourceLength = (SizeT)nInputLeft;
            ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
            const SRes ret = X_LzmaDec_DecodeToBuf(&state, (Byte *)baBlock.data() + nProduced, &nDestLength, (const Byte *)pInput, &nSourceLength,
                                                  LZMA_FINISH_ANY, &status);
            if (ret != 0) {
                bOk = false;
                break;
            }
            nProduced += (qint64)nDestLength;
            pInput += nSourceLength;
            nInputLeft -= (qint64)nSourceLength;
            if ((nDestLength == 0) && (nSourceLength == 0)) break;
            if (status == LZMA_STATUS_FINISHED_WITH_MARK) break;
        }
        X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

        if (bOk && (nProduced > 0)) {
            baBlock.truncate((qint32)nProduced);
            *pbaOut = baBlock;
            return true;
        }
    }

    return false;
}

// The reference sniffs the first bytes before it trusts the compressor field,
// because v1..v3 have no such field at all.
bool sqfsDecompress(const quint8 *pData, qint64 nSize, qint64 nMaxOut, qint32 nHint, QByteArray *pbaOut)
{
    if (nSize <= 0) {
        *pbaOut = QByteArray();
        return true;
    }

    qint32 nOrder[3] = {SQFS_COMP_ZLIB, SQFS_COMP_LZMA, SQFS_COMP_XZ};
    if (nHint == SQFS_COMP_LZMA) {
        nOrder[0] = SQFS_COMP_LZMA;
        nOrder[1] = SQFS_COMP_ZLIB;
        nOrder[2] = SQFS_COMP_XZ;
    } else if (nHint == SQFS_COMP_XZ) {
        nOrder[0] = SQFS_COMP_XZ;
        nOrder[1] = SQFS_COMP_LZMA;
        nOrder[2] = SQFS_COMP_ZLIB;
    }

    qint32 nFirst = -1;
    if ((pData[0] == 0x5d) && (nSize >= 3) && (pData[1] == 0) && (pData[2] == 0)) {
        nFirst = SQFS_COMP_LZMA;
    } else if (pData[0] == 0x78) {
        nFirst = SQFS_COMP_ZLIB;
    }
    if (nFirst >= 0) {
        qint32 nRest[3];
        qint32 nCount = 0;
        nRest[nCount++] = nFirst;
        for (qint32 i = 0; i < 3; i++) {
            if (nOrder[i] != nFirst) nRest[nCount++] = nOrder[i];
        }
        for (qint32 i = 0; i < 3; i++) nOrder[i] = nRest[i];
    }

    for (qint32 i = 0; i < 3; i++) {
        if (nOrder[i] == SQFS_COMP_ZLIB) {
            if (sqfsInflate(pData, nSize, nMaxOut, pbaOut)) return true;
        } else if (nOrder[i] == SQFS_COMP_LZMA) {
            if (sqfsLzmaAlone(pData, nSize, nMaxOut, pbaOut)) return true;
        }
        // XZ, LZO, LZ4 and ZSTD blocks are not decoded here.
    }

    return false;
}

struct METABLOCK {
    QByteArray baData;
    qint64 nNext;
};

struct CHUNK {
    qint64 nOffset;
    qint64 nOnDisk;
    qint64 nOutSize;
    qint64 nSkip;
    qint32 nKind;
};

struct SQUASH {
    const quint8 *pData;
    qint64 nSize;
    XSquashFSDecoder::SUPERBLOCK super;
    QMap<qint64, QMap<qint64, METABLOCK> > mapCache;
    QList<XSquashFSDecoder::MEMBER> *pListMembers;
    XBinary::PDSTRUCT *pPdStruct;
};

// A metadata stream: 8 KiB blocks each behind a u16 length header whose 0x8000
// bit means the payload is stored raw.  A block number is a byte offset
// relative to the owning table, except fragment index targets which are
// absolute - those use base 0.
class MetaStream {
public:
    MetaStream(SQUASH *pSquash, qint64 nBase);

    void seek(qint64 nBlock, qint64 nOffset);
    bool read(qint64 nSize, QByteArray *pbaOut);

private:
    bool getBlock(qint64 nBlock, QByteArray *pbaData, qint64 *pnNext);

    SQUASH *m_pSquash;
    qint64 m_nBase;
    qint64 m_nBlock;
    qint64 m_nOffset;
};

MetaStream::MetaStream(SQUASH *pSquash, qint64 nBase)
{
    m_pSquash = pSquash;
    m_nBase = nBase;
    m_nBlock = 0;
    m_nOffset = 0;
}

void MetaStream::seek(qint64 nBlock, qint64 nOffset)
{
    m_nBlock = nBlock;
    m_nOffset = nOffset;
}

bool MetaStream::getBlock(qint64 nBlock, QByteArray *pbaData, qint64 *pnNext)
{
    QMap<qint64, METABLOCK> &mapBlocks = m_pSquash->mapCache[m_nBase];
    if (mapBlocks.contains(nBlock)) {
        const METABLOCK &block = mapBlocks[nBlock];
        *pbaData = block.baData;
        *pnNext = block.nNext;
        return true;
    }

    const qint64 nPosition = m_nBase + nBlock;
    if ((nPosition < 0) || ((nPosition + 2) > m_pSquash->nSize)) return false;
    const quint16 nHeader = sqfsU16(m_pSquash->pData + nPosition, m_pSquash->super.bBigEndian);
    const qint64 nLength = nHeader & 0x7fff;
    if ((nPosition + 2 + nLength) > m_pSquash->nSize) return false;

    METABLOCK block;
    block.nNext = nBlock + 2 + nLength;
    if (nHeader & 0x8000) {
        block.baData = QByteArray((const char *)(m_pSquash->pData + nPosition + 2), (qint32)nLength);
    } else {
        if (!sqfsDecompress(m_pSquash->pData + nPosition + 2, nLength, SQFS_META_MAX, m_pSquash->super.nCompressor, &block.baData)) return false;
    }
    mapBlocks.insert(nBlock, block);
    *pbaData = block.baData;
    *pnNext = block.nNext;

    return true;
}

bool MetaStream::read(qint64 nSize, QByteArray *pbaOut)
{
    QByteArray baResult;
    if (nSize < 0) return false;
    qint64 nLeft = nSize;
    qint32 nGuard = 0;

    while (nLeft > 0) {
        if (++nGuard > 0x10000) return false;
        QByteArray baBlock;
        qint64 nNext = 0;
        if (!getBlock(m_nBlock, &baBlock, &nNext)) return false;
        const qint64 nAvailable = (qint64)baBlock.size() - m_nOffset;
        if (nAvailable <= 0) {
            if (nNext == m_nBlock) return false;
            m_nBlock = nNext;
            m_nOffset = 0;
            continue;
        }
        const qint64 nTake = qMin<qint64>(nAvailable, nLeft);
        baResult.append(baBlock.constData() + m_nOffset, (qint32)nTake);
        m_nOffset += nTake;
        nLeft -= nTake;
        if (m_nOffset >= baBlock.size()) {
            m_nBlock = nNext;
            m_nOffset = 0;
        }
    }

    *pbaOut = baResult;

    return true;
}

bool sqfsBaseInode(SQUASH *pSquash, MetaStream *pStream, qint32 *pnType)
{
    QByteArray baData;
    const qint32 nMajor = pSquash->super.nMajor;
    const bool bBigEndian = pSquash->super.bBigEndian;

    if (nMajor == 1) {
        if (!pStream->read(3, &baData) || (baData.size() != 3)) return false;
        *pnType = (qint32)(sqfsU16((const quint8 *)baData.constData(), bBigEndian) & 0xf);
        return true;
    }
    if (nMajor == 4) {
        if (!pStream->read(8, &baData) || (baData.size() != 8)) return false;
        *pnType = (qint32)sqfsU16((const quint8 *)baData.constData(), bBigEndian);
        return true;
    }
    if (!pStream->read(4, &baData) || (baData.size() != 4)) return false;
    *pnType = (qint32)(sqfsU16((const quint8 *)baData.constData(), bBigEndian) & 0xf);

    return true;
}

bool sqfsDirInode(SQUASH *pSquash, MetaStream *pStream, bool bExtended, qint64 *pnSize, qint64 *pnOffset, qint64 *pnStartBlock)
{
    const qint32 nMajor = pSquash->super.nMajor;
    const bool bBigEndian = pSquash->super.bBigEndian;
    QByteArray baData;

    if ((nMajor == 1) || (nMajor == 2)) {
        if (bExtended && (nMajor == 2)) {
            if (!pStream->read(4, &baData) || (baData.size() != 4)) return false;
            const quint32 nWord = sqfsU32((const quint8 *)baData.constData(), bBigEndian);
            *pnSize = (qint64)(nWord & 0x7ffffff);
            QByteArray baLow;
            if (!pStream->read(1, &baLow) || (baLow.size() != 1)) return false;
            *pnOffset = (qint64)(nWord >> 27) + (qint64)(quint8)baLow.at(0) * 32;
        } else {
            if (!pStream->read(4, &baData) || (baData.size() != 4)) return false;
            const quint32 nWord = sqfsU32((const quint8 *)baData.constData(), bBigEndian);
            *pnSize = (qint64)(nWord & 0x7ffff);
            *pnOffset = (qint64)(nWord >> 19);
        }
        QByteArray baSkip;
        if (!pStream->read(4, &baSkip) || (baSkip.size() != 4)) return false;  // mtime
        QByteArray baStart;
        if (!pStream->read(3, &baStart) || (baStart.size() != 3)) return false;
        const quint8 *pStart = (const quint8 *)baStart.constData();
        if (bBigEndian) {
            *pnStartBlock = ((qint64)pStart[0] << 16) | ((qint64)pStart[1] << 8) | (qint64)pStart[2];
        } else {
            *pnStartBlock = (qint64)pStart[0] | ((qint64)pStart[1] << 8) | ((qint64)pStart[2] << 16);
        }
        return true;
    }

    if (nMajor == 3) {
        QByteArray baSkip;
        if (!pStream->read(12, &baSkip) || (baSkip.size() != 12)) return false;  // mtime, inode number, nlink
        if (bExtended) {
            if (!pStream->read(4, &baData) || (baData.size() != 4)) return false;
            const quint32 nWord = sqfsU32((const quint8 *)baData.constData(), bBigEndian);
            *pnSize = (qint64)(nWord & 0x7ffffff);
            QByteArray baLow;
            if (!pStream->read(1, &baLow) || (baLow.size() != 1)) return false;
            *pnOffset = (qint64)(nWord >> 27) + (qint64)(quint8)baLow.at(0) * 32;
        } else {
            if (!pStream->read(4, &baData) || (baData.size() != 4)) return false;
            const quint32 nWord = sqfsU32((const quint8 *)baData.constData(), bBigEndian);
            *pnSize = (qint64)(nWord & 0x7ffff);
            *pnOffset = (qint64)(nWord >> 19);
        }
        QByteArray baStart;
        if (!pStream->read(4, &baStart) || (baStart.size() != 4)) return false;
        *pnStartBlock = (qint64)sqfsU32((const quint8 *)baStart.constData(), bBigEndian);
        *pnSize -= 3;
        return true;
    }

    // v4
    if (bExtended) {
        if (!pStream->read(32, &baData) || (baData.size() != 32)) return false;
        const quint8 *p = (const quint8 *)baData.constData();
        *pnSize = (qint64)sqfsU32(p + 12, bBigEndian);
        *pnStartBlock = (qint64)sqfsU32(p + 16, bBigEndian);
        *pnOffset = (qint64)sqfsU16(p + 26, bBigEndian);
    } else {
        if (!pStream->read(24, &baData) || (baData.size() != 24)) return false;
        const quint8 *p = (const quint8 *)baData.constData();
        *pnStartBlock = (qint64)sqfsU32(p + 8, bBigEndian);
        *pnSize = (qint64)sqfsU16(p + 16, bBigEndian);
        *pnOffset = (qint64)sqfsU16(p + 18, bBigEndian);
    }
    *pnSize -= 3;

    return true;
}

bool sqfsFileInode(SQUASH *pSquash, MetaStream *pStream, bool bExtended, qint64 *pnStart, qint64 *pnFragment, qint64 *pnBlockOffset, qint64 *pnSize)
{
    const qint32 nMajor = pSquash->super.nMajor;
    const bool bBigEndian = pSquash->super.bBigEndian;
    QByteArray baData;

    if (nMajor == 1) {
        if (!pStream->read(12, &baData) || (baData.size() != 12)) return false;
        const quint8 *p = (const quint8 *)baData.constData();
        *pnStart = (qint64)sqfsU32(p + 4, bBigEndian);
        *pnSize = (qint64)sqfsU32(p + 8, bBigEndian);
        *pnFragment = 0xffffffff;
        *pnBlockOffset = 0;
        return true;
    }
    if (nMajor == 2) {
        if (!pStream->read(20, &baData) || (baData.size() != 20)) return false;
        const quint8 *p = (const quint8 *)baData.constData();
        *pnStart = (qint64)sqfsU32(p + 4, bBigEndian);
        *pnFragment = (qint64)sqfsU32(p + 8, bBigEndian);
        *pnBlockOffset = (qint64)sqfsU32(p + 12, bBigEndian);
        *pnSize = (qint64)sqfsU32(p + 16, bBigEndian);
        return true;
    }
    if (nMajor == 3) {
        if (bExtended) {
            if (!pStream->read(36, &baData) || (baData.size() != 36)) return false;
            const quint8 *p = (const quint8 *)baData.constData();
            *pnStart = (qint64)sqfsU64(p + 12, bBigEndian);
            *pnFragment = (qint64)sqfsU32(p + 20, bBigEndian);
            *pnBlockOffset = (qint64)sqfsU32(p + 24, bBigEndian);
            *pnSize = (qint64)sqfsU64(p + 28, bBigEndian);
        } else {
            if (!pStream->read(28, &baData) || (baData.size() != 28)) return false;
            const quint8 *p = (const quint8 *)baData.constData();
            *pnStart = (qint64)sqfsU64(p + 8, bBigEndian);
            *pnFragment = (qint64)sqfsU32(p + 16, bBigEndian);
            *pnBlockOffset = (qint64)sqfsU32(p + 20, bBigEndian);
            *pnSize = (qint64)sqfsU32(p + 24, bBigEndian);
        }
        return true;
    }

    if (bExtended) {
        if (!pStream->read(48, &baData) || (baData.size() != 48)) return false;
        const quint8 *p = (const quint8 *)baData.constData();
        *pnStart = (qint64)sqfsU64(p + 8, bBigEndian);
        *pnSize = (qint64)sqfsU64(p + 16, bBigEndian);
        *pnFragment = (qint64)sqfsU32(p + 36, bBigEndian);
        *pnBlockOffset = (qint64)sqfsU32(p + 40, bBigEndian);
    } else {
        if (!pStream->read(24, &baData) || (baData.size() != 24)) return false;
        const quint8 *p = (const quint8 *)baData.constData();
        *pnStart = (qint64)sqfsU32(p + 8, bBigEndian);
        *pnFragment = (qint64)sqfsU32(p + 12, bBigEndian);
        *pnBlockOffset = (qint64)sqfsU32(p + 16, bBigEndian);
        *pnSize = (qint64)sqfsU32(p + 20, bBigEndian);
    }

    return true;
}

// The index value is an ABSOLUTE file offset of a metadata block, which is why
// the stream is opened on base 0.
bool sqfsFragment(SQUASH *pSquash, qint64 nIndex, qint64 *pnStart, qint64 *pnSize)
{
    const qint32 nMajor = pSquash->super.nMajor;
    const bool bBigEndian = pSquash->super.bBigEndian;
    if ((nMajor == 1) || (nIndex < 0) || (nIndex >= pSquash->super.nFragments)) return false;

    qint64 nBlock = 0;
    qint64 nEntryOffset = 0;
    if (nMajor == 2) {
        const qint64 nIndexPosition = pSquash->super.nFragmentTable + (nIndex >> 10) * 4;
        if ((nIndexPosition < 0) || ((nIndexPosition + 4) > pSquash->nSize)) return false;
        nBlock = (qint64)sqfsU32(pSquash->pData + nIndexPosition, bBigEndian);
        nEntryOffset = (nIndex & 0x3ff) * 8;
    } else {
        const qint64 nIndexPosition = pSquash->super.nFragmentTable + (nIndex >> 9) * 8;
        if ((nIndexPosition < 0) || ((nIndexPosition + 8) > pSquash->nSize)) return false;
        nBlock = (qint64)sqfsU64(pSquash->pData + nIndexPosition, bBigEndian);
        nEntryOffset = (nIndex & 0x1ff) * 16;
    }

    MetaStream stream(pSquash, 0);
    stream.seek(nBlock, nEntryOffset);
    QByteArray baEntry;
    if (nMajor == 2) {
        if (!stream.read(8, &baEntry) || (baEntry.size() != 8)) return false;
        const quint8 *p = (const quint8 *)baEntry.constData();
        *pnStart = (qint64)sqfsU32(p, bBigEndian);
        *pnSize = (qint64)sqfsU32(p + 4, bBigEndian);
        return true;
    }
    if (!stream.read(16, &baEntry) || (baEntry.size() != 16)) return false;
    const quint8 *p = (const quint8 *)baEntry.constData();
    *pnStart = (qint64)sqfsU64(p, bBigEndian);
    *pnSize = (qint64)sqfsU32(p + 8, bBigEndian);

    return true;
}

void sqfsAppendChunk(QList<CHUNK> *pListChunks, qint64 nOffset, qint64 nOnDisk, qint64 nOutSize, qint64 nSkip, qint32 nKind)
{
    CHUNK chunk = {};
    chunk.nOffset = nOffset;
    chunk.nOnDisk = nOnDisk;
    chunk.nOutSize = nOutSize;
    chunk.nSkip = nSkip;
    chunk.nKind = nKind;
    pListChunks->append(chunk);
}

bool sqfsFileData(SQUASH *pSquash, MetaStream *pStream, bool bExtended, const QString &sName, XSquashFSDecoder::MEMBER *pMember)
{
    qint64 nStart = 0;
    qint64 nFragment = 0;
    qint64 nBlockOffset = 0;
    qint64 nSize = 0;
    if (!sqfsFileInode(pSquash, pStream, bExtended, &nStart, &nFragment, &nBlockOffset, &nSize)) return false;
    if ((nSize < 0) || (nSize > XSquashFSDecoder::MAX_UNCOMPRESSED_SIZE)) return false;

    const qint64 nBlockSize = pSquash->super.nBlockSize;
    const bool bBigEndian = pSquash->super.bBigEndian;
    qint64 nBlockCount = 0;
    qint64 nTail = 0;
    if ((pSquash->super.nMajor == 1) || (nFragment == 0xffffffff)) {
        nBlockCount = (nSize + nBlockSize - 1) / nBlockSize;
        nTail = 0;
    } else {
        nBlockCount = nSize / nBlockSize;
        nTail = nSize % nBlockSize;
    }
    if ((nBlockCount < 0) || (nBlockCount > SQFS_MAX_CHUNKS)) return false;

    QList<CHUNK> listChunks;
    qint64 nPosition = nStart;

    for (qint64 i = 0; i < nBlockCount; i++) {
        if (!XBinary::isPdStructNotCanceled(pSquash->pPdStruct)) return false;
        quint32 nEntry = 0;
        QByteArray baEntry;
        if (pSquash->super.nMajor == 1) {
            if (!pStream->read(2, &baEntry) || (baEntry.size() != 2)) return false;
            const quint32 nWord = sqfsU16((const quint8 *)baEntry.constData(), bBigEndian);
            // v1 packs the stored flag as 0x8000; rescale it to 0x1000000.
            nEntry = ((nWord & 0x8000) << 9) | (nWord & 0x7fff);
        } else {
            if (!pStream->read(4, &baEntry) || (baEntry.size() != 4)) return false;
            nEntry = sqfsU32((const quint8 *)baEntry.constData(), bBigEndian);
        }

        const qint64 nOnDisk = (qint64)(nEntry & 0xffffff);
        if (nEntry == 0) {
            qint64 nZeroSize = nBlockSize;
            const qint64 nRemaining = nSize - i * nBlockSize;
            if ((i == (nBlockCount - 1)) && (nRemaining < nBlockSize)) nZeroSize = nRemaining;
            if (nZeroSize < 0) nZeroSize = 0;
            sqfsAppendChunk(&listChunks, 0, 0, nZeroSize, 0, SQFS_KIND_SPARSE);
        } else {
            if ((nPosition < 0) || (nOnDisk < 0) || ((nPosition + nOnDisk) > pSquash->nSize)) return false;
            sqfsAppendChunk(&listChunks, nPosition, nOnDisk, nBlockSize, 0, (nEntry & 0x1000000) ? SQFS_KIND_STORED : SQFS_KIND_COMPRESSED);
        }
        nPosition += nOnDisk;
    }

    if (nTail) {
        qint64 nFragmentStart = 0;
        qint64 nFragmentSize = 0;
        if (!sqfsFragment(pSquash, nFragment, &nFragmentStart, &nFragmentSize)) return false;
        const qint64 nOnDisk = nFragmentSize & 0xffffff;
        if ((nFragmentStart < 0) || (nOnDisk < 0) || ((nFragmentStart + nOnDisk) > pSquash->nSize)) return false;
        sqfsAppendChunk(&listChunks, nFragmentStart, nOnDisk, nTail, nBlockOffset, (nFragmentSize & 0x1000000) ? SQFS_KIND_STORED : SQFS_KIND_COMPRESSED);
    }

    // The smallest byte range that holds every chunk becomes the record's
    // stream; a fully sparse member has none and publishes an empty range.
    qint64 nMinOffset = -1;
    qint64 nMaxEnd = -1;
    for (qint32 i = 0; i < listChunks.count(); i++) {
        if (listChunks.at(i).nKind == SQFS_KIND_SPARSE) continue;
        const qint64 nFrom = listChunks.at(i).nOffset;
        const qint64 nTo = nFrom + listChunks.at(i).nOnDisk;
        if ((nMinOffset < 0) || (nFrom < nMinOffset)) nMinOffset = nFrom;
        if ((nMaxEnd < 0) || (nTo > nMaxEnd)) nMaxEnd = nTo;
    }

    XSquashFSDecoder::MEMBER member;
    member.sFileName = sName;
    member.nUncompressedSize = nSize;
    member.nSpanOffset = (nMinOffset < 0) ? 0 : nMinOffset;
    member.nSpanSize = (nMaxEnd < 0) ? 0 : (nMaxEnd - member.nSpanOffset);

    QByteArray baDescriptor(XSquashFSDecoder::DESCRIPTOR_HEADER_SIZE, (char)0);
    quint8 *pHeader = (quint8 *)baDescriptor.data();
    qToLittleEndian<quint32>(SQFS_DESCRIPTOR_MAGIC, pHeader);
    qToLittleEndian<quint32>((quint32)listChunks.count(), pHeader + 4);
    qToLittleEndian<quint64>((quint64)nSize, pHeader + 8);
    qToLittleEndian<quint32>((quint32)pSquash->super.nCompressor, pHeader + 16);

    for (qint32 i = 0; i < listChunks.count(); i++) {
        QByteArray baChunk(XSquashFSDecoder::CHUNK_SIZE, (char)0);
        quint8 *pChunk = (quint8 *)baChunk.data();
        const qint64 nRelative = (listChunks.at(i).nKind == SQFS_KIND_SPARSE) ? 0 : (listChunks.at(i).nOffset - member.nSpanOffset);
        qToLittleEndian<quint64>((quint64)nRelative, pChunk);
        qToLittleEndian<quint64>((quint64)listChunks.at(i).nOnDisk, pChunk + 8);
        qToLittleEndian<quint64>((quint64)listChunks.at(i).nOutSize, pChunk + 16);
        qToLittleEndian<quint32>((quint32)listChunks.at(i).nSkip, pChunk + 24);
        qToLittleEndian<quint32>((quint32)listChunks.at(i).nKind, pChunk + 28);
        baDescriptor.append(baChunk);
    }
    member.baDescriptor = baDescriptor;

    *pMember = member;

    return true;
}

bool sqfsWalkDir(SQUASH *pSquash, qint64 nStartBlock, qint64 nOffset, qint64 nSize, const QString &sPrefix, qint32 nDepth);

bool sqfsWalk(SQUASH *pSquash, qint64 nBlock, qint64 nOffset, qint32 nWant, const QString &sName, qint32 nDepth)
{
    if (nDepth > SQFS_MAX_DEPTH) return false;
    if (!XBinary::isPdStructNotCanceled(pSquash->pPdStruct)) return false;

    MetaStream stream(pSquash, pSquash->super.nInodeTable);
    stream.seek(nBlock, nOffset);
    qint32 nType = 0;
    if (!sqfsBaseInode(pSquash, &stream, &nType)) return false;
    // Only directories and regular files, in both their basic and extended
    // forms; everything else is skipped by the reference.
    if ((nType != nWant) && (!((nType == 8) && (nWant == 1))) && (!((nType == 9) && (nWant == 2)))) return false;

    if ((nType == 1) || (nType == 8)) {
        qint64 nDirSize = 0;
        qint64 nDirOffset = 0;
        qint64 nDirStart = 0;
        if (!sqfsDirInode(pSquash, &stream, (nType == 8), &nDirSize, &nDirOffset, &nDirStart)) return false;
        return sqfsWalkDir(pSquash, nDirStart, nDirOffset, nDirSize, sName, nDepth);
    }
    if ((nType == 2) || (nType == 9)) {
        if (pSquash->pListMembers->size() >= SQFS_MAX_MEMBERS) return false;
        XSquashFSDecoder::MEMBER member;
        if (!sqfsFileData(pSquash, &stream, (nType == 9), sName, &member)) return false;
        pSquash->pListMembers->append(member);
        return true;
    }

    return false;
}

bool sqfsWalkDir(SQUASH *pSquash, qint64 nStartBlock, qint64 nOffset, qint64 nSize, const QString &sPrefix, qint32 nDepth)
{
    const qint32 nMajor = pSquash->super.nMajor;
    const bool bBigEndian = pSquash->super.bBigEndian;

    MetaStream stream(pSquash, pSquash->super.nDirectoryTable);
    stream.seek(nStartBlock, nOffset);
    qint64 nRemaining = nSize;

    while (nRemaining > 0) {
        if (!XBinary::isPdStructNotCanceled(pSquash->pPdStruct)) return false;
        qint64 nCount = 0;
        qint64 nEntryBlock = 0;
        QByteArray baHeader;

        if ((nMajor == 1) || (nMajor == 2)) {
            if (nRemaining < 4) return false;
            nRemaining -= 4;
            if (!stream.read(4, &baHeader) || (baHeader.size() != 4)) return false;
            const quint32 nWord = sqfsU32((const quint8 *)baHeader.constData(), bBigEndian);
            nCount = (qint64)(nWord & 0xff) + 1;
            nEntryBlock = (qint64)(nWord >> 8);
        } else if (nMajor == 3) {
            if (nRemaining < 9) return false;
            nRemaining -= 9;
            QByteArray baCount;
            if (!stream.read(1, &baCount) || (baCount.size() != 1)) return false;
            nCount = (qint64)(quint8)baCount.at(0) + 1;
            if (!stream.read(4, &baHeader) || (baHeader.size() != 4)) return false;
            nEntryBlock = (qint64)sqfsU32((const quint8 *)baHeader.constData(), bBigEndian);
            QByteArray baSkip;
            if (!stream.read(4, &baSkip) || (baSkip.size() != 4)) return false;
        } else {
            if (nRemaining < 12) return false;
            nRemaining -= 12;
            if (!stream.read(12, &baHeader) || (baHeader.size() != 12)) return false;
            const quint8 *p = (const quint8 *)baHeader.constData();
            nCount = (qint64)sqfsU32(p, bBigEndian) + 1;
            nEntryBlock = (qint64)sqfsU32(p + 4, bBigEndian);
        }

        for (qint64 i = 0; i < nCount; i++) {
            if (!XBinary::isPdStructNotCanceled(pSquash->pPdStruct)) return false;
            qint64 nEntryOffset = 0;
            qint64 nEntryType = 0;
            qint64 nNameSize = 0;
            QByteArray baEntry;

            if (nMajor == 4) {
                if (nRemaining < 8) return false;
                nRemaining -= 8;
                if (!stream.read(8, &baEntry) || (baEntry.size() != 8)) return false;
                const quint8 *p = (const quint8 *)baEntry.constData();
                nEntryOffset = (qint64)sqfsU16(p, bBigEndian);
                nEntryType = (qint64)sqfsU16(p + 4, bBigEndian);
                nNameSize = (qint64)sqfsU16(p + 6, bBigEndian) + 1;
            } else {
                const qint64 nNeed = ((nMajor == 1) || (nMajor == 2)) ? 3 : 5;
                if (nRemaining < nNeed) return false;
                nRemaining -= nNeed;
                if (!stream.read(2, &baEntry) || (baEntry.size() != 2)) return false;
                const quint32 nWord = sqfsU16((const quint8 *)baEntry.constData(), bBigEndian);
                nEntryOffset = (qint64)(nWord & 0x1fff);
                nEntryType = (qint64)(nWord >> 13);
                QByteArray baNameSize;
                if (!stream.read(1, &baNameSize) || (baNameSize.size() != 1)) return false;
                nNameSize = (qint64)(quint8)baNameSize.at(0) + 1;
                if (nMajor == 3) {
                    QByteArray baDelta;
                    if (!stream.read(2, &baDelta) || (baDelta.size() != 2)) return false;
                }
            }

            if (nNameSize > nRemaining) return false;
            nRemaining -= nNameSize;
            if (nNameSize > SQFS_MAX_NAME) return false;
            QByteArray baName;
            if (!stream.read(nNameSize, &baName) || (baName.size() != nNameSize)) return false;

            if ((nEntryType == 1) || (nEntryType == 2)) {
                QString sChild = sPrefix + QChar('/') + QString::fromLatin1(baName.constData(), baName.size());
                if (!sqfsWalk(pSquash, nEntryBlock, nEntryOffset, (qint32)nEntryType, sChild, nDepth + 1)) return false;
            }
        }
    }

    return (nRemaining == 0);
}
}  // namespace

const qint64 XSquashFSDecoder::MAX_UNCOMPRESSED_SIZE;
const qint64 XSquashFSDecoder::MAX_INPUT_SIZE;
const qint32 XSquashFSDecoder::CHUNK_SIZE;
const qint32 XSquashFSDecoder::DESCRIPTOR_HEADER_SIZE;

bool XSquashFSDecoder::parseSuperBlock(const QByteArray &baHeader, qint64 nFileSize, SUPERBLOCK *pSuperBlock)
{
    if (!pSuperBlock) return false;
    if (baHeader.size() < SQFS_MIN_SIZE) return false;
    const quint8 *pData = (const quint8 *)baHeader.constData();

    const quint32 nMagic = qFromLittleEndian<quint32>(pData);
    SUPERBLOCK super;
    memset(&super, 0, sizeof(super));

    if (nMagic == 0x73717368) {  // 'hsqs'
        super.bBigEndian = false;
        super.nCompressor = 0;
    } else if (nMagic == 0x74717368) {  // 'hsqt'
        super.bBigEndian = false;
        super.nCompressor = 3;
    } else if (nMagic == 0x71736873) {  // 'shsq'
        super.bBigEndian = false;
        super.nCompressor = 5;
    } else if (nMagic == 0x68737173) {  // 'sqsh' big endian
        super.bBigEndian = true;
        super.nCompressor = 0;
    } else if (nMagic == 0x68737174) {
        super.bBigEndian = true;
        super.nCompressor = 3;
    } else if (nMagic == 0x68191122) {
        super.bBigEndian = false;
        super.nCompressor = 0;
    } else {
        return false;
    }

    const bool bBigEndian = super.bBigEndian;
    super.nMajor = (qint32)sqfsU16(pData + 0x1c, bBigEndian);
    super.nMinor = (qint32)sqfsU16(pData + 0x1e, bBigEndian);
    if ((super.nMajor < 1) || (super.nMajor > 4)) return false;

    if (super.nMajor == 4) {
        super.nInodes = (qint64)sqfsU32(pData + 4, bBigEndian);
        super.nBlockSize = (qint64)sqfsU32(pData + 12, bBigEndian);
        super.nFragments = (qint64)sqfsU32(pData + 16, bBigEndian);
        const qint32 nCompressorId = (qint32)sqfsU16(pData + 0x14, bBigEndian);
        super.nRootInode = (qint64)sqfsU64(pData + 0x20, bBigEndian);
        super.nBytesUsed = (qint64)sqfsU64(pData + 0x28, bBigEndian);
        super.nInodeTable = (qint64)sqfsU64(pData + 0x40, bBigEndian);
        super.nDirectoryTable = (qint64)sqfsU64(pData + 0x48, bBigEndian);
        super.nFragmentTable = (qint64)sqfsU64(pData + 0x50, bBigEndian);
        if (nCompressorId == 1) {
            super.nCompressor = 0;
        } else {
            super.nCompressor = nCompressorId;
        }
    } else if (super.nMajor == 3) {
        super.nInodes = (qint64)sqfsU32(pData + 4, bBigEndian);
        super.nRootInode = (qint64)sqfsU64(pData + 0x2b, bBigEndian);
        super.nBlockSize = (qint64)sqfsU32(pData + 0x33, bBigEndian);
        super.nFragments = (qint64)sqfsU32(pData + 0x37, bBigEndian);
        super.nBytesUsed = (qint64)sqfsU64(pData + 0x3f, bBigEndian);
        super.nInodeTable = (qint64)sqfsU64(pData + 0x57, bBigEndian);
        super.nDirectoryTable = (qint64)sqfsU64(pData + 0x5f, bBigEndian);
        super.nFragmentTable = (qint64)sqfsU64(pData + 0x67, bBigEndian);
    } else {
        super.nInodes = (qint64)sqfsU32(pData + 4, bBigEndian);
        if (super.nMajor == 1) {
            super.nBlockSize = (qint64)sqfsU16(pData + 0x20, bBigEndian);
        } else {
            super.nBlockSize = (qint64)sqfsU32(pData + 0x33, bBigEndian);
        }
        super.nRootInode = (qint64)sqfsU64(pData + 0x2b, bBigEndian);
        super.nBytesUsed = (qint64)sqfsU32(pData + 8, bBigEndian);
        super.nInodeTable = (qint64)sqfsU32(pData + 0x14, bBigEndian);
        super.nDirectoryTable = (qint64)sqfsU32(pData + 0x18, bBigEndian);
        super.nFragments = (super.nMajor == 2) ? (qint64)sqfsU32(pData + 0x37, bBigEndian) : 0;
        super.nFragmentTable = (super.nMajor == 2) ? (qint64)sqfsU32(pData + 0x3b, bBigEndian) : 0;
    }

    if (super.nBlockSize == 0) {
        const qint32 nBlockLog = (qint32)sqfsU16(pData + 0x22, bBigEndian);
        if ((nBlockLog < 0) || (nBlockLog > 23)) return false;
        super.nBlockSize = (qint64)1 << nBlockLog;
    }
    if ((super.nBlockSize <= 0) || (super.nBlockSize > SQFS_MAX_BLOCK_SIZE)) return false;
    if ((nFileSize > 0) && ((super.nInodeTable > nFileSize) || (super.nDirectoryTable > nFileSize))) return false;

    *pSuperBlock = super;

    return true;
}

bool XSquashFSDecoder::listMembers(const QByteArray &baFile, QList<MEMBER> *pListMembers, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pListMembers) return false;
    if ((baFile.size() < SQFS_MIN_SIZE) || ((qint64)baFile.size() > MAX_INPUT_SIZE)) return false;

    SQUASH squash;
    squash.pData = (const quint8 *)baFile.constData();
    squash.nSize = baFile.size();
    squash.pListMembers = pListMembers;
    squash.pPdStruct = pPdStruct;
    if (!parseSuperBlock(baFile, baFile.size(), &squash.super)) return false;

    // A walk that dies half way keeps everything collected so far.
    sqfsWalk(&squash, squash.super.nRootInode >> 16, squash.super.nRootInode & 0xffff, 1, QString(), 0);

    return !pListMembers->isEmpty();
}

bool XSquashFSDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;
    if (baProperties.size() < DESCRIPTOR_HEADER_SIZE) return false;

    const quint8 *pDescriptor = (const quint8 *)baProperties.constData();
    if (qFromLittleEndian<quint32>(pDescriptor) != SQFS_DESCRIPTOR_MAGIC) return false;
    const qint64 nChunkCount = (qint64)qFromLittleEndian<quint32>(pDescriptor + 4);
    const qint64 nDeclaredSize = (qint64)qFromLittleEndian<quint64>(pDescriptor + 8);
    const qint32 nCompressor = (qint32)qFromLittleEndian<quint32>(pDescriptor + 16);
    if (nDeclaredSize != nUncompressedSize) return false;
    if ((nChunkCount < 0) || (nChunkCount > SQFS_MAX_CHUNKS)) return false;
    if (baProperties.size() < (DESCRIPTOR_HEADER_SIZE + nChunkCount * CHUNK_SIZE)) return false;

    QByteArray baResult;
    baResult.reserve((qint32)qMin<qint64>(nUncompressedSize + 1, 0x400000));

    for (qint64 i = 0; i < nChunkCount; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const quint8 *pChunk = pDescriptor + DESCRIPTOR_HEADER_SIZE + i * CHUNK_SIZE;
        const qint64 nOffset = (qint64)qFromLittleEndian<quint64>(pChunk);
        const qint64 nOnDisk = (qint64)qFromLittleEndian<quint64>(pChunk + 8);
        const qint64 nOutSize = (qint64)qFromLittleEndian<quint64>(pChunk + 16);
        const qint64 nSkip = (qint64)qFromLittleEndian<quint32>(pChunk + 24);
        const qint32 nKind = (qint32)qFromLittleEndian<quint32>(pChunk + 28);

        if ((nOffset < 0) || (nOnDisk < 0) || (nOutSize < 0) || (nSkip < 0)) return false;
        if (nOutSize > MAX_UNCOMPRESSED_SIZE) return false;

        if (nKind == SQFS_KIND_SPARSE) {
            if (((qint64)baResult.size() + nOutSize) > MAX_UNCOMPRESSED_SIZE) return false;
            baResult.append((qint32)nOutSize, (char)0);
            continue;
        }
        if ((nOffset > baPacked.size()) || (nOnDisk > ((qint64)baPacked.size() - nOffset))) return false;

        QByteArray baBlock;
        if (nKind == SQFS_KIND_STORED) {
            baBlock = QByteArray(baPacked.constData() + nOffset, (qint32)nOnDisk);
        } else {
            if (!sqfsDecompress((const quint8 *)baPacked.constData() + nOffset, nOnDisk, nOutSize + nSkip, nCompressor, &baBlock)) return false;
        }
        if (nSkip > 0) {
            if (nSkip >= baBlock.size()) {
                baBlock.clear();
            } else {
                baBlock = baBlock.mid((qint32)nSkip, (qint32)nOutSize);
            }
        }
        if ((qint64)baBlock.size() > nOutSize) baBlock.truncate((qint32)nOutSize);
        if (((qint64)baResult.size() + (qint64)baBlock.size()) > MAX_UNCOMPRESSED_SIZE) return false;
        baResult.append(baBlock);
    }

    if ((qint64)baResult.size() > nUncompressedSize) baResult.truncate((qint32)nUncompressedSize);
    if ((qint64)baResult.size() != nUncompressedSize) return false;
    *pbaResult = baResult;

    return true;
}
