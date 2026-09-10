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
#ifndef XZFSFARCHIVE_H
#define XZFSFARCHIVE_H

#include "xarchive.h"

// ZFSF - a stored-only container whose directory is a LINKED LIST OF GROUPS
// rather than one contiguous table.
//
//   +0x00 "ZFSF"
//   +0x04 u32  version, must be 1
//   +0x08 u32  must be 0x10
//   +0x0c i32  groupCap    entries per directory group, > 0 (100 in practice)
//   +0x10 i32  totalCount  total members, >= 0
//   +0x14 u32  timestamp-looking
//   +0x18 u32  must be 0x1c - the offset of the first group
//
//   group:  +0x00 u32 nextGroup   absolute offset, 0 = last
//           +0x04 up to groupCap entries of 0x24
//   entry:  +0x00 char[0x10] name, NUL padded
//           +0x10 i32 offset      ABSOLUTE file offset of the data
//           +0x14 u32 ?
//           +0x18 i32 size
//           +0x1c u32 mtime
//           +0x20 u32 ?
//
// TWO TRAPS IN THE WALK, both of which the reference walker guards and both of
// which turn into a hang or a wrong file list if you drop them:
//
//  * THE LAST GROUP IS SHORT.  A group holds min(groupCap, remaining) entries,
//    NOT groupCap.  Reading the full capacity out of the final group appends
//    whatever bytes follow the directory - usually the first member's data -
//    as phantom entries with absurd offsets.
//  * nextGroup MUST MOVE FORWARD.  A nextGroup that points at or before the
//    current group is how a truncated or crafted file loops forever; the
//    reference walker rejects the archive outright rather than skipping it.
//
// Member offsets are absolute and need not be ordered, so the archive size is
// the whole file rather than a running sum.
class XZFSFArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nDataSize;
        quint32 nMTime;
        QString sFileName;
    };

    explicit XZFSFArchive(QIODevice *pDevice = nullptr);
    ~XZFSFArchive() override;

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

#endif  // XZFSFARCHIVE_H
