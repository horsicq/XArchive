/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCREATEINSTALLSFX_H
#define XCREATEINSTALLSFX_H

#include "Algos/xcreateinstalldecoder.h"
#include "xarchive.h"

// CreateInstall self-extractor, "instcrin" generation.
//
// This is the PRE-Gentee CreateInstall builder (Gentee Inc., late 1990s to
// early 2000s): the carrier is a PE32 stub, the overlay opens with the
// compressed installer runtime "instcrin.dll", and the member records follow a
// short builder prelude.  The LATER CreateInstall installers carry a ".gentee"
// section and a "GEA" container and belong to XCreateInstall / XGentee, not
// here; the two do not overlap - this reader claims a carrier only when the
// first eight overlay bytes are the constant compressed form of the runtime's
// MZ header (61 57 41 57 AE 40 60 1B).
//
// The container layout, the codec and the record walk are documented in
// Algos/xcreateinstalldecoder.h.  Everything that reads bytes lives there so
// that the walk and the codec can be exercised without the archive framework.
//
// WHAT THIS READER PUBLISHES.  One member per type 1 record plus the runtime
// DLL as member 0, in container order.  Directory records (type 2 / type 3)
// only shape the member paths; they are not published as entries of their own.
//
// LISTING COSTS A FULL DECODE.  Nothing in the container records a stream's
// compressed length, so the only way to find the next record is to decode the
// current member.  parseContext() therefore walks - and decodes - the whole
// payload, exactly as XGentee and XQSetup do.  The decoded bytes are thrown
// away during the walk; only the stream extents are kept.
//
// NAMES.  A record name is an ANSI byte string, sometimes a quoted builder
// script path.  The quotes are removed when both are present, '\\' becomes
// '/', and each component has the characters Windows forbids replaced by '_'.
// '%' is deliberately NOT touched: a name may hold the builder's
// "%installpath%" variable and folding it away would merge two members the
// container keeps apart - one carrier ships both "Uninstal.exe" and
// "%installpath%\\Uninstal.exe" and they are different files.  Two members that
// still land on the same name after that fall back to an indexed name rather
// than overwriting each other.
class XCreateInstallSFX final : public XArchive {
    Q_OBJECT

public:
    explicit XCreateInstallSFX(QIODevice *pDevice = nullptr);
    ~XCreateInstallSFX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nArchiveSize;
        qint64 nRuntimeSize;
        QList<XCreateInstallDecoder::RECORD> listMembers;
    };

    bool locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static void fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCREATEINSTALLSFX_H
