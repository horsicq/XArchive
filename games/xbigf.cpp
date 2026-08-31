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
#include "xbigf.h"

#include <QDateTime>
#include <QDir>
#include <QPointer>
#include <QVector>
#include <QtEndian>

#include <algorithm>
#include <limits>
#include <new>

namespace {

const qint64 BIGF_HEADER_SIZE = 64;
const qint32 BIGF_DIRECTORY_RECORD_SIZE = 40;
const quint32 BIGF_MAX_RECORDS = 1000000;
const qint32 BIGF_MAX_NAME_SIZE = 65535;
const qint32 BIGF_IO_BUFFER_SIZE = 1 << 16;
const quint32 BIGF_LZW_CLEAR_CODE = 256;
const quint32 BIGF_LZW_FIRST_CODE = 257;
const qint32 BIGF_LZW_INITIAL_BITS = 9;
const qint32 BIGF_LZW_MAX_BITS = 24;
const quint32 BIGF_INVALID_PREFIX = (std::numeric_limits<quint32>::max)();

class BIGF_DEVICE_POSITION_GUARD {
public:
    explicit BIGF_DEVICE_POSITION_GUARD(QIODevice *pDevice)
        : m_pDevice(pDevice), m_nPosition(-1), m_bRestored(false)
    {
        if (m_pDevice && !m_pDevice->isSequential()) {
            m_nPosition = m_pDevice->pos();
        }
    }

    ~BIGF_DEVICE_POSITION_GUARD()
    {
        restore();
    }

    bool isValid() const
    {
        return m_pDevice && (m_nPosition >= 0);
    }

    bool restore()
    {
        if (m_bRestored) return m_pDevice && (m_nPosition >= 0);
        m_bRestored = true;
        return m_pDevice && (m_nPosition >= 0) &&
               m_pDevice->seek(m_nPosition) &&
               (m_pDevice->pos() == m_nPosition);
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

quint16 bigfReadLE16(const uchar *pData)
{
    return qFromLittleEndian<quint16>(pData);
}

quint32 bigfReadLE32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

quint64 bigfReadLE64(const uchar *pData)
{
    return qFromLittleEndian<quint64>(pData);
}

bool bigfToSignedOffset(quint64 nValue, qint64 *pResult)
{
    if (!pResult ||
        (nValue > (quint64)(std::numeric_limits<qint64>::max)())) {
        return false;
    }
    *pResult = (qint64)nValue;
    return true;
}

bool bigfRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) &&
           (nSize <= (nTotalSize - nOffset));
}

void bigfDecodeDirectoryRecord(QByteArray *pData)
{
    static const quint8 key[16] = {
        0x32, 0xf3, 0x1e, 0x06, 0x45, 0x70, 0x32, 0xaa,
        0x55, 0x3f, 0xf1, 0xde, 0xa3, 0x44, 0x21, 0xb4
    };

    if (!pData) return;
    quint8 nChain = (quint8)pData->size();
    for (qint32 i = 0; i < pData->size(); ++i) {
        const quint8 nEncoded = (quint8)pData->at(i);
        (*pData)[i] = (char)(nEncoded ^ key[nChain & 15]);
        nChain = nEncoded;
    }
}

bool bigfNormalizeName(const QByteArray &baName, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pResult || baName.isEmpty() || baName.contains('\0')) return false;

    for (char c : baName) {
        const quint8 nCharacter = (quint8)c;
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
    }

    QString sName = QString::fromLatin1(baName);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) ||
        QDir::isAbsolutePath(sName) ||
        ((sName.size() >= 2) && sName.at(0).isLetter() &&
         (sName.at(1) == QLatin1Char(':')))) {
        return false;
    }

    const QStringList listParts =
        sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    if (listParts.isEmpty()) return false;
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) ||
            (sPart == QLatin1String(".."))) {
            return false;
        }
    }

    sName = sName.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sName) != sName) return false;
    *pResult = sName;
    return true;
}

bool bigfAccountOutput(XBinary::UNPACK_STATE *pState, qint64 nSize)
{
    if (!pState || (nSize < 0)) return false;
    if (pState->spOutputBudget) {
        const XBinary::OUTPUT_BUDGET::REFUSAL refusalBefore =
            pState->spOutputBudget->refusal();
        const bool bBudgetAccepted = pState->spOutputBudget->debit(nSize);
        if (!bBudgetAccepted && pState->spOutputBudget->isEnforcing()) {
            return false;
        }
        if ((refusalBefore == XBinary::OUTPUT_BUDGET::REFUSAL_NONE) &&
            (pState->spOutputBudget->refusal() !=
             XBinary::OUTPUT_BUDGET::REFUSAL_NONE)) {
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }
    return true;
}

}  // namespace

