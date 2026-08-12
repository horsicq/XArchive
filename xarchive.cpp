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
#include "xarchive.h"
#include "xdecompress.h"
#include "Algos/xppmddecoder.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <new>
#include <QSaveFile>
#include <QTemporaryFile>

namespace {
static qint64 archiveReadWithBoundedProgress(QIODevice *pDevice, char *pBuffer, qint64 nSize);

class ArchiveBoundedReadDevice : public QIODevice {
public:
    ArchiveBoundedReadDevice(QIODevice *pSource, qint64 nLimit)
        : m_pSource(pSource), m_nLimit(nLimit), m_nRead(0), m_bError(false)
    {
    }

    bool isSequential() const override { return true; }
    qint64 consumed() const { return m_nRead; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *pData, qint64 nMaximumSize) override
    {
        if (!m_pSource || (nMaximumSize < 0) || ((nMaximumSize > 0) && !pData) || (m_nRead < 0) || (m_nRead > m_nLimit)) {
            m_bError = true;
            return -1;
        }

        const qint64 nRemaining = m_nLimit - m_nRead;
        if ((nMaximumSize == 0) || (nRemaining == 0)) {
            return 0;
        }

        const qint64 nRequest = (std::min)(nMaximumSize, nRemaining);
        const qint64 nResult = archiveReadWithBoundedProgress(m_pSource, pData, nRequest);
        if ((nResult < 0) || (nResult > nRequest)) {
            m_bError = true;
            return -1;
        }
        if (nResult == 0) {
            m_bError = true;  // EOF before the declared compressed extent.
            return 0;
        }

        m_nRead += nResult;
        return nResult;
    }

    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QIODevice *m_pSource;
    qint64 m_nLimit;
    qint64 m_nRead;
    bool m_bError;
};

class ArchiveWindowWriteDevice : public QIODevice {
public:
    ArchiveWindowWriteDevice(QIODevice *pDestination, qint64 nOffset, qint64 nSize)
        : m_pDestination(pDestination), m_nOffset(nOffset), m_nSize(nSize), m_nProduced(0), m_nWritten(0), m_bError(false)
    {
    }

    bool isSequential() const override { return true; }
    qint64 produced() const { return m_nProduced; }
    qint64 written() const { return m_nWritten; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *, qint64) override { return -1; }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if (!m_pDestination || !pData || (nSize < 0) || (m_nProduced > (nMax - nSize))) {
            m_bError = true;
            return -1;
        }

        const qint64 nChunkStart = m_nProduced;
        const qint64 nChunkEnd = nChunkStart + nSize;
        const qint64 nWindowEnd = (m_nSize == -1) ? nMax : m_nOffset + m_nSize;
        const qint64 nWriteStart = (std::max)(nChunkStart, m_nOffset);
        const qint64 nWriteEnd = (std::min)(nChunkEnd, nWindowEnd);

        if (nWriteEnd > nWriteStart) {
            const qint64 nSkip = nWriteStart - nChunkStart;
            const qint64 nWriteSize = nWriteEnd - nWriteStart;
            qint64 nDone = 0;
            while (nDone < nWriteSize) {
                const qint64 nResult = m_pDestination->write(pData + nSkip + nDone, nWriteSize - nDone);
                if ((nResult <= 0) || (nResult > (nWriteSize - nDone))) {
                    m_bError = true;
                    return -1;
                }
                nDone += nResult;
            }
            m_nWritten += nDone;
        }

        m_nProduced = nChunkEnd;
        return nSize;
    }

private:
    QIODevice *m_pDestination;
    qint64 m_nOffset;
    qint64 m_nSize;
    qint64 m_nProduced;
    qint64 m_nWritten;
    bool m_bError;
};

static qint64 archiveReadWithBoundedProgress(QIODevice *pDevice, char *pBuffer, qint64 nSize)
{
    if (!pDevice || (nSize < 0) || ((nSize > 0) && !pBuffer)) return -1;

    for (qint32 i = 0; i < 3; i++) {
        const qint64 nRead = pDevice->read(pBuffer, nSize);
        if ((nRead != 0) || (nSize == 0) || pDevice->atEnd()) return nRead;
        if (i != 2) pDevice->waitForReadyRead(10);
    }

    return pDevice->atEnd() ? 0 : -1;
}

static bool archiveWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nSize < 0) || ((nSize > 0) && !pData) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    qint64 nWrittenTotal = 0;
    while ((nWrittenTotal < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nWritten = pDevice->write(pData + nWrittenTotal, nSize - nWrittenTotal);
        if ((nWritten <= 0) || (nWritten > (nSize - nWrittenTotal))) return false;
        nWrittenTotal += nWritten;
    }

    return (nWrittenTotal == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

static bool archiveGetSafeRelativePath(const QString &sPath, QString *pNormalizedPath)
{
    if (!pNormalizedPath) return false;

    QString sNormalized = QDir::fromNativeSeparators(sPath);
    if (sNormalized.isEmpty() || sNormalized.startsWith(QLatin1Char('/'))) return false;

    const QStringList listParts = sNormalized.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
    }

    sNormalized = sNormalized.normalized(QString::NormalizationForm_C);
    // fixFileName preserves safe path components and changes every portable
    // filesystem hazard: traversal components, control characters, NTFS ADS
    // colons, trailing dots/spaces, and Windows device aliases.
    if (XBinary::fixFileName(sNormalized) != sNormalized) return false;

    *pNormalizedPath = sNormalized;
    return true;
}

static bool archivePathHasUnsafeLink(const QString &sCanonicalRoot, const QString &sRelativePath)
{
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif

    QString sRoot = QDir::fromNativeSeparators(QDir::cleanPath(sCanonicalRoot));
    QString sRootPrefix = sRoot;
    if (!sRootPrefix.endsWith(QLatin1Char('/'))) sRootPrefix.append(QLatin1Char('/'));

    QString sCurrent = sRoot;
    const QStringList listParts = sRelativePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString &sPart : listParts) {
        sCurrent = QDir(sCurrent).filePath(sPart);
        QFileInfo fileInfo(sCurrent);

        // isSymLink() also detects a broken link, for which exists() is false.
        if (fileInfo.isSymLink()) return true;

        if (fileInfo.exists()) {
            const QString sCanonical = QDir::fromNativeSeparators(fileInfo.canonicalFilePath());
            if (sCanonical.isEmpty() ||
                ((sCanonical.compare(sRoot, pathCaseSensitivity) != 0) &&
                 !sCanonical.startsWith(sRootPrefix, pathCaseSensitivity))) {
                return true;
            }
        }
    }

    return false;
}
}  // namespace

