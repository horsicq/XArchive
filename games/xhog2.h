// The reference implementation HOG2.
#ifndef XHOG2_H
#define XHOG2_H
#include "xgamestorearchive_p.h"

// Descent 3 HOG2 has a table and contiguous STORE payloads. It is distinct
// from the Descent 1/2 DHF format implemented by XHOG.
class XHOG2 final : public XGameStoreArchiveBase {
    Q_OBJECT
public:
    explicit XHOG2(QIODevice *device = nullptr);
    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *device, PDSTRUCT *progress = nullptr);
    XBinary *createInstance(QIODevice *device, bool image = false, XADDR address = -1) override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
private:
    bool scanFormat(QList<ENTRY> *entries, qint64 *archiveEnd, PDSTRUCT *progress) override;
};
#endif
