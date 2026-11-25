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
#ifndef XMSA_H
#define XMSA_H

#include "xarchive.h"

// Atari ST MSA disk image (Magic Shadow Archiver, *.msa), as the scan engine
// opens it.  The same rules are implemented step for step in xxfclib
// (src/scan/xx_scan_floppy.c): CHANGE BOTH TOGETHER.
//
//   +0   u16 BE  0x0E0F
//   +2   u16 BE  sectors per track, 1..36
//   +4   u16 BE  sides - 1, 0..1
//   +6   u16 BE  first track
//   +8   u16 BE  last track, first <= last <= 85
//   +10  for each track and side: u16 BE length, then that many bytes.  A
//        track of sectors * 512 bytes is stored as it is; a shorter one is RLE
//        packed: 0xE5, value, u16 BE run length; every other byte is literal.
//
// The file is an MSA when every length word is 1..track size and its data lies
// inside the file.  Its single member "msa-content" is the decoded disk image
// (normally a FAT12 volume, see XFATImage).  A run that is empty or leaves the
// track, or a track that does not decode to its exact size, fails the member.
// The member is a decoded stream without an extent in the file, so it is
// published as an index-paired archive-stream record.
class XMSA : public XArchive {
    Q_OBJECT

public:
    explicit XMSA(QIODevice *pDevice = nullptr);
    ~XMSA() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getMIMEString() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        quint32 nTrackSize;  // Decoded bytes of one track side
        quint32 nTracks;     // Track sides stored
        qint64 nPackedSize;  // Bytes of track records after the header
        qint64 nImageSize;   // Decoded bytes
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XMSA_H
