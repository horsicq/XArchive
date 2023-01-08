/* Copyright (c) 2023 hors<horsicq@gmail.com>
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

XCompress::XCompress()
{

}

bool XCompress::lzh_decode_init(lzh_stream *strm, int method)
{
    struct lzh_dec *ds;
    int w_bits, w_size;

    if (strm->ds == NULL) {
        strm->ds = (lzh_dec *)calloc(1, sizeof(*strm->ds));
    }
    ds = strm->ds;

    switch (method) {
    case 5:
        w_bits = 13;/* 8KiB for window */
        break;
    case 6:
        w_bits = 15;/* 32KiB for window */
        break;
    case 7:
        w_bits = 16;/* 64KiB for window */
        break;
    default:
        return false;/* Not supported. */
    }
    /* Expand a window size up to 128 KiB for decompressing process
     * performance whatever its original window size is. */
    ds->w_size = 1U << 17;
    ds->w_mask = ds->w_size -1;
    if (ds->w_buff == NULL) {
        ds->w_buff = (unsigned char *)malloc(ds->w_size);
    }
    w_size = 1U << w_bits;
    memset(ds->w_buff + ds->w_size - w_size, 0x20, w_size);
    ds->w_pos = 0;
    ds->state = 0;
    ds->pos_pt_len_size = w_bits + 1;
    ds->pos_pt_len_bits = (w_bits == 15 || w_bits == 16)? 5: 4;
    ds->literal_pt_len_size = PT_BITLEN_SIZE;
    ds->literal_pt_len_bits = 5;
    ds->br.cache_buffer = 0;
    ds->br.cache_avail = 0;

    lzh_huffman_init(&(ds->lt), LT_BITLEN_SIZE, 16);
    lzh_huffman_init(&(ds->pt), PT_BITLEN_SIZE, 16);
    ds->lt.len_bits = 9;

    ds->error = 0;

    return true;
}

bool XCompress::lzh_huffman_init(huffman *hf, size_t len_size, int tbl_bits)
{
    int bits;

    if (hf->bitlen == NULL) {
        hf->bitlen = (unsigned char	*)malloc(len_size * sizeof(hf->bitlen[0]));
    }
    if (hf->tbl == NULL) {
        if (tbl_bits < HTBL_BITS)
            bits = tbl_bits;
        else
            bits = HTBL_BITS;
        hf->tbl = (uint16_t	*)malloc(((size_t)1 << bits) * sizeof(hf->tbl[0]));
    }
    if (hf->tree == NULL && tbl_bits > HTBL_BITS) {
        hf->tree_avail = 1 << (tbl_bits - HTBL_BITS + 4);
        hf->tree = (htree_t *)malloc(hf->tree_avail * sizeof(hf->tree[0]));
    }
    hf->len_size = (int)len_size;
    hf->tbl_bits = tbl_bits;

    return true;
}
