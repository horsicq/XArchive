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
#include "xrardecoder.h"

#define STARTL1 2
static uint DecL1[] = {0x8000, 0xa000, 0xc000, 0xd000, 0xe000, 0xea00, 0xee00, 0xf000, 0xf200, 0xf200, 0xffff};
static uint PosL1[] = {0, 0, 0, 2, 3, 5, 7, 11, 16, 20, 24, 32, 32};

#define STARTL2 3
static uint DecL2[] = {0xa000, 0xc000, 0xd000, 0xe000, 0xea00, 0xee00, 0xf000, 0xf200, 0xf240, 0xffff};
static uint PosL2[] = {0, 0, 0, 0, 5, 7, 9, 13, 18, 22, 26, 34, 36};

#define STARTHF0 4
static uint DecHf0[] = {0x8000, 0xc000, 0xe000, 0xf200, 0xf200, 0xf200, 0xf200, 0xf200, 0xffff};
static uint PosHf0[] = {0, 0, 0, 0, 0, 8, 16, 24, 33, 33, 33, 33, 33};

#define STARTHF1 5
static uint DecHf1[] = {0x2000, 0xc000, 0xe000, 0xf000, 0xf200, 0xf200, 0xf7e0, 0xffff};
static uint PosHf1[] = {0, 0, 0, 0, 0, 0, 4, 44, 60, 76, 80, 80, 127};

#define STARTHF2 5
static uint DecHf2[] = {0x1000, 0x2400, 0x8000, 0xc000, 0xfa00, 0xffff, 0xffff, 0xffff};
static uint PosHf2[] = {0, 0, 0, 0, 0, 0, 2, 7, 53, 117, 233, 0, 0};

#define STARTHF3 6
static uint DecHf3[] = {0x800, 0x2400, 0xee00, 0xfe80, 0xffff, 0xffff, 0xffff};
static uint PosHf3[] = {0, 0, 0, 0, 0, 0, 0, 2, 16, 218, 251, 0, 0};

#define STARTHF4 8
static uint DecHf4[] = {0xff00, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
static uint PosHf4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0};

#define GetShortLen1(pos) ((pos) == 1 ? Buf60 + 3 : ShortLen1[pos])
#define GetShortLen2(pos) ((pos) == 3 ? Buf60 + 3 : ShortLen2[pos])

bool rar_Unpack::UnpReadBuf()
{
    int DataSize = ReadTop - Inp.InAddr;  // Data left to process.
    if (DataSize < 0) return false;
    BlockHeader.BlockSize -= Inp.InAddr - BlockHeader.BlockStart;
    if (Inp.InAddr > BitInput::MAX_SIZE / 2) {
        // If we already processed more than half of buffer, let's move
        // remaining data into beginning to free more space for new data
        // and ensure that calling function does not cross the buffer border
        // even if we did not read anything here. Also it ensures that read size
        // is not less than CRYPT_BLOCK_SIZE, so we can align it without risk
        // to make it zero.
        if (DataSize > 0) memmove(Inp.InBuf, Inp.InBuf + Inp.InAddr, DataSize);
        Inp.InAddr = 0;
        ReadTop = DataSize;
    } else DataSize = ReadTop;
    int ReadCode = 0;
    if (BitInput::MAX_SIZE != DataSize) ReadCode = g_pDeviceInput->read((char *)(Inp.InBuf + DataSize), BitInput::MAX_SIZE - DataSize);
    if (ReadCode > 0)  // Can be also -1.
        ReadTop += ReadCode;
    ReadBorder = ReadTop - 30;
    BlockHeader.BlockStart = Inp.InAddr;
    if (BlockHeader.BlockSize != -1)  // '-1' means not defined yet.
    {
        // We may need to quit from main extraction loop and read new block header
        // and trees earlier than data in input buffer ends.
        ReadBorder = qMin(ReadBorder, BlockHeader.BlockStart + BlockHeader.BlockSize - 1);
    }
    return ReadCode != -1;
}

