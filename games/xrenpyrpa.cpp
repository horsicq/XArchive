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
#include "xrenpyrpa.h"

#include <QBuffer>
#include <QHash>
#include <QTemporaryFile>
#include <QtEndian>
#include <cstring>
#include <new>

#include "xdeflatedecoder.h"

namespace {
const qint64 MaxHeaderLine = 128;
const qint64 MaxCompressedIndex = 64LL * 1024 * 1024;
const qint64 MaxIndexSize = 256LL * 1024 * 1024;
const qint64 MaxOutputSize = 4LL * 1024 * 1024 * 1024;
const qint32 MaxMembers = 1000000;
const qint32 MaxStackDepth = 100000;
const qint32 ChunkSize = 1024 * 1024;

bool isHexString(const QByteArray &baText)
{
    if (baText.isEmpty()) return false;
    const qint32 nCount = baText.size();
    for (qint32 i = 0; i < nCount; i++) {
        const char c = baText.at(i);
        const bool bHex = ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
        if (!bHex) return false;
    }
    return true;
}

bool isMemberNameSafe(const QString &sName)
{
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) || sName.contains(QLatin1Char(':')) || sName.contains(QLatin1Char('\\'))) return false;
    const QStringList listParts = sName.split(QLatin1Char('/'));
    const qint32 nCount = listParts.size();
    for (qint32 i = 0; i < nCount; i++) {
        const QString &sPart = listParts.at(i);
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
        const qint32 nLength = sPart.size();
        for (qint32 j = 0; j < nLength; j++) {
            if (sPart.at(j).unicode() < 0x20) return false;
        }
    }
    return true;
}

struct RPA_CANCELED {
    XRenpyRpa *owner;
    QIODevice *source;
    QIODevice *output;
    XBinary::PDSTRUCT *pPdStruct;
    RPA_CANCELED(XRenpyRpa *ownerRef, QIODevice *sourceRef, QIODevice *outputRef, XBinary::PDSTRUCT *pPd)
        : owner(ownerRef), source(sourceRef), output(outputRef), pPdStruct(pPd)
    {
    }
    bool operator()() const
    {
        return !owner || !source || !output || !XBinary::isPdStructNotCanceled(pPdStruct);
    }
};
}  // namespace

XRenpyRpa::XRenpyRpa(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRenpyRpa::~XRenpyRpa()
{
}

XBinary::FT XRenpyRpa::getFileType()
{
    return FT_RENPY_RPA;
}

XBinary::MODE XRenpyRpa::getMode()
{
    return MODE_DATA;
}

qint32 XRenpyRpa::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XRenpyRpa::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRenpyRpa::getArch()
{
    return QString();
}

QString XRenpyRpa::getVersion()
{
    HEADER header;
    return parseHeader(read_array(0, MaxHeaderLine), getSize(), &header) ? header.sVersion : QString();
}

QString XRenpyRpa::getFileFormatExt()
{
    return QStringLiteral("rpa");
}

QString XRenpyRpa::getFileFormatExtsString()
{
    return QStringLiteral("Ren'Py archive (*.rpa)");
}

QString XRenpyRpa::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

qint64 XRenpyRpa::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return readContext(&context, pPdStruct) ? context.nArchiveEnd : 0;
}

QList<QString> XRenpyRpa::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'RPA-'"));
    return listResult;
}

XBinary *XRenpyRpa::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRenpyRpa(pDevice);
}

bool XRenpyRpa::isValid(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return readContext(&context, pPdStruct);
}

