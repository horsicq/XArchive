/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGSTPACK_H
#define XGSTPACK_H

#include "xarchive.h"

// GST Software installation archive (GST PressWorks / Timeworks Publisher /
// GST "Publish It!" distribution disks, 1993-1996).
//
// The container has no whole-file header at all: the file IS a chain of
// 32-byte member headers, each immediately followed by that member's
// payload.  A member header is
//
//   +0x00  u8[2]   magic 0xE9 0xC8
//   +0x02  u16     DOS time
//   +0x04  u16     DOS date
//   +0x06  u8      DOS file attributes (0x00 / 0x01 / 0x20 in the corpus)
//   +0x07  u8      method: 0 = stored, 1 = PKWARE DCL Implode
//   +0x08  u32     uncompressed size
//   +0x0C  u32     compressed size (payload length, header excluded)
//   +0x10  char[12] NUL-terminated 8.3 name; bytes after the NUL are stale
//   +0x1C  u32     CRC-32 (EDB88320, standard init/final) of the UNPACKED data
//
// Note the split of the 16-bit word at +0x06: reading it as one little-endian
// "method" field yields 0x0000/0x0100/0x0101/0x0120, which looks like four
// methods.  It is really attribute+method, and only two methods exist.
// Stored members always have compressed size == uncompressed size; compressed
// members carry a complete PKWARE DCL Implode stream that begins with its own
// two-byte prelude (literal mode 0, dictionary bits 6 on every known sample).
//
// Because there is no file-level signature, isValid() has to earn its keep:
// it walks the whole chain, requires it to land exactly on EOF, validates
// every header field of every member, and then bounded-trial-decodes the
// first compressed member and checks both its length and its CRC-32 against
// the header.  Verified on all 105 corpus archives (525 members).
class XGstPack final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC32;
        quint16 nDosTime;
        quint16 nDosDate;
        quint8 nAttributes;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XGstPack(QIODevice *pDevice = nullptr);
    ~XGstPack() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bDeepCheck, PDSTRUCT *pPdStruct);
    bool trialDecode(const CONTEXT &context, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XGSTPACK_H