void rar_Unpack::UnpWriteBuf()
{
    size_t WrittenBorder = WrPtr;
    size_t FullWriteSize = WrapDown(UnpPtr - WrittenBorder);
    size_t WriteSizeLeft = FullWriteSize;
    bool NotAllFiltersProcessed = false;
    for (size_t I = 0; I < Filters.size(); I++) {
        // Here we apply filters to data which we need to write.
        // We always copy data to another memory block before processing.
        // We cannot process them just in place in Window buffer, because
        // these data can be used for future string matches, so we must
        // preserve them in original form.

        UnpackFilter *flt = &Filters[I];
        if (flt->Type == FILTER_NONE) continue;
        if (flt->NextWindow) {
            // Here we skip filters which have block start in current data range
            // due to address wrap around in circular dictionary, but actually
            // belong to next dictionary block. If such filter start position
            // is included to current write range, then we reset 'NextWindow' flag.
            // In fact we can reset it even without such check, because current
            // implementation seems to guarantee 'NextWindow' flag reset after
            // buffer writing for all existing filters. But let's keep this check
            // just in case. Compressor guarantees that distance between
            // filter block start and filter storing position cannot exceed
            // the dictionary size. So if we covered the filter block start with
            // our write here, we can safely assume that filter is applicable
            // to next block and no further wrap arounds is possible.
            if (WrapDown(flt->BlockStart - WrPtr) <= FullWriteSize) flt->NextWindow = false;
            continue;
        }
        size_t BlockStart = flt->BlockStart;
        uint BlockLength = flt->BlockLength;
        if (WrapDown(BlockStart - WrittenBorder) < WriteSizeLeft) {
            if (WrittenBorder != BlockStart) {
                UnpWriteArea(WrittenBorder, BlockStart);
                WrittenBorder = BlockStart;
                WriteSizeLeft = WrapDown(UnpPtr - WrittenBorder);
            }
            if (BlockLength <= WriteSizeLeft) {
                if (BlockLength > 0)  // We set it to 0 also for invalid filters.
                {
                    size_t BlockEnd = WrapUp(BlockStart + BlockLength);

                    FilterSrcMemory.resize(BlockLength);
                    quint8 *Mem = FilterSrcMemory.data();
                    if (BlockStart < BlockEnd || BlockEnd == 0) {
                        if (Fragmented) FragWindow.CopyData(Mem, BlockStart, BlockLength);
                        else memcpy(Mem, Window + BlockStart, BlockLength);
                    } else {
                        size_t FirstPartLength = size_t(MaxWinSize - BlockStart);
                        if (Fragmented) {
                            FragWindow.CopyData(Mem, BlockStart, FirstPartLength);
                            FragWindow.CopyData(Mem + FirstPartLength, 0, BlockEnd);
                        } else {
                            memcpy(Mem, Window + BlockStart, FirstPartLength);
                            memcpy(Mem + FirstPartLength, Window, BlockEnd);
                        }
                    }

                    quint8 *OutMem = ApplyFilter(Mem, BlockLength, flt);

                    Filters[I].Type = FILTER_NONE;

                    if (OutMem != NULL) g_pDeviceOutput->write((char *)OutMem, BlockLength);

                    UnpSomeRead = true;
                    WrittenFileSize += BlockLength;
                    WrittenBorder = BlockEnd;
                    WriteSizeLeft = WrapDown(UnpPtr - WrittenBorder);
                }
            } else {
                // Current filter intersects the window write border, so we adjust
                // the window border to process this filter next time, not now.
                WrPtr = WrittenBorder;

                // Since Filter start position can only increase, we quit processing
                // all following filters for this data block and reset 'NextWindow'
                // flag for them.
                for (size_t J = I; J < Filters.size(); J++) {
                    UnpackFilter *flt = &Filters[J];
                    if (flt->Type != FILTER_NONE) flt->NextWindow = false;
                }

                // Do not write data left after current filter now.
                NotAllFiltersProcessed = true;
                break;
            }
        }
    }

    // Remove processed filters from queue.
    size_t EmptyCount = 0;
    for (size_t I = 0; I < Filters.size(); I++) {
        if (EmptyCount > 0) Filters[I - EmptyCount] = Filters[I];
        if (Filters[I].Type == FILTER_NONE) EmptyCount++;
    }
    if (EmptyCount > 0) Filters.resize(Filters.size() - EmptyCount);

    if (!NotAllFiltersProcessed)  // Only if all filters are processed.
    {
        // Write data left after last filter.
        UnpWriteArea(WrittenBorder, UnpPtr);
        WrPtr = UnpPtr;
    }

    // We prefer to write data in blocks not exceeding UNPACK_MAX_WRITE
    // instead of potentially huge MaxWinSize blocks. It also allows us
    // to keep the size of Filters array reasonable.
    WriteBorder = WrapUp(UnpPtr + qMin(MaxWinSize, (size_t)UNPACK_MAX_WRITE));

    // Choose the nearest among WriteBorder and WrPtr actual written border.
    // If border is equal to UnpPtr, it means that we have MaxWinSize data ahead.
    if (WriteBorder == UnpPtr || WrPtr != UnpPtr && WrapDown(WrPtr - UnpPtr) < WrapDown(WriteBorder - UnpPtr)) WriteBorder = WrPtr;
}

