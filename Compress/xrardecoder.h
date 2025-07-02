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
#ifndef XRARDECODER_H
#define XRARDECODER_H

#include <QtCore>
#include <QIODevice>
#include "xbinary.h"

// Load 4 big endian bytes from memory and return uint32.
inline quint32 RawGetBE4(const quint8 *m)
{
#if defined(USE_MEM_BYTESWAP) && defined(_MSC_VER)
    return _byteswap_ulong(*(quint32 *)m);
#elif defined(USE_MEM_BYTESWAP) && (defined(__clang__) || defined(__GNUC__))
    return __builtin_bswap32(*(quint32 *)m);
#else
    return quint32(m[0]) << 24 | quint32(m[1]) << 16 | quint32(m[2]) << 8 | m[3];
#endif
}

// Load 8 big endian bytes from memory and return uint64.
inline quint64 RawGetBE8(const quint8 *m)
{
#if defined(USE_MEM_BYTESWAP) && defined(_MSC_VER)
    return _byteswap_uint64(*(quint64 *)m);
#elif defined(USE_MEM_BYTESWAP) && (defined(__clang__) || defined(__GNUC__))
    return quint64(*(quint64 *)m);
#else
    return quint64(m[0]) << 56 | quint64(m[1]) << 48 | quint64(m[2]) << 40 | quint64(m[3]) << 32 | quint64(m[4]) << 24 | quint64(m[5]) << 16 | quint64(m[6]) << 8 | m[7];
#endif
}

inline void RawPut4(uint Field, void *Data)
{
#if defined(BIG_ENDIAN) || !defined(ALLOW_MISALIGNED)
    quint8 *D = (quint8 *)Data;
    D[0] = (quint8)(Field);
    D[1] = (quint8)(Field >> 8);
    D[2] = (quint8)(Field >> 16);
    D[3] = (quint8)(Field >> 24);
#else
    *(quint32 *)Data = (quint32)Field;
#endif
}

inline quint32 RawGet4(const void *Data)
{
#if defined(BIG_ENDIAN) || !defined(ALLOW_MISALIGNED)
    quint8 *D = (quint8 *)Data;
    return D[0] + (D[1] << 8) + (D[2] << 16) + (D[3] << 24);
#else
    return *(quint32 *)Data;
#endif
}
// Combine pack and unpack constants to class to avoid polluting global
// namespace with numerous short names.
class PackDef {
public:
    // Maximum LZ match length we can encode even for short distances.
    static const uint MAX_LZ_MATCH = 0x1001;

    // We increment LZ match length for longer distances, because shortest
    // matches are not allowed for them. Maximum length increment is 3
    // for distances larger than 256KB (0x40000). Here we define the maximum
    // incremented LZ match. Normally packer does not use it, but we must be
    // ready to process it in corrupt archives.
    static const uint MAX_INC_LZ_MATCH = MAX_LZ_MATCH + 3;

    static const uint MAX3_LZ_MATCH = 0x101;  // Maximum match length for RAR v3.
    static const uint MAX3_INC_LZ_MATCH = MAX3_LZ_MATCH + 3;
    static const uint LOW_DIST_REP_COUNT = 16;

    static const uint NC = 306;  /* alphabet = {0, 1, 2, ..., NC - 1} */
    static const uint DCB = 64;  // Base distance codes up to 4 GB.
    static const uint DCX = 80;  // Extended distance codes up to 1 TB.
    static const uint LDC = 16;
    static const uint RC = 44;
    static const uint HUFF_TABLE_SIZEB = NC + DCB + RC + LDC;
    static const uint HUFF_TABLE_SIZEX = NC + DCX + RC + LDC;
    static const uint BC = 20;

    static const uint NC30 = 299; /* alphabet = {0, 1, 2, ..., NC - 1} */
    static const uint DC30 = 60;
    static const uint LDC30 = 17;
    static const uint RC30 = 28;
    static const uint BC30 = 20;
    static const uint HUFF_TABLE_SIZE30 = NC30 + DC30 + RC30 + LDC30;

    static const uint NC20 = 298; /* alphabet = {0, 1, 2, ..., NC - 1} */
    static const uint DC20 = 48;
    static const uint RC20 = 28;
    static const uint BC20 = 19;
    static const uint MC20 = 257;

