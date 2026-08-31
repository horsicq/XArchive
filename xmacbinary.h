/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMACBINARY_H
#define XMACBINARY_H

#include "games/xgamestorearchive_p.h"

// Native reader for MacBinary I, II, and III fork containers.
class XMacBinary final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XMacBinary(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    struct ENTRY_CONTEXT;

    bool appendEntry(ENTRY_CONTEXT *pContext, const QString &sSourceName,
                     qint64 nOffset, qint64 nSize);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif // XMACBINARY_H
