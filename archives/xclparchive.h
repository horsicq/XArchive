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
#ifndef XCLPARCHIVE_H
#define XCLPARCHIVE_H

#include "xarchive.h"

// Windows Clipboard file (.CLP), as written by Clipbook Viewer.
//
//   +0   u16  format version word
//   +2   u16  number of records
//   +4   the records, 0x59 bytes each:
//          +0  u16  clipboard format id
//          +2  i32  data size
//          +6  i32  data offset
//
// Only formats that map onto a file are extracted, which is the same set the
// reference implementation accepts: CF_TEXT (1), CF_OEMTEXT (7), CF_DIB (8),
// CF_UNICODETEXT (13) and CF_DIBV5 (17).  Anything else - CF_BITMAP, CF_METAFILEPICT,
// the private 0xC000+ formats - is LISTED but has no extractable form; the
// reference silently drops those records, so a file made only of them yields
// nothing from either tool.
//
// Text records are truncated at their first NUL, which is where the real text
// ends: the record's size field covers the whole clipboard allocation.
//
// A DIB record has to be given a 14-byte BITMAPFILEHEADER to become a usable
// .bmp.  bfSize is the record size plus 14 and bfOffBits is bfSize minus
// (biWidth * biHeight * biBitCount) / 8, so the pixel data lands where the
// header says - the palette, if any, sits between.  That synthesised prefix
// travels as FPART_PROP_COMPRESSPROPERTIES and is emitted ahead of the stored
// stream by HANDLE_METHOD_SCL_SECTORS, which is a prefix-then-copy codec
// rather than anything SCL specific.
class XCLPArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nFormat;
        QByteArray baPrefix;
        QString sFileName;
    };

    explicit XCLPArchive(QIODevice *pDevice = nullptr);
    ~XCLPArchive() override;

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
    static HANDLE_METHOD methodOf(const MEMBER &member);
    static QString describe(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCLPARCHIVE_H
