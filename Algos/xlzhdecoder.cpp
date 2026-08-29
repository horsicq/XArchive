/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xlzhdecoder.h"
#include "algo_utils.h"
#include "xbinary.h"

#include <limits>
#include <memory>
#include <new>

// LH1 currently uses whole-member input and output buffers. Bound both sides
// well below QByteArray's signed-int ceiling so hostile headers cannot trigger
// near-2-GiB allocations (or two such allocations at once).
static const qint64 LH1_MAX_PACKED_BUFFER_SIZE = 256LL * 1024 * 1024;
static const qint64 LH1_MAX_UNPACKED_BUFFER_SIZE = 256LL * 1024 * 1024;
static const qint32 LH1_READ_BUFFER_SIZE = 0x10000;

static const char bitlen_tbl[0x400] = {
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
    9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
    9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
    9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 16, 0};

XLZHDecoder::XLZHDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XLZHDecoder::lzh_br_bit_count_is_valid(qint32 n)
{
    return (n >= 0) && (n <= 16);
}

quint16 XLZHDecoder::lzh_br_bits(const lzh_br *br, qint32 n)
{
    if ((br == nullptr) || !lzh_br_bit_count_is_valid(n) || (br->cache_avail < n) || (br->cache_avail > (qint32)CACHE_BITS) || (n == 0)) {
        return 0;
    }

    const quint64 nMask = (((quint64)1) << n) - 1;
    return (quint16)((br->cache_buffer >> (br->cache_avail - n)) & nMask);
}

quint16 XLZHDecoder::lzh_br_bits_forced(const lzh_br *br, qint32 n)
{
    if ((br == nullptr) || !lzh_br_bit_count_is_valid(n) || (br->cache_avail < 0) || (br->cache_avail > (qint32)CACHE_BITS) || (n == 0)) {
        return 0;
    }
    if (br->cache_avail >= n) {
        return lzh_br_bits(br, n);
    }

    const quint64 nMask = (((quint64)1) << n) - 1;
    return (quint16)((br->cache_buffer << (n - br->cache_avail)) & nMask);
}

bool XLZHDecoder::lzh_decode_init(lzh_stream *strm, qint32 method)
{
    return lzh_decode_init(strm, method, TERMINATION_PHYSICAL_EOF);
}

bool XLZHDecoder::lzh_decode_init(lzh_stream *strm, qint32 method, TERMINATION_MODE terminationMode)
{
    struct lzh_dec *ds;
    qint32 w_bits, w_size;

    if ((terminationMode != TERMINATION_PHYSICAL_EOF) && (terminationMode != TERMINATION_ZERO_BLOCK)) {
        return false;
    }

    switch (method) {
        case 4:
            w_bits = 12; /* 4KiB for window */
            break;
        case 5:
            w_bits = 13; /* 8KiB for window */
            break;
        case 6:
            w_bits = 15; /* 32KiB for window */
            break;
        case 7:
            w_bits = 16; /* 64KiB for window */
            break;
        default: return false; /* Not supported. */
    }

    if (strm->ds == nullptr) {
        strm->ds = static_cast<lzh_dec *>(calloc(1, sizeof(*strm->ds)));
    }
    if (strm->ds == nullptr) {
        return false;
    }

    ds = strm->ds;

    /* Expand a window size up to 128 KiB for decompressing process
     * performance whatever its original window size is. */
    ds->w_size = 1U << 17;
    ds->w_mask = ds->w_size - 1;
    if (ds->w_buff == nullptr) {
        ds->w_buff = static_cast<quint8 *>(malloc(ds->w_size));
    }
    if (ds->w_buff == nullptr) {
        return false;
    }
    w_size = 1U << w_bits;
    memset(ds->w_buff + ds->w_size - w_size, 0x20, w_size);
    ds->w_pos = 0;
    ds->state = 0;
    ds->pos_pt_len_size = w_bits + 1;
    ds->pos_pt_len_bits = (w_bits == 15 || w_bits == 16) ? 5 : 4;
    ds->literal_pt_len_size = LZH_PT_BITLEN_SIZE;
    ds->literal_pt_len_bits = 5;
    ds->br.cache_buffer = 0;
    ds->br.cache_avail = 0;
    ds->bZeroBlockTermination = (terminationMode == TERMINATION_ZERO_BLOCK);
    ds->bExplicitEOFSeen = false;

    if (!lzh_huffman_init(&(ds->lt), LZH_LT_BITLEN_SIZE, 16) || !lzh_huffman_init(&(ds->pt), LZH_PT_BITLEN_SIZE, 16)) {
        return false;
    }
    ds->lt.len_bits = 9;

    ds->error = 0;

    return true;
}

bool XLZHDecoder::lzh_huffman_init(lzh_huffman *hf, size_t len_size, qint32 tbl_bits)
{
    if (!hf || (len_size == 0) || (tbl_bits <= 0) || (tbl_bits >= (qint32)(sizeof(size_t) * 8))) {
        return false;
    }

    qint32 bits;

    if (hf->bitlen == nullptr) {
        hf->bitlen = static_cast<quint8 *>(malloc(len_size * sizeof(hf->bitlen[0])));
        if (hf->bitlen == nullptr) {
            return false;
        }
    }
    if (hf->tbl == nullptr) {
        if (tbl_bits < LZH_HTBL_BITS) bits = tbl_bits;
        else bits = LZH_HTBL_BITS;
        hf->tbl = static_cast<quint16 *>(malloc(((size_t)1 << bits) * sizeof(hf->tbl[0])));
        if (hf->tbl == nullptr) {
            return false;
        }
    }
    if (hf->tree == nullptr && tbl_bits > LZH_HTBL_BITS) {
        const qint32 nTreeBits = tbl_bits - LZH_HTBL_BITS + 4;
        if ((nTreeBits < 0) || (nTreeBits >= (qint32)(sizeof(size_t) * 8))) {
            return false;
        }
        const size_t nTreeAvail = (size_t)1 << nTreeBits;
        if (nTreeAvail > (size_t)(std::numeric_limits<qint32>::max)()) {
            return false;
        }
        hf->tree_avail = (qint32)nTreeAvail;
        hf->tree = static_cast<lzh_htree_t *>(malloc(hf->tree_avail * sizeof(hf->tree[0])));
        if (hf->tree == nullptr) {
            return false;
        }
    }
    hf->len_size = static_cast<int>(len_size);
    hf->tbl_bits = tbl_bits;

    return true;
}