quint8 *rar_Unpack::ApplyFilter(quint8 *Data, uint DataSize, UnpackFilter *Flt)
{
    quint8 *SrcData = Data;
    switch (Flt->Type) {
        case FILTER_E8:
        case FILTER_E8E9: {
            uint FileOffset = (uint)WrittenFileSize;

            const uint FileSize = 0x1000000;
            quint8 CmpByte2 = Flt->Type == FILTER_E8E9 ? 0xe9 : 0xe8;
            // DataSize is unsigned, so we use "CurPos+4" and not "DataSize-4"
            // to avoid overflow for DataSize<4.
            for (uint CurPos = 0; CurPos + 4 < DataSize;) {
                quint8 CurByte = *(Data++);
                CurPos++;
                if (CurByte == 0xe8 || CurByte == CmpByte2) {
                    uint Offset = (CurPos + FileOffset) % FileSize;
                    uint Addr = RawGet4(Data);

                    // We check 0x80000000 bit instead of '< 0' comparison
                    // not assuming int32 presence or uint size and endianness.
                    if ((Addr & 0x80000000) != 0)  // Addr<0
                    {
                        if (((Addr + Offset) & 0x80000000) == 0)  // Addr+Offset>=0
                            RawPut4(Addr + FileSize, Data);
                    } else if (((Addr - FileSize) & 0x80000000) != 0)  // Addr<FileSize
                        RawPut4(Addr - Offset, Data);

                    Data += 4;
                    CurPos += 4;
                }
            }
        }
            return SrcData;
        case FILTER_ARM:
            // 2019-11-15: we turned off ARM filter by default when compressing,
            // mostly because it is inefficient for modern 64 bit ARM binaries.
            // It was turned on by default in 5.0 - 5.80b3 , so we still need it
            // here for compatibility with some of previously created archives.
            {
                uint FileOffset = (uint)WrittenFileSize;
                // DataSize is unsigned, so we use "CurPos+3" and not "DataSize-3"
                // to avoid overflow for DataSize<3.
                for (uint CurPos = 0; CurPos + 3 < DataSize; CurPos += 4) {
                    quint8 *D = Data + CurPos;
                    if (D[3] == 0xeb)  // BL command with '1110' (Always) condition.
                    {
                        uint Offset = D[0] + uint(D[1]) * 0x100 + uint(D[2]) * 0x10000;
                        Offset -= (FileOffset + CurPos) / 4;
                        D[0] = (quint8)Offset;
                        D[1] = (quint8)(Offset >> 8);
                        D[2] = (quint8)(Offset >> 16);
                    }
                }
            }
            return SrcData;
        case FILTER_DELTA: {
            // Unlike RAR3, we do not need to reject excessive channel
            // values here, since RAR5 uses only 5 bits to store channel.
            uint Channels = Flt->Channels, SrcPos = 0;

            FilterDstMemory.resize(DataSize);
            quint8 *DstData = FilterDstMemory.data();

            // Bytes from same channels are grouped to continual data blocks,
            // so we need to place them back to their interleaving positions.
            for (uint CurChannel = 0; CurChannel < Channels; CurChannel++) {
                quint8 PrevByte = 0;
                for (uint DestPos = CurChannel; DestPos < DataSize; DestPos += Channels) DstData[DestPos] = (PrevByte -= Data[SrcPos++]);
            }
            return DstData;
        }
    }
    return NULL;
}

void rar_Unpack::UnpWriteArea(size_t StartPtr, size_t EndPtr)
{
    if (EndPtr != StartPtr) UnpSomeRead = true;

    if (Fragmented) {
        size_t SizeToWrite = WrapDown(EndPtr - StartPtr);
        while (SizeToWrite > 0) {
            size_t BlockSize = FragWindow.GetBlockSize(StartPtr, SizeToWrite);
            UnpWriteData(&FragWindow[StartPtr], BlockSize);
            SizeToWrite -= BlockSize;
            StartPtr = WrapUp(StartPtr + BlockSize);
        }
    } else if (EndPtr < StartPtr) {
        UnpWriteData(Window + StartPtr, MaxWinSize - StartPtr);
        UnpWriteData(Window, EndPtr);
    } else UnpWriteData(Window + StartPtr, EndPtr - StartPtr);
}

void rar_Unpack::UnpWriteData(quint8 *Data, size_t Size)
{
    if (WrittenFileSize >= DestUnpSize) return;
    size_t WriteSize = Size;
    qint64 LeftToWrite = DestUnpSize - WrittenFileSize;
    if ((qint64)WriteSize > LeftToWrite) WriteSize = (size_t)LeftToWrite;
    g_pDeviceOutput->write((char *)Data, WriteSize);
    WrittenFileSize += Size;
}

void rar_Unpack::UnpInitData50(bool Solid)
{
    if (!Solid) TablesRead5 = false;
}

void rar_Unpack::UnpInitData(bool Solid)
{
    if (!Solid) {
        OldDist[0] = OldDist[1] = OldDist[2] = OldDist[3] = (size_t)-1;

        OldDistPtr = 0;

        LastDist = (uint)-1;  // Initialize it to -1 like LastDist.
        LastLength = 0;

        //    memset(Window,0,MaxWinSize);
        memset(&BlockTables, 0, sizeof(BlockTables));
        UnpPtr = WrPtr = 0;
        PrevPtr = 0;
        FirstWinDone = false;
        WriteBorder = qMin(MaxWinSize, (size_t)UNPACK_MAX_WRITE);
    }
    // Filters never share several solid files, so we can safely reset them
    // even in solid archive.
    InitFilters();

    Inp.InitBitInput();
    WrittenFileSize = 0;
    ReadTop = 0;
    ReadBorder = 0;

    memset(&BlockHeader, 0, sizeof(BlockHeader));
    BlockHeader.BlockSize = -1;  // '-1' means not defined yet.
    UnpInitData20(Solid);
    UnpInitData30(Solid);
    UnpInitData50(Solid);
}

void rar_Unpack::InitFilters()
{
}

