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
#include "ximplodedecoder.h"

#define UI6A_VERSION 20210124

#ifndef UI6A_UINT8
#define UI6A_UINT8 unsigned char
#endif
#ifndef UI6A_UINT16
#define UI6A_UINT16 uint16_t
#endif
#ifndef UI6A_UINT32
#define UI6A_UINT32 uint32_t
#endif
#ifndef UI6A_OFF_T
#define UI6A_OFF_T long
#endif
#ifndef UI6A_CALLOC
#define UI6A_CALLOC(u, nmemb, size, ty) (ty) calloc((nmemb), (size))
#endif
#ifndef UI6A_FREE
#define UI6A_FREE(u, ptr) free(ptr)
#endif
#ifndef UI6A_ZEROMEM
#define UI6A_ZEROMEM(ptr, size) memset((ptr), 0, (size))
#endif

#ifndef UI6A_API
#define UI6A_API(ty) static ty
#endif

#define UI6A_ERRCODE_OK 0
#define UI6A_ERRCODE_GENERIC_ERROR 1
#define UI6A_ERRCODE_BAD_CDATA 2
#define UI6A_ERRCODE_MALLOC_FAILED 3
#define UI6A_ERRCODE_READ_FAILED 6
#define UI6A_ERRCODE_WRITE_FAILED 7
#define UI6A_ERRCODE_INSUFFICIENT_CDATA 8

#define UI6A_FLAG_4KDICT 0x0000
#define UI6A_FLAG_8KDICT 0x0002
#define UI6A_FLAG_2TREES 0x0000
#define UI6A_FLAG_3TREES 0x0004

#define UI6A_ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

#define UI6A_WSIZE 0x2000 /* window size--must be a power of two, and */
/* at least 8K for zip's implode method */

struct ui6a_huftarray;

struct ui6a_huft {
    UI6A_UINT8 e;                 /* number of extra bits or operation */
    UI6A_UINT8 b;                 /* number of bits in this code or subcode */
    UI6A_UINT16 n;                /* literal, length base, or distance base */
    struct ui6a_huftarray *t_arr; /* pointer to next level of table */
};

struct ui6a_huftarray {
    unsigned int num_alloc_h;
    struct ui6a_huft *h;
    struct ui6a_huftarray *next_array;
};

struct ui6a_htable {
    struct ui6a_huftarray *first_array;
    int b; /* bits for this table */
    const char *tblname;
};

struct ui6a_htables {
    struct ui6a_htable b; /* literal code table */
    struct ui6a_htable l; /* length code table */
    struct ui6a_htable d; /* distance code table */
};

struct ui6a_ctx_struct;
typedef struct ui6a_ctx_struct ui6a_ctx;

typedef size_t (*ui6a_cb_read_type)(ui6a_ctx *ui6a, UI6A_UINT8 *buf, size_t size);
typedef size_t (*ui6a_cb_write_type)(ui6a_ctx *ui6a, const UI6A_UINT8 *buf, size_t size);
typedef void (*ui6a_cb_post_read_trees_type)(ui6a_ctx *ui6a, struct ui6a_htables *tbls);

struct ui6a_ctx_struct {
    // Fields the user can set:
    void *userdata;
    UI6A_OFF_T cmpr_size;    // compressed size
    UI6A_OFF_T uncmpr_size;  // reported uncompressed size
    UI6A_UINT16 bit_flags;   // Sum of UI6A_FLAG_* values
    UI6A_UINT8 emulate_pkzip10x;
    ui6a_cb_read_type cb_read;
    ui6a_cb_write_type cb_write;
    ui6a_cb_post_read_trees_type cb_post_read_trees;  // Optional hook

    // Fields the user can read:
    int error_code;  // UI6A_ERRCODE_*
    UI6A_OFF_T uncmpr_nbytes_written;
    UI6A_OFF_T cmpr_nbytes_consumed;

    // Fields private to the library:
    UI6A_OFF_T cmpr_nbytes_read;
    size_t inbuf_nbytes_consumed;
    size_t inbuf_nbytes_total;
    UI6A_UINT8 Slide[UI6A_WSIZE];
    UI6A_UINT8 inbuf[4096];
};

typedef UI6A_UINT16 (*ui6a_len_or_dist_getter)(unsigned int i);

struct ui6a_iarray {
    size_t count;
    int *data;
    ui6a_ctx *ui6a;
};

struct ui6a_uarray {
    size_t count;
    unsigned int *data;
    ui6a_ctx *ui6a;
};