#if defined(_MSC_VER)
#if _MSC_VER > 1800                                   // TODO Check !!!
#pragma comment(lib, "legacy_stdio_definitions.lib")  // bzip2.lib(compress.obj) __imp__fprintf

FILE _iob[] = {*stdin, *stdout, *stderr};  // bzip2.lib(compress.obj) _iob_func

extern "C" FILE *__cdecl __iob_func(void)
{
    return _iob;
}
#endif
#endif

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

XArchive::XArchive(QIODevice *pDevice) : XBinary(pDevice)
{
}

quint64 XArchive::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return getNumberOfArchiveRecords(pPdStruct);
}

QList<XArchive::RECORD> XArchive::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;

    // -1 is the only unbounded sentinel.  Avoid initializing an archive when
    // the requested result is necessarily empty or the limit is invalid.
    if (nLimit < -1 || nLimit == 0) {
        return listResult;
    }

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    QMap<UNPACK_PROP, QVariant> mapProperties;

    // Initialize unpacking state
    UNPACK_STATE state = {};

    if (!initUnpack(&state, mapProperties, pPdStruct)) {
        return listResult;
    }

    // Iterate through records using streaming API.  A successful initializer
    // must expose a sane cursor; otherwise calling infoCurrent() with a
    // negative index is undefined for most format implementations.
    qint32 nIndex = 0;
    bool bEnumerationValid = (state.nCurrentIndex >= 0) && (state.nNumberOfRecords >= 0) &&
                             (state.nCurrentIndex <= state.nNumberOfRecords);

    while (bEnumerationValid && (state.nCurrentIndex < state.nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        // Get current record info
        ARCHIVERECORD archiveRecord = infoCurrent(&state, pPdStruct);

        // A callback or parser is allowed to cancel during infoCurrent().  Do
        // not publish the record that was being assembled when that happened.
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (state.nCurrentIndex < 0) || (state.nNumberOfRecords < 0)) {
            if ((state.nCurrentIndex < 0) || (state.nNumberOfRecords < 0)) bEnumerationValid = false;
            break;
        }

        // Convert ARCHIVERECORD to legacy RECORD structure
        RECORD record = {};

        record.nDataOffset = archiveRecord.nStreamOffset;
        record.nDataSize = archiveRecord.nStreamSize;
        record.mapProperties = archiveRecord.mapProperties;
        record.spInfo.nUncompressedSize = archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

        // Extract common properties from mapProperties
        if (archiveRecord.mapProperties.contains(FPART_PROP_ORIGINALNAME)) {
            record.spInfo.sRecordName = archiveRecord.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_HANDLEMETHOD)) {
            record.spInfo.compressMethod = (HANDLE_METHOD)archiveRecord.mapProperties.value(FPART_PROP_HANDLEMETHOD).toInt();
        } else {
            record.spInfo.compressMethod = HANDLE_METHOD_UNKNOWN;
        }
        record.spInfo.compressMethod2 = HANDLE_METHOD_UNKNOWN;

        if (archiveRecord.mapProperties.contains(FPART_PROP_RESULTCRC)) {
            record.spInfo.nCRC32 = archiveRecord.mapProperties.value(FPART_PROP_RESULTCRC).toUInt();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_WINDOWSIZE)) {
            record.spInfo.nWindowSize = archiveRecord.mapProperties.value(FPART_PROP_WINDOWSIZE).toULongLong();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_ISSOLID)) {
            record.spInfo.bIsSolid = archiveRecord.mapProperties.value(FPART_PROP_ISSOLID).toBool();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_HEADER_OFFSET)) {
            record.nHeaderOffset = archiveRecord.mapProperties.value(FPART_PROP_HEADER_OFFSET).toLongLong();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_HEADER_SIZE)) {
            record.nHeaderSize = archiveRecord.mapProperties.value(FPART_PROP_HEADER_SIZE).toLongLong();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_OPTHEADER_OFFSET)) {
            record.nOptHeaderOffset = archiveRecord.mapProperties.value(FPART_PROP_OPTHEADER_OFFSET).toLongLong();
        }

        if (archiveRecord.mapProperties.contains(FPART_PROP_OPTHEADER_SIZE)) {
            record.nOptHeaderSize = archiveRecord.mapProperties.value(FPART_PROP_OPTHEADER_SIZE).toLongLong();
        }

        record.sUUID = generateUUID();
        if (archiveRecord.mapProperties.contains(FPART_PROP_HANDLEMETHOD2)) {
            record.spInfo.compressMethod2 = (HANDLE_METHOD)archiveRecord.mapProperties.value(FPART_PROP_HANDLEMETHOD2).toInt();
        }

        listResult.append(record);

        nIndex++;

        // Check limit
        if ((nLimit != -1) && (nIndex >= nLimit)) {
            break;
        }

        // Move to next record
        const qint32 nPreviousIndex = state.nCurrentIndex;
        const bool bMoved = moveToNext(&state, pPdStruct);
        if ((state.nCurrentIndex < 0) || (state.nNumberOfRecords < 0)) {
            bEnumerationValid = false;
            break;
        }
        if (!bMoved) {
            // Returning false is normal after the last declared record.  It is
            // corruption when records are still outstanding, and a partial
            // prefix must not be published as a complete enumeration.
            if ((nPreviousIndex + 1) < state.nNumberOfRecords) {
                bEnumerationValid = false;
            }
            break;
        }
        if (state.nCurrentIndex <= nPreviousIndex) {
            bEnumerationValid = false;
            break;
        }
    }

    // Clean up unpacking state
    // Cleanup must not inherit a canceled enumeration token.
    const bool bFinished = finishUnpack(&state, nullptr);
    if (!bEnumerationValid || !bFinished || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
    }

    return listResult;
}

