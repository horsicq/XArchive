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
#ifndef XCMPARCHIVE_H
#define XCMPARCHIVE_H

#include "xarchive.h"

// Single-member ".CMP" container (the shape used by, among others, Adaptec and
// Cheyenne install media).  It carries exactly one file and a 61-byte header:
//
//   +0    u16  0x007F
//   +2    u16  0x003D, the header size
//   +4    u16  compression method, 1 or 2
//   +40   15   the original name, NUL terminated inside the field
//   +55   u16  0x1000
//   +57   u32  uncompressed size
//
// The payload runs from +61 to the end of the file.  See XCMPDecoder for the
// two codecs.
//
// A newer variant of the container (recognisable by an 11 rather than a 9 in
// the word at +14) keeps method number 2 but stores a PKWARE Data Compression
// Library "implode" stream instead of the old LZSS.  The payload names itself:
// its first two bytes are the DCL header - literal mode 0 or 1, then the
// dictionary size 4, 5 or 6 - so the codec is chosen by looking at the stream,
// not at the variant word.  Across the whole reference set no old-LZSS member
// matches that pair; the closest starts 00 00, and a zero dictionary size is
// not a legal DCL header.  The reference implementation misses this entirely
// and runs its LZSS over the DCL stream, which is why it emits a few bytes of
// noise and stops.
class XCMPArchive : public XArchive {
    Q_OBJECT

public:
    explicit XCMPArchive(QIODevice *pDevice = nullptr);
    ~XCMPArchive() override;

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
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nMethod;
        quint16 nVariant;
        bool bIsDcl;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint16 nMethod, bool bIsDcl);
    static HANDLE_METHOD methodToHandleMethod(quint16 nMethod, bool bIsDcl);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCMPARCHIVE_H
