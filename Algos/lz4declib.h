/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef LZ4DECLIB_H
#define LZ4DECLIB_H

#include <stddef.h>

/* Decoder surface of lz4declib.cpp, the amalgamated LZ4 1.10.0 sources.
 * Only what XLZ4Decoder consumes is declared; the amalgamation keeps the full
 * upstream API internally.
 *
 * The upstream headers wrap their declarations in `extern "C"`, so the
 * definitions keep C language linkage and plain symbol names even though the
 * amalgamation compiles them inside a namespace.  These declarations therefore
 * live at global scope with matching linkage.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/* -- block API (lz4.h) -- */

#define LZ4_MAX_INPUT_SIZE 0x7E000000 /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize) ((unsigned)(isize) > (unsigned)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize) / 255) + 16)

int LZ4_decompress_safe(const char *src, char *dst, int compressedSize, int dstCapacity);

/* -- frame API (lz4frame.h) -- */

#define LZ4F_VERSION 100
#define LZ4F_MAGICNUMBER 0x184D2204U
#define LZ4F_MAGIC_SKIPPABLE_START 0x184D2A50U

typedef size_t LZ4F_errorCode_t;
typedef struct LZ4F_dctx_s LZ4F_dctx; /* incomplete type */

typedef struct {
    unsigned stableDst;     /* last 64KB of decompressed data sits right before dstBuffer */
    unsigned skipChecksums; /* skip checksum calculation and verification */
    unsigned reserved1;     /* must be zero */
    unsigned reserved0;     /* must be zero */
} LZ4F_decompressOptions_t;

unsigned LZ4F_isError(LZ4F_errorCode_t code);
LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_dctx **dctxPtr, unsigned version);
LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_dctx *dctx);
size_t LZ4F_decompress(LZ4F_dctx *dctx, void *dstBuffer, size_t *dstSizePtr, const void *srcBuffer, size_t *srcSizePtr,
                       const LZ4F_decompressOptions_t *dOptPtr);

#if defined(__cplusplus)
}
#endif

#endif  // LZ4DECLIB_H