XArchive::COMPRESS_RESULT XArchive::_decompress(DECOMPRESSSTRUCT *pDecompressStruct, PDSTRUCT *pPdStruct)
{
    if (!pDecompressStruct) {
        return COMPRESS_RESULT_DATAERROR;
    }

    // Result fields belong to this invocation even when argument validation
    // fails; never expose counters or limit state left by an earlier call.
    pDecompressStruct->nOutSize = 0;
    pDecompressStruct->nDecompressedWrote = 0;
    pDecompressStruct->bLimit = false;

    if (!pDecompressStruct->pSourceDevice || !pDecompressStruct->pDestDevice || (pDecompressStruct->nInSize < 0) ||
        (pDecompressStruct->nDecompressedOffset < 0) || (pDecompressStruct->nDecompressedLimit < -1) ||
        ((pDecompressStruct->nDecompressedLimit != -1) &&
         (pDecompressStruct->nDecompressedOffset > ((std::numeric_limits<qint64>::max)() - pDecompressStruct->nDecompressedLimit)))) {
        return COMPRESS_RESULT_DATAERROR;
    }

    const qint64 nWindowEnd = (pDecompressStruct->nDecompressedLimit == -1)
                                  ? (std::numeric_limits<qint64>::max)()
                                  : pDecompressStruct->nDecompressedOffset + pDecompressStruct->nDecompressedLimit;

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_STORE) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XStoreDecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_PPMD8) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PPMD8);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XPPMdDecoder::decompressPPMD8(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_DEFLATE) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XDeflateDecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_BZIP2) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_BZIP2);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XBZIP2Decoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZMA) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZMA);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZMADecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if ((pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZH5) || (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZH6) ||
               (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZH7)) {
        qint32 nMethod = 5;

        if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZH5) {
            nMethod = 5;
        } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZH6) {
            nMethod = 6;
        } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZH7) {
            nMethod = 7;
        }

        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, pDecompressStruct->spInfo.compressMethod);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZHDecoder::decompress(&decompressState, nMethod, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if ((pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_15) || (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_20) ||
               (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_29) || (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_50) ||
               (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_70)) {
        if (pDecompressStruct->spInfo.nUncompressedSize < 0) {
            return COMPRESS_RESULT_DATAERROR;
        }
        bool bIsSolid = false;
        qint64 nRarInputSize = pDecompressStruct->nInSize;
        const qint64 nSourceSize = pDecompressStruct->pSourceDevice->size();
        if (nRarInputSize == 0) {
            if (nSourceSize < 0) {
                return COMPRESS_RESULT_DATAERROR;
            }
            nRarInputSize = nSourceSize;
        } else if (!pDecompressStruct->pSourceDevice->isSequential() && (nSourceSize >= 0) && (nRarInputSize > nSourceSize)) {
            return COMPRESS_RESULT_READERROR;
        }

        if (!pDecompressStruct->pSourceDevice->seek(0) && (pDecompressStruct->pSourceDevice->pos() != 0)) {
            return COMPRESS_RESULT_READERROR;
        }

        ArchiveBoundedReadDevice inputDevice(pDecompressStruct->pSourceDevice, nRarInputSize);
        if (!inputDevice.open(QIODevice::ReadOnly)) {
            return COMPRESS_RESULT_READERROR;
        }

        ArchiveWindowWriteDevice windowDevice(pDecompressStruct->pDestDevice, pDecompressStruct->nDecompressedOffset,
                                               pDecompressStruct->nDecompressedLimit);
        if (!windowDevice.open(QIODevice::WriteOnly)) {
            inputDevice.close();
            return COMPRESS_RESULT_WRITEERROR;
        }

        std::unique_ptr<rar_Unpack> pRarUnpack(new (std::nothrow) rar_Unpack());
        if (!pRarUnpack) {
            windowDevice.close();
            inputDevice.close();
            return COMPRESS_RESULT_MEMORYERROR;
        }

        pRarUnpack->setDevices(&inputDevice, &windowDevice);
        qint32 nInit = pRarUnpack->Init(pDecompressStruct->spInfo.nWindowSize, bIsSolid);

        if (nInit > 0) {
            pRarUnpack->SetDestSize(pDecompressStruct->spInfo.nUncompressedSize);

            if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_15) {
                pRarUnpack->Unpack15(bIsSolid, pPdStruct);
            } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_20) {
                pRarUnpack->Unpack20(bIsSolid, pPdStruct);
            } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_29) {
                pRarUnpack->Unpack29(bIsSolid, pPdStruct);
            } else if ((pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_50) || (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_RAR_70)) {
                pRarUnpack->Unpack5(bIsSolid, pPdStruct);
            }

            if (windowDevice.hasError()) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else if (inputDevice.hasError()) {
                result = COMPRESS_RESULT_READERROR;
            } else {
                result = (pRarUnpack->IsFileExtracted() &&
                          (windowDevice.produced() == pDecompressStruct->spInfo.nUncompressedSize) &&
                          XBinary::isPdStructNotCanceled(pPdStruct))
                             ? COMPRESS_RESULT_OK
                             : COMPRESS_RESULT_DATAERROR;
            }
        } else {
            result = COMPRESS_RESULT_MEMORYERROR;
        }

        pDecompressStruct->nOutSize = windowDevice.produced();
        pDecompressStruct->nDecompressedWrote = windowDevice.written();
        pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (windowDevice.produced() >= nWindowEnd);
        windowDevice.close();
        inputDevice.close();
        if (result == COMPRESS_RESULT_OK) {
            pDecompressStruct->nInSize = inputDevice.consumed();
        }
    } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_LZSS_SZDD) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZSS_SZDD);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZSSDecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == HANDLE_METHOD_XZ) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XZ);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nProcessedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nProcessedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZMADecoder::decompressXZ(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit != -1) && (decompressState.nCountOutput >= nWindowEnd);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    }

    if (result == COMPRESS_RESULT_OK) {
        const qint64 nWriteEnd = (std::min)(pDecompressStruct->nOutSize, nWindowEnd);
        pDecompressStruct->nDecompressedWrote =
            (nWriteEnd > pDecompressStruct->nDecompressedOffset) ? (nWriteEnd - pDecompressStruct->nDecompressedOffset) : 0;
    }

    return result;
}

