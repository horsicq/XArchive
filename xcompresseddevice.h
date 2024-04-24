/* Copyright (c) 2024 hors<horsicq@gmail.com>
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
#ifndef XCOMPRESSEDDEVICE_H
#define XCOMPRESSEDDEVICE_H

#include "xiodevice.h"
#include "subdevice.h"
#include "xgzip.h"

class XCompressedDevice : public XIODevice {
    Q_OBJECT
public:
    explicit XCompressedDevice(QObject *parent = nullptr);
    ~XCompressedDevice();

    bool setData(QIODevice *pDevice, XBinary::FT fileType);
    virtual bool open(OpenMode mode);

    void setLayerSize(qint64 nLayerSize);
    qint64 getLayerSize();
    void setLayerOffset(qint64 nLayerOffset);
    qint64 getLayerOffset();
    void setLayerCompressMethod(XArchive::COMPRESS_METHOD compressMethod);
    XArchive::COMPRESS_METHOD getLayerCompressMethod();

protected:
    virtual qint64 readData(char *pData, qint64 nMaxSize);
    virtual qint64 writeData(const char *pData, qint64 nMaxSize);

private:
    SubDevice *g_pSubDevice;
    XBinary::FT g_fileType;
    bool g_bIsValid;
    qint64 g_nLayerSize;
    qint64 g_nLayerOffset;
    XArchive::COMPRESS_METHOD g_compressMethod;
};

#endif  // XCOMPRESSEDDEVICE_H
