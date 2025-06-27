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
#ifndef XCOMPRESS_H
#define XCOMPRESS_H

#include <QObject>
#include <QIODevice>
#include "xbinary.h"

#ifdef Q_OS_LINUX
#if (QT_VERSION_MAJOR > 5)
#undef SCHAR_MIN
#define SCHAR_MIN (-SCHAR_MAX - 1)
#undef SCHAR_MAX
#define SCHAR_MAX __SCHAR_MAX__
#undef UCHAR_MAX
#if __SCHAR_MAX__ == __INT_MAX__
#define UCHAR_MAX (SCHAR_MAX * 2U + 1U)
#else
#define UCHAR_MAX (SCHAR_MAX * 2 + 1)
#endif
#endif
#endif

class XCompress {
public:
    XCompress();

// LZH
#define LZH_MAXMATCH 256 /* Maximum match length. */
#define LZH_MINMATCH 3   /* Minimum match length. */
/*
 * Literal table format:
 * +0              +256                      +510
 * +---------------+-------------------------+
 * | literal code  |       match length      |
 * |   0 ... 255   |  MINMATCH ... MAXMATCH  |
 * +---------------+-------------------------+
 *  <---          LT_BITLEN_SIZE         --->
 */
/* Literal table size. */
#define LZH_LT_BITLEN_SIZE (UCHAR_MAX + 1 + LZH_MAXMATCH - LZH_MINMATCH + 1)
/* Position table size.
 * Note: this used for both position table and pre literal table.*/
#define LZH_PT_BITLEN_SIZE (3 + 16)
#define LZH_HTBL_BITS 10

#define ST_RD_BLOCK 0
#define ST_RD_PT_1 1
#define ST_RD_PT_2 2
#define ST_RD_PT_3 3
#define ST_RD_PT_4 4
#define ST_RD_LITERAL_1 5
#define ST_RD_LITERAL_2 6
#define ST_RD_LITERAL_3 7
#define ST_RD_POS_DATA_1 8
#define ST_GET_LITERAL 9
#define ST_GET_POS_1 10
#define ST_GET_POS_2 11
#define ST_COPY_DATA 12

#define LZH_ARCHIVE_EOF 1       /* Found end of archive. */
#define LZH_ARCHIVE_OK 0        /* Operation was successful. */
#define LZH_ARCHIVE_RETRY (-10) /* Retry might succeed. */
#define LZH_ARCHIVE_WARN (-20)  /* Partial success. */
/* For example, if write_header "fails", then you can't push data. */
#define LZH_ARCHIVE_FAILED (-25) /* Current operation cannot complete. */
/* But if write_header is "fatal," then this archive is dead and useless. */
#define LZH_ARCHIVE_FATAL (-30) /* No more operations are possible. */

#define CACHE_TYPE quint64
#define CACHE_BITS (8 * sizeof(CACHE_TYPE))
#define ASIZE(x) (sizeof(x) / sizeof(x[0]))

    /*
     * Huffman coding.
     */
    struct lzh_htree_t {
        quint16 left;
        quint16 right;
    };

    /*
     * Bit stream reader.
     */
    struct lzh_br {
        /* Cache buffer. */
        quint64 cache_buffer;
        /* Indicates how many bits avail in cache_buffer. */
        qint32 cache_avail;
    };

    struct lzh_huffman {
        qint32 len_size;
        qint32 len_avail;
        qint32 len_bits;
        qint32 freq[17];
        quint8 *bitlen;

        /*
         * Use a index table. It's faster than searching a lzh_huffman
         * coding tree, which is a binary tree. But a use of a large
         * index table causes L1 cache read miss many times.
         */
        qint32 max_bits;
        qint32 shift_bits;
        qint32 tbl_bits;
        qint32 tree_used;
        qint32 tree_avail;
        /* Direct access table. */
        quint16 *tbl;
        /* Binary tree table for extra bits over the direct access. */
        lzh_htree_t *tree;
    };

