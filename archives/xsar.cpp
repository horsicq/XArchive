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
#include "xsar.h"

XSAR::XSAR(QIODevice *pDevice) : XLHA(pDevice)
{
}

bool XSAR::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    // Same contract as XLHA::isValid: the caller's device cursor is restored,
    // because detection probes a device the caller still owns.
    QIODevice *pSourceDevice = getDevice();
    const qint64 nSavedPos = pSourceDevice ? pSourceDevice->pos() : -1;

    // Smallest possible archive: 2 prefix bytes, a level 0/1 base header, and
    // the end-of-archive marker.
    if (XBinary::isPdStructNotCanceled(pPdStruct) && (getSize() >= 24)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        // Byte 0 is a header length and byte 1 a checksum, so the only fixed
        // bytes are the tag itself: " LH" then one method character then " ".
        if (compareSignature(&memoryMap, "....' LH'..20", 0, pPdStruct)) {
            const QString sMethod = read_ansiString(2, 5);

            // SAR.DOC documents exactly three methods: LH5, LH4 and LH0.
            if ((sMethod == " LH0 ") || (sMethod == " LH4 ") || (sMethod == " LH5 ")) {
                const quint8 nHeaderSize = read_uint8(0);
                const quint8 nLevel = read_uint8(20);

                // Level 2 replaces the checksum byte with the low half of a
                // 16-bit header size, so the check below does not apply there.
                // No level 2 SAR archive has been seen; refuse rather than
                // guess.
                if ((nHeaderSize >= 21) && (nLevel <= 1)) {
                    const QByteArray baHeader = read_array(0, 2 + (qint64)nHeaderSize);
                    bResult = _isHeaderChecksumValid(baHeader);
                }
            }
        }
    }

    if (pSourceDevice && (nSavedPos >= 0)) {
        pSourceDevice->seek(nSavedPos);
    }

    return bResult;
}

bool XSAR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSAR xsar(pDevice);

    return xsar.isValid(pPdStruct);
}

XBinary *XSAR::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSAR(pDevice);
}

QList<QString> XSAR::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("....' LH'..20");

    return listResult;
}

XBinary::FT XSAR::getFileType()
{
    return FT_SAR;
}

QString XSAR::getFileFormatExt()
{
    return "sar";
}

QString XSAR::getFileFormatExtsString()
{
    return "SAR(sar)";
}

QString XSAR::getMIMEString()
{
    return "application/x-lzh-compressed";
}

QString XSAR::getVersion()
{
    // The method characters, matching how XLHA reports its own tag.
    return read_ansiString(3, 3);
}

bool XSAR::_isMemberTag(const QByteArray &baHeader)
{
    if (baHeader.size() < 21) return false;

    return (baHeader.mid(2, 3) == " LH") && (baHeader.at(6) == ' ');
}
