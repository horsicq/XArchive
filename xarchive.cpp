/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#include "xarchive.h"

#if defined(_MSC_VER)
#if _MSC_VER > 1800                                   // TODO Check !!!
#pragma comment(lib, "legacy_stdio_definitions.lib")  // bzip2.lib(compress.obj) __imp__fprintf

FILE _iob[] = {*stdin, *stdout, *stderr};  // bzip2.lib(compress.obj) _iob_func

extern "C" FILE *__cdecl __iob_func(void)
{
    return _iob;
}
#endif
#endif

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

XArchive::XArchive(QIODevice *pDevice) : XBinary(pDevice)
{
}

XArchive::COMPRESS_RESULT XArchive::_decompress(DECOMPRESSSTRUCT *pDecompressStruct, PDSTRUCT *pPdStruct)
{
    if (pDecompressStruct->nDecompressedLimit == 0) {
        pDecompressStruct->nDecompressedLimit = -1;
    }

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_STORE) {
        const qint32 CHUNK = DECOMPRESS_BUFFERSIZE;
        char buffer[CHUNK];
        qint64 nSize = pDecompressStruct->pSourceDevice->size();

        result = COMPRESS_RESULT_OK;

        while (nSize > 0) {
            qint64 nTemp = qMin((qint64)CHUNK, nSize);

            if (pDecompressStruct->pSourceDevice->read(buffer, nTemp) != nTemp) {
                result = COMPRESS_RESULT_READERROR;
                break;
            }

            if (!_writeToDevice((char *)buffer, nTemp, pDecompressStruct)) {
                result = COMPRESS_RESULT_WRITEERROR;
                break;
            }

            if (pPdStruct->bIsStop) {
                break;
            }

            nSize -= nTemp;
            pDecompressStruct->nInSize += nTemp;
            pDecompressStruct->nOutSize += nTemp;

            if ((pDecompressStruct->nDecompressedLimit != -1) &&
                ((pDecompressStruct->nDecompressedOffset + pDecompressStruct->nDecompressedLimit) < pDecompressStruct->nOutSize)) {
                pDecompressStruct->bLimit = true;
                break;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_PPMD) {
        // TODO Check
#ifdef PPMD_SUPPORT
        quint8 nOrder = 0;
        quint32 nMemSize = 0;

        pSourceDevice->read((char *)(&nOrder), 1);
        pSourceDevice->read((char *)(&nMemSize), 4);

        bool bSuccess = true;

        if ((nOrder < PPMD7_MIN_ORDER) || (nOrder > PPMD7_MAX_ORDER) || (nMemSize < PPMD7_MIN_MEM_SIZE) || (nMemSize > PPMD7_MAX_MEM_SIZE)) {
            bSuccess = false;
        }

        bSuccess = true;

        if (bSuccess) {
            CPpmd7 ppmd;
            Ppmd7_Construct(&ppmd);

            if (Ppmd7_Alloc(&ppmd, nMemSize, &g_Alloc)) {
                Ppmd7_Init(&ppmd, nOrder);
            }
        }
#endif
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_DEFLATE) {
        const qint32 CHUNK = DECOMPRESS_BUFFERSIZE;

        unsigned char in[CHUNK];
        unsigned char out[CHUNK];

        z_stream strm;

        strm.zalloc = nullptr;
        strm.zfree = nullptr;
        strm.opaque = nullptr;
        strm.avail_in = 0;
        strm.next_in = nullptr;

        qint32 ret = Z_OK;
        result = COMPRESS_RESULT_OK;

        if (inflateInit2(&strm, -MAX_WBITS) == Z_OK)  // -MAX_WBITS for raw data
        {
            do {
                strm.avail_in = pDecompressStruct->pSourceDevice->read((char *)in, CHUNK);

                if (strm.avail_in == 0) {
                    ret = Z_ERRNO;
                    break;
                }

                strm.next_in = in;

                do {
                    strm.total_in = 0;
                    strm.total_out = 0;
                    strm.avail_out = CHUNK;
                    //                    strm.avail_out=1;
                    strm.next_out = out;
                    ret = inflate(&strm, Z_NO_FLUSH);
                    //                    ret=inflate(&strm,Z_SYNC_FLUSH);

                    if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT)) {
                        break;
                    }

                    qint32 nTemp = CHUNK - strm.avail_out;

                    if (!_writeToDevice((char *)out, nTemp, pDecompressStruct)) {
                        ret = Z_DATA_ERROR;
                        break;
                    }

                    pDecompressStruct->nInSize += strm.total_in;
                    pDecompressStruct->nOutSize += strm.total_out;

                    if ((pDecompressStruct->nDecompressedLimit != -1) &&
                        ((pDecompressStruct->nDecompressedOffset + pDecompressStruct->nDecompressedLimit) < pDecompressStruct->nOutSize)) {
                        pDecompressStruct->bLimit = true;
                        ret = Z_STREAM_END;
                        break;
                    }
                } while (strm.avail_out == 0);

                if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT) || (ret == Z_ERRNO)) {
                    break;
                }

                if (pPdStruct->bIsStop) {
                    break;
                }
            } while (ret != Z_STREAM_END);

            inflateEnd(&strm);

            if ((ret == Z_OK) || (ret == Z_STREAM_END))  // TODO Check Z_OK
            {
                result = COMPRESS_RESULT_OK;
            } else if (ret == Z_BUF_ERROR) {
                result = COMPRESS_RESULT_BUFFERERROR;
            } else if (ret == Z_MEM_ERROR) {
                result = COMPRESS_RESULT_MEMORYERROR;
            } else if (ret == Z_DATA_ERROR) {
                result = COMPRESS_RESULT_DATAERROR;
            } else {
                result = COMPRESS_RESULT_UNKNOWN;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_BZIP2) {
        const qint32 CHUNK = DECOMPRESS_BUFFERSIZE;

        char in[CHUNK];
        char out[CHUNK];

        bz_stream strm = {};
        qint32 ret = BZ_MEM_ERROR;
        result = COMPRESS_RESULT_OK;

        qint32 rc = BZ2_bzDecompressInit(&strm, 0, 0);

        if (rc == BZ_OK) {
            do {
                strm.avail_in = pDecompressStruct->pSourceDevice->read((char *)in, CHUNK);

                if (strm.avail_in == 0) {
                    ret = BZ_MEM_ERROR;
                    break;
                }

                strm.next_in = in;

                do {
                    strm.total_in_hi32 = 0;
                    strm.total_in_lo32 = 0;
                    strm.total_out_hi32 = 0;
                    strm.total_out_lo32 = 0;
                    strm.avail_out = CHUNK;
                    strm.next_out = out;
                    ret = BZ2_bzDecompress(&strm);

                    if ((ret != BZ_STREAM_END) && (ret != BZ_OK)) {
                        break;
                    }

                    qint32 nTemp = CHUNK - strm.avail_out;

                    if (!_writeToDevice((char *)out, nTemp, pDecompressStruct)) {
                        ret = BZ_MEM_ERROR;
                        break;
                    }

                    pDecompressStruct->nInSize += strm.total_in_lo32;
                    pDecompressStruct->nOutSize += strm.total_out_lo32;

                    if ((pDecompressStruct->nDecompressedLimit != -1) &&
                        ((pDecompressStruct->nDecompressedOffset + pDecompressStruct->nDecompressedLimit) < pDecompressStruct->nOutSize)) {
                        pDecompressStruct->bLimit = true;
                        ret = BZ_STREAM_END;
                        break;
                    }
                } while (strm.avail_out == 0);

                if (ret != BZ_OK) {
                    break;
                }

                if (pPdStruct->bIsStop) {
                    break;
                }
            } while (ret != BZ_STREAM_END);

            BZ2_bzDecompressEnd(&strm);
        }

        // TODO more error codes
        if ((ret == BZ_OK) || (ret == BZ_STREAM_END)) {
            result = COMPRESS_RESULT_OK;
        } else if (ret == BZ_MEM_ERROR) {
            result = COMPRESS_RESULT_DATAERROR;
        } else {
            result = COMPRESS_RESULT_UNKNOWN;
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZMA_ZIP) {
        result = COMPRESS_RESULT_OK;

        // TODO more error codes
        qint32 nPropSize = 0;
        char header1[4] = {};
        quint8 properties[32] = {};

        pDecompressStruct->pSourceDevice->read(header1, sizeof(header1));
        nPropSize = header1[2];  // TODO Check

        if (nPropSize && (nPropSize < 30)) {
            pDecompressStruct->pSourceDevice->read((char *)properties, nPropSize);

            CLzmaDec state = {};

            SRes ret = LzmaProps_Decode(&state.prop, (Byte *)properties, nPropSize);

            if (ret == 0)  // S_OK
            {
                LzmaDec_Construct(&state);
                ret = LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, &g_Alloc);

                if (ret == 0)  // S_OK
                {
                    LzmaDec_Init(&state);

                    const qint32 CHUNK = DECOMPRESS_BUFFERSIZE;

                    char in[CHUNK];
                    char out[CHUNK];

                    bool bRun = true;

                    while (bRun) {
                        qint32 nSize = pDecompressStruct->pSourceDevice->read((char *)in, CHUNK);

                        if (nSize) {
                            qint64 nPos = 0;

                            while (true) {
                                ELzmaStatus status;
                                SizeT inProcessed = nSize - nPos;
                                SizeT outProcessed = CHUNK;

                                ret = LzmaDec_DecodeToBuf(&state, (Byte *)out, &outProcessed, (Byte *)(in + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

                                // TODO Check ret

                                nPos += inProcessed;

                                if (!_writeToDevice((char *)out, (qint32)outProcessed, pDecompressStruct)) {
                                    result = COMPRESS_RESULT_WRITEERROR;
                                    bRun = false;
                                    break;
                                }

                                if (status != LZMA_STATUS_NOT_FINISHED) {
                                    if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
                                        result = COMPRESS_RESULT_OK;
                                        bRun = false;
                                    }

                                    break;
                                }

                                pDecompressStruct->nInSize += inProcessed;
                                pDecompressStruct->nOutSize += outProcessed;

                                if ((pDecompressStruct->nDecompressedLimit != -1) &&
                                    ((pDecompressStruct->nDecompressedOffset + pDecompressStruct->nDecompressedLimit) < pDecompressStruct->nOutSize)) {
                                    pDecompressStruct->bLimit = true;
                                    result = COMPRESS_RESULT_OK;
                                    bRun = false;
                                    break;
                                }
                            }
                        } else {
                            result = COMPRESS_RESULT_READERROR;
                            bRun = false;
                        }

                        if (pPdStruct->bIsStop) {
                            bRun = false;
                        }
                    }
                }

                LzmaDec_Free(&state, &g_Alloc);
            }
        }
    } else if ((pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH5) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH6) ||
               (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH7)) {
        qint32 nMethod = 5;
        qint32 nBufferSize = 1U << 17;

        if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH5) {
            nMethod = 5;
        } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH6) {
            nMethod = 6;
        } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH7) {
            nMethod = 7;
        }

        XCompress::lzh_stream strm = {};

        qint32 ret = ARCHIVE_OK;

        //        qDebug("Size: %lld", pSourceDevice->size());

        //        if (pSourceDevice->size() > 25000) {
        //            ret = ARCHIVE_OK;
        //        }

        unsigned char *pInBuffer = (unsigned char *)malloc(nBufferSize);

        result = COMPRESS_RESULT_OK;

        if (XCompress::lzh_decode_init(&strm, nMethod)) {
            strm.avail_in = pDecompressStruct->pSourceDevice->read((char *)pInBuffer, nBufferSize);  // We read from Device so if size < nBufferSize is OK

            if (strm.avail_in) {
                strm.next_in = pInBuffer;

                strm.total_in = 0;
                strm.avail_out = 0;
                // strm.ref_ptr = out;

                while (true) {
                    ret = XCompress::lzh_decode(&strm, true);

                    if (!_writeToDevice((char *)strm.ref_ptr, strm.avail_out, pDecompressStruct)) {
                        ret = ARCHIVE_FATAL;
                        break;
                    }

                    if (strm.avail_in == 0) {
                        break;
                    }
                }

                pDecompressStruct->nInSize += strm.total_in;
                pDecompressStruct->nOutSize += strm.total_out;

                // if ((nDecompressedSize != -1) && ((nDecompressedOffset + nDecompressedSize) < *pnOutSize)) {
                //     break;
                // }
            }

            XCompress::lzh_decode_free(&strm);
        }

        free(pInBuffer);

        if (ret == ARCHIVE_OK) {
            result = COMPRESS_RESULT_OK;
        }
    } else if ((pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_15) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_20) ||
               (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_29) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_50) ||
               (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_70)) {
        // TODO Check Fragmented

        bool bSolid = false;

        XCompress::rar_stream rarStream = {};

        XCompress::rar_init(&rarStream, pDecompressStruct->spInfo.nWindowSize, bSolid);
        rarStream.DestUnpSize = pDecompressStruct->spInfo.nUncompressedSize;

        if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_15) {
            result = COMPRESS_RESULT_OK;

            XCompress::rar_UnpInitData(&rarStream, bSolid);
            XCompress::rar_UnpInitData15(&rarStream, bSolid);
            XCompress::rar_UnpReadBuf(&rarStream, pDecompressStruct->pSourceDevice);

            if (!bSolid) {
                XCompress::rar_InitHuff(&rarStream);
                rarStream.UnpPtr = 0;
            } else rarStream.UnpPtr = rarStream.WrPtr;
            --rarStream.DestUnpSize;
            if (rarStream.DestUnpSize >= 0) {
                XCompress::rar_GetFlagsBuf(&rarStream);
                rarStream.FlagsCnt = 8;
            }

            while (rarStream.DestUnpSize >= 0) {
                rarStream.UnpPtr &= rarStream.MaxWinMask;

                rarStream.FirstWinDone |= (rarStream.PrevPtr > rarStream.UnpPtr);
                rarStream.PrevPtr = rarStream.UnpPtr;

                if (rarStream.InAddr > rarStream.ReadTop - 30 && !XCompress::rar_UnpReadBuf(&rarStream, pDecompressStruct->pSourceDevice)) break;
                if (((rarStream.WrPtr - rarStream.UnpPtr) & rarStream.MaxWinMask) < 270 && rarStream.WrPtr != rarStream.UnpPtr)
                    XCompress::rar_UnpWriteBuf20(&rarStream, pDecompressStruct->pDestDevice);
                if (rarStream.StMode) {
                    XCompress::rar_HuffDecode(&rarStream);
                    continue;
                }

                if (--rarStream.FlagsCnt < 0) {
                    XCompress::rar_GetFlagsBuf(&rarStream);
                    rarStream.FlagsCnt = 7;
                }

                if (rarStream.FlagBuf & 0x80) {
                    rarStream.FlagBuf <<= 1;
                    if (rarStream.Nlzb > rarStream.Nhfb) XCompress::rar_LongLZ(&rarStream);
                    else XCompress::rar_HuffDecode(&rarStream);
                } else {
                    rarStream.FlagBuf <<= 1;
                    if (--rarStream.FlagsCnt < 0) {
                        XCompress::rar_GetFlagsBuf(&rarStream);
                        rarStream.FlagsCnt = 7;
                    }
                    if (rarStream.FlagBuf & 0x80) {
                        rarStream.FlagBuf <<= 1;
                        if (rarStream.Nlzb > rarStream.Nhfb) XCompress::rar_HuffDecode(&rarStream);
                        else XCompress::rar_LongLZ(&rarStream);
                    } else {
                        rarStream.FlagBuf <<= 1;
                        XCompress::rar_ShortLZ(&rarStream);
                    }
                }
            }
            XCompress::rar_UnpWriteBuf20(&rarStream, pDecompressStruct->pDestDevice);
        } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_20) {
            result = COMPRESS_RESULT_OK;
            XCompress::rar_UnpInitData(&rarStream, bSolid);
            if (!XCompress::rar_UnpReadBuf(&rarStream, pDecompressStruct->pSourceDevice)) {
                result = COMPRESS_RESULT_READERROR;
            }
            if ((!bSolid || !rarStream.TablesRead2) && !XCompress::rar_ReadTables20(&rarStream, pDecompressStruct->pSourceDevice)) {
                result = COMPRESS_RESULT_DATAERROR;
            }
            --rarStream.DestUnpSize;

            if (result == COMPRESS_RESULT_OK) {
            }
        } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_29) {
            static unsigned char LDecode[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224};
            static unsigned char LBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
            static int DDecode[XCompress::RAR_DC30];
            static quint8 DBits[XCompress::RAR_DC30];
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

            rarStream.FileExtracted = true;

            XCompress::rar_UnpInitData(&rarStream, bSolid);
            if (!XCompress::rar_UnpReadBuf(&rarStream, pDecompressStruct->pSourceDevice)) {
                result = COMPRESS_RESULT_READERROR;
            }
            if ((!bSolid || !rarStream.TablesRead3) && !XCompress::rar_ReadTables30(&rarStream, pDecompressStruct->pSourceDevice)) {
                result = COMPRESS_RESULT_DATAERROR;
            }

            //             if (result == COMPRESS_RESULT_OK) {
            //                 while (true)
            //                 {
            //                   rarStream.UnpPtr&=rarStream.MaxWinMask;

            //                   rarStream.FirstWinDone|=(rarStream.PrevPtr>rarStream.UnpPtr);
            //                   rarStream.PrevPtr=rarStream.UnpPtr;

            //                   if (rarStream.InAddr>rarStream.ReadBorder)
            //                   {
            //                     if (!XCompress::rar_UnpReadBuf30(&rarStream, pDecompressStruct->pSourceDevice))
            //                       break;
            //                   }
            //                   if (((rarStream.WrPtr-rarStream.UnpPtr) & rarStream.MaxWinMask)<=XCompress::RAR_MAX3_INC_LZ_MATCH && rarStream.WrPtr!=rarStream.UnpPtr)
            //                   {
            //                     XCompress::rar_UnpReadBuf30(&rarStream, pDecompressStruct->pSourceDevice);
            //                     if (rarStream.WrittenFileSize>rarStream.DestUnpSize)
            //                         result = COMPRESS_RESULT_DATAERROR;
            //                         break;
            //                     if (rarStream.Suspended)
            //                     {
            //                         rarStream.FileExtracted=false;
            //                         result = COMPRESS_RESULT_DATAERROR;
            //                         break;
            //                     }
            //                   }
            //                   if (rarStream.UnpBlockType==XCompress::RAR_BLOCK_PPM)
            //                   {
            // #ifdef QT_DEBUG
            //                     qDebug("PPM");
            // #endif
            //                       result = COMPRESS_RESULT_DATAERROR;
            //                       break;
            //                     // // Here speed is critical, so we do not use SafePPMDecodeChar,
            //                     // // because sometimes even the inline function can introduce
            //                     // // some additional penalty.
            //                     // int Ch=PPM.DecodeChar();
            //                     // if (Ch==-1)              // Corrupt PPM data found.
            //                     // {
            //                     //   PPM.CleanUp();         // Reset possibly corrupt PPM data structures.
            //                     //   rarStream.UnpBlockType=XCompress::RAR_BLOCK_LZ; // Set faster and more fail proof LZ mode.
            //                     //   break;
            //                     // }
            //                     // if (Ch==rarStream.PPMEscChar)
            //                     // {
            //                     //   int NextCh=SafePPMDecodeChar();
            //                     //   if (NextCh==0)  // End of PPM encoding.
            //                     //   {
            //                     //     if (!XCompress::rar_ReadTables30(&rarStream, pDecompressStruct->pSourceDevice))
            //                     //       break;
            //                     //     continue;
            //                     //   }
            //                     //   if (NextCh==-1) // Corrupt PPM data found.
            //                     //     break;
            //                     //   if (NextCh==2)  // End of file in PPM mode.
            //                     //     break;
            //                     //   if (NextCh==3)  // Read VM code.
            //                     //   {
            //                     //     if (!ReadVMCodePPM())
            //                     //       break;
            //                     //     continue;
            //                     //   }
            //                     //   if (NextCh==4) // LZ inside of PPM.
            //                     //   {
            //                     //     unsigned int Distance=0,Length;
            //                     //     bool Failed=false;
            //                     //     for (int I=0;I<4 && !Failed;I++)
            //                     //     {
            //                     //       int Ch=SafePPMDecodeChar();
            //                     //       if (Ch==-1)
            //                     //         Failed=true;
            //                     //       else
            //                     //         if (I==3)
            //                     //           Length=(quint8)Ch;
            //                     //         else
            //                     //           Distance=(Distance<<8)+(quint8)Ch;
            //                     //     }
            //                     //     if (Failed)
            //                     //       break;

            //                     //     XCompress::rar_CopyString(Length+32,Distance+2);
            //                     //     continue;
            //                     //   }
            //                     //   if (NextCh==5) // One byte distance match (RLE) inside of PPM.
            //                     //   {
            //                     //     int Length=SafePPMDecodeChar();
            //                     //     if (Length==-1)
            //                     //       break;
            //                     //     XCompress::rar_CopyString(Length+4,1);
            //                     //     continue;
            //                     //   }
            //                     //   // If we are here, NextCh must be 1, what means that current byte
            //                     //   // is equal to our 'escape' byte, so we just store it to Window.
            //                     // }
            //                     // rarStream.Window[rarStream.UnpPtr++]=Ch;
            //                     // continue;
            //                   }

            //                   uint Number=XCompress::rar_DecodeNumber(&rarStream,&rarStream.BlockTables.LD);
            //                   if (Number<256)
            //                   {
            //                     rarStream.Window[rarStream.UnpPtr++]=(quint8)Number;
            //                     continue;
            //                   }
            //                   if (Number>=271)
            //                   {
            //                     uint Length=LDecode[Number-=271]+3;
            //                     if ((Bits=LBits[Number])>0)
            //                     {
            //                       Length+=XCompress::rar_getbits(&rarStream)>>(16-Bits);
            //                       XCompress::rar_addbits(&rarStream, Bits);
            //                     }

            //                     uint DistNumber=XCompress::rar_DecodeNumber(&rarStream,&rarStream.BlockTables.DD);
            //                     uint Distance=DDecode[DistNumber]+1;
            //                     if ((Bits=DBits[DistNumber])>0)
            //                     {
            //                       if (DistNumber>9)
            //                       {
            //                         if (Bits>4)
            //                         {
            //                           Distance+=((XCompress::rar_getbits(&rarStream)>>(20-Bits))<<4);
            //                           XCompress::rar_addbits(&rarStream, Bits-4);
            //                         }
            //                         if (rarStream.LowDistRepCount>0)
            //                         {
            //                           rarStream.LowDistRepCount--;
            //                           Distance+=rarStream.PrevLowDist;
            //                         }
            //                         else
            //                         {
            //                           uint LowDist=XCompress::rar_DecodeNumber(&rarStream,&rarStream.BlockTables.LDD);
            //                           if (LowDist==16)
            //                           {
            //                             rarStream.LowDistRepCount=XCompress::RAR_LOW_DIST_REP_COUNT-1;
            //                             Distance+=rarStream.PrevLowDist;
            //                           }
            //                           else
            //                           {
            //                             Distance+=LowDist;
            //                             rarStream.PrevLowDist=LowDist;
            //                           }
            //                         }
            //                       }
            //                       else
            //                       {
            //                         Distance+=XCompress::rar_getbits(&rarStream)>>(16-Bits);
            //                         XCompress::rar_addbits(&rarStream, Bits);
            //                       }
            //                     }

            //                     if (Distance>=0x2000)
            //                     {
            //                       Length++;
            //                       if (Distance>=0x40000)
            //                         Length++;
            //                     }

            //                     XCompress::rar_InsertOldDist(&rarStream, Distance);
            //                     rarStream.LastLength=Length;
            //                     XCompress::rar_CopyString(&rarStream, Length,Distance);
            //                     continue;
            //                   }
            //                   if (Number==256)
            //                   {
            //                     if (!XCompress::rar_ReadEndOfBlock(&rarStream, pDecompressStruct->pSourceDevice))
            //                       break;
            //                     continue;
            //                   }
            //                   if (Number==257)
            //                   {
            //                     if (!XCompress::rar_ReadVMCode())
            //                       break;
            //                     continue;
            //                   }
            //                   if (Number==258)
            //                   {
            //                     if (rarStream.LastLength!=0)
            //                       XCompress::rar_CopyString(&rarStream, rarStream.LastLength,rarStream.OldDist[0]);
            //                     continue;
            //                   }
            //                   if (Number<263)
            //                   {
            //                     uint DistNum=Number-259;
            //                     uint Distance=(uint)rarStream.OldDist[DistNum];
            //                     for (uint I=DistNum;I>0;I--)
            //                       rarStream.OldDist[I]=rarStream.OldDist[I-1];
            //                     rarStream.OldDist[0]=Distance;

            //                     uint LengthNumber=XCompress::rar_DecodeNumber(&rarStream, &rarStream.BlockTables.RD);
            //                     int Length=LDecode[LengthNumber]+2;
            //                     if ((Bits=LBits[LengthNumber])>0)
            //                     {
            //                       Length+=XCompress::rar_getbits(&rarStream)>>(16-Bits);
            //                       XCompress::rar_addbits(&rarStream, Bits);
            //                     }
            //                     rarStream.LastLength=Length;
            //                     XCompress::rar_CopyString(&rarStream, Length,Distance);
            //                     continue;
            //                   }
            //                   if (Number<272)
            //                   {
            //                     uint Distance=SDDecode[Number-=263]+1;
            //                     if ((Bits=SDBits[Number])>0)
            //                     {
            //                       Distance+=XCompress::rar_getbits(&rarStream)>>(16-Bits);
            //                       XCompress::rar_addbits(&rarStream, Bits);
            //                     }
            //                     XCompress::rar_InsertOldDist(&rarStream, Distance);
            //                     rarStream.LastLength=2;
            //                     XCompress::rar_CopyString(&rarStream, 2,Distance);
            //                     continue;
            //                   }
            //                 }
            //                 XCompress::rar_UnpWriteBuf30(&rarStream, pDecompressStruct->pDestDevice);
            //             }
        }
    }

    return result;
}

