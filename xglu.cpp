/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xglu.h"

#include "Algos/xrawlzw15vdecoder.h"

#include <QPointer>
#include <QVector>

#include <new>

namespace {
// Shortest imaginable container: a one-character name, its NUL, and a code
// stream that carries at least one literal plus the END code.
const qint64 GLU_MIN_FILE_SIZE = 8;

// Detection has to decode the whole file (there is no header at all), so the
// input is capped instead of the decode being sampled.  The reference corpus
// tops out at 86 KB.
const qint64 GLU_MAX_FILE_SIZE = Q_INT64_C(16) * 1024 * 1024;

// LZW may legitimately expand a lot, but a "valid" member running into the
// hundreds of megabytes is a runaway, not a file.
const qint64 GLU_MAX_MEMBER_OUTPUT = Q_INT64_C(64) * 1024 * 1024;

// A DOS-era name field; the whole corpus is plain 8.3.
const qint32 GLU_MAX_NAME_LENGTH = 12;

const qint32 GLU_MAX_MEMBERS = 65536;

bool gluIsValidNameByte(quint8 nCharacter)
{
    if ((nCharacter < 0x21) || (nCharacter > 0x7e)) return false;
    return (nCharacter != '/') && (nCharacter != '\\') && (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') &&
           (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|') && (nCharacter != '%');
}

bool gluIsValidName(const QByteArray &baName)
{
    if ((baName.size() < 1) || (baName.size() > GLU_MAX_NAME_LENGTH)) return false;
    qint32 nDots = 0;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (!gluIsValidNameByte(nCharacter)) return false;
        if (nCharacter == '.') nDots++;
    }
    // A DOS name carries at most one extension separator, and never leads with
    // it; both rules cost nothing and cut a large slice of random byte strings.
    if (nDots > 1) return false;
    if (baName.at(0) == '.') return false;
    return true;
}

// Strict walk of one LZW member, byte-for-byte the grammar XRawLzw15vDecoder
// applies in probe() - the difference is that a GLU member is NOT the whole
// buffer, so instead of demanding that the input be fully consumed this
// reports where the stream ended.  The decoded bytes are counted, not kept:
// extraction goes back through HANDLE_METHOD_RAW_LZW15V with exactly this
// offset/size pair.
bool gluScanMember(const quint8 *pData, qint64 nSize, qint64 nStart, qint64 *pnEnd, qint64 *pnOutputSize)
{
    if (!pData || !pnEnd || !pnOutputSize || (nStart < 0) || (nStart >= nSize)) return false;

    QVector<quint16> vPrefix(static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES), 0);
    QVector<quint8> vAppend(static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES), 0);
    for (qint32 i = 0; i < 256; i++) vAppend[i] = static_cast<quint8>(i);

    qint64 nPosition = nStart;
    quint8 nCurrentByte = 0;
    qint32 nBitsLeft = 0;

    // MSB-first bit pump; a byte is taken whole and drained from bit 7 down.
    struct Reader {
        const quint8 *pData;
        qint64 nSize;
        qint64 *pnPosition;
        quint8 *pnCurrent;
        qint32 *pnBitsLeft;
        bool read(qint32 nBits, quint32 *pnValue)
        {
            quint32 nValue = 0;
            for (qint32 i = 0; i < nBits; i++) {
                if (*pnBitsLeft == 0) {
                    if (*pnPosition >= nSize) return false;
                    *pnCurrent = pData[*pnPosition];
                    (*pnPosition)++;
                    *pnBitsLeft = 8;
                }
                nValue = (nValue << 1) | static_cast<quint32>((*pnCurrent >> 7) & 0x01U);
                *pnCurrent = static_cast<quint8>((*pnCurrent << 1) & 0xFFU);
                (*pnBitsLeft)--;
            }
            *pnValue = nValue;
            return true;
        }
    } reader = {pData, nSize, &nPosition, &nCurrentByte, &nBitsLeft};

