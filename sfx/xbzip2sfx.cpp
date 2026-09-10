/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xbzip2sfx.h"

XBzip2SFX::XBzip2SFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_BZIP2)
{
}

bool XBzip2SFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XBzip2SFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBzip2SFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
