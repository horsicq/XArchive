/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xzpaqsfx.h"

XZPAQSFX::XZPAQSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_ZPAQ)
{
}

bool XZPAQSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XZPAQSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZPAQSFX sfx(pDevice);

    return sfx.isValid(pPdStruct);
}
