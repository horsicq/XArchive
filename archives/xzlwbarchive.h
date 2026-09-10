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
#ifndef XZLWBARCHIVE_H
#define XZLWBARCHIVE_H

#include "xarchive.h"

// ZLWB - a Delphi container whose DIRECTORY IS ITSELF COMPRESSED, one zlib
// stream per member record.
//
//   header, 0x1e bytes
//     +0x00  "ZLWB"
//     +0x04  u8   0x1a
//     +0x05  u32  version, 1
//     +0x09  u8   flags
//     +0x0a  f64  Delphi TDateTime
//     +0x12  i32  member count
//     +0x16  i32  directory offset
//
//   directory blob, `count` of them back to back from that offset
//     +0x00  4 bytes ignored
//     +0x04  i32  compressed length
//     +0x08  i32  uncompressed length, either 0x110 or 0x228 - and WHICH OF THE
//                 TWO IT IS selects the record layout that comes out
//     +0x0c  i32  crc
//     +0x10  the zlib stream
//
//   short record (0x110)            long record (0x228)
//     +0x000 ShortString name         +0x006 ShortString file name
//     +0x100 i32 data offset          +0x106 ShortString full path
//     +0x104 i32 compressed size      +0x218 i32 data offset
//     +0x108 i32 uncompressed size    +0x21c i32 compressed size
//     +0x10c i32 crc                  +0x220 i32 uncompressed size
//
// A member's data is a plain zlib stream at its own offset, so members use the
// existing HANDLE_METHOD_ZLIB and this class adds no codec.
//
// A COUNT OF ZERO IS VALID and yields nothing; one file of the reference corpus
// is exactly that.  Treating an empty member list as a parse failure would drop
// a well formed archive, so the empty case is carried all the way through
// initUnpack rather than rejected.
//
// That file is the installer's SCRIPT, not a container, and it is worth knowing
// what it looks like before anyone "fixes" the enumerator to find members in it.
// Its whole body from 0x11e to the last byte is a chain of the blob records
// above - a project record, one per language, one per sub-archive, then one per
// file - and every one of them inflates cleanly, so the file looks full of
// directory entries.  They are not this file's entries: their data offsets and
// compressed sizes address the SIBLING archives the script installs from, and
// they match those files exactly.  Enumerating them here would hand out members
// whose bytes live in another file.  Zero really is the member count, and the
// bytes really are all overhead.
class XZLWBArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        QString sFileName;
    };

    explicit XZLWBArchive(QIODevice *pDevice = nullptr);
    ~XZLWBArchive() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZLWBARCHIVE_H
