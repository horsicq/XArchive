/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGENTEE_H
#define XGENTEE_H

#include "xarchive.h"

#include "Algos/xgenteedecoder.h"

// Gentee installer self-extractor (gentee.com's script-driven setup builder).
//
// The carrier is a small PE32 stub and everything the installer owns lives in
// the overlay as ONE payload:
//
//   quint32 nRuntimeSize   -- decoded size of the first block, < 0x100000
//   AB 67 A7 36 FF 4D FB 6F
//   ...
//
// Those eight bytes are not a format magic, they are the first eight bytes of
// the first block's compressed stream: every build of the tool ships the same
// runtime image, so its stream starts identically every time.  That is what
// the reference implementation recognises the format by, and measured over the
// 43,149-file reference corpus (10.3 GiB) the eight bytes plus the bounded
// size word appear in 18 files, all of them Gentee installers.
//
// The payload layout and its codec are documented on XGenteeDecoder.  Two
// things matter to this reader:
//
//   * a block never stores its packed length, so the member table can only be
//     produced by DECODING the whole chain - listing costs a full unpack;
//   * the member data blocks share one decoder state, so a member can only be
//     decoded by replaying the members ahead of it.  A compressed member's
//     stream is therefore published as the payload from its first byte through
//     the end of that member's block, with the member's index in
//     FPART_PROP_COMPRESSPROPERTIES - the arrangement XAINArchive already uses
//     for its solid members.
//
// A member whose byte at +0x19 is zero carries its payload STORED instead, and
// that one rides HANDLE_METHOD_STORE with no solid dependency at all.
class XGentee final : public XArchive {
    Q_OBJECT

public:
    explicit XGentee(QIODevice *pDevice = nullptr);
    ~XGentee() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

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
        qint64 nPayloadOffset;
        qint64 nArchiveSize;
        QList<XGenteeDecoder::MEMBER> listMembers;
    };

    bool locatePayload(qint64 *pnPayloadOffset, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static qint64 memberStreamOffset(const CONTEXT &context, qint32 nIndex);
    static qint64 memberStreamSize(const CONTEXT &context, qint32 nIndex);
    static void fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XGENTEE_H