bool XArchive::_decompressRecord(const RECORD *pRecord, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset = 0,
                                 qint64 nDecompressedLimit = -1)
{
    bool bResult = false;

    if (!pRecord || !pSourceDevice || !pDestDevice || (pRecord->nDataOffset < 0) || (pRecord->nDataSize < 0) || (nDecompressedOffset < 0) ||
        (nDecompressedLimit < -1) || ((nDecompressedLimit != -1) &&
                                      (nDecompressedOffset > ((std::numeric_limits<qint64>::max)() - nDecompressedLimit)))) {
        return false;
    }

    const qint64 nSourceSize = pSourceDevice->size();
    if ((nSourceSize < 0) || (pRecord->nDataOffset > nSourceSize) || (pRecord->nDataSize > (nSourceSize - pRecord->nDataOffset))) {
        return false;
    }

    SubDevice sd(pSourceDevice, pRecord->nDataOffset, pRecord->nDataSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties = pRecord->mapProperties;
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, pRecord->spInfo.compressMethod);
        if (pRecord->spInfo.compressMethod2 != HANDLE_METHOD_UNKNOWN) {
            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD2, pRecord->spInfo.compressMethod2);
        }
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pRecord->spInfo.nUncompressedSize);
        state.mapProperties.insert(XBinary::FPART_PROP_WINDOWSIZE, pRecord->spInfo.nWindowSize);
        if (pRecord->spInfo.bIsSolid) {
            state.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if ((pRecord->spInfo.compressMethod == HANDLE_METHOD_STORE_CAB) || (pRecord->spInfo.compressMethod == HANDLE_METHOD_MSZIP_CAB) ||
            (pRecord->spInfo.compressMethod == HANDLE_METHOD_LZX_CAB)) {
            state.mapProperties.insert(XBinary::FPART_PROP_SUBSTREAMOFFSET, pRecord->nOptHeaderOffset);
            state.mapProperties.insert(XBinary::FPART_PROP_OPTHEADER_SIZE, pRecord->nOptHeaderSize);
        }

        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDestDevice;
        state.nInputOffset = 0;
        state.nInputLimit = pRecord->nDataSize;
        state.nProcessedOffset = nDecompressedOffset;
        state.nProcessedLimit = nDecompressedLimit;

        XDecompress decompressor;
        bResult = decompressor.multiDecompress(&state, pPdStruct);

        sd.close();
    }

        return bResult;
}

XArchive::COMPRESS_RESULT XArchive::_compress(XArchive::HANDLE_METHOD compressMethod, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    if (!pSourceDevice) return COMPRESS_RESULT_READERROR;
    if (!pDestDevice) return COMPRESS_RESULT_WRITEERROR;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return COMPRESS_RESULT_UNKNOWN;

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    if (compressMethod == HANDLE_METHOD_STORE) {
        const qint32 CHUNK = COMPRESS_BUFFERSIZE;
        char *pBuffer = new (std::nothrow) char[CHUNK];
        if (!pBuffer) return COMPRESS_RESULT_MEMORYERROR;

        result = COMPRESS_RESULT_OK;

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint64 nRead = archiveReadWithBoundedProgress(pSourceDevice, pBuffer, CHUNK);
            if ((nRead < 0) || (nRead > CHUNK)) {
                result = COMPRESS_RESULT_READERROR;
                break;
            }
            if (nRead == 0) {
                if (!pSourceDevice->atEnd()) result = COMPRESS_RESULT_READERROR;
                break;
            }
            if (!archiveWriteAll(pDestDevice, pBuffer, nRead, pPdStruct)) {
                result = COMPRESS_RESULT_WRITEERROR;
                break;
            }
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct) && (result == COMPRESS_RESULT_OK)) result = COMPRESS_RESULT_UNKNOWN;
        delete[] pBuffer;
    } else if (compressMethod == HANDLE_METHOD_DEFLATE) {
        result = _compress_deflate(pSourceDevice, pDestDevice, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY,
                                   pPdStruct);  // -MAX_WBITS for raw data
    }

    return result;
}

XArchive::COMPRESS_RESULT XArchive::_compress_deflate(QIODevice *pSourceDevice, QIODevice *pDestDevice, qint32 nLevel, qint32 nMethod, qint32 nWindowsBits,
                                                      qint32 nMemLevel, qint32 nStrategy, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    if (!pSourceDevice) return COMPRESS_RESULT_READERROR;
    if (!pDestDevice) return COMPRESS_RESULT_WRITEERROR;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return COMPRESS_RESULT_UNKNOWN;

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    const qint32 CHUNK = COMPRESS_BUFFERSIZE;
    unsigned char *pIn = new (std::nothrow) unsigned char[CHUNK];
    if (!pIn) {
        return COMPRESS_RESULT_MEMORYERROR;
    }
    unsigned char *pOut = new (std::nothrow) unsigned char[CHUNK];
    if (!pOut) {
        delete[] pIn;
        return COMPRESS_RESULT_MEMORYERROR;
    }

    z_stream strm = {};

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;

    qint32 ret = deflateInit2(&strm, nLevel, nMethod, nWindowsBits, nMemLevel, nStrategy);
    bool bReadError = false;
    bool bWriteError = false;

    if (ret == Z_OK) {
        bool bInputFinished = false;
        do {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;

            const qint64 nRead = archiveReadWithBoundedProgress(pSourceDevice, reinterpret_cast<char *>(pIn), CHUNK);
            if ((nRead < 0) || (nRead > CHUNK)) {
                bReadError = true;
                break;
            }
            if (nRead == 0) {
                if (!pSourceDevice->atEnd()) {
                    bReadError = true;
                    break;
                }
                bInputFinished = true;
            }

            strm.avail_in = static_cast<uInt>(nRead);
            strm.next_in = pIn;
            const qint32 nFlush = bInputFinished ? Z_FINISH : Z_NO_FLUSH;

            do {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
                strm.avail_out = CHUNK;
                strm.next_out = pOut;
                ret = deflate(&strm, nFlush);

                if ((ret != Z_OK) && (ret != Z_STREAM_END)) break;

                const qint32 nProduced = CHUNK - static_cast<qint32>(strm.avail_out);
                if ((nProduced > 0) &&
                    !archiveWriteAll(pDestDevice, reinterpret_cast<const char *>(pOut), nProduced, pPdStruct)) {
                    bWriteError = true;
                    break;
                }
            } while ((strm.avail_out == 0) && (ret != Z_STREAM_END));

            if (bWriteError || ((ret != Z_OK) && (ret != Z_STREAM_END))) break;
        } while (ret != Z_STREAM_END);

        deflateEnd(&strm);
    }

    if (bReadError) {
        result = COMPRESS_RESULT_READERROR;
    } else if (bWriteError) {
        result = COMPRESS_RESULT_WRITEERROR;
    } else if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        result = COMPRESS_RESULT_UNKNOWN;
    } else if (ret == Z_STREAM_END) {
        result = COMPRESS_RESULT_OK;
    } else if (ret == Z_BUF_ERROR) {
        result = COMPRESS_RESULT_BUFFERERROR;
    } else if (ret == Z_MEM_ERROR) {
        result = COMPRESS_RESULT_MEMORYERROR;
    } else if (ret == Z_DATA_ERROR) {
        result = COMPRESS_RESULT_DATAERROR;
    } else {
        result = COMPRESS_RESULT_UNKNOWN;
    }

    delete[] pIn;
    delete[] pOut;

    return result;
}

