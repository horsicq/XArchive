/* StuffIt method-13 decoder adapter for Deark.
 *
 * The implementation is a C port of the MIT-licensed compcol clean-room
 * decoder. See COMPCOL_LICENSE in this directory.
 */
#ifndef XSTUFFIT13_H
#define XSTUFFIT13_H

#include <stddef.h>
#include <stdint.h>

/* Returns 0 on success. The caller supplies the exact output length from the
 * StuffIt member header; no bytes outside output[0..output_len) are written. */
int xstuffit13_decode(const uint8_t *input, size_t input_len,
    uint8_t *output, size_t output_len);

#endif
