/* Copyright (c) 2023-2025 hors<horsicq@gmail.com>
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

#include "xcompress.h"

static const quint16 cache_masks[] = {0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF,
                                      0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

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

#define RAR_STARTL1 2
static uint RAR_DecL1[] = {0x8000, 0xa000, 0xc000, 0xd000, 0xe000, 0xea00, 0xee00, 0xf000, 0xf200, 0xf200, 0xffff};
static uint RAR_PosL1[] = {0, 0, 0, 2, 3, 5, 7, 11, 16, 20, 24, 32, 32};

#define RAR_STARTL2 3
static uint RAR_DecL2[] = {0xa000, 0xc000, 0xd000, 0xe000, 0xea00, 0xee00, 0xf000, 0xf200, 0xf240, 0xffff};
static uint RAR_PosL2[] = {0, 0, 0, 0, 5, 7, 9, 13, 18, 22, 26, 34, 36};

#define RAR_STARTHF0 4
static uint RAR_DecHf0[] = {0x8000, 0xc000, 0xe000, 0xf200, 0xf200, 0xf200, 0xf200, 0xf200, 0xffff};
static uint RAR_PosHf0[] = {0, 0, 0, 0, 0, 8, 16, 24, 33, 33, 33, 33, 33};

#define RAR_STARTHF1 5
static uint RAR_DecHf1[] = {0x2000, 0xc000, 0xe000, 0xf000, 0xf200, 0xf200, 0xf7e0, 0xffff};
static uint RAR_PosHf1[] = {0, 0, 0, 0, 0, 0, 4, 44, 60, 76, 80, 80, 127};

#define RAR_STARTHF2 5
static uint RAR_DecHf2[] = {0x1000, 0x2400, 0x8000, 0xc000, 0xfa00, 0xffff, 0xffff, 0xffff};
static uint RAR_PosHf2[] = {0, 0, 0, 0, 0, 0, 2, 7, 53, 117, 233, 0, 0};

#define RAR_STARTHF3 6
static uint RAR_DecHf3[] = {0x800, 0x2400, 0xee00, 0xfe80, 0xffff, 0xffff, 0xffff};
static uint RAR_PosHf3[] = {0, 0, 0, 0, 0, 0, 0, 2, 16, 218, 251, 0, 0};

#define RAR_STARTHF4 8
static uint RAR_DecHf4[] = {0xff00, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
static uint RAR_PosHf4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0};

XCompress::XCompress()
{
}

bool XCompress::lzh_decode_init(lzh_stream *strm, qint32 method)
{
    struct lzh_dec *ds;
    qint32 w_bits, w_size;

    if (strm->ds == NULL) {
        strm->ds = (lzh_dec *)calloc(1, sizeof(*strm->ds));
    }
    ds = strm->ds;

    switch (method) {
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
    /* Expand a window size up to 128 KiB for decompressing process
     * performance whatever its original window size is. */
    ds->w_size = 1U << 17;
    ds->w_mask = ds->w_size - 1;
    if (ds->w_buff == NULL) {
        ds->w_buff = (quint8 *)malloc(ds->w_size);
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

    lzh_huffman_init(&(ds->lt), LZH_LT_BITLEN_SIZE, 16);
    lzh_huffman_init(&(ds->pt), LZH_PT_BITLEN_SIZE, 16);
    ds->lt.len_bits = 9;

    ds->error = 0;

    return true;
}

bool XCompress::lzh_huffman_init(lzh_huffman *hf, size_t len_size, qint32 tbl_bits)
{
    qint32 bits;

    if (hf->bitlen == NULL) {
        hf->bitlen = (quint8 *)malloc(len_size * sizeof(hf->bitlen[0]));
    }
    if (hf->tbl == NULL) {
        if (tbl_bits < LZH_HTBL_BITS) bits = tbl_bits;
        else bits = LZH_HTBL_BITS;
        hf->tbl = (quint16 *)malloc(((size_t)1 << bits) * sizeof(hf->tbl[0]));
    }
    if (hf->tree == NULL && tbl_bits > LZH_HTBL_BITS) {
        hf->tree_avail = 1 << (tbl_bits - LZH_HTBL_BITS + 4);
        hf->tree = (lzh_htree_t *)malloc(hf->tree_avail * sizeof(hf->tree[0]));
    }
    hf->len_size = (int)len_size;
    hf->tbl_bits = tbl_bits;

    return true;
}

qint32 XCompress::lzh_decode(lzh_stream *strm, qint32 last)
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

qint32 XCompress::lzh_read_blocks(lzh_stream *strm, qint32 last)
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
                if (ds->blocks_avail == 0) goto failed;
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
                if (!lzh_br_read_ahead(strm, br, ds->pt.len_bits)) {
                    if (last) goto failed; /* Truncated data. */
                    ds->state = ST_RD_PT_1;
                    return (LZH_ARCHIVE_OK);
                }
                ds->pt.len_avail = lzh_br_bits(br, ds->pt.len_bits);
                lzh_br_consume(br, ds->pt.len_bits);
                /* FALL THROUGH */
            case ST_RD_PT_2:
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
                if (!lzh_br_read_ahead(strm, br, ds->lt.len_bits)) {
                    if (last) goto failed; /* Truncated data. */
                    ds->state = ST_RD_LITERAL_1;
                    return (LZH_ARCHIVE_OK);
                }
                ds->lt.len_avail = lzh_br_bits(br, ds->lt.len_bits);
                lzh_br_consume(br, ds->lt.len_bits);
                /* FALL THROUGH */
            case ST_RD_LITERAL_2:
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

qint32 XCompress::lzh_decode_blocks(lzh_stream *strm, qint32 last)
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

