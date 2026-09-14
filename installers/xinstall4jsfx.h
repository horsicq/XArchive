/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XINSTALL4JSFX_H
#define XINSTALL4JSFX_H

#include "xarchive.h"

// ej-technologies install4j / exe4j self-extracting launcher.
//
// The carrier is a PE32 stub and the container sits AT the PE overlay.  It
// opens with a table of builder variables:
//
//   quint32 0xE8E413D5            -- stored D5 13 E4 E8
//   qint32  nVariableCount        -- 0 < n < 0x400
//   nVariableCount x { qint32 nKey; qint32 nLength; nLength bytes }
//   qint32  nExtraCount
//   nExtraCount   x { qint32 nKey; qint32 nLength; nLength bytes }
//
// The first variable is always key 101 (the product name), which together with
// the magic and the two bounded counts is what the reference implementation
// recognises the format by.  Key 2003 carries the MEMBER LIST: the installed
// file names joined and terminated by ';' ("exe4jlib.jar;i4jdel.exe;").
// Nothing else in the container names a member, so the member count is the
// number of names in that one string.
//
// Behind the variable tables the members follow back to back, in the order of
// the key 2003 list:
//
//   quint32 nSize
//   [quint32 0]        -- present only in the later builds, see below
//   nSize bytes, every one of them XOR 0x88
//
// The 0x88 is the whole codec: 17,011 XORed bytes are the 17,011-byte
// exe4jlib.jar, PK header and central directory included.  No size other than
// nSize is declared anywhere, so packed and unpacked size are the same number.
//
// The optional zero word is how the format tells its two generations apart,
// and the reference implementation reads it exactly this way: the dword behind
// nSize is peeked, and it is swallowed only when it is zero, otherwise it is
// the member's first four bytes.  A member's first four plaintext bytes would
// have to be 88 88 88 88 to be mistaken for the pad, which no member in the
// reference corpus is.  The zero pad ALSO announces a trailer behind the last
// member:
//
//   quint32 0xD513E4E8            -- stored E8 E4 13 D5, the magic reversed
//   quint32be nCount
//   nCount x { quint16be nNameLength; name; quint64be nSize; nSize bytes }
//
// Those trailer members are STORED, not XORed.  Every carrier in the reference
// corpus declares nCount == 0, so that arm is implemented from the reference
// implementation's own walk and is not exercised by a sample; it is written to
// refuse anything it cannot bound rather than to guess.
class XInstall4jSFX final : public XArchive {
    Q_OBJECT

public:
    explicit XInstall4jSFX(QIODevice *pDevice = nullptr);
    ~XInstall4jSFX() override;

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
    struct MEMBER {
        QString sFileName;    // the name the launcher installs the member under
        qint64 nStreamOffset; // first byte of the member's data
        qint64 nStreamSize;   // its length; packed and unpacked size are equal
        bool bStored;         // trailer member: no XOR filter
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nRecordsOffset;
        qint64 nArchiveSize;
        QString sProductName;
        QList<MEMBER> listMembers;
    };

    bool locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct);
    bool parseVariables(CONTEXT *pContext, QByteArray *pbaNameList, PDSTRUCT *pPdStruct);
    bool parseMembers(CONTEXT *pContext, const QByteArray &baNameList, PDSTRUCT *pPdStruct);
    bool parseTrailer(CONTEXT *pContext, qint64 *pnPosition, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static void fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XINSTALL4JSFX_H
