/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xdecompress.h"
#include "subdevice.h"

XDecompress::XDecompress(QObject *parent) : QObject(parent)
{
    m_pRarUnpacker = nullptr;
    m_nRarSolidIndex = 0;
}

XDecompress::~XDecompress()
{
    clearSolidCache();
}

bool XDecompress::decompressFPART(const XBinary::FPART &fPart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = fPart.mapProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = fPart.nFileOffset;
    state.nInputLimit = fPart.nFileSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    return multiDecompress(&state, pPdStruct);
}

bool XDecompress::decompressArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = archiveRecord.mapProperties;
    state.mapUnpackProperties = mapUnpackProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = archiveRecord.nStreamOffset;
    state.nInputLimit = archiveRecord.nStreamSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    return multiDecompress(&state, pPdStruct);
}


void XDecompress::clearSolidCache()
{
    QList<QString> listKeys = m_mapSolidCache.keys();
    for (qint32 i = 0; i < listKeys.count(); i++) {
        QIODevice *pDevice = m_mapSolidCache.value(listKeys.at(i));
        XBinary::freeFileBuffer(&pDevice);
    }
    m_mapSolidCache.clear();

    delete m_pRarUnpacker;
    m_pRarUnpacker = nullptr;
    m_nRarSolidIndex = 0;
}

bool XDecompress::decompressRarSolid(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput) {
        return false;
    }

    QString sFileName = pState->mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
    QString sCacheKey = QString("rar_%1").arg(sFileName);

    // If the requested file is not yet cached, decompress it using a persistent rar_Unpack
    // instance that maintains decoder dictionary state across sequential solid files.
    if (!m_mapSolidCache.contains(sCacheKey)) {
        XBinary::HANDLE_METHOD compressMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
        qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();
        qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

        // For encrypted RAR5: decrypt first, then use the inner compression method
        QIODevice *pDecryptedDevice = nullptr;
        QIODevice *pInputDevice = pState->pDeviceInput;
        qint64 nInputOffset = pState->nInputOffset;
        qint64 nInputLimit = pState->nInputLimit;

        XBinary::HANDLE_METHOD outerMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, XBinary::HANDLE_METHOD_UNKNOWN).toUInt();
        if (outerMethod == XBinary::HANDLE_METHOD_RAR5_AES) {
            // Decrypt the encrypted data into a temporary buffer
            QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
            qint64 nEncryptedSize = pState->nInputLimit;

            // Align to AES block size
            if (nEncryptedSize > 0 && (nEncryptedSize % AES_BLOCK_SIZE) == 0) {
                pDecryptedDevice = XBinary::createFileBuffer(nEncryptedSize, pPdStruct);
                if (pDecryptedDevice) {
                    XBinary::DATAPROCESS_STATE decryptState = *pState;
                    decryptState.pDeviceOutput = pDecryptedDevice;
                    decryptState.nCountInput = 0;
                    decryptState.nCountOutput = 0;

                    if (XAESDecoder::decryptRar5(&decryptState, sPassword, pPdStruct)) {
                        pInputDevice = pDecryptedDevice;
                        nInputOffset = 0;
                        nInputLimit = nEncryptedSize;
                    } else {
                        XBinary::freeFileBuffer(&pDecryptedDevice);
                        pDecryptedDevice = nullptr;
                    }
                }
            }
        }

        // For solid archives: first file is not solid (bIsSolid=false), subsequent files are solid (bIsSolid=true)
        bool bIsSolid = (m_nRarSolidIndex > 0);

        if (nUncompressedSize > 0) {
            QIODevice *pBuffer = XBinary::createFileBuffer(nUncompressedSize, pPdStruct);

            if (pBuffer) {
                bool bDecompressOk = false;

                if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
                    // STORE: copy data directly, decoder state is unaffected
                    bDecompressOk = XBinary::copyDeviceMemory(pInputDevice, nInputOffset, pBuffer, 0, nInputLimit);
                } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
                           (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
                           (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                    if (!m_pRarUnpacker) {
                        m_pRarUnpacker = new rar_Unpack();
                    }

                    SubDevice sd(pInputDevice, nInputOffset, nInputLimit);

                    if (sd.open(QIODevice::ReadOnly)) {
                        m_pRarUnpacker->setDevices(&sd, pBuffer);
                        qint32 nInit = m_pRarUnpacker->Init(nWindowSize, bIsSolid);

                        if (nInit > 0) {
                            m_pRarUnpacker->SetDestSize(nUncompressedSize);

                            if (compressMethod == XBinary::HANDLE_METHOD_RAR_15) {
                                m_pRarUnpacker->Unpack15(bIsSolid, pPdStruct);
                                bDecompressOk = true;
                            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_20) {
                                m_pRarUnpacker->Unpack20(bIsSolid, pPdStruct);
                                bDecompressOk = true;
                            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_29) {
                                m_pRarUnpacker->Unpack29(bIsSolid, pPdStruct);
                                bDecompressOk = true;
                            } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_50) || (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                                m_pRarUnpacker->Unpack5(bIsSolid, pPdStruct);
                                bDecompressOk = true;
                            }
                        }

                        sd.close();
                    }
                }

                if (bDecompressOk) {
                    m_mapSolidCache.insert(sCacheKey, pBuffer);
                } else {
                    XBinary::freeFileBuffer(&pBuffer);
                }
            }
        }

        // Clean up decrypted device if we created one
        XBinary::freeFileBuffer(&pDecryptedDevice);

        m_nRarSolidIndex++;
    }

    // Retrieve the requested file from cache
    if (m_mapSolidCache.contains(sCacheKey)) {
        QIODevice *pCachedDevice = m_mapSolidCache.value(sCacheKey);
        qint64 nDecompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();

        if (pCachedDevice) {
            pCachedDevice->seek(0);
            QByteArray baData = pCachedDevice->read(nDecompressedSize);
            pState->pDeviceOutput->seek(0);
            pState->nCountOutput = pState->pDeviceOutput->write(baData);
            bResult = (pState->nCountOutput == nDecompressedSize);

            if (bResult) {
                // Verify CRC of the extracted file
                XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
                QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                bResult = checkCRC(crcType, varCRC, pState->pDeviceOutput, pPdStruct);
            }
        }
    }

    return bResult;
}