void rar_Unpack::Unpack15(bool Solid, XBinary::PDSTRUCT *pPdStruct)
{
    UnpInitData(Solid);
    UnpInitData15(Solid);
    UnpReadBuf();
    if (!Solid) {
        InitHuff();
        UnpPtr = 0;
    } else UnpPtr = WrPtr;
    --DestUnpSize;
    if (DestUnpSize >= 0) {
        GetFlagsBuf();
        FlagsCnt = 8;
    }

    while (DestUnpSize >= 0) {
        UnpPtr &= MaxWinMask;

        FirstWinDone |= (PrevPtr > UnpPtr);
        PrevPtr = UnpPtr;

        if (Inp.InAddr > ReadTop - 30 && !UnpReadBuf()) break;
        if (((WrPtr - UnpPtr) & MaxWinMask) < 270 && WrPtr != UnpPtr) UnpWriteBuf20();
        if (StMode) {
            HuffDecode();
            continue;
        }

        if (--FlagsCnt < 0) {
            GetFlagsBuf();
            FlagsCnt = 7;
        }

        if (FlagBuf & 0x80) {
            FlagBuf <<= 1;
            if (Nlzb > Nhfb) LongLZ();
            else HuffDecode();
        } else {
            FlagBuf <<= 1;
            if (--FlagsCnt < 0) {
                GetFlagsBuf();
                FlagsCnt = 7;
            }
            if (FlagBuf & 0x80) {
                FlagBuf <<= 1;
                if (Nlzb > Nhfb) HuffDecode();
                else LongLZ();
            } else {
                FlagBuf <<= 1;
                ShortLZ();
            }
        }
    }
    UnpWriteBuf20();
}

void rar_Unpack::ShortLZ()
{
    static uint ShortLen1[] = {1, 3, 4, 4, 5, 6, 7, 8, 8, 4, 4, 5, 6, 6, 4, 0};
    static uint ShortXor1[] = {0, 0xa0, 0xd0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xc0, 0x80, 0x90, 0x98, 0x9c, 0xb0};
    static uint ShortLen2[] = {2, 3, 3, 3, 4, 4, 5, 6, 6, 4, 4, 5, 6, 6, 4, 0};
    static uint ShortXor2[] = {0, 0x40, 0x60, 0xa0, 0xd0, 0xe0, 0xf0, 0xf8, 0xfc, 0xc0, 0x80, 0x90, 0x98, 0x9c, 0xb0};

    uint Length, SaveLength;
    uint LastDistance;
    uint Distance;
    int DistancePlace;
    NumHuf = 0;

    uint BitField = Inp.fgetbits();
    if (LCount == 2) {
        Inp.faddbits(1);
        if (BitField >= 0x8000) {
            CopyString15(LastDist, LastLength);
            return;
        }
        BitField <<= 1;
        LCount = 0;
    }

    BitField >>= 8;

    //  not thread safe, replaced by GetShortLen1 and GetShortLen2 macro
    //  ShortLen1[1]=ShortLen2[3]=Buf60+3;

    if (AvrLn1 < 37) {
        for (Length = 0;; Length++)
            if (((BitField ^ ShortXor1[Length]) & (~(0xff >> GetShortLen1(Length)))) == 0) break;
        Inp.faddbits(GetShortLen1(Length));
    } else {
        for (Length = 0;; Length++)
            if (((BitField ^ ShortXor2[Length]) & (~(0xff >> GetShortLen2(Length)))) == 0) break;
        Inp.faddbits(GetShortLen2(Length));
    }

    if (Length >= 9) {
        if (Length == 9) {
            LCount++;
            CopyString15(LastDist, LastLength);
            return;
        }
        if (Length == 14) {
            LCount = 0;
            Length = DecodeNum(Inp.fgetbits(), STARTL2, DecL2, PosL2) + 5;
            Distance = (Inp.fgetbits() >> 1) | 0x8000;
            Inp.faddbits(15);
            LastLength = Length;
            LastDist = Distance;
            CopyString15(Distance, Length);
            return;
        }

        LCount = 0;
        SaveLength = Length;
        Distance = (uint)OldDist[(OldDistPtr - (Length - 9)) & 3];
        Length = DecodeNum(Inp.fgetbits(), STARTL1, DecL1, PosL1) + 2;
        if (Length == 0x101 && SaveLength == 10) {
            Buf60 ^= 1;
            return;
        }
        if (Distance > 256) Length++;
        if (Distance >= MaxDist3) Length++;

        OldDist[OldDistPtr++] = Distance;
        OldDistPtr = OldDistPtr & 3;
        LastLength = Length;
        LastDist = Distance;
        CopyString15(Distance, Length);
        return;
    }

    LCount = 0;
    AvrLn1 += Length;
    AvrLn1 -= AvrLn1 >> 4;

    DistancePlace = DecodeNum(Inp.fgetbits(), STARTHF2, DecHf2, PosHf2) & 0xff;
    Distance = ChSetA[DistancePlace];
    if (--DistancePlace != -1) {
        LastDistance = ChSetA[DistancePlace];
        ChSetA[DistancePlace + 1] = (ushort)LastDistance;
        ChSetA[DistancePlace] = (ushort)Distance;
    }
    Length += 2;
    OldDist[OldDistPtr++] = ++Distance;
    OldDistPtr = OldDistPtr & 3;
    LastLength = Length;
    LastDist = Distance;
    CopyString15(Distance, Length);
}

