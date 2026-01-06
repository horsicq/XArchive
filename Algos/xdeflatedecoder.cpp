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
#include "xdeflatedecoder.h"

const qint32 N_BUFFER_SIZE = 65536;

void *(_zcalloc)(void *opaque, unsigned items, unsigned size)
{
    Q_UNUSED(opaque);
    return malloc((size_t)items * (size_t)size);
}

void(_zcfree)(void *opaque, void *ptr)
{
    Q_UNUSED(opaque);
    free(ptr);
}

const quint32 WSIZE = 65536UL;
const quint32 ENOUGH_LENS = 852;
const quint32 ENOUGH_DISTS = 594;
const quint32 ENOUGH = (ENOUGH_LENS + ENOUGH_DISTS);
const quint32 MAXBITS = 15;

typedef struct {
    unsigned char op;   /* operation, extra bits, table bits */
    unsigned char bits; /* bits in this part of the code */
    unsigned short val; /* offset in table or code value */
} code;

struct inflate_state {
    /* sliding window */
    unsigned char *window; /* allocated sliding window, if needed */
    /* dynamic table building */
    unsigned ncode;           /* number of code length code lengths */
    unsigned nlen;            /* number of length code lengths */
    unsigned ndist;           /* number of distance code lengths */
    unsigned have;            /* number of code lengths in lens[] */
    code *next;               /* next available space in codes[] */
    unsigned short lens[320]; /* temporary storage for code lengths */
    unsigned short work[288]; /* work area for code table building */
    code codes[ENOUGH];       /* space for code tables */
};

typedef enum {
    TYPE,   /* i: waiting for type bits, including last-flag bit */
    STORED, /* i: waiting for stored size (length and complement) */
    TABLE,  /* i: waiting for dynamic block table lengths */
    LEN,    /* i: waiting for length/lit code */
    DONE,   /* finished check, done -- remain here until reset */
    BAD     /* got a data error -- remain here until reset */
} inflate_mode;

int inflateBack9Init(z_stream *strm, unsigned char *window)
{
    struct inflate_state *state;

    if (strm == Z_NULL || window == Z_NULL) return Z_STREAM_ERROR;
    strm->msg = Z_NULL; /* in case we return an error */
    if (strm->zalloc == (alloc_func)0) {
        strm->zalloc = _zcalloc;
        strm->opaque = (voidpf)0;
    }
    if (strm->zfree == (free_func)0) strm->zfree = _zcfree;
    state = (struct inflate_state *)_zcalloc(strm, 1, sizeof(struct inflate_state));
    if (state == Z_NULL) return Z_MEM_ERROR;

    strm->state = (internal_state *)state;
    state->window = window;
    return Z_OK;
}

/* Macros for inflateBack(): */

/* Clear the input bit accumulator */
#define INITBITS() \
    do {           \
        hold = 0;  \
        bits = 0;  \
    } while (0)

/* Assure that some input is available.  If input is requested, but denied,
   then return a Z_BUF_ERROR from inflateBack(). */
#define PULL()                         \
    do {                               \
        if (have == 0) {               \
            have = in(in_desc, &next); \
            if (have == 0) {           \
                next = Z_NULL;         \
                ret = Z_BUF_ERROR;     \
                goto inf_leave;        \
            }                          \
        }                              \
    } while (0)

/* Get a byte of input into the bit accumulator, or return from inflateBack()
   with an error if there is no input available. */
#define PULLBYTE()                                \
    do {                                          \
        PULL();                                   \
        have--;                                   \
        hold += (unsigned long)(*next++) << bits; \
        bits += 8;                                \
    } while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflateBack() with
   an error. */
#define NEEDBITS(n)                              \
    do {                                         \
        while (bits < (unsigned)(n)) PULLBYTE(); \
    } while (0)

/* Return the low n bits of the bit accumulator (n <= 16) */
#define BITS(n) ((unsigned)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n)            \
    do {                       \
        hold >>= (n);          \
        bits -= (unsigned)(n); \
    } while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS()         \
    do {                   \
        hold >>= bits & 7; \
        bits -= bits & 7;  \
    } while (0)

/* Assure that some output space is available, by writing out the window
   if it's full.  If the write fails, return from inflateBack() with a
   Z_BUF_ERROR. */
#define ROOM()                                        \
    do {                                              \
        if (left == 0) {                              \
            put = window;                             \
            left = WSIZE;                             \
            wrap = 1;                                 \
            if (out(out_desc, put, (unsigned)left)) { \
                ret = Z_BUF_ERROR;                    \
                goto inf_leave;                       \
            }                                         \
        }                                             \
    } while (0)

typedef enum {
    CODES,
    LENS,
    DISTS
} codetype;