QByteArray XArchive::decompress(const XArchive::RECORD *pRecord, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset, qint64 nDecompressedLimit)
{
    QByteArray result;

    QBuffer buffer;
    buffer.setBuffer(&result);

    if (buffer.open(QIODevice::WriteOnly)) {
        const bool bDecompressed = _decompressRecord(pRecord, getDevice(), &buffer, pPdStruct, nDecompressedOffset, nDecompressedLimit);
        buffer.close();

        // A QByteArray return value must never expose a valid-looking prefix
        // from a failed or canceled decode.
        if (!bDecompressed || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            result.clear();
        }
    }

    return result;
}

QByteArray XArchive::decompress(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, pListArchive, pPdStruct);

    if (!record.spInfo.sRecordName.isEmpty()) {
        // Empty members still have a codec/checksum contract that must be
        // validated; a declared zero size is not permission to skip decoding.
        baResult = decompress(&record, pPdStruct);
    }

    return baResult;
}

QByteArray XArchive::decompress(const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listArchive = getRecords(-1, pPdStruct);

    return decompress(&listArchive, sRecordFileName, pPdStruct);
}

bool XArchive::decompressToFile(const XArchive::RECORD *pRecord, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    if (!pRecord || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (pRecord->mapProperties.value(XBinary::FPART_PROP_ISFOLDER, false).toBool()) {
        return XBinary::createDirectory(sResultFileName) && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    QIODevice *pSourceDevice = getDevice();
    if (!pSourceDevice || !pSourceDevice->isReadable()) {
        return false;
    }

    QFileInfo fi(sResultFileName);

    if (!XBinary::createDirectory(fi.absolutePath())) {
        return false;
    }

    // QSaveFile is deliberately write-only, while the shared decompressor
    // rereads complete output for CRC/authentication.  Decode into a private
    // readable temporary first, then copy the authenticated result into the
    // atomic replacement file.
    QTemporaryFile workFile;
    if (!workFile.open()) {
        return false;
    }

    // A zero packed size is not proof of a valid empty member.  The selected
    // codec must still validate its terminator, declared output size, password,
    // and checksum contract.
    const bool bResult = _decompressRecord(pRecord, pSourceDevice, &workFile, pPdStruct, 0, -1);

    if (!bResult || !XBinary::isPdStructNotCanceled(pPdStruct) || !workFile.seek(0)) {
        return false;
    }

    QSaveFile file(sResultFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QByteArray baBuffer(0x4000, 0);
    qint64 nRemaining = workFile.size();
    bool bCopyResult = nRemaining >= 0;
    while (bCopyResult && (nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRead = archiveReadWithBoundedProgress(&workFile, baBuffer.data(), (std::min)(nRemaining, (qint64)baBuffer.size()));
        if ((nRead <= 0) || (nRead > nRemaining) || !archiveWriteAll(&file, baBuffer.constData(), nRead, pPdStruct)) {
            bCopyResult = false;
            break;
        }
        nRemaining -= nRead;
    }

    if (!bCopyResult || (nRemaining != 0) || !XBinary::isPdStructNotCanceled(pPdStruct) || (file.error() != QFile::NoError)) {
        file.cancelWriting();
        return false;
    }

    return file.commit();
}

bool XArchive::decompressToDevice(const RECORD *pRecord, QIODevice *pDestDevice, PDSTRUCT *pPdStruct)
{
    return _decompressRecord(pRecord, getDevice(), pDestDevice, pPdStruct, 0, -1);
}

bool XArchive::decompressToFile(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pListArchive) return false;

    XArchive::RECORD record = getArchiveRecord(sRecordFileName, pListArchive);

    if (record.spInfo.sRecordName != "")  // TODO bIsValid
    {
        bResult = decompressToFile(&record, sResultFileName, pPdStruct);
    }

    return bResult;
}

bool XArchive::decompressToPath(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    if (!pListArchive || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    bool bResult = true;
    bool bMatched = false;

    const QString sRawRecordFileName = QDir::fromNativeSeparators(sRecordFileName);
    const bool bSelectorWasProvided = !sRawRecordFileName.isEmpty();
    // Archive member names are exact identifiers.  Trimming here could turn a
    // request for " name" into a request for the distinct member "name".
    QString sNormalizedRecordFileName = sRawRecordFileName;
    while (sNormalizedRecordFileName.endsWith(QLatin1Char('/'))) {
        sNormalizedRecordFileName.chop(1);
    }

    if (bSelectorWasProvided) {
        QString sSafeSelector;
        // Do not clean traversal components here: "safe/.." must be rejected,
        // not normalized to "." and accidentally interpreted as extract-all.
        if (sNormalizedRecordFileName.isEmpty() ||
            !archiveGetSafeRelativePath(sNormalizedRecordFileName, &sSafeSelector)) {
            return false;
        }
        sNormalizedRecordFileName = sSafeSelector;
    }

    QString sCanonicalRoot = _normalizeOutputPath(QDir(sResultPathName).absolutePath());
    if (!XBinary::createDirectory(sCanonicalRoot)) return false;
    sCanonicalRoot = QDir::fromNativeSeparators(QFileInfo(sCanonicalRoot).canonicalFilePath());
    if (sCanonicalRoot.isEmpty() || !QFileInfo(sCanonicalRoot).isDir()) return false;

    qint32 nNumberOfArchives = pListArchive->count();
    bool bExtractAll = sNormalizedRecordFileName.isEmpty();

    for (qint32 i = 0; (i < nNumberOfArchives) && isPdStructNotCanceled(pPdStruct); i++) {
        XArchive::RECORD record = pListArchive->at(i);
        QString sRecordFileNameInArchive = QDir::fromNativeSeparators(record.spInfo.sRecordName);
        while (sRecordFileNameInArchive.endsWith(QLatin1Char('/'))) {
            sRecordFileNameInArchive.chop(1);
        }

        QString sSafeRecordPath;
        if (!archiveGetSafeRelativePath(sRecordFileNameInArchive, &sSafeRecordPath)) {
            if (bExtractAll) bResult = false;
            continue;
        }

        const QString sRecordFileNameForMatch = sSafeRecordPath;

        bool bNamePresent = false;
        if (bExtractAll) {
            bNamePresent = true;
        } else if (sRecordFileNameForMatch == sNormalizedRecordFileName) {
            bNamePresent = true;
        } else if (sRecordFileNameForMatch.startsWith(sNormalizedRecordFileName + QLatin1Char('/'))) {
            bNamePresent = true;
        }

        if (bNamePresent || bExtractAll) {
            bMatched = true;
            QString sFileName = sSafeRecordPath;

            if (!bExtractAll && (sFileName != sNormalizedRecordFileName)) {
                const QString sPrefix = sNormalizedRecordFileName + QLatin1Char('/');
                if (sFileName.startsWith(sPrefix)) {
                    sFileName = sFileName.mid(sPrefix.size(), -1);
                }
            }

            if (sFileName.isEmpty()) {
                continue;
            }

            QString sSafeOutputPath;
            if (!archiveGetSafeRelativePath(sFileName, &sSafeOutputPath)) {
                bResult = false;
                continue;
            }

            const QString sResultFileName = _normalizeOutputPath(QDir(sCanonicalRoot).absoluteFilePath(sSafeOutputPath));

            if (!XArchive::_isSafeChildPath(sResultFileName, sCanonicalRoot) ||
                archivePathHasUnsafeLink(sCanonicalRoot, sSafeOutputPath)) {
                bResult = false;
                continue;
            }

            const bool bIsFolder = record.mapProperties.value(XBinary::FPART_PROP_ISFOLDER, false).toBool();
            const QString sDirectoryName = bIsFolder ? sResultFileName : QFileInfo(sResultFileName).absolutePath();
            if (!XBinary::createDirectory(sDirectoryName) ||
                archivePathHasUnsafeLink(sCanonicalRoot, sSafeOutputPath)) {
                bResult = false;
                continue;
            }

            if (!bIsFolder && !decompressToFile(&record, sResultFileName, pPdStruct)) {
                bResult = false;
            }
        }
    }
    // TODO emits

    return bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (bExtractAll || bMatched);
}

bool XArchive::decompressToFile(const QString &sArchiveFileName, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sArchiveFileName);

    if (file.open(QIODevice::ReadOnly)) {
        QIODevice *pOriginalDevice = getDevice();
        setDevice(&file);

        if (isValid(pPdStruct)) {
            QList<RECORD> listRecords = getRecords(-1, pPdStruct);

            bResult = decompressToFile(&listRecords, sRecordFileName, sResultFileName, pPdStruct);
        }

        // Never leave the archive pointing at this stack-local QFile.
        setDevice(pOriginalDevice);
        file.close();
    }

    return bResult;
}

bool XArchive::unpackToFolder(const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    PDSTRUCT pdStructEmpty = {};
    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    QString sCanonicalRoot = _normalizeOutputPath(QDir(sResultPathName).absolutePath());
    if (!XBinary::createDirectory(sCanonicalRoot)) return false;
    sCanonicalRoot = QDir::fromNativeSeparators(QFileInfo(sCanonicalRoot).canonicalFilePath());
    if (sCanonicalRoot.isEmpty() || !QFileInfo(sCanonicalRoot).isDir()) return false;

    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> mapProperties;
    if (!initUnpack(&state, mapProperties, pPdStruct)) return false;

    bool bResult = state.nNumberOfRecords > 0;
    bool bEnumerationValid = true;

    while (bEnumerationValid && (state.nCurrentIndex < state.nNumberOfRecords) && isPdStructNotCanceled(pPdStruct)) {
        const ARCHIVERECORD archiveRecord = infoCurrent(&state, pPdStruct);
        QString sRecordName = QDir::fromNativeSeparators(archiveRecord.mapProperties.value(FPART_PROP_ORIGINALNAME).toString());
        QString sSafeRecordPath;

        if (!archiveGetSafeRelativePath(sRecordName, &sSafeRecordPath)) {
            bResult = false;
        } else {
            const QString sResultFileName = _normalizeOutputPath(QDir(sCanonicalRoot).absoluteFilePath(sSafeRecordPath));
            const bool bIsFolder = archiveRecord.mapProperties.value(FPART_PROP_ISFOLDER, false).toBool() || sRecordName.endsWith(QLatin1Char('/'));

            if (!_isSafeChildPath(sResultFileName, sCanonicalRoot) || archivePathHasUnsafeLink(sCanonicalRoot, sSafeRecordPath)) {
                bResult = false;
            } else if (bIsFolder) {
                if (!XBinary::createDirectory(sResultFileName)) bResult = false;
            } else if (!XBinary::createDirectory(QFileInfo(sResultFileName).absolutePath()) ||
                       archivePathHasUnsafeLink(sCanonicalRoot, sSafeRecordPath)) {
                bResult = false;
            } else {
                QFile fileResult(sResultFileName);
                if (!fileResult.open(QIODevice::ReadWrite | QIODevice::Truncate) ||
                    !unpackCurrent(&state, &fileResult, pPdStruct)) {
                    bResult = false;
                }
                fileResult.close();
            }
        }

        const qint32 nPreviousIndex = state.nCurrentIndex;
        if (!moveToNext(&state, pPdStruct)) break;
        if (state.nCurrentIndex <= nPreviousIndex) bEnumerationValid = false;
    }

    const bool bFinished = finishUnpack(&state, nullptr);
    return bResult && bEnumerationValid && bFinished && isPdStructNotCanceled(pPdStruct);
}

bool XArchive::decompressToPath(const QString &sArchiveFileName, const QString &sRecordPathName, const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sArchiveFileName);

    if (file.open(QIODevice::ReadOnly)) {
        QIODevice *pOriginalDevice = getDevice();
        setDevice(&file);

        if (isValid(pPdStruct)) {
            QList<RECORD> listRecords = getRecords(-1, pPdStruct);

            bResult = decompressToPath(&listRecords, sRecordPathName, sResultPathName, pPdStruct);
        }

        // Never leave the archive pointing at this stack-local QFile.
        setDevice(pOriginalDevice);
        file.close();
    }

    return bResult;
}

bool XArchive::dumpToFile(const XArchive::RECORD *pRecord, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    if (!pRecord) return false;
    return XBinary::dumpToFile(sFileName, pRecord->nDataOffset, pRecord->nDataSize, pPdStruct);
}

XArchive::RECORD XArchive::getArchiveRecord(const QString &sRecordFileName, QList<XArchive::RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    RECORD result = {};

    if (!pListRecords) return result;

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (pListRecords->at(i).spInfo.sRecordName == sRecordFileName) {
            result = pListRecords->at(i);
            break;
        }
    }

    return result;
}

XArchive::RECORD XArchive::getArchiveRecordByUUID(const QString &sUUID, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    RECORD result = {};

    if (!pListRecords) return result;

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (pListRecords->at(i).sUUID == sUUID) {
            result = pListRecords->at(i);
            break;
        }
    }

    return result;
}

