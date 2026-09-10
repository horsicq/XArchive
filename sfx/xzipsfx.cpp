/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xzipsfx.h"

XZipSFX::XZipSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_ZIP)
{
}

bool XZipSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XZipSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZipSFX sfx(pDevice);

    return sfx.isValid(pPdStruct);
}
