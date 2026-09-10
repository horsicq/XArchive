/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlhasfx.h"

XLhaSFX::XLhaSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_LHA)
{
}

bool XLhaSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XLhaSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLhaSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
