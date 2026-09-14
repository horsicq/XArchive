/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGENIUSLIBRARY_H
#define XGENIUSLIBRARY_H

#include "xarchive.h"

// "GENIUS LIBRARY" (".GPL"): the part-library container of the Genius CAD/CAM
// products.  Every archive holds one "_gpl.dir" index member followed by the
// AutoCAD DXF drawings it names.
//
//   file header, 0x20 bytes
//     +0x00  char[14]  "GENIUS LIBRARY"
//     +0x0E  quint8    0
//     +0x0F  quint8    0
//     +0x10  quint16   0 (44/44)
//     +0x12  2 bytes   uninitialised writer memory, 22 distinct values
//                      over 44 files - not a field, never published
//     +0x14  qint32    number of members, > 0
//     +0x18  quint32   archive id
//     +0x1C  quint32   0
//
//   member header, 0x36 bytes, then the name, then the data
//     +0x00  qint64    ABSOLUTE file offset of the data - it equals the offset
//                      the walk has already reached, so it is a cross-check
//                      rather than a seek target
//     +0x08  qint64    stored (compressed) size
//     +0x10  qint64    plaintext size
//     +0x18  qint64    creation FILETIME
//     +0x20  qint64    access FILETIME
//     +0x28  qint64    last-write FILETIME - the one the reference tool stamps
//                      the extracted file with
//     +0x30  quint16   0
//     +0x32  quint8    DOS/Win32 attribute bits (0x20 or 0xA0 here)
//     +0x33  quint8    METHOD: 0 = stored, 1 = block-framed PKWARE DCL Implode
//     +0x34  quint16   length of the name, 1..0x400
//
// A stored member is the plaintext verbatim, with no framing and no checksum -
// which is exactly what "_gpl.dir" is in all 44 reference files, and what the
// reference tool FAILS on: it runs its block walker over every member and
// writes _gpl.dir as a zero-byte file at exit 0.  A compressed member is the
// block chain XGeniusLibraryDecoder documents and decodes.
//
// The names are 8.3 and carry no path component, so they are published
// verbatim; "_gpl.dir" included, because it is a real member with real bytes.
class XGeniusLibrary : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint64 nWriteTime;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XGeniusLibrary(QIODevice *pDevice = nullptr);
    ~XGeniusLibrary() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QString sVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // A member whose method byte is not one this reader can reproduce is
    // published as HANDLE_METHOD_UNKNOWN rather than left unset, because an
    // unset method is read as HANDLE_METHOD_STORE and would hand the stored
    // bytes back as if they were the file.
    static HANDLE_METHOD memberHandleMethod(const MEMBER &member);
    static QString memberReportedMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XGENIUSLIBRARY_H