bool XDecompress::checkCRC(XBinary::CRC_TYPE crcType, QVariant value, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (crcType != XBinary::CRC_TYPE_UNKNOWN) {
        if (!pDevice || !pDevice->isReadable()) {
            return true;
        }

        pDevice->reset();
        bResult = XBinary::checkCRC(pDevice, crcType, value, pPdStruct);
        pDevice->reset();

        if (!bResult) {
            emit warningMessage(QString("%1").arg(tr("Invalid CRC")));
        }
    }

    return bResult;
}

bool XDecompress::multiDecompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QString sArchiveMD5 = pState->mapProperties.value(XBinary::FPART_PROP_FILEMD5).toString();
    if (!sArchiveMD5.isEmpty() && sArchiveMD5 != m_sCurrentArchiveMD5) {
        clearSolidCache();
        m_sCurrentArchiveMD5 = sArchiveMD5;
    }

    bool bIsSolid = pState->mapProperties.value(XBinary::FPART_PROP_ISSOLID, false).toBool();

    qint32 nNumberOfMethods = 1;

    XBinary::HANDLE_METHOD topMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    // BCJ2 handles its own 4 sub-streams internally in decompress() — never treat it as multi-method
    if (topMethod != XBinary::HANDLE_METHOD_BCJ2) {
        if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD3)) {
            nNumberOfMethods = 3;
        } else if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) {
            nNumberOfMethods = 2;
        }
    }

    if ((nNumberOfMethods == 1) && (!bIsSolid)) {
        // Single-method, non-solid: decompress directly to the output device
        bResult = decompress(pState, pPdStruct);
        if (bResult && pState->pDeviceOutput) {
            // Verify CRC of the decompressed result
            XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
            QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
            bResult = checkCRC(crcType, varCRC, pState->pDeviceOutput, pPdStruct);
        }
    } else if (bIsSolid) {
        // Check if this is a RAR solid archive — RAR solid requires sequential decompression
        // with persistent decoder state, unlike 7z solid which uses a single compressed block.
        bool bIsRarSolid = (topMethod == XBinary::HANDLE_METHOD_RAR_15) || (topMethod == XBinary::HANDLE_METHOD_RAR_20) ||
                           (topMethod == XBinary::HANDLE_METHOD_RAR_29) || (topMethod == XBinary::HANDLE_METHOD_RAR_50) ||
                           (topMethod == XBinary::HANDLE_METHOD_RAR_70);

        if (bIsRarSolid) {
            // RAR solid: use XRar streaming API to decompress all files with proper decoder state,
            // cache each file's output, and return the requested file from cache.
            bResult = decompressRarSolid(pState, pPdStruct);
        } else {
            // Non-RAR solid (e.g., 7z): decompress the entire folder block once, cache it,
            // then extract this file's sub-stream.
            // Prefer the explicit solid-folder ID (set by archive parsers such as XSevenZip
            // via FPART_PROP_SOLIDFOLDERINDEX); fall back to offset_size when absent.
            QString sCacheKey;
            qint64 nSolidFolderIndex = pState->mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)-1).toLongLong();
            if (nSolidFolderIndex >= 0) {
                sCacheKey = QString("f%1").arg(nSolidFolderIndex);
            } else {
                sCacheKey = QString("%1_%2").arg(pState->nInputOffset).arg(pState->nInputLimit);
            }

            if (!m_mapSolidCache.contains(sCacheKey)) {
                qint64 nStreamUnpackedSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, (qint64)0).toLongLong();

                // Build a block-level state: same source, ISSOLID=false, full block uncompressed size.
                // The recursive call goes to single-method or multi-method non-solid branch.
                XBinary::DATAPROCESS_STATE blockState = *pState;
                blockState.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, false);
                blockState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nStreamUnpackedSize);
                // Remove per-file CRC from the block state — CRC belongs to individual files, not the solid block
                blockState.mapProperties.remove(XBinary::FPART_PROP_CRC_TYPE);
                blockState.mapProperties.remove(XBinary::FPART_PROP_RESULTCRC);

                QIODevice *pSolidDevice = XBinary::createFileBuffer(nStreamUnpackedSize, pPdStruct);
                blockState.pDeviceOutput = pSolidDevice;

                bool bBlockResult = multiDecompress(&blockState, pPdStruct);
                if (pSolidDevice && bBlockResult) {
                    m_mapSolidCache.insert(sCacheKey, pSolidDevice);
                } else {
                    XBinary::freeFileBuffer(&pSolidDevice);
                }
            }

            if (m_mapSolidCache.contains(sCacheKey)) {
                qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, (qint64)0).toLongLong();
                qint64 nDecompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();
                QIODevice *pSolidDevice = m_mapSolidCache.value(sCacheKey);
                if (pState->pDeviceOutput && pSolidDevice) {
                    pSolidDevice->seek(nSubstreamOffset);
                    QByteArray baSubStream = pSolidDevice->read(nDecompressedSize);
                    pState->pDeviceOutput->seek(0);
                    qint64 nWritten = pState->pDeviceOutput->write(baSubStream);
                    pState->nCountOutput = nWritten;
                    bResult = (nWritten == nDecompressedSize);
                    if (bResult) {
                        // Verify CRC of the extracted sub-stream
                        XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
                        QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                        bResult = checkCRC(crcType, varCRC, pState->pDeviceOutput, pPdStruct);
                    }
                }
            }
        }
    } else {
        // Multi-method, non-solid: apply decompression layers in reverse order (outermost first)
        QIODevice *pDeviceInput = nullptr;
        QIODevice *pDeviceOutput = nullptr;
        qint64 nIntermediateSize = 0;

        qint64 nStreamSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE, 0).toLongLong();
        if (nStreamSize == 0) {
            nStreamSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
        }

        for (qint32 i = nNumberOfMethods - 1; i >= 0; i--) {
            XBinary::DATAPROCESS_STATE state = *pState;

            XBinary::FPART_PROP fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD;
            XBinary::FPART_PROP fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES;
            XBinary::FPART_PROP fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE;
            XBinary::FPART_PROP fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE;

            if (i == 2) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD3))
                    fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES3))
                    fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE3))
                    fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE3))
                    fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE3;
            } else if (i == 1) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2))
                    fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES2))
                    fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE2))
                    fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2))
                    fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE2;
            }

            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD,       pState->mapProperties.value(fpHandleMethod));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, pState->mapProperties.value(fpCompressProperties));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE,     pState->mapProperties.value(fpCompressedSize));
            state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE,   pState->mapProperties.value(fpUncompressedSize));

            state.pDeviceInput = nullptr;
            state.pDeviceOutput = nullptr;

            if (i == nNumberOfMethods - 1) {
                state.pDeviceInput = pState->pDeviceInput;
            } else if (i == 0) {
                state.pDeviceOutput = pState->pDeviceOutput;
            }

            if (state.pDeviceInput == nullptr) {
                state.pDeviceInput = pDeviceInput;
                state.nInputOffset = 0;
                state.nInputLimit = nIntermediateSize;
            }

            if (state.pDeviceOutput == nullptr) {
                pDeviceOutput = XBinary::createFileBuffer(nStreamSize, pPdStruct);
                state.pDeviceOutput = pDeviceOutput;
            }

            bResult = decompress(&state, pPdStruct);
            nIntermediateSize = state.nCountOutput;

            if (pDeviceInput) {
                XBinary::freeFileBuffer(&pDeviceInput);
            }

            if (pDeviceOutput) {
                pDeviceInput = pDeviceOutput;
                pDeviceOutput = nullptr;
            }

            if (!bResult) {
                break;
            }
        }

        if (pDeviceInput) {
            XBinary::freeFileBuffer(&pDeviceInput);
        }

        if (pDeviceOutput) {
            XBinary::freeFileBuffer(&pDeviceOutput);
        }

        if (bResult && pState->pDeviceOutput) {
            // Verify CRC of the final decompressed result
            XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
            QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
            bResult = checkCRC(crcType, varCRC, pState->pDeviceOutput, pPdStruct);
        }
    }

    return bResult;
}

