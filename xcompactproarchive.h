/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Parser structure and codec behavior are based on XADMaster, LGPL 2.1 or
 * later; see Algos/xadmaster/COPYING.
 */
#ifndef XCOMPACTPROARCHIVE_H
#define XCOMPACTPROARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XCompactProArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XCompactProArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    struct PARSE_CONTEXT;

    bool appendFork(PARSE_CONTEXT *pContext, const QString &sPath,
                    qint64 nRecordOffset, quint32 nStoredCrc,
                    qint32 nForkCount, bool bResource, qint64 nOffset,
                    qint64 nPacked, qint64 nRaw, bool bLzh);
    bool parseDirectory(PARSE_CONTEXT *pContext, const QString &sParent,
                        qint32 nRecords, qint32 nDepth);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XCOMPACTPROARCHIVE_H