bool XArchive::_decompressRecord(const RECORD *pRecord, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset = 0,
                                 qint64 nDecompressedLimit = -1)
{
    bool bResult = false;

    if (pRecord->layerCompressMethod == COMPRESS_METHOD_UNKNOWN) {
        SubDevice sd(pSourceDevice, pRecord->nDataOffset, pRecord->nDataSize);

        if (sd.open(QIODevice::ReadOnly)) {
            XArchive::DECOMPRESSSTRUCT decompressStruct = {};
            decompressStruct.spInfo = pRecord->spInfo;
            decompressStruct.pSourceDevice = &sd;
            decompressStruct.pDestDevice = pDestDevice;
            decompressStruct.nDecompressedOffset = nDecompressedOffset;
            decompressStruct.nDecompressedLimit = nDecompressedLimit;

            bResult = (_decompress(&decompressStruct, pPdStruct) == COMPRESS_RESULT_OK);

            sd.close();
        }
    } else if ((pRecord->layerCompressMethod != COMPRESS_METHOD_UNKNOWN) && (pRecord->spInfo.compressMethod == COMPRESS_METHOD_STORE)) {
        SubDevice sd(pSourceDevice, pRecord->nLayerOffset, pRecord->nLayerSize);

        if (sd.open(QIODevice::ReadOnly)) {
            XArchive::DECOMPRESSSTRUCT decompressStruct = {};
            decompressStruct.spInfo.compressMethod = pRecord->layerCompressMethod;
            decompressStruct.pSourceDevice = &sd;
            decompressStruct.pDestDevice = pDestDevice;
            decompressStruct.nDecompressedOffset = pRecord->nDataOffset;
            decompressStruct.nDecompressedLimit = pRecord->spInfo.nUncompressedSize;

            bResult = (_decompress(&decompressStruct, pPdStruct) == COMPRESS_RESULT_OK);

            sd.close();
        }
        // TODO nDecompressedOffset -> Create buffer copy
    }

    return bResult;
}

