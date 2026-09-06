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
#ifndef XECMPACKED_H
#define XECMPACKED_H

#include "xarchive.h"

// EmmaSetup (Softuseful "EmmaSetup" DOS/Windows installer builder) packed
// member - one compressed payload file per container, produced at build time
// and expanded by the installer at install time.
//
// Layout (always 38 bytes of header, then the packed stream):
//   0x00   4  "ECM\0"
//   0x04   2  version word: 0x2313 (138/171 of the reference corpus),
//             0x0000 (28) or 0x0100 (5).  All three carry the same codec.
//   0x06   4  build-time scratch (constant across the members of one
//             installer; not a size, not a checksum)
//   0x0A   2  build-time scratch (varies per member)
//   0x0C   4  build-time scratch (constant across the members of one
//             installer)
//   0x10  20  always zero - the strongest structural discriminator this
//             container has, and what keeps "ECM\0" from colliding with
//             Neill Corlett's unrelated ECM (Error Code Modeler) files,
//             whose stream cannot contain a run of zero bytes (0x00 is its
//             end-of-stream marker).
//   0x24   2  0x0032 or 0x0000
//   0x26  .. LZSS stream, running to end of file
//
// The codec is Haruhiko Okumura's reference LZSS with N = 4096, F = 18,
// THRESHOLD = 2: one LSB-first flag byte per eight tokens, set bit = literal,
// matches as {u8 low, u8 (high nibble = position bits 8..11, low nibble =
// length - 3)}, ring pre-filled with ' ' and the write cursor starting at
// N - F.  That is bit-for-bit the codec already carried by
// HANDLE_METHOD_AMPK_LZSS, which this class reuses - it is NOT
// HANDLE_METHOD_LZSS_SZDD, which starts the cursor at N - 16 and yields the
// right byte count with the wrong bytes.
//
// The header stores no uncompressed size and no member name: the stream simply
// runs to the end of the file, and the installer supplies the name out of band.
// The uncompressed size is therefore recovered by walking the token structure
// (a pass that costs one add per token and doubles as the validity check - a
// genuine stream never truncates a two-byte match token).
class XEcmPacked : public XArchive {
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
        STRUCTID_ECM_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct ECM_HEADER {
        char signature[4];
        quint16 version;
        quint32 field_06;
        quint16 field_0a;
        quint32 field_0c;
        quint8 reserved[20];
        quint16 field_24;
    };
#pragma pack(pop)

    explicit XEcmPacked(QIODevice *pDevice = nullptr);

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

    ECM_HEADER _read_ECM_HEADER(qint64 nOffset);

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
        quint16 nVersion;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // Walks the token structure of the packed stream without materialising the
    // output, returning the exact uncompressed size.  Returns false when the
    // stream is not a well-formed Okumura LZSS token sequence.
    bool measureStream(qint64 nOffset, qint64 nSize, qint64 *pnUncompressedSize, PDSTRUCT *pPdStruct);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XECMPACKED_H
