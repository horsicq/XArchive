/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHDCOPYDECODER_H
#define XHDCOPYDECODER_H

#include <QByteArray>
#include <QtGlobal>

// HD-COPY disk-image decoder (Oliver Fromme's HD-COPY 1.x/2.x, ".IMG").
//
// Container header:
//
//     +0x00 u8   0xff
//     +0x01 u8   0x18
//     +0x02 u8   label length (0..11)
//     +0x03 char label[11]           padded with spaces, or all NUL when empty
//     +0x0e u8   last cylinder       (79..83 in every known writer)
//     +0x0f u8   sectors per track   (9, 10, 15, 17, 18, 20 or 21)
//     +0x10 u8   trackUsed[168]      2 heads x 84 cylinders, index = cyl*2+head
//     +0xb8      the compressed blocks of the used tracks, in map order
//
// Each used track is one block:  [u16 blockLength][u8 escape][RLE data], where
// blockLength counts the escape byte and the data.  The RLE is
//
//     <escape> <value> <count>   ->  count copies of value
//     <byte>                     ->  that byte
//
// and a track always expands to exactly sectorsPerTrack * 512 bytes, which is
// what makes the format self-checking: the block chain has to consume the file
// exactly and every track has to land on its declared length.
//
// Tracks whose map byte is 0 were never read off the disk.  They are emitted as
// the DOS format filler 0xf6 - the byte HD-COPY writes back for a track it does
// not have.  Note this is the one part of the decode with no oracle behind it:
// the reference tool exposes only the FAT contents of the decoded image, and
// none of the corpus files reference a skipped track, so the filler value is
// taken from the format's documented behaviour rather than from a comparison.
class XHDCopyDecoder {
public:
    static const quint8 FILL_BYTE = 0xf6U;

    // baPacked is the whole container, starting at the 0xff 0x18 magic.
    // nUncompressedSize is the image size the caller published; pass -1 to let
    // the decoder size the image from the header alone.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked);

    // Image size the header describes ((lastCylinder+1) * 2 * sectors * 512),
    // or -1 when the header is not an HD-COPY header.
    static qint64 imageSize(const QByteArray &baHeader);
};

#endif  // XHDCOPYDECODER_H
