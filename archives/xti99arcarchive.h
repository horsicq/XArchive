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
#ifndef XTI99ARCARCHIVE_H
#define XTI99ARCARCHIVE_H

#include "xarchive.h"

// TI99 ARC (.ARK) - the TI-99/4A archiver, normally shipped inside one of the
// two TI file-transfer wrappers.
//
// WRAPPER (0x80 bytes, optional; both are validated by ARITHMETIC, not magic):
//   TIFILES  0x07 "TIFILES", and the file length must equal
//            BE16(+0x08) * 0x100 + 0x80  or  (BE16(+0x08) + 1) * 0x100.
//            Flags byte at +0x0a.
//   FIAD/V9T9  a 10-byte name, u16 LE at +0x0a == 0, 100 zero bytes at +0x1c,
//            BE16(+0x0e) != 0 and the file length == BE16(+0x0e) * 0x100 +
//            0x80.  Flags byte at +0x0c.
//   With neither wrapper the catalogue starts at offset 0.
//
// FLAG BIT 1 MEANS THE WHOLE PAYLOAD IS LZW COMPRESSED - AND THE CATALOGUE IS
// INSIDE IT.  That is the fact that shapes this class: 12 of the 13 reference
// files are compressed, so for the normal case a member is NOT a byte range of
// the file and cannot be published as one.  Those members go out as
// HANDLE_METHOD_TI99ARC, whose properties blob carries the slice coordinates
// inside the expanded stream (see XTI99ARCDecoder).  An uncompressed payload
// keeps the ordinary shape and uses HANDLE_METHOD_SCL_SECTORS instead, because
// then it really is prefix-then-copy over a file range.
//
// CATALOGUE - a chain of 0x100-byte sectors:
//   +0x00  14 entries of 0x12 bytes  (14 * 0x12 = 0xfc, exactly up to the tail)
//   +0xfc  u32: 0 = another sector follows, "END!" = last.  Anything else, or
//          more than 0x400 sectors, is a rejection.
//   entry: +0x00 char[10] name
//          +0x0a u8  status flags
//          +0x0b u8  records per sector
//          +0x0c u16 BE  size in 0x100-byte sectors
//          +0x0e u8  bytes used in the last sector
//          +0x0f u8  record length
//          +0x10 u16 number of records
//   An all-zero entry is a free slot and is SKIPPED WITHOUT CONSUMING DATA -
//   it must not advance the running data cursor, or every member after the
//   first hole comes out shifted.
//   Member data follows the whole catalogue, back to back, sectors * 0x100
//   bytes each.
//
// A member is written as a TIFILES-style file: a 0x80-byte header holding
//   out[0x00:0x0a] = entry[0x00:0x0a]   (the name)
//   out[0x0a:0x0c] = 0                  (where the "TIFILES" tag would sit)
//   out[0x0c:0x14] = entry[0x0a:0x12]
//   out[0x14:0x80] = 0
// then the raw sectors.  Note the two-byte GAP: the entry's fields are shifted
// by two, not copied straight through.
class XTI99ARCArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        // Synthetic cursor key.  For an uncompressed payload it is the
        // catalogue entry's real file offset; for a compressed one it is its
        // offset inside the expanded stream, biased by the payload offset.
        qint64 nHeaderOffset;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nMemberOffset;  // inside the expanded (or raw) payload
        qint64 nMemberSize;
        qint64 nUncompressedSize;
        bool bCompressed;
        QByteArray baPrefix;
        QString sFileName;
    };

    explicit XTI99ARCArchive(QIODevice *pDevice = nullptr);
    ~XTI99ARCArchive() override;

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
        qint64 nPayloadOffset;
        qint64 nPayloadSize;
        bool bCompressed;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QByteArray buildTiFilesPrefix(const QByteArray &baEntry);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTI99ARCARCHIVE_H
