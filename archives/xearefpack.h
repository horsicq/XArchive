/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XEAREFPACK_H
#define XEAREFPACK_H

#include "xarchive.h"

// Electronic Arts RefPack / QFS compressed file (.QFS, .FSH, .CFS, .ORI,
// .IFF, .POG, .FFN, .PVI - the extension belongs to the payload, never to the
// wrapper).
//
// The whole file is one member.  Its header is the signature word - a flags
// byte whose base value is 0x10, then the constant 0xFB - followed by a
// BIG-endian uncompressed size (three bytes, or four when flag 0x01 is set),
// optionally preceded by a packed size when flag 0x80 is set.  The command
// stream starts on the very next byte, so the wrapper stores no name, no
// timestamp and no checksum; the member name is derived from the container's
// own file name, exactly as the reference extractor does.
//
// Two properties of the codec make the format self-validating and are what
// isValid() leans on instead of the two-byte signature, which is far too weak
// on its own: the command stream must close on an explicit 0xFC..0xFF
// terminator, and there is no pre-filled history window, so no back-reference
// may ever reach in front of the bytes already produced.  A complete walk that
// satisfies both AND yields exactly the declared uncompressed size is what a
// file has to do to be accepted.  Verified on the whole 62-file corpus: every
// sample terminates on its last byte with the produced length matching the
// header, and the extracted bytes are identical to the reference extractor's.
class XEARefPack : public XArchive {
    Q_OBJECT

public:
    explicit XEARefPack(QIODevice *pDevice = nullptr);

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
        // The decoder re-reads the signature word itself, so the member stream
        // starts at offset 0 and spans the whole container.
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nHeaderSize;
        qint64 nUncompressedSize;
        bool bLargeSizes;
        bool bHasPackedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString deriveMemberName();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XEAREFPACK_H
