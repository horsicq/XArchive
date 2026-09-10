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
#ifndef XIMPARCHIVE_H
#define XIMPARCHIVE_H

#include "xarchive.h"

// IMP - the Technelysium "IMP\n" archive.
//
// ARCHIVE HEADER (0x2a bytes)
//   +0x00  4    "IMP\n"
//   +0x04  u32  offset of the directory
//   +0x08  u32  number of members
//   +0x10  u32  (unused here)
//   +0x14  u32  (unused here)
//   +0x18  u32  (unused here)
//   +0x26  u16  flags; bits 0 and 2 mark archives the reference refuses
//   +0x28  u16  a CRC-32 OF THE HEADER TRUNCATED TO 16 BITS, computed with
//               these two bytes zeroed.  Every checksum in this container works
//               that way.
//
// THE DIRECTORY IS COMPRESSED - it is a chain of "IMPDE\0" chunks decoded by
// XIMPDecoder::decodeDirectory.  Each 0x26-byte member record is followed by
// its name, an "extra" blob and an "aux" blob, and A RECORD NEVER STRADDLES A
// CHUNK: if what is left of a chunk is shorter than a record, the walk moves to
// the next chunk.
//   +0x00 u16  version; (version & 0xfff) must be below 0x10b
//   +0x04 u32  file offset of the member's STREAM ("IMPLH\0")
//   +0x0a u8   extra blob length
//   +0x0b u8   attributes (bits 1..2 select the x86 branch converter)
//   +0x0c u32  the member's offset inside the DECODED stream
//   +0x10 u32  decoded size
//   +0x14 u32  CRC-32 of the decoded bytes
//   +0x18 u16  aux blob length
//   +0x1a u16  name length
//   +0x20 u16  DOS time
//   +0x22 u16  DOS date
//   +0x24 u16  the record's own truncated CRC
//
// THE MEMBERS OF ONE STREAM ARE SOLID.  Several records can share a stream
// offset and read at different points of the decoded output, so a record cannot
// be expressed as a byte range: the reader publishes the stream (from its
// "IMPLH\0" signature to the end of the file) as the member's data and puts the
// member's position inside the decoded stream into FPART_PROP_COMPRESSPROPERTIES,
// which is what XIMPDecoder::decode replays.
class XIMPArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordIndex;    // the member's position in the directory
        qint64 nStreamBase;     // file offset of the "IMPLH\0" signature
        qint64 nStreamOffset;   // offset inside the DECODED stream
        qint64 nUncompressedSize;
        quint32 nCRC;
        quint32 nTime;
        quint8 nAttributes;
        QString sFileName;
    };

    explicit XIMPArchive(QIODevice *pDevice = nullptr);
    ~XIMPArchive() override;

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
        qint64 nDirectoryOffset;
        qint64 nNumberOfFiles;
        quint16 nFlags;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIMPARCHIVE_H
