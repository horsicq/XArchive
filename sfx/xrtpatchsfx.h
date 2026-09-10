/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRTPATCHSFX_H
#define XRTPATCHSFX_H

#include "xsfx.h"

class XRTPatchSFX : public XSFX {
    Q_OBJECT

public:
    explicit XRTPatchSFX(QIODevice *pDevice = nullptr,
                         bool bIsImage = false,
                         XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice,
                        PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XRTPATCHSFX_H
