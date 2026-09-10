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
#ifndef XSQXARCHIVE_H
#define XSQXARCHIVE_H

#include "xarchive.h"

// SQX (SQX-Software / Michael Bierhoff) archive.
//
// Signature: byte 2 is 'R', the u16 at +5 is 0x19 and bytes 7..11 are "-sqx-".
// The main flags at +3 carry 0x10 for encrypted headers, which makes the
// archive unreadable.  The member chain starts at offset 25 and each header is
//
//   u16 CRC, u8 type, u16 flags, u16 header size
//
// followed by (header size - 7) body bytes.  Types 'A', 'S' and 'X' end the
// archive, type 'D' is a file.  A file body is
//
//   +0   u8  filter selector ("b0")
//   +5   u8  METHOD - it lives here, not in the flags
//   +6   u32 CRC32, u32 attributes, u32 timestamp, u32 packed, u32 unpacked
//   +26  (flags & 0x80) two more u32: the high halves of packed/unpacked
//   then  u16 name length and the name
//
// A header with flags & 0x8000 is followed by chained extension headers, and
// the member payload starts after the last of them.
//
// Method 0 is stored and methods 1..4 are the Huffman LZ coder in XSQXDecoder.
// Methods 5 and up are a different coder and are NOT implemented: those
// members are published with HANDLE_METHOD_UNKNOWN so they fail cleanly rather
// than emitting plausible garbage.  One of the three reference archives
// (SQX-SDK_v206.sqx) is mostly method 6 and the reference tool cannot read it
// either.
class XSQXArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        quint32 nAttributes;
        quint32 nTime;
        quint16 nFlags;
        quint8 nFilter;
        quint8 nMethod;
        qint32 nTableIndex;  // position in CONTEXT::baMemberTable, -1 when not coded
        bool bIsFolder;
        QString sFileName;
    };

    explicit XSQXArchive(QIODevice *pDevice = nullptr);
    ~XSQXArchive() override;

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
        // Every coded member in archive order; a record publishes this table
        // stamped with its own index so the decoder can replay the solid
        // window in front of it.
        QByteArray baMemberTable;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSQXARCHIVE_H
