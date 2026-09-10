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
#ifndef XWINTERSOFTARCHIVE_H
#define XWINTERSOFTARCHIVE_H

#include "xarchive.h"

// Wintersoft - the "**++" container.  It is about as small as a container gets:
//
//   0x00  4  magic "**++"
//   0x04  4  method tag, ASCII "LZW " or "HUFF"
//   0x08 ..  members back to back until end of file, each one
//              +0x00  4  i32 uncompressed size
//              +0x04  4  i32 compressed size
//              +0x08  n  payload, exactly `compressed size` bytes
//
// Nothing else is stored: NO NAMES, no CRC, no timestamps, and - the part worth
// stating - NO PER MEMBER METHOD.  The tag in the FILE header selects the codec
// for every member, so the reader resolves it once and each record just repeats
// it.  Names do not exist either, so members are called "<index>.bin" counting
// from zero, which is what the reference extractor produces.
//
// The compressed size is authoritative: the reader seeks to dataStart +
// compressedSize for the next member rather than to wherever the decoder
// stopped, so a member whose codec ends early still leaves the walk in step.
//
// Both codecs are verbatim ports of the listings in Mark Nelson's "The Data
// Compression Book" and live in XWintersoftDecoder; "LZW " is LZW15V and "HUFF"
// is the adaptive AHUFF.
//
// UNVERIFIED: all 25 archives of the reference corpus are "LZW " with exactly
// ONE member each.  The AHUFF branch and the multi-member walk are therefore
// structurally correct transcriptions that no sample exercises - a first
// failure in either is more likely a bug here than a bad archive.
class XWintersoftArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    enum METHOD {
        METHOD_UNKNOWN = 0,
        METHOD_LZW15V,
        METHOD_AHUFF
    };

    explicit XWintersoftArchive(QIODevice *pDevice = nullptr);
    ~XWintersoftArchive() override;

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
        METHOD method;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(METHOD method);
    static HANDLE_METHOD methodToHandleMethod(METHOD method);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XWINTERSOFTARCHIVE_H