void rar_Unpack::LongLZ()
{
    uint Length;
    uint Distance;
    uint DistancePlace, NewDistancePlace;
    uint OldAvr2, OldAvr3;

    NumHuf = 0;
    Nlzb += 16;
    if (Nlzb > 0xff) {
        Nlzb = 0x90;
        Nhfb >>= 1;
    }
    OldAvr2 = AvrLn2;

    uint BitField = Inp.fgetbits();
    if (AvrLn2 >= 122) Length = DecodeNum(BitField, STARTL2, DecL2, PosL2);
    else if (AvrLn2 >= 64) Length = DecodeNum(BitField, STARTL1, DecL1, PosL1);
    else if (BitField < 0x100) {
        Length = BitField;
        Inp.faddbits(16);
    } else {
        for (Length = 0; ((BitField << Length) & 0x8000) == 0; Length++)
            ;
        Inp.faddbits(Length + 1);
    }

    AvrLn2 += Length;
    AvrLn2 -= AvrLn2 >> 5;

    BitField = Inp.fgetbits();
    if (AvrPlcB > 0x28ff) DistancePlace = DecodeNum(BitField, STARTHF2, DecHf2, PosHf2);
    else if (AvrPlcB > 0x6ff) DistancePlace = DecodeNum(BitField, STARTHF1, DecHf1, PosHf1);
    else DistancePlace = DecodeNum(BitField, STARTHF0, DecHf0, PosHf0);

    AvrPlcB += DistancePlace;
    AvrPlcB -= AvrPlcB >> 8;
    while (1) {
        Distance = ChSetB[DistancePlace & 0xff];
        NewDistancePlace = NToPlB[Distance++ & 0xff]++;
        if (!(Distance & 0xff)) CorrHuff(ChSetB, NToPlB);
        else break;
    }

    ChSetB[DistancePlace & 0xff] = ChSetB[NewDistancePlace];
    ChSetB[NewDistancePlace] = (ushort)Distance;

    Distance = ((Distance & 0xff00) | (Inp.fgetbits() >> 8)) >> 1;
    Inp.faddbits(7);

    OldAvr3 = AvrLn3;
    if (Length != 1 && Length != 4)
        if (Length == 0 && Distance <= MaxDist3) {
            AvrLn3++;
            AvrLn3 -= AvrLn3 >> 8;
        } else if (AvrLn3 > 0) AvrLn3--;
    Length += 3;
    if (Distance >= MaxDist3) Length++;
    if (Distance <= 256) Length += 8;
    if (OldAvr3 > 0xb0 || AvrPlc >= 0x2a00 && OldAvr2 < 0x40) MaxDist3 = 0x7f00;
    else MaxDist3 = 0x2001;
    OldDist[OldDistPtr++] = Distance;
    OldDistPtr = OldDistPtr & 3;
    LastLength = Length;
    LastDist = Distance;
    CopyString15(Distance, Length);
}

void rar_Unpack::HuffDecode()
{
    uint CurByte, NewBytePlace;
    uint Length;
    uint Distance;
    int BytePlace;

    uint BitField = Inp.fgetbits();

    if (AvrPlc > 0x75ff) BytePlace = DecodeNum(BitField, STARTHF4, DecHf4, PosHf4);
    else if (AvrPlc > 0x5dff) BytePlace = DecodeNum(BitField, STARTHF3, DecHf3, PosHf3);
    else if (AvrPlc > 0x35ff) BytePlace = DecodeNum(BitField, STARTHF2, DecHf2, PosHf2);
    else if (AvrPlc > 0x0dff) BytePlace = DecodeNum(BitField, STARTHF1, DecHf1, PosHf1);
    else BytePlace = DecodeNum(BitField, STARTHF0, DecHf0, PosHf0);
    BytePlace &= 0xff;
    if (StMode) {
        if (BytePlace == 0 && BitField > 0xfff) BytePlace = 0x100;
        if (--BytePlace == -1) {
            BitField = Inp.fgetbits();
            Inp.faddbits(1);
            if (BitField & 0x8000) {
                NumHuf = StMode = 0;
                return;
            } else {
                Length = (BitField & 0x4000) ? 4 : 3;
                Inp.faddbits(1);
                Distance = DecodeNum(Inp.fgetbits(), STARTHF2, DecHf2, PosHf2);
                Distance = (Distance << 5) | (Inp.fgetbits() >> 11);
                Inp.faddbits(5);
                CopyString15(Distance, Length);
                return;
            }
        }
    } else if (NumHuf++ >= 16 && FlagsCnt == 0) StMode = 1;
    AvrPlc += BytePlace;
    AvrPlc -= AvrPlc >> 8;
    Nhfb += 16;
    if (Nhfb > 0xff) {
        Nhfb = 0x90;
        Nlzb >>= 1;
    }

    Window[UnpPtr++] = (quint8)(ChSet[BytePlace] >> 8);
    --DestUnpSize;

    while (1) {
        CurByte = ChSet[BytePlace];
        NewBytePlace = NToPl[CurByte++ & 0xff]++;
        if ((CurByte & 0xff) > 0xa1) CorrHuff(ChSet, NToPl);
        else break;
    }

    ChSet[BytePlace] = ChSet[NewBytePlace];
    ChSet[NewBytePlace] = (ushort)CurByte;
}

