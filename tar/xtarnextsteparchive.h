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
#ifndef XTARNEXTSTEPARCHIVE_H
#define XTARNEXTSTEPARCHIVE_H

#include "xarchive.h"

// TAR NextStep - ordinary tar with ONE CHANGE that moves every field: the name
// is 225 bytes instead of 100.  The 512-byte header block therefore reads
//
//   +0x000 char[225] name        (0xe1 bytes, not 0x64)
//   +0x0e1 char[8]   mode
//   +0x0e9 char[8]   uid
//   +0x0f1 char[8]   gid
//   +0x0f9 char[12]  size        octal
//   +0x105 char[12]  mtime       octal
//   +0x111 char[8]   chksum      octal
//   +0x119 char      typeflag
//
// and the checksum is the classic tar rule AT THE SHIFTED OFFSET: the sum of
// every header byte with the checksum field itself counted as eight spaces,
// i.e. 0x100 + sum(block[0 .. 0x111)) + sum(block[0x119 .. 0x200)).
//
// WHY THE SHIFT MATTERS MORE THAN IT LOOKS: a NextStep block is still a
// perfectly well-formed 512-byte tar block, so a POSIX tar reader parses it,
// finds a plausible name, and computes a checksum over the wrong ranges.  The
// checksum is the only thing that says which of the two layouts is in force -
// and it is also the whole detector here, because the format has NO MAGIC AND
// NO VERSION FIELD.  The FIRST block must verify or the file is rejected; a
// later mismatch just ends the walk, which is how trailing garbage and short
// archives behave in the reference implementation.
//
// typeflag: '\0' and '0' are regular files (but a name ending in '/' is a
// directory even then), '1' and '2' are hard and soft links and CARRY NO
// PAYLOAD - their size field must be forced to zero before the block cursor
// advances, or the walk lands mid-file and reports the rest of the archive as
// corrupt - and '5' is a directory.  Directories and links are not listed:
// the reference extractor writes file content only, and inventing records for
// them would put entries in the output that the reference does not produce.
class XTarNextStepArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nDataSize;
        quint32 nMTime;
        QString sFileName;
    };

    explicit XTarNextStepArchive(QIODevice *pDevice = nullptr);
    ~XTarNextStepArchive() override;

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
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTARNEXTSTEPARCHIVE_H
