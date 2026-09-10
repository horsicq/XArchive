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
#ifndef XVMSDATABASEARCHIVE_H
#define XVMSDATABASEARCHIVE_H

#include "xarchive.h"

// VMS DataBase, the PCSI$ product kit that an OpenVMS DCX container expands to.
// It is a BER-like TLV tree, not a record chain; see XVMSDataBaseDecoder for
// the tag encoding.
//
//   +0x00  u32 0x8074FFFF        (ff ff 74 80)
//   +0x04  u32 0x018080A0
//   +0x08  u32 0x00018101
//
// Two bytes are skipped and the walk then descends through the constructed
// elements 0x74 -> 0xA2 -> 0x61 -> 0xAF -> 0xA8.  A kit with no 0xAF or no
// 0xA8 simply holds no files.  Inside 0xA8 each member is
//
//   0x30 constructed
//     0x80 ...          skipped
//     0x81 len          the file name
//     0xA2 constructed
//       0x04 len=0xAB   the attribute record
//         +0x13 i32     allocated blocks
//         +0x1f u16     bytes used in the last block
//       EOC
//     0xA3 constructed  the 0x04 chunks holding the file, or
//     EOC               when the file is empty
//
// and the size is (blocks - 1) * 0x200 + bytes-in-last-block.
//
// TRAP: after the 0xA3 element's own end-of-contents marker there is ONE MORE
// end-of-contents marker to consume - the enclosing 0x30's.  Leaving it in the
// stream makes the next member's tag read land on it, the walk sees a 0x00 and
// stops, and the kit yields exactly ONE file however many it holds.  That is
// the whole difference between a correct reader and a plausible one here.
//
// A member's bytes are a run of chunks rather than one range, so the record's
// stream is the 0xA3 content INCLUDING its end-of-contents marker and the
// decoder concatenates what it finds inside.
class XVMSDataBaseArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nUncompressedSize;
        bool bStored;
        QString sFileName;
    };

    explicit XVMSDataBaseArchive(QIODevice *pDevice = nullptr);
    ~XVMSDataBaseArchive() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XVMSDATABASEARCHIVE_H