XArchive::COMPRESS_RESULT XArchive::_compress(XArchive::COMPRESS_METHOD compressMethod, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    if (compressMethod == COMPRESS_METHOD_STORE) {
        const qint32 CHUNK = COMPRESS_BUFFERSIZE;
        char buffer[CHUNK];
        qint64 nSize = pSourceDevice->size();

        result = COMPRESS_RESULT_OK;

        while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint64 nTemp = qMin((qint64)CHUNK, nSize);

            if (pSourceDevice->read(buffer, nTemp) != nTemp) {
                result = COMPRESS_RESULT_READERROR;
                break;
            }

            if (pDestDevice->write(buffer, nTemp) != nTemp) {
                result = COMPRESS_RESULT_WRITEERROR;
                break;
            }

            nSize -= nTemp;
        }
    } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
        result = _compress_deflate(pSourceDevice, pDestDevice, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);  // -MAX_WBITS for raw data
    }

    return result;
}

XArchive::COMPRESS_RESULT XArchive::_compress_deflate(QIODevice *pSourceDevice, QIODevice *pDestDevice, qint32 nLevel, qint32 nMethod, qint32 nWindowsBits,
                                                      qint32 nMemLevel, qint32 nStrategy, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    const qint32 CHUNK = COMPRESS_BUFFERSIZE;

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    z_stream strm;

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;

    qint32 ret = Z_OK;

    if (deflateInit2(&strm, nLevel, nMethod, nWindowsBits, nMemLevel, nStrategy) == Z_OK) {
        do {
            strm.avail_in = pSourceDevice->read((char *)in, CHUNK);

            qint32 nFlush = Z_NO_FLUSH;

            if (strm.avail_in != CHUNK) {
                nFlush = Z_FINISH;
            }

            if (strm.avail_in == 0) {
                if (!pSourceDevice->atEnd()) {
                    ret = Z_ERRNO;
                    break;
                }
            }

            strm.next_in = in;

            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = deflate(&strm, nFlush);

                if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT)) {
                    break;
                }

                qint32 nTemp = CHUNK - strm.avail_out;

                if (pDestDevice->write((char *)out, nTemp) != nTemp) {
                    ret = Z_ERRNO;
                    break;
                }
            } while ((strm.avail_out == 0) && (!(pPdStruct->bIsStop)));

            if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT) || (ret == Z_ERRNO)) {
                break;
            }
        } while (ret != Z_STREAM_END);

        deflateEnd(&strm);

        if ((ret == Z_OK) || (ret == Z_STREAM_END)) {
            result = COMPRESS_RESULT_OK;
        } else if (ret == Z_BUF_ERROR) {
            result = COMPRESS_RESULT_BUFFERERROR;
        } else if (ret == Z_MEM_ERROR) {
            result = COMPRESS_RESULT_MEMORYERROR;
        } else if (ret == Z_DATA_ERROR) {
            result = COMPRESS_RESULT_DATAERROR;
        } else {
            result = COMPRESS_RESULT_UNKNOWN;
        }
    }

    return result;
}

