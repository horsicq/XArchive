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
#ifndef XAINARCHIVE_H
#define XAINARCHIVE_H

#include "xarchive.h"

// AIN, a DOS archiver whose directory is itself compressed.
//
//   file header, 24 bytes
//     +0    '!'
//     +1    u8   low nibble the method (1..4, 4 meaning stored), high nibble
//                the version (1..3)
//     +2    i16  zero, or 0x8000 for an encrypted archive
//     +8    u16  number of members
//     +0xe  i32  offset of the DIRECTORY, which is an XAINDecoder stream
//     +0x16 u16  the sum of the first 22 bytes, XOR 0x5555
//
//   member record, 29 bytes read FROM that stream, then a NUL-terminated name
//   and one more NUL
//     +0    u8   DOS attributes
//     +1    u16  time
//     +3    u16  date
//     +5    i32  original size
//     +9    i32  compressed size, recorded only on the member that closes a
//                group (the one with 0x08 set)
//     +0xd  i32  file offset of the group's compressed data, meaningful when
//                0x10 is set
//     +0x16 u8   flags; only 0x08 and 0x10 may be set
//
// AIN is SOLID.  A member with 0x10 opens a new stream at its +0xd offset and
// every member after it continues that same stream, so extracting member k
// means decoding all of its predecessors first - the reader publishes that
// leading amount as the record's compress-properties and XAINDecoder discards
// it.  Every member of a group therefore reports the SAME stream range.
class XAINArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nSkipSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        quint32 nTime;
        quint8 nMethod;
        bool bIsFolder;
        QString sFileName;
    };

    explicit XAINArchive(QIODevice *pDevice = nullptr);
    ~XAINArchive() override;

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
        bool bEncrypted;
        bool bStored;
        quint8 nMethod;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static QByteArray skipToProperty(qint64 nSkipSize);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XAINARCHIVE_H
