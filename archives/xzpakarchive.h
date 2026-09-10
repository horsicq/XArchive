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
#ifndef XZPAKARCHIVE_H
#define XZPAKARCHIVE_H

#include "xarchive.h"

// ZPAK / zpk2 - ZSoft's archive format.  A six byte header, a fixed size
// directory, then the payloads:
//
//   0x00  4  magic "zpak" (v1) or "zpk2" (v2)
//   0x04  2  u16 member count, non zero
//   0x06 ..  `count` directory entries of 33 bytes each
//
// A directory entry:
//   +0x00  13  name, NUL terminated; only TWELVE characters are significant -
//              byte 12 is forced to NUL before the name is read and holds
//              padding garbage whenever the name is shorter
//   +0x0D   4  i32 absolute file offset of the payload
//   +0x11   4  i32 compressed size
//   +0x15   2  u16 DOS date
//   +0x17   2  u16 DOS time
//   +0x19   2  u16 CRC low half
//   +0x1B   2  u16 CRC high half; zero means "not checked"
//   +0x1D   4  i32 uncompressed size
//
// The MAGIC PICKS THE CODEC FOR EVERY MEMBER; it is not a per-entry field.
//
// "zpk2" is plain PKWARE DCL ("implode", XDclDecoder), header byte 0 the
// literal mode and byte 1 the 4/5/6 dictionary-size code.
//
// "zpak" is 12-bit LZW with a chunk layer wrapped round it, and BOTH halves of
// that sentence are traps - the framing that is not blocking, and the LZW
// settings that cannot be read off the reference call site.  Algos/
// xzpakdecoder.h spells both out; XZPAKDecoder::decodeLZW() is the whole
// pipeline for HANDLE_METHOD_ZPAK_LZW.
class XZPAKArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        bool bHasCRC;
        quint16 nDate;
        quint16 nTime;
        QString sFileName;
    };

    explicit XZPAKArchive(QIODevice *pDevice = nullptr);
    ~XZPAKArchive() override;

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
        bool bIsV2;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(bool bIsV2);
    static HANDLE_METHOD methodToHandleMethod(bool bIsV2);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZPAKARCHIVE_H
