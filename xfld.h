/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFLD_H
#define XFLD_H

#include "xarchive.h"

// Sequiter Software CodeBase 5 / CodeReporter install file group (.FLD).
//
// HEADERLESS: there is no magic anywhere in the container.  The file is a flat
// chain of members, each one a 27-byte record followed immediately by its
// payload, and it closes with a 5-byte trailer.
//
// Member record (27 bytes, little endian):
//   quint8  nNameLength     always 0x0C - the width of the name field, and the
//                           only fixed byte the format has
//   char    szName[12]      8.3 name, space padded, never contains a control
//                           character
//   qint32  nCompressedSize   payload length, header excluded
//   qint32  nUncompressedSize
//   quint16 nDosTime
//   quint16 nDosDate
//   quint8  nMethod         0 = stored, 1 = PKWARE DCL implode
//   quint8  nMarker         always '$'
//
// The payload of a method-1 member is a complete PKWARE Data Compression
// Library implode stream WITH its own two-byte prelude (literal mode byte in
// {0,1}, dictionary-size byte in {4,5,6}), so it is handed to
// HANDLE_METHOD_PKWARE_DCL_IMPLODE verbatim from the first payload byte.
//
// Because nothing here is a magic number, detection walks the whole chain and
// requires it to tile the file exactly: every record well formed, every
// compressed payload carrying a valid DCL prelude, and the walk landing on the
// 5-byte trailer (or the end of file) with nothing left over.
class XFLD : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nDosDateTime;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XFLD(QIODevice *pDevice = nullptr);
    ~XFLD() override;

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
        qint64 nArchiveSize;
        qint64 nTrailerOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static QString methodToString(quint8 nMethod);
};

#endif  // XFLD_H
