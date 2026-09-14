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
#ifndef XWIIWAD_H
#define XWIIWAD_H

#include "xarchive.h"

// Nintendo Wii WAD installable package (types 'Is' and 'ib').  This is the
// container WiiWare, Virtual Console, channel and IOS titles are distributed
// and installed in; it is NOT the PC Doom IWAD/PWAD handled by XWAD.  Format
// understanding derived from wiibrew.org (WAD_files, Ticket, Title_metadata,
// Certificate_chain) and libWiiPy (MIT).  Nothing from libWiiSharp or Sharpii
// (GPL) is reproduced here; their member naming is followed as a convention.
//
// Header, 0x20 bytes, every integer big endian, padded to 0x40:
//
//   0x00  u32      header size, must be 0x20
//   0x04  char[2]  'Is' installable / 'ib' boot2 ('Bk' backups are rejected,
//                  their contents are bound to a console-specific key)
//   0x06  u16      version, 0 in every known writer
//   0x08  u32      certificate chain size (normally 0xA00: CA, CP, XS)
//   0x0C  u32      CRL size (normally 0; honoured when non-zero)
//   0x10  u32      ticket size (>= 0x2A4)
//   0x14  u32      TMD size (>= 0x1E4 + 0x24 * contents)
//   0x18  u32      encrypted content data size
//   0x1C  u32      footer size (0 or a small build stamp)
//
// Every section starts at the next multiple of 0x40 after the previous one:
// certs at 0x40, then CRL, ticket, TMD, content data, footer.  Inside the
// content data each content occupies align64(size) bytes in TMD record order,
// and the ciphertext that must be decrypted is align16(size) bytes.  Some
// writers declare the data size with the last content unpadded, so the
// consistency rule is  sum(align64(size[i<n-1])) + align16(size[n-1]) <=
// align16(data size)  and every slot is bounds-checked against the device,
// not against the declared data size.  A file may end short of the final
// 0x40 padding only; WADs embedded in larger files (NAND dumps, ELF resources)
// are allowed trailing data, so getFileFormatSize() reports the section chain
// end clipped to the device.
//
// Ticket (v0, 0x2A4 bytes): signature type at 0x000 must be 0x00010001
// (RSA-2048; every offset below assumes that signature length), issuer at
// 0x140, format version at 0x1BC, the encrypted title key at 0x1BF, ticket id
// at 0x1D0, console id at 0x1D8, title id at 0x1DC, title version at 0x1E6
// and the common-key index at 0x1F1 (0 retail, 1 Korean, 2 vWii; fakesigned
// homebrew WADs carry garbage there, which is only reported, never acted on).
// TMD: signature type 0x00010001 at 0x000, issuer 0x140, version 0x180, vWii
// flag 0x183, system (IOS) version 0x184, title id 0x18C, group id 0x198,
// region 0x19C, access rights 0x1D8, title version 0x1DC, number of contents
// 0x1DE, boot index 0x1E0, then 0x24-byte records: content id u32, index u16,
// type u16 (0x0001 normal, 0x4001 DLC, 0x8001 shared), plaintext size u64,
// SHA-1 of the plaintext (20 bytes).
//
// Detection gate (isValid == parseContext, no key needed, deterministic and
// bounded by the input size): device >= 0x4E4 bytes, the 8-byte magic
// 00 00 00 20 'Is'/'ib' 00 00 (version pinned to 0 exactly as libWiiPy does;
// relax to 6 bytes if a non-zero-version sample ever appears), every section
// size within its cap, the whole 0x40-aligned section chain inside the
// device (a zero-length section - no certs, no CRL, no content data - may
// sit at or beyond the device end, so the minimal 0x4E4-byte WAD passes),
// both 0x00010001 signature types at header-derived offsets, the content
// count within WIIWAD_MAX_CONTENTS, the TMD large enough for its records,
// every content size non-zero (no installable title has an empty content;
// an empty member would be published at exit 0) and a self-consistent
// content slot map.  There is no checksum in
// the container: the per-content SHA-1s are only checkable with the key, so
// the structural chain IS the gate, and it is strong (magic + two signature
// type words at computed offsets + the slot map).
//
// Members, in this order (names follow the Sharpii/libWiiSharp convention,
// <tid> = ticket title id as 16 lowercase hex digits):
//   <tid>.cert  (when certs size > 0)   STORE, raw section
//   <tid>.crl   (when CRL size > 0)     STORE, raw section
//   <tid>.tik                           STORE, raw section
//   <tid>.tmd                           STORE, raw section
//   %08x.app of the content INDEX, one per TMD record in record order,
//               HANDLE_METHOD_ARCHIVE_STREAM: decrypted by this class;
//               a duplicate index gets "-<record number>" appended
//   <tid>.footer (when footer size > 0) STORE; a "TmStmp"/"CMiiUT" + 10
//               decimal digits stamp is published as its DATETIME
//
// Cryptography (section 3 of the spec):  title key = AES-128-CBC-decrypt
// (common key, IV = title id || 8 zero bytes) of ticket[0x1BF..0x1CF];
// content plaintext = AES-128-CBC-decrypt(title key, IV = content index as
// u16 big endian || 14 zero bytes) of the align16(size) ciphertext, truncated
// to size.  This tree embeds NO Nintendo key.  The common key is taken, in
// this order, from UNPACK_PROP_PASSWORD_BYTES (console -H <32 hex digits>,
// must be exactly 16 bytes - any other length is an error, not a fallback),
// UNPACK_PROP_PASSWORD when it is exactly 32 hex digits (any other string is
// ignored as a ZIP-style password), or a 16-byte companion file
// common-key.bin next to the WAD.  Without a key the listing is complete
// (content members carry FPART_PROP_ENCRYPTED = true and their TMD SHA-1),
// metadata members extract normally, and content extraction fails with
// "Wii common key required".  The decrypted plaintext is ALWAYS SHA-1
// checked against the TMD record - deliberately not gated on
// XBinary::isUnpackCRCEnabled / UNPACK_PROP_CHECKCRC, because the digest is
// the only detector of a wrong key (there is no check value for the title
// key) and skipping it would publish AES-CBC noise at exit 0; a mismatch
// fails the member before anything is published.  Keys are never placed in
// properties, INFO strings or error messages.
class XWiiWAD final : public XArchive {
    Q_OBJECT

public:
    explicit XWiiWAD(QIODevice *pDevice = nullptr);
    ~XWiiWAD() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct ENTRY {
        bool bIsContent;            // false = raw metadata section (STORE)
        qint64 nOffset;             // section start or content slot start
        qint64 nSize;               // bytes on the device (align16 for contents)
        qint64 nUncompressedSize;   // section size or plaintext size
        quint32 nContentId;
        quint16 nContentIndex;
        quint16 nContentType;
        QByteArray baSha1;          // 20 bytes, contents only
        QString sFileName;
        QString sInfo;              // per-member INFO, contents only
        QDateTime dtFooter;         // footer only, may be invalid
    };

