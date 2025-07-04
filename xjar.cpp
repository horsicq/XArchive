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
#include "xjar.h"

XJAR::XJAR(QIODevice *pDevice) : XZip(pDevice)
{
}

bool XJAR::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XZip xzip(getDevice());

    if (xzip.isValid()) {
        qint64 nECDOffset = xzip.findECDOffset(pPdStruct);
        bResult = xzip.isJAR(nECDOffset, pPdStruct);
    }

    return bResult;
}

bool XJAR::isValid(QIODevice *pDevice)
{
    XJAR xjar(pDevice);

    return xjar.isValid();
}

bool XJAR::isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    bResult = XArchive::isArchiveRecordPresent("META-INF/MANIFEST.MF", pListRecords, pPdStruct);

    return bResult;
}

XBinary::FT XJAR::getFileType()
{
    return FT_JAR;
}

XBinary::FILEFORMATINFO XJAR::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    XBinary::FILEFORMATINFO result = {};

    QList<XArchive::RECORD> listArchiveRecords = getRecords(20000, pPdStruct);

    if (isValid(&listArchiveRecords, pPdStruct)) {
        result.bIsValid = true;
        result.nSize = getSize();
        result.sExt = "jar";
        result.fileType = FT_JAR;

        result.osName = OSNAME_JVM;
        result.bIsVM = true;

        result.sArch = getArch();
        result.mode = getMode();
        result.sType = typeIdToString(getType());
        result.endian = getEndian();

        qint32 nNumberOfRecords = listArchiveRecords.count();

        for (qint32 i = 0; i < nNumberOfRecords; i++) {
            if (listArchiveRecords.at(i).spInfo.sRecordName.section(".", -1, -1) == "class") {
                RECORD record = listArchiveRecords.at(i);
                QByteArray baData = XArchive::decompress(&record, pPdStruct, 0, 0x100);

                if (baData.size() > 10) {
                    char *pData = baData.data();
                    if (XBinary::_read_uint32(pData, true) == 0xCAFEBABE) {
                        quint16 nMinor = XBinary::_read_uint16(pData + 4, true);
                        quint16 nMajor = XBinary::_read_uint16(pData + 6, true);

                        result.sOsVersion = XJavaClass::_getJDKVersion(nMajor, nMinor);

                        break;
                    }
                }
            }
        }

        // if (nNumberOfRecords < 20000) {
        //     result.nNumberOfRecords = nNumberOfRecords;
        // } else {
        //     result.nNumberOfRecords = getNumberOfRecords(pPdStruct);
        // }
    }

    return result;
}

QString XJAR::getFileFormatExt()
{
    return "jar";
}

XBinary::ENDIAN XJAR::getEndian()
{
    return ENDIAN_UNKNOWN;
}

XBinary::MODE XJAR::getMode()
{
    return MODE_DATA;
}

QString XJAR::getArch()
{
    return tr("Universal");
}

qint32 XJAR::getType()
{
    return TYPE_PACKAGE;
}

QString XJAR::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_PACKAGE: sResult = tr("Package");
    }

    return sResult;
}
