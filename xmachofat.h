/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#ifndef XMACHOFAT_H
#define XMACHOFAT_H

#include "xarchive.h"
#include "xmach.h"

class XMACHOFat : public XArchive {
    Q_OBJECT

public:
    /*!
        \brief XMACHOFat class for handling Universal Mach-O (fat) binary files
        Universal Mach-O files contain multiple architecture-specific Mach-O binaries
        in a single file, allowing for multi-architecture support on macOS.
    */
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_ARCHITECTURE
    };

    enum TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_BUNDLE,
    };

    explicit XMACHOFat(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual ENDIAN getEndian();
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct);
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct);

    virtual OSNAME getOsName();
    virtual QString getFileFormatExt();
    virtual QString getFileFormatExtsString();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);
    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
    virtual QString getArch();
    virtual qint32 getType();
    virtual QString typeIdToString(qint32 nType);
    virtual FT getFileType();
    virtual QString getMIMEString();
    virtual bool isArchive();
    virtual QString structIDToString(quint32 nID);
    virtual qint64 getNumberOfArchiveRecords(PDSTRUCT *pPdStruct) override;

    XMACH_DEF::fat_header read_fat_header();
    XMACH_DEF::fat_arch read_fat_arch(qint32 nIndex);
    QList<XMACH_DEF::fat_arch> read_fat_arch_list(PDSTRUCT *pPdStruct);

    static QMap<quint64, QString> getHeaderMagics();
    static QMap<quint64, QString> getHeaderMagicsS();
    
    QString getArchitectureString(qint32 nIndex);
    qint64 getArchitectureOffset(qint32 nIndex);
    qint64 getArchitectureSize(qint32 nIndex);
    bool isArchitectureValid(qint32 nIndex);
};

#endif  // XMACHOFAT_H