qint32 XCompress::lzh_br_fillup(lzh_stream *strm, lzh_br *br)
{
    qint32 n = CACHE_BITS - br->cache_avail;

    for (;;) {
        const qint32 x = n >> 3;
        if (strm->avail_in >= x) {
            switch (x) {
                case 8:
                    br->cache_buffer = ((quint64)strm->next_in[0]) << 56 | ((quint64)strm->next_in[1]) << 48 | ((quint64)strm->next_in[2]) << 40 |
                                       ((quint64)strm->next_in[3]) << 32 | ((quint32)strm->next_in[4]) << 24 | ((quint32)strm->next_in[5]) << 16 |
                                       ((quint32)strm->next_in[6]) << 8 | (quint32)strm->next_in[7];
                    strm->next_in += 8;
                    strm->avail_in -= 8;
                    br->cache_avail += 8 * 8;
                    return (1);
                case 7:
                    br->cache_buffer = (br->cache_buffer << 56) | ((quint64)strm->next_in[0]) << 48 | ((quint64)strm->next_in[1]) << 40 |
                                       ((quint64)strm->next_in[2]) << 32 | ((quint32)strm->next_in[3]) << 24 | ((quint32)strm->next_in[4]) << 16 |
                                       ((quint32)strm->next_in[5]) << 8 | (quint32)strm->next_in[6];
                    strm->next_in += 7;
                    strm->avail_in -= 7;
                    br->cache_avail += 7 * 8;
                    return (1);
                case 6:
                    br->cache_buffer = (br->cache_buffer << 48) | ((quint64)strm->next_in[0]) << 40 | ((quint64)strm->next_in[1]) << 32 |
                                       ((quint32)strm->next_in[2]) << 24 | ((quint32)strm->next_in[3]) << 16 | ((quint32)strm->next_in[4]) << 8 |
                                       (quint32)strm->next_in[5];
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

void XCompress::lzh_emit_window(lzh_stream *strm, size_t s)
{
    strm->ref_ptr = strm->ds->w_buff;
    strm->avail_out = (int)s;
    strm->total_out += s;
}

qint32 XCompress::lzh_decode_huffman_tree(lzh_huffman *hf, unsigned rbits, qint32 c)
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

qint32 XCompress::lzh_decode_huffman(lzh_huffman *hf, unsigned rbits)
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

qint32 XCompress::lzh_make_fake_table(lzh_huffman *hf, quint16 c)
{
    if (c >= hf->len_size) return (0);
    hf->tbl[0] = c;
    hf->max_bits = 0;
    hf->shift_bits = 0;
    hf->bitlen[hf->tbl[0]] = 0;
    return (1);
}

qint32 XCompress::lzh_read_pt_bitlen(lzh_stream *strm, qint32 start, qint32 end)
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

qint32 XCompress::lzh_make_huffman_table(lzh_huffman *hf)
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

void XCompress::lzh_decode_free(lzh_stream *strm)
{
    if (strm->ds == NULL) return;
    free(strm->ds->w_buff);
    lzh_huffman_free(&(strm->ds->lt));
    lzh_huffman_free(&(strm->ds->pt));
    free(strm->ds);
    strm->ds = NULL;
}

void XCompress::lzh_huffman_free(lzh_huffman *hf)
{
    free(hf->bitlen);
    free(hf->tbl);
    free(hf->tree);
}

void XCompress::rar_init(rar_stream *strm, quint64 WinSize, bool Solid)
{
    // Inp(true),VMCodeInp(true)

    strm->ExternalBuffer = false;
    // getbits*() attempt to read data from InAddr, ... InAddr+8 positions.
    // So let's allocate 8 additional bytes for situation, when we need to
    // read only 1 byte from the last position of buffer and avoid a crash
    // from access to next 8 bytes, which contents we do not need.
    size_t BufSize = RAR_BufferSize_MAX_SIZE + 8;
    strm->InBuf = new quint8[BufSize];

    // Ensure that we get predictable results when accessing bytes in area
    // not filled with read data.
    memset(strm->InBuf, 0, BufSize);

    strm->Window = NULL;
    strm->Fragmented = false;
    strm->Suspended = false;
    strm->UnpSomeRead = false;
    strm->ExtraDist = false;
    // strm->MaxUserThreads=1;
    // strm->UnpThreadPool=NULL;
    // strm->ReadBufMT=NULL;
    // strm->UnpThreadData=NULL;
    strm->AllocWinSize = 0;
    strm->MaxWinSize = 0;
    strm->MaxWinMask = 0;

    // Perform initialization, which should be done only once for all files.
    // It prevents crash if first unpacked file has the wrong "true" Solid flag,
    // so first DoUnpack call is made with the wrong "true" Solid value later.
    rar_UnpInitData(strm, false);
    // RAR 1.5 decompression initialization
    rar_UnpInitData15(strm, false);
    rar_InitHuff(strm);

    // void Unpack::Init(uint64 WinSize,bool Solid)

    // Minimum window size must be at least twice more than maximum possible
    // size of filter block, which is 0x10000 in RAR now. If window size is
    // smaller, we can have a block with never cleared flt->NextWindow flag
    // in UnpWriteBuf(). Minimum window size 0x20000 would be enough, but let's
    // use 0x40000 for extra safety and possible filter area size expansion.
    const size_t MinAllocSize = 0x40000;
    if (WinSize < MinAllocSize) WinSize = MinAllocSize;

    // if (WinSize>Min(0x10000000000ULL,RAR_UNPACK_MAX_DICT)) // Window size must not exceed 1 TB.
    //   throw std::bad_alloc();

    // 32-bit build can't unpack dictionaries exceeding 32-bit even in theory.
    // Also we've not verified if WrapUp and WrapDown work properly in 32-bit
    // version and >2GB dictionary and if 32-bit version can handle >2GB
    // distances. Since such version is unlikely to allocate >2GB anyway,
    // we prohibit >2GB dictionaries for 32-bit build here.
    // if (WinSize>0x80000000 && sizeof(size_t)<=4)
    //   throw std::bad_alloc();

    // Solid block shall use the same window size for all files.
    // But if Window isn't initialized when Solid is set, it means that
    // first file in solid block doesn't have the solid flag. We initialize
    // the window anyway for such malformed archive.
    // Non-solid files shall use their specific window sizes,
    // so current window size and unpack routine behavior doesn't depend on
    // previously unpacked files and their extraction order.
    if (!Solid || strm->Window == nullptr) {
        strm->MaxWinSize = (size_t)WinSize;
        strm->MaxWinMask = strm->MaxWinSize - 1;
    }

    // Use the already allocated window when processing non-solid files
    // with reducing dictionary sizes.
    if (WinSize <= strm->AllocWinSize) return;

    // Archiving code guarantees that window size does not grow in the same
    // solid stream. So if we are here, we are either creating a new window
    // or increasing the size of non-solid window. So we could safely reject
    // current window data without copying them to a new window.
    // if (Solid && (strm->Window!=NULL || Fragmented && WinSize>FragWindow.GetWinSize()))
    //   throw std::bad_alloc();

    // Alloc.delete_l<byte>(Window); // delete Window;
    // Window=nullptr;

    strm->Window = new quint8[WinSize];

    // try
    // {
    //   if (!Fragmented)
    //     Window=Alloc.new_l<byte>((size_t)WinSize,false); // Window=new byte[(size_t)WinSize];
    // }
    // catch (std::bad_alloc) // Use the fragmented window in this case.
    // {
    // }

    // if (Window==nullptr)
    //   if (WinSize<0x1000000 || sizeof(size_t)>4)
    //     throw std::bad_alloc(); // Exclude RAR4, small dictionaries and 64-bit.
    //   else
    //   {
    //     if (WinSize>FragWindow.GetWinSize())
    //       FragWindow.Init((size_t)WinSize);
    //     Fragmented=true;
    //   }

    if (!strm->Fragmented) {
        // Clean the window to generate the same output when unpacking corrupt
        // RAR files, which may access unused areas of sliding dictionary.
        // 2023.10.31: We've added FirstWinDone based unused area access check
        // in Unpack::CopyString(), so this memset might be unnecessary now.
        //    memset(Window,0,(size_t)WinSize);

        strm->AllocWinSize = WinSize;
    }
}

void XCompress::rar_InitHuff(rar_stream *strm)
{
    for (ushort I = 0; I < 256; I++) {
        strm->ChSet[I] = strm->ChSetB[I] = I << 8;
        strm->ChSetA[I] = I;
        strm->ChSetC[I] = ((~I + 1) & 0xff) << 8;
    }
    memset(strm->NToPl, 0, sizeof(strm->NToPl));
    memset(strm->NToPlB, 0, sizeof(strm->NToPlB));
    memset(strm->NToPlC, 0, sizeof(strm->NToPlC));
    rar_CorrHuff(strm, strm->ChSetB, strm->NToPlB);
}

void XCompress::rar_CorrHuff(struct rar_stream *strm, ushort *CharSet, quint8 *NumToPlace)
{
    int I, J;
    for (I = 7; I >= 0; I--)
        for (J = 0; J < 32; J++, CharSet++) *CharSet = (*CharSet & ~0xff) | I;
    memset(NumToPlace, 0, sizeof(strm->NToPl));
    for (I = 6; I >= 0; I--) NumToPlace[I] = (7 - I) * 32;
}

void XCompress::rar_UnpInitData(rar_stream *strm, bool Solid)
{
    if (!Solid) {
        strm->OldDist[0] = strm->OldDist[1] = strm->OldDist[2] = strm->OldDist[3] = (size_t)-1;

        strm->OldDistPtr = 0;

        strm->LastDist = (uint)-1;  // Initialize it to -1 like LastDist.
        strm->LastLength = 0;

        memset(&strm->BlockTables, 0, sizeof(strm->BlockTables));
        strm->UnpPtr = strm->WrPtr = 0;
        strm->PrevPtr = 0;
        strm->FirstWinDone = false;
        strm->WriteBorder = strm->MaxWinSize;

        if (strm->WriteBorder > RAR_UNPACK_MAX_WRITE) {
            strm->WriteBorder = RAR_UNPACK_MAX_WRITE;
        }
    }
    // Filters never share several solid files, so we can safely reset them
    // even in solid archive.
    strm->Filters.clear();

    strm->InAddr = 0;
    strm->InBit = 0;

    strm->WrittenFileSize = 0;
    strm->ReadTop = 0;
    strm->ReadBorder = 0;

    memset(&strm->BlockHeader, 0, sizeof(strm->BlockHeader));
    strm->BlockHeader.BlockSize = -1;  // '-1' means not defined yet.
    rar_UnpInitData20(strm, Solid);
    rar_UnpInitData30(strm, Solid);
    rar_UnpInitData50(strm, Solid);
}

void XCompress::rar_UnpInitData15(rar_stream *strm, bool Solid)
{
    if (!Solid) {
        strm->AvrPlcB = strm->AvrLn1 = strm->AvrLn2 = strm->AvrLn3 = strm->NumHuf = strm->Buf60 = 0;
        strm->AvrPlc = 0x3500;
        strm->MaxDist3 = 0x2001;
        strm->Nhfb = strm->Nlzb = 0x80;
    }
    strm->FlagsCnt = 0;
    strm->FlagBuf = 0;
    strm->StMode = 0;
    strm->LCount = 0;
    strm->ReadTop = 0;
}

void XCompress::rar_UnpInitData20(rar_stream *strm, bool Solid)
{
    if (!Solid) {
        strm->TablesRead2 = false;
        strm->UnpAudioBlock = false;
        strm->UnpChannelDelta = 0;
        strm->UnpCurChannel = 0;
        strm->UnpChannels = 1;

        memset(strm->AudV, 0, sizeof(strm->AudV));
        memset(strm->UnpOldTable20, 0, sizeof(strm->UnpOldTable20));
        memset(strm->MD, 0, sizeof(strm->MD));
    }
}

void XCompress::rar_UnpInitData30(rar_stream *strm, bool Solid)
{
    if (!Solid) {
        strm->TablesRead3 = false;
        memset(strm->UnpOldTable, 0, sizeof(strm->UnpOldTable));
        strm->PPMEscChar = 2;
        strm->UnpBlockType = RAR_BLOCK_LZ;
    }
    rar_InitFilters30(strm, Solid);
}

void XCompress::rar_UnpInitData50(rar_stream *strm, bool Solid)
{
    if (!Solid) strm->TablesRead5 = false;
}

void XCompress::rar_InitFilters30(rar_stream *strm, bool Solid)
{
    if (!Solid) {
        strm->OldFilterLengths.clear();
        strm->LastFilter = 0;

        for (size_t I = 0; I < strm->Filters30.size(); I++) delete strm->Filters30[I];
        strm->Filters30.clear();
    }
    for (size_t I = 0; I < strm->PrgStack.size(); I++) delete strm->PrgStack[I];
    strm->PrgStack.clear();
}

bool XCompress::rar_UnpReadBuf(rar_stream *strm, QIODevice *pDevice)
{
    int DataSize = strm->ReadTop - strm->InAddr;  // Data left to process.
    if (DataSize < 0) return false;
    strm->BlockHeader.BlockSize -= strm->InAddr - strm->BlockHeader.BlockStart;
    if (strm->InAddr > RAR_BufferSize_MAX_SIZE / 2) {
        // If we already processed more than half of buffer, let's move
        // remaining data into beginning to free more space for new data
        // and ensure that calling function does not cross the buffer border
        // even if we did not read anything here. Also it ensures that read size
        // is not less than CRYPT_BLOCK_SIZE, so we can align it without risk
        // to make it zero.
        if (DataSize > 0) memmove(strm->InBuf, strm->InBuf + strm->InAddr, DataSize);
        strm->InAddr = 0;
        strm->ReadTop = DataSize;
    } else DataSize = strm->ReadTop;
    int ReadCode = 0;
    if (RAR_BufferSize_MAX_SIZE != DataSize)
        // ReadCode=strm->UnpIO->UnpRead(strm->InBuf+DataSize,BufferSize_MAX_SIZE-DataSize);
        ReadCode = pDevice->read((char *)(strm->InBuf + DataSize), RAR_BufferSize_MAX_SIZE - DataSize);
    if (ReadCode > 0)  // Can be also -1.
        strm->ReadTop += ReadCode;
    strm->ReadBorder = strm->ReadTop - 30;
    strm->BlockHeader.BlockStart = strm->InAddr;
    if (strm->BlockHeader.BlockSize != -1)  // '-1' means not defined yet.
    {
        // We may need to quit from main extraction loop and read new block header
        // and trees earlier than data in input buffer ends.
        strm->ReadBorder = qMin(strm->ReadBorder, strm->BlockHeader.BlockStart + strm->BlockHeader.BlockSize - 1);
    }
    return ReadCode != -1;
}

void XCompress::rar_UnpWriteBuf20(rar_stream *strm, QIODevice *pDevice)
{
    if (strm->UnpPtr != strm->WrPtr) strm->UnpSomeRead = true;
    if (strm->UnpPtr < strm->WrPtr) {
        pDevice->write((char *)(&strm->Window[strm->WrPtr]), -(int)strm->WrPtr & strm->MaxWinMask);
        pDevice->write((char *)(strm->Window), strm->UnpPtr);

        // 2024.12.24: Before 7.10 we set "UnpAllBuf=true" here. It was needed for
        // Pack::PrepareSolidAppend(). Since both UnpAllBuf and FirstWinDone
        // variables indicate the same thing and we set FirstWinDone in other place
        // anyway, we replaced UnpAllBuf with FirstWinDone and removed this code.
    } else pDevice->write((char *)(&strm->Window[strm->WrPtr]), strm->UnpPtr - strm->WrPtr);
    strm->WrPtr = strm->UnpPtr;
}

void XCompress::rar_UnpWriteBuf30(rar_stream *strm, QIODevice *pDevice)
{
    // uint WrittenBorder=(uint)strm->WrPtr;
    // uint WriteSize=(uint)((strm->UnpPtr-WrittenBorder)&strm->MaxWinMask);
    // for (size_t I=0;I<strm->PrgStack.size();I++)
    // {
    //   // Here we apply filters to data which we need to write.
    //   // We always copy data to virtual machine memory before processing.
    //   // We cannot process them just in place in Window buffer, because
    //   // these data can be used for future string matches, so we must
    //   // preserve them in original form.

    //   rar_UnpackFilter30 *flt=strm->PrgStack[I];
    //   if (flt==NULL)
    //     continue;
    //   if (flt->NextWindow)
    //   {
    //     flt->NextWindow=false;
    //     continue;
    //   }
    //   unsigned int BlockStart=flt->BlockStart;
    //   unsigned int BlockLength=flt->BlockLength;
    //   if (((BlockStart-WrittenBorder)&strm->MaxWinMask)<WriteSize)
    //   {
    //     if (WrittenBorder!=BlockStart)
    //     {
    //       rar_UnpWriteArea(strm, pDevice, WrittenBorder,BlockStart);
    //       WrittenBorder=BlockStart;
    //       WriteSize=(uint)((strm->UnpPtr-WrittenBorder)&strm->MaxWinMask);
    //     }
    //     if (BlockLength<=WriteSize)
    //     {
    //       uint BlockEnd=(BlockStart+BlockLength)&strm->MaxWinMask;
    //       if (BlockStart<BlockEnd || BlockEnd==0)
    //         VM.SetMemory(0,strm->Window+BlockStart,BlockLength);
    //       else
    //       {
    //         uint FirstPartLength=uint(strm->MaxWinSize-BlockStart);
    //         VM.SetMemory(0,strm->Window+BlockStart,FirstPartLength);
    //         VM.SetMemory(FirstPartLength,strm->Window,BlockEnd);
    //       }

    //       VM_PreparedProgram *ParentPrg=&strm->Filters30[flt->ParentFilter]->Prg;
    //       VM_PreparedProgram *Prg=&flt->Prg;

    //       ExecuteCode(Prg);

    //       quint8 *FilteredData=Prg->FilteredData;
    //       unsigned int FilteredDataSize=Prg->FilteredDataSize;

    //       delete strm->PrgStack[I];
    //       strm->PrgStack[I]=nullptr;
    //       while (I+1<strm->PrgStack.size())
    //       {
    //         rar_UnpackFilter30 *NextFilter=strm->PrgStack[I+1];
    //         // It is required to check NextWindow here.
    //         if (NextFilter==NULL || NextFilter->BlockStart!=BlockStart ||
    //             NextFilter->BlockLength!=FilteredDataSize || NextFilter->NextWindow)
    //           break;

    //         // Apply several filters to same data block.

    //         VM.SetMemory(0,FilteredData,FilteredDataSize);

    //         VM_PreparedProgram *ParentPrg=&strm->Filters30[NextFilter->ParentFilter]->Prg;
    //         VM_PreparedProgram *NextPrg=&NextFilter->Prg;

    //         ExecuteCode(NextPrg);

    //         FilteredData=NextPrg->FilteredData;
    //         FilteredDataSize=NextPrg->FilteredDataSize;
    //         I++;
    //         delete strm->PrgStack[I];
    //         strm->PrgStack[I]=nullptr;
    //       }
    //       pDevice->write((char *)FilteredData,FilteredDataSize);
    //       strm->UnpSomeRead=true;
    //       strm->WrittenFileSize+=FilteredDataSize;
    //       WrittenBorder=BlockEnd;
    //       WriteSize=uint((strm->UnpPtr-WrittenBorder)&strm->MaxWinMask);
    //     }
    //     else
    //     {
    //       // Current filter intersects the window write border, so we adjust
    //       // the window border to process this filter next time, not now.
    //       for (size_t J=I;J<strm->PrgStack.size();J++)
    //       {
    //         rar_UnpackFilter30 *flt=strm->PrgStack[J];
    //         if (flt!=nullptr && flt->NextWindow)
    //           flt->NextWindow=false;
    //       }
    //       strm->WrPtr=WrittenBorder;
    //       return;
    //     }
    //   }
    // }

    // rar_UnpWriteArea(strm,pDevice,WrittenBorder,strm->UnpPtr);
    // strm->WrPtr=strm->UnpPtr;
}

void XCompress::rar_UnpWriteData(rar_stream *strm, QIODevice *pDevice, quint8 *Data, size_t Size)
{
    if (strm->WrittenFileSize >= strm->DestUnpSize) return;
    size_t WriteSize = Size;
    qint64 LeftToWrite = strm->DestUnpSize - strm->WrittenFileSize;
    if ((qint64)WriteSize > LeftToWrite) WriteSize = (size_t)LeftToWrite;
    pDevice->write((char *)Data, WriteSize);
    strm->WrittenFileSize += Size;
}

void XCompress::rar_UnpWriteArea(rar_stream *strm, QIODevice *pDevice, size_t StartPtr, size_t EndPtr)
{
    Q_UNUSED(strm);
    Q_UNUSED(pDevice);
    Q_UNUSED(StartPtr);
    Q_UNUSED(EndPtr);

    // if (EndPtr!=StartPtr)
    //   strm->UnpSomeRead=true;

    // if (strm->Fragmented)
    // {
    //   size_t SizeToWrite=WrapDown(EndPtr-StartPtr);
    //   while (SizeToWrite>0)
    //   {
    //     size_t BlockSize=FragWindow.GetBlockSize(StartPtr,SizeToWrite);
    //     rar_UnpWriteData(strm, pDevice, &FragWindow[StartPtr],BlockSize);
    //     SizeToWrite-=BlockSize;
    //     StartPtr=rar_WrapUp(strm,StartPtr+BlockSize);
    //   }
    // }
    // else
    //   if (EndPtr<StartPtr)
    //   {
    //     rar_UnpWriteData(strm, pDevice, strm->Window+StartPtr,strm->MaxWinSize-StartPtr);
    //     rar_UnpWriteData(strm, pDevice, strm->Window,EndPtr);
    //   }
    //   else
    //     rar_UnpWriteData(strm, pDevice, strm->Window+StartPtr,EndPtr-StartPtr);
}

bool XCompress::rar_UnpReadBuf30(rar_stream *strm, QIODevice *pDevice)
{
    int DataSize = strm->ReadTop - strm->InAddr;  // Data left to process.
    if (DataSize < 0) return false;
    if (strm->InAddr > RAR_BufferSize_MAX_SIZE / 2) {
        // If we already processed more than half of buffer, let's move
        // remaining data into beginning to free more space for new data
        // and ensure that calling function does not cross the buffer border
        // even if we did not read anything here. Also it ensures that read size
        // is not less than CRYPT_BLOCK_SIZE, so we can align it without risk
        // to make it zero.
        if (DataSize > 0) memmove(strm->InBuf, strm->InBuf + strm->InAddr, DataSize);
        strm->InAddr = 0;
        strm->ReadTop = DataSize;
    } else DataSize = strm->ReadTop;
    int ReadCode = pDevice->read((char *)(strm->InBuf + DataSize), RAR_BufferSize_MAX_SIZE - DataSize);
    if (ReadCode > 0) strm->ReadTop += ReadCode;
    strm->ReadBorder = strm->ReadTop - 30;
    return ReadCode != -1;
}

void XCompress::rar_GetFlagsBuf(rar_stream *strm)
{
    uint Flags, NewFlagsPlace;
    uint FlagsPlace = rar_DecodeNum(strm, rar_fgetbits(strm), RAR_STARTHF2, RAR_DecHf2, RAR_PosHf2);

    // Our Huffman table stores 257 items and needs all them in other parts
    // of code such as when StMode is on, so the first item is control item.
    // While normally we do not use the last item to code the flags byte here,
    // we need to check for value 256 when unpacking in case we unpack
    // a corrupt archive.
    if (FlagsPlace >= sizeof(strm->ChSetC) / sizeof(strm->ChSetC[0])) return;

    while (1) {
        Flags = strm->ChSetC[FlagsPlace];
        strm->FlagBuf = Flags >> 8;
        NewFlagsPlace = strm->NToPlC[Flags++ & 0xff]++;
        if ((Flags & 0xff) != 0) break;
        rar_CorrHuff(strm, strm->ChSetC, strm->NToPlC);
    }

    strm->ChSetC[FlagsPlace] = strm->ChSetC[NewFlagsPlace];
    strm->ChSetC[NewFlagsPlace] = (ushort)Flags;
}

uint XCompress::rar_DecodeNum(rar_stream *strm, uint Num, uint StartPos, uint *DecTab, uint *PosTab)
{
    int I;
    for (Num &= 0xfff0, I = 0; DecTab[I] <= Num; I++) StartPos++;
    rar_faddbits(strm, StartPos);
    return (((Num - (I ? DecTab[I - 1] : 0)) >> (16 - StartPos)) + PosTab[StartPos]);
}

uint XCompress::rar_fgetbits(rar_stream *strm)
{
    // Function wrapped version of inline getbits to reduce the code size.
    return rar_getbits(strm);
}

void XCompress::rar_faddbits(rar_stream *strm, uint Bits)
{
    rar_addbits(strm, Bits);
}

uint XCompress::rar_getbits(rar_stream *strm)
{
    uint BitField = (uint)strm->InBuf[strm->InAddr] << 16;
    BitField |= (uint)strm->InBuf[strm->InAddr + 1] << 8;
    BitField |= (uint)strm->InBuf[strm->InAddr + 2];
    BitField >>= (8 - strm->InBit);

    return BitField & 0xffff;
}

void XCompress::rar_addbits(rar_stream *strm, uint Bits)
{
    Bits += strm->InBit;
    strm->InAddr += Bits >> 3;
    strm->InBit = Bits & 7;
}

void XCompress::rar_HuffDecode(rar_stream *strm)
{
    uint CurByte, NewBytePlace;
    uint Length;
    uint Distance;
    int BytePlace;

    uint BitField = rar_fgetbits(strm);

    if (strm->AvrPlc > 0x75ff) BytePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF4, RAR_DecHf4, RAR_PosHf4);
    else if (strm->AvrPlc > 0x5dff) BytePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF3, RAR_DecHf3, RAR_PosHf3);
    else if (strm->AvrPlc > 0x35ff) BytePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF2, RAR_DecHf2, RAR_PosHf2);
    else if (strm->AvrPlc > 0x0dff) BytePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF1, RAR_DecHf1, RAR_PosHf1);
    else BytePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF0, RAR_DecHf0, RAR_PosHf0);
    BytePlace &= 0xff;
    if (strm->StMode) {
        if (BytePlace == 0 && BitField > 0xfff) BytePlace = 0x100;
        if (--BytePlace == -1) {
            BitField = rar_fgetbits(strm);
            rar_faddbits(strm, 1);
            if (BitField & 0x8000) {
                strm->NumHuf = strm->StMode = 0;
                return;
            } else {
                Length = (BitField & 0x4000) ? 4 : 3;
                rar_faddbits(strm, 1);
                Distance = rar_DecodeNum(strm, rar_fgetbits(strm), RAR_STARTHF2, RAR_DecHf2, RAR_PosHf2);
                Distance = (Distance << 5) | (rar_fgetbits(strm) >> 11);
                rar_faddbits(strm, 5);
                rar_CopyString15(strm, Distance, Length);
                return;
            }
        }
    } else if (strm->NumHuf++ >= 16 && strm->FlagsCnt == 0) strm->StMode = 1;
    strm->AvrPlc += BytePlace;
    strm->AvrPlc -= strm->AvrPlc >> 8;
    strm->Nhfb += 16;
    if (strm->Nhfb > 0xff) {
        strm->Nhfb = 0x90;
        strm->Nlzb >>= 1;
    }

    strm->Window[strm->UnpPtr++] = (quint8)(strm->ChSet[BytePlace] >> 8);
    --strm->DestUnpSize;

    while (1) {
        CurByte = strm->ChSet[BytePlace];
        NewBytePlace = strm->NToPl[CurByte++ & 0xff]++;
        if ((CurByte & 0xff) > 0xa1) rar_CorrHuff(strm, strm->ChSet, strm->NToPl);
        else break;
    }

    strm->ChSet[BytePlace] = strm->ChSet[NewBytePlace];
    strm->ChSet[NewBytePlace] = (ushort)CurByte;
}