static void ui6a_set_error(ui6a_ctx *ui6a, int error_code)
{
    // Only record the first error.
    if (ui6a->error_code == UI6A_ERRCODE_OK) {
        ui6a->error_code = error_code;
    }
}

static UI6A_UINT16 ui6a_get_mask_bits(unsigned int n)
{
    if (n >= 17) return 0;
    return (UI6A_UINT16)(0xffffU >> (16 - n));
}

static int ui6a_nextbyte(ui6a_ctx *ui6a)
{
    size_t ret;
    size_t nbytes_to_read;
    UI6A_UINT8 ch = 0xff;

    if (ui6a->error_code != UI6A_ERRCODE_OK) {
        goto done;
    }

    if (ui6a->inbuf_nbytes_consumed < ui6a->inbuf_nbytes_total) {
        // Next byte is available in inbuf.
        ch = ui6a->inbuf[ui6a->inbuf_nbytes_consumed++];
        ui6a->cmpr_nbytes_consumed++;
        goto done;
    }

    // No more bytes in inbuf ...
    ui6a->inbuf_nbytes_consumed = 0;
    ui6a->inbuf_nbytes_total = 0;

    if (ui6a->cmpr_nbytes_consumed >= ui6a->cmpr_size) {
        // ... and there are no more to read.
        if (ui6a->cmpr_nbytes_consumed == ui6a->cmpr_size) {
            // Allow reading one byte too many, before calling it an error.
            ui6a->cmpr_nbytes_consumed++;
        } else {
            ui6a_set_error(ui6a, UI6A_ERRCODE_INSUFFICIENT_CDATA);
        }
        goto done;
    }

    // ... but we can read more.
    if (ui6a->cmpr_size - ui6a->cmpr_nbytes_read < (UI6A_OFF_T)sizeof(ui6a->inbuf)) {
        nbytes_to_read = (size_t)(ui6a->cmpr_size - ui6a->cmpr_nbytes_read);
    } else {
        nbytes_to_read = sizeof(ui6a->inbuf);
    }

    ret = ui6a->cb_read(ui6a, ui6a->inbuf, nbytes_to_read);
    if (ret != nbytes_to_read) {
        ui6a_set_error(ui6a, UI6A_ERRCODE_READ_FAILED);
        goto done;
    }
    ui6a->inbuf_nbytes_total = nbytes_to_read;
    ui6a->cmpr_nbytes_read += (UI6A_OFF_T)nbytes_to_read;
    ch = ui6a->inbuf[ui6a->inbuf_nbytes_consumed++];
    ui6a->cmpr_nbytes_consumed++;

done:
    return (int)ch;
}

static void ui6a_flush(ui6a_ctx *ui6a, const UI6A_UINT8 *rawbuf, size_t size)
{
    size_t ret;
    size_t nbytes_to_write = size;

    if (size < 1) return;
    if (ui6a->uncmpr_nbytes_written >= ui6a->uncmpr_size) return;
    if (ui6a->uncmpr_nbytes_written + (UI6A_OFF_T)nbytes_to_write > ui6a->uncmpr_size) {
        nbytes_to_write = (size_t)(ui6a->uncmpr_size - ui6a->uncmpr_nbytes_written);
    }

    ret = ui6a->cb_write(ui6a, rawbuf, nbytes_to_write);
    if (ret != nbytes_to_write) {
        ui6a_set_error(ui6a, UI6A_ERRCODE_WRITE_FAILED);
    }
    ui6a->uncmpr_nbytes_written += (UI6A_OFF_T)size;
}

/* (virtual) Tables for length and distance */

static UI6A_UINT16 ui6a_get_cplen2(unsigned int i)
{
    if (i >= 64) return 0;
    return i + 2;
}

static UI6A_UINT16 ui6a_get_cplen3(unsigned int i)
{
    if (i >= 64) return 0;
    return i + 3;
}

static UI6A_UINT16 ui6a_get_extra(unsigned int i)
{
    return (i == 63) ? 8 : 0;
}

static UI6A_UINT16 ui6a_get_cpdist4(unsigned int i)
{
    if (i >= 64) return 0;
    return 1 + i * 64;
}

static UI6A_UINT16 ui6a_get_cpdist8(unsigned int i)
{
    if (i >= 64) return 0;
    return 1 + i * 128;
}

static void ui6a_iarray_init(ui6a_ctx *ui6a, struct ui6a_iarray *a, int *data, size_t count)
{
    UI6A_ZEROMEM(data, count * sizeof(int));
    a->data = data;
    a->count = count;
    a->ui6a = ui6a;
}

