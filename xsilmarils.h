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
#ifndef XSILMARILS_H
#define XSILMARILS_H

#include "xarchive.h"

// Silmarils game-resource container (.IO / .CO / .DO), as shipped with the
// French studio's Amiga / Atari ST / PC titles.  Single member, no directory,
// no name, no checksum - the whole file is one header plus one packed stream.
//
// Header, six bytes for method 0x81 and fourteen for method 0xA1:
//
//   0x00  quint32  flag<<24 | rawSize     ENDIAN DEPENDENT (see below)
//   0x04  quint16  version                always 1
//   0x06  quint8   codeTable[8]           method 0xA1 only, always
//                                         0B 09 0A 0B 07 05 06 07
//
// THE FILE HAS NO MAGIC.  The only fixed field is the version word, and it is
// what pins the byte order: the PC build writes every scalar little endian, the
// Amiga/ST build writes the same fields big endian, so `01 00` at 0x04 means
// little endian and `00 01` means big endian.  The packed stream itself is a
// byte stream and is identical in both builds - only the header is swapped.
// (Proof: FONDCOMM.IO and FONDCOMM.CO, the PC and Amiga cuts of one asset,
// decode to 7352 bytes each and agree on 96% of them.)
//
// rawSize IS SIX LARGER THAN THE PLAINTEXT.  The 24-bit field counts the
// six-byte header, so the member's real size is rawSize - 6; see
// Algos/xsilmarilsdecoder.h for the evidence.
//
// Two methods exist:
//
//   0x81  byte-run codec - implemented, see XSilmarilsDecoder
//   0xA1  bit-stream LZ codec - NOT implemented.  Its header, its byte order,
//         its constant 8-byte parameter block and the leading literal-run token
//         are understood, but the match token is not, so those members are
//         listed with HANDLE_METHOD_UNKNOWN and extraction refuses rather than
//         writing plausible-looking garbage.
//
// Detection has to be earned, not assumed: for 0x81 isValid trial-walks the
// whole token stream and requires that it produce exactly rawSize - 6 bytes and
// stop exactly on the last input byte, and for 0xA1 it requires the constant
// parameter block.  Without that, four members of the reference corpus are
// claimed by the magic-less Stunts/4D Sports DSI heuristic in
// xlegacystorearchive.cpp.
class XSilmarils : public XArchive {
    Q_OBJECT

public:
    enum METHOD {
        METHOD_UNKNOWN = 0,
        METHOD_BYTERUN = 0x81,
        METHOD_BITSTREAM = 0xA1
    };

    explicit XSilmarils(QIODevice *pDevice = nullptr);

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
        qint64 nHeaderSize;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint32 nMethod;
        quint32 nVersion;
        bool bBigEndian;
        bool bSupported;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString deriveMemberName();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static XBinary::HANDLE_METHOD handleMethodOf(const CONTEXT &context);
    static QString reportedMethodOf(const CONTEXT &context);
};

#endif  // XSILMARILS_H
