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
#ifndef XVMARCARCHIVE_H
#define XVMARCARCHIVE_H

#include "xarchive.h"

// VM/CMS VMARC (John Fisher, Rice University).
//
//   +0x00  "zCFF    " in EBCDIC (7A C3 C6 C6 40 40 40 40)
//   +0x08  u8   1
//   +0x0A  8    file name,  EBCDIC, blank padded
//   +0x12  8    file type,  EBCDIC, blank padded
//   +0x1C  u16  LRECL, BIG endian
//   +0x24  u8   record format, EBCDIC 'F' (0xC6) or 'V' (0xE5)
//   +0x25  u8   flags: 0x01 a 12-byte extended header follows the 38-byte one,
//                      0x40 the member is stored, 0x80 a second codec this
//                      reader does not implement
//   +0x26  the member data
//
// MEMBERS START ON 80-BYTE BOUNDARIES.  Nothing in the header says where a
// member ends, so the walk has to DECODE the member to find the byte after it
// and then round that up to the next multiple of 80.  That is why the member
// list is only built on the full parse and isValid() checks the signature
// alone.
//
// Content is written raw EBCDIC with the records concatenated and no
// separators of any kind - see XVMARCDecoder.  Only the member NAME is
// translated (CP037), because it has to become a file name.
class XVMARCArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nLRECL;
        quint8 nRecordFormat;
        quint8 nFlags;
        quint8 nMode;
        bool bFixed;
        QString sFileName;
    };

    explicit XVMARCArchive(QIODevice *pDevice = nullptr);
    ~XVMARCArchive() override;

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

    bool parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct);
    static QString ebcdicToString(const quint8 *pData, qint32 nSize);
    static QString methodToString(quint8 nMode);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMode);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XVMARCARCHIVE_H