    // Largest alphabet size among all values listed above.
    static const uint LARGEST_TABLE_SIZE = 306;
};

enum FilterType {
    // These values must not be changed, because we use them directly
    // in RAR5 compression and decompression code.
    FILTER_DELTA = 0,
    FILTER_E8,
    FILTER_E8E9,
    FILTER_ARM,
    FILTER_AUDIO,
    FILTER_RGB,
    FILTER_ITANIUM,
    FILTER_TEXT,

    // These values can be changed.
    FILTER_LONGRANGE,
    FILTER_EXHAUSTIVE,
    FILTER_NONE
};

// Maximum allowed number of compressed bits processed in quick mode.
#define MAX_QUICK_DECODE_BITS 9

// Maximum number of filters per entire data block. Must be at least
// twice more than MAX_PACK_FILTERS to store filters from two data blocks.
#define MAX_UNPACK_FILTERS 8192

// Maximum number of filters per entire data block for RAR3 unpack.
// Must be at least twice more than v3_MAX_PACK_FILTERS to store filters
// from two data blocks.
#define MAX3_UNPACK_FILTERS 8192

// Limit maximum number of channels in RAR3 delta filter to some reasonable
// value to prevent too slow processing of corrupt archives with invalid
// channels number. Must be equal or larger than v3_MAX_FILTER_CHANNELS.
// No need to provide it for RAR5, which uses only 5 bits to store channels.
#define MAX3_UNPACK_CHANNELS 1024

// Maximum size of single filter block. We restrict it to limit memory
// allocation. Must be equal or larger than MAX_ANALYZE_SIZE.
#define MAX_FILTER_BLOCK_SIZE 0x400000

// Write data in 4 MB or smaller blocks. Must not exceed PACK_MAX_READ,
// so we keep the number of buffered filters in unpacker reasonable.
#define UNPACK_MAX_WRITE 0x400000

// Get lowest 16 bits.
#define GET_SHORT16(x) (sizeof(ushort) == 2 ? (ushort)(x) : ((x)&0xffff))
#define ASIZE(x) (sizeof(x) / sizeof(x[0]))

// Maximum dictionary allowed by compression. Can be less than
// maximum dictionary supported by decompression.
#define PACK_MAX_DICT 0x1000000000ULL  // 64 GB.

// Maximum dictionary allowed by decompression.
#define UNPACK_MAX_DICT 0x1000000000ULL  // 64 GB.

// (int) cast before "low" added only to suppress compiler warnings.
#define ARI_DEC_NORMALIZE(code, low, range, read)                                                    \
    {                                                                                                \
        while ((low ^ (low + range)) < TOP || range < BOT && ((range = -(int)low & (BOT - 1)), 1)) { \
            code = (code << 8) | read->GetChar();                                                    \
            range <<= 8;                                                                             \
            low <<= 8;                                                                               \
        }                                                                                            \
    }

#define VM_MEMSIZE 0x40000
#define VM_MEMMASK (VM_MEMSIZE - 1)

class BitInput {
public:
    enum BufferSize {
        MAX_SIZE = 0x8000
    };  // Size of input buffer.

    int InAddr;  // Curent byte position in the buffer.
    int InBit;   // Current bit position in the current byte.

    bool ExternalBuffer;

public:
    BitInput(bool AllocBuffer);
    ~BitInput();

    quint8 *InBuf;  // Dynamically allocated input buffer.

    void InitBitInput()
    {
        InAddr = InBit = 0;
    }

    // Move forward by 'Bits' bits.
    void addbits(uint Bits)
    {
        Bits += InBit;
        InAddr += Bits >> 3;
        InBit = Bits & 7;
    }

    // Return 16 bits from current position in the buffer.
    // Bit at (InAddr,InBit) has the highest position in returning data.
    uint getbits()
    {
        uint BitField = (uint)InBuf[InAddr] << 16;
        BitField |= (uint)InBuf[InAddr + 1] << 8;
        BitField |= (uint)InBuf[InAddr + 2];
        BitField >>= (8 - InBit);

        return BitField & 0xffff;
    }

