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
#ifndef XWIIU8ARCHIVE_H
#define XWIIU8ARCHIVE_H

#include "xarchive.h"

// Nintendo Wii U8 archive ("U.8-", bytes 55 AA 38 2D): the store-only tree
// container the Wii uses for menu resources (*.arc), NAND title contents
// (00000000.app banners), disc banners (opening.bnr) and homebrew (*.u8).
// Format understanding derived from libWiiPy (NinjaCheetah & contributors,
// MIT) and the WiiBrew U8_archive / Opening.bnr pages.
//
// The same class reads the three shapes a U8 ships in.  "nBase" is the file
// offset of the U8 magic; every offset stored INSIDE the U8 structure is
// relative to nBase, never to the file start:
//
//   plain          U8 magic at 0                              nBase = 0
//   IMD5 wrapper   "IMD5" at 0, U8 magic at 0x20              nBase = 0x20
//   IMET (disc)    "IMET" at 0x40, U8 magic at 0x600          nBase = 0x600
//   IMET (NAND)    "IMET" at 0x80, U8 magic at 0x640          nBase = 0x640
//                  (the 0x40 bytes before the magic may hold a build tag)
//
// An "IMD5" whose payload is not a U8 (banner.bin / icon.bin carry an LZ77
// stream there) is NOT this format; FT_WII_LZ77 owns that shape.  Whole-file
// LZ77 / Yaz0 compressed U8 (*.carc, *.szs) never starts with the U8 magic and
// is not handled here either.
//
// U8 header, 32 bytes at nBase, all integers big endian:
//
//   +0x00  u32  magic            55 AA 38 2D
//   +0x04  u32  root node offset always 0x20 (validated)
//   +0x08  u32  header size      12 * nodeCount + string pool bytes; counted
//                                from the root node, the 32-byte header is
//                                not included (validated against the file)
//   +0x0C  u32  data offset      start of the data region, relative to nBase
//   +0x10  16   reserved         zeros in every writer, not validated
//
// Node table, nodeCount records of 12 bytes at nBase + 0x20:
//
//   +0  u8   type            0x00 file, 0x01 directory (anything else rejects)
//   +1  u24  name offset     into the string pool
//   +4  u32  files: data offset relative to nBase
//            directories: "parent" - Nintendo and libWiiPy store the parent
//            node index, some homebrew writers store the nesting depth,
//            WiiBrew says "0 or 1".  It is neither used nor validated,
//            because the tree is fully determined by the end indexes below.
//   +8  u32  files: byte length (0 is legal)
//            directories: end index = index of the first node that is NOT
//            inside this directory.  Nodes are in pre-order, so a directory
//            is immediately followed by its contents up to that index.
//
// Node 0 is the root: type 1, name offset 0, end index = nodeCount.  The
// string pool follows the node table and ends at nBase + 0x20 + headerSize;
// it is a sequence of NUL-terminated names whose first byte is the root's
// empty name.  File payloads are stored verbatim in the data region; every
// writer aligns them to 0x20 but nothing requires it, files need not be in
// node order and zero-length files may share another file's offset.
//
// IMD5 wrapper (32 bytes): "IMD5", u32 size of the payload, 8 zero bytes and
// the MD5 of the payload.  The size feeds getFileFormatSize when it lies inside
// the device; the digest is reported, not verified (that would hash the whole
// file in the detection gate).
//
// IMET wrapper: 0x600 bytes ending exactly at the U8 magic.  Relative to the
// magic offset M: u32 hash size (always 0x600 - the one constant that tells an
// IMET header from a stray "IMET" string, so it is a hard reject), u32 version
// (3 everywhere), u32 icon / banner / sound sizes (the DEcompressed sizes of
// the three inner .bin members, so they cannot be checked against the node
// table), four flag bytes, then 10 titles of 84 bytes UTF-16BE (Japanese,
// English, German, French, Spanish, Italian, Dutch, Simplified Chinese,
// Traditional Chinese, Korean), 588 bytes of padding and, at nBase - 0x10, the
// MD5 over [nBase - 0x600, nBase) with the digest field zeroed.  The digest IS
// recomputed (1536 bytes) and reported as ok / mismatch, never enforced:
// homebrew banner tools are known to write bad ones.
//
// What isValid() checks, in this order (any failure rejects): wrapper and U8
// magic; root node offset 0x20; header size within 13 .. 16 MiB and inside the
// device; root node type 1 / name 0 / node count 1 .. 100000; node table
// fits in front of the pool; root's name is empty; every node type is 0 or 1;
// every name is NUL-terminated inside the pool, at most 255 bytes, free of
// control characters, 0x7F, '/' and '\' and is not "." or ".."; every
// directory's end index is at least its own index + 1 and at most its
// parent's end index; every non-empty file lies at or past the header's data
// offset and inside the device, every empty file's offset is inside the
// device; the data offset lies past the pool and inside the device (0 is also
// accepted when the archive holds no non-empty file, which is what some
// homebrew writers emit for a directory-only tree); the full path of every
// node is at most 4096 characters.  Magic, fixed root offset and the closure
// of the node table
// (bounded, monotone end indexes; terminated names; extents inside the
// device) make random data pass with negligible probability, so no trial
// decode is needed for a store-only container.
//
// What is tolerated on purpose: the directory "parent" field, data offset
// alignment (0x40 Nintendo / libWiiPy, 0x20 in other writers), file alignment,
// file order, overlapping file extents, the 16 reserved bytes, IMD5 / IMET
// padding, an IMET digest mismatch, and a name that is not valid UTF-8 (it is
// decoded as Latin-1 instead).  A non-root node with an empty name is listed
// as "_node<index>", and duplicate paths (case-folded, files and directories
// in one namespace) get "_2", "_3", ... inserted before the leaf's last '.'
// so that every member can be extracted.
//
// Members: every non-root node in node order.  Files are HANDLE_METHOD_STORE
// records at nBase + data offset; directories are FPART_PROP_ISFOLDER records
// of size 0 whose stream offset is their own 12-byte node record (unique per
// node, unlike data offsets).  Paths use '/' and carry no trailing slash.
class XWiiU8Archive final : public XArchive {
    Q_OBJECT

public:
    explicit XWiiU8Archive(QIODevice *pDevice = nullptr);
    ~XWiiU8Archive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    enum WRAPPER {
        WRAPPER_NONE = 0,
        WRAPPER_IMD5,
        WRAPPER_IMET_SHORT,  // "IMET" at 0x40, U8 at 0x600 (disc opening.bnr)
        WRAPPER_IMET_LONG    // "IMET" at 0x80, U8 at 0x640 (NAND 00000000.app)
    };

    struct MEMBER {
        qint64 nRecordOffset;  // the node's own 12-byte record, unique per node
        qint64 nDataOffset;    // files: nBase + data offset; folders: nRecordOffset
        qint64 nSize;          // files: byte length; folders: 0
        bool bIsFolder;
        QString sPath;         // full relative path, '/' separated, no trailing '/'
    };

    struct CONTEXT {
        qint64 nBase;
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint32 nWrapper;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nNodeCount;
        qint64 nFileCount;
        qint64 nDirectoryCount;
        qint64 nImd5PayloadSize;
        QString sImd5Digest;
        quint32 nImetVersion;
        quint32 nImetIconSize;
        quint32 nImetBannerSize;
        quint32 nImetSoundSize;
        quint32 nImetFlags;
        bool bImetHashValid;
        QString sImetTitle;
        QList<MEMBER> listEntries;
    };

    // The single structural gate described above; isValid(), getFileParts(),
    // getFileFormatSize() and initUnpack() all run exactly this parse.
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool parseImetWrapper(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString buildInfoString(const CONTEXT &context);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XWIIU8ARCHIVE_H
