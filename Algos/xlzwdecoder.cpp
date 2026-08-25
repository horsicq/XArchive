/* Copyright (c) 2017-2026 hors<horsic@gmail.com>
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
#include "algo_utils.h"
#include <assert.h>
#include <limits>
#include <string.h>
#include <vector>

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
#define NEXTBYTE(a) ((a) & 0xff)
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
    LZWSTATE *state = init_lzw(earlychange, rf, NULL, user_read, NULL, 1 << LZW_MAXBITS, stack);
    if (!state) {
        free(stack);
    }

    return state;
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

struct LZW_DEVICE_CONTEXT {
    XBinary::DATAPROCESS_STATE *pState;
    XBinary::PDSTRUCT *pPdStruct;
};

int rdfunc_device(void *user, unsigned char *buf, int len)
{
    LZW_DEVICE_CONTEXT *pContext = static_cast<LZW_DEVICE_CONTEXT *>(user);
    if (!pContext || !pContext->pState || !buf || (len <= 0) || !XBinary::isPdStructNotCanceled(pContext->pPdStruct)) {
        if (pContext && pContext->pState) pContext->pState->bReadError = true;
        return 1;
    }

    const qint32 nRequest = Algo_utils::getReadChunkSize(pContext->pState, len);
    if (nRequest != len || XBinary::_readDevice(reinterpret_cast<char *>(buf), len, pContext->pState) != len) {
        pContext->pState->bReadError = true;
        return 1;
    }
    return 0;
}

int lzwDecodeDevice(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, int early = -1)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput) {
        // qDebug() << "Invalid device pointers";
        return 1;
    }

    if (!pState->pDeviceInput->isOpen() || !pState->pDeviceInput->isReadable()) {
        // qDebug() << "Input device is not open or not readable";
        return 2;
    }

    if (!pState->pDeviceOutput->isOpen() || !pState->pDeviceOutput->isWritable()) {
        // qDebug() << "Output device is not open or not writable";
        return 3;
    }

    LZWSTATE *lzw = NULL;
    unsigned char *buffer = NULL;
    int result = 0;

    // Initialize LZW decoder
    LZW_DEVICE_CONTEXT context = {pState, pPdStruct};
    lzw = init_lzw_read(early, rdfunc_device, &context);
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
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            pState->bReadError = true;
            result = 7;
            break;
        }

        result = decode_lzw(lzw, buffer, BUFFER_SIZE);

        if (result > 0) {  // Decoding complete
            int length = result - 1;
            if (length > 0) {
                if (XBinary::_writeDevice(reinterpret_cast<char *>(buffer), length, pState) != length) {
                    // qDebug() << "Write error:" << outputDevice->errorString();
                    result = 6;
                }
            }
            if (result != 6) result = 0;  // Success
            break;
        } else if (result < 0) {
            // qDebug() << "Decoder error:" << result;
            break;
        }

        // Write the full buffer to output device
        if (XBinary::_writeDevice(reinterpret_cast<char *>(buffer), BUFFER_SIZE, pState) != BUFFER_SIZE) {
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
bool XLZWDecoder::decompress_pdf(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) return false;

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const int nResult = lzwDecodeDevice(pDecompressState, pPdStruct);

    const bool bConsumedBoundedInput = (pDecompressState->nInputLimit == -1) ||
                                       (pDecompressState->nCountInput == pDecompressState->nInputLimit);

    return (nResult == 0) && bConsumedBoundedInput && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

namespace {
const quint32 ZOO_LZD_CLEAR = 256;
const quint32 ZOO_LZD_EOF = 257;
const quint32 ZOO_LZD_FIRST_FREE = 258;
const quint32 ZOO_LZD_TABLE_SIZE = 8192;
const qint32 ZOO_LZD_MIN_BITS = 9;
const qint32 ZOO_LZD_MAX_BITS = 13;
const qint32 ZOO_LZD_OUTPUT_BUFFER_SIZE = 0x10000;

class ZooLzdBitReader {
public:
    explicit ZooLzdBitReader(XBinary::DATAPROCESS_STATE *pState) : m_pState(pState), m_nBits(0), m_nBuffer(0) {}

    bool readCode(qint32 nWidth, quint32 *pCode)
    {
        if (!m_pState || !pCode || (nWidth < ZOO_LZD_MIN_BITS) || (nWidth > ZOO_LZD_MAX_BITS)) return false;

        while (m_nBits < nWidth) {
            char cByte = 0;
            if (XBinary::_readDevice(&cByte, 1, m_pState) != 1) {
                m_pState->bReadError = true;
                return false;
            }
            m_nBuffer |= ((quint64)(quint8)cByte << m_nBits);
            m_nBits += 8;
        }

        *pCode = (quint32)(m_nBuffer & ((((quint64)1) << nWidth) - 1));
        m_nBuffer >>= nWidth;
        m_nBits -= nWidth;
        return true;
    }

    bool hasOnlyZeroPadding() const { return m_nBuffer == 0; }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    qint32 m_nBits;
    quint64 m_nBuffer;
};
}  // namespace

// ZOO method 1 (LZD) is a distinct 13-bit, LSB-first LZW stream.  It starts
// with CLEAR, reserves 257 as an explicit end marker, and resets the table when
// it reaches 8192 codes.  Keep it separate from PDF LZW, whose packing and
// maximum code width are different.
bool XLZWDecoder::decompress_zoo(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput ||
        (pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1)) {
        return false;
    }

    const bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    const qint64 nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    if (bHasExpectedSize && (nExpectedSize < 0)) return false;

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    std::vector<quint16> prefix(ZOO_LZD_TABLE_SIZE, 0);
    std::vector<quint8> suffix(ZOO_LZD_TABLE_SIZE, 0);
    std::vector<quint8> decodeStack(ZOO_LZD_TABLE_SIZE, 0);
    QByteArray baOutput;
    baOutput.reserve(ZOO_LZD_OUTPUT_BUFFER_SIZE);
    ZooLzdBitReader reader(pDecompressState);

    qint32 nCodeWidth = ZOO_LZD_MIN_BITS;
    quint32 nWidthLimit = (1U << ZOO_LZD_MIN_BITS);
    quint32 nNextCode = ZOO_LZD_FIRST_FREE;
    qint32 nPreviousCode = -1;
    qint64 nProduced = 0;
    bool bInitialClearSeen = false;
    bool bExplicitEOFSeen = false;
    bool bDataError = false;

    const auto flushOutput = [&]() -> bool {
        if (baOutput.isEmpty()) return true;
        const qint32 nSize = baOutput.size();
        if (XBinary::_writeDevice(baOutput.constData(), nSize, pDecompressState) != nSize) return false;
        baOutput.clear();
        return true;
    };

    const auto emitByte = [&](quint8 nByte) -> bool {
        if ((nProduced == (std::numeric_limits<qint64>::max)()) ||
            (bHasExpectedSize && (nProduced >= nExpectedSize))) {
            return false;
        }
        baOutput.append((char)nByte);
        nProduced++;
        return (baOutput.size() < ZOO_LZD_OUTPUT_BUFFER_SIZE) || flushOutput();
    };

    const auto expandCode = [&](quint32 nCode, quint32 *pStackSize, quint8 *pFirstByte) -> bool {
        if (!pStackSize || !pFirstByte) return false;
        quint32 nStackSize = 0;
        quint32 nDepth = 0;

        while (nCode >= ZOO_LZD_FIRST_FREE) {
            if ((nCode >= nNextCode) || (nStackSize >= ZOO_LZD_TABLE_SIZE) || (++nDepth > ZOO_LZD_TABLE_SIZE)) return false;
            decodeStack[nStackSize++] = suffix[nCode];
            nCode = prefix[nCode];
        }
        if ((nCode >= 256) || (nStackSize >= ZOO_LZD_TABLE_SIZE)) return false;
        decodeStack[nStackSize++] = (quint8)nCode;
        *pStackSize = nStackSize;
        *pFirstByte = (quint8)nCode;
        return true;
    };

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nCode = 0;
        if (!reader.readCode(nCodeWidth, &nCode)) break;

        if (!bInitialClearSeen) {
            if (nCode != ZOO_LZD_CLEAR) {
                bDataError = true;
                break;
            }
            bInitialClearSeen = true;
        }

        if (nCode == ZOO_LZD_EOF) {
            bExplicitEOFSeen = true;
            break;
        }

        if (nCode == ZOO_LZD_CLEAR) {
            nCodeWidth = ZOO_LZD_MIN_BITS;
            nWidthLimit = (1U << ZOO_LZD_MIN_BITS);
            nNextCode = ZOO_LZD_FIRST_FREE;
            nPreviousCode = -1;

            if (!reader.readCode(nCodeWidth, &nCode)) break;
            if (nCode == ZOO_LZD_EOF) {
                bExplicitEOFSeen = true;
                break;
            }
            if (nCode >= 256) {
                bDataError = true;
                break;
            }
            if (!emitByte((quint8)nCode)) {
                bDataError = !pDecompressState->bWriteError;
                break;
            }
            nPreviousCode = (qint32)nCode;
            continue;
        }

        if ((nPreviousCode < 0) || (nNextCode >= ZOO_LZD_TABLE_SIZE)) {
            bDataError = true;
            break;
        }

        const quint32 nInputCode = nCode;
        quint32 nStackSize = 0;
        quint8 nFirstByte = 0;

        if (nCode < nNextCode) {
            if (!expandCode(nCode, &nStackSize, &nFirstByte)) {
                bDataError = true;
                break;
            }
            bool bEmitOK = true;
            while (nStackSize > 0) {
                if (!emitByte(decodeStack[--nStackSize])) {
                    bEmitOK = false;
                    break;
                }
            }
            if (!bEmitOK) bDataError = !pDecompressState->bWriteError;
        } else if (nCode == nNextCode) {
            if (!expandCode((quint32)nPreviousCode, &nStackSize, &nFirstByte)) {
                bDataError = true;
                break;
            }
            bool bEmitOK = true;
            while (nStackSize > 0) {
                if (!emitByte(decodeStack[--nStackSize])) {
                    bEmitOK = false;
                    break;
                }
            }
            if (!bEmitOK) bDataError = !pDecompressState->bWriteError;
            if (!pDecompressState->bWriteError && !bDataError && !emitByte(nFirstByte)) bDataError = !pDecompressState->bWriteError;
        } else {
            bDataError = true;
            break;
        }

        if (pDecompressState->bWriteError || bDataError) break;

        prefix[nNextCode] = (quint16)nPreviousCode;
        suffix[nNextCode] = nFirstByte;
        nNextCode++;
        if ((nNextCode >= nWidthLimit) && (nCodeWidth < ZOO_LZD_MAX_BITS)) {
            nCodeWidth++;
            nWidthLimit <<= 1;
        }
        nPreviousCode = (qint32)nInputCode;
    }

    if (!bDataError && bExplicitEOFSeen && XBinary::isPdStructNotCanceled(pPdStruct) && !pDecompressState->bReadError &&
        !pDecompressState->bWriteError && !flushOutput()) {
        bDataError = !pDecompressState->bWriteError;
    }

    const bool bInputComplete = (pDecompressState->nInputLimit == -1)
                                    ? pDecompressState->pDeviceInput->atEnd()
                                    : (pDecompressState->nCountInput == pDecompressState->nInputLimit);
    const bool bOutputComplete = !bHasExpectedSize || (nProduced == nExpectedSize);

    return bInitialClearSeen && bExplicitEOFSeen && reader.hasOnlyZeroPadding() && bInputComplete && bOutputComplete &&
           (pDecompressState->nCountOutput == nProduced) && !bDataError && !pDecompressState->bReadError &&
           !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