    qint64 nProduced = 0;
    bool bEndSeen = false;

    while (!bEndSeen) {
        quint32 nCode = 0;
        if (!reader.read(XRawLzw15vDecoder::MIN_CODE_BITS, &nCode)) return false;
        if (nCode == static_cast<quint32>(XRawLzw15vDecoder::CODE_END)) {
            bEndSeen = true;
            break;
        }
        if (nCode > 0xFFU) return false;
        if (nProduced >= GLU_MAX_MEMBER_OUTPUT) return false;
        nProduced++;

        quint32 nNextCode = static_cast<quint32>(XRawLzw15vDecoder::FIRST_CODE);
        qint32 nCodeBits = XRawLzw15vDecoder::MIN_CODE_BITS;
        quint32 nPreviousCode = nCode;
        quint32 nCharacter = nCode;
        bool bClear = false;

        while (!bClear) {
            quint32 nNew = 0;
            if (!reader.read(nCodeBits, &nNew)) return false;
            if (nNew == static_cast<quint32>(XRawLzw15vDecoder::CODE_END)) {
                bEndSeen = true;
                break;
            }
            if (nNew == static_cast<quint32>(XRawLzw15vDecoder::CODE_BUMP)) {
                if (nCodeBits >= XRawLzw15vDecoder::MAX_CODE_BITS) return false;
                nCodeBits++;
                continue;
            }
            if (nNew == static_cast<quint32>(XRawLzw15vDecoder::CODE_CLEAR)) {
                bClear = true;
                break;
            }
            if (nNew > nNextCode) return false;
            if (nNextCode > (1U << nCodeBits)) return false;

            qint32 nStackSize = 0;
            quint32 nWalk = 0;
            if (nNew >= nNextCode) {
                nStackSize++;  // the KwKwK byte
                nWalk = nPreviousCode;
            } else {
                nWalk = nNew;
            }
            while (nWalk > 0xFFU) {
                if (nWalk >= static_cast<quint32>(XRawLzw15vDecoder::MAX_CODES)) return false;
                if (nStackSize >= static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES)) return false;
                nStackSize++;
                nWalk = vPrefix[static_cast<qint32>(nWalk)];
            }
            if (nStackSize >= static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES)) return false;
            nStackSize++;
            nCharacter = nWalk;

            if (nProduced > (GLU_MAX_MEMBER_OUTPUT - static_cast<qint64>(nStackSize))) return false;
            nProduced += nStackSize;

            if (nNextCode < static_cast<quint32>(XRawLzw15vDecoder::MAX_CODES)) {
                vPrefix[static_cast<qint32>(nNextCode)] = static_cast<quint16>(nPreviousCode);
                vAppend[static_cast<qint32>(nNextCode)] = static_cast<quint8>(nCharacter & 0xFFU);
                nNextCode++;
            }
            nPreviousCode = nNew;
        }
    }

    if (!bEndSeen || (nProduced <= 0)) return false;

    *pnEnd = nPosition;
    *pnOutputSize = nProduced;
    return true;
}
}  // namespace

XGLU::XGLU(QIODevice *pDevice)
    : XArchive(pDevice), m_bContextCached(false), m_bContextValid(false), m_context(), m_pCachedDevice(nullptr), m_nCachedSize(-1)
{
}

XGLU::~XGLU()
{
}

