/* StuffIt method-13 (LZ+Huffman) decoder.
 *
 * C port of the MIT-licensed compcol clean-room implementation:
 * Copyright (c) 2026 Karpeles Lab Inc.
 * See COMPCOL_LICENSE for the license terms and xdearkstuffit13tables_p.h for the
 * fixed interoperability data.
 */

#include "xdearkstuffit13_p.h"
#include "xdearkstuffit13tables_p.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define XSIT13_NONE UINT32_MAX
#define XSIT13_MAX_CODE_LENGTH 32U
#define XSIT13_LITLEN_SYMBOLS 321U
#define XSIT13_EOS 0x140U

typedef struct XSIT13_BITREADER {
    const uint8_t *data;
    size_t size;
    size_t bit_pos;
} XSIT13_BITREADER;

typedef struct XSIT13_HUFFMAN {
    uint32_t *links;
    uint32_t *leaf;
    size_t node_count;
    size_t node_capacity;
} XSIT13_HUFFMAN;

static int xsit13_read_bit(XSIT13_BITREADER *r, uint32_t *value)
{
    size_t byte_index;
    if(!r || !value) return 1;
    byte_index = r->bit_pos >> 3;
    if(byte_index >= r->size) return 2;
    *value = (uint32_t)((r->data[byte_index] >> (r->bit_pos & 7U)) & 1U);
    r->bit_pos++;
    return 0;
}

static int xsit13_read_bits(XSIT13_BITREADER *r, uint32_t count, uint32_t *value)
{
    uint32_t i;
    uint32_t result = 0;
    if(!r || !value || count > 32U) return 1;
    if(count > 0 && (r->bit_pos > r->size * 8U ||
        (size_t)count > r->size * 8U - r->bit_pos)) return 2;
    for(i = 0; i < count; i++) {
        uint32_t bit = 0;
        int rc = xsit13_read_bit(r, &bit);
        if(rc) return rc;
        result |= bit << i;
    }
    *value = result;
    return 0;
}

static void xsit13_huffman_destroy(XSIT13_HUFFMAN *h)
{
    if(!h) return;
    free(h->links);
    free(h->leaf);
    memset(h, 0, sizeof(*h));
}

static int xsit13_huffman_init(XSIT13_HUFFMAN *h, size_t symbols)
{
    size_t capacity;
    if(!h || symbols == 0 || symbols > (SIZE_MAX - 1U) / 32U) return 1;
    memset(h, 0, sizeof(*h));
    capacity = symbols * 32U + 1U;
    h->links = (uint32_t *)malloc(capacity * 2U * sizeof(uint32_t));
    h->leaf = (uint32_t *)malloc(capacity * sizeof(uint32_t));
    if(!h->links || !h->leaf) {
        xsit13_huffman_destroy(h);
        return 4;
    }
    h->node_capacity = capacity;
    h->node_count = 1;
    h->links[0] = h->links[1] = XSIT13_NONE;
    h->leaf[0] = XSIT13_NONE;
    return 0;
}

static int xsit13_huffman_new_node(XSIT13_HUFFMAN *h, uint32_t *index)
{
    size_t n;
    if(!h || !index || h->node_count >= h->node_capacity) return 3;
    n = h->node_count++;
    h->links[n * 2U] = h->links[n * 2U + 1U] = XSIT13_NONE;
    h->leaf[n] = XSIT13_NONE;
    *index = (uint32_t)n;
    return 0;
}

static int xsit13_huffman_insert(XSIT13_HUFFMAN *h, uint32_t code,
    uint32_t length, uint32_t symbol)
{
    uint32_t node = 0;
    uint32_t i;
    if(!h || length == 0 || length > XSIT13_MAX_CODE_LENGTH) return 3;
    for(i = 0; i < length; i++) {
        uint32_t bit;
        size_t slot;
        if(node >= h->node_count || h->leaf[node] != XSIT13_NONE) return 3;
        bit = (code >> i) & 1U;
        slot = (size_t)node * 2U + bit;
        if(h->links[slot] == XSIT13_NONE) {
            uint32_t new_node = 0;
            int rc = xsit13_huffman_new_node(h, &new_node);
            if(rc) return rc;
            h->links[slot] = new_node;
        }
        node = h->links[slot];
    }
    if(node >= h->node_count || h->leaf[node] != XSIT13_NONE ||
        h->links[(size_t)node * 2U] != XSIT13_NONE ||
        h->links[(size_t)node * 2U + 1U] != XSIT13_NONE) return 3;
    h->leaf[node] = symbol;
    return 0;
}

static uint32_t xsit13_reverse_bits(uint32_t value, uint32_t count)
{
    uint32_t i;
    uint32_t result = 0;
    for(i = 0; i < count; i++) {
        result |= ((value >> i) & 1U) << (count - 1U - i);
    }
    return result;
}

