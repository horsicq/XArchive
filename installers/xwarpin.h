/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XWARPIN_H
#define XWARPIN_H

#include "xarchive.h"

// WarpIN package (".wpi"), the installer archive of the OS/2 - eComStation -
// ArcaOS "WarpIN" installer.  The container is a plain archive at file offset
// 0; the executable "WarpIN self-installer" variants are a different animal and
// are NOT claimed here.
//
// Fixed 0x214 header:
//   0x0000  quint32 0xBE020477      magic
//   0x0004  quint16 nVersion        3 in every sample; the tool refuses >= 5
//   0x0006..0x0209                  reserved, zero in every sample
//   0x020a  quint16 nPackages       number of package-table entries
//   0x020c  quint16 nScriptSize     UNCOMPRESSED size of the install script
//   0x020e  quint16 nScriptPacked   COMPRESSED size of the install script
//   0x0210  qint32  nOptionalSize   optional blob between script and table
//                                   (0 in every sample of the reach set)
//
// The body then runs:
//   0x0214                          the install script, one bzip2 stream
//                                   ("BZh1") of nScriptPacked bytes that
//                                   expands to nScriptSize bytes of WarpIN
//                                   markup (<WARPIN> ... <PCK> ...)
//   +nOptionalSize                  the optional blob, when present
//   package table                   nPackages entries of 0x30 bytes:
//       0x00  quint16 nId           1-based package id a member refers to
//       0x02  quint16 nFiles        member count of this package
//       0x04  qint32  nDataOffset   file offset of this package's first member
//       0x08  qint32  nTotalUnpacked
//       0x0c  qint32  nTotalPacked
//       0x10  char    szLabel[0x20] "Pck001", "Pck011", ...
//   member chain                    exactly sum(nFiles) records, contiguous
//
// One member record is a 0x11d header immediately followed by its data:
//   0x0000  quint16 0xF012          record magic
//   0x0004  quint16 nMethod         0 = stored, 1 = bzip2
//   0x0006  quint16 nPackageId      owning package
//   0x0008  qint32  nUnpackedSize
//   0x000c  qint32  nPackedSize
//   0x0014  char    szName[0x100]   NUL padded, '\\' path separators
//   0x0114  quint32 nMTime          Unix time_t
//   0x011c  quint8  0               mandatory terminator
//
// Method 1 is an ordinary bzip2 stream, so extraction rides the existing
// HANDLE_METHOD_BZIP2, and method 0 rides HANDLE_METHOD_STORE.  No new codec.
//
// EMPTY MEMBERS.  A zero-byte file is written with nMethod == 1 but with BOTH
// sizes 0, i.e. there is no bzip2 stream at all - not even the 4-byte "BZh9"
// header.  Dispatching such a record to the bzip2 decoder makes
// XBZIP2Decoder::decompress() return BZ_UNEXPECTED_EOF, which is the right
// answer for an empty input and the wrong answer for the member: the member is
// simply empty.  Those records therefore take HANDLE_METHOD_STORE, the same
// rule xdecompress.cpp already applies to ARJ's zero-length records.  Three of
// the 748 members in the reach set are of this kind
// (help\en\shared.db, shared.ht and shared.key in the OpenOffice langpack).
class XWarpIn final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUnpackedSize;
        quint16 nMethod;
        quint16 nPackageId;
        quint32 nMTime;
        QString sFileName;
        QString sPackage;
    };

    struct PACKAGE {
        quint16 nId;
        quint16 nFiles;
        qint64 nDataOffset;
        qint64 nTotalUnpacked;
        qint64 nTotalPacked;
        QString sLabel;
    };

    explicit XWarpIn(QIODevice *pDevice = nullptr);
    ~XWarpIn() override;

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

    QList<FPART_PROP> getAvailableFPARTProperties() override;
    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nScriptOffset;
        qint64 nScriptPacked;
        qint64 nScriptUnpacked;
        qint64 nPackageTableOffset;
        qint64 nMemberOffset;
        quint16 nVersion;
        QList<PACKAGE> listPackages;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readMember(qint64 nOffset, const CONTEXT *pContext, MEMBER *pMember, PDSTRUCT *pPdStruct);
    static void fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties);
    static HANDLE_METHOD memberHandleMethod(const MEMBER &member);
    static QString memberMethodName(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString sanitizeName(const QByteArray &baRaw);
    static QString labelString(const QByteArray &baRaw);
};

#endif  // XWARPIN_H
