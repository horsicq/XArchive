/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XAPPLESINGLE_H
#define XAPPLESINGLE_H

#include "games/xgamestorearchive_p.h"

// Native store-only reader for AppleSingle and AppleDouble entry containers.
class XAppleSingle final : public XGameStoreArchiveBase {
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XAppleSingle(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XAPPLESINGLE_H
