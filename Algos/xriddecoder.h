/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRIDDECODER_H
#define XRIDDECODER_H

#include <QByteArray>
#include <QtGlobal>

// Payload codec of the "RID" OS/2 installer archive (OS2YOU / LANTERM /
// TERM2 / DRIVERS packages, 1998-99).
//
// A member's payload is NOT a single compressed stream: it is a chain of
// framed blocks, each frame being
//
//     u16 nBlockSize (little endian)   u8 nBlockType
//
// followed by nBlockSize payload bytes.  Three block types exist:
//
//     0x00  stored   - the nBlockSize bytes are plaintext, copied verbatim
//     0x01  packed   - the nBlockSize bytes are ONE COMPLETE PKWARE Data
//                      Compression Library ("implode"/blast) stream, prelude
//                      bytes and end-of-stream code included.  The decoder is
//                      re-initialised per block, so blocks share no window and
//                      no bit buffer; the plaintext is simply concatenated.
//     0xff  end      - always with nBlockSize == 0, terminates the member.
//                      A zero-length 0x00 or 0x01 block is malformed.
//
// The member header's uncompressed size is the only length authority, so a
// completed chain whose plaintext length disagrees with it is a decode
// failure, never a partial result.
//
// NO NEW CODEC MATHS LIVES HERE.  The per-block stream is decoded by the
// existing XDclDecoder (the in-tree blast port already used by
// HANDLE_METHOD_PKWARE_DCL_IMPLODE).  That the two agree was verified block by
// block: the eight static tables at 0x7bb96c/0x7bbb6c/0x7bbc6c/0x7bbc7c/
// 0x7bbc8c/0x7bbcac/0x7bbcbc/0x7bbcfc inside the reference tool spell exactly
// the PKWARE DCL literal/length/distance code (end-of-stream length 519), and
// XDclDecoder reproduces all 754 packed blocks of the 41-file reference corpus
// byte for byte.
class XRidDecoder {
public:
    enum {
        BLOCK_FRAME_SIZE = 3,
        BLOCK_TYPE_STORED = 0x00,
        BLOCK_TYPE_PACKED = 0x01,
        BLOCK_TYPE_END = 0xff
    };

    // Decodes one member's whole block chain.  baPacked must start at the
    // first frame and cover the chain exactly up to and including the
    // terminator.  Succeeds only when exactly nUncompressedSize bytes come
    // out and every byte of baPacked has been consumed.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pbaUnpacked);

    // Structural walk with no decompression at all: follows the frames from
    // pData and reports how many bytes the chain occupies.  Used while
    // parsing, where the member boundary is needed but the plaintext is not.
    static bool measure(const uchar *pData, qint64 nSize, qint64 *pnConsumed,
                        qint32 *pnBlocks);

    // Bounded trial decode for detection.  Walks the chain, decoding blocks,
    // and stops as soon as more than nMaxOutput plaintext bytes have been
    // produced.  *pbComplete says whether the terminator was reached inside
    // that budget, in which case *pnRawSize is the member's full plaintext
    // length and can be cross-checked against the header.
    static bool probe(const QByteArray &baPacked, qint64 nMaxOutput,
                      bool *pbComplete, qint64 *pnRawSize);
};

#endif  // XRIDDECODER_H
