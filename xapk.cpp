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
#include "xapk.h"

XAPK::XAPK(QIODevice *pDevice)
    : XJAR(pDevice)
{

}

bool XAPK::isValid(PDSTRUCT *pPdStruct)
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

bool XAPK::isValid(QIODevice *pDevice)
{
    XAPK xapk(pDevice);

    return xapk.isValid();
}

bool XAPK::isValid(QList<RECORD> *pListRecords)
{
    bool bResult = false;

    bResult = (XArchive::isArchiveRecordPresent("classes.dex", pListRecords) ||
               XArchive::isArchiveRecordPresent("AndroidManifest.xml", pListRecords));

    return bResult;
}

XBinary::FT XAPK::getFileType()
{
    return FT_APK;
}

XBinary::FILEFORMATINFO XAPK::getFileFormatInfo()
{
    XBinary::FILEFORMATINFO result = {};

    XAPK xapk(getDevice());

    if (xapk.isValid()) {
        result.bIsValid = true;
        result.nSize = xapk.getFileFormatSize();
        result.sString = "APK";
        result.sExt = "apk";
        result.fileType = FT_APK;
    }

    return result;
}

QString XAPK::getFileFormatExt()
{
    return "apk";
}

XBinary::OSINFO XAPK::getOsInfo()
{
    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    QList<XArchive::RECORD> listRecords = getRecords(2000, &pdStructEmpty);

    return getOsInfo(&listRecords, &pdStructEmpty);
}

XBinary::OSINFO XAPK::getOsInfo(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    XBinary::OSINFO result = {};

    // TODO

    return result;
}

bool XAPK::isBigEndian()
{
    return false;
}

XBinary::MODE XAPK::getMode()
{
    return MODE_UNKNOWN;
}

QString XAPK::getArch()
{
    return "";
}

qint32 XAPK::getType()
{
    return 0;
}

QString XAPK::typeIdToString(qint32 nType)
{
    return "";
}
