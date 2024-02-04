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
#include "xjar.h"

XJAR::XJAR(QIODevice *pDevice) : XZip(pDevice)
{

}

bool XJAR::isValid(PDSTRUCT *pPdStruct) // PDSTRUCT
{
    bool bResult = false;

    XZip xzip(getDevice());

    if (xzip.isValid()) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();

        QList<XArchive::RECORD> listArchiveRecords = xzip.getRecords(20000, &pdStruct);

        bResult = isValid(&listArchiveRecords);
    }

    return bResult;
}

bool XJAR::isValid(QIODevice *pDevice)
{
    XJAR xjar(pDevice);

    return xjar.isValid();
}

bool XJAR::isValid(QList<RECORD> *pListRecords)
{
    bool bResult = false;

    bResult = XArchive::isArchiveRecordPresent("META-INF/MANIFEST.MF", pListRecords);

    return bResult;
}

XBinary::FT XJAR::getFileType()
{
    return FT_JAR;
}

XBinary::FILEFORMATINFO XJAR::getFileFormatInfo()
{
    XBinary::FILEFORMATINFO result = {};

    XJAR xjar(getDevice());

    if (xjar.isValid()) {
        result.bIsValid = true;
        result.nSize = xjar.getFileFormatSize();
        result.sString = "JAR";
        result.sExt = "jar";
        result.fileType = FT_JAR;
    }

    return result;
}

QString XJAR::getFileFormatExt()
{
    return "jar";
}
