/* Copyright (c) 2020-2025 hors<horsicq@gmail.com>
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
#ifndef XARCHIVES_H
#define XARCHIVES_H

// TODO more
#include "x_ar.h"
#include "xcab.h"
#include "xcpio.h"
#include "xgzip.h"
#include "xiso9660.h"
#include "xmachofat.h"
#include "xrar.h"
#include "xsevenzip.h"
#include "xsquashfs.h"
#include "xzip.h"
#include "xzlib.h"
#include "xlha.h"
#include "xjar.h"
#include "xapk.h"
#include "xipa.h"
#include "xapks.h"
#include "xtar.h"
#include "xtgz.h"
#include "xnpm.h"
#include "xdeb.h"
#include "xdos16.h"
#include "xcfbf.h"
#include "xcab.h"
#include "xszdd.h"
#include "xbzip2.h"
#include "xxz.h"
#include "xminidump.h"

class XArchives : public QObject {
    Q_OBJECT

public:
    explicit XArchives(QObject *pParent = nullptr);

    static XArchive *getClass(XBinary::FT fileType, QIODevice *pDevice);
    static QList<XArchive::RECORD> getRecords(QIODevice *pDevice, XBinary::FT fileType = XBinary::FT_UNKNOWN, qint32 nLimit = -1, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static QList<XArchive::RECORD> getRecords(const QString &sFileName, XBinary::FT fileType = XBinary::FT_UNKNOWN, qint32 nLimit = -1,
                                              XBinary::PDSTRUCT *pPdStruct = nullptr);
    static QList<XArchive::RECORD> getRecordsFromDirectory(const QString &sDirectoryName, qint32 nLimit = -1, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static QByteArray decompress(QIODevice *pDevice, const XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct = nullptr, qint64 nDecompressedOffset = 0,
                                 qint64 nDecompressedSize = -1);
    static QByteArray decompress(const QString &sFileName, XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct = nullptr, qint64 nDecompressedOffset = 0,
                                 qint64 nDecompressedSize = -1);
    static QByteArray decompress(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static QByteArray decompress(const QString &sFileName, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decompressToFile(QIODevice *pDevice, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decompressToDevice(QIODevice *pDevice, XArchive::RECORD *pRecord, QIODevice *pDestDevice, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decompressToFile(const QString &sFileName, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decompressToFile(const QString &sFileName, const QString &sRecordFileName, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decompressToFolder(QIODevice *pDevice, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decompressToFolder(const QString &sFileName, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool isArchiveRecordPresent(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool isArchiveRecordPresent(const QString &sFileName, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool isArchiveOpenValid(QIODevice *pDevice, const QSet<XBinary::FT> &stAvailable);
    static bool isArchiveOpenValid(const QString &sFileName, const QSet<XBinary::FT> &stAvailable);
    static QSet<XBinary::FT> getArchiveOpenValidFileTypes();

private:
    static void _findFiles(const QString &sDirectoryName, QList<XArchive::RECORD> *pListRecords, qint32 nLimit,
                           XBinary::PDSTRUCT *pPdStruct);  // TODO mb nLimit pointer to qint32
};

#endif  // XARCHIVES_H