struct XBIGF::LZW_CONTEXT
{
    QPointer<XBIGF> guardedArchive;
    QPointer<QIODevice> guardedSource;
    QPointer<QIODevice> guardedOutput;
    const BIGF_BLOCK *pBlock;
    QByteArray *pInputBuffer;
    QByteArray *pPhrase;
    QVector<quint32> *pPrefixes;
    QByteArray *pSuffixes;
    PDSTRUCT *pPdStruct;
    qint64 nInputLoaded;
    qint32 nInputPosition;
    qint32 nInputAvailable;
    quint64 nBitBuffer;
    qint32 nBitsAvailable;
    quint64 nBitsConsumed;
};

XBIGF::XBIGF(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XBIGF::entryOffsetLess(const BIGF_ENTRY &a, const BIGF_ENTRY &b)
{
    if (a.nDataOffset != b.nDataOffset) return a.nDataOffset < b.nDataOffset;
    return a.nStoredSize < b.nStoredSize;
}

bool XBIGF::resetLzwDictionary(LZW_CONTEXT *pContext)
{
    if (!pContext || !pContext->pPrefixes || !pContext->pSuffixes) return false;
    try {
        pContext->pPrefixes->resize(BIGF_LZW_FIRST_CODE);
        pContext->pSuffixes->resize(BIGF_LZW_FIRST_CODE);
    } catch (const std::bad_alloc &) {
        return false;
    }
    for (quint32 i = 0; i < 256; ++i) {
        (*pContext->pPrefixes)[(qint32)i] = BIGF_INVALID_PREFIX;
        (*pContext->pSuffixes)[(qint32)i] = (char)i;
    }
    (*pContext->pPrefixes)[(qint32)BIGF_LZW_CLEAR_CODE] = BIGF_INVALID_PREFIX;
    (*pContext->pSuffixes)[(qint32)BIGF_LZW_CLEAR_CODE] = 0;
    return true;
}

bool XBIGF::readLzwByte(LZW_CONTEXT *pContext, quint8 *pValue)
{
    if (!pContext || !pValue || !pContext->guardedArchive ||
        !pContext->guardedSource || !pContext->guardedOutput ||
        !pContext->pBlock || !pContext->pInputBuffer) return false;
    if (pContext->nInputPosition >= pContext->nInputAvailable) {
        if (pContext->nInputLoaded >= pContext->pBlock->nCompressedSize)
            return false;
        const qint64 nRequest = qMin<qint64>(
            pContext->pInputBuffer->size(),
            pContext->pBlock->nCompressedSize - pContext->nInputLoaded);
        const qint64 nRead = safeReadData(
            pContext->guardedSource.data(),
            pContext->pBlock->nDataOffset + pContext->nInputLoaded,
            pContext->pInputBuffer->data(), nRequest, pContext->pPdStruct);
        if (!pContext->guardedArchive || !pContext->guardedSource ||
            !pContext->guardedOutput || (nRead != nRequest)) return false;
        pContext->nInputLoaded += nRead;
        pContext->nInputPosition = 0;
        pContext->nInputAvailable = (qint32)nRead;
    }
    *pValue = (quint8)pContext->pInputBuffer->at(pContext->nInputPosition++);
    return true;
}

bool XBIGF::readLzwBits(LZW_CONTEXT *pContext, qint32 nWidth, quint32 *pCode)
{
    if (!pContext || !pCode || (nWidth <= 0) || (nWidth > 32)) return false;
    while (pContext->nBitsAvailable < nWidth) {
        quint8 nByte = 0;
        if (!readLzwByte(pContext, &nByte)) return false;
        pContext->nBitBuffer |= ((quint64)nByte << pContext->nBitsAvailable);
        pContext->nBitsAvailable += 8;
    }
    const quint64 nMask = (Q_UINT64_C(1) << nWidth) - 1;
    *pCode = (quint32)(pContext->nBitBuffer & nMask);
    pContext->nBitBuffer >>= nWidth;
    pContext->nBitsAvailable -= nWidth;
    pContext->nBitsConsumed += nWidth;
    return true;
}

bool XBIGF::alignLzwInput(LZW_CONTEXT *pContext)
{
    if (!pContext) return false;
    const qint32 nDiscard = (qint32)((8 - (pContext->nBitsConsumed & 7)) & 7);
    while (pContext->nBitsAvailable < nDiscard) {
        quint8 nByte = 0;
        if (!readLzwByte(pContext, &nByte)) return false;
        pContext->nBitBuffer |= ((quint64)nByte << pContext->nBitsAvailable);
        pContext->nBitsAvailable += 8;
    }
    pContext->nBitBuffer >>= nDiscard;
    pContext->nBitsAvailable -= nDiscard;
    pContext->nBitsConsumed += nDiscard;
    return true;
}

bool XBIGF::appendLzwDictionary(LZW_CONTEXT *pContext, quint32 nPrefix,
                                quint8 nSuffix)
{
    if (!pContext || !pContext->pPrefixes || !pContext->pSuffixes ||
        (pContext->pPrefixes->size() != pContext->pSuffixes->size()) ||
        (pContext->pPrefixes->size() >= (1 << BIGF_LZW_MAX_BITS)))
        return false;
    try {
        pContext->pPrefixes->append(nPrefix);
        pContext->pSuffixes->append((char)nSuffix);
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

bool XBIGF::decodeLzwPhrase(LZW_CONTEXT *pContext, quint32 nCode,
                            quint8 *pFirstCharacter)
{
    if (!pContext || !pFirstCharacter || !pContext->pPrefixes ||
        !pContext->pSuffixes || !pContext->pPhrase ||
        (nCode >= (quint32)pContext->pPrefixes->size())) return false;
    pContext->pPhrase->clear();
    quint32 nSteps = 0;
    while (nCode >= BIGF_LZW_FIRST_CODE) {
        if ((nCode >= (quint32)pContext->pPrefixes->size()) ||
            (++nSteps > (quint32)pContext->pPrefixes->size())) return false;
        pContext->pPhrase->append(pContext->pSuffixes->at((qint32)nCode));
        nCode = pContext->pPrefixes->at((qint32)nCode);
    }
    if (nCode >= BIGF_LZW_CLEAR_CODE) return false;
    pContext->pPhrase->append((char)nCode);
    *pFirstCharacter =
        (quint8)pContext->pPhrase->at(pContext->pPhrase->size() - 1);
    return true;
}

bool XBIGF::readZeroTerminatedName(qint64 nOffset, qint64 nLimit,
                                   QByteArray *pName,
                                   qint64 *pBytesConsumed,
                                   PDSTRUCT *pPdStruct)
{
    if (pName) pName->clear();
    if (pBytesConsumed) *pBytesConsumed = 0;
    if (!pName || !pBytesConsumed || (nOffset < 0) ||
        (nLimit < nOffset)) {
        return false;
    }

    QPointer<XBIGF> guardedThis(this);
    qint64 nCurrent = nOffset;
    while ((nCurrent < nLimit) &&
           (pName->size() <= BIGF_MAX_NAME_SIZE) &&
           isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRequest = qMin<qint64>(
            256, qMin<qint64>(nLimit - nCurrent,
                              BIGF_MAX_NAME_SIZE + 1 - pName->size()));
        if (nRequest <= 0) return false;
        const QByteArray baChunk =
            read_array_process(nCurrent, nRequest, pPdStruct);
        if (!guardedThis || (baChunk.size() != nRequest)) return false;
        const qint32 nTerminator = baChunk.indexOf('\0');
        if (nTerminator >= 0) {
            pName->append(baChunk.constData(), nTerminator);
            *pBytesConsumed = (nCurrent - nOffset) + nTerminator + 1;
            return !pName->isEmpty() &&
                   (pName->size() <= BIGF_MAX_NAME_SIZE);
        }
        pName->append(baChunk);
        nCurrent += baChunk.size();
    }
    return false;
}

bool XBIGF::scanArchive(BIGF_HEADER *pHeader,
                        QList<BIGF_ENTRY> *pEntries,
                        PDSTRUCT *pPdStruct)
{
    if (pHeader) *pHeader = BIGF_HEADER();
    if (pEntries) pEntries->clear();

    QPointer<XBIGF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    BIGF_DEVICE_POSITION_GUARD positionGuard(guardedDevice.data());
    if (!guardedThis || !guardedDevice || !positionGuard.isValid() ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nDeviceSize = getSize();
    if (!guardedThis || (nDeviceSize < BIGF_HEADER_SIZE)) return false;
    const QByteArray baHeader =
        read_array_process(0, BIGF_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedDevice ||
        (baHeader.size() != BIGF_HEADER_SIZE) ||
        (memcmp(baHeader.constData(), "BIGF", 4) != 0) ||
        (memcmp(baHeader.constData() + 5, "ZBL", 3) != 0)) {
        return false;
    }

    const uchar *pHeaderData =
        reinterpret_cast<const uchar *>(baHeader.constData());
    BIGF_HEADER header = {};
    header.nVersion = pHeaderData[4];
    if (!bigfToSignedOffset(bigfReadLE64(pHeaderData + 8),
                            &header.nArchiveSize) ||
        !bigfToSignedOffset(bigfReadLE64(pHeaderData + 20),
                            &header.nDirectoryOffset) ||
        !bigfToSignedOffset(bigfReadLE64(pHeaderData + 28),
                            &header.nDirectorySize) ||
        !bigfToSignedOffset(bigfReadLE64(pHeaderData + 36),
                            &header.nDataOffset)) {
        return false;
    }
    header.nNumberOfRecords = bigfReadLE32(pHeaderData + 16);

    if ((header.nArchiveSize < BIGF_HEADER_SIZE) ||
        (header.nArchiveSize > nDeviceSize) ||
        (header.nDataOffset < BIGF_HEADER_SIZE) ||
        (header.nDataOffset > header.nDirectoryOffset) ||
        (header.nDirectoryOffset > header.nArchiveSize) ||
        !bigfRangeWithin(header.nArchiveSize,
                         header.nDirectoryOffset,
                         header.nDirectorySize) ||
        (header.nDirectoryOffset + header.nDirectorySize !=
         header.nArchiveSize) ||
        (header.nNumberOfRecords == 0) ||
        (header.nNumberOfRecords > BIGF_MAX_RECORDS)) {
        return false;
    }

    QList<BIGF_ENTRY> listEntries;
    if (pEntries) listEntries.reserve((qint32)header.nNumberOfRecords);
    qint64 nDirectoryCursor = header.nDirectoryOffset;
    const qint64 nDirectoryEnd =
        header.nDirectoryOffset + header.nDirectorySize;

    for (quint32 i = 0; i < header.nNumberOfRecords; ++i) {
        if (!guardedThis || !guardedDevice ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const qint64 nRecordOffset = nDirectoryCursor;
        QByteArray baRecord;
        QByteArray baName;
        if (header.nVersion != 0) {
            if (!bigfRangeWithin(nDirectoryEnd, nDirectoryCursor, 2))
                return false;
            const QByteArray baLength =
                read_array_process(nDirectoryCursor, 2, pPdStruct);
            if (!guardedThis || (baLength.size() != 2)) return false;
            const quint16 nRecordSize = bigfReadLE16(
                reinterpret_cast<const uchar *>(baLength.constData()));
            if ((nRecordSize <= BIGF_DIRECTORY_RECORD_SIZE) ||
                !bigfRangeWithin(nDirectoryEnd,
                                 nDirectoryCursor + 2,
                                 nRecordSize)) {
                return false;
            }
            baRecord = read_array_process(nDirectoryCursor + 2,
                                          nRecordSize, pPdStruct);
            if (!guardedThis || (baRecord.size() != nRecordSize))
                return false;
            bigfDecodeDirectoryRecord(&baRecord);
            const qint32 nNameEnd =
                baRecord.indexOf('\0', BIGF_DIRECTORY_RECORD_SIZE);
            if (nNameEnd <= BIGF_DIRECTORY_RECORD_SIZE) return false;
            baName = baRecord.mid(BIGF_DIRECTORY_RECORD_SIZE,
                                  nNameEnd - BIGF_DIRECTORY_RECORD_SIZE);
            nDirectoryCursor += 2 + nRecordSize;
        } else {
            if (!bigfRangeWithin(nDirectoryEnd, nDirectoryCursor,
                                 BIGF_DIRECTORY_RECORD_SIZE)) {
                return false;
            }
            baRecord = read_array_process(nDirectoryCursor,
                                          BIGF_DIRECTORY_RECORD_SIZE,
                                          pPdStruct);
            if (!guardedThis ||
                (baRecord.size() != BIGF_DIRECTORY_RECORD_SIZE)) {
                return false;
            }
            qint64 nNameBytes = 0;
            if (!readZeroTerminatedName(
                    nDirectoryCursor + BIGF_DIRECTORY_RECORD_SIZE,
                    nDirectoryEnd, &baName, &nNameBytes, pPdStruct) ||
                !guardedThis) {
                return false;
            }
            nDirectoryCursor += BIGF_DIRECTORY_RECORD_SIZE + nNameBytes;
        }

        if (baRecord.size() < BIGF_DIRECTORY_RECORD_SIZE ||
            (bigfReadLE32(reinterpret_cast<const uchar *>(
                 baRecord.constData()) + 8) != 32)) {
            return false;
        }

        QString sName;
        if (!bigfNormalizeName(baName, &sName)) return false;
        const uchar *pRecordData =
            reinterpret_cast<const uchar *>(baRecord.constData());

        BIGF_ENTRY entry = {};
        entry.nHeaderOffset = nRecordOffset;
        entry.nHeaderSize = nDirectoryCursor - nRecordOffset;
        entry.nFileTime = bigfReadLE64(pRecordData + 12);
        if (!bigfToSignedOffset(bigfReadLE64(pRecordData),
                                &entry.nDataOffset) ||
            !bigfToSignedOffset(bigfReadLE64(pRecordData + 20),
                                &entry.nUncompressedSize)) {
            return false;
        }
        const quint32 nPackedSize = bigfReadLE32(pRecordData + 28);
        const quint32 nCompressionFlag = bigfReadLE32(pRecordData + 32);
        if ((nCompressionFlag > 1) ||
            (entry.nDataOffset < header.nDataOffset) ||
            (entry.nDataOffset > header.nDirectoryOffset)) {
            return false;
        }

        bool bCompressedSignature = false;
        QByteArray baMemberHeader;
        if (bigfRangeWithin(header.nDirectoryOffset,
                            entry.nDataOffset, 4)) {
            baMemberHeader = read_array_process(entry.nDataOffset, 4,
                                                pPdStruct);
            if (!guardedThis || (baMemberHeader.size() != 4))
                return false;
            bCompressedSignature =
                (baMemberHeader == QByteArrayLiteral("[..]"));
        }
        // Compression is a directory property.  Do not classify solely by
        // payload magic: valid stored BRU resources are known to begin with
        // the four literal bytes "[..]".  A compressed entry, however, must
        // carry both the flag and its stream header.
        if ((nCompressionFlag == 1) && !bCompressedSignature) return false;

        entry.bCompressed = (nCompressionFlag == 1);
        entry.sFileName = sName;

        if (entry.bCompressed) {
            if ((nPackedSize < 12) ||
                !bigfRangeWithin(header.nDirectoryOffset,
                                 entry.nDataOffset, nPackedSize)) {
                return false;
            }
            entry.nStoredSize = nPackedSize;
            const qint64 nMemberEnd =
                entry.nDataOffset + entry.nStoredSize;
            qint64 nBlockOffset = entry.nDataOffset;
            qint64 nTotalUncompressed = 0;
            while (nBlockOffset < nMemberEnd) {
                if ((entry.listBlocks.size() >=
                     (qint32)BIGF_MAX_RECORDS) ||
                    !bigfRangeWithin(nMemberEnd, nBlockOffset, 12)) {
                    return false;
                }
                baMemberHeader = read_array_process(nBlockOffset, 12,
                                                    pPdStruct);
                if (!guardedThis || (baMemberHeader.size() != 12) ||
                    (memcmp(baMemberHeader.constData(), "[..]", 4) != 0)) {
                    return false;
                }
                const uchar *pMemberHeader =
                    reinterpret_cast<const uchar *>(
                        baMemberHeader.constData());
                const quint32 nCompressedSize =
                    bigfReadLE32(pMemberHeader + 4);
                const quint32 nUncompressedSize =
                    bigfReadLE32(pMemberHeader + 8);
                if (!bigfRangeWithin(nMemberEnd, nBlockOffset + 12,
                                     nCompressedSize) ||
                    ((qint64)nUncompressedSize >
                     (entry.nUncompressedSize -
                      nTotalUncompressed))) {
                    return false;
                }

                BIGF_BLOCK block = {};
                block.nDataOffset = nBlockOffset + 12;
                block.nCompressedSize = nCompressedSize;
                block.nUncompressedSize = nUncompressedSize;
                entry.listBlocks.append(block);
                nBlockOffset += 12 + (qint64)nCompressedSize;
                nTotalUncompressed += nUncompressedSize;
            }
            if ((nBlockOffset != nMemberEnd) ||
                (nTotalUncompressed != entry.nUncompressedSize) ||
                entry.listBlocks.isEmpty()) {
                return false;
            }
        } else {
            entry.nStoredSize = entry.nUncompressedSize;
            if (!bigfRangeWithin(header.nDirectoryOffset,
                                 entry.nDataOffset,
                                 entry.nStoredSize) ||
                ((nPackedSize != 0) &&
                 ((qint64)nPackedSize != entry.nStoredSize))) {
                return false;
            }
        }

        if (!bigfRangeWithin(nDirectoryEnd, entry.nHeaderOffset,
                             entry.nHeaderSize)) {
            return false;
        }
        listEntries.append(entry);
    }

    if (nDirectoryCursor != nDirectoryEnd) return false;

    QList<BIGF_ENTRY> listByOffset = listEntries;
    std::sort(listByOffset.begin(), listByOffset.end(), entryOffsetLess);
    qint64 nPreviousEnd = header.nDataOffset;
    for (const BIGF_ENTRY &entry : listByOffset) {
        if ((entry.nStoredSize > 0) &&
            (entry.nDataOffset < nPreviousEnd)) {
            return false;
        }
        if (entry.nStoredSize > 0)
            nPreviousEnd = entry.nDataOffset + entry.nStoredSize;
    }

    if (!guardedThis || !guardedDevice ||
        !isPdStructNotCanceled(pPdStruct) || !positionGuard.restore()) {
        return false;
    }
    if (pHeader) *pHeader = header;
    if (pEntries) *pEntries = listEntries;
    return true;
}

bool XBIGF::isValid(PDSTRUCT *pPdStruct)
{
    return scanArchive(nullptr, nullptr, pPdStruct);
}

bool XBIGF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBIGF archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XBIGF::getFileType()
{
    return FT_BIGF;
}

XBinary::MODE XBIGF::getMode()
{
    return MODE_DATA;
}

qint32 XBIGF::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XBIGF::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBIGF::getArch()
{
    return QString();
}

QString XBIGF::getFileFormatExt()
{
    return QStringLiteral("cbf");
}

QString XBIGF::getFileFormatExtsString()
{
    return QStringLiteral("Ptero-Engine BIGF (*.cbf)");
}

QString XBIGF::getMIMEString()
{
    return QStringLiteral("application/x-ptero-cbf");
}

qint64 XBIGF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    BIGF_HEADER header = {};
    return scanArchive(&header, nullptr, pPdStruct)
        ? header.nArchiveSize : 0;
}

XBinary::OSNAME XBIGF::getOsName()
{
    return OSNAME_WINDOWS;
}

QString XBIGF::getVersion()
{
    BIGF_HEADER header = {};
    return scanArchive(&header, nullptr, nullptr)
        ? QString::number(header.nVersion) : QString();
}

QList<QString> XBIGF::getSearchSignatures()
{
    return {QStringLiteral("'BIGF'")};
}

XBinary *XBIGF::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBIGF(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant>
XBIGF::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBIGF::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    BIGF_UNPACK_CONTEXT *pOldContext =
        static_cast<BIGF_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    BIGF_HEADER header = {};
    QList<BIGF_ENTRY> listEntries;
    const bool bScanned = scanArchive(&header, &listEntries, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || listEntries.isEmpty() ||
        !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nDeviceSize = getSize();
    if (!guardedThis || (header.nArchiveSize > nDeviceSize)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    BIGF_UNPACK_CONTEXT *pContext =
        new (std::nothrow) BIGF_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->listEntries = listEntries;
    pContext->header = header;
    pContext->nDeviceSize = nDeviceSize;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.constFirst().nHeaderOffset;
    pState->nTotalSize = nDeviceSize;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XBIGF::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext)
        return ARCHIVERECORD();

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }

    BIGF_UNPACK_CONTEXT *pContext =
        static_cast<BIGF_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nTotalSize != pContext->nDeviceSize) ||
        (getSize() != pContext->nDeviceSize) || !guardedThis ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return ARCHIVERECORD();
    }

    const BIGF_ENTRY entry =
        pContext->listEntries.at(pState->nCurrentIndex);
    if (!bigfRangeWithin(pContext->header.nArchiveSize,
                         entry.nDataOffset, entry.nStoredSize) ||
        !bigfRangeWithin(pContext->header.nArchiveSize,
                         entry.nHeaderOffset, entry.nHeaderSize) ||
        entry.sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nStoredSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                entry.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                entry.nStoredSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
        entry.bCompressed ? QStringLiteral("Ptero LZW")
                          : ((pContext->header.nVersion == 0)
                                 ? QStringLiteral("Store")
                                 : QStringLiteral("Store (obfuscated)")));
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE,
                                entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dateTime =
        XBinary::winFileTimeToQDateTime(entry.nFileTime);
    if (dateTime.isValid())
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);

    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex))
        return ARCHIVERECORD();
    return result;
}

