// Reconstructed from U3.unp.exe; see xu3cpmdecoder.PROVENANCE.md.
#ifndef XU3CPMDECODER_H
#define XU3CPMDECODER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class XU3CpmDecoder {
public:
    enum class Format { Unknown, Crunch, Crlzh, UnixCompact };
    struct Header {
        Format format = Format::Unknown;
        std::string name;
        unsigned revision = 0;
        unsigned checksumType = 0;
        std::size_t dataOffset = 0;
    };
    struct Result {
        Header header;
        std::vector<std::uint8_t> data;
        std::size_t uncompressedSize = 0;
        std::size_t consumed = 0;
        std::uint16_t checksum = 0;
    };
    using Cancel = bool (*)(void *);
    static constexpr std::size_t MaxInput = 64 * 1024 * 1024;
    static constexpr std::size_t MaxOutput = 128 * 1024 * 1024;
    static bool parseHeader(const std::uint8_t *data, std::size_t size, Header *header);
    static bool decode(const std::uint8_t *data, std::size_t size, std::size_t outputLimit,
                       Result *result, Cancel canceled = nullptr, void *context = nullptr, bool measureOnly = false);
};

#endif
