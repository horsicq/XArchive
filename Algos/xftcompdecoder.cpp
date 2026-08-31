/*
 * IBM OS/2 PACK2 fT19 decoder.
 *
 * This is a clean-room implementation of the format behavior observed in
 * original PACK2 streams and their decoder.  fT19 has two stages: a compact
 * block Huffman transform expands into a framed byte-oriented LZ stream, then
 * the LZ stream expands through a 512 KiB circular history initialized with a
 * format-defined preset dictionary.
 *
 * Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xftcompdecoder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace {
const qint32 FTCOMP_LEAF_COUNT = 433;
const qint32 FTCOMP_LEAF_LIMIT = FTCOMP_LEAF_COUNT * 4;
const qint32 FTCOMP_FIRST_INTERNAL = 0x710;
const qint32 FTCOMP_NODE_CAPACITY = 908;
const qint32 FTCOMP_WINDOW_SIZE = 0x80000;
const qint32 FTCOMP_WINDOW_MASK = FTCOMP_WINDOW_SIZE - 1;
const qint32 FTCOMP_PRESET_OFFSET = 0x3c0;
const qint32 FTCOMP_PRESET_SIZE = 0xfba;
const qint32 FTCOMP_INITIAL_POSITION = FTCOMP_PRESET_OFFSET + FTCOMP_PRESET_SIZE;
const qint32 FTCOMP_IO_CHUNK_SIZE = 0x10000;

// Only symbols 0..256 have weight in these two format-defined bootstrap
// models. The remaining 176 entries are zero-filled by makeBootstrapWeights().
const quint16 FTCOMP_HEADER_WEIGHTS[257] = {
    1024, 600, 300, 260, 230, 212, 192, 172, 148, 132, 120, 108, 92, 84, 80, 76,
    72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 22, 20, 19,
    18, 17, 16, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9,
    9, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 4,
    4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2,
    2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4,
    15
};

const quint16 FTCOMP_FOLLOW_WEIGHTS[257] = {
    40, 39, 39, 38, 38, 37, 37, 36, 36, 35, 35, 34, 34, 33, 33, 32,
    32, 31, 31, 30, 30, 29, 29, 28, 28, 27, 26, 25, 24, 24, 23, 23,
    22, 22, 21, 21, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17, 17,
    16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15, 14,
    14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12,
    11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 7, 7, 7,
    255
};

const QByteArray &ftcompPreset()
{
    // The preset is an intrinsic 4,026-byte part of the fT19 format. Base64
    // keeps the otherwise mostly textual/binary table compact in this source.
    static const char BASE64[] =
        "PDxOTVNHPj5SNjAwMCAtIHN0YWNrIG92ZXJmbG93IFI2MDAzIC0gaW50ZWdlciBkaXZpZGUgYnkgMCBSNjAwOAAALSBub3QgZW5v"
        "dWdoIHNwYWNlIGZvciBhcmd1bWVudHMgUjYwMDkgLSBub3QgZW5vdWdoIHNwYWNlIGZvciBlbnZpcm9ubWVudCBydW4tdGltZSBl"
        "cnJvciBSNjAwMiAtIGZsb2F0aW5nIHBvaW50IG5vdCBsb2FkZWQgUjYwMDEgLSBudWxsIHBvaW50ZXIgYXNzaWdubWVudCBBIHBy"
        "b2dyYW0gdHJpZWQgdG8gZGl2aWRlIGEgbnVtYmVyIGJ5IHplcm8sIGFuZCBhIGRpdmlkZSBieSB6ZXJvIGV4Y2VwdGlvbiBoYW5k"
        "bGVyIHdhcyBub3QgcmVnaXN0ZXJlZCBUaGUgcmVzdWx0IG9mIGEgZGl2aXNpb24gb3BlcmF0aW9uIHdhcyB0b28gbGFyZ2UsIGFu"
        "ZCBhIGRpdmlkZSBvdmVyZmxvdyBleGNlcHRpb24gaGFuZGxlciB3YXMgbm90IHJlZ2lzdGVyZWQgQSBwcm9ncmFtIHN0YXJ0ZWQg"
        "YSBCT1VORCBpbnN0cnVjdGlvbiB3aXRob3V0IHJlZ2lzdGVyaW5nIGEgYm91bmQgZXhjZXB0aW9uIGhhbmRsZXIgQSBwcm9ncmFt"
        "IHN0YXJ0ZWQgYW4gaW52YWxpZCBpbnN0cnVjdGlvbiB3aXRob3V0IHJlZ2lzdGVyaW5nIGFuIGludmFsaWQgb3Bjb2RlIGV4Y2Vw"
        "dGlvbiBoYW5kbGVyIEEgbnVtZXJpYyBjb3Byb2Nlc3NvciBleGNlcHRpb24gb2NjdXJyZWQgYW5kIGEgbnVtZXJpYyBjb3Byb2Nl"
        "c3NvciBleGNlcHRpb24gaGFuZGxlciB3YXMgbm90IHJlZ2lzdGVyZWQgU1lTMjA5MDogVGhlIHN5c3RlbSBpcyB1bmFibGUgdG8g"
        "bG9hZCB0aGUgcHJvZ3JhbS4gVGhlIEFwcGxpY2F0aW9uIFByb2dyYW0gSW50ZXJmYWNlIChBUEkpIGVudGVyZWQgd2lsbCBvbmx5"
        "IHdvcmsgaW4gT1MvMiBtb2RlLiBQU1FSVldTUVIgUEFUSD09IElEPUVIQyBWZXJzaW9uIChDKSAoYykgQ29weXJpZ2h0IElCTSBD"
        "b3JwLiBMaWNlbnNlZCBNYXRlcmlhbCBNU19SdW4gVGltZSBMaWJyYXJ5IEJvcmxhbmQgQk9STEFORCAgIMDExMTExMTExMTExMTE"
        "xMTExMTExMTExMTExMTZw8TExMTExMTExMTExMTExMTExMTExLTaxMTExMTExMTExMTExMTExMTExMTExL/Izc3Nzc3Nzc3Nzc3N"
        "zc3Nzc28yc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzbvMzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzbkrLS0tLS0tLS0tLS0tLS0tLS0rKioq"
        "KioqKioqKioqKioqKioqKioqKio9PT09PT09PT09PT09PT09PT09PT09PT0vLy8vLy8vLy8vLy8vLy8vLy8vIyMjIyMjIyMjIyMj"
        "IyMjIyMjIyMjIyMkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCRAQEBAQEBAQEBAQEBAQEBAQEBAICAgICAgICAgICAgICAgICAgICAg"
        "IDs/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8rKysrKysrKysrKysrKysrKysr1M3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzb7Gzc3Nzc3N"
        "zc3Nzc3Nzc3Nzc3Nzc3NzbXVzc3Nzc3Nzc3Nzc3Nzc3Nzc2408TExMTExMTExMTExMTExMTExMTExL3HxMTExMTExMTExMTExMTE"
        "xMTExMTExLbWxMTExMTExMTExMTExMTExMS3IyQlJicoKSorLC0uLzAxMjM0NTY3ODk6Ozw9Pj9AQUJDREVGR0hJSktMTU5PUFFS"
        "U1RVVldYWVpbXV5fYGFiY2QAZWZnaGlqa2xtbm9wcXJzdHV2d3h5ent8fX5/gIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5yd"
        "np+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo"
        "6err7O3u7/Dx8vP09fb3+Pn6+/z9/lJ1bnRpbWUgZXJyb3IgT1MvMiAyLjAgV2luZG93cyBXSU5ET1dTIERPUyA1LjAgRE9TIDQu"
        "MCBET1MgMy4zIERPUyAzLjIgRE9TIDMuMSBET1MgMy4wIFByb2dyYW0gcHJvZ3JhbSBQUk9HUkFNTWVtb3J5IENvbnRyb2wgQmxv"
        "Y2sgRXhwYW5kZWQgRXh0ZW5kZWQgRElSIERpcmVjdG9yeSBESVJFQ1RPUlkgUHJvbXB0IEVudGVyIGVudGVyIFF1aXQgUVVJVCBG"
        "aWxlIGZpbGUgRklMRSBNYW5hZ2VyIE1BTkFHRVIgRm9sZGVyIEZPTERFUiBFeGFtcGxlIEVYQU1QTEUgSW5zZXJ0IElOU0VSVCBP"
        "dmVyd3JpdGUgT1ZFUldSSVRFIFVuZG8gVU5ETyBJbnZhbGlkIElOVkFMSUQgTm90IEZvdW5kIE5PVCBGT1VORCBSZXRyeSBhZ2Fp"
        "biBSRVRSWSBBR0FJTiBEaXNrZXR0ZSBESVNLRVRURSBIYXJkIERpc2sgSEFSRCBESVNLIEhpZ2ggUGVyZm9ybWFuY2UgSElHSCBQ"
        "RVJGT1JNQU5DRSBRdWV1ZSBRVUVVRSBVbm5hbWVkIFBpcGVzIFVOTkFNRUQgUElQRVMgVW5uYW1lZCBTZW1hcGhvcmVzIFVOTkFN"
        "RUQgU0VNQVBIT1JFUyBTY3JlZW4gU2F2ZSBTQ1JFRU4gU0FWRQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUVdFUlRZ"
        "VUlPUHt9fEFTREZHSEpLTDpaWENWQk5NPD4/cXdlcnR5dWlvcFtdB3NkZmdoamtsOyd6eGN2Ym5tLC4AUHJvZ3JhbSBQcm9wZXJ0"
        "eSBvZiBJQk0uIEFsbCBSaWdodHMgUmVzZXJ2ZWQgQ29tbWFuZCBIZWxwIEVkaXQgSUYgRVJST1JMRVZFTCAxIElmIEVycm9ybGV2"
        "ZWwgMSBHb3RvIEdPVE8gU0VUIFBBVEg9IHNldCBwYXRoPSBERVZJQ0U9RGV2aWNlPSBERVZJQ0VISUdIPSBEZXZpY2VoaWdoPSBa"
        "b29tIFpPT00gQ09QWSBYQ09QWSBjb3B5IHhjb3B5IFNIRUxMPSBTaGVsbD0gTEFORFAgRkJTUyBGaW5hbmNpYWwgQnJhbmNoIFN5"
        "c3RlbSBTZXJ2aWNlcyB1bnNpZ25lZCBsb25nIHNob3J0IFVDSEFSIEJZVEUgVUxPTkcgY2RlY2wgUkVBTCByZWFsIGZsb2F0IHJl"
        "dHVybih2b2lkKSBET1NDQUxMUy5ETEwgS0JEQ0FMTFMuRExMIE9TQ0hBUi5ETEwgVklPQ0hBUi5ETEwgTU9VQ0FMTFMuRExMIFFV"
        "RUNBTExTLkRMTCBQTUdQSS5ETEwgUE1HUkUuRExMIFBNV0lOLkRMTCBQTVNIQVBJLkRMTCBTRVNNR1IuRExMIE5BTVBJUEVTLkRM"
        "TCBORVRBUEkuRExMIE5FVFNQT09MLkRMTCBNU0dfRklMRV9OT1QgcmVwZWF0IFJFUEVBVCB1bnRpbCBVTlRJTCBVTklUIHVuaXQg"
        "Ki5FWEUgKi5DT00gKi5PQkogKi5ETEwgYnl0ZXMgQllURVMgX0NfRklMRV9JTkZPPSAobnVsbCkgLyogICAgICAqLyAgLy8gICBz"
        "dHJyY2hyKCBzdHJjcHkoIHN0cmNhdCggc3RyY3B5KCBtZW1tb3ZlKCBtZW1jcHkoIGNvbmNhdCggY2FsbCggVENQSVAgQ09ORklH"
        "VVJBVElPTiBDb25maWd1cmF0aW9uIFNlcnZlciBSZXF1ZXN0ZXIgU0VSVkVSIFJFUVVFU1RFUiBTZXJ2aWNlcyBTRVJWSUNFUyB1"
        "bmtub3duIHBhcmFtZXRlcnMga2V5IHRvIGNvbnRpbnVlIGNoYXIgKiB1bnNpZ25lZCBpbnQgYXV0byByZWdpc3RlciB2YXJpYWJs"
        "ZSB0eXBlZGVmIHN0cnVjdCBzdGF0aWMgICAgICAgICAgDQogICAgICAgICAgI2RlZmluZSBpbmNsdWRlPCBtZW1jbXAoIHdoaWxl"
        "KCAgAAAAAAAAZm9yKCA7Ozs7Ozs7Ozs7Ozs7Ozs7Ozs6Ojo6Ojo6Ojo6Ojo6Oi4uLi4uLi4uLi4uLi4uLi49PT09PT09PT09PT09"
        "IGJlZ2luIGVuZCBCRUdJTiBFTkQgV0hJTEUgRU5ESUYgSUZFTkQgSU5URVJGQUNFIGludGVyZmFjZSBzZWN0aW9uIFNFQ1RJT04g"
        "ICgqICoqKioqKioqKioqKiogKikgQ09QWVJJR0hUIChDKSBMSUNFTlNFRCBNQVRFUklBTCAgICAgIFJlbGVhc2UgRGF0ZSBUaW1l"
        "IEhvdXJzIE1pbnV0ZXMgU2Vjb25kcyBPcGVuKCBDbG9zZSggV3JpdGUoIFJlYWQoIE1TIFJ1bi1UaW1lIExpYnJhcnkgLSB1c2Ug"
        "b25seSBVU0hPUlQgdW5hdXRob3JpemVkIHBhc3N3b3JkIHVzZXIgIHN3aXRjaCBjYXNlIGRlZmF1bHQgYnJlYWsgc2l6ZW9mKCBw"
        "cmludGYoIHdyaXRlbG4oIHNhbXBsZSBjb25zdCAgICAgICAgIH0NCiAgICAgICB7DQogICAgICAgIDsNCiAgICAgICAgICAgICAg"
        "ICAgAAAAAAAAAAAAAAAAAAAAAAAAIE1pY3Jvc29mdCBDb3Jwb3JhdGlvbiBMb3R1cyBDb3Jwb3JhdGlvbiBTVEFDSyBQQVRDSCBB"
        "UkVBIDgwODggMzg2IFRoaXMgcHJvZ3JhbSBjYW5ub3QgYmUgcnVuIGluIGEgRE9TIHNlc3Npb24gRUhDT1MyIFJNVFJFUSBHRVRS"
        "RVEgUk1UUlBMWSBTUlZJTklUIEVIQ01FU1NBR0UgTE9BREVSIGxvYWRlciBDUFJCLmNwcmIuIFJNVEFSRVEgUmM9JTEgcHJpbnRl"
        "ciBscHQxOiBscHQyOiBjb20xOiBjb20yOiBudWw6IExQVDE6IExQVDI6IENPTTE6IENPTTI6IE5VTDog////////////////////"
        "/////////////wABAgMEBQZVieUxwFBVi+yD7CCL5V3KDAAAAAAAAAAAAAAAAAAAAAAA";
    static const QByteArray result = QByteArray::fromBase64(
        QByteArray(BASE64, qint32(sizeof(BASE64) - 1)));
    return result;
}

bool readU16(const QByteArray &data, qint64 nOffset, quint16 *pValue)
{
    if (!pValue || (nOffset < 0) || (nOffset > data.size() - 2)) return false;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    *pValue = quint16(p[nOffset]) | (quint16(p[nOffset + 1]) << 8);
    return true;
}

std::array<quint16, FTCOMP_LEAF_COUNT> makeBootstrapWeights(
    const quint16 *pValues)
{
    std::array<quint16, FTCOMP_LEAF_COUNT> result = {};
    std::copy(pValues, pValues + 257, result.begin());
    return result;
}

class FtcompBitReader
{
public:
    FtcompBitReader(const QByteArray &data, qint64 nOffset)
        : m_data(data), m_nStart(nOffset), m_nBitPosition(nOffset * 8),
          m_nEndBit(qint64(data.size()) * 8)
    {
        if ((nOffset < 0) || (nOffset > data.size() - 4)) return;
        const uchar *p = reinterpret_cast<const uchar *>(data.constData());
        for (qint32 i = 0; i < 4; ++i) m_seed[i] = p[nOffset + i];
        m_nBitPosition += 32;
        m_bValid = true;
    }

    bool readBit(quint32 *pValue)
    {
        if (!pValue || !m_bValid || (m_nBitPosition >= m_nEndBit)) {
            m_bValid = false;
            return false;
        }
        const uchar *p = reinterpret_cast<const uchar *>(m_data.constData());
        const qint64 nByte = m_nBitPosition >> 3;
        const qint32 nShift = 7 - qint32(m_nBitPosition & 7);
        *pValue = (p[nByte] >> nShift) & 1U;
        ++m_nBitPosition;
        return true;
    }

    bool readBits(qint32 nCount, quint32 *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 16)) return false;
        quint32 nValue = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            quint32 nBit = 0;
            if (!readBit(&nBit)) return false;
            nValue = (nValue << 1) | nBit;
        }
        *pValue = nValue;
        return true;
    }

    quint8 seed(qint32 nIndex) const { return m_seed[nIndex]; }
    qint64 consumed() const
    {
        return ((m_nBitPosition + 7) >> 3) - m_nStart;
    }

private:
    const QByteArray &m_data;
    qint64 m_nStart = 0;
    qint64 m_nBitPosition = 0;
    qint64 m_nEndBit = 0;
    quint8 m_seed[4] = {};
    bool m_bValid = false;
};

class FtcompHuffmanTree
{
public:
    bool build(const std::array<quint16, FTCOMP_LEAF_COUNT> &source)
    {
        m_weight.fill(0);
        m_left.fill(0);
        m_right.fill(0);
        m_bValid = false;
        for (qint32 i = 0; i < FTCOMP_LEAF_COUNT; ++i) m_weight[i] = source[i];

        std::array<quint16, FTCOMP_LEAF_COUNT> ordered = {};
        qint32 nCount = 0;
        qint32 nOnes = 0;
        quint16 nLastZero = 0;
        for (qint32 i = 0; i < FTCOMP_LEAF_COUNT; ++i) {
            const quint16 nNode = quint16(i * 4);
            if (source[i] == 0) {
                nLastZero = nNode;
            } else if (source[i] == 1) {
                const quint16 nDisplaced = (nOnes < nCount) ? ordered[nOnes] : 0;
                ordered[nCount++] = nDisplaced;
                ordered[nOnes++] = nNode;
            } else {
                ordered[nCount++] = nNode;
            }
        }
        if (nCount == 0) return false;
        if (nCount == 1) {
            ordered[nCount++] = nLastZero;
            m_weight[nLastZero / 4] = 1;
        }
        if (!sortNodes(&ordered, nOnes, nCount - 1)) return false;

        qint32 nCursor = 0;
        qint32 nRemaining = nCount;
        quint16 nNextNode = FTCOMP_FIRST_INTERNAL;
        while (nRemaining != 2) {
            --nRemaining;
            const quint16 nFirst = ordered[nCursor];
            const quint16 nSecond = ordered[nCursor + 1];
            ++nCursor;
            if (!validNode(nFirst) || !validNode(nSecond) ||
                !validNode(nNextNode)) {
                return false;
            }
            const quint32 nTotal = quint32(m_weight[nFirst / 4]) +
                                   quint32(m_weight[nSecond / 4]);
            qint32 nLow = nCursor + 1;
            qint32 nHigh = nCount;
            while (nLow < nHigh) {
                const qint32 nMiddle = (nLow + nHigh) >> 1;
                if (m_weight[ordered[nMiddle] / 4] >= nTotal)
                    nHigh = nMiddle;
                else
                    nLow = nMiddle + 1;
            }
            const qint32 nInsertion = (nLow + nHigh) >> 1;
            if ((nInsertion <= nCursor) || (nInsertion > nCount)) return false;
            for (qint32 i = nCursor; i < nInsertion - 1; ++i)
                ordered[i] = ordered[i + 1];
            ordered[nInsertion - 1] = nNextNode;

            const qint32 nIndex = nNextNode / 4;
            m_weight[nIndex] = quint16(nTotal);
            m_left[nIndex] = nFirst;
            m_right[nIndex] = nSecond;
            nNextNode = quint16(nNextNode + 4);
        }

        const quint16 nFirst = ordered[nCursor];
        const quint16 nSecond = ordered[nCursor + 1];
        if (!validNode(nFirst) || !validNode(nSecond) || !validNode(nNextNode))
            return false;
        m_nRoot = nNextNode;
        const qint32 nIndex = nNextNode / 4;
        m_weight[nIndex] = quint16(quint32(m_weight[nFirst / 4]) +
                                   quint32(m_weight[nSecond / 4]));
        m_left[nIndex] = nFirst;
        m_right[nIndex] = nSecond;
        m_bValid = true;
        return true;
    }

    bool decode(FtcompBitReader *pReader, quint16 *pSymbol) const
    {
        if (!pReader || !pSymbol || !m_bValid) return false;
        quint16 nNode = m_nRoot;
        for (qint32 nGuard = 0; nGuard < FTCOMP_NODE_CAPACITY; ++nGuard) {
            if (nNode < FTCOMP_LEAF_LIMIT) {
                if ((nNode & 3) || (nNode / 4 >= FTCOMP_LEAF_COUNT)) return false;
                *pSymbol = quint16(nNode / 4);
                return true;
            }
            if (!validNode(nNode) || (nNode & 3)) return false;
            quint32 nBit = 0;
            if (!pReader->readBit(&nBit)) return false;
            nNode = nBit ? m_left[nNode / 4] : m_right[nNode / 4];
        }
        return false;
    }

private:
    bool validNode(quint16 nNode) const
    {
        return (nNode / 4) < FTCOMP_NODE_CAPACITY;
    }

    qint32 compareNodes(const std::array<quint16, FTCOMP_LEAF_COUNT> &ordered,
                        qint32 nFirst, qint32 nSecond) const
    {
        return qint32(m_weight[ordered[nFirst] / 4]) -
               qint32(m_weight[ordered[nSecond] / 4]);
    }

    bool sortNodes(std::array<quint16, FTCOMP_LEAF_COUNT> *pOrdered,
                   qint32 nLow, qint32 nHigh)
    {
        if (!pOrdered || (nLow < 0) || (nHigh >= FTCOMP_LEAF_COUNT)) return false;
        if (nLow >= nHigh) return true;
        std::vector<std::pair<qint32, qint32> > stack;
        stack.push_back(std::make_pair(nLow, nHigh));
        while (!stack.empty()) {
            nLow = stack.back().first;
            nHigh = stack.back().second;
            stack.pop_back();
            while (nLow < nHigh) {
                if (nHigh - nLow > 16) {
                    qint32 nLeft = nLow;
                    qint32 nRight = nHigh;
                    qint32 nPivot = (nLow + nHigh) >> 1;
                    while (nLeft <= nRight) {
                        while ((nLeft <= nHigh) &&
                               (compareNodes(*pOrdered, nLeft, nPivot) < 0))
                            ++nLeft;
                        while ((nRight >= nLow) &&
                               (compareNodes(*pOrdered, nRight, nPivot) > 0))
                            --nRight;
                        if (nLeft <= nRight) {
                            std::swap((*pOrdered)[nLeft], (*pOrdered)[nRight]);
                            if (nPivot == nLeft)
                                nPivot = nRight;
                            else if (nPivot == nRight)
                                nPivot = nLeft;
                            ++nLeft;
                            --nRight;
                        }
                    }
                    if (nHigh - nLeft > nRight - nLow) {
                        if (nLow < nRight)
                            stack.push_back(std::make_pair(nLow, nRight));
                        nLow = nLeft;
                    } else {
                        if (nLeft < nHigh)
                            stack.push_back(std::make_pair(nLeft, nHigh));
                        nHigh = nRight;
                    }
                } else {
                    for (qint32 nCurrent = nLow + 1; nCurrent <= nHigh;
                         ++nCurrent) {
                        qint32 nInsertion = nLow;
                        while ((nInsertion < nCurrent) &&
                               (compareNodes(*pOrdered, nInsertion,
                                             nCurrent) < 0))
                            ++nInsertion;
                        for (qint32 nScan = nCurrent - 1;
                             nScan >= nInsertion; --nScan)
                            std::swap((*pOrdered)[nScan],
                                      (*pOrdered)[nScan + 1]);
                    }
                    nLow = nHigh;
                }
            }
        }
        return true;
    }

    std::array<quint16, FTCOMP_NODE_CAPACITY> m_weight = {};
    std::array<quint16, FTCOMP_NODE_CAPACITY> m_left = {};
    std::array<quint16, FTCOMP_NODE_CAPACITY> m_right = {};
    quint16 m_nRoot = 0;
    bool m_bValid = false;
};

qint32 contextClass(quint16 nSymbol)
{
    return ((nSymbol >= 0x100) && (nSymbol <= 0x13f)) ||
           ((nSymbol >= 0x141) && (nSymbol <= 0x180)) ||
           ((nSymbol >= 0x1a1) && (nSymbol <= 0x1b0));
}

qint32 commandClass(quint8 nControl)
{
    if (nControl < 0x40) return 1;
    if (nControl == 0x40) return 0;
    if (nControl < 0x80) return 2;
    if (nControl == 0x80) return 3;
    return -1;
}

std::array<quint16, FTCOMP_LEAF_COUNT> scaleModel(
    const std::array<quint16, FTCOMP_LEAF_COUNT> &model, quint8 nZeroScale,
    quint8 nOneScale)
{
    std::array<quint16, FTCOMP_LEAF_COUNT> result = {};
    quint32 nMaximum = 0;
    for (qint32 i = 0; i < FTCOMP_LEAF_COUNT; ++i) {
        const quint32 nValue = quint32(model[i]) *
            quint32(contextClass(quint16(i)) ? nOneScale : nZeroScale);
        result[i] = quint16(nValue);
        nMaximum = qMax(nMaximum, nValue);
    }
    if (nMaximum > 0xff) {
        const quint32 nFactor = 0xffffU / nMaximum;
        for (qint32 i = 0; i < FTCOMP_LEAF_COUNT; ++i) {
            if (result[i]) {
                const quint32 nValue = (quint32(result[i]) * nFactor) >> 8;
                result[i] = quint16(qMax(1U, nValue));
            }
        }
    }
    return result;
}

bool appendByte(QByteArray *pOutput, qint32 nLimit, quint32 nValue)
{
    if (!pOutput || (nValue > 0xff) || (pOutput->size() >= nLimit)) return false;
    pOutput->append(char(nValue));
    return true;
}

bool appendWord(QByteArray *pOutput, qint32 nLimit, quint32 nValue)
{
    return (nValue <= 0xffff) && appendByte(pOutput, nLimit, nValue & 0xff) &&
           appendByte(pOutput, nLimit, nValue >> 8);
}

template <typename T>
void pushHistory(std::array<T, 16> *pHistory, const T &value)
{
    for (qint32 i = 15; i > 0; --i) (*pHistory)[i] = (*pHistory)[i - 1];
    (*pHistory)[0] = value;
}

bool decodeDense(const QByteArray &packed, qint64 nOffset,
                 quint16 nExpectedSize, const FtcompHuffmanTree &headerTree,
                 const FtcompHuffmanTree &followTree, QByteArray *pOutput,
                 qint64 *pConsumed, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || !pConsumed || (nExpectedSize == 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    FtcompBitReader reader(packed, nOffset);
    std::array<quint16, FTCOMP_LEAF_COUNT> model = {};
    qint32 nModelPosition = 0;
    while (nModelPosition < FTCOMP_LEAF_COUNT) {
        quint16 nSymbol = 0;
        if (!headerTree.decode(&reader, &nSymbol)) return false;
        if (nSymbol == 0x100) {
            const qint32 nCount = qMin(16, FTCOMP_LEAF_COUNT - nModelPosition);
            nModelPosition += nCount;
        } else if (nSymbol < 0x100) {
            model[nModelPosition++] = nSymbol;
        } else {
            return false;
        }
    }

    FtcompHuffmanTree firstTree;
    FtcompHuffmanTree secondTree;
    if (!firstTree.build(scaleModel(model, reader.seed(0), reader.seed(1))))
        return false;
    if ((reader.seed(0) == reader.seed(3)) &&
        (reader.seed(1) == reader.seed(2))) {
        secondTree = firstTree;
    } else if (!secondTree.build(scaleModel(model, reader.seed(3),
                                            reader.seed(2)))) {
        return false;
    }

    QByteArray output;
    output.reserve(nExpectedSize);
    qint32 nState = 0;
    qint32 nContext = 0;
    quint16 nLastState1 = 0;
    quint16 nLastMode0 = 0;
    quint16 nLastMode1 = 0;
    quint16 nLastMode2 = 0;
    quint16 nLastLongFinal = 0;
    std::array<quint16, 16> pairHistory = {};
    std::array<qint32, 16> commandHistory = {};

    while (output.size() < nExpectedSize) {
        if (((output.size() & 0x3fff) == 0) &&
            !XBinary::isPdStructNotCanceled(pPdStruct))
            return false;
        if (nState == 0) {
            quint16 nSymbol = 0;
            const FtcompHuffmanTree &tree = nContext ? secondTree : firstTree;
            if (!tree.decode(&reader, &nSymbol)) return false;
            nContext = contextClass(nSymbol);
            if (nSymbol <= 0xff) {
                if (!appendByte(&output, nExpectedSize, nSymbol)) return false;
            } else if (nSymbol < 0x181) {
                const quint8 nControl = quint8(nSymbol - 0x100);
                if (!appendByte(&output, nExpectedSize, 0x9e) ||
                    !appendByte(&output, nExpectedSize, nControl))
                    return false;
                nState = commandClass(nControl);
                if (nState < 0) return false;
                if (nState) pushHistory(&commandHistory, output.size() - 1);
            } else if (nSymbol < 0x191) {
                const qint32 nDistance = nSymbol - 0x17f;
                const qint32 nSource = output.size() - nDistance;
                if ((nSource < 0) || (nSource > output.size() - 2)) return false;
                const quint16 nWord = quint8(output.at(nSource)) |
                    (quint16(quint8(output.at(nSource + 1))) << 8);
                if (!appendWord(&output, nExpectedSize, nWord)) return false;
                pushHistory(&pairHistory, nWord);
            } else if (nSymbol < 0x1a1) {
                const qint32 nHistoryIndex = nSymbol - 0x191;
                const quint16 nWord = pairHistory[nHistoryIndex];
                if (!appendWord(&output, nExpectedSize, nWord)) return false;
                for (qint32 i = nHistoryIndex; i > 0; --i)
                    pairHistory[i] = pairHistory[i - 1];
                pairHistory[0] = nWord;
            } else if (nSymbol <= 0x1b0) {
                const qint32 nHistoryIndex = nSymbol - 0x1a1;
                const qint32 nSource = commandHistory[nHistoryIndex];
                if ((nSource < 0) || (nSource >= output.size())) return false;
                const quint8 nControl = quint8(output.at(nSource));
                if (commandClass(nControl) <= 0) return false;
                const qint32 nAvailable = (nControl == 0x80) ? 4 :
                    ((nControl & 0x40) ? 3 : 2);
                const qint32 nOldSize = output.size();
                if (nSource > nOldSize - nAvailable) return false;
                const qint32 nNewPosition = nOldSize + 1;
                if (!appendByte(&output, nExpectedSize, 0x9e)) return false;
                for (qint32 i = 0; i < nAvailable; ++i) {
                    if (!appendByte(&output, nExpectedSize,
                                    quint8(output.at(nSource + i))))
                        return false;
                }
                for (qint32 i = nHistoryIndex; i > 0; --i)
                    commandHistory[i] = commandHistory[i - 1];
                commandHistory[0] = nNewPosition;
            } else {
                return false;
            }
        } else if (nState == 1) {
            quint16 nSymbol = 0;
            if (!followTree.decode(&reader, &nSymbol)) return false;
            if (nSymbol == 0x100)
                nSymbol = nLastState1;
            else if (nSymbol <= 0xff)
                nLastState1 = nSymbol;
            else
                return false;
            if (!appendByte(&output, nExpectedSize, nSymbol)) return false;
            nState = 0;
        } else if (nState == 2) {
            quint32 nPrefix = 0;
            quint32 nBit = 0;
            qint32 nMode = 0;
            if (!reader.readBit(&nBit)) return false;
            if (!nBit) {
                if (!reader.readBits(4, &nPrefix)) return false;
            } else {
                if (!reader.readBit(&nBit)) return false;
                if (!nBit) {
                    nMode = 1;
                    if (!reader.readBits(6, &nPrefix)) return false;
                } else {
                    nMode = 2;
                    if (!reader.readBits(7, &nPrefix)) return false;
                }
            }
            quint16 nSymbol = 0;
            if (!followTree.decode(&reader, &nSymbol) || (nSymbol > 0x100))
                return false;
            quint32 nWord = 0;
            if (nMode == 0) {
                if (nSymbol == 0x100) nSymbol = nLastMode0;
                else nLastMode0 = nSymbol;
                nWord = 0x100U + quint32(nSymbol) * 16U + nPrefix;
            } else if (nMode == 1) {
                if (nSymbol == 0x100) nSymbol = nLastMode1;
                else nLastMode1 = nSymbol;
                nWord = 0x1100U + quint32(nSymbol) * 64U + nPrefix;
            } else {
                if (nSymbol == 0x100) nSymbol = nLastMode2;
                else nLastMode2 = nSymbol;
                nWord = (quint32(nSymbol) + 0xa2U) * 128U + nPrefix;
            }
            if (!appendWord(&output, nExpectedSize, nWord)) return false;
            nState = 0;
        } else if ((nState >= 3) && (nState < 6)) {
            quint16 nSymbol = 0;
            if (!followTree.decode(&reader, &nSymbol) || (nSymbol > 0x100))
                return false;
            ++nState;
            if (nState == 6) {
                nState = 0;
                if (nSymbol == 0x100)
                    nSymbol = nLastLongFinal;
                else
                    nLastLongFinal = nSymbol;
            } else {
                nSymbol = (nSymbol == 0x100) ? 0 : quint16(nSymbol + 1);
            }
            if (!appendByte(&output, nExpectedSize, nSymbol)) return false;
        } else {
            return false;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pConsumed = reader.consumed();
    if ((*pConsumed < 4) || (*pConsumed > packed.size() - nOffset)) return false;
    *pOutput = output;
    return true;
}

class FtcompLzState
{
public:
    bool initialize()
    {
        const QByteArray &preset = ftcompPreset();
        if (preset.size() != FTCOMP_PRESET_SIZE) return false;
        m_window = QByteArray(FTCOMP_WINDOW_SIZE, 0);
        std::memset(m_window.data(), 0x20, 0x140);
        std::memset(m_window.data() + 0x140, 0xff, 0x140);
        std::memcpy(m_window.data() + FTCOMP_PRESET_OFFSET,
                    preset.constData(), FTCOMP_PRESET_SIZE);
        m_nPosition = FTCOMP_INITIAL_POSITION;
        return true;
    }

    bool put(quint8 nValue, QByteArray *pOutput, qint32 nLimit)
    {
        if (!appendByte(pOutput, nLimit, nValue)) return false;
        m_window[m_nPosition] = char(nValue);
        m_nPosition = (m_nPosition + 1) & FTCOMP_WINDOW_MASK;
        return true;
    }

    bool copy(qint32 nDistance, qint32 nLength, QByteArray *pOutput,
              qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
    {
        if ((nDistance < 1) || (nDistance > 0x10000) || (nLength < 1) ||
            !pOutput || (nLength > nLimit - pOutput->size()))
            return false;
        qint32 nSource = (m_nPosition - nDistance) & FTCOMP_WINDOW_MASK;
        for (qint32 i = 0; i < nLength; ++i) {
            if (((i & 0xffff) == 0) &&
                !XBinary::isPdStructNotCanceled(pPdStruct))
                return false;
            const quint8 nValue = quint8(m_window.at(nSource));
            nSource = (nSource + 1) & FTCOMP_WINDOW_MASK;
            if (!put(nValue, pOutput, nLimit)) return false;
        }
        return true;
    }

private:
    QByteArray m_window;
    qint32 m_nPosition = 0;
};

bool decodeTokens(const QByteArray &data, qint32 nBegin, qint32 nEnd,
                  FtcompLzState *pLz, QByteArray *pOutput, qint32 nLimit,
                  XBinary::PDSTRUCT *pPdStruct)
{
    if (!pLz || !pOutput || (nBegin < 0) || (nBegin > nEnd) ||
        (nEnd > data.size()))
        return false;
    qint32 nPosition = nBegin;
    while (nPosition < nEnd) {
        if ((((nPosition - nBegin) & 0xffff) == 0) &&
            !XBinary::isPdStructNotCanceled(pPdStruct))
            return false;
        const quint8 nValue = quint8(data.at(nPosition++));
        if (nValue != 0x9e) {
            if (!pLz->put(nValue, pOutput, nLimit)) return false;
            continue;
        }
        if (nPosition >= nEnd) return false;
        const quint8 nControl = quint8(data.at(nPosition++));
        if (nControl == 0x40) {
            if (!pLz->put(0x9e, pOutput, nLimit)) return false;
            continue;
        }
        qint32 nLength = 0;
        qint32 nDistance = 0;
        if (nControl == 0x80) {
            if (nPosition > nEnd - 3) return false;
            nLength = quint8(data.at(nPosition)) + 0x43;
            nDistance = quint8(data.at(nPosition + 1)) |
                (qint32(quint8(data.at(nPosition + 2))) << 8);
            nPosition += 3;
        } else if (nControl & 0x40) {
            if (nPosition > nEnd - 2) return false;
            nLength = (nControl & 0x3f) + 3;
            nDistance = quint8(data.at(nPosition)) |
                (qint32(quint8(data.at(nPosition + 1))) << 8);
            nPosition += 2;
        } else {
            if (nPosition >= nEnd) return false;
            nLength = nControl + 3;
            nDistance = quint8(data.at(nPosition++));
        }
        if (!pLz->copy(nDistance + 1, nLength, pOutput, nLimit,
                       pPdStruct))
            return false;
    }
    return true;
}

bool decodeFrames(const QByteArray &data, FtcompLzState *pLz,
                  QByteArray *pOutput, qint32 nLimit,
                  XBinary::PDSTRUCT *pPdStruct)
{
    qint32 nPosition = 0;
    while (nPosition < data.size()) {
        quint16 nChunkSize = 0;
        if (!readU16(data, nPosition, &nChunkSize) || (nChunkSize < 2))
            return false;
        nPosition += 2;
        if (nChunkSize > data.size() - nPosition) return false;
        const qint32 nEnd = nPosition + nChunkSize;
        const quint8 nMode = quint8(data.at(nPosition++));
        if (nMode == 0) {
            for (; nPosition < nEnd; ++nPosition) {
                if (((nPosition & 0x3fff) == 0) &&
                    !XBinary::isPdStructNotCanceled(pPdStruct))
                    return false;
                if (!pLz->put(quint8(data.at(nPosition)), pOutput, nLimit))
                    return false;
            }
        } else {
            if (!decodeTokens(data, nPosition, nEnd, pLz, pOutput, nLimit,
                              pPdStruct))
                return false;
            nPosition = nEnd;
        }
    }
    return nPosition == data.size();
}
}  // namespace

bool XFtcompDecoder::decompress(XBinary::DATAPROCESS_STATE *pState,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput ||
        (pState->nInputOffset < 0) || (pState->nInputLimit < 8) ||
        (pState->nInputLimit > (std::numeric_limits<qint32>::max)()) ||
        !pState->mapProperties.contains(
            XBinary::FPART_PROP_UNCOMPRESSEDSIZE))
        return false;

    bool bRawSizeOK = false;
    const qint64 nRawSize = pState->mapProperties.value(
        XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bRawSizeOK);
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!bRawSizeOK || (nRawSize < 0) ||
        (nRawSize > (std::numeric_limits<qint32>::max)()) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nRawSize) ||
        (pState->nInputLimit > nMax - nRawSize) ||
        (pState->nProcessedOffset < 0) || (pState->nProcessedLimit < -1) ||
        ((pState->nProcessedLimit != -1) &&
         (pState->nProcessedOffset > nMax - pState->nProcessedLimit)))
        return false;

    // The adapter buffers the authoritative packed extent and decoded result;
    // account for both before allocating either one.
    XBinary::UNPACK_MEMORY_RESERVATION reservation;
    if (!reservation.acquire(pState->mapUnpackProperties,
                             pState->nInputLimit + nRawSize))
        return false;

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    if (!pState->pDeviceInput->isReadable() ||
        !pState->pDeviceInput->seek(pState->nInputOffset) ||
        (pState->pDeviceInput->pos() != pState->nInputOffset)) {
        pState->bReadError = true;
        return false;
    }
    if (!pState->pDeviceOutput->isWritable() ||
        (!pState->pDeviceOutput->isSequential() &&
         !pState->pDeviceOutput->seek(0)) ||
        (pState->pDeviceOutput->pos() != 0)) {
        pState->bWriteError = true;
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QByteArray packed(qint32(pState->nInputLimit), 0);
    qint64 nReadTotal = 0;
    while ((nReadTotal < pState->nInputLimit) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequest = qint32((std::min)(
            pState->nInputLimit - nReadTotal,
            qint64(FTCOMP_IO_CHUNK_SIZE)));
        const qint32 nRead = XBinary::_readDevice(
            packed.data() + nReadTotal, nRequest, pState);
        if ((nRead <= 0) || (nRead > nRequest)) {
            pState->bReadError = true;
            return false;
        }
        nReadTotal += nRead;
    }
    if ((nReadTotal != pState->nInputLimit) || pState->bReadError ||
        (pState->nCountInput != pState->nInputLimit) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    QByteArray unpacked;
    if (!decode(packed, nRawSize, &unpacked, pPdStruct) ||
        (unpacked.size() != nRawSize))
        return false;

    qint64 nWrittenTotal = 0;
    while ((nWrittenTotal < nRawSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = qint32((std::min)(
            nRawSize - nWrittenTotal, qint64(FTCOMP_IO_CHUNK_SIZE)));
        if (XBinary::_writeDevice(unpacked.constData() + nWrittenTotal,
                                  nChunk, pState) != nChunk)
            return false;
        nWrittenTotal += nChunk;
    }

    return (nWrittenTotal == nRawSize) && !pState->bReadError &&
           !pState->bWriteError &&
           (pState->nCountInput == pState->nInputLimit) &&
           (pState->nCountOutput == nRawSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XFtcompDecoder::decode(const QByteArray &packed, qint64 nRawSize,
                            QByteArray *pUnpacked,
                            XBinary::PDSTRUCT *pPdStruct)
{
    static const uchar MAGIC[8] = {0x80, 0x60, 0, 0,
                                   'f', 'T', '1', '9'};
    if (!pUnpacked || (packed.size() < 8) || (nRawSize < 0) ||
        (nRawSize > (std::numeric_limits<qint32>::max)()) ||
        (std::memcmp(packed.constData(), MAGIC, sizeof(MAGIC)) != 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const qint32 nLimit = qint32(nRawSize);
    QByteArray unpacked;
    unpacked.reserve(nLimit);
    if (nLimit == 0) {
        // The initial fT19 tag is the complete empty stream. As with a
        // non-empty member, only the observed opaque four-byte trailer may
        // follow it.
        const qint32 nTrailing = packed.size() - 8;
        if ((nTrailing != 0) && (nTrailing != 4)) return false;
        *pUnpacked = unpacked;
        return true;
    }

    FtcompHuffmanTree headerTree;
    FtcompHuffmanTree followTree;
    if (!headerTree.build(makeBootstrapWeights(FTCOMP_HEADER_WEIGHTS)) ||
        !followTree.build(makeBootstrapWeights(FTCOMP_FOLLOW_WEIGHTS)))
        return false;
    FtcompLzState lz;
    if (!lz.initialize()) return false;

    qint64 nPosition = 4;  // The first four-byte PACK2 prefix is consumed once.
    qint64 nBlockCount = 0;
    while (unpacked.size() < nLimit) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            (++nBlockCount > packed.size()))
            return false;
        if ((nPosition < 0) || (nPosition > packed.size() - 6) ||
            (std::memcmp(packed.constData() + nPosition, "fT19", 4) != 0))
            return false;
        nPosition += 4;
        quint16 nFirstSize = 0;
        if (!readU16(packed, nPosition, &nFirstSize)) return false;
        nPosition += 2;

        QByteArray intermediate;
        if (nFirstSize == 0xffff) {
            quint16 nStoredSize = 0;
            if (!readU16(packed, nPosition, &nStoredSize)) return false;
            nPosition += 2;
            if ((nStoredSize == 0) || (nPosition > packed.size() - nStoredSize))
                return false;
            intermediate = QByteArray(packed.constData() + nPosition,
                                      nStoredSize);
            nPosition += nStoredSize;
        } else {
            qint64 nConsumed = 0;
            if (!decodeDense(packed, nPosition, nFirstSize, headerTree,
                             followTree, &intermediate, &nConsumed,
                             pPdStruct) ||
                (nConsumed <= 0) || (nPosition > packed.size() - nConsumed))
                return false;
            nPosition += nConsumed;
        }

        const qint32 nBefore = unpacked.size();
        if (!decodeFrames(intermediate, &lz, &unpacked, nLimit, pPdStruct) ||
            (unpacked.size() <= nBefore))
            return false;
    }

    if ((unpacked.size() != nLimit) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    // PACK2 members either end at the last coded byte or carry one opaque
    // four-byte member trailer. Anything else is structural trailing junk.
    const qint64 nTrailing = packed.size() - nPosition;
    if ((nTrailing != 0) && (nTrailing != 4)) return false;
    *pUnpacked = unpacked;
    return true;
}
