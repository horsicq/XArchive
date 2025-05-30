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

    // Maximum number of filters per entire data block. Must be at least
    // twice more than MAX_PACK_FILTERS to store filters from two data blocks.
    static const uint RAR_MAX_UNPACK_FILTERS = 8192;

    // Maximum number of filters per entire data block for RAR3 unpack.
    // Must be at least twice more than v3_MAX_PACK_FILTERS to store filters
    // from two data blocks.
    static const uint RAR_MAX3_UNPACK_FILTERS = 8192;

    // Limit maximum number of channels in RAR3 delta filter to some reasonable
    // value to prevent too slow processing of corrupt archives with invalid
    // channels number. Must be equal or larger than v3_MAX_FILTER_CHANNELS.
    // No need to provide it for RAR5, which uses only 5 bits to store channels.
    static const uint RAR_MAX3_UNPACK_CHANNELS = 1024;

    // Maximum LZ match length we can encode even for short distances.
    static const uint RAR_MAX_LZ_MATCH = 0x1001;

    static const uint RAR_VM_MEMSIZE = 0x40000;
    static const uint RAR_VM_MEMMASK = (RAR_VM_MEMSIZE - 1);

    // We increment LZ match length for longer distances, because shortest
    // matches are not allowed for them. Maximum length increment is 3
    // for distances larger than 256KB (0x40000). Here we define the maximum
    // incremented LZ match. Normally packer does not use it, but we must be
    // ready to process it in corrupt archives.
    static const uint RAR_MAX_INC_LZ_MATCH = RAR_MAX_LZ_MATCH + 3;

    static const uint RAR_MAX3_LZ_MATCH = 0x101;  // Maximum match length for RAR v3.
    static const uint RAR_MAX3_INC_LZ_MATCH = RAR_MAX3_LZ_MATCH + 3;
    static const uint RAR_LOW_DIST_REP_COUNT = 16;
    static const uint RAR_NC = 306;  /* alphabet = {0, 1, 2, ..., NC - 1} */
    static const uint RAR_DCB = 64;  // Base distance codes up to 4 GB.
    static const uint RAR_DCX = 80;  // Extended distance codes up to 1 TB.
    static const uint RAR_LDC = 16;
    static const uint RAR_RC = 44;
    static const uint RAR_HUFF_TABLE_SIZEB = RAR_NC + RAR_DCB + RAR_RC + RAR_LDC;
    static const uint RAR_HUFF_TABLE_SIZEX = RAR_NC + RAR_DCX + RAR_RC + RAR_LDC;
    static const uint RAR_BC = 20;
    // Maximum allowed number of compressed bits processed in quick mode.
    static const uint RAR_MAX_QUICK_DECODE_BITS = 9;
    static const uint RAR_NC20 = 298; /* alphabet = {0, 1, 2, ..., NC - 1} */
    static const uint RAR_DC20 = 48;
    static const uint RAR_RC20 = 28;
    static const uint RAR_BC20 = 19;
    static const uint RAR_MC20 = 257;
    static const uint RAR_NC30 = 299; /* alphabet = {0, 1, 2, ..., NC - 1} */
    static const uint RAR_DC30 = 60;
    static const uint RAR_LDC30 = 17;
    static const uint RAR_RC30 = 28;
    static const uint RAR_BC30 = 20;
    static const uint RAR_HUFF_TABLE_SIZE30 = RAR_NC30 + RAR_DC30 + RAR_RC30 + RAR_LDC30;

    static const uint RAR_LARGEST_TABLE_SIZE = 306;
    // Write data in 4 MB or smaller blocks. Must not exceed PACK_MAX_READ,
    // so we keep the number of buffered filters in unpacker reasonable.
    static const size_t RAR_UNPACK_MAX_WRITE = 0x400000;
    static const size_t RAR_BufferSize_MAX_SIZE = 0x8000;
    static const size_t RAR_BitInput_MAX_SIZE = 0x8000;

    // Decode compressed bit fields to alphabet numbers.
    struct rar_DecodeTable {
        // Real size of DecodeNum table.
        uint MaxNum;

        // Left aligned start and upper limit codes defining code space
        // ranges for bit lengths. DecodeLen[BitLength-1] defines the start of
        // range for bit length and DecodeLen[BitLength] defines next code
        // after the end of range or in other words the upper limit code
        // for specified bit length.
        uint DecodeLen[16];

        // Every item of this array contains the sum of all preceding items.
        // So it contains the start position in code list for every bit length.
        uint DecodePos[16];

        // Number of compressed bits processed in quick mode.
        // Must not exceed MAX_QUICK_DECODE_BITS.
        uint QuickBits;

        // Translates compressed bits (up to QuickBits length)
        // to bit length in quick mode.
        quint8 QuickLen[1 << RAR_MAX_QUICK_DECODE_BITS];

        // Translates compressed bits (up to QuickBits length)
        // to position in alphabet in quick mode.
        // 'ushort' saves some memory and even provides a little speed gain
        // comparing to 'uint' here.
        ushort QuickNum[1 << RAR_MAX_QUICK_DECODE_BITS];

        // Translate the position in code list to position in alphabet.
        // We do not allocate it dynamically to avoid performance overhead
        // introduced by pointer, so we use the largest possible table size
        // as array dimension. Real size of this array is defined in MaxNum.
        // We use this array if compressed bit field is too lengthy
        // for QuickLen based translation.
        // 'ushort' saves some memory and even provides a little speed gain
        // comparting to 'uint' here.
        ushort DecodeNum[RAR_LARGEST_TABLE_SIZE];
    };

    struct rar_UnpackBlockHeader {
        int BlockSize;
        int BlockBitSize;
        int BlockStart;
        int HeaderSize;
        bool LastBlockInFile;
        bool TablePresent;
    };

    struct rar_UnpackBlockTables {
        rar_DecodeTable LD;   // Decode literals.
        rar_DecodeTable DD;   // Decode distances.
        rar_DecodeTable LDD;  // Decode lower bits of distances.
        rar_DecodeTable RD;   // Decode repeating distances.
        rar_DecodeTable BD;   // Decode bit lengths in Huffman table.
    };

    struct rar_UnpackFilter {
        // Groop 'byte' and 'bool' together to reduce the actual struct size.
        quint8 Type;
        quint8 Channels;
        bool NextWindow;

        size_t BlockStart;
        uint BlockLength;
    };

    struct rar_AudioVariables  // For RAR 2.0 archives only.
    {
        int K1, K2, K3, K4, K5;
        int D1, D2, D3, D4;
        int LastDelta;
        unsigned int Dif[11];
        unsigned int ByteCount;
        int LastChar;
    };

    enum rar_VM_StandardFilters {
        VMSF_NONE,
        VMSF_E8,
        VMSF_E8E9,
        VMSF_ITANIUM,
        VMSF_RGB,
        VMSF_AUDIO,
        VMSF_DELTA
    };

    struct rar_VM_PreparedProgram {
        // VM_PreparedProgram()
        // {
        //   FilteredDataSize=0;
        //   Type=VMSF_NONE;
        // }
        rar_VM_StandardFilters Type;
        uint InitR[7];
        quint8 *FilteredData;
        uint FilteredDataSize;
    };

    struct rar_UnpackFilter30 {
        unsigned int BlockStart;
        unsigned int BlockLength;
        bool NextWindow;

        // Position of parent filter in Filters array used as prototype for filter
        // in PrgStack array. Not defined for filters in Filters array.
        unsigned int ParentFilter;

        rar_VM_PreparedProgram Prg;
    };

    enum RAR_BLOCK_TYPES {
        RAR_BLOCK_LZ,
        RAR_BLOCK_PPM
    };

    struct rar_inp_bit {
        // Bits
        bool ExternalBuffer;
        int InAddr;     // Curent byte position in the buffer.
        int InBit;      // Current bit position in the current byte.
        quint8 *InBuf;  // Input buffer.
    };

    struct rar_VM {
        quint8 *Mem;
        uint R[8];
    };

    struct rar_stream {
        quint8 *Window;

        // FragmentedWindow FragWindow;
        bool Fragmented;

        qint64 DestUnpSize;

        bool Suspended;
        bool UnpSomeRead;
        bool FileExtracted;

        ushort ChSet[256], ChSetA[256], ChSetB[256], ChSetC[256];
        quint8 NToPl[256], NToPlB[256], NToPlC[256];
        uint FlagBuf, AvrPlc, AvrPlcB, AvrLn1, AvrLn2, AvrLn3;
        int Buf60, NumHuf, StMode, LCount, FlagsCnt;
        uint Nhfb, Nlzb, MaxDist3;

        quint64 AllocWinSize;
        size_t MaxWinSize;
        size_t MaxWinMask;

        bool ExtraDist;  // Allow distances up to 1 TB.
        size_t OldDist[4], OldDistPtr;
        uint LastLength;
        uint LastDist;      // LastDist is necessary only for RAR2 and older with circular OldDist array. In RAR3 last distance is always stored in OldDist[0].
        size_t UnpPtr;      // Current position in window.
        size_t PrevPtr;     // UnpPtr value for previous loop iteration.
        bool FirstWinDone;  // At least one dictionary was processed.
        size_t WrPtr;       // Last written unpacked data position.
        qint64 WrittenFileSize;
        int ReadTop;     // Top border of read packed data.
        int ReadBorder;  // Border to call UnpReadBuf. We use it instead of (ReadTop-C) for optimization reasons. Ensures that we have C bytes in buffer unless we are at
                         // the end of file.
        size_t WriteBorder;  // Perform write when reaching this border.
        rar_UnpackBlockHeader BlockHeader;
        rar_UnpackBlockTables BlockTables;
        std::vector<rar_UnpackFilter> Filters;
        // 2
        rar_DecodeTable MD[4];  // Decode multimedia data, up to 4 channels.
        unsigned char UnpOldTable20[RAR_MC20 * 4];
        bool UnpAudioBlock;
        uint UnpChannels, UnpCurChannel;
        int UnpChannelDelta;
        struct rar_AudioVariables AudV[4];
        // 3

        int PrevLowDist, LowDistRepCount;
        // rar_ModelPPM PPM;
        int PPMEscChar;
        quint8 UnpOldTable[RAR_HUFF_TABLE_SIZE30];
        int UnpBlockType;

        // If we already read decoding tables for Unpack v2,v3,v5.
        // We should not use a single variable for all algorithm versions,
        // because we can have a corrupt archive with one algorithm file
        // followed by another algorithm file with "solid" flag and we do not
        // want to reuse tables from one algorithm in another.
        bool TablesRead2, TablesRead3, TablesRead5;

        // // Virtual machine to execute filters code.
        rar_VM VM;
        // // Buffer to read VM filters code. We moved it here from AddVMCode
        // // function to reduce time spent in BitInput constructor.
        // Filters code, one entry per filter.
        std::vector<rar_UnpackFilter30 *> Filters30;

        // Filters stack, several entrances of same filter are possible.
        std::vector<rar_UnpackFilter30 *> PrgStack;

        // Lengths of preceding data blocks, one length of one last block
        // for every filter. Used to reduce the size required to write
        // the data block length if lengths are repeating.
        std::vector<int> OldFilterLengths;

        int LastFilter;

        rar_inp_bit Inp;
        rar_inp_bit VMCodeInp;
    };

    static void rar_init(struct rar_stream *strm, quint64 WinSize, bool Solid);
    static void rar_InitHuff(struct rar_stream *strm);
    static void rar_CorrHuff(struct rar_stream *strm, ushort *CharSet, quint8 *NumToPlace);
    static void rar_UnpInitData(struct rar_stream *strm, bool Solid);
    static void rar_UnpInitData15(struct rar_stream *strm, bool Solid);
    static void rar_UnpInitData20(struct rar_stream *strm, bool Solid);
    static void rar_UnpInitData30(struct rar_stream *strm, bool Solid);
    static void rar_UnpInitData50(struct rar_stream *strm, bool Solid);
    static void rar_InitFilters30(struct rar_stream *strm, bool Solid);
    static bool rar_UnpReadBuf(struct rar_stream *strm, QIODevice *pDevice);
    static bool rar_UnpReadBuf30(struct rar_stream *strm, QIODevice *pDevice);
    static void rar_UnpWriteBuf20(struct rar_stream *strm, QIODevice *pDevice);
    static void rar_UnpWriteBuf30(struct rar_stream *strm, QIODevice *pDevice);
    static void rar_UnpWriteArea(struct rar_stream *strm, QIODevice *pDevice, size_t StartPtr, size_t EndPtr);
    static void rar_UnpWriteData(struct rar_stream *strm, QIODevice *pDevice, quint8 *Data, size_t Size);
    static void rar_GetFlagsBuf(struct rar_stream *strm);
    static uint rar_DecodeNum(struct rar_stream *strm, uint Num, uint StartPos, uint *DecTab, uint *PosTab);
    static uint rar_fgetbits(struct rar_inp_bit *bits);
    static void rar_faddbits(struct rar_inp_bit *bits, uint Bits);
    static uint rar_getbits(struct rar_inp_bit *bits);
    static void rar_addbits(struct rar_inp_bit *bits, uint Bits);
    static void rar_InitBitInput(struct rar_inp_bit *bits);
    static bool rar_Overflow(struct rar_inp_bit *bits, uint IncPtr);
    static uint rar_VM_ReadData(struct rar_inp_bit *bits);
    static void rar_HuffDecode(struct rar_stream *strm);
    static void rar_LongLZ(struct rar_stream *strm);
    static void rar_ShortLZ(struct rar_stream *strm);
    static void rar_CopyString15(struct rar_stream *strm, uint Distance, uint Length);
    static void rar_CopyString(struct rar_stream *strm, uint Distance, uint Length);
    static bool rar_ReadTables20(struct rar_stream *strm, QIODevice *pDevice);
    static bool rar_ReadTables30(struct rar_stream *strm, QIODevice *pDevice);
    static void rar_MakeDecodeTables(struct rar_stream *strm, quint8 *LengthTable, rar_DecodeTable *Dec, uint Size);
    static uint rar_DecodeNumber(struct rar_stream *strm, rar_DecodeTable *Dec);
    static void rar_InsertOldDist(struct rar_stream *strm, size_t Distance);
    static size_t rar_WrapUp(struct rar_stream *strm, size_t WinPos);
    static bool rar_ReadEndOfBlock(struct rar_stream *strm, QIODevice *pDevice);
    static bool rar_ReadVMCode(struct rar_stream *strm, QIODevice *pDevice);
    static bool rar_AddVMCode(struct rar_stream *strm, uint FirstByte, quint8 *Code, uint CodeSize);
    static void rar_VM_Init(struct rar_VM *vm);
    static void rar_VM_final(struct rar_VM *vm);
    static void rar_VM_Prepare(quint8 *Code, uint CodeSize, rar_VM_PreparedProgram *Prg);
    static void rar_VM_SetMemory(struct rar_VM *vm, size_t Pos, quint8 *Data, size_t DataSize);
    static void rar_VM_ExecuteCode(struct rar_VM *vm, rar_VM_PreparedProgram *Prg);
    static bool rar_VM_ExecuteStandardFilter(struct rar_VM *vm, rar_VM_StandardFilters FilterType);
    static uint rar_CRC32(uint StartCRC, const void *Addr, size_t Size);
    static uint rar_VM_FilterItanium_GetBits(quint8 *Data, uint BitPos, uint BitCount);
    static void rar_VM_FilterItanium_SetBits(quint8 *Data, uint BitField, uint BitPos, uint BitCount);
    static quint32 rar_RawGet4(const void *Data);
    static void rar_RawPut4(quint32 Value, void *Data);
};

#endif  // XCOMPRESS_H
