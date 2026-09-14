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
#ifndef XISSETUPSTREAM_H
#define XISSETUPSTREAM_H

#include "xarchive.h"

// InstallShield "ISSetupStream" - the payload block that a modern (2009+)
// InstallShield Setup.exe, or the Binary.ISSetup.dll stream of an
// InstallShield-authored MSI, carries inside its own PE image.
//
// It is NOT an overlay and NOT at offset 0: the block lives in a data section
// of the carrier, so the reader SCANS for the 14-byte tag "ISSetupStream\0"
// and validates the 46-byte header behind it.  A carrier normally also holds
// the same 14 bytes as a plain string constant in .rdata; that decoy is
// rejected because its record count is zero and the byte behind it is not a
// container version.
//
// Header, 46 bytes:
//     +0x00  char[14]  "ISSetupStream\0"
//     +0x0e  u16       number of records, non-zero
//     +0x10  u8        container version - 2 or 3 (every observed file is 3)
//     +0x11  u8[29]    zero
//
// Record, 24 bytes, followed by the name and then the member stream:
//     +0x00  u32       name size IN BYTES, non-zero, even, <= 0x10000
//     +0x04  u32       cipher selector (see below)
//     +0x08  u16       zero
//     +0x0a  u32       member stream size, as stored
//     +0x0e  u64       zero
//     +0x16  u16       storage: 0 = the stream is the file, 1 = zlib stream
//     then      name, UTF-16LE, exactly the declared number of bytes
//     then      the member stream, exactly the declared number of bytes
//
// EVERY member is wrapped in a stream cipher keyed by the member's OWN name,
// which is why the container has to be read record by record rather than
// sliced by offset: the key travels with the record.  The cipher selector
// picks the wrapper - 0 (and the -1/-3 "no filter" encodings the reference
// spells out) means the stream is not enciphered, 2 and 6 are the two filter
// variants.  Anything else is a member this reader cannot produce, and such a
// record is published with HANDLE_METHOD_UNKNOWN rather than being passed off
// as stored bytes.
//
// THE KEY IS SALTED.  The reader publishes only the member name as UTF-8 in
// FPART_PROP_COMPRESSPROPERTIES; the four-byte salt and the nibble swap that
// go with it live in ONE place, decISSetupStream() in
// XArchive/core/xdecompress.cpp.  Do not re-apply the salt here - two
// applications cancel out and every member decodes to noise.
//
// The container records no inflated length for a compressed member, so
// FPART_PROP_UNCOMPRESSEDSIZE is published ONLY for a stored one.  The zlib
// Adler-32 footer is the authenticator for the rest.
class XISSetupStream : public XArchive {
    Q_OBJECT

public:
    explicit XISSetupStream(QIODevice *pDevice = nullptr);
    ~XISSetupStream() override;

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
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nStreamSize;
        qint32 nCipher;   // 0, 2 or 6; -1 marks a selector this reader refuses
        qint32 nStorage;  // 0 = stored, 1 = zlib
        QString sFileName;
        QByteArray baName;  // the member name as UTF-8: the UNSALTED key material
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nBaseOffset;
        qint64 nArchiveSize;
        qint32 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool parseAt(qint64 nBaseOffset, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static HANDLE_METHOD methodOf(const MEMBER &member);
    static QString describe(const MEMBER &member);
    static QByteArray propertyOf(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XISSETUPSTREAM_H
