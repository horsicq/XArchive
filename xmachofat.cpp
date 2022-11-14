/* Copyright (c) 2017-2022 hors<horsicq@gmail.com>
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

XMACHOFat::XMACHOFat(QIODevice *pDevice) : XArchive(pDevice) {
}

bool XMACHOFat::isValid() {
    bool bResult = false;

    quint32 nMagic = read_uint32(0);

    if ((nMagic == XMACH_DEF::S_FAT_MAGIC) || (nMagic == XMACH_DEF::S_FAT_CIGAM)) {
        PDSTRUCT pdStruct = {};

        bResult = (getNumberOfRecords(&pdStruct) < 10);  // TODO Check !!!
    }

    return bResult;
}

bool XMACHOFat::isValid(QIODevice *pDevice) {
    XMACHOFat xmachofat(pDevice);

    return xmachofat.isValid();
}

bool XMACHOFat::isBigEndian() {
    bool bResult = false;

    quint32 nMagic = read_uint32(0);

    if (nMagic == XMACH_DEF::S_FAT_CIGAM) {
        bResult = true;
    }

    return bResult;
}

quint64 XMACHOFat::getNumberOfRecords(PDSTRUCT *pPdStruct) {
    return read_uint32(offsetof(XMACH_DEF::fat_header, nfat_arch), isBigEndian());
}

QList<XArchive::RECORD> XMACHOFat::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) {
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    QList<RECORD> listResult;

    qint32 nNumberOfRecords = (qint32)getNumberOfRecords(pPdStruct);

    if (nLimit != -1) {
        nNumberOfRecords = qMin(nNumberOfRecords, nLimit);
    }

    bool bIsBigEndian = isBigEndian();

    QMap<quint64, QString> mapCpuTypes = XMACH::getHeaderCpuTypesS();

    for (qint32 i = 0; (i < nNumberOfRecords) && (!(pPdStruct->bIsStop)); i++) {
        qint64 nOffset = sizeof(XMACH_DEF::fat_header) + i * sizeof(XMACH_DEF::fat_arch);

        quint32 _cputype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cputype), bIsBigEndian);
        quint32 _cpusubtype = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, cpusubtype), bIsBigEndian);
        quint32 _offset = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, offset), bIsBigEndian);
        quint32 _size = read_uint32(nOffset + offsetof(XMACH_DEF::fat_arch, size), bIsBigEndian);
        //        quint32 _align=read_uint32(nOffset+offsetof(XMACH_DEF::fat_arch,align),bIsBigEndian);

        RECORD record = {};

        record.sFileName = QString("%1").arg(mapCpuTypes.value(_cputype, tr("Unknown")));

        if (_cpusubtype) {
            record.sFileName += QString("-%1").arg(_cpusubtype, 0, 16);
        }

        record.nDataOffset = _offset;
        record.nCompressedSize = _size;
        record.nUncompressedSize = _size;
        record.compressMethod = COMPRESS_METHOD_STORE;

        listResult.append(record);
    }

    return listResult;
}

// QString XMACHOFat::getFileFormatExt()
//{

//}

// qint64 XMACHOFat::getFileFormatSize()
//{

//}

// QString XMACHOFat::getFileFormatString()
//{

//}
