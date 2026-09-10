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
#ifndef XTARX2ARCHIVE_H
#define XTARX2ARCHIVE_H

#include "xarchive.h"

// TARX 2 (QNX "tarx") - a gzipped tar under fixed-key Blowfish.
//
//   +0x00  u32  0x7d957678
//   +0x04  u32  (unused)
//   +0x08  u32  0x45ad3b9c
//   +0x0c  u32  (unused)
//   +0x10       the Blowfish ciphertext, ECB, 8-byte blocks to the end
//
// The container is Blowfish( gzip( tar ) ), so the reader publishes exactly ONE
// record covering the ciphertext (offset 0x10 to end of file) and lets
// XFilteredArchive unwrap it - the decoded layer is handed to the existing tar
// reader.  Nothing about tar is duplicated here.  The key material and the
// little-endian-in / big-endian-out quirk live in Algos/xtarx2decoder.h.
//
// THE RECORD DECODES THROUGH THE GZIP LAYER AS WELL, so what the filter chain
// sees is a plain tar and not a .tar.gz.  Publishing the gzip and letting the
// chain unwrap it is what this reader did first, and it silently produced a
// one-record listing instead of the archive: a decoded buffer that detects as
// FT_TAR_GZ is handed to the FT_GZIP reader, which publishes its payload as an
// ARCHIVE_STREAM record carrying no byte extent, and
// XFilteredArchive::materializeLayer has no way to decode a record with no
// extent, so the second layer failed and detection fell back to this class on
// its own.  One layer, tar out, is the shape the machinery supports.
//
// THE ONE THING THIS CLASS HAS TO DO ITSELF is size that tar.  The plaintext
// was padded up to a multiple of eight before it was encrypted, so the
// decrypted buffer carries up to SEVEN bytes of junk after the gzip trailer -
// both reference samples do, seven bytes and six.  inflate() stops at the gzip
// member's own end marker, so the padding never reaches the tar, but the record
// still has to declare the exact decoded size, and only a real inflate knows
// it.  parseContext() therefore streams one decrypt-and-inflate pass (keeping
// neither the plaintext nor the tar) and the reader remembers the answer,
// because initUnpack() and getFileParts() both ask for it.
class XTARX2Archive : public XArchive {
    Q_OBJECT

public:
    explicit XTARX2Archive(QIODevice *pDevice = nullptr);
    ~XTARX2Archive() override;

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
        qint64 nUncompressedSize;  // the size of the tar the record decodes to
        bool bSizeKnown;
        QString sFileName;
    };

    // bMeasure decrypts and inflates once to size the tar; detection does not
    // need it and must not pay for it.
    bool parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct);
    bool measureTar(qint64 nOffset, qint64 nSize, qint64 *pnTarSize, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    // Remembered across parseContext() calls on this reader; -1 means not
    // measured yet.  Keyed on the source size AND the device binding generation
    // so a rebound device can never reuse another file's answer.
    qint64 m_nMeasuredInputSize;
    qint64 m_nMeasuredTarSize;
    quint64 m_nMeasuredGeneration;
};

#endif  // XTARX2ARCHIVE_H
