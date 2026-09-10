/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFLS_H
#define XFLS_H

#include "games/xgamestorearchive_p.h"

// SaveRam/SaveRam2 (FLS) archive. Both stored and compressed members are
// available through the normal archive extraction pipeline.
class XFLS final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XFLS(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XFLS_H
