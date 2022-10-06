/* Copyright (c) 2022 hors<horsicq@gmail.com>
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
#include "xgzip.h"

XGzip::XGzip(QIODevice *pDevice) : XArchive(pDevice)
{

}

bool XGzip::isValid()
{
    bool bResult=false;

    if(getSize()>sizeof(GZIP_HEADER))
    {
        quint16 nSignature=read_uint16(0);

        if(nSignature==0x8B1F) // TODO Const
        {
            bResult=true;
        }
    }

    return bResult;
}

bool XGzip::isValid(QIODevice *pDevice)
{
    XGzip xgzip(pDevice);

    return xgzip.isValid();
}

quint64 XGzip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return 1; // Always 1
}

QList<XArchive::RECORD> XGzip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit) // Always 1

    QList<RECORD> listResult;

    RECORD record={};

    // TODO

    listResult.append(record);

    return listResult;
}