void rar_Unpack::GetFlagsBuf()
{
    uint Flags, NewFlagsPlace;
    uint FlagsPlace = DecodeNum(Inp.fgetbits(), STARTHF2, DecHf2, PosHf2);

    // Our Huffman table stores 257 items and needs all them in other parts
    // of code such as when StMode is on, so the first item is control item.
    // While normally we do not use the last item to code the flags byte here,
    // we need to check for value 256 when unpacking in case we unpack
    // a corrupt archive.
    if (FlagsPlace >= sizeof(ChSetC) / sizeof(ChSetC[0])) return;

    while (1) {
        Flags = ChSetC[FlagsPlace];
        FlagBuf = Flags >> 8;
        NewFlagsPlace = NToPlC[Flags++ & 0xff]++;
        if ((Flags & 0xff) != 0) break;
        CorrHuff(ChSetC, NToPlC);
    }

    ChSetC[FlagsPlace] = ChSetC[NewFlagsPlace];
    ChSetC[NewFlagsPlace] = (ushort)Flags;
}

void rar_Unpack::UnpInitData15(bool Solid)
{
    if (!Solid) {
        AvrPlcB = AvrLn1 = AvrLn2 = AvrLn3 = NumHuf = Buf60 = 0;
        AvrPlc = 0x3500;
        MaxDist3 = 0x2001;
        Nhfb = Nlzb = 0x80;
    }
    FlagsCnt = 0;
    FlagBuf = 0;
    StMode = 0;
    LCount = 0;
    ReadTop = 0;
}

void rar_Unpack::InitHuff()
{
    for (ushort I = 0; I < 256; I++) {
        ChSet[I] = ChSetB[I] = I << 8;
        ChSetA[I] = I;
        ChSetC[I] = ((~I + 1) & 0xff) << 8;
    }
    memset(NToPl, 0, sizeof(NToPl));
    memset(NToPlB, 0, sizeof(NToPlB));
    memset(NToPlC, 0, sizeof(NToPlC));
    CorrHuff(ChSetB, NToPlB);
}

void rar_Unpack::CorrHuff(ushort *CharSet, quint8 *NumToPlace)
{
    int I, J;
    for (I = 7; I >= 0; I--)
        for (J = 0; J < 32; J++, CharSet++) *CharSet = (*CharSet & ~0xff) | I;
    memset(NumToPlace, 0, sizeof(NToPl));
    for (I = 6; I >= 0; I--) NumToPlace[I] = (7 - I) * 32;
}

void rar_Unpack::CopyString15(uint Distance, uint Length)
{
    DestUnpSize -= Length;
    // 2024.04.18: Distance can be 0 in corrupt RAR 1.5 archives.
    if (!FirstWinDone && Distance > UnpPtr || Distance > MaxWinSize || Distance == 0)
        while (Length-- > 0) {
            Window[UnpPtr] = 0;
            UnpPtr = (UnpPtr + 1) & MaxWinMask;
        }
    else
        while (Length-- > 0) {
            Window[UnpPtr] = Window[(UnpPtr - Distance) & MaxWinMask];
            UnpPtr = (UnpPtr + 1) & MaxWinMask;
        }
}

uint rar_Unpack::DecodeNum(uint Num, uint StartPos, uint *DecTab, uint *PosTab)
{
    int I;
    for (Num &= 0xfff0, I = 0; DecTab[I] <= Num; I++) StartPos++;
    Inp.faddbits(StartPos);
    return (((Num - (I ? DecTab[I - 1] : 0)) >> (16 - StartPos)) + PosTab[StartPos]);
}

void rar_Unpack::UnpWriteBuf20()
{
    if (UnpPtr != WrPtr) UnpSomeRead = true;
    if (UnpPtr < WrPtr) {
        g_pDeviceOutput->write((char *)&Window[WrPtr], -(int)WrPtr & MaxWinMask);
        g_pDeviceOutput->write((char *)Window, UnpPtr);

        // 2024.12.24: Before 7.10 we set "UnpAllBuf=true" here. It was needed for
        // Pack::PrepareSolidAppend(). Since both UnpAllBuf and FirstWinDone
        // variables indicate the same thing and we set FirstWinDone in other place
        // anyway, we replaced UnpAllBuf with FirstWinDone and removed this code.
    } else g_pDeviceOutput->write((char *)&Window[WrPtr], UnpPtr - WrPtr);
    WrPtr = UnpPtr;
}

