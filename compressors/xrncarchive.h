/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRNCARCHIVE_H
#define XRNCARCHIVE_H

#include "games/xgamestorearchive_p.h"

// Delphine/Bullfrog multi-file RNC container ("RNCA").  Each directory
// member is either a canonical RNC1/RNC2 stream or an RNC0 stored record.
class XRncArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XRncArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XRNCARCHIVE_H
