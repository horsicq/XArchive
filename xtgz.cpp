/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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

XTGZ::XTGZ(QIODevice *pDevice) : XGzip(pDevice)
{
}

XTGZ::~XTGZ()
{
}

bool XTGZ::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (XGzip::isValid(pPdStruct)) {
        bResult = true;
    }

    return bResult;
}

bool XTGZ::isValid(QIODevice *pDevice)
{
    XTGZ xtgz(pDevice);

    return xtgz.isValid();
}

bool XTGZ::initUnpack(UNPACK_STATE *pUnpackState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    return XTAR::sub_initUnpack(FT_TARGZ, getDevice(), pUnpackState, mapProperties, pPdStruct);
}

XBinary::ARCHIVERECORD XTGZ::infoCurrent(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    return XTAR::sub_infoCurrent(FT_TARGZ, pUnpackState, pPdStruct);
}

bool XTGZ::unpackCurrent(UNPACK_STATE *pUnpackState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    return XTAR::sub_unpackCurrent(FT_TARGZ, pUnpackState, pDevice, pPdStruct);
}

bool XTGZ::moveToNext(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    return XTAR::sub_moveToNext(FT_TARGZ, pUnpackState, pPdStruct);
}

bool XTGZ::finishUnpack(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    return XTAR::sub_finishUnpack(FT_TARGZ, pUnpackState, pPdStruct);
}

QString XTGZ::getFileFormatExt()
{
    return "tar.gz";
}

XBinary::FT XTGZ::getFileType()
{
    return FT_TARGZ;
}
