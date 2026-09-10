/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrtpatchsfx.h"

XRTPatchSFX::XRTPatchSFX(QIODevice *pDevice, bool bIsImage,
                         XADDR nModuleAddress)
    : XSFX(pDevice, bIsImage, nModuleAddress, ARC_RTPATCH)
{
}

bool XRTPatchSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XRTPatchSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRTPatchSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}
