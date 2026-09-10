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
#ifndef XDSL2ARCHIVE_H
#define XDSL2ARCHIVE_H

#include "xarchive.h"

// DS'L install 2.0 (.D00) - an installer whose DIRECTORY IS OBFUSCATED while
// its member data is not.
//
//   header, 0x1c bytes
//     +0x00  "DS'L install 2.0"
//     +0x10  u16  zero
//     +0x12  u16  file count, non-zero
//     +0x14  u32  unused
//     +0x18  i32  offset where the directory ENDS and the data area starts
//
// Straight after the header come two u16-length-prefixed strings (the install
// paths).  The directory is everything from there to the header's +0x18 offset,
// and every byte of it has had 0x33 added; subtracting it back is the whole of
// the obfuscation.  THE HEADER AND THE TWO STRINGS ARE NOT OBFUSCATED, so
// de-obfuscating from the wrong start byte still yields plausible-looking
// record lengths and misreads the whole directory.
//
//   directory entry, variable length, at least 0x14 bytes
//     +0x00  u16  record length
//     +0x02  i32  uncompressed size
//     +0x06  i32  compressed size
//     +0x0a  i32  absolute data offset
//     +0x0e  u16  DOS time
//     +0x10  u16  DOS date
//     +0x12  u16  flags; bit 0x10 marks a compressed member
//     +0x14  u16  name length INCLUDING its NUL, then the name
//
// A compressed member is a PKWARE DCL implode stream - the same codec the
// existing HANDLE_METHOD_PKWARE_DCL_IMPLODE already covers, so this class adds
// no decoder of its own.
class XDSL2Archive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nFlags;
        quint16 nDosTime;
        quint16 nDosDate;
        QString sFileName;
    };

    explicit XDSL2Archive(QIODevice *pDevice = nullptr);
    ~XDSL2Archive() override;

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

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static XBinary::HANDLE_METHOD flagsToHandleMethod(quint16 nFlags);
    static QString flagsToString(quint16 nFlags);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XDSL2ARCHIVE_H
