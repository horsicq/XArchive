/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSCI_H
#define XSCI_H

#include "xarchive.h"

// SCI100 / SCI200 - the container the "sixxac/INSTALL" installer reads.  It
// ships the sixxac/CHAT BBS door as .sxd / .sxp payloads.
//
//   +0x00  "SCI100 - use sixxac/INSTALL 1.00 to expand." 0D 0A 1A
//                                                46-byte banner header
//
// then a flat chain of members, each
//
//   +0x00  u8   kind (0 plain, 1 display/macro, 2 the installer script)
//   +0x01  .    name[50], NUL terminated inside the field
//   +0x33  i32  member size
//   +0x37  .    member bytes, uncompressed
//
// running to end of file - there is no terminator record and no directory.
// Nothing is compressed.
//
// Two details bite readers of this format:
//   * the 50-byte name field is NOT NUL padded.  The writer leaves whatever
//     stood in its buffer behind the terminator, so only the bytes up to the
//     first NUL are the name and the tail must not be validated.
//   * names carry DOS path components ("disp\yell.ans") and the first member
//     is normally an absolute one ("\pro\sci\sccdinst.cfg").  Separators are
//     normalised to '/' and the leading one is dropped, matching where the
//     reference extractor puts the file.
//
// The banner is only partly fixed: the reference detector pins "SCI1"/"SCI2"
// at +0, "00 -" at +4 and "nd." CR LF 1A at +0x28, and lets the middle vary.
//
// Reference: handler "SCI" (class inb, entry A701), detector at VA
// 0x006dfee0, worker at 0x006dff30.
class XSCI final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nSize;
        quint8 nKind;
        QString sFileName;
    };

    explicit XSCI(QIODevice *pDevice = nullptr);
    ~XSCI() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        bool bTruncated;
        QString sBanner;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSCI_H