void rar_Unpack::UnpInitData20(int Solid)
{
    if (!Solid) {
        TablesRead2 = false;
        UnpAudioBlock = false;
        UnpChannelDelta = 0;
        UnpCurChannel = 0;
        UnpChannels = 1;

        memset(AudV, 0, sizeof(AudV));
        memset(UnpOldTable20, 0, sizeof(UnpOldTable20));
        memset(MD, 0, sizeof(MD));
    }
}

void rar_Unpack::UnpInitData30(bool Solid)
{
    if (!Solid) {
        TablesRead3 = false;
        memset(UnpOldTable, 0, sizeof(UnpOldTable));
        PPMEscChar = 2;
        UnpBlockType = BLOCK_LZ;
    }
    InitFilters30(Solid);
}

void rar_Unpack::InitFilters30(bool Solid)
{
    if (!Solid) {
        OldFilterLengths.clear();
        LastFilter = 0;

        for (size_t I = 0; I < Filters30.size(); I++) delete Filters30[I];
        Filters30.clear();
    }
    for (size_t I = 0; I < PrgStack.size(); I++) delete PrgStack[I];
    PrgStack.clear();
}

rar_Unpack::rar_Unpack(QIODevice *pDeviceInput, QIODevice *pDeviceOut) : Inp(true), VMCodeInp(true)
{
    g_pDeviceInput = pDeviceInput;
    g_pDeviceOutput = pDeviceOut;
    // UnpIO=DataIO;
    Window = NULL;
    Fragmented = false;
    Suspended = false;
    UnpSomeRead = false;
    ExtraDist = false;
    AllocWinSize = 0;
    MaxWinSize = 0;
    MaxWinMask = 0;

    // Perform initialization, which should be done only once for all files.
    // It prevents crash if first unpacked file has the wrong "true" Solid flag,
    // so first DoUnpack call is made with the wrong "true" Solid value later.
    UnpInitData(false);
    // RAR 1.5 decompression initialization
    UnpInitData15(false);
    InitHuff();
}

rar_Unpack::~rar_Unpack()
{
    InitFilters30(false);

    Alloc.delete_l<quint8>(Window);  // delete Window;
}

void rar_Unpack::Init(quint64 WinSize, bool Solid)
{
    // Minimum window size must be at least twice more than maximum possible
    // size of filter block, which is 0x10000 in RAR now. If window size is
    // smaller, we can have a block with never cleared flt->NextWindow flag
    // in UnpWriteBuf(). Minimum window size 0x20000 would be enough, but let's
    // use 0x40000 for extra safety and possible filter area size expansion.
    const size_t MinAllocSize = 0x40000;
    if (WinSize < MinAllocSize) WinSize = MinAllocSize;

    if (WinSize > qMin(0x10000000000ULL, UNPACK_MAX_DICT))  // Window size must not exceed 1 TB.
        throw std::bad_alloc();

    // 32-bit build can't unpack dictionaries exceeding 32-bit even in theory.
    // Also we've not verified if WrapUp and WrapDown work properly in 32-bit
    // version and >2GB dictionary and if 32-bit version can handle >2GB
    // distances. Since such version is unlikely to allocate >2GB anyway,
    // we prohibit >2GB dictionaries for 32-bit build here.
    if (WinSize > 0x80000000 && sizeof(size_t) <= 4) throw std::bad_alloc();

    // Solid block shall use the same window size for all files.
    // But if Window isn't initialized when Solid is set, it means that
    // first file in solid block doesn't have the solid flag. We initialize
    // the window anyway for such malformed archive.
    // Non-solid files shall use their specific window sizes,
    // so current window size and unpack routine behavior doesn't depend on
    // previously unpacked files and their extraction order.
    if (!Solid || Window == nullptr) {
        MaxWinSize = (size_t)WinSize;
        MaxWinMask = MaxWinSize - 1;
    }

    // Use the already allocated window when processing non-solid files
    // with reducing dictionary sizes.
    if (WinSize <= AllocWinSize) return;

    // Archiving code guarantees that window size does not grow in the same
    // solid stream. So if we are here, we are either creating a new window
    // or increasing the size of non-solid window. So we could safely reject
    // current window data without copying them to a new window.
    if (Solid && (Window != NULL || Fragmented && WinSize > FragWindow.GetWinSize())) throw std::bad_alloc();

    Alloc.delete_l<quint8>(Window);  // delete Window;
    Window = nullptr;

    try {
        if (!Fragmented) Window = Alloc.new_l<quint8>((size_t)WinSize, false);  // Window=new byte[(size_t)WinSize];
    } catch (std::bad_alloc)                                                    // Use the fragmented window in this case.
    {
    }

    if (Window == nullptr)
        if (WinSize < 0x1000000 || sizeof(size_t) > 4) throw std::bad_alloc();  // Exclude RAR4, small dictionaries and 64-bit.
        else {
            if (WinSize > FragWindow.GetWinSize()) FragWindow.Init((size_t)WinSize);
            Fragmented = true;
        }

    if (!Fragmented) {
        // Clean the window to generate the same output when unpacking corrupt
        // RAR files, which may access unused areas of sliding dictionary.
        // 2023.10.31: We've added FirstWinDone based unused area access check
        // in Unpack::CopyString(), so this memset might be unnecessary now.
        //    memset(Window,0,(size_t)WinSize);

        AllocWinSize = WinSize;
    }
}