bool XRenpyRpa::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRenpyRpa archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XRenpyRpa::parseHeader(const QByteArray &baHead, qint64 nTotalSize, HEADER *pHeader)
{
    if (!pHeader || (baHead.size() < 8) || (memcmp(baHead.constData(), "RPA-", 4) != 0)) return false;
    const qint32 nLineEnd = baHead.indexOf('\n');
    if ((nLineEnd < 0) || (nLineEnd > MaxHeaderLine)) return false;
    const QList<QByteArray> listFields = baHead.left(nLineEnd).split(' ');
    const qint32 nFields = listFields.size();
    if (nFields < 2) return false;
    const QByteArray baTag = listFields.at(0);
    HEADER header;
    if ((baTag == "RPA-2.0") && (nFields == 2)) {
        header.sVersion = QStringLiteral("2.0");
    } else if ((baTag == "RPA-3.0") && (nFields == 3)) {
        header.sVersion = QStringLiteral("3.0");
    } else if ((baTag == "RPA-3.2") && (nFields == 4)) {
        header.sVersion = QStringLiteral("3.2");
    } else {
        return false;
    }
    if ((listFields.at(1).size() != 16) || !isHexString(listFields.at(1))) return false;
    bool bOk = false;
    const qulonglong nOffset = listFields.at(1).toULongLong(&bOk, 16);
    if (!bOk || (nOffset <= (qulonglong)nLineEnd) || (nOffset >= (qulonglong)nTotalSize)) return false;
    header.nIndexOffset = (qint64)nOffset;
    if (nFields >= 3) {
        if ((listFields.at(2).size() != 8) || !isHexString(listFields.at(2))) return false;
        header.nKey = listFields.at(2).toUInt(&bOk, 16);
        if (!bOk) return false;
    }
    header.nHeaderSize = nLineEnd + 1;
    *pHeader = header;
    return true;
}

