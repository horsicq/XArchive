/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#ifndef XBZIP2SFX_H
#define XBZIP2SFX_H

#include "xsfx.h"

class XBzip2SFX : public XSFX {
    Q_OBJECT

public:
    explicit XBzip2SFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XBZIP2SFX_H
