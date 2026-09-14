/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xacesfx.h"

XAceSFX::XAceSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_ACE)
{
}

bool XAceSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XAceSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAceSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
