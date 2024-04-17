/* Copyright (c) 2017-2024 hors<horsicq@gmail.com>
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
#include "xtgz.h"

XTGZ::XTGZ(QIODevice *pDevice)
{
    g_pXtar = new XTAR;
    g_pCompressedDevice = new XCompressedDevice;
    XTGZ::setDevice(pDevice);
}

XTGZ::~XTGZ()
{
    delete g_pXtar;
    delete g_pCompressedDevice;
}

bool XTGZ::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (g_pCompressedDevice->isValid()) {
        bResult = g_pXtar->isValid(pPdStruct);
    }

    return bResult;
}

bool XTGZ::isValid(QIODevice *pDevice)
{
    XTGZ xtgz(pDevice);

    return xtgz.isValid();
}

void XTGZ::setDevice(QIODevice *pDevice)
{
    g_pCompressedDevice->setData(pDevice, FT_GZIP);

    XBinary::setDevice(g_pCompressedDevice);
}

quint64 XTGZ::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    if (g_pCompressedDevice->isValid()) {
        nResult = g_pXtar->getNumberOfRecords(pPdStruct);
    }

    return nResult;
}

QList<XArchive::RECORD> XTGZ::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> result;

    if (g_pCompressedDevice->isValid()) {
        result = g_pXtar->getRecords(nLimit, pPdStruct);
    }

    return result;
}

QString XTGZ::getFileFormatExt()
{
    return "tgz";
}

QList<XBinary::MAPMODE> XTGZ::getMapModesList(PDSTRUCT *pPdStruct)
{
    QList<XBinary::MAPMODE> result;

    if (g_pCompressedDevice->isValid()) {
        result = g_pXtar->getMapModesList(pPdStruct);
    }

    return result;
}
