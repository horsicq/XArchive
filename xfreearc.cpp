/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#include "xfreearc.h"

XBinary::XCONVERT _TABLE_XFREEARC_STRUCTID[] = {
    {XFREEARC::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XFREEARC::STRUCTID_ARCHIVE_HEADER, "ARCHIVE_HEADER", QString("Archive Header")},
    {XFREEARC::STRUCTID_BLOCK, "BLOCK", QString("Block")},
};

XFREEARC::XFREEARC(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XFREEARC::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    // FreeARC archive: starts with "ArC\x01" (4 bytes)
    // Minimum: signature(4) + header_flags(2) + version(2) = 8 bytes
    if (getSize() >= FREEARC_HEADER_SIZE) {
        quint32 nMagic = read_uint32(0, false);

        if (nMagic == FREEARC_MAGIC) {
            // Verify there is at least one block after the header
            // The second "ArC\x01" should appear at offset 8
            if (getSize() >= FREEARC_HEADER_SIZE + FREEARC_SIGNATURE_SIZE) {
                quint32 nBlockMagic = read_uint32(FREEARC_HEADER_SIZE, false);

                if (nBlockMagic == FREEARC_MAGIC) {
                    bResult = true;
                }
            }
        }
    }

    return bResult;
}

bool XFREEARC::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFREEARC xfreearc(pDevice);

    return xfreearc.isValid();
}

qint64 XFREEARC::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XFREEARC::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XFREEARC::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

XBinary::FT XFREEARC::getFileType()
{
    return FT_FREEARC;
}

QString XFREEARC::getFileFormatExt()
{
    return "arc";
}

QString XFREEARC::getFileFormatExtsString()
{
    return "FreeARC (*.arc)";
}

QString XFREEARC::getMIMEString()
{
    return "application/x-freearc";
}

QString XFREEARC::getVersion()
{
    // Read version bytes from the archive header
    if (getSize() >= FREEARC_HEADER_SIZE) {
        quint8 nVersionMajor = read_uint8(6);
        quint8 nVersionMinor = read_uint8(7);

        return QString("%1.%2").arg(nVersionMajor).arg(nVersionMinor);
    }

    return QString();
}

QString XFREEARC::getArch()
{
    return QString();
}

XBinary::ENDIAN XFREEARC::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XFREEARC::getMode()
{
    return MODE_DATA;
}

bool XFREEARC::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    Q_UNUSED(pPdStruct)

    // TODO: FreeARC unpacking requires parsing the directory block,
    // which is itself compressed (typically with LZMA).
    // For now, return false to indicate unpacking is not yet supported.
    if (pState) {
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;
    }

    return false;
}

XBinary::ARCHIVERECORD XFREEARC::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    // TODO: implement when directory block parsing is available
    XBinary::ARCHIVERECORD result = {};

    return result;
}

bool XFREEARC::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState) {
        pState->nCurrentIndex++;
        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XFREEARC::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    return true;
}