int inflate_table9(codetype type, unsigned short FAR *lens, unsigned codes, code FAR *FAR *table, unsigned FAR *bits, unsigned short FAR *work)
{
    unsigned len;                            /* a code's length in bits */
    unsigned sym;                            /* index of code symbols */
    unsigned min, max;                       /* minimum and maximum code lengths */
    unsigned root;                           /* number of index bits for root table */
    unsigned curr;                           /* number of index bits for current table */
    unsigned drop;                           /* code bits to drop for sub-table */
    int left;                                /* number of prefix codes available */
    unsigned used;                           /* code entries in table used */
    unsigned huff;                           /* Huffman code */
    unsigned incr;                           /* for incrementing code, index */
    unsigned fill;                           /* index for replicating entries */
    unsigned low;                            /* low bits for current root entry */
    unsigned mask;                           /* mask for low root bits */
    code _this;                              /* table entry for duplication */
    code FAR *next;                          /* next available space in table */
    const unsigned short FAR *base;          /* base value table to use */
    const unsigned short FAR *extra;         /* extra bits table to use */
    int end;                                 /* use base and extra for symbol > end */
    unsigned short count[MAXBITS + 1];       /* number of codes of each length */
    unsigned short offs[MAXBITS + 1];        /* offsets in table for each length */
    static const unsigned short lbase[31] = {/* Length codes 257..285 base */
                                             3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 3, 0, 0};
    static const unsigned short lext[31] = {
        /* Length codes 257..285 extra */
        128, 128, 128, 128, 128, 128, 128, 128, 129, 129, 129, 129, 130, 130, 130, 130, 131, 131, 131, 131, 132, 132, 132, 132, 133, 133, 133, 133, 144, 198, 203};
    static const unsigned short dbase[32] = {
        /* Distance codes 0..31 base */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32769, 49153};
    static const unsigned short dext[32] = {
        /* Distance codes 0..31 extra */
        128, 128, 128, 128, 129, 129, 130, 130, 131, 131, 132, 132, 133, 133, 134, 134, 135, 135, 136, 136, 137, 137, 138, 138, 139, 139, 140, 140, 141, 141, 142, 142};

    /*
       Process a set of code lengths to create a canonical Huffman code.  The
       code lengths are lens[0..codes-1].  Each length corresponds to the
       symbols 0..codes-1.  The Huffman code is generated by first sorting the
       symbols by length from short to long, and retaining the symbol order
       for codes with equal lengths.  Then the code starts with all zero bits
       for the first code of the shortest length, and the codes are integer
       increments for the same length, and zeros are appended as the length
       increases.  For the deflate format, these bits are stored backwards
       from their more natural integer increment ordering, and so when the
       decoding tables are built in the large loop below, the integer codes
       are incremented backwards.

       This routine assumes, but does not check, that all of the entries in
       lens[] are in the range 0..MAXBITS.  The caller must assure this.
       1..MAXBITS is interpreted as that code length.  zero means that that
       symbol does not occur in this code.

       The codes are sorted by computing a count of codes for each length,
       creating from that a table of starting indices for each length in the
       sorted table, and then entering the symbols in order in the sorted
       table.  The sorted table is work[], with that space being provided by
       the caller.

       The length counts are used for other purposes as well, i.e. finding
       the minimum and maximum length codes, determining if there are any
       codes at all, checking for a valid set of lengths, and looking ahead
       at length counts to determine sub-table sizes when building the
       decoding tables.
     */

    /* accumulate lengths for codes (assumes lens[] all in 0..MAXBITS) */
    for (len = 0; len <= MAXBITS; len++) count[len] = 0;
    for (sym = 0; sym < codes; sym++) count[lens[sym]]++;

    /* bound code lengths, force root to be within code lengths */
    root = *bits;
    for (max = MAXBITS; max >= 1; max--)
        if (count[max] != 0) break;
    if (root > max) root = max;
    if (max == 0) return -1; /* no codes! */
    for (min = 1; min <= MAXBITS; min++)
        if (count[min] != 0) break;
    if (root < min) root = min;

    /* check for an over-subscribed or incomplete set of lengths */
    left = 1;
    for (len = 1; len <= MAXBITS; len++) {
        left <<= 1;
        left -= count[len];
        if (left < 0) return -1; /* over-subscribed */
    }
    if (left > 0 && (type == CODES || max != 1)) return -1; /* incomplete set */

    /* generate offsets into symbol table for each length for sorting */
    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++) offs[len + 1] = offs[len] + count[len];

    /* sort symbols by length, by symbol order within each length */
    for (sym = 0; sym < codes; sym++)
        if (lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;

    /*
       Create and fill in decoding tables.  In this loop, the table being
       filled is at next and has curr index bits.  The code being used is huff
       with length len.  That code is converted to an index by dropping drop
       bits off of the bottom.  For codes where len is less than drop + curr,
       those top drop + curr - len bits are incremented through all values to
       fill the table with replicated entries.

       root is the number of index bits for the root table.  When len exceeds
       root, sub-tables are created pointed to by the root entry with an index
       of the low root bits of huff.  This is saved in low to check for when a
       new sub-table should be started.  drop is zero when the root table is
       being filled, and drop is root when sub-tables are being filled.

       When a new sub-table is needed, it is necessary to look ahead in the
       code lengths to determine what size sub-table is needed.  The length
       counts are used for this, and so count[] is decremented as codes are
       entered in the tables.

       used keeps track of how many table entries have been allocated from the
       provided *table space.  It is checked for LENS and DIST tables against
       the constants ENOUGH_LENS and ENOUGH_DISTS to guard against changes in
       the initial root table size constants.  See the comments in inftree9.h
       for more information.

       sym increments through all symbols, and the loop terminates when
       all codes of length max, i.e. all codes, have been processed.  This
       routine permits incomplete codes, so another loop after this one fills
       in the rest of the decoding tables with invalid code markers.
     */

    /* set up for code type */
    switch (type) {
        case CODES:
            base = extra = work; /* dummy value--not used */
            end = 19;
            break;
        case LENS:
            base = lbase;
            base -= 257;
            extra = lext;
            extra -= 257;
            end = 256;
            break;
        default: /* DISTS */
            base = dbase;
            extra = dext;
            end = -1;
    }

    /* initialize state for loop */
    huff = 0;             /* starting code */
    sym = 0;              /* starting code symbol */
    len = min;            /* starting code length */
    next = *table;        /* current table to fill in */
    curr = root;          /* current table index bits */
    drop = 0;             /* current bits to drop from code for index */
    low = (unsigned)(-1); /* trigger new sub-table when len > root */
    used = 1U << root;    /* use root table entries */
    mask = used - 1;      /* mask for comparing low */

    /* check available table space */
    if ((type == LENS && used >= ENOUGH_LENS) || (type == DISTS && used >= ENOUGH_DISTS)) return 1;

    /* process all codes and make table entries */
    for (;;) {
        /* create table entry */
        _this.bits = (unsigned char)(len - drop);
        if ((int)(work[sym]) < end) {
            _this.op = (unsigned char)0;
            _this.val = work[sym];
        } else if ((int)(work[sym]) > end) {
            _this.op = (unsigned char)(extra[work[sym]]);
            _this.val = base[work[sym]];
        } else {
            _this.op = (unsigned char)(32 + 64); /* end of block */
            _this.val = 0;
        }

        /* replicate for those indices with low len bits equal to huff */
        incr = 1U << (len - drop);
        fill = 1U << curr;
        do {
            fill -= incr;
            next[(huff >> drop) + fill] = _this;
        } while (fill != 0);

        /* backwards increment the len-bit code huff */
        incr = 1U << (len - 1);
        while (huff & incr) incr >>= 1;
        if (incr != 0) {
            huff &= incr - 1;
            huff += incr;
        } else huff = 0;

        /* go to next symbol, update count, len */
        sym++;
        if (--(count[len]) == 0) {
            if (len == max) break;
            len = lens[work[sym]];
        }

        /* create new sub-table if needed */
        if (len > root && (huff & mask) != low) {
            /* if first time, transition to sub-tables */
            if (drop == 0) drop = root;

            /* increment past last table */
            next += 1U << curr;

            /* determine length of next table */
            curr = len - drop;
            left = (int)(1 << curr);
            while (curr + drop < max) {
                left -= count[curr + drop];
                if (left <= 0) break;
                curr++;
                left <<= 1;
            }

            /* check for enough space */
            used += 1U << curr;
            if ((type == LENS && used >= ENOUGH_LENS) || (type == DISTS && used >= ENOUGH_DISTS)) return 1;

            /* point entry in root table to sub-table */
            low = huff & mask;
            (*table)[low].op = (unsigned char)curr;
            (*table)[low].bits = (unsigned char)root;
            (*table)[low].val = (unsigned short)(next - *table);
        }
    }

    /*
       Fill in rest of table for incomplete codes.  This loop is similar to the
       loop above in incrementing huff for table indices.  It is assumed that
       len is equal to curr + drop, so there is no loop needed to increment
       through high index bits.  When the current sub-table is filled, the loop
       drops back to the root table to fill in any remaining entries there.
     */
    _this.op = (unsigned char)64; /* invalid code marker */
    _this.bits = (unsigned char)(len - drop);
    _this.val = (unsigned short)0;
    while (huff != 0) {
        /* when done with sub-table, drop back to root table */
        if (drop != 0 && (huff & mask) != low) {
            drop = 0;
            len = root;
            next = *table;
            curr = root;
            _this.bits = (unsigned char)len;
        }

        /* put invalid code marker in table */
        next[huff >> drop] = _this;

        /* backwards increment the len-bit code huff */
        incr = 1U << (len - 1);
        while (huff & incr) incr >>= 1;
        if (incr != 0) {
            huff &= incr - 1;
            huff += incr;
        } else huff = 0;
    }

    /* set return parameters */
    *table += used;
    *bits = root;
    return 0;
}

