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
#ifndef XNETWAREPACKEDFILE_H
#define XNETWAREPACKEDFILE_H

#include "xarchive.h"

// Personal NetWare / Novell DOS "Packed File" - the single-member container
// Novell's DOS-era install disks use for the members whose last extension
// character was replaced by '_' or '@' (LSL.CO@, NWADMIN.IN_, ...).
//
// Fixed 31-byte header, then the packed stream, then nothing: no directory, no
// checksum, no trailer.
//
//   0x00  char    magic[12]   "Packed File " (trailing space included)
//   0x0C  char    name[12]    original name, NUL padded - NOT NUL terminated
//   0x18  quint8  eof         0x1A
//   0x19  quint8  version     0x01
//   0x1A  quint8  method      0x0A
//   0x1B  quint32 size        uncompressed size, little endian, < 0x80000000
//   0x1F  ...     stream      XNetWarePackDecoder payload to end of file
//
// The name field is a FIXED 12 BYTES, so it is read as 12 and then cut at the
// first NUL; reading 11 would clip every full-length name.  In 24 of the 1713
// reference files the field holds binary junk instead of a name (an older
// packer left its own scratch there), so an unusable field falls back to the
// container's own file name, which is what the reference implementation publishes for those members too.
//
// The size field is the ONLY end-of-stream signal the codec has, which is why
// this class refuses a header whose size does not fit in a positive qint32:
// handing the decoder a wrong length does not fail, it silently truncates.
class XNetWarePackedFile : public XArchive {
    Q_OBJECT

public:
    explicit XNetWarePackedFile(QIODevice *pDevice = nullptr);

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
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint8 nVersion;
        quint8 nMethod;
        bool bNameFromHeader;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString deriveContainerName();
    static bool isUsableMemberName(const QByteArray &baName);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XNETWAREPACKEDFILE_H
