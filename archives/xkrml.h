/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XKRML_H
#define XKRML_H

#include "xarchive.h"

// "KRML" resource archive of the DOS game KREML ("Кремль") - the game ships its
// data as KREML.ENG / KRSND.ENG / ROM.RUS / FAB.RUS, all in this container.
//
//   +0x00 char[4]  "KRML"
//   +0x04 u16      member count (never 0)
//   +0x06          count records of 21 bytes:
//                      +0x00 char[13]  member name, NUL padded
//                      +0x0d i32       absolute file offset of the member
//                      +0x11 i32       member size
//
// Members are STORED - there is no compression, no checksum and no timestamp
// anywhere in the container, and the data area follows the directory with the
// members laid out back to back in directory order.
//
// The one structural invariant strong enough to detect a container with a
// 4-byte magic and nothing else in it is the original's own test: the first
// record's offset must be exactly 6 + count * 21, i.e. the data must start
// where the directory ends.
class XKRML final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nIndexOffset;
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XKRML(QIODevice *pDevice = nullptr);
    ~XKRML() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
        qint64 nDirectorySize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XKRML_H
