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
#include "xcompresseddevice.h"

XCompressedDevice::XCompressedDevice(QObject *parent) : XIODevice(parent)
{
    g_pSubDevice = nullptr;
    g_fileType = XBinary::FT_UNKNOWN;
}

XCompressedDevice::~XCompressedDevice()
{
    if (g_pSubDevice) {
        g_pSubDevice->close();
    }
}

bool XCompressedDevice::setData(QIODevice *pDevice, XBinary::FT fileType)
{
    bool bResult = false;

    g_fileType = fileType;

    if (fileType == XBinary::FT_GZIP) {
        XGzip xgzip(pDevice);

        if (xgzip.isValid()) {
            XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
            QList<XArchive::RECORD> listRecords = xgzip.getRecords(1, &pdStruct);

            if (listRecords.count()) {
                XArchive::RECORD record = listRecords.at(0);

                g_pSubDevice = new SubDevice(pDevice, record.nDataOffset, record.nCompressedSize);

                if (g_pSubDevice->open(QIODevice::ReadOnly)) {
                    setSize(record.nUncompressedSize);
                }
            }
        }
    }

    return bResult;
}
