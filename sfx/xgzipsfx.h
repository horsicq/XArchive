/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGZIPSFX_H
#define XGZIPSFX_H

#include "xsfx.h"

class XGzipSFX : public XSFX {
    Q_OBJECT

public:
    explicit XGzipSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XGZIPSFX_H
