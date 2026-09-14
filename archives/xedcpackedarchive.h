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
#ifndef XEDCPACKEDARCHIVE_H
#define XEDCPACKEDARCHIVE_H

#include "xarchive.h"

// "EDC Packed" container, the packer Novell's DOS-era NetWare client disks use
// beside the Personal NetWare "Packed File " one (PING.PI_, IPXODI.MS_,
// MODEMS.PAC, ...).
//
// NO NEW CODEC IS ADDED HERE.  The payload is the very stream
// Algos/xnetwarepackdecoder.* already decodes for FT_NETWARE_PACK and
// FT_NETWARE_PACK2 - LZ77 over three Huffman trees, LSB-first, 16 KiB window -
// so this class is a container reader only.  In the reference implementation
// the three formats literally share one decompressor entry point.
//
// The file is a CHAIN of self-describing members laid end to end from offset 0.
// That is the whole difference from FT_NETWARE_PACK, which stops at one member
// and has no size field to find a second with:
//
//   0x00  char   magic[12]     " EDC Packed "   LEADING AND TRAILING SPACE
//   0x0C  char   name[12]      original name, NUL PADDED, not NUL terminated
//   0x18  quint8 eof           0x1A
//   0x19  quint16 version      3
//   0x1B  qint32 unpackedSize  little endian, not negative
//   0x1F  qint32 memberSize    HEADER PLUS STREAM, not negative
//   0x23  quint16 reserved     0
//   0x25  quint16 dosTime
//   0x27  quint16 dosDate
//   0x29  ...    stream        XNetWarePackDecoder payload, memberSize - 0x29 bytes
//
// THE SIZE AT 0x1F IS THE WHOLE MEMBER, NOT THE STREAM.  Reading it as a
// compressed length puts the next header 0x29 bytes early and the walk then
// finds no magic there, so the archive silently degrades to one member - the
// 431-file reference corpus has 1112 members in it and 424 of the files carry
// exactly one, which is why a one-member bug looks almost right.
//
// The name field is a FIXED 12 BYTES.  297 of the 1112 reference members fill
// it completely and carry no NUL at all, so it is read as 12 and then cut at
// the first NUL; reading 11 would clip every one of them.  Names are published
// exactly as stored - the reference implementation publishes the same string,
// no member in the corpus carries a path separator, and no archive holds the
// same name twice.
//
// Over the reference corpus every chain tiles its file EXACTLY: the last
// member ends on the last byte, 431 of 431.  A trailing remainder is therefore
// treated as overlay rather than as a member, and a member is only accepted
// while its declared size still fits inside the file.
class XEDCPackedArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XEDCPackedArchive(QIODevice *pDevice = nullptr);
    ~XEDCPackedArchive() override;

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
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool parseMemberHeader(const QByteArray &baHeader, qint64 *pnUncompressedSize, qint64 *pnMemberSize, QByteArray *pbaName, quint16 *pnDosTime,
                                  quint16 *pnDosDate);
    static bool isUsableMemberName(const QByteArray &baName);
    QString deriveContainerName();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XEDCPACKEDARCHIVE_H
