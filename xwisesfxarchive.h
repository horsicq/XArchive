/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XWISESFXARCHIVE_H
#define XWISESFXARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XWiseSFXArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XWiseSFXArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XWISESFXARCHIVE_H