static const code lenfix[512] = {
    {96, 7, 0},   {0, 8, 80},  {0, 8, 16}, {132, 8, 115}, {130, 7, 31}, {0, 8, 112}, {0, 8, 48}, {0, 9, 192},   {128, 7, 10}, {0, 8, 96},  {0, 8, 32}, {0, 9, 160},
    {0, 8, 0},    {0, 8, 128}, {0, 8, 64}, {0, 9, 224},   {128, 7, 6},  {0, 8, 88},  {0, 8, 24}, {0, 9, 144},   {131, 7, 59}, {0, 8, 120}, {0, 8, 56}, {0, 9, 208},
    {129, 7, 17}, {0, 8, 104}, {0, 8, 40}, {0, 9, 176},   {0, 8, 8},    {0, 8, 136}, {0, 8, 72}, {0, 9, 240},   {128, 7, 4},  {0, 8, 84},  {0, 8, 20}, {133, 8, 227},
    {131, 7, 43}, {0, 8, 116}, {0, 8, 52}, {0, 9, 200},   {129, 7, 13}, {0, 8, 100}, {0, 8, 36}, {0, 9, 168},   {0, 8, 4},    {0, 8, 132}, {0, 8, 68}, {0, 9, 232},
    {128, 7, 8},  {0, 8, 92},  {0, 8, 28}, {0, 9, 152},   {132, 7, 83}, {0, 8, 124}, {0, 8, 60}, {0, 9, 216},   {130, 7, 23}, {0, 8, 108}, {0, 8, 44}, {0, 9, 184},
    {0, 8, 12},   {0, 8, 140}, {0, 8, 76}, {0, 9, 248},   {128, 7, 3},  {0, 8, 82},  {0, 8, 18}, {133, 8, 163}, {131, 7, 35}, {0, 8, 114}, {0, 8, 50}, {0, 9, 196},
    {129, 7, 11}, {0, 8, 98},  {0, 8, 34}, {0, 9, 164},   {0, 8, 2},    {0, 8, 130}, {0, 8, 66}, {0, 9, 228},   {128, 7, 7},  {0, 8, 90},  {0, 8, 26}, {0, 9, 148},
    {132, 7, 67}, {0, 8, 122}, {0, 8, 58}, {0, 9, 212},   {130, 7, 19}, {0, 8, 106}, {0, 8, 42}, {0, 9, 180},   {0, 8, 10},   {0, 8, 138}, {0, 8, 74}, {0, 9, 244},
    {128, 7, 5},  {0, 8, 86},  {0, 8, 22}, {65, 8, 0},    {131, 7, 51}, {0, 8, 118}, {0, 8, 54}, {0, 9, 204},   {129, 7, 15}, {0, 8, 102}, {0, 8, 38}, {0, 9, 172},
    {0, 8, 6},    {0, 8, 134}, {0, 8, 70}, {0, 9, 236},   {128, 7, 9},  {0, 8, 94},  {0, 8, 30}, {0, 9, 156},   {132, 7, 99}, {0, 8, 126}, {0, 8, 62}, {0, 9, 220},
    {130, 7, 27}, {0, 8, 110}, {0, 8, 46}, {0, 9, 188},   {0, 8, 14},   {0, 8, 142}, {0, 8, 78}, {0, 9, 252},   {96, 7, 0},   {0, 8, 81},  {0, 8, 17}, {133, 8, 131},
    {130, 7, 31}, {0, 8, 113}, {0, 8, 49}, {0, 9, 194},   {128, 7, 10}, {0, 8, 97},  {0, 8, 33}, {0, 9, 162},   {0, 8, 1},    {0, 8, 129}, {0, 8, 65}, {0, 9, 226},
    {128, 7, 6},  {0, 8, 89},  {0, 8, 25}, {0, 9, 146},   {131, 7, 59}, {0, 8, 121}, {0, 8, 57}, {0, 9, 210},   {129, 7, 17}, {0, 8, 105}, {0, 8, 41}, {0, 9, 178},
    {0, 8, 9},    {0, 8, 137}, {0, 8, 73}, {0, 9, 242},   {128, 7, 4},  {0, 8, 85},  {0, 8, 21}, {144, 8, 3},   {131, 7, 43}, {0, 8, 117}, {0, 8, 53}, {0, 9, 202},
    {129, 7, 13}, {0, 8, 101}, {0, 8, 37}, {0, 9, 170},   {0, 8, 5},    {0, 8, 133}, {0, 8, 69}, {0, 9, 234},   {128, 7, 8},  {0, 8, 93},  {0, 8, 29}, {0, 9, 154},
    {132, 7, 83}, {0, 8, 125}, {0, 8, 61}, {0, 9, 218},   {130, 7, 23}, {0, 8, 109}, {0, 8, 45}, {0, 9, 186},   {0, 8, 13},   {0, 8, 141}, {0, 8, 77}, {0, 9, 250},
    {128, 7, 3},  {0, 8, 83},  {0, 8, 19}, {133, 8, 195}, {131, 7, 35}, {0, 8, 115}, {0, 8, 51}, {0, 9, 198},   {129, 7, 11}, {0, 8, 99},  {0, 8, 35}, {0, 9, 166},
    {0, 8, 3},    {0, 8, 131}, {0, 8, 67}, {0, 9, 230},   {128, 7, 7},  {0, 8, 91},  {0, 8, 27}, {0, 9, 150},   {132, 7, 67}, {0, 8, 123}, {0, 8, 59}, {0, 9, 214},
    {130, 7, 19}, {0, 8, 107}, {0, 8, 43}, {0, 9, 182},   {0, 8, 11},   {0, 8, 139}, {0, 8, 75}, {0, 9, 246},   {128, 7, 5},  {0, 8, 87},  {0, 8, 23}, {77, 8, 0},
    {131, 7, 51}, {0, 8, 119}, {0, 8, 55}, {0, 9, 206},   {129, 7, 15}, {0, 8, 103}, {0, 8, 39}, {0, 9, 174},   {0, 8, 7},    {0, 8, 135}, {0, 8, 71}, {0, 9, 238},
    {128, 7, 9},  {0, 8, 95},  {0, 8, 31}, {0, 9, 158},   {132, 7, 99}, {0, 8, 127}, {0, 8, 63}, {0, 9, 222},   {130, 7, 27}, {0, 8, 111}, {0, 8, 47}, {0, 9, 190},
    {0, 8, 15},   {0, 8, 143}, {0, 8, 79}, {0, 9, 254},   {96, 7, 0},   {0, 8, 80},  {0, 8, 16}, {132, 8, 115}, {130, 7, 31}, {0, 8, 112}, {0, 8, 48}, {0, 9, 193},
    {128, 7, 10}, {0, 8, 96},  {0, 8, 32}, {0, 9, 161},   {0, 8, 0},    {0, 8, 128}, {0, 8, 64}, {0, 9, 225},   {128, 7, 6},  {0, 8, 88},  {0, 8, 24}, {0, 9, 145},
    {131, 7, 59}, {0, 8, 120}, {0, 8, 56}, {0, 9, 209},   {129, 7, 17}, {0, 8, 104}, {0, 8, 40}, {0, 9, 177},   {0, 8, 8},    {0, 8, 136}, {0, 8, 72}, {0, 9, 241},
    {128, 7, 4},  {0, 8, 84},  {0, 8, 20}, {133, 8, 227}, {131, 7, 43}, {0, 8, 116}, {0, 8, 52}, {0, 9, 201},   {129, 7, 13}, {0, 8, 100}, {0, 8, 36}, {0, 9, 169},
    {0, 8, 4},    {0, 8, 132}, {0, 8, 68}, {0, 9, 233},   {128, 7, 8},  {0, 8, 92},  {0, 8, 28}, {0, 9, 153},   {132, 7, 83}, {0, 8, 124}, {0, 8, 60}, {0, 9, 217},
    {130, 7, 23}, {0, 8, 108}, {0, 8, 44}, {0, 9, 185},   {0, 8, 12},   {0, 8, 140}, {0, 8, 76}, {0, 9, 249},   {128, 7, 3},  {0, 8, 82},  {0, 8, 18}, {133, 8, 163},
    {131, 7, 35}, {0, 8, 114}, {0, 8, 50}, {0, 9, 197},   {129, 7, 11}, {0, 8, 98},  {0, 8, 34}, {0, 9, 165},   {0, 8, 2},    {0, 8, 130}, {0, 8, 66}, {0, 9, 229},
    {128, 7, 7},  {0, 8, 90},  {0, 8, 26}, {0, 9, 149},   {132, 7, 67}, {0, 8, 122}, {0, 8, 58}, {0, 9, 213},   {130, 7, 19}, {0, 8, 106}, {0, 8, 42}, {0, 9, 181},
    {0, 8, 10},   {0, 8, 138}, {0, 8, 74}, {0, 9, 245},   {128, 7, 5},  {0, 8, 86},  {0, 8, 22}, {65, 8, 0},    {131, 7, 51}, {0, 8, 118}, {0, 8, 54}, {0, 9, 205},
    {129, 7, 15}, {0, 8, 102}, {0, 8, 38}, {0, 9, 173},   {0, 8, 6},    {0, 8, 134}, {0, 8, 70}, {0, 9, 237},   {128, 7, 9},  {0, 8, 94},  {0, 8, 30}, {0, 9, 157},
    {132, 7, 99}, {0, 8, 126}, {0, 8, 62}, {0, 9, 221},   {130, 7, 27}, {0, 8, 110}, {0, 8, 46}, {0, 9, 189},   {0, 8, 14},   {0, 8, 142}, {0, 8, 78}, {0, 9, 253},
    {96, 7, 0},   {0, 8, 81},  {0, 8, 17}, {133, 8, 131}, {130, 7, 31}, {0, 8, 113}, {0, 8, 49}, {0, 9, 195},   {128, 7, 10}, {0, 8, 97},  {0, 8, 33}, {0, 9, 163},
    {0, 8, 1},    {0, 8, 129}, {0, 8, 65}, {0, 9, 227},   {128, 7, 6},  {0, 8, 89},  {0, 8, 25}, {0, 9, 147},   {131, 7, 59}, {0, 8, 121}, {0, 8, 57}, {0, 9, 211},
    {129, 7, 17}, {0, 8, 105}, {0, 8, 41}, {0, 9, 179},   {0, 8, 9},    {0, 8, 137}, {0, 8, 73}, {0, 9, 243},   {128, 7, 4},  {0, 8, 85},  {0, 8, 21}, {144, 8, 3},
    {131, 7, 43}, {0, 8, 117}, {0, 8, 53}, {0, 9, 203},   {129, 7, 13}, {0, 8, 101}, {0, 8, 37}, {0, 9, 171},   {0, 8, 5},    {0, 8, 133}, {0, 8, 69}, {0, 9, 235},
    {128, 7, 8},  {0, 8, 93},  {0, 8, 29}, {0, 9, 155},   {132, 7, 83}, {0, 8, 125}, {0, 8, 61}, {0, 9, 219},   {130, 7, 23}, {0, 8, 109}, {0, 8, 45}, {0, 9, 187},
    {0, 8, 13},   {0, 8, 141}, {0, 8, 77}, {0, 9, 251},   {128, 7, 3},  {0, 8, 83},  {0, 8, 19}, {133, 8, 195}, {131, 7, 35}, {0, 8, 115}, {0, 8, 51}, {0, 9, 199},
    {129, 7, 11}, {0, 8, 99},  {0, 8, 35}, {0, 9, 167},   {0, 8, 3},    {0, 8, 131}, {0, 8, 67}, {0, 9, 231},   {128, 7, 7},  {0, 8, 91},  {0, 8, 27}, {0, 9, 151},
    {132, 7, 67}, {0, 8, 123}, {0, 8, 59}, {0, 9, 215},   {130, 7, 19}, {0, 8, 107}, {0, 8, 43}, {0, 9, 183},   {0, 8, 11},   {0, 8, 139}, {0, 8, 75}, {0, 9, 247},
    {128, 7, 5},  {0, 8, 87},  {0, 8, 23}, {77, 8, 0},    {131, 7, 51}, {0, 8, 119}, {0, 8, 55}, {0, 9, 207},   {129, 7, 15}, {0, 8, 103}, {0, 8, 39}, {0, 9, 175},
    {0, 8, 7},    {0, 8, 135}, {0, 8, 71}, {0, 9, 239},   {128, 7, 9},  {0, 8, 95},  {0, 8, 31}, {0, 9, 159},   {132, 7, 99}, {0, 8, 127}, {0, 8, 63}, {0, 9, 223},
    {130, 7, 27}, {0, 8, 111}, {0, 8, 47}, {0, 9, 191},   {0, 8, 15},   {0, 8, 143}, {0, 8, 79}, {0, 9, 255}};

