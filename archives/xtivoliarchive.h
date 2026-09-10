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
#ifndef XTIVOLIARCHIVE_H
#define XTIVOLIARCHIVE_H

#include "xarchive.h"

// Tivoli Filepack Block (.PKT) - a 79-byte ASCII header line, then a chain of
// blocks that together carry ONE byte stream, which for this family is a cpio
// archive holding the actual members.
//
// The header line is exactly 0x4F bytes and ends with '\n':
//
//   "    79 TFPB-v2.01 Tivoli Filepack Block.  fpname=\"-\" cksum=md5 "
//   "compress=native\n"
//
// (the leading field is the record length, 79, right justified in six columns).
// The block chain and the "compress=native" LZ77 behind it are XTivoliDecoder's
// business; what remains here is the container's TWO LAYER SHAPE, and that is
// what makes this reader look unlike the others in the tree.
//
// TRAP - THE MEMBERS DO NOT EXIST ANYWHERE IN THE FILE.  A member's bytes only
// appear once the whole block chain has been unwrapped, so there is no file
// extent that addresses a member.  Each record consequently publishes the WHOLE
// FILE as its stream (the xuleadarchive.cpp pattern) plus the member's position
// inside the DECODED stream in FPART_PROP_COMPRESSPROPERTIES, as the eight
// little-endian bytes XTivoliDecoder::memberProperties() builds:
//
//   u32 offset  - the member's first byte inside the unwrapped stream
//   u32 size    - its length there, which is also its uncompressed size
//
// XTivoliDecoder::decodeMember() is that whole path in one call and is what a
// dispatch should use; Algos/xtivolidecoder.h has the rest of the reasoning.
//
// The inner stream is a cpio "070702" (SVR4 with CRC; "070701" is accepted too)
// archive whose 110-byte headers are ASCII hex and, unlike ordinary newc cpio,
// carry NO four-byte padding after the name or the data - a padded reader
// desynchronises on the second member. It ends at the "TRAILER!!!" record.
// A stream that is not cpio at all is published as one member, which is what
// the reference extractor falls back to.
class XTivoliArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        // Position of the member inside the DECODED stream, not the file.
        qint64 nStreamOffset;
        qint64 nSize;
        quint32 nMode;
        bool bIsFolder;
        QString sFileName;
    };

    explicit XTivoliArchive(QIODevice *pDevice = nullptr);
    ~XTivoliArchive() override;

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
        qint64 nInnerSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool parseCpio(const QByteArray &baInner, QList<MEMBER> *plistMembers, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTIVOLIARCHIVE_H