// ---------------------------------------------------------------- pickle ---
bool XRenpyRpa::unpickle(const QByteArray &baPickle, PVALUE *pResult)
{
    const uchar *pData = reinterpret_cast<const uchar *>(baPickle.constData());
    const qint64 nSize = baPickle.size();
    if (!pResult || (nSize < 3) || (pData[0] != 0x80) || ((pData[1] != 2) && (pData[1] != 3))) return false;
    qint64 nPos = 2;
    QList<PVALUE> stack;
    QHash<quint32, PVALUE> memo;

    while (nPos < nSize) {
        if (stack.size() > MaxStackDepth) return false;
        const uchar nOpcode = pData[nPos++];
        switch (nOpcode) {
            case '(': {  // MARK
                PVALUE mark;
                mark.type = PVALUE::TYPE_MARK;
                stack.append(mark);
                break;
            }
            case '}': {  // EMPTY_DICT
                PVALUE value;
                value.type = PVALUE::TYPE_DICT;
                stack.append(value);
                break;
            }
            case ']': {  // EMPTY_LIST
                PVALUE value;
                value.type = PVALUE::TYPE_LIST;
                stack.append(value);
                break;
            }
            case ')': {  // EMPTY_TUPLE
                PVALUE value;
                value.type = PVALUE::TYPE_TUPLE;
                stack.append(value);
                break;
            }
            case 'X':    // BINUNICODE
            case 'T':    // BINSTRING
            case 'B': {  // BINBYTES
                if (nSize - nPos < 4) return false;
                const qint64 nLength = qFromLittleEndian<quint32>(pData + nPos);
                nPos += 4;
                if ((nOpcode == 'T') && (nLength > 0x7FFFFFFF)) return false;
                if (nLength > nSize - nPos) return false;
                PVALUE value;
                value.type = (nOpcode == 'X') ? PVALUE::TYPE_TEXT : PVALUE::TYPE_BYTES;
                value.baData = QByteArray(reinterpret_cast<const char *>(pData + nPos), (int)nLength);
                nPos += nLength;
                stack.append(value);
                break;
            }
            case 'U':    // SHORT_BINSTRING
            case 'C': {  // SHORT_BINBYTES
                if (nSize - nPos < 1) return false;
                const qint64 nLength = pData[nPos++];
                if (nLength > nSize - nPos) return false;
                PVALUE value;
                value.type = PVALUE::TYPE_BYTES;
                value.baData = QByteArray(reinterpret_cast<const char *>(pData + nPos), (int)nLength);
                nPos += nLength;
                stack.append(value);
                break;
            }
            case 'J': {  // BININT
                if (nSize - nPos < 4) return false;
                PVALUE value;
                value.type = PVALUE::TYPE_INT;
                value.nInt = (qint32)qFromLittleEndian<quint32>(pData + nPos);
                nPos += 4;
                stack.append(value);
                break;
            }
            case 'K': {  // BININT1
                if (nSize - nPos < 1) return false;
                PVALUE value;
                value.type = PVALUE::TYPE_INT;
                value.nInt = pData[nPos++];
                stack.append(value);
                break;
            }
            case 'M': {  // BININT2
                if (nSize - nPos < 2) return false;
                PVALUE value;
                value.type = PVALUE::TYPE_INT;
                value.nInt = qFromLittleEndian<quint16>(pData + nPos);
                nPos += 2;
                stack.append(value);
                break;
            }
            case 0x8a: {  // LONG1: u8 length + little-endian two's complement
                if (nSize - nPos < 1) return false;
                const qint32 nLength = pData[nPos++];
                if ((nLength > 8) || (nLength > nSize - nPos)) return false;
                quint64 nRaw = 0;
                for (qint32 i = 0; i < nLength; i++) nRaw |= (quint64)pData[nPos + i] << (8 * i);
                if ((nLength > 0) && (nLength < 8) && (pData[nPos + nLength - 1] & 0x80)) {
                    nRaw |= ~((quint64(1) << (8 * nLength)) - 1);  // sign-extend
                }
                nPos += nLength;
                PVALUE value;
                value.type = PVALUE::TYPE_INT;
                value.nInt = (qint64)nRaw;
                stack.append(value);
                break;
            }
            case 't': {  // TUPLE: items back to the mark
                qint32 nMark = stack.size() - 1;
                while ((nMark >= 0) && (stack.at(nMark).type != PVALUE::TYPE_MARK)) nMark--;
                if (nMark < 0) return false;
                PVALUE value;
                value.type = PVALUE::TYPE_TUPLE;
                for (qint32 i = nMark + 1; i < stack.size(); i++) value.listItems.append(stack.at(i));
                while (stack.size() > nMark) stack.removeLast();
                stack.append(value);
                break;
            }
            case 0x85:    // TUPLE1
            case 0x86:    // TUPLE2
            case 0x87: {  // TUPLE3
                const qint32 nCount = nOpcode - 0x84;
                if (stack.size() < nCount) return false;
                PVALUE value;
                value.type = PVALUE::TYPE_TUPLE;
                for (qint32 i = stack.size() - nCount; i < stack.size(); i++) {
                    if (stack.at(i).type == PVALUE::TYPE_MARK) return false;
                    value.listItems.append(stack.at(i));
                }
                for (qint32 i = 0; i < nCount; i++) stack.removeLast();
                stack.append(value);
                break;
            }
            case 'q':    // BINPUT
            case 'r': {  // LONG_BINPUT
                quint32 nIndex = 0;
                if (nOpcode == 'q') {
                    if (nSize - nPos < 1) return false;
                    nIndex = pData[nPos++];
                } else {
                    if (nSize - nPos < 4) return false;
                    nIndex = qFromLittleEndian<quint32>(pData + nPos);
                    nPos += 4;
                }
                if (stack.isEmpty() || (stack.last().type == PVALUE::TYPE_MARK)) return false;
                memo.insert(nIndex, stack.last());
                break;
            }
            case 'h':    // BINGET
            case 'j': {  // LONG_BINGET
                quint32 nIndex = 0;
                if (nOpcode == 'h') {
                    if (nSize - nPos < 1) return false;
                    nIndex = pData[nPos++];
                } else {
                    if (nSize - nPos < 4) return false;
                    nIndex = qFromLittleEndian<quint32>(pData + nPos);
                    nPos += 4;
                }
                if (!memo.contains(nIndex)) return false;
                stack.append(memo.value(nIndex));
                break;
            }
            case 'a': {  // APPEND
                if (stack.size() < 2) return false;
                const PVALUE item = stack.last();
                stack.removeLast();
                if ((item.type == PVALUE::TYPE_MARK) || (stack.last().type != PVALUE::TYPE_LIST)) return false;
                stack.last().listItems.append(item);
                break;
            }
            case 'e': {  // APPENDS
                qint32 nMark = stack.size() - 1;
                while ((nMark >= 0) && (stack.at(nMark).type != PVALUE::TYPE_MARK)) nMark--;
                if ((nMark < 1) || (stack.at(nMark - 1).type != PVALUE::TYPE_LIST)) return false;
                QList<PVALUE> listItems;
                for (qint32 i = nMark + 1; i < stack.size(); i++) listItems.append(stack.at(i));
                while (stack.size() > nMark) stack.removeLast();
                stack.last().listItems.append(listItems);
                break;
            }
            case 's': {  // SETITEM
                if (stack.size() < 3) return false;
                const PVALUE value = stack.last();
                stack.removeLast();
                const PVALUE key = stack.last();
                stack.removeLast();
                if ((key.type == PVALUE::TYPE_MARK) || (value.type == PVALUE::TYPE_MARK) || (stack.last().type != PVALUE::TYPE_DICT)) return false;
                stack.last().listKeys.append(key);
                stack.last().listItems.append(value);
                break;
            }
            case 'u': {  // SETITEMS
                qint32 nMark = stack.size() - 1;
                while ((nMark >= 0) && (stack.at(nMark).type != PVALUE::TYPE_MARK)) nMark--;
                if ((nMark < 1) || (stack.at(nMark - 1).type != PVALUE::TYPE_DICT)) return false;
                if (((stack.size() - nMark - 1) % 2) != 0) return false;
                QList<PVALUE> listKeys;
                QList<PVALUE> listValues;
                for (qint32 i = nMark + 1; i < stack.size(); i += 2) {
                    listKeys.append(stack.at(i));
                    listValues.append(stack.at(i + 1));
                }
                while (stack.size() > nMark) stack.removeLast();
                stack.last().listKeys.append(listKeys);
                stack.last().listItems.append(listValues);
                break;
            }
            case '.': {  // STOP
                if ((stack.size() != 1) || (stack.last().type == PVALUE::TYPE_MARK)) return false;
                *pResult = stack.last();
                return true;
            }
            default:
                // NONE, GLOBAL, REDUCE, FRAME, LONG4, ... are not what an
                // RPA index contains; refuse rather than guess.
                return false;
        }
    }
    return false;
}

