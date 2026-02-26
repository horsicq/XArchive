/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "xmachofat.h"

XBinary::XCONVERT _TABLE_XMACHOFAT_STRUCTID[] = {
    {XMACHOFat::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XMACHOFat::STRUCTID_HEADER, "HEADER", QObject::tr("Header")},
    {XMACHOFat::STRUCTID_ARCHITECTURE, "ARCHITECTURE", QObject::tr("Architecture")},
};

XMACHOFat::XMACHOFat(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XMACHOFat::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= sizeof(XMACH_DEF::fat_header)) {
        quint32 nMagic = read_uint32(0);

        if ((nMagic == XMACH_DEF::S_FAT_MAGIC) || (nMagic == XMACH_DEF::S_FAT_CIGAM)) {
            quint32 nNumberOfRecords = read_uint32(offsetof(XMACH_DEF::fat_header, nfat_arch), isBigEndian());

            // Check for reasonable number of architectures (typically 1-10)
            if ((nNumberOfRecords > 0) && (nNumberOfRecords <= 20)) {
                // Verify that all architecture records fit within the file
                qint64 nRequiredSize = sizeof(XMACH_DEF::fat_header) + (qint64)nNumberOfRecords * sizeof(XMACH_DEF::fat_arch);

                if (getSize() >= nRequiredSize) {
                    bResult = true;

                    // Additional validation: check that offsets and sizes are reasonable
                    bool bIsBigEndian = isBigEndian();
                    qint64 nFileSize = getSize();

                    for (quint32 i = 0; i < nNumberOfRecords; i++) {
                        qint64 nOffset = sizeof(XMACH_DEF::fat_header) + (qint64)i * sizeof(XMACH_DEF::fat_arch);

                        quint32 nArchOffset = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, offset), bIsBigEndian);
                        quint32 nArchSize = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, size), bIsBigEndian);

                        // Check that the architecture data fits within the file
                        if ((qint64)nArchOffset + (qint64)nArchSize > nFileSize) {
                            bResult = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    return bResult;
}

bool XMACHOFat::isValid(QIODevice *pDevice)
{
    XMACHOFat xmachofat(pDevice);

    return xmachofat.isValid();
}

XBinary::ENDIAN XMACHOFat::getEndian()
{
    ENDIAN result = ENDIAN_UNKNOWN;

    quint32 nData = read_uint32(0);

    if (nData == XMACH_DEF::S_FAT_MAGIC) {
        result = ENDIAN_LITTLE;
    } else if (nData == XMACH_DEF::S_FAT_CIGAM) {
        result = ENDIAN_BIG;
    }

    return result;
}

quint64 XMACHOFat::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    quint64 nResult = 0;

    if (getSize() >= sizeof(XMACH_DEF::fat_header)) {
        nResult = read_uint32(offsetof(XMACH_DEF::fat_header, nfat_arch), isBigEndian());
    }

    return nResult;
}

XBinary::OSNAME XMACHOFat::getOsName()
{
    return OSNAME_MACOS;
}

QList<XArchive::RECORD> XMACHOFat::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;

    qint32 nNumberOfRecords = (qint32)getNumberOfRecords(pPdStruct);

    if (nLimit != -1) {
        nNumberOfRecords = qMin(nNumberOfRecords, nLimit);
    }

    if (nNumberOfRecords < 0) {
        return listResult;  // Invalid number of records
    }

    bool bIsBigEndian = isBigEndian();
    qint64 nFileSize = getSize();

    QMap<quint64, QString> mapCpuTypes = XMACH::getHeaderCpuTypesS();

    for (qint32 i = 0; (i < nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        qint64 nOffset = sizeof(XMACH_DEF::fat_header) + i * sizeof(XMACH_DEF::fat_arch);

        if (nOffset + (qint64)sizeof(XMACH_DEF::fat_arch) > nFileSize) {
            break;  // Prevent reading beyond file bounds
        }

        quint32 _cputype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cputype), bIsBigEndian);
        quint32 _cpusubtype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cpusubtype), bIsBigEndian);
        quint32 _offset = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, offset), bIsBigEndian);
        quint32 _size = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, size), bIsBigEndian);

        // Validate that the architecture data is within file bounds
        if ((qint64)_offset + (qint64)_size > nFileSize) {
            continue;  // Skip invalid records
        }

        RECORD record = {};

        record.spInfo.sRecordName = XMACH::_getArch(_cputype, _cpusubtype);
        record.spInfo.nUncompressedSize = _size;
        record.spInfo.compressMethod = HANDLE_METHOD_STORE;
        record.nHeaderOffset = nOffset;
        record.nHeaderSize = sizeof(XMACH_DEF::fat_arch);
        record.nDataOffset = _offset;
        record.nDataSize = _size;

        listResult.append(record);
    }

    return listResult;
}

QString XMACHOFat::getFileFormatExt()
{
    return "";
}

QString XMACHOFat::getFileFormatExtsString()
{
    return "Universal Mach-O (fat) (*.fat)";
}

