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

static const int MAX_O = 64; /* maximum allowed model order */
const uint TOP = 1 << 24, BOT = 1 << 15;

static const uint UNIT_SIZE = qMax(sizeof(RARPPM_CONTEXT), sizeof(RARPPM_MEM_BLK));
static const uint FIXED_UNIT_SIZE = 12;

static uint crc_tables[16][256];  // Tables for Slicing-by-16.

uint CRC32(uint StartCRC, const void *Addr, size_t Size)
{
    quint8 *Data = (quint8 *)Addr;

    for (; Size > 0; Size--, Data++)  // Process left data.
        StartCRC = crc_tables[0][(quint8)(StartCRC ^ Data[0])] ^ (StartCRC >> 8);

    return StartCRC;
}

template <class T>
inline void _PPMD_SWAP(T &t1, T &t2)
{
    T tmp = t1;
    t1 = t2;
    t2 = tmp;
}

void rar_Unpack::Unpack5(bool Solid, XBinary::PDSTRUCT *pPdStruct)
{
    FileExtracted = true;

    if (!Suspended) {
        UnpInitData(Solid);
        if (!UnpReadBuf()) return;

        // Check TablesRead5 to be sure that we read tables at least once
        // regardless of current block header TablePresent flag.
        // So we can safefly use these tables below.
        if (!ReadBlockHeader(Inp, BlockHeader) || !ReadTables(Inp, BlockHeader, BlockTables) || !TablesRead5) return;
    }

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        UnpPtr = WrapUp(UnpPtr);

        // To combine this code with WrapUp above, we also need to set FirstWinDone
        // in CopyString. Performance gain is questionable in this case.
        FirstWinDone |= (PrevPtr > UnpPtr);
        PrevPtr = UnpPtr;

        if (Inp.InAddr >= ReadBorder) {
            bool FileDone = false;

            // We use 'while', because for empty block containing only Huffman table,
            // we'll be on the block border once again just after reading the table.
            while (Inp.InAddr > BlockHeader.BlockStart + BlockHeader.BlockSize - 1 ||
                   Inp.InAddr == BlockHeader.BlockStart + BlockHeader.BlockSize - 1 && Inp.InBit >= BlockHeader.BlockBitSize) {
                if (BlockHeader.LastBlockInFile) {
                    FileDone = true;
                    break;
                }
                if (!ReadBlockHeader(Inp, BlockHeader) || !ReadTables(Inp, BlockHeader, BlockTables)) return;
            }
            if (FileDone || !UnpReadBuf()) break;
        }

        // WriteBorder==UnpPtr means that we have MaxWinSize data ahead.
        if (WrapDown(WriteBorder - UnpPtr) <= MAX_INC_LZ_MATCH && WriteBorder != UnpPtr) {
            UnpWriteBuf();
            if (WrittenFileSize > DestUnpSize) return;
            if (Suspended) {
                FileExtracted = false;
                return;
            }
        }

        uint MainSlot = DecodeNumber(Inp, &BlockTables.LD);
        if (MainSlot < 256) {
            if (Fragmented) FragWindow[UnpPtr++] = (quint8)MainSlot;
            else Window[UnpPtr++] = (quint8)MainSlot;
            continue;
        }
        if (MainSlot >= 262) {
            uint Length = SlotToLength(Inp, MainSlot - 262);

            size_t Distance = 1;
            uint DBits, DistSlot = DecodeNumber(Inp, &BlockTables.DD);
            if (DistSlot < 4) {
                DBits = 0;
                Distance += DistSlot;
            } else {
                DBits = DistSlot / 2 - 1;
                Distance += size_t(2 | (DistSlot & 1)) << DBits;
            }

            if (DBits > 0) {
                if (DBits >= 4) {
                    if (DBits > 4) {
                        // It is also possible to always use getbits64() here.
                        if (DBits > 36) Distance += ((size_t(Inp.getbits64()) >> (68 - DBits)) << 4);
                        else Distance += ((size_t(Inp.getbits32()) >> (36 - DBits)) << 4);
                        Inp.addbits(DBits - 4);
                    }
                    uint LowDist = DecodeNumber(Inp, &BlockTables.LDD);
                    Distance += LowDist;

                    // Distance can be 0 for multiples of 4 GB as result of size_t
                    // overflow in 32-bit build. Its lower 32-bit can also erroneously
                    // fit into dictionary after truncating upper 32-bits. Replace such
                    // invalid distances with -1, so CopyString sets 0 data for them.
                    // DBits>=30 also as DistSlot>=62 indicate distances >=0x80000001.
                    if (sizeof(Distance) == 4 && DBits >= 30) Distance = (size_t)-1;
                } else {
                    Distance += Inp.getbits() >> (16 - DBits);
                    Inp.addbits(DBits);
                }
            }

            if (Distance > 0x100) {
                Length++;
                if (Distance > 0x2000) {
                    Length++;
                    if (Distance > 0x40000) Length++;
                }
            }

            InsertOldDist(Distance);
            LastLength = Length;
            if (Fragmented) FragWindow.CopyString(Length, Distance, UnpPtr, FirstWinDone, MaxWinSize);
            else CopyString(Length, Distance);
            continue;
        }
        if (MainSlot == 256) {
            UnpackFilter Filter;
            if (!ReadFilter(Inp, Filter) || !AddFilter(Filter)) break;
            continue;
        }
        if (MainSlot == 257) {
            if (LastLength != 0)
                if (Fragmented) FragWindow.CopyString(LastLength, OldDist[0], UnpPtr, FirstWinDone, MaxWinSize);
                else CopyString(LastLength, OldDist[0]);
            continue;
        }
        if (MainSlot < 262) {
            uint DistNum = MainSlot - 258;
            size_t Distance = OldDist[DistNum];
            for (uint I = DistNum; I > 0; I--) OldDist[I] = OldDist[I - 1];
            OldDist[0] = Distance;

            uint LengthSlot = DecodeNumber(Inp, &BlockTables.RD);
            uint Length = SlotToLength(Inp, LengthSlot);
            LastLength = Length;
            if (Fragmented) FragWindow.CopyString(Length, Distance, UnpPtr, FirstWinDone, MaxWinSize);
            else CopyString(Length, Distance);
            continue;
        }
    }
    UnpWriteBuf();
}

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

uint rar_Unpack::SlotToLength(BitInput &Inp, uint Slot)
{
    uint LBits, Length = 2;
    if (Slot < 8) {
        LBits = 0;
        Length += Slot;
    } else {
        LBits = Slot / 4 - 1;
        Length += (4 | (Slot & 3)) << LBits;
    }

    if (LBits > 0) {
        Length += Inp.getbits() >> (16 - LBits);
        Inp.addbits(LBits);
    }
    return Length;
}

void rar_Unpack::UnpInitData50(bool Solid)
{
    if (!Solid) TablesRead5 = false;
}

bool rar_Unpack::ReadBlockHeader(BitInput &Inp, UnpackBlockHeader &Header)
{
    Header.HeaderSize = 0;

    if (!Inp.ExternalBuffer && Inp.InAddr > ReadTop - 7)
        if (!UnpReadBuf()) return false;
    Inp.faddbits((8 - Inp.InBit) & 7);

    quint8 BlockFlags = quint8(Inp.fgetbits() >> 8);
    Inp.faddbits(8);
    uint ByteCount = ((BlockFlags >> 3) & 3) + 1;  // Block size byte count.

    if (ByteCount == 4) return false;

    Header.HeaderSize = 2 + ByteCount;

    Header.BlockBitSize = (BlockFlags & 7) + 1;

    quint8 SavedCheckSum = Inp.fgetbits() >> 8;
    Inp.faddbits(8);

    int BlockSize = 0;
    for (uint I = 0; I < ByteCount; I++) {
        BlockSize += (Inp.fgetbits() >> 8) << (I * 8);
        Inp.addbits(8);
    }

    Header.BlockSize = BlockSize;
    quint8 CheckSum = quint8(0x5a ^ BlockFlags ^ BlockSize ^ (BlockSize >> 8) ^ (BlockSize >> 16));

    // 2024.01.04: In theory the valid block can have Header.BlockSize == 0
    // and Header.TablePresent == false in case the only block purpose is to
    // store Header.LastBlockInFile flag if it didn't fit into previous block.
    // So we do not reject Header.BlockSize == 0. Though currently RAR doesn't
    // seem to issue such zero length blocks.
    if (CheckSum != SavedCheckSum) return false;

    Header.BlockStart = Inp.InAddr;

    // We called Inp.faddbits(8) above, thus Header.BlockStart can't be 0 here.
    // So there is no overflow even if Header.BlockSize is 0.
    ReadBorder = qMin(ReadBorder, Header.BlockStart + Header.BlockSize - 1);

    Header.LastBlockInFile = (BlockFlags & 0x40) != 0;
    Header.TablePresent = (BlockFlags & 0x80) != 0;

    return true;
}

bool rar_Unpack::ReadTables(BitInput &Inp, UnpackBlockHeader &Header, UnpackBlockTables &Tables)
{
    if (!Header.TablePresent) return true;

    if (!Inp.ExternalBuffer && Inp.InAddr > ReadTop - 25)
        if (!UnpReadBuf()) return false;

    quint8 BitLength[BC];
    for (uint I = 0; I < BC; I++) {
        uint Length = (quint8)(Inp.fgetbits() >> 12);
        Inp.faddbits(4);
        if (Length == 15) {
            uint ZeroCount = (quint8)(Inp.fgetbits() >> 12);
            Inp.faddbits(4);
            if (ZeroCount == 0) BitLength[I] = 15;
            else {
                ZeroCount += 2;
                while (ZeroCount-- > 0 && I < ASIZE(BitLength)) BitLength[I++] = 0;
                I--;
            }
        } else BitLength[I] = Length;
    }

    MakeDecodeTables(BitLength, &Tables.BD, BC);

    quint8 Table[HUFF_TABLE_SIZEX];
    const uint TableSize = ExtraDist ? HUFF_TABLE_SIZEX : HUFF_TABLE_SIZEB;
    for (uint I = 0; I < TableSize;) {
        if (!Inp.ExternalBuffer && Inp.InAddr > ReadTop - 5)
            if (!UnpReadBuf()) return false;
        uint Number = DecodeNumber(Inp, &Tables.BD);
        if (Number < 16) {
            Table[I] = Number;
            I++;
        } else if (Number < 18) {
            uint N;
            if (Number == 16) {
                N = (Inp.fgetbits() >> 13) + 3;
                Inp.faddbits(3);
            } else {
                N = (Inp.fgetbits() >> 9) + 11;
                Inp.faddbits(7);
            }
            if (I == 0) {
                // We cannot have "repeat previous" code at the first position.
                // Multiple such codes would shift Inp position without changing I,
                // which can lead to reading beyond of Inp boundary in mutithreading
                // mode, where Inp.ExternalBuffer disables bounds check and we just
                // reserve a lot of buffer space to not need such check normally.
                return false;
            } else
                while (N-- > 0 && I < TableSize) {
                    Table[I] = Table[I - 1];
                    I++;
                }
        } else {
            uint N;
            if (Number == 18) {
                N = (Inp.fgetbits() >> 13) + 3;
                Inp.faddbits(3);
            } else {
                N = (Inp.fgetbits() >> 9) + 11;
                Inp.faddbits(7);
            }
            while (N-- > 0 && I < TableSize) Table[I++] = 0;
        }
    }
    TablesRead5 = true;
    if (!Inp.ExternalBuffer && Inp.InAddr > ReadTop) return false;
    MakeDecodeTables(&Table[0], &Tables.LD, NC);
    uint DCodes = ExtraDist ? DCX : DCB;
    MakeDecodeTables(&Table[NC], &Tables.DD, DCodes);
    MakeDecodeTables(&Table[NC + DCodes], &Tables.LDD, LDC);
    MakeDecodeTables(&Table[NC + DCodes + LDC], &Tables.RD, RC);
    return true;
}

