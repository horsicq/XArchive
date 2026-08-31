/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLEGACYDISKDECODER_H
#define XLEGACYDISKDECODER_H

#include <QByteArray>
#include <QString>

class XLegacyDiskDecoder
{
public:
    struct RESULT {
        QByteArray rawImage;
        QString driver;
        qint32 cylinders = 0;
        qint32 heads = 0;
        qint32 sectorsPerTrack = 0;
        qint32 sectorSize = 0;
        qint64 recoveredSectors = 0;
    };

    static QString identify(const QByteArray &data);
    static bool decode(const QByteArray &data, qint64 maxOutputSize,
                       RESULT *result, QString *error = nullptr);
};

#endif  // XLEGACYDISKDECODER_H
