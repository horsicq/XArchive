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
#ifndef XWIILZ77_H
#define XWIILZ77_H

#include "xarchive.h"

// Nintendo LZ77 single-stream file: the LZ10 / LZ11 streams the GBA, DS, DSi,
// 3DS and Wii SDKs consume, exposed as an archive with exactly one member.
// Format understanding derived from the wiibrew.org LZ77 page, GBATEK's
// LZ77UnCompReadNormalWrite description, libWiiPy (MIT) and the DSDecmp LZ11
// format notes; the codec lives in XNintendoLZDecoder.
//
// Three carriers are accepted, all evaluated relative to a base offset nBase:
//
//   bare form      nBase = 0      GBA/DS/DSi/3DS files; header at 0
//   tagged form    nBase = 0      "LZ77" at 0 (Wii tooling); header at 4
//   IMD5 wrapper   nBase = 0x20   "IMD5" at 0, 32-byte wrapper, then the
//                                 TAGGED form at 0x20 (banner.bin / icon.bin /
//                                 sound.bin inside every Wii channel banner)
//
// Layout from nBase (integers little endian):
//
//   +0   u8   "LZ77" tag (optional, 4 bytes) - shifts everything below by 4
//   +0   u8   type byte           0x10 = LZ10, 0x11 = LZ11
//   +1   u24  plaintext length    0 means "see the next field"
//   +4   u32  length extension    only when the u24 is 0 (DSDecmp form,
//                                 files past 16 MiB); 1 .. 0x7FFFFFFF
//   +4/8      item stream         flag byte + 8 items, see the decoder
//
// The IMD5 wrapper is 0x20 bytes: "IMD5", u32 big-endian size of the data
// behind the wrapper (reported, not validated), 8 reserved bytes and an MD5
// that is not verified here.  Nothing else is stored anywhere: no name, no
// time stamp, no checksum.
//
// WHAT THE GATE VALIDATES AND WHY.  The magic is one byte (bare form) or five
// (tagged form); neither is a gate on its own.  The one integrity check the
// format has is that the item stream reproduces EXACTLY the declared plaintext
// length, so isValid() trial-decodes the whole stream over a 4 KiB history
// ring and requires: no truncation, no reference before the start of the
// output, no reference past the declared end, and the declared length reached.
// Around that: the plaintext length is 1 .. 512 MiB (a zero-length member
// would write an empty file at exit 0 instead of failing), it may not exceed
// what the stream could possibly encode (LZ10 tops out at 144 plaintext bytes
// per 17 packed, LZ11 at 526464 per 33, so the ratio caps 9 / 16384 are exact
// bounds, not heuristics), and whatever follows the stream is bounded: up to
// 16 bytes of any content behind a tagged stream (tools align to 4 or 16), up
// to 10 bytes behind a bare stream and all of them zero (ndspy leaves one zero
// byte per unused slot of the last flag group, GBATEK pads to 4) except that
// the first may be 0xFF: nlzss11 writes the flag byte of a next group that
// never receives an item whenever the stream ends exactly on a group boundary
// (one file in eight; measured on nlzss11 1.8, both bare and tagged).  The bare
// form additionally demands at least 12 input bytes and 16 plaintext bytes so
// that a six-byte "10 01 00 00 00 XX" cannot claim to be a one-byte member.
// Measured over 53 989 unrelated files, 121 pass the bare header checks and
// every one of them fails the trial decode at its first reference; the
// theoretical residue is a file whose first byte is 0x10/0x11, whose bytes 1-3
// spell exactly the length a random item stream reproduces, whose references
// all point inside what is produced so far, and which ends within 10 zero
// bytes of the stream - a shape no other format produces at offset 0.  The
// bare form therefore has no search signature; only the tagged one does.
//
// The member is named after the container: one of the suffixes .lz77 / .lz /
// .lz10 / .lz11 / .cmp is removed case-insensitively when something remains
// (abc.lz -> abc), anything else keeps the container's name verbatim
// (banner.arc, 00000001.app), and a nameless device yields "lz77_data".
// Whatever the plaintext is - usually a U8 archive on the Wii - is emitted
// as-is; nesting is the unpacker's business.
class XWiiLZ77 final : public XArchive {
    Q_OBJECT

public:
    explicit XWiiLZ77(QIODevice *pDevice = nullptr);
    ~XWiiLZ77() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nBaseOffset;       // 0, or 0x20 behind an IMD5 wrapper
        qint64 nHeaderSize;       // from file offset 0, wrapper and tag included
        qint64 nDataOffset;       // == nHeaderSize
        qint64 nCompressedSize;   // nInputSize - nDataOffset, trailing slack included
        qint64 nConsumedSize;     // stream bytes the trial decode used (0 when not verified)
        qint64 nUncompressedSize;
        qint64 nImd5DeclaredSize; // the wrapper's size field, reported only
        quint8 nType;             // 0x10 / 0x11
        bool bHasTag;
        bool bImd5;
        bool bExtendedLength;
        QString sFileName;
    };

    // bVerifyPayload = true runs the bounded trial decode described above.  It
    // is the gate every entry point that has to be sure uses; getFileParts()
    // and the unpack path need it too, because the plaintext length they
    // publish has to be one the decoder can actually reproduce.
    bool parseContext(CONTEXT *pContext, bool bVerifyPayload, PDSTRUCT *pPdStruct);
    static QString restoreFileName(const QString &sContainerName);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XWIILZ77_H
