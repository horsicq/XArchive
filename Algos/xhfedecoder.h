/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHFEDECODER_H
#define XHFEDECODER_H

#include <QByteArray>

#include "xbinary.h"

// HxC Floppy Emulator "HFE" v1 image (the SDCard HxC / DSKAnnnn files).  The
// container does not store sectors at all: it stores the raw magnetic flux of
// each track as an MFM bit cell stream, so turning it into something a reader
// can use means running the same recovery a floppy controller would.
//
// File header (rev 0):
//     +0x00 char[8] "HXCPICFE"
//     +0x08 u8      format revision, must be 0
//     +0x09 u8      number of tracks (must not be 0)
//     +0x0a u8      number of sides, 1 or 2
//     +0x0b u8      track encoding (0 = ISOIBM_MFM)
//     +0x0c u16     bit rate in kbit/s
//     +0x0e u16     nominal RPM
//     +0x10 u8      floppy interface mode
//     +0x11 u8      unused
//     +0x12 u16     track look-up-table offset, in 512-byte blocks
//     ...
// Track LUT entry (4 bytes per track): u16 offset in 512-byte blocks,
// u16 length in bytes.  A track's data holds BOTH sides, interleaved in
// 256-byte chunks (side 0 chunk, side 1 chunk, ...), and each byte carries
// eight bit cells LSB first.
//
// The cell stream is IBM System 34 MFM: clock and data cells alternate, an
// address mark is the 0xA1 byte written with a missing clock (cell pattern
// 0x4489), and a sector is the pair
//     A1 A1 A1 FE  cyl head sector sizecode crc16       (ID field)
//     A1 A1 A1 FB  data[128 << sizecode]     crc16      (data field)
// with CRC-16/CCITT seeded 0xffff over the three A1s, the mark and the field.
//
// This decoder rebuilds the flat sector image (cylinder-major, then head, then
// sector 1..n).  Both CRCs are checked; a sector that fails, or that no ID
// field ever named, is emitted as zeroes so the image keeps its geometry.
class XHFEDecoder {
public:
    struct GEOMETRY {
        qint32 nTracks = 0;
        qint32 nSides = 0;
        qint32 nSectorsPerTrack = 0;
        qint32 nSectorSize = 0;
        qint64 nImageSize = 0;
    };

    // Reads the header and decodes cylinder 0 far enough to learn the sector
    // count and sector size, i.e. the size of the image decode() will produce.
    static bool probeGeometry(const QByteArray &baFile, GEOMETRY *pGeometry,
                              XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Produces the flat sector image.  nImageSize must be the value
    // probeGeometry() reported.
    static bool decode(const QByteArray &baFile, qint64 nImageSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    XHFEDecoder() = delete;
};

#endif  // XHFEDECODER_H