void XCompress::rar_LongLZ(rar_stream *strm)
{
    uint Length;
    uint Distance;
    uint DistancePlace, NewDistancePlace;
    uint OldAvr2, OldAvr3;

    strm->NumHuf = 0;
    strm->Nlzb += 16;
    if (strm->Nlzb > 0xff) {
        strm->Nlzb = 0x90;
        strm->Nhfb >>= 1;
    }
    OldAvr2 = strm->AvrLn2;

    uint BitField = rar_fgetbits(strm);
    if (strm->AvrLn2 >= 122) Length = rar_DecodeNum(strm, BitField, RAR_STARTL2, RAR_DecL2, RAR_PosL2);
    else if (strm->AvrLn2 >= 64) Length = rar_DecodeNum(strm, BitField, RAR_STARTL1, RAR_DecL1, RAR_PosL1);
    else if (BitField < 0x100) {
        Length = BitField;
        rar_faddbits(strm, 16);
    } else {
        for (Length = 0; ((BitField << Length) & 0x8000) == 0; Length++)
            ;
        rar_faddbits(strm, Length + 1);
    }

    strm->AvrLn2 += Length;
    strm->AvrLn2 -= strm->AvrLn2 >> 5;

    BitField = rar_fgetbits(strm);
    if (strm->AvrPlcB > 0x28ff) DistancePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF2, RAR_DecHf2, RAR_PosHf2);
    else if (strm->AvrPlcB > 0x6ff) DistancePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF1, RAR_DecHf1, RAR_PosHf1);
    else DistancePlace = rar_DecodeNum(strm, BitField, RAR_STARTHF0, RAR_DecHf0, RAR_PosHf0);

    strm->AvrPlcB += DistancePlace;
    strm->AvrPlcB -= strm->AvrPlcB >> 8;
    while (1) {
        Distance = strm->ChSetB[DistancePlace & 0xff];
        NewDistancePlace = strm->NToPlB[Distance++ & 0xff]++;
        if (!(Distance & 0xff)) rar_CorrHuff(strm, strm->ChSetB, strm->NToPlB);
        else break;
    }

    strm->ChSetB[DistancePlace & 0xff] = strm->ChSetB[NewDistancePlace];
    strm->ChSetB[NewDistancePlace] = (ushort)Distance;

    Distance = ((Distance & 0xff00) | (rar_fgetbits(strm) >> 8)) >> 1;
    rar_faddbits(strm, 7);

    OldAvr3 = strm->AvrLn3;
    if (Length != 1 && Length != 4) {
        if (Length == 0 && Distance <= strm->MaxDist3) {
            strm->AvrLn3++;
            strm->AvrLn3 -= strm->AvrLn3 >> 8;
        } else if (strm->AvrLn3 > 0) {
            strm->AvrLn3--;
        }
    }
    Length += 3;
    if (Distance >= strm->MaxDist3) Length++;
    if (Distance <= 256) Length += 8;
    if (OldAvr3 > 0xb0 || strm->AvrPlc >= 0x2a00 && OldAvr2 < 0x40) strm->MaxDist3 = 0x7f00;
    else strm->MaxDist3 = 0x2001;
    strm->OldDist[strm->OldDistPtr++] = Distance;
    strm->OldDistPtr = strm->OldDistPtr & 3;
    strm->LastLength = Length;
    strm->LastDist = Distance;
    rar_CopyString15(strm, Distance, Length);
}

