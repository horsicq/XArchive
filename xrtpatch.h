/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRTPATCH_H
#define XRTPATCH_H

#include "games/xgamestorearchive_p.h"

// Pocket Soft RTPatch update packages.  Structurally validated source-free
// whole-file streams across the supported descriptor generations and the
// optional plain-text banner are decoded natively.  In mixed packages,
// source-dependent binary-delta operations are omitted from the extractable
// archive view.  Packages containing only deltas keep those records visible as
// unsupported so extraction cannot claim to have reconstructed target files.
class XRTPatch final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XRTPatch(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XRTPATCH_H