static void ui6a_uarray_init(ui6a_ctx *ui6a, struct ui6a_uarray *a, unsigned int *data, size_t count)
{
    UI6A_ZEROMEM(data, count * sizeof(unsigned int));
    a->data = data;
    a->count = count;
    a->ui6a = ui6a;
}

static void ui6a_iarray_setval(struct ui6a_iarray *a, size_t idx, int val)
{
    if (idx < a->count) {
        a->data[idx] = val;
    }
}

static void ui6a_uarray_setval(struct ui6a_uarray *a, size_t idx, unsigned int val)
{
    if (idx < a->count) {
        a->data[idx] = val;
    }
}

static int ui6a_iarray_getval(struct ui6a_iarray *a, size_t idx)
{
    if (idx < a->count) {
        return a->data[idx];
    }
    return 0;
}

static unsigned int ui6a_uarray_getval(struct ui6a_uarray *a, size_t idx)
{
    if (idx < a->count) {
        return a->data[idx];
    }
    return 0;
}

/* Get the bit lengths for a code representation from the compressed
   stream.  On error, sets ui6a->error_code. */
// l: bit lengths
// n: number expected
static void ui6a_get_tree(ui6a_ctx *ui6a, unsigned int *l, unsigned int n)
{
    unsigned int i; /* bytes remaining in list */
    unsigned int k; /* lengths entered */
    unsigned int j; /* number of codes */
    unsigned int b; /* bit length for those codes */

    /* get bit lengths */
    i = ui6a_nextbyte(ui6a) + 1; /* length/count pairs to read */
    k = 0;                       /* next code */
    do {
        b = ((j = ui6a_nextbyte(ui6a)) & 0xf) + 1; /* bits in code (1..16) */
        j = ((j & 0xf0) >> 4) + 1;                 /* codes with those bits (1..16) */
        if (k + j > n) {
            ui6a_set_error(ui6a, UI6A_ERRCODE_BAD_CDATA);
            return; /* don't overflow l[] */
        }
        do {
            l[k++] = b;
        } while (--j);
    } while (--i);

    if (k != n) { /* should have read n of them */
        ui6a_set_error(ui6a, UI6A_ERRCODE_BAD_CDATA);
    }
}

#define UI6A_BMAX 16   /* maximum bit length of any code (16 for explode) */
#define UI6A_N_MAX 288 /* maximum number of codes in any set */

/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.
   On error, sets ui6a->error_code. (Tables might still be created.)

   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
// b: code lengths in bits (all assumed <= UI6A_BMAX)
// n: number of codes (assumed <= UI6A_N_MAX)
// s: number of simple-valued codes (0..s-1)
// d_fn: (function supplying the) list of base values for non-simple codes
// e_fn: (function supplying the) list of extra bits for non-simple codes
// tbl->t: result: starting table
// tbl->b: maximum lookup bits, returns actual
static void ui6a_huft_build(ui6a_ctx *ui6a, const unsigned int *b, unsigned int n, unsigned int s, ui6a_len_or_dist_getter d_fn, ui6a_len_or_dist_getter e_fn,
                            struct ui6a_htable *tbl)
{
    unsigned int a;           /* counter for codes of length k */
    struct ui6a_uarray c_arr; /* bit length count table */
    unsigned int c_data[UI6A_BMAX + 1];
    unsigned int el; /* length of EOB code (value 256) */
    unsigned int f;  /* i repeats in table every f entries */
    int g;           /* maximum code length */
    int h;           /* table level */
    unsigned int i;  /* counter, current code */
    unsigned int j;  /* counter */
    int k;           /* number of bits in current code */
    struct ui6a_iarray lx_arr;
    int lx_data[UI6A_BMAX + 1];          /* &lx_data[1] = stack of bits per table */
    struct ui6a_huft *q;                 /* points to current table */
    struct ui6a_huft r;                  /* table entry for structure assignment */
    struct ui6a_huftarray *u[UI6A_BMAX]; /* table stack */
    struct ui6a_uarray v_arr;            /* values in order of bit length */
    unsigned int v_data[UI6A_N_MAX];
    int w;                    /* bits before this table */
    struct ui6a_uarray x_arr; /* bit offsets, then code stack */
    unsigned int x_data[UI6A_BMAX + 1];
    int y;          /* number of dummy codes added */
    unsigned int z; /* number of entries in current table */
    unsigned int c_idx;
    unsigned int v_idx;
    unsigned int x_idx;
    struct ui6a_huftarray *prev_array = NULL;

    if (n > 256) goto done;

