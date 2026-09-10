/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XASCENDBACKUP_H
#define XASCENDBACKUP_H

#include "xarchive.h"

// Ascend 4.2b (Franklin Quest / Franklin Covey "Ascend" for Windows) PIM backup
// volume, written as TUTBCK.000 / <set>.000 and continued in .001, .002 ...
//
// The container has NO magic, no global header, no directory and no terminator:
// it is a bare chain of "uint16le nameLen | char name[nameLen] | uint32le
// packedSize | PKWARE DCL stream" records that must tile the file exactly.  The
// only fixed bytes anywhere are the two-byte DCL prelude at the front of each
// payload, and those sit at a variable offset (0x11 in the reference volume), so
// no byte signature can be written for this format - detection is structural and
// getSearchSignatures() is deliberately NOT overridden.
//
// The uncompressed size of a member is stored nowhere; it only falls out of the
// DCL end-of-stream code.  parseContext() therefore takes a bScanSizes flag:
// probing walks headers only, and the sizes are recovered via XDclDecoder::scan()
// exclusively on the paths that actually need them.
class XAscendBackup final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        // False until XDclDecoder::scan() has recovered the plaintext length.
        // A member whose size is unknown must be reported as HANDLE_METHOD_UNKNOWN:
        // decPkwareDcl() takes the output length as an input, so handing it a zero
        // would silently truncate the member instead of failing.
        bool bUncompressedSizeKnown;
        QString sFileName;
    };

    explicit XAscendBackup(QIODevice *pDevice = nullptr);
    ~XAscendBackup() override;

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
        qint64 nFirstMemberOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bScanSizes, PDSTRUCT *pPdStruct);
    bool scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XASCENDBACKUP_H
