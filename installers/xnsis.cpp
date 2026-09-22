/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

/* The NSIS parser/extractor below is a focused port of the NSIS archive handler
 * from 7-Zip (CPP/7zip/Archive/Nsis, vendored under XArchive/inbox). It covers
 * the parts required to enumerate and extract the packed files:
 *   - first header + method/solid detection (NsisIn::Open2)
 *   - header decompression (Copy / Deflate / LZMA / BZip2; solid and non-solid)
 *   - block-header table + install-script instruction walk (NsisIn::ReadEntries)
 *   - NSIS string decoding with variable / shell-folder / lang substitution
 *   - per-file extraction by data-block position (NsisIn::Decode)
 * The NSIS-modified BZip2 variant is decoded via the vendored nsis_bzip2 decoder.
 */

#include "xnsis.h"

#include "xpe.h"
#include "xarchive.h"

#include <QByteArray>
#include <QBuffer>
#include <QTemporaryFile>

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <zlib.h>

#include "LzmaDec.h"
#include "nsis_bzip2/nsis_bzip2.h"

namespace {
class NsisOperationGuard {
public:
    explicit NsisOperationGuard(const QSharedPointer<XNSIS::UNPACK_LIFETIME_STATE> &pState) : m_pState(pState), m_bAcquired(false)
    {
        if (m_pState && m_pState->bOwnerAlive && !m_pState->bOperationInProgress) {
            m_pState->bOperationInProgress = true;
            m_bAcquired = true;
        }
    }
    ~NsisOperationGuard()
    {
        if (m_pState && m_bAcquired) m_pState->bOperationInProgress = false;
    }
    bool isAcquired() const
    {
        return m_bAcquired;
    }

private:
    QSharedPointer<XNSIS::UNPACK_LIFETIME_STATE> m_pState;
    bool m_bAcquired;
};

class NsisPublisher : public XArchive {
public:
    explicit NsisPublisher(QIODevice *pDevice) : XArchive(pDevice)
    {
    }
    using XArchive::publishUnpackOutput;
};

class NsisContextValidator {
public:
    NsisContextValidator(XNSIS *pOwner, QIODevice *pOutput,
                         const QSharedPointer<XNSIS::UNPACK_LIFETIME_STATE> &pLifetime, XNSIS::UNPACK_CONTEXT *pContext,
                         XBinary::UNPACK_STATE *pState)
        : m_pOwner(pOwner), m_pOutput(pOutput), m_pLifetime(pLifetime), m_pContext(pContext), m_pState(pState)
    {
    }

    bool isCurrent() const
    {
        return m_pLifetime && m_pLifetime->bOwnerAlive && m_pLifetime->setContexts.contains(m_pContext) &&
               (m_pContext->pOwnerState == m_pState) && (m_pState->pContext == m_pContext);
    }

private:
    XNSIS *m_pOwner;
    QIODevice *m_pOutput;
    QSharedPointer<XNSIS::UNPACK_LIFETIME_STATE> m_pLifetime;
    XNSIS::UNPACK_CONTEXT *m_pContext;
    XBinary::UNPACK_STATE *m_pState;
};
}  // namespace

// ---------------------------------------------------------------------------
// Constants (mirror 7-Zip NsisIn.cpp)
// ---------------------------------------------------------------------------

static const unsigned kNumCommandParams = 6;
static const unsigned kCmdSize = 4 + kNumCommandParams * 4;  // 28

// install-script opcodes we care about
enum {
    EW_CREATEDIR = 11,        // CreateDirectory / SetOutPath
    EW_EXTRACTFILE = 20,      // File
    EW_ASSIGNVAR = 25,        // StrCpy
    EW_FCLOSE = 54,           // FileClose
    EW_FOPEN = 55,            // FileOpen
    EW_WRITEUNINSTALLER = 62  // WriteUninstaller
};

static const quint32 kWinGenericWrite = (quint32)1 << 30;
static const quint32 kWinCreateAlways = 2;

// internal variable indexes
#define kVar_INSTDIR 21
#define kVar_OUTDIR 22
#define kVar_EXEDIR 23
#define kVar_TEMP 25
#define kVar_PLUGINSDIR 26
#define kVar_Spec_OUTDIR 31

// NSIS-2 / Park string escape codes
#define NS_CODE_SKIP 252
#define NS_CODE_VAR 253
#define NS_CODE_SHELL 254
// NS_CODE_LANG 255

// NSIS-3 string escape codes
#define NS_3_CODE_LANG 1
#define NS_3_CODE_SHELL 2
#define NS_3_CODE_VAR 3
#define NS_3_CODE_SKIP 4

// Park (unicode) string escape codes
#define PARK_CODE_SKIP 0xE000
#define PARK_CODE_VAR 0xE001
#define PARK_CODE_SHELL 0xE002
#define PARK_CODE_LANG 0xE003

#define IS_NS_SPEC_CHAR(c) ((c) >= NS_CODE_SKIP)
#define IS_PARK_SPEC_CHAR(c) ((c) >= PARK_CODE_SKIP && (c) <= PARK_CODE_LANG)

#define DECODE_NUMBER_FROM_2_CHARS(c0, c1) (((unsigned)(c0) & 0x7F) | (((unsigned)((c1) & 0x7F)) << 7))
#define CONVERT_NUMBER_NS_3_UNICODE(n) ((n) = (((n) & 0x7F) | ((((n) >> 8) & 0x7F) << 7)))
#define CONVERT_NUMBER_PARK(n) ((n) &= 0x7FFF)

static const quint32 kMask_IsCompressed = (quint32)1 << 31;
static const qint64 kNsisMaxHeaderDecoded = 64LL * 1024 * 1024;
static const qint64 kNsisMaxPackedBlock = 512LL * 1024 * 1024;
static const qint64 kNsisMaxMemberDecoded = 512LL * 1024 * 1024;
static const qint64 kNsisMaxSolidDecoded = 512LL * 1024 * 1024;
static const quint32 kNsisMaxLzmaDictionary = 64U * 1024 * 1024;

static const char *const kVarStrings[] = {"CMDLINE",    "INSTDIR", "OUTDIR",  "EXEDIR",     "LANGUAGE", "TEMP",
                                          "PLUGINSDIR", "EXEPATH", "EXEFILE", "HWNDPARENT", "_CLICK",   "_OUTDIR"};

static const char *const kShellStrings[] = {"DESKTOP",
                                            "INTERNET",
                                            "SMPROGRAMS",
                                            "CONTROLS",
                                            "PRINTERS",
                                            "DOCUMENTS",
                                            "FAVORITES",
                                            "SMSTARTUP",
                                            "RECENT",
                                            "SENDTO",
                                            "BITBUCKET",
                                            "STARTMENU",
                                            nullptr,
                                            "MUSIC",
                                            "VIDEOS",
                                            nullptr,
                                            "DESKTOP",
                                            "DRIVES",
                                            "NETWORK",
                                            "NETHOOD",
                                            "FONTS",
                                            "TEMPLATES",
                                            "STARTMENU",
                                            "SMPROGRAMS",
                                            "SMSTARTUP",
                                            "DESKTOP",
                                            "APPDATA",
                                            "PRINTHOOD",
                                            "LOCALAPPDATA",
                                            "ALTSTARTUP",
                                            "ALTSTARTUP",
                                            "FAVORITES",
                                            "INTERNET_CACHE",
                                            "COOKIES",
                                            "HISTORY",
                                            "APPDATA",
                                            "WINDIR",
                                            "SYSDIR",
                                            "PROGRAM_FILES",
                                            "PICTURES",
                                            "PROFILE",
                                            "SYSTEMX86",
                                            "PROGRAM_FILESX86",
                                            "PROGRAM_FILES_COMMON",
                                            "PROGRAM_FILES_COMMONX8",
                                            "TEMPLATES",
                                            "DOCUMENTS",
                                            "ADMINTOOLS",
                                            "ADMINTOOLS",
                                            "CONNECTIONS",
                                            nullptr,
                                            nullptr,
                                            nullptr,
                                            "MUSIC",
                                            "PICTURES",
                                            "VIDEOS",
                                            "RESOURCES",
                                            "RESOURCES_LOCALIZED",
                                            "COMMON_OEM_LINKS",
                                            "CDBURN_AREA",
                                            nullptr,
                                            "COMPUTERSNEARME"};

static const unsigned kNumShellStrings = sizeof(kShellStrings) / sizeof(kShellStrings[0]);

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

static inline quint16 rd16(const quint8 *p)
{
    return (quint16)(p[0] | ((quint16)p[1] << 8));
}

static inline quint32 rd32(const quint8 *p)
{
    return (quint32)(p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24));
}

// read the offset/count pair of block-header entry k (32- or 64-bit layout)
static void nsisParseBlockHeader(const quint8 *p, unsigned nBhoSize, int k, quint32 *pOffset, quint32 *pNum)
{
    const quint8 *pb = p + 4 + nBhoSize * k;
    *pOffset = rd32(pb);
    *pNum = rd32(pb + nBhoSize - 4);
}

static bool nsisIsLZMA(const quint8 *p, quint32 *pDict)
{
    if (pDict) {
        *pDict = rd32(p + 1);
    }
    return (p[0] == 0x5D && p[1] == 0x00 && p[2] == 0x00 && p[5] == 0x00 && (p[6] & 0x80) == 0x00);
}

static bool nsisIsLZMA_flag(const quint8 *p, quint32 *pDict, bool *pThereIsFlag)
{
    if (nsisIsLZMA(p, pDict)) {
        *pThereIsFlag = false;
        return true;
    }
    if (p[0] <= 1 && nsisIsLZMA(p + 1, pDict)) {
        *pThereIsFlag = true;
        return true;
    }
    return false;
}

static bool nsisIsBZip2(const quint8 *p)
{
    return (p[0] == 0x31 && p[1] < 14);
}

