/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xrtasfx.h"

XRTASFX::XRTASFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_RTA)
{
}

bool XRTASFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XRTASFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRTASFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