    /* Generate counts for each bit length */
    el = UI6A_BMAX; /* set length of EOB code, if any */
    ui6a_uarray_init(ui6a, &c_arr, c_data, UI6A_ARRAYSIZE(c_data));

    for (i = 0; i < n; i++) {
        if (b[i] >= UI6A_BMAX + 1) goto done;
        /* assume all entries <= UI6A_BMAX */
        ui6a_uarray_setval(&c_arr, b[i], ui6a_uarray_getval(&c_arr, b[i]) + 1);
    }

    if (ui6a_uarray_getval(&c_arr, 0) == n) { /* null input--all zero length codes */
        tbl->b = 0;
        return;
    }

    /* Find minimum and maximum length, bound *m by those */
    for (j = 1; j <= UI6A_BMAX; j++) {
        if (ui6a_uarray_getval(&c_arr, j)) break;
    }
    k = j; /* minimum code length */
    if ((unsigned int)tbl->b < j) tbl->b = j;
    for (i = UI6A_BMAX; i; i--) {
        if (ui6a_uarray_getval(&c_arr, i)) break;
    }
    g = i; /* maximum code length */
    if ((unsigned int)tbl->b > i) tbl->b = i;

    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1) {
        y -= ui6a_uarray_getval(&c_arr, j);
        if (y < 0) {
            ui6a_set_error(ui6a, UI6A_ERRCODE_BAD_CDATA);
            return; /* bad input: more codes than bits */
        }
    }
    y -= ui6a_uarray_getval(&c_arr, i);
    if (y < 0) {
        ui6a_set_error(ui6a, UI6A_ERRCODE_BAD_CDATA);
        return;
    }
    ui6a_uarray_setval(&c_arr, i, ui6a_uarray_getval(&c_arr, i) + y);

    /* Generate starting offsets into the value table for each length */
    j = 0;
    ui6a_uarray_init(ui6a, &x_arr, x_data, UI6A_ARRAYSIZE(x_data));
    ui6a_uarray_setval(&x_arr, 1, 0);
    c_idx = 1;
    x_idx = 2;
    while (--i) { /* note that i == g from above */
        j += ui6a_uarray_getval(&c_arr, c_idx);
        c_idx++;
        ui6a_uarray_setval(&x_arr, x_idx, j);
        x_idx++;
    }

    /* Make a table of values in order of bit lengths */
    ui6a_uarray_init(ui6a, &v_arr, v_data, UI6A_ARRAYSIZE(v_data));
    for (i = 0; i < n; i++) {
        j = b[i];
        if (j != 0) {
            ui6a_uarray_setval(&v_arr, ui6a_uarray_getval(&x_arr, j), i);
            ui6a_uarray_setval(&x_arr, j, ui6a_uarray_getval(&x_arr, j) + 1);
        }
    }
    n = ui6a_uarray_getval(&x_arr, g); /* set n to length of v */

    /* Generate the Huffman codes and for each, make the table entries */
    i = 0; /* first Huffman code is zero */
    ui6a_uarray_setval(&x_arr, 0, 0);
    v_idx = 0; /* grab values in bit order */
    h = -1;    /* no tables yet--level -1 */
    ui6a_iarray_init(ui6a, &lx_arr, lx_data, UI6A_ARRAYSIZE(lx_data));
    ui6a_iarray_setval(&lx_arr, 0, 0); /* no bits decoded yet */
    w = 0;
    u[0] = NULL; /* just to keep compilers happy */
    q = NULL;    /* ditto */
    z = 0;       /* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= g; k++) {
        a = ui6a_uarray_getval(&c_arr, k);
        while (a--) {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > w + ui6a_iarray_getval(&lx_arr, 1 + (size_t)h)) {
                struct ui6a_huftarray *ha;

                w += ui6a_iarray_getval(&lx_arr, 1 + (size_t)h); /* add bits already decoded */
                h++;

                /* compute minimum size table less than or equal to *m bits */
                z = g - w;
                z = (z > (unsigned int)tbl->b) ? ((unsigned int)tbl->b) : z; /* upper limit */
                j = k - w;
                f = 1 << j;
                if (f > a + 1) { /* try a k-w bit table */
                    /* too few codes for k-w bit table */
                    f -= a + 1; /* deduct codes from patterns left */

                    c_idx = k;
                    while (++j < z) { /* try smaller tables up to z bits */
                        c_idx++;
                        f <<= 1;
                        if (f <= ui6a_uarray_getval(&c_arr, c_idx)) break; /* enough codes to use up j bits */
                        f -= ui6a_uarray_getval(&c_arr, c_idx);            /* else deduct codes from patterns */
                    }
                }
                if ((unsigned int)w + j > el && (unsigned int)w < el) j = el - w; /* make EOB code end at table */
                z = 1 << j;                                                       /* table entries for j-bit table */
                ui6a_iarray_setval(&lx_arr, 1 + (size_t)h, j);                    /* set table size in stack */

                /* allocate and link in new table */
                ha = UI6A_CALLOC(ui6a->userdata, 1, sizeof(struct ui6a_huftarray), struct ui6a_huftarray *);
                if (!ha) {
                    ui6a_set_error(ui6a, UI6A_ERRCODE_MALLOC_FAILED);
                    goto done;
                }
                ha->h = UI6A_CALLOC(ui6a->userdata, (size_t)z, sizeof(struct ui6a_huft), struct ui6a_huft *);
                if (!ha->h) {
                    UI6A_FREE(ui6a->userdata, ha);
                    ui6a_set_error(ui6a, UI6A_ERRCODE_MALLOC_FAILED);
                    goto done;
                }
                ha->num_alloc_h = z;
                q = ha->h;

                /* link to list for ui6a_huft_free() */
                if (prev_array) {
                    prev_array->next_array = ha;
                } else {
                    tbl->first_array = ha;
                }
                ha->next_array = NULL;
                prev_array = ha;

                if (h < 0 || h >= UI6A_BMAX) goto done;
                u[h] = ha;

                /* connect to last table, if there is one */
                if (h) {
                    UI6A_ZEROMEM(&r, sizeof(struct ui6a_huft));
                    ui6a_uarray_setval(&x_arr, h, i); /* save pattern for backing up */
                    /* bits to dump before this table */
                    r.b = (UI6A_UINT8)ui6a_iarray_getval(&lx_arr, 1 + (size_t)h - 1);
                    r.e = (UI6A_UINT8)(16 + j); /* bits in this table */
                    r.t_arr = ha;               /* pointer to this table */
                    j = (i & ((1 << w) - 1)) >> (w - ui6a_iarray_getval(&lx_arr, 1 + (size_t)h - 1));
                    if ((h - 1 < 0) || (h - 1 >= UI6A_BMAX)) goto done;
                    u[h - 1]->h[j] = r; /* connect to last table */
                }
            }

            /* set up table entry in r */
            UI6A_ZEROMEM(&r, sizeof(struct ui6a_huft));
            r.b = (UI6A_UINT8)(k - w);
            if (v_idx >= n) {
                r.e = 99; /* out of values--invalid code */
            } else if (ui6a_uarray_getval(&v_arr, v_idx) < s) {
                /* 256 is end-of-block code */
                r.e = (UI6A_UINT8)(ui6a_uarray_getval(&v_arr, v_idx) < 256 ? 16 : 15);
                /* simple code is just the value */
                r.n = (UI6A_UINT16)ui6a_uarray_getval(&v_arr, v_idx);
                v_idx++;
            } else {
                /* non-simple--look up in lists */
                r.e = (UI6A_UINT8)e_fn(ui6a_uarray_getval(&v_arr, v_idx) - s);
                r.n = d_fn(ui6a_uarray_getval(&v_arr, v_idx) - s);
                v_idx++;
            }

            /* fill code-like entries with r */
            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f) {
                q[j] = r;
            }

            /* backwards increment the k-bit code i */
            for (j = 1 << (k - 1); i & j; j >>= 1) {
                i ^= j;
            }
            i ^= j;

            /* backup over finished tables */
            while ((i & ((1 << w) - 1)) != ui6a_uarray_getval(&x_arr, h)) {
                --h;
                w -= ui6a_iarray_getval(&lx_arr, 1 + (size_t)h); /* don't need to update q */
            }
        }
    }

    /* return actual size of base table */
    tbl->b = ui6a_iarray_getval(&lx_arr, 1 + 0);

    /* Check if we were given an incomplete table */
    if (y != 0 && g != 1) {
        ui6a_set_error(ui6a, UI6A_ERRCODE_BAD_CDATA);
    }

