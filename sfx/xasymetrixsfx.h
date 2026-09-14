/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XASYMETRIXSFX_H
#define XASYMETRIXSFX_H

#include "xsfx.h"

// Asymetrix ToolBook Setup self-extractor.  The carrier is a 16-bit Windows NE
// stub ("appsetup") that appends a complete, ordinary Asymetrix disk-set volume
// which XAsymetrix already reads in full: the directory record offsets are
// stored relative to the CONTAINER, not to the carrier, so the located region
// parses byte-for-byte like a bare ".001" file and needs no new decoder.
//
// The container never begins at the NE overlay boundary.  NE places its
// resource table after the last segment, so the archive starts roughly a
// kilobyte past the boundary XNE reports, which is why the plain overlay
// candidates miss it and the bounded post-overlay signature scan is what finds
// the 8-byte header.  Distinct from FT_ASYMETRIX, which is the bare volume at
// offset 0.
class XAsymetrixSFX : public XSFX {
    Q_OBJECT

public:
    explicit XAsymetrixSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XASYMETRIXSFX_H
