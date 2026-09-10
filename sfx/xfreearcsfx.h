/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFREEARCSFX_H
#define XFREEARCSFX_H

#include "xsfx.h"

class XFreeArcSFX : public XSFX {
    Q_OBJECT

public:
    explicit XFreeArcSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XFREEARCSFX_H