static const code distfix[32] = {{128, 5, 1}, {135, 5, 257}, {131, 5, 17}, {139, 5, 4097},  {129, 5, 5},  {137, 5, 1025}, {133, 5, 65},  {141, 5, 16385},
                                 {128, 5, 3}, {136, 5, 513}, {132, 5, 33}, {140, 5, 8193},  {130, 5, 9},  {138, 5, 2049}, {134, 5, 129}, {142, 5, 32769},
                                 {128, 5, 2}, {135, 5, 385}, {131, 5, 25}, {139, 5, 6145},  {129, 5, 7},  {137, 5, 1537}, {133, 5, 97},  {141, 5, 24577},
                                 {128, 5, 4}, {136, 5, 769}, {132, 5, 49}, {140, 5, 12289}, {130, 5, 13}, {138, 5, 3073}, {134, 5, 193}, {142, 5, 49153}};

int inflateBack9(z_stream *strm, in_func in, void *in_desc, out_func out, void *out_desc)
{
    struct inflate_state *state;
    z_const unsigned char *next;            /* next input */
    unsigned char *put;                     /* next output */
    unsigned have;                          /* available input */
    unsigned long left;                     /* available output */
    inflate_mode mode;                      /* current inflate mode */
    int lastblock;                          /* true if processing last block */
    int wrap;                               /* true if the window has wrapped */
    unsigned char *window;                  /* allocated sliding window, if needed */
    unsigned long hold;                     /* bit buffer */
    unsigned bits;                          /* bits in bit buffer */
    unsigned extra;                         /* extra bits needed */
    unsigned long length;                   /* literal or length of data to copy */
    unsigned long offset;                   /* distance back to copy string from */
    unsigned long copy;                     /* number of stored or match bytes to copy */
    unsigned char *from;                    /* where to copy match bytes from */
    code const *lencode;                    /* starting table for length/literal codes */
    code const *distcode;                   /* starting table for distance codes */
    unsigned lenbits;                       /* index bits for lencode */
    unsigned distbits;                      /* index bits for distcode */
    code here;                              /* current decoding table entry */
    code last;                              /* parent table entry */
    unsigned len;                           /* length to copy for repeats, bits to drop */
    int ret;                                /* return code */
    static const unsigned short order[19] = /* permutation of code lengths */
        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    /* Check that the strm exists and that the state was initialized */
    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
    state = (struct inflate_state *)strm->state;

    /* Reset the state */
    strm->msg = Z_NULL;
    mode = TYPE;
    lastblock = 0;
    wrap = 0;
    window = state->window;
    next = strm->next_in;
    have = next != Z_NULL ? strm->avail_in : 0;
    hold = 0;
    bits = 0;
    put = window;
    left = WSIZE;
    lencode = Z_NULL;
    distcode = Z_NULL;

    /* Inflate until end of block marked as last */
    for (;;) switch (mode) {
            case TYPE:
                /* determine and dispatch block type */
                if (lastblock) {
                    BYTEBITS();
                    mode = DONE;
                    break;
                }
                NEEDBITS(3);
                lastblock = BITS(1);
                DROPBITS(1);
                switch (BITS(2)) {
                    case 0: /* stored block */ mode = STORED; break;
                    case 1: /* fixed block */
                        lencode = lenfix;
                        lenbits = 9;
                        distcode = distfix;
                        distbits = 5;
                        mode = LEN; /* decode codes */
                        break;
                    case 2: /* dynamic block */ mode = TABLE; break;
                    case 3: strm->msg = (char *)"invalid block type"; mode = BAD;
                }
                DROPBITS(2);
                break;

            case STORED:
                /* get and verify stored block length */
                BYTEBITS(); /* go to byte boundary */
                NEEDBITS(32);
                if ((hold & 0xffff) != ((hold >> 16) ^ 0xffff)) {
                    strm->msg = (char *)"invalid stored block lengths";
                    mode = BAD;
                    break;
                }
                length = (unsigned)hold & 0xffff;
                INITBITS();

                /* copy stored block from input to output */
                while (length != 0) {
                    copy = length;
                    PULL();
                    ROOM();
                    if (copy > have) copy = have;
                    if (copy > left) copy = left;
                    memcpy(put, next, copy);
                    have -= copy;
                    next += copy;
                    left -= copy;
                    put += copy;
                    length -= copy;
                }
                mode = TYPE;
                break;

            case TABLE:
                /* get dynamic table entries descriptor */
                NEEDBITS(14);
                state->nlen = BITS(5) + 257;
                DROPBITS(5);
                state->ndist = BITS(5) + 1;
                DROPBITS(5);
                state->ncode = BITS(4) + 4;
                DROPBITS(4);
                if (state->nlen > 286) {
                    strm->msg = (char *)"too many length symbols";
                    mode = BAD;
                    break;
                }

                /* get code length code lengths (not a typo) */
                state->have = 0;
                while (state->have < state->ncode) {
                    NEEDBITS(3);
                    state->lens[order[state->have++]] = (unsigned short)BITS(3);
                    DROPBITS(3);
                }
                while (state->have < 19) state->lens[order[state->have++]] = 0;
                state->next = state->codes;
                lencode = (code const *)(state->next);
                lenbits = 7;
                ret = inflate_table9(CODES, state->lens, 19, &(state->next), &(lenbits), state->work);
                if (ret) {
                    strm->msg = (char *)"invalid code lengths set";
                    mode = BAD;
                    break;
                }

                /* get length and distance code code lengths */
                state->have = 0;
                while (state->have < state->nlen + state->ndist) {
                    for (;;) {
                        here = lencode[BITS(lenbits)];
                        if ((unsigned)(here.bits) <= bits) break;
                        PULLBYTE();
                    }
                    if (here.val < 16) {
                        NEEDBITS(here.bits);
                        DROPBITS(here.bits);
                        state->lens[state->have++] = here.val;
                    } else {
                        if (here.val == 16) {
                            NEEDBITS(here.bits + 2);
                            DROPBITS(here.bits);
                            if (state->have == 0) {
                                strm->msg = (char *)"invalid bit length repeat";
                                mode = BAD;
                                break;
                            }
                            len = (unsigned)(state->lens[state->have - 1]);
                            copy = 3 + BITS(2);
                            DROPBITS(2);
                        } else if (here.val == 17) {
                            NEEDBITS(here.bits + 3);
                            DROPBITS(here.bits);
                            len = 0;
                            copy = 3 + BITS(3);
                            DROPBITS(3);
                        } else {
                            NEEDBITS(here.bits + 7);
                            DROPBITS(here.bits);
                            len = 0;
                            copy = 11 + BITS(7);
                            DROPBITS(7);
                        }
                        if (state->have + copy > state->nlen + state->ndist) {
                            strm->msg = (char *)"invalid bit length repeat";
                            mode = BAD;
                            break;
                        }
                        while (copy--) state->lens[state->have++] = (unsigned short)len;
                    }
                }

                /* handle error breaks in while */
                if (mode == BAD) break;

                /* check for end-of-block code (better have one) */
                if (state->lens[256] == 0) {
                    strm->msg = (char *)"invalid code -- missing end-of-block";
                    mode = BAD;
                    break;
                }

                /* build code tables -- note: do not change the lenbits or distbits
                   values here (9 and 6) without reading the comments in inftree9.h
                   concerning the ENOUGH constants, which depend on those values */
                state->next = state->codes;
                lencode = (code const *)(state->next);
                lenbits = 9;
                ret = inflate_table9(LENS, state->lens, state->nlen, &(state->next), &(lenbits), state->work);
                if (ret) {
                    strm->msg = (char *)"invalid literal/lengths set";
                    mode = BAD;
                    break;
                }
                distcode = (code const *)(state->next);
                distbits = 6;
                ret = inflate_table9(DISTS, state->lens + state->nlen, state->ndist, &(state->next), &(distbits), state->work);
                if (ret) {
                    strm->msg = (char *)"invalid distances set";
                    mode = BAD;
                    break;
                }
                mode = LEN;

            case LEN:
                /* get a literal, length, or end-of-block code */
                for (;;) {
                    here = lencode[BITS(lenbits)];
                    if ((unsigned)(here.bits) <= bits) break;
                    PULLBYTE();
                }
                if (here.op && (here.op & 0xf0) == 0) {
                    last = here;
                    for (;;) {
                        here = lencode[last.val + (BITS(last.bits + last.op) >> last.bits)];
                        if ((unsigned)(last.bits + here.bits) <= bits) break;
                        PULLBYTE();
                    }
                    DROPBITS(last.bits);
                }
                DROPBITS(here.bits);
                length = (unsigned)here.val;

                /* process literal */
                if (here.op == 0) {
                    ROOM();
                    *put++ = (unsigned char)(length);
                    left--;
                    mode = LEN;
                    break;
                }

                /* process end of block */
                if (here.op & 32) {
                    mode = TYPE;
                    break;
                }

                /* invalid code */
                if (here.op & 64) {
                    strm->msg = (char *)"invalid literal/length code";
                    mode = BAD;
                    break;
                }

                /* length code -- get extra bits, if any */
                extra = (unsigned)(here.op) & 31;
                if (extra != 0) {
                    NEEDBITS(extra);
                    length += BITS(extra);
                    DROPBITS(extra);
                }

                /* get distance code */
                for (;;) {
                    here = distcode[BITS(distbits)];
                    if ((unsigned)(here.bits) <= bits) break;
                    PULLBYTE();
                }
                if ((here.op & 0xf0) == 0) {
                    last = here;
                    for (;;) {
                        here = distcode[last.val + (BITS(last.bits + last.op) >> last.bits)];
                        if ((unsigned)(last.bits + here.bits) <= bits) break;
                        PULLBYTE();
                    }
                    DROPBITS(last.bits);
                }
                DROPBITS(here.bits);
                if (here.op & 64) {
                    strm->msg = (char *)"invalid distance code";
                    mode = BAD;
                    break;
                }
                offset = (unsigned)here.val;

                /* get distance extra bits, if any */
                extra = (unsigned)(here.op) & 15;
                if (extra != 0) {
                    NEEDBITS(extra);
                    offset += BITS(extra);
                    DROPBITS(extra);
                }
                if (offset > WSIZE - (wrap ? 0 : left)) {
                    strm->msg = (char *)"invalid distance too far back";
                    mode = BAD;
                    break;
                }

                /* copy match from window to output */
                do {
                    ROOM();
                    copy = WSIZE - offset;
                    if (copy < left) {
                        from = put + copy;
                        copy = left - copy;
                    } else {
                        from = put - offset;
                        copy = left;
                    }
                    if (copy > length) copy = length;
                    length -= copy;
                    left -= copy;

                    do {
                        *put++ = *from++;
                    } while (--copy);
                } while (length != 0);
                break;

            case DONE:
                /* inflate stream terminated properly -- write leftover output */
                ret = Z_STREAM_END;
                if (left < WSIZE) {
                    if (out(out_desc, window, (unsigned)(WSIZE - left))) ret = Z_BUF_ERROR;
                }
                goto inf_leave;

            case BAD: ret = Z_DATA_ERROR; goto inf_leave;

            default: /* can't happen, but makes compilers happy */ ret = Z_STREAM_ERROR; goto inf_leave;
        }

    /* Return unused input */
inf_leave:
    strm->next_in = next;
    strm->avail_in = have;
    return ret;
}

