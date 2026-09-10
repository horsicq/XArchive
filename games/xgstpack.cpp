/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgstpack.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

#include "Algos/xdcldecoder.h"

namespace {
// Member header: magic, DOS time/date, attributes, method, sizes, name, CRC.
const qint64 GST_HEADER_SIZE = 32;
const qint32 GST_NAME_OFFSET = 16;
// The name field is twelve bytes wide.  A name that fills all twelve of them
// has no terminator inside the field, so the field must be read at its full
// width - reading eleven would truncate such a name.
const qint32 GST_NAME_FIELD = 12;
const qint32 GST_NAME_MAX = 12;

const quint8 GST_MAGIC_0 = 0xE9U;
const quint8 GST_MAGIC_1 = 0xC8U;

const quint8 GST_METHOD_STORE = 0U;
const quint8 GST_METHOD_DCL = 1U;

// Read-only | hidden | system | archive.  Every corpus member is 0x00, 0x01
// or 0x20; directory and volume-label bits never appear and would mean the
// walk had wandered into payload bytes.
const quint8 GST_ATTRIBUTE_MASK = 0x27U;

const qint64 GST_MAX_MEMBERS = 100000;
const qint64 GST_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap

// A PKWARE DCL Implode stream carries its own prelude: literal mode (0 or 1)
// then dictionary-size bits (4..6).  Every corpus member uses 0/6.
const qint64 GST_MIN_DCL_PAYLOAD = 3;
const quint8 GST_DCL_MAX_LITERAL_MODE = 1U;
const quint8 GST_DCL_MIN_DICT_BITS = 4U;
const quint8 GST_DCL_MAX_DICT_BITS = 6U;

// Upper bound on the payload the acceptance test is willing to explode.  The
// largest corpus member is ~292 KB, so this never skips a real archive; it
// only stops a hostile input from turning file-type detection into a long
// decompression.
const qint64 GST_TRIAL_MAX_PACKED = 8 * 1024 * 1024;

bool gstRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Fixed-width NUL-padded field: the name is everything up to the first
// terminator, and the bytes after it are stale buffer content that must be
// ignored (several corpus headers carry visible garbage there, e.g.
// "HYUS.TXT\0.EN").
QByteArray gstFixedName(const QByteArray &baField)
{
    const qint32 nIndex = baField.indexOf('\0');
    return (nIndex < 0) ? baField : baField.left(nIndex);
}

bool gstIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || baName.size() > GST_NAME_MAX) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
        if ((c == '/') || (c == '\\') || (c == ':') || (c == '*') ||
            (c == '?') || (c == '"') || (c == '<') || (c == '>') ||
            (c == '|')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XGstPack::XGstPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGstPack::~XGstPack()
{
}

bool XGstPack::parseContext(CONTEXT *pContext, bool bDeepCheck,
                            PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XGstPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < GST_HEADER_SIZE + GST_MIN_DCL_PAYLOAD) {
        return false;
    }

    qint64 nOffset = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        if (nOffset == context.nInputSize) break;
        if (!gstRangeWithin(context.nInputSize, nOffset, GST_HEADER_SIZE)) {
            return false;
        }
        if (context.listMembers.size() >= GST_MAX_MEMBERS) return false;

        const QByteArray baHeader =
            read_array_process(nOffset, GST_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baHeader.size() != GST_HEADER_SIZE) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        if ((pHeader[0] != GST_MAGIC_0) || (pHeader[1] != GST_MAGIC_1)) {
            return false;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + GST_HEADER_SIZE;
        member.nDosTime = qFromLittleEndian<quint16>(pHeader + 2);
        member.nDosDate = qFromLittleEndian<quint16>(pHeader + 4);
        member.nAttributes = pHeader[6];
        member.nMethod = pHeader[7];
        member.nUncompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 8));
        member.nCompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 12));
        member.nCRC32 = qFromLittleEndian<quint32>(pHeader + 28);

        if ((member.nMethod != GST_METHOD_STORE) &&
            (member.nMethod != GST_METHOD_DCL)) {
            return false;
        }
        if (member.nAttributes & ~GST_ATTRIBUTE_MASK) return false;
        if (member.nUncompressedSize > GST_MAX_UNCOMPRESSED_SIZE) return false;

        const QByteArray baName =
            gstFixedName(baHeader.mid(GST_NAME_OFFSET, GST_NAME_FIELD));
        if (!gstIsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName);

        if (member.nMethod == GST_METHOD_STORE) {
            // Stored payloads are copied verbatim; the two size fields must
            // agree or the record is not a stored member.
            if (member.nCompressedSize != member.nUncompressedSize) {
                return false;
            }
        } else if (member.nCompressedSize < GST_MIN_DCL_PAYLOAD) {
            return false;
        }

        if (!gstRangeWithin(context.nInputSize, member.nDataOffset,
                            member.nCompressedSize)) {
            return false;
        }

        if (member.nMethod == GST_METHOD_DCL) {
            // Cross-check the DCL prelude.  Together with the chain landing
            // on EOF this is what keeps a stray 0xE9 0xC8 pair from being
            // accepted as an archive.
            const QByteArray baPrelude =
                read_array_process(member.nDataOffset, 2, pPdStruct);
            if (!guardedThis || !guardedSource || baPrelude.size() != 2) {
                return false;
            }
            if (static_cast<quint8>(baPrelude.at(0)) >
                GST_DCL_MAX_LITERAL_MODE) {
                return false;
            }
            const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
            if ((nDictBits < GST_DCL_MIN_DICT_BITS) ||
                (nDictBits > GST_DCL_MAX_DICT_BITS)) {
                return false;
            }
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (!isPdStructNotCanceled(pPdStruct)) return false;
    // The chain has no terminator record: it ends by landing exactly on EOF.
    if (nOffset != context.nInputSize) return false;
    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;

    if (bDeepCheck && !trialDecode(context, pPdStruct)) return false;
    if (!guardedThis || !guardedSource) return false;

    *pContext = context;
    return true;
}