QByteArray XArchive::decompress(const XArchive::RECORD *pRecord, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset, qint64 nDecompressedLimit)
{
    QByteArray result;

    QBuffer buffer;
    buffer.setBuffer(&result);

    if (buffer.open(QIODevice::WriteOnly)) {
        _decompressRecord(pRecord, getDevice(), &buffer, pPdStruct, nDecompressedOffset, nDecompressedLimit);
        buffer.close();
    }

    return result;
}

QByteArray XArchive::decompress(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, pListArchive, pPdStruct);

    if (!record.spInfo.sRecordName.isEmpty()) {
        if (record.spInfo.nUncompressedSize) {
            baResult = decompress(&record, pPdStruct);
        }
    }

    return baResult;
}

QByteArray XArchive::decompress(const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listArchive = getRecords(-1, pPdStruct);

    return decompress(&listArchive, sRecordFileName, pPdStruct);
}

bool XArchive::decompressToFile(const XArchive::RECORD *pRecord, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFileInfo fi(sResultFileName);

    bResult = XBinary::createDirectory(fi.absolutePath());

    if (pRecord->nDataSize) {
        QFile file;
        file.setFileName(sResultFileName);

        if (file.open(QIODevice::ReadWrite)) {
            bResult = _decompressRecord(pRecord, getDevice(), &file, pPdStruct, 0, -1);
            file.close();
        }
    }

    return bResult;
}