int inflateBack9End(z_stream *strm)
{
    if (strm == Z_NULL || strm->state == Z_NULL || strm->zfree == (free_func)0) return Z_STREAM_ERROR;
    _zcfree(strm, strm->state);
    strm->state = Z_NULL;
    return Z_OK;
}

unsigned readFunc(void *in_desc, unsigned char **buf)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)in_desc;

    *buf = (unsigned char *)(pDecompressState->pInputBuffer);

    return XBinary::_readDevice(pDecompressState);
}

int writeFunc(void *out_desc, unsigned char *buf, unsigned len)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)out_desc;

    qint32 nResult = XBinary::_writeDevice((char *)buf, len, pDecompressState);

    return 0;  // TODO Stop
}

XDeflateDecoder::XDeflateDecoder(QObject *parent) : QObject(parent)
{
}

bool XDeflateDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        qint32 _nBufferSize = XBinary::getBufferSize(pPdStruct);

        char *bufferIn = new char[_nBufferSize];
        char *bufferOut = new char[_nBufferSize];

        if (pDecompressState->pDeviceInput) {
            pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
        }

        if (pDecompressState->pDeviceOutput) {
            pDecompressState->pDeviceOutput->seek(0);
        }

        z_stream strm;

        strm.zalloc = nullptr;
        strm.zfree = nullptr;
        strm.opaque = nullptr;
        strm.avail_in = 0;
        strm.next_in = nullptr;

        qint32 ret = Z_OK;

        if (inflateInit2(&strm, -MAX_WBITS) == Z_OK)  // -MAX_WBITS for raw data
        {
            do {
                qint32 nBufferSize = (pDecompressState->nInputLimit == -1) ? _nBufferSize
                                                                            : qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), _nBufferSize);
                strm.avail_in = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

                if (strm.avail_in == 0) {
                    ret = Z_ERRNO;
                    break;
                }

                strm.next_in = (quint8 *)bufferIn;

                do {
                    strm.avail_out = _nBufferSize;
                    //                    strm.avail_out=1;
                    strm.next_out = (quint8 *)bufferOut;
                    ret = inflate(&strm, Z_NO_FLUSH);
                    //                    ret=inflate(&strm,Z_SYNC_FLUSH);

                    if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT)) {
                        break;
                    }

                    qint32 nTemp = _nBufferSize - strm.avail_out;

                    if (nTemp > 0) {
                        if (!XBinary::_writeDevice(bufferOut, nTemp, pDecompressState)) {
                            ret = Z_ERRNO;
                            break;
                        }
                    }
                } while (strm.avail_out == 0);
                if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT) || (ret == Z_ERRNO)) {
                    break;
                }

                if (XBinary::isPdStructStopped(pPdStruct)) {
                    break;
                }
            } while (ret != Z_STREAM_END);

            inflateEnd(&strm);

            bResult = (ret == Z_STREAM_END);
        }

        delete[] bufferIn;
        delete[] bufferOut;
    }

    return bResult;
}