qint32 XLZHDecoder::lzh_decode(lzh_stream *strm, qint32 last)
{
    struct lzh_dec *ds = strm->ds;
    qint32 avail_in;
    qint32 r;

    if (ds->error) return (ds->error);

    avail_in = strm->avail_in;
    do {
        if (ds->state < ST_GET_LITERAL) r = lzh_read_blocks(strm, last);
        else r = lzh_decode_blocks(strm, last);
    } while (r == 100);
    strm->total_in += avail_in - strm->avail_in;
    return (r);
}

qint32 XLZHDecoder::lzh_read_blocks(lzh_stream *strm, qint32 last)
{
    struct lzh_dec *ds = strm->ds;
    struct lzh_br *br = &(ds->br);
    qint32 c = 0, i;
    unsigned rbits;

    for (;;) {
        switch (ds->state) {
            case ST_RD_BLOCK:
                /*
                 * Read a block number indicates how many blocks
                 * we will handle. The block is composed of a
                 * literal and a match, sometimes a literal only
                 * in particular, there are no reference data at
                 * the beginning of the decompression.
                 */
                if (!lzh_br_read_ahead_0(strm, br, 16)) {
                    if (!last) /* We need following data. */
                        return (LZH_ARCHIVE_OK);
                    if (ds->bZeroBlockTermination) {
                        /* ZOO method 2 requires an explicit zero-sized block. */
                        goto failed;
                    }
                    if (lzh_br_has(br, 8)) {
                        /*
                         * It seems there are extra bits.
                         *  1. Compressed data is broken.
                         *  2. `last' flag does not properly
                         *     set.
                         */
                        goto failed;
                    }
                    if (ds->w_pos > 0) {
                        lzh_emit_window(strm, ds->w_pos);
                        ds->w_pos = 0;
                        return (LZH_ARCHIVE_OK);
                    }
                    /* End of compressed data; we have completely
                     * handled all compressed data. */
                    return (LZH_ARCHIVE_EOF);
                }
                ds->blocks_avail = lzh_br_bits(br, 16);
                if (ds->blocks_avail == 0) {
                    if (!ds->bZeroBlockTermination) goto failed;
                    lzh_br_consume(br, 16);

                    /*
                     * Zoo's encoder pads with zero bits around its explicit
                     * terminator.  More than seven residual padding
                     * bits, a
                     * non-zero residual bit, or unread bytes are trailing
                     * member data and must not be silently
                     * accepted.
                     */
                    if ((strm->avail_in != 0) || (br->cache_avail > 7) || ((br->cache_avail > 0) && (lzh_br_bits(br, br->cache_avail) != 0))) {
                        goto failed;
                    }

                    ds->bExplicitEOFSeen = true;
                    if (ds->w_pos > 0) {
                        lzh_emit_window(strm, ds->w_pos);
                        ds->w_pos = 0;
                    }
                    return (LZH_ARCHIVE_EOF);
                }
                lzh_br_consume(br, 16);
                /*
                 * Read a literal table compressed in huffman
                 * coding.
                 */
                ds->pt.len_size = ds->literal_pt_len_size;
                ds->pt.len_bits = ds->literal_pt_len_bits;
                ds->reading_position = 0;
                /* FALL THROUGH */
            case ST_RD_PT_1:
                /* Note: ST_RD_PT_1, ST_RD_PT_2 and ST_RD_PT_4 are
                 * used in reading both a literal table and a
                 * position table. */
                if (!lzh_br_bit_count_is_valid(ds->pt.len_bits)) goto failed;
                if (!lzh_br_read_ahead(strm, br, ds->pt.len_bits)) {
                    if (last) goto failed; /* Truncated data. */
                    ds->state = ST_RD_PT_1;
                    return (LZH_ARCHIVE_OK);
                }
                ds->pt.len_avail = lzh_br_bits(br, ds->pt.len_bits);
                lzh_br_consume(br, ds->pt.len_bits);
                /* FALL THROUGH */
            case ST_RD_PT_2:
                if (!lzh_br_bit_count_is_valid(ds->pt.len_bits)) goto failed;
                if (ds->pt.len_avail == 0) {
                    /* There is no bitlen. */
                    if (!lzh_br_read_ahead(strm, br, ds->pt.len_bits)) {
                        if (last) goto failed; /* Truncated data.*/
                        ds->state = ST_RD_PT_2;
                        return (LZH_ARCHIVE_OK);
                    }
                    if (!lzh_make_fake_table(&(ds->pt), lzh_br_bits(br, ds->pt.len_bits))) goto failed; /* Invalid data. */
                    lzh_br_consume(br, ds->pt.len_bits);
                    if (ds->reading_position) ds->state = ST_GET_LITERAL;
                    else ds->state = ST_RD_LITERAL_1;
                    break;
                } else if (ds->pt.len_avail > ds->pt.len_size) goto failed; /* Invalid data. */
                ds->loop = 0;
                memset(ds->pt.freq, 0, sizeof(ds->pt.freq));
                if (ds->pt.len_avail < 3 || ds->pt.len_size == ds->pos_pt_len_size) {
                    ds->state = ST_RD_PT_4;
                    break;
                }
                /* FALL THROUGH */
            case ST_RD_PT_3:
                ds->loop = lzh_read_pt_bitlen(strm, ds->loop, 3);
                if (ds->loop < 3) {
                    if (ds->loop < 0 || last) goto failed; /* Invalid data. */
                    /* Not completed, get following data. */
                    ds->state = ST_RD_PT_3;
                    return (LZH_ARCHIVE_OK);
                }
                /* There are some null in bitlen of the literal. */
                if (!lzh_br_read_ahead(strm, br, 2)) {
                    if (last) goto failed; /* Truncated data. */
                    ds->state = ST_RD_PT_3;
                    return (LZH_ARCHIVE_OK);
                }
                c = lzh_br_bits(br, 2);
                lzh_br_consume(br, 2);
                if (c > ds->pt.len_avail - 3) goto failed; /* Invalid data. */
                for (i = 3; c-- > 0;) ds->pt.bitlen[i++] = 0;
                ds->loop = i;
                /* FALL THROUGH */
            case ST_RD_PT_4:
                ds->loop = lzh_read_pt_bitlen(strm, ds->loop, ds->pt.len_avail);
                if (ds->loop < ds->pt.len_avail) {
                    if (ds->loop < 0 || last) goto failed; /* Invalid data. */
                    /* Not completed, get following data. */
                    ds->state = ST_RD_PT_4;
                    return (LZH_ARCHIVE_OK);
                }
                if (!lzh_make_huffman_table(&(ds->pt))) goto failed; /* Invalid data */
                if (ds->reading_position) {
                    ds->state = ST_GET_LITERAL;
                    break;
                }
                /* FALL THROUGH */
            case ST_RD_LITERAL_1:
                if (!lzh_br_bit_count_is_valid(ds->lt.len_bits)) goto failed;
                if (!lzh_br_read_ahead(strm, br, ds->lt.len_bits)) {
                    if (last) goto failed; /* Truncated data. */
                    ds->state = ST_RD_LITERAL_1;
                    return (LZH_ARCHIVE_OK);
                }
                ds->lt.len_avail = lzh_br_bits(br, ds->lt.len_bits);
                lzh_br_consume(br, ds->lt.len_bits);
                /* FALL THROUGH */
            case ST_RD_LITERAL_2:
                if (!lzh_br_bit_count_is_valid(ds->lt.len_bits)) goto failed;
                if (ds->lt.len_avail == 0) {
                    /* There is no bitlen. */
                    if (!lzh_br_read_ahead(strm, br, ds->lt.len_bits)) {
                        if (last) goto failed; /* Truncated data.*/
                        ds->state = ST_RD_LITERAL_2;
                        return (LZH_ARCHIVE_OK);
                    }
                    if (!lzh_make_fake_table(&(ds->lt), lzh_br_bits(br, ds->lt.len_bits))) goto failed; /* Invalid data */
                    lzh_br_consume(br, ds->lt.len_bits);
                    ds->state = ST_RD_POS_DATA_1;
                    break;
                } else if (ds->lt.len_avail > ds->lt.len_size) goto failed; /* Invalid data */
                ds->loop = 0;
                memset(ds->lt.freq, 0, sizeof(ds->lt.freq));
                /* FALL THROUGH */
            case ST_RD_LITERAL_3:
                if (!lzh_br_bit_count_is_valid(ds->pt.max_bits)) goto failed;
                i = ds->loop;
                while (i < ds->lt.len_avail) {
                    if (!lzh_br_read_ahead(strm, br, ds->pt.max_bits)) {
                        if (last) goto failed; /* Truncated data.*/
                        ds->loop = i;
                        ds->state = ST_RD_LITERAL_3;
                        return (LZH_ARCHIVE_OK);
                    }
                    rbits = lzh_br_bits(br, ds->pt.max_bits);
                    c = lzh_decode_huffman(&(ds->pt), rbits);
                    if (c > 2) {
                        /* Note: 'c' will never be more than
                         * eighteen since it's limited by
                         * PT_BITLEN_SIZE, which is being set
                         * to ds->pt.len_size through
                         * ds->literal_pt_len_size. */
                        lzh_br_consume(br, ds->pt.bitlen[c]);
                        c -= 2;
                        ds->lt.freq[c]++;
                        ds->lt.bitlen[i++] = c;
                    } else if (c == 0) {
                        lzh_br_consume(br, ds->pt.bitlen[c]);
                        ds->lt.bitlen[i++] = 0;
                    } else {
                        /* c == 1 or c == 2 */
                        qint32 n = (c == 1) ? 4 : 9;
                        if (!lzh_br_read_ahead(strm, br, ds->pt.bitlen[c] + n)) {
                            if (last) /* Truncated data. */
                                goto failed;
                            ds->loop = i;
                            ds->state = ST_RD_LITERAL_3;
                            return (LZH_ARCHIVE_OK);
                        }
                        lzh_br_consume(br, ds->pt.bitlen[c]);
                        c = lzh_br_bits(br, n);
                        lzh_br_consume(br, n);
                        c += (n == 4) ? 3 : 20;
                        if (i + c > ds->lt.len_avail) goto failed; /* Invalid data */
                        memset(&(ds->lt.bitlen[i]), 0, c);
                        i += c;
                    }
                }
                if (i > ds->lt.len_avail || !lzh_make_huffman_table(&(ds->lt))) goto failed; /* Invalid data */
                /* FALL THROUGH */
            case ST_RD_POS_DATA_1:
                /*
                 * Read a position table compressed in huffman
                 * coding.
                 */
                ds->pt.len_size = ds->pos_pt_len_size;
                ds->pt.len_bits = ds->pos_pt_len_bits;
                ds->reading_position = 1;
                ds->state = ST_RD_PT_1;
                break;
            case ST_GET_LITERAL: return (100);
        }
    }
failed:
    return (ds->error = LZH_ARCHIVE_FAILED);
}

