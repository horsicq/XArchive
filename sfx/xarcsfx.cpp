/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarcsfx.h"

XArcSFX::XArcSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_ARC)
{
}

bool XArcSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XArcSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArcSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
