/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xsqzsfx.h"

XSqzSFX::XSqzSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_SQZ)
{
}

bool XSqzSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XSqzSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSqzSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