qint32 XLZHDecoder::lzh_decode_blocks(lzh_stream *strm, qint32 last)
{
    struct lzh_dec *ds = strm->ds;
    struct lzh_br bre = ds->br;
    struct lzh_huffman *lt = &(ds->lt);
    struct lzh_huffman *pt = &(ds->pt);
    quint8 *w_buff = ds->w_buff;
    quint8 *lt_bitlen = lt->bitlen;
    quint8 *pt_bitlen = pt->bitlen;
    qint32 blocks_avail = ds->blocks_avail, c = 0;
    qint32 copy_len = ds->copy_len, copy_pos = ds->copy_pos;
    qint32 w_pos = ds->w_pos, w_mask = ds->w_mask, w_size = ds->w_size;
    qint32 lt_max_bits = lt->max_bits, pt_max_bits = pt->max_bits;
    qint32 state = ds->state;

    if (!lzh_br_bit_count_is_valid(lt_max_bits) || !lzh_br_bit_count_is_valid(pt_max_bits)) goto failed;

    for (;;) {
        switch (state) {
            case ST_GET_LITERAL:
                for (;;) {
                    if (blocks_avail == 0) {
                        /* We have decoded all blocks.
                         * Let's handle next blocks. */
                        ds->state = ST_RD_BLOCK;
                        ds->br = bre;
                        ds->blocks_avail = 0;
                        ds->w_pos = w_pos;
                        ds->copy_pos = 0;
                        return (100);
                    }

                    /* lzh_br_read_ahead() always try to fill the
                     * cache buffer up. In specific situation we
                     * are close to the end of the data, the cache
                     * buffer will not be full and thus we have to
                     * determine if the cache buffer has some bits
                     * as much as we need after lzh_br_read_ahead()
                     * failed. */
                    if (!lzh_br_read_ahead(strm, &bre, lt_max_bits)) {
                        if (!last) goto next_data;
                        /* Remaining bits are less than
                         * maximum bits(lt.max_bits) but maybe
                         * it still remains as much as we need,
                         * so we should try to use it with
                         * dummy bits. */
                        c = lzh_decode_huffman(lt, lzh_br_bits_forced(&bre, lt_max_bits));
                        lzh_br_consume(&bre, lt_bitlen[c]);
                        if (!lzh_br_has(&bre, 0)) goto failed; /* Over read. */
                    } else {
                        c = lzh_decode_huffman(lt, lzh_br_bits(&bre, lt_max_bits));
                        lzh_br_consume(&bre, lt_bitlen[c]);
                    }
                    blocks_avail--;
                    if (c > UCHAR_MAX) /* Current block is a match data. */
                        break;
                    /*
                     * 'c' is exactly a literal code.
                     */
                    /* Save a decoded code to reference it
                     * afterward. */
                    w_buff[w_pos] = c;
                    if (++w_pos >= w_size) {
                        w_pos = 0;
                        lzh_emit_window(strm, w_size);
                        goto next_data;
                    }
                }
                /* 'c' is the length of a match pattern we have
                 * already extracted, which has be stored in
                 * window(ds->w_buff). */
                copy_len = c - (UCHAR_MAX + 1) + LZH_MINMATCH;
                /* FALL THROUGH */
            case ST_GET_POS_1:
                /*
                 * Get a reference position.
                 */
                if (!lzh_br_read_ahead(strm, &bre, pt_max_bits)) {
                    if (!last) {
                        state = ST_GET_POS_1;
                        ds->copy_len = copy_len;
                        goto next_data;
                    }
                    copy_pos = lzh_decode_huffman(pt, lzh_br_bits_forced(&bre, pt_max_bits));
                    lzh_br_consume(&bre, pt_bitlen[copy_pos]);
                    if (!lzh_br_has(&bre, 0)) goto failed; /* Over read. */
                } else {
                    copy_pos = lzh_decode_huffman(pt, lzh_br_bits(&bre, pt_max_bits));
                    lzh_br_consume(&bre, pt_bitlen[copy_pos]);
                }
                /* FALL THROUGH */
            case ST_GET_POS_2:
                if (copy_pos > 1) {
                    /* We need an additional adjustment number to
                     * the position. */
                    qint32 p = copy_pos - 1;
                    if (!lzh_br_bit_count_is_valid(p)) goto failed;
                    if (!lzh_br_read_ahead(strm, &bre, p)) {
                        if (last) goto failed; /* Truncated data.*/
                        state = ST_GET_POS_2;
                        ds->copy_len = copy_len;
                        ds->copy_pos = copy_pos;
                        goto next_data;
                    }
                    copy_pos = (1 << p) + lzh_br_bits(&bre, p);
                    lzh_br_consume(&bre, p);
                }
                /* The position is actually a distance from the last
                 * code we had extracted and thus we have to convert
                 * it to a position of the window. */
                copy_pos = (w_pos - copy_pos - 1) & w_mask;
                /* FALL THROUGH */
            case ST_COPY_DATA:
                /*
                 * Copy `copy_len' bytes as extracted data from
                 * the window into the output buffer.
                 */
                for (;;) {
                    qint32 l;

                    l = copy_len;
                    if (copy_pos > w_pos) {
                        if (l > w_size - copy_pos) l = w_size - copy_pos;
                    } else {
                        if (l > w_size - w_pos) l = w_size - w_pos;
                    }
                    if ((copy_pos + l < w_pos) || (w_pos + l < copy_pos)) {
                        /* No overlap. */
                        memcpy(w_buff + w_pos, w_buff + copy_pos, l);
                    } else {
                        const quint8 *s;
                        quint8 *d;
                        qint32 li;

                        d = w_buff + w_pos;
                        s = w_buff + copy_pos;
                        for (li = 0; li < l - 1;) {
                            d[li] = s[li];
                            li++;
                            d[li] = s[li];
                            li++;
                        }
                        if (li < l) d[li] = s[li];
                    }
                    w_pos += l;
                    if (w_pos == w_size) {
                        w_pos = 0;
                        lzh_emit_window(strm, w_size);
                        if (copy_len <= l) state = ST_GET_LITERAL;
                        else {
                            state = ST_COPY_DATA;
                            ds->copy_len = copy_len - l;
                            ds->copy_pos = (copy_pos + l) & w_mask;
                        }
                        goto next_data;
                    }
                    if (copy_len <= l) /* A copy of current pattern ended. */
                        break;
                    copy_len -= l;
                    copy_pos = (copy_pos + l) & w_mask;
                }
                state = ST_GET_LITERAL;
                break;
        }
    }
failed:
    return (ds->error = LZH_ARCHIVE_FAILED);
next_data:
    ds->br = bre;
    ds->blocks_avail = blocks_avail;
    ds->state = state;
    ds->w_pos = w_pos;
    return (LZH_ARCHIVE_OK);
}