bool XDeflateDecoder::decompress64(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (pDecompressState->pDeviceInput) {
        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    }

    if (pDecompressState->pDeviceOutput) {
        pDecompressState->pDeviceOutput->seek(0);
    }

    bool bResult = false;

    pDecompressState->pInputBuffer = new char[N_BUFFER_SIZE];
    pDecompressState->nInputBufferSize = N_BUFFER_SIZE;
    pDecompressState->pOutputBuffer = new char[N_BUFFER_SIZE];
    pDecompressState->nOutputBufferSize = N_BUFFER_SIZE;

    z_stream strm;

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = pDecompressState->nInputLimit;
    strm.next_in = nullptr;

    if (inflateBack9Init(&strm, (unsigned char *)pDecompressState->pOutputBuffer) == Z_OK) {
        qint32 ret = inflateBack9(&strm, readFunc, pDecompressState, writeFunc, pDecompressState);

        inflateBack9End(&strm);

        bResult = true;  // TODO
    }

    delete[] pDecompressState->pInputBuffer;
    delete[] pDecompressState->pOutputBuffer;

    return bResult;
}

bool XDeflateDecoder::decompress_zlib(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (pDecompressState->pDeviceInput) {
        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    }

    if (pDecompressState->pDeviceOutput) {
        pDecompressState->pDeviceOutput->seek(0);
    }

    XBinary::DATAPROCESS_STATE decompressState = *pDecompressState;
    decompressState.nInputLimit = pDecompressState->nInputLimit - 6;    // Skip zlib header and footer
    decompressState.nInputOffset = pDecompressState->nInputOffset + 2;  // Skip zlib header

    quint32 nAdler = XBinary(pDecompressState->pDeviceInput).read_uint32(pDecompressState->nInputOffset + pDecompressState->nInputLimit - 4, true);

    bool bResult = decompress(&decompressState, pPdStruct);

    if (bResult) {
        quint32 _nAdler = XBinary::getAdler32(pDecompressState->pDeviceOutput, pPdStruct);
        bResult = (nAdler == _nAdler);
    }

    return bResult;
}