void rar_Unpack::MakeDecodeTables(quint8 *LengthTable, DecodeTable *Dec, uint Size)
{
    // Size of alphabet and DecodePos array.
    Dec->MaxNum = Size;

    // Calculate how many entries for every bit length in LengthTable we have.
    uint LengthCount[16];
    memset(LengthCount, 0, sizeof(LengthCount));
    for (size_t I = 0; I < Size; I++) LengthCount[LengthTable[I] & 0xf]++;

    // We must not calculate the number of zero length codes.
    LengthCount[0] = 0;

    // Set the entire DecodeNum to zero.
    memset(Dec->DecodeNum, 0, Size * sizeof(*Dec->DecodeNum));

    // Initialize not really used entry for zero length code.
    Dec->DecodePos[0] = 0;

    // Start code for bit length 1 is 0.
    Dec->DecodeLen[0] = 0;

    // Right aligned upper limit code for current bit length.
    uint UpperLimit = 0;

    for (size_t I = 1; I < 16; I++) {
        // Adjust the upper limit code.
        UpperLimit += LengthCount[I];

        // Left aligned upper limit code.
        uint LeftAligned = UpperLimit << (16 - I);

        // Prepare the upper limit code for next bit length.
        UpperLimit *= 2;

        // Store the left aligned upper limit code.
        Dec->DecodeLen[I] = (uint)LeftAligned;

        // Every item of this array contains the sum of all preceding items.
        // So it contains the start position in code list for every bit length.
        Dec->DecodePos[I] = Dec->DecodePos[I - 1] + LengthCount[I - 1];
    }

    // Prepare the copy of DecodePos. We'll modify this copy below,
    // so we cannot use the original DecodePos.
    uint CopyDecodePos[ASIZE(Dec->DecodePos)];
    memcpy(CopyDecodePos, Dec->DecodePos, sizeof(CopyDecodePos));

    // For every bit length in the bit length table and so for every item
    // of alphabet.
    for (uint I = 0; I < Size; I++) {
        // Get the current bit length.
        quint8 CurBitLength = LengthTable[I] & 0xf;

        if (CurBitLength != 0) {
            // Last position in code list for current bit length.
            uint LastPos = CopyDecodePos[CurBitLength];

            // Prepare the decode table, so this position in code list will be
            // decoded to current alphabet item number.
            Dec->DecodeNum[LastPos] = (ushort)I;

            // We'll use next position number for this bit length next time.
            // So we pass through the entire range of positions available
            // for every bit length.
            CopyDecodePos[CurBitLength]++;
        }
    }

    // Define the number of bits to process in quick mode. We use more bits
    // for larger alphabets. More bits means that more codes will be processed
    // in quick mode, but also that more time will be spent to preparation
    // of tables for quick decode.
    switch (Size) {
        case NC:
        case NC20:
        case NC30: Dec->QuickBits = MAX_QUICK_DECODE_BITS; break;
        default: Dec->QuickBits = MAX_QUICK_DECODE_BITS > 3 ? MAX_QUICK_DECODE_BITS - 3 : 0; break;
    }

    // Size of tables for quick mode.
    uint QuickDataSize = 1 << Dec->QuickBits;

    // Bit length for current code, start from 1 bit codes. It is important
    // to use 1 bit instead of 0 for minimum code length, so we are moving
    // forward even when processing a corrupt archive.
    uint CurBitLength = 1;

    // For every right aligned bit string which supports the quick decoding.
    for (uint Code = 0; Code < QuickDataSize; Code++) {
        // Left align the current code, so it will be in usual bit field format.
        uint BitField = Code << (16 - Dec->QuickBits);

        // Prepare the table for quick decoding of bit lengths.

        // Find the upper limit for current bit field and adjust the bit length
        // accordingly if necessary.
        while (CurBitLength < ASIZE(Dec->DecodeLen) && BitField >= Dec->DecodeLen[CurBitLength]) CurBitLength++;

        // Translation of right aligned bit string to bit length.
        Dec->QuickLen[Code] = CurBitLength;

        // Prepare the table for quick translation of position in code list
        // to position in alphabet.

        // Calculate the distance from the start code for current bit length.
        uint Dist = BitField - Dec->DecodeLen[CurBitLength - 1];

        // Right align the distance.
        Dist >>= (16 - CurBitLength);

        // Now we can calculate the position in the code list. It is the sum
        // of first position for current bit length and right aligned distance
        // between our bit field and start code for current bit length.
        uint Pos;
        if (CurBitLength < ASIZE(Dec->DecodePos) && (Pos = Dec->DecodePos[CurBitLength] + Dist) < Size) {
            // Define the code to alphabet number translation.
            Dec->QuickNum[Code] = Dec->DecodeNum[Pos];
        } else {
            // Can be here for length table filled with zeroes only (empty).
            Dec->QuickNum[Code] = 0;
        }
    }
}

uint rar_Unpack::DecodeNumber(BitInput &Inp, DecodeTable *Dec)
{
    // Left aligned 15 bit length raw bit field.
    uint BitField = Inp.getbits() & 0xfffe;

    if (BitField < Dec->DecodeLen[Dec->QuickBits]) {
        uint Code = BitField >> (16 - Dec->QuickBits);
        Inp.addbits(Dec->QuickLen[Code]);
        return Dec->QuickNum[Code];
    }

    // Detect the real bit length for current code.
    uint Bits = 15;
    for (uint I = Dec->QuickBits + 1; I < 15; I++)
        if (BitField < Dec->DecodeLen[I]) {
            Bits = I;
            break;
        }

    Inp.addbits(Bits);

    // Calculate the distance from the start code for current bit length.
    uint Dist = BitField - Dec->DecodeLen[Bits - 1];

    // Start codes are left aligned, but we need the normal right aligned
    // number. So we shift the distance to the right.
    Dist >>= (16 - Bits);

    // Now we can calculate the position in the code list. It is the sum
    // of first position for current bit length and right aligned distance
    // between our bit field and start code for current bit length.
    uint Pos = Dec->DecodePos[Bits] + Dist;

    // Out of bounds safety check required for damaged archives.
    if (Pos >= Dec->MaxNum) Pos = 0;

    // Convert the position in the code list to position in alphabet
    // and return it.
    return Dec->DecodeNum[Pos];
}