bool XGLU::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XGLU> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((nInputSize < GLU_MIN_FILE_SIZE) || (nInputSize > GLU_MAX_FILE_SIZE)) return false;

    // Cache key only.  A legitimate container can be shorter than 32 bytes (a
    // one-member archive with a short name and a tiny code stream), so the read
    // is clamped instead of demanding a full 32 bytes.
    const qint64 nPrefixSize = qMin<qint64>(32, nInputSize);
    const QByteArray baPrefix = read_array_process(0, nPrefixSize, pPdStruct);
    if (!guardedThis || !guardedSource || (static_cast<qint64>(baPrefix.size()) != nPrefixSize)) return false;

    if (m_bContextCached && (m_pCachedDevice == guardedSource.data()) && (m_nCachedSize == nInputSize) && (m_baCachedPrefix == baPrefix)) {
        if (!m_bContextValid) return false;
        *pContext = m_context;
        return isPdStructNotCanceled(pPdStruct);
    }

    m_bContextCached = true;
    m_bContextValid = false;
    m_context = CONTEXT();
    m_pCachedDevice = guardedSource.data();
    m_nCachedSize = nInputSize;
    m_baCachedPrefix = baPrefix;

    const QByteArray baFile = read_array_process(0, nInputSize, pPdStruct);
    if (!guardedThis || !guardedSource || (static_cast<qint64>(baFile.size()) != nInputSize)) return false;
    const quint8 *pData = reinterpret_cast<const quint8 *>(baFile.constData());

    CONTEXT context = {};
    context.nInputSize = nInputSize;

    qint64 nOffset = 0;
    while (nOffset < nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= GLU_MAX_MEMBERS) return false;

        const qint64 nNameOffset = nOffset;
        qint64 nNameEnd = nOffset;
        while ((nNameEnd < nInputSize) && (pData[nNameEnd] != 0)) nNameEnd++;
        if (nNameEnd >= nInputSize) return false;  // unterminated name

        const QByteArray baName = baFile.mid(static_cast<qint32>(nNameOffset), static_cast<qint32>(nNameEnd - nNameOffset));
        nOffset = nNameEnd + 1;
        // The reference reader skips empty name strings before it starts a
        // member; nothing in the corpus produces one, but keeping the rule
        // costs nothing and cannot loop (nOffset always advances).
        if (baName.isEmpty()) continue;
        if (!gluIsValidName(baName)) return false;

        qint64 nEnd = 0;
        qint64 nOutputSize = 0;
        if (!gluScanMember(pData, nInputSize, nOffset, &nEnd, &nOutputSize)) return false;

        MEMBER member = {};
        member.nNameOffset = nNameOffset;
        member.nDataOffset = nOffset;
        member.nCompressedSize = nEnd - nOffset;
        member.nUncompressedSize = nOutputSize;
        member.sFileName = QString::fromLatin1(baName);
        context.listMembers.append(member);

        nOffset = nEnd;
    }

    // The chain must consume the container exactly: the last END code lands on
    // the last byte, with no trailing slack.  Without a magic number this is the
    // detector.
    if (context.listMembers.isEmpty() || (nOffset != nInputSize)) return false;

    context.nArchiveSize = nInputSize;
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    m_bContextValid = true;
    m_context = context;
    *pContext = context;
    return guardedThis && guardedSource;
}

bool XGLU::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XGLU::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGLU archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGLU::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGLU(pDevice);
}

XBinary::FT XGLU::getFileType()
{
    return FT_GLU;
}

XBinary::MODE XGLU::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGLU::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGLU::getArch()
{
    return QString();
}

QString XGLU::getFileFormatExt()
{
    return QStringLiteral("glu");
}

QString XGLU::getFileFormatExtsString()
{
    return QStringLiteral("GLU network distribution file (*.glu)");
}

QString XGLU::getMIMEString()
{
    return QStringLiteral("application/x-glu");
}

QString XGLU::getVersion()
{
    return QString();
}

qint64 XGLU::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGLU::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGLU::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XGLU::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGLU::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nNameOffset;
            part.nFileSize = member.nDataOffset - member.nNameOffset;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_RAW_LZW15V);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZW15V"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nNameOffset;
            part.nFileSize = (member.nDataOffset - member.nNameOffset) + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XGLU::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGLU::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XGLU> guardedThis(this);
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
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("GLU network file"));
    pState->nCurrentOffset = pContext->listMembers.first().nNameOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

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

XBinary::ARCHIVERECORD XGLU::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nNameOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_RAW_LZW15V);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZW15V"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XGLU::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nNameOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XGLU::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