BitInput::BitInput(bool AllocBuffer)
{
    ExternalBuffer = false;
    if (AllocBuffer) {
        // getbits*() attempt to read data from InAddr, ... InAddr+8 positions.
        // So let's allocate 8 additional bytes for situation, when we need to
        // read only 1 byte from the last position of buffer and avoid a crash
        // from access to next 8 bytes, which contents we do not need.
        size_t BufSize = MAX_SIZE + 8;
        InBuf = new quint8[BufSize];

        // Ensure that we get predictable results when accessing bytes in area
        // not filled with read data.
        memset(InBuf, 0, BufSize);
    } else InBuf = nullptr;
}

BitInput::~BitInput()
{
    if (!ExternalBuffer) delete[] InBuf;
}

void BitInput::faddbits(uint Bits)
{
    // Function wrapped version of inline addbits to reduce the code size.
    addbits(Bits);
}

uint BitInput::fgetbits()
{
    // Function wrapped version of inline getbits to reduce the code size.
    return getbits();
}

SubAllocator::SubAllocator()
{
    Clean();
}

void SubAllocator::Clean()
{
    SubAllocatorSize = 0;
}

void SubAllocator::StopSubAllocator()
{
    if (SubAllocatorSize) {
        SubAllocatorSize = 0;
        free(HeapStart);
    }
}

RarVM::RarVM()
{
    Mem = NULL;
}

RarVM::~RarVM()
{
    if (Mem == NULL) Mem = new quint8[VM_MEMSIZE + 4];
}

void FragmentedWindow::Reset()
{
    LastAllocated = 0;
    for (uint I = 0; I < ASIZE(Mem); I++)
        if (Mem[I] != NULL) {
            free(Mem[I]);
            Mem[I] = NULL;
        }
}

FragmentedWindow::FragmentedWindow()
{
    LastAllocated = 0;
    for (uint I = 0; I < ASIZE(Mem); I++)
        if (Mem[I] != NULL) {
            Mem[I] = NULL;
        }
    // Reset();
}

FragmentedWindow::~FragmentedWindow()
{
    LastAllocated = 0;
    for (uint I = 0; I < ASIZE(Mem); I++)
        if (Mem[I] != NULL) {
            free(Mem[I]);
            Mem[I] = NULL;
        }
}

void FragmentedWindow::Init(size_t WinSize)
{
    Reset();

    uint BlockNum = 0;
    size_t TotalSize = 0;  // Already allocated.
    while (TotalSize < WinSize && BlockNum < ASIZE(Mem)) {
        size_t Size = WinSize - TotalSize;  // Size needed to allocate.

        // Minimum still acceptable block size. Next allocations cannot be larger
        // than current, so we do not need blocks if they are smaller than
        // "size left / attempts left". Also we do not waste time to blocks
        // smaller than some arbitrary constant.
        size_t MinSize = qMax(Size / (ASIZE(Mem) - BlockNum), (size_t)0x400000);

        quint8 *NewMem = NULL;
        while (Size >= MinSize) {
            NewMem = (quint8 *)malloc(Size);
            if (NewMem != NULL) break;
            Size -= Size / 32;
        }
        if (NewMem == NULL) throw std::bad_alloc();

        // Clean the window to generate the same output when unpacking corrupt
        // RAR files, which may access to unused areas of sliding dictionary.
        memset(NewMem, 0, Size);

        Mem[BlockNum] = NewMem;
        TotalSize += Size;
        MemSize[BlockNum] = TotalSize;
        BlockNum++;
    }
    if (TotalSize < WinSize)  // Not found enough free blocks.
        throw std::bad_alloc();
    LastAllocated = WinSize;
}

quint8 &FragmentedWindow::operator[](size_t Item)
{
    if (Item < MemSize[0]) return Mem[0][Item];
    for (uint I = 1; I < ASIZE(MemSize); I++)
        if (Item < MemSize[I]) return Mem[I][Item - MemSize[I - 1]];
    return Mem[0][0];  // Must never happen;
}

void FragmentedWindow::CopyData(quint8 *Dest, size_t WinPos, size_t Size)
{
    for (size_t I = 0; I < Size; I++) Dest[I] = (*this)[WinPos + I];
}

size_t FragmentedWindow::GetBlockSize(size_t StartPos, size_t RequiredSize)
{
    for (uint I = 0; I < ASIZE(MemSize); I++)
        if (StartPos < MemSize[I]) return qMin(MemSize[I] - StartPos, RequiredSize);
    return 0;  // Must never be here.
}

ModelPPM::ModelPPM()
{
    MinContext = NULL;
    MaxContext = NULL;
    MedContext = NULL;
}

void *LargePageAlloc::new_large(size_t Size)
{
    void *Allocated = nullptr;
    return Allocated;
}

bool LargePageAlloc::delete_large(void *Addr)
{
    return false;
}

LargePageAlloc::LargePageAlloc()
{
    UseLargePages = false;
}

void LargePageAlloc::AllowLargePages(bool Allow)
{
    Q_UNUSED(Allow)
}
