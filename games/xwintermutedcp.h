/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XWINTERMUTEDCP_H
#define XWINTERMUTEDCP_H

#include "xgamestorearchive_p.h"

// Wintermute Engine DCP package reader (versions 0x100 and 0x200).
class XWintermuteDCP final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XWintermuteDCP(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XWINTERMUTEDCP_H