static bool nsisIsDrivePath(const QString &s)
{
    if (s.length() < 2) {
        return false;
    }
    QChar c = s.at(0);
    return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) && (s.at(1) == ':');
}

static bool nsisIsAbsolutePath(const QString &s)
{
    if (s.startsWith("\\\\")) {
        return true;
    }
    return nsisIsDrivePath(s);
}

static QString nsisReducedName(const QString &sPrefix, bool bHasPrefix, const QString &sName)
{
    QString s;
    if (bHasPrefix) {
        s = sPrefix;
        if (!s.isEmpty() && !s.endsWith('\\')) {
            s += '\\';
        }
    }
    s += sName.isEmpty() ? QString("file") : sName;

    const QString sRemove = "$INSTDIR\\";
    if (s.startsWith(sRemove, Qt::CaseInsensitive)) {
        s = s.mid(sRemove.length());
        if (s.startsWith('\\')) {
            s = s.mid(1);
        }
    }
    // Normalise separators to forward slash so output paths are portable.
    s.replace('\\', '/');
    return s;
}

// The SetOutPath ($OUTDIR) directory a file is extracted to, reduced the same way as
// nsisReducedName (strip a leading "$INSTDIR"/"$INSTDIR\", normalise separators). "$INSTDIR"
// itself (the install root) reduces to an empty path.
static QString nsisReducedPath(const QString &sPrefix, bool bHasPrefix)
{
    if (!bHasPrefix) {
        return QString();
    }

    QString s = sPrefix;
    const QString sRemove = "$INSTDIR\\";

    if (s.startsWith(sRemove, Qt::CaseInsensitive)) {
        s = s.mid(sRemove.length());
        if (s.startsWith('\\')) {
            s = s.mid(1);
        }
    } else if (s.compare(QString("$INSTDIR"), Qt::CaseInsensitive) == 0) {
        s.clear();
    }

    s.replace('\\', '/');
    return s;
}

static void *nsisSzAlloc(ISzAllocPtr, size_t nSize)
{
    return malloc(nSize);
}

static void nsisSzFree(ISzAllocPtr, void *p)
{
    free(p);
}

static ISzAlloc g_nsisAlloc = {nsisSzAlloc, nsisSzFree};

// ---------------------------------------------------------------------------
// XCONVERT table + boilerplate
// ---------------------------------------------------------------------------

XBinary::XCONVERT _TABLE_XNSIS_STRUCTID[] = {
    {XNSIS::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XNSIS::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XNSIS::STRUCTID_FILEENTRY, "FILEENTRY", QString("File Entry")},
};

XNSIS::UNPACK_LIFETIME_STATE::~UNPACK_LIFETIME_STATE()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) XNSIS::deleteUnpackContext(pContext);
}

void XNSIS::deleteUnpackContext(UNPACK_CONTEXT *pContext)
{
    if (!pContext) return;
    if (pContext->pSourceValidator) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
    }
    delete pContext;
}

XNSIS::XNSIS(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackLifetimeState = QSharedPointer<UNPACK_LIFETIME_STATE>::create();
}

XNSIS::~XNSIS()
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    if (pLifetime) pLifetime->bOwnerAlive = false;
    m_pUnpackLifetimeState.clear();
}

QString XNSIS::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XNSIS_STRUCTID, sizeof(_TABLE_XNSIS_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XNSIS::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XNSIS_STRUCTID, sizeof(_TABLE_XNSIS_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XNSIS::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XNSIS_STRUCTID, sizeof(_TABLE_XNSIS_STRUCTID) / sizeof(XBinary::XCONVERT));
}

XBinary::FT XNSIS::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_NSIS;
    }

    return FT_PE32_NSIS;
}

bool XNSIS::isValid(PDSTRUCT *pPdStruct)
{
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
}