    struct lzh_dec {
        /* Decoding status. */
        qint32 state;

        /*
         * Window to see last 8Ki(lh5),32Ki(lh6),64Ki(lh7) bytes of decoded
         * data.
         */
        qint32 w_size;
        qint32 w_mask;
        /* Window buffer, which is a loop buffer. */
        quint8 *w_buff;
        /* The insert position to the window. */
        qint32 w_pos;
        /* The position where we can copy decoded code from the window. */
        qint32 copy_pos;
        /* The length how many bytes we can copy decoded code from
         * the window. */
        qint32 copy_len;

        /*
         * Bit stream reader.
         */
        lzh_br br;

        lzh_huffman lt, pt;

        qint32 blocks_avail;
        qint32 pos_pt_len_size;
        qint32 pos_pt_len_bits;
        qint32 literal_pt_len_size;
        qint32 literal_pt_len_bits;
        qint32 reading_position;
        qint32 loop;
        qint32 error;
    };

    struct lzh_stream {
        const quint8 *next_in;
        qint32 avail_in;
        qint64 total_in;
        const quint8 *ref_ptr;
        qint32 avail_out;
        qint64 total_out;
        struct lzh_dec *ds;
    };

/*
 * Bit stream reader.
 */
/* Check that the cache buffer has enough bits. */
#define lzh_br_has(br, n) ((br)->cache_avail >= n)
/* Get compressed data by bit. */
#define lzh_br_bits(br, n) (((quint16)((br)->cache_buffer >> ((br)->cache_avail - (n)))) & cache_masks[n])
#define lzh_br_bits_forced(br, n) (((quint16)((br)->cache_buffer << ((n) - (br)->cache_avail))) & cache_masks[n])
/* Read ahead to make sure the cache buffer has enough compressed data we
 * will use.
 *  True  : completed, there is enough data in the cache buffer.
 *  False : we met that strm->next_in is empty, we have to get following
 *          bytes. */
#define lzh_br_read_ahead_0(strm, br, n) (lzh_br_has(br, (n)) || lzh_br_fillup(strm, br))
/*  True  : the cache buffer has some bits as much as we need.
 *  False : there are no enough bits in the cache buffer to be used,
 *          we have to get following bytes if we could. */
#define lzh_br_read_ahead(strm, br, n) (lzh_br_read_ahead_0((strm), (br), (n)) || lzh_br_has((br), (n)))

/* Notify how many bits we consumed. */
#define lzh_br_consume(br, n) ((br)->cache_avail -= (n))
#define lzh_br_unconsume(br, n) ((br)->cache_avail += (n))

    static bool lzh_decode_init(struct lzh_stream *strm, qint32 method);
    static bool lzh_huffman_init(struct lzh_huffman *hf, size_t len_size, qint32 tbl_bits);
    static qint32 lzh_decode(struct lzh_stream *strm, qint32 last);
    static qint32 lzh_read_blocks(struct lzh_stream *strm, qint32 last);
    static qint32 lzh_decode_blocks(struct lzh_stream *strm, qint32 last);
    static qint32 lzh_br_fillup(struct lzh_stream *strm, struct lzh_br *br);
    static void lzh_emit_window(struct lzh_stream *strm, size_t s);
    static qint32 lzh_decode_huffman_tree(struct lzh_huffman *hf, unsigned rbits, qint32 c);
    static inline qint32 lzh_decode_huffman(struct lzh_huffman *hf, unsigned rbits);
    static qint32 lzh_make_fake_table(struct lzh_huffman *hf, quint16 c);
    static qint32 lzh_read_pt_bitlen(struct lzh_stream *strm, qint32 start, qint32 end);
    static qint32 lzh_make_huffman_table(struct lzh_huffman *hf);
    static void lzh_decode_free(struct lzh_stream *strm);
    static void lzh_huffman_free(struct lzh_huffman *hf);
};

#endif  // XCOMPRESS_H
