/* Copyright (c) 2025 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xcfbf.h"

XCFBF::XCFBF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCFBF::~XCFBF()
{
}

bool XCFBF::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= 512) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "D0CF11E0A1B11AE100000000000000000000000000000000", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XCFBF::isValid(QIODevice *pDevice)
{
    XCFBF xcfbf(pDevice);

    return xcfbf.isValid();
}

QString XCFBF::getArch()
{
    return "";  // TODO
}

QString XCFBF::getVersion()
{
    QString sResult;

    quint16 uMinorVersion = read_uint16(0x18);
    quint16 uDllVersion = read_uint16(0x1A);

    sResult = QString("%1.%2").arg(uDllVersion).arg(uMinorVersion);

    return sResult;
}

QString XCFBF::getFileFormatExt()
{
    return "";
}

quint64 XCFBF::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return 0;  // TODO
}

QList<XArchive::RECORD> XCFBF::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    return listResult;
}

XCFBF::StructuredStorageHeader XCFBF::read_StructuredStorageHeader(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    StructuredStorageHeader header = {};

    read_array(nOffset, (char *)&header, sizeof(header), pPdStruct);

    return header;
}

XBinary::_MEMORY_MAP XCFBF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    qint64 nTotalSize = getSize();

    _MEMORY_MAP result = {};

    result.nBinarySize = nTotalSize;
    result.fileType = getFileType();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = getTypeAsString();

    StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);

    qint32 nIndex = 0;
    qint32 nSectSize = 512;
    qint32 nMiniFatSectSize = 64;

    if (ssh._uSectorShift == 9) {
        nSectSize = 512;
    } else if (ssh._uSectorShift == 12) {
        nSectSize = 4096;
    }

    qint64 nCurrentOffset = 0;

    {
        _MEMORY_RECORD recordHeader = {};
        recordHeader.nAddress = -1;
        recordHeader.segment = ADDRESS_SEGMENT_FLAT;
        recordHeader.nOffset = nCurrentOffset;
        recordHeader.nSize = nSectSize;
        recordHeader.nIndex = nIndex++;
        recordHeader.type = MMT_HEADER;
        recordHeader.sName = tr("Header");

        result.listRecords.append(recordHeader);
    }

    nCurrentOffset += nSectSize;

    // quint32 nNumberOfSects = ssh._csectFat + ssh._csectDir + ssh._csectMiniFat + ssh._csectDif;

    // for (quint32 i = 0; i < nNumberOfSects; i++) {
    //     _MEMORY_RECORD recordHeader = {};
    //     recordHeader.nAddress = -1;
    //     recordHeader.segment = ADDRESS_SEGMENT_FLAT;
    //     recordHeader.nOffset = nCurrentOffset;
    //     recordHeader.nSize = nSectSize;
    //     recordHeader.nIndex = nIndex++;
    //     recordHeader.type = MMT_FILESEGMENT;
    //     recordHeader.sName = QString("%1 %2").arg(tr("Sector"), QString::number(i));

    //     result.listRecords.append(recordHeader);

    //     nCurrentOffset += nSectSize;
    // }

    return result;
}

XBinary::FT XCFBF::getFileType()
{
    return FT_CFBF;
}

XBinary::ENDIAN XCFBF::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XCFBF::getMode()
{
    return MODE_32;
}