    struct CONTEXT {
        qint64 nSourceSize;
        qint64 nArchiveEnd;
        quint16 nVersion;
        bool bIsBoot2;
        qint64 nCertOffset;
        qint64 nCertSize;
        qint64 nCrlOffset;
        qint64 nCrlSize;
        qint64 nTicketOffset;
        qint64 nTicketSize;
        qint64 nTmdOffset;
        qint64 nTmdSize;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nFooterOffset;
        qint64 nFooterSize;
        QByteArray baTitleId;            // 8 bytes from the ticket
        QByteArray baEncryptedTitleKey;  // 16 bytes from the ticket
        quint8 nCommonKeyIndex;
        bool bDevTicket;
        QString sInfo;                   // archive-level INFO
        QList<ENTRY> listEntries;
        // Key cache filled by resolveTitleKey(); both empty until a key is
        // supplied, recomputed when the supplied key bytes change.
        QByteArray baCommonKeyUsed;
        QByteArray baTitleKey;
    };

    // The single parser behind isValid(), getFileFormatSize(), getFileParts()
    // and initUnpack().  Needs no key and never decrypts.
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // Reads the common key from the unpack properties or the companion file
    // and derives the title key into the context cache.  Returns false with a
    // user-facing reason in *psError when no usable key is available.
    bool resolveTitleKey(UNPACK_STATE *pState, CONTEXT *pContext,
                         QString *psError);
    static bool resolveCommonKey(const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                 const QString &sDeviceFileName,
                                 QByteArray *pbaKey, QString *psError);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XWIIWAD_H
