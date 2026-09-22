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
#include "xlarcpfx.h"

#include <QFileInfo>
#include <QtEndian>

#include <limits>

namespace {
// GEMDOS program header, and the level-0 LHA header laid over the DATA segment.
const qint64 N_PFX_PROGRAM_HEADER_SIZE = 28;
const qint64 N_PFX_MIN_TEXT_SIZE = 32;
const qint32 N_PFX_LHA_FIXED_SIZE = 22;   // method .. name length, name excluded
const qint32 N_PFX_LHA_MIN_TOTAL = 24;    // the two leading bytes plus the above
const qint32 N_PFX_MAX_NAME_LENGTH = 64;
// -lz5- emits at most eight commands of eighteen bytes for every seventeen
// input bytes, so no honest member can claim more than this.
const qint64 N_PFX_MAX_EXPANSION = 9;
const qint64 N_PFX_MAX_EXPANSION_SLACK = 144;

quint32 pfxReadLe32(const QByteArray &baData, qint32 nOffset)
{
    return static_cast<quint32>(static_cast<quint8>(baData.at(nOffset))) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24);
}
}  // namespace

XLArcPfx::XLArcPfx(QIODevice *pDevice) : XGameStoreArchiveBase(pDevice, FT_LARC_PFX)
{
}

bool XLArcPfx::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLArcPfx archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLArcPfx::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLArcPfx(pDevice);
}

bool XLArcPfx::decodeStoredName(const QByteArray &baField, QString *pName)
{
    if (!pName) return false;

    QByteArray baWork = baField;
    const qint32 nEnd = baWork.indexOf('\0');
    if (nEnd >= 0) baWork.truncate(nEnd);
    if (baWork.isEmpty()) return false;

    baWork.replace('\\', '/');
    const qint32 nSeparator = baWork.lastIndexOf('/');
    if (nSeparator >= 0) baWork = baWork.mid(nSeparator + 1);
    if (baWork.isEmpty() || (baWork.size() > N_PFX_MAX_NAME_LENGTH)) return false;

    for (qint32 i = 0; i < baWork.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baWork.at(i));
        if ((nCharacter < 0x21) || (nCharacter > 0x7e)) return false;
        if ((nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') ||
            (nCharacter == '"') || (nCharacter == '<') || (nCharacter == '>') ||
            (nCharacter == '|')) {
            return false;
        }
    }
    if ((baWork == QByteArray(".")) || (baWork == QByteArray(".."))) return false;

    *pName = QString::fromLatin1(baWork);

    return true;
}

