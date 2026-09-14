/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xasymetrixsfx.h"

XAsymetrixSFX::XAsymetrixSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_ASYMETRIX)
{
}

bool XAsymetrixSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XAsymetrixSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAsymetrixSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
