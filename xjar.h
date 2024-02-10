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
#ifndef XJAR_H
#define XJAR_H

#include "xzip.h"

class XJAR : public XZip {
    Q_OBJECT
public:
    enum TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_PACKAGE,
        // TODO more
    };

    explicit XJAR(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    static bool isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct);

    virtual FT getFileType();
    virtual FILEFORMATINFO getFileFormatInfo();
    virtual QString getFileFormatExt();
    virtual OSINFO getOsInfo();
    virtual OSINFO getOsInfo(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct);
    virtual ENDIAN getEndian();
    virtual MODE getMode();
    virtual QString getArch();
    virtual qint32 getType();
    virtual QString typeIdToString(qint32 nType);
    static QString _getJDKVersion(quint16 nMajor, quint16 nMinor);
};

#endif  // XJAR_H
