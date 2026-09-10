// The reference implementation CHM adapter.
#ifndef XCHMARCHIVE_H
#define XCHMARCHIVE_H
#include "xarchive.h"

class XChmArchive final : public XArchive {
    Q_OBJECT
public:
    explicit XChmArchive(QIODevice *device = nullptr);
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
    struct MEMBER { QString name; qint32 section = 0; qint64 offset = 0, size = 0; bool folder = false; };
    struct SECTION {
        bool lzx = false;
        qint64 offset = 0, size = 0, rawSize = 0;
        qint32 windowBits = 0, resetFrames = 0;
        QList<qint64> frames;
    };
    struct CONTEXT { QByteArray input; qint32 version = 0; QList<MEMBER> members; QList<SECTION> sections; };
    bool readContext(CONTEXT *context, PDSTRUCT *progress);
    static bool parse(const QByteArray &input, CONTEXT *context, PDSTRUCT *progress);
};
#endif
