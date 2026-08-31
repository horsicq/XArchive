/*
 * DiskDoubler/DDAR/DDA2 reader derived from XADMaster, LGPL 2.1 or later;
 * see Algos/xadmaster/COPYING.
 */
#ifndef XDISKDOUBLERARCHIVE_H
#define XDISKDOUBLERARCHIVE_H

#include "games/xgamestorearchive_p.h"

class XDiskDoublerArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XDiskDoublerArchive(QIODevice *pDevice = nullptr,
                                 FT fileType = FT_DISK_DOUBLER);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, FT fileType,
                        PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    struct PARSE_CONTEXT;

    bool addUniqueEntry(PARSE_CONTEXT *pContext, ENTRY *pEntry,
                        const QString &sPath);
    bool parseFileHeader(PARSE_CONTEXT *pContext, qint64 nMagicOffset,
                         const QString &sPath, qint64 nContainerEnd);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XDISKDOUBLERARCHIVE_H
