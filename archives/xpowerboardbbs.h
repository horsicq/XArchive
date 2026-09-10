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
#ifndef XPOWERBOARDBBS_H
#define XPOWERBOARDBBS_H

#include "xarchive.h"

// Powerboard BBS (DOS, Nu-Kote / Powerboard Systems, 1993-1996) library file.
// The BBS ships its display screens, help text, .CTL macro scripts and even
// its own executables glued together in one flat ".BBS" / ".DAT" library that
// the board opens by name; the corpus samples are named PBBS.BBS, PBFILES.BBS,
// EXPRESS.EXP, LETTERS.LTR, HELP1.H1, and so on.
//
// The container is HEADERLESS - no magic, no index, no trailer.  It is a bare
// chain of stored records:
//
//     [lead u8][name field][size field][raw data]
//
// The lead byte carries both the base-name length and the width of the size
// field that follows the name:
//
//     lead 11..18  ->  base length = lead - 10, size is 1 byte  (< 255)
//     lead  1..8   ->  base length = lead,      size is 2 or 4 bytes
//
// The name field is always base length + 3: an unpadded base name followed by
// a fixed 3-byte extension field that is blank-filled when the member has no
// extension ("LAUGH" is stored as "LAUGH   ", "PWRMAIL.EXE" as "PWRMAILEXE").
//
// For the lead 1..8 form the writer used a Turbo Pascal Integer when the
// member fitted in one (<= 32767 bytes) and a LongInt otherwise; there is no
// flag bit to tell the two apart, and the low word of a LongInt size is very
// often < 0x8000 (pboard.EXE at 335556 bytes stores C4 1E 05 00).  The only
// thing that disambiguates them is the chain itself, so parseContext() walks
// the file with a bounded backtracking stack: it takes the 2-byte reading
// first and falls back to the 4-byte reading when the tail no longer chains.
// Across the 84-sample corpus that costs at most 3 backtracks per file.
//
// Acceptance is deliberately strict, because the format has no magic to lean
// on and a loose gate here previously let two of these samples be claimed by
// the ShrinkWrap disk-image reader, which then wrote three meaningless raw
// slices and exited 0.  isValid() therefore demands that the whole file parse
// as an unbroken chain landing exactly on EOF, with every record carrying a
// non-empty DOS-legal 8.3 name and a non-zero payload.  Measured: 84/84 of
// the family accepted, 0 hits over 41089 files of the other ARC/ARC2/ARC3/
// ARC4 families.
//
// Members are STORED - the format has no compression at all.
class XPowerBoardBBS : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XPowerBoardBBS(QIODevice *pDevice = nullptr);
    ~XPowerBoardBBS() override;

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

    // One decoded record header.  A lead of 1..8 can yield two readings of the
    // size field (2-byte first, then 4-byte); nCount says how many are live.
    struct HEADER {
        qint32 nCount;
        qint32 nNameLength;
        qint32 nWidth[2];
        qint64 nSize[2];
        QString sFileName;
    };

    bool readHeader(qint64 nOffset, qint64 nInputSize, HEADER *pHeader, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPOWERBOARDBBS_H
