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
#ifndef XPEA_H
#define XPEA_H

#include "xexternalarchive.h"

class XPEA : public XExternalArchive {
    Q_OBJECT

public:
    explicit XPEA(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    FT getFileType() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QString getMIMEString() override;
    OSNAME getOsName() override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    quint8 getFormatVersion() const;
    quint8 getFormatRevision() const;
    qint64 getFirstStreamTriggerOffset() const;

private:
    static const qint32 PEA_ARCHIVE_HEADER_SIZE = 10;
    static const qint32 PEA_STREAM_FIXED_SIZE = 10;
};

#endif  // XPEA_H
