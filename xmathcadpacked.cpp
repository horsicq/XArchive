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
#include "xmathcadpacked.h"

#include <QFileInfo>
#include <QPointer>

#include <cstring>
#include <new>

namespace {
// ".MCDCOMPRESSION" - 15 ASCII bytes, no NUL, no version field, no size field.
const char *const MATHCAD_MAGIC = ".MCDCOMPRESSION";
const qint64 MATHCAD_MAGIC_SIZE = 15;
const qint32 MATHCAD_WINDOW_SIZE = 4096;
const qint32 MATHCAD_WINDOW_START = 1;
const qint32 MATHCAD_LENGTH_BIAS = 2;
// Trial-decode budget for the probe.  Large enough that a real archive shows
// many tokens, small enough that probing an arbitrary file stays cheap.
const qint64 MATHCAD_PROBE_INPUT = 4096;
const qint64 MATHCAD_PROBE_OUTPUT = 32768;
const QString MATHCAD_FALLBACK_NAME = QStringLiteral("mathcad.mcd");

// Structural walk over the bit stream.  It does not reconstruct the payload; it
// only checks that the token grammar holds and that every back-reference taken
// before the ring wraps points strictly behind the write pointer, which is the
// invariant the encoder cannot violate and random data almost never satisfies.
//
// bComplete says the sample covers the whole stream; then the explicit end
// token is required and at most seven padding bits may follow it.
bool mathcadProbeStream(const QByteArray &baStream, bool bComplete)
{
    const qint64 nTotalBits = static_cast<qint64>(baStream.size()) * 8;
    qint64 nBitPos = 0;
    qint32 nWindowPos = MATHCAD_WINDOW_START;
    bool bWrapped = false;
    qint64 nProduced = 0;
    bool bTerminated = false;

    while (nBitPos < nTotalBits) {
        const bool bLiteral = ((static_cast<quint8>(baStream.at(static_cast<qint32>(nBitPos >> 3))) >> (7 - (nBitPos & 7))) & 1) != 0;
        nBitPos++;

        qint32 nLength = 0;

        if (bLiteral) {
            if ((nBitPos + 8) > nTotalBits) break;
            nBitPos += 8;
            nLength = 1;
        } else {
            if ((nBitPos + 12) > nTotalBits) break;
            quint32 nPosition = 0;
            for (qint32 i = 0; i < 12; i++) {
                nPosition = (nPosition << 1) | ((static_cast<quint8>(baStream.at(static_cast<qint32>(nBitPos >> 3))) >> (7 - (nBitPos & 7))) & 1);
                nBitPos++;
            }

            if (nPosition == 0) {
                bTerminated = true;
                break;
            }
            if (!bWrapped && (static_cast<qint32>(nPosition) >= nWindowPos)) {
                return false;
            }
            if ((nBitPos + 4) > nTotalBits) break;
            quint32 nLengthCode = 0;
            for (qint32 i = 0; i < 4; i++) {
                nLengthCode = (nLengthCode << 1) | ((static_cast<quint8>(baStream.at(static_cast<qint32>(nBitPos >> 3))) >> (7 - (nBitPos & 7))) & 1);
                nBitPos++;
            }
            nLength = static_cast<qint32>(nLengthCode) + MATHCAD_LENGTH_BIAS;
        }

        for (qint32 i = 0; i < nLength; i++) {
            nWindowPos = (nWindowPos + 1) & (MATHCAD_WINDOW_SIZE - 1);
            if (nWindowPos == 0) bWrapped = true;
        }
        nProduced += nLength;

        if (nProduced > MATHCAD_PROBE_OUTPUT) break;
    }

    if (nProduced <= 0) return false;

    if (bComplete) {
        // A whole stream that never reaches its end token is truncated or is
        // simply not this format; either way it must not be reported valid.
        return bTerminated && ((nTotalBits - nBitPos) < 8);
    }

    return true;
}
}  // namespace

