/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhfedecoder.h"

#include <QtEndian>

#include <map>
#include <vector>

namespace {

const qint64 HFE_HEADER_SIZE = 24;
const qint64 HFE_BLOCK = 512;
const qint64 HFE_CHUNK = 256;
const qint32 HFE_MAX_TRACKS = 256;
const qint32 HFE_MAX_SECTORS = 255;
const qint64 HFE_MAX_IMAGE_SIZE = 0x8000000;  // 128 MB ceiling
const quint32 HFE_SYNC_A1 = 0x4489;           // MFM 0xA1 with a missing clock

struct HFEHeader {
    qint32 nTracks = 0;
    qint32 nSides = 0;
    qint32 nEncoding = 0;
    qint64 nLutOffset = 0;
};

bool hfeParseHeader(const QByteArray &baFile, HFEHeader *pHeader)
{
    if (!pHeader || (baFile.size() < HFE_HEADER_SIZE)) return false;
    const uchar *p = reinterpret_cast<const uchar *>(baFile.constData());
    if (memcmp(p, "HXCPICFE", 8) != 0) return false;
    if (p[8] != 0) return false;                       // revision 0 only
    if (p[9] == 0) return false;                       // number of tracks
    if ((p[10] != 1) && (p[10] != 2)) return false;    // number of sides
    pHeader->nTracks = p[9];
    pHeader->nSides = p[10];
    pHeader->nEncoding = p[11];
    pHeader->nLutOffset =
        static_cast<qint64>(qFromLittleEndian<quint16>(p + 0x12)) * HFE_BLOCK;
    if (pHeader->nTracks > HFE_MAX_TRACKS) return false;
    if (pHeader->nLutOffset < HFE_HEADER_SIZE) return false;
    const qint64 nLutSize = qint64(pHeader->nTracks) * 4;
    if (nLutSize > baFile.size() - pHeader->nLutOffset) return false;
    return true;
}

// Splits one LUT track into per-side bit-cell streams (one entry per cell).
bool hfeTrackBits(const QByteArray &baFile, const HFEHeader &header,
                  qint32 nTrack, std::vector<quint8> *pSide0,
                  std::vector<quint8> *pSide1)
{
    const uchar *p = reinterpret_cast<const uchar *>(baFile.constData());
    const qint64 nEntry = header.nLutOffset + qint64(nTrack) * 4;
    const qint64 nOffset =
        static_cast<qint64>(qFromLittleEndian<quint16>(p + nEntry)) * HFE_BLOCK;
    const qint64 nLength =
        static_cast<qint64>(qFromLittleEndian<quint16>(p + nEntry + 2));
    if ((nLength <= 0) || (nOffset < HFE_HEADER_SIZE) ||
        (nLength > baFile.size() - nOffset)) {
        return false;
    }
    pSide0->clear();
    pSide1->clear();
    for (qint64 nChunk = 0; nChunk < nLength; nChunk += HFE_BLOCK) {
        for (qint32 nSide = 0; nSide < 2; ++nSide) {
            std::vector<quint8> *pTarget = nSide ? pSide1 : pSide0;
            const qint64 nStart = nChunk + nSide * HFE_CHUNK;
            const qint64 nStop = qMin<qint64>(nStart + HFE_CHUNK, nLength);
            for (qint64 i = nStart; i < nStop; ++i) {
                const quint8 nByte = p[nOffset + i];
                for (qint32 k = 0; k < 8; ++k) {
                    pTarget->push_back(quint8((nByte >> k) & 1));
                }
            }
        }
    }
    return true;
}

quint16 hfeCrc16(const quint8 *pData, qint32 nSize, quint16 nCrc)
{
    for (qint32 i = 0; i < nSize; ++i) {
        nCrc ^= quint16(pData[i]) << 8;
        for (qint32 k = 0; k < 8; ++k) {
            nCrc = (nCrc & 0x8000) ? quint16((nCrc << 1) ^ 0x1021)
                                   : quint16(nCrc << 1);
        }
    }
    return nCrc;
}

// One MFM byte is 16 cells; the data bits are the odd-indexed ones.
bool hfeMfmByte(const std::vector<quint8> &bits, qint64 nPosition,
                quint8 *pValue)
{
    if ((nPosition < 0) || (nPosition + 16 > qint64(bits.size()))) return false;
    quint32 nValue = 0;
    for (qint32 k = 0; k < 8; ++k) {
        nValue = (nValue << 1) | bits[size_t(nPosition + 2 * k + 1)];
    }
    *pValue = quint8(nValue);
    return true;
}

bool hfeMfmBytes(const std::vector<quint8> &bits, qint64 nPosition,
                 qint32 nCount, std::vector<quint8> *pOut)
{
    pOut->resize(size_t(nCount));
    for (qint32 i = 0; i < nCount; ++i) {
        if (!hfeMfmByte(bits, nPosition + qint64(i) * 16, &(*pOut)[size_t(i)])) {
            return false;
        }
    }
    return true;
}

struct HFESector {
    qint32 nCylinder = 0;
    qint32 nHead = 0;
    qint32 nSector = 0;
    qint32 nSizeCode = 0;
    bool bValid = false;
    std::vector<quint8> data;
};

void hfeDecodeTrack(const std::vector<quint8> &bits,
                    std::vector<HFESector> *pSectors)
{
    // Collect the positions of every 0x4489 address-mark cell pattern.
    std::vector<qint64> syncs;
    quint32 nShift = 0;
    const qint64 nBits = qint64(bits.size());
    for (qint64 i = 0; i < nBits; ++i) {
        nShift = ((nShift << 1) | bits[size_t(i)]) & 0xffff;
        if ((i >= 15) && (nShift == HFE_SYNC_A1)) syncs.push_back(i - 15);
    }

    bool bHaveId = false;
    HFESector identifier;
    std::vector<quint8> field;
    for (size_t i = 0; i + 2 < syncs.size();) {
        if ((syncs[i + 1] != syncs[i] + 16) || (syncs[i + 2] != syncs[i] + 32)) {
            ++i;
            continue;
        }
        const qint64 nMark = syncs[i] + 48;
        quint8 nMarkValue = 0;
        if (!hfeMfmByte(bits, nMark, &nMarkValue)) break;
        if (nMarkValue == 0xfe) {
            if (hfeMfmBytes(bits, nMark + 16, 6, &field)) {
                const quint8 preamble[4] = {0xa1, 0xa1, 0xa1, 0xfe};
                quint16 nCrc = hfeCrc16(preamble, 4, 0xffff);
                nCrc = hfeCrc16(field.data(), 4, nCrc);
                const quint16 nStored = quint16((field[4] << 8) | field[5]);
                identifier = HFESector();
                identifier.nCylinder = field[0];
                identifier.nHead = field[1];
                identifier.nSector = field[2];
                identifier.nSizeCode = field[3] & 7;
                identifier.bValid = (nCrc == nStored);
                bHaveId = true;
            }
        } else if (((nMarkValue == 0xfb) || (nMarkValue == 0xf8)) && bHaveId) {
            const qint32 nSize = 128 << identifier.nSizeCode;
            if (hfeMfmBytes(bits, nMark + 16, nSize + 2, &field)) {
                const quint8 preamble[4] = {0xa1, 0xa1, 0xa1, nMarkValue};
                quint16 nCrc = hfeCrc16(preamble, 4, 0xffff);
                nCrc = hfeCrc16(field.data(), nSize, nCrc);
                const quint16 nStored =
                    quint16((field[size_t(nSize)] << 8) |
                            field[size_t(nSize) + 1]);
                HFESector sector = identifier;
                sector.bValid = identifier.bValid && (nCrc == nStored);
                sector.data.assign(field.begin(), field.begin() + nSize);
                pSectors->push_back(sector);
            }
            bHaveId = false;
        }
        i += 3;
    }
}

struct HFEKey {
    qint32 nCylinder;
    qint32 nHead;
    qint32 nSector;
    bool operator<(const HFEKey &other) const
    {
        if (nCylinder != other.nCylinder) return nCylinder < other.nCylinder;
        if (nHead != other.nHead) return nHead < other.nHead;
        return nSector < other.nSector;
    }
};

// Walks every track once, collecting the sectors.  bGeometryOnly stops after
// cylinder 0, which is all probeGeometry() needs.
bool hfeCollect(const QByteArray &baFile, const HFEHeader &header,
                bool bGeometryOnly, std::map<HFEKey, HFESector> *pSectors,
                qint32 *pnMaxSector, qint32 *pnSectorSize,
                XBinary::PDSTRUCT *pPdStruct)
{
    std::vector<quint8> side0;
    std::vector<quint8> side1;
    std::vector<HFESector> found;
    const qint32 nTrackLimit = bGeometryOnly ? 1 : header.nTracks;
    for (qint32 nTrack = 0; nTrack < nTrackLimit; ++nTrack) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (!hfeTrackBits(baFile, header, nTrack, &side0, &side1)) return false;
        for (qint32 nSide = 0; nSide < header.nSides; ++nSide) {
            found.clear();
            hfeDecodeTrack(nSide ? side1 : side0, &found);
            for (const HFESector &sector : found) {
                if ((sector.nSector <= 0) ||
                    (sector.nSector > HFE_MAX_SECTORS)) {
                    continue;
                }
                if (*pnSectorSize == 0) {
                    *pnSectorSize = 128 << sector.nSizeCode;
                } else if (*pnSectorSize != (128 << sector.nSizeCode)) {
                    // Mixed sector sizes cannot be laid out as a flat image.
                    return false;
                }
                if (sector.nSector > *pnMaxSector) {
                    *pnMaxSector = sector.nSector;
                }
                const HFEKey key = {sector.nCylinder, sector.nHead,
                                    sector.nSector};
                (*pSectors)[key] = sector;
            }
        }
    }
    return (*pnMaxSector > 0) && (*pnSectorSize > 0);
}

}  // namespace

