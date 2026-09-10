/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGKSETUP_H
#define XGKSETUP_H

#include "xarchive.h"

// GkSetup installer data file (SETUP.DAT / SETUP.DA_), the payload container of
// the freeware "GkSetup" Win16/Win32 setup builder.
//
// Fixed header:
//   0x0000  "This is a binary data file. Keep out !" 0x1A   (39 bytes)
//   0x0050  "GK"                                            (signature)
//   0x0052  quint16 0
//   0x0054  quint32 0x160
//   0x0128  quint32 nDataOffset   -- 0x600 or 0x700, where the member chain
//                                    starts; the bytes in between hold the
//                                    builder's own script/UI blob.
//
// From nDataOffset the file is a flat, sequential chain of records; there is no
// index and no member count, the chain simply runs to EOF.  One record is a
// 0x124-byte WIN32_FIND_DATA-shaped descriptor immediately followed by the
// member payload:
//   0x0000  char     szName[0x104]   NUL padded, may contain '\' path segments
//   0x0104  quint32  nAttributes     FILE_ATTRIBUTE_* (0x10 == directory)
//   0x0108  FILETIME ftCreation
//   0x0110  FILETIME ftLastAccess
//   0x0118  FILETIME ftLastWrite
//   0x0120  qint32   nSize           payload length, 0 for directories
//
// Some writers emit an extra all-zero quint32 between the descriptor and the
// payload.  Nothing in the header announces it, so the reader probes the first
// two records exactly the way the reference extractor does and then holds the
// answer for the whole chain.
//
// Directories are not members: a record with FILE_ATTRIBUTE_DIRECTORY pushes
// its name onto the current path, and the literal name ".." pops one level.
// Everything else is a file whose payload is STORED verbatim, so extraction
// rides HANDLE_METHOD_STORE and no codec is involved.
class XGkSetup final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nSize;
        quint32 nAttributes;
        QString sFileName;
    };

    explicit XGkSetup(QIODevice *pDevice = nullptr);
    ~XGkSetup() override;

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
        qint64 nArchiveSize;
        qint64 nDataOffset;
        bool bHasPadding;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readRecord(qint64 nOffset, QString *pName, quint32 *pAttributes, qint64 *pSize, PDSTRUCT *pPdStruct);
    bool probePadding(qint64 nOffset, qint64 nInputSize, bool *pbHasPadding, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString sanitizeName(const QString &sRaw);
};

#endif  // XGKSETUP_H