    // Return 32 bits from current position in the buffer.
    // Bit at (InAddr,InBit) has the highest position in returning data.
    uint getbits32()
    {
        uint BitField = RawGetBE4(InBuf + InAddr);
        BitField <<= InBit;
        BitField |= (uint)InBuf[InAddr + 4] >> (8 - InBit);
        return BitField & 0xffffffff;
    }

    // Return 64 bits from current position in the buffer.
    // Bit at (InAddr,InBit) has the highest position in returning data.
    quint64 getbits64()
    {
        quint64 BitField = RawGetBE8(InBuf + InAddr);
        BitField <<= InBit;
        BitField |= (uint)InBuf[InAddr + 8] >> (8 - InBit);
        return BitField;
    }

    void faddbits(uint Bits);
    uint fgetbits();

    // Check if buffer has enough space for IncPtr bytes. Returns 'true'
    // if buffer will be overflown.
    bool Overflow(uint IncPtr)
    {
        return InAddr + IncPtr >= MAX_SIZE;
    }

    void SetExternalBuffer(quint8 *Buf);
};

struct RARPPM_MEM_BLK {
    ushort Stamp, NU;
    RARPPM_MEM_BLK *next, *prev;
    void insertAt(RARPPM_MEM_BLK *p)
    {
        next = (prev = p)->next;
        p->next = next->prev = this;
    }
    void remove()
    {
        prev->next = next;
        next->prev = prev;
    }
};

class SubAllocator {
private:
    static const int N1 = 4, N2 = 4, N3 = 4, N4 = (128 + 3 - 1 * N1 - 2 * N2 - 3 * N3) / 4;
    static const int N_INDEXES = N1 + N2 + N3 + N4;

    struct RAR_NODE {
        RAR_NODE *next;
    };

    inline void InsertNode(void *p, int indx);
    inline void *RemoveNode(int indx);
    inline uint U2B(int NU);
    inline void SplitBlock(void *pv, int OldIndx, int NewIndx);
    inline void GlueFreeBlocks();
    void *AllocUnitsRare(int indx);
    inline RARPPM_MEM_BLK *MBPtr(RARPPM_MEM_BLK *BasePtr, int Items);

    long SubAllocatorSize;
    quint8 Indx2Units[N_INDEXES], Units2Indx[128], GlueCount;
    quint8 *HeapStart, *LoUnit, *HiUnit;
    struct RAR_NODE FreeList[N_INDEXES];

public:
    SubAllocator();
    ~SubAllocator()
    {
        StopSubAllocator();
    }
    void Clean();
    bool StartSubAllocator(int SASize);
    void StopSubAllocator();
    void InitSubAllocator();
    inline void *AllocContext();
    inline void *AllocUnits(int NU);
    inline void *ExpandUnits(void *OldPtr, int OldNU);
    inline void *ShrinkUnits(void *OldPtr, int OldNU, int NewNU);
    inline void FreeUnits(void *ptr, int OldNU);
    long GetAllocatedMemory()
    {
        return (SubAllocatorSize);
    }

    quint8 *pText, *UnitsStart, *HeapEnd, *FakeUnitsStart;
};

enum VM_StandardFilters {
    VMSF_NONE,
    VMSF_E8,
    VMSF_E8E9,
    VMSF_ITANIUM,
    VMSF_RGB,
    VMSF_AUDIO,
    VMSF_DELTA
};

struct VM_PreparedProgram {
    VM_PreparedProgram()
    {
        FilteredDataSize = 0;
        Type = VMSF_NONE;
    }
    VM_StandardFilters Type;
    uint InitR[7];
    quint8 *FilteredData;
    uint FilteredDataSize;
};

class RarVM {
private:
    bool ExecuteStandardFilter(VM_StandardFilters FilterType);
    uint FilterItanium_GetBits(quint8 *Data, uint BitPos, uint BitCount);
    void FilterItanium_SetBits(quint8 *Data, uint BitField, uint BitPos, uint BitCount);

    quint8 *Mem;
    uint R[8];

public:
    RarVM();
    ~RarVM();
    void Init();
    void Prepare(quint8 *Code, uint CodeSize, VM_PreparedProgram *Prg);
    void Execute(VM_PreparedProgram *Prg);
    void SetMemory(size_t Pos, quint8 *Data, size_t DataSize);
    static uint ReadData(BitInput &Inp);
};