bool XHFEDecoder::probeGeometry(const QByteArray &baFile, GEOMETRY *pGeometry,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pGeometry) return false;
    HFEHeader header;
    if (!hfeParseHeader(baFile, &header)) return false;

    std::map<HFEKey, HFESector> sectors;
    qint32 nMaxSector = 0;
    qint32 nSectorSize = 0;
    if (!hfeCollect(baFile, header, true, &sectors, &nMaxSector, &nSectorSize,
                    pPdStruct)) {
        return false;
    }

    pGeometry->nTracks = header.nTracks;
    pGeometry->nSides = header.nSides;
    pGeometry->nSectorsPerTrack = nMaxSector;
    pGeometry->nSectorSize = nSectorSize;
    pGeometry->nImageSize = qint64(header.nTracks) * header.nSides *
                            nMaxSector * nSectorSize;
    return (pGeometry->nImageSize > 0) &&
           (pGeometry->nImageSize <= HFE_MAX_IMAGE_SIZE);
}

bool XHFEDecoder::decode(const QByteArray &baFile, qint64 nImageSize,
                         QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nImageSize <= 0) || (nImageSize > HFE_MAX_IMAGE_SIZE)) {
        return false;
    }
    HFEHeader header;
    if (!hfeParseHeader(baFile, &header)) return false;

    std::map<HFEKey, HFESector> sectors;
    qint32 nMaxSector = 0;
    qint32 nSectorSize = 0;
    if (!hfeCollect(baFile, header, false, &sectors, &nMaxSector, &nSectorSize,
                    pPdStruct)) {
        return false;
    }
    const qint64 nExpected = qint64(header.nTracks) * header.nSides *
                             nMaxSector * nSectorSize;
    if (nExpected != nImageSize) return false;

    QByteArray baImage(qint32(nImageSize), 0);
    char *pImage = baImage.data();
    qint64 nCursor = 0;
    for (qint32 nCylinder = 0; nCylinder < header.nTracks; ++nCylinder) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        for (qint32 nHead = 0; nHead < header.nSides; ++nHead) {
            for (qint32 nSector = 1; nSector <= nMaxSector; ++nSector) {
                const HFEKey key = {nCylinder, nHead, nSector};
                const std::map<HFEKey, HFESector>::iterator it =
                    sectors.find(key);
                if ((it != sectors.end()) && it->second.bValid &&
                    (qint32(it->second.data.size()) == nSectorSize)) {
                    memcpy(pImage + nCursor, it->second.data.data(),
                           size_t(nSectorSize));
                }
                // A missing or CRC-failed sector stays as zeroes so the image
                // keeps its geometry.
                nCursor += nSectorSize;
            }
        }
    }
    *pOutput = baImage;
    return true;
}
