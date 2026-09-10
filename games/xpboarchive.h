// The reference implementation PBO.
#ifndef XPBOARCHIVE_H
#define XPBOARCHIVE_H
#include "xarchive.h"

class XPboArchive final : public XArchive {
    Q_OBJECT
public:
    explicit XPboArchive(QIODevice *device = nullptr);
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
    struct MEMBER { QString name; qint64 offset = 0, size = 0, packedSize = 0; quint32 timestamp = 0; bool compressed = false; };
    struct CONTEXT { QByteArray input; QList<MEMBER> members; bool properties = false, sha1 = false; };
    bool readContext(CONTEXT *context, PDSTRUCT *progress);
    static bool parse(const QByteArray &input, CONTEXT *context, PDSTRUCT *progress);
};
#endif