// Decode compressed bit fields to alphabet numbers.
struct DecodeTable : PackDef {
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
    quint8 QuickLen[1 << MAX_QUICK_DECODE_BITS];

    // Translates compressed bits (up to QuickBits length)
    // to position in alphabet in quick mode.
    // 'ushort' saves some memory and even provides a little speed gain
    // comparing to 'uint' here.
    ushort QuickNum[1 << MAX_QUICK_DECODE_BITS];

    // Translate the position in code list to position in alphabet.
    // We do not allocate it dynamically to avoid performance overhead
    // introduced by pointer, so we use the largest possible table size
    // as array dimension. Real size of this array is defined in MaxNum.
    // We use this array if compressed bit field is too lengthy
    // for QuickLen based translation.
    // 'ushort' saves some memory and even provides a little speed gain
    // comparting to 'uint' here.
    ushort DecodeNum[LARGEST_TABLE_SIZE];
};

struct UnpackBlockHeader {
    int BlockSize;
    int BlockBitSize;
    int BlockStart;
    int HeaderSize;
    bool LastBlockInFile;
    bool TablePresent;
};

struct UnpackBlockTables {
    DecodeTable LD;   // Decode literals.
    DecodeTable DD;   // Decode distances.
    DecodeTable LDD;  // Decode lower bits of distances.
    DecodeTable RD;   // Decode repeating distances.
    DecodeTable BD;   // Decode bit lengths in Huffman table.
};

struct UnpackFilter {
    // Groop 'byte' and 'bool' together to reduce the actual struct size.
    quint8 Type;
    quint8 Channels;
    bool NextWindow;

    size_t BlockStart;
    uint BlockLength;
};

struct UnpackFilter30 {
    unsigned int BlockStart;
    unsigned int BlockLength;
    bool NextWindow;

    // Position of parent filter in Filters array used as prototype for filter
    // in PrgStack array. Not defined for filters in Filters array.
    unsigned int ParentFilter;

    VM_PreparedProgram Prg;
};

struct AudioVariables  // For RAR 2.0 archives only.
{
    int K1, K2, K3, K4, K5;
    int D1, D2, D3, D4;
    int LastDelta;
    unsigned int Dif[11];
    unsigned int ByteCount;
    int LastChar;
};

// We can use the fragmented dictionary in case heap does not have the single
// large enough memory block. It is slower than normal dictionary.
class FragmentedWindow {
private:
    enum {
        MAX_MEM_BLOCKS = 32
    };

    void Reset();
    quint8 *Mem[MAX_MEM_BLOCKS];
    size_t MemSize[MAX_MEM_BLOCKS];
    size_t LastAllocated;

public:
    FragmentedWindow();
    ~FragmentedWindow();
    void Init(size_t WinSize);
    quint8 &operator[](size_t Item);
    void CopyString(uint Length, size_t Distance, size_t &UnpPtr, bool FirstWinDone, size_t MaxWinSize);
    void CopyData(quint8 *Dest, size_t WinPos, size_t Size);
    size_t GetBlockSize(size_t StartPos, size_t RequiredSize);
    size_t GetWinSize()
    {
        return LastAllocated;
    }
};

struct RARPPM_DEF {
    static const int INT_BITS = 7, PERIOD_BITS = 7, TOT_BITS = INT_BITS + PERIOD_BITS, INTERVAL = 1 << INT_BITS, BIN_SCALE = 1 << TOT_BITS, MAX_FREQ = 124;
};

struct RARPPM_SEE2_CONTEXT : RARPPM_DEF {  // SEE-contexts for PPM-contexts with masked symbols
    ushort Summ;
    quint8 Shift, Count;
    void init(int InitVal)
    {
        Summ = InitVal << (Shift = PERIOD_BITS - 4);
        Count = 4;
    }
    uint getMean()
    {
        short RetVal = GET_SHORT16(Summ) >> Shift;
        Summ -= RetVal;
        return RetVal + (RetVal == 0);
    }
    void update()
    {
        if (Shift < PERIOD_BITS && --Count == 0) {
            Summ += Summ;
            Count = 3 << Shift++;
        }
    }
};

class ModelPPM;
class rar_Unpack;
struct RARPPM_CONTEXT;

