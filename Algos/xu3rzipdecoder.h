// Functional translation of U3 archive[390]; see xu3rzipdecoder.PROVENANCE.md.
#ifndef XU3RZIPDECODER_H
#define XU3RZIPDECODER_H

#include "xbinary.h"

class XU3RzipDecoder final {
public:
    static constexpr qint64 MaxInput = Q_INT64_C(256) * 1024 * 1024;
    static constexpr qint64 MaxOutput = Q_INT64_C(1024) * 1024 * 1024;
    static constexpr qint64 MaxBlock = Q_INT64_C(16) * 1024 * 1024;
    struct HEADER { quint8 major = 0, minor = 0; qint64 rawSize = 0; };
    struct RESULT { qint64 consumed = 0; qint64 outputSize = 0; qint32 chunks = 0; qint32 blocks = 0; };
    static bool parseHeader(const QByteArray &data, HEADER *header);
    // The writer owns a fresh, readable/writable, seekable private staging device.
    // All produced bytes pass through _writeDevice exactly once. The caller only
    // publishes staging after successful CRC, declared-size and full-EOF checks.
    static bool decode(const QByteArray &data, XBinary::DATAPROCESS_STATE *writer, RESULT *result,
                       XBinary::PDSTRUCT *progress = nullptr);
};

#endif
