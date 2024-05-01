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
#include "xnpm.h"

XNPM::XNPM(QIODevice *pDevice) : XTGZ(pDevice)
{
}

bool XNPM::isValid(PDSTRUCT *pPdStruct)
{
    // TODO
    return false;
}

bool XNPM::isValid(QIODevice *pDevice)
{
    XNPM xtar(pDevice);

    return xtar.isValid();
}

QString XNPM::getFileFormatExt()
{
    return "tgz";
}

XBinary::FT XNPM::getFileType()
{
    return FT_NPM;
}

XBinary::FILEFORMATINFO XNPM::getFileFormatInfo()
{
    XBinary::FILEFORMATINFO result = {};

    XNPM xnpm(getDevice());

    if (xnpm.isValid()) {
        result.bIsValid = true;
        result.nSize = xnpm.getFileFormatSize();
        result.sString = "NPM";
        result.sExt = "tgz";
        result.fileType = FT_NPM;
    }

    return result;
}

XBinary::OSINFO XNPM::getOsInfo()
{
    OSINFO result = {};

    result.osName = OSNAME_MULTIPLATFORM;
    result.sArch = getArch();
    result.mode = getMode();
    result.sType = typeIdToString(getType());
    result.endian = getEndian();

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