struct RARPPM_STATE {
    quint8 Symbol;
    quint8 Freq;
    RARPPM_CONTEXT *Successor;
};

class RangeCoder {
public:
    void InitDecoder(rar_Unpack *UnpackRead);
    inline int GetCurrentCount();
    inline uint GetCurrentShiftCount(uint SHIFT);
    inline void Decode();
    inline void PutChar(unsigned int c);
    inline quint8 GetChar();

    uint low, code, range;
    struct SUBRANGE {
        uint LowCount, HighCount, scale;
    } SubRange;

    rar_Unpack *UnpackRead;
};

class LargePageAlloc {
private:
    void *new_large(size_t Size);
    bool delete_large(void *Addr);
    bool UseLargePages;

public:
    LargePageAlloc();
    void AllowLargePages(bool Allow);

    template <class T>
    T *new_l(size_t Size, bool Clear = false)
    {
        T *Allocated = (T *)new_large(Size * sizeof(T));
        if (Allocated == nullptr) Allocated = Clear ? new T[Size]{} : new T[Size];
        return Allocated;
    }

    template <class T>
    void delete_l(T *Addr)
    {
        if (!delete_large(Addr)) delete[] Addr;
    }
};

struct RARPPM_CONTEXT : RARPPM_DEF {
    ushort NumStats;

    struct FreqData {
        ushort SummFreq;
        RARPPM_STATE *Stats;
    };

    union {
        FreqData U;
        RARPPM_STATE OneState;
    };

    RARPPM_CONTEXT *Suffix;
    inline void encodeBinSymbol(ModelPPM *Model, int symbol);  // MaxOrder:
    inline void encodeSymbol1(ModelPPM *Model, int symbol);    //  ABCD    context
    inline void encodeSymbol2(ModelPPM *Model, int symbol);    //   BCD    suffix
    inline void decodeBinSymbol(ModelPPM *Model);              //   BCDE   successor
    inline bool decodeSymbol1(ModelPPM *Model);                // other orders:
    inline bool decodeSymbol2(ModelPPM *Model);                //   BCD    context
    inline void update1(ModelPPM *Model, RARPPM_STATE *p);     //    CD    suffix
    inline void update2(ModelPPM *Model, RARPPM_STATE *p);     //   BCDE   successor
    void rescale(ModelPPM *Model);
    inline RARPPM_CONTEXT *createChild(ModelPPM *Model, RARPPM_STATE *pStats, RARPPM_STATE &FirstState);
    inline RARPPM_SEE2_CONTEXT *makeEscFreq2(ModelPPM *Model, int Diff);
};

class ModelPPM : RARPPM_DEF {
private:
    friend struct RARPPM_CONTEXT;

    RARPPM_SEE2_CONTEXT SEE2Cont[25][16], DummySEE2Cont;

    struct RARPPM_CONTEXT *MinContext, *MedContext, *MaxContext;
    RARPPM_STATE *FoundState;  // found next state transition
    int NumMasked, InitEsc, OrderFall, MaxOrder, RunLength, InitRL;
    quint8 CharMask[256], NS2Indx[256], NS2BSIndx[256], HB2Flag[256];
    quint8 EscCount, PrevSuccess, HiBitsFlag;
    ushort BinSumm[128][64];  // binary SEE-contexts

    RangeCoder Coder;
    SubAllocator SubAlloc;

    void RestartModelRare();
    void StartModelRare(int MaxOrder);
    inline RARPPM_CONTEXT *CreateSuccessors(bool Skip, RARPPM_STATE *p1);

    inline void UpdateModel();
    inline void ClearMask();

public:
    ModelPPM();
    void CleanUp();  // reset PPM variables after data error
    bool DecodeInit(rar_Unpack *UnpackRead, int &EscChar);
    int DecodeChar();
};

