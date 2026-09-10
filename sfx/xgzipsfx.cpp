/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgzipsfx.h"

XGzipSFX::XGzipSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_GZIP)
{
}

bool XGzipSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XGzipSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGzipSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