bool XGstPack::trialDecode(const CONTEXT &context, PDSTRUCT *pPdStruct)
{
    QPointer<XGstPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint32 nCount = context.listMembers.size();
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const MEMBER &member = context.listMembers.at(i);
        if (member.nMethod != GST_METHOD_DCL) continue;
        if (member.nCompressedSize > GST_TRIAL_MAX_PACKED) continue;

        const QByteArray baPacked = read_array_process(
            member.nDataOffset, member.nCompressedSize, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baPacked.size() != member.nCompressedSize) {
            return false;
        }

        QByteArray baRaw;
        qint64 nConsumed = 0;
        if (!XDclDecoder::decode(baPacked, &baRaw, member.nUncompressedSize,
                                 &nConsumed)) {
            return false;
        }
        // The header sizes are exact, not hints: the explode must produce the
        // declared byte count and must have eaten the declared payload.
        if (static_cast<qint64>(baRaw.size()) != member.nUncompressedSize) {
            return false;
        }
        if (nConsumed != member.nCompressedSize) return false;
        // Verified against the whole corpus: the stored value is a plain
        // CRC-32 (EDB88320, 0xFFFFFFFF init and final xor) over the UNPACKED
        // member.  This is the check that makes a false positive essentially
        // impossible.
        const quint32 nCRC32 =
            _getCRC32(baRaw, 0xFFFFFFFF, _getCRC32Table_EDB88320()) ^
            0xFFFFFFFF;
        if (nCRC32 != member.nCRC32) return false;
        return true;
    }

    // Nothing explodable to test (every member stored, or every compressed
    // member above the trial cap).  The structural walk already required the
    // chain to land exactly on EOF with well-formed headers throughout.
    return true;
}

bool XGstPack::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XGstPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGstPack archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGstPack::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGstPack(pDevice);
}

QList<QString> XGstPack::getSearchSignatures()
{
    // 0xE9 0xC8, then the DOS time/date pair and the attribute byte as
    // wildcards, then the method byte pinned to its only two legal values.
    return {QStringLiteral("E9C8..........01"),
            QStringLiteral("E9C8..........00")};
}

XBinary::FT XGstPack::getFileType()
{
    return FT_GST_PACK;
}

XBinary::MODE XGstPack::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGstPack::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGstPack::getArch()
{
    return QString();
}

QString XGstPack::getFileFormatExt()
{
    return QStringLiteral("gst");
}

QString XGstPack::getFileFormatExtsString()
{
    return QStringLiteral("GST installation archive (*.gst *.__ *.___)");
}

QString XGstPack::getMIMEString()
{
    return QStringLiteral("application/x-gst-pack");
}

QString XGstPack::getVersion()
{
    // The container carries no version field of any kind.
    return QString();
}

qint64 XGstPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGstPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGstPack::getMemoryMap(MAPMODE mapMode,
                                            PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XGstPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XGstPack::getFileParts(quint32 nFileParts, qint32 nLimit,
                                             PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return result;

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = GST_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            if (member.nMethod == GST_METHOD_STORE) {
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                          HANDLE_METHOD_STORE);
                part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                          QStringLiteral("Store"));
            } else {
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                          HANDLE_METHOD_PKWARE_DCL_IMPLODE);
                part.mapProperties.insert(
                    FPART_PROP_REPORTEDMETHOD,
                    QStringLiteral("PKWARE DCL Implode"));
            }
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                      CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            part.mapProperties.insert(
                FPART_PROP_DATETIME,
                dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
            part.mapProperties.insert(FPART_PROP_FILEMODE,
                                      static_cast<quint32>(member.nAttributes));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = GST_HEADER_SIZE + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
        // parseContext() only accepts a chain that lands on EOF, so this
        // cannot fire today; it is kept so the part list stays correct if the
        // acceptance rule is ever relaxed.
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XGstPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGstPack::initUnpack(UNPACK_STATE *pState,
                          const QMap<UNPACK_PROP, QVariant> &mapProperties,
                          PDSTRUCT *pPdStruct)
{
    QPointer<XGstPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, false, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("GST installation archive; stored and PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XGstPack::infoCurrent(UNPACK_STATE *pState,
                                             PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    if (member.nMethod == GST_METHOD_STORE) {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                    HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                    QStringLiteral("Store"));
    } else {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                    HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                    QStringLiteral("PKWARE DCL Implode"));
    }
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    result.mapProperties.insert(
        FPART_PROP_DATETIME,
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
    result.mapProperties.insert(FPART_PROP_FILEMODE,
                                static_cast<quint32>(member.nAttributes));
    return result;
}

bool XGstPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XGstPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
