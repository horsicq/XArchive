/* Copyright (c) 2022-2025 hors<horsicq@gmail.com>
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
#include "xdeb.h"

XDEB::XDEB(QIODevice *pDevice) : X_Ar(pDevice)
{
}

bool XDEB::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    X_Ar xar(getDevice());

    if (xar.isValid()) {
        QList<XArchive::RECORD> listArchiveRecords = xar.getRecords(10, pPdStruct);

        bResult = isValid(&listArchiveRecords, pPdStruct);
    }

    return bResult;
}

bool XDEB::isValid(QIODevice *pDevice)
{
    XDEB xdeb(pDevice);

    return xdeb.isValid();
}

bool XDEB::isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    return isArchiveRecordPresent("debian-binary", pListRecords, pPdStruct);
}

QString XDEB::getVersion()
{
    return getFileFormatInfo(nullptr).sVersion;
}

QString XDEB::getFileFormatExt()
{
    return "deb";
}

QString XDEB::getMIMEString()
{
    return "application/vnd.debian.binary-package";
}

XBinary::FILEFORMATINFO XDEB::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    XBinary::FILEFORMATINFO result = {};

    QList<XArchive::RECORD> listArchiveRecords = getRecords(3, nullptr);

    if (isValid(&listArchiveRecords, pPdStruct)) {
        result.bIsValid = true;
        result.nSize = getSize();
        result.sExt = "apk";
        result.fileType = FT_DEB;

        RECORD record = getArchiveRecord("debian-binary", &listArchiveRecords);

        if (record.spInfo.nUncompressedSize < 10) {
            QByteArray baVersion = decompress(&record);
            result.sVersion.append(baVersion);
            result.sVersion = result.sVersion.trimmed();
        }

        result.osName = OSNAME_DEBIANLINUX;

        result.sArch = getArch();
        result.mode = getMode();
        result.sType = typeIdToString(getType());
        result.endian = getEndian();
    }

    return result;
}

XBinary::FT XDEB::getFileType()
{
    return FT_DEB;
}
