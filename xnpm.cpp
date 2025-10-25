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
#include "xnpm.h"

XBinary::XCONVERT _TABLE_XNPM_STRUCTID[] = {
    {XNPM::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
};

XNPM::XNPM(QIODevice *pDevice) : XTGZ(pDevice)
{
}

bool XNPM::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XTGZ xtgz(getDevice());

    if (xtgz.isValid()) {
        QList<XArchive::RECORD> listArchiveRecords = xtgz.getRecords(20000, pPdStruct);

        bResult = isValid(&listArchiveRecords, pPdStruct);
    }

    return bResult;
}

bool XNPM::isValid(QIODevice *pDevice)
{
    XNPM xtar(pDevice);

    return xtar.isValid();
}

bool XNPM::isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    bResult = XArchive::isArchiveRecordPresent("package/package.json", pListRecords, pPdStruct);

    return bResult;
}

QString XNPM::getFileFormatExt()
{
    return "tgz";
}

XBinary::FT XNPM::getFileType()
{
    return FT_NPM;
}

XBinary::FILEFORMATINFO XNPM::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    XBinary::FILEFORMATINFO result = {};

    if (isValid(pPdStruct)) {
        result.bIsValid = true;
        result.nSize = getSize();
        result.sExt = "tgz";
        result.fileType = FT_NPM;
    }

    return result;
}

XBinary::MODE XNPM::getMode()
{
    return MODE_32;
}

QString XNPM::getArch()
{
    return tr("Universal");
}

qint32 XNPM::getType()
{
    return TYPE_PACKAGE;
}

QString XNPM::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_PACKAGE: sResult = tr("Package");
    }

    return sResult;
}

QString XNPM::getMIMEString()
{
    return "application/x-npm";
}

QString XNPM::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XNPM_STRUCTID, sizeof(_TABLE_XNPM_STRUCTID) / sizeof(XBinary::XCONVERT));
}
