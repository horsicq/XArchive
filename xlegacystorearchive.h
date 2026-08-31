/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLEGACYSTOREARCHIVE_H
#define XLEGACYSTOREARCHIVE_H

#include "games/xgamestorearchive_p.h"

// Small, fully bounded readers for historical containers whose directory is
// either store-only or describes one LPAK-compressed stream.  Keeping these
// related table formats in one implementation avoids several nearly identical
// XArchive state machines while retaining a distinct FT for every format.
class XLegacyStoreArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XLegacyStoreArchive(QIODevice *pDevice = nullptr,
                                 FT fileType = FT_UNKNOWN);

    using XGameStoreArchiveBase::isValid;
    static FT detectFileType(QIODevice *pDevice,
                             PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice, FT fileType,
                        PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    struct EntryBuilder;

    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XLEGACYSTOREARCHIVE_H