bool XArchive::decompressToDevice(const RECORD *pRecord, QIODevice *pDestDevice, PDSTRUCT *pPdStruct)
{
    return _decompressRecord(pRecord, getDevice(), pDestDevice, pPdStruct, 0, -1);
}

bool XArchive::decompressToFile(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XArchive::RECORD record = getArchiveRecord(sRecordFileName, pListArchive);

    if (record.spInfo.sRecordName != "")  // TODO bIsValid
    {
        bResult = decompressToFile(&record, sResultFileName, pPdStruct);
    }

    return bResult;
}

bool XArchive::decompressToPath(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    QFileInfo fi(sResultPathName);

    XBinary::createDirectory(fi.absolutePath());

    qint32 nNumberOfArchives = pListArchive->count();

    for (qint32 i = 0; i < nNumberOfArchives; i++) {
        XArchive::RECORD record = pListArchive->at(i);

        bool bNamePresent = XBinary::isRegExpPresent(QString("^%1").arg(sRecordFileName), record.spInfo.sRecordName);

        if (bNamePresent || (sRecordFileName == "/") || (sRecordFileName == "")) {
            QString sFileName = record.spInfo.sRecordName;

            if (bNamePresent) {
                sFileName = sFileName.mid(sRecordFileName.size(), -1);
            }

            QString sResultFileName = sResultPathName + QDir::separator() + sFileName;

            QFileInfo fi(sResultFileName);
            XBinary::createDirectory(fi.absolutePath());

            if (!decompressToFile(&record, sResultFileName, pPdStruct)) {
                bResult = false;
                break;
            }
        }
    }
    // TODO emits

    return bResult;
}

