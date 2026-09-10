/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARCSFX_H
#define XARCSFX_H

#include "xsfx.h"

class XArcSFX : public XSFX {
    Q_OBJECT

public:
    explicit XArcSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XARCSFX_H
