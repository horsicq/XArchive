/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xbsnsfx.h"

XBsnSFX::XBsnSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_BSN)
{
}

bool XBsnSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XBsnSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBsnSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
