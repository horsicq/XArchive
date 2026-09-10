/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIS14SFXARCHIVE_H
#define XIS14SFXARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XIS14SFXArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XIS14SFXArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool readCString(qint64 nTotalSize, qint64 *pPosition,
                     QByteArray *pValue, PDSTRUCT *pPdStruct);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XIS14SFXARCHIVE_H