void XCompress::rar_ShortLZ(rar_stream *strm)
{
    static uint ShortLen1[] = {1, 3, 4, 4, 5, 6, 7, 8, 8, 4, 4, 5, 6, 6, 4, 0};
    static uint ShortXor1[] = {0, 0xa0, 0xd0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xc0, 0x80, 0x90, 0x98, 0x9c, 0xb0};
    static uint ShortLen2[] = {2, 3, 3, 3, 4, 4, 5, 6, 6, 4, 4, 5, 6, 6, 4, 0};
    static uint ShortXor2[] = {0, 0x40, 0x60, 0xa0, 0xd0, 0xe0, 0xf0, 0xf8, 0xfc, 0xc0, 0x80, 0x90, 0x98, 0x9c, 0xb0};

#define GetShortLen1(pos) ((pos) == 1 ? strm->Buf60 + 3 : ShortLen1[pos])
#define GetShortLen2(pos) ((pos) == 3 ? strm->Buf60 + 3 : ShortLen2[pos])

    uint Length, SaveLength;
    uint LastDistance;
    uint Distance;
    int DistancePlace;
    strm->NumHuf = 0;

    uint BitField = rar_fgetbits(strm);
    if (strm->LCount == 2) {
        rar_faddbits(strm, 1);
        if (BitField >= 0x8000) {
            rar_CopyString15(strm, strm->LastDist, strm->LastLength);
            return;
        }
        BitField <<= 1;
        strm->LCount = 0;
    }

    BitField >>= 8;

    //  not thread safe, replaced by GetShortLen1 and GetShortLen2 macro
    //  ShortLen1[1]=ShortLen2[3]=Buf60+3;

    if (strm->AvrLn1 < 37) {
        for (Length = 0;; Length++)
            if (((BitField ^ ShortXor1[Length]) & (~(0xff >> GetShortLen1(Length)))) == 0) break;
        rar_faddbits(strm, GetShortLen1(Length));
    } else {
        for (Length = 0;; Length++)
            if (((BitField ^ ShortXor2[Length]) & (~(0xff >> GetShortLen2(Length)))) == 0) break;
        rar_faddbits(strm, GetShortLen2(Length));
    }

    if (Length >= 9) {
        if (Length == 9) {
            strm->LCount++;
            rar_CopyString15(strm, strm->LastDist, strm->LastLength);
            return;
        }
        if (Length == 14) {
            strm->LCount = 0;
            Length = rar_DecodeNum(strm, rar_fgetbits(strm), RAR_STARTL2, RAR_DecL2, RAR_PosL2) + 5;
            Distance = (rar_fgetbits(strm) >> 1) | 0x8000;
            rar_faddbits(strm, 15);
            strm->LastLength = Length;
            strm->LastDist = Distance;
            rar_CopyString15(strm, Distance, Length);
            return;
        }

        strm->LCount = 0;
        SaveLength = Length;
        Distance = (uint)strm->OldDist[(strm->OldDistPtr - (Length - 9)) & 3];
        Length = rar_DecodeNum(strm, rar_fgetbits(strm), RAR_STARTL1, RAR_DecL1, RAR_PosL1) + 2;
        if (Length == 0x101 && SaveLength == 10) {
            strm->Buf60 ^= 1;
            return;
        }
        if (Distance > 256) Length++;
        if (Distance >= strm->MaxDist3) Length++;

        strm->OldDist[strm->OldDistPtr++] = Distance;
        strm->OldDistPtr = strm->OldDistPtr & 3;
        strm->LastLength = Length;
        strm->LastDist = Distance;
        rar_CopyString15(strm, Distance, Length);
        return;
    }

    strm->LCount = 0;
    strm->AvrLn1 += Length;
    strm->AvrLn1 -= strm->AvrLn1 >> 4;

    DistancePlace = rar_DecodeNum(strm, rar_fgetbits(strm), RAR_STARTHF2, RAR_DecHf2, RAR_PosHf2) & 0xff;
    Distance = strm->ChSetA[DistancePlace];
    if (--DistancePlace != -1) {
        LastDistance = strm->ChSetA[DistancePlace];
        strm->ChSetA[DistancePlace + 1] = (ushort)LastDistance;
        strm->ChSetA[DistancePlace] = (ushort)Distance;
    }
    Length += 2;
    strm->OldDist[strm->OldDistPtr++] = ++Distance;
    strm->OldDistPtr = strm->OldDistPtr & 3;
    strm->LastLength = Length;
    strm->LastDist = Distance;
    rar_CopyString15(strm, Distance, Length);
}

