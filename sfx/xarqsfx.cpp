/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarqsfx.h"

XArqSFX::XArqSFX(QIODevice *pDevice, bool bIsImage,
                 XADDR nModuleAddress)
    : XSFX(pDevice, bIsImage, nModuleAddress, ARC_ARQ)
{
}

bool XArqSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XArqSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArqSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
