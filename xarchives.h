/* Copyright (c) 2020-2022 hors<horsicq@gmail.com>
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

// TODO tar
#include "xcab.h"
#include "xmachofat.h"
#include "xrar.h"
#include "xsevenzip.h"
#include "xzip.h"

class XArchives : public QObject
{
    Q_OBJECT

public:
    explicit XArchives(QObject *pParent=nullptr);

    static QList<XArchive::RECORD> getRecords(QIODevice *pDevice,qint32 nLimit=-1);
    static QList<XArchive::RECORD> getRecords(QString sFileName,qint32 nLimit=-1);
    static QList<XArchive::RECORD> getRecordsFromDirectory(QString sDirectoryName,qint32 nLimit=-1);
    static QByteArray decompress(QIODevice *pDevice,XArchive::RECORD *pRecord,bool bHeaderOnly=false,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static QByteArray decompress(QString sFileName,XArchive::RECORD *pRecord,bool bHeaderOnly=false,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static QByteArray decompress(QIODevice *pDevice,QString sRecordFileName,bool bHeaderOnly=false,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static QByteArray decompress(QString sFileName,QString sRecordFileName,bool bHeaderOnly=false,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static bool decompressToFile(QIODevice *pDevice,XArchive::RECORD *pRecord,QString sResultFileName,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static bool decompressToFile(QString sFileName,XArchive::RECORD *pRecord,QString sResultFileName,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static bool decompressToFile(QString sFileName,QString sRecordFileName,QString sResultFileName,XBinary::PDSTRUCT *pPdStruct=nullptr);
    static bool isArchiveRecordPresent(QIODevice *pDevice,QString sRecordFileName);
    static bool isArchiveRecordPresent(QString sFileName,QString sRecordFileName);
    static bool isArchiveOpenValid(QIODevice *pDevice,QSet<XBinary::FT> stAvailable);
    static bool isArchiveOpenValid(QString sFileName,QSet<XBinary::FT> stAvailable);

private:
    static void _findFiles(QString sDirectoryName,QList<XArchive::RECORD> *pListRecords,qint32 nLimit); // TODO mb nLimit pointer to qint32 Check
};

#endif // XARCHIVES_H
