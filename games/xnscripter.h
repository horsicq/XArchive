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
#ifndef XNSCRIPTER_H
#define XNSCRIPTER_H

#include "xarchive.h"

// NScripter engine archives (UniExtract parity gap G50).  Layouts as
// documented by GARbro (MIT, ArcFormats/NScripter/ArcSAR.cs, ArcNSA.cs,
// ArcNS2.cs) and verified against GARbro.Console on generated samples:
//
//   SAR: BE u16 count, BE u32 base, count x {NUL-terminated CP932 name,
//        BE u32 offset relative to base, BE u32 size}; members stored.
//   NSA: BE u16 count, BE u32 base, count x {name, u8 codec (0 none, 1 SPB,
//        2 LZSS, 4 NBZ), BE u32 offset, BE u32 packed, BE u32 unpacked}.
//        A codec-0 member named *.nbz is an NBZ stream (engine rule, GARbro
//        measured); a *.spb name is NOT inferred.
//   NS2: LE u32 base, then {'"' name '"', LE u32 size} up to base-1, the
//        byte 'e' at base-1, then the member data back to back.
//
// Codecs: NBZ = BE u32 unpacked size + bzip2; LZSS = NScripter's 256-byte
// ring / 4-bit length variant; SPB = the engine's planar delta image codec,
// emitted as the 24-bit bottom-up BMP GARbro writes.  None of the three
// carries a magic, so detection is structural (table ends exactly at base,
// monotonic in-range offsets, data ending exactly at EOF) and the format is
// probed last in the ladder, with STK and AP4.
//
// FT_NSCRIPTER_SAR is not FT_SAR: that one is Streamline SAR, an LHA dialect.
class XNScripterArchive : public XArchive {
    Q_OBJECT

public:
    explicit XNScripterArchive(QIODevice *pDevice = nullptr, FT fileType = FT_NSCRIPTER_NSA);
    ~XNScripterArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    // FT_NSCRIPTER_NSA / FT_NSCRIPTER_SAR / FT_NSCRIPTER_NS2, or FT_UNKNOWN.
    static FT detectFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    enum CODEC {
        CODEC_NONE = 0,
        CODEC_SPB = 1,
        CODEC_LZSS = 2,
        CODEC_NBZ = 4
    };

    struct MEMBER {
        QString sName;
        qint64 nDataOffset = 0;  // absolute
        qint64 nPackedSize = 0;
        qint64 nUnpackedSize = -1;  // -1: not knowable from the table (fails closed at extraction)
        qint32 nCodec = CODEC_NONE;
    };

    struct CONTEXT {
        FT fileType = FT_UNKNOWN;
        QList<MEMBER> listMembers;
        qint64 nBase = 0;
        qint64 nArchiveEnd = 0;
    };

private:
    enum PARSE_RESULT {
        PARSE_INVALID,
        PARSE_NEED_MORE,
        PARSE_OK
    };

    bool readContext(FT fileType, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readTableArchive(bool bNsa, qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readNs2Archive(qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static PARSE_RESULT parseTable(const uchar *pIndex, qint64 nAvailable, qint64 nIndexSize, bool bNsa, qint32 nCount,
                                   qint64 nBase, qint64 nDataSize, CONTEXT *pContext);
    static PARSE_RESULT parseNs2Index(const uchar *pIndex, qint64 nAvailable, qint64 nIndexSize, qint64 nBase, qint64 nDataSize,
                                      CONTEXT *pContext);
    bool resolveCodecSizes(CONTEXT *pContext, PDSTRUCT *pPdStruct);

    static bool decodeLzss(const QByteArray &baPacked, qint64 nUnpackedSize, QByteArray *pResult);
    static bool decodeSpb(const QByteArray &baPacked, QByteArray *pResult);
    static bool decodeNbz(const QByteArray &baPacked, QByteArray *pResult, PDSTRUCT *pPdStruct);
    static bool spbOutputSize(const uchar *pHeader, qint64 nHeaderSize, qint64 *pnSize);

private:
    FT m_fileType;
};

#endif  // XNSCRIPTER_H
