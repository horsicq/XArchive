// U3 archive[390] RZIP adapter; see Algos/xu3rzipdecoder.PROVENANCE.md.
#ifndef XRZIPARCHIVE_H
#define XRZIPARCHIVE_H

#include "xarchive.h"
#include "Algos/xu3rzipdecoder.h"

class XRzipArchive final : public XArchive {
    Q_OBJECT
public:
    explicit XRzipArchive(QIODevice *device = nullptr);
    bool isValid(PDSTRUCT *progress = nullptr) override;
    static bool isValid(QIODevice *device, PDSTRUCT *progress = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getVersion() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *progress = nullptr) override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *device, bool image = false, XADDR address = -1) override;
    bool initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP,QVariant> &properties, PDSTRUCT *progress = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *state, PDSTRUCT *progress = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *state, QIODevice *device, PDSTRUCT *progress = nullptr) override;
    bool moveToNext(UNPACK_STATE *state, PDSTRUCT *progress = nullptr) override;
    bool finishUnpack(UNPACK_STATE *state, PDSTRUCT *progress = nullptr) override;
private:
    struct CONTEXT { XU3RzipDecoder::HEADER header; QString name; };
    bool readHeader(XU3RzipDecoder::HEADER *header, PDSTRUCT *progress);
};

#endif
