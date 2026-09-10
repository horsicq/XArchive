/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBINSHSFX_H
#define XBINSHSFX_H

#include "xarchive.h"

class SubDevice;

// Sun Microsystems /bin/sh self-extracting installer (Java Plug-in 1.1.2 for
// Solaris, Java Web Start install.sh).  The wrapper is an ordinary shell
// script whose payload is appended verbatim; the script carves it out with a
// historic BSD "tail +<N> $0" byte-offset directive, i.e. "start at line N".
// N is script-revision dependent (256, 415 and 427 all occur in the wild), so
// it must be parsed out of the script rather than assumed.  The carved bytes
// are a complete archive in their own right (Unix compress .Z wrapping a
// ustar tar, or a bare ustar tar), so this class authenticates the wrapper and
// then delegates the whole streaming session to the archive class that owns
// the payload format.
class XBinShSFX final : public XArchive {
    Q_OBJECT

public:
    explicit XBinShSFX(QIODevice *pDevice = nullptr);
    ~XBinShSFX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    // Detection of the payload runs XFormats over a view of this very file.
    // The property marks that view so the outer class can never claim it back.
    static const char *recursionPropertyName();
    static bool isRecursionSuppressed(QIODevice *pDevice);

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    OSNAME getOsName() override;
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
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    enum PAYLOAD_KIND {
        PAYLOAD_KIND_UNKNOWN = 0,
        PAYLOAD_KIND_COMPRESS,  // 1F 9D, a Unix compress stream
        PAYLOAD_KIND_TAR        // a POSIX ustar header
    };

    struct CARVE {
        qint64 nInputSize;
        qint64 nCarveOffset;
        qint64 nPayloadSize;
        qint64 nDirectiveOffset;
        qint32 nLineNumber;
        PAYLOAD_KIND payloadKind;
    };

    struct CONTEXT {
        SubDevice *pPayloadDevice;
        XArchive *pInnerArchive;
        UNPACK_STATE innerState;
        CARVE carve;
        FT innerFileType;
        bool bInnerInitialized;

        CONTEXT();
        ~CONTEXT();
        bool finishInner(PDSTRUCT *pPdStruct);
    };

    bool parseCarve(CARVE *pCarve, PDSTRUCT *pPdStruct);
    static QString payloadName(const CARVE &carve);
    static QString payloadDescription(const CARVE &carve);
    static FT detectPayloadFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct);
    static XArchive *createPayloadArchive(FT fileType, QIODevice *pDevice);
    static bool publicStateMatchesInner(const UNPACK_STATE *pState,
                                        const CONTEXT *pContext);
    static void copyInnerState(UNPACK_STATE *pState, const CONTEXT *pContext);
    static bool failUnpackInitialization(XBinShSFX *pArchive,
                                         UNPACK_STATE *pState);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBINSHSFX_H
