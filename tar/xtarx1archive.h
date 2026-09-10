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
#ifndef XTARX1ARCHIVE_H
#define XTARX1ARCHIVE_H

#include "xarchive.h"

// TARX 1 (QNX .tarx) - an encrypted tar, not a compressed one.
//
//   +0x00  "TaRx"
//   +0x04  the rest of the file, enciphered with the CRC-32 driven stream
//          cipher in XTARX1Decoder; the plaintext is an ordinary POSIX tar.
//
// The file size modulo 0x200 must be 4, which is the tar's own block alignment
// showing through the 4-byte magic and is the container's only structural
// self-check.
//
// THE CIPHER IS SOLID: its state is updated from the PLAINTEXT, so byte n
// cannot be produced without having produced every byte before it.  Members
// therefore all report the SAME stream - the whole ciphertext - and carry the
// plaintext offset they start at as compress-properties, which the decoder
// runs past and discards.  It is the same arrangement XAINArchive uses for its
// solid groups.
//
// The key is not stored anywhere; XTARX1Decoder recovers it from the first 100
// enciphered bytes.  Recovery is cheap, so identification runs it and rejects a
// file whose key does not come out unique.  Walking the tar needs the whole
// plaintext, so parseContext() takes a bWalkMembers flag and only the paths
// that need the member list pay for the decryption.
class XTARX1Archive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        // File offset of the tar header block: 4 + its plaintext offset.  The
        // ciphertext is byte-for-byte as long as the plaintext, so this is a
        // real, ascending position inside the container.
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        // Plaintext bytes ahead of this member, handed to the decoder.
        qint64 nSkipSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XTARX1Archive(QIODevice *pDevice = nullptr);
    ~XTARX1Archive() override;

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
        qint64 nStreamOffset;
        qint64 nStreamSize;
        quint32 nKey;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct);
    static QByteArray skipToProperty(qint64 nSkipSize);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTARX1ARCHIVE_H