QString XFREEARC::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XFREEARC_STRUCTID, sizeof(_TABLE_XFREEARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XFREEARC::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XFREEARC_STRUCTID, sizeof(_TABLE_XFREEARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XFREEARC::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XFREEARC_STRUCTID, sizeof(_TABLE_XFREEARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XFREEARC::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        QList<BLOCK> listBlocks = getBlocks(pPdStruct);

        qint64 nRealSize = 0;

        if (!listBlocks.isEmpty()) {
            BLOCK lastBlock = listBlocks.last();
            nRealSize = lastBlock.nOffset + lastBlock.nSize;
        }

        _dataHeadersOptions.nID = STRUCTID_BLOCK;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nCount = listBlocks.count();
        _dataHeadersOptions.nSize = nRealSize;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_BLOCK) {
                QList<BLOCK> listBlocks = getBlocks(pPdStruct);

                for (qint32 i = 0; i < listBlocks.count(); i++) {
                    BLOCK block = listBlocks.at(i);

                    DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_BLOCK));
                    dataHeader.nSize = block.nSize;

                    dataHeader.listRecords.append(getDataRecord(0, 4, "Signature", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(4, 1, "Block Type", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                    qint32 nCompressorLen = block.sCompressor.length() + 1;

                    dataHeader.listRecords.append(getDataRecord(5, nCompressorLen, "Compressor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                    qint64 nDataSize = block.nSize - 5 - nCompressorLen;

                    if (nDataSize > 0) {
                        dataHeader.listRecords.append(
                            getDataRecord(5 + nCompressorLen, nDataSize, "Block Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    listResult.append(dataHeader);
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XFREEARC::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();

    // Archive header
    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = FREEARC_HEADER_SIZE;
        record.nVirtualAddress = -1;
        record.sName = tr("Header");

        listResult.append(record);
    }

    // Blocks as streams
    QList<BLOCK> listBlocks = getBlocks(pPdStruct);
    qint64 nMaxOffset = FREEARC_HEADER_SIZE;

    for (qint32 i = 0; i < listBlocks.count(); i++) {
        BLOCK block = listBlocks.at(i);

        if (nFileParts & FILEPART_STREAM) {
            FPART record = {};

            record.filePart = FILEPART_STREAM;
            record.nFileOffset = block.nOffset;
            record.nFileSize = block.nSize;
            record.nVirtualAddress = -1;
            record.sName = QString("%1 (%2)").arg(blockTypeToString(block.nType), block.sCompressor);

            listResult.append(record);
        }

        if (nFileParts & FILEPART_REGION) {
            FPART record = {};

            record.filePart = FILEPART_REGION;
            record.nFileOffset = block.nOffset;
            record.nFileSize = block.nSize;
            record.nVirtualAddress = -1;
            record.sName = QString("%1 (%2)").arg(blockTypeToString(block.nType), block.sCompressor);

            listResult.append(record);
        }

        qint64 nBlockEnd = block.nOffset + block.nSize;

        if (nBlockEnd > nMaxOffset) {
            nMaxOffset = nBlockEnd;
        }
    }

    // Add overlay if any
    if ((nFileParts & FILEPART_OVERLAY) && (nMaxOffset < nFileSize)) {
        FPART record = {};

        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nMaxOffset;
        record.nFileSize = nFileSize - nMaxOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Overlay");

        listResult.append(record);
    }

    return listResult;
}

QList<XFREEARC::BLOCK> XFREEARC::getBlocks(PDSTRUCT *pPdStruct)
{
    QList<BLOCK> listResult;

    qint64 nFileSize = getSize();
    qint64 nOffset = FREEARC_HEADER_SIZE;

    while ((nOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nFileSize - nOffset) < FREEARC_SIGNATURE_SIZE + 1) {
            break;
        }

        quint32 nMagic = read_uint32(nOffset, false);

        if (nMagic != FREEARC_MAGIC) {
            break;
        }

        BLOCK block = {};
        block.nOffset = nOffset;
        block.nType = read_uint8(nOffset + 4);
        block.sCompressor = _readCompressorString(nOffset + 5, nFileSize - nOffset - 5);

        // Find the next block to determine this block's size
        qint64 nNextBlock = _findNextBlock(nOffset + FREEARC_SIGNATURE_SIZE, nFileSize);

        if (nNextBlock == -1) {
            // Last block extends to end of file
            block.nSize = nFileSize - nOffset;
        } else {
            block.nSize = nNextBlock - nOffset;
        }

        listResult.append(block);

        if (nNextBlock == -1) {
            break;
        }

        nOffset = nNextBlock;
    }

    return listResult;
}

QString XFREEARC::blockTypeToString(quint8 nType)
{
    QString sResult;

    switch (nType) {
        case BLOCKTYPE_HEADER: sResult = "Header"; break;
        case BLOCKTYPE_DATA: sResult = "Data"; break;
        case BLOCKTYPE_DIR: sResult = "Directory"; break;
        case BLOCKTYPE_FOOTER: sResult = "Footer"; break;
        default: sResult = QString("Unknown (0x%1)").arg(nType, 2, 16, QChar('0')); break;
    }

    return sResult;
}

qint64 XFREEARC::_findNextBlock(qint64 nOffset, qint64 nFileSize)
{
    // Scan forward from nOffset looking for the next "ArC\x01" signature
    QByteArray baSignature;
    baSignature.append('A');
    baSignature.append('r');
    baSignature.append('C');
    baSignature.append('\x01');

    qint64 nPos = nOffset;

    while (nPos < nFileSize - 3) {
        qint64 nFound = find_signature(nPos, nFileSize - nPos, "41724301");

        if (nFound == -1) {
            return -1;
        }

        // Verify it looks like a valid block start (type byte should be reasonable)
        if ((nFound + 4) < nFileSize) {
            quint8 nType = read_uint8(nFound + 4);

            if ((nType == BLOCKTYPE_HEADER) || (nType == BLOCKTYPE_DATA) || (nType == BLOCKTYPE_DIR) || (nType == BLOCKTYPE_FOOTER)) {
                return nFound;
            }
        }

        nPos = nFound + 1;
    }

    return -1;
}

QString XFREEARC::_readCompressorString(qint64 nOffset, qint64 nMaxSize)
{
    QString sResult;

    if (nMaxSize > 0) {
        qint32 nReadSize = qMin(nMaxSize, (qint64)256);
        QByteArray baData = read_array(nOffset, nReadSize);

        qint32 nNullPos = baData.indexOf('\0');

        if (nNullPos >= 0) {
            sResult = QString::fromLatin1(baData.data(), nNullPos);
        }
    }

    return sResult;
}

QList<QString> XFREEARC::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'ArC'01");

    return listResult;
}

XBinary *XFREEARC::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XFREEARC(pDevice);
}

