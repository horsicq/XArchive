/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIS3SFXARCHIVE_H
#define XIS3SFXARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XIS3SFXArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XIS3SFXArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool hasZip64Extra(qint64 nExtraOffset, qint64 nExtraSize,
                       PDSTRUCT *pPdStruct);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XIS3SFXARCHIVE_H
