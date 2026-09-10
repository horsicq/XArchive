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
#ifndef XZIEARCHIVE_H
#define XZIEARCHIVE_H

#include "xarchive.h"

// ZIE - a plain ZIP under the "ProtectIt/2" (OS/2) XOR cipher, with a
// 0x118-byte header in front of it.
//
// There is no container of its own here beyond that header: the payload IS an
// ordinary ZIP with every byte transformed.  The reader therefore publishes
// exactly ONE record covering the payload (offset 0x118 to end of file), the
// xobfuscatedarchive.cpp shape, so XFilteredArchive can treat it as one more
// filter layer - it materialises the plaintext and hands it to the real ZIP
// reader, exactly as it unwraps gzip or bzip2.  No ZIP logic is duplicated
// here.
//
//   +0x000  u32       "PIT2"
//   +0x004  16 bytes  obfuscated key; key[i] = header[4+i] ^ "ProtectIt/2 OS/2"[i]
//   +0x014  13 bytes  original file name, NUL padded, 8.3-ish
//   +0x024 .. 0x118   zero padding
//   +0x118            the encrypted ZIP
//
// The cipher and its length-derived phase live in Algos/xziedecoder.h.  The one
// thing worth repeating here is why detection is BY KNOWN PLAINTEXT rather than
// by the header alone: the phase is derived from the payload LENGTH, so a
// truncated .zie decrypts to noise under the length rule - and 5 of the 13
// reference samples are truncated, which is why the reference extracts ZERO
// files from all five.  Their real phase is 0.  Resolving the phase against the
// "PK" 03 04 the ZIP underneath must start with reproduces the reference on the
// eight intact samples and recovers 11-20 CRC-verified members from each of the
// other five.
class XZIEArchive : public XArchive {
    Q_OBJECT

public:
    explicit XZIEArchive(QIODevice *pDevice = nullptr);
    ~XZIEArchive() override;

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
        QByteArray baProperty;
        QString sReportedMethod;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZIEARCHIVE_H