done:
    return;
}

#undef UI6A_BMAX
#undef UI6A_N_MAX

/* Free the malloc'ed tables built by ui6a_huft_build(), which makes a linked
   list of the tables it made. */
static void ui6a_huft_free(ui6a_ctx *ui6a, struct ui6a_htable *tbl)
{
    struct ui6a_huftarray *p;

    p = tbl->first_array;
    while (p) {
        struct ui6a_huftarray *q;

        q = p->next_array;
        UI6A_FREE(ui6a->userdata, p->h);
        UI6A_FREE(ui6a->userdata, p);
        p = q;
    }
}

static struct ui6a_huft *ui6a_huftarr_plus_offset(struct ui6a_huftarray *ha, UI6A_UINT32 offset)
{
    if (offset >= ha->num_alloc_h) {
        return NULL;
    }
    return &(ha->h[offset]);
}

static struct ui6a_huft *ui6a_follow_huft_ptr(struct ui6a_huft *h1, UI6A_UINT32 offset)
{
    return ui6a_huftarr_plus_offset(h1->t_arr, offset);
}

/* Macros for bit peeking and grabbing.
   The usage is:

        UI6A_NEEDBITS(j);
        x = b & mask_bits[j];
        UI6A_DUMPBITS(j);

   where UI6A_NEEDBITS makes sure that b has at least j bits in it, and
   UI6A_DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.
 */
