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
#ifndef XCOMPRESS_H
#define XCOMPRESS_H

#include <QObject>

class XCompress
{
public:

#define MAXMATCH		256	/* Maximum match length. */
#define MINMATCH		3	/* Minimum match length. */
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
#define LT_BITLEN_SIZE		(UCHAR_MAX + 1 + MAXMATCH - MINMATCH + 1)
/* Position table size.
 * Note: this used for both position table and pre literal table.*/
#define PT_BITLEN_SIZE		(3 + 16)
#define HTBL_BITS	10

    /*
     * Huffman coding.
     */
    struct htree_t {
        uint16_t left;
        uint16_t right;
    };

    struct huffman {
        int		 len_size;
        int		 len_avail;
        int		 len_bits;
        int		 freq[17];
        unsigned char	*bitlen;

        /*
         * Use a index table. It's faster than searching a huffman
         * coding tree, which is a binary tree. But a use of a large
         * index table causes L1 cache read miss many times.
         */
        int		 max_bits;
        int		 shift_bits;
        int		 tbl_bits;
        int		 tree_used;
        int		 tree_avail;
        /* Direct access table. */
        uint16_t	*tbl;
        /* Binary tree table for extra bits over the direct access. */
        htree_t *tree;
    };

    struct lzh_dec {
        /* Decoding status. */
        int     		 state;

        /*
         * Window to see last 8Ki(lh5),32Ki(lh6),64Ki(lh7) bytes of decoded
         * data.
         */
        int			 w_size;
        int			 w_mask;
        /* Window buffer, which is a loop buffer. */
        unsigned char		*w_buff;
        /* The insert position to the window. */
        int			 w_pos;
        /* The position where we can copy decoded code from the window. */
        int     		 copy_pos;
        /* The length how many bytes we can copy decoded code from
         * the window. */
        int     		 copy_len;

        /*
         * Bit stream reader.
         */
        struct lzh_br {
            /* Cache buffer. */
            uint64_t	 cache_buffer;
            /* Indicates how many bits avail in cache_buffer. */
            int		 cache_avail;
        } br;

        huffman lt, pt;

        int			 blocks_avail;
        int			 pos_pt_len_size;
        int			 pos_pt_len_bits;
        int			 literal_pt_len_size;
        int			 literal_pt_len_bits;
        int			 reading_position;
        int			 loop;
        int			 error;
    };

    struct lzh_stream {
        const unsigned char	*next_in;
        int			 avail_in;
        int64_t			 total_in;
        const unsigned char	*ref_ptr;
        int			 avail_out;
        int64_t			 total_out;
        struct lzh_dec		*ds;
    };

    XCompress();

    static bool lzh_decode_init(struct lzh_stream *strm, int method);
    static bool lzh_huffman_init(struct huffman *hf, size_t len_size, int tbl_bits);
};

#endif // XCOMPRESS_H