qint64 XMACHOFat::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XMACHOFat::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XMACHOFat::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_REGIONS;  // Default mode
    }

    if (mapMode == MAPMODE_REGIONS) {
        XBinary::PDSTRUCT pdStructEmpty = {};

        if (!pPdStruct) {
            pdStructEmpty = XBinary::createPdStruct();
            pPdStruct = &pdStructEmpty;
        }

        result.endian = getEndian();
        result.nBinarySize = getSize();

        bool bIsBigEndian = (result.endian == ENDIAN_BIG);

        qint32 nIndex = 0;

        {
            _MEMORY_RECORD record = {};

            record.nIndex = nIndex++;
            record.filePart = FILEPART_HEADER;
            record.nOffset = 0;
            record.nSize = sizeof(XMACH_DEF::fat_header);
            record.nAddress = -1;
            record.sName = tr("Header");

            result.listRecords.append(record);
        }

        quint32 nNumberOfRecords = read_uint32(offsetof(XMACH_DEF::fat_header, nfat_arch), bIsBigEndian);

        for (qint32 i = 0; i < (qint32)nNumberOfRecords; i++) {
            _MEMORY_RECORD record = {};

            qint64 nOffset = sizeof(XMACH_DEF::fat_header) + i * sizeof(XMACH_DEF::fat_arch);

            quint32 _cputype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cputype), bIsBigEndian);
            quint32 _cpusubtype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cpusubtype), bIsBigEndian);
            quint32 _offset = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, offset), bIsBigEndian);
            quint32 _size = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, size), bIsBigEndian);

            record.sName = XMACH::_getArch(_cputype, _cpusubtype);
            record.nOffset = _offset;
            record.nSize = _size;
            record.nAddress = -1;
            record.filePart = FILEPART_SEGMENT;

            result.listRecords.append(record);
        }

        _handleOverlay(&result);
    }

    return result;
}

QString XMACHOFat::getArch()
{
    QStringList listArchs;

    if (getSize() >= sizeof(XMACH_DEF::fat_header)) {
        bool bIsBigEndian = isBigEndian();
        quint32 nNumberOfRecords = read_uint32(offsetof(XMACH_DEF::fat_header, nfat_arch), bIsBigEndian);
        qint64 nFileSize = getSize();

        for (quint32 i = 0; i < nNumberOfRecords; i++) {
            qint64 nOffset = sizeof(XMACH_DEF::fat_header) + (qint64)i * sizeof(XMACH_DEF::fat_arch);

            if (nOffset + (qint64)sizeof(XMACH_DEF::fat_arch) <= nFileSize) {
                quint32 cputype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cputype), bIsBigEndian);
                quint32 cpusubtype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cpusubtype), bIsBigEndian);

                QString sArch = XMACH::_getArch(cputype, cpusubtype);
                if (!sArch.isEmpty()) listArchs.append(sArch);
            }
        }
    }

    if (listArchs.isEmpty()) return tr("Universal");

    listArchs.removeDuplicates();
    return listArchs.join(", ");
}

qint32 XMACHOFat::getType()
{
    return TYPE_BUNDLE;
}

QString XMACHOFat::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_BUNDLE: sResult = tr("Bundle");
    }

    return sResult;
}

QString XMACHOFat::getMIMEString()
{
    return "application/x-mach-binary";
    ;
}

XBinary::FT XMACHOFat::getFileType()
{
    return FT_MACHOFAT;
}

bool XMACHOFat::isArchive()
{
    return true;
}

QString XMACHOFat::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XMACHOFAT_STRUCTID, sizeof(_TABLE_XMACHOFAT_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// qint64 XMACHOFat::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
// {
//     return (qint64)getNumberOfRecords(pPdStruct);
// }

XMACH_DEF::fat_header XMACHOFat::read_fat_header()
{
    XMACH_DEF::fat_header result = {};

    if (getSize() >= sizeof(XMACH_DEF::fat_header)) {
        bool bIsBigEndian = isBigEndian();

        result.magic = read_uint32(offsetof(XMACH_DEF::fat_header, magic), bIsBigEndian);
        result.nfat_arch = read_uint32(offsetof(XMACH_DEF::fat_header, nfat_arch), bIsBigEndian);
    }

    return result;
}

XMACH_DEF::fat_arch XMACHOFat::read_fat_arch(qint32 nIndex)
{
    XMACH_DEF::fat_arch result = {};

    qint64 nOffset = sizeof(XMACH_DEF::fat_header) + (qint64)nIndex * sizeof(XMACH_DEF::fat_arch);

    if (nOffset + (qint64)sizeof(XMACH_DEF::fat_arch) <= getSize()) {
        bool bIsBigEndian = isBigEndian();

        result.cputype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cputype), bIsBigEndian);
        result.cpusubtype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cpusubtype), bIsBigEndian);
        result.offset = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, offset), bIsBigEndian);
        result.size = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, size), bIsBigEndian);
        result.align = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, align), bIsBigEndian);
    }

    return result;
}