#define UI6A_NEEDBITS(n)                                  \
    do {                                                  \
        while (k < (n)) {                                 \
            b |= ((UI6A_UINT32)ui6a_nextbyte(ui6a)) << k; \
            k += 8;                                       \
        }                                                 \
    } while (0)
#define UI6A_DUMPBITS(n) \
    do {                 \
        b >>= (n);       \
        k -= (n);        \
    } while (0)

// tb, tl, td: literal, length, and distance tables
//  Uses literals if tbls->b.t!=NULL.
// bb, bl, bd: number of bits decoded by those
static void ui6a_unimplode_internal(ui6a_ctx *ui6a, unsigned int window_k, struct ui6a_htables *tbls)
{
    UI6A_OFF_T s;            /* bytes to decompress */
    unsigned int e;          /* table entry flag/number of extra bits */
    unsigned int n, d;       /* length and index for copy */
    unsigned int w;          /* current window position */
    struct ui6a_huft *t;     /* pointer to table entry */
    unsigned int mb, ml, md; /* masks for (b, l, d) bits */
    UI6A_UINT32 b;           /* bit buffer */
    unsigned int k;          /* number of bits in bit buffer */
    int ok = 0;

    /* explode the coded data */
    b = k = w = 0;                      /* initialize bit buffer, window */
    mb = ui6a_get_mask_bits(tbls->b.b); /* precompute masks */
    ml = ui6a_get_mask_bits(tbls->l.b);
    md = ui6a_get_mask_bits(tbls->d.b);
    s = ui6a->uncmpr_size;
    while (s > 0) { /* do until uncmpr_size bytes uncompressed */
        if (ui6a->error_code != UI6A_ERRCODE_OK) {
            goto done;
        }
        UI6A_NEEDBITS(1);
        if (b & 1) { /* then literal--decode it */
            UI6A_DUMPBITS(1);
            s--;
            if (tbls->b.first_array) {
                UI6A_NEEDBITS((unsigned int)tbls->b.b); /* get coded literal */
                t = ui6a_huftarr_plus_offset(tbls->b.first_array, ((~(unsigned int)b) & mb));
                if (!t) goto done;
                e = t->e;
                if (e > 16) {
                    do {
                        if (e == 99) goto done;
                        UI6A_DUMPBITS(t->b);
                        e -= 16;
                        UI6A_NEEDBITS(e);
                        t = ui6a_follow_huft_ptr(t, ((~(unsigned int)b) & ui6a_get_mask_bits(e)));
                        if (!t) goto done;
                        e = t->e;
                    } while (e > 16);
                }
                UI6A_DUMPBITS(t->b);
                ui6a->Slide[w++] = (UI6A_UINT8)t->n;
            } else {
                UI6A_NEEDBITS(8);
                ui6a->Slide[w++] = (UI6A_UINT8)b;
            }
            if (w == UI6A_WSIZE) {
                ui6a_flush(ui6a, ui6a->Slide, (size_t)w);
                w = 0;
            }
            if (!tbls->b.first_array) {
                UI6A_DUMPBITS(8);
            }
        } else { /* else distance/length */
            UI6A_DUMPBITS(1);

            if (window_k == 8) {
                UI6A_NEEDBITS(7); /* get distance low bits */
                d = (unsigned int)b & 0x7f;
                UI6A_DUMPBITS(7);
            } else {
                UI6A_NEEDBITS(6); /* get distance low bits */
                d = (unsigned int)b & 0x3f;
                UI6A_DUMPBITS(6);
            }

            UI6A_NEEDBITS((unsigned int)tbls->d.b); /* get coded distance high bits */
            t = ui6a_huftarr_plus_offset(tbls->d.first_array, ((~(unsigned int)b) & md));
            if (!t) goto done;
            e = t->e;
            if (e > 16) {
                do {
                    if (e == 99) goto done;
                    UI6A_DUMPBITS(t->b);
                    e -= 16;
                    UI6A_NEEDBITS(e);
                    t = ui6a_follow_huft_ptr(t, ((~(unsigned int)b) & ui6a_get_mask_bits(e)));
                    if (!t) goto done;
                    e = t->e;
                } while (e > 16);
            }
            UI6A_DUMPBITS(t->b);
            d = w - d - t->n;                       /* construct offset */
            UI6A_NEEDBITS((unsigned int)tbls->l.b); /* get coded length */
            t = ui6a_huftarr_plus_offset(tbls->l.first_array, ((~(unsigned int)b) & ml));
            if (!t) goto done;
            e = t->e;
            if (e > 16) {
                do {
                    if (e == 99) goto done;
                    UI6A_DUMPBITS(t->b);
                    e -= 16;
                    UI6A_NEEDBITS(e);
                    t = ui6a_follow_huft_ptr(t, ((~(unsigned int)b) & ui6a_get_mask_bits(e)));
                    if (!t) goto done;
                    e = t->e;
                } while (e > 16);
            }
            UI6A_DUMPBITS(t->b);
            n = t->n;
            if (e) { /* get length extra bits */
                UI6A_NEEDBITS(8);
                n += (unsigned int)b & 0xff;
                UI6A_DUMPBITS(8);
            }

            /* do the copy */
            s -= (UI6A_OFF_T)n;

            do {
                d &= (UI6A_WSIZE - 1);
                w &= (UI6A_WSIZE - 1);
                ui6a->Slide[w++] = ui6a->Slide[d++];

                if (w == UI6A_WSIZE) {
                    ui6a_flush(ui6a, ui6a->Slide, (UI6A_UINT32)w);
                    w = 0;
                }

                n--;
            } while (n);
        }
    }

    /* flush out ui6a->Slide */
    ui6a_flush(ui6a, ui6a->Slide, (UI6A_UINT32)w);
    ok = 1;

done:
    if (!ok) {
        ui6a_set_error(ui6a, UI6A_ERRCODE_GENERIC_ERROR);
    }

    if (ui6a->cmpr_nbytes_consumed > ui6a->cmpr_size) {
        // Don't claim we read more bytes than available.
        ui6a->cmpr_nbytes_consumed = ui6a->cmpr_size;
    }
}

