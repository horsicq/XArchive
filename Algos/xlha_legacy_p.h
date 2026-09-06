/* Private adapter for Lhasa's ISC-licensed decoder implementations.
 * See xlha_legacy.PROVENANCE.md and the notices in each source file. */
#ifndef XLHA_LEGACY_P_H
#define XLHA_LEGACY_P_H

#include <stddef.h>
#include <stdint.h>

/* The six decoder translation units are C++ (see xlha_legacy.PROVENANCE.md), so
 * no extern "C" wrapper here: it would give the descriptor tables and the
 * callback types C language linkage while the static decoder functions stored in
 * them keep C++ linkage. */

typedef size_t (*XLhaLegacyDecoderCallback)(void *buffer, size_t length, void *context);

typedef struct {
    int (*init)(void *context, XLhaLegacyDecoderCallback callback, void *callback_context);
    void (*free)(void *context);
    size_t (*read)(void *context, uint8_t *buffer);
    size_t extra_size;
    size_t max_read;
    size_t block_size;
} XLhaLegacyDecoderType;

extern const XLhaLegacyDecoderType xlha_legacy_lzs_decoder;
extern const XLhaLegacyDecoderType xlha_legacy_lz5_decoder;
extern const XLhaLegacyDecoderType xlha_legacy_lhx_decoder;
extern const XLhaLegacyDecoderType xlha_legacy_lk7_decoder;
extern const XLhaLegacyDecoderType xlha_legacy_pm1_decoder;
extern const XLhaLegacyDecoderType xlha_legacy_pm2_decoder;

#endif
