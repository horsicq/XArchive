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
#include "xtgz.h"

XTGZ::XTGZ(QIODevice *pDevice)
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
    bool bResult = false;

    T_UNPACK_CONTEXT *pContext = new T_UNPACK_CONTEXT;
    pContext->pDevice = XBinary::createFileBuffer(getSize(), pPdStruct);

    // Unpack to device
    bResult = XGzip(getDevice()).unpackSingleStream(pContext->pDevice, pPdStruct);

    // Init TAR unpacking
    if (bResult) {
        pContext->pTar = new XTAR(pContext->pDevice);
        bResult = pContext->pTar->initUnpack(&(pContext->usTar), mapProperties, pPdStruct);
    }

    pUnpackState->pContext = pContext;

    return bResult;
}

XBinary::ARCHIVERECORD XTGZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    XBinary::ARCHIVERECORD record;

    T_UNPACK_CONTEXT *pContext = (T_UNPACK_CONTEXT *)pState->pContext;
    XBinary::ARCHIVERECORD _record = pContext->pTar->infoCurrent(&(pContext->usTar), pPdStruct);

    return record;
}

bool XTGZ::unpackCurrent(UNPACK_STATE *pUnpackState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    // if ((m_pXtar) && (pUnpackState) && (pDevice)) {
    //     bResult = m_pXtar->unpackCurrent(pUnpackState, pDevice, pPdStruct);
    // }

    return bResult;
}

bool XTGZ::moveToNext(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    // if ((m_pXtar) && (pUnpackState)) {
    //     bResult = m_pXtar->moveToNext(pUnpackState, pPdStruct);
    // }

    return bResult;
}

bool XTGZ::finishUnpack(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pUnpackState && pUnpackState->pContext) {
        T_UNPACK_CONTEXT *_pState = (T_UNPACK_CONTEXT *)pUnpackState->pContext;
        _pState->pTar->finishUnpack(&_pState->usTar, pPdStruct);

        if (_pState->pTar) {
            delete _pState->pTar;
        }
        XBinary::freeFileBuffer(&(_pState->pDevice), pPdStruct);
        delete _pState;
        pUnpackState->pContext = nullptr;
        bResult = true;
    }

    return bResult;
}

QString XTGZ::getFileFormatExt()
{
    return "tar.gz";
}

XBinary::FT XTGZ::getFileType()
{
    return FT_TARGZ;
}