#undef UI6A_NEEDBITS
#undef UI6A_DUMPBITS

/* Explode an imploded compressed stream.  Based on the general purpose
   bit flag, decide on coded or uncoded literals, and an 8K or 4K sliding
   window.  Construct the literal (if any), length, and distance codes and
   the tables needed to decode them (using ui6a_huft_build()),
   and call [ui6a_unimplode_internal() to do the real work]. */
UI6A_API(void) ui6a_unimplode(ui6a_ctx *ui6a)
{
    struct ui6a_htables tbls;
    unsigned int l[256]; /* bit lengths for codes */
    int has_literal_tree;
    int has_8k_window;
    ui6a_len_or_dist_getter len_getter;

    UI6A_ZEROMEM(&tbls, sizeof(struct ui6a_htables));
    tbls.b.tblname = "B";
    tbls.l.tblname = "L";
    tbls.d.tblname = "D";

    has_8k_window = (ui6a->bit_flags & UI6A_FLAG_8KDICT) ? 1 : 0;
    has_literal_tree = (ui6a->bit_flags & UI6A_FLAG_3TREES) ? 1 : 0;

    /* Tune base table sizes. [...] */
    // Note: The tbls.*.b values were optimized for best speed under conditions
    // that are presumably no longer relevant.
    // I'm leaving them as they are, though, because I don't know what to
    // change them to.
    tbls.l.b = 7;
    tbls.d.b = ui6a->cmpr_size > 200000 ? 8 : 7;

    if (has_literal_tree) { /* With literal tree--minimum match length is 3 */
        tbls.b.b = 9;       /* base table size for literals */
        ui6a_get_tree(ui6a, l, 256);
        if (ui6a->error_code != UI6A_ERRCODE_OK) goto done;
        ui6a_huft_build(ui6a, l, 256, 256, NULL, NULL, &tbls.b);
        if (ui6a->error_code != UI6A_ERRCODE_OK) goto done;
    } else { /* No literal tree--minimum match length is 2 */
        tbls.b.first_array = NULL;
    }

    ui6a_get_tree(ui6a, l, 64);
    if (ui6a->error_code != UI6A_ERRCODE_OK) goto done;
    if (ui6a->emulate_pkzip10x) {
        len_getter = has_8k_window ? ui6a_get_cplen3 : ui6a_get_cplen2;
    } else {
        len_getter = has_literal_tree ? ui6a_get_cplen3 : ui6a_get_cplen2;
    }
    ui6a_huft_build(ui6a, l, 64, 0, len_getter, ui6a_get_extra, &tbls.l);
    if (ui6a->error_code != UI6A_ERRCODE_OK) goto done;

    ui6a_get_tree(ui6a, l, 64);
    if (ui6a->error_code != UI6A_ERRCODE_OK) goto done;
    ui6a_huft_build(ui6a, l, 64, 0, (has_8k_window ? ui6a_get_cpdist8 : ui6a_get_cpdist4), ui6a_get_extra, &tbls.d);
    if (ui6a->error_code != UI6A_ERRCODE_OK) goto done;

    if (ui6a->cb_post_read_trees) {
        ui6a->cb_post_read_trees(ui6a, &tbls);
    }

    ui6a_unimplode_internal(ui6a, (has_8k_window ? 8 : 4), &tbls);

done:
    ui6a_huft_free(ui6a, &tbls.d);
    ui6a_huft_free(ui6a, &tbls.l);
    ui6a_huft_free(ui6a, &tbls.b);
}

