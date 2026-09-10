/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLEGACYDISKDECODER_H
#define XLEGACYDISKDECODER_H

#include <QByteArray>
#include <QList>
#include <QString>

class XLegacyDiskDecoder
{
public:
    // A container can hold more than one image: QRST version 5 carries a fixed
    // pair of image descriptors, so a decode result is a LIST, never a single
    // buffer.  A single-image driver (IMD, QRST version 1) returns one entry
    // with an empty name, which tells the reader to name the member after the
    // archive itself.
    struct IMAGE {
        QByteArray rawImage;
        QString name;

        IMAGE()
        {
        }
    };

    struct RESULT {
        QList<IMAGE> images;
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
