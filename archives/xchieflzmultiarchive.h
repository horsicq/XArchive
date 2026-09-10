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
#ifndef XCHIEFLZMULTIARCHIVE_H
#define XCHIEFLZMULTIARCHIVE_H

#include "xarchive.h"

// ChiefLZ "Multiple". The codec is the very one ChiefLZ Single
// uses and is already in this tree as Algos/xchieflzdecoder.* - nothing new is
// added here.
//
// Header, 0x2b bytes
//   +0x00 u8   0x0c
//   +0x01      "\x04\x0dChfLZ_2\x05\x06\x04"  (bytes 1..12)
//   +0x13 i32  unused, not negative
//   +0x17 i32  entry count, greater than zero and below 0x100000
//   +0x1b i32  unused, not negative
//   +0x1f i32  unused, not negative
//   +0x23 i32  total name bytes, greater than zero and below 0x1000000
//
// 0x53 bytes are then SKIPPED, so the directory starts at 0x2b + 0x53 = 0x7e:
// `count` entries of 0x29 bytes, immediately followed by every name packed end
// to end (entry i consumes `nameLen` bytes in entry order, and the lengths must
// sum to the declared total exactly).  Member data starts at
// 0x7e + count * 0x29 + totalNameBytes and each member advances the cursor by
// its PACKED size.
//
// Entry, 0x29 bytes
//   +0x00 u8   kind          0 selects the parent index at +0x01, else +0x03
//   +0x01 u16  parent index (kind 0)   - 0 is the root, else parentIndex - 1
//   +0x03 u16  parent index (kind != 0)
//   +0x0f i32  packed size
//   +0x13 i32  unpacked size
//   +0x17 u16  DOS time
//   +0x19 u16  DOS date
//   +0x1b u32  attributes (kind 0 only)
//   +0x1f u32  ~CRC32 of the unpacked data
//   +0x23 u8   name length, never zero
//   +0x28 u8   METHOD   2 stored (packed == unpacked)
// 3 the reference implementation codec (absent from every reference
//                         archive, so it is reported and refused rather than
//                         guessed at)
// 4 ChiefLZ
//
// THE METHOD BYTE IS AT +0x28, NOT AT +0x00.  An earlier prototype read +0x00,
// classified every member as "stored" and silently emitted the COMPRESSED bytes
// under the right file names and the right byte counts, which no size or name
// check can catch.
class XChiefLZMultiArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XChiefLZMultiArchive(QIODevice *pDevice = nullptr);
    ~XChiefLZMultiArchive() override;

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
        qint64 nDirectoryEnd;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCHIEFLZMULTIARCHIVE_H
