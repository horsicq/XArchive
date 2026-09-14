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
#include "xedcpackedarchive.h"

#include "Algos/xnetwarepackdecoder.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {

const char *const EDC_MAGIC = " EDC Packed ";
const qint32 EDC_MAGIC_SIZE = 12;
const qint32 EDC_NAME_OFFSET = 12;
// A FIXED-WIDTH field: all 12 bytes belong to the name, and a name that fills
// the field has no NUL at all.
const qint32 EDC_NAME_SIZE = 12;
const qint32 EDC_EOF_OFFSET = 24;
const qint32 EDC_VERSION_OFFSET = 25;
const qint32 EDC_UNPACKEDSIZE_OFFSET = 27;
const qint32 EDC_MEMBERSIZE_OFFSET = 31;
const qint32 EDC_RESERVED_OFFSET = 35;
const qint32 EDC_TIME_OFFSET = 37;
const qint32 EDC_DATE_OFFSET = 39;
const qint32 EDC_HEADER_SIZE = 41;

const quint8 EDC_EOF_MARKER = 0x1AU;
const quint16 EDC_VERSION = 3;

// The producer writes both sizes as SIGNED 32-bit, so the high bit never
// appears; the codec cannot address more than a qint32 of output either.
const qint64 EDC_MAX_SIZE = Q_INT64_C(0x7FFFFFFF);
const qint64 EDC_MAX_FILE_SIZE = Q_INT64_C(2) * 1024 * 1024 * 1024;
const qint32 EDC_MAX_MEMBERS = 0x100000;

// Trial-decode budget for detection.  Wide enough that a real member walks
// well past its three Huffman tables, cheap enough to run on every probed file.
const qint64 EDC_PROBE_INPUT = 8192;
const qint64 EDC_PROBE_OUTPUT = 16384;

const QString EDC_FALLBACK_NAME = QStringLiteral("edc.bin");

}  // namespace

XEDCPackedArchive::XEDCPackedArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEDCPackedArchive::~XEDCPackedArchive()
{
}

// The member name becomes an output file name, so every separator, traversal
// and control character has to be rejected here rather than downstream.
bool XEDCPackedArchive::isUsableMemberName(const QByteArray &baName)
{
    const qint32 nSize = baName.size();
    if ((nSize < 1) || (nSize > EDC_NAME_SIZE)) return false;
    if ((baName == ".") || (baName == "..")) return false;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = (quint8)baName.at(i);
        if ((nCharacter < 0x20U) || (nCharacter >= 0x7FU)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }

    return true;
}

QString XEDCPackedArchive::deriveContainerName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return EDC_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return EDC_FALLBACK_NAME;

    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return EDC_FALLBACK_NAME;

    return sFileName;
}

// Every constant the reference predicate tests is tested here, in the same
// places: the 12-byte magic, the 0x1A end-of-text marker, the version word and
// the reserved word, plus both sizes being non-negative.
bool XEDCPackedArchive::parseMemberHeader(const QByteArray &baHeader, qint64 *pnUncompressedSize, qint64 *pnMemberSize, QByteArray *pbaName, quint16 *pnDosTime,
                                          quint16 *pnDosDate)
{
    if (baHeader.size() != EDC_HEADER_SIZE) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (std::memcmp(pHeader, EDC_MAGIC, (size_t)EDC_MAGIC_SIZE) != 0) return false;
    if (pHeader[EDC_EOF_OFFSET] != EDC_EOF_MARKER) return false;
    if (qFromLittleEndian<quint16>(pHeader + EDC_VERSION_OFFSET) != EDC_VERSION) return false;
    if (qFromLittleEndian<quint16>(pHeader + EDC_RESERVED_OFFSET) != 0) return false;

    const qint64 nUncompressedSize = (qint64)(qint32)qFromLittleEndian<quint32>(pHeader + EDC_UNPACKEDSIZE_OFFSET);
    const qint64 nMemberSize = (qint64)(qint32)qFromLittleEndian<quint32>(pHeader + EDC_MEMBERSIZE_OFFSET);

    if ((nUncompressedSize < 0) || (nUncompressedSize > EDC_MAX_SIZE)) return false;
    // The size at 0x1F covers the header as well, so a member that claims no
    // more than the header carries no stream at all.  The codec has nothing to
    // read there, and accepting it would only publish an empty file under a
    // real name; no member in the reference corpus is smaller than 201 bytes.
    if (nMemberSize <= EDC_HEADER_SIZE) return false;

    const QByteArray baName = baHeader.mid(EDC_NAME_OFFSET, EDC_NAME_SIZE);
    if (baName.size() != EDC_NAME_SIZE) return false;

    const qint32 nNulPosition = baName.indexOf('\0');
    const QByteArray baTrimmed = (nNulPosition >= 0) ? baName.left(nNulPosition) : baName;

    *pnUncompressedSize = nUncompressedSize;
    *pnMemberSize = nMemberSize;
    *pbaName = baTrimmed;
    *pnDosTime = qFromLittleEndian<quint16>(pHeader + EDC_TIME_OFFSET);
    *pnDosDate = qFromLittleEndian<quint16>(pHeader + EDC_DATE_OFFSET);

    return true;
}

