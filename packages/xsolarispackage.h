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
#ifndef XSOLARISPACKAGE_H
#define XSOLARISPACKAGE_H

#include "xarchive.h"

// SVR4 / Solaris package datastream (pkgtrans, pkgadd; .pkg, .img and
// extension-less "package" files).
//
// Layout, exactly as the reference implementation walks it:
//   +0x000  "# PaCkAgE DaTaStReAm\n" then "<pkgabbrev> <nparts> <nblocks>\n"
//           and "# end of header\n".  The whole leading block is 512 bytes and
//           is SKIPPED WHOLESALE - the ASCII counters in it are advisory and
//           are never used to bound the walk.
//   +0x200  a chain of complete cpio archives, one per package "part".  Each
//           archive ends with its own TRAILER!!! record; the next one starts at
//           the following 512-byte boundary.  The chain ends at EOF or at the
//           first archive whose walk does not reach a trailer.
//
// The members are plain cpio - there is no compression anywhere in this
// variant, so every stream rides HANDLE_METHOD_STORE.  (The sibling
// "# PaCkAgE DaTaStReAm:zip\n" magic is a DIFFERENT format and is deliberately
// rejected here: the newline at +0x14 is part of the accepted magic.)
//
// All three cpio header dialects that can legally appear in a datastream are
// decoded: "070701"/"070702" (newc/crc, 110 bytes, 4-byte alignment),
// "070707" (odc, 76 bytes, no alignment) and the 26-byte binary header in
// either endianness (2-byte alignment).
class XSolarisPackage final : public XArchive {
    Q_OBJECT

public:
    enum CPIO_DIALECT {
        CPIO_DIALECT_UNKNOWN = 0,
        CPIO_DIALECT_NEWC,       // "070701"
        CPIO_DIALECT_CRC,        // "070702"
        CPIO_DIALECT_ODC,        // "070707"
        CPIO_DIALECT_BINARY_BE,  // 0xc771 as read little-endian
        CPIO_DIALECT_BINARY_LE   // 0x71c7 as read little-endian
    };

    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;
        quint64 nMTime;
        quint32 nMode;
        quint32 nUID;
        quint32 nGID;
        qint32 nPart;  // index of the cpio archive the member came from
        CPIO_DIALECT dialect;
        QString sFileName;
    };

    explicit XSolarisPackage(QIODevice *pDevice = nullptr);
    ~XSolarisPackage() override;

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
        qint64 nFirstMemberOffset;
        QString sPackageAbbrev;
        qint32 nNumberOfParts;
        QList<MEMBER> listMembers;
    };

    struct RAWHEADER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nAlignMask;
        quint64 nMTime;
        quint32 nMode;
        quint32 nUID;
        quint32 nGID;
        CPIO_DIALECT dialect;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readRawHeader(qint64 nOffset, qint64 nInputSize, RAWHEADER *pHeader, PDSTRUCT *pPdStruct);
    static CPIO_DIALECT classifyMagic(const char *pMagic);
    static bool rangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString dialectToString(CPIO_DIALECT dialect);
    static QString normalizeName(const QByteArray &baName, bool *pbOk);
};

#endif  // XSOLARISPACKAGE_H
