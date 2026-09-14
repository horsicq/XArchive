/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRTASFX_H
#define XRTASFX_H

#include "xsfx.h"

// RTA self-extractor: a 16-bit DOS MZ stub with the complete RTA container
// appended behind the declared executable image.  On every observed carrier
// the container's "KJd\0" magic starts at exactly the MZ overlay offset, so
// the identity comes from XRTA's structural validation of the appended
// archive - the stub carries no signature of its own.  Distinct from FT_RTA,
// which is the bare container at offset 0.
class XRTASFX : public XSFX {
    Q_OBJECT

public:
    explicit XRTASFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XRTASFX_H
