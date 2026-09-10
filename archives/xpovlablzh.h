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
#ifndef XPOVLABLZH_H
#define XPOVLABLZH_H

#include "xarchive.h"

// POVLAB 3.0 install-disk container.
//
// This is a byte-for-byte LHA *level-1* archive whose five-character method
// tag has been renamed.  Nothing else about the container differs: the same
// header layout, the same additive header checksum, the same level-1 "skip
// size" semantics, the same CRC-16/ARC over the plaintext, the same single
// 0x00 byte as the end-of-archive marker.  Only the tag spelling was changed,
// which is why stock LHA tools reject the files and why patching the tag back
// to "-lh5-" (and fixing the one checksum byte) makes 7-Zip read them.
//
//   0x00  quint8   headerSize     size of the base header counted from 0x02
//   0x01  quint8   headerChecksum sum of the base header bytes, modulo 256
//   0x02  char     method[5]      "-ARA-" (LH5) or "-ARS-" (stored)
//   0x07  quint32  skipSize       packed bytes + total extended-header bytes
//   0x0b  quint32  originalSize
//   0x0f  quint32  mtime          always 0 in the reference corpus
//   0x13  quint8   attributes     always 0x20
//   0x14  quint8   level          always 0x01
//   0x15  quint8   nameLength
//   0x16  char     name[nameLength]
//   ...   quint16  crc16          CRC-16/ARC of the UNPACKED member
//   ...   quint8   osId           always 0x20
//   ...   quint16  extHeaderSize  0 terminates the extended-header chain
//
// "-ARA-" payloads are ordinary LHA lh5 bitstreams (13-bit window, static
// Huffman blocks), so they reuse HANDLE_METHOD_LZH5 unchanged; "-ARS-"
// payloads are stored and reuse HANDLE_METHOD_STORE.  A "-ARS-" member always
// reports skipSize == originalSize and that identity is part of the gate.
//
// The reference corpus (113 files, F:/ARC/ARC4/POVLAB) holds exactly one
// member per archive, but the walk is written as the general LHA chain so a
// multi-member POVLAB volume would still enumerate.
class XPovlabLzh : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nTime;
        quint16 nCRC16;
        quint8 nAttributes;
        quint8 nOS;
        bool bStored;
        QString sMethod;
        QString sFileName;
    };

    explicit XPovlabLzh(QIODevice *pDevice = nullptr);
    ~XPovlabLzh() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
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
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readMember(qint64 nOffset, qint64 nInputSize, MEMBER *pMember, PDSTRUCT *pPdStruct);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPOVLABLZH_H