bool XArchive::isArchiveRecordPresent(const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords = getRecords(-1, pPdStruct);

    return isArchiveRecordPresent(sRecordFileName, &listRecords, pPdStruct);
}

bool XArchive::isArchiveRecordPresent(const QString &sRecordFileName, QList<XArchive::RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    return (!getArchiveRecord(sRecordFileName, pListRecords, pPdStruct).spInfo.sRecordName.isEmpty());
}

bool XArchive::isArchiveRecordPresentExp(const QString &sRecordFileName, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pListRecords) return false;

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (isRegExpPresent(sRecordFileName, pListRecords->at(i).spInfo.sRecordName)) {
            bResult = true;
            break;
        }
    }

    return bResult;
}

quint32 XArchive::getCompressBufferSize()
{
    return COMPRESS_BUFFERSIZE;
}

quint32 XArchive::getDecompressBufferSize()
{
    return DECOMPRESS_BUFFERSIZE;
}

void XArchive::showRecords(QList<XArchive::RECORD> *pListArchive)
{
    if (!pListArchive) return;
    qint32 nNumberOfRecords = pListArchive->count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
#ifdef QT_DEBUG
        qDebug("%s", pListArchive->at(i).spInfo.sRecordName.toUtf8().data());
#endif
    }
}

QList<XBinary::FPART_PROP> XArchive::getAvailableFPARTProperties()
{
    return XBinary::getAvailableFPARTProperties();
}

