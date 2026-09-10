/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCONCATZIPARCHIVE_H
#define XCONCATZIPARCHIVE_H

#include "games/xgamestorearchive_p.h"

// Some InstallShield 3 launchers concatenate several complete, independent
// ZIP files instead of writing one multi-member ZIP. This bounded adapter
// presents all of those component members as one archive to the SFX layer.
class XConcatZipArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XConcatZipArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XCONCATZIPARCHIVE_H
