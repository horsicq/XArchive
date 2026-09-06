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
#include "xnetwarepackedfile.h"

#include "Algos/xnetwarepackdecoder.h"

#include <QFileInfo>
#include <QPointer>

#include <cstring>
#include <limits>
#include <new>

namespace {

const char *const NWPF_MAGIC = "Packed File ";
const qint32 NWPF_MAGIC_SIZE = 12;
const qint32 NWPF_NAME_OFFSET = 12;
// A FIXED-WIDTH field: all 12 bytes belong to the name, and a name that fills
// the field has no NUL at all.  Reading 11 here would clip every one of them.
const qint32 NWPF_NAME_SIZE = 12;
const qint32 NWPF_EOF_OFFSET = 24;
const qint32 NWPF_VERSION_OFFSET = 25;
const qint32 NWPF_METHOD_OFFSET = 26;
const qint32 NWPF_SIZE_OFFSET = 27;
const qint32 NWPF_HEADER_SIZE = 31;

const quint8 NWPF_EOF_MARKER = 0x1AU;
const quint8 NWPF_VERSION = 0x01U;
const quint8 NWPF_METHOD_LZH = 0x0AU;

// The u32 is signed in the producer, so 0x80000000 and above never appear; the
// codec also cannot address more than a qint32 of output.
const qint64 NWPF_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(0x7FFFFFFF);
const qint64 NWPF_MAX_FILE_SIZE = Q_INT64_C(1) * 1024 * 1024 * 1024;

// Trial-decode budget for detection.  Wide enough that a real member walks well
// past its three Huffman tables, cheap enough to run on every probed file.
const qint64 NWPF_PROBE_INPUT = 8192;
const qint64 NWPF_PROBE_OUTPUT = 16384;

const QString NWPF_FALLBACK_NAME = QStringLiteral("netware.bin");

quint32 nwpfReadUInt32(const QByteArray &baData, qint32 nOffset)
{
    return (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset)))) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24);
}

}  // namespace

XNetWarePackedFile::XNetWarePackedFile(QIODevice *pDevice) : XArchive(pDevice)
{
}

// The member name becomes an output file name, so every separator, traversal
// and control character has to be rejected here rather than downstream.
bool XNetWarePackedFile::isUsableMemberName(const QByteArray &baName)
{
    const qint32 nSize = baName.size();
    if ((nSize < 1) || (nSize > NWPF_NAME_SIZE)) return false;
    if ((baName == ".") || (baName == "..")) return false;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if ((nCharacter < 0x20U) || (nCharacter >= 0x7FU)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }

    return true;
}

QString XNetWarePackedFile::deriveContainerName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return NWPF_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return NWPF_FALLBACK_NAME;

    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return NWPF_FALLBACK_NAME;

    return sFileName;
}

