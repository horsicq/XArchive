/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XACESFX_H
#define XACESFX_H

#include "xsfx.h"

// ACE self-extractor.  Two carriers exist and both hold a complete, ordinary
// ACE 1.x/2.0 container that XACE already reads:
//   - a 16-bit DOS MZ stub with the archive appended at exactly the declared
//     executable image end (the overlay offset), and
//   - the WinACE 32-bit stub, which stores the archive in the PE resource
//     directory as resource type "ARCDATA", name "DATA".
// The stub itself carries no signature, so the identity comes entirely from
// XACE's structural validation of the located container.  Distinct from
// FT_ACE, which is the bare container at offset 0.
class XAceSFX : public XSFX {
    Q_OBJECT

public:
    explicit XAceSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XACESFX_H