bool XBIGF::unpackStoredRecord(const BIGF_ENTRY &entry,
                               quint8 nVersion, QIODevice *pOutput,
                               UNPACK_STATE *pState,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pOutput);
    if (!guardedThis || !guardedSource || !guardedOutput ||
        (entry.nUncompressedSize < 0)) {
        return false;
    }

    QByteArray baBuffer(BIGF_IO_BUFFER_SIZE, '\0');
    if (baBuffer.size() != BIGF_IO_BUFFER_SIZE) return false;
    const bool bConvertIni = (nVersion != 0) &&
        entry.sFileName.startsWith(QStringLiteral("INI/DAT/"),
                                   Qt::CaseInsensitive);
    const quint8 nLengthByte = (quint8)entry.nUncompressedSize;
    const quint8 nDelta = (quint8)(90 - nLengthByte);
    quint8 nCrpKey = 234;
    qint64 nProcessed = 0;

    while ((nProcessed < entry.nUncompressedSize) && guardedThis &&
           guardedSource && guardedOutput &&
           isPdStructNotCanceled(pPdStruct)) {
        const qint64 nChunkSize = qMin<qint64>(
            baBuffer.size(), entry.nUncompressedSize - nProcessed);
        const qint64 nRead = safeReadData(
            guardedSource.data(), entry.nDataOffset + nProcessed,
            baBuffer.data(), nChunkSize, pPdStruct);
        if (!guardedThis || !guardedSource || !guardedOutput ||
            (nRead != nChunkSize)) {
            return false;
        }

        if (nVersion != 0) {
            for (qint64 i = 0; i < nChunkSize; ++i) {
                quint8 nValue = (quint8)baBuffer.at((qint32)i);
                nValue = (quint8)((nValue - nDelta) ^ nLengthByte);
                if (bConvertIni) {
                    nValue = (quint8)(nValue - nCrpKey);
                    const qint64 nAbsoluteIndex = nProcessed + i;
                    if ((nAbsoluteIndex % 3) == 0) {
                        nCrpKey = (quint8)(nCrpKey + 13);
                    } else if ((nAbsoluteIndex % 3) == 1) {
                        nCrpKey = (quint8)(
                            nCrpKey + nLengthByte);
                    } else {
                        nCrpKey = (quint8)(
                            nCrpKey - (quint8)nAbsoluteIndex);
                    }
                }
                baBuffer[(qint32)i] = (char)nValue;
            }
        }

        if (!bigfAccountOutput(pState, nChunkSize)) {
            XBinary::setPdStructErrorString(
                pPdStruct,
                QObject::tr("Unpacked output exceeds the configured limit"));
            return false;
        }
        const qint64 nWritten = safeWriteData(
            guardedOutput.data(), nProcessed, baBuffer.constData(),
            nChunkSize, pPdStruct);
        if (!guardedThis || !guardedSource || !guardedOutput ||
            (nWritten != nChunkSize)) {
            return false;
        }
        nProcessed += nChunkSize;
    }
    return guardedThis && guardedSource && guardedOutput &&
           isPdStructNotCanceled(pPdStruct) &&
           (nProcessed == entry.nUncompressedSize);
}