bool XDeflateDecoder::compress(XBinary::DATAPROCESS_STATE *pCompressState, XBinary::PDSTRUCT *pPdStruct, int nCompressionLevel)
{
    bool bResult = false;

    if (pCompressState && pCompressState->pDeviceInput && pCompressState->pDeviceOutput) {
        // Initialize error states
        pCompressState->bReadError = false;
        pCompressState->bWriteError = false;
        pCompressState->nCountInput = 0;
        pCompressState->nCountOutput = 0;

        // Set input device position
        if (pCompressState->nInputOffset > 0) {
            pCompressState->pDeviceInput->seek(pCompressState->nInputOffset);
        }

        // Set output device position
        if (pCompressState->pDeviceOutput) {
            pCompressState->pDeviceOutput->seek(0);
        }

        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        // Initialize deflate
        if (deflateInit2(&stream, nCompressionLevel, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            return false;
        }

        char inputBuffer[N_BUFFER_SIZE];
        char outputBuffer[N_BUFFER_SIZE];

        qint64 nTotalProcessed = 0;
        int flush = Z_NO_FLUSH;
        int ret = Z_OK;

        do {
            // Read input data
            qint32 nToRead = qMin((qint32)(pCompressState->nInputLimit - nTotalProcessed), N_BUFFER_SIZE);
            if (nToRead == 0) {
                flush = Z_FINISH;
            } else {
                qint32 nRead = pCompressState->pDeviceInput->read(inputBuffer, nToRead);
                if (nRead <= 0) {
                    flush = Z_FINISH;
                    stream.avail_in = 0;
                } else {
                    pCompressState->nCountInput += nRead;
                    nTotalProcessed += nRead;
                    stream.avail_in = nRead;
                    stream.next_in = (Bytef *)inputBuffer;
                }
            }

            // Compress data
            do {
                stream.avail_out = N_BUFFER_SIZE;
                stream.next_out = (Bytef *)outputBuffer;

                ret = deflate(&stream, flush);
                if (ret == Z_STREAM_ERROR) {
                    deflateEnd(&stream);
                    return false;
                }

                qint32 nCompressed = N_BUFFER_SIZE - stream.avail_out;
                if (nCompressed > 0) {
                    qint64 nWritten = pCompressState->pDeviceOutput->write(outputBuffer, nCompressed);
                    if (nWritten != nCompressed) {
                        pCompressState->bWriteError = true;
                        deflateEnd(&stream);
                        return false;
                    }
                    pCompressState->nCountOutput += nCompressed;
                }

                // Continue until all output is consumed or stream ends
            } while (stream.avail_out == 0 && ret != Z_STREAM_END);

            // Check for cancellation
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                deflateEnd(&stream);
                return false;
            }

        } while (flush != Z_FINISH || ret != Z_STREAM_END);

        deflateEnd(&stream);
        bResult = !pCompressState->bReadError && !pCompressState->bWriteError;
    }

    return bResult;
}

