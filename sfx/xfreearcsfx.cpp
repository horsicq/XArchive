/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xfreearcsfx.h"

XFreeArcSFX::XFreeArcSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_FREEARC)
{
}

bool XFreeArcSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XFreeArcSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFreeArcSFX sfx(pDevice);

    return sfx.isValid(pPdStruct);
}