bool XBIGF::unpackLzwBlock(const BIGF_BLOCK &block,
                           qint64 nOutputBase, QIODevice *pOutput,
                           UNPACK_STATE *pState,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pOutput);
    if (!guardedThis || !guardedSource || !guardedOutput ||
        (nOutputBase < 0) || (block.nCompressedSize < 0) ||
        (block.nUncompressedSize < 0)) {
        return false;
    }
    if (block.nUncompressedSize == 0) return true;

    QByteArray baInputBuffer(BIGF_IO_BUFFER_SIZE, '\0');
    QByteArray baOutputBuffer(BIGF_IO_BUFFER_SIZE, '\0');
    QByteArray baPhrase;
    QVector<quint32> listPrefixes;
    QByteArray baSuffixes;
    if ((baInputBuffer.size() != BIGF_IO_BUFFER_SIZE) ||
        (baOutputBuffer.size() != BIGF_IO_BUFFER_SIZE)) {
        return false;
    }

    try {
        listPrefixes.resize(BIGF_LZW_FIRST_CODE);
        baSuffixes.resize(BIGF_LZW_FIRST_CODE);
    } catch (const std::bad_alloc &) {
        return false;
    }
    LZW_CONTEXT lzwContext = {guardedThis, guardedSource, guardedOutput,
                              &block, &baInputBuffer, &baPhrase,
                              &listPrefixes, &baSuffixes, pPdStruct,
                              0, 0, 0, 0, 0, 0};
    if (!resetLzwDictionary(&lzwContext)) return false;

    qint32 nCodeWidth = BIGF_LZW_INITIAL_BITS;
    quint32 nPreviousCode = 0;
    quint8 nPreviousFirstCharacter = 0;
    bool bHasPreviousCode = false;
    qint64 nOutputOffset = 0;

    while ((nOutputOffset < block.nUncompressedSize) && guardedThis &&
           guardedSource && guardedOutput &&
           isPdStructNotCanceled(pPdStruct)) {
        quint32 nCode = 0;
        if (!readLzwBits(&lzwContext, nCodeWidth, &nCode)) return false;
        if (nCode == BIGF_LZW_CLEAR_CODE) {
            if (!alignLzwInput(&lzwContext) ||
                !resetLzwDictionary(&lzwContext)) return false;
            nCodeWidth = BIGF_LZW_INITIAL_BITS;
            bHasPreviousCode = false;
            continue;
        }

        const quint32 nDictionarySize =
            (quint32)listPrefixes.size();
        quint8 nFirstCharacter = 0;
        bool bAddedEntry = false;
        if (nCode < nDictionarySize) {
            if (!decodeLzwPhrase(&lzwContext, nCode, &nFirstCharacter)) return false;
            if (bHasPreviousCode) {
                if (!appendLzwDictionary(&lzwContext, nPreviousCode,
                                         nFirstCharacter)) {
                    return false;
                }
                bAddedEntry = true;
            }
        } else if ((nCode == nDictionarySize) && bHasPreviousCode) {
            if (!appendLzwDictionary(&lzwContext, nPreviousCode,
                                     nPreviousFirstCharacter) ||
                !decodeLzwPhrase(&lzwContext, nCode, &nFirstCharacter)) {
                return false;
            }
            bAddedEntry = true;
        } else {
            return false;
        }

        if (bAddedEntry &&
            (((quint64)listPrefixes.size() + 1) >=
             (Q_UINT64_C(1) << nCodeWidth))) {
            ++nCodeWidth;
            if (nCodeWidth > BIGF_LZW_MAX_BITS) return false;
        }

        const qint64 nPhraseSize = baPhrase.size();
        if ((nPhraseSize <= 0) ||
            (nPhraseSize >
             (block.nUncompressedSize - nOutputOffset))) {
            return false;
        }

        qint64 nPhraseWritten = 0;
        while (nPhraseWritten < nPhraseSize) {
            const qint32 nChunkSize = (qint32)qMin<qint64>(
                baOutputBuffer.size(), nPhraseSize - nPhraseWritten);
            for (qint32 i = 0; i < nChunkSize; ++i) {
                baOutputBuffer[i] = baPhrase.at(
                    (qint32)(nPhraseSize - 1 -
                             (nPhraseWritten + i)));
            }
            if (!bigfAccountOutput(pState, nChunkSize)) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    QObject::tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            const qint64 nWritten = safeWriteData(
                guardedOutput.data(), nOutputBase + nOutputOffset,
                baOutputBuffer.constData(), nChunkSize, pPdStruct);
            if (!guardedThis || !guardedSource || !guardedOutput ||
                (nWritten != nChunkSize)) {
                return false;
            }
            nOutputOffset += nChunkSize;
            nPhraseWritten += nChunkSize;
        }

        nPreviousCode = nCode;
        nPreviousFirstCharacter = nFirstCharacter;
        bHasPreviousCode = true;
    }

    return guardedThis && guardedSource && guardedOutput &&
           isPdStructNotCanceled(pPdStruct) &&
           (nOutputOffset == block.nUncompressedSize);
}

