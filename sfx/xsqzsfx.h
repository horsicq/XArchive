/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSQZSFX_H
#define XSQZSFX_H

#include "xsfx.h"

class XSqzSFX : public XSFX {
    Q_OBJECT

public:
    explicit XSqzSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSQZSFX_H