bool XArchive::decompressToFile(const QString &sArchiveFileName, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sArchiveFileName);

    if (file.open(QIODevice::ReadOnly)) {
        setDevice(&file);

        if (isValid()) {
            QList<RECORD> listRecords = getRecords(-1, pPdStruct);

            bResult = decompressToFile(&listRecords, sRecordFileName, sResultFileName, pPdStruct);
        }

        file.close();
    }

    return bResult;
}

bool XArchive::decompressToPath(const QString &sArchiveFileName, const QString &sRecordPathName, const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sArchiveFileName);

    if (file.open(QIODevice::ReadOnly)) {
        setDevice(&file);

        if (isValid()) {
            QList<RECORD> listRecords = getRecords(-1, pPdStruct);

            bResult = decompressToPath(&listRecords, sRecordPathName, sResultPathName, pPdStruct);
        }

        file.close();
    }

    return bResult;
}

bool XArchive::dumpToFile(const XArchive::RECORD *pRecord, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    return XBinary::dumpToFile(sFileName, pRecord->nDataOffset, pRecord->nDataSize, pPdStruct);
}

XArchive::RECORD XArchive::getArchiveRecord(const QString &sRecordFileName, QList<XArchive::RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    RECORD result = {};

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (pListRecords->at(i).spInfo.sRecordName == sRecordFileName) {
            result = pListRecords->at(i);
            break;
        }
    }

    return result;
}

