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
#ifndef LZ5LIZARDDECLIB_H
#define LZ5LIZARDDECLIB_H

#include <stddef.h>

/* Frame-decoder surface of lz5lizarddeclib.cpp, the amalgamated LZ5 1.5.0 and
 * Lizard 2.1 sources.  Only what XLZ5Decoder and XLizardDecoder consume is
 * declared here; the amalgamation keeps the full upstream API internally.
 *
 * The definitions have C linkage, matching the upstream headers.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/* ---------------------------------------------------------------- LZ5 ---- */

#define LZ5F_VERSION 100

typedef size_t LZ5F_errorCode_t;
typedef struct LZ5F_dctx_s *LZ5F_decompressionContext_t; /* aligned on 8 bytes */

typedef struct {
    unsigned stableDst; /* decompressed data stays valid across calls */
    unsigned reserved[3];
} LZ5F_decompressOptions_t;

unsigned LZ5F_isError(LZ5F_errorCode_t code);
LZ5F_errorCode_t LZ5F_createDecompressionContext(LZ5F_decompressionContext_t *dctxPtr, unsigned version);
LZ5F_errorCode_t LZ5F_freeDecompressionContext(LZ5F_decompressionContext_t dctx);
size_t LZ5F_decompress(LZ5F_decompressionContext_t dctx, void *dstBuffer, size_t *dstSizePtr, const void *srcBuffer, size_t *srcSizePtr,
                       const LZ5F_decompressOptions_t *dOptPtr);

/* ------------------------------------------------------------- Lizard ---- */

#define LIZARDF_VERSION 100

typedef size_t LizardF_errorCode_t;
typedef struct LizardF_dctx_s *LizardF_decompressionContext_t; /* aligned on 8 bytes */

typedef struct {
    unsigned stableDst; /* decompressed data stays valid across calls */
    unsigned reserved[3];
} LizardF_decompressOptions_t;

unsigned LizardF_isError(LizardF_errorCode_t code);
LizardF_errorCode_t LizardF_createDecompressionContext(LizardF_decompressionContext_t *dctxPtr, unsigned version);
LizardF_errorCode_t LizardF_freeDecompressionContext(LizardF_decompressionContext_t dctx);
size_t LizardF_decompress(LizardF_decompressionContext_t dctx, void *dstBuffer, size_t *dstSizePtr, const void *srcBuffer, size_t *srcSizePtr,
                          const LizardF_decompressOptions_t *dOptPtr);

#if defined(__cplusplus)
}
#endif

#endif  // LZ5LIZARDDECLIB_H