void rar_Unpack::InsertOldDist(size_t Distance)
{
    OldDist[3] = OldDist[2];
    OldDist[2] = OldDist[1];
    OldDist[1] = OldDist[0];
    OldDist[0] = Distance;
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

void rar_Unpack::CopyString(uint Length, size_t Distance)
{
    size_t SrcPtr = UnpPtr - Distance;

    // Perform the correction here instead of "else", so matches crossing
    // the window beginning can also be processed by first "if" part.
    if (Distance > UnpPtr)  // Unlike SrcPtr>=MaxWinSize, it catches invalid distances like 0xfffffff0 in 32-bit build.
    {
        // Same as WrapDown(SrcPtr), needed because of UnpPtr-Distance above.
        // We need the same condition below, so we expanded WrapDown() here.
        SrcPtr += MaxWinSize;

        // About Distance>MaxWinSize check.
        // SrcPtr can be >=MaxWinSize if distance exceeds MaxWinSize
        // in a malformed archive. Our WrapDown replacement above might not
        // correct it, so to prevent out of bound Window read we check it here.
        // Unlike SrcPtr>=MaxWinSize check, it allows MaxWinSize>0x80000000
        // in 32-bit build, which could cause overflow in SrcPtr.
        // About !FirstWinDone check.
        // If first window hasn't filled yet and it points outside of window,
        // set data to 0 instead of copying preceding file data, so result doesn't
        // depend on previously extracted files in non-solid archive.
        if (Distance > MaxWinSize || !FirstWinDone) {
            // Fill area of specified length with 0 instead of returning.
            // So if only the distance is broken and rest of packed data is valid,
            // it preserves offsets and allows to continue extraction.
            // If we set SrcPtr to random offset instead, let's say, 0,
            // we still will be copying preceding file data if UnpPtr is also 0.
            while (Length-- > 0) {
                Window[UnpPtr] = 0;
                UnpPtr = WrapUp(UnpPtr + 1);
            }
            return;
        }
    }

    if (SrcPtr < MaxWinSize - MAX_INC_LZ_MATCH && UnpPtr < MaxWinSize - MAX_INC_LZ_MATCH) {
        // If we are not close to end of window, we do not need to waste time
        // to WrapUp and WrapDown position protection.

        quint8 *Src = Window + SrcPtr;
        quint8 *Dest = Window + UnpPtr;
        UnpPtr += Length;

        while (Length >= 8) {
            Dest[0] = Src[0];
            Dest[1] = Src[1];
            Dest[2] = Src[2];
            Dest[3] = Src[3];
            Dest[4] = Src[4];
            Dest[5] = Src[5];
            Dest[6] = Src[6];
            Dest[7] = Src[7];

            Src += 8;
            Dest += 8;
            Length -= 8;
        }

        // Unroll the loop for 0 - 7 bytes left. Note that we use nested "if"s.
        if (Length > 0) {
            Dest[0] = Src[0];
            if (Length > 1) {
                Dest[1] = Src[1];
                if (Length > 2) {
                    Dest[2] = Src[2];
                    if (Length > 3) {
                        Dest[3] = Src[3];
                        if (Length > 4) {
                            Dest[4] = Src[4];
                            if (Length > 5) {
                                Dest[5] = Src[5];
                                if (Length > 6) {
                                    Dest[6] = Src[6];
                                }
                            }
                        }
                    }
                }
            }
        }  // Close all nested "if"s.
    } else
        while (Length-- > 0)  // Slow copying with all possible precautions.
        {
            Window[UnpPtr] = Window[WrapUp(SrcPtr++)];
            // We need to have masked UnpPtr after quit from loop, so it must not
            // be replaced with 'Window[WrapUp(UnpPtr++)]'
            UnpPtr = WrapUp(UnpPtr + 1);
        }
}

uint rar_Unpack::ReadFilterData(BitInput &Inp)
{
    quint8 ByteCount = (Inp.fgetbits() >> 14) + 1;
    Inp.addbits(2);

    uint Data = 0;
    for (uint I = 0; I < ByteCount; I++) {
        Data += (Inp.fgetbits() >> 8) << (I * 8);
        Inp.addbits(8);
    }
    return Data;
}

bool rar_Unpack::ReadFilter(BitInput &Inp, UnpackFilter &Filter)
{
    if (!Inp.ExternalBuffer && Inp.InAddr > ReadTop - 16)
        if (!UnpReadBuf()) return false;

    Filter.BlockStart = ReadFilterData(Inp);
    Filter.BlockLength = ReadFilterData(Inp);
    if (Filter.BlockLength > MAX_FILTER_BLOCK_SIZE) Filter.BlockLength = 0;

    Filter.Type = Inp.fgetbits() >> 13;
    Inp.faddbits(3);

    if (Filter.Type == FILTER_DELTA) {
        Filter.Channels = (Inp.fgetbits() >> 11) + 1;
        Inp.faddbits(5);
    }

    return true;
}

bool rar_Unpack::AddFilter(UnpackFilter &Filter)
{
    if (Filters.size() >= MAX_UNPACK_FILTERS) {
        UnpWriteBuf();                                            // Write data, apply and flush filters.
        if (Filters.size() >= MAX_UNPACK_FILTERS) InitFilters();  // Still too many filters, prevent excessive memory use.
    }

    // If distance to filter start is that large that due to circular dictionary
    // mode now it points to old not written yet data, then we set 'NextWindow'
    // flag and process this filter only after processing that older data.
    Filter.NextWindow = WrPtr != UnpPtr && WrapDown(WrPtr - UnpPtr) <= Filter.BlockStart;

    // In malformed archive Filter.BlockStart can be many times larger
    // than window size, so here we must use the reminder instead of
    // subtracting the single window size as WrapUp can do. So the result
    // is always within the window. Since we add and not subtract here,
    // reminder always provides the valid result in valid archives.
    Filter.BlockStart = (Filter.BlockStart + UnpPtr) % MaxWinSize;
    Filters.push_back(Filter);
    return true;
}

void rar_Unpack::InitFilters()
{
    Filters.clear();
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

void rar_Unpack::Unpack20(bool Solid, XBinary::PDSTRUCT *pPdStruct)
{
    static unsigned char LDecode[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224};
    static unsigned char LBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
    static uint DDecode[] = {0,     1,     2,      3,      4,      6,      8,      12,     16,     24,     32,     48,     64,     96,     128,    192,
                             256,   384,   512,    768,    1024,   1536,   2048,   3072,   4096,   6144,   8192,   12288,  16384,  24576,  32768U, 49152U,
                             65536, 98304, 131072, 196608, 262144, 327680, 393216, 458752, 524288, 589824, 655360, 720896, 786432, 851968, 917504, 983040};
    static unsigned char DBits[] = {0,  0,  0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9,  10, 10,
                                    11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
    static unsigned char SDDecode[] = {0, 4, 8, 16, 32, 64, 128, 192};
    static unsigned char SDBits[] = {2, 2, 3, 4, 5, 6, 6, 6};
    uint Bits;

    if (Suspended) UnpPtr = WrPtr;
    else {
        UnpInitData(Solid);
        if (!UnpReadBuf()) return;
        if ((!Solid || !TablesRead2) && !ReadTables20()) return;
        --DestUnpSize;
    }

    while (DestUnpSize >= 0) {
        UnpPtr &= MaxWinMask;

        FirstWinDone |= (PrevPtr > UnpPtr);
        PrevPtr = UnpPtr;

        if (Inp.InAddr > ReadTop - 30)
            if (!UnpReadBuf()) break;
        if (((WrPtr - UnpPtr) & MaxWinMask) < 270 && WrPtr != UnpPtr) {
            UnpWriteBuf20();
            if (Suspended) return;
        }
        if (UnpAudioBlock) {
            uint AudioNumber = DecodeNumber(Inp, &MD[UnpCurChannel]);

            if (AudioNumber == 256) {
                if (!ReadTables20()) break;
                continue;
            }
            Window[UnpPtr++] = DecodeAudio((int)AudioNumber);
            if (++UnpCurChannel == UnpChannels) UnpCurChannel = 0;
            --DestUnpSize;
            continue;
        }

        uint Number = DecodeNumber(Inp, &BlockTables.LD);
        if (Number < 256) {
            Window[UnpPtr++] = (quint8)Number;
            --DestUnpSize;
            continue;
        }
        if (Number > 269) {
            uint Length = LDecode[Number -= 270] + 3;
            if ((Bits = LBits[Number]) > 0) {
                Length += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }

            uint DistNumber = DecodeNumber(Inp, &BlockTables.DD);
            uint Distance = DDecode[DistNumber] + 1;
            if ((Bits = DBits[DistNumber]) > 0) {
                Distance += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }

            if (Distance >= 0x2000) {
                Length++;
                if (Distance >= 0x40000L) Length++;
            }

            CopyString20(Length, Distance);
            continue;
        }
        if (Number == 269) {
            if (!ReadTables20()) break;
            continue;
        }
        if (Number == 256) {
            CopyString20(LastLength, LastDist);
            continue;
        }
        if (Number < 261) {
            uint Distance = (uint)OldDist[(OldDistPtr - (Number - 256)) & 3];
            uint LengthNumber = DecodeNumber(Inp, &BlockTables.RD);
            uint Length = LDecode[LengthNumber] + 2;
            if ((Bits = LBits[LengthNumber]) > 0) {
                Length += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }
            if (Distance >= 0x101) {
                Length++;
                if (Distance >= 0x2000) {
                    Length++;
                    if (Distance >= 0x40000) Length++;
                }
            }
            CopyString20(Length, Distance);
            continue;
        }
        if (Number < 270) {
            uint Distance = SDDecode[Number -= 261] + 1;
            if ((Bits = SDBits[Number]) > 0) {
                Distance += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }
            CopyString20(2, Distance);
            continue;
        }
    }
    ReadLastTables();
    UnpWriteBuf20();
}

void rar_Unpack::CopyString20(uint Length, uint Distance)
{
    LastDist = Distance;
    OldDist[OldDistPtr++] = Distance;
    OldDistPtr = OldDistPtr & 3;  // Needed if RAR 1.5 file is called after RAR 2.0.
    LastLength = Length;
    DestUnpSize -= Length;
    CopyString(Length, Distance);
}

bool rar_Unpack::ReadTables20()
{
    quint8 BitLength[BC20];
    quint8 Table[MC20 * 4];
    if (Inp.InAddr > ReadTop - 25)
        if (!UnpReadBuf()) return false;
    uint BitField = Inp.getbits();
    UnpAudioBlock = (BitField & 0x8000) != 0;

    if (!(BitField & 0x4000)) memset(UnpOldTable20, 0, sizeof(UnpOldTable20));
    Inp.addbits(2);

    uint TableSize;
    if (UnpAudioBlock) {
        UnpChannels = ((BitField >> 12) & 3) + 1;
        if (UnpCurChannel >= UnpChannels) UnpCurChannel = 0;
        Inp.addbits(2);
        TableSize = MC20 * UnpChannels;
    } else TableSize = NC20 + DC20 + RC20;

    for (uint I = 0; I < BC20; I++) {
        BitLength[I] = (quint8)(Inp.getbits() >> 12);
        Inp.addbits(4);
    }
    MakeDecodeTables(BitLength, &BlockTables.BD, BC20);
    for (uint I = 0; I < TableSize;) {
        if (Inp.InAddr > ReadTop - 5)
            if (!UnpReadBuf()) return false;
        uint Number = DecodeNumber(Inp, &BlockTables.BD);
        if (Number < 16) {
            Table[I] = (Number + UnpOldTable20[I]) & 0xf;
            I++;
        } else if (Number == 16) {
            uint N = (Inp.getbits() >> 14) + 3;
            Inp.addbits(2);
            if (I == 0) return false;  // We cannot have "repeat previous" code at the first position.
            else
                while (N-- > 0 && I < TableSize) {
                    Table[I] = Table[I - 1];
                    I++;
                }
        } else {
            uint N;
            if (Number == 17) {
                N = (Inp.getbits() >> 13) + 3;
                Inp.addbits(3);
            } else {
                N = (Inp.getbits() >> 9) + 11;
                Inp.addbits(7);
            }
            while (N-- > 0 && I < TableSize) Table[I++] = 0;
        }
    }
    TablesRead2 = true;
    if (Inp.InAddr > ReadTop) return true;
    if (UnpAudioBlock)
        for (uint I = 0; I < UnpChannels; I++) MakeDecodeTables(&Table[I * MC20], &MD[I], MC20);
    else {
        MakeDecodeTables(&Table[0], &BlockTables.LD, NC20);
        MakeDecodeTables(&Table[NC20], &BlockTables.DD, DC20);
        MakeDecodeTables(&Table[NC20 + DC20], &BlockTables.RD, RC20);
    }
    memcpy(UnpOldTable20, Table, TableSize);
    return true;
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

void rar_Unpack::ReadLastTables()
{
    if (ReadTop >= Inp.InAddr + 5)
        if (UnpAudioBlock) {
            if (DecodeNumber(Inp, &MD[UnpCurChannel]) == 256) ReadTables20();
        } else if (DecodeNumber(Inp, &BlockTables.LD) == 269) ReadTables20();
}

quint8 rar_Unpack::DecodeAudio(int Delta)
{
    struct AudioVariables *V = &AudV[UnpCurChannel];
    V->ByteCount++;
    V->D4 = V->D3;
    V->D3 = V->D2;
    V->D2 = V->LastDelta - V->D1;
    V->D1 = V->LastDelta;
    int PCh = 8 * V->LastChar + V->K1 * V->D1 + V->K2 * V->D2 + V->K3 * V->D3 + V->K4 * V->D4 + V->K5 * UnpChannelDelta;
    PCh = (PCh >> 3) & 0xFF;

    uint Ch = PCh - Delta;

    int D = (signed char)Delta;
    // Left shift of negative value is undefined behavior in C++,
    // so we cast it to unsigned to follow the standard.
    D = (uint)D << 3;

    V->Dif[0] += abs(D);
    V->Dif[1] += abs(D - V->D1);
    V->Dif[2] += abs(D + V->D1);
    V->Dif[3] += abs(D - V->D2);
    V->Dif[4] += abs(D + V->D2);
    V->Dif[5] += abs(D - V->D3);
    V->Dif[6] += abs(D + V->D3);
    V->Dif[7] += abs(D - V->D4);
    V->Dif[8] += abs(D + V->D4);
    V->Dif[9] += abs(D - UnpChannelDelta);
    V->Dif[10] += abs(D + UnpChannelDelta);

    UnpChannelDelta = V->LastDelta = (signed char)(Ch - V->LastChar);
    V->LastChar = Ch;

    if ((V->ByteCount & 0x1F) == 0) {
        uint MinDif = V->Dif[0], NumMinDif = 0;
        V->Dif[0] = 0;
        for (uint I = 1; I < ASIZE(V->Dif); I++) {
            if (V->Dif[I] < MinDif) {
                MinDif = V->Dif[I];
                NumMinDif = I;
            }
            V->Dif[I] = 0;
        }
        switch (NumMinDif) {
            case 1:
                if (V->K1 >= -16) V->K1--;
                break;
            case 2:
                if (V->K1 < 16) V->K1++;
                break;
            case 3:
                if (V->K2 >= -16) V->K2--;
                break;
            case 4:
                if (V->K2 < 16) V->K2++;
                break;
            case 5:
                if (V->K3 >= -16) V->K3--;
                break;
            case 6:
                if (V->K3 < 16) V->K3++;
                break;
            case 7:
                if (V->K4 >= -16) V->K4--;
                break;
            case 8:
                if (V->K4 < 16) V->K4++;
                break;
            case 9:
                if (V->K5 >= -16) V->K5--;
                break;
            case 10:
                if (V->K5 < 16) V->K5++;
                break;
        }
    }
    return (quint8)Ch;
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

void rar_Unpack::Unpack29(bool Solid, XBinary::PDSTRUCT *pPdStruct)
{
    static unsigned char LDecode[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224};
    static unsigned char LBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
    static int DDecode[DC30];
    static quint8 DBits[DC30];
    static int DBitLengthCounts[] = {4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 14, 0, 12};
    static unsigned char SDDecode[] = {0, 4, 8, 16, 32, 64, 128, 192};
    static unsigned char SDBits[] = {2, 2, 3, 4, 5, 6, 6, 6};
    unsigned int Bits;

    if (DDecode[1] == 0) {
        int Dist = 0, BitLength = 0, Slot = 0;
        for (int I = 0; I < ASIZE(DBitLengthCounts); I++, BitLength++)
            for (int J = 0; J < DBitLengthCounts[I]; J++, Slot++, Dist += (1 << BitLength)) {
                DDecode[Slot] = Dist;
                DBits[Slot] = BitLength;
            }
    }

    FileExtracted = true;

    if (!Suspended) {
        UnpInitData(Solid);
        if (!UnpReadBuf30()) return;
        if ((!Solid || !TablesRead3) && !ReadTables30()) return;
    }

    while (true) {
        UnpPtr &= MaxWinMask;

        FirstWinDone |= (PrevPtr > UnpPtr);
        PrevPtr = UnpPtr;

        if (Inp.InAddr > ReadBorder) {
            if (!UnpReadBuf30()) break;
        }
        if (((WrPtr - UnpPtr) & MaxWinMask) <= MAX3_INC_LZ_MATCH && WrPtr != UnpPtr) {
            UnpWriteBuf30();
            if (WrittenFileSize > DestUnpSize) return;
            if (Suspended) {
                FileExtracted = false;
                return;
            }
        }
        if (UnpBlockType == BLOCK_PPM) {
            // Here speed is critical, so we do not use SafePPMDecodeChar,
            // because sometimes even the inline function can introduce
            // some additional penalty.
            int Ch = PPM.DecodeChar();
            if (Ch == -1)  // Corrupt PPM data found.
            {
                PPM.CleanUp();            // Reset possibly corrupt PPM data structures.
                UnpBlockType = BLOCK_LZ;  // Set faster and more fail proof LZ mode.
                break;
            }
            if (Ch == PPMEscChar) {
                int NextCh = SafePPMDecodeChar();
                if (NextCh == 0)  // End of PPM encoding.
                {
                    if (!ReadTables30()) break;
                    continue;
                }
                if (NextCh == -1)  // Corrupt PPM data found.
                    break;
                if (NextCh == 2)  // End of file in PPM mode.
                    break;
                if (NextCh == 3)  // Read VM code.
                {
                    if (!ReadVMCodePPM()) break;
                    continue;
                }
                if (NextCh == 4)  // LZ inside of PPM.
                {
                    unsigned int Distance = 0, Length;
                    bool Failed = false;
                    for (int I = 0; I < 4 && !Failed; I++) {
                        int Ch = SafePPMDecodeChar();
                        if (Ch == -1) Failed = true;
                        else if (I == 3) Length = (quint8)Ch;
                        else Distance = (Distance << 8) + (quint8)Ch;
                    }
                    if (Failed) break;

                    CopyString(Length + 32, Distance + 2);
                    continue;
                }
                if (NextCh == 5)  // One byte distance match (RLE) inside of PPM.
                {
                    int Length = SafePPMDecodeChar();
                    if (Length == -1) break;
                    CopyString(Length + 4, 1);
                    continue;
                }
                // If we are here, NextCh must be 1, what means that current byte
                // is equal to our 'escape' byte, so we just store it to Window.
            }
            Window[UnpPtr++] = Ch;
            continue;
        }

        uint Number = DecodeNumber(Inp, &BlockTables.LD);
        if (Number < 256) {
            Window[UnpPtr++] = (quint8)Number;
            continue;
        }
        if (Number >= 271) {
            uint Length = LDecode[Number -= 271] + 3;
            if ((Bits = LBits[Number]) > 0) {
                Length += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }

            uint DistNumber = DecodeNumber(Inp, &BlockTables.DD);
            uint Distance = DDecode[DistNumber] + 1;
            if ((Bits = DBits[DistNumber]) > 0) {
                if (DistNumber > 9) {
                    if (Bits > 4) {
                        Distance += ((Inp.getbits() >> (20 - Bits)) << 4);
                        Inp.addbits(Bits - 4);
                    }
                    if (LowDistRepCount > 0) {
                        LowDistRepCount--;
                        Distance += PrevLowDist;
                    } else {
                        uint LowDist = DecodeNumber(Inp, &BlockTables.LDD);
                        if (LowDist == 16) {
                            LowDistRepCount = LOW_DIST_REP_COUNT - 1;
                            Distance += PrevLowDist;
                        } else {
                            Distance += LowDist;
                            PrevLowDist = LowDist;
                        }
                    }
                } else {
                    Distance += Inp.getbits() >> (16 - Bits);
                    Inp.addbits(Bits);
                }
            }

            if (Distance >= 0x2000) {
                Length++;
                if (Distance >= 0x40000) Length++;
            }

            InsertOldDist(Distance);
            LastLength = Length;
            CopyString(Length, Distance);
            continue;
        }
        if (Number == 256) {
            if (!ReadEndOfBlock()) break;
            continue;
        }
        if (Number == 257) {
            if (!ReadVMCode()) break;
            continue;
        }
        if (Number == 258) {
            if (LastLength != 0) CopyString(LastLength, OldDist[0]);
            continue;
        }
        if (Number < 263) {
            uint DistNum = Number - 259;
            uint Distance = (uint)OldDist[DistNum];
            for (uint I = DistNum; I > 0; I--) OldDist[I] = OldDist[I - 1];
            OldDist[0] = Distance;

            uint LengthNumber = DecodeNumber(Inp, &BlockTables.RD);
            int Length = LDecode[LengthNumber] + 2;
            if ((Bits = LBits[LengthNumber]) > 0) {
                Length += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }
            LastLength = Length;
            CopyString(Length, Distance);
            continue;
        }
        if (Number < 272) {
            uint Distance = SDDecode[Number -= 263] + 1;
            if ((Bits = SDBits[Number]) > 0) {
                Distance += Inp.getbits() >> (16 - Bits);
                Inp.addbits(Bits);
            }
            InsertOldDist(Distance);
            LastLength = 2;
            CopyString(2, Distance);
            continue;
        }
    }
    UnpWriteBuf30();
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

bool rar_Unpack::ReadEndOfBlock()
{
    uint BitField = Inp.getbits();
    bool NewTable, NewFile = false;

    // "1"  - no new file, new table just here.
    // "00" - new file,    no new table.
    // "01" - new file,    new table (in beginning of next file).

    if ((BitField & 0x8000) != 0) {
        NewTable = true;
        Inp.addbits(1);
    } else {
        NewFile = true;
        NewTable = (BitField & 0x4000) != 0;
        Inp.addbits(2);
    }
    TablesRead3 = !NewTable;

    // Quit immediately if "new file" flag is set. If "new table" flag
    // is present, we'll read the table in beginning of next file
    // based on 'TablesRead3' 'false' value.
    if (NewFile) return false;
    return ReadTables30();  // Quit only if we failed to read tables.
}

bool rar_Unpack::ReadVMCode()
{
    // Entire VM code is guaranteed to fully present in block defined
    // by current Huffman table. Compressor checks that VM code does not cross
    // Huffman block boundaries.
    uint FirstByte = Inp.getbits() >> 8;
    Inp.addbits(8);
    uint Length = (FirstByte & 7) + 1;
    if (Length == 7) {
        Length = (Inp.getbits() >> 8) + 7;
        Inp.addbits(8);
    } else if (Length == 8) {
        Length = Inp.getbits();
        Inp.addbits(16);
    }
    if (Length == 0) return false;
    std::vector<quint8> VMCode(Length);
    for (uint I = 0; I < Length; I++) {
        // Try to read the new buffer if only one byte is left.
        // But if we read all bytes except the last, one byte is enough.
        if (Inp.InAddr >= ReadTop - 1 && !UnpReadBuf30() && I < Length - 1) return false;
        VMCode[I] = Inp.getbits() >> 8;
        Inp.addbits(8);
    }
    return AddVMCode(FirstByte, VMCode.data(), Length);
}

bool rar_Unpack::ReadVMCodePPM()
{
    uint FirstByte = SafePPMDecodeChar();
    if ((int)FirstByte == -1) return false;
    uint Length = (FirstByte & 7) + 1;
    if (Length == 7) {
        int B1 = SafePPMDecodeChar();
        if (B1 == -1) return false;
        Length = B1 + 7;
    } else if (Length == 8) {
        int B1 = SafePPMDecodeChar();
        if (B1 == -1) return false;
        int B2 = SafePPMDecodeChar();
        if (B2 == -1) return false;
        Length = B1 * 256 + B2;
    }
    if (Length == 0) return false;
    std::vector<quint8> VMCode(Length);
    for (uint I = 0; I < Length; I++) {
        int Ch = SafePPMDecodeChar();
        if (Ch == -1) return false;
        VMCode[I] = Ch;
    }
    return AddVMCode(FirstByte, VMCode.data(), Length);
}

bool rar_Unpack::AddVMCode(uint FirstByte, quint8 *Code, uint CodeSize)
{
    VMCodeInp.InitBitInput();
    memcpy(VMCodeInp.InBuf, Code, qMin((uint)BitInput::MAX_SIZE, CodeSize));
    VM.Init();

    uint FiltPos;
    if ((FirstByte & 0x80) != 0) {
        FiltPos = RarVM::ReadData(VMCodeInp);
        if (FiltPos == 0) InitFilters30(false);
        else FiltPos--;
    } else FiltPos = LastFilter;  // Use the same filter as last time.

    if (FiltPos > Filters30.size() || FiltPos > OldFilterLengths.size()) return false;
    LastFilter = FiltPos;
    bool NewFilter = (FiltPos == Filters30.size());

    UnpackFilter30 *StackFilter = new UnpackFilter30;  // New filter for PrgStack.

    UnpackFilter30 *Filter;
    if (NewFilter)  // New filter code, never used before since VM reset.
    {
        if (FiltPos > MAX3_UNPACK_FILTERS) {
            // Too many different filters, corrupt archive.
            delete StackFilter;
            return false;
        }

        StackFilter->ParentFilter = (uint)Filters30.size();
        Filter = new UnpackFilter30;
        Filters30.push_back(Filter);

        // Reserve one item to store the data block length of our new filter
        // entry. We'll set it to real block length below, after reading it.
        // But we need to initialize it now, because when processing corrupt
        // data, we can access this item even before we set it to real value.
        OldFilterLengths.push_back(0);
    } else  // Filter was used in the past.
    {
        Filter = Filters30[FiltPos];
        StackFilter->ParentFilter = FiltPos;
    }

    uint EmptyCount = 0;
    for (uint I = 0; I < PrgStack.size(); I++) {
        PrgStack[I - EmptyCount] = PrgStack[I];
        if (PrgStack[I] == NULL) EmptyCount++;
        if (EmptyCount > 0) PrgStack[I] = NULL;
    }
    if (EmptyCount == 0) {
        if (PrgStack.size() > MAX3_UNPACK_FILTERS) {
            delete StackFilter;
            return false;
        }
        PrgStack.resize(PrgStack.size() + 1);
        EmptyCount = 1;
    }
    size_t StackPos = PrgStack.size() - EmptyCount;
    PrgStack[StackPos] = StackFilter;

    uint BlockStart = RarVM::ReadData(VMCodeInp);
    if ((FirstByte & 0x40) != 0) BlockStart += 258;
    StackFilter->BlockStart = (uint)((BlockStart + UnpPtr) & MaxWinMask);
    if ((FirstByte & 0x20) != 0) {
        StackFilter->BlockLength = RarVM::ReadData(VMCodeInp);

        // Store the last data block length for current filter.
        OldFilterLengths[FiltPos] = StackFilter->BlockLength;
    } else {
        // Set the data block size to same value as the previous block size
        // for same filter. It is possible for corrupt data to access a new
        // and not filled yet item of OldFilterLengths array here. This is why
        // we set new OldFilterLengths items to zero above.
        StackFilter->BlockLength = FiltPos < OldFilterLengths.size() ? OldFilterLengths[FiltPos] : 0;
    }

    StackFilter->NextWindow = WrPtr != UnpPtr && ((WrPtr - UnpPtr) & MaxWinMask) <= BlockStart;

    //  DebugLog("\nNextWindow: UnpPtr=%08x WrPtr=%08x BlockStart=%08x",UnpPtr,WrPtr,BlockStart);

    memset(StackFilter->Prg.InitR, 0, sizeof(StackFilter->Prg.InitR));
    StackFilter->Prg.InitR[4] = StackFilter->BlockLength;

    if ((FirstByte & 0x10) != 0)  // Set registers to optional parameters if any.
    {
        uint InitMask = VMCodeInp.fgetbits() >> 9;
        VMCodeInp.faddbits(7);
        for (uint I = 0; I < 7; I++)
            if (InitMask & (1 << I)) StackFilter->Prg.InitR[I] = RarVM::ReadData(VMCodeInp);
    }

    if (NewFilter) {
        uint VMCodeSize = RarVM::ReadData(VMCodeInp);
        if (VMCodeSize >= 0x10000 || VMCodeSize == 0 || VMCodeInp.InAddr + VMCodeSize > CodeSize) return false;
        std::vector<quint8> VMCode(VMCodeSize);
        for (uint I = 0; I < VMCodeSize; I++) {
            if (VMCodeInp.Overflow(3)) return false;
            VMCode[I] = VMCodeInp.fgetbits() >> 8;
            VMCodeInp.faddbits(8);
        }
        VM.Prepare(VMCode.data(), VMCodeSize, &Filter->Prg);
    }
    StackFilter->Prg.Type = Filter->Prg.Type;

    return true;
}

int rar_Unpack::SafePPMDecodeChar()
{
    int Ch = PPM.DecodeChar();
    if (Ch == -1)  // Corrupt PPM data found.
    {
        PPM.CleanUp();            // Reset possibly corrupt PPM data structures.
        UnpBlockType = BLOCK_LZ;  // Set faster and more fail proof LZ mode.
    }
    return (Ch);
}

bool rar_Unpack::ReadTables30()
{
    quint8 BitLength[BC];
    quint8 Table[HUFF_TABLE_SIZE30];
    if (Inp.InAddr > ReadTop - 25)
        if (!UnpReadBuf30()) return (false);
    Inp.faddbits((8 - Inp.InBit) & 7);
    uint BitField = Inp.fgetbits();
    if (BitField & 0x8000) {
        UnpBlockType = BLOCK_PPM;
        return (PPM.DecodeInit(this, PPMEscChar));
    }
    UnpBlockType = BLOCK_LZ;

    PrevLowDist = 0;
    LowDistRepCount = 0;

    if (!(BitField & 0x4000)) memset(UnpOldTable, 0, sizeof(UnpOldTable));
    Inp.faddbits(2);

    for (uint I = 0; I < BC; I++) {
        uint Length = (quint8)(Inp.fgetbits() >> 12);
        Inp.faddbits(4);
        if (Length == 15) {
            uint ZeroCount = (quint8)(Inp.fgetbits() >> 12);
            Inp.faddbits(4);
            if (ZeroCount == 0) BitLength[I] = 15;
            else {
                ZeroCount += 2;
                while (ZeroCount-- > 0 && I < ASIZE(BitLength)) BitLength[I++] = 0;
                I--;
            }
        } else BitLength[I] = Length;
    }
    MakeDecodeTables(BitLength, &BlockTables.BD, BC30);

    const uint TableSize = HUFF_TABLE_SIZE30;
    for (uint I = 0; I < TableSize;) {
        if (Inp.InAddr > ReadTop - 5)
            if (!UnpReadBuf30()) return (false);
        uint Number = DecodeNumber(Inp, &BlockTables.BD);
        if (Number < 16) {
            Table[I] = (Number + UnpOldTable[I]) & 0xf;
            I++;
        } else if (Number < 18) {
            uint N;
            if (Number == 16) {
                N = (Inp.fgetbits() >> 13) + 3;
                Inp.faddbits(3);
            } else {
                N = (Inp.fgetbits() >> 9) + 11;
                Inp.faddbits(7);
            }
            if (I == 0) return false;  // We cannot have "repeat previous" code at the first position.
            else
                while (N-- > 0 && I < TableSize) {
                    Table[I] = Table[I - 1];
                    I++;
                }
        } else {
            uint N;
            if (Number == 18) {
                N = (Inp.fgetbits() >> 13) + 3;
                Inp.faddbits(3);
            } else {
                N = (Inp.fgetbits() >> 9) + 11;
                Inp.faddbits(7);
            }
            while (N-- > 0 && I < TableSize) Table[I++] = 0;
        }
    }
    TablesRead3 = true;
    if (Inp.InAddr > ReadTop) return false;
    MakeDecodeTables(&Table[0], &BlockTables.LD, NC30);
    MakeDecodeTables(&Table[NC30], &BlockTables.DD, DC30);
    MakeDecodeTables(&Table[NC30 + DC30], &BlockTables.LDD, LDC30);
    MakeDecodeTables(&Table[NC30 + DC30 + LDC30], &BlockTables.RD, RC30);
    memcpy(UnpOldTable, Table, sizeof(UnpOldTable));
    return true;
}

bool rar_Unpack::UnpReadBuf30()
{
    int DataSize = ReadTop - Inp.InAddr;  // Data left to process.
    if (DataSize < 0) return false;
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
    // int ReadCode=UnpIO->UnpRead(Inp.InBuf+DataSize,BitInput::MAX_SIZE-DataSize);
    int ReadCode = g_pDeviceInput->read((char *)(Inp.InBuf + DataSize), BitInput::MAX_SIZE - DataSize);
    if (ReadCode > 0) ReadTop += ReadCode;
    ReadBorder = ReadTop - 30;
    return ReadCode != -1;
}

void rar_Unpack::UnpWriteBuf30()
{
    uint WrittenBorder = (uint)WrPtr;
    uint WriteSize = (uint)((UnpPtr - WrittenBorder) & MaxWinMask);
    for (size_t I = 0; I < PrgStack.size(); I++) {
        // Here we apply filters to data which we need to write.
        // We always copy data to virtual machine memory before processing.
        // We cannot process them just in place in Window buffer, because
        // these data can be used for future string matches, so we must
        // preserve them in original form.

        UnpackFilter30 *flt = PrgStack[I];
        if (flt == NULL) continue;
        if (flt->NextWindow) {
            flt->NextWindow = false;
            continue;
        }
        unsigned int BlockStart = flt->BlockStart;
        unsigned int BlockLength = flt->BlockLength;
        if (((BlockStart - WrittenBorder) & MaxWinMask) < WriteSize) {
            if (WrittenBorder != BlockStart) {
                UnpWriteArea(WrittenBorder, BlockStart);
                WrittenBorder = BlockStart;
                WriteSize = (uint)((UnpPtr - WrittenBorder) & MaxWinMask);
            }
            if (BlockLength <= WriteSize) {
                uint BlockEnd = (BlockStart + BlockLength) & MaxWinMask;
                if (BlockStart < BlockEnd || BlockEnd == 0) VM.SetMemory(0, Window + BlockStart, BlockLength);
                else {
                    uint FirstPartLength = uint(MaxWinSize - BlockStart);
                    VM.SetMemory(0, Window + BlockStart, FirstPartLength);
                    VM.SetMemory(FirstPartLength, Window, BlockEnd);
                }

                VM_PreparedProgram *ParentPrg = &Filters30[flt->ParentFilter]->Prg;
                VM_PreparedProgram *Prg = &flt->Prg;

                ExecuteCode(Prg);

                quint8 *FilteredData = Prg->FilteredData;
                unsigned int FilteredDataSize = Prg->FilteredDataSize;

                delete PrgStack[I];
                PrgStack[I] = nullptr;
                while (I + 1 < PrgStack.size()) {
                    UnpackFilter30 *NextFilter = PrgStack[I + 1];
                    // It is required to check NextWindow here.
                    if (NextFilter == NULL || NextFilter->BlockStart != BlockStart || NextFilter->BlockLength != FilteredDataSize || NextFilter->NextWindow) break;

                    // Apply several filters to same data block.

                    VM.SetMemory(0, FilteredData, FilteredDataSize);

                    VM_PreparedProgram *ParentPrg = &Filters30[NextFilter->ParentFilter]->Prg;
                    VM_PreparedProgram *NextPrg = &NextFilter->Prg;

                    ExecuteCode(NextPrg);

                    FilteredData = NextPrg->FilteredData;
                    FilteredDataSize = NextPrg->FilteredDataSize;
                    I++;
                    delete PrgStack[I];
                    PrgStack[I] = nullptr;
                }
                // UnpIO->UnpWrite(FilteredData,FilteredDataSize);
                g_pDeviceOutput->write((char *)FilteredData, FilteredDataSize);
                UnpSomeRead = true;
                WrittenFileSize += FilteredDataSize;
                WrittenBorder = BlockEnd;
                WriteSize = uint((UnpPtr - WrittenBorder) & MaxWinMask);
            } else {
                // Current filter intersects the window write border, so we adjust
                // the window border to process this filter next time, not now.
                for (size_t J = I; J < PrgStack.size(); J++) {
                    UnpackFilter30 *flt = PrgStack[J];
                    if (flt != nullptr && flt->NextWindow) flt->NextWindow = false;
                }
                WrPtr = WrittenBorder;
                return;
            }
        }
    }

    UnpWriteArea(WrittenBorder, UnpPtr);
    WrPtr = UnpPtr;
}

void rar_Unpack::ExecuteCode(VM_PreparedProgram *Prg)
{
    Prg->InitR[6] = (uint)WrittenFileSize;
    VM.Execute(Prg);
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

qint32 rar_Unpack::Init(quint64 WinSize, bool Solid)
{
    // Minimum window size must be at least twice more than maximum possible
    // size of filter block, which is 0x10000 in RAR now. If window size is
    // smaller, we can have a block with never cleared flt->NextWindow flag
    // in UnpWriteBuf(). Minimum window size 0x20000 would be enough, but let's
    // use 0x40000 for extra safety and possible filter area size expansion.
    const size_t MinAllocSize = 0x40000;
    if (WinSize < MinAllocSize) WinSize = MinAllocSize;

    if (WinSize > qMin(0x10000000000ULL, UNPACK_MAX_DICT))  // Window size must not exceed 1 TB.
        return -1;                                          // Too big window size, we cannot allocate it.;

    // 32-bit build can't unpack dictionaries exceeding 32-bit even in theory.
    // Also we've not verified if WrapUp and WrapDown work properly in 32-bit
    // version and >2GB dictionary and if 32-bit version can handle >2GB
    // distances. Since such version is unlikely to allocate >2GB anyway,
    // we prohibit >2GB dictionaries for 32-bit build here.
    if (WinSize > 0x80000000 && sizeof(size_t) <= 4) return -1;  // Too big window size for 32-bit build.

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
    if (WinSize <= AllocWinSize) return 1;

    // Archiving code guarantees that window size does not grow in the same
    // solid stream. So if we are here, we are either creating a new window
    // or increasing the size of non-solid window. So we could safely reject
    // current window data without copying them to a new window.
    if (Solid && (Window != NULL || Fragmented && WinSize > FragWindow.GetWinSize())) return -1;

    Alloc.delete_l<quint8>(Window);  // delete Window;
    Window = nullptr;

    try {
        if (!Fragmented) Window = Alloc.new_l<quint8>((size_t)WinSize, false);  // Window=new byte[(size_t)WinSize];
    } catch (std::bad_alloc)                                                    // Use the fragmented window in this case.
    {
    }

    if (Window == nullptr)
        if (WinSize < 0x1000000 || sizeof(size_t) > 4) return -1;  // Exclude RAR4, small dictionaries and 64-bit.
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

    return 1;
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

void SubAllocator::InsertNode(void *p, int indx)
{
    ((RAR_NODE *)p)->next = FreeList[indx].next;
    FreeList[indx].next = (RAR_NODE *)p;
}

void *SubAllocator::RemoveNode(int indx)
{
    RAR_NODE *RetVal = FreeList[indx].next;
    FreeList[indx].next = RetVal->next;
    return RetVal;
}

uint SubAllocator::U2B(int NU)
{
    // We calculate the size of units in bytes based on real UNIT_SIZE.
    // In original implementation it was 8*NU+4*NU.
    return UNIT_SIZE * NU;
}

void SubAllocator::SplitBlock(void *pv, int OldIndx, int NewIndx)
{
    int i, UDiff = Indx2Units[OldIndx] - Indx2Units[NewIndx];
    quint8 *p = ((quint8 *)pv) + U2B(Indx2Units[NewIndx]);
    if (Indx2Units[i = Units2Indx[UDiff - 1]] != UDiff) {
        InsertNode(p, --i);
        p += U2B(i = Indx2Units[i]);
        UDiff -= i;
    }
    InsertNode(p, Units2Indx[UDiff - 1]);
}

void SubAllocator::GlueFreeBlocks()
{
    RARPPM_MEM_BLK s0, *p, *p1;
    int i, k, sz;
    if (LoUnit != HiUnit) *LoUnit = 0;
    for (i = 0, s0.next = s0.prev = &s0; i < N_INDEXES; i++)
        while (FreeList[i].next) {
            p = (RARPPM_MEM_BLK *)RemoveNode(i);
            p->insertAt(&s0);
            p->Stamp = 0xFFFF;
            p->NU = Indx2Units[i];
        }
    for (p = s0.next; p != &s0; p = p->next)
        while ((p1 = MBPtr(p, p->NU))->Stamp == 0xFFFF && int(p->NU) + p1->NU < 0x10000) {
            p1->remove();
            p->NU += p1->NU;
        }
    while ((p = s0.next) != &s0) {
        for (p->remove(), sz = p->NU; sz > 128; sz -= 128, p = MBPtr(p, 128)) InsertNode(p, N_INDEXES - 1);
        if (Indx2Units[i = Units2Indx[sz - 1]] != sz) {
            k = sz - Indx2Units[--i];
            InsertNode(MBPtr(p, sz - k), k - 1);
        }
        InsertNode(p, i);
    }
}

void *SubAllocator::AllocUnitsRare(int indx)
{
    if (!GlueCount) {
        GlueCount = 255;
        GlueFreeBlocks();
        if (FreeList[indx].next) return RemoveNode(indx);
    }
    int i = indx;
    do {
        if (++i == N_INDEXES) {
            GlueCount--;
            i = U2B(Indx2Units[indx]);
            int j = FIXED_UNIT_SIZE * Indx2Units[indx];
            if (FakeUnitsStart - pText > j) {
                FakeUnitsStart -= j;
                UnitsStart -= i;
                return UnitsStart;
            }
            return NULL;
        }
    } while (!FreeList[i].next);
    void *RetVal = RemoveNode(i);
    SplitBlock(RetVal, i, indx);
    return RetVal;
}

RARPPM_MEM_BLK *SubAllocator::MBPtr(RARPPM_MEM_BLK *BasePtr, int Items)
{
    return ((RARPPM_MEM_BLK *)(((quint8 *)(BasePtr)) + U2B(Items)));
}

SubAllocator::SubAllocator()
{
    Clean();
}

void SubAllocator::Clean()
{
    SubAllocatorSize = 0;
}

bool SubAllocator::StartSubAllocator(int SASize)
{
    uint t = SASize << 20;
    if (SubAllocatorSize == t) return true;
    StopSubAllocator();

    // Original algorithm expects FIXED_UNIT_SIZE, but actual structure size
    // can be larger. So let's recalculate the allocated size and add two more
    // units: one as reserve for HeapEnd overflow checks and another
    // to provide the space to correctly align UnitsStart.
    uint AllocSize = t / FIXED_UNIT_SIZE * UNIT_SIZE + 2 * UNIT_SIZE;
    if ((HeapStart = (quint8 *)malloc(AllocSize)) == NULL) {
        // ErrHandler.MemoryError(); // TODO
        return false;
    }

    // HeapEnd did not present in original algorithm. We added it to control
    // invalid memory access attempts when processing corrupt archived data.
    HeapEnd = HeapStart + AllocSize - UNIT_SIZE;

    SubAllocatorSize = t;
    return true;
}

void SubAllocator::StopSubAllocator()
{
    if (SubAllocatorSize) {
        SubAllocatorSize = 0;
        free(HeapStart);
    }
}

void SubAllocator::InitSubAllocator()
{
    int i, k;
    memset(FreeList, 0, sizeof(FreeList));
    pText = HeapStart;

    // Original algorithm operates with 12 byte FIXED_UNIT_SIZE, but actual
    // size of RARPPM_MEM_BLK and RARPPM_CONTEXT structures can exceed this value
    // because of alignment and larger pointer fields size.
    // So we define UNIT_SIZE for this larger size and adjust memory
    // pointers accordingly.

    // Size2 is (HiUnit-LoUnit) memory area size to allocate as originally
    // supposed by compression algorithm. It is 7/8 of total allocated size.
    uint Size2 = FIXED_UNIT_SIZE * (SubAllocatorSize / 8 / FIXED_UNIT_SIZE * 7);

    // RealSize2 is the real adjusted size of (HiUnit-LoUnit) memory taking
    // into account that our UNIT_SIZE can be larger than FIXED_UNIT_SIZE.
    uint RealSize2 = Size2 / FIXED_UNIT_SIZE * UNIT_SIZE;

    // Size1 is the size of memory area from HeapStart to FakeUnitsStart
    // as originally supposed by compression algorithm. This area can contain
    // different data types, both single symbols and structures.
    uint Size1 = SubAllocatorSize - Size2;

    // Real size of this area. We correct it according to UNIT_SIZE vs
    // FIXED_UNIT_SIZE difference. Also we add one more UNIT_SIZE
    // to compensate a possible reminder from Size1/FIXED_UNIT_SIZE,
    // which would be lost otherwise. We add UNIT_SIZE instead of
    // this Size1%FIXED_UNIT_SIZE reminder, because it allows to align
    // UnitsStart easily and adding more than reminder is ok for algorithm.
    uint RealSize1 = Size1 / FIXED_UNIT_SIZE * UNIT_SIZE + UNIT_SIZE;

    // RealSize1 must be divided by UNIT_SIZE without a reminder, so UnitsStart
    // is aligned to UNIT_SIZE. It is important for those architectures,
    // where a proper memory alignment is mandatory. Since we produce RealSize1
    // multiplying by UNIT_SIZE, this condition is always true. So LoUnit,
    // UnitsStart, HeapStart are properly aligned,
    LoUnit = UnitsStart = HeapStart + RealSize1;

    // When we reach FakeUnitsStart, we restart the model. It is where
    // the original algorithm expected to see UnitsStart. Real UnitsStart
    // can have a larger value.
    FakeUnitsStart = HeapStart + Size1;

    HiUnit = LoUnit + RealSize2;
    for (i = 0, k = 1; i < N1; i++, k += 1) Indx2Units[i] = k;
    for (k++; i < N1 + N2; i++, k += 2) Indx2Units[i] = k;
    for (k++; i < N1 + N2 + N3; i++, k += 3) Indx2Units[i] = k;
    for (k++; i < N1 + N2 + N3 + N4; i++, k += 4) Indx2Units[i] = k;
    for (GlueCount = k = i = 0; k < 128; k++) {
        i += (Indx2Units[i] < k + 1);
        Units2Indx[k] = i;
    }
}

void *SubAllocator::AllocContext()
{
    if (HiUnit != LoUnit) return (HiUnit -= UNIT_SIZE);
    if (FreeList->next) return RemoveNode(0);
    return AllocUnitsRare(0);
}

void *SubAllocator::AllocUnits(int NU)
{
    int indx = Units2Indx[NU - 1];
    if (FreeList[indx].next) return RemoveNode(indx);
    void *RetVal = LoUnit;
    LoUnit += U2B(Indx2Units[indx]);
    if (LoUnit <= HiUnit) return RetVal;
    LoUnit -= U2B(Indx2Units[indx]);
    return AllocUnitsRare(indx);
}

void *SubAllocator::ExpandUnits(void *OldPtr, int OldNU)
{
    int i0 = Units2Indx[OldNU - 1], i1 = Units2Indx[OldNU - 1 + 1];
    if (i0 == i1) return OldPtr;
    void *ptr = AllocUnits(OldNU + 1);
    if (ptr) {
        memcpy(ptr, OldPtr, U2B(OldNU));
        InsertNode(OldPtr, i0);
    }
    return ptr;
}

void *SubAllocator::ShrinkUnits(void *OldPtr, int OldNU, int NewNU)
{
    int i0 = Units2Indx[OldNU - 1], i1 = Units2Indx[NewNU - 1];
    if (i0 == i1) return OldPtr;
    if (FreeList[i1].next) {
        void *ptr = RemoveNode(i1);
        memcpy(ptr, OldPtr, U2B(NewNU));
        InsertNode(OldPtr, i0);
        return ptr;
    } else {
        SplitBlock(OldPtr, i0, i1);
        return OldPtr;
    }
}

void SubAllocator::FreeUnits(void *ptr, int OldNU)
{
    InsertNode(ptr, Units2Indx[OldNU - 1]);
}

bool RarVM::ExecuteStandardFilter(VM_StandardFilters FilterType)
{
    switch (FilterType) {
        case VMSF_E8:
        case VMSF_E8E9: {
            quint8 *Data = Mem;
            uint DataSize = R[4], FileOffset = R[6];

            if (DataSize > VM_MEMSIZE || DataSize < 4) return false;

            const uint FileSize = 0x1000000;
            quint8 CmpByte2 = FilterType == VMSF_E8E9 ? 0xe9 : 0xe8;
            for (uint CurPos = 0; CurPos < DataSize - 4;) {
                quint8 CurByte = *(Data++);
                CurPos++;
                if (CurByte == 0xe8 || CurByte == CmpByte2) {
                    uint Offset = CurPos + FileOffset;
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
        } break;
        case VMSF_ITANIUM: {
            quint8 *Data = Mem;
            uint DataSize = R[4], FileOffset = R[6];

            if (DataSize > VM_MEMSIZE || DataSize < 21) return false;

            uint CurPos = 0;

            FileOffset >>= 4;

            while (CurPos < DataSize - 21) {
                int Byte = (Data[0] & 0x1f) - 0x10;
                if (Byte >= 0) {
                    static quint8 Masks[16] = {4, 4, 6, 6, 0, 0, 7, 7, 4, 4, 0, 0, 4, 4, 0, 0};
                    quint8 CmdMask = Masks[Byte];
                    if (CmdMask != 0)
                        for (uint I = 0; I <= 2; I++)
                            if (CmdMask & (1 << I)) {
                                uint StartPos = I * 41 + 5;
                                uint OpType = FilterItanium_GetBits(Data, StartPos + 37, 4);
                                if (OpType == 5) {
                                    uint Offset = FilterItanium_GetBits(Data, StartPos + 13, 20);
                                    FilterItanium_SetBits(Data, (Offset - FileOffset) & 0xfffff, StartPos + 13, 20);
                                }
                            }
                }
                Data += 16;
                CurPos += 16;
                FileOffset++;
            }
        } break;
        case VMSF_DELTA: {
            uint DataSize = R[4], Channels = R[0], SrcPos = 0, Border = DataSize * 2;
            if (DataSize > VM_MEMSIZE / 2 || Channels > MAX3_UNPACK_CHANNELS || Channels == 0) return false;

            // Bytes from same channels are grouped to continual data blocks,
            // so we need to place them back to their interleaving positions.
            for (uint CurChannel = 0; CurChannel < Channels; CurChannel++) {
                quint8 PrevByte = 0;
                for (uint DestPos = DataSize + CurChannel; DestPos < Border; DestPos += Channels) Mem[DestPos] = (PrevByte -= Mem[SrcPos++]);
            }
        } break;
        case VMSF_RGB: {
            uint DataSize = R[4], Width = R[0] - 3, PosR = R[1];
            if (DataSize > VM_MEMSIZE / 2 || DataSize < 3 || Width > DataSize || PosR > 2) return false;
            quint8 *SrcData = Mem, *DestData = SrcData + DataSize;
            const uint Channels = 3;
            for (uint CurChannel = 0; CurChannel < Channels; CurChannel++) {
                uint PrevByte = 0;

                for (uint I = CurChannel; I < DataSize; I += Channels) {
                    uint Predicted;
                    if (I >= Width + 3) {
                        quint8 *UpperData = DestData + I - Width;
                        uint UpperByte = *UpperData;
                        uint UpperLeftByte = *(UpperData - 3);
                        Predicted = PrevByte + UpperByte - UpperLeftByte;
                        int pa = abs((int)(Predicted - PrevByte));
                        int pb = abs((int)(Predicted - UpperByte));
                        int pc = abs((int)(Predicted - UpperLeftByte));
                        if (pa <= pb && pa <= pc) Predicted = PrevByte;
                        else if (pb <= pc) Predicted = UpperByte;
                        else Predicted = UpperLeftByte;
                    } else Predicted = PrevByte;
                    PrevByte = DestData[I] = (quint8)(Predicted - *(SrcData++));
                }
            }
            for (uint I = PosR, Border = DataSize - 2; I < Border; I += 3) {
                quint8 G = DestData[I + 1];
                DestData[I] += G;
                DestData[I + 2] += G;
            }
        } break;
        case VMSF_AUDIO: {
            uint DataSize = R[4], Channels = R[0];
            quint8 *SrcData = Mem, *DestData = SrcData + DataSize;
            // In fact, audio channels never exceed 4.
            if (DataSize > VM_MEMSIZE / 2 || Channels > 128 || Channels == 0) return false;
            for (uint CurChannel = 0; CurChannel < Channels; CurChannel++) {
                uint PrevByte = 0, PrevDelta = 0, Dif[7];
                int D1 = 0, D2 = 0, D3;
                int K1 = 0, K2 = 0, K3 = 0;
                memset(Dif, 0, sizeof(Dif));

                for (uint I = CurChannel, ByteCount = 0; I < DataSize; I += Channels, ByteCount++) {
                    D3 = D2;
                    D2 = PrevDelta - D1;
                    D1 = PrevDelta;

                    uint Predicted = 8 * PrevByte + K1 * D1 + K2 * D2 + K3 * D3;
                    Predicted = (Predicted >> 3) & 0xff;

                    uint CurByte = *(SrcData++);

                    Predicted -= CurByte;
                    DestData[I] = Predicted;
                    PrevDelta = (signed char)(Predicted - PrevByte);
                    PrevByte = Predicted;

                    int D = (signed char)CurByte;
                    // Left shift of negative value is undefined behavior in C++,
                    // so we cast it to unsigned to follow the standard.
                    D = (uint)D << 3;

                    Dif[0] += abs(D);
                    Dif[1] += abs(D - D1);
                    Dif[2] += abs(D + D1);
                    Dif[3] += abs(D - D2);
                    Dif[4] += abs(D + D2);
                    Dif[5] += abs(D - D3);
                    Dif[6] += abs(D + D3);

                    if ((ByteCount & 0x1f) == 0) {
                        uint MinDif = Dif[0], NumMinDif = 0;
                        Dif[0] = 0;
                        for (uint J = 1; J < ASIZE(Dif); J++) {
                            if (Dif[J] < MinDif) {
                                MinDif = Dif[J];
                                NumMinDif = J;
                            }
                            Dif[J] = 0;
                        }
                        switch (NumMinDif) {
                            case 1:
                                if (K1 >= -16) K1--;
                                break;
                            case 2:
                                if (K1 < 16) K1++;
                                break;
                            case 3:
                                if (K2 >= -16) K2--;
                                break;
                            case 4:
                                if (K2 < 16) K2++;
                                break;
                            case 5:
                                if (K3 >= -16) K3--;
                                break;
                            case 6:
                                if (K3 < 16) K3++;
                                break;
                        }
                    }
                }
            }
        } break;
    }
    return true;
}

uint RarVM::FilterItanium_GetBits(quint8 *Data, uint BitPos, uint BitCount)
{
    uint InAddr = BitPos / 8;
    uint InBit = BitPos & 7;
    uint BitField = (uint)Data[InAddr++];
    BitField |= (uint)Data[InAddr++] << 8;
    BitField |= (uint)Data[InAddr++] << 16;
    BitField |= (uint)Data[InAddr] << 24;
    BitField >>= InBit;
    return BitField & (0xffffffff >> (32 - BitCount));
}

void RarVM::FilterItanium_SetBits(quint8 *Data, uint BitField, uint BitPos, uint BitCount)
{
    uint InAddr = BitPos / 8;
    uint InBit = BitPos & 7;
    uint AndMask = 0xffffffff >> (32 - BitCount);
    AndMask = ~(AndMask << InBit);

    BitField <<= InBit;

    for (uint I = 0; I < 4; I++) {
        Data[InAddr + I] &= AndMask;
        Data[InAddr + I] |= BitField;
        AndMask = (AndMask >> 8) | 0xff000000;
        BitField >>= 8;
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

void RarVM::Init()
{
    if (Mem == NULL) Mem = new quint8[VM_MEMSIZE + 4];
}

void RarVM::Prepare(quint8 *Code, uint CodeSize, VM_PreparedProgram *Prg)
{
    // Calculate the single byte XOR checksum to check validity of VM code.
    quint8 XorSum = 0;
    for (uint I = 1; I < CodeSize; I++) XorSum ^= Code[I];

    if (XorSum != Code[0]) return;

    struct StandardFilters {
        uint Length;
        uint CRC;
        VM_StandardFilters Type;
    } static StdList[] = {53, 0xad576887, VMSF_E8,    57,  0x3cd7e57e, VMSF_E8E9, 120, 0x3769893f, VMSF_ITANIUM,
                          29, 0x0e06077d, VMSF_DELTA, 149, 0x1c2c5dc8, VMSF_RGB,  216, 0xbc85e701, VMSF_AUDIO};
    // uint CodeCRC=CRC32(0xffffffff,Code,CodeSize)^0xffffffff;
    quint32 CodeCRC = XBinary::_getCRC32((char *)Code, (qint32)CodeSize, (quint32)0xffffffff, XBinary::_getCRC32Table_EDB88320()) ^ 0xffffffff;
    for (uint I = 0; I < ASIZE(StdList); I++)
        if (StdList[I].CRC == CodeCRC && StdList[I].Length == CodeSize) {
            Prg->Type = StdList[I].Type;
            break;
        }
}

void RarVM::Execute(VM_PreparedProgram *Prg)
{
    memcpy(R, Prg->InitR, sizeof(Prg->InitR));
    Prg->FilteredData = NULL;
    if (Prg->Type != VMSF_NONE) {
        bool Success = ExecuteStandardFilter(Prg->Type);
        uint BlockSize = Prg->InitR[4] & VM_MEMMASK;
        Prg->FilteredDataSize = BlockSize;
        if (Prg->Type == VMSF_DELTA || Prg->Type == VMSF_RGB || Prg->Type == VMSF_AUDIO)
            Prg->FilteredData = 2 * BlockSize > VM_MEMSIZE || !Success ? Mem : Mem + BlockSize;
        else Prg->FilteredData = Mem;
    }
}

void RarVM::SetMemory(size_t Pos, quint8 *Data, size_t DataSize)
{
    if (Pos < VM_MEMSIZE && Data != Mem + Pos) {
        // We can have NULL Data for invalid filters with DataSize==0. While most
        // sensible memmove implementations do not care about data if size is 0,
        // let's follow the standard and check the size first.
        size_t CopySize = qMin(DataSize, (size_t)(VM_MEMSIZE - Pos));
        if (CopySize != 0) memmove(Mem + Pos, Data, CopySize);
    }
}

uint RarVM::ReadData(BitInput &Inp)
{
    uint Data = Inp.fgetbits();
    switch (Data & 0xc000) {
        case 0: Inp.faddbits(6); return (Data >> 10) & 0xf;
        case 0x4000:
            if ((Data & 0x3c00) == 0) {
                Data = 0xffffff00 | ((Data >> 2) & 0xff);
                Inp.faddbits(14);
            } else {
                Data = (Data >> 6) & 0xff;
                Inp.faddbits(10);
            }
            return Data;
        case 0x8000:
            Inp.faddbits(2);
            Data = Inp.fgetbits();
            Inp.faddbits(16);
            return Data;
        default:
            Inp.faddbits(2);
            Data = (Inp.fgetbits() << 16);
            Inp.faddbits(16);
            Data |= Inp.fgetbits();
            Inp.faddbits(16);
            return Data;
    }
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

void FragmentedWindow::CopyString(uint Length, size_t Distance, size_t &UnpPtr, bool FirstWinDone, size_t MaxWinSize)
{
    size_t SrcPtr = UnpPtr - Distance;
    if (Distance > UnpPtr) {
        SrcPtr += MaxWinSize;

        if (Distance > MaxWinSize || !FirstWinDone) {
            while (Length-- > 0) {
                (*this)[UnpPtr] = 0;
                if (++UnpPtr >= MaxWinSize) UnpPtr -= MaxWinSize;
            }
            return;
        }
    }

    while (Length-- > 0) {
        (*this)[UnpPtr] = (*this)[SrcPtr];
        if (++SrcPtr >= MaxWinSize) SrcPtr -= MaxWinSize;
        if (++UnpPtr >= MaxWinSize) UnpPtr -= MaxWinSize;
    }
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

void ModelPPM::RestartModelRare()
{
    int i, k, m;
    memset(CharMask, 0, sizeof(CharMask));
    SubAlloc.InitSubAllocator();
    InitRL = -(MaxOrder < 12 ? MaxOrder : 12) - 1;
    MinContext = MaxContext = (RARPPM_CONTEXT *)SubAlloc.AllocContext();
    if (MinContext == NULL) throw std::bad_alloc();
    MinContext->Suffix = NULL;
    OrderFall = MaxOrder;
    MinContext->U.SummFreq = (MinContext->NumStats = 256) + 1;
    FoundState = MinContext->U.Stats = (RARPPM_STATE *)SubAlloc.AllocUnits(256 / 2);
    if (FoundState == NULL) throw std::bad_alloc();
    for (RunLength = InitRL, PrevSuccess = i = 0; i < 256; i++) {
        MinContext->U.Stats[i].Symbol = i;
        MinContext->U.Stats[i].Freq = 1;
        MinContext->U.Stats[i].Successor = NULL;
    }

    static const ushort InitBinEsc[] = {0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

    for (i = 0; i < 128; i++)
        for (k = 0; k < 8; k++)
            for (m = 0; m < 64; m += 8) BinSumm[i][k + m] = BIN_SCALE - InitBinEsc[k] / (i + 2);
    for (i = 0; i < 25; i++)
        for (k = 0; k < 16; k++) SEE2Cont[i][k].init(5 * i + 10);
}

void ModelPPM::StartModelRare(int MaxOrder)
{
    int i, k, m, Step;
    EscCount = 1;
    /*
  if (MaxOrder < 2)
  {
    memset(CharMask,0,sizeof(CharMask));
    OrderFall=ModelPPM::MaxOrder;
    MinContext=MaxContext;
    while (MinContext->Suffix != NULL)
    {
      MinContext=MinContext->Suffix;
      OrderFall--;
    }
    FoundState=MinContext->U.Stats;
    MinContext=MaxContext;
  }
  else
*/
    {
        ModelPPM::MaxOrder = MaxOrder;
        RestartModelRare();
        NS2BSIndx[0] = 2 * 0;
        NS2BSIndx[1] = 2 * 1;
        memset(NS2BSIndx + 2, 2 * 2, 9);
        memset(NS2BSIndx + 11, 2 * 3, 256 - 11);
        for (i = 0; i < 3; i++) NS2Indx[i] = i;
        for (m = i, k = Step = 1; i < 256; i++) {
            NS2Indx[i] = m;
            if (!--k) {
                k = ++Step;
                m++;
            }
        }
        memset(HB2Flag, 0, 0x40);
        memset(HB2Flag + 0x40, 0x08, 0x100 - 0x40);
        DummySEE2Cont.Shift = PERIOD_BITS;
    }
}

RARPPM_CONTEXT *ModelPPM::CreateSuccessors(bool Skip, RARPPM_STATE *p1)
{
    RARPPM_STATE UpState;
    RARPPM_CONTEXT *pc = MinContext, *UpBranch = FoundState->Successor;
    RARPPM_STATE *p, *ps[MAX_O], **pps = ps;
    if (!Skip) {
        *pps++ = FoundState;
        if (!pc->Suffix) goto NO_LOOP;
    }
    if (p1) {
        p = p1;
        pc = pc->Suffix;
        goto LOOP_ENTRY;
    }
    do {
        pc = pc->Suffix;
        if (pc->NumStats != 1) {
            if ((p = pc->U.Stats)->Symbol != FoundState->Symbol) do {
                    p++;
                } while (p->Symbol != FoundState->Symbol);
        } else p = &(pc->OneState);
    LOOP_ENTRY:
        if (p->Successor != UpBranch) {
            pc = p->Successor;
            break;
        }
        // We ensure that PPM order input parameter does not exceed MAX_O (64),
        // so we do not really need this check and added it for extra safety.
        // See CVE-2017-17969 for details.
        if (pps >= ps + ASIZE(ps)) return NULL;

        *pps++ = p;
    } while (pc->Suffix);
NO_LOOP:
    if (pps == ps) return pc;
    UpState.Symbol = *(quint8 *)UpBranch;
    UpState.Successor = (RARPPM_CONTEXT *)(((quint8 *)UpBranch) + 1);
    if (pc->NumStats != 1) {
        if ((quint8 *)pc <= SubAlloc.pText) return (NULL);
        if ((p = pc->U.Stats)->Symbol != UpState.Symbol) do {
                p++;
            } while (p->Symbol != UpState.Symbol);
        uint cf = p->Freq - 1;
        uint s0 = pc->U.SummFreq - pc->NumStats - cf;
        UpState.Freq = 1 + ((2 * cf <= s0) ? (5 * cf > s0) : ((2 * cf + 3 * s0 - 1) / (2 * s0)));
    } else UpState.Freq = pc->OneState.Freq;
    do {
        pc = pc->createChild(this, *--pps, UpState);
        if (!pc) return NULL;
    } while (pps != ps);
    return pc;
}

void ModelPPM::UpdateModel()
{
    RARPPM_STATE fs = *FoundState, *p = NULL;
    RARPPM_CONTEXT *pc, *Successor;
    uint ns1, ns, cf, sf, s0;
    if (fs.Freq < MAX_FREQ / 4 && (pc = MinContext->Suffix) != NULL) {
        if (pc->NumStats != 1) {
            if ((p = pc->U.Stats)->Symbol != fs.Symbol) {
                do {
                    p++;
                } while (p->Symbol != fs.Symbol);
                if (p[0].Freq >= p[-1].Freq) {
                    _PPMD_SWAP(p[0], p[-1]);
                    p--;
                }
            }
            if (p->Freq < MAX_FREQ - 9) {
                p->Freq += 2;
                pc->U.SummFreq += 2;
            }
        } else {
            p = &(pc->OneState);
            p->Freq += (p->Freq < 32);
        }
    }
    if (!OrderFall) {
        MinContext = MaxContext = FoundState->Successor = CreateSuccessors(true, p);
        if (!MinContext) goto RESTART_MODEL;
        return;
    }
    *SubAlloc.pText++ = fs.Symbol;
    Successor = (RARPPM_CONTEXT *)SubAlloc.pText;
    if (SubAlloc.pText >= SubAlloc.FakeUnitsStart) goto RESTART_MODEL;
    if (fs.Successor) {
        if ((quint8 *)fs.Successor <= SubAlloc.pText && (fs.Successor = CreateSuccessors(false, p)) == NULL) goto RESTART_MODEL;
        if (!--OrderFall) {
            Successor = fs.Successor;
            SubAlloc.pText -= (MaxContext != MinContext);
        }
    } else {
        FoundState->Successor = Successor;
        fs.Successor = MinContext;
    }
    s0 = MinContext->U.SummFreq - (ns = MinContext->NumStats) - (fs.Freq - 1);
    for (pc = MaxContext; pc != MinContext; pc = pc->Suffix) {
        if ((ns1 = pc->NumStats) != 1) {
            if ((ns1 & 1) == 0) {
                pc->U.Stats = (RARPPM_STATE *)SubAlloc.ExpandUnits(pc->U.Stats, ns1 >> 1);
                if (!pc->U.Stats) goto RESTART_MODEL;
            }
            pc->U.SummFreq += (2 * ns1 < ns) + 2 * ((4 * ns1 <= ns) & (pc->U.SummFreq <= 8 * ns1));
        } else {
            p = (RARPPM_STATE *)SubAlloc.AllocUnits(1);
            if (!p) goto RESTART_MODEL;
            *p = pc->OneState;
            pc->U.Stats = p;
            if (p->Freq < MAX_FREQ / 4 - 1) p->Freq += p->Freq;
            else p->Freq = MAX_FREQ - 4;
            pc->U.SummFreq = p->Freq + InitEsc + (ns > 3);
        }
        cf = 2 * fs.Freq * (pc->U.SummFreq + 6);
        sf = s0 + pc->U.SummFreq;
        if (cf < 6 * sf) {
            cf = 1 + (cf > sf) + (cf >= 4 * sf);
            pc->U.SummFreq += 3;
        } else {
            cf = 4 + (cf >= 9 * sf) + (cf >= 12 * sf) + (cf >= 15 * sf);
            pc->U.SummFreq += (ushort)cf;
        }
        p = pc->U.Stats + ns1;
        p->Successor = Successor;
        p->Symbol = fs.Symbol;
        p->Freq = (quint8)cf;
        pc->NumStats = (ushort)++ns1;
    }
    MaxContext = MinContext = fs.Successor;
    return;
RESTART_MODEL:
    RestartModelRare();
    EscCount = 0;
}

void ModelPPM::ClearMask()
{
    EscCount = 1;
    memset(CharMask, 0, sizeof(CharMask));
}

ModelPPM::ModelPPM()
{
    MinContext = NULL;
    MaxContext = NULL;
    MedContext = NULL;
}

void ModelPPM::CleanUp()
{
    SubAlloc.StopSubAllocator();
    SubAlloc.StartSubAllocator(1);
    StartModelRare(2);
}

bool ModelPPM::DecodeInit(rar_Unpack *UnpackRead, int &EscChar)
{
    int MaxOrder = UnpackRead->GetChar();
    bool Reset = (MaxOrder & 0x20) != 0;

    int MaxMB;
    if (Reset) MaxMB = UnpackRead->GetChar();
    else if (SubAlloc.GetAllocatedMemory() == 0) return (false);
    if (MaxOrder & 0x40) EscChar = UnpackRead->GetChar();
    Coder.InitDecoder(UnpackRead);
    if (Reset) {
        MaxOrder = (MaxOrder & 0x1f) + 1;
        if (MaxOrder > 16) MaxOrder = 16 + (MaxOrder - 16) * 3;
        if (MaxOrder == 1) {
            SubAlloc.StopSubAllocator();
            return (false);
        }
        SubAlloc.StartSubAllocator(MaxMB + 1);
        StartModelRare(MaxOrder);
    }
    return (MinContext != NULL);
}

int ModelPPM::DecodeChar()
{
    if ((quint8 *)MinContext <= SubAlloc.pText || (quint8 *)MinContext > SubAlloc.HeapEnd) return (-1);
    if (MinContext->NumStats != 1) {
        if ((quint8 *)MinContext->U.Stats <= SubAlloc.pText || (quint8 *)MinContext->U.Stats > SubAlloc.HeapEnd) return (-1);
        if (!MinContext->decodeSymbol1(this)) return (-1);
    } else MinContext->decodeBinSymbol(this);
    Coder.Decode();
    while (!FoundState) {
        ARI_DEC_NORMALIZE(Coder.code, Coder.low, Coder.range, Coder.UnpackRead);
        do {
            OrderFall++;
            MinContext = MinContext->Suffix;
            if ((quint8 *)MinContext <= SubAlloc.pText || (quint8 *)MinContext > SubAlloc.HeapEnd) return (-1);
        } while (MinContext->NumStats == NumMasked);
        if (!MinContext->decodeSymbol2(this)) return (-1);
        Coder.Decode();
    }
    int Symbol = FoundState->Symbol;
    if (!OrderFall && (quint8 *)FoundState->Successor > SubAlloc.pText) MinContext = MaxContext = FoundState->Successor;
    else {
        UpdateModel();
        if (EscCount == 0) ClearMask();
    }
    ARI_DEC_NORMALIZE(Coder.code, Coder.low, Coder.range, Coder.UnpackRead);
    return (Symbol);
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

void RangeCoder::InitDecoder(rar_Unpack *UnpackRead)
{
    RangeCoder::UnpackRead = UnpackRead;

    low = code = 0;
    range = 0xffffffff;
    for (uint i = 0; i < 4; i++) code = (code << 8) | GetChar();
}

int RangeCoder::GetCurrentCount()
{
    return (code - low) / (range /= SubRange.scale);
}

uint RangeCoder::GetCurrentShiftCount(uint SHIFT)
{
    return (code - low) / (range >>= SHIFT);
}

void RangeCoder::Decode()
{
    low += range * SubRange.LowCount;
    range *= SubRange.HighCount - SubRange.LowCount;
}

quint8 RangeCoder::GetChar()
{
    return UnpackRead->GetChar();
}

// Tabulated escapes for exponential symbol distribution
static const quint8 ExpEscape[16] = {25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2};
#define GET_MEAN(SUMM, SHIFT, ROUND) ((SUMM + (1 << (SHIFT - ROUND))) >> (SHIFT))

void RARPPM_CONTEXT::decodeBinSymbol(ModelPPM *Model)
{
    RARPPM_STATE &rs = OneState;
    Model->HiBitsFlag = Model->HB2Flag[Model->FoundState->Symbol];
    ushort &bs = Model->BinSumm[rs.Freq - 1][Model->PrevSuccess + Model->NS2BSIndx[Suffix->NumStats - 1] + Model->HiBitsFlag + 2 * Model->HB2Flag[rs.Symbol] +
                                             ((Model->RunLength >> 26) & 0x20)];
    if (Model->Coder.GetCurrentShiftCount(TOT_BITS) < bs) {
        Model->FoundState = &rs;
        rs.Freq += (rs.Freq < 128);
        Model->Coder.SubRange.LowCount = 0;
        Model->Coder.SubRange.HighCount = bs;
        bs = GET_SHORT16(bs + INTERVAL - GET_MEAN(bs, PERIOD_BITS, 2));
        Model->PrevSuccess = 1;
        Model->RunLength++;
    } else {
        Model->Coder.SubRange.LowCount = bs;
        bs = GET_SHORT16(bs - GET_MEAN(bs, PERIOD_BITS, 2));
        Model->Coder.SubRange.HighCount = BIN_SCALE;
        Model->InitEsc = ExpEscape[bs >> 10];
        Model->NumMasked = 1;
        Model->CharMask[rs.Symbol] = Model->EscCount;
        Model->PrevSuccess = 0;
        Model->FoundState = NULL;
    }
}

bool RARPPM_CONTEXT::decodeSymbol1(ModelPPM *Model)
{
    Model->Coder.SubRange.scale = U.SummFreq;
    RARPPM_STATE *p = U.Stats;
    int i, HiCnt;
    int count = Model->Coder.GetCurrentCount();
    if (count >= (int)Model->Coder.SubRange.scale) return (false);
    if (count < (HiCnt = p->Freq)) {
        Model->PrevSuccess = (2 * (Model->Coder.SubRange.HighCount = HiCnt) > Model->Coder.SubRange.scale);
        Model->RunLength += Model->PrevSuccess;
        (Model->FoundState = p)->Freq = (HiCnt += 4);
        U.SummFreq += 4;
        if (HiCnt > MAX_FREQ) rescale(Model);
        Model->Coder.SubRange.LowCount = 0;
        return (true);
    } else if (Model->FoundState == NULL) return (false);
    Model->PrevSuccess = 0;
    i = NumStats - 1;
    while ((HiCnt += (++p)->Freq) <= count)
        if (--i == 0) {
            Model->HiBitsFlag = Model->HB2Flag[Model->FoundState->Symbol];
            Model->Coder.SubRange.LowCount = HiCnt;
            Model->CharMask[p->Symbol] = Model->EscCount;
            i = (Model->NumMasked = NumStats) - 1;
            Model->FoundState = NULL;
            do {
                Model->CharMask[(--p)->Symbol] = Model->EscCount;
            } while (--i);
            Model->Coder.SubRange.HighCount = Model->Coder.SubRange.scale;
            return (true);
        }
    Model->Coder.SubRange.LowCount = (Model->Coder.SubRange.HighCount = HiCnt) - p->Freq;
    update1(Model, p);
    return (true);
}

bool RARPPM_CONTEXT::decodeSymbol2(ModelPPM *Model)
{
    int count, HiCnt, i = NumStats - Model->NumMasked;
    RARPPM_SEE2_CONTEXT *psee2c = makeEscFreq2(Model, i);
    RARPPM_STATE *ps[256], **pps = ps, *p = U.Stats - 1;
    HiCnt = 0;
    do {
        do {
            p++;
        } while (Model->CharMask[p->Symbol] == Model->EscCount);
        HiCnt += p->Freq;

        // We do not reuse PPMd coder in unstable state, so we do not really need
        // this check and added it for extra safety. See CVE-2017-17969 for details.
        if (pps >= ps + ASIZE(ps)) return false;

        *pps++ = p;
    } while (--i);
    Model->Coder.SubRange.scale += HiCnt;
    count = Model->Coder.GetCurrentCount();
    if (count >= (int)Model->Coder.SubRange.scale) return (false);
    p = *(pps = ps);
    if (count < HiCnt) {
        HiCnt = 0;
        while ((HiCnt += p->Freq) <= count) {
            pps++;
            if (pps >= ps + ASIZE(ps))  // Extra safety check.
                return false;
            p = *pps;
        }
        Model->Coder.SubRange.LowCount = (Model->Coder.SubRange.HighCount = HiCnt) - p->Freq;
        psee2c->update();
        update2(Model, p);
    } else {
        Model->Coder.SubRange.LowCount = HiCnt;
        Model->Coder.SubRange.HighCount = Model->Coder.SubRange.scale;
        i = NumStats - Model->NumMasked;

        // 2022.12.02: we removed pps-- here and changed the code below to avoid
        // "array subscript -1 is outside array bounds" warning in some compilers.
        do {
            if (pps >= ps + ASIZE(ps))  // Extra safety check.
                return false;
            Model->CharMask[(*pps)->Symbol] = Model->EscCount;
            pps++;
        } while (--i);
        psee2c->Summ += (ushort)Model->Coder.SubRange.scale;
        Model->NumMasked = NumStats;
    }
    return true;
}

void RARPPM_CONTEXT::update1(ModelPPM *Model, RARPPM_STATE *p)
{
    (Model->FoundState = p)->Freq += 4;
    U.SummFreq += 4;
    if (p[0].Freq > p[-1].Freq) {
        _PPMD_SWAP(p[0], p[-1]);
        Model->FoundState = --p;
        if (p->Freq > MAX_FREQ) rescale(Model);
    }
}

void RARPPM_CONTEXT::update2(ModelPPM *Model, RARPPM_STATE *p)
{
    (Model->FoundState = p)->Freq += 4;
    U.SummFreq += 4;
    if (p->Freq > MAX_FREQ) rescale(Model);
    Model->EscCount++;
    Model->RunLength = Model->InitRL;
}

void RARPPM_CONTEXT::rescale(ModelPPM *Model)
{
    int OldNS = NumStats, i = NumStats - 1, Adder, EscFreq;
    RARPPM_STATE *p1, *p;
    for (p = Model->FoundState; p != U.Stats; p--) _PPMD_SWAP(p[0], p[-1]);
    U.Stats->Freq += 4;
    U.SummFreq += 4;
    EscFreq = U.SummFreq - p->Freq;
    Adder = (Model->OrderFall != 0);
    U.SummFreq = (p->Freq = (p->Freq + Adder) >> 1);
    do {
        EscFreq -= (++p)->Freq;
        U.SummFreq += (p->Freq = (p->Freq + Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) {
            RARPPM_STATE tmp = *(p1 = p);
            do {
                p1[0] = p1[-1];
            } while (--p1 != U.Stats && tmp.Freq > p1[-1].Freq);
            *p1 = tmp;
        }
    } while (--i);
    if (p->Freq == 0) {
        do {
            i++;
        } while ((--p)->Freq == 0);
        EscFreq += i;
        if ((NumStats -= i) == 1) {
            RARPPM_STATE tmp = *U.Stats;
            do {
                tmp.Freq -= (tmp.Freq >> 1);
                EscFreq >>= 1;
            } while (EscFreq > 1);
            Model->SubAlloc.FreeUnits(U.Stats, (OldNS + 1) >> 1);
            *(Model->FoundState = &OneState) = tmp;
            return;
        }
    }
    U.SummFreq += (EscFreq -= (EscFreq >> 1));
    int n0 = (OldNS + 1) >> 1, n1 = (NumStats + 1) >> 1;
    if (n0 != n1) U.Stats = (RARPPM_STATE *)Model->SubAlloc.ShrinkUnits(U.Stats, n0, n1);
    Model->FoundState = U.Stats;
}

RARPPM_CONTEXT *RARPPM_CONTEXT::createChild(ModelPPM *Model, RARPPM_STATE *pStats, RARPPM_STATE &FirstState)
{
    RARPPM_CONTEXT *pc = (RARPPM_CONTEXT *)Model->SubAlloc.AllocContext();
    if (pc) {
        pc->NumStats = 1;
        pc->OneState = FirstState;
        pc->Suffix = this;
        pStats->Successor = pc;
    }
    return pc;
}

RARPPM_SEE2_CONTEXT *RARPPM_CONTEXT::makeEscFreq2(ModelPPM *Model, int Diff)
{
    RARPPM_SEE2_CONTEXT *psee2c;
    if (NumStats != 256) {
        psee2c = Model->SEE2Cont[Model->NS2Indx[Diff - 1]] + (Diff < Suffix->NumStats - NumStats) + 2 * (U.SummFreq < 11 * NumStats) + 4 * (Model->NumMasked > Diff) +
                 Model->HiBitsFlag;
        Model->Coder.SubRange.scale = psee2c->getMean();
    } else {
        psee2c = &Model->DummySEE2Cont;
        Model->Coder.SubRange.scale = 1;
    }
    return psee2c;
}