void XCompress::rar_CopyString15(rar_stream *strm, uint Distance, uint Length)
{
    strm->DestUnpSize -= Length;
    // 2024.04.18: Distance can be 0 in corrupt RAR 1.5 archives.
    if (!strm->FirstWinDone && Distance > strm->UnpPtr || Distance > strm->MaxWinSize || Distance == 0)
        while (Length-- > 0) {
            strm->Window[strm->UnpPtr] = 0;
            strm->UnpPtr = (strm->UnpPtr + 1) & strm->MaxWinMask;
        }
    else
        while (Length-- > 0) {
            strm->Window[strm->UnpPtr] = strm->Window[(strm->UnpPtr - Distance) & strm->MaxWinMask];
            strm->UnpPtr = (strm->UnpPtr + 1) & strm->MaxWinMask;
        }
}

void XCompress::rar_CopyString(rar_stream *strm, uint Distance, uint Length)
{
    size_t SrcPtr = strm->UnpPtr - Distance;

    // Perform the correction here instead of "else", so matches crossing
    // the window beginning can also be processed by first "if" part.
    if (Distance > strm->UnpPtr)  // Unlike SrcPtr>=MaxWinSize, it catches invalid distances like 0xfffffff0 in 32-bit build.
    {
        // Same as WrapDown(SrcPtr), needed because of UnpPtr-Distance above.
        // We need the same condition below, so we expanded WrapDown() here.
        SrcPtr += strm->MaxWinSize;

        // About Distance>MaxWinSize check.
        // SrcPtr can be >=MaxWinSize if distance exceeds MaxWinSize
        // in a malformed archive. Our WrapDown replacement above might not
        // correct it, so to prevent out of bound Window read we check it here.
        // Unlike SrcPtr>=MaxWinSize check, it allows MaxWinSize>0x80000000
        // in 32-bit build, which could cause overflow in SrcPtr.
        // About !FirstWinDone check.
        // If first window hasn't filled yet and it points outside of window,
        // set data to 0 instead of copying preceding file data, so result doesn't
        // depend on previously extracted files in non-solid archive.
        if (Distance > strm->MaxWinSize || !strm->FirstWinDone) {
            // Fill area of specified length with 0 instead of returning.
            // So if only the distance is broken and rest of packed data is valid,
            // it preserves offsets and allows to continue extraction.
            // If we set SrcPtr to random offset instead, let's say, 0,
            // we still will be copying preceding file data if UnpPtr is also 0.
            while (Length-- > 0) {
                strm->Window[strm->UnpPtr] = 0;
                strm->UnpPtr = rar_WrapUp(strm, strm->UnpPtr + 1);
            }
            return;
        }
    }

    if (SrcPtr < strm->MaxWinSize - RAR_MAX_INC_LZ_MATCH && strm->UnpPtr < strm->MaxWinSize - RAR_MAX_INC_LZ_MATCH) {
        // If we are not close to end of window, we do not need to waste time
        // to WrapUp and WrapDown position protection.

        quint8 *Src = strm->Window + SrcPtr;
        quint8 *Dest = strm->Window + strm->UnpPtr;
        strm->UnpPtr += Length;

        while (Length >= 8) {
            Dest[0] = Src[0];
            Dest[1] = Src[1];
            Dest[2] = Src[2];
            Dest[3] = Src[3];
            Dest[4] = Src[4];
            Dest[5] = Src[5];
            Dest[6] = Src[6];
            Dest[7] = Src[7];

            Src += 8;
            Dest += 8;
            Length -= 8;
        }

        // Unroll the loop for 0 - 7 bytes left. Note that we use nested "if"s.
        if (Length > 0) {
            Dest[0] = Src[0];
            if (Length > 1) {
                Dest[1] = Src[1];
                if (Length > 2) {
                    Dest[2] = Src[2];
                    if (Length > 3) {
                        Dest[3] = Src[3];
                        if (Length > 4) {
                            Dest[4] = Src[4];
                            if (Length > 5) {
                                Dest[5] = Src[5];
                                if (Length > 6) {
                                    Dest[6] = Src[6];
                                }
                            }
                        }
                    }
                }
            }
        }  // Close all nested "if"s.
    } else
        while (Length-- > 0)  // Slow copying with all possible precautions.
        {
            strm->Window[strm->UnpPtr] = strm->Window[rar_WrapUp(strm, SrcPtr++)];
            // We need to have masked UnpPtr after quit from loop, so it must not
            // be replaced with 'Window[WrapUp(UnpPtr++)]'
            strm->UnpPtr = rar_WrapUp(strm, strm->UnpPtr + 1);
        }
}

