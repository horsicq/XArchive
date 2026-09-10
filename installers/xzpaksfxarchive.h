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
#ifndef XZPAKSFXARCHIVE_H
#define XZPAKSFXARCHIVE_H

#include "xarchive.h"

// The "-ZPAK" self-extracting installer: an MZ / NE extraction stub with the
// archive appended behind it.  It shares the five-byte magic and the PKWARE DCL
// payload with XIBMZPak and NOTHING ELSE - the directory entry is 114 bytes,
// not 88, the trailer is 22 or 38 bytes, not 2, and version 2 replaces the
// directory with a per-member record.  It is a separate reader for that reason.
//
// The archive NEVER starts at offset 0, so the container cannot be found by
// reading the head of the file.  It is found from the END:
//
//   version 2 trailer, the last 38 bytes:
//     +0x00  4  i32 member count
//     +0x04  4  u32 FILE OFFSET OF THE ARCHIVE HEADER   <- self-locating
//     +0x08  4  u32 size hint: file size plus 16 * member count, unused here
//     +0x0c 26  product tag, NUL padded, frequently empty
//
//   version 1 trailer, the last 22 bytes:
//     +0x00  2  u16 member count
//     +0x02  4  u32 FILE OFFSET OF THE ARCHIVE HEADER   <- self-locating
//     +0x06  4  u32 total file size
//     +0x0a 12  product tag, NUL padded, frequently empty
//
// The 8-byte header that offset points at is "-ZPAK" 00 then a u16 version, and
// the byte AFTER it is the version gate: version 1 is followed directly by the
// first member's DCL prelude (literal mode 0/1, dictionary code 4/5/6), version
// 2 by the 0x0a record tag.  That gate is what the reference implementation tests,
// and it is what keeps the two layouts apart before either directory is read.
//
// version 1 - directory at the end, immediately before the trailer, one
// 114-byte entry per member in stream order:
//     +0x00 102  name, NUL terminated and NUL padded to the full field
//     +0x66   4  i32 COMPRESSED size
//     +0x6a   4  i32 reserved, always zero AND CHECKED - see below
//     +0x6e   2  u16 DOS date
//     +0x70   2  u16 DOS time
//   Payloads run back to back from headerOffset+8; the running sum of the
//   compressed sizes landing exactly on the first directory byte is this
//   layout's only integrity check and a mismatch is treated as a reject.
//
// version 2 - no directory.  Each member is a 16-byte record, then a name, then
// the payload:
//     +0x00   1  tag, always 0x0a
//     +0x01   1  unused
//     +0x02   4  i32 COMPRESSED size
//     +0x06   4  WRITER SCRATCH - NOT a reserved-zero field.  See below.
//     +0x0a   2  u16 DOS date
//     +0x0c   2  u16 DOS time
//     +0x0e   2  i16 name size, NUL included; the name follows the record
//   After the last member 0 to 6 filler bytes separate the payloads from the
//   trailer (six in every corpus file, the head of an unwritten record whose
//   tag byte is not 0x0a).
//
// THE TWO "RESERVED" WORDS ARE NOT THE SAME FIELD.  The reference implementation
// reads the 114-byte version 1 entry into a struct that names the dword at
// +0x6a and rejects the archive when it is non-zero, so this reader checks it
// too.  It reads the 16-byte version 2 record into a DIFFERENT struct whose
// members are the tag pair at +0, the i32 at +2, the two u16 at +0x0a/+0x0c and
// the i16 at +0x0e - +0x06 is an unreferenced four-byte hole between two of
// them, is never loaded, and is therefore never checked.  Measured over this
// family: six of the nine version 2 archives write zero there, three write a
// per-archive non-zero constant (0x18, 0x13, 0x11, identical across every
// member of the archive and produced by a byte-identical writer stub), so a
// reserved-zero check on it rejects those three outright.  This reader does not
// read the field at all.
//
// Names mix separators - "\INSTALL/DLL\FXSINDEX.DL_" is one real member - so
// BOTH '\' and '/' have to be accepted as one, which is the single behavioural
// difference from XIBMZPak's name gate.
//
// Every member is one complete PKWARE DCL Implode stream with its two prelude
// bytes attached, so HANDLE_METHOD_PKWARE_DCL_IMPLODE decodes it as it stands
// and this class adds NO codec.  As in XIBMZPak the plaintext length is stored
// nowhere and only falls out of the end-of-stream code, so parseContext() takes
// a bScanSizes flag and recovers it with XDclDecoder::scan() on the paths that
// need it; a member whose length could not be measured is reported as
// HANDLE_METHOD_UNKNOWN rather than extracted at length zero.
class XZPakSFXArchive : public XArchive {
    Q_OBJECT

public:
    // clang-format off
    // ===== BEGIN ZPAKSFX SHARED TYPES =====
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        // False until XDclDecoder::scan() has recovered the plaintext length.
        bool bUncompressedSizeKnown;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };
    // ===== END ZPAKSFX SHARED TYPES =====
    // clang-format on

    explicit XZPakSFXArchive(QIODevice *pDevice = nullptr);
    ~XZPakSFXArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    // clang-format off
    // ===== BEGIN ZPAKSFX SHARED CONTEXT =====
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nArchiveOffset;
        qint64 nTrailerOffset;
        // The version 1 directory; -1 for version 2, which has none.
        qint64 nDirectoryOffset;
        qint64 nFirstMemberOffset;
        quint16 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bScanSizes, PDSTRUCT *pPdStruct);
    bool readHeaderAt(qint64 nOffset, qint64 nLimit, quint16 nVersion,
                      PDSTRUCT *pPdStruct);
    bool walkVersion1(CONTEXT *pContext, qint32 nNumberOfMembers,
                      PDSTRUCT *pPdStruct);
    bool walkVersion2(CONTEXT *pContext, qint32 nNumberOfMembers,
                      PDSTRUCT *pPdStruct);
    bool isDclPreludeAt(qint64 nOffset, PDSTRUCT *pPdStruct);
    bool scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct);
    static bool isNameField(const QByteArray &baName, QString *pName);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    // ===== END ZPAKSFX SHARED CONTEXT =====
    // clang-format on

    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZPAKSFXARCHIVE_H
