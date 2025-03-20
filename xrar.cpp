/* Copyright (c) 2017-2024 hors<horsicq@gmail.com>
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
#include "xrar.h"

XRar::XRar(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRar::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > 20)  // TODO
    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "'RE~^'", 0, pPdStruct) || compareSignature(&memoryMap, "'Rar!'1A07", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XRar::isValid(QIODevice *pDevice)
{
    XRar xrar(pDevice);

    return xrar.isValid();
}

QString XRar::getVersion()
{
    QString sResult;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

    // TODO more
    if (compareSignature(&memoryMap, "'RE~^'")) {
        sResult = "1.4";
    } else if (compareSignature(&memoryMap, "'Rar!'1A0700")) {
        sResult = "1.5-4.X";
    } else if (compareSignature(&memoryMap, "'Rar!'1A070100")) {
        sResult = "5.X-7.X";
    }

    return sResult;
}

quint64 XRar::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    qint64 nFileHeaderSize = 0;
    qint32 nVersion = 0;

    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);  // TODO rewrite
        if (compareSignature(&memoryMap, "'Rar!'1A0700")) {
            nFileHeaderSize = 7;
            nVersion = 4;
        } else if (compareSignature(&memoryMap, "'Rar!'1A070100")) {
            nFileHeaderSize = 8;
            nVersion = 5;
        }
    }

    if (nFileHeaderSize) {
        qint64 nCurrentOffset = nFileHeaderSize;

        if (nVersion == 4) {
            while (!(pPdStruct->bIsStop)) {
                GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

                if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {
                    if (genericBlock.nType == BLOCKTYPE4_FILE) {
                        FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);

                        nCurrentOffset += fileBlock.genericBlock4.nHeaderSize + fileBlock.packSize;
                        nResult++;
                    } else {
                        nCurrentOffset += genericBlock.nHeaderSize;
                    }
                } else {
                    break;
                }

                if (genericBlock.nType == 0x7B) {  // END
                    break;
                }
            }
        }
        if (nVersion == 5) {
            while (!(pPdStruct->bIsStop)) {
                GENERICHEADER5 genericHeader = XRar::readGenericHeader5(nCurrentOffset);

                if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
                    if (genericHeader.nType == HEADERTYPE5_FILE) {
                        nResult++;
                    }

                    nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
                } else {
                    break;
                }

                if (genericHeader.nType == 5) {  // END
                    break;
                }
            }
        }
    }

    return nResult;
}

QList<XArchive::RECORD> XRar::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    QList<XArchive::RECORD> listResult;

    qint64 nFileHeaderSize = 0;
    qint32 nVersion = 0;

    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);  // TODO rewrite
        if (compareSignature(&memoryMap, "'Rar!'1A0700")) {
            nFileHeaderSize = 7;
            nVersion = 4;
        } else if (compareSignature(&memoryMap, "'Rar!'1A070100")) {
            nFileHeaderSize = 8;
            nVersion = 5;
        }
    }

    if (nFileHeaderSize) {
        qint64 nCurrentOffset = nFileHeaderSize;

        if (nVersion == 4) {
            while (!(pPdStruct->bIsStop)) {
                GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

                if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {
                    if (genericBlock.nType == BLOCKTYPE4_FILE) {
                        FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);

                        XArchive::RECORD record = {};
                        record.sFileName = fileBlock4.sFileName;
                        record.nCRC32 = fileBlock4.genericBlock4.nCRC16;
                        record.nDataOffset = nCurrentOffset + fileBlock4.genericBlock4.nHeaderSize;
                        record.nCompressedSize = fileBlock4.packSize;
                        record.nUncompressedSize = fileBlock4.unpSize;
                        record.nHeaderOffset = nCurrentOffset;
                        record.nHeaderSize = fileBlock4.genericBlock4.nHeaderSize;

                        if (fileBlock4.method == RAR_METHOD_STORE) {
                            record.compressMethod = COMPRESS_METHOD_STORE;
                        } else {
                            record.compressMethod = COMPRESS_METHOD_RAR;
                        }

                        listResult.append(record); // TODO large files

                        nCurrentOffset += fileBlock4.genericBlock4.nHeaderSize + fileBlock4.packSize;
                    } else {
                        nCurrentOffset += genericBlock.nHeaderSize;
                    }
                } else {
                    break;
                }

                if (genericBlock.nType == 0x7B) {  // END
                    break;
                }
            }
        }
        if (nVersion == 5) {
            while (!(pPdStruct->bIsStop)) {
                GENERICHEADER5 genericHeader = XRar::readGenericHeader5(nCurrentOffset);

                if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
                    if (genericHeader.nType == HEADERTYPE5_FILE) {
                        // TODO
                    }

                    nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;;
                } else {
                    break;
                }

                if (genericHeader.nType == 5) {  // END
                    break;
                }
            }
        }
    }

    return listResult;
}

QString XRar::getFileFormatExt()
{
    return "rar";
}

qint64 XRar::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XRar::getFileFormatString()
{
    QString sResult;

    sResult = QString("RAR(%1)").arg(getVersion());
    // TODO more info

    return sResult;
}

QString XRar::blockType4ToString(BLOCKTYPE4 type)
{
    QString sResult;

    switch (type) {
        case BLOCKTYPE4_MARKER: sResult = QString("Marker block"); break;
        case BLOCKTYPE4_ARCHIVE: sResult = QString("Archive header"); break;
        case BLOCKTYPE4_FILE: sResult = QString("File header"); break;
        case BLOCKTYPE4_COMMENT: sResult = QString("Comment header"); break;
        case BLOCKTYPE4_EXTRA: sResult = QString("Extra information"); break;
        case BLOCKTYPE4_SUBBLOCK: sResult = QString("Subblock"); break;
        case BLOCKTYPE4_RECOVERY: sResult = QString("Recovery record"); break;
        case BLOCKTYPE4_AUTH: sResult = QString("Archive authentication"); break;
        case BLOCKTYPE4_SUBBLOCK_NEW: sResult = QString("Subblock"); break;
        case BLOCKTYPE4_END: sResult = QString("End of archive"); break;
        default: sResult = QString("Unknown (%1)").arg(type, 0, 16);
    }

    return sResult;
}

QString XRar::headerType5ToString(HEADERTYPE5 type)
{
    QString sResult;

    switch (type) {
        case HEADERTYPE5_MAIN: sResult = QString("Main archive header"); break;
        case HEADERTYPE5_FILE: sResult = QString("File header"); break;
        case HEADERTYPE5_SERVICE: sResult = QString("Service header"); break;
        case HEADERTYPE5_ENCRYPTION: sResult = QString("Archive encryption header"); break;
        case HEADERTYPE5_ENDARC: sResult = QString("End of archive header"); break;
        default: sResult = QString("Unknown (%1)").arg(type, 0, 16); break;
    }

    return sResult;
}

XRar::FILEHEADER5 XRar::readFileHeader5(qint64 nOffset)
{
    FILEHEADER5 result = {};

    // TODO

    return result;
}

XRar::FILEBLOCK4 XRar::readFileBlock4(qint64 nOffset)
{
    FILEBLOCK4 result = {};

    qint64 nCurrentOffset = nOffset;

    // Read header fields
    result.genericBlock4 = readGenericBlock4(nCurrentOffset);
    nCurrentOffset += 7;

    // Continue reading file block specific fields
    result.packSize = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.unpSize = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.hostOS = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.fileCRC = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.fileTime = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.unpVer = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.method = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.nameSize = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;
    result.fileAttr = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;

    // Read high bits of pack/unpack size if large file flag is set
    if (result.genericBlock4.nFlags & RAR4_FILE_LARGE) {
        result.highPackSize = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.highUnpSize = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
    } else {
        result.highPackSize = 0;
        result.highUnpSize = 0;
    }

    // Read filename
    if (result.nameSize > 0) {
        QByteArray nameData = read_array(nCurrentOffset, result.nameSize);
        nCurrentOffset += result.nameSize;

        // Handle Unicode filenames
        if (result.genericBlock4.nFlags & RAR4_FILE_UNICODE_FILENAME) {
            // This is a simplified approach for Unicode filename handling
            // Real implementation would need more complex parsing of the RarUnicodeFileName format
            result.sFileName = decodeRarUnicodeName(nameData);
        } else {
            result.sFileName = QString::fromLatin1(nameData);
        }
    }

    return result;
}

XRar::GENERICBLOCK4 XRar::readGenericBlock4(qint64 nOffset)
{
    GENERICBLOCK4 result = {};

    qint64 nCurrentOffset = nOffset;

    result.nCRC16 = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;
    result.nType = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.nFlags = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;
    result.nHeaderSize = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;

    // if ((result.nType != BLOCKTYPE4_END) && (result.nHeaderSize >= 11)) {
    //     result.nDataSize = read_uint32(nCurrentOffset);
    // }

    return result;
}

QString XRar::decodeRarUnicodeName(const QByteArray &nameData)
{
    // This is a complex process in RAR - simplified version here
    // Real implementation would need to follow the RarUnicodeFileName format

    // Try UTF-8 first
    QString result = QString::fromUtf8(nameData);
    if (!result.contains(QChar(0xFFFD))) { // No replacement character
        return result;
    }

    // Fall back to system locale
    return QString::fromLocal8Bit(nameData);
}

QList<XBinary::MAPMODE> XRar::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XRar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    XBinary::_MEMORY_MAP result = {};

    result.nBinarySize = getSize();

    qint64 nFileHeaderSize = 0;
    qint32 nVersion = 0;

    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);  // TODO rewrite
        if (compareSignature(&memoryMap, "'Rar!'1A0700")) {
            nFileHeaderSize = 7;
            nVersion = 4;
        } else if (compareSignature(&memoryMap, "'Rar!'1A070100")) {
            nFileHeaderSize = 8;
            nVersion = 5;
        }
    }

    qint64 nMaxOffset = 0;

    if (nFileHeaderSize) {
        qint32 nIndex = 0;

        {
            _MEMORY_RECORD record = {};

            record.nIndex = nIndex++;
            record.type = MMT_HEADER;
            record.nOffset = 0;
            record.nSize = nFileHeaderSize;
            record.nAddress = -1;
            record.sName = tr("Header");

            result.listRecords.append(record);
        }

        qint64 nCurrentOffset = nFileHeaderSize;

        if (nVersion == 4) {
            while (!(pPdStruct->bIsStop)) {
                GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

                if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {

                    {
                        _MEMORY_RECORD record = {};

                        record.nIndex = nIndex++;
                        record.type = MMT_DATA;
                        record.nOffset = nCurrentOffset;
                        record.nSize = genericBlock.nHeaderSize;
                        record.nAddress = -1;
                        record.sName = blockType4ToString((BLOCKTYPE4)genericBlock.nType);

                        nMaxOffset = qMax(nMaxOffset, nCurrentOffset + record.nSize);

                        result.listRecords.append(record);
                    }

                    nCurrentOffset += genericBlock.nHeaderSize;

                    if (genericBlock.nType == BLOCKTYPE4_FILE) {
                        FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);

                        {
                            _MEMORY_RECORD record = {};

                            record.nIndex = nIndex++;
                            record.type = MMT_DATA;
                            record.nOffset = nCurrentOffset;
                            record.nSize = fileBlock4.packSize;
                            record.nAddress = -1;
                            record.sName = "FileName";

                            nMaxOffset = qMax(nMaxOffset, nCurrentOffset + fileBlock4.packSize);

                            result.listRecords.append(record);
                        }

                        nCurrentOffset += fileBlock4.packSize;
                    }
                } else {
                    break;
                }

                if (genericBlock.nType == 0x7B) {  // END
                    break;
                }
            }
        }
        if (nVersion == 5) {
            while (!(pPdStruct->bIsStop)) {
                GENERICHEADER5 genericHeader = XRar::readGenericHeader5(nCurrentOffset);

                if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
                    {
                        _MEMORY_RECORD record = {};

                        record.nIndex = nIndex++;
                        record.type = MMT_DATA;
                        record.nOffset = nCurrentOffset;
                        record.nSize = genericHeader.nHeaderSize;
                        record.nAddress = -1;
                        record.sName = headerType5ToString((HEADERTYPE5)genericHeader.nType);

                        nMaxOffset = qMax(nMaxOffset, nCurrentOffset + record.nSize);

                        result.listRecords.append(record);
                    }
                    {
                        _MEMORY_RECORD record = {};

                        record.nIndex = nIndex++;
                        record.type = MMT_DATA;
                        record.nOffset = nCurrentOffset + genericHeader.nHeaderSize;
                        record.nSize = genericHeader.nDataSize;
                        record.nAddress = -1;
                        record.sName = "DATA";

                        nMaxOffset = qMax(nMaxOffset, nCurrentOffset + record.nSize);

                        result.listRecords.append(record);
                    }

                    nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
                } else {
                    break;
                }

                if (genericHeader.nType == 5) {  // END
                    break;
                }
            }
        }

        if ((nMaxOffset > 0) && (nMaxOffset < result.nBinarySize)) {
            _MEMORY_RECORD recordOverlay = {};
            recordOverlay.nAddress = -1;
            recordOverlay.nOffset = nMaxOffset;
            recordOverlay.nSize = result.nBinarySize - nMaxOffset;
            recordOverlay.nIndex++;
            recordOverlay.type = MMT_OVERLAY;
            recordOverlay.sName = tr("Overlay");

            result.listRecords.append(recordOverlay);
        }
    }

    return result;
}

XBinary::FT XRar::getFileType()
{
    return FT_RAR;
}

XRar::GENERICHEADER5 XRar::readGenericHeader5(qint64 nOffset)
{
    GENERICHEADER5 result = {};

    qint64 nCurrentOffset = nOffset;
    PACKED_UINT packeInt = {};

    result.nCRC32 = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result._nHeaderSize = packeInt.nValue;
    result.nHeaderSize = result._nHeaderSize + packeInt.nByteSize;
    nCurrentOffset += packeInt.nByteSize;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nType = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nFlags = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    if (result.nFlags & 0x0001) {
        packeInt = read_uleb128(nCurrentOffset, 4);
        result.nExtraAreaSize = packeInt.nValue;
        nCurrentOffset += packeInt.nByteSize;
    }

    if (result.nFlags & 0x0002) {
        packeInt = read_uleb128(nCurrentOffset, 8);
        result.nDataSize = packeInt.nValue;
    }

    return result;
}