bool XCompress::rar_ReadTables20(rar_stream *strm, QIODevice *pDevice)
{
    quint8 BitLength[RAR_BC20];
    quint8 Table[RAR_MC20 * 4];
    if (strm->InAddr > strm->ReadTop - 25)

        if (!rar_UnpReadBuf(strm, pDevice)) return false;
    uint BitField = rar_getbits(strm);
    strm->UnpAudioBlock = (BitField & 0x8000) != 0;

    if (!(BitField & 0x4000)) memset(strm->UnpOldTable20, 0, sizeof(strm->UnpOldTable20));
    rar_addbits(strm, 2);

    uint TableSize;
    if (strm->UnpAudioBlock) {
        strm->UnpChannels = ((BitField >> 12) & 3) + 1;
        if (strm->UnpCurChannel >= strm->UnpChannels) strm->UnpCurChannel = 0;
        rar_addbits(strm, 2);
        TableSize = RAR_MC20 * strm->UnpChannels;
    } else TableSize = RAR_NC20 + RAR_DC20 + RAR_RC20;

    for (uint I = 0; I < RAR_BC20; I++) {
        BitLength[I] = (quint8)(rar_getbits(strm) >> 12);
        rar_addbits(strm, 4);
    }
    rar_MakeDecodeTables(strm, BitLength, &strm->BlockTables.BD, RAR_BC20);
    for (uint I = 0; I < TableSize;) {
        if (strm->InAddr > strm->ReadTop - 5)
            if (!rar_UnpReadBuf(strm, pDevice)) return false;
        uint Number = rar_DecodeNumber(strm, &strm->BlockTables.BD);
        if (Number < 16) {
            Table[I] = (Number + strm->UnpOldTable20[I]) & 0xf;
            I++;
        } else if (Number == 16) {
            uint N = (rar_getbits(strm) >> 14) + 3;
            rar_addbits(strm, 2);
            if (I == 0) return false;  // We cannot have "repeat previous" code at the first position.
            else
                while (N-- > 0 && I < TableSize) {
                    Table[I] = Table[I - 1];
                    I++;
                }
        } else {
            uint N;
            if (Number == 17) {
                N = (rar_getbits(strm) >> 13) + 3;
                rar_addbits(strm, 3);
            } else {
                N = (rar_getbits(strm) >> 9) + 11;
                rar_addbits(strm, 7);
            }
            while (N-- > 0 && I < TableSize) Table[I++] = 0;
        }
    }
    strm->TablesRead2 = true;
    if (strm->InAddr > strm->ReadTop) return true;
    if (strm->UnpAudioBlock)
        for (uint I = 0; I < strm->UnpChannels; I++) rar_MakeDecodeTables(strm, &Table[I * RAR_MC20], &strm->MD[I], RAR_MC20);
    else {
        rar_MakeDecodeTables(strm, &Table[0], &strm->BlockTables.LD, RAR_NC20);
        rar_MakeDecodeTables(strm, &Table[RAR_NC20], &strm->BlockTables.DD, RAR_DC20);
        rar_MakeDecodeTables(strm, &Table[RAR_NC20 + RAR_DC20], &strm->BlockTables.RD, RAR_RC20);
    }
    memcpy(strm->UnpOldTable20, Table, TableSize);
    return true;
}

