/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XZPAQSFX_H
#define XZPAQSFX_H

#include "xsfx.h"

class XZPAQSFX : public XSFX {
    Q_OBJECT

public:
    explicit XZPAQSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZPAQSFX_H
