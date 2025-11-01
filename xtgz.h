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
#ifndef XTGZ_H
#define XTGZ_H

#include "xtar.h"
#include "xcompresseddevice.h"

class XTGZ : public XArchive {
    Q_OBJECT

public:
    explicit XTGZ(QIODevice *pDevice = nullptr);
    virtual ~XTGZ();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);

    void setDevice(QIODevice *pDevice);

    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<RECORD> getRecords(qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getFileFormatExt() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual FT getFileType() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getMIMEString() override;

    virtual bool initUnpack(UNPACK_STATE *pUnpackState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pUnpackState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct = nullptr) override;

    XCompressedDevice *getCompressedDevice();

private:
    void _closeResources();

    XTAR *m_pXtar;
    XCompressedDevice *m_pCompressedDevice;
    QIODevice *m_pDevice;
};
#endif  // XTGZ_H
