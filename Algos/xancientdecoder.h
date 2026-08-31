/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XANCIENTDECODER_H
#define XANCIENTDECODER_H

#include <QByteArray>
#include <QString>

class XAncientDecoder
{
public:
    enum TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_DMS,
        TYPE_POWERPACKER,
        TYPE_RNC,
        TYPE_TPWM,
        TYPE_FREEZE,
        TYPE_UNIX_PACK
    };

    enum DECODE_ERROR {
        ERROR_NONE = 0,
        ERROR_INVALID_FORMAT,
        ERROR_DECOMPRESSION,
        ERROR_VERIFICATION,
        ERROR_MEMORY
    };

    struct INFO {
        QString method;
        qint64 packedSize = -1;
        qint64 rawSize = -1;
        qint64 imageSize = -1;
        qint64 imageOffset = 0;
    };

    static constexpr qint64 MAX_PACKED_SIZE = Q_INT64_C(128) * 1024 * 1024;
    static constexpr qint64 MAX_RAW_SIZE = Q_INT64_C(128) * 1024 * 1024;

    static TYPE identify(const QByteArray &data);
    static bool describe(const QByteArray &data, TYPE type, INFO *info);
    static bool decode(const QByteArray &data, TYPE type, QByteArray *raw,
                        INFO *info = nullptr, DECODE_ERROR *error = nullptr,
                       bool verify = true);
};

#endif  // XANCIENTDECODER_H