bool XCompress::rar_ReadTables30(rar_stream *strm, QIODevice *pDevice)
{
    quint8 BitLength[RAR_BC];
    quint8 Table[RAR_HUFF_TABLE_SIZE30];
    if (strm->InAddr > strm->ReadTop - 25)
        if (!rar_UnpReadBuf30(strm, pDevice)) return (false);
    rar_faddbits(strm, (8 - strm->InBit) & 7);
    uint BitField = rar_fgetbits(strm);
    if (BitField & 0x8000) {
        strm->UnpBlockType = RAR_BLOCK_PPM;
#ifdef QT_DEBUG
        qDebug("Add PPM support");
#endif
        return false;
        // return(rar_PPM_DecodeInit(this,strm->PPMEscChar));
    }
    strm->UnpBlockType = RAR_BLOCK_LZ;

    strm->PrevLowDist = 0;
    strm->LowDistRepCount = 0;

    if (!(BitField & 0x4000)) memset(strm->UnpOldTable, 0, sizeof(strm->UnpOldTable));
    rar_faddbits(strm, 2);

    for (uint I = 0; I < RAR_BC; I++) {
        uint Length = (quint8)(rar_fgetbits(strm) >> 12);
        rar_faddbits(strm, 4);
        if (Length == 15) {
            uint ZeroCount = (quint8)(rar_fgetbits(strm) >> 12);
            rar_faddbits(strm, 4);
            if (ZeroCount == 0) BitLength[I] = 15;
            else {
                ZeroCount += 2;
                while (ZeroCount-- > 0 && I < ASIZE(BitLength)) BitLength[I++] = 0;
                I--;
            }
        } else BitLength[I] = Length;
    }
    rar_MakeDecodeTables(strm, BitLength, &strm->BlockTables.BD, RAR_BC30);

    const uint TableSize = RAR_HUFF_TABLE_SIZE30;
    for (uint I = 0; I < TableSize;) {
        if (strm->InAddr > strm->ReadTop - 5)
            if (!rar_UnpReadBuf30(strm, pDevice)) return (false);
        uint Number = rar_DecodeNumber(strm, &strm->BlockTables.BD);
        if (Number < 16) {
            Table[I] = (Number + strm->UnpOldTable[I]) & 0xf;
            I++;
        } else if (Number < 18) {
            uint N;
            if (Number == 16) {
                N = (rar_fgetbits(strm) >> 13) + 3;
                rar_faddbits(strm, 3);
            } else {
                N = (rar_fgetbits(strm) >> 9) + 11;
                rar_faddbits(strm, 7);
            }
            if (I == 0) return false;  // We cannot have "repeat previous" code at the first position.
            else
                while (N-- > 0 && I < TableSize) {
                    Table[I] = Table[I - 1];
                    I++;
                }
        } else {
            uint N;
            if (Number == 18) {
                N = (rar_fgetbits(strm) >> 13) + 3;
                rar_faddbits(strm, 3);
            } else {
                N = (rar_fgetbits(strm) >> 9) + 11;
                rar_faddbits(strm, 7);
            }
            while (N-- > 0 && I < TableSize) Table[I++] = 0;
        }
    }
    strm->TablesRead3 = true;
    if (strm->InAddr > strm->ReadTop) return false;
    rar_MakeDecodeTables(strm, &Table[0], &strm->BlockTables.LD, RAR_NC30);
    rar_MakeDecodeTables(strm, &Table[RAR_NC30], &strm->BlockTables.DD, RAR_DC30);
    rar_MakeDecodeTables(strm, &Table[RAR_NC30 + RAR_DC30], &strm->BlockTables.LDD, RAR_LDC30);
    rar_MakeDecodeTables(strm, &Table[RAR_NC30 + RAR_DC30 + RAR_LDC30], &strm->BlockTables.RD, RAR_RC30);
    memcpy(strm->UnpOldTable, Table, sizeof(strm->UnpOldTable));
    return true;
}

