/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPAPERPORTDECODER_H
#define XPAPERPORTDECODER_H

#include "xbinary.h"

// Visioneer PaperPort image object renderer.
//
// A PaperPort page is not stored as a file: it is an image object whose pixels
// live in a rectangular grid of independently coded tiles.  The extractor's
// only sensible output is the assembled page, so this decoder consumes the
// whole image object (everything after its 32-byte "VZ" chunk header) and
// produces a bottom-up 1 bpp Windows BMP - byte for byte what the reference
// extractor writes.
//
// Object layout, all offsets relative to the object payload start:
//   +10 quint16 nWidth
//   +12 quint16 nHeight
//   +20 quint16 nBitsPerPixel   1, 8 or 24 (only 1 is renderable here)
//   +34 qint16  nTag            always 5
//   +60 qint32  nTileHeaderOff
//   +68 qint32  nTileDataOff
// Tile header (14 bytes at nTileHeaderOff):
//   +0 quint16 0x000E    +4 quint16 nTileWidth (bytes when 1 bpp)
//   +6 quint16 nTileHeight   +8 quint16 nTilesAcross  +10 quint16 nTilesDown
// then one 10-byte directory entry per tile
//   quint16 nCompression (1 = raw, 2 = line coded, 4 = JPEG)
//   qint32  nRecordSize   qint32 nRawSize
// and, at nTileDataOff, the records themselves, each
//   quint16 nIndex   quint16 nTag (0x43, or 0x44 = stored full-width tile)
// followed by nRecordSize - 4 payload bytes.
//
// Compression 2 is a Visioneer line wrapper around CCITT T.6 two-dimensional
// coding: each row starts with a 2-bit selector - 0 uncompressed row, 1 an
// explicit big-endian list of changing element positions, 2 a G4 2D coded row
// against the previous row, 3 a 6-bit count of blank rows.  Compression 4
// (JPEG) is not implemented and is reported as a failure rather than rendered
// wrongly.
class XPaperPortDecoder {
public:
    struct IMAGEINFO {
        qint32 nWidth;
        qint32 nHeight;
        qint32 nBitsPerPixel;
        qint32 nStride;
        qint64 nOutputSize;  // size of the BMP this decoder writes
    };

    // Validates the image object header + tile header carried by the first
    // bytes of an object payload and fills in the geometry.
    static bool probe(const QByteArray &object, IMAGEINFO *pInfo);

    // Two-part form for a lister that does not want to read the whole object:
    // probeObjectHeader() validates the 0x88-byte object header and reports
    // where the 14-byte tile header lives, probeGeometry() finishes the job.
    static bool probeObjectHeader(const QByteArray &objectHeader, qint32 *pnTileHeaderOffset);
    static bool probeGeometry(const QByteArray &objectHeader, const QByteArray &tileHeader, IMAGEINFO *pInfo);

    static bool decode(const QByteArray &packed, qint64 nUncompressedSize, QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XPAPERPORTDECODER_H
