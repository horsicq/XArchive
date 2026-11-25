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
#ifndef XFATIMAGE_H
#define XFATIMAGE_H

#include "xarchive.h"

// Raw FAT12/FAT16 floppy disk image (Atari ST .st, MSX and PC .img/.ima), as
// the scan engine opens it.  The same rules are implemented step for step in
// xxfclib (src/scan/xx_scan_floppy.c), so XScanEngine and cdie type the same
// files and list the same members in the same order: CHANGE BOTH TOGETHER.
// They are deliberately narrow: no FAT32, no long names, no truncated volumes.
//
// DETECTION.  Nothing but the BPB and the root directory identifies the image:
// Atari boot sectors carry no 0x55AA, often no media byte, and many images
// have zeroed FAT ids.
//   - bytes/sector (+11) is 512; sectors/cluster (+13) is a power of two up to
//     64; reserved sectors (+14) of 0 reads as 1 (some Atari formatters); 1 or
//     2 FATs (+16); root entries (+17) non-zero and a multiple of 16; total
//     sectors (+19, or +32 when that is 0) and sectors/FAT (+22) non-zero
//   - reserved + FATs + root is below the total, the cluster count is
//     1..65524 (below 4085: FAT12, else FAT16) and one FAT holds that many
//     entries
//   - the BPB volume is within half and five quarters of the image size, and
//     cluster 2 lies inside the image
//   - the root directory, up to its end marker, has at least one live entry
//     and at most one impossible entry per four live ones.  An entry is
//     impossible with attribute bits 0xC0, both the directory and the label
//     bit, a name byte below 0x20 or one of 0x7F " * / : < > ? \ |, a leading
//     space, a start cluster outside the volume, or a size above the data
//     area.  Deleted entries and long-name pieces do not count.
//
// MEMBERS.  The tree is walked in directory order, a subdirectory where its
// entry stands, up to 8 levels deep; every directory cluster is read once (a
// cycle ends there) and at most 2 MiB of one directory.  Listed are files with
// a non-zero size whose cluster chain covers the size inside the image;
// labels, deleted entries, long-name pieces, "." entries and directories
// themselves are not.  The name is the 8.3 short name, base and extension
// right-trimmed, bytes outside 0x20..0x7E and the separators / \ shown as '_',
// joined to its directories with '/'.  The walk ends after 10,000 members or
// when the listed sizes would pass twice the data area.
//
// RECORDS.  A file whose clusters follow one another is a stored extent of the
// image: a HANDLE_METHOD_STORE record at the offset of its first cluster.  A
// fragmented file has no extent, so it is published as an index-paired
// archive-stream record (markArchiveStreamRecord) and only unpackCurrent() can
// assemble it.
class XFATImage : public XArchive {
    Q_OBJECT

public:
    explicit XFATImage(QIODevice *pDevice = nullptr);
    ~XFATImage() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct GEOMETRY {
        quint32 nSectorsPerCluster;
        quint32 nReservedSectors;
        quint32 nNumberOfFats;
        quint32 nRootEntries;
        quint32 nTotalSectors;
        quint32 nFatSectors;
        quint32 nClusters;
        quint32 nClusterSize;
        quint32 nFatBytes;  // Bytes of one FAT that hold the entries of every cluster
        bool bFat16;
        qint64 nFatOffset;
        qint64 nRootOffset;
        qint64 nDataOffset;
        qint64 nImageSize;
    };

    struct MEMBER {
        QString sPath;
        qint64 nOffset;  // Image offset of the first cluster
        quint32 nSize;
        quint32 nCluster;
        bool bContiguous;
    };

    struct CONTEXT {
        GEOMETRY geometry;
        QByteArray baFat;
        QByteArray baSeen;  // One bit per cluster: directory clusters already read
        QList<MEMBER> listMembers;
        quint64 nListedSize;
        quint64 nListedLimit;
        bool bStop;
    };

    static bool readGeometry(const uchar *pBoot, qint64 nImageSize, GEOMETRY *pGeometry);
    static bool isDataCluster(const GEOMETRY &geometry, quint32 nCluster);
    static bool isClusterInImage(const GEOMETRY &geometry, quint32 nCluster);
    static qint64 getClusterOffset(const GEOMETRY &geometry, quint32 nCluster);
    static bool isNameValid(const uchar *pEntry);
    static qint32 classifyEntry(const GEOMETRY &geometry, const uchar *pEntry);
    static quint32 getFatEntry(const CONTEXT &context, quint32 nCluster);
    static bool isChainValid(const CONTEXT &context, quint32 nCluster, quint32 nSize, bool *pbContiguous);
    static QString getNamePart(const uchar *pData, qint32 nSize);
    static QString getShortName(const uchar *pEntry);

    bool detect(GEOMETRY *pGeometry, PDSTRUCT *pPdStruct);
    QByteArray readDirectory(CONTEXT *pContext, quint32 nCluster, PDSTRUCT *pPdStruct);
    void walkEntries(CONTEXT *pContext, const QByteArray &baEntries, const QString &sPrefix, qint32 nDepth, PDSTRUCT *pPdStruct);
    bool parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XFATIMAGE_H
