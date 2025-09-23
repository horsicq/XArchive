/* Copyright (c) 2017-2025 hors<horsic@gmail.com>
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

typedef int (*WRITEFUNC)(void *user, unsigned char *buf, int len);
typedef int (*READFUNC)(void *user, unsigned char *buf, int len);

typedef struct LZWSTATE {
    READFUNC read;
    WRITEFUNC write;
    void *user_read, *user_write;

    int earlychange;

    int numcodes;         // currently used codes
    int codebits;         // currently used bits
    int prefix;           // current prefix (encoding) / last code (decoding)
    unsigned int *table;  // encoding: hash-table (code[12bit],prefixcode[12bit],nextbyte)[hash(prefixcode,nextbyte)]
    // decoding: symbol-table (prefixcode,nextbyte)[code]
    unsigned char *stackend, *stackptr;  // for decoding.

    int bitpos;
    unsigned int bitbuf;
} LZWSTATE;

#define LZW_CLEAR 256
#define LZW_END 257
#define LZW_START 258
#define LZW_MINBITS 9
#define LZW_MAXBITS 12     // max 12 because of table=32 bit
#define LZW_HASHSIZE 9001  // at least 1<<MAXBITS, should be prime

// accessors to table[]-values
#define NEXTBYTE(a) ((a)&0xff)
#define PREFIXCODE(a) ((a >> 8) & 0xfff)
#define CODE(a) ((a >> 20) & 0xfff)                                                            // encode only
#define MAKETABLE(code, prefixcode, nextbyte) ((code << 20) | (prefixcode << 8) | (nextbyte))  // for decode: code=0
// hash func;
#define HASH(prefixcode, nextbyte) ((((prefixcode << 8) | nextbyte) << 11) % LZW_HASHSIZE)

void restart_lzw(LZWSTATE *state)
{
    assert(state);
    if (state) {
        state->numcodes = LZW_START;
        state->codebits = LZW_MINBITS;
        state->prefix = -1;  // no prefix / clear table
        state->stackptr = state->stackend;

        state->bitbuf = 0;
        state->bitpos = 0;
    }
}

void free_lzw(LZWSTATE *state)
{
    if (state) {
        free(state->stackend - (1 << LZW_MAXBITS));  // look at init_lzw !
        free(state->table);
        free(state);
    }
}

LZWSTATE *init_lzw(int earlychange, READFUNC rf, WRITEFUNC wf, void *user_read, void *user_write, int tablesize, unsigned char *stack)
{
    LZWSTATE *ret;

    if (earlychange < 0) {
        earlychange = 1;  // default
    }

    ret = (LZWSTATE *)malloc(sizeof(LZWSTATE));
    if (!ret) {
        return 0;
    }

    ret->read = rf;
    ret->write = wf;
    ret->user_read = user_read;
    ret->user_write = user_write;

    ret->earlychange = earlychange;

    ret->table = (unsigned int *)malloc(tablesize * sizeof(unsigned int));
    if (!ret->table) {
        free(ret);
        return NULL;
    }
    ret->stackend = ret->stackptr = stack + (1 << LZW_MAXBITS);  // this is tricky!

    restart_lzw(ret);
    return ret;
}

LZWSTATE *init_lzw_read(int earlychange, READFUNC rf, void *user_read)
{
    unsigned char *stack;
    assert(rf);
    if (!rf) {
        return 0;
    }
    stack = (unsigned char *)malloc((1 << LZW_MAXBITS) * sizeof(unsigned char));
    if (!stack) {
        return NULL;
    }
    return init_lzw(earlychange, rf, NULL, user_read, NULL, 1 << LZW_MAXBITS, stack);
}

static int readbits(LZWSTATE *state)
{
    int ret, iA;
    unsigned char buf[4];

    if (state->bitpos < state->codebits)  // ensure enough bits
    {
        int num = (state->codebits - state->bitpos + 7) / 8;
        ret = (*state->read)(state->user_read, buf, num);
        if (ret) {
            return -1;
        }
        for (iA = 0; iA < num; iA++) {
            state->bitbuf |= buf[iA] << (24 - state->bitpos);
            state->bitpos += 8;
        }
    }
    state->bitpos -= state->codebits;
    ret = state->bitbuf >> (32 - state->codebits);
    state->bitbuf <<= state->codebits;
    return ret;
}

int decode_lzw(LZWSTATE *state, unsigned char *buf, int len)
{
    int outlen = 0;
    assert(state);
    assert(len >= 0);

    while (len > 0) {
        // first empty the stack
        const int stacklen = state->stackend - state->stackptr;
        if (stacklen > 0) {
            if (len < stacklen) {
                memcpy(buf, state->stackptr, len * sizeof(char));
                state->stackptr += len;
                return 0;
            } else {
                memcpy(buf, state->stackptr, stacklen * sizeof(char));
                outlen += stacklen;
                len -= stacklen;
                buf += stacklen;
                state->stackptr = state->stackend;
                continue;  // check for len==0;
            }
        }
        // decode next code
        int code = readbits(state);
        if (code < 0) {
            return -1;  // read error
        } else if (code == LZW_CLEAR) {
            state->numcodes = LZW_START;
            state->codebits = LZW_MINBITS;
            state->prefix = -1;
        } else if (code == LZW_END) {
            return 1 + outlen;  // done
        } else if (code < 256)  // not in table
        {
            *buf = code;
            buf++;
            len--;
            outlen++;
            if (state->prefix >= 0) {
                state->table[state->numcodes++] = MAKETABLE(0, state->prefix, code);
            }
            state->prefix = code;
        } else if (code < state->numcodes) {
            int scode = code;
            assert(state->prefix >= 0);
            // push on stack to reverse
            while (code >= 256) {
                *--state->stackptr = NEXTBYTE(state->table[code]);
                code = PREFIXCODE(state->table[code]);
            }
            *--state->stackptr = code;
            // add to table
            state->table[state->numcodes++] = MAKETABLE(0, state->prefix, code);
            state->prefix = scode;
        } else if (code == state->numcodes) {
            if (state->prefix < 0) {
                return -2;  // invalid code, a <256 code is required first
            }
            code = state->prefix;
            assert(state->stackptr == state->stackend);  // the stack is empty!
            --state->stackptr;                           // will be filled later: first char==last char
            while (code >= 256) {
                *--state->stackptr = NEXTBYTE(state->table[code]);
                code = PREFIXCODE(state->table[code]);
            }
            *--state->stackptr = code;
            state->stackend[-1] = code;
            state->table[state->numcodes] = MAKETABLE(0, state->prefix, code);
            state->prefix = state->numcodes++;
        } else {
            return -2;  // invalid code
        }
        if (state->numcodes == (1 << state->codebits) - state->earlychange) {
            if (state->codebits == LZW_MAXBITS) {
                state->numcodes--;
            } else {
                state->codebits++;
            }
        }
    }
    return 0;
}

int rdfunc_device(void *user, unsigned char *buf, int len)
{
    QIODevice *device = static_cast<QIODevice *>(user);
    return (device->read(reinterpret_cast<char *>(buf), len) == len) ? 0 : 1;
}

int lzwDecodeDevice(QIODevice *inputDevice, QIODevice *outputDevice, int early = -1)
{
    if (!inputDevice || !outputDevice) {
        // qDebug() << "Invalid device pointers";
        return 1;
    }

    if (!inputDevice->isOpen() || !inputDevice->isReadable()) {
        // qDebug() << "Input device is not open or not readable";
        return 2;
    }

    if (!outputDevice->isOpen() || !outputDevice->isWritable()) {
        // qDebug() << "Output device is not open or not writable";
        return 3;
    }

    LZWSTATE *lzw = NULL;
    unsigned char *buffer = NULL;
    int result = 0;

    // Initialize LZW decoder
    lzw = init_lzw_read(early, rdfunc_device, inputDevice);
    if (!lzw) {
        // qDebug() << "Failed to initialize LZW decoder";
        return 4;
    }

    // Allocate buffer for decoded data
    const int BUFFER_SIZE = 4096;
    buffer = (unsigned char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        // qDebug() << "Memory allocation failed:" << strerror(errno);
        free_lzw(lzw);
        return 5;
    }

    // Decode loop
    while (true) {
        result = decode_lzw(lzw, buffer, BUFFER_SIZE);

        if (result > 0) {  // Decoding complete
            int length = result - 1;
            if (length > 0) {
                if (outputDevice->write(reinterpret_cast<char *>(buffer), length) != length) {
                    // qDebug() << "Write error:" << outputDevice->errorString();
                    result = 6;
                }
            }
            result = 0;  // Success
            break;
        } else if (result < 0) {
            // qDebug() << "Decoder error:" << result;
            break;
        }

        // Write the full buffer to output device
        if (outputDevice->write(reinterpret_cast<char *>(buffer), BUFFER_SIZE) != BUFFER_SIZE) {
            // qDebug() << "Write error:" << outputDevice->errorString();
            result = 6;
            break;
        }
    }

    // Clean up
    free(buffer);
    free_lzw(lzw);

    return result;
}

XLZWDecoder::XLZWDecoder(QObject *parent) : QObject(parent)
{
}

// Streaming PDF LZWDecode implementation.
bool XLZWDecoder::decompress_pdf(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) return false;

    // Set input device position
    if (pDecompressState->nInputOffset > 0) {
        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    }

    // Set output device position
    if (pDecompressState->pDeviceOutput) {
        pDecompressState->pDeviceOutput->seek(0);
    }

    lzwDecodeDevice(pDecompressState->pDeviceInput, pDecompressState->pDeviceOutput);

    return !(pDecompressState->bReadError || pDecompressState->bWriteError);
}
