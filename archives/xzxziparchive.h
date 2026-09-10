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
#ifndef XZXZIPARCHIVE_H
#define XZXZIPARCHIVE_H

#include "xarchive.h"

// ZXZIP - the ZX Spectrum "ZIP" archiver.
//
// ARCHIVE HEADER (0x11 bytes)
//   +0x00  8    archive name; every byte must be >= 0x20
//   +0x08  3    "ZIP"
//   +0x0b  u16  the total size of the (entry + data) chain that follows
//   +0x0d  u8   zero
//   +0x0e  u8   high-byte guard: (u16 at +0x0b) >> 8 must be <= this byte
//   +0x0f  u16  a Hobeta-style checksum of bytes 0..0x0e:
//               (sum(bytes) & 0xffff) * 0x101 + 0x69, truncated to 16 bits
//
// DIRECTORY ENTRY (0x16 bytes), immediately followed by its packed data
//   +0x00  8    TR-DOS file name
//   +0x08  1    TR-DOS type letter
//   +0x09  u16  "start" (the BASIC program length for type B/b)
//   +0x0b  u16  "length"
//   +0x0d  u8   sector count
//   +0x0e  u16  packed size
//   +0x10  u32  a checksum of the decompressed data (opaque; only compared)
//   +0x14  u8   method: 0 store, 1 unsupported, 2 PKZIP Shrink, 3 ZXZIP LZH
//   +0x15  u8   sub-method, used by method 3
//
// A MEMBER IS NOT EMITTED AS ITS OWN BYTES.  The reference writes a 17-byte
// HOBETA header built from the entry, then the data, then zero padding up to
// the sector count - so every member, stored ones included, goes through
// XZXZIPDecoder, which is handed the entry verbatim in
// FPART_PROP_COMPRESSPROPERTIES.  The uncompressed size published here is the
// size of that whole emitted file (17 + padded size), not the payload size.
class XZXZIPArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;  // 0x11 + the padded data size
        quint32 nCRC;
        quint8 nMethod;
        quint8 nSubMethod;
        QString sFileName;
        QByteArray baEntry;
    };

    explicit XZXZIPArchive(QIODevice *pDevice = nullptr);
    ~XZXZIPArchive() override;

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
        QString sArchiveName;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZXZIPARCHIVE_H
