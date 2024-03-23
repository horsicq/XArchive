/* Copyright (c) 2017-2023 hors<horsicq@gmail.com>
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
#ifndef XTAR_H
#define XTAR_H

#include "xarchive.h"

class XTAR : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct posix_header {   /* byte offset */
        char name[100];     /*   0 */
        char mode[8];       /* 100 */
        char uid[8];        /* 108 */
        char gid[8];        /* 116 */
        char size[12];      /* 124 */
        char mtime[12];     /* 136 */
        char chksum[8];     /* 148 */
        char typeflag[1];   /* 156 */
        char linkname[100]; /* 157 */
        char magic[6];      /* 257 */
        char version[2];    /* 263 */
        char uname[32];     /* 265 */
        char gname[32];     /* 297 */
        char devmajor[8];   /* 329 */
        char devminor[8];   /* 337 */
        char prefix[155];   /* 345 */
                            /* 500 */
    };
#pragma pack(pop)

public:
    explicit XTAR(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    bool _isValid(_MEMORY_MAP *pMemoryMap, qint64 nOffset, PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct);
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct);
    virtual QString getFileFormatExt();
    virtual QList<MAPMODE> getMapModesList(PDSTRUCT *pPdStruct = nullptr);

private:
    posix_header read_posix_header(qint64 nOffset);

signals:
};

#endif  // XTAR_H
