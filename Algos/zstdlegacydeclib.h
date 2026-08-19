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
#ifndef ZSTDLEGACYDECLIB_H
#define ZSTDLEGACYDECLIB_H

#include <stddef.h>

/* Buffered-stream surface of the Zstandard v0.4-v0.7 legacy decoders, which
 * live in Algos/zstdlegacy_v04.cpp .. zstdlegacy_v07.cpp (each its own
 * translation unit, see the note in those files).
 *
 * Only what XZstdDecoder calls is declared. The upstream headers wrap their
 * declarations in extern "C", so these keep plain C symbol names.
 *
 * v0.1-v0.3 are intentionally absent: ZSTD_decompressStream() in 1.5.7 reports
 * those versions as unsupported, so XArchive does not claim them.
 */

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ZBUFFv04_DCtx_s ZBUFFv04_DCtx;
ZBUFFv04_DCtx *ZBUFFv04_createDCtx(void);
size_t ZBUFFv04_freeDCtx(ZBUFFv04_DCtx *dctx);
size_t ZBUFFv04_decompressInit(ZBUFFv04_DCtx *dctx);
size_t ZBUFFv04_decompressContinue(ZBUFFv04_DCtx *dctx, void *dst, size_t *maxDstSizePtr, const void *src, size_t *srcSizePtr);
unsigned ZBUFFv04_isError(size_t errorCode);

typedef struct ZBUFFv05_DCtx_s ZBUFFv05_DCtx;
ZBUFFv05_DCtx *ZBUFFv05_createDCtx(void);
size_t ZBUFFv05_freeDCtx(ZBUFFv05_DCtx *dctx);
size_t ZBUFFv05_decompressInit(ZBUFFv05_DCtx *dctx);
size_t ZBUFFv05_decompressContinue(ZBUFFv05_DCtx *dctx, void *dst, size_t *maxDstSizePtr, const void *src, size_t *srcSizePtr);
unsigned ZBUFFv05_isError(size_t errorCode);

typedef struct ZBUFFv06_DCtx_s ZBUFFv06_DCtx;
ZBUFFv06_DCtx *ZBUFFv06_createDCtx(void);
size_t ZBUFFv06_freeDCtx(ZBUFFv06_DCtx *dctx);
size_t ZBUFFv06_decompressInit(ZBUFFv06_DCtx *dctx);
size_t ZBUFFv06_decompressContinue(ZBUFFv06_DCtx *dctx, void *dst, size_t *maxDstSizePtr, const void *src, size_t *srcSizePtr);
unsigned ZBUFFv06_isError(size_t errorCode);

typedef struct ZBUFFv07_DCtx_s ZBUFFv07_DCtx;
ZBUFFv07_DCtx *ZBUFFv07_createDCtx(void);
size_t ZBUFFv07_freeDCtx(ZBUFFv07_DCtx *dctx);
size_t ZBUFFv07_decompressInit(ZBUFFv07_DCtx *dctx);
size_t ZBUFFv07_decompressContinue(ZBUFFv07_DCtx *dctx, void *dst, size_t *maxDstSizePtr, const void *src, size_t *srcSizePtr);
unsigned ZBUFFv07_isError(size_t errorCode);

#if defined(__cplusplus)
}
#endif

#endif  // ZSTDLEGACYDECLIB_H