XNSIS::INTERNAL_INFO XNSIS::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _analyse(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XNSIS::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bAlreadyHandled = isInternalInfoHandled();
    if (!bAlreadyHandled) {
        const quint64 nTransaction = beginInternalInfoTransaction();
        if (!nTransaction) return false;

        // The transaction supplies the recursion sentinel. Keep every
        // source-derived value local until the same binding is revalidated.
        m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = _getInternalInfo(pPdStruct);
        if (!isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap = getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!isInternalInfoTransactionCurrent(nTransaction)) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        m_internalInfo = info;
        if (!commitInternalInfoTransaction(nTransaction, static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo))) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XNSIS::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XNSIS::setInternalInfo(void *pInternalInfo)
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

XNSIS::INTERNAL_INFO XNSIS::_analyse(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};

    qint64 nFileSize = getSize();
    if (nFileSize < 0) {
        nFileSize = 0;
    }

    const char *apszSignatures[] = {"NullsoftInst", "Nullsoft.NSIS", ";!@Install@!UTF-8!", ";!@Install@!UTF-16LE", ";!@Install@!"};
    const qint32 nSignatureCount = sizeof(apszSignatures) / sizeof(const char *);

    for (qint32 i = 0; (i < nSignatureCount) && (!result.bIsValid); i++) {
        QString sSignatureCandidate = QString::fromLatin1(apszSignatures[i]);
        qint64 nOffset = find_ansiString(0, nFileSize, sSignatureCandidate, pPdStruct);

        if (nOffset != -1) {
            result.bIsValid = true;
            result.nSignatureOffset = nOffset;
            result.sSignature = sSignatureCandidate;

            const qint64 nWindowSize = 0x200;
            qint64 nRemainingSize = nFileSize - nOffset;
            if (nRemainingSize < 0) {
                nRemainingSize = 0;
            }

            qint64 nBytesToRead = (std::min)(nWindowSize, nRemainingSize);

            if (nBytesToRead > 0) {
                QByteArray baWindow = read_array_process(nOffset, nBytesToRead, pPdStruct);

                if (!baWindow.isEmpty()) {
                    QString sWindow = QString::fromLatin1(baWindow.constData(), baWindow.size());

                    if ((sWindow.contains("UTF-8")) || (sWindow.contains("UTF-16"))) {
                        result.bIsUnicode = true;
                    }

                    qint32 nWindowLength = sWindow.length();
                    for (qint32 j = 0; (j < nWindowLength) && result.sVersion.isEmpty(); j++) {
                        QChar cCurrent = sWindow.at(j);
                        if ((cCurrent == QChar('v')) || (cCurrent == QChar('V'))) {
                            qint32 nPos = j + 1;
                            while ((nPos < nWindowLength) && sWindow.at(nPos).isSpace()) {
                                nPos++;
                            }
                            qint32 nVersionStart = nPos;
                            while ((nPos < nWindowLength) && (sWindow.at(nPos).isDigit() || (sWindow.at(nPos) == QChar('.')) || (sWindow.at(nPos) == QChar('_')))) {
                                nPos++;
                            }
                            if (nPos > nVersionStart) {
                                result.sVersion = sWindow.mid(nVersionStart, nPos - nVersionStart).trimmed();
                            }
                        }
                    }

                    if (result.sVersion.isEmpty()) {
                        qint32 nWindowIndex = 0;
                        while ((nWindowIndex < nWindowLength) && result.sVersion.isEmpty()) {
                            QChar cSymbol = sWindow.at(nWindowIndex);
                            if (cSymbol.isDigit()) {
                                qint32 nStartIndex = nWindowIndex;
                                qint32 nEndIndex = nWindowIndex;
                                while ((nEndIndex < nWindowLength) && (sWindow.at(nEndIndex).isDigit() || (sWindow.at(nEndIndex) == QChar('.')))) {
                                    nEndIndex++;
                                }
                                if (nEndIndex > nStartIndex) {
                                    QString sCandidate = sWindow.mid(nStartIndex, nEndIndex - nStartIndex).trimmed();
                                    if (sCandidate.count('.') >= 1) {
                                        result.sVersion = sCandidate;
                                    }
                                }
                                nWindowIndex = nEndIndex;
                            } else {
                                nWindowIndex++;
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// first header
// ---------------------------------------------------------------------------

bool XNSIS::_findFirstHeader(qint64 nSignatureOffset, qint64 nTotalSize, UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    // Header layout: [4 flags][0xDEADBEEF][16-byte signature ...] where the signature
    // 'NullsoftInst' begins 4 bytes after the DEADBEEF marker. HeaderSize is at +0x14,
    // ArcSize at +0x18. The DEADBEEF marker sits 4 bytes before the signature.
    qint64 nSearchStart = (std::max)((qint64)0, nSignatureOffset - 32);

    for (qint64 i = nSignatureOffset; (i >= nSearchStart) && isPdStructNotCanceled(pPdStruct); i--) {
        if (read_uint32(i, false) == 0xDEADBEEF) {
            qint64 nHeaderOffset = i - 4;
            if (nHeaderOffset < 0) {
                return false;
            }

            quint32 nFlags = read_uint32(nHeaderOffset, false);
            quint32 nHeaderSize = read_uint32(nHeaderOffset + 0x14, false);
            quint32 nArcSize = read_uint32(nHeaderOffset + 0x18, false);

            if ((nArcSize > 0x1C) && (nHeaderSize > 0) && ((qint64)nArcSize <= (nTotalSize - nHeaderOffset))) {
                pContext->nFirstHeaderOffset = nHeaderOffset;
                pContext->nDataStreamOffset = nHeaderOffset + 0x1C;
                pContext->nFlags = nFlags;
                pContext->nHeaderSize = nHeaderSize;
                pContext->nArchiveSize = nArcSize;
                return true;
            }
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// method / solid detection (NsisIn::Open2)
// ---------------------------------------------------------------------------

bool XNSIS::_detectMethod(UNPACK_CONTEXT *pContext, const quint8 *pSig, qint64 nSigSize)
{
    const qint64 kSigNeeded = 4 + 1 + 5 + 2;
    if (nSigSize < kSigNeeded) {
        return false;
    }

    pContext->bHeaderIsCompressed = true;
    pContext->bIsSolid = true;
    pContext->bFilterFlag = false;
    pContext->nNonSolidStartOffset = 0;

    quint32 nCompressedHeaderSize = rd32(pSig);
    quint32 nDict = 0;

    if (nCompressedHeaderSize == pContext->nHeaderSize) {
        pContext->bHeaderIsCompressed = false;
        pContext->bIsSolid = false;
        pContext->method = NMETHOD_COPY;
    } else if (nsisIsLZMA_flag(pSig, &nDict, &pContext->bFilterFlag)) {
        pContext->method = NMETHOD_LZMA;
    } else if (pSig[3] == 0x80) {
        pContext->bIsSolid = false;
        bool bFlag = false;
        if (nsisIsLZMA_flag(pSig + 4, &nDict, &bFlag)) {
            pContext->method = NMETHOD_LZMA;
            pContext->bFilterFlag = bFlag;
        } else if (nsisIsBZip2(pSig + 4)) {
            pContext->method = NMETHOD_BZIP2;
        } else {
            pContext->method = NMETHOD_DEFLATE;
        }
    } else if (nsisIsBZip2(pSig)) {
        pContext->method = NMETHOD_BZIP2;
    } else {
        pContext->method = NMETHOD_DEFLATE;
    }

    if (!pContext->bIsSolid) {
        pContext->bHeaderIsCompressed = ((nCompressedHeaderSize & kMask_IsCompressed) != 0);
        pContext->nNonSolidStartOffset = nCompressedHeaderSize & ~kMask_IsCompressed;
    }

    switch (pContext->method) {
        case NMETHOD_LZMA: pContext->compressMethod = HANDLE_METHOD_LZMA; break;
        case NMETHOD_BZIP2: pContext->compressMethod = HANDLE_METHOD_BZIP2; break;
        case NMETHOD_DEFLATE: pContext->compressMethod = HANDLE_METHOD_DEFLATE; break;
        default: pContext->compressMethod = HANDLE_METHOD_STORE; break;
    }

    return true;
}

// ---------------------------------------------------------------------------
// low-level decoders
// ---------------------------------------------------------------------------

static bool nsisGetDecodeLimits(qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, qint64 *pnEffectiveLimit, qint64 *pnProbeLimit)
{
    if (!pnEffectiveLimit || !pnProbeLimit || (nOutputLimit <= 0) || (nOutputLimit >= (std::numeric_limits<int>::max)()) ||
        (bOutHintKnown && ((nOutHint < 0) || (nOutHint > nOutputLimit)))) {
        return false;
    }

    *pnEffectiveLimit = bOutHintKnown ? nOutHint : nOutputLimit;
    *pnProbeLimit = *pnEffectiveLimit + 1;
    return true;
}

static bool nsisResizeDecoded(QByteArray *pResult, qint64 nSize)
{
    if (!pResult || (nSize < 0) || (nSize > (std::numeric_limits<int>::max)())) return false;

    try {
        pResult->resize((int)nSize);
    } catch (const std::bad_alloc &) {
        pResult->clear();
        return false;
    }

    return pResult->size() == nSize;
}

static bool nsisPrepareDecoded(QByteArray *pResult, qint64 nProbeLimit, qint64 nPreferredCapacity)
{
    if (!pResult || (nProbeLimit <= 0)) return false;
    pResult->clear();
    const qint64 nCapacity = (std::min)(nProbeLimit, (std::max)(1LL, nPreferredCapacity));
    return nsisResizeDecoded(pResult, nCapacity);
}

static bool nsisGrowDecoded(QByteArray *pResult, qint64 nProbeLimit)
{
    if (!pResult) return false;
    const qint64 nCurrent = pResult->size();
    if ((nCurrent < 0) || (nCurrent >= nProbeLimit)) return false;
    const qint64 nDoubled = (nCurrent > (nProbeLimit / 2)) ? nProbeLimit : (nCurrent * 2);
    const qint64 nNext = (std::min)(nProbeLimit, (std::max)(nCurrent + 1, nDoubled));
    return nsisResizeDecoded(pResult, nNext);
}

bool XNSIS::_lzmaDecode(const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, QByteArray *pResult, PDSTRUCT *pPdStruct)
{
    qint64 nEffectiveLimit = 0;
    qint64 nProbeLimit = 0;
    if (!pSrc || !pResult || (nSrcSize < 5) || (nSrcSize > kNsisMaxPackedBlock) || (rd32(pSrc + 1) > kNsisMaxLzmaDictionary) ||
        !nsisGetDecodeLimits(nOutHint, bOutHintKnown, nOutputLimit, &nEffectiveLimit, &nProbeLimit) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CLzmaDec dec;
    LzmaDec_Construct(&dec);

    if (LzmaDec_Allocate(&dec, (const Byte *)pSrc, 5, &g_nsisAlloc) != SZ_OK) {
        return false;
    }
    LzmaDec_Init(&dec);

    const Byte *pIn = (const Byte *)pSrc + 5;
    SizeT nInRemaining = (SizeT)(nSrcSize - 5);
    const qint64 nPreferredCapacity = bOutHintKnown ? (std::max)(1LL, nOutHint) : (1LL << 20);
    bool bOk = nsisPrepareDecoded(pResult, nProbeLimit, (std::max)(1LL << 16, nPreferredCapacity));
    qint64 nOutPos = 0;

    while (bOk) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            bOk = false;
            break;
        }
        if ((nOutPos >= pResult->size()) && !nsisGrowDecoded(pResult, nProbeLimit)) {
            bOk = false;
            break;
        }

        SizeT nDestLen = (SizeT)(pResult->size() - nOutPos);
        SizeT nSrcLen = nInRemaining;
        ELzmaStatus status = LZMA_STATUS_NOT_FINISHED;
        const SRes res = LzmaDec_DecodeToBuf(&dec, (Byte *)pResult->data() + nOutPos, &nDestLen, pIn, &nSrcLen, LZMA_FINISH_ANY, &status);

        nOutPos += (qint64)nDestLen;
        pIn += nSrcLen;
        nInRemaining -= nSrcLen;

        if ((res != SZ_OK) || (nOutPos > nEffectiveLimit)) {
            bOk = false;
            break;
        }
        if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
            break;
        }
        if ((nDestLen == 0) && (nSrcLen == 0)) {
            break;
        }
    }

    LzmaDec_Free(&dec, &g_nsisAlloc);
    bOk = bOk && isPdStructNotCanceled(pPdStruct) && (!bOutHintKnown || (nOutPos == nOutHint));

    if (bOk && nsisResizeDecoded(pResult, nOutPos)) return true;
    pResult->clear();
    return false;
}

bool XNSIS::_inflateRaw(const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, QByteArray *pResult, PDSTRUCT *pPdStruct)
{
    qint64 nEffectiveLimit = 0;
    qint64 nProbeLimit = 0;
    if (!pSrc || !pResult || (nSrcSize <= 0) || (nSrcSize > kNsisMaxPackedBlock) ||
        !nsisGetDecodeLimits(nOutHint, bOutHintKnown, nOutputLimit, &nEffectiveLimit, &nProbeLimit) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    z_stream stream = {};
    stream.next_in = (Bytef *)pSrc;
    stream.avail_in = (uInt)nSrcSize;
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return false;

    const qint64 nPreferredCapacity = bOutHintKnown ? (std::max)(1LL, nOutHint) : (1LL << 20);
    bool bOk = nsisPrepareDecoded(pResult, nProbeLimit, (std::max)(1LL << 16, nPreferredCapacity));
    bool bStreamComplete = false;
    qint64 nOutPos = 0;

    while (bOk) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            bOk = false;
            break;
        }
        if ((nOutPos >= pResult->size()) && !nsisGrowDecoded(pResult, nProbeLimit)) {
            bOk = false;
            break;
        }

        const uLong nPreviousOut = stream.total_out;
        const uInt nPreviousIn = stream.avail_in;
        stream.next_out = (Bytef *)pResult->data() + nOutPos;
        stream.avail_out = (uInt)(pResult->size() - nOutPos);

        const int nRes = inflate(&stream, Z_NO_FLUSH);
        nOutPos = (qint64)stream.total_out;
        if (nOutPos > nEffectiveLimit) {
            bOk = false;
            break;
        }
        if (nRes == Z_STREAM_END) {
            bStreamComplete = true;
            break;
        }
        if ((nRes != Z_OK) && (nRes != Z_BUF_ERROR)) {
            bOk = false;
            break;
        }
        if ((stream.total_out == nPreviousOut) && (stream.avail_in == nPreviousIn)) {
            break;
        }
        if ((stream.avail_in == 0) && (stream.avail_out != 0)) {
            break;
        }
    }

    inflateEnd(&stream);
    bOk = bOk && bStreamComplete && isPdStructNotCanceled(pPdStruct) && (!bOutHintKnown || (nOutPos == nOutHint));

    if (bOk && nsisResizeDecoded(pResult, nOutPos)) return true;
    pResult->clear();
    return false;
}

bool XNSIS::_bzip2Decode(const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, QByteArray *pResult, PDSTRUCT *pPdStruct)
{
    qint64 nEffectiveLimit = 0;
    qint64 nProbeLimit = 0;
    if (!pSrc || !pResult || (nSrcSize <= 0) || (nSrcSize > kNsisMaxPackedBlock) ||
        !nsisGetDecodeLimits(nOutHint, bOutHintKnown, nOutputLimit, &nEffectiveLimit, &nProbeLimit) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // NSIS uses a modified bzip2 (no "BZh" stream header, no RLE1 pass, no CRC).
    nsis_bzstream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = (unsigned char *)pSrc;
    strm.avail_in = (unsigned int)nSrcSize;
    if (nsis_BZ2_bzDecompressInit(&strm, 0, 0) != BZ_OK) return false;

    const qint64 nPreferredCapacity = bOutHintKnown ? (std::max)(1LL, nOutHint) : (1LL << 20);
    bool bOk = nsisPrepareDecoded(pResult, nProbeLimit, (std::max)(1LL << 16, nPreferredCapacity));
    bool bStreamComplete = false;
    qint64 nOutPos = 0;

    while (bOk) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            bOk = false;
            break;
        }
        if ((nOutPos >= pResult->size()) && !nsisGrowDecoded(pResult, nProbeLimit)) {
            bOk = false;
            break;
        }

        const unsigned int nPreviousOut = strm.total_out_lo32;
        const unsigned int nPreviousIn = strm.avail_in;
        strm.next_out = (unsigned char *)pResult->data() + nOutPos;
        strm.avail_out = (unsigned int)(pResult->size() - nOutPos);

        const int nRes = nsis_BZ2_bzDecompress(&strm);
        if (strm.total_out_hi32 != 0) {
            bOk = false;
            break;
        }
        nOutPos = (qint64)strm.total_out_lo32;
        if (nOutPos > nEffectiveLimit) {
            bOk = false;
            break;
        }
        if (nRes == BZ_STREAM_END) {
            bStreamComplete = true;
            break;
        }
        if (nRes != BZ_OK) {
            bOk = false;
            break;
        }
        if ((strm.total_out_lo32 == nPreviousOut) && (strm.avail_in == nPreviousIn)) {
            bStreamComplete = (strm.avail_in == 0);
            break;
        }
        if ((strm.avail_in == 0) && (strm.avail_out != 0)) {
            // NSIS streams can end when the framed input is exhausted.
            bStreamComplete = true;
            break;
        }
    }

    nsis_BZ2_bzDecompressEnd(&strm);
    bOk = bOk && bStreamComplete && isPdStructNotCanceled(pPdStruct) && (!bOutHintKnown || (nOutPos == nOutHint));

    if (bOk && nsisResizeDecoded(pResult, nOutPos)) return true;
    pResult->clear();
    return false;
}

bool XNSIS::_decodeBlock(NMETHOD method, bool bFilterFlag, const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit,
                         QByteArray *pResult, PDSTRUCT *pPdStruct)
{
    if (bFilterFlag || !pSrc || !pResult || !isPdStructNotCanceled(pPdStruct)) {
        // 7-Zip-modified NSIS (BCJ filter) is not supported here.
        return false;
    }

    switch (method) {
        case NMETHOD_LZMA: return _lzmaDecode(pSrc, nSrcSize, nOutHint, bOutHintKnown, nOutputLimit, pResult, pPdStruct);
        case NMETHOD_DEFLATE: return _inflateRaw(pSrc, nSrcSize, nOutHint, bOutHintKnown, nOutputLimit, pResult, pPdStruct);
        case NMETHOD_BZIP2: return _bzip2Decode(pSrc, nSrcSize, nOutHint, bOutHintKnown, nOutputLimit, pResult, pPdStruct);
        case NMETHOD_COPY: {
            qint64 nEffectiveLimit = 0;
            qint64 nProbeLimit = 0;
            if (!nsisGetDecodeLimits(nOutHint, bOutHintKnown, nOutputLimit, &nEffectiveLimit, &nProbeLimit)) return false;
            Q_UNUSED(nProbeLimit)
            const qint64 nSize = bOutHintKnown ? nOutHint : nSrcSize;
            if ((nSize < 0) || (nSize > nSrcSize) || (nSize > nEffectiveLimit) || (nSize > (std::numeric_limits<int>::max)())) return false;
            try {
                *pResult = QByteArray((const char *)pSrc, (int)nSize);
            } catch (const std::bad_alloc &) {
                pResult->clear();
                return false;
            }
            return (pResult->size() == nSize) && isPdStructNotCanceled(pPdStruct);
        }
        default: return false;
    }
}

// ---------------------------------------------------------------------------
// header / solid stream decoding
// ---------------------------------------------------------------------------

bool XNSIS::_decodeSolidStream(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (pContext->bSolidDecoded) {
        return !pContext->baSolid.isEmpty();
    }
    pContext->bSolidDecoded = true;

    if ((pContext->nDataSize <= 0) || (pContext->nDataSize > kNsisMaxPackedBlock)) {
        return false;
    }

    QByteArray baCompressed = read_array_process(pContext->nDataStreamOffset, pContext->nDataSize, pPdStruct);
    if (baCompressed.isEmpty()) {
        return false;
    }

    bool bOk = _decodeBlock(pContext->method, pContext->bFilterFlag, (const quint8 *)baCompressed.constData(), baCompressed.size(), 0, false, kNsisMaxSolidDecoded,
                            &pContext->baSolid, pPdStruct);
    return bOk && !pContext->baSolid.isEmpty();
}

bool XNSIS::_decodeHeader(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    const quint32 nHeaderSize = pContext->nHeaderSize;

    if (nHeaderSize > (quint32)kNsisMaxHeaderDecoded) {
        return false;
    }

    if (pContext->bIsSolid) {
        if (!_decodeSolidStream(pContext, pPdStruct)) {
            return false;
        }
        // solid layout: [4-byte HeaderSize][header][files...]
        if ((qint64)(4 + nHeaderSize) > pContext->baSolid.size()) {
            return false;
        }
        quint32 nStored = rd32((const quint8 *)pContext->baSolid.constData());
        if (nStored != nHeaderSize) {
            return false;
        }
        pContext->baHeader = pContext->baSolid.mid(4, nHeaderSize);
        return (quint32)pContext->baHeader.size() == nHeaderSize;
    }

    // non-solid: header is its own block right after the 4-byte size word
    if (!pContext->bHeaderIsCompressed || (pContext->method == NMETHOD_COPY)) {
        pContext->baHeader = read_array_process(pContext->nDataStreamOffset + 4, nHeaderSize, pPdStruct);
        return (quint32)pContext->baHeader.size() == nHeaderSize;
    }

    if (pContext->nNonSolidStartOffset > (quint32)kNsisMaxPackedBlock) {
        return false;
    }

    QByteArray baCompressed = read_array_process(pContext->nDataStreamOffset + 4, pContext->nNonSolidStartOffset, pPdStruct);
    if ((quint32)baCompressed.size() != pContext->nNonSolidStartOffset) {
        return false;
    }

    bool bOk = _decodeBlock(pContext->method, pContext->bFilterFlag, (const quint8 *)baCompressed.constData(), baCompressed.size(), nHeaderSize, true,
                            kNsisMaxHeaderDecoded, &pContext->baHeader, pPdStruct);
    return bOk && ((quint32)pContext->baHeader.size() == nHeaderSize);
}

// ---------------------------------------------------------------------------
// string helpers (operate on pContext->baHeader string table)
// ---------------------------------------------------------------------------

void XNSIS::_appendVar(QString *pRes, quint32 nIndex, const UNPACK_CONTEXT *pContext) const
{
    Q_UNUSED(pContext)
    *pRes += '$';
    if (nIndex < 20) {
        if (nIndex >= 10) {
            *pRes += 'R';
            nIndex -= 10;
        }
        *pRes += QString::number(nIndex);
    } else {
        const unsigned nNumInternal = 20 + (sizeof(kVarStrings) / sizeof(kVarStrings[0]));
        if (nIndex < nNumInternal) {
            *pRes += QString::fromLatin1(kVarStrings[nIndex - 20]);
        } else {
            *pRes += '_';
            *pRes += QString::number(nIndex - nNumInternal);
            *pRes += '_';
        }
    }
}

void XNSIS::_appendShellString(QString *pRes, unsigned nIndex1, unsigned nIndex2, const UNPACK_CONTEXT *pContext) const
{
    if ((nIndex1 & 0x80) != 0) {
        unsigned nOffset = (nIndex1 & 0x3F);
        if (nOffset >= pContext->nNumStringChars) {
            *pRes += "$_ERROR_STR_";
            return;
        }

        const quint8 *pStr = (const quint8 *)pContext->baHeader.constData() + pContext->nStringsPos;
        int nId = -1;
        QString sValue;
        if (pContext->bIsUnicode) {
            const quint8 *p = pStr + nOffset * 2;
            QString sReg;
            for (unsigned i = 0; i < 256; i++) {
                quint16 c = rd16(p + i * 2);
                if (c == 0) {
                    break;
                }
                sReg += QChar(c);
            }
            if (sReg == "ProgramFilesDir") {
                nId = 0;
            } else if (sReg == "CommonFilesDir") {
                nId = 1;
            }
            sValue = sReg;
        } else {
            const char *p = (const char *)pStr + nOffset;
            sValue = QString::fromLatin1(p);
            if (sValue == "ProgramFilesDir") {
                nId = 0;
            } else if (sValue == "CommonFilesDir") {
                nId = 1;
            }
        }

        *pRes += (nId >= 0) ? (nId == 0 ? "$PROGRAMFILES" : "$COMMONFILES") : "$_ERROR_UNSUPPORTED_VALUE_REGISTRY_";
        if ((nIndex1 & 0x40) != 0) {
            *pRes += "64";
        }
        if (nId < 0) {
            *pRes += '(';
            *pRes += sValue;
            *pRes += ')';
        }
        return;
    }

    *pRes += '$';
    if (nIndex1 < kNumShellStrings && kShellStrings[nIndex1]) {
        *pRes += QString::fromLatin1(kShellStrings[nIndex1]);
        return;
    }
    if (nIndex2 < kNumShellStrings && kShellStrings[nIndex2]) {
        *pRes += QString::fromLatin1(kShellStrings[nIndex2]);
        return;
    }
    *pRes += QString("$_ERROR_UNSUPPORTED_SHELL_[%1,%2]").arg(nIndex1).arg(nIndex2);
}

QString XNSIS::_readStringRaw(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const
{
    QString sResult;

    if ((qint32)nStrPos < 0) {
        return QString("$(LSTR_%1)").arg((quint32) - ((qint32)nStrPos + 1));
    }
    if (nStrPos >= pContext->nNumStringChars) {
        return QString("$_ERROR_STR_");
    }

    const quint8 *pData = (const quint8 *)pContext->baHeader.constData();
    const qint64 nHeaderSize = pContext->baHeader.size();
    const bool bIsPark = (pContext->nNsisType >= NSISTYPE_PARK1);
    const bool bIsNsis3 = (pContext->nNsisType == NSISTYPE_NSIS3);

    if (pContext->bIsUnicode) {
        qint64 nPos = pContext->nStringsPos + (qint64)nStrPos * 2;

        if (bIsPark) {
            for (;;) {
                if (nPos + 2 > nHeaderSize) break;
                unsigned c = rd16(pData + nPos);
                nPos += 2;
                if (c == 0) break;
                if (c < 0x80) {
                    sResult += QChar((ushort)c);
                    continue;
                }
                if (IS_PARK_SPEC_CHAR(c)) {
                    if (nPos + 2 > nHeaderSize) break;
                    unsigned n = rd16(pData + nPos);
                    nPos += 2;
                    if (n == 0) break;
                    if (c != PARK_CODE_SKIP) {
                        if (c == PARK_CODE_SHELL) {
                            _appendShellString(&sResult, n & 0xFF, n >> 8, pContext);
                        } else {
                            CONVERT_NUMBER_PARK(n);
                            if (c == PARK_CODE_VAR) {
                                _appendVar(&sResult, n, pContext);
                            } else {
                                sResult += QString("$(LSTR_%1)").arg(n);
                            }
                        }
                        continue;
                    }
                    c = n;
                }
                sResult += QChar((ushort)c);
            }
        } else {
            // NSIS-3 unicode
            for (;;) {
                if (nPos + 2 > nHeaderSize) break;
                unsigned c = rd16(pData + nPos);
                nPos += 2;
                if (c > NS_3_CODE_SKIP) {
                    sResult += QChar((ushort)c);
                    continue;
                }
                if (c == 0) break;
                if (nPos + 2 > nHeaderSize) break;
                unsigned n = rd16(pData + nPos);
                nPos += 2;
                if (n == 0) break;
                if (c == NS_3_CODE_SKIP) {
                    sResult += QChar((ushort)n);
                    continue;
                }
                if (c == NS_3_CODE_SHELL) {
                    _appendShellString(&sResult, n & 0xFF, n >> 8, pContext);
                } else {
                    CONVERT_NUMBER_NS_3_UNICODE(n);
                    if (c == NS_3_CODE_VAR) {
                        _appendVar(&sResult, n, pContext);
                    } else {
                        sResult += QString("$(LSTR_%1)").arg(n);
                    }
                }
            }
        }
        return sResult;
    }

    // ANSI
    qint64 nPos = pContext->nStringsPos + nStrPos;
    QByteArray baResult;

    if (!bIsNsis3) {
        for (;;) {
            if (nPos >= nHeaderSize) break;
            quint8 c = pData[nPos++];
            if (c == 0) break;
            if (IS_NS_SPEC_CHAR(c)) {
                if (nPos >= nHeaderSize) break;
                quint8 c0 = pData[nPos++];
                if (c0 == 0) break;
                if (c != NS_CODE_SKIP) {
                    if (nPos >= nHeaderSize) break;
                    quint8 c1 = pData[nPos++];
                    if (c1 == 0) break;
                    QString sTok;
                    if (c == NS_CODE_SHELL) {
                        _appendShellString(&sTok, c0, c1, pContext);
                    } else {
                        unsigned n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
                        if (c == NS_CODE_VAR) {
                            _appendVar(&sTok, n, pContext);
                        } else {
                            sTok = QString("$(LSTR_%1)").arg(n);
                        }
                    }
                    baResult += sTok.toLatin1();
                    continue;
                }
                c = c0;
            }
            baResult += (char)c;
        }
    } else {
        // NSIS-3 ANSI
        for (;;) {
            if (nPos >= nHeaderSize) break;
            quint8 c = pData[nPos++];
            if (c <= NS_3_CODE_SKIP) {
                if (c == 0) break;
                if (nPos >= nHeaderSize) break;
                quint8 c0 = pData[nPos++];
                if (c0 == 0) break;
                if (c != NS_3_CODE_SKIP) {
                    if (nPos >= nHeaderSize) break;
                    quint8 c1 = pData[nPos++];
                    if (c1 == 0) break;
                    QString sTok;
                    if (c == NS_3_CODE_SHELL) {
                        _appendShellString(&sTok, c0, c1, pContext);
                    } else {
                        unsigned n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
                        if (c == NS_3_CODE_VAR) {
                            _appendVar(&sTok, n, pContext);
                        } else {
                            sTok = QString("$(LSTR_%1)").arg(n);
                        }
                    }
                    baResult += sTok.toLatin1();
                    continue;
                }
                c = c0;
            }
            baResult += (char)c;
        }
    }

    return QString::fromLocal8Bit(baResult.constData(), baResult.size());
}

qint32 XNSIS::_getVarIndex(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const
{
    if (nStrPos >= pContext->nNumStringChars) {
        return -1;
    }
    const quint8 *pStr = (const quint8 *)pContext->baHeader.constData() + pContext->nStringsPos;
    const bool bIsPark = (pContext->nNsisType >= NSISTYPE_PARK1);

    if (pContext->bIsUnicode) {
        if (pContext->nNumStringChars - nStrPos < 3 * 2) {
            return -1;
        }
        const quint8 *p = pStr + (qint64)nStrPos * 2;
        unsigned code = rd16(p);
        if (bIsPark) {
            if (code != PARK_CODE_VAR) return -1;
            quint32 n = rd16(p + 2);
            if (n == 0) return -1;
            CONVERT_NUMBER_PARK(n);
            return (qint32)n;
        }
        if (code != NS_3_CODE_VAR) return -1;
        quint32 n = rd16(p + 2);
        if (n == 0) return -1;
        CONVERT_NUMBER_NS_3_UNICODE(n);
        return (qint32)n;
    }

    if (pContext->nNumStringChars - nStrPos < 4) {
        return -1;
    }
    const quint8 *p = pStr + nStrPos;
    unsigned c = p[0];
    if (pContext->nNsisType == NSISTYPE_NSIS3) {
        if (c != NS_3_CODE_VAR) return -1;
    } else if (c != NS_CODE_VAR) {
        return -1;
    }
    unsigned c0 = p[1];
    if (c0 == 0) return -1;
    unsigned c1 = p[2];
    if (c1 == 0) return -1;
    return (qint32)DECODE_NUMBER_FROM_2_CHARS(c0, c1);
}

qint32 XNSIS::_getVarIndex(const UNPACK_CONTEXT *pContext, quint32 nStrPos, quint32 *pResOffset) const
{
    *pResOffset = 0;
    qint32 nVar = _getVarIndex(pContext, nStrPos);
    if (nVar < 0) {
        return nVar;
    }
    if (pContext->bIsUnicode) {
        if (pContext->nNumStringChars - nStrPos < 2 * 2) return -1;
        *pResOffset = 2;
    } else {
        if (pContext->nNumStringChars - nStrPos < 3) return -1;
        *pResOffset = 3;
    }
    return nVar;
}

bool XNSIS::_isAbsolutePathVar(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const
{
    qint32 nVar = _getVarIndex(pContext, nStrPos);
    switch (nVar) {
        case kVar_INSTDIR:
        case kVar_EXEDIR:
        case kVar_TEMP:
        case kVar_PLUGINSDIR: return true;
    }
    return false;
}

bool XNSIS::_isGoodString(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const
{
    if (nStrPos >= pContext->nNumStringChars) {
        return false;
    }
    if (nStrPos == 0) {
        return true;
    }
    const quint8 *p = (const quint8 *)pContext->baHeader.constData() + pContext->nStringsPos;
    unsigned c;
    if (pContext->bIsUnicode) {
        c = rd16(p + (qint64)nStrPos * 2 - 2);
    } else {
        c = p[nStrPos - 1];
    }
    return (c == 0 || c == '\\');
}

quint32 XNSIS::_getCmd(const UNPACK_CONTEXT *pContext, quint32 nCmd) const
{
    Q_UNUSED(pContext)
    // Park/log command-id translation only affects opcodes >= EW_REGISTERDLL (44).
    // The opcodes we use for extraction are all below that, so identity is correct.
    return nCmd;
}

bool XNSIS::_uninstallerPatch(const QByteArray &baPatch, QByteArray *pDest)
{
    // The uninstaller data is a sequence of [len:u32][offset:u32][len bytes] edits
    // applied onto a copy of the installer's PE stub, terminated by a zero length.
    const quint8 *p = (const quint8 *)baPatch.constData();
    qint64 nSize = baPatch.size();
    quint8 *pDst = (quint8 *)pDest->data();
    const qint64 nDestSize = pDest->size();

    for (;;) {
        if (nSize < 4) {
            return false;
        }
        quint32 nLen = rd32(p);
        if (nLen == 0) {
            return (nSize == 4);
        }
        if (nSize < 8) {
            return false;
        }
        quint32 nOffs = rd32(p + 4);
        p += 8;
        nSize -= 8;
        if (((qint64)nLen > nSize) || ((qint64)nOffs > nDestSize) || ((qint64)nLen > nDestSize - (qint64)nOffs)) {
            return false;
        }
        memcpy(pDst + nOffs, p, nLen);
        p += nLen;
        nSize -= nLen;
    }
}

// ---------------------------------------------------------------------------
// header parse + instruction walk
// ---------------------------------------------------------------------------

void XNSIS::_detectNsisType(UNPACK_CONTEXT *pContext, quint32 nEntriesOffset, quint32 nEntriesNum)
{
    Q_UNUSED(nEntriesOffset)
    Q_UNUSED(nEntriesNum)

    pContext->nNsisType = NSISTYPE_NSIS2;

    const quint8 *pStr = (const quint8 *)pContext->baHeader.constData() + pContext->nStringsPos;
    const quint32 nNum = pContext->nNumStringChars;

    bool bStrongNsis = false;

    if (nNum > 2) {
        if (pContext->bIsUnicode) {
            quint32 num = nNum - 2;
            for (quint32 i = 0; i < num; i++) {
                if (rd16(pStr + (qint64)i * 2) == 0) {
                    unsigned c2 = rd16(pStr + 2 + (qint64)i * 2);
                    if (c2 == NS_3_CODE_VAR) {
                        if ((rd16(pStr + 4 + (qint64)i * 2) & 0x8080) == 0x8080) {
                            bStrongNsis = true;
                            break;
                        }
                    }
                }
            }
            pContext->nNsisType = bStrongNsis ? NSISTYPE_NSIS3 : NSISTYPE_PARK1;
        } else {
            quint32 num = nNum - 2;
            for (quint32 i = 0; i < num; i++) {
                if (pStr[i] == 0) {
                    quint8 c2 = pStr[i + 1];
                    if (c2 == NS_3_CODE_VAR) {
                        if ((pStr[i + 2] & 0x80) != 0) {
                            bStrongNsis = true;
                            break;
                        }
                    }
                }
            }
            pContext->nNsisType = bStrongNsis ? NSISTYPE_NSIS3 : NSISTYPE_NSIS2;
        }
    }
}

bool XNSIS::_parseHeader(UNPACK_CONTEXT *pContext)
{
    const quint8 *p = (const quint8 *)pContext->baHeader.constData();
    const qint64 nSize = pContext->baHeader.size();

    // detect 32/64-bit block-header layout
    bool bIs64 = false;
    if (nSize >= (qint64)(4 + 12 * 8)) {
        bIs64 = true;
        for (int k = 0; k < 8; k++) {
            if (rd32(p + 4 + 12 * k + 4) != 0) {
                bIs64 = false;
                break;
            }
        }
    }
    pContext->bIs64Bit = bIs64;

    const unsigned nBhoSize = bIs64 ? 12 : 8;
    if (nSize < (qint64)(4 + nBhoSize * 8)) {
        return false;
    }

    quint32 nEntriesOffset, nEntriesNum;
    quint32 nStringsOffset, nStringsNum;
    quint32 nLangOffset, nLangNum;
    nsisParseBlockHeader(p, nBhoSize, 2, &nEntriesOffset, &nEntriesNum);
    nsisParseBlockHeader(p, nBhoSize, 3, &nStringsOffset, &nStringsNum);
    nsisParseBlockHeader(p, nBhoSize, 4, &nLangOffset, &nLangNum);
    Q_UNUSED(nStringsNum)
    Q_UNUSED(nLangNum)

    pContext->nStringsPos = nStringsOffset;

    if (((qint64)nStringsOffset > nSize) || ((qint64)nLangOffset > nSize) || ((qint64)nEntriesOffset > nSize)) {
        return false;
    }
    if (nLangOffset < nStringsOffset) {
        return false;
    }

    quint32 nStringTableSize = nLangOffset - nStringsOffset;
    if (nStringTableSize < 2) {
        return false;
    }
    const quint8 *pStrData = p + nStringsOffset;
    if (pStrData[nStringTableSize - 1] != 0) {
        return false;
    }

    pContext->bIsUnicode = (rd16(pStrData) == 0);
    pContext->nNumStringChars = nStringTableSize;
    if (pContext->bIsUnicode) {
        if ((nStringTableSize & 1) != 0) {
            return false;
        }
        pContext->nNumStringChars >>= 1;
        if (pStrData[nStringTableSize - 2] != 0) {
            return false;
        }
    }

    if (nEntriesNum > (1u << 25)) {
        return false;
    }
    if ((qint64)nEntriesNum * kCmdSize > nSize - (qint64)nEntriesOffset) {
        return false;
    }

    _detectNsisType(pContext, nEntriesOffset, nEntriesNum);

    QStringList prefixes;
    if (!_readEntries(pContext, nEntriesOffset, nEntriesNum, &prefixes)) {
        return false;
    }

    _sortItems(pContext);

    return true;
}

bool XNSIS::_readEntries(UNPACK_CONTEXT *pContext, quint32 nEntriesOffset, quint32 nEntriesNum, QStringList *pPrefixes)
{
    const quint8 *pBase = (const quint8 *)pContext->baHeader.constData();
    const quint8 *p = pBase + nEntriesOffset;

    pPrefixes->clear();
    pPrefixes->append("$INSTDIR");

    QString sSpecOutdir;
    const quint32 nSpecOutdirVar = kVar_Spec_OUTDIR;  // NSIS 2.26+

    pContext->listEntries.clear();

    for (quint32 kkk = 0; kkk < nEntriesNum; kkk++, p += kCmdSize) {
        quint32 nCmd = _getCmd(pContext, rd32(p));
        quint32 params[kNumCommandParams];
        for (unsigned i = 0; i < kNumCommandParams; i++) {
            params[i] = rd32(p + 4 + 4 * i);
        }

        switch (nCmd) {
            case EW_CREATEDIR: {
                bool bIsSetOutPath = (params[1] != 0);
                if (bIsSetOutPath) {
                    quint32 nPar0 = params[0];
                    quint32 nResOffset = 0;
                    qint32 nIdx = _getVarIndex(pContext, nPar0, &nResOffset);
                    if ((nIdx == (qint32)nSpecOutdirVar) || (nIdx == kVar_OUTDIR)) {
                        nPar0 += nResOffset;
                    }
                    QString sRaw = _readStringRaw(pContext, nPar0);
                    if (nIdx == (qint32)nSpecOutdirVar) {
                        sRaw = sSpecOutdir + sRaw;
                    } else if (nIdx == kVar_OUTDIR) {
                        sRaw = pPrefixes->last() + sRaw;
                    }
                    pPrefixes->append(sRaw);
                }
                break;
            }

            case EW_ASSIGNVAR: {
                if (params[0] == nSpecOutdirVar) {
                    sSpecOutdir.clear();
                    quint32 nResOffset = 0;
                    qint32 nIdx = _getVarIndex(pContext, params[1], &nResOffset);
                    // IsVarStr(params[1], kVar_OUTDIR): the string is exactly "$OUTDIR"
                    bool bIsOutdir = false;
                    if (nIdx == kVar_OUTDIR) {
                        // check terminator right after the var
                        quint32 nAfter = params[1] + nResOffset;
                        if (nAfter <= pContext->nNumStringChars) {
                            QString sTail = _readStringRaw(pContext, nAfter);
                            bIsOutdir = sTail.isEmpty();
                        }
                    }
                    if (bIsOutdir && (params[2] == 0) && (params[3] == 0)) {
                        sSpecOutdir = pPrefixes->last();
                    }
                }
                break;
            }

            case EW_FOPEN: {
                // NSIS represents an explicitly created empty file as:
                //   FileOpen <handle> <name> w
                //   FileClose <handle>
                // It has no data-block record, so recognize the instruction
                // pair instead of mistaking an absent size for zero later.
                if ((params[1] == kWinGenericWrite) && (params[2] == kWinCreateAlways) && (kkk + 1 < nEntriesNum)) {
                    const quint8 *pNext = p + kCmdSize;
                    if ((_getCmd(pContext, rd32(pNext)) == EW_FCLOSE) && (rd32(pNext + 4) == params[0])) {
                        FILE_ENTRY entry = {};
                        entry.nSize = 0;
                        entry.bSizeDefined = true;
                        entry.bIsEmptyFile = true;

                        const quint32 nName = params[3];
                        const QString sName = _readStringRaw(pContext, nName);
                        const bool bIsAbs = _isAbsolutePathVar(pContext, nName) || nsisIsAbsolutePath(sName);
                        QString sPrefix;
                        bool bHasPrefix = false;
                        if (!bIsAbs) {
                            sPrefix = pPrefixes->last();
                            bHasPrefix = true;
                        }
                        entry.sFileName = nsisReducedName(sPrefix, bHasPrefix, sName);
                        entry.sPath = nsisReducedPath(sPrefix, bHasPrefix);
                        pContext->listEntries.append(entry);
                    }
                }
                break;
            }

            case EW_EXTRACTFILE: {
                FILE_ENTRY entry = {};
                entry.bSizeDefined = false;
                entry.bIsEmptyFile = false;

                quint32 nPar1 = params[1];
                QString sName = _readStringRaw(pContext, nPar1);
                bool bIsAbs = _isAbsolutePathVar(pContext, nPar1) || nsisIsAbsolutePath(sName);

                QString sPrefix;
                bool bHasPrefix = false;
                if (!bIsAbs) {
                    sPrefix = pPrefixes->last();
                    bHasPrefix = true;
                }
                entry.sFileName = nsisReducedName(sPrefix, bHasPrefix, sName);
                entry.sPath = nsisReducedPath(sPrefix, bHasPrefix);
                entry.nPos = params[2];
                entry.nMTimeLow = params[3];
                entry.nMTimeHigh = params[4];
                pContext->listEntries.append(entry);
                break;
            }

            case EW_WRITEUNINSTALLER: {
                FILE_ENTRY entry = {};
                entry.bSizeDefined = false;
                entry.bIsEmptyFile = false;

                QString sName = _readStringRaw(pContext, params[0]);
                bool bIsAbs = _isAbsolutePathVar(pContext, params[0]) || nsisIsAbsolutePath(sName);
                QString sPrefix;
                bool bHasPrefix = false;
                if (!bIsAbs) {
                    sPrefix = pPrefixes->last();
                    bHasPrefix = true;
                }
                entry.sFileName = nsisReducedName(sPrefix, bHasPrefix, sName);
                entry.sPath = nsisReducedPath(sPrefix, bHasPrefix);
                entry.nPos = params[1];
                entry.bIsUninstaller = true;
                entry.nPatchSize = params[2];
                pContext->listEntries.append(entry);
                break;
            }

            default: break;
        }
    }

    return true;
}

bool XNSIS::_fileEntryPosLess(const FILE_ENTRY &a, const FILE_ENTRY &b)
{
    if (a.nPos != b.nPos) {
        return a.nPos < b.nPos;
    }

    // Empty files have no data-block position (Pos == 0). Keep them ahead of
    // real position-zero blocks, matching NSIS's native item ordering.
    return a.bIsEmptyFile && !b.bIsEmptyFile;
}

void XNSIS::_sortItems(UNPACK_CONTEXT *pContext)
{
    QList<FILE_ENTRY> &list = pContext->listEntries;

    std::stable_sort(list.begin(), list.end(), _fileEntryPosLess);

    // drop consecutive duplicates (same position + same name)
    for (int i = 0; i + 1 < list.size();) {
        if (!list.at(i).bIsEmptyFile && !list.at(i + 1).bIsEmptyFile && (list.at(i).nPos == list.at(i + 1).nPos) && (list.at(i).sFileName == list.at(i + 1).sFileName)) {
            list.removeAt(i + 1);
        } else {
            i++;
        }
    }

    // For solid archives we already have the decoded stream: fill in real sizes so
    // infoCurrent() can report them.
    if (pContext->bIsSolid && !pContext->baSolid.isEmpty()) {
        const quint8 *pSolid = (const quint8 *)pContext->baSolid.constData();
        const qint64 nSolidSize = pContext->baSolid.size();
        for (int i = 0; i < list.size(); i++) {
            // Synthetic empty files have no solid-stream block. Patched
            // uninstallers do have a block here, but it is only the patch
            // stream; its length is not the reconstructed executable's size.
            if (list[i].bIsEmptyFile || (list[i].bIsUninstaller && (list[i].nPatchSize != 0))) {
                continue;
            }

            qint64 nAbs = 4 + (qint64)pContext->nHeaderSize + list[i].nPos;
            if ((nAbs + 4) <= nSolidSize) {
                const quint32 nSize = rd32(pSolid + nAbs);

                if ((qint64)nSize <= (nSolidSize - (nAbs + 4))) {
                    list[i].nSize = nSize;
                    list[i].bSizeDefined = true;
                }
            }
        }
    } else if (!pContext->bIsSolid && (pContext->nDataStreamOffset >= 0) && (pContext->nDataSize >= 0)) {
        // A non-solid member starts with [size|compressed-flag:u32]. The
        // payload size is always known; when the flag is clear it is also the
        // exact uncompressed size. Compressed members deliberately retain an
        // unknown uncompressed size until they are decoded.
        const quint64 nDataStart = (quint64)pContext->nDataStreamOffset;
        const quint64 nDataSize = (quint64)pContext->nDataSize;
        const qint64 nFileSize = getSize();
        if ((nFileSize >= 0) && (nDataSize <= (std::numeric_limits<quint64>::max)() - nDataStart) && (nDataStart + nDataSize <= (quint64)nFileSize)) {
            const quint64 nDataEnd = nDataStart + nDataSize;
            for (int i = 0; i < list.size(); i++) {
                if (list[i].bIsEmptyFile) {
                    continue;
                }

                quint64 nRelative = 4;
                if ((quint64)pContext->nNonSolidStartOffset > (std::numeric_limits<quint64>::max)() - nRelative) continue;
                nRelative += pContext->nNonSolidStartOffset;
                if ((quint64)list[i].nPos > (std::numeric_limits<quint64>::max)() - nRelative) continue;
                nRelative += list[i].nPos;
                if (nRelative > (std::numeric_limits<quint64>::max)() - nDataStart) continue;
                const quint64 nBlockPos = nDataStart + nRelative;
                if ((nBlockPos > nDataEnd) || ((nDataEnd - nBlockPos) < 4) || (nBlockPos > (quint64)(std::numeric_limits<qint64>::max)())) continue;

                const QByteArray baSize = read_array((qint64)nBlockPos, 4);
                if (baSize.size() != 4) continue;
                const quint32 nSizeField = rd32((const quint8 *)baSize.constData());
                const quint32 nPayloadSize = nSizeField & ~kMask_IsCompressed;
                if ((quint64)nPayloadSize > nDataEnd - (nBlockPos + 4)) continue;

                list[i].bIsCompressed = ((nSizeField & kMask_IsCompressed) != 0);
                list[i].nCompressedSize = nPayloadSize;
                list[i].bCompressedSizeDefined = true;

                if (!list[i].bIsCompressed && (!list[i].bIsUninstaller || (list[i].nPatchSize == 0))) {
                    list[i].nSize = nPayloadSize;
                    list[i].bSizeDefined = true;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// open
// ---------------------------------------------------------------------------

bool XNSIS::_open(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    // Compressed-data size = ArcSize - firstheader(0x1C) - CRC(4 if present)
    bool bThereIsCrc = ((pContext->nFlags & 8 /*ForceCrc*/) != 0) || ((pContext->nFlags & 4 /*NoCrc*/) == 0);
    qint64 nAvail = getSize() - pContext->nDataStreamOffset;
    qint64 nDataSize = (qint64)pContext->nArchiveSize - 0x1C - (bThereIsCrc ? 4 : 0);
    if (nDataSize > nAvail) {
        nDataSize = nAvail;
    }
    if (nDataSize <= 0) {
        return false;
    }
    pContext->nDataOffset = pContext->nDataStreamOffset;
    pContext->nDataSize = nDataSize;

    QByteArray baSig = read_array_process(pContext->nDataStreamOffset, (std::min)((qint64)64, nDataSize), pPdStruct);
    if (baSig.size() < 12) {
        return false;
    }

    if (!_detectMethod(pContext, (const quint8 *)baSig.constData(), baSig.size())) {
        return false;
    }

    if (!_decodeHeader(pContext, pPdStruct)) {
        return false;
    }

    if (!_parseHeader(pContext)) {
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// streaming API
// ---------------------------------------------------------------------------

QMap<XBinary::UNPACK_PROP, QVariant> XNSIS::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    return result;
}

bool XNSIS::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    NsisOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pLifetime->setContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        pLifetime->setContexts.remove(pOldContext);
        pState->pContext = nullptr;
        deleteUnpackContext(pOldContext);
        *pState = UNPACK_STATE();
        if (!pLifetime->bOwnerAlive) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;

    QIODevice *guardedSource = getDevice();
    XArchive *pSourceValidator = new XArchive(guardedSource);
    UNPACK_STATE sourceValidationState = {};
    if (!pSourceValidator->bindUnpackSource(&sourceValidationState, pPdStruct)) {
        pSourceValidator->releaseUnpackSource(&sourceValidationState);
        delete pSourceValidator;
        return false;
    }
    XNSIS detector(guardedSource, isImage(), getModuleAddress());
    const qint64 nTotalSize = guardedSource->size();
    INTERNAL_INFO info = detector._analyse(pPdStruct);
    if ((nTotalSize < 0) || !info.bIsValid || !pSourceValidator->isUnpackSourceCurrent(&sourceValidationState, pPdStruct)) {
        pSourceValidator->releaseUnpackSource(&sourceValidationState);
        delete pSourceValidator;
        return false;
    }

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT();
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pSourceValidator = pSourceValidator;
    pContext->sourceValidationState = UNPACK_STATE();
    if (!pContext->pSourceValidator->transferUnpackSourceOwnership(&sourceValidationState, &pContext->sourceValidationState)) {
        pContext->pSourceValidator->releaseUnpackSource(&sourceValidationState);
        deleteUnpackContext(pContext);
        return false;
    }
    pContext->nFirstHeaderOffset = 0;
    pContext->nDataStreamOffset = 0;
    pContext->nFlags = 0;
    pContext->nHeaderSize = 0;
    pContext->nArchiveSize = 0;
    pContext->method = NMETHOD_DEFLATE;
    pContext->compressMethod = HANDLE_METHOD_UNKNOWN;
    pContext->bIsSolid = false;
    pContext->bFilterFlag = false;
    pContext->bHeaderIsCompressed = true;
    pContext->nNonSolidStartOffset = 0;
    pContext->nStringsPos = 0;
    pContext->nNumStringChars = 0;
    pContext->bIsUnicode = false;
    pContext->bIs64Bit = false;
    pContext->nNsisType = NSISTYPE_NSIS2;
    pContext->bIsNsis225 = false;
    pContext->bSolidDecoded = false;
    pContext->nDataOffset = 0;
    pContext->nDataSize = 0;

    if (!detector._findFirstHeader(info.nSignatureOffset, nTotalSize, pContext, pPdStruct) || detector._open(pContext, pPdStruct) == false || !isPdStructNotCanceled(pPdStruct) || pContext->listEntries.isEmpty() ||
        !pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct)) {
        deleteUnpackContext(pContext);
        return false;
    }

    pState->nTotalSize = nTotalSize;
    pState->nNumberOfRecords = pContext->listEntries.size();
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    pLifetime->setContexts.insert(pContext);

    return true;
}

XBinary::ARCHIVERECORD XNSIS::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    NsisOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return result;
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !isPdStructNotCanceled(pPdStruct)) return result;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    qint32 nIndex = pState->nCurrentIndex;
    if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pState->nNumberOfRecords != pContext->listEntries.size()) || (nIndex < 0) || (nIndex >= pContext->listEntries.size()) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext))
        return result;

    const FILE_ENTRY &entry = pContext->listEntries.at(nIndex);

    result.mapProperties[FPART_PROP_ORIGINALNAME] = entry.sFileName.isEmpty() ? QString("file_%1").arg(nIndex, 4, 10, QChar('0')) : entry.sFileName;
    // Native SetOutPath directory (kept separate from the full name; surfaced only in advanced mode).
    if (!entry.sPath.isEmpty()) {
        result.mapProperties[FPART_PROP_OPTIONAL_PATH] = entry.sPath;
    }
    if (entry.bSizeDefined) {
        result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)entry.nSize;
    }
    if (entry.bIsEmptyFile) {
        result.nStreamOffset = 0;
        result.nStreamSize = 0;
        result.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)0;
    } else {
        result.nStreamOffset = pContext->nDataOffset;
        result.nStreamSize = pContext->nDataSize;
    }
    if (!entry.bIsEmptyFile && entry.bCompressedSizeDefined) {
        result.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)entry.nCompressedSize;
    }
    result.mapProperties[FPART_PROP_HANDLEMETHOD] = entry.bIsEmptyFile ? HANDLE_METHOD_STORE : pContext->compressMethod;
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    result.mapProperties[FPART_PROP_ISSOLID] = entry.bIsEmptyFile ? false : pContext->bIsSolid;

    return result;
}

static bool nsisFailOutput(bool bSeekableOutput, QIODevice *pDevice, XBinary::UNPACK_STATE *pState)
{
    if (bSeekableOutput) {
        XBinary::resize(pDevice, 0);
        pDevice->seek(0);
    }
    pState->nCurrentOffset = 0;
    return false;
}

bool XNSIS::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    NsisOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    QIODevice *guardedOutput = pDevice;
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isResizeEnable(guardedOutput) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    qint32 nIndex = pState->nCurrentIndex;

    if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pState->nNumberOfRecords != pContext->listEntries.size()) || (nIndex < 0) || (nIndex >= pContext->listEntries.size()) ||
        XBinary::devicesAlias(pContext->pOuterSourceDevice, guardedOutput) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    const FILE_ENTRY &entry = pContext->listEntries.at(nIndex);
    QTemporaryFile stage;
    if (!stage.open()) return false;
    UNPACK_STATE stageState = *pState;
    stageState.baUnpackSourceToken.clear();
    stageState.pContext = nullptr;
    const bool bOuterIsImage = isImage();
    const XADDR nOuterModuleAddress = getModuleAddress();
    XNSIS decoder(pContext->pOuterSourceDevice, bOuterIsImage, nOuterModuleAddress);

    const NsisContextValidator contextValidator(this, guardedOutput, pLifetime, pContext, pState);

    // Obtain the item's raw (uncompressed) data.
    QByteArray baData;
    QByteArray baSolidTail;  // for a patched uninstaller: the block that follows the patch block

    if (entry.bIsEmptyFile) {
        baData.clear();
    } else if (pContext->bIsSolid) {
        if (!decoder._decodeSolidStream(pContext, pPdStruct) || !contextValidator.isCurrent()) {
            return false;
        }
        const qint64 nSolidSize = pContext->baSolid.size();
        const quint8 *pSolid = (const quint8 *)pContext->baSolid.constData();

        qint64 nAbs = 4 + (qint64)pContext->nHeaderSize + entry.nPos;
        if ((nAbs + 4) > nSolidSize) {
            return false;
        }
        quint32 nSize = rd32(pSolid + nAbs);
        if (nSize == 0) {
            if (entry.bSizeDefined && (entry.nSize != 0)) return false;
            baData.clear();
        }
        if ((qint64)(nAbs + 4 + nSize) > nSolidSize) {
            return false;
        }
        baData = QByteArray((const char *)pSolid + nAbs + 4, (int)nSize);

        // A patched uninstaller is stored as two consecutive blocks: the patch, then
        // the uninstaller's own stub archive appended after the patched PE stub.
        if (entry.bIsUninstaller && (entry.nPatchSize != 0)) {
            if ((quint64)baData.size() != entry.nPatchSize) return false;
            qint64 nAbsB = nAbs + 4 + nSize;
            if ((nAbsB < 0) || ((nAbsB + 4) > nSolidSize)) return false;
            const quint32 nSizeB = rd32(pSolid + nAbsB);
            if ((nSizeB == 0) || ((qint64)nSizeB > (nSolidSize - (nAbsB + 4)))) return false;
            baSolidTail = QByteArray((const char *)pSolid + nAbsB + 4, (int)nSizeB);
        }
    } else {
        // non-solid: [4-byte size|flag][block]
        const qint64 nFileSize = decoder.getSize();
        if (!contextValidator.isCurrent()) return false;
        if ((pContext->nDataStreamOffset < 0) || (pContext->nDataSize < 0) || (nFileSize < 0)) return false;

        const quint64 nDataStart = (quint64)pContext->nDataStreamOffset;
        const quint64 nDataSize = (quint64)pContext->nDataSize;
        if ((nDataSize > (std::numeric_limits<quint64>::max)() - nDataStart) || (nDataStart + nDataSize > (quint64)nFileSize)) return false;
        const quint64 nDataEnd = nDataStart + nDataSize;

        quint64 nItemRelative = 4;
        if ((quint64)pContext->nNonSolidStartOffset > (std::numeric_limits<quint64>::max)() - nItemRelative) return false;
        nItemRelative += pContext->nNonSolidStartOffset;
        if ((quint64)entry.nPos > (std::numeric_limits<quint64>::max)() - nItemRelative) return false;
        nItemRelative += entry.nPos;
        if (nItemRelative > (std::numeric_limits<quint64>::max)() - nDataStart) return false;
        const quint64 nItemPos = nDataStart + nItemRelative;

        // A patched non-solid uninstaller has a second independently framed
        // block immediately after its patch block. Decode the one or two
        // consecutive blocks with the same bounded framing walk.
        const bool bHasSecondBlock = entry.bIsUninstaller && (entry.nPatchSize != 0);
        const qint32 nBlockCount = bHasSecondBlock ? 2 : 1;
        quint64 nBlockPos = nItemPos;
        for (qint32 nBlockIndex = 0; nBlockIndex < nBlockCount; nBlockIndex++) {
            QByteArray *pOutput = (nBlockIndex == 0) ? &baData : &baSolidTail;
            if ((nBlockPos > nDataEnd) || ((nDataEnd - nBlockPos) < 4) || (nBlockPos > (quint64)(std::numeric_limits<qint64>::max)())) return false;

            const quint32 nSizeField = decoder.read_uint32((qint64)nBlockPos, false);
            if (!contextValidator.isCurrent()) return false;
            const bool bIsCompressed = (nSizeField & kMask_IsCompressed) != 0;
            const quint32 nSize = nSizeField & ~kMask_IsCompressed;
            const quint64 nPayloadPos = nBlockPos + 4;
            if ((nSize > (quint32)kNsisMaxPackedBlock) || ((quint64)nSize > nDataEnd - nPayloadPos)) return false;

            QByteArray baBlock = decoder.read_array_process((qint64)nPayloadPos, nSize, pPdStruct);
            if (!contextValidator.isCurrent() || ((quint32)baBlock.size() != nSize)) return false;

            if (!bIsCompressed) {
                *pOutput = baBlock;
            } else if (!decoder._decodeBlock(pContext->method, pContext->bFilterFlag, (const quint8 *)baBlock.constData(), baBlock.size(), 0, false,
                                             kNsisMaxMemberDecoded, pOutput, pPdStruct) ||
                       !contextValidator.isCurrent()) {
                return false;
            }

            nBlockPos = nPayloadPos + nSize;
            if (nBlockIndex == 0) {
                if (entry.bSizeDefined && (entry.nSize != (quint64)baData.size())) return false;
                if (bHasSecondBlock && ((quint64)baData.size() != entry.nPatchSize)) return false;
            }
        }
    }

    if (!entry.bIsUninstaller && entry.bSizeDefined && ((quint64)baData.size() != entry.nSize)) {
        return false;
    }

    // Patched uninstaller: apply the patch stream onto a copy of the installer's PE stub,
    // then append the following block (the uninstaller's own stub archive).
    if (entry.bIsUninstaller && (entry.nPatchSize != 0)) {
        if ((pContext->nFirstHeaderOffset <= 0) || (pContext->nFirstHeaderOffset > kNsisMaxMemberDecoded)) return false;
        QByteArray baStub = decoder.read_array_process(0, pContext->nFirstHeaderOffset, pPdStruct);
        if (!contextValidator.isCurrent()) return false;
        if ((baStub.size() != pContext->nFirstHeaderOffset) || ((qint64)baStub.size() > kNsisMaxMemberDecoded - (qint64)baSolidTail.size())) return false;

        QByteArray baPatched = baStub;
        if (!decoder._uninstallerPatch(baData, &baPatched) || ((qint64)baPatched.size() > kNsisMaxMemberDecoded - (qint64)baSolidTail.size())) return false;
        baPatched.append(baSolidTail);
        baData = baPatched;
    }

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here. Produced bytes are charged once by writeUnpackData
    // through stageState; the publish copy of the stage stays uncharged.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    if (!writeUnpackData(&stageState, &stage, baData, pPdStruct) || !contextValidator.isCurrent()) return false;
    if (!pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext))
        return false;
    NsisPublisher publisher(pContext->pOuterSourceDevice);
    UNPACK_STATE publicationState = {};
    if (!publisher.bindUnpackSource(&publicationState, pPdStruct) || !publisher.validateAndFinalizeUnpackSource(&publicationState, pPdStruct) || !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !publisher.publishUnpackOutput(&stage, guardedOutput, &publicationState, pPdStruct) || !contextValidator.isCurrent())
        return false;
    pState->nCurrentOffset = stage.size();
    return true;
}

bool XNSIS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    NsisOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pState->nNumberOfRecords != pContext->listEntries.size()) || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext))
        return false;
    pState->nCurrentIndex++;
    pState->nCurrentOffset = 0;
    const bool bCurrent = pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct);
    return bCurrent && pLifetime->bOwnerAlive && pLifetime->setContexts.contains(pContext) && (pState->pContext == pContext) &&
           (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XNSIS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    NsisOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        pLifetime->setContexts.remove(pContext);
        pState->pContext = nullptr;
        deleteUnpackContext(pContext);
    }
    *pState = UNPACK_STATE();
    return true;
}
