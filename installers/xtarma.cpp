/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xtarma.h"
#include "xmaterializedunpackguard.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <zlib.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QScopedValueRollback>
#include <QSet>
#include <QUuid>

#include "xpe.h"
#include "LzmaDec.h"  // vendored public-domain LZMA-SDK decoder

namespace {
const qint64 TARMA_MAX_CONTAINER_SIZE = Q_INT64_C(256) * 1024 * 1024;
const qint64 TARMA_MAX_INNER_SIZE = Q_INT64_C(256) * 1024 * 1024;
const quint64 TARMA_MAX_TOTAL_SOURCE_SIZE = Q_UINT64_C(512) * 1024 * 1024;
const quint64 TARMA_MAX_TOTAL_DECODED_SIZE = Q_UINT64_C(512) * 1024 * 1024;
const quint64 TARMA_MAX_TOTAL_PAYLOAD_SIZE = Q_UINT64_C(256) * 1024 * 1024;
const int TARMA_MAX_VOLUME_COUNT = 1024;
const int TARMA_MAX_BLOCK_COUNT = 8192;
const int TARMA_MAX_FILE_COUNT = 4096;
const int TARMA_MAX_DIRECTORY_ENTRIES = 100000;
}  // namespace

static bool tarmaRetainCompanionGuard(QList<XMaterializedUnpackGuard *> *pGuards, std::unique_ptr<XMaterializedUnpackGuard> *pGuard)
{
    if (!pGuards || !pGuard || !pGuard->get()) return false;
    try {
        pGuards->reserve(pGuards->size() + 1);
        pGuards->append(pGuard->get());
    } catch (const std::bad_alloc &) {
        return false;
    }
    pGuard->release();
    return true;
}

static bool tarmaFinalizeCompanionGuards(const QList<XMaterializedUnpackGuard *> &listGuards, XBinary::PDSTRUCT *pPdStruct)
{
    for (XMaterializedUnpackGuard *pGuard : listGuards) {
        if (!pGuard || !pGuard->validateAndFinalize(pPdStruct)) return false;
    }
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

XTarma::XTarma(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackLifetimeState = QSharedPointer<LIFETIME_STATE>::create();
    setIsArchive(true);
}

XTarma::UNPACK_CONTEXT::~UNPACK_CONTEXT()
{
    delete pSourceGuard;
    for (XMaterializedUnpackGuard *pGuard : listCompanionGuards) delete pGuard;
}

XTarma::~XTarma()
{
    QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (pLifetimeState) pLifetimeState->bOwnerAlive = false;
    m_pUnpackLifetimeState.clear();
    if (pLifetimeState && !pLifetimeState->bOperationInProgress) {
        const QSet<UNPACK_CONTEXT *> setContextsCopy = pLifetimeState->setContexts;
        pLifetimeState->setContexts.clear();
        for (UNPACK_CONTEXT *pContext : setContextsCopy) delete pContext;
    }
}

XTarma::LIFETIME_STATE::~LIFETIME_STATE()
{
    const QSet<UNPACK_CONTEXT *> setContextsCopy = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : setContextsCopy) delete pContext;
}

bool XTarma::isDeviceReplacementAllowed() const
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    return pLifetimeState && pLifetimeState->bOwnerAlive && !pLifetimeState->bOperationInProgress && pLifetimeState->setContexts.isEmpty();
}

bool XTarma::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XTarma> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XTarma::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTarma x(pDevice);
    return x.isValid(pPdStruct);
}

XTarma::INTERNAL_INFO XTarma::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XTarma::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTarma> guardedThis(this);
    const bool bAlreadyHandled = guardedThis->isInternalInfoHandled();
    if (!guardedThis) return false;

    if (!bAlreadyHandled) {
        const quint64 nTransaction = guardedThis->beginInternalInfoTransaction();
        if (!nTransaction) return false;

        // The transaction supplies the recursion sentinel. Keep every
        // source-derived value local until the same binding is revalidated.
        guardedThis->m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = guardedThis->_getInternalInfo(pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap = guardedThis->getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        guardedThis->m_internalInfo = info;
        if (!guardedThis->commitInternalInfoTransaction(nTransaction, static_cast<XBinary::INTERNAL_INFO *>(&guardedThis->m_internalInfo))) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XTarma::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTarma> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XTarma::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        setIsInternalInfoHandled(true);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        setIsInternalInfoHandled(false);
        XBinary::setInternalInfo(nullptr);
    }
}

XBinary::FT XTarma::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_TARMA;
    }

    return FT_PE32_TARMA;
}

// Match a PE section by exact name (8-byte COFF field, NUL-padded).
static qint64 tarmaSectionRaw(const QList<XPE_DEF::IMAGE_SECTION_HEADER> &listSections, const char *pszName)
{
    qint64 nResult = -1;
    for (int i = 0; i < listSections.size(); i++) {
        QByteArray baName((const char *)listSections.at(i).Name, 8);
        int nZero = baName.indexOf('\0');
        if (nZero >= 0) baName.truncate(nZero);
        if (baName == pszName) {
            if (nResult >= 0) return -2;  // duplicate authoritative section
            nResult = (qint64)listSections.at(i).PointerToRawData;
        }
    }
    return nResult;
}

