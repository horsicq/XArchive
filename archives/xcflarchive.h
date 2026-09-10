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
#ifndef XCFLARCHIVE_H
#define XCFLARCHIVE_H

#include "xarchive.h"

// CFL3.
//
// File header, 0x0c bytes
//   +0x00 "CFL3"
//   +0x04 i32 directory offset, at least 0x0c
//   +0x08 i32 directory size
//
// THE u32 AT +0x08 IS THE UNCOMPRESSED DIRECTORY SIZE.  An earlier reading took
// it for a preview thumbnail, which is what got this format abandoned once
// already: the bytes from +0x0c up to the directory offset are simply the
// member data area.
//
// At the directory offset sits a compressed BLOCK, the same shape a member's
// data block has:
//   +0x00 i32 method       0 stored, 1 zlib
//   +0x04 i32 block size
//   method 0: block size equals the directory size, then that many raw bytes
//   method 1: i32 uncompressed size (equal to the directory size) then
//             blockSize - 4 bytes of zlib
//
// The inflated directory is a packed list of entries, each
//   +0x00 i32 uncompressed size, not negative
//   +0x04 i32 data offset, at least 0x0c
//   +0x08 u16 method   0 stored, 1 zlib
//   +0x0a u16 unused
//   +0x0c u16 name length, never zero
//   +0x0e char[nameLength] name
// so an entry is 0x0e + nameLength bytes long.
//
// Member data at `offset`
//   method 0 -> `size` raw bytes
//   method 1 -> i32 block size, i32 uncompressed size (equal to the entry's
//               size), then blockSize - 4 bytes of compressed stream
// A member whose size is zero has no data block at all.
//
// THE COMPRESSED STREAM HAS NO ADLER-32.  Those blockSize - 4 bytes are the
// two-byte RFC 1950 header and the raw DEFLATE that follows it, and nothing
// else - the next block starts immediately after the last DEFLATE byte.  So
// the record is published as HANDLE_METHOD_DEFLATE over the extent that begins
// two bytes into the payload, NOT as HANDLE_METHOD_ZLIB: the shared zlib
// decoder would take the final four DEFLATE bytes for the missing checksum
// footer and every member would decode short.
//
// One corpus file (porrasturvat.dat) points its directory offset at bytes that
// are not a block at all - the method word there reads 0x74000001 - so nothing
// can be recovered from it.  The reference refuses that archive and so does
// this reader: the parse fails rather than emitting a short member.
class XCFLArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nMethod;
        QString sFileName;
    };

    explicit XCFLArchive(QIODevice *pDevice = nullptr);
    ~XCFLArchive() override;

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
        qint64 nDirectoryOffset;
        qint64 nDirectoryBlockSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readBlock(qint64 nOffset, qint64 nExpandedSize, qint64 nInputSize, QByteArray *pbaResult, qint64 *pnBlockSize, PDSTRUCT *pPdStruct);
    static QString methodToString(quint16 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint16 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCFLARCHIVE_H
