/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMI10ARCHIVE_H
#define XMI10ARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XMI10Archive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XMI10Archive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XMI10ARCHIVE_H
