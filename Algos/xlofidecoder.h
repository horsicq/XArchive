/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLOFIDECODER_H
#define XLOFIDECODER_H

#include "xbinary.h"

// Solaris compressed lofi disk image (what `lofiadm -C lzma` writes).
//
// The container is a header plus a segment index plus the segments; there is no
// per-member framing, so the whole image is one logical output.  The decoder
// therefore re-reads the header itself rather than taking the geometry through
// FPART properties: a single (offset, size, method) triple is all the dispatch
// can carry, and the triple points at the start of the file.
//
//   +0x00  char[36]   algorithm name, "lzma" then 32 zero bytes
//   +0x24  u32 BE     segment size (0x20000 in every known image)
//   +0x28  u32 BE     number of index entries
//   +0x2c  u32 BE     size of the final segment, 1..segmentSize
//   +0x30  u64 BE[n]  index; entry i is the offset of segment i measured from
//                     the end of the index, entry 0 is 0, and the LAST entry is
//                     the end of the segment data rather than a segment start.
//                     There are therefore n-1 segments.
//
// Each segment is [u8 0x01][13-byte LZMA "alone" header][LZMA data]; the alone
// header carries the real props byte, the dictionary size and the segment's
// uncompressed length, which is segmentSize for every segment but the last.
// Nothing is checksummed - the LZMA streams are the only integrity check.
//
// Only the "lzma" flavour is implemented.  `lofiadm -C gzip` writes the same
// container with a "gzip" name field and deflate segments; no sample of that
// exists in the reference corpus, so XLOFI refuses it outright rather than
// guessing at its segment framing.
//
// Reference: handler "LOFI" (class yfa, entry A431), detector at VA
// 0x004432a0 with the name/geometry checks at 0x00443240.
class XLOFIDecoder {
public:
    // Container geometry, shared by XLOFI and the decoder so that only one copy
    // of the layout rules exists.
    struct GEOMETRY {
        qint64 nSegmentSize;
        qint64 nLastSegmentSize;
        qint64 nIndexEntries;   // segments + 1
        qint64 nDataOffset;     // first byte after the index
        qint64 nDataSize;       // index[last]
        qint64 nImageSize;      // total uncompressed size
    };

    // Parses the header and index out of the first bytes of the file.  baHeader
    // may be a prefix of the container; parse() reports what it needs.
    static bool parseGeometry(const QByteArray &baHeader, qint64 nInputSize,
                              GEOMETRY *pGeometry, QList<qint64> *pListIndex);

    // Decodes a whole image.  `packed` must start at offset 0 of the container
    // and reach at least to nDataOffset + nDataSize.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XLOFIDECODER_H
