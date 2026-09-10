/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xcabsfx.h"

XCabSFX::XCabSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_CAB)
{
}

bool XCabSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XCabSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCabSFX sfx(pDevice);

    return sfx.isValid(pPdStruct);
}
