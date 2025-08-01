/* Copyright (c) 2025 hors<horsicq@gmail.com>
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

XLZHDecoder::XLZHDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XLZHDecoder::lzh_decode_init(lzh_stream *strm, qint32 method)
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

bool XLZHDecoder::lzh_huffman_init(lzh_huffman *hf, size_t len_size, qint32 tbl_bits)
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

qint32 XLZHDecoder::lzh_br_fillup(lzh_stream *strm, lzh_br *br)
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

void XLZHDecoder::lzh_emit_window(lzh_stream *strm, size_t s)
{
    strm->ref_ptr = strm->ds->w_buff;
    strm->avail_out = (int)s;
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
    if (strm->ds == NULL) return;
    free(strm->ds->w_buff);
    lzh_huffman_free(&(strm->ds->lt));
    lzh_huffman_free(&(strm->ds->pt));
    free(strm->ds);
    strm->ds = NULL;
}

void XLZHDecoder::lzh_huffman_free(lzh_huffman *hf)
{
    free(hf->bitlen);
    free(hf->tbl);
    free(hf->tree);
}