bool XLArcPfx::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd, PDSTRUCT *pPdStruct)
{
    const qint64 nMaxSize = static_cast<qint64>((std::numeric_limits<qint32>::max)());
    const qint64 nTotalSize = getSize();
    if ((nTotalSize < 60) || (nTotalSize > nMaxSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baProgramHeader =
        read_array_process(0, N_PFX_PROGRAM_HEADER_SIZE, pPdStruct);
    if ((baProgramHeader.size() != N_PFX_PROGRAM_HEADER_SIZE)) return false;
    const uchar *pProgramHeader =
        reinterpret_cast<const uchar *>(baProgramHeader.constData());
    if (qFromBigEndian<quint16>(pProgramHeader) != 0x601a) return false;

    const qint64 nTextSize = static_cast<qint64>(qFromBigEndian<quint32>(pProgramHeader + 2));
    const qint64 nDataSegmentSize = static_cast<qint64>(qFromBigEndian<quint32>(pProgramHeader + 6));
    const qint64 nBssSize = static_cast<qint64>(qFromBigEndian<quint32>(pProgramHeader + 10));
    const qint64 nSymbolSize = static_cast<qint64>(qFromBigEndian<quint32>(pProgramHeader + 14));
    if ((nTextSize < N_PFX_MIN_TEXT_SIZE) || (nTextSize > nMaxSize) ||
        (nDataSegmentSize <= 0) || (nDataSegmentSize > nMaxSize) ||
        (nBssSize > nMaxSize) || (nSymbolSize > nMaxSize)) {
        return false;
    }
    if ((N_PFX_PROGRAM_HEADER_SIZE + nTextSize) > nTotalSize) return false;
    const qint64 nDataEnd = N_PFX_PROGRAM_HEADER_SIZE + nTextSize + nDataSegmentSize;
    if (nDataEnd > nTotalSize) return false;

    // The marker is the last long word of the stub, so it also pins the member
    // header to a single offset instead of a scan.
    const qint64 nMarkerOffset = N_PFX_PROGRAM_HEADER_SIZE + nTextSize - 4;
    if (nMarkerOffset < N_PFX_PROGRAM_HEADER_SIZE) return false;
    const QByteArray baMarker = read_array_process(nMarkerOffset, 4, pPdStruct);
    if ((baMarker.size() != 4) ||
        (baMarker != QByteArray("\xde\xad\xfa\xce", 4))) {
        return false;
    }

    const qint64 nHeaderOffset = N_PFX_PROGRAM_HEADER_SIZE + nTextSize;
    if ((nHeaderOffset + N_PFX_LHA_MIN_TOTAL) > nDataEnd) return false;
    const QByteArray baPrefix = read_array_process(nHeaderOffset, 2, pPdStruct);
    if ((baPrefix.size() != 2)) return false;
    const qint32 nDeclaredHeaderSize = static_cast<quint8>(baPrefix.at(0));
    if (nDeclaredHeaderSize < N_PFX_LHA_FIXED_SIZE) return false;
    const qint64 nHeaderSize = 2 + static_cast<qint64>(nDeclaredHeaderSize);
    if ((nHeaderOffset + nHeaderSize) > nDataEnd) return false;

    const QByteArray baHeader = read_array_process(nHeaderOffset, nHeaderSize, pPdStruct);
    if ((static_cast<qint64>(baHeader.size()) != nHeaderSize)) return false;
    quint32 nSum = 0;
    for (qint32 i = 2; i < baHeader.size(); i++) {
        nSum += static_cast<quint8>(baHeader.at(i));
    }
    if ((nSum & 0xff) != static_cast<quint32>(static_cast<quint8>(baHeader.at(1)))) return false;
    if (baHeader.mid(2, 5) != QByteArray("-lz5-", 5)) return false;
    if (static_cast<quint8>(baHeader.at(20)) != 0) return false;
    const qint32 nNameLength = static_cast<quint8>(baHeader.at(21));
    // The two CRC bytes follow the name inside the declared header.
    if ((N_PFX_LHA_FIXED_SIZE + nNameLength) > nDeclaredHeaderSize) return false;

    const qint64 nDeclaredPackedSize = static_cast<qint64>(pfxReadLe32(baHeader, 7));
    const qint64 nUncompressedSize = static_cast<qint64>(pfxReadLe32(baHeader, 11));
    const qint64 nStreamOffset = nHeaderOffset + nHeaderSize;
    const qint64 nStreamSize = nDataEnd - nStreamOffset;
    if (nStreamSize <= 0) return false;
    // THE discriminator. A LArc/LHarc self-extractor whose member fits the
    // bytes it declares is a well-formed archive and belongs to the existing
    // XSFX/XLHA path; only the PFX stub's overrun brings a carrier here.
    if (nDeclaredPackedSize <= nStreamSize) return false;
    if ((nUncompressedSize <= 0) || (nUncompressedSize > nMaxSize)) return false;
    if (nUncompressedSize >
        ((nStreamSize * N_PFX_MAX_EXPANSION) + N_PFX_MAX_EXPANSION_SLACK)) {
        return false;
    }

    QString sFileName;
    if (!decodeStoredName(baHeader.mid(22, nNameLength), &sFileName)) {
        // PFX packs one program and the carrier is that program, so the
        // carrier's own name is the member's name whenever the field is empty
        // or holds something that is not one.
        sFileName = QFileInfo(getDeviceFileName(getDevice())).fileName();
        if (sFileName.isEmpty()) sFileName = QString("data");
    }

    ENTRY entry = {};
    entry.nHeaderOffset = nHeaderOffset;
    entry.nHeaderSize = nHeaderSize;
    entry.nDataOffset = nStreamOffset;
    entry.nDataSize = nStreamSize;
    entry.nUncompressedSize = nUncompressedSize;
    entry.handleMethod = HANDLE_METHOD_LHA_LEGACY;
    // XLZHDecoder::decompressLegacyLha selects the codec from this property and
    // refuses the member when it is absent, so it must be carried explicitly.
    entry.baCompressProperties = QByteArray("-lz5-", 5);
    entry.sFileName = sFileName;
    // No time stamp and no checksum: the stub writes zero into the level-0 time
    // field and a CRC-16 that does not match the payload, and a published
    // checksum that never verifies is worse than none.

    if (pEntries) pEntries->append(entry);
    // The GEMDOS relocation trailer after DATA belongs to the carrier.
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;

    return isPdStructNotCanceled(pPdStruct);
}