XBinary::MODE XArchive::getMode()
{
    return MODE_DATA;
}

qint32 XArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XArchive::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_ARCHIVE: sResult = tr("Archive"); break;
        case TYPE_DOSEXTENDER: sResult = QString("DOS %1").arg(tr("extender")); break;
    }

    return sResult;
}

bool XArchive::isArchive()
{
    return true;
}

bool XArchive::_writeToDevice(char *pBuffer, qint32 nBufferSize, DECOMPRESSSTRUCT *pDecompressStruct)
{
    bool bResult = true;

    if (pDecompressStruct->pDestDevice) {
        char *_pOffset = pBuffer;
        qint32 _nSize = nBufferSize;
        qint64 nDecompressedSize = pDecompressStruct->nDecompressedLimit;

        if (nDecompressedSize == -1) {
            nDecompressedSize = pDecompressStruct->nOutSize + nBufferSize;
        }

        if ((pDecompressStruct->nDecompressedOffset) < (pDecompressStruct->nOutSize + nBufferSize)) {
            if ((pDecompressStruct->nDecompressedOffset < (pDecompressStruct->nOutSize + nBufferSize)) &&
                (pDecompressStruct->nDecompressedOffset > pDecompressStruct->nOutSize)) {
                _pOffset += (pDecompressStruct->nDecompressedOffset - pDecompressStruct->nOutSize);
                _nSize -= (pDecompressStruct->nDecompressedOffset - pDecompressStruct->nOutSize);
            }

            if ((pDecompressStruct->nDecompressedOffset + nDecompressedSize) < (pDecompressStruct->nOutSize + nBufferSize)) {
                _nSize -= ((pDecompressStruct->nOutSize + nBufferSize) - (pDecompressStruct->nDecompressedOffset + nDecompressedSize));
            }

            if (_nSize > 0) {
                if (archiveWriteAll(pDecompressStruct->pDestDevice, _pOffset, _nSize, nullptr)) {
                    pDecompressStruct->nDecompressedWrote += _nSize;
                } else {
                    bResult = false;
                }
            }
        }
    }

    return bResult;
}

QString XArchive::_normalizeOutputPath(const QString &sPath)
{
    return QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(sPath).absoluteFilePath()));
}

bool XArchive::_isSafeChildPath(const QString &sPath, const QString &sCanonicalRoot)
{
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif

    const QString sNormalizedPath = _normalizeOutputPath(sPath);
    const QString sNormalizedRoot = _normalizeOutputPath(sCanonicalRoot);

    if (sNormalizedPath.compare(sNormalizedRoot, pathCaseSensitivity) == 0) {
        return false;
    }

    QString sExpectedRoot = sNormalizedRoot;
    if (!sExpectedRoot.endsWith(QLatin1Char('/'))) {
        sExpectedRoot.append(QLatin1Char('/'));
    }

    return sNormalizedPath.startsWith(sExpectedRoot, pathCaseSensitivity);
}

// XBinary::_MEMORY_MAP XArchive::getMemoryMap()
//{
//     _MEMORY_MAP result={};

