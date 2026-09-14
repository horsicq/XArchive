/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCREATEINSTALLDECODER_H
#define XCREATEINSTALLDECODER_H

#include <QList>
#include <QString>
#include <QStringList>

#include "xbinary.h"

// Codec and structural walk of the CreateInstall "instcrin" self-extractor, the
// pre-Gentee generation of Gentee Inc.'s CreateInstall builder.  The carrier is
// a PE32 stub whose overlay opens with one compressed stream holding the
// installer runtime ("instcrin.dll"), then a fixed prelude, then the member
// records.  The newer, Gentee-engine CreateInstall installers are a different
// container and belong to XCreateInstall / XGentee, not here.
//
// THE CODEC.  One stream is an LZ77 front end over an adaptive Huffman coder.
// There is no header of any kind: the decoder simply starts reading bits.
//
//   alphabet     629 symbols
//                  0..255  literal byte
//                  256     end of stream
//                  257..628  (length, distance-slot) pair:
//                              nCode  = symbol - 257
//                              length = nCode % 62 + 3      (3..64)
//                              slot   = nCode / 62          (0..5)
//   distance     slot extra bits {4, 6, 8, 10, 12, 14}, read LSB-first, added
//                to the slot base {0, 16, 80, 336, 1360, 5456} (the running sum
//                of 1 << bits) AND to the length:
//                    back = extra + length + base[slot]
//                so the shortest reachable distance is the match length itself.
//   window       32 KiB, restarted empty for every stream
//   bit order    MSB first inside each byte
//
// The Huffman tree is a 1257-node array (leaves 629..1257 carry symbols
// 0..628).  It starts as the complete tree left[i] = 2i, right[i] = 2i+1,
// parent[i] = i/2 with every node weighted 1, and after each decoded symbol the
// leaf's weight is incremented and the leaf is swapped up past any lighter
// uncle, recomputing the ancestors' weights.  When the root reaches weight 2000
// every weight is halved.  The tree is NOT shared between streams.
//
// A stream carries at most 4,000,000 decoded bytes; a member larger than that
// is written as a chain of streams, one after the other with no framing, and
// only the member's declared size says how many of them belong to it.  Nothing
// records a stream's compressed length, so the end of a member's data can only
// be found by decoding it - which is why walkContainer() decodes the whole
// container in order to produce the member table.
//
// THE CONTAINER.  Everything below is an offset inside the PE overlay.
//
//   0x0000        the runtime stream.  Its first eight bytes are always
//                 61 57 41 57 AE 40 60 1B: the runtime is a PE and the coder
//                 starts from a fixed state, so the compressed form of its MZ
//                 header is a constant.  That constant is the detection
//                 signature.
//   +consumed     a gap of builder padding.  The prelude is found by reading a
//                 little-endian dword and sliding forward a byte at a time
//                 (at most 0x101 times) until one is the size of the whole
//                 carrier file; the reference implementation stops on the
//                 weaker "top byte is zero" instead, which this walk falls
//                 back to.
//   prelude       0x12 bytes, then 0x0c or 0x0e bytes, then 0x2f: 77 or 79 in
//                 total.  The dword at prelude+8 is how many further bytes of
//                 builder script follow the prelude.  The 77/79 choice tracks
//                 the builder version; it is resolved by validating the record
//                 header each candidate lands on, because the reference's own
//                 rule ("the runtime decodes to exactly 0x10000 bytes") is
//                 wrong for the oldest of the samples.
//   records       the member chain, 17 bytes each:
//                   0x00 quint8   type  1 file, 2 enter directory,
//                                       3 leave directory, 4 end of archive
//                   0x01 quint8   flag  0 in 252 of the 254 records seen; the
//                                       two exceptions are ordinary files.  The
//                                       reference reads type and flag as ONE
//                                       little-endian word and rejects the
//                                       record when the flag is set, which
//                                       truncates one of the eleven samples.
//                   0x02 quint32  Win32 file attributes
//                   0x06 quint64  Windows FILETIME
//                   0x0e quint8   method  0 packed, 1 stored
//                   0x0f qint16   name length in bytes
//                 followed by that many ANSI name bytes and, for type 1 only,
//                 a qint32 decoded size and then the member's stream chain.
//                 A type 1 record of size 0 carries no stream at all.
//                 Type 4 ends the archive and is its last 17 bytes.
class XCreateInstallDecoder {
public:
    // One published member of the container.  Offsets are relative to the
    // payload (the PE overlay), not to the file.
    struct RECORD {
        QString sFileName;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint32 nAttributes;
        quint64 nFileTime;
        bool bStored;
        quint8 nFlag;
    };

    // 61 57 41 57 AE 40 60 1B - see the container note above.
    static const qint32 SIGNATURE_SIZE = 8;
    static bool checkSignature(const quint8 *pData, qint64 nSize);

    // The member codec as XDecompress calls it: `packed` is the member's whole
    // stream extent and the result is exactly nUncompressedSize bytes.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize, QByteArray *punpacked, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Structural walk of a whole payload.  nContainerOffset is where the
    // payload starts in the carrier, and is used only to recognise the
    // carrier's own size inside the prelude.  *pnArchiveSize comes back as the
    // payload-relative end of the record chain.
    static bool walkContainer(const QByteArray &baPayload, qint64 nContainerOffset, QList<RECORD> *plistRecords, qint64 *pnArchiveSize,
                              XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    static bool decodeStream(const quint8 *pData, qint64 nSize, qint64 nOutputLimit, QByteArray *pOutput, qint64 *pnConsumed, qint64 *pnProduced,
                             XBinary::PDSTRUCT *pPdStruct);
    static bool isRecordHeaderValid(const QByteArray &baPayload, qint64 nOffset);
    static void composeParts(const QStringList &listDirectories, const QByteArray &baRawName, QStringList *plistParts);
};

#endif  // XCREATEINSTALLDECODER_H