bool XDeflateDecoder::compress_zlib(XBinary::DATAPROCESS_STATE *pCompressState, XBinary::PDSTRUCT *pPdStruct, int nCompressionLevel)
{
    bool bResult = false;

    if (pCompressState && pCompressState->pDeviceInput && pCompressState->pDeviceOutput) {
        // Initialize error states
        pCompressState->bReadError = false;
        pCompressState->bWriteError = false;
        pCompressState->nCountInput = 0;
        pCompressState->nCountOutput = 0;

        // Set input device position
        if (pCompressState->nInputOffset > 0) {
            pCompressState->pDeviceInput->seek(pCompressState->nInputOffset);
        }

        // Set output device position
        if (pCompressState->pDeviceOutput) {
            pCompressState->pDeviceOutput->seek(0);
        }

        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        // Initialize deflate with zlib wrapper
        if (deflateInit(&stream, nCompressionLevel) != Z_OK) {
            return false;
        }

        char inputBuffer[N_BUFFER_SIZE];
        char outputBuffer[N_BUFFER_SIZE];

        qint64 nTotalProcessed = 0;
        int flush = Z_NO_FLUSH;

        do {
            // Read input data
            qint32 nToRead = qMin((qint32)(pCompressState->nInputLimit - nTotalProcessed), N_BUFFER_SIZE);
            if (nToRead == 0) {
                flush = Z_FINISH;
            } else {
                qint32 nRead = pCompressState->pDeviceInput->read(inputBuffer, nToRead);
                if (nRead <= 0) {
                    flush = Z_FINISH;
                    stream.avail_in = 0;
                } else {
                    pCompressState->nCountInput += nRead;
                    nTotalProcessed += nRead;
                    stream.avail_in = nRead;
                    stream.next_in = (Bytef *)inputBuffer;
                }
            }

            // Compress data
            do {
                stream.avail_out = N_BUFFER_SIZE;
                stream.next_out = (Bytef *)outputBuffer;

                int ret = deflate(&stream, flush);
                if (ret == Z_STREAM_ERROR) {
                    deflateEnd(&stream);
                    return false;
                }

                qint32 nCompressed = N_BUFFER_SIZE - stream.avail_out;
                if (nCompressed > 0) {
                    qint64 nWritten = pCompressState->pDeviceOutput->write(outputBuffer, nCompressed);
                    if (nWritten != nCompressed) {
                        pCompressState->bWriteError = true;
                        deflateEnd(&stream);
                        return false;
                    }
                    pCompressState->nCountOutput += nCompressed;
                }

            } while (stream.avail_out == 0);

            // Check for cancellation
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                deflateEnd(&stream);
                return false;
            }

        } while (flush != Z_FINISH);

        deflateEnd(&stream);
        bResult = !pCompressState->bReadError && !pCompressState->bWriteError;
    }

    return bResult;
}
