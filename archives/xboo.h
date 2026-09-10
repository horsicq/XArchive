/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBOO_H
#define XBOO_H

#include "xarchive.h"

// Ward Christensen BOO (MAKEBOO/DEBOO) printable transport encoding: one
// binary file armoured as 6-bit ASCII in 0x30..0x6f with '~' as a NUL-run
// escape, preceded by a bare name line.  The format carries no magic, no
// length field and no checksum, so recognition is purely structural and the
// decoded payload has to be materialized - it is not a byte range of the
// source device.
class XBOO final : public XArchive {
    Q_OBJECT

public:
    explicit XBOO(QIODevice *pDevice = nullptr);
    ~XBOO() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
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
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;

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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    // Exactly one member per container; the name line is its name.
    struct CONTEXT {
        qint64 nInputSize = 0;
        qint64 nBodyOffset = 0;
        qint64 nRawDecodedSize = 0;  // before the final-group NUL trim
        QString sFileName;
        QByteArray baDecoded;
    };

    struct UNPACK_CONTEXT {
        CONTEXT context;
    };

    bool readSource(QByteArray *pbaSource, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, qint64 nDecodedLimit,
                      PDSTRUCT *pPdStruct);

    static bool parseNameLine(const QByteArray &baSource, QString *psName,
                              qint64 *pnBodyOffset);
    static bool probeGate(const QByteArray &baSource, qint64 nBodyOffset);
    static bool decodeBody(const QByteArray &baSource, qint64 nBodyOffset,
                           qint64 nDecodedLimit, QByteArray *pbaDecoded,
                           qint64 *pnRawDecodedSize, PDSTRUCT *pPdStruct);
};

#endif  // XBOO_H