//    qint64 nTotalSize=getSize();

//    result.nBaseAddress=_getBaseAddress();
//    result.nRawSize=nTotalSize;
//    result.nImageSize=nTotalSize;
//    result.fileType=FT_ARCHIVE;
//    result.mode=getMode();
//    result.sArch=getArch();
//    result.bIsBigEndian=isBigEndian();
//    result.sType=getTypeAsString();

//    qint32 nIndex=0;

//    QList<XArchive::RECORD> listRecords=getRecords();

//    qint32 nNumberOfRecords=listRecords.count();

//    for(qint32 i=0;i<nNumberOfRecords;i++)
//    {
//        _MEMORY_RECORD record={};
//        record.nAddress=-1;
//        record.segment=ADDRESS_SEGMENT_FLAT;
//        record.nOffset=listRecords.at(i).nDataOffset;
//        record.nSize=listRecords.at(i).nDataSize;
//        record.nIndex=nIndex++;
//        record.type=MMT_FILESEGMENT;
//        record.sName=listRecords.at(i).sFileName;

//        result.listRecords.append(record);
//    }

//    return result;
//}

QList<QString> XArchive::getSearchSignatures()
{
    return XBinary::getSearchSignatures();
}

XBinary *XArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    return XBinary::createInstance(pDevice, bIsImage, nModuleAddress);
}

QMap<XBinary::UNPACK_PROP, QVariant> XArchive::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    return result;
}

bool XArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState && pDevice && (pState->nCurrentIndex >= 0) && (pState->nNumberOfRecords > 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        XBinary::ARCHIVERECORD archiveRecord = infoCurrent(pState, pPdStruct);

        if (archiveRecord.mapProperties.value(XBinary::FPART_PROP_ISFOLDER).toBool()) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            if (pDevice->isSequential()) return pDevice->pos() == 0;

            const bool bReset = pDevice->seek(0) && ((pDevice->size() == 0) || XBinary::resize(pDevice, 0));
            return bReset && XBinary::isPdStructNotCanceled(pPdStruct);  // Directory
        }

        XBinary::CRC_TYPE crcType =
            (XBinary::CRC_TYPE)archiveRecord.mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
        bool bCheckCRC = XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType) && (crcType != XBinary::CRC_TYPE_UNKNOWN) &&
                         archiveRecord.mapProperties.contains(XBinary::FPART_PROP_RESULTCRC);
        QIODevice *pWorkDevice = pDevice;
        QIODevice *pCRCBuffer = nullptr;

        if (bCheckCRC && !pDevice->isReadable()) {
            qint64 nExpectedSize = archiveRecord.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();
            pCRCBuffer = XBinary::createFileBuffer(qMax((qint64)0, nExpectedSize), pPdStruct);
            pWorkDevice = pCRCBuffer;

            if (!pWorkDevice) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Cannot create CRC verification buffer"));
                return false;
            }
        }

        // Decoder writes start at offset zero, so make the destination an
        // exact new result rather than leaving a stale tail from earlier
        // contents.  For an unsupported random-access QIODevice, fail instead
        // of silently returning a mixture of old and new bytes.
        if (!pWorkDevice->isSequential()) {
            bool bOutputReset = pWorkDevice->seek(0);

            if (bOutputReset && (pWorkDevice->size() != 0)) {
                bOutputReset = XBinary::resize(pWorkDevice, 0);
            }

            if (!bOutputReset) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Cannot clear unpacked output"));
                XBinary::freeFileBuffer(&pCRCBuffer);
                return false;
            }
        }

        // A zero output size is not proof that the compressed stream is valid.
        // Run it through the normal pipeline so decoders can consume stream
        // terminators and encryption layers can authenticate empty payloads.
        XDecompress xDecompress;
        connect(&xDecompress, &XDecompress::errorMessage, this, &XBinary::errorMessage);
        connect(&xDecompress, &XDecompress::infoMessage, this, &XBinary::infoMessage);

        bResult = xDecompress.decompressArchiveRecord(archiveRecord, getDevice(), pWorkDevice, pState->mapUnpackProperties, pPdStruct);

        if (bResult && pCRCBuffer) {
            bResult = pCRCBuffer->seek(0);

            if (bResult && !pDevice->isSequential()) {
                bResult = pDevice->seek(0);

                // The verified buffer is the complete result.  Remove any
                // previous caller data before copying it, including the
                // zero-byte case where the copy loop performs no writes.
                if (bResult && (pDevice->size() != 0)) {
                    bResult = XBinary::resize(pDevice, 0);
                }
            } else if (bResult && pDevice->isSequential() && (pCRCBuffer->size() == 0)) {
                bResult = (pDevice->pos() == 0);
            }

            QByteArray baBuffer(XBinary::getBufferSize(pPdStruct), 0);
            qint64 nRemaining = pCRCBuffer->size();

            while (bResult && (nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                qint64 nChunkSize = qMin(nRemaining, (qint64)baBuffer.size());
                qint64 nRead = pCRCBuffer->read(baBuffer.data(), nChunkSize);

                if (nRead != nChunkSize) {
                    bResult = false;
                    break;
                }

                qint64 nWritten = 0;
                while ((nWritten < nRead) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    qint64 nResult = pDevice->write(baBuffer.constData() + nWritten, nRead - nWritten);

                    if ((nResult <= 0) || (nResult > (nRead - nWritten))) {
                        bResult = false;
                        break;
                    }

                    nWritten += nResult;
                }

                if (!bResult || (nWritten != nRead)) {
                    bResult = false;
                    break;
                }

                nRemaining -= nRead;
            }

            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                bResult = false;
            }

            if (!bResult) {
                if (!pDevice->isSequential()) {
                    XBinary::resize(pDevice, 0);
                    pDevice->seek(0);
                }
                XBinary::setPdStructErrorString(pPdStruct, tr("Cannot write unpacked output"));
            }
        }

        XBinary::freeFileBuffer(&pCRCBuffer);
    }

    return bResult;
}

bool XArchive::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XBinary::handleInternalInfo(pPdStruct);
        static_cast<XBinary::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XBinary::INTERNAL_INFO *>(XBinary::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XArchive::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XArchive::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XBinary::setInternalInfo(nullptr);
    }
}
