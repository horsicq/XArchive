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
#ifndef XWPKARCHIVE_H
#define XWPKARCHIVE_H

#include "xarchive.h"

// WPK - the Watcom installer "pack" archive (magic 03 24 01 01 / 03 24 33 01).
//
//   0x00  4  u32 magic, 0x01012403 or 0x01332403
//   0x04  2  u16 member count, non zero
//   0x06  2  u16 directory size in bytes
//   0x08  4  u32 directory file offset; directorySize + directoryOffset is the
//            file size exactly, and the offset is past the 0x0C byte header
//   0x0C ..  member payloads, back to back, ending where the directory begins
//
// The directory is a run of VARIABLE length records, parsed sequentially while
// more than 0x10 bytes of it remain:
//
//   +0x00  4  i32 uncompressed size
//   +0x04  4  i32 payload file offset
//   +0x08  4  u32 timestamp (Unix time_t)
//   +0x0C  4  u32 CRC-32
//   +0x10  1  u8  low seven bits = name length; bit 7 clear -> method A,
//                 bit 7 set -> method B
//   +0x11  n  name, no NUL terminator
//
// TRAP 1 - A MEMBER DOES NOT STORE ITS COMPRESSED SIZE.  Only the payload
// offset is recorded, and the directory is in no particular order, so the next
// member's offset is not an end marker.  The only sound bound is the payload
// area itself, which ends where the directory begins: a record therefore
// publishes [payloadOffset, directoryOffset - payloadOffset] and the decoder
// stops on its own once nUncompressedSize bytes have come out.  Handing over
// the whole rest of the FILE instead would let a truncated or hostile archive
// run the LZSS loop over the directory it was validated against.
//
// TRAP 2 - THE CRC-32 COVERS THE COMPRESSED BYTES, NOT THE PLAINTEXT.  It is
// the CRC of exactly the prefix of the payload the decoder consumed, which is
// why XWPKDecoder reports the consumed length.  Checking it against the decoded
// bytes fails on every member of every archive; and because it validates the
// COMPRESSED side it is the only thing that can tell a correct method A decode
// from a wrong one (see TRAP 3).  Both the CRC and the sort selector therefore
// travel to the decoder in FPART_PROP_COMPRESSPROPERTIES, as the eight little
// endian bytes decoderProperties() builds:
//
//   u32 sorter  - an XWPKDecoder::SORTER value, already resolved by the reader
//   u32 crc32   - the directory's CRC-32 of the consumed compressed prefix
//
// TRAP 3 - THE METHOD A CODE-LENGTH SORT IS PART OF THE FORMAT AND NOTHING IN
// THE FILE SAYS WHICH ONE.  The sort that orders the transmitted code lengths
// is unstable, and which symbol a run of equal lengths is left in decides the
// whole canonical alphabet.  The reference implementation carries two of them
// and PROBES, and it has to: of the 127 archives of the reference corpus 122
// decode with SORTER_A and five - the members of _DBLIBWC, _DBVALID, _DBLIBBP2,
// _DBERASE and _AABBCC - decode only with SORTER_B, while ALL 127 carry the
// same magic 0x01012403.  The magic does not discriminate; picking a sorter
// from it (which the format notes elsewhere claim) quietly CORRUPTS those five,
// because the wrong sort still produces output of exactly the right length
// rather than failing.
//
// resolveSorter() therefore decodes real members and keeps the first sorter
// whose CONSUMED-PREFIX CRC matches the directory - the decode succeeding is
// not evidence on its own.  It is settled once per archive over the few
// smallest method A members, not per member, which is what the reference does
// too; probing several members rather than only the first also covers the case
// where a small member's code lengths happen to have no ties at all and both
// sorts agree on it.
//
// UNVERIFIED: the reference corpus contains no method B member and no archive
// with magic 0x01332403, so the method B path and the magic-implied SORTER_B
// preference below are transcriptions of the format, not measured behaviour.
class XWPKArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        quint32 nTime;
        bool bMethodB;
        QString sFileName;
    };

    explicit XWPKArchive(QIODevice *pDevice = nullptr);
    ~XWPKArchive() override;

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

    // The eight byte FPART_PROP_COMPRESSPROPERTIES blob described above:
    // u32 sorter, u32 CRC-32, both little endian.
    static QByteArray decoderProperties(quint32 nSorter, quint32 nCRC);
    // Reads that blob back. Returns false for anything but exactly eight bytes.
    static bool parseDecoderProperties(const QByteArray &baProperties, quint32 *pnSorter, quint32 *pnCRC);

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        quint32 nMagic;
        quint32 nSorter;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // Trial decode of the few smallest method A members, keeping the first
    // sorter whose consumed-prefix CRC matches for all of them. Returns false
    // only when the device died or the operation was cancelled; a probe that
    // settles on nothing leaves the magic-implied default in place so that
    // extraction fails visibly instead of producing wrong bytes.
    bool resolveSorter(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(bool bMethodB);
    static HANDLE_METHOD methodToHandleMethod(bool bMethodB);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XWPKARCHIVE_H