qint32 XLZHDecoder::lzh_br_fillup(lzh_stream *strm, lzh_br *br)
{
    if ((strm == nullptr) || (br == nullptr) || (br->cache_avail < 0) || (br->cache_avail > (qint32)CACHE_BITS) || (strm->avail_in < 0) ||
        ((strm->avail_in > 0) && (strm->next_in == nullptr))) {
        return 0;
    }

    qint32 n = CACHE_BITS - br->cache_avail;

    for (;;) {
        const qint32 x = n >> 3;
        if (strm->avail_in >= x) {
            switch (x) {
                case 8:
                    br->cache_buffer = ((quint64)strm->next_in[0]) << 56 | ((quint64)strm->next_in[1]) << 48 | ((quint64)strm->next_in[2]) << 40 |
                                       ((quint64)strm->next_in[3]) << 32 | ((quint64)strm->next_in[4]) << 24 | ((quint64)strm->next_in[5]) << 16 |
                                       ((quint64)strm->next_in[6]) << 8 | (quint64)strm->next_in[7];
                    strm->next_in += 8;
                    strm->avail_in -= 8;
                    br->cache_avail += 8 * 8;
                    return (1);
                case 7:
                    br->cache_buffer = (br->cache_buffer << 56) | ((quint64)strm->next_in[0]) << 48 | ((quint64)strm->next_in[1]) << 40 |
                                       ((quint64)strm->next_in[2]) << 32 | ((quint64)strm->next_in[3]) << 24 | ((quint64)strm->next_in[4]) << 16 |
                                       ((quint64)strm->next_in[5]) << 8 | (quint64)strm->next_in[6];
                    strm->next_in += 7;
                    strm->avail_in -= 7;
                    br->cache_avail += 7 * 8;
                    return (1);
                case 6:
                    br->cache_buffer = (br->cache_buffer << 48) | ((quint64)strm->next_in[0]) << 40 | ((quint64)strm->next_in[1]) << 32 |
                                       ((quint64)strm->next_in[2]) << 24 | ((quint64)strm->next_in[3]) << 16 | ((quint64)strm->next_in[4]) << 8 |
                                       (quint64)strm->next_in[5];
                    strm->next_in += 6;
                    strm->avail_in -= 6;
                    br->cache_avail += 6 * 8;
                    return (1);
                case 0:
                    /* We have enough compressed data in
                     * the cache buffer.*/
                    return (1);
                default: break;
            }
        }
        if (strm->avail_in == 0) {
            /* There is not enough compressed data to fill up the
             * cache buffer. */
            return (0);
        }
        br->cache_buffer = (br->cache_buffer << 8) | *strm->next_in++;
        strm->avail_in--;
        br->cache_avail += 8;
        n -= 8;
    }
}