// ---------------------------------------------------------------- index ---
bool XRenpyRpa::buildMembers(const PVALUE &index, const HEADER &header, qint64 nTotalSize, QList<MEMBER> *pMembers)
{
    if (index.type != PVALUE::TYPE_DICT) return false;
    const qint32 nCount = index.listKeys.size();
    if ((nCount < 1) || (nCount > MaxMembers) || (index.listItems.size() != nCount)) return false;
    const bool bKeyed = header.sVersion != QLatin1String("2.0");
    QList<MEMBER> listMembers;
    for (qint32 i = 0; i < nCount; i++) {
        const PVALUE &key = index.listKeys.at(i);
        const PVALUE &entries = index.listItems.at(i);
        if ((key.type != PVALUE::TYPE_TEXT) && (key.type != PVALUE::TYPE_BYTES)) return false;
        if ((entries.type != PVALUE::TYPE_LIST) && (entries.type != PVALUE::TYPE_TUPLE)) return false;
        MEMBER member;
        member.sName = QString::fromUtf8(key.baData);
        if (!isMemberNameSafe(member.sName)) return false;
        const qint32 nSegments = entries.listItems.size();
        if (nSegments < 1) return false;
        for (qint32 j = 0; j < nSegments; j++) {
            const PVALUE &entry = entries.listItems.at(j);
            if ((entry.type != PVALUE::TYPE_TUPLE) || (entry.listItems.size() < 2) || (entry.listItems.size() > 3)) return false;
            if ((entry.listItems.at(0).type != PVALUE::TYPE_INT) || (entry.listItems.at(1).type != PVALUE::TYPE_INT)) return false;
            SEGMENT segment;
            qint64 nOffset = entry.listItems.at(0).nInt;
            qint64 nLength = entry.listItems.at(1).nInt;
            if (bKeyed) {
                nOffset ^= header.nKey;
                nLength ^= header.nKey;
            }
            if (entry.listItems.size() == 3) {
                const PVALUE &prefix = entry.listItems.at(2);
                if (prefix.type == PVALUE::TYPE_BYTES) {
                    segment.baPrefix = prefix.baData;
                } else if ((prefix.type == PVALUE::TYPE_TEXT) && prefix.baData.isEmpty()) {
                    // an empty str prefix carries nothing
                } else {
                    // A non-empty str prefix has no verified byte mapping.
                    return false;
                }
            }
            segment.nDataOffset = nOffset;
            segment.nDataSize = nLength - segment.baPrefix.size();
            if ((nOffset < header.nHeaderSize) || (segment.nDataSize < 0) || (nOffset > header.nIndexOffset) ||
                (segment.nDataSize > header.nIndexOffset - nOffset)) {
                return false;
            }
            member.nSize += nLength;
            member.listSegments.append(segment);
        }
        if (member.nSize > MaxOutputSize) return false;
        listMembers.append(member);
    }
    Q_UNUSED(nTotalSize)
    *pMembers = listMembers;
    return true;
}