QList<XMACH_DEF::fat_arch> XMACHOFat::read_fat_arch_list(PDSTRUCT *pPdStruct)
{
    QList<XMACH_DEF::fat_arch> listResult;

    qint32 nNumberOfRecords = (qint32)getNumberOfRecords(pPdStruct);

    if (nNumberOfRecords > 0) {
        for (qint32 i = 0; (i < nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            XMACH_DEF::fat_arch fatArch = read_fat_arch(i);
            listResult.append(fatArch);
        }
    }

    return listResult;
}

QMap<quint64, QString> XMACHOFat::getHeaderMagics()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(XMACH_DEF::S_FAT_MAGIC, "FAT_MAGIC");
    mapResult.insert(XMACH_DEF::S_FAT_CIGAM, "FAT_CIGAM");

    return mapResult;
}

QMap<quint64, QString> XMACHOFat::getHeaderMagicsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(XMACH_DEF::S_FAT_MAGIC, "Universal Mach-O (Big Endian)");
    mapResult.insert(XMACH_DEF::S_FAT_CIGAM, "Universal Mach-O (Little Endian)");

    return mapResult;
}

QString XMACHOFat::getArchitectureString(qint32 nIndex)
{
    QString sResult;

    XMACH_DEF::fat_arch fatArch = read_fat_arch(nIndex);

    if (fatArch.cputype != 0) {
        sResult = XMACH::_getArch(fatArch.cputype, fatArch.cpusubtype);
    }

    return sResult;
}

qint64 XMACHOFat::getArchitectureOffset(qint32 nIndex)
{
    qint64 nResult = -1;

    XMACH_DEF::fat_arch fatArch = read_fat_arch(nIndex);

    if (fatArch.cputype != 0) {
        nResult = fatArch.offset;
    }

    return nResult;
}

qint64 XMACHOFat::getArchitectureSize(qint32 nIndex)
{
    qint64 nResult = 0;

    XMACH_DEF::fat_arch fatArch = read_fat_arch(nIndex);

    if (fatArch.cputype != 0) {
        nResult = fatArch.size;
    }

    return nResult;
}

bool XMACHOFat::isArchitectureValid(qint32 nIndex)
{
    bool bResult = false;

    qint64 nOffset = sizeof(XMACH_DEF::fat_header) + (qint64)nIndex * sizeof(XMACH_DEF::fat_arch);

    if (nOffset + (qint64)sizeof(XMACH_DEF::fat_arch) <= getSize()) {
        XMACH_DEF::fat_arch fatArch = read_fat_arch(nIndex);

        if (fatArch.cputype != 0) {
            qint64 nFileSize = getSize();

            // Verify that the architecture data fits within the file
            if ((qint64)fatArch.offset + (qint64)fatArch.size <= nFileSize) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XMACHOFat::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    pState->nCurrentOffset = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = (qint32)getNumberOfRecords(pPdStruct);
    pState->nTotalSize = getSize();
    pState->mapProperties = mapProperties;
    pState->pContext = nullptr;

    return (pState->nNumberOfRecords > 0);
}

XBinary::ARCHIVERECORD XMACHOFat::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (!pState || pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    bool bIsBigEndian = isBigEndian();
    qint64 nOffset = sizeof(XMACH_DEF::fat_header) + (qint64)pState->nCurrentIndex * sizeof(XMACH_DEF::fat_arch);

    quint32 _cputype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cputype), bIsBigEndian);
    quint32 _cpusubtype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cpusubtype), bIsBigEndian);
    quint32 _offset = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, offset), bIsBigEndian);
    quint32 _size = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, size), bIsBigEndian);

    QString sArchName = XMACH::_getArch(_cputype, _cpusubtype);

    result.nStreamOffset = _offset;
    result.nStreamSize = _size;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sArchName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)_size);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)HANDLE_METHOD_STORE);

    return result;
}

bool XMACHOFat::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pDevice || pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    ARCHIVERECORD ar = infoCurrent(pState, pPdStruct);

    if (ar.nStreamSize == 0) {
        return false;
    }

    // Since Mach-O Fat uses STORE compression, just copy the data
    qint64 nBytesWritten = 0;
    qint64 nBytesToWrite = ar.nStreamSize;
    qint64 nCurrentOffset = ar.nStreamOffset;
    const qint64 nBufferSize = 0x10000;  // 64KB buffer

    QByteArray baBuffer;
    baBuffer.resize(nBufferSize);

    while ((nBytesWritten < nBytesToWrite) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint64 nChunkSize = qMin(nBufferSize, nBytesToWrite - nBytesWritten);
        nChunkSize = read_array(nCurrentOffset, baBuffer.data(), nChunkSize);

        if (nChunkSize <= 0) {
            return false;
        }

        qint64 nWritten = pDevice->write(baBuffer.data(), nChunkSize);
        if (nWritten != nChunkSize) {
            return false;
        }

        nBytesWritten += nChunkSize;
        nCurrentOffset += nChunkSize;
    }

    return (nBytesWritten == nBytesToWrite);
}

bool XMACHOFat::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex <= pState->nNumberOfRecords);
}

bool XMACHOFat::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    pState->nCurrentOffset = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;

    return true;
}
