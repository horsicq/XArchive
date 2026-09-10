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
#ifndef XMSCOMPRESSSZ_H
#define XMSCOMPRESSSZ_H

#include "xarchive.h"

// Microsoft COMPRESS.EXE "SZ " container - the pre-SZDD variant shipped with
// the Microsoft language products (QuickC / QuickBASIC / Word distribution
// disks, names mangled to .C$ / .OB$ / .KE$).
//
// Layout (always 12 bytes of header, then the packed stream):
//   0x00  8  53 5A 20 88 F0 27 33 D1   fixed signature ("SZ " + magic)
//   0x08  4  uncompressed size, u32 LE
//   0x0C  .. LZSS stream
//
// The codec is Haruhiko Okumura's reference LZSS with N = 4096, F = 18,
// THRESHOLD = 2: one LSB-first flag byte per eight tokens, literals verbatim,
// matches as {u8 low, u8 (high nibble = position bits 8..11, low nibble =
// length - 3)}, ring cursor starting at N - F.  That is NOT the SZDD path
// (HANDLE_METHOD_LZSS_SZDD starts the cursor at N - 16 and produces the right
// byte count with the wrong bytes); it is bit-for-bit the codec already
// carried by HANDLE_METHOD_AMPK_LZSS, which this class reuses.
class XMSCompressSZ : public XArchive {
    Q_OBJECT
public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_SZ_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct SZ_HEADER {
        char signature[8];
        quint32 uncompressed_size;
    };
#pragma pack(pop)

    explicit XMSCompressSZ(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    bool isSigned() override;
    OSNAME getOsName() override;
    QString getOsVersion() override;
    QString getVersion() override;
    bool isEncrypted() override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    SZ_HEADER _read_SZ_HEADER(qint64 nOffset);

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nHeaderSize;
        qint64 nCompressedOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XMSCOMPRESSSZ_H