void XLZHDecoder::lzh_emit_window(lzh_stream *strm, size_t s)
{
    strm->ref_ptr = strm->ds->w_buff;
    strm->avail_out = static_cast<int>(s);
    strm->total_out += s;
}

qint32 XLZHDecoder::lzh_decode_huffman_tree(lzh_huffman *hf, unsigned int rbits, qint32 c)
{
    struct lzh_htree_t *ht;
    qint32 extlen;

    ht = hf->tree;
    extlen = hf->shift_bits;
    while (c >= hf->len_avail) {
        c -= hf->len_avail;
        if (extlen-- <= 0 || c >= hf->tree_used) return (0);
        if (rbits & (1U << extlen)) c = ht[c].left;
        else c = ht[c].right;
    }
    return (c);
}

qint32 XLZHDecoder::lzh_decode_huffman(lzh_huffman *hf, unsigned int rbits)
{
    qint32 c;
    /*
     * At first search an index table for a bit pattern.
     * If it fails, search a huffman tree for.
     */
    c = hf->tbl[rbits >> hf->shift_bits];
    if (c < hf->len_avail || hf->len_avail == 0) return (c);
    /* This bit pattern needs to be found out at a huffman tree. */
    return (lzh_decode_huffman_tree(hf, rbits, c));
}

qint32 XLZHDecoder::lzh_make_fake_table(lzh_huffman *hf, quint16 c)
{
    if (c >= hf->len_size) return (0);
    hf->tbl[0] = c;
    hf->max_bits = 0;
    hf->shift_bits = 0;
    hf->bitlen[hf->tbl[0]] = 0;
    return (1);
}

qint32 XLZHDecoder::lzh_read_pt_bitlen(lzh_stream *strm, qint32 start, qint32 end)
{
    struct lzh_dec *ds = strm->ds;
    struct lzh_br *br = &(ds->br);
    qint32 c, i;

    for (i = start; i < end;) {
        /*
         *  bit pattern     the number we need
         *     000           ->  0
         *     001           ->  1
         *     010           ->  2
         *     ...
         *     110           ->  6
         *     1110          ->  7
         *     11110         ->  8
         *     ...
         *     1111111111110 ->  16
         */
        if (!lzh_br_read_ahead(strm, br, 3)) return (i);
        if ((c = lzh_br_bits(br, 3)) == 7) {
            if (!lzh_br_read_ahead(strm, br, 13)) return (i);
            c = bitlen_tbl[lzh_br_bits(br, 13) & 0x3FF];
            if (c) lzh_br_consume(br, c - 3);
            else return (-1); /* Invalid data. */
        } else lzh_br_consume(br, 3);
        ds->pt.bitlen[i++] = c;
        ds->pt.freq[c]++;
    }
    return (i);
}

qint32 XLZHDecoder::lzh_make_huffman_table(lzh_huffman *hf)
{
    quint16 *tbl;
    const quint8 *bitlen;
    qint32 bitptn[17], weight[17];
    qint32 i, maxbits = 0, ptn, tbl_size, w;
    qint32 diffbits, len_avail;

    /*
     * Initialize bit patterns.
     */
    ptn = 0;
    for (i = 1, w = 1 << 15; i <= 16; i++, w >>= 1) {
        bitptn[i] = ptn;
        weight[i] = w;
        if (hf->freq[i]) {
            ptn += hf->freq[i] * w;
            maxbits = i;
        }
    }
    if (ptn != 0x10000 || maxbits > hf->tbl_bits) return (0); /* Invalid */

    hf->max_bits = maxbits;

    /*
     * Cut out extra bits which we won't house in the table.
     * This preparation reduces the same calculation in the for-loop
     * making the table.
     */
    if (maxbits < 16) {
        qint32 ebits = 16 - maxbits;
        for (i = 1; i <= maxbits; i++) {
            bitptn[i] >>= ebits;
            weight[i] >>= ebits;
        }
    }
    if (maxbits > LZH_HTBL_BITS) {
        unsigned htbl_max;
        quint16 *p;

        diffbits = maxbits - LZH_HTBL_BITS;
        for (i = 1; i <= LZH_HTBL_BITS; i++) {
            bitptn[i] >>= diffbits;
            weight[i] >>= diffbits;
        }
        htbl_max = bitptn[LZH_HTBL_BITS] + weight[LZH_HTBL_BITS] * hf->freq[LZH_HTBL_BITS];
        p = &(hf->tbl[htbl_max]);
        while (p < &hf->tbl[1U << LZH_HTBL_BITS]) *p++ = 0;
    } else diffbits = 0;
    hf->shift_bits = diffbits;

    /*
     * Make the table.
     */
    tbl_size = 1 << LZH_HTBL_BITS;
    tbl = hf->tbl;
    bitlen = hf->bitlen;
    len_avail = hf->len_avail;
    hf->tree_used = 0;
    for (i = 0; i < len_avail; i++) {
        quint16 *p;
        qint32 len, cnt;
        quint16 bit;
        qint32 extlen;
        struct lzh_htree_t *ht;

        if (bitlen[i] == 0) continue;
        /* Get a bit pattern */
        len = bitlen[i];
        ptn = bitptn[len];
        cnt = weight[len];
        if (len <= LZH_HTBL_BITS) {
            /* Calculate next bit pattern */
            if ((bitptn[len] = ptn + cnt) > tbl_size) return (0); /* Invalid */
            /* Update the table */
            p = &(tbl[ptn]);
            if (cnt > 7) {
                quint16 *pc;

                cnt -= 8;
                pc = &p[cnt];
                pc[0] = (quint16)i;
                pc[1] = (quint16)i;
                pc[2] = (quint16)i;
                pc[3] = (quint16)i;
                pc[4] = (quint16)i;
                pc[5] = (quint16)i;
                pc[6] = (quint16)i;
                pc[7] = (quint16)i;
                if (cnt > 7) {
                    cnt -= 8;
                    memcpy(&p[cnt], pc, 8 * sizeof(quint16));
                    pc = &p[cnt];
                    while (cnt > 15) {
                        cnt -= 16;
                        memcpy(&p[cnt], pc, 16 * sizeof(quint16));
                    }
                }
                if (cnt) memcpy(p, pc, cnt * sizeof(quint16));
            } else {
                while (cnt > 1) {
                    p[--cnt] = (quint16)i;
                    p[--cnt] = (quint16)i;
                }
                if (cnt) p[--cnt] = (quint16)i;
            }
            continue;
        }

        /*
         * A bit length is too big to be housed to a direct table,
         * so we use a tree model for its extra bits.
         */
        bitptn[len] = ptn + cnt;
        bit = 1U << (diffbits - 1);
        extlen = len - LZH_HTBL_BITS;

        p = &(tbl[ptn >> diffbits]);
        if (*p == 0) {
            *p = len_avail + hf->tree_used;
            ht = &(hf->tree[hf->tree_used++]);
            if (hf->tree_used > hf->tree_avail) return (0); /* Invalid */
            ht->left = 0;
            ht->right = 0;
        } else {
            if (*p < len_avail || *p >= (len_avail + hf->tree_used)) return (0); /* Invalid */
            ht = &(hf->tree[*p - len_avail]);
        }
        while (--extlen > 0) {
            if (ptn & bit) {
                if (ht->left < len_avail) {
                    ht->left = len_avail + hf->tree_used;
                    ht = &(hf->tree[hf->tree_used++]);
                    if (hf->tree_used > hf->tree_avail) return (0); /* Invalid */
                    ht->left = 0;
                    ht->right = 0;
                } else {
                    ht = &(hf->tree[ht->left - len_avail]);
                }
            } else {
                if (ht->right < len_avail) {
                    ht->right = len_avail + hf->tree_used;
                    ht = &(hf->tree[hf->tree_used++]);
                    if (hf->tree_used > hf->tree_avail) return (0); /* Invalid */
                    ht->left = 0;
                    ht->right = 0;
                } else {
                    ht = &(hf->tree[ht->right - len_avail]);
                }
            }
            bit >>= 1;
        }
        if (ptn & bit) {
            if (ht->left != 0) return (0); /* Invalid */
            ht->left = (quint16)i;
        } else {
            if (ht->right != 0) return (0); /* Invalid */
            ht->right = (quint16)i;
        }
    }
    return (1);
}