bool XEDCPackedArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XEDCPackedArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((context.nInputSize <= EDC_HEADER_SIZE) || (context.nInputSize > EDC_MAX_FILE_SIZE)) return false;

    qint64 nOffset = 0;

    while ((nOffset + EDC_HEADER_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= EDC_MAX_MEMBERS) return false;

        const QByteArray baHeader = read_array_process(nOffset, EDC_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baHeader.size() != EDC_HEADER_SIZE) break;

        qint64 nUncompressedSize = 0;
        qint64 nMemberSize = 0;
        QByteArray baName;
        quint16 nDosTime = 0;
        quint16 nDosDate = 0;

        if (!parseMemberHeader(baHeader, &nUncompressedSize, &nMemberSize, &baName, &nDosTime, &nDosDate)) break;
        // A member that runs past the end of the file is not a member: stop the
        // walk and leave the remainder as overlay rather than publish a record
        // whose stream cannot be read.
        if (nMemberSize > (context.nInputSize - nOffset)) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nStreamOffset = nOffset + EDC_HEADER_SIZE;
        member.nStreamSize = nMemberSize - EDC_HEADER_SIZE;
        member.nUncompressedSize = nUncompressedSize;
        member.nDosTime = nDosTime;
        member.nDosDate = nDosDate;
        member.sFileName = isUsableMemberName(baName) ? QString::fromLatin1(baName) : deriveContainerName();
        if (!guardedThis || !guardedSource) return false;

        context.listMembers.append(member);

        nOffset += nMemberSize;
    }

    if (context.listMembers.isEmpty()) return false;

    // A 17-byte fixed prefix is strong, but not strong enough on its own to
    // hand an arbitrary file to a decoder: walk the real bit stream of the
    // first member far enough to prove the three Huffman tables and the token
    // grammar hold.
    const MEMBER &first = context.listMembers.at(0);
    const qint64 nSampleSize = (first.nStreamSize < EDC_PROBE_INPUT) ? first.nStreamSize : EDC_PROBE_INPUT;
    const QByteArray baSample = read_array_process(first.nStreamOffset, nSampleSize, pPdStruct);
    if (!guardedThis || !guardedSource || ((qint64)baSample.size() != nSampleSize)) return false;

    if (!XNetWarePackDecoder::probe(baSample, first.nUncompressedSize, nSampleSize == first.nStreamSize, EDC_PROBE_OUTPUT)) return false;

    context.nArchiveSize = nOffset;
    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XEDCPackedArchive::isValid(PDSTRUCT *pPdStruct)
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

bool XEDCPackedArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEDCPackedArchive archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XEDCPackedArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XEDCPackedArchive(pDevice);
}

QList<QString> XEDCPackedArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("' EDC Packed '");
}

XBinary::FT XEDCPackedArchive::getFileType()
{
    return FT_EDC_PACKED;
}

XBinary::MODE XEDCPackedArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEDCPackedArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEDCPackedArchive::getArch()
{
    return QString();
}

qint32 XEDCPackedArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XEDCPackedArchive::getFileFormatExt()
{
    // The containers keep the DOS "last extension character replaced" habit
    // (PING.PI_, IPXODI.MS_); the multi-member ones are named ".PAC".
    return QStringLiteral("pac");
}

QString XEDCPackedArchive::getFileFormatExtsString()
{
    return QStringLiteral("EDC Packed (*.pac *.*_)");
}

QString XEDCPackedArchive::getMIMEString()
{
    return QStringLiteral("application/x-edc-packed");
}

QString XEDCPackedArchive::getVersion()
{
    return QString::number((qint32)EDC_VERSION);
}

qint64 XEDCPackedArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XEDCPackedArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XEDCPackedArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XEDCPackedArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XEDCPackedArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = EDC_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if (nFileParts & FILEPART_STREAM) {
        for (qint32 i = 0; i < context.listMembers.size(); i++) {
            if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;

            const MEMBER &member = context.listMembers.at(i);

            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nStreamOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NETWARE_PACK);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("NetWare pack"));
            const QDateTime dateTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
            if (dateTime.isValid()) part.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XEDCPackedArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEDCPackedArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XEDCPackedArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("EDC Packed; chained members"));
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XEDCPackedArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NETWARE_PACK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("NetWare pack"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dateTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dateTime.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);

    return result;
}

bool XEDCPackedArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    // The index must move PAST the last record; stopping one short makes both
    // the GUI and the CLI list nothing at all.
    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XEDCPackedArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XEDCPackedArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_DATETIME << FPART_PROP_ISFOLDER;
}
