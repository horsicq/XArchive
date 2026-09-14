/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xtgcfsfx.h"

XTgcfSFX::XTgcfSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_TGCF)
{
}

bool XTgcfSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XTgcfSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTgcfSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
