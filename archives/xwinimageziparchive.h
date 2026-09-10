/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XWINIMAGEZIPARCHIVE_H
#define XWINIMAGEZIPARCHIVE_H

#include "games/xgamestorearchive_p.h"

// WinImage self-extractors ("RsDl" overlay header followed by "WSfx" and the
// appended ZIP) write a hybrid directory: the end-of-central-directory record
// addresses the central directory RELATIVE to the appended archive, while
// every central record addresses its local header by its ABSOLUTE position in
// the outer executable. Neither the rebased nor the whole-device XZip view can
// resolve that mixture.
//
// This adapter is deliberately bound to the COMPLETE outer file, never to a
// rebased view, because that is the only place where the archive's true start
// can be derived instead of guessed:
//
//     nCentralOffset = nEcdOffset - nSizeOfCentralDirectory     (footer-local)
//     nArchiveOffset = nCentralOffset - nStoredOffsetToCentral  (the delta)
//
// With nArchiveOffset known, the central directory's local-header links stop
// being free parameters: each one must equal the absolute position the
// sequential local-header walk has already reached, so a link that points into
// the central directory - or anywhere other than the next real, non-
// overlapping local header - is rejected outright. A conforming archive and a
// fully absolute SFX both derive nArchiveOffset == 0 and are therefore never
// claimed here; they stay with XZip.
class XWinImageZipArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XWinImageZipArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    struct CENTRALRECORD {
        quint16 nFlags;
        quint16 nMethod;
        quint16 nDosTime;
        quint16 nDosDate;
        quint32 nCRC32;
        quint32 nCompressedSize;
        quint32 nUncompressedSize;
        quint32 nLocalHeaderOffset;
        QByteArray baName;
    };

    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
    bool readCentralDirectory(qint64 nCentralOffset, qint64 nCentralSize,
                              qint32 nRecords, QList<CENTRALRECORD> *pRecords,
                              PDSTRUCT *pPdStruct);
    bool walkLocalHeaders(const QList<CENTRALRECORD> &listRecords,
                          qint64 nArchiveOffset, qint64 nCentralOffset,
                          QList<ENTRY> *pEntries, PDSTRUCT *pPdStruct);
};

#endif  // XWINIMAGEZIPARCHIVE_H