XArchive::RECORD XArchive::getArchiveRecordByUUID(const QString &sUUID, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    RECORD result = {};

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (pListRecords->at(i).sUUID == sUUID) {
            result = pListRecords->at(i);
            break;
        }
    }

    return result;
}

bool XArchive::isArchiveRecordPresent(const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords = getRecords(-1, pPdStruct);

    return isArchiveRecordPresent(sRecordFileName, &listRecords, pPdStruct);
}

bool XArchive::isArchiveRecordPresent(const QString &sRecordFileName, QList<XArchive::RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    return (!getArchiveRecord(sRecordFileName, pListRecords, pPdStruct).spInfo.sRecordName.isEmpty());
}

bool XArchive::isArchiveRecordPresentExp(const QString &sRecordFileName, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    bool bResult = false;

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && (!(pPdStruct->bIsStop)); i++) {
        if (isRegExpPresent(sRecordFileName, pListRecords->at(i).spInfo.sRecordName)) {
            bResult = true;
            break;
        }
    }

    return bResult;
}

quint32 XArchive::getCompressBufferSize()
{
    return COMPRESS_BUFFERSIZE;
}

quint32 XArchive::getDecompressBufferSize()
{
    return DECOMPRESS_BUFFERSIZE;
}

void XArchive::showRecords(QList<XArchive::RECORD> *pListArchive)
{
    qint32 nNumberOfRecords = pListArchive->count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
#ifdef QT_DEBUG
        qDebug("%s", pListArchive->at(i).spInfo.sRecordName.toUtf8().data());
#endif
    }
}