XMathCadPacked::XMathCadPacked(QIODevice *pDevice) : XArchive(pDevice)
{
}

QString XMathCadPacked::deriveMemberName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return MATHCAD_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return MATHCAD_FALLBACK_NAME;

    // The container's own name is the only name this format carries; the
    // decompressed worksheet keeps it verbatim, extension included.
    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return MATHCAD_FALLBACK_NAME;

    return sFileName;
}

bool XMathCadPacked::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XMathCadPacked> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (MATHCAD_MAGIC_SIZE + 1)) return false;

    const QByteArray baMagic = read_array_process(0, MATHCAD_MAGIC_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baMagic.size() != MATHCAD_MAGIC_SIZE)) return false;
    if (std::memcmp(baMagic.constData(), MATHCAD_MAGIC, static_cast<size_t>(MATHCAD_MAGIC_SIZE)) != 0) return false;

    context.nStreamOffset = MATHCAD_MAGIC_SIZE;
    context.nStreamSize = context.nInputSize - MATHCAD_MAGIC_SIZE;

    const qint64 nSampleSize = (context.nStreamSize < MATHCAD_PROBE_INPUT) ? context.nStreamSize : MATHCAD_PROBE_INPUT;
    const QByteArray baSample = read_array_process(context.nStreamOffset, nSampleSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baSample.size() != nSampleSize)) return false;

    if (!mathcadProbeStream(baSample, nSampleSize == context.nStreamSize)) return false;

    context.sFileName = deriveMemberName();
    if (!guardedThis || !guardedSource) return false;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XMathCadPacked::isValid(PDSTRUCT *pPdStruct)
{
    // The probe runs on a device the caller still owns: remember where its
    // cursor was and put it back, whatever the outcome.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XMathCadPacked::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMathCadPacked archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XMathCadPacked::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XMathCadPacked(pDevice);
}

QList<QString> XMathCadPacked::getSearchSignatures()
{
    return {QStringLiteral("'.MCDCOMPRESSION'")};
}

XBinary::FT XMathCadPacked::getFileType()
{
    return FT_MATHCAD_PACK;
}

XBinary::MODE XMathCadPacked::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMathCadPacked::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMathCadPacked::getArch()
{
    return QString();
}

qint32 XMathCadPacked::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XMathCadPacked::getFileFormatExt()
{
    return QStringLiteral("mcd");
}

QString XMathCadPacked::getFileFormatExtsString()
{
    return QStringLiteral("MathCAD compressed worksheet (*.mcd *.smr)");
}

QString XMathCadPacked::getMIMEString()
{
    return QStringLiteral("application/x-mathcad");
}

QString XMathCadPacked::getVersion()
{
    // The magic carries no version field and the stream has no parameter byte:
    // the first payload byte is already the first flag bit of the LZSS stream.
    return QString();
}

qint64 XMathCadPacked::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XMathCadPacked::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMathCadPacked::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XMathCadPacked::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XMathCadPacked::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = MATHCAD_MAGIC_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        record.nFileOffset = context.nStreamOffset;
        record.nFileSize = context.nStreamSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        // No FPART_PROP_UNCOMPRESSEDSIZE: the format stores no output size and
        // publishing a guess here would cap the decoder at the wrong length.
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MATHCAD);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("MathCAD LZSS"));
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XMathCadPacked::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMathCadPacked::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMathCadPacked> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("MathCAD compressed worksheet; single LZSS member"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source; without this finalize the listing would
    // work and every extraction would silently produce nothing.
    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XMathCadPacked::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    // Deliberately no FPART_PROP_UNCOMPRESSEDSIZE: the stream is
    // terminator-driven and the base unpack path stages it accordingly.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MATHCAD);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("MathCAD LZSS"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XMathCadPacked::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // The index must move PAST the last record; stopping one short makes both
    // the GUI and the CLI list nothing at all.
    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XMathCadPacked::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}