static int xsit13_huffman_from_lengths(XSIT13_HUFFMAN *h,
    const uint8_t *lengths, size_t symbol_count)
{
    uint32_t counts[XSIT13_MAX_CODE_LENGTH + 1U];
    uint32_t next_code[XSIT13_MAX_CODE_LENGTH + 1U];
    uint32_t max_length = 0;
    uint32_t code = 0;
    uint64_t kraft = 0;
    size_t i;
    int rc;
    if(!h || !lengths || symbol_count == 0) return 1;
    memset(counts, 0, sizeof(counts));
    memset(next_code, 0, sizeof(next_code));
    for(i = 0; i < symbol_count; i++) {
        uint32_t length = lengths[i];
        if(length > XSIT13_MAX_CODE_LENGTH) return 3;
        if(length) {
            counts[length]++;
            if(length > max_length) max_length = length;
        }
    }
    if(max_length == 0) return 3;
    for(i = 1; i <= max_length; i++) {
        kraft += (uint64_t)counts[i] << (max_length - (uint32_t)i);
    }
    if(kraft != ((uint64_t)1U << max_length)) return 3;
    rc = xsit13_huffman_init(h, symbol_count);
    if(rc) return rc;
    for(i = 1; i <= max_length; i++) {
        next_code[i] = code;
        code = (code + counts[i]) << 1U;
    }
    for(i = 0; i < symbol_count; i++) {
        uint32_t length = lengths[i];
        if(length) {
            uint32_t canonical = next_code[length]++;
            rc = xsit13_huffman_insert(h,
                xsit13_reverse_bits(canonical, length), length, (uint32_t)i);
            if(rc) {
                xsit13_huffman_destroy(h);
                return rc;
            }
        }
    }
    return 0;
}

static int xsit13_huffman_from_codes(XSIT13_HUFFMAN *h,
    const uint32_t *codes, const uint8_t *lengths, size_t symbol_count)
{
    size_t i;
    int rc = xsit13_huffman_init(h, symbol_count);
    if(rc) return rc;
    for(i = 0; i < symbol_count; i++) {
        if(lengths[i]) {
            rc = xsit13_huffman_insert(h, codes[i], lengths[i], (uint32_t)i);
            if(rc) {
                xsit13_huffman_destroy(h);
                return rc;
            }
        }
    }
    return 0;
}

static int xsit13_huffman_decode(const XSIT13_HUFFMAN *h,
    XSIT13_BITREADER *reader, uint32_t *symbol)
{
    uint32_t node = 0;
    if(!h || !reader || !symbol) return 1;
    for(;;) {
        uint32_t bit = 0;
        uint32_t next;
        int rc;
        if(node >= h->node_count) return 3;
        if(h->leaf[node] != XSIT13_NONE) {
            *symbol = h->leaf[node];
            return 0;
        }
        rc = xsit13_read_bit(reader, &bit);
        if(rc) return rc;
        next = h->links[(size_t)node * 2U + bit];
        if(next == XSIT13_NONE) return 3;
        node = next;
    }
}

static int xsit13_read_code_lengths(XSIT13_BITREADER *reader,
    const XSIT13_HUFFMAN *meta, uint8_t *lengths, size_t count)
{
    size_t used = 0;
    int accumulator = 0;
    if(!reader || !meta || !lengths) return 1;
    while(used < count) {
        uint32_t value = 0;
        size_t extra = 0;
        size_t i;
        uint8_t length;
        int rc = xsit13_huffman_decode(meta, reader, &value);
        if(rc) return rc;
        if(value <= 30U) accumulator = (int)value + 1;
        else if(value == 31U) accumulator = -1;
        else if(value == 32U) {
            if(accumulator == INT_MAX) return 3;
            accumulator++;
        }
        else if(value == 33U) {
            if(accumulator == INT_MIN) return 3;
            accumulator--;
        }
        else if(value == 34U) {
            uint32_t bit = 0;
            rc = xsit13_read_bit(reader, &bit);
            if(rc) return rc;
            if(bit) extra = 1;
        }
        else if(value == 35U) {
            uint32_t bits = 0;
            rc = xsit13_read_bits(reader, 3, &bits);
            if(rc) return rc;
            extra = (size_t)bits + 2U;
        }
        else if(value == 36U) {
            uint32_t bits = 0;
            rc = xsit13_read_bits(reader, 6, &bits);
            if(rc) return rc;
            extra = (size_t)bits + 10U;
        }
        else return 3;
        if(accumulator > (int)XSIT13_MAX_CODE_LENGTH) return 3;
        length = accumulator >= 1 ? (uint8_t)accumulator : 0;
        for(i = 0; i <= extra && used < count; i++) lengths[used++] = length;
    }
    return 0;
}

