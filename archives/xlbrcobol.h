/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLBRCOBOL_H
#define XLBRCOBOL_H

#include "xarchive.h"

// Micro Focus COBOL Library File (.LBR / .OBR / .16 / .32).
//
// A 0x100-byte fixed header ("Micro Focus COBOL Library File" padded with
// spaces, plus a creation date/time stamp in ASCII) followed by a SINGLY
// LINKED LIST of 18-byte directory records, each immediately followed by a
// Pascal (length-byte prefixed) member name.  The first record always sits at
// 0x100; every following record is reached through the 32-bit big-endian link
// in field +0x00 of the current one - and that link is NOT monotonic: real
// archives in the corpus walk backwards by as much as 10400 bytes, so the
// directory cannot be scanned linearly and a cursor-only walk needs an
// explicit iteration bound (the record count in the header).
//
// Members are STORED verbatim; there is no compression in this format at all.
// The payload offset is held as a count of 128-byte blocks (field +0x04 shifted
// left by 7), and the exact byte length is field +0x08.
class XLbrCobol final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;   // directory record (18 bytes + Pascal name)
        qint64 nRecordSize;     // 18 + 1 + name length
        qint64 nDataOffset;     // block number << 7
        qint64 nDataSize;       // exact byte count
        quint16 nDosDate;  // record +0x0e
        quint16 nDosTime;  // record +0x0c
        quint16 nFlags;         // record +0x10, seen as 0x0000 / 0x0001
        QString sFileName;
    };

    explicit XLbrCobol(QIODevice *pDevice = nullptr);
    ~XLbrCobol() override;

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
        qint64 nFirstMemberOffset;
        QString sCreated;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLBRCOBOL_H