void XLZHDecoder::lzh_decode_free(lzh_stream *strm)
{
    if (strm->ds == nullptr) return;
    free(strm->ds->w_buff);
    lzh_huffman_free(&(strm->ds->lt));
    lzh_huffman_free(&(strm->ds->pt));
    free(strm->ds);
    strm->ds = nullptr;
}

void XLZHDecoder::lzh_huffman_free(lzh_huffman *hf)
{
    free(hf->bitlen);
    free(hf->tbl);
    free(hf->tree);
}

bool XLZHDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct)
{
    return decompress(pDecompressState, nMethod, pPdStruct, TERMINATION_PHYSICAL_EOF);
}

bool XLZHDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct, TERMINATION_MODE terminationMode)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (nMethod == 1) {  // -lh1- has its own algorithm (LZHUF), not the lh4/5/6/7 state machine
        return decompressLh1(pDecompressState, pPdStruct);
    }

    const bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    const qint64 nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    if (bHasExpectedSize && (nExpectedSize < 0)) {
        return false;
    }

    const qint32 N_BUFFER_SIZE = 0x10000;  // 64KB buffer

    quint8 *pBufferIn = new (std::nothrow) quint8[N_BUFFER_SIZE];

    if (!pBufferIn) {
        return false;
    }

    if ((pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1)) {
        delete[] pBufferIn;
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    lzh_stream strm = {};
    qint32 nResult = LZH_ARCHIVE_OK;
    bool bSizeError = false;
    bool bReachedEOF = false;

    if (!lzh_decode_init(&strm, nMethod, terminationMode)) {
        lzh_decode_free(&strm);
        delete[] pBufferIn;
        return false;
    }

    qint64 nBytesProcessed = 0;
    qint64 nInputLimit = pDecompressState->nInputLimit;

    while (((nInputLimit == -1) || (nBytesProcessed < nInputLimit)) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nReadSize = (nInputLimit == -1) ? N_BUFFER_SIZE : (qint32)qMin(nInputLimit - nBytesProcessed, (qint64)N_BUFFER_SIZE);
        qint32 nBytesRead = XBinary::_readDevice(reinterpret_cast<char *>(pBufferIn), nReadSize, pDecompressState);

        if (nBytesRead <= 0) {
            break;
        }

        strm.next_in = pBufferIn;
        strm.avail_in = nBytesRead;
        strm.total_in = 0;

        while ((strm.avail_in > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            nResult = lzh_decode(&strm, false);

            if (nResult == LZH_ARCHIVE_FAILED) {
                pDecompressState->bReadError = true;
                break;
            }

            if (strm.avail_out > 0) {
                const qint32 nProduced = static_cast<qint32>(strm.avail_out);
                if (bHasExpectedSize && ((pDecompressState->nCountOutput > nExpectedSize) || ((qint64)nProduced > (nExpectedSize - pDecompressState->nCountOutput)))) {
                    bSizeError = true;
                    break;
                }
                if (XBinary::_writeDevice(reinterpret_cast<const char *>(strm.ref_ptr), nProduced, pDecompressState) != nProduced) {
                    break;
                }
                strm.avail_out = 0;
            }

            if (nResult == LZH_ARCHIVE_EOF) {
                bReachedEOF = true;
                break;
            }
        }

        nBytesProcessed += nBytesRead;

        if (bReachedEOF || pDecompressState->bReadError || pDecompressState->bWriteError || bSizeError || nResult == LZH_ARCHIVE_FAILED) {
            break;
        }
    }

    // Flush any remaining decoded data from the sliding window
    if (XBinary::isPdStructNotCanceled(pPdStruct) && !pDecompressState->bReadError && !pDecompressState->bWriteError && !bSizeError && nResult != LZH_ARCHIVE_FAILED &&
        !bReachedEOF) {
        strm.avail_in = 0;
        nResult = lzh_decode(&strm, true);

        if (nResult == LZH_ARCHIVE_FAILED) {
            pDecompressState->bReadError = true;
        } else if (strm.avail_out > 0) {
            const qint32 nProduced = static_cast<qint32>(strm.avail_out);
            if (bHasExpectedSize && ((pDecompressState->nCountOutput > nExpectedSize) || ((qint64)nProduced > (nExpectedSize - pDecompressState->nCountOutput)))) {
                bSizeError = true;
            } else if (XBinary::_writeDevice(reinterpret_cast<const char *>(strm.ref_ptr), nProduced, pDecompressState) == nProduced) {
                strm.avail_out = 0;
            }
        }
        if (nResult == LZH_ARCHIVE_EOF) {
            bReachedEOF = true;
        }
    }

    const bool bExplicitEOFSeen = strm.ds && strm.ds->bExplicitEOFSeen;

    lzh_decode_free(&strm);

    delete[] pBufferIn;

    const bool bInputComplete =
        (nInputLimit == -1) ? ((terminationMode != TERMINATION_ZERO_BLOCK) || pDecompressState->pDeviceInput->atEnd()) : (pDecompressState->nCountInput == nInputLimit);
    const bool bOutputSizeMatches = !bHasExpectedSize || (pDecompressState->nCountOutput == nExpectedSize);
    const bool bTerminationMatches = (terminationMode != TERMINATION_ZERO_BLOCK) || (bReachedEOF && bExplicitEOFSeen);
    return bInputComplete && XBinary::isPdStructNotCanceled(pPdStruct) && !pDecompressState->bReadError && !pDecompressState->bWriteError && !bSizeError &&
           bOutputSizeMatches && bTerminationMatches && nResult != LZH_ARCHIVE_FAILED;
}

namespace {
// LZHUF decoder for LHA -lh1- (LArc-compatible): a 4 KiB sliding dictionary (LZSS) whose
// literal/length symbols are coded with an adaptive (self-adjusting) Huffman tree, and whose
// match positions are coded with a fixed table. This is the classic Okumura/Yoshizaki scheme
// that the block-based lh4/5/6/7 decoders later replaced. All state is local to this struct.
struct Lzhuf {
    static const int N = 4096;                        // ring buffer size
    static const int LZF = 60;                        // upper limit for match length
    static const int THRESHOLD = 2;                   // encode matches only when longer than this
    static const int N_CHAR = 256 - THRESHOLD + LZF;  // 314 kinds of {literal, length} symbols
    static const int T = N_CHAR * 2 - 1;              // 627 nodes in the Huffman tree
    static const int R = T - 1;                       // 626 root position
    static const int MAX_FREQ = 0x8000;               // tree is rebuilt when the root freq reaches this

    const quint8 *in;
    qint64 inSize;
    qint64 inPos;
    qint64 bitsConsumed;
    qint32 paddingReads;
    bool inputError;
    quint16 getbuf;
    qint32 getlen;

    int freq[T + 1];
    int prnt[T + N_CHAR];
    int son[T];
    quint8 dLen[256];
    quint8 dCode[256];
    quint8 textBuf[N];

    Lzhuf(const quint8 *pIn, qint64 nInSize) : in(pIn), inSize(nInSize), inPos(0), bitsConsumed(0), paddingReads(0), inputError(false), getbuf(0), getlen(0)
    {
        // Fixed position-code tables: dLen[i] = number of leading bits of the 8-bit prefix i that
        // form the position's high-6-bit symbol dCode[i]. Canonical LZHUF layout: 1 symbol of
        // length 3, 3 of length 4, 8 of 5, 12 of 6, 24 of 7, 16 of 8 (64 symbols, 256 prefixes).
        const int nSyms[6] = {1, 3, 8, 12, 24, 16};
        int nIdx = 0, nSym = 0;
        for (int nLen = 3; nLen <= 8; nLen++) {
            int nSpan = 1 << (8 - nLen);
            for (int s = 0; s < nSyms[nLen - 3]; s++) {
                for (int k = 0; k < nSpan; k++) {
                    dLen[nIdx] = (quint8)nLen;
                    dCode[nIdx] = (quint8)nSym;
                    nIdx++;
                }
                nSym++;
            }
        }
        _startHuff();
    }

    int _getBit()
    {
        while (getlen <= 8) {
            quint8 c = 0;
            if (inPos < inSize) {
                c = in[inPos++];
            } else if (paddingReads < 2) {
                // The classic LZHUF reader keeps a 16-bit look-ahead and can
                // request up to two zero padding bytes after the final coded
                // bit. More than that is a genuinely truncated stream.
                paddingReads++;
            } else {
                inputError = true;
            }
            getbuf = (quint16)(getbuf | (c << (8 - getlen)));
            getlen += 8;
        }
        int x = (getbuf >> 15) & 1;
        getbuf = (quint16)(getbuf << 1);
        getlen -= 1;
        bitsConsumed++;
        if (bitsConsumed > (inSize * 8)) {
            inputError = true;
        }
        return x;
    }

    int _getByte()
    {
        while (getlen <= 8) {
            quint8 c = 0;
            if (inPos < inSize) {
                c = in[inPos++];
            } else if (paddingReads < 2) {
                paddingReads++;
            } else {
                inputError = true;
            }
            getbuf = (quint16)(getbuf | (c << (8 - getlen)));
            getlen += 8;
        }
        int x = (getbuf >> 8) & 0xFF;
        getbuf = (quint16)(getbuf << 8);
        getlen -= 8;
        bitsConsumed += 8;
        if (bitsConsumed > (inSize * 8)) {
            inputError = true;
        }
        return x;
    }

    void _startHuff()
    {
        for (int i = 0; i < N_CHAR; i++) {
            freq[i] = 1;
            son[i] = i + T;
            prnt[i + T] = i;
        }
        int i = 0, j = N_CHAR;
        while (j <= R) {
            freq[j] = freq[i] + freq[i + 1];
            son[j] = i;
            prnt[i] = prnt[i + 1] = j;
            i += 2;
            j++;
        }
        freq[T] = 0xFFFF;
        prnt[R] = 0;
    }

    void _reconst()
    {
        // Collect leaf nodes, halving their frequencies, then rebuild the tree bottom-up.
        int j = 0;
        for (int i = 0; i < T; i++) {
            if (son[i] >= T) {
                freq[j] = (freq[i] + 1) / 2;
                son[j] = son[i];
                j++;
            }
        }
        for (int i = 0, k = N_CHAR; k < T; i += 2, k++) {
            int f = freq[k] = freq[i] + freq[i + 1];
            int l = k - 1;
            while (f < freq[l]) l--;
            l++;
            for (int m = k; m > l; m--) {
                freq[m] = freq[m - 1];
                son[m] = son[m - 1];
            }
            freq[l] = f;
            son[l] = i;
        }
        for (int i = 0; i < T; i++) {
            int k = son[i];
            if (k >= T) {
                prnt[k] = i;
            } else {
                prnt[k] = prnt[k + 1] = i;
            }
        }
    }

    void _update(int c)
    {
        if (freq[R] == MAX_FREQ) {
            _reconst();
        }
        c = prnt[c + T];
        do {
            int k = ++freq[c];
            int l = c + 1;
            // Keep the frequency list ordered: if node c now outweighs its successor, swap.
            if (k > freq[l]) {
                while (k > freq[l + 1]) l++;
                freq[c] = freq[l];
                freq[l] = k;
                int i = son[c];
                prnt[i] = l;
                if (i < T) prnt[i + 1] = l;
                int j = son[l];
                son[l] = i;
                prnt[j] = c;
                if (j < T) prnt[j + 1] = c;
                son[c] = j;
                c = l;
            }
            c = prnt[c];
        } while (c != 0);
    }

    int _decodeChar()
    {
        int c = son[R];
        // Walk the tree from the root, one bit per step, until a leaf (>= T) is reached.
        while (c < T) {
            c += _getBit();
            c = son[c];
        }
        c -= T;
        _update(c);
        return c;
    }

    int _decodePosition()
    {
        int i = _getByte();
        int c = (int)dCode[i] << 6;
        int j = dLen[i] - 2;
        while (j-- > 0) {
            i = ((i << 1) + _getBit());
        }
        return c | (i & 0x3F);
    }

    bool decode(QByteArray *pOut, qint64 nTextSize, XBinary::PDSTRUCT *pPdStruct)
    {
        if (!pOut || (nTextSize < 0) || (nTextSize > LH1_MAX_UNPACKED_BUFFER_SIZE)) {
            return false;
        }

        for (int i = 0; i < N - LZF; i++) textBuf[i] = ' ';
        int r = N - LZF;
        qint64 count = 0;
        // Allocate the exact result once. nTextSize is validated above against
        // LH1_MAX_UNPACKED_BUFFER_SIZE, so the allocation size is bounded.
        // This also avoids growth reallocations while decoding.
        pOut->resize((int)nTextSize);
        while (count < nTextSize) {
            if (((count & 0x3FFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }

            int c = _decodeChar();
            if (inputError) {
                return false;
            }
            if (c < 256) {
                (*pOut)[(int)count] = (char)c;
                textBuf[r++] = (quint8)c;
                r &= (N - 1);
                count++;
            } else {
                int pos = _decodePosition();
                if (inputError) {
                    return false;
                }
                int i = (r - pos - 1) & (N - 1);
                int j = c - 255 + THRESHOLD;
                if ((qint64)j > (nTextSize - count)) {
                    return false;
                }
                for (int k = 0; k < j; k++) {
                    quint8 cc = textBuf[(i + k) & (N - 1)];
                    (*pOut)[(int)count] = (char)cc;
                    textBuf[r++] = cc;
                    r &= (N - 1);
                    count++;
                }
            }
        }

        return true;
    }
};
}  // namespace

bool XLZHDecoder::decompressLh1(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (!pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        return false;
    }

    qint64 nTextSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    if ((nTextSize < 0) || (nTextSize > LH1_MAX_UNPACKED_BUFFER_SIZE) || (pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1) ||
        (pDecompressState->nInputLimit > LH1_MAX_PACKED_BUFFER_SIZE)) {
        return false;
    }

    // LH1 is a whole-buffer decoder: compressed bytes and the complete output
    // coexist.  Unknown input extents reserve the decoder's hard maximum so a
    // sequential source cannot grow past the process-wide aggregate budget.
    const qint64 nPackedReservation = (pDecompressState->nInputLimit >= 0) ? pDecompressState->nInputLimit : LH1_MAX_PACKED_BUFFER_SIZE;
    if (nPackedReservation > (std::numeric_limits<qint64>::max)() - nTextSize) {
        return false;
    }
    XBinary::UNPACK_MEMORY_RESERVATION memoryReservation;
    if (!memoryReservation.acquire(pDecompressState->mapUnpackProperties, nPackedReservation + nTextSize)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    // -lh1- files use a 4 KiB window. Read the bounded compressed member with
    // exact short-read handling before running the adaptive decoder.
    QByteArray baIn;
    // nInputLimit is validated above against LH1_MAX_PACKED_BUFFER_SIZE, so the
    // reservation size is bounded.
    if (pDecompressState->nInputLimit > 0) {
        baIn.reserve((int)pDecompressState->nInputLimit);
    }
    std::unique_ptr<char[]> pReadBuffer(new (std::nothrow) char[LH1_READ_BUFFER_SIZE]);
    if (!pReadBuffer) {
        return false;
    }
    while (((pDecompressState->nInputLimit == -1) || (pDecompressState->nCountInput < pDecompressState->nInputLimit)) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((pDecompressState->nInputLimit == -1) && (baIn.size() >= LH1_MAX_PACKED_BUFFER_SIZE)) {
            // At the cap, probe only for EOF. A further byte proves this member
            // cannot be represented by the bounded whole-buffer implementation.
            char nExtraByte = 0;
            const qint32 nExtraRead = XBinary::_readDevice(&nExtraByte, 1, pDecompressState);
            if (nExtraRead != 0) {
                return false;
            }
            break;
        }

        const qint32 nReadSize = Algo_utils::getReadChunkSize(pDecompressState, LH1_READ_BUFFER_SIZE);
        if (nReadSize <= 0) {
            break;
        }
        const qint32 nRead = XBinary::_readDevice(pReadBuffer.get(), nReadSize, pDecompressState);
        if (nRead <= 0) {
            break;
        }
        if ((qint64)nRead > (LH1_MAX_PACKED_BUFFER_SIZE - baIn.size())) {
            return false;
        }
        // The check above keeps baIn within LH1_MAX_PACKED_BUFFER_SIZE.
        baIn.append(pReadBuffer.get(), nRead);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || pDecompressState->bReadError ||
        ((pDecompressState->nInputLimit != -1) && (pDecompressState->nCountInput != pDecompressState->nInputLimit))) {
        return false;
    }

    if (baIn.isEmpty() && (nTextSize != 0)) {
        return false;
    }

    QByteArray baOut;
    std::unique_ptr<Lzhuf> pDecoder(new (std::nothrow) Lzhuf(reinterpret_cast<const quint8 *>(baIn.constData()), baIn.size()));
    if (!pDecoder || !pDecoder->decode(&baOut, nTextSize, pPdStruct)) {
        return false;
    }

    // The final byte may contain at most seven container padding bits.  Whole
    // unused bytes mean the declared compressed member contains trailing data
    // and must not be accepted as an exact LH1 stream.
    const qint64 nTotalInputBits = (qint64)baIn.size() * 8;
    if ((pDecoder->bitsConsumed > nTotalInputBits) || ((nTotalInputBits - pDecoder->bitsConsumed) >= 8)) {
        return false;
    }

    return (XBinary::_writeDevice(baOut.data(), baOut.size(), pDecompressState) == baOut.size()) && XBinary::isPdStructNotCanceled(pPdStruct) &&
           !pDecompressState->bWriteError;
}