static int xsit13_prepare_dynamic_codes(XSIT13_BITREADER *reader,
    uint8_t control, XSIT13_HUFFMAN *first, XSIT13_HUFFMAN *second,
    XSIT13_HUFFMAN *offset)
{
    XSIT13_HUFFMAN meta;
    uint8_t first_lengths[XSIT13_LITLEN_SYMBOLS];
    uint8_t second_lengths[XSIT13_LITLEN_SYMBOLS];
    uint8_t offset_lengths[17];
    size_t offset_count = (size_t)(control & 7U) + 10U;
    int rc;
    memset(&meta, 0, sizeof(meta));
    rc = xsit13_huffman_from_codes(&meta, X_SIT13_META_CODE_VALUES,
        X_SIT13_META_CODE_LENGTHS, 37);
    if(rc) return rc;
    rc = xsit13_read_code_lengths(reader, &meta, first_lengths,
        XSIT13_LITLEN_SYMBOLS);
    if(!rc) rc = xsit13_huffman_from_lengths(first, first_lengths,
        XSIT13_LITLEN_SYMBOLS);
    if(!rc) {
        if(control & 8U) memcpy(second_lengths, first_lengths,
            XSIT13_LITLEN_SYMBOLS);
        else rc = xsit13_read_code_lengths(reader, &meta, second_lengths,
            XSIT13_LITLEN_SYMBOLS);
    }
    if(!rc) rc = xsit13_huffman_from_lengths(second, second_lengths,
        XSIT13_LITLEN_SYMBOLS);
    if(!rc) rc = xsit13_read_code_lengths(reader, &meta, offset_lengths,
        offset_count);
    if(!rc) rc = xsit13_huffman_from_lengths(offset, offset_lengths,
        offset_count);
    xsit13_huffman_destroy(&meta);
    return rc;
}

int xstuffit13_decode(const uint8_t *input, size_t input_len,
    uint8_t *output, size_t output_len)
{
    XSIT13_BITREADER reader;
    XSIT13_HUFFMAN first;
    XSIT13_HUFFMAN second;
    XSIT13_HUFFMAN offset;
    uint32_t control_value = 0;
    uint8_t control;
    uint8_t high;
    size_t output_pos = 0;
    int use_first = 1;
    int rc = 0;
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(&offset, 0, sizeof(offset));
    if(output_len == 0) return 0;
    if(!input || input_len == 0 || !output) return 1;
    reader.data = input;
    reader.size = input_len;
    reader.bit_pos = 0;
    rc = xsit13_read_bits(&reader, 8, &control_value);
    if(rc) goto done;
    control = (uint8_t)control_value;
    high = control >> 4;
    if(high == 0) {
        rc = xsit13_prepare_dynamic_codes(&reader, control, &first, &second,
            &offset);
        if(rc) goto done;
    }
    else if(high <= 5) {
        size_t index = (size_t)high - 1U;
        rc = xsit13_huffman_from_lengths(&first, X_SIT13_FIRST[index],
            XSIT13_LITLEN_SYMBOLS);
        if(!rc) rc = xsit13_huffman_from_lengths(&second,
            X_SIT13_SECOND[index], XSIT13_LITLEN_SYMBOLS);
        if(!rc) rc = xsit13_huffman_from_lengths(&offset,
            X_SIT13_OFFSET[index], X_SIT13_OFFSET_SIZE[index]);
        if(rc) goto done;
    }
    else {
        rc = 3;
        goto done;
    }
    while(output_pos < output_len) {
        uint32_t symbol = 0;
        const XSIT13_HUFFMAN *code = use_first ? &first : &second;
        rc = xsit13_huffman_decode(code, &reader, &symbol);
        if(rc) goto done;
        if(symbol <= 0xffU) {
            output[output_pos++] = (uint8_t)symbol;
            use_first = 1;
        }
        else if(symbol == XSIT13_EOS) {
            rc = 3;
            goto done;
        }
        else {
            size_t length;
            size_t distance;
            uint32_t offset_bits = 0;
            uint32_t extra = 0;
            size_t i;
            if(symbol <= 0x13dU) length = (size_t)(symbol - 0x100U) + 3U;
            else if(symbol == 0x13eU) {
                rc = xsit13_read_bits(&reader, 10, &extra);
                if(rc) goto done;
                length = (size_t)extra + 65U;
            }
            else if(symbol == 0x13fU) {
                rc = xsit13_read_bits(&reader, 15, &extra);
                if(rc) goto done;
                length = (size_t)extra + 65U;
            }
            else {
                rc = 3;
                goto done;
            }
            rc = xsit13_huffman_decode(&offset, &reader, &offset_bits);
            if(rc) goto done;
            if(offset_bits == 0) distance = 1;
            else if(offset_bits == 1) distance = 2;
            else {
                if(offset_bits > 17U) {
                    rc = 3;
                    goto done;
                }
                rc = xsit13_read_bits(&reader, offset_bits - 1U, &extra);
                if(rc) goto done;
                distance = ((size_t)1U << (offset_bits - 1U)) +
                    (size_t)extra + 1U;
            }
            if(distance == 0 || distance > 0x10000U || distance > output_pos ||
                length > output_len - output_pos) {
                rc = 3;
                goto done;
            }
            for(i = 0; i < length; i++) {
                output[output_pos] = output[output_pos - distance];
                output_pos++;
            }
            use_first = 0;
        }
    }

done:
    xsit13_huffman_destroy(&first);
    xsit13_huffman_destroy(&second);
    xsit13_huffman_destroy(&offset);
    return rc;
}