class rar_Unpack : PackDef {
private:
    QIODevice *g_pDeviceInput;
    QIODevice *g_pDeviceOutput;

public:
    void Unpack5(bool Solid, XBinary::PDSTRUCT *pPdStruct);

private:
    bool UnpReadBuf();
    void UnpWriteBuf();
    quint8 *ApplyFilter(quint8 *Data, uint DataSize, UnpackFilter *Flt);
    void UnpWriteArea(size_t StartPtr, size_t EndPtr);
    void UnpWriteData(quint8 *Data, size_t Size);
    uint SlotToLength(BitInput &Inp, uint Slot);
    void UnpInitData50(bool Solid);
    bool ReadBlockHeader(BitInput &Inp, UnpackBlockHeader &Header);
    bool ReadTables(BitInput &Inp, UnpackBlockHeader &Header, UnpackBlockTables &Tables);
    void MakeDecodeTables(quint8 *LengthTable, DecodeTable *Dec, uint Size);
    uint DecodeNumber(BitInput &Inp, DecodeTable *Dec);
    inline void InsertOldDist(size_t Distance);
    void UnpInitData(bool Solid);
    void CopyString(uint Length, size_t Distance);
    uint ReadFilterData(BitInput &Inp);
    bool ReadFilter(BitInput &Inp, UnpackFilter &Filter);
    bool AddFilter(UnpackFilter &Filter);
    bool AddFilter();
    void InitFilters();

    // ComprDataIO *UnpIO;
    BitInput Inp;
    LargePageAlloc Alloc;

    std::vector<quint8> FilterSrcMemory;
    std::vector<quint8> FilterDstMemory;

    // Filters code, one entry per filter.
    std::vector<UnpackFilter> Filters;

    size_t OldDist[4], OldDistPtr;
    uint LastLength;

    // LastDist is necessary only for RAR2 and older with circular OldDist
    // array. In RAR3 last distance is always stored in OldDist[0].
    uint LastDist;

    size_t UnpPtr;  // Current position in window.

    size_t PrevPtr;     // UnpPtr value for previous loop iteration.
    bool FirstWinDone;  // At least one dictionary was processed.

    size_t WrPtr;  // Last written unpacked data position.

    // Top border of read packed data.
    int ReadTop;

    // Border to call UnpReadBuf. We use it instead of (ReadTop-C)
    // for optimization reasons. Ensures that we have C bytes in buffer
    // unless we are at the end of file.
    int ReadBorder;

    UnpackBlockHeader BlockHeader;
    UnpackBlockTables BlockTables;

    size_t WriteBorder;  // Perform write when reaching this border.

    quint8 *Window;

    FragmentedWindow FragWindow;
    bool Fragmented;

    qint64 DestUnpSize;

    bool Suspended;
    bool UnpSomeRead;
    qint64 WrittenFileSize;
    bool FileExtracted;

    /***************************** rar_Unpack v 1.5 *********************************/
public:
    void Unpack15(bool Solid, XBinary::PDSTRUCT *pPdStruct);

private:
    void ShortLZ();
    void LongLZ();
    void HuffDecode();
    void GetFlagsBuf();
    void UnpInitData15(bool Solid);
    void InitHuff();
    void CorrHuff(ushort *CharSet, quint8 *NumToPlace);
    void CopyString15(uint Distance, uint Length);
    uint DecodeNum(uint Num, uint StartPos, uint *DecTab, uint *PosTab);

    ushort ChSet[256], ChSetA[256], ChSetB[256], ChSetC[256];
    quint8 NToPl[256], NToPlB[256], NToPlC[256];
    uint FlagBuf, AvrPlc, AvrPlcB, AvrLn1, AvrLn2, AvrLn3;
    int Buf60, NumHuf, StMode, LCount, FlagsCnt;
    uint Nhfb, Nlzb, MaxDist3;
    /***************************** rar_Unpack v 1.5 *********************************/

    /***************************** rar_Unpack v 2.0 *********************************/
public:
    void Unpack20(bool Solid, XBinary::PDSTRUCT *pPdStruct);

private:
    DecodeTable MD[4];  // Decode multimedia data, up to 4 channels.

    unsigned char UnpOldTable20[MC20 * 4];
    bool UnpAudioBlock;
    uint UnpChannels, UnpCurChannel;
    int UnpChannelDelta;
    void CopyString20(uint Length, uint Distance);
    bool ReadTables20();
    void UnpWriteBuf20();
    void UnpInitData20(int Solid);
    void ReadLastTables();
    quint8 DecodeAudio(int Delta);
    struct AudioVariables AudV[4];
    /***************************** rar_Unpack v 2.0 *********************************/

    /***************************** rar_Unpack v 3.0 *********************************/
    enum BLOCK_TYPES {
        BLOCK_LZ,
        BLOCK_PPM
    };

