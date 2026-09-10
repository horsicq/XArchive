/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDISKJUGGLERARCHIVE_H
#define XDISKJUGGLERARCHIVE_H

#include "games/xgamestorearchive_p.h"

// Bounded DiscJuggler CDI reader.  Track layout follows the public No$cash
// description and is independently validated against the image data boundary.
class XDiskJugglerArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XDiskJugglerArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    struct ENTRY_CONTEXT;

    bool appendEntry(ENTRY_CONTEXT *pContext, qint64 nOffset,
                     qint64 nPacked, qint64 nRaw, HANDLE_METHOD method,
                     const QString &sSourceName, qint64 nDescriptor);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XDISKJUGGLERARCHIVE_H