void XCompress::rar_MakeDecodeTables(rar_stream *strm, quint8 *LengthTable, rar_DecodeTable *Dec, uint Size)
{
    // Size of alphabet and DecodePos array.
    Dec->MaxNum = Size;

    // Calculate how many entries for every bit length in LengthTable we have.
    uint LengthCount[16];
    memset(LengthCount, 0, sizeof(LengthCount));
    for (size_t I = 0; I < Size; I++) LengthCount[LengthTable[I] & 0xf]++;

    // We must not calculate the number of zero length codes.
    LengthCount[0] = 0;

    // Set the entire DecodeNum to zero.
    memset(Dec->DecodeNum, 0, Size * sizeof(*Dec->DecodeNum));

    // Initialize not really used entry for zero length code.
    Dec->DecodePos[0] = 0;

    // Start code for bit length 1 is 0.
    Dec->DecodeLen[0] = 0;

    // Right aligned upper limit code for current bit length.
    uint UpperLimit = 0;

    for (size_t I = 1; I < 16; I++) {
        // Adjust the upper limit code.
        UpperLimit += LengthCount[I];

        // Left aligned upper limit code.
        uint LeftAligned = UpperLimit << (16 - I);

        // Prepare the upper limit code for next bit length.
        UpperLimit *= 2;

        // Store the left aligned upper limit code.
        Dec->DecodeLen[I] = (uint)LeftAligned;

        // Every item of this array contains the sum of all preceding items.
        // So it contains the start position in code list for every bit length.
        Dec->DecodePos[I] = Dec->DecodePos[I - 1] + LengthCount[I - 1];
    }

    // Prepare the copy of DecodePos. We'll modify this copy below,
    // so we cannot use the original DecodePos.
    uint CopyDecodePos[ASIZE(Dec->DecodePos)];
    memcpy(CopyDecodePos, Dec->DecodePos, sizeof(CopyDecodePos));

    // For every bit length in the bit length table and so for every item
    // of alphabet.
    for (uint I = 0; I < Size; I++) {
        // Get the current bit length.
        quint8 CurBitLength = LengthTable[I] & 0xf;

        if (CurBitLength != 0) {
            // Last position in code list for current bit length.
            uint LastPos = CopyDecodePos[CurBitLength];

            // Prepare the decode table, so this position in code list will be
            // decoded to current alphabet item number.
            Dec->DecodeNum[LastPos] = (ushort)I;

            // We'll use next position number for this bit length next time.
            // So we pass through the entire range of positions available
            // for every bit length.
            CopyDecodePos[CurBitLength]++;
        }
    }

    // Define the number of bits to process in quick mode. We use more bits
    // for larger alphabets. More bits means that more codes will be processed
    // in quick mode, but also that more time will be spent to preparation
    // of tables for quick decode.
    switch (Size) {
        case RAR_NC:
        case RAR_NC20:
        case RAR_NC30: Dec->QuickBits = RAR_MAX_QUICK_DECODE_BITS; break;
        default: Dec->QuickBits = RAR_MAX_QUICK_DECODE_BITS > 3 ? RAR_MAX_QUICK_DECODE_BITS - 3 : 0; break;
    }

    // Size of tables for quick mode.
    uint QuickDataSize = 1 << Dec->QuickBits;

    // Bit length for current code, start from 1 bit codes. It is important
    // to use 1 bit instead of 0 for minimum code length, so we are moving
    // forward even when processing a corrupt archive.
    uint CurBitLength = 1;

    // For every right aligned bit string which supports the quick decoding.
    for (uint Code = 0; Code < QuickDataSize; Code++) {
        // Left align the current code, so it will be in usual bit field format.
        uint BitField = Code << (16 - Dec->QuickBits);

        // Prepare the table for quick decoding of bit lengths.

        // Find the upper limit for current bit field and adjust the bit length
        // accordingly if necessary.
        while (CurBitLength < ASIZE(Dec->DecodeLen) && BitField >= Dec->DecodeLen[CurBitLength]) CurBitLength++;

        // Translation of right aligned bit string to bit length.
        Dec->QuickLen[Code] = CurBitLength;

        // Prepare the table for quick translation of position in code list
        // to position in alphabet.

        // Calculate the distance from the start code for current bit length.
        uint Dist = BitField - Dec->DecodeLen[CurBitLength - 1];

        // Right align the distance.
        Dist >>= (16 - CurBitLength);

        // Now we can calculate the position in the code list. It is the sum
        // of first position for current bit length and right aligned distance
        // between our bit field and start code for current bit length.
        uint Pos;
        if (CurBitLength < ASIZE(Dec->DecodePos) && (Pos = Dec->DecodePos[CurBitLength] + Dist) < Size) {
            // Define the code to alphabet number translation.
            Dec->QuickNum[Code] = Dec->DecodeNum[Pos];
        } else {
            // Can be here for length table filled with zeroes only (empty).
            Dec->QuickNum[Code] = 0;
        }
    }
}

uint XCompress::rar_DecodeNumber(rar_stream *strm, rar_DecodeTable *Dec)
{
    // Left aligned 15 bit length raw bit field.
    uint BitField = rar_getbits(strm) & 0xfffe;

    if (BitField < Dec->DecodeLen[Dec->QuickBits]) {
        uint Code = BitField >> (16 - Dec->QuickBits);
        rar_addbits(strm, Dec->QuickLen[Code]);
        return Dec->QuickNum[Code];
    }

    // Detect the real bit length for current code.
    uint Bits = 15;
    for (uint I = Dec->QuickBits + 1; I < 15; I++)
        if (BitField < Dec->DecodeLen[I]) {
            Bits = I;
            break;
        }

    rar_addbits(strm, Bits);

    // Calculate the distance from the start code for current bit length.
    uint Dist = BitField - Dec->DecodeLen[Bits - 1];

    // Start codes are left aligned, but we need the normal right aligned
    // number. So we shift the distance to the right.
    Dist >>= (16 - Bits);

    // Now we can calculate the position in the code list. It is the sum
    // of first position for current bit length and right aligned distance
    // between our bit field and start code for current bit length.
    uint Pos = Dec->DecodePos[Bits] + Dist;

    // Out of bounds safety check required for damaged archives.
    if (Pos >= Dec->MaxNum) Pos = 0;

    // Convert the position in the code list to position in alphabet
    // and return it.
    return Dec->DecodeNum[Pos];
}

void XCompress::rar_InsertOldDist(struct rar_stream *strm, size_t Distance)
{
    strm->OldDist[3] = strm->OldDist[2];
    strm->OldDist[2] = strm->OldDist[1];
    strm->OldDist[1] = strm->OldDist[0];
    strm->OldDist[0] = Distance;
}

size_t XCompress::rar_WrapUp(rar_stream *strm, size_t WinPos)
{
    return WinPos >= strm->MaxWinSize ? WinPos - strm->MaxWinSize : WinPos;
}

bool XCompress::rar_ReadEndOfBlock(rar_stream *strm, QIODevice *pDevice)
{
    uint BitField = rar_getbits(strm);
    bool NewTable, NewFile = false;

    // "1"  - no new file, new table just here.
    // "00" - new file,    no new table.
    // "01" - new file,    new table (in beginning of next file).

    if ((BitField & 0x8000) != 0) {
        NewTable = true;
        rar_addbits(strm, 1);
    } else {
        NewFile = true;
        NewTable = (BitField & 0x4000) != 0;
        rar_addbits(strm, 2);
    }
    strm->TablesRead3 = !NewTable;

    // Quit immediately if "new file" flag is set. If "new table" flag
    // is present, we'll read the table in beginning of next file
    // based on 'TablesRead3' 'false' value.
    if (NewFile) return false;
    return rar_ReadTables30(strm, pDevice);  // Quit only if we failed to read tables.
}