    void UnpInitData30(bool Solid);

public:
    void Unpack29(bool Solid, XBinary::PDSTRUCT *pPdStruct);

private:
    void InitFilters30(bool Solid);
    bool ReadEndOfBlock();
    bool ReadVMCode();
    bool ReadVMCodePPM();
    bool AddVMCode(uint FirstByte, quint8 *Code, uint CodeSize);
    int SafePPMDecodeChar();
    bool ReadTables30();
    bool UnpReadBuf30();
    void UnpWriteBuf30();
    void ExecuteCode(VM_PreparedProgram *Prg);

    int PrevLowDist, LowDistRepCount;

    ModelPPM PPM;
    int PPMEscChar;

    quint8 UnpOldTable[HUFF_TABLE_SIZE30];
    int UnpBlockType;

    // If we already read decoding tables for rar_Unpack v2,v3,v5.
    // We should not use a single variable for all algorithm versions,
    // because we can have a corrupt archive with one algorithm file
    // followed by another algorithm file with "solid" flag and we do not
    // want to reuse tables from one algorithm in another.
    bool TablesRead2, TablesRead3, TablesRead5;

    // Virtual machine to execute filters code.
    RarVM VM;

    // Buffer to read VM filters code. We moved it here from AddVMCode
    // function to reduce time spent in BitInput constructor.
    BitInput VMCodeInp;

    // Filters code, one entry per filter.
    std::vector<UnpackFilter30 *> Filters30;

    // Filters stack, several entrances of same filter are possible.
    std::vector<UnpackFilter30 *> PrgStack;

    // Lengths of preceding data blocks, one length of one last block
    // for every filter. Used to reduce the size required to write
    // the data block length if lengths are repeating.
    std::vector<int> OldFilterLengths;

    int LastFilter;
    /***************************** rar_Unpack v 3.0 *********************************/

public:
    // rar_Unpack(ComprDataIO *DataIO);
    rar_Unpack(QIODevice *pDeviceInput, QIODevice *pDeviceOut);
    ~rar_Unpack();
    qint32 Init(quint64 WinSize, bool Solid);
    void AllowLargePages(bool Allow)
    {
        Alloc.AllowLargePages(Allow);
    }
    void DoUnpack(uint Method, bool Solid);
    bool IsFileExtracted()
    {
        return FileExtracted;
    }
    void SetDestSize(qint64 DestSize)
    {
        DestUnpSize = DestSize;
        FileExtracted = false;
    }
    void SetSuspended(bool Suspended)
    {
        rar_Unpack::Suspended = Suspended;
    }

    quint64 AllocWinSize;
    size_t MaxWinSize;
    size_t MaxWinMask;

    bool ExtraDist;  // Allow distances up to 1 TB.

    quint8 GetChar()
    {
        if (Inp.InAddr > BitInput::MAX_SIZE - 30) {
            UnpReadBuf();
            if (Inp.InAddr >= BitInput::MAX_SIZE)  // If nothing was read.
                return 0;
        }
        return Inp.InBuf[Inp.InAddr++];
    }

    // If window position crosses the window beginning, wrap it to window end.
    // Replaces &MaxWinMask for non-power 2 window sizes.
    // We can't use %WinSize everywhere not only for performance reason,
    // but also because C++ % is reminder instead of modulo.
    // We need additional checks in the code if WinPos distance from 0
    // can exceed MaxWinSize. Alternatively we could add such check here.
    inline size_t WrapDown(size_t WinPos)
    {
        return WinPos >= MaxWinSize ? WinPos + MaxWinSize : WinPos;
    }

    // If window position crosses the window end, wrap it to window beginning.
    // Replaces &MaxWinMask for non-power 2 window sizes.
    // Unlike WrapDown, we can use %WinSize here if there was no size_t
    // overflow when calculating WinPos.
    // We need additional checks in the code if WinPos distance from MaxWinSize
    // can be MaxWinSize or more. Alternatively we could add such check here
    // or use %WinSize.
    inline size_t WrapUp(size_t WinPos)
    {
        return WinPos >= MaxWinSize ? WinPos - MaxWinSize : WinPos;
    }
};

#endif  // XRARDECODER_H