bool XRenpyRpa::inflateIndex(const HEADER &header, qint64 nTotalSize, QByteArray *pIndex, PDSTRUCT *pPdStruct)
{
    QIODevice *source = getDevice();
    const qint64 nCompressed = nTotalSize - header.nIndexOffset;
    if (!source || (nCompressed < 2) || (nCompressed > MaxCompressedIndex)) return false;
    QByteArray baIndex;
    QBuffer bufferOut(&baIndex);
    // The strict zlib decoder verifies the Adler-32 by reading the completed
    // output, so the buffer must be readable as well as writable.
    if (!bufferOut.open(QIODevice::ReadWrite)) return false;
    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = source;
    state.pDeviceOutput = &bufferOut;
    state.nInputOffset = header.nIndexOffset;
    state.nInputLimit = nCompressed;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = MaxIndexSize;
    const bool bDecoded = XDeflateDecoder::decompress_zlib(&state, pPdStruct);
    bufferOut.close();
    if (!bDecoded || state.bReadError || state.bWriteError || baIndex.isEmpty() || (baIndex.size() >= MaxIndexSize)) return false;
    *pIndex = baIndex;
    return true;
}

bool XRenpyRpa::readContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QIODevice *source = getDevice();
    if (!pContext || !source || !source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const qint64 nTotalSize = getSize();
    if (!source || (nTotalSize < 32)) return false;
    const QByteArray baHead = read_array_process(0, qMin<qint64>(MaxHeaderLine, nTotalSize), pPdStruct);
    if (baHead.isEmpty()) return false;
    CONTEXT parsed;
    if (!parseHeader(baHead, nTotalSize, &parsed.header)) return false;
    QByteArray baIndex;
    try {
        if (!inflateIndex(parsed.header, nTotalSize, &baIndex, pPdStruct)) return false;
        PVALUE index;
        if (!unpickle(baIndex, &index)) return false;
        if (!buildMembers(index, parsed.header, nTotalSize, &parsed.listMembers)) return false;
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    parsed.nArchiveEnd = nTotalSize;
    *pContext = parsed;
    return true;
}

// -------------------------------------------------------------- unpack ---
bool XRenpyRpa::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *pOld = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    delete pOld;
    *pState = UNPACK_STATE();
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!bBound) return false;
    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    OUTPUT_POLICY policy = {};
    XRenpyRpa *owner = this;
    const bool bValid = pContext && resolveUnpackOutputPolicy(mapProperties, &policy) && readContext(pContext, pPdStruct);
    if (!owner) {
        delete pContext;
        return false;
    }
    if (!bValid) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    pState->pContext = pContext;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->nCurrentIndex = 0;
    pState->nTotalSize = pContext->nArchiveEnd;
    pState->mapUnpackProperties = mapProperties;
    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XRenpyRpa::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct)) return record;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint32 nCount = pContext->listMembers.size();
    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount) || (pState->nNumberOfRecords != nCount)) return record;
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sName);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QString("Store (RPA-%1)").arg(pContext->header.sVersion));
    if (!markArchiveStreamRecord(&record, pState->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XRenpyRpa::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QIODevice *source = getDevice();
    QIODevice *output = pDevice;
    XRenpyRpa *owner = this;
    if (!guard.isAcquired() || !pState || !pState->pContext || !source || !output || !isUnpackSourceCurrent(pState, pPdStruct) || !source ||
        !output) {
        return false;
    }
    const bool bSupported = isUnpackOutputSupported(output);
    if (!source || !output || !bSupported) return false;
    const bool bAliases = devicesAlias(source, output);
    if (!source || !output || bAliases) return false;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint32 nCount = pContext->listMembers.size();
    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount) || (pState->nNumberOfRecords != nCount)) return false;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    const QMap<UNPACK_PROP, QVariant> mapProperties = pState->mapUnpackProperties;
    const QSharedPointer<OUTPUT_BUDGET> spBudget = pState->spOutputBudget;
    if ((member.nSize < 0) || (member.nSize > MaxOutputSize)) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy) || ((policy.nMaxEntryOutputSize >= 0) && (member.nSize > policy.nMaxEntryOutputSize))) {
        return false;
    }
    if (spBudget && !spBudget->beginEntry(pState->nCurrentIndex, member.sName)) {
        if (spBudget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(spBudget.data());
    }
    if (spBudget && spBudget->isEnforcing() && (spBudget->totalLimit() >= 0) &&
        ((spBudget->totalWritten() > spBudget->totalLimit()) || (member.nSize > spBudget->totalLimit() - spBudget->totalWritten()))) {
        return false;
    }

    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writer = {};
    writer.pDeviceOutput = &stage;
    writer.nProcessedLimit = -1;
    writer.mapUnpackProperties = mapProperties;
    writer.spOutputBudget = spBudget;
    const RPA_CANCELED canceled(owner, source, output, pPdStruct);
    try {
        const qint32 nSegments = member.listSegments.size();
        for (qint32 i = 0; i < nSegments; i++) {
            const SEGMENT &segment = member.listSegments.at(i);
            const qint32 nPrefixSize = segment.baPrefix.size();
            if (nPrefixSize > 0) {
                if (canceled() || (XBinary::_writeDevice(segment.baPrefix.constData(), nPrefixSize, &writer) != nPrefixSize)) return false;
            }
            for (qint64 nDone = 0; nDone < segment.nDataSize;) {
                const qint32 nTake = (qint32)qMin<qint64>(ChunkSize, segment.nDataSize - nDone);
                const QByteArray baChunk = read_array_process(segment.nDataOffset + nDone, nTake, pPdStruct);
                if (canceled() || (baChunk.size() != nTake)) return false;
                if (XBinary::_writeDevice(baChunk.constData(), nTake, &writer) != nTake) return false;
                nDone += nTake;
            }
        }
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (canceled() || (stage.size() != member.nSize) || !stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(pState, pPdStruct) || canceled()) {
        return false;
    }
    const bool bPublished = publishUnpackOutput(&stage, output, pState, pPdStruct);
    return output && bPublished;
}

bool XRenpyRpa::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    const qint64 nCount = static_cast<CONTEXT *>(pState->pContext)->listMembers.size();
    if ((pState->nNumberOfRecords != nCount) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount)) return false;
    ++pState->nCurrentIndex;
    return pState->nCurrentIndex < nCount;
}

bool XRenpyRpa::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