UI6A_API(ui6a_ctx *) ui6a_create(void *userdata)
{
    ui6a_ctx *ui6a = UI6A_CALLOC(userdata, 1, sizeof(ui6a_ctx), ui6a_ctx *);

    if (!ui6a) return NULL;
    ui6a->userdata = userdata;
    return ui6a;
}

UI6A_API(void) ui6a_destroy(ui6a_ctx *ui6a)
{
    if (!ui6a) return;
    UI6A_FREE(ui6a->userdata, ui6a);
}

static size_t my_read(ui6a_ctx *ui6a, UI6A_UINT8 *buf, size_t size)
{
    XBinary::DECOMPRESS_STATE *pDecompressState = (XBinary::DECOMPRESS_STATE *)ui6a->userdata;
    return XBinary::_readDevice((char *)buf, size, pDecompressState);
}

static size_t my_write(ui6a_ctx *ui6a, const UI6A_UINT8 *buf, size_t size)
{
    XBinary::DECOMPRESS_STATE *pDecompressState = (XBinary::DECOMPRESS_STATE *)ui6a->userdata;
    return XBinary::_writeDevice((char *)buf, size, pDecompressState);
}

XImplodeDecoder::XImplodeDecoder(QObject *parent) : QObject(parent)
{
}

bool XImplodeDecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    UI6A_OFF_T cmpr_size = pDecompressState->nInputLimit;
    UI6A_OFF_T uncmpr_size = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    UI6A_UINT16 bit_flags = 0;

    if (pDecompressState->mapProperties.value(XBinary::FPART_PROP_COMPRESSION_OPTION_0, false).toBool()) bit_flags |= UI6A_FLAG_8KDICT;
    if (pDecompressState->mapProperties.value(XBinary::FPART_PROP_COMPRESSION_OPTION_1, false).toBool()) bit_flags |= UI6A_FLAG_3TREES;

    ui6a_ctx *ui6a = NULL;

    // Initialize the library
    ui6a = ui6a_create((void *)pDecompressState);
    if (!ui6a) return false;

    // Set some required fields
    ui6a->cmpr_size = cmpr_size;
    ui6a->uncmpr_size = uncmpr_size;
    ui6a->bit_flags = bit_flags;

    // It seems that PKZIP 1.01 and 1.02 have a bug in which two of the four
    // Implode variants (4KDICT+3TREES and 8KDICT+2TREES) do not work in
    // accordance with the specification. Set the ->emulate_pkzip10x field to 1
    // if for some reason you want to recreate this bug. Doing so will cause
    // some valid ZIP files to fail.
    // ui6a->emulate_pkzip10x = 0;

    // cb_read must supply all of the bytes requested. Returning any other number
    // is considered a failure.
    ui6a->cb_read = my_read;
    // cb_write must consume all of the bytes supplied.
    ui6a->cb_write = my_write;

    // Do the decompression
    ui6a_unimplode(ui6a);
    if (ui6a->error_code != UI6A_ERRCODE_OK) {
        printf("Decompression failed (code %d)\n", ui6a->error_code);
    }

    // Clean up
    ui6a_destroy(ui6a);

    return bResult;
}
