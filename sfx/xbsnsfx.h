/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBSNSFX_H
#define XBSNSFX_H

#include "xsfx.h"

// PhysTechSoft BSA (".BSN") self-extractor: a 16-bit DOS MZ stub with the
// complete BSA container appended behind the declared executable image.  The
// carrier has no signature of its own, so the identity comes entirely from
// XBSN's structural validation of the appended archive.
class XBsnSFX : public XSFX {
    Q_OBJECT

public:
    explicit XBsnSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XBSNSFX_H
