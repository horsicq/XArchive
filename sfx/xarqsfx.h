/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARQSFX_H
#define XARQSFX_H

#include "xsfx.h"

class XArqSFX : public XSFX {
    Q_OBJECT

public:
    explicit XArqSFX(QIODevice *pDevice = nullptr, bool bIsImage = false,
                     XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XARQSFX_H