bool XBIGF::unpackLzwRecord(const BIGF_ENTRY &entry,
                            QIODevice *pOutput,
                            UNPACK_STATE *pState,
                            PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pOutput);
    if (!guardedThis || !guardedOutput || !entry.bCompressed ||
        entry.listBlocks.isEmpty()) {
        return false;
    }

    qint64 nOutputBase = 0;
    for (const BIGF_BLOCK &block : entry.listBlocks) {
        if ((block.nUncompressedSize >
             (entry.nUncompressedSize - nOutputBase)) ||
            !unpackLzwBlock(block, nOutputBase, guardedOutput.data(),
                            pState, pPdStruct) ||
            !guardedThis || !guardedOutput) {
            return false;
        }
        nOutputBase += block.nUncompressedSize;
    }
    return guardedThis && guardedOutput &&
           isPdStructNotCanceled(pPdStruct) &&
           (nOutputBase == entry.nUncompressedSize);
}

bool XBIGF::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                          PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XBIGF> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    BIGF_DEVICE_POSITION_GUARD positionGuard(guardedSource.data());
    if (!operationGuard.isAcquired() || !pState || !guardedThis ||
        !guardedOutput || !guardedSource || !positionGuard.isValid() ||
        !isUnpackOutputSupported(guardedOutput.data()) ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    BIGF_UNPACK_CONTEXT *pContext =
        static_cast<BIGF_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex >= pContext->listEntries.count()) ||
        (pState->nTotalSize != pContext->nDeviceSize)) {
        return false;
    }
    const BIGF_ENTRY entry =
        pContext->listEntries.at(pState->nCurrentIndex);
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   entry.nUncompressedSize)) {
        setPdStructErrorString(
            pPdStruct,
            tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    QIODevice *pWorkDevice = createUnpackFileBuffer(
        entry.nUncompressedSize, pState->mapUnpackProperties, pPdStruct);
    if (!pWorkDevice) return false;
    QPointer<QIODevice> guardedWork(pWorkDevice);
    bool bResult = guardedWork &&
                   (guardedWork->size() == entry.nUncompressedSize) &&
                   guardedWork->seek(0);

    if (bResult && pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(
                pState->nCurrentIndex, entry.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                freeFileBuffer(&pWorkDevice);
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }
    if (bResult) {
        bResult = entry.bCompressed
            ? unpackLzwRecord(entry, guardedWork.data(), pState,
                              pPdStruct)
            : unpackStoredRecord(entry, pContext->header.nVersion,
                                 guardedWork.data(), pState, pPdStruct);
    }
    if (bResult) {
        bResult = guardedThis && guardedSource && guardedOutput &&
                  guardedWork && isPdStructNotCanceled(pPdStruct) &&
                  (guardedWork->size() == entry.nUncompressedSize) &&
                  isUnpackSourceCurrent(pState, pPdStruct) &&
                  guardedThis && guardedSource && guardedOutput &&
                  guardedWork;
    }
    if (bResult) {
        bResult = publishUnpackOutput(guardedWork.data(),
                                      guardedOutput.data(), pState,
                                      pPdStruct);
    }

    freeFileBuffer(&pWorkDevice);
    if (!guardedThis || !guardedSource || !guardedOutput) return false;
    if (bResult) pState->nCurrentOffset = entry.nUncompressedSize;
    return bResult && positionGuard.restore();
}

bool XBIGF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext)
        return false;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    BIGF_UNPACK_CONTEXT *pContext =
        static_cast<BIGF_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pState->nTotalSize != pContext->nDeviceSize)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listEntries.at(
            pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XBIGF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    BIGF_UNPACK_CONTEXT *pContext =
        static_cast<BIGF_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;
    return true;
}

QList<XBinary::FPART_PROP> XBIGF::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME,
            FPART_PROP_UNCOMPRESSEDSIZE,
            FPART_PROP_COMPRESSEDSIZE,
            FPART_PROP_REPORTEDMETHOD,
            FPART_PROP_MTIME,
            FPART_PROP_FILEMODE};
}

bool XBIGF::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    bool bResult = true;
    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pInfo;
    }
    return guardedThis && bResult;
}

void *XBIGF::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XBIGF> guardedThis(this);
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &m_internalInfo;
}

void XBIGF::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(
            static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