bool XNetWarePackedFile::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XNetWarePackedFile> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((context.nInputSize <= NWPF_HEADER_SIZE) || (context.nInputSize > NWPF_MAX_FILE_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, NWPF_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != NWPF_HEADER_SIZE)) return false;

    if (std::memcmp(baHeader.constData(), NWPF_MAGIC, static_cast<size_t>(NWPF_MAGIC_SIZE)) != 0) return false;
    if (static_cast<quint8>(baHeader.at(NWPF_EOF_OFFSET)) != NWPF_EOF_MARKER) return false;

    context.nVersion = static_cast<quint8>(baHeader.at(NWPF_VERSION_OFFSET));
    context.nMethod = static_cast<quint8>(baHeader.at(NWPF_METHOD_OFFSET));
    // Only 01/0A exists in the wild.  Anything else is a container this decoder
    // cannot read, and reporting it as valid would publish garbage at exit 0.
    if ((context.nVersion != NWPF_VERSION) || (context.nMethod != NWPF_METHOD_LZH)) return false;

    const quint32 nDeclaredSize = nwpfReadUInt32(baHeader, NWPF_SIZE_OFFSET);
    if (nDeclaredSize > static_cast<quint32>(NWPF_MAX_UNCOMPRESSED_SIZE)) return false;

    context.nUncompressedSize = static_cast<qint64>(nDeclaredSize);
    context.nStreamOffset = NWPF_HEADER_SIZE;
    context.nStreamSize = context.nInputSize - NWPF_HEADER_SIZE;
    if (context.nStreamSize <= 0) return false;

    const QByteArray baName = baHeader.mid(NWPF_NAME_OFFSET, NWPF_NAME_SIZE);
    if (baName.size() != NWPF_NAME_SIZE) return false;

    // The field is NUL padded, so the name stops at the first NUL; a field that
    // is not a name at all (24 of the reference files carry packer scratch
    // there) falls back to the container's own file name.
    const qint32 nNulPosition = baName.indexOf('\0');
    const QByteArray baTrimmed = (nNulPosition >= 0) ? baName.left(nNulPosition) : baName;

    context.bNameFromHeader = isUsableMemberName(baTrimmed);
    if (context.bNameFromHeader) {
        context.sFileName = QString::fromLatin1(baTrimmed);
    } else {
        context.sFileName = deriveContainerName();
        if (!guardedThis || !guardedSource) return false;
    }

    // A 15-byte fixed prefix is strong, but not strong enough on its own to
    // hand an arbitrary file to a decoder: walk the real bit stream far enough
    // to prove the three Huffman tables and the token grammar hold.
    const qint64 nSampleSize = (context.nStreamSize < NWPF_PROBE_INPUT) ? context.nStreamSize : NWPF_PROBE_INPUT;
    const QByteArray baSample = read_array_process(context.nStreamOffset, nSampleSize, pPdStruct);
    if (!guardedThis || !guardedSource || (static_cast<qint64>(baSample.size()) != nSampleSize)) return false;

    if (!XNetWarePackDecoder::probe(baSample, context.nUncompressedSize, nSampleSize == context.nStreamSize, NWPF_PROBE_OUTPUT)) {
        return false;
    }

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XNetWarePackedFile::isValid(PDSTRUCT *pPdStruct)
{
    // Detection runs on a device the caller still owns: snapshot the cursor and
    // put it back whatever the outcome.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XNetWarePackedFile::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNetWarePackedFile archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XNetWarePackedFile::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XNetWarePackedFile(pDevice);
}

QList<QString> XNetWarePackedFile::getSearchSignatures()
{
    return {QStringLiteral("'Packed File '")};
}

XBinary::FT XNetWarePackedFile::getFileType()
{
    return FT_NETWARE_PACK;
}

XBinary::MODE XNetWarePackedFile::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XNetWarePackedFile::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XNetWarePackedFile::getArch()
{
    return QString();
}

qint32 XNetWarePackedFile::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XNetWarePackedFile::getFileFormatExt()
{
    // These members keep the DOS "last extension character replaced" habit
    // ('_' or '@'), so there is no extension of their own; the sibling
    // NetWare install-file class reports the same placeholder.
    return QStringLiteral("_");
}

QString XNetWarePackedFile::getFileFormatExtsString()
{
    return QStringLiteral("Personal NetWare Packed File (*.*_ *.*@)");
}

QString XNetWarePackedFile::getMIMEString()
{
    return QStringLiteral("application/x-netware-packed");
}

QString XNetWarePackedFile::getVersion()
{
    // Probes a device the caller still owns; put its cursor back either way.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, nullptr);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bResult) return QString();

    // The header's format version.  The method byte is the codec selector, not
    // a container version, and is published through FPART_PROP_REPORTEDMETHOD.
    return QString::number(static_cast<qint32>(context.nVersion));
}

qint64 XNetWarePackedFile::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XNetWarePackedFile::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XNetWarePackedFile::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XNetWarePackedFile::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XNetWarePackedFile::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = NWPF_HEADER_SIZE;
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
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NETWARE_PACK);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("NetWare pack 01/0A"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XNetWarePackedFile::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XNetWarePackedFile::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XNetWarePackedFile> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Personal NetWare Packed File; single member"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source.  Without this finalize the listing works
    // and every extraction silently writes nothing.
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

XBinary::ARCHIVERECORD XNetWarePackedFile::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NETWARE_PACK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("NetWare pack 01/0A"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XNetWarePackedFile::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XNetWarePackedFile::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
