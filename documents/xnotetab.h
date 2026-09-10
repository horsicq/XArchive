/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNOTETAB_H
#define XNOTETAB_H

#include "xarchive.h"

// NoteTab (Fookes Software) Clipbook Library / Outline / Clip-Help document
// (.clb / .otl / .clh).  A plain-text container: line 1 is the document
// declaration ("= V4 " / "= V5 " plus type keywords), line 2 is reserved, and
// the body is a run of clips, each opened by an H="name" line and running to
// the next H=" line or to end of file.  Nothing is compressed.
class XNoteTab : public XArchive {
    Q_OBJECT

public:
    struct CLIP {
        qint64 nNameOffset;  // offset of the clip name text (after the H=" tag)
        qint32 nNameSize;    // clipped to NOTETAB_MAX_NAME, exactly as the original tool does
        qint64 nBodyOffset;  // first body line
        qint64 nBodySize;    // up to (but not including) the next H=" line, or EOF
        QByteArray baPrefix;  // name + CRLF CRLF, prepended to the body on extraction
        QString sFileName;    // sanitised name + ".txt"
    };

    explicit XNoteTab(QIODevice *pDevice = nullptr);
    ~XNoteTab() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    bool isSigned() override;
    bool isEncrypted() override;
    OSNAME getOsName() override;
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
        qint64 nHeaderSize;  // the two declaration lines
        QString sDeclaration;
        QList<CLIP> listClips;
    };

    enum LINERESULT {
        LINERESULT_ERROR = 0,
        LINERESULT_OK,
        LINERESULT_END
    };

    LINERESULT _readLine(qint64 *pnPosition, qint64 nInputSize, qint64 *pnLineOffset, qint64 *pnLineSize, PDSTRUCT *pPdStruct);
    bool _isHeadingLine(qint64 nLineOffset, qint64 nLineSize, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool checkDeclaration(QString *psDeclaration, PDSTRUCT *pPdStruct);
    static QString sanitizeName(const QByteArray &baName);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XNOTETAB_H