bool XDecompress::decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (pState->pDeviceInput) {
        pState->pDeviceInput->seek(pState->nInputOffset);
    }

    if (pState->pDeviceOutput) {
        pState->pDeviceOutput->seek(0);
    }

    XBinary::HANDLE_METHOD compressMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    QByteArray baProperty = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
    qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();

    // state.compressMethod = (XBinary::HANDLE_METHOD)fpart.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD1, XBinary::HANDLE_METHOD_UNKNOWN).toUInt();
    // state.nUncompressedSize = fpart.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

    if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
        // For STORE after AES decryption, the input may include AES padding bytes.
        // Cap input to the actual uncompressed size to avoid copying padding.
        if (nUncompressedSize > 0 && nUncompressedSize < pState->nInputLimit) {
            pState->nInputLimit = nUncompressedSize;
        }
        bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BZIP2) {
        bResult = XBZIP2Decoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZMA) {
        if (!baProperty.isEmpty()) {
            bResult = XLZMADecoder::decompress(pState, baProperty, pPdStruct);
        } else {
            bResult = XLZMADecoder::decompress(pState, pPdStruct);
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZMA2) {
        if (!baProperty.isEmpty()) {
            bResult = XLZMADecoder::decompressLZMA2(pState, baProperty, pPdStruct);
        } else {
            bResult = XLZMADecoder::decompressLZMA2(pState, pPdStruct);
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_BCJ) {
        // x86 BCJ inverse filter (7-Zip compatible, matching 7-zip Bra86.c reference)
        // Implements the stateful BCJ decode including the prevMask double-transform path.
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData = pState->pDeviceInput->read(pState->nInputLimit);
            pState->nCountInput = baData.size();

            qint32 nSize = baData.size();
            unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data());

            if (nSize >= 5) {
                qint32 nPos = 0;
                quint32 nMask = 0;  // prevMask state
                // pc is the "return address" of the current call/jmp: pc = nPos + 5
                // (offset of byte AFTER the 5-byte instruction). Tracked implicitly as nPos+5.

                while (nPos <= nSize - 5) {
                    unsigned char b = pData[nPos];
                    if ((b & 0xFE) != 0xE8) {
                        // Not E8/E9 — advance; if in outer-scan mode, shift mask
                        if (nMask != 0) {
                            nMask >>= 1;
                        }
                        nPos++;
                    } else {
                        // E8 or E9 found at nPos. Read 4-byte operand.
                        quint32 v = (quint32)pData[nPos + 1]
                                    | ((quint32)pData[nPos + 2] << 8)
                                    | ((quint32)pData[nPos + 3] << 16)
                                    | ((quint32)pData[nPos + 4] << 24);

                        if (nMask == 0) {
                            // main_loop / a3 path: plain decode
                            quint32 vt = v + 0x01000000u;
                            if (!(vt & 0xFE000000u)) {
                                quint32 nPc = (quint32)(nPos + 5);
                                v = vt - nPc;
                                v &= 0x01FFFFFFu;
                                v -= 0x01000000u;
                                pData[nPos + 1] = (unsigned char)(v);
                                pData[nPos + 2] = (unsigned char)(v >> 8);
                                pData[nPos + 3] = (unsigned char)(v >> 16);
                                pData[nPos + 4] = (unsigned char)(v >> 24);
                                nPos += 5;
                                // nMask stays 0 (main_loop resumes)
                            } else {
                                // Skip: mask |= 4 for next outer scan
                                nMask = 4u;
                                nPos++;
                            }
                        } else {
                            // Outer-scan / m-path: prevMask is active
                            // At this point nMask reflects the mask BEFORE the >>= 1 in m-path
                            if (nMask > 4u || nMask == 3u) {
                                // Skip: still inside protected zone
                                nMask = (nMask >> 1u) | 4u;
                                nPos++;
                            } else {
                                // Shift mask (as done in m1/m2): mask >>= 1
                                nMask >>= 1u;
                                // Check the byte at operand[nMask] — this is p[mask] in the reference
                                unsigned char bCheckByte = pData[nPos + 1 + (qint32)(nMask)];
                                if (((bCheckByte + 1u) & 0xFEu) == 0u) {
                                    // BR86_NEED_CONV_FOR_MS_BYTE: byte is 0x00 or 0xFF → skip
                                    nMask = (nMask >> 1u) | 4u;
                                    nPos++;
                                } else {
                                    // Double-transform decode
                                    quint32 vt = v + 0x01000000u;
                                    if (!(vt & 0xFE000000u)) {
                                        quint32 nPc = (quint32)(nPos + 5);
                                        // First decode: v -= pc (via vt)
                                        v = vt - nPc;
                                        // Check byte at mask*8 bit position (mask <<= 3)
                                        quint32 nMaskBits = nMask << 3u; // bytes → bits
                                        unsigned char bMsb = (unsigned char)((v >> nMaskBits) & 0xFFu);
                                        if (((bMsb + 1u) & 0xFEu) == 0u) {
                                            // BR86_NEED_CONV_FOR_MS_BYTE: XOR and second decode
                                            v ^= (((quint32)0x100u << nMaskBits) - 1u);
                                            v -= nPc;
                                        }
                                        v &= 0x01FFFFFFu;
                                        v -= 0x01000000u;
                                        pData[nPos + 1] = (unsigned char)(v);
                                        pData[nPos + 2] = (unsigned char)(v >> 8);
                                        pData[nPos + 3] = (unsigned char)(v >> 16);
                                        pData[nPos + 4] = (unsigned char)(v >> 24);
                                        nPos += 5;
                                        nMask = 0u;
                                    } else {
                                        // Initial check failed: skip
                                        nMask = (nMask >> 1u) | 4u;
                                        nPos++;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            qint64 nWritten = pState->pDeviceOutput->write(baData);
            pState->nCountOutput = nWritten;
            bResult = (nWritten == (qint64)baData.size());
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARM64_BCJ) {
        // ARM64 BCJ inverse filter (7-Zip compatible, from Bra.c BranchConv_ARM64).
        // Handles two instruction types:
        //   1. BL (opcode 0x94??????): converts absolute imm26 back to PC-relative
        //   2. ADRP (opcode 0x90??????): converts absolute page-number back to PC-relative
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData = pState->pDeviceInput->read(pState->nInputLimit);
            pState->nCountInput = baData.size();

            qint32 nSize = baData.size() & ~3;  // must be 4-byte aligned
            unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data());

            const quint32 kFlag = (quint32)1 << 20;              // 0x00100000
            const quint32 kMask = ((quint32)1 << 24) - (kFlag << 1);  // 0x00E00000

            qint32 nPos = 0;
            while (nPos < nSize) {
                // Read 4-byte LE word
                quint32 v = (quint32)pData[nPos]       |
                            ((quint32)pData[nPos + 1] << 8)  |
                            ((quint32)pData[nPos + 2] << 16) |
                            ((quint32)pData[nPos + 3] << 24);

                if (((v - 0x94000000) & 0xfc000000) == 0) {
                    // BL instruction: decode absolute → relative
                    // Decode: rel = stored_abs - pos/4
                    quint32 c = (quint32)(nPos) >> 2;
                    v -= c;
                    v &= 0x03ffffff;
                    v |= 0x94000000;
                } else {
                    quint32 vt = v - 0x90000000;
                    if ((vt & 0x9f000000) == 0) {
                        // ADRP instruction: decode absolute → relative
                        vt += kFlag;
                        if (!(vt & kMask)) {
                            // z = imm_hi/imm_lo/Rd fields packed from modified vt
                            quint32 z = (vt & 0xffffffe0) | (vt >> 26);
                            // c = PC page number
                            quint32 c = ((quint32)nPos >> 9) & ~(quint32)7;
                            // Decode: z -= c
                            z -= c;
                            // Reconstruct ADRP
                            v &= 0x0000001f;  // keep Rd (bits 4:0)
                            v |= 0x90000000;  // ADRP base
                            v |= z << 26;     // immhi/immlo packed in high bits of z
                            v |= 0x00ffffe0 & ((z & ((kFlag << 1) - 1)) - kFlag);
                        }
                    }
                }

                pData[nPos]     = (unsigned char)(v);
                pData[nPos + 1] = (unsigned char)(v >> 8);
                pData[nPos + 2] = (unsigned char)(v >> 16);
                pData[nPos + 3] = (unsigned char)(v >> 24);
                nPos += 4;
            }

            qint64 nWritten = pState->pDeviceOutput->write(baData);
            pState->nCountOutput = nWritten;
            bResult = (nWritten == (qint64)baData.size());
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_XZ) {
        bResult = XLZMADecoder::decompressXZ(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPMD7) {
        bResult = XPPMdDecoder::decompressPPMD7(pState, baProperty, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPMD8) {
        bResult = XPPMdDecoder::decompressPPMD8(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DEFLATE) {
        bResult = XDeflateDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DEFLATE64) {
        bResult = XDeflateDecoder::decompress64(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT214_8) {
        bResult = XIT214Decoder::decompress(pState, 8, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT214_16) {
        bResult = XIT214Decoder::decompress(pState, 16, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT215_8) {
        bResult = XIT214Decoder::decompress(pState, 8, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT215_16) {
        bResult = XIT214Decoder::decompress(pState, 16, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_4KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_4KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_8KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_8KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_SHRINK) {
        bResult = XShrinkDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_1) {
        bResult = XReduceDecoder::decompress(pState, 1, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_2) {
        bResult = XReduceDecoder::decompress(pState, 2, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_3) {
        bResult = XReduceDecoder::decompress(pState, 3, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_4) {
        bResult = XReduceDecoder::decompress(pState, 4, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZLIB) {
        bResult = XDeflateDecoder::decompress_zlib(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZW_PDF) {
        bResult = XLZWDecoder::decompress_pdf(pState, pPdStruct);
        // bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ASCII85) {
        bResult = XASCII85Decoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH5) {
        bResult = XLZHDecoder::decompress(pState, 5, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH6) {
        bResult = XLZHDecoder::decompress(pState, 6, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH7) {
        bResult = XLZHDecoder::decompress(pState, 7, pPdStruct);
    } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
        bool bIsSolid = false;
        rar_Unpack rarUnpack;
        rarUnpack.setDevices(pState->pDeviceInput, pState->pDeviceOutput);
        qint32 nInit = rarUnpack.Init(nWindowSize, bIsSolid);

        if (nInit > 0) {
            rarUnpack.SetDestSize(nUncompressedSize);

            if (compressMethod == XBinary::HANDLE_METHOD_RAR_15) {
                rarUnpack.Unpack15(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_20) {
                rarUnpack.Unpack20(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_29) {
                rarUnpack.Unpack29(bIsSolid, pPdStruct);
            } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_50) || (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                rarUnpack.Unpack5(bIsSolid, pPdStruct);
            }

            bResult = true;
        }
    } else if ((compressMethod == XBinary::HANDLE_METHOD_ZIP_AES) || (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES128) ||
               (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES192 || (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES256))) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();

        if (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES) {
            compressMethod = XBinary::HANDLE_METHOD_ZIP_AES256;
        }

        bResult = XAESDecoder::decrypt(pState, sPassword, compressMethod, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZIPCRYPTO) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XZipCryptoDecoder::decrypt(pState, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_7Z_AES) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XAESDecoder::decrypt(pState, baProperty, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_RAR5_AES) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XAESDecoder::decryptRar5(pState, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BCJ2) {
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            XBinary::HANDLE_METHOD cmMain = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD4, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropMain = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
            qint64 nMainUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE4, (qint64)0).toLongLong();

            XBinary::HANDLE_METHOD cmCall = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropCall = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES2).toByteArray();
            qint64 nCallUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)0).toLongLong();
            qint64 nCallOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET2, (qint64)0).toLongLong();
            qint64 nCallSize   = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE2,   (qint64)0).toLongLong();

            XBinary::HANDLE_METHOD cmJmp = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD3, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropJmp = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES3).toByteArray();
            qint64 nJmpUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE3, (qint64)0).toLongLong();
            qint64 nJmpOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET3, (qint64)0).toLongLong();
            qint64 nJmpSize   = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE3,   (qint64)0).toLongLong();

            qint64 nRangeOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET4, (qint64)0).toLongLong();
            qint64 nRangeSize   = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE4,   (qint64)0).toLongLong();

            qint64 nOutputSize = nUncompressedSize;  // BCJ2 total output set by multiDecompress

            // nCallUnpack / nJmpUnpack may be 0 when the data contains no CALL/JMP instructions
            // (e.g. pure image or text files). Only require nMainUnpack > 0 and nOutputSize > 0.
            if (nMainUnpack > 0 && nOutputSize > 0) {
                QByteArray baMain, baCall, baJmp;
                baMain.resize((qint32)nMainUnpack);
                baCall.resize((qint32)nCallUnpack);
                baJmp.resize((qint32)nJmpUnpack);

                struct _BCJ2Task {
                    qint64 nOffset;
                    qint64 nSize;
                    qint64 nOutputSize;
                    XBinary::HANDLE_METHOD cm;
                    QByteArray *pOutput;
                    const QByteArray *pProperty;
                };

                _BCJ2Task tasks[3];
                tasks[0] = {pState->nInputOffset, pState->nInputLimit, nMainUnpack, cmMain, &baMain, &baPropMain};
                tasks[1] = {nCallOffset, nCallSize, nCallUnpack, cmCall, &baCall, &baPropCall};
                tasks[2] = {nJmpOffset,  nJmpSize,  nJmpUnpack,  cmJmp,  &baJmp,  &baPropJmp};

                bool bLZMAOk = true;
                for (qint32 nTask = 0; nTask < 3 && bLZMAOk && XBinary::isPdStructNotCanceled(pPdStruct); nTask++) {
                    QBuffer outBuf(tasks[nTask].pOutput);
                    if (!outBuf.open(QIODevice::WriteOnly)) {
                        bLZMAOk = false;
                        break;
                    }
                    XBinary::DATAPROCESS_STATE dpState = {};
                    dpState.pDeviceInput     = pState->pDeviceInput;
                    dpState.pDeviceOutput    = &outBuf;
                    dpState.nInputOffset     = tasks[nTask].nOffset;
                    dpState.nInputLimit      = tasks[nTask].nSize;
                    dpState.nProcessedOffset = 0;
                    dpState.nProcessedLimit  = tasks[nTask].nOutputSize;
                    dpState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD,       (quint32)tasks[nTask].cm);
                    dpState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, *tasks[nTask].pProperty);
                    dpState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE,   tasks[nTask].nOutputSize);
                    bLZMAOk = decompress(&dpState, pPdStruct);
                    outBuf.close();
                }

                if (bLZMAOk && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    // Range coder stream is raw (not LZMA-compressed), read directly
                    pState->pDeviceInput->seek(nRangeOffset);
                    QByteArray baRange = pState->pDeviceInput->read(nRangeSize);
                    if (baRange.size() == (qint32)nRangeSize) {
                        QBuffer mainBuf(&baMain);
                        QBuffer callBuf(&baCall);
                        QBuffer jmpBuf(&baJmp);
                        QBuffer rangeBuf(&baRange);
                        if (mainBuf.open(QIODevice::ReadOnly)  &&
                            callBuf.open(QIODevice::ReadOnly)   &&
                            jmpBuf.open(QIODevice::ReadOnly)    &&
                            rangeBuf.open(QIODevice::ReadOnly)) {
                            bResult = XBCJ2Decoder::decompress(&mainBuf, &callBuf, &jmpBuf, &rangeBuf,
                                                               pState->pDeviceOutput, nOutputSize, pPdStruct);
                            pState->nCountOutput = nOutputSize;
                        }
                    }
                }
            }
        }
    } else {
#ifdef QT_DEBUG
        qDebug() << "Unknown compression method" << XBinary::handleMethodToString(compressMethod);
#endif
        emit errorMessage(QString("%1: %2").arg(tr("Unknown compression method"), XBinary::handleMethodToString(compressMethod)));
    }

    return bResult;
}

QByteArray XDecompress::decomressToByteArray(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    if (pDevice) {
        QBuffer buffer(&baResult);

        if (buffer.open(QIODevice::ReadWrite)) {
            XBinary::DATAPROCESS_STATE state = {};
            state.pDeviceInput = pDevice;
            state.pDeviceOutput = &buffer;
            state.nInputOffset = nOffset;
            state.nInputLimit = nSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;
            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

            multiDecompress(&state, pPdStruct);

            buffer.close();
        }
    }

    return baResult;
}

qint64 XDecompress::getCompressedDataSize(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if (nSize == -1) {
        nSize = pDevice->size() - nOffset;
    }

    qint64 nResult = 0;

    if (pDevice) {
        XBinary::DATAPROCESS_STATE state = {};
        state.pDeviceInput = pDevice;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = nOffset;
        state.nInputLimit = nSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        multiDecompress(&state, pPdStruct);

        nResult = state.nCountInput;
    }

    return nResult;
}
