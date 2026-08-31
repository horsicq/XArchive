/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPYINSTALLERCARCHIVE_H
#define XPYINSTALLERCARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XPyInstallerCArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XPyInstallerCArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XPYINSTALLERCARCHIVE_H
