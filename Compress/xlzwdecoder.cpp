/* Copyright (c) 2017-2024 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xlzwdecoder.h"
#include <string.h>

// typedef struct{
//     int prefix; // prefix for byte > 255
//     int character; // the last byte of the string
// } DictElement;

// struct STATE_LZW {
//     int leftover;
//     int leftoverBits;
//     DictElement dictionaryArray[4095];
//     XBinary::DECOMPRESS_STATE *pDecompressState;
//     XBinary::PDSTRUCT *pPdStruct;
// };

// int X_fputc(int _char, STATE_LZW *pState) {
//     quint8 nChar = (quint8)_char;
//     pState->pDecompressState->nCountOutput++;
//     return pState->pDecompressState->pDeviceOutput->write((char *)&nChar, 1);
// }

// int X_fgets(STATE_LZW *pState) {
//     quint8 nChar = 0;
//     if (pState->pDecompressState->nCountInput < pState->pDecompressState->nInputLimit) {
//         pState->pDecompressState->pDeviceInput->read((char *)&nChar, 1);
//     }

//     pState->pDecompressState->nCountInput++;

//     return nChar;
// }

// enum {
//     dictionarySize = 4095, // maximum number of entries defined for the dictionary (2^12 = 4096)
//     codeLength = 12, // the codes which are taking place of the substrings
//     maxValue = dictionarySize - 1
// };

// // add prefix + character to the dictionary
// void dictionaryArrayAdd(STATE_LZW *pState, int prefix, int character, int value) {
//     pState->dictionaryArray[value].prefix = prefix;
//     pState->dictionaryArray[value].character = character;
// }

// int dictionaryArrayPrefix(STATE_LZW *pState, int value) {
//     return pState->dictionaryArray[value].prefix;
// }

// int dictionaryArrayCharacter(STATE_LZW *pState, int value) {
//     return pState->dictionaryArray[value].character;
// }

// void writeBinary(STATE_LZW *pState, int code) {
//     if (pState->leftover > 0) {
//         int previousCode = (pState->leftoverBits << 4) + (code >> 8);

//         X_fputc(previousCode, pState); // write previous code
//         X_fputc(code, pState);

//         pState->leftover = 0; // no leftover now
//     } else {
//         pState->leftoverBits = code & 0xF; // save leftover, the last 00001111
//         pState->leftover = 1;

//         X_fputc(code >> 4, pState);
//     }
// }

// int readBinary(STATE_LZW *pState) {
//     int code = X_fgets(pState);

//     // TODO read error

//     if (pState->leftover > 0) {
//         code = (pState->leftoverBits << 8) + code;

//         pState->leftover = 0;
//     } else {
//         int nextCode = X_fgets(pState);

//         pState->leftoverBits = nextCode & 0xF; // save leftover, the last 00001111
//         pState->leftover = 1;

//         code = (code << 4) + (nextCode >> 4);
//     }

//     return code;
// }

// int decode(int code, STATE_LZW *pState) {
//     int character; int temp;

//     if (code > 255) { // decode
//         character = dictionaryArrayCharacter(pState, code);
//         temp = decode(dictionaryArrayPrefix(pState, code), pState); // recursion
//     } else {
//         character = code; // ASCII
//         temp = code;
//     }
//     X_fputc(character, pState);
//     //printf("%c", character);
//     //appendCharacter(character);
//     return temp;
// }

// void _decompress(STATE_LZW *pState) {
//     // int prevcode, currcode
//     int previousCode;
//     int currentCode;
//     int nextCode = 256; // start with the same dictionary of 256 characters

//     int firstChar;

//     // prevcode = read in a code
//     previousCode = readBinary(pState);
//     if (previousCode == 0) {
//         return;
//     }

//     X_fputc(previousCode, pState);

//     // while (there is still data to read)
//     while ((currentCode = readBinary(pState)) > 0) { // currcode = read in a code

//         if (currentCode >= nextCode) {
//             X_fputc(firstChar = decode(previousCode, pState), pState); // S+C+S+C+S exception [2.]
//             //printf("%c", firstChar);
//             //appendCharacter(firstChar = decode(previousCode, outputFile));
//         } else firstChar = decode(currentCode, pState); // first character returned! [1.]

//         // add a new code to the string table
//         if (nextCode < dictionarySize) dictionaryArrayAdd(pState, previousCode, firstChar, nextCode++);

//         // prevcode = currcode
//         previousCode = currentCode;
//     }
//     //printf("\n");
// }

// A simple LZW decoder (9..12-bit, MSB-first) with Clear (256) and EOI (257) codes
// Suitable for common LZW streams (e.g., TIFF/PDF without predictors).

namespace {
    typedef quint16 LZW_CODE;

    const qint32 LZW_INBUF_SIZE = 1024;
    const qint32 LZW_OUTBUF_SIZE = 4096;

    const qint32 LZW_MIN_BITS = 9;
    const qint32 LZW_MAX_BITS = 12;
    const qint32 LZW_MAX_CODES = (1 << LZW_MAX_BITS); // 4096

    const LZW_CODE LZW_CLEAR = 256;
    const LZW_CODE LZW_EOI = 257;
    const LZW_CODE LZW_FIRST_FREE = 258;

    const LZW_CODE LZW_INVALID = 0xFFFF;

    struct dictionary_entry {
        LZW_CODE parent; // previous code in the string (or LZW_INVALID)
        quint8 value;    // last byte of the string for this code
    };

    struct lzw_ctx {
        // User data
        XBinary::DECOMPRESS_STATE *state;
        qint64 cmpr_size;   // compressed bytes limit
        qint64 cmpr_used;   // consumed compressed bytes
        qint64 uncmpr_goal; // optional uncompressed size (0 if unknown)

        // Bit reader (MSB-first)
        quint32 bitbuf;
        int bitcnt; // number of valid bits in bitbuf
        int code_bits;

        // Dictionary
        dictionary_entry dict[LZW_MAX_CODES];
        LZW_CODE next_code;

        // State for string expansion
        bool have_old;
        LZW_CODE old_code;
        quint8 last_value; // first character of last expanded string

        // I/O buffers
        quint8 inbuf[LZW_INBUF_SIZE];
        qint32 inbuf_total;
        qint32 inbuf_pos;

        quint8 outbuf[LZW_OUTBUF_SIZE];
        qint32 outbuf_used;
    };

    static void lzw_reset_dict(lzw_ctx *ctx)
    {
        for (int i = 0; i < 256; ++i) {
            ctx->dict[i].parent = LZW_INVALID;
            ctx->dict[i].value = (quint8)i;
        }
        for (int i = 256; i < LZW_MAX_CODES; ++i) {
            ctx->dict[i].parent = LZW_INVALID;
            ctx->dict[i].value = 0;
        }
        ctx->next_code = LZW_FIRST_FREE;
        ctx->code_bits = LZW_MIN_BITS;
        ctx->have_old = false;
        ctx->bitbuf = 0;
        ctx->bitcnt = 0;
    }

    static qint32 lzw_cb_read(lzw_ctx *ctx, quint8 *buf, qint32 size)
    {
        return XBinary::_readDevice((char *)buf, size, ctx->state);
    }

    static void lzw_refill_inbuf(lzw_ctx *ctx)
    {
        ctx->inbuf_pos = 0;
        ctx->inbuf_total = 0;

        qint64 remain = ctx->cmpr_size - ctx->cmpr_used;
        if (remain <= 0) return;

        qint32 to_read = (qint32)qMin<qint64>(remain, LZW_INBUF_SIZE);
        qint32 got = lzw_cb_read(ctx, ctx->inbuf, to_read);
        if (got > 0) {
            ctx->inbuf_total = got;
            ctx->cmpr_used += got;
        }
    }

    static bool lzw_get_byte(lzw_ctx *ctx, quint8 *out)
    {
        if (ctx->inbuf_pos >= ctx->inbuf_total) {
            lzw_refill_inbuf(ctx);
            if (ctx->inbuf_total == 0) return false;
        }
        *out = ctx->inbuf[ctx->inbuf_pos++];
        return true;
    }

    // MSB-first code reader
    static bool lzw_get_code(lzw_ctx *ctx, LZW_CODE *pcode)
    {
        while (ctx->bitcnt < ctx->code_bits) {
            quint8 b = 0;
            if (!lzw_get_byte(ctx, &b)) {
                return false;
            }
            ctx->bitbuf = (ctx->bitbuf << 8) | b;
            ctx->bitcnt += 8;
        }

        int shift = ctx->bitcnt - ctx->code_bits;
        quint32 mask = (1u << ctx->code_bits) - 1u;
        quint32 code = (ctx->bitbuf >> shift) & mask;
        ctx->bitcnt -= ctx->code_bits;
        if (ctx->bitcnt) {
            ctx->bitbuf &= ((1u << ctx->bitcnt) - 1u);
        } else {
            ctx->bitbuf = 0;
        }

        *pcode = (LZW_CODE)code;
        return true;
    }

    static bool lzw_flush_out(lzw_ctx *ctx)
    {
        if (ctx->outbuf_used <= 0) return true;
        qint32 written = XBinary::_writeDevice((char *)ctx->outbuf, ctx->outbuf_used, ctx->state);
        ctx->outbuf_used = 0;
        return (written > 0) && (!ctx->state->bWriteError);
    }

    static bool lzw_write(lzw_ctx *ctx, const quint8 *buf, qint32 n)
    {
        if (n <= 0) return true;
        if (n > LZW_OUTBUF_SIZE) {
            // large chunk: flush existing and write directly in pieces
            if (!lzw_flush_out(ctx)) return false;
            qint32 offset = 0;
            while (offset < n) {
                qint32 chunk = qMin(LZW_OUTBUF_SIZE, n - offset);
                qint32 written = XBinary::_writeDevice((char *)(buf + offset), chunk, ctx->state);
                if (written != chunk || ctx->state->bWriteError) return false;
                offset += chunk;
            }
            return true;
        }

        if (ctx->outbuf_used + n > LZW_OUTBUF_SIZE) {
            if (!lzw_flush_out(ctx)) return false;
        }
        memcpy(ctx->outbuf + ctx->outbuf_used, buf, (size_t)n);
        ctx->outbuf_used += n;
        return true;
    }

    // Expand a code into a temporary buffer (reversed), return length and first byte
    static bool lzw_expand_code(lzw_ctx *ctx, LZW_CODE code, quint8 *tmp, qint32 tmpSize, qint32 *outLen, quint8 *pFirst)
    {
        qint32 pos = tmpSize;

        if (code >= LZW_MAX_CODES) return false;

        while (code != LZW_INVALID && pos > 0) {
            tmp[--pos] = ctx->dict[code].value;
            if (ctx->dict[code].parent == LZW_INVALID) break;
            code = ctx->dict[code].parent;
        }

        if (pos == tmpSize) return false;
        *pFirst = tmp[pos];
        *outLen = (tmpSize - pos);
        return true;
    }
}

XLZWDecoder::XLZWDecoder(QObject *parent) : QObject(parent) {}

bool XLZWDecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (pDecompressState->nInputOffset > 0) {
        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    }
    if (pDecompressState->pDeviceOutput) {
        pDecompressState->pDeviceOutput->seek(0);
    }

    // STATE_LZW state = {};

    // state.pDecompressState = pDecompressState;
    // state.pPdStruct = pPdStruct;

    // _decompress(&state);

    return false;
}

bool XLZWDecoder::decompress2(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (pDecompressState->nInputOffset > 0) {
        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    }
    if (pDecompressState->pDeviceOutput) {
        pDecompressState->pDeviceOutput->seek(0);
    }

    lzw_ctx ctx = {};
    ctx.state = pDecompressState;
    ctx.cmpr_size = pDecompressState->nInputLimit;
    ctx.cmpr_used = 0;
    ctx.uncmpr_goal = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    ctx.outbuf_used = 0;
    ctx.inbuf_total = 0;
    ctx.inbuf_pos = 0;

    lzw_reset_dict(&ctx);

    // Read first meaningful code (skip leading CLEAR if present)
    LZW_CODE code = 0;
    if (!lzw_get_code(&ctx, &code)) {
        // No data
        lzw_flush_out(&ctx);
        return !ctx.state->bReadError && !ctx.state->bWriteError;
    }

    if (code == LZW_CLEAR) {
        lzw_reset_dict(&ctx);
        if (!lzw_get_code(&ctx, &code)) {
            lzw_flush_out(&ctx);
            return !ctx.state->bReadError && !ctx.state->bWriteError;
        }
    }

    if (code == LZW_EOI) {
        lzw_flush_out(&ctx);
        return true;
    }

    // First code: should be a literal
    if (code >= 256) {
        // Invalid start; try to continue conservatively
        code = code & 0xFF;
    }

    quint8 first = (quint8)code;
    if (!lzw_write(&ctx, &first, 1)) {
        lzw_flush_out(&ctx);
        return false;
    }
    ctx.have_old = true;
    ctx.old_code = code;
    ctx.last_value = (quint8)code;

    quint8 tmpbuf[8192]; // Enough for worst-case string

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (ctx.uncmpr_goal > 0 && (pDecompressState->nCountOutput >= ctx.uncmpr_goal)) {
            break; // reached goal
        }

        LZW_CODE inCode = 0;
        if (!lzw_get_code(&ctx, &inCode)) {
            break; // no more data
        }

        if (inCode == LZW_CLEAR) {
            lzw_reset_dict(&ctx);
            // Next code after clear should be a literal
            if (!lzw_get_code(&ctx, &inCode)) break;
            if (inCode == LZW_EOI) break;
            if (inCode >= 256) inCode &= 0xFF;
            quint8 b = (quint8)inCode;
            if (!lzw_write(&ctx, &b, 1)) break;
            ctx.old_code = inCode;
            ctx.last_value = b;
            ctx.have_old = true;
            continue;
        }

        if (inCode == LZW_EOI) {
            break;
        }

        // Determine the sequence for inCode
        qint32 outLen = 0;
        quint8 firstByte = 0;
        bool known = (inCode < ctx.next_code) && (ctx.dict[inCode].parent != LZW_INVALID || inCode < 256);
        if (known) {
            if (!lzw_expand_code(&ctx, inCode, tmpbuf, (qint32)sizeof(tmpbuf), &outLen, &firstByte)) break;
        } else {
            // KwKwK case: use old string + last_value
            if (!lzw_expand_code(&ctx, ctx.old_code, tmpbuf, (qint32)sizeof(tmpbuf), &outLen, &firstByte)) break;
            // append last_value
            if (outLen >= (qint32)sizeof(tmpbuf)) break;
            tmpbuf[outLen] = ctx.last_value;
            firstByte = tmpbuf[0];
            outLen += 1;
        }

        // Output expanded sequence
        if (!lzw_write(&ctx, tmpbuf, outLen)) break;

        // Add new dictionary entry old_code + firstByte
        if (ctx.next_code < LZW_MAX_CODES) {
            ctx.dict[ctx.next_code].parent = ctx.old_code;
            ctx.dict[ctx.next_code].value = firstByte;
            ctx.next_code++;
            if ((ctx.next_code >= (LZW_CODE)(1u << ctx.code_bits)) && (ctx.code_bits < LZW_MAX_BITS)) {
                ctx.code_bits++;
            }
        }

        ctx.old_code = inCode;
        ctx.last_value = firstByte;
    }

    bool ok = lzw_flush_out(&ctx);
    return ok && !ctx.state->bReadError && !ctx.state->bWriteError;
}
