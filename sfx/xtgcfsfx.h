/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XTGCFSFX_H
#define XTGCFSFX_H

#include "xsfx.h"

// "Setup Specialist" TGCF self-extractor: a 16-bit Windows NE stub with the
// complete TGCF container appended behind the declared executable image.  On
// every observed carrier the container header starts at exactly the NE overlay
// offset, so the identity comes from XTGCFArchive's structural validation of
// the appended archive - the stub itself carries no signature of its own.
class XTgcfSFX : public XSFX {
    Q_OBJECT

public:
    explicit XTgcfSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTGCFSFX_H