XBinary::MODE XArchive::getMode()
{
    return MODE_DATA;
}

qint32 XArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XArchive::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_ARCHIVE: sResult = tr("Archive"); break;
        case TYPE_DOSEXTENDER: sResult = QString("DOS %1").arg(tr("extender")); break;
    }

    return sResult;
}

bool XArchive::_writeToDevice(char *pBuffer, qint32 nBufferSize, DECOMPRESSSTRUCT *pDecompressStruct)
{
    bool bResult = true;

    if (pDecompressStruct->pDestDevice) {
        char *_pOffset = pBuffer;
        qint32 _nSize = nBufferSize;
        qint64 nDecompressedSize = pDecompressStruct->nDecompressedLimit;

        if (nDecompressedSize == -1) {
            nDecompressedSize = pDecompressStruct->nOutSize + nBufferSize;
        }

        if ((pDecompressStruct->nDecompressedOffset) < (pDecompressStruct->nOutSize + nBufferSize)) {
            if ((pDecompressStruct->nDecompressedOffset < (pDecompressStruct->nOutSize + nBufferSize)) &&
                (pDecompressStruct->nDecompressedOffset > pDecompressStruct->nOutSize)) {
                _pOffset += (pDecompressStruct->nDecompressedOffset - pDecompressStruct->nOutSize);
                _nSize -= (pDecompressStruct->nDecompressedOffset - pDecompressStruct->nOutSize);
            }

            if ((pDecompressStruct->nDecompressedOffset + nDecompressedSize) < (pDecompressStruct->nOutSize + nBufferSize)) {
                _nSize -= ((pDecompressStruct->nOutSize + nBufferSize) - (pDecompressStruct->nDecompressedOffset + nDecompressedSize));
            }

            if (_nSize > 0) {
                qint64 nBytesWrote = pDecompressStruct->pDestDevice->write(_pOffset, _nSize);

                pDecompressStruct->nDecompressedWrote += nBytesWrote;

                if (nBytesWrote != _nSize) {
                    bResult = false;
                }
            }
        }
    }

    return bResult;
}

// XBinary::_MEMORY_MAP XArchive::getMemoryMap()
//{
//     _MEMORY_MAP result={};

//    qint64 nTotalSize=getSize();

//    result.nBaseAddress=_getBaseAddress();
//    result.nRawSize=nTotalSize;
//    result.nImageSize=nTotalSize;
//    result.fileType=FT_ARCHIVE;
//    result.mode=getMode();
//    result.sArch=getArch();
//    result.bIsBigEndian=isBigEndian();
//    result.sType=getTypeAsString();

//    qint32 nIndex=0;

//    QList<XArchive::RECORD> listRecords=getRecords();

//    qint32 nNumberOfRecords=listRecords.count();

//    for(qint32 i=0;i<nNumberOfRecords;i++)
//    {
//        _MEMORY_RECORD record={};
//        record.nAddress=-1;
//        record.segment=ADDRESS_SEGMENT_FLAT;
//        record.nOffset=listRecords.at(i).nDataOffset;
//        record.nSize=listRecords.at(i).nDataSize;
//        record.nIndex=nIndex++;
//        record.type=MMT_FILESEGMENT;
//        record.sName=listRecords.at(i).sFileName;

//        result.listRecords.append(record);
//    }

//    return result;
//}
