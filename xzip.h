#ifndef XZIP_H
#define XZIP_H

#include <QObject>
#include "xarchive.h"

class XZip : public XArchive
{
    const quint32 ECD=0x06054B50;
    const quint32 CFD=0x02014b50;
    const quint32 LFD=0x04034b50;
    Q_OBJECT
public:
    explicit XZip(QIODevice *__pDevice);
    virtual bool isVaild();
    virtual quint64 getNumberOfRecords();
    virtual QList<RECORD> getRecords();
private:
    qint64 findECDOffset();
signals:

public slots:
};

#endif // XZIP_H
