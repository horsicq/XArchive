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
#ifndef XRAWLZW15V_H
#define XRAWLZW15V_H

#include "xarchive.h"

// Raw LZW15V - a single file compressed with Mark Nelson's variable-width LZW
// (9..15 bits) and stored with NO CONTAINER AT ALL.
//
// These are the MS-style "truncated extension" install payloads (WIPEOUT.EX_,
// BILLBD.DL_, KSCOPE.WO_, WIPEOUT.HL_, WIPERS.DA_, WIPEDD.SY_) produced by an
// installer toolchain that, unlike MS COMPRESS, wrote no SZDD/KWAJ header: byte
// 0 of the file is already the first bit of the first 9-bit code.  There is no
// magic, no stored name, no stored size and no checksum, so the ONLY thing that
// can tell this format apart from arbitrary data is the code stream itself.
//
// Detection therefore runs a STRICT FULL TRIAL DECODE (XRawLzw15vDecoder::
// probe) and accepts only a stream that:
//   - opens with a literal code (< 0x100),
//   - never widens past 15 bits and never emits a BUMP once there,
//   - never uses a code more than one past the next assignable one,
//   - never lets the next assignable code outrun the current code width,
//   - terminates on an explicit 0x100 END code,
//   - consumes every input byte, and
//   - expands (output larger than input, and at least 64 bytes).
// Measured on 51k files (the whole ARC/ARC2/ARC3/ARC4 corpora plus
// C:\Windows\System32) that gate fires on the 50 real members and nothing else.
//
// The member name is the container's own file name, which is exactly what U3
// publishes: nothing inside the stream carries a name.
class XRawLzw15v : public XArchive {
    Q_OBJECT

public:
    explicit XRawLzw15v(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

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
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    // A trial decode is the whole detector, so it is not free: remember the
    // outcome for the exact device/size/prefix that produced it instead of
    // re-decoding once per metadata call.
    bool m_bContextCached;
    bool m_bContextValid;
    CONTEXT m_context;
    QIODevice *m_pCachedDevice;
    qint64 m_nCachedSize;
    QByteArray m_baCachedPrefix;
};

#endif  // XRAWLZW15V_H