XTarma::INTERNAL_INFO XTarma::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nContainerOffset = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections = pe.getSectionHeaders(pPdStruct);

    qint64 nStubRaw = tarmaSectionRaw(listSections, ".tsustub");
    qint64 nArchRaw = tarmaSectionRaw(listSections, ".tsuarch");
    if ((nStubRaw == -2) || (nArchRaw == -2)) return result;

    // Primary (v9): both .tsu* sections + the "tiz" container magic at raw+0x10.
    if ((nStubRaw >= 0) && (nArchRaw >= 0)) {
        QByteArray baMagic = read_array_process(nArchRaw + 0x10, 8, pPdStruct);
        if (baMagic.size() == 8) {
            const quint8 *p = (const quint8 *)baMagic.constData();
            // "tiz" <digit> 'z' 00 <version word 09 00>
            if ((p[0] == 0x74) && (p[1] == 0x69) && (p[2] == 0x7A) && (p[3] >= 0x30) && (p[3] <= 0x39) && (p[4] == 0x7A) && (p[5] == 0x00)) {
                result.bIsValid = true;
                quint16 nVer = (quint16)(p[6] | ((quint16)p[7] << 8));
                result.sVersion = QString::number(nVer);  // container version word (9)
                result.nContainerOffset = nArchRaw;
                result.bExtractable = (p[3] == 0x32) || (p[3] == 0x33);
                return result;
            }
        }
    }

    // Legacy: the payload sits in the overlay as "tiz1" + raw zlib (78 DA).
    qint64 nOverlayOffset = getOverlayOffset(pPdStruct);
    if ((nOverlayOffset > 0) && (nOverlayOffset < getSize())) {
        QByteArray baOv = read_array_process(nOverlayOffset, 10, pPdStruct);
        if (baOv.size() >= 10) {
            const quint8 *p = (const quint8 *)baOv.constData();
            if ((p[0] == 0x74) && (p[1] == 0x69) && (p[2] == 0x7A) && (p[3] == 0x31) && (p[8] == 0x78) && (p[9] == 0xDA)) {
                result.bIsValid = true;
                result.bIsLegacy = true;
                result.bExtractable = true;
                result.nContainerOffset = nOverlayOffset;
                return result;
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// extraction (tiz2z: zlib, tiz3z: LZMA1 -> "tzf3" block sequence)
// ---------------------------------------------------------------------------

static void *tarmaAlloc(ISzAllocPtr, size_t nSize)
{
    return malloc(nSize);
}
static void tarmaFree(ISzAllocPtr, void *p)
{
    free(p);
}

// Raw LZMA1 decode to the end-of-stream marker (output size not known in advance).
static bool tarmaLzmaToEnd(const quint8 *pProps, const quint8 *pSrc, qint64 nSrcSize, qint64 nMaxOutput, QByteArray *pOut, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pProps || !pSrc || !pOut || (nSrcSize <= 0) || (nSrcSize > TARMA_MAX_CONTAINER_SIZE) || (nMaxOutput < 0) || (nMaxOutput > TARMA_MAX_INNER_SIZE)) {
        return false;
    }

    static const ISzAlloc g_alloc = {tarmaAlloc, tarmaFree};

    CLzmaDec dec;
    LzmaDec_Construct(&dec);
    if (LzmaDec_Allocate(&dec, (const Byte *)pProps, 5, &g_alloc) != SZ_OK) return false;
    LzmaDec_Init(&dec);

    pOut->clear();
    const int nChunk = 1 << 16;
    QByteArray baChunk(nChunk, (char)0);

    qint64 nSrcPos = 0;
    bool bOk = false;
    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;

        SizeT nDestLen = nChunk;
        SizeT nSrcRemain = (SizeT)(nSrcSize - nSrcPos);
        ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
        SRes res = LzmaDec_DecodeToBuf(&dec, (Byte *)baChunk.data(), &nDestLen, (const Byte *)pSrc + nSrcPos, &nSrcRemain, LZMA_FINISH_ANY, &status);
        if (res != SZ_OK) break;

        if ((qint64)nDestLen > nMaxOutput - pOut->size()) break;
        pOut->append(baChunk.constData(), (int)nDestLen);
        nSrcPos += (qint64)nSrcRemain;

        if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
            bOk = true;
            break;
        }
        if ((nDestLen == 0) && (nSrcRemain == 0)) break;  // no progress -> give up
    }

    LzmaDec_Free(&dec, &g_alloc);
    if (!bOk) return false;

    // .tsuarch is raw-section padded with zeroes. Authenticate all bytes after
    // the LZMA end marker so a valid decoded prefix cannot hide appended
    // corruption inside the declared container.
    for (qint64 i = nSrcPos; i < nSrcSize; i++) {
        if (((i & 0xFFFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (pSrc[i] != 0) return false;
    }
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

static inline quint32 tarmaRd32(const quint8 *p)
{
    return (quint32)(p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24));
}

static inline quint64 tarmaRd64(const quint8 *p)
{
    return (quint64)tarmaRd32(p) | ((quint64)tarmaRd32(p + 4) << 32);
}

static bool tarmaInflate(const quint8 *pSrc, qint64 nSrcSize, qint64 nMaxOutput, QByteArray *pOut, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSrc || !pOut || (nSrcSize <= 0) || (nSrcSize > TARMA_MAX_CONTAINER_SIZE) || (nMaxOutput < 0) || (nMaxOutput > TARMA_MAX_INNER_SIZE)) {
        return false;
    }
    static const uInt TARMA_INFLATE_BUFFER_SIZE = 65536;
    QScopedArrayPointer<char> pBuffer(new (std::nothrow) char[TARMA_INFLATE_BUFFER_SIZE]);
    if (pBuffer.isNull()) return false;

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, 15) != Z_OK) return false;

    stream.next_in = (Bytef *)pSrc;
    stream.avail_in = (uInt)qMin(nSrcSize, (qint64)0x7FFFFFFF);
    pOut->clear();

    bool bOk = false;
    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;

        stream.next_out = reinterpret_cast<Bytef *>(pBuffer.data());
        stream.avail_out = TARMA_INFLATE_BUFFER_SIZE;
        int nResult = inflate(&stream, Z_NO_FLUSH);
        qint64 nProduced = TARMA_INFLATE_BUFFER_SIZE - stream.avail_out;
        if (nProduced > nMaxOutput - pOut->size()) break;
        pOut->append(pBuffer.data(), (int)nProduced);
        if (nResult == Z_STREAM_END) {
            bOk = true;
            break;
        }
        if (nResult != Z_OK) break;
        if ((stream.avail_in == 0) && (stream.avail_out == TARMA_INFLATE_BUFFER_SIZE)) break;
    }

    qint64 nConsumed = (qint64)stream.total_in;
    inflateEnd(&stream);
    if (!bOk || (nConsumed < 0) || (nConsumed > nSrcSize)) return false;

    for (qint64 i = nConsumed; i < nSrcSize; i++) {
        if (((i & 0xFFFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (pSrc[i] != 0) return false;
    }
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

static bool tarmaDecodeContainer(const QByteArray &baContainer, qint64 nMaxOutput, QByteArray *pInner, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pInner || (baContainer.size() <= 0) || (baContainer.size() > TARMA_MAX_CONTAINER_SIZE) || (nMaxOutput < 0) || (nMaxOutput > TARMA_MAX_INNER_SIZE) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const quint8 *p = (const quint8 *)baContainer.constData();
    const qint64 n = baContainer.size();

    if ((n >= 10) && (memcmp(p, "tiz1", 4) == 0) && (p[8] == 0x78)) {
        return tarmaInflate(p + 8, n - 8, nMaxOutput, pInner, pPdStruct);
    }

    if ((n < 0x4D) || (memcmp(p + 0x10, "tiz", 3) != 0) || (p[0x14] != 'z')) return false;
    if (p[0x13] == '2') {
        // "Deflate (fast)" in InstallMate's compressor selector. The eight
        // bytes at +0x40 are framing/checksum data.
        return tarmaInflate(p + 0x48, n - 0x48, nMaxOutput, pInner, pPdStruct);
    }
    if (p[0x13] == '3') {
        return tarmaLzmaToEnd(p + 0x48, p + 0x4D, n - 0x4D, nMaxOutput, pInner, pPdStruct);
    }

    return false;
}

struct TARMA_BLOCK {
    QByteArray baData;
    bool bUsed;
};

struct TARMA_NAME {
    QString sName;
    QString sLoosePath;
    quint64 nSize;
};

struct TARMA_FOLDER {
    QByteArray baParentId;
    QString sPhysicalName;
};

static QList<TARMA_BLOCK> tarmaReadBlocks(const QByteArray &baInner, XBinary::PDSTRUCT *pPdStruct)
{
    QList<TARMA_BLOCK> result;
    const quint8 *p = (const quint8 *)baInner.constData();
    const qint64 n = baInner.size();

    // The DWORD at +4 varies by compressor and is not a reliable file type.
    // The framing and bounded payload size are the authoritative block test.
    for (qint64 i = 0; i + 64 <= n;) {
        if (((i & 0xFFFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            result.clear();
            return result;
        }

        if (memcmp(p + i, "tzf3", 4) == 0) {
            quint32 nSize = tarmaRd32(p + i + 16);
            qint64 nData = i + 64;
            if ((qint64)nSize <= n - nData) {
                if (result.size() >= TARMA_MAX_BLOCK_COUNT) {
                    result.clear();
                    return result;
                }
                TARMA_BLOCK block;
                block.baData = QByteArray((const char *)p + nData, (int)nSize);
                block.bUsed = false;
                result.append(block);
                i = nData + nSize;
                continue;
            }
        }
        i++;
    }
    return result;
}

static bool tarmaReadDbString(const quint8 *p, qint64 nSize, qint64 nLengthOffset, QByteArray *pValue)
{
    if ((nLengthOffset < 0) || (nLengthOffset + 4 > nSize)) return false;

    quint32 nLength = tarmaRd32(p + nLengthOffset);
    qint64 nValueOffset = nLengthOffset + 4;
    if ((nLength == 0) || (nLength > 260) || ((qint64)nLength + 4 > nSize - nValueOffset)) return false;

    // tin9 stores display strings in the local 8-bit encoding. Preserve
    // high-bit bytes for fromLocal8Bit(), but reject embedded C0/DEL controls
    // (including NUL) before interpreting the field.
    for (quint32 i = 0; i < nLength; i++) {
        if ((p[nValueOffset + i] < 0x20) || (p[nValueOffset + i] == 0x7F)) return false;
    }
    if (tarmaRd32(p + nValueOffset + nLength) != 0) return false;

    *pValue = QByteArray((const char *)p + nValueOffset, (int)nLength);
    return true;
}

static bool tarmaIsAsciiIdentifier(const QByteArray &baValue)
{
    for (int i = 0; i < baValue.size(); i++) {
        quint8 nByte = (quint8)baValue.at(i);
        if ((nByte < 0x20) || (nByte >= 0x7F)) return false;
    }
    return !baValue.isEmpty();
}

static bool tarmaIsSafePathPart(const QString &sValue)
{
    if (sValue.isEmpty() || (sValue.size() > 255) || (sValue == ".") || (sValue == "..")) return false;
    static const QString g_sInvalidPathChars = "<>:\"/\\|?*";
    for (int i = 0; i < sValue.size(); i++) {
        if (!sValue.at(i).isPrint() || (sValue.at(i) == QChar::ReplacementCharacter) || g_sInvalidPathChars.contains(sValue.at(i))) return false;
    }
    if (sValue.endsWith('.') || sValue.endsWith(' ')) return false;

    static const QSet<QString> setReserved = {
        QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),    QStringLiteral("NUL"),     QStringLiteral("COM1"),
        QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),   QStringLiteral("COM5"),    QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),   QStringLiteral("LPT1"),    QStringLiteral("LPT2"),
        QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),   QStringLiteral("LPT6"),    QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9"), QStringLiteral("CONIN$"), QStringLiteral("CONOUT$"), QStringLiteral("CLOCK$")};
    QString sStem = sValue.section('.', 0, 0).toUpper();
    sStem.replace(QChar(0x00B9), QLatin1Char('1'));
    sStem.replace(QChar(0x00B2), QLatin1Char('2'));
    sStem.replace(QChar(0x00B3), QLatin1Char('3'));
    return !setReserved.contains(sStem);
}

static QString tarmaNameKey(const QString &sName)
{
    return sName.toCaseFolded().normalized(QString::NormalizationForm_C);
}

static QString tarmaUniqueOutputName(const QString &sPreferred, qint32 nRecordIndex, const QSet<QString> &setDeclaredNames, QSet<QString> *pSetAssignedNames)
{
    if (!pSetAssignedNames || sPreferred.isEmpty() || (nRecordIndex < 0)) return QString();

    const QString sPreferredKey = tarmaNameKey(sPreferred);
    if (!pSetAssignedNames->contains(sPreferredKey)) {
        pSetAssignedNames->insert(sPreferredKey);
        return sPreferred;
    }

    const QString sBase = QString("file_%1").arg(nRecordIndex, 4, 10, QChar('0'));
    for (qint32 i = 0; i <= TARMA_MAX_FILE_COUNT; i++) {
        const QString sCandidate = (i == 0) ? sBase : QString("%1_%2").arg(sBase).arg(i);
        const QString sCandidateKey = tarmaNameKey(sCandidate);
        if (!setDeclaredNames.contains(sCandidateKey) && !pSetAssignedNames->contains(sCandidateKey)) {
            pSetAssignedNames->insert(sCandidateKey);
            return sCandidate;
        }
    }

    return QString();
}

static QString tarmaBuildLoosePath(const QByteArray &baFolderId, const QString &sName, const QHash<QByteArray, TARMA_FOLDER> &mapFolders)
{
    if ((baFolderId.size() != 8) || !tarmaIsSafePathPart(sName)) return QString();

    QStringList listParts;
    QList<QByteArray> listVisited;
    QByteArray baCurrentId = baFolderId;
    const QByteArray baNullId(8, 0);

    while (baCurrentId != baNullId) {
        if ((listVisited.size() >= 64) || listVisited.contains(baCurrentId) || !mapFolders.contains(baCurrentId)) return QString();
        listVisited.append(baCurrentId);

        const TARMA_FOLDER &folder = mapFolders.value(baCurrentId);
        if (!tarmaIsSafePathPart(folder.sPhysicalName)) return QString();
        listParts.prepend(folder.sPhysicalName);
        baCurrentId = folder.baParentId;
    }

    // A null folder id denotes the installer directory itself. The caller
    // still canonicalizes the resolved companion and proves that it remains
    // inside that directory before opening it.
    if (listParts.isEmpty()) return sName;
    listParts.append(sName);
    return listParts.join('/');
}

static bool tarmaPathHasLink(const QString &sBasePath, const QString &sRelativePath)
{
    QString sCurrent = sBasePath;
    const QStringList listParts = QDir::fromNativeSeparators(sRelativePath).split('/', Qt::SkipEmptyParts);
    for (int i = 0; i < listParts.size(); i++) {
        sCurrent = QDir(sCurrent).absoluteFilePath(listParts.at(i));
        QFileInfo info(sCurrent);
        if (info.isSymLink()) return true;
    }
    return false;
}

static bool tarmaReadFileBounded(QFile *pFile, qint64 nExpectedSize, QByteArray *pData, XBinary::PDSTRUCT *pPdStruct)
{
    QPointer<QFile> guardedFile(pFile);
    if (!guardedFile || !pData || !guardedFile->isOpen() || !guardedFile || (nExpectedSize < 0) || (nExpectedSize > TARMA_MAX_CONTAINER_SIZE)) {
        return false;
    }
    const qint64 nObservedSize = guardedFile->size();
    if (!guardedFile || (nObservedSize != nExpectedSize)) return false;

    pData->clear();
    pData->reserve((int)nExpectedSize);
    const qint64 nChunkSize = 1024 * 1024;
    while (pData->size() < nExpectedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            pData->clear();
            return false;
        }
        qint64 nToRead = qMin(nChunkSize, nExpectedSize - pData->size());
        if (!guardedFile) {
            pData->clear();
            return false;
        }
        QByteArray baChunk = guardedFile->read(nToRead);
        if (!guardedFile || baChunk.isEmpty() || (baChunk.size() > nToRead)) {
            pData->clear();
            return false;
        }
        pData->append(baChunk);
    }

    if (!guardedFile) return false;
    const bool bAtEnd = guardedFile->atEnd();
    return guardedFile && bAtEnd && XBinary::isPdStructNotCanceled(pPdStruct);
}

static QString tarmaDeviceFileName(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return QString();
    const bool bSourceIdentityBound = guardedDevice->property("XStaticUnpacker.SourceIdentityBound").toBool();
    if (!guardedDevice) return QString();
    const QString sSourceFileName = guardedDevice->property("XStaticUnpacker.SourceFileName").toString();
    if (!guardedDevice) return QString();
    if (bSourceIdentityBound || !sSourceFileName.isEmpty()) {
        return sSourceFileName;
    }

    QFile *pFile = dynamic_cast<QFile *>(guardedDevice.data());
    QPointer<QFile> guardedFile(pFile);
    if (!guardedDevice || !guardedFile) return QString();
    const QString sResult = guardedFile->fileName();
    return (guardedDevice && guardedFile) ? sResult : QString();
}

static QList<TARMA_NAME> tarmaReadNames(const QList<TARMA_BLOCK> &listBlocks, XBinary::PDSTRUCT *pPdStruct)
{
    QList<TARMA_NAME> result;
    if (listBlocks.isEmpty()) return result;

    // The first tzf3 block is the tin9 database. Resolve its file records
    // through the folder-ID tree so plain-file packages can use an exact
    // declared relative path instead of a directory-wide basename search.
    const QByteArray &baMeta = listBlocks.first().baData;
    const quint8 *p = (const quint8 *)baMeta.constData();
    const qint64 n = baMeta.size();

    QHash<QByteArray, TARMA_FOLDER> mapFolders;
    for (qint64 i = 0; i + 64 <= n; i++) {
        if (((i & 0xFFFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            result.clear();
            return result;
        }
        if ((memcmp(p + i, "fldr", 4) != 0) || (tarmaRd32(p + i + 4) != 0)) continue;

        // Standard folders store the identifier length at +44. Folders tied
        // to a component have one additional eight-byte component field and
        // store it at +52.
        const qint64 anNameLengthOffsets[] = {44, 52};
        for (qint32 j = 0; j < 2; j++) {
            qint64 nNameLengthOffset = anNameLengthOffsets[j];
            QByteArray baIdentifier;
            if (!tarmaReadDbString(p, n, i + nNameLengthOffset, &baIdentifier)) continue;
            if (!tarmaIsAsciiIdentifier(baIdentifier)) continue;

            qint64 nPhysicalLengthOffset = i + nNameLengthOffset + 8 + baIdentifier.size();
            QByteArray baPhysicalName;
            if (!tarmaReadDbString(p, n, nPhysicalLengthOffset, &baPhysicalName)) continue;

            TARMA_FOLDER folder;
            folder.baParentId = QByteArray((const char *)p + i + nNameLengthOffset - 8, 8);
            folder.sPhysicalName = QString::fromLocal8Bit(baPhysicalName);
            if (!tarmaIsSafePathPart(folder.sPhysicalName)) continue;
            QByteArray baFolderKey((const char *)p + i + 8, 8);
            if (mapFolders.contains(baFolderKey)) {
                result.clear();
                return result;
            }
            mapFolders.insert(baFolderKey, folder);
            break;
        }
    }

    for (qint64 i = 0; i + 88 <= n; i++) {
        if (((i & 0xFFFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            result.clear();
            return result;
        }
        if ((memcmp(p + i, "file", 4) != 0) || (tarmaRd32(p + i + 4) != 0)) continue;

        QByteArray baName;
        if (!tarmaReadDbString(p, n, i + 84, &baName)) continue;

        quint64 nFileSize = tarmaRd64(p + i + 60);
        QString sName = QString::fromLocal8Bit(baName);
        if (!tarmaIsSafePathPart(sName) || (nFileSize > (256U << 20))) {
            continue;
        }
        TARMA_NAME entry;
        entry.sName = sName;
        entry.sLoosePath = tarmaBuildLoosePath(QByteArray((const char *)p + i + 44, 8), entry.sName, mapFolders);
        entry.nSize = nFileSize;
        result.append(entry);
        if (result.size() > TARMA_MAX_FILE_COUNT) {
            result.clear();
            return result;
        }
    }

    QSet<QString> setDeclaredNames;
    for (const TARMA_NAME &entry : result) setDeclaredNames.insert(tarmaNameKey(entry.sName));
    QSet<QString> setAssignedNames;
    for (qint32 i = 0; i < result.size(); i++) {
        const QString sUniqueName = tarmaUniqueOutputName(result.at(i).sName, i, setDeclaredNames, &setAssignedNames);
        if (sUniqueName.isEmpty()) {
            result.clear();
            return result;
        }
        result[i].sName = sUniqueName;
    }

    return result;
}

bool XTarma::_buildEntries(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;
    pContext->listEntries.clear();

    INTERNAL_INFO info = _detect(pPdStruct);
    if (!info.bIsValid || !info.bExtractable || (info.nContainerOffset < 0)) return false;

    qint64 nPrimarySize = -1;
    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return false;
    if (!info.bIsLegacy) {
        const QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections = pe.getSectionHeaders(pPdStruct);
        for (int i = 0; i < listSections.size(); i++) {
            QByteArray baName(reinterpret_cast<const char *>(listSections.at(i).Name), 8);
            int nZero = baName.indexOf('\0');
            if (nZero >= 0) baName.truncate(nZero);
            if ((baName == ".tsuarch") && ((qint64)listSections.at(i).PointerToRawData == info.nContainerOffset)) {
                nPrimarySize = (qint64)listSections.at(i).SizeOfRawData;
                break;
            }
        }
    } else {
        qint64 nContainerEnd = getSize();
        XBinary::OFFSETSIZE osSignature = pe.getSignOffsetSize();
        if ((osSignature.nOffset > info.nContainerOffset) && (osSignature.nSize > 0) && (osSignature.nOffset <= getSize()) &&
            (osSignature.nSize == getSize() - osSignature.nOffset)) {
            nContainerEnd = osSignature.nOffset;
        }
        nPrimarySize = nContainerEnd - info.nContainerOffset;
    }
    if ((nPrimarySize <= 0) || (nPrimarySize > TARMA_MAX_CONTAINER_SIZE) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if ((info.nContainerOffset > getSize()) || (nPrimarySize > getSize() - info.nContainerOffset)) return false;

    QByteArray baPrimaryContainer = read_array_process(info.nContainerOffset, nPrimarySize, pPdStruct);
    if ((baPrimaryContainer.size() != nPrimarySize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QByteArray baPrimaryInner;
    if (!tarmaDecodeContainer(baPrimaryContainer, TARMA_MAX_INNER_SIZE, &baPrimaryInner, pPdStruct)) return false;

    QList<TARMA_BLOCK> listBlocks = tarmaReadBlocks(baPrimaryInner, pPdStruct);
    QList<TARMA_NAME> listNames = tarmaReadNames(listBlocks, pPdStruct);
    if (listBlocks.isEmpty() || listNames.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    quint64 nDeclaredPayloadSize = 0;
    for (const TARMA_NAME &name : listNames) {
        if ((name.nSize > TARMA_MAX_TOTAL_PAYLOAD_SIZE - nDeclaredPayloadSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        nDeclaredPayloadSize += name.nSize;
    }

    if (!listBlocks.isEmpty()) {
        // The first primary tzf3 block is the tin9 metadata database. It is
        // never a payload, even if a declared file happens to have its size.
        listBlocks[0].bUsed = true;
    }

    QPointer<QIODevice> guardedInputDevice(getDevice());
    if (!guardedInputDevice) return false;
    const QString sInputFileName = tarmaDeviceFileName(guardedInputDevice.data());
    if (!guardedInputDevice) return false;
    QString sInputDirectory;
    quint64 nTotalSourceSize = (quint64)nPrimarySize;
    quint64 nTotalDecodedSize = (quint64)baPrimaryInner.size();
    if (!sInputFileName.isEmpty()) {
        QFileInfo inputInfo(sInputFileName);
        sInputDirectory = inputInfo.absolutePath();

        // Loader/archive distributions place one or more matching TIZ volumes
        // next to the executable.
        if (!info.bIsLegacy && (baPrimaryContainer.size() >= 0x40)) {
            QDir inputDir(sInputDirectory);
            const QFileInfoList listCandidates = inputDir.entryInfoList(QDir::Files | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
            if (listCandidates.size() > TARMA_MAX_DIRECTORY_ENTRIES) return false;
            static const QRegularExpression reVolume(QStringLiteral("^Disk([0-9]{4})\\.tiz$"), QRegularExpression::CaseInsensitiveOption);
            QMap<int, QFileInfo> mapVolumes;
            QByteArray baGuid = baPrimaryContainer.mid(0x30, 16);
            QString sCanonicalDirectory = QFileInfo(inputDir.absolutePath()).canonicalFilePath();
            if (sCanonicalDirectory.isEmpty()) return false;
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
            const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
            const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif
            for (int i = 0; i < listCandidates.size(); i++) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

                const QFileInfo &candidate = listCandidates.at(i);
                QRegularExpressionMatch match = reVolume.match(candidate.fileName());
                if (!match.hasMatch()) continue;
                bool bNumberOK = false;
                int nVolume = match.captured(1).toInt(&bNumberOK);
                // Files that only resemble a Tarma volume must not poison a
                // valid package in the same directory. Authenticate the
                // package GUID before applying strict volume constraints.
                QString sCanonicalCandidate = candidate.canonicalFilePath();
                if (!bNumberOK || (nVolume <= 0) || (nVolume > TARMA_MAX_VOLUME_COUNT) || candidate.isSymLink() || sCanonicalCandidate.isEmpty() ||
                    (QFileInfo(sCanonicalCandidate).absolutePath().compare(sCanonicalDirectory, pathCaseSensitivity) != 0) || (candidate.size() < 0x40)) {
                    continue;
                }

                QFile headerFile(sCanonicalCandidate);
                if (!headerFile.open(QIODevice::ReadOnly)) continue;
                QByteArray baHeader = headerFile.read(0x40);
                headerFile.close();
                if ((baHeader.size() != 0x40) || (baHeader.mid(0x30, 16) != baGuid)) continue;

                if (mapVolumes.contains(nVolume)) return false;
                mapVolumes.insert(nVolume, candidate);
            }

            int nLastMatchingVolume = 0;
            QMap<int, QFileInfo>::const_iterator it = mapVolumes.constBegin();
            while (it != mapVolumes.constEnd()) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

                const QFileInfo &volumeInfo = it.value();
                qint64 nVolumeSize = volumeInfo.size();
                if ((nVolumeSize < 0x40) || (nVolumeSize > TARMA_MAX_CONTAINER_SIZE) || ((quint64)nVolumeSize > TARMA_MAX_TOTAL_SOURCE_SIZE - nTotalSourceSize)) {
                    return false;
                }

                QString sCanonicalVolume = volumeInfo.canonicalFilePath();
                if (sCanonicalVolume.isEmpty() || (QFileInfo(sCanonicalVolume).absolutePath().compare(sCanonicalDirectory, pathCaseSensitivity) != 0)) {
                    return false;
                }
                std::unique_ptr<XMaterializedUnpackGuard> pVolumeGuard(XMaterializedUnpackGuard::openFile(sCanonicalVolume, pPdStruct));
                QPointer<QIODevice> guardedVolumeDevice(pVolumeGuard ? pVolumeGuard->device() : nullptr);
                QFile *pVolumeFile = guardedVolumeDevice ? dynamic_cast<QFile *>(guardedVolumeDevice.data()) : nullptr;
                QPointer<QFile> guardedVolumeFile(pVolumeFile);
                if (!guardedVolumeDevice || !guardedVolumeFile) return false;
                const qint64 nObservedVolumeSize = guardedVolumeFile->size();
                if (!guardedVolumeDevice || !guardedVolumeFile || (nObservedVolumeSize != nVolumeSize)) return false;
                QByteArray baVolume;
                bool bReadOK = tarmaReadFileBounded(guardedVolumeFile.data(), nVolumeSize, &baVolume, pPdStruct);
                if (!bReadOK || !guardedVolumeDevice || !guardedVolumeFile) return false;

                // Standard DiskNNNN files belonging to another installer may
                // coexist in the directory. Only the exact package GUID joins
                // this logical archive.
                if (baVolume.mid(0x30, 16) == baGuid) {
                    if (it.key() != nLastMatchingVolume + 1) return false;
                    nLastMatchingVolume = it.key();
                    nTotalSourceSize += (quint64)nVolumeSize;

                    QByteArray baVolumeInner;
                    if (nTotalDecodedSize > TARMA_MAX_TOTAL_DECODED_SIZE) return false;
                    qint64 nRemainingDecoded = (qint64)qMin<quint64>(TARMA_MAX_INNER_SIZE, TARMA_MAX_TOTAL_DECODED_SIZE - nTotalDecodedSize);
                    if (!tarmaDecodeContainer(baVolume, nRemainingDecoded, &baVolumeInner, pPdStruct) ||
                        ((quint64)baVolumeInner.size() > TARMA_MAX_TOTAL_DECODED_SIZE - nTotalDecodedSize)) {
                        return false;
                    }
                    nTotalDecodedSize += (quint64)baVolumeInner.size();
                    QList<TARMA_BLOCK> listVolumeBlocks = tarmaReadBlocks(baVolumeInner, pPdStruct);
                    if (listVolumeBlocks.isEmpty() || (listVolumeBlocks.size() > TARMA_MAX_BLOCK_COUNT - listBlocks.size())) return false;
                    listBlocks.append(listVolumeBlocks);
                    if (!tarmaRetainCompanionGuard(&pContext->listCompanionGuards, &pVolumeGuard)) return false;
                }
                ++it;
            }
        }
    }

    quint64 nTotalPayloadSize = 0;
    for (int i = 0; i < listNames.size(); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        QByteArray baFile;
        bool bResolved = false;
        std::unique_ptr<XMaterializedUnpackGuard> pLooseGuard;
        for (int j = 0; j < listBlocks.size(); j++) {
            if (!listBlocks[j].bUsed && ((quint64)listBlocks[j].baData.size() == listNames.at(i).nSize)) {
                baFile = listBlocks[j].baData;
                listBlocks[j].bUsed = true;
                bResolved = true;
                break;
            }
        }

        // Plain-file-tree distributions materialize the tin9 folder tree next
        // to the installer. Open only the exact metadata-declared path, and
        // reject links that resolve outside the installer directory.
        if (!bResolved && !sInputDirectory.isEmpty() && !listNames.at(i).sLoosePath.isEmpty()) {
            QString sBasePath = QFileInfo(sInputDirectory).canonicalFilePath();
            QFileInfo looseInfo(QDir(sInputDirectory).absoluteFilePath(listNames.at(i).sLoosePath));
            QString sLoosePath = looseInfo.canonicalFilePath();
            if (!sBasePath.isEmpty() && !sLoosePath.isEmpty() && looseInfo.isFile() && !looseInfo.isSymLink() &&
                !tarmaPathHasLink(sInputDirectory, listNames.at(i).sLoosePath) && ((quint64)looseInfo.size() == listNames.at(i).nSize)) {
                QString sRelativePath = QDir(sBasePath).relativeFilePath(sLoosePath);
                if (!QDir::isAbsolutePath(sRelativePath) && (sRelativePath != "..") && !sRelativePath.startsWith("../") && !sRelativePath.startsWith("..\\")) {
                    pLooseGuard.reset(XMaterializedUnpackGuard::openFile(sLoosePath, pPdStruct));
                    QPointer<QIODevice> guardedLooseDevice(pLooseGuard ? pLooseGuard->device() : nullptr);
                    QFile *pLooseFile = guardedLooseDevice ? dynamic_cast<QFile *>(guardedLooseDevice.data()) : nullptr;
                    QPointer<QFile> guardedLooseFile(pLooseFile);
                    qint64 nLooseFileSize = guardedLooseFile ? guardedLooseFile->size() : -1;
                    if (guardedLooseDevice && guardedLooseFile && (nLooseFileSize == (qint64)listNames.at(i).nSize)) {
                        bResolved =
                            tarmaReadFileBounded(guardedLooseFile.data(), (qint64)listNames.at(i).nSize, &baFile, pPdStruct) && guardedLooseDevice && guardedLooseFile;
                        if (!bResolved) baFile.clear();
                    }
                }
            }
        }

        if (bResolved && ((quint64)baFile.size() == listNames.at(i).nSize)) {
            if (pLooseGuard && !tarmaRetainCompanionGuard(&pContext->listCompanionGuards, &pLooseGuard)) return false;
            if ((quint64)baFile.size() > TARMA_MAX_TOTAL_PAYLOAD_SIZE - nTotalPayloadSize) return false;
            nTotalPayloadSize += (quint64)baFile.size();

            FILE_ENTRY entry;
            entry.sName = listNames.at(i).sName;
            entry.baData = baFile;
            pContext->listEntries.append(entry);
        } else {
            // Never expose a successful prefix of a package. Every file in
            // the authoritative tin9 metadata must resolve exactly.
            pContext->listEntries.clear();
            return false;
        }
    }

    return (pContext->listEntries.size() == listNames.size()) && !pContext->listEntries.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XTarma::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    const PDSTRUCTLIFETIME progressLifetime = pPdStruct ? retainPdStructLifetime(pPdStruct) : PDSTRUCTLIFETIME();
    struct PROGRESS_ALIVE_PROBE {
        PDSTRUCT *pPdStruct;
        const PDSTRUCTLIFETIME *pProgressLifetime;
        bool operator()() const { return !pPdStruct || XBinary::isPdStructLifetimeAlive(*pProgressLifetime); }
    };
    const PROGRESS_ALIVE_PROBE isProgressAlive = {pPdStruct, &progressLifetime};
    if (!isProgressAlive()) return false;
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XTarma> guardedThis(this);
    if (pState->pContext || !pState->baUnpackSourceToken.isEmpty()) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pOldContext || !pLifetimeState->setContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState) ||
            (pOldContext->baToken != pState->baUnpackSourceToken))
            return false;
        pLifetimeState->setContexts.remove(pOldContext);
        *pState = UNPACK_STATE();
        delete pOldContext;
    } else {
        *pState = UNPACK_STATE();
    }
    if (!isProgressAlive() || !guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<QIODevice> guardedSource(getDevice());
    const quint64 nGeneration = getDeviceGeneration();
    const bool bIsImage = isImage();
    const XADDR nModuleAddress = getModuleAddress();
    if (!guardedSource) return false;
    const qint64 nSourceSize = guardedSource->size();
    if (!isProgressAlive() || !guardedThis || !guardedSource || (nSourceSize < 0) || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
        return false;
    std::unique_ptr<XMaterializedUnpackGuard> pSourceGuard(XMaterializedUnpackGuard::bind(guardedSource.data(), pPdStruct));
    if (!isProgressAlive() || !pSourceGuard || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
        return false;
    if (!m_bTrustedSnapshot) {
        QScopedPointer<QIODevice> pSnapshot(createFileBuffer(nSourceSize, pPdStruct));
        if (!isProgressAlive() || !guardedThis || !guardedSource || !pSnapshot || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        const QString sSourceFileName = tarmaDeviceFileName(guardedSource.data());
        if (!isProgressAlive() || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data())) return false;
        pSnapshot->setProperty("XStaticUnpacker.SourceIdentityBound", true);
        if (!sSourceFileName.isEmpty()) pSnapshot->setProperty("XStaticUnpacker.SourceFileName", sSourceFileName);
        const bool bCopied = copyDeviceMemory(guardedSource.data(), 0, pSnapshot.data(), 0, nSourceSize, pPdStruct);
        if (!isProgressAlive() || !bCopied || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        XTarma worker(pSnapshot.data(), bIsImage, nModuleAddress);
        worker.m_bTrustedSnapshot = true;
        UNPACK_STATE materializedState = {};
        const bool bMaterialized = worker.initUnpack(&materializedState, mapProperties, pPdStruct);
        if (!isProgressAlive() || !guardedThis || !guardedSource || !bMaterialized || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        UNPACK_CONTEXT *pMaterializedContext = static_cast<UNPACK_CONTEXT *>(materializedState.pContext);
        if (!pMaterializedContext) return false;
        std::unique_ptr<UNPACK_CONTEXT> pContext(new (std::nothrow) UNPACK_CONTEXT);
        if (!pContext) return false;
        pContext->listEntries = pMaterializedContext->listEntries;
        pContext->listCompanionGuards.swap(pMaterializedContext->listCompanionGuards);
        if (!worker.finishUnpack(&materializedState, nullptr) || !isProgressAlive() || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
            (getDevice() != guardedSource.data()) || !isPdStructNotCanceled(pPdStruct) || pContext->listEntries.isEmpty())
            return false;
        pContext->pSourceDevice = guardedSource;
        pContext->pOwnerState = pState;
        pContext->baToken = QUuid::createUuid().toRfc4122();
        pContext->nDeviceGeneration = nGeneration;
        pContext->nSourceSize = nSourceSize;
        if (pContext->baToken.isEmpty()) return false;
        const bool bSourceFinal = pSourceGuard->validateAndFinalize(pPdStruct);
        const bool bCompanionsCurrent = isProgressAlive() && XMaterializedUnpackGuard::areCurrent(pSourceGuard.get(), pContext->listCompanionGuards, pPdStruct);
        if (!isProgressAlive() || !bSourceFinal || !bCompanionsCurrent || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
            (getDevice() != guardedSource.data()) || !isPdStructNotCanceled(pPdStruct))
            return false;
        pContext->pSourceGuard = pSourceGuard.release();
        pState->nTotalSize = nSourceSize;
        pState->nNumberOfRecords = pContext->listEntries.size();
        pState->mapUnpackProperties = mapProperties;
        pState->pContext = pContext.get();
        pState->baUnpackSourceToken = pContext->baToken;
        pLifetimeState->setContexts.insert(pContext.release());
        return guardedThis && pLifetimeState->bOwnerAlive;
    }
    pState->nTotalSize = nSourceSize;
    pState->mapUnpackProperties = mapProperties;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) return false;
    const bool bBuilt = _buildEntries(pContext, pPdStruct);
    if (!isProgressAlive() || !guardedThis || !guardedSource || !bBuilt) {
        delete pContext;
        return false;
    }

    pContext->pSourceDevice = guardedSource;
    pContext->pOwnerState = pState;
    pContext->baToken = QUuid::createUuid().toRfc4122();
    pContext->nDeviceGeneration = nGeneration;
    pContext->nSourceSize = nSourceSize;
    if (pContext->baToken.isEmpty()) {
        delete pContext;
        return false;
    }
    const bool bCompanionsFinal = tarmaFinalizeCompanionGuards(pContext->listCompanionGuards, pPdStruct);
    const bool bSourceFinal = isProgressAlive() && bCompanionsFinal && pSourceGuard->validateAndFinalize(pPdStruct);
    const bool bCompanionsCurrent =
        isProgressAlive() && bSourceFinal && XMaterializedUnpackGuard::areCurrent(pSourceGuard.get(), pContext->listCompanionGuards, pPdStruct);
    if (!isProgressAlive() || !bCompanionsFinal || !bSourceFinal || !bCompanionsCurrent || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
        (getDevice() != guardedSource.data()) || !isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        return false;
    }
    pContext->pSourceGuard = pSourceGuard.release();
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;
    pState->baUnpackSourceToken = pContext->baToken;
    pLifetimeState->setContexts.insert(pContext);
    return guardedThis && pLifetimeState->bOwnerAlive;
}

XBinary::ARCHIVERECORD XTarma::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return result;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XTarma> guardedThis(this);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    qint32 nIndex = pState->nCurrentIndex;
    if (!pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken) ||
        (pContext->nDeviceGeneration != getDeviceGeneration()) || (pContext->pSourceDevice.data() != getDevice()) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) || (nIndex != pContext->nCurrentIndex) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nTotalSize != pContext->nSourceSize) || (nIndex < 0) || (nIndex >= pContext->listEntries.size()))
        return result;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !guardedThis || !pLifetimeState->bOwnerAlive ||
        !pLifetimeState->setContexts.contains(pContext) || (pState->pContext != pContext) || (pContext->pOwnerState != pState) ||
        (pContext->baToken != pState->baUnpackSourceToken) || (pState->nCurrentIndex != pContext->nCurrentIndex))
        return result;

    const FILE_ENTRY &e = pContext->listEntries.at(nIndex);
    result.nStreamSize = e.baData.size();
    result.mapProperties[FPART_PROP_ORIGINALNAME] = e.sName;
    result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)e.baData.size();
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    return guardedThis ? result : ARCHIVERECORD();
}

static bool tarmaFailOutput(bool bSeekableOutput, QIODevice *pDevice, XBinary::UNPACK_STATE *pState)
{
    if (bSeekableOutput) {
        XBinary::resize(pDevice, 0);
        pDevice->seek(0);
        pState->nCurrentOffset = 0;
    }
    return false;
}

bool XTarma::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XTarma> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !guardedOutput || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint32 nIndex = pState->nCurrentIndex;
    struct AUTHENTICATION_PROBE {
        QPointer<XTarma> guardedThis;
        QSharedPointer<LIFETIME_STATE> pLifetimeState;
        UNPACK_CONTEXT *pContext;
        UNPACK_STATE *pState;
        qint32 nIndex;

        bool operator()() const
        {
            return guardedThis && pLifetimeState->bOwnerAlive && pLifetimeState->setContexts.contains(pContext) && (pState->pContext == pContext) &&
                   (pContext->pOwnerState == pState) && (pState->baUnpackSourceToken == pContext->baToken) &&
                   (pContext->nDeviceGeneration == guardedThis->getDeviceGeneration()) && (pContext->pSourceDevice.data() == guardedThis->getDevice()) &&
                   (pState->nCurrentIndex == pContext->nCurrentIndex) && (pState->nCurrentOffset == pContext->nCurrentOffset) &&
                   (pState->nNumberOfRecords == pContext->listEntries.size()) && (pState->nTotalSize == pContext->nSourceSize) && (nIndex >= 0) &&
                   (nIndex < pContext->listEntries.size());
        }
    };
    const AUTHENTICATION_PROBE isAuthenticated = {guardedThis, pLifetimeState, pContext, pState, nIndex};
    if (!isAuthenticated()) return false;
    const bool bOpen = guardedOutput->isOpen();
    if (!isAuthenticated() || !guardedOutput || !bOpen) return false;
    const bool bWritable = guardedOutput->isWritable();
    if (!isAuthenticated() || !guardedOutput || !bWritable) return false;
    const bool bSequential = guardedOutput->isSequential();
    if (!isAuthenticated() || !guardedOutput || bSequential) return false;
    const QIODevice::OpenMode openMode = guardedOutput->openMode();
    if (!isAuthenticated() || !guardedOutput || (openMode & (QIODevice::Append | QIODevice::Text)) || !isResizeEnable(guardedOutput.data()) || !guardedOutput ||
        devicesAlias(pContext->pSourceDevice.data(), guardedOutput.data()) || !isAuthenticated() || !guardedOutput)
        return false;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !guardedOutput || !isAuthenticated()) return false;
    // This override bypasses the base decode chain's per-entry gate; account the member here.
    // Produced bytes are charged by writeUnpackData at publication below.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->listEntries.at(nIndex).sName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }
    const QByteArray baData = pContext->listEntries.at(nIndex).baData;
    QScopedPointer<QIODevice> pStage(createFileBuffer(baData.size(), pPdStruct));
    QPointer<QIODevice> guardedStage(pStage.data());
    if (!isAuthenticated() || !guardedOutput || !guardedStage) return false;
    UNPACK_STATE writeState = *pState;
    writeState.pContext = nullptr;
    writeState.baUnpackSourceToken.clear();
    // The stage copy re-writes bytes charged again at publication below;
    // detach the budget here so each produced member is charged exactly once.
    writeState.spOutputBudget.clear();
    if (!writeUnpackData(&writeState, guardedStage.data(), baData, pPdStruct) || !guardedStage || !guardedOutput || !isAuthenticated()) return false;
    writeState.nCurrentOffset = 0;
    writeState.spOutputBudget = pState->spOutputBudget;
    const bool bPublished = writeUnpackData(&writeState, guardedOutput.data(), baData, pPdStruct);
    const bool bSourceCurrent = bPublished && XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct);
    const bool bFinal = bSourceCurrent && guardedOutput && isAuthenticated() && isPdStructNotCanceled(pPdStruct);
    if (!bFinal) {
        if (bPublished && guardedOutput) {
            resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }
    pContext->nCurrentOffset = writeState.nCurrentOffset;
    pState->nCurrentOffset = writeState.nCurrentOffset;
    return true;
}

bool XTarma::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XTarma> guardedThis(this);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken) ||
        (pContext->nDeviceGeneration != getDeviceGeneration()) || (pContext->pSourceDevice.data() != getDevice()) || (pState->nCurrentIndex != pContext->nCurrentIndex) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nTotalSize != pContext->nSourceSize) || (pContext->nCurrentIndex < 0) || (pContext->nCurrentIndex >= pContext->listEntries.size()))
        return false;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !guardedThis || !pLifetimeState->bOwnerAlive ||
        !pLifetimeState->setContexts.contains(pContext) || (pState->pContext != pContext) || (pContext->pOwnerState != pState) ||
        (pContext->baToken != pState->baUnpackSourceToken) || (pContext->nCurrentIndex >= pContext->listEntries.size()))
        return false;
    ++pContext->nCurrentIndex;
    pContext->nCurrentOffset = 0;
    pState->nCurrentIndex = pContext->nCurrentIndex;
    pState->nCurrentOffset = 0;
    return (pContext->nCurrentIndex < pContext->listEntries.size());
}

bool XTarma::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    if (!pState->pContext && pState->baUnpackSourceToken.isEmpty()) {
        *pState = UNPACK_STATE();
        return true;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken))
        return false;
    pLifetimeState->setContexts.remove(pContext);
    *pState = UNPACK_STATE();
    delete pContext;
    return true;
}
