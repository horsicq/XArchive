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
#ifndef XIBMSPACK_H
#define XIBMSPACK_H

#include "xarchive.h"

// IBM install-diskette packed file, the "'S' packer" IBM shipped its DOS and
// Windows products on (the reference corpus is IBM TCP/IP for DOS/Windows).
// A packed member keeps its original name with the LAST character of the
// extension replaced by '#': SAMPLE.BAT -> SAMPLE.BA#, FTP.DOC -> FTP.DO#.
//
// THE ENTIRE HEADER IS ONE BYTE.  Offset 0 holds 'S' (0x53) and the codec
// stream starts in the very next bit; there is no size field, no checksum, no
// stored name, no trailer and no member directory.  The single member's
// decoded length is knowable only by decoding, and end of data is signalled
// inside the bit stream by the codec's own end code.
//
// The codec is the SAME adaptive-phrase codec XArchive already implements for
// IBM SaveRam/SaveRam2 FLS archives: Algos/xflsdecoder.cpp, published as
// HANDLE_METHOD_FLS_LZ.  XFLSDecoder consumes the leading 'S' tag itself, so
// this class hands it the whole file starting at offset 0 and adds no codec of
// its own. A short description of that stream, confirmed against the reference implementation's
// decompressor at VA 0x00660260 and byte-exact on all 238 reference files:
//
//   * MSB-first bits.  A dictionary of 5695 entries: 0..255 are the literals,
//     the rest are phrases grown one token at a time (previous phrase plus up
//     to 30 bytes of the current one, capped at 250 bytes).
//   * Entries are addressed through a code space split into six classes with
//     bases {0, 128, 192, 320, 576, 1600} and widths {7, 6, 7, 8, 10, 12}
//     bits; code 0x163F is the end code.  Every 500 tokens the stream restates
//     the two hottest classes in 3+3 bits and the remaining four follow in
//     ascending order, so a class is selected by a 2- or 3-bit prefix.
//   * Codes migrate between classes by use, and freshly created phrases are
//     parked in class 0; both movements are plain swaps inside a clock-swept
//     range, which is why the decoder must reproduce the sweep exactly.
//
// DETECTION.  One byte of magic is not detection.  isValid() therefore trial
// decodes the WHOLE stream and accepts it only when it terminates on its own
// end code with every input byte consumed and a non-empty result, and only
// when there is a declared size the codec accepts exactly - the same search
// that measures the uncompressed size the extractor needs. The reference implementation instead
// gates on a 300-entry whitelist of the little-endian dword at offset 1, which
// is exact for the IBM TCP/IP product set and rejects any other 'S' file; the
// trial decode is used here because it generalises and is strictly stronger
// against false positives.
class XIBMSPack : public XArchive {
    Q_OBJECT

public:
    explicit XIBMSPack(QIODevice *pDevice = nullptr);

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

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString deriveContainerName();
    // Recovers the member's decoded length, which the format does not store.
    //
    // XFLSDecoder cannot simply be asked: it needs the exact size UP FRONT
    // (declare anything else and it returns false), and the only figure it
    // reports back is nCountOutput, the count of the 0x4000-byte blocks it
    // flushed - it withholds the final partial block on every unsuccessful
    // run, which is precisely the run a measuring pass has to make.  So this
    // searches for the size instead: probe, bracket, bisect, then one strict
    // verifying decode.  See the comments in the .cpp for the predicate.
    static bool measureStream(QIODevice *pDevice, qint64 nOffset, qint64 nSize, qint64 *pnUncompressedSize, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    // parseContext is called from isValid, getFileFormatSize, getFileParts,
    // getMemoryMap and initUnpack, and each call costs roughly sixteen full
    // decodes (one per size probe); the result is memoised against the identity
    // of the bytes it was derived from.
    bool m_bContextCached;
    CONTEXT m_contextCache;
    const QIODevice *m_pContextDevice;
    qint64 m_nContextSize;
    QByteArray m_baContextPrefix;
};

#endif  // XIBMSPACK_H
