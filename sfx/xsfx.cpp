/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsfx.h"

#include <QBuffer>
#include <QDir>
#include <QSet>
#include <QTemporaryFile>

#include <limits>
#include <new>

#include "subdevice.h"
#include "xatarist.h"
#include "xcom.h"
#include "xelf.h"
#include "xmsdos.h"
#include "xne.h"
#include "xpe.h"
#include "xarj.h"
#include "xfreearc.h"
#include "xgzip.h"
#include "xkwaj.h"
#include "xlha.h"
#include "xdearkarchive.h"
#include "xsevenzip.h"
#include "xrar.h"
#include "xcab.h"
#include "xseaarc.h"
#include "xszdd.h"
#include "xzpaq.h"
#include "xzip.h"
#include "xconcatziparchive.h"
#include "xwinimageziparchive.h"
#include "xpyinstallercarchive.h"
#include "xarq.h"
#include "xsqz.h"
#include "xrtpatch.h"
#include "xrta.h"
#include "xbzip2.h"
#include "xbsn.h"
#include "xtgcfarchive.h"
#include "xace.h"
#include "xasymetrix.h"

// A modeled ZPAQ segment can legitimately be much larger than the overlay
// signature window, so validation must be allowed to stream to EOF.  Keep the
// results of those four-zero terminator searches in absolute file coordinates:
// every later query either reuses a known result or searches only an uncovered
// gap.  The input extent is therefore the shared byte budget instead of an
// arbitrary per-candidate size cap.
struct XSFX_ZPAQ_SCAN_CACHE {
    enum {
        MAX_INTERVALS = 4096
    };

    QIODevice *pDevice = nullptr;
    qint64 nDeviceSize = -1;
    qint64 nNoTerminatorFrom = -1;
    QMap<qint64, qint64> mapTerminatorBySearchStart;

    void bind(QIODevice *pNewDevice, qint64 nNewDeviceSize)
    {
        if ((pDevice != pNewDevice) || (nDeviceSize != nNewDeviceSize)) {
            pDevice = pNewDevice;
            nDeviceSize = nNewDeviceSize;
            nNoTerminatorFrom = -1;
            mapTerminatorBySearchStart.clear();
        }
    }

    qint64 findTerminator(XBinary *pBinary, qint64 nStart, qint64 nEnd, XBinary::PDSTRUCT *pPdStruct)
    {
        if (!pBinary || !pDevice || (pBinary->getDevice() != pDevice) || (nDeviceSize < 0) || (nStart < 0) || (nStart >= nEnd) || (nEnd != nDeviceSize) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return -1;
        }
        if ((nNoTerminatorFrom >= 0) && (nStart >= nNoTerminatorFrom)) {
            return -1;
        }

        // A predecessor covers all search starts through its first match, or
        // through EOF when it proved that no terminator remains.
        QMap<qint64, qint64>::const_iterator itUpper = mapTerminatorBySearchStart.upperBound(nStart);
        if (itUpper != mapTerminatorBySearchStart.constBegin()) {
            QMap<qint64, qint64>::const_iterator itPrevious = itUpper;
            --itPrevious;
            const qint64 nPreviousResult = itPrevious.value();
            const qint64 nPreviousEnd = (nPreviousResult >= 0) ? nPreviousResult : nEnd;
            if (nStart <= nPreviousEnd) return nPreviousResult;
        }

        // Stop at the next cached interval. Three bytes of look-ahead are
        // sufficient to catch a four-byte signature crossing that boundary.
        const QMap<qint64, qint64>::const_iterator itNext = mapTerminatorBySearchStart.lowerBound(nStart);
        const bool bHasNextInterval = (itNext != mapTerminatorBySearchStart.constEnd()) && (itNext.key() < nEnd);
        qint64 nNextStart = bHasNextInterval ? itNext.key() : nEnd;
        qint64 nNextResult = bHasNextInterval ? itNext.value() : -1;
        bool bHasNext = bHasNextInterval;
        if ((nNoTerminatorFrom >= 0) && (!bHasNext || (nNoTerminatorFrom < nNextStart))) {
            nNextStart = nNoTerminatorFrom;
            nNextResult = -1;
            bHasNext = true;
        }
        const qint64 nSearchEnd = bHasNext ? ((nNextStart > nEnd - 3) ? nEnd : nNextStart + 3) : nEnd;
        const qint64 nFound = pBinary->find_signature(nStart, nSearchEnd - nStart, "00000000", nullptr, pPdStruct);
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return -1;

        qint64 nResult = -1;
        if ((nFound >= nStart) && (!bHasNext || (nFound < nNextStart))) {
            nResult = nFound;
        } else if (bHasNext) {
            nResult = nNextResult;
        }
        if (nResult < 0) {
            nNoTerminatorFrom = (nNoTerminatorFrom < 0) ? nStart : qMin(nNoTerminatorFrom, nStart);
        }
        // A valid archive may contain very many tiny segments. Keep the cache
        // itself bounded; once full, later segment scans are still monotonic
        // within that candidate and do not require retained interval state.
        if (mapTerminatorBySearchStart.size() < MAX_INTERVALS) {
            mapTerminatorBySearchStart.insert(nStart, nResult);
        }
        return nResult;
    }
};

// Local FreeArc footer verification is deliberately bounded across all
// candidates in one SFX scan. A shared tuple cache avoids decoding the same
// corrupt control stream repeatedly, while the aggregate byte budget bounds
// distinct decoys as well.
struct XSFX_FREEARC_SCAN_CACHE {
    enum {
        MAX_RESULTS = 256
    };

    QIODevice *pDevice = nullptr;
    qint64 nDeviceSize = -1;
    qint64 nRemainingOutput = 128LL * 1024 * 1024;
    QHash<QString, qint32> mapStatus;
    QHash<QString, qint64> mapExpectedCandidateOffset;

    void bind(QIODevice *pNewDevice, qint64 nNewDeviceSize)
    {
        if ((pDevice != pNewDevice) || (nDeviceSize != nNewDeviceSize)) {
            pDevice = pNewDevice;
            nDeviceSize = nNewDeviceSize;
            nRemainingOutput = 128LL * 1024 * 1024;
            mapStatus.clear();
            mapExpectedCandidateOffset.clear();
        }
    }

    bool lookup(const QString &sKey, qint32 *pnStatus) const
    {
        const QHash<QString, qint32>::const_iterator it = mapStatus.constFind(sKey);
        if (it == mapStatus.constEnd()) return false;
        if (pnStatus) *pnStatus = it.value();
        return true;
    }

    void remember(const QString &sKey, qint32 nStatus)
    {
        if ((mapStatus.size() < MAX_RESULTS) || mapStatus.contains(sKey)) {
            mapStatus.insert(sKey, nStatus);
        }
    }

    bool lookupExpectedCandidate(const QString &sKey, qint64 *pnCandidateOffset) const
    {
        const QHash<QString, qint64>::const_iterator it = mapExpectedCandidateOffset.constFind(sKey);
        if (it == mapExpectedCandidateOffset.constEnd()) return false;
        if (pnCandidateOffset) *pnCandidateOffset = it.value();
        return true;
    }

    void rememberExpectedCandidate(const QString &sKey, qint64 nCandidateOffset)
    {
        if ((mapExpectedCandidateOffset.size() < MAX_RESULTS) || mapExpectedCandidateOffset.contains(sKey)) {
            mapExpectedCandidateOffset.insert(sKey, nCandidateOffset);
        }
    }

    bool charge(qint64 nOutputSize)
    {
        if ((nOutputSize < 0) || (nOutputSize > nRemainingOutput)) return false;
        nRemainingOutput -= nOutputSize;
        return true;
    }
};

namespace {

class SFX_OPERATION_STATE_DELETER {
public:
    explicit SFX_OPERATION_STATE_DELETER(const QSharedPointer<XSFX::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XSFX::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

const qint64 SFX_OVERLAY_SCAN_LIMIT = 16LL * 1024 * 1024;
const qint32 SFX_ELF_TABLE_LIMIT = 4096;
const qint64 SFX_ZPAQ_METADATA_SCAN_LIMIT = 1024 * 1024;
const qint32 SFX_SIGNATURE_CANDIDATE_LIMIT = 256;
const qint64 SFX_ATARIST_EMBEDDED_PREFIX_LIMIT = 64LL * 1024;
const qint64 SFX_ATARIST_TRAILER_LIMIT = 64LL * 1024;
// FreeArc's aSCAN_MAX permits descriptors (including long method chains) up
// to 4096 bytes. Read the same bounded window so valid encrypted/composed
// footers are not rejected merely because their method descriptor exceeds 1 KiB.
const qint64 SFX_FREEARC_DESCRIPTOR_SCAN_LIMIT = 4096;
const qint64 SFX_FREEARC_HEADER_SIZE = 8;
const qint64 SFX_FREEARC_CONTROL_OUTPUT_LIMIT = 64LL * 1024 * 1024;
const quint64 SFX_FREEARC_LZMA_DICTIONARY_LIMIT = 64ULL * 1024 * 1024;
const qint64 SFX_FREEARC_MEMORY_LIMIT = 192LL * 1024 * 1024;
const quint64 SFX_FREEARC_CONTROL_BLOCK_LIMIT = 4096;
const QByteArray SFX_ZPAQFRANZ_START_TAG("rVVboBqlhbQksmjLfITQlKVxMB8oUiezUpip3End", 40);
const QByteArray SFX_ZPAQFRANZ_END_TAG("oOEik4pAXOyDLNTQ7zG2Jtc4eX5N0ESHsP6ApUzx", 40);

// Knowledge Dynamics Corp ".RED" (the container behind their INSTALL /
// wINSTALL "WinSFX" stubs). A member is one 41-byte header immediately
// followed by its stored bytes, and the headers are chained by that length
// alone: there is no directory and no end marker. Byte 2 is the record
// version, byte 3 the base header size, and the final two bytes a big-endian
// CRC-16/CCITT over the 37 bytes between them, so a CRC residue of zero over
// bytes 2..40 authenticates the whole header at once.
const qint64 SFX_RED_HEADER_SIZE = 41;
// A chain with no directory can only be validated by walking it, and a hostile
// or corrupt carrier can present an arbitrarily long run of CRC-clean headers,
// so both the length of one walk and the total number of headers the locator
// may read across all of its candidates are bounded. The longest real chain
// anywhere in the corpora is 151 members, the longest inside an executable
// carrier is 2, and the most headers a real carrier makes the locator read is
// 34, so neither cap can reject a genuine archive.
const qint32 SFX_RED_MEMBER_LIMIT = 4096;
const qint32 SFX_RED_SCAN_HEADER_BUDGET = 4096;

quint16 getRedHeaderCrc(const quint8 *pData, qint32 nSize)
{
    quint16 nCrc = 0xffff;

    for (qint32 i = 0; i < nSize; i++) {
        nCrc = (quint16)(nCrc ^ ((quint16)pData[i] << 8));
        for (qint32 j = 0; j < 8; j++) {
            if (nCrc & 0x8000) {
                nCrc = (quint16)((quint16)(nCrc << 1) ^ 0x1021);
            } else {
                nCrc = (quint16)(nCrc << 1);
            }
        }
    }

    return nCrc;
}

// True when an authentic ".RED" member chain begins at nStart and covers
// [nStart, nEnd) exactly. Only the version-1 record is accepted: it is the one
// the bounded legacy decoder behind FT_DEARK_LEGACY_ARCHIVE implements, and the only
// one present in the reference carriers.
//
// pnHeaderBudget, when supplied, is a caller-owned allowance shared by every
// walk that caller performs. Each header read spends one unit and the walk
// fails as soon as the allowance is gone, so a caller that tries many
// candidates cannot multiply the per-walk cap by the candidate cap.
bool hasRedMemberChain(XBinary *pOuter, qint64 nStart, qint64 nEnd, XBinary::PDSTRUCT *pPdStruct, qint32 *pnHeaderBudget)
{
    if (!pOuter || (nStart < 0) || (nEnd <= nStart)) return false;

    qint64 nCurrent = nStart;
    qint32 nMemberCount = 0;

    while ((nCurrent < nEnd) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (((nEnd - nCurrent) < SFX_RED_HEADER_SIZE) || (nMemberCount >= SFX_RED_MEMBER_LIMIT)) return false;

        if (pnHeaderBudget) {
            if (*pnHeaderBudget <= 0) return false;
            (*pnHeaderBudget)--;
        }

        const QByteArray baHeader = pOuter->read_array_process(nCurrent, SFX_RED_HEADER_SIZE, pPdStruct);
        if (baHeader.size() != SFX_RED_HEADER_SIZE) return false;

        const quint8 *pHeader = (const quint8 *)baHeader.constData();
        if ((pHeader[0] != 0x52) || (pHeader[1] != 0x52) || (pHeader[2] != 0x01) || (pHeader[3] != 0x29)) return false;
        if (getRedHeaderCrc(pHeader + 2, (qint32)(SFX_RED_HEADER_SIZE - 2)) != 0) return false;

        // Multi-volume members carry a non-zero fragment index or a cleared
        // last-fragment flag; neither the reference implementation nor the
        // decoder this locator hands the payload to reassembles them.
        const quint16 nFragmentIndex = XBinary::_read_uint16(baHeader.constData() + 20);
        const quint16 nLastFragment = XBinary::_read_uint16(baHeader.constData() + 22);
        if ((nFragmentIndex != 0) || (nLastFragment != 1)) return false;

        const quint16 nMethod = XBinary::_read_uint16(baHeader.constData() + 24);
        if ((nMethod != 1) && (nMethod != 9) && (nMethod != 11)) return false;

        const qint32 nPackedSize = (qint32)XBinary::_read_uint32(baHeader.constData() + 8);
        const qint32 nOriginalSize = (qint32)XBinary::_read_uint32(baHeader.constData() + 12);
        if ((nPackedSize < 0) || (nOriginalSize < 0)) return false;
        if ((qint64)nPackedSize > (nEnd - nCurrent - SFX_RED_HEADER_SIZE)) return false;

        nCurrent += SFX_RED_HEADER_SIZE + (qint64)nPackedSize;
        nMemberCount++;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct) && (nMemberCount > 0) && (nCurrent == nEnd);
}

// True when the header at nOffset is an authentic ".RED" record that belongs to
// a multi-volume set. Such a record can never begin a locatable archive, and
// neither can the records that follow it in the same volume, so the scan has to
// stop rather than advance to the next member and lock onto the tail of a split
// archive - which would publish only its final members and silently drop the
// leading ones.
bool isRedFragmentHeader(XBinary *pOuter, qint64 nOffset, XBinary::PDSTRUCT *pPdStruct, qint32 *pnHeaderBudget)
{
    if (!pOuter) return false;

    if (pnHeaderBudget) {
        if (*pnHeaderBudget <= 0) return false;
        (*pnHeaderBudget)--;
    }

    const QByteArray baHeader = pOuter->read_array_process(nOffset, SFX_RED_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != SFX_RED_HEADER_SIZE) return false;

    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    if ((pHeader[0] != 0x52) || (pHeader[1] != 0x52) || (pHeader[2] != 0x01) || (pHeader[3] != 0x29)) return false;
    if (getRedHeaderCrc(pHeader + 2, (qint32)(SFX_RED_HEADER_SIZE - 2)) != 0) return false;

    const quint16 nFragmentIndex = XBinary::_read_uint16(baHeader.constData() + 20);
    const quint16 nLastFragment = XBinary::_read_uint16(baHeader.constData() + 22);

    return (nFragmentIndex != 0) || (nLastFragment != 1);
}

// The ".RED" payload of a Knowledge Dynamics self-extractor has no locator
// field, and the 16-bit builders store a SECOND, loader-owned RED blob (the
// installation engine, "I.EXE") ahead of it whose chain deliberately stops
// short of end of file. The payload is therefore the earliest candidate whose
// complete member chain ends exactly at end of file; the engine blob can never
// satisfy that and is never selected.
//
// Every axis of the search is bounded. The signature sweep covers at most
// nScanLimit bytes from nScanStart, where the caller passes the retry loop's
// minimum archive offset so that a carrier re-probed for a second payload does
// not rescan a region whose offsets it would reject anyway. The candidate count
// stops at SFX_SIGNATURE_CANDIDATE_LIMIT, and every header read by every walk
// draws on one shared SFX_RED_SCAN_HEADER_BUDGET: without that last bound a
// carrier made of nothing but CRC-clean headers costs candidates x members
// device reads, which is minutes rather than milliseconds.
qint64 getRedSfxCandidate(XBinary *pOuter, qint64 nScanStart, qint64 nScanLimit, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOuter || !pOuter->getDevice() || !XBinary::isPdStructNotCanceled(pPdStruct)) return -1;

    const qint64 nFileSize = pOuter->getSize();
    if ((nFileSize < SFX_RED_HEADER_SIZE) || (nScanLimit <= 0)) return -1;

    qint64 nCurrent = qMax((qint64)0, nScanStart);
    if (nCurrent > (nFileSize - SFX_RED_HEADER_SIZE)) return -1;

    const qint64 nScanEnd = qMin(nFileSize, nCurrent + nScanLimit);
    qint32 nCandidateCount = 0;
    qint32 nHeaderBudget = SFX_RED_SCAN_HEADER_BUDGET;

    while ((nCurrent < nScanEnd) && (nCandidateCount < SFX_SIGNATURE_CANDIDATE_LIMIT) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nFound = pOuter->find_signature(nCurrent, nScanEnd - nCurrent, "52520129", nullptr, pPdStruct);
        if ((nFound < nCurrent) || (nFound > (nFileSize - SFX_RED_HEADER_SIZE))) break;
        nCandidateCount++;
        if (isRedFragmentHeader(pOuter, nFound, pPdStruct, &nHeaderBudget)) return -1;
        if (hasRedMemberChain(pOuter, nFound, nFileSize, pPdStruct, &nHeaderBudget)) return nFound;
        if (nHeaderBudget <= 0) return -1;
        nCurrent = nFound + 1;
    }

    return -1;
}

// ZIP carries an authoritative footer within the final 64 KiB. Derive the
// first local record from it instead of linearly searching as much as 16 MiB
// of executable image/overlay. This also reaches large PKSFX payloads whose
// first local header lies beyond the generic scan window.
qint64 getZipSfxCandidate(XBinary *pOuter, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOuter || !pOuter->getDevice() || !XBinary::isPdStructNotCanceled(pPdStruct)) return -1;

    const qint64 nFileSize = pOuter->getSize();
    if (nFileSize < (qint64)sizeof(XZip::ENDOFCENTRALDIRECTORYRECORD)) return -1;

    // XZip deliberately interprets offsets against its complete device.  A
    // traditional SFX stores them relative to the appended ZIP instead, so a
    // full-file XZip probe must remain invalid.  Locate EOCD candidates in the
    // format-defined 64 KiB tail, derive the one possible archive prefix, and
    // authenticate the complete structure through a zero-based SubDevice.
    const qint64 nMaximumSearchSize = 0xffff +
                                      (qint64)sizeof(XZip::ENDOFCENTRALDIRECTORYRECORD);
    const qint64 nSearchOffset = qMax((qint64)0, nFileSize - nMaximumSearchSize);
    const qint64 nSearchSize = nFileSize - nSearchOffset;
    const QByteArray baTail = pOuter->read_array_process(nSearchOffset, nSearchSize, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (baTail.size() != nSearchSize)) return -1;

    static const QByteArray baEcdSignature("PK\x05\x06", 4);
    qint32 nCandidatePosition = baTail.lastIndexOf(baEcdSignature);
    qint32 nCandidateCount = 0;
    while ((nCandidatePosition >= 0) &&
           (nCandidateCount < SFX_SIGNATURE_CANDIDATE_LIMIT) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        ++nCandidateCount;
        const qint64 nEcdOffset = nSearchOffset + nCandidatePosition;
        const qint64 nEcdAvailable = nFileSize - nEcdOffset;
        if (nEcdAvailable >= (qint64)sizeof(XZip::ENDOFCENTRALDIRECTORYRECORD)) {
            const char *pEcd = baTail.constData() + nCandidatePosition;
            const quint16 nDisk = XBinary::_read_uint16(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nDiskNumber));
            const quint16 nCentralDisk = XBinary::_read_uint16(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nStartDisk));
            const quint16 nDiskRecords = XBinary::_read_uint16(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nDiskNumberOfRecords));
            const quint16 nRecords = XBinary::_read_uint16(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));
            const quint32 nCentralSize = XBinary::_read_uint32(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nSizeOfCentralDirectory));
            const quint32 nStoredCentralOffset = XBinary::_read_uint32(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));
            const quint16 nCommentSize = XBinary::_read_uint16(
                const_cast<char *>(pEcd) + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nCommentLength));

            const qint64 nEcdSize = (qint64)sizeof(XZip::ENDOFCENTRALDIRECTORYRECORD) + nCommentSize;
            const bool bBasicLayout = (nDisk == 0) && (nCentralDisk == 0) &&
                (nDiskRecords == nRecords) && (nRecords != 0xffff) &&
                (nCentralSize != 0xffffffffU) &&
                (nStoredCentralOffset != 0xffffffffU) &&
                (nEcdSize <= nEcdAvailable) &&
                ((qint64)nCentralSize <= nEcdOffset);
            if (bBasicLayout) {
                const qint64 nActualCentralOffset = nEcdOffset - nCentralSize;
                const qint64 nArchiveOffset = nActualCentralOffset -
                                              (qint64)nStoredCentralOffset;
                const qint64 nArchiveEnd = nEcdOffset + nEcdSize;
                const qint64 nArchiveSize = nArchiveEnd - nArchiveOffset;
                const quint32 nExpectedFirstSignature = (nRecords == 0)
                    ? XZip::SIGNATURE_ECD : XZip::SIGNATURE_LFD;
                if ((nArchiveOffset >= 0) && (nArchiveOffset <= nFileSize - 4) &&
                    (nArchiveSize >= (qint64)sizeof(XZip::ENDOFCENTRALDIRECTORYRECORD)) &&
                    (pOuter->read_uint32(nArchiveOffset) == nExpectedFirstSignature)) {
                    SubDevice archiveDevice(pOuter->getDevice(), nArchiveOffset, nArchiveSize);
                    if (archiveDevice.open(QIODevice::ReadOnly)) {
                        XZip zip(&archiveDevice);
                        const qint64 nRelativeEcdOffset = zip.findECDOffset(pPdStruct);
                        const bool bValid = (nRelativeEcdOffset == nEcdOffset - nArchiveOffset) &&
                            zip.isValid(pPdStruct) &&
                            (zip.getFileFormatSize(pPdStruct) == nArchiveSize);
                        archiveDevice.close();
                        if (bValid) return nArchiveOffset;
                    }
                }
            }
        }

        nCandidatePosition = (nCandidatePosition > 0)
            ? baTail.lastIndexOf(baEcdSignature, nCandidatePosition - 1)
            : -1;
    }

    return -1;
}

bool checkedExtent(quint64 nOffset, quint64 nSize, qint64 nFileSize, qint64 *pnEnd)
{
    if (!pnEnd || (nFileSize < 0)) return false;
    const quint64 nUnsignedFileSize = static_cast<quint64>(nFileSize);
    if ((nOffset > nUnsignedFileSize) || (nSize > nUnsignedFileSize - nOffset)) return false;
    *pnEnd = static_cast<qint64>(nOffset + nSize);
    return true;
}

bool checkedTableExtent(quint64 nOffset, quint64 nEntrySize, quint64 nCount, qint64 nFileSize, qint64 *pnEnd)
{
    if (nCount && (nEntrySize > static_cast<quint64>(nFileSize) / nCount)) return false;
    return checkedExtent(nOffset, nEntrySize * nCount, nFileSize, pnEnd);
}

bool getZpaqFranzSfxPayload(XBinary *pOuter, qint64 nOverlayOffset, qint64 *pnPayloadOffset, qint64 *pnPayloadSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOuter || !pnPayloadOffset || !pnPayloadSize || (nOverlayOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nFileSize = pOuter->getSize();
    const qint64 nMinimumFraming = SFX_ZPAQFRANZ_START_TAG.size() + SFX_ZPAQFRANZ_END_TAG.size() + 2;
    if ((nFileSize < 0) || (nOverlayOffset > nFileSize - nMinimumFraming)) {
        return false;
    }

    const qint64 nReadSize = qMin(nFileSize - nOverlayOffset, SFX_ZPAQ_METADATA_SCAN_LIMIT);
    const QByteArray baFraming = pOuter->read_array_process(nOverlayOffset, nReadSize, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (baFraming.size() != nReadSize)) {
        return false;
    }

    // A certificate or another pre-existing unmapped trailer can precede the
    // zpaqfranz record. The paired 320-bit tags still identify it precisely
    // within this bounded metadata window.
    const int nStartTagOffset = baFraming.indexOf(SFX_ZPAQFRANZ_START_TAG);
    if (nStartTagOffset < 0) return false;
    const int nCommandOffset = nStartTagOffset + SFX_ZPAQFRANZ_START_TAG.size();
    const int nEndTagOffset = baFraming.indexOf(SFX_ZPAQFRANZ_END_TAG, nCommandOffset);
    if (nEndTagOffset <= nCommandOffset) return false;

    QByteArray baCommand = baFraming.mid(nCommandOffset, nEndTagOffset - nCommandOffset);
    for (char &c : baCommand) {
        c = static_cast<char>(static_cast<quint8>(c) ^ 0x21U);
        const quint8 nByte = static_cast<quint8>(c);
        if ((nByte < 0x20) || (nByte > 0x7E)) return false;
    }
    if (!baCommand.startsWith("x ") || !baCommand.toLower().contains(".zpaq")) {
        return false;
    }

    const qint64 nRelativePayloadOffset = static_cast<qint64>(nEndTagOffset) + SFX_ZPAQFRANZ_END_TAG.size();
    if ((nRelativePayloadOffset < 0) || (nRelativePayloadOffset > nFileSize - nOverlayOffset)) {
        return false;
    }
    const qint64 nPayloadOffset = nOverlayOffset + nRelativePayloadOffset;
    const qint64 nPayloadSize = nFileSize - nPayloadOffset;
    if (nPayloadSize <= 32) return false;

    *pnPayloadOffset = nPayloadOffset;
    *pnPayloadSize = nPayloadSize;
    return true;
}

bool resemblesPlainZpaqPrefix(const QByteArray &baPrefix)
{
    if (baPrefix.startsWith("zPQ")) return true;

    static const QByteArray baTaggedPrefix(
        "\x37\x6B\x53\x74\xA0\x31\x83\xD3"
        "\x8C\xB2\x28\xB0\xD3zPQ",
        16);
    if (baPrefix.size() < baTaggedPrefix.size()) return false;

    qint32 nDifferences = 0;
    for (qint32 i = 0; i < baTaggedPrefix.size(); ++i) {
        if (baPrefix.at(i) != baTaggedPrefix.at(i)) ++nDifferences;
    }
    return nDifferences <= 1;
}

// XELF's normal raw-size calculation honors segment alignment. For an SFX that
// can consume a short unmapped trailer and hide the real archive. Derive the
// exact file-backed extent instead and reject malformed/out-of-range tables.
qint64 getExactELFExtent(XELF *pELF, qint64 nFileSize)
{
    if (!pELF || (nFileSize <= 0)) return -1;

    const XELF_DEF::Elf_Ehdr header = pELF->getHdr();
    const bool bIs64 = pELF->is64();
    const quint16 nExpectedHeaderSize = bIs64 ? sizeof(XELF_DEF::Elf64_Ehdr) : sizeof(XELF_DEF::Elf32_Ehdr);
    const quint16 nExpectedProgramSize = bIs64 ? sizeof(XELF_DEF::Elf64_Phdr) : sizeof(XELF_DEF::Elf32_Phdr);
    const quint16 nExpectedSectionSize = bIs64 ? sizeof(XELF_DEF::Elf64_Shdr) : sizeof(XELF_DEF::Elf32_Shdr);

    // XELF's list readers advance by the native entry sizes. Accept only the
    // canonical layout they actually parse, and decline extended numbering
    // instead of deriving a potentially unsafe overlay boundary from it.
    if ((header.e_ehsize != nExpectedHeaderSize) || (header.e_phnum && (header.e_phentsize != nExpectedProgramSize)) ||
        (header.e_shnum && (header.e_shentsize != nExpectedSectionSize)) || ((!header.e_shnum) && header.e_shoff) || (header.e_shstrndx == XELF_DEF::S_SHN_XINDEX)) {
        return -1;
    }
    if ((header.e_phnum > SFX_ELF_TABLE_LIMIT) || (header.e_shnum > SFX_ELF_TABLE_LIMIT)) {
        return -1;
    }

    qint64 nResult = 0;
    qint64 nEnd = 0;
    if (!checkedExtent(0, header.e_ehsize, nFileSize, &nEnd)) return -1;
    nResult = qMax(nResult, nEnd);

    if (header.e_phnum) {
        if (!header.e_phentsize || !checkedTableExtent(header.e_phoff, header.e_phentsize, header.e_phnum, nFileSize, &nEnd)) {
            return -1;
        }
        nResult = qMax(nResult, nEnd);
    }
    if (header.e_shnum) {
        if (!header.e_shentsize || !checkedTableExtent(header.e_shoff, header.e_shentsize, header.e_shnum, nFileSize, &nEnd)) {
            return -1;
        }
        nResult = qMax(nResult, nEnd);
    }

    const QList<XELF_DEF::Elf_Phdr> programs = pELF->getElf_PhdrList(SFX_ELF_TABLE_LIMIT);
    if (programs.size() != header.e_phnum) return -1;
    for (const XELF_DEF::Elf_Phdr &program : programs) {
        if (!checkedExtent(program.p_offset, program.p_filesz, nFileSize, &nEnd)) return -1;
        nResult = qMax(nResult, nEnd);
    }

    const QList<XELF_DEF::Elf_Shdr> sections = pELF->getElf_ShdrList(SFX_ELF_TABLE_LIMIT);
    if (sections.size() != header.e_shnum) return -1;
    for (const XELF_DEF::Elf_Shdr &section : sections) {
        // SHT_NOBITS (8) occupies no bytes in the file.
        if (section.sh_type == 8) continue;
        if (!checkedExtent(section.sh_offset, section.sh_size, nFileSize, &nEnd)) return -1;
        nResult = qMax(nResult, nEnd);
    }

    return nResult;
}

bool hasFreeArcMethod(const QByteArray &baMethod)
{
    const int nEnd = baMethod.indexOf('\0');
    if (nEnd <= 0) return false;
    for (int i = 0; i < nEnd; i++) {
        const quint8 nByte = static_cast<quint8>(baMethod.at(i));
        if ((nByte < 0x20) || (nByte > 0x7E)) return false;
    }
    return true;
}

bool readFreeArcInteger(const QByteArray &baData, int *pnOffset, quint64 *pnValue)
{
    if (!pnOffset || !pnValue || (*pnOffset < 0) || (*pnOffset >= baData.size())) {
        return false;
    }

    const quint8 nFirst = static_cast<quint8>(baData.at(*pnOffset));
    int nEncodedSize = 0;
    if (!(nFirst & 1)) nEncodedSize = 1;
    else if ((nFirst & 3) == 1) nEncodedSize = 2;
    else if ((nFirst & 7) == 3) nEncodedSize = 3;
    else if ((nFirst & 15) == 7) nEncodedSize = 4;
    else if ((nFirst & 31) == 15) nEncodedSize = 5;
    else if ((nFirst & 63) == 31) nEncodedSize = 6;
    else if ((nFirst & 127) == 63) nEncodedSize = 7;
    else if (nFirst == 127) nEncodedSize = 8;
    else if (nFirst == 255) nEncodedSize = 9;
    else return false;

    if (nEncodedSize > baData.size() - *pnOffset) return false;

    quint64 nValue = 0;
    if (nEncodedSize == 9) {
        for (int i = 0; i < 8; i++) {
            nValue |= static_cast<quint64>(static_cast<quint8>(baData.at(*pnOffset + 1 + i))) << (i * 8);
        }
    } else {
        for (int i = 0; i < nEncodedSize; i++) {
            nValue |= static_cast<quint64>(static_cast<quint8>(baData.at(*pnOffset + i))) << (i * 8);
        }
        nValue >>= nEncodedSize;
    }

    *pnOffset += nEncodedSize;
    *pnValue = nValue;
    return true;
}

enum FREEARC_DATA_STATUS {
    FREEARC_DATA_CODEC_UNAVAILABLE = 0,
    FREEARC_DATA_RESOURCE_LIMIT,
    FREEARC_DATA_INVALID,
    FREEARC_DATA_AUTHENTICATED
};

enum FREEARC_LAYOUT_STATUS {
    FREEARC_LAYOUT_INVALID = 0,
    FREEARC_LAYOUT_RESOURCE_LIMIT,
    FREEARC_LAYOUT_PROVISIONAL,
    FREEARC_LAYOUT_AUTHENTICATED
};

struct FREEARC_DESCRIPTOR {
    qint64 nOffset = -1;
    qint64 nLength = 0;
    qint64 nDataOffset = -1;
    quint64 nType = 0;
    QByteArray baMethod;
    quint64 nOriginalSize = 0;
    quint64 nCompressedSize = 0;
    quint32 nDataCRC = 0;
};

quint32 readFreeArcLE32(const QByteArray &baData, int nOffset)
{
    if ((nOffset < 0) || (nOffset > baData.size() - 4)) return 0;
    return static_cast<quint32>(static_cast<quint8>(baData.at(nOffset))) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24);
}

bool isFreeArcMethodText(const QByteArray &baMethod)
{
    if (baMethod.isEmpty()) return false;
    for (char c : baMethod) {
        const quint8 nByte = static_cast<quint8>(c);
        if ((nByte < 0x20) || (nByte > 0x7E)) return false;
    }
    return true;
}

bool readFreeArcCString(const QByteArray &baData, int *pnOffset, QByteArray *pValue)
{
    if (!pnOffset || !pValue || (*pnOffset < 0) || (*pnOffset >= baData.size())) {
        return false;
    }
    const int nEnd = baData.indexOf('\0', *pnOffset);
    if (nEnd < *pnOffset) return false;
    *pValue = baData.mid(*pnOffset, nEnd - *pnOffset);
    *pnOffset = nEnd + 1;
    return true;
}

bool parseFreeArcDescriptorData(const QByteArray &baDescriptor, qint64 nOffset, qint64 nArchiveSize, FREEARC_DESCRIPTOR *pDescriptor)
{
    if (!pDescriptor || (nOffset < 0) || (nArchiveSize < 0) || (nOffset > nArchiveSize - 15) || (baDescriptor.size() < 15) ||
        (baDescriptor.left(4) != QByteArray("ArC\x01", 4))) {
        return false;
    }

    int nCursor = 4;
    quint64 nType = 0;
    QByteArray baMethod;
    quint64 nOriginalSize = 0;
    quint64 nCompressedSize = 0;
    if (!readFreeArcInteger(baDescriptor, &nCursor, &nType) || !readFreeArcCString(baDescriptor, &nCursor, &baMethod) || !isFreeArcMethodText(baMethod) ||
        !readFreeArcInteger(baDescriptor, &nCursor, &nOriginalSize) || !readFreeArcInteger(baDescriptor, &nCursor, &nCompressedSize) || !nOriginalSize ||
        !nCompressedSize || (nCursor > baDescriptor.size() - 8) || (nCompressedSize > static_cast<quint64>(nOffset))) {
        return false;
    }

    const quint32 nStoredDescriptorCRC = readFreeArcLE32(baDescriptor, nCursor + 4);
    const quint32 nCalculatedDescriptorCRC = XBinary::_getCRC32(baDescriptor.constData(), nCursor + 4, 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFFU;
    if (nStoredDescriptorCRC != nCalculatedDescriptorCRC) return false;

    FREEARC_DESCRIPTOR descriptor;
    descriptor.nOffset = nOffset;
    descriptor.nLength = nCursor + 8;
    descriptor.nDataOffset = nOffset - static_cast<qint64>(nCompressedSize);
    descriptor.nType = nType;
    descriptor.baMethod = baMethod;
    descriptor.nOriginalSize = nOriginalSize;
    descriptor.nCompressedSize = nCompressedSize;
    descriptor.nDataCRC = readFreeArcLE32(baDescriptor, nCursor);
    *pDescriptor = descriptor;
    return true;
}

bool parseFreeArcDescriptorAt(XFREEARC *pArchive, qint64 nOffset, FREEARC_DESCRIPTOR *pDescriptor, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || !pDescriptor || (nOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const qint64 nArchiveSize = pArchive->getSize();
    if ((nArchiveSize < 0) || (nOffset > nArchiveSize - 15)) return false;

    const qint64 nReadSize = qMin(nArchiveSize - nOffset, SFX_FREEARC_DESCRIPTOR_SCAN_LIMIT);
    const QByteArray baDescriptor = pArchive->read_array_process(nOffset, nReadSize, pPdStruct);
    return XBinary::isPdStructNotCanceled(pPdStruct) && (baDescriptor.size() == nReadSize) &&
           parseFreeArcDescriptorData(baDescriptor, nOffset, nArchiveSize, pDescriptor);
}

bool isAuthenticatedFreeArcHeaderDescriptor(XFREEARC *pArchive, const FREEARC_DESCRIPTOR &descriptor, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || (descriptor.nDataOffset < 0) || (descriptor.nOffset != descriptor.nDataOffset + SFX_FREEARC_HEADER_SIZE) || (descriptor.nType != 1) ||
        (descriptor.baMethod != "storing") || (descriptor.nOriginalSize != SFX_FREEARC_HEADER_SIZE) || (descriptor.nCompressedSize != SFX_FREEARC_HEADER_SIZE) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baHeader = pArchive->read_array_process(descriptor.nDataOffset, SFX_FREEARC_HEADER_SIZE, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (baHeader.size() != SFX_FREEARC_HEADER_SIZE) || (baHeader.left(4) != QByteArray("ArC\x01", 4))) {
        return false;
    }

    const quint32 nCalculatedCRC = pArchive->_getCRC32(descriptor.nDataOffset, SFX_FREEARC_HEADER_SIZE, 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320(), pPdStruct);
    return XBinary::isPdStructNotCanceled(pPdStruct) && (nCalculatedCRC == descriptor.nDataCRC);
}

bool hasAuthenticatedFreeArcHeader(XFREEARC *pArchive, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || (pArchive->getSize() < SFX_FREEARC_HEADER_SIZE + 15) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    FREEARC_DESCRIPTOR descriptor;
    return parseFreeArcDescriptorAt(pArchive, SFX_FREEARC_HEADER_SIZE, &descriptor, pPdStruct) && (descriptor.nDataOffset == 0) &&
           isAuthenticatedFreeArcHeaderDescriptor(pArchive, descriptor, pPdStruct);
}

bool parseFreeArcDecimal(const QByteArray &baValue, qint32 *pnResult)
{
    if (!pnResult || baValue.isEmpty()) return false;
    for (char c : baValue) {
        if ((c < '0') || (c > '9')) return false;
    }
    bool bOK = false;
    const qlonglong nValue = baValue.toLongLong(&bOK);
    if (!bOK || (nValue > (std::numeric_limits<qint32>::max)())) return false;
    *pnResult = static_cast<qint32>(nValue);
    return true;
}

bool parseFreeArcMemory(const QByteArray &baValue, quint64 *pnResult)
{
    if (!pnResult || baValue.isEmpty()) return false;
    QByteArray baNumber = baValue;
    quint64 nMultiplier = 1;
    bool bPowerOfTwo = false;
    if (baNumber.startsWith('=')) baNumber.remove(0, 1);
    if (baNumber.endsWith("gb")) {
        nMultiplier = 1024ULL * 1024 * 1024;
        baNumber.chop(2);
    } else if (baNumber.endsWith('g')) {
        nMultiplier = 1024ULL * 1024 * 1024;
        baNumber.chop(1);
    } else if (baNumber.endsWith("mb")) {
        nMultiplier = 1024ULL * 1024;
        baNumber.chop(2);
    } else if (baNumber.endsWith('m')) {
        nMultiplier = 1024ULL * 1024;
        baNumber.chop(1);
    } else if (baNumber.endsWith("kb")) {
        nMultiplier = 1024;
        baNumber.chop(2);
    } else if (baNumber.endsWith('k')) {
        nMultiplier = 1024;
        baNumber.chop(1);
    } else if (baNumber.endsWith('b')) {
        baNumber.chop(1);
    } else if (baNumber.endsWith('^')) {
        baNumber.chop(1);
        bPowerOfTwo = true;
    } else {
        // FreeArc's C parseMem() defaults a suffixless d/h value to 2^N.
        bPowerOfTwo = true;
    }

    if (baNumber.isEmpty()) return false;
    for (char c : baNumber) {
        if ((c < '0') || (c > '9')) return false;
    }

    bool bOK = false;
    const quint64 nValue = baNumber.toULongLong(&bOK);
    if (!bOK) {
        return false;
    }
    if (bPowerOfTwo) {
        if (nValue >= 64) return false;
        *pnResult = Q_UINT64_C(1) << nValue;
    } else {
        if (nValue > (std::numeric_limits<quint64>::max)() / nMultiplier) {
            return false;
        }
        *pnResult = nValue * nMultiplier;
    }
    return true;
}

enum FREEARC_LZMA_PROPERTIES_STATUS {
    FREEARC_LZMA_PROPERTIES_CODEC_UNAVAILABLE = 0,
    FREEARC_LZMA_PROPERTIES_RESOURCE_LIMIT,
    FREEARC_LZMA_PROPERTIES_INVALID,
    FREEARC_LZMA_PROPERTIES_VALID
};

FREEARC_LZMA_PROPERTIES_STATUS parseFreeArcLzmaProperties(const QByteArray &baMethod, QByteArray *pProperties, quint64 *pnDictionarySize)
{
    if (!pProperties || !pnDictionarySize) {
        return FREEARC_LZMA_PROPERTIES_INVALID;
    }
    if (baMethod.contains('+')) {
        const QList<QByteArray> chain = baMethod.split('+');
        for (const QByteArray &component : chain) {
            if (component.isEmpty()) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
        }
        return FREEARC_LZMA_PROPERTIES_CODEC_UNAVAILABLE;
    }

    const QList<QByteArray> parts = baMethod.split(':');
    if (parts.isEmpty() || (parts.first() != "lzma")) {
        return FREEARC_LZMA_PROPERTIES_CODEC_UNAVAILABLE;
    }

    quint64 nDictionarySize = 64ULL * 1024 * 1024;
    qint32 nPosStateBits = 2;
    qint32 nLiteralContextBits = 3;
    qint32 nLiteralPosBits = 0;

    for (qint32 i = 1; i < parts.size(); i++) {
        QByteArray baPart = parts.at(i);
        const bool bOptional = baPart.startsWith('*');
        if (bOptional) baPart.remove(0, 1);
        if (baPart.isEmpty()) return FREEARC_LZMA_PROPERTIES_INVALID;

        qint32 nInteger = 0;
        quint64 nMemory = 0;
        if ((baPart == "fastest") || (baPart == "fast") || (baPart == "normal") || (baPart == "max") || (baPart == "ultra") || (baPart == "ht4") || (baPart == "hc4") ||
            (baPart == "bt2") || (baPart == "bt3") || (baPart == "bt4")) {
            // Encoder-only choices do not alter the raw LZMA properties.
        } else if (baPart.startsWith('d')) {
            QByteArray baValue = baPart.mid(1);
            if (baValue.startsWith('=')) baValue.remove(0, 1);
            if (!parseFreeArcMemory(baValue, &nMemory)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
            nDictionarySize = nMemory;
        } else if (baPart.startsWith("pb")) {
            QByteArray baValue = baPart.mid(2);
            if (baValue.startsWith('=')) baValue.remove(0, 1);
            if (!parseFreeArcDecimal(baValue, &nInteger)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
            nPosStateBits = nInteger;
        } else if (baPart.startsWith("lc")) {
            QByteArray baValue = baPart.mid(2);
            if (baValue.startsWith('=')) baValue.remove(0, 1);
            if (!parseFreeArcDecimal(baValue, &nInteger)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
            nLiteralContextBits = nInteger;
        } else if (baPart.startsWith("lp")) {
            QByteArray baValue = baPart.mid(2);
            if (baValue.startsWith('=')) baValue.remove(0, 1);
            if (!parseFreeArcDecimal(baValue, &nInteger)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
            nLiteralPosBits = nInteger;
        } else if (baPart.startsWith('h')) {
            QByteArray baValue = baPart.mid(1);
            if (!parseFreeArcMemory(baValue, &nMemory)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
        } else if (baPart.startsWith('a')) {
            QByteArray baValue = baPart.mid(1);
            if (baValue.startsWith('=')) baValue.remove(0, 1);
            if (!parseFreeArcDecimal(baValue, &nInteger)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
        } else if (baPart.startsWith("fb") || baPart.startsWith("mc")) {
            QByteArray baValue = baPart.mid(2);
            if (baValue.startsWith('=')) baValue.remove(0, 1);
            if (!parseFreeArcDecimal(baValue, &nInteger)) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
        } else if (baPart.startsWith("mf")) {
            QByteArray baMatchFinder = baPart.mid(2);
            if (baMatchFinder.startsWith('=')) baMatchFinder.remove(0, 1);
            if ((baMatchFinder != "ht4") && (baMatchFinder != "hc4") && (baMatchFinder != "bt2") && (baMatchFinder != "bt3") && (baMatchFinder != "bt4")) {
                return FREEARC_LZMA_PROPERTIES_INVALID;
            }
        } else if (parseFreeArcDecimal(baPart, &nInteger)) {
            // Unnamed decimal is numFastBytes, an encoder-only setting.
        } else if (parseFreeArcMemory(baPart, &nMemory)) {
            nDictionarySize = nMemory;
        } else if (!bOptional) {
            // FreeArc has encoder-only option spellings which do not affect
            // raw decoder properties. Unknown options are indeterminate, not
            // proof that the archive itself is corrupt.
            return FREEARC_LZMA_PROPERTIES_CODEC_UNAVAILABLE;
        }
    }

    if (!nDictionarySize || (nPosStateBits < 0) || (nPosStateBits > 4) || (nLiteralContextBits < 0) || (nLiteralContextBits > 8) || (nLiteralPosBits < 0) ||
        (nLiteralPosBits > 4) || (nLiteralContextBits + nLiteralPosBits > 12)) {
        return FREEARC_LZMA_PROPERTIES_INVALID;
    }
    if ((nDictionarySize > SFX_FREEARC_LZMA_DICTIONARY_LIMIT) || (nDictionarySize > (std::numeric_limits<quint32>::max)())) {
        return FREEARC_LZMA_PROPERTIES_RESOURCE_LIMIT;
    }

    const quint32 nDictionary32 = static_cast<quint32>(nDictionarySize);
    QByteArray properties(5, 0);
    properties[0] = static_cast<char>((nPosStateBits * 5 + nLiteralPosBits) * 9 + nLiteralContextBits);
    for (qint32 i = 0; i < 4; i++) {
        properties[1 + i] = static_cast<char>(nDictionary32 >> (8 * i));
    }
    *pProperties = properties;
    *pnDictionarySize = nDictionarySize;
    return FREEARC_LZMA_PROPERTIES_VALID;
}

FREEARC_DATA_STATUS validateFreeArcFooterData(XFREEARC *pArchive, const QByteArray &baData, qint64 nFooterDataOffset, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || baData.isEmpty() || (nFooterDataOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FREEARC_DATA_INVALID;
    }

    int nCursor = 0;
    quint64 nBlockCount = 0;
    if (!readFreeArcInteger(baData, &nCursor, &nBlockCount) || !nBlockCount) {
        return FREEARC_DATA_INVALID;
    }
    if (nBlockCount > SFX_FREEARC_CONTROL_BLOCK_LIMIT) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }

    qint64 nPreviousControlEnd = 0;
    for (quint64 i = 0; i < nBlockCount; i++) {
        quint64 nType = 0;
        QByteArray baMethod;
        quint64 nRelativePosition = 0;
        quint64 nOriginalSize = 0;
        quint64 nCompressedSize = 0;
        if (!readFreeArcInteger(baData, &nCursor, &nType) || !readFreeArcCString(baData, &nCursor, &baMethod) || !isFreeArcMethodText(baMethod) ||
            !readFreeArcInteger(baData, &nCursor, &nRelativePosition) || !readFreeArcInteger(baData, &nCursor, &nOriginalSize) ||
            !readFreeArcInteger(baData, &nCursor, &nCompressedSize) || !nOriginalSize || !nCompressedSize || (nCursor > baData.size() - 4) ||
            (nRelativePosition > static_cast<quint64>(nFooterDataOffset))) {
            return FREEARC_DATA_INVALID;
        }
        const quint32 nDataCRC = readFreeArcLE32(baData, nCursor);
        nCursor += 4;

        if (((i == 0) && (nType != 1)) || ((i != 0) && (nType == 1)) || ((nType != 1) && (nType != 3) && (nType != 4) && (nType != 5))) {
            return FREEARC_DATA_INVALID;
        }
        const qint64 nBlockOffset = nFooterDataOffset - static_cast<qint64>(nRelativePosition);
        if ((nBlockOffset < nPreviousControlEnd) || (nCompressedSize > static_cast<quint64>(nFooterDataOffset - nBlockOffset))) {
            return FREEARC_DATA_INVALID;
        }
        const qint64 nDescriptorOffset = nBlockOffset + static_cast<qint64>(nCompressedSize);
        FREEARC_DESCRIPTOR descriptor;
        if (!parseFreeArcDescriptorAt(pArchive, nDescriptorOffset, &descriptor, pPdStruct) || (descriptor.nType != nType) || (descriptor.baMethod != baMethod) ||
            (descriptor.nOriginalSize != nOriginalSize) || (descriptor.nCompressedSize != nCompressedSize) || (descriptor.nDataCRC != nDataCRC) ||
            (descriptor.nDataOffset != nBlockOffset) || (descriptor.nLength > nFooterDataOffset - nDescriptorOffset)) {
            return FREEARC_DATA_INVALID;
        }
        nPreviousControlEnd = nDescriptorOffset + descriptor.nLength;

        if (i == 0) {
            if ((nBlockOffset != 0) || (baMethod != "storing") || (nOriginalSize != 8) || (nCompressedSize != 8)) {
                return FREEARC_DATA_INVALID;
            }
            const quint32 nCalculatedHeaderCRC = pArchive->_getCRC32(0, 8, 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320(), pPdStruct);
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nCalculatedHeaderCRC != nDataCRC)) {
                return FREEARC_DATA_INVALID;
            }
        }
    }
    if (nCursor >= baData.size()) {
        return FREEARC_DATA_INVALID;
    }

    const quint8 nLocked = static_cast<quint8>(baData.at(nCursor++));
    if (nLocked > 1) return FREEARC_DATA_INVALID;

    quint64 nOldCommentLength = 0;
    if (!readFreeArcInteger(baData, &nCursor, &nOldCommentLength) || (nOldCommentLength > static_cast<quint64>((baData.size() - nCursor) / 4))) {
        return FREEARC_DATA_INVALID;
    }
    nCursor += static_cast<int>(nOldCommentLength * 4);
    if (nCursor == baData.size()) return FREEARC_DATA_AUTHENTICATED;

    QByteArray baRecovery;
    if (!readFreeArcCString(baData, &nCursor, &baRecovery)) {
        return FREEARC_DATA_INVALID;
    }
    if (nCursor == baData.size()) return FREEARC_DATA_AUTHENTICATED;

    quint64 nCommentLength = 0;
    if (!readFreeArcInteger(baData, &nCursor, &nCommentLength) || (nCommentLength != static_cast<quint64>(baData.size() - nCursor))) {
        return FREEARC_DATA_INVALID;
    }
    return FREEARC_DATA_AUTHENTICATED;
}

QString freeArcAuthenticationKey(qint64 nAbsoluteDataOffset, const QByteArray &baMethod, quint64 nOriginalSize, quint64 nCompressedSize, quint32 nStoredCRC)
{
    return QString::number(nAbsoluteDataOffset) + QLatin1Char(':') + QString::number(nOriginalSize) + QLatin1Char(':') + QString::number(nCompressedSize) +
           QLatin1Char(':') + QString::number(nStoredCRC) + QLatin1Char(':') + QString::fromLatin1(baMethod.toHex());
}

bool getExpectedFreeArcCandidateOffset(const QByteArray &baFooterData, qint64 nAbsoluteFooterDataOffset, qint64 *pnExpectedCandidateOffset)
{
    if (!pnExpectedCandidateOffset || baFooterData.isEmpty() || (nAbsoluteFooterDataOffset < 0)) {
        return false;
    }

    int nCursor = 0;
    quint64 nBlockCount = 0;
    quint64 nType = 0;
    QByteArray baMethod;
    quint64 nRelativePosition = 0;
    quint64 nOriginalSize = 0;
    quint64 nCompressedSize = 0;
    if (!readFreeArcInteger(baFooterData, &nCursor, &nBlockCount) || !nBlockCount || !readFreeArcInteger(baFooterData, &nCursor, &nType) ||
        !readFreeArcCString(baFooterData, &nCursor, &baMethod) || !readFreeArcInteger(baFooterData, &nCursor, &nRelativePosition) ||
        !readFreeArcInteger(baFooterData, &nCursor, &nOriginalSize) || !readFreeArcInteger(baFooterData, &nCursor, &nCompressedSize) ||
        (nCursor > baFooterData.size() - 4) || (nType != 1) || (baMethod != "storing") || (nOriginalSize != 8) || (nCompressedSize != 8) ||
        (nRelativePosition > static_cast<quint64>(nAbsoluteFooterDataOffset))) {
        return false;
    }

    *pnExpectedCandidateOffset = nAbsoluteFooterDataOffset - static_cast<qint64>(nRelativePosition);
    return true;
}

FREEARC_DATA_STATUS authenticateFreeArcStoredData(XFREEARC *pArchive, qint64 nCandidateOffset, qint64 nDataOffset, quint64 nOriginalSize, quint64 nCompressedSize,
                                                  quint32 nStoredCRC, XSFX_FREEARC_SCAN_CACHE *pScanCache, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || !pScanCache || (nCandidateOffset < 0) || (nDataOffset < 0) || !nOriginalSize || (nOriginalSize != nCompressedSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FREEARC_DATA_INVALID;
    }
    if ((nOriginalSize > static_cast<quint64>(SFX_FREEARC_CONTROL_OUTPUT_LIMIT)) || (nCompressedSize > static_cast<quint64>(SFX_FREEARC_CONTROL_OUTPUT_LIMIT))) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    if (nCandidateOffset > (std::numeric_limits<qint64>::max)() - nDataOffset) {
        return FREEARC_DATA_INVALID;
    }

    const qint64 nAbsoluteDataOffset = nCandidateOffset + nDataOffset;
    const QString sKey = freeArcAuthenticationKey(nAbsoluteDataOffset, QByteArray("storing"), nOriginalSize, nCompressedSize, nStoredCRC);
    qint64 nExpectedCandidateOffset = -1;
    if (pScanCache->lookupExpectedCandidate(sKey, &nExpectedCandidateOffset) && (nCandidateOffset != nExpectedCandidateOffset)) {
        return FREEARC_DATA_INVALID;
    }
    qint32 nCachedStatus = 0;
    if (pScanCache->lookup(sKey, &nCachedStatus)) {
        return static_cast<FREEARC_DATA_STATUS>(nCachedStatus);
    }
    QMap<XBinary::UNPACK_PROP, QVariant> mapMemoryProperties;
    mapMemoryProperties.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE, SFX_FREEARC_MEMORY_LIMIT);
    XBinary::UNPACK_MEMORY_RESERVATION outputReservation;
    if (!outputReservation.acquire(mapMemoryProperties, static_cast<qint64>(nOriginalSize))) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    if (!pScanCache->charge(static_cast<qint64>(nOriginalSize))) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }

    const QByteArray baData = pArchive->read_array_process(nDataOffset, static_cast<qint64>(nCompressedSize), pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (baData.size() != static_cast<qint64>(nCompressedSize))) {
        return FREEARC_DATA_INVALID;
    }
    const quint32 nCalculatedCRC = XBinary::_getCRC32(baData.constData(), baData.size(), 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFFU;
    if (nCalculatedCRC != nStoredCRC) {
        pScanCache->remember(sKey, FREEARC_DATA_INVALID);
        return FREEARC_DATA_INVALID;
    }
    if (!getExpectedFreeArcCandidateOffset(baData, nAbsoluteDataOffset, &nExpectedCandidateOffset)) {
        pScanCache->remember(sKey, FREEARC_DATA_INVALID);
        return FREEARC_DATA_INVALID;
    }
    pScanCache->rememberExpectedCandidate(sKey, nExpectedCandidateOffset);
    if (nCandidateOffset != nExpectedCandidateOffset) {
        return FREEARC_DATA_INVALID;
    }
    const FREEARC_DATA_STATUS status = validateFreeArcFooterData(pArchive, baData, nDataOffset, pPdStruct);
    pScanCache->remember(sKey, status);
    return status;
}

FREEARC_DATA_STATUS authenticateFreeArcLzmaData(XFREEARC *pArchive, qint64 nCandidateOffset, const QByteArray &baMethod, qint64 nDataOffset, quint64 nOriginalSize,
                                                quint64 nCompressedSize, quint32 nStoredCRC, XSFX_FREEARC_SCAN_CACHE *pScanCache, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || !pScanCache || (nCandidateOffset < 0) || (nDataOffset < 0) || !nOriginalSize || !nCompressedSize || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FREEARC_DATA_INVALID;
    }
    if ((nOriginalSize > static_cast<quint64>(SFX_FREEARC_CONTROL_OUTPUT_LIMIT)) || (nCompressedSize > static_cast<quint64>(SFX_FREEARC_CONTROL_OUTPUT_LIMIT))) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    if (nCandidateOffset > (std::numeric_limits<qint64>::max)() - nDataOffset) {
        return FREEARC_DATA_INVALID;
    }

    // Composed codecs are delegated to the FreeArc backend, but recognizable
    // LZMA stages still have to satisfy the same local dictionary cap. Adding
    // an unsupported filter or encryption stage must not bypass that limit.
    if (baMethod.contains('+')) {
        const QList<QByteArray> chain = baMethod.split('+');
        for (const QByteArray &component : chain) {
            if (component.isEmpty()) return FREEARC_DATA_INVALID;
            if ((component == "lzma") || component.startsWith("lzma:")) {
                QByteArray baComponentProperties;
                quint64 nComponentDictionarySize = 0;
                const FREEARC_LZMA_PROPERTIES_STATUS componentStatus = parseFreeArcLzmaProperties(component, &baComponentProperties, &nComponentDictionarySize);
                if (componentStatus == FREEARC_LZMA_PROPERTIES_RESOURCE_LIMIT) {
                    return FREEARC_DATA_RESOURCE_LIMIT;
                }
                if (componentStatus == FREEARC_LZMA_PROPERTIES_INVALID) {
                    return FREEARC_DATA_INVALID;
                }
            }
        }
        return FREEARC_DATA_CODEC_UNAVAILABLE;
    }

    QByteArray baProperties;
    quint64 nDictionarySize = 0;
    const FREEARC_LZMA_PROPERTIES_STATUS propertiesStatus = parseFreeArcLzmaProperties(baMethod, &baProperties, &nDictionarySize);
    if (propertiesStatus == FREEARC_LZMA_PROPERTIES_CODEC_UNAVAILABLE) {
        return FREEARC_DATA_CODEC_UNAVAILABLE;
    }
    if (propertiesStatus == FREEARC_LZMA_PROPERTIES_RESOURCE_LIMIT) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    if (propertiesStatus != FREEARC_LZMA_PROPERTIES_VALID) {
        return FREEARC_DATA_INVALID;
    }
    Q_UNUSED(nDictionarySize)

    const qint64 nAbsoluteDataOffset = nCandidateOffset + nDataOffset;
    const QString sKey = freeArcAuthenticationKey(nAbsoluteDataOffset, baMethod, nOriginalSize, nCompressedSize, nStoredCRC);
    qint64 nExpectedCandidateOffset = -1;
    if (pScanCache->lookupExpectedCandidate(sKey, &nExpectedCandidateOffset) && (nCandidateOffset != nExpectedCandidateOffset)) {
        return FREEARC_DATA_INVALID;
    }
    qint32 nCachedStatus = 0;
    if (pScanCache->lookup(sKey, &nCachedStatus)) {
        return static_cast<FREEARC_DATA_STATUS>(nCachedStatus);
    }
    QMap<XBinary::UNPACK_PROP, QVariant> mapMemoryProperties;
    mapMemoryProperties.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE, SFX_FREEARC_MEMORY_LIMIT);
    qint64 nDecoderMemory = 0;
    if (!XLZMADecoder::getMemoryRequirement(baProperties, &nDecoderMemory, pPdStruct)) {
        return FREEARC_DATA_INVALID;
    }
    if (nOriginalSize > static_cast<quint64>((std::numeric_limits<qint64>::max)() - nDecoderMemory)) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    const qint64 nCombinedMemory = static_cast<qint64>(nOriginalSize) + nDecoderMemory;
    XBinary::UNPACK_MEMORY_RESERVATION combinedReservation;
    if (!combinedReservation.acquire(mapMemoryProperties, nCombinedMemory)) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    if (!pScanCache->charge(static_cast<qint64>(nOriginalSize))) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }

    SubDevice input(pArchive->getDevice(), nDataOffset, static_cast<qint64>(nCompressedSize));
    QBuffer output;
    output.buffer().reserve(static_cast<qsizetype>(nOriginalSize));
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::ReadWrite)) {
        return FREEARC_DATA_INVALID;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &input;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = static_cast<qint64>(nCompressedSize);
    state.nProcessedOffset = 0;
    state.nProcessedLimit = static_cast<qint64>(nOriginalSize);
    state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(nOriginalSize));
    state.mapUnpackProperties = mapMemoryProperties;

    const XLZMADecoder::DECOMPRESS_RESULT decodeResult = XLZMADecoder::decompressWithResult(&state, baProperties, pPdStruct, &combinedReservation);
    input.close();
    output.close();
    if (decodeResult == XLZMADecoder::DECOMPRESS_RESULT_RESOURCE_LIMIT) {
        return FREEARC_DATA_RESOURCE_LIMIT;
    }
    if ((decodeResult != XLZMADecoder::DECOMPRESS_RESULT_SUCCESS) || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (state.nCountInput != static_cast<qint64>(nCompressedSize)) || (state.nCountOutput != static_cast<qint64>(nOriginalSize)) ||
        (output.data().size() != static_cast<qint64>(nOriginalSize))) {
        pScanCache->remember(sKey, FREEARC_DATA_INVALID);
        return FREEARC_DATA_INVALID;
    }

    const quint32 nCalculatedCRC = XBinary::_getCRC32(output.data().constData(), output.data().size(), 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFFU;
    if (nCalculatedCRC != nStoredCRC) {
        pScanCache->remember(sKey, FREEARC_DATA_INVALID);
        return FREEARC_DATA_INVALID;
    }
    if (!getExpectedFreeArcCandidateOffset(output.data(), nAbsoluteDataOffset, &nExpectedCandidateOffset)) {
        pScanCache->remember(sKey, FREEARC_DATA_INVALID);
        return FREEARC_DATA_INVALID;
    }
    pScanCache->rememberExpectedCandidate(sKey, nExpectedCandidateOffset);
    if (nCandidateOffset != nExpectedCandidateOffset) {
        return FREEARC_DATA_INVALID;
    }
    const FREEARC_DATA_STATUS status = validateFreeArcFooterData(pArchive, output.data(), nDataOffset, pPdStruct);
    pScanCache->remember(sKey, status);
    return status;
}

FREEARC_DATA_STATUS getFreeArcFinalFooterStatus(XFREEARC *pArchive, qint64 nCandidateOffset, const FREEARC_DESCRIPTOR &descriptor, XSFX_FREEARC_SCAN_CACHE *pScanCache,
                                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || !pScanCache || (nCandidateOffset < 0) || (descriptor.nType != 4) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FREEARC_DATA_INVALID;
    }

    const qint64 nArchiveSize = pArchive->getSize();
    if ((descriptor.nOffset < 0) || (descriptor.nLength != nArchiveSize - descriptor.nOffset) || (descriptor.nDataOffset < 0)) {
        return FREEARC_DATA_INVALID;
    }

    FREEARC_DATA_STATUS status = FREEARC_DATA_CODEC_UNAVAILABLE;
    if (descriptor.baMethod == "storing") {
        status = authenticateFreeArcStoredData(pArchive, nCandidateOffset, descriptor.nDataOffset, descriptor.nOriginalSize, descriptor.nCompressedSize,
                                               descriptor.nDataCRC, pScanCache, pPdStruct);
    } else {
        status = authenticateFreeArcLzmaData(pArchive, nCandidateOffset, descriptor.baMethod, descriptor.nDataOffset, descriptor.nOriginalSize,
                                             descriptor.nCompressedSize, descriptor.nDataCRC, pScanCache, pPdStruct);
    }
    return status;
}

FREEARC_LAYOUT_STATUS getFreeArcBlockLayoutStatus(XFREEARC *pArchive, qint64 nCandidateOffset, XSFX_FREEARC_SCAN_CACHE *pScanCache, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || !pScanCache || (nCandidateOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FREEARC_LAYOUT_INVALID;
    }

    const qint64 nArchiveSize = pArchive->getSize();
    if ((nArchiveSize < 23) || !hasAuthenticatedFreeArcHeader(pArchive, pPdStruct)) {
        return FREEARC_LAYOUT_INVALID;
    }
    const qint64 nScanSize = qMin(nArchiveSize, SFX_FREEARC_DESCRIPTOR_SCAN_LIMIT);
    const qint64 nScanOffset = nArchiveSize - nScanSize;
    const QByteArray baTail = pArchive->read_array_process(nScanOffset, nScanSize, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (baTail.size() != nScanSize)) {
        return FREEARC_LAYOUT_INVALID;
    }

    bool bResourceLimited = false;
    const QByteArray baSignature("ArC\x01", 4);
    int nSearchFrom = baTail.size() - baSignature.size();
    while ((nSearchFrom >= 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const int nRelativeOffset = baTail.lastIndexOf(baSignature, nSearchFrom);
        if (nRelativeOffset < 0) break;

        const qint64 nDescriptorOffset = nScanOffset + nRelativeOffset;
        FREEARC_DESCRIPTOR descriptor;
        if (parseFreeArcDescriptorAt(pArchive, nDescriptorOffset, &descriptor, pPdStruct) && (descriptor.nType == 4) &&
            (descriptor.nLength == nArchiveSize - nDescriptorOffset)) {
            const FREEARC_DATA_STATUS status = getFreeArcFinalFooterStatus(pArchive, nCandidateOffset, descriptor, pScanCache, pPdStruct);
            if (status == FREEARC_DATA_AUTHENTICATED) {
                return FREEARC_LAYOUT_AUTHENTICATED;
            }
            if (status == FREEARC_DATA_CODEC_UNAVAILABLE) {
                // Encrypted/composed control streams require backend validation
                // with the caller's password before this offset is committed.
                return FREEARC_LAYOUT_PROVISIONAL;
            }
            if (status == FREEARC_DATA_RESOURCE_LIMIT) {
                bResourceLimited = true;
            }
        }
        nSearchFrom = nRelativeOffset - 1;
    }
    return bResourceLimited ? FREEARC_LAYOUT_RESOURCE_LIMIT : FREEARC_LAYOUT_INVALID;
}

bool hasZpaqBlockLayout(XZPAQ *pArchive, XBinary *pOuter, qint64 nCandidateOffset, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pArchive || !pOuter || (nCandidateOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nArchiveSize = pArchive->getSize();
    const qint64 nOuterSize = pOuter->getSize();
    if ((nOuterSize < 0) || (nCandidateOffset > nOuterSize) || (nArchiveSize != nOuterSize - nCandidateOffset)) {
        return false;
    }
    const qint64 nBlockOffset = pArchive->getFirstBlockOffset();
    const quint16 nHeaderSize = pArchive->getHeaderSize();
    const qint64 nHeaderStart = nBlockOffset + 7;
    if ((nBlockOffset < 0) || (nHeaderSize < 7) || (nHeaderStart < nBlockOffset) || (nHeaderStart > nArchiveSize) || (nHeaderSize > nArchiveSize - nHeaderStart)) {
        return false;
    }

    const qint64 nHeaderEnd = nHeaderStart + nHeaderSize;
    const QByteArray baPrefix = pArchive->read_array_process(0, nHeaderEnd, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (baPrefix.size() != nHeaderEnd)) {
        return false;
    }

    // Validate the COMP layout, not just the final zero byte. The first five
    // bytes are hh, hm, ph, pm, and the component count. Each component has a
    // fixed record size defined by the ZPAQ specification.
    static const quint8 anComponentSizes[] = {0, 2, 3, 2, 3, 4, 6, 6, 3, 5};
    const int nBodyStart = static_cast<int>(nHeaderStart);
    const int nBodyEnd = static_cast<int>(nHeaderEnd);
    const quint8 nLevel = static_cast<quint8>(baPrefix.at(static_cast<int>(nBlockOffset + 3)));
    const quint8 nComponents = static_cast<quint8>(baPrefix.at(nBodyStart + 4));
    if ((nLevel == 1) && !nComponents) return false;

    int nCursor = nBodyStart + 5;
    for (quint16 i = 0; i < nComponents; i++) {
        if (nCursor >= nBodyEnd) return false;
        const quint8 nType = static_cast<quint8>(baPrefix.at(nCursor));
        if ((nType == 0) || (nType >= sizeof(anComponentSizes) / sizeof(anComponentSizes[0]))) {
            return false;
        }
        const int nComponentSize = anComponentSizes[nType];
        if ((nComponentSize <= 0) || (nComponentSize > nBodyEnd - nCursor)) {
            return false;
        }
        nCursor += nComponentSize;
    }

    // COMP and HCOMP are independently zero-terminated.
    if ((nCursor >= nBodyEnd) || (baPrefix.at(nCursor) != 0) || (nCursor + 1 > nBodyEnd - 1) || (baPrefix.at(nBodyEnd - 1) != 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Parse through the end of the first block. For modeled data, four zero
    // bytes are an unambiguous arithmetic-stream terminator. Level-2 blocks
    // without components store a sequence of big-endian sized chunks ending
    // in a zero length. Requiring the segment checksum marker and final EOB
    // keeps a planted header/segment prefix from masking the real payload.
    const qint64 nScanEnd = nArchiveSize;
    qint64 nSegmentOffset = nHeaderEnd;
    bool bSeenSegment = false;
    while ((nSegmentOffset < nScanEnd) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pArchive->read_uint8(nSegmentOffset) == 255) {
            // A block with no segments is valid (for example, an empty
            // journal), but only when that complete empty block reaches EOF.
            // Otherwise a planted 28-byte block could mask a real payload
            // beginning later in the executable overlay.
            return bSeenSegment || (nSegmentOffset + 1 == nScanEnd);
        }
        if (pArchive->read_uint8(nSegmentOffset) != 1) return false;
        bSeenSegment = true;

        const qint64 nFilenameStart = nSegmentOffset + 1;
        const qint64 nFilenameScanSize = qMin(nScanEnd - nFilenameStart, SFX_ZPAQ_METADATA_SCAN_LIMIT);
        const qint64 nFilenameEnd = pArchive->find_signature(nFilenameStart, nFilenameScanSize, "00", nullptr, pPdStruct);
        if ((nFilenameEnd < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint64 nCommentStart = nFilenameEnd + 1;
        const qint64 nCommentScanSize = qMin(nScanEnd - nCommentStart, SFX_ZPAQ_METADATA_SCAN_LIMIT);
        const qint64 nCommentEnd = pArchive->find_signature(nCommentStart, nCommentScanSize, "00", nullptr, pPdStruct);
        if ((nCommentEnd < 0) || (nCommentEnd + 1 >= nScanEnd) || (pArchive->read_uint8(nCommentEnd + 1) != 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        qint64 nDataOffset = nCommentEnd + 2;
        if (nComponents) {
            qint64 nTerminator = -1;
            if (pZpaqScanCache) {
                const qint64 nAbsoluteStart = nCandidateOffset + nDataOffset;
                const qint64 nAbsoluteEnd = nCandidateOffset + nScanEnd;
                const qint64 nAbsoluteTerminator = pZpaqScanCache->findTerminator(pOuter, nAbsoluteStart, nAbsoluteEnd, pPdStruct);
                if (nAbsoluteTerminator >= nCandidateOffset) {
                    nTerminator = nAbsoluteTerminator - nCandidateOffset;
                }
            } else {
                nTerminator = pArchive->find_signature(nDataOffset, nScanEnd - nDataOffset, "00000000", nullptr, pPdStruct);
            }
            if ((nTerminator < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            nDataOffset = nTerminator + 4;
        } else {
            bool bEndOfData = false;
            while ((nDataOffset <= nScanEnd - 4) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const quint32 nChunkSize = pArchive->read_uint32(nDataOffset, true);
                nDataOffset += 4;
                if (!nChunkSize) {
                    bEndOfData = true;
                    break;
                }
                if (nChunkSize > static_cast<quint64>(nScanEnd - nDataOffset)) {
                    return false;
                }
                nDataOffset += nChunkSize;
            }
            if (!bEndOfData || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
        }

        if (nDataOffset >= nScanEnd) return false;
        const quint8 nEndMarker = pArchive->read_uint8(nDataOffset++);
        if (nEndMarker == 253) {
            if (nDataOffset > nScanEnd - 20) return false;
            nDataOffset += 20;
        } else if (nEndMarker != 254) {
            return false;
        }
        nSegmentOffset = nDataOffset;
    }

    return false;
}

bool isExternalSfxType(XBinary::FT arcType)
{
    return (arcType == XBinary::FT_FREEARC) || (arcType == XBinary::FT_ZPAQ);
}

XExternalArchive *externalArchiveFor(XArchive *pArchive, XBinary::FT arcType)
{
    if (!pArchive || !isExternalSfxType(arcType)) return nullptr;
    return static_cast<XExternalArchive *>(pArchive);
}

void clearPrivateUnpackCredential(XSFX::UNPACK_CONTEXT *pContext)
{
    if (!pContext) return;
    pContext->mapPrivateUnpackProperties.remove(XBinary::UNPACK_PROP_PASSWORD);
    pContext->mapPrivateUnpackProperties.remove(XBinary::UNPACK_PROP_PASSWORD_BYTES);
    if (!pContext->sPrivatePassword.isEmpty()) {
        pContext->sPrivatePassword.detach();
        pContext->sPrivatePassword.fill(QChar::Null);
        pContext->sPrivatePassword.clear();
        pContext->sPrivatePassword.squeeze();
    }
    if (!pContext->baPrivatePassword.isEmpty()) {
        pContext->baPrivatePassword.detach();
        pContext->baPrivatePassword.fill('\0');
        pContext->baPrivatePassword.clear();
        pContext->baPrivatePassword.squeeze();
    }
}

void initializePrivateUnpackProperties(XSFX::UNPACK_CONTEXT *pContext, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, bool bKeepCredential)
{
    if (!pContext) return;
    clearPrivateUnpackCredential(pContext);
    pContext->mapPrivateUnpackProperties = mapProperties;
    pContext->mapPrivateUnpackProperties.remove(XBinary::UNPACK_PROP_PASSWORD);
    pContext->mapPrivateUnpackProperties.remove(XBinary::UNPACK_PROP_PASSWORD_BYTES);
    if (!bKeepCredential) return;

    pContext->sPrivatePassword = mapProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
    pContext->sPrivatePassword.detach();
    pContext->baPrivatePassword = mapProperties.value(XBinary::UNPACK_PROP_PASSWORD_BYTES).toByteArray();
    pContext->baPrivatePassword.detach();
}

QMap<XBinary::UNPACK_PROP, QVariant> privateUnpackProperties(const XSFX::UNPACK_CONTEXT *pContext)
{
    QMap<XBinary::UNPACK_PROP, QVariant> result;
    if (!pContext) return result;
    result = pContext->mapPrivateUnpackProperties;
    if (!pContext->baPrivatePassword.isEmpty()) {
        result.insert(XBinary::UNPACK_PROP_PASSWORD_BYTES, pContext->baPrivatePassword);
    } else if (!pContext->sPrivatePassword.isEmpty()) {
        result.insert(XBinary::UNPACK_PROP_PASSWORD, pContext->sPrivatePassword);
    }
    return result;
}

void scrubPublicUnpackProperties(QMap<XBinary::UNPACK_PROP, QVariant> *pProperties)
{
    if (!pProperties) return;
    pProperties->remove(XBinary::UNPACK_PROP_PASSWORD);
    pProperties->remove(XBinary::UNPACK_PROP_PASSWORD_BYTES);
}

}  // namespace

XSFX::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) {
        clearPrivateUnpackCredential(pContext);
        if (pContext->pArchive) {
            pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }
        delete pContext;
    }
}

XSFX::XSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_UNKNOWN)
{
}

XSFX::XSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress, FT requiredArcType)
    : XBinary(pDevice, bIsImage, nModuleAddress), m_requiredArcType(requiredArcType)
{
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XSFX::~XSFX()
{
}

bool XSFX::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
}

bool XSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}

XSFX::INTERNAL_INFO XSFX::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XSFX::handleInternalInfo(PDSTRUCT *pPdStruct)
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
        if (info.bResourceIndeterminate) {
            // Resource reservations are transient. Leave the internal-info
            // transaction uncommitted so a later caller can retry detection.
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

void *XSFX::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XSFX::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        const INTERNAL_INFO info = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        if ((m_requiredArcType != FT_UNKNOWN) && info.bIsValid && (info.arcType != m_requiredArcType)) {
            m_internalInfo = INTERNAL_INFO();
            setIsInternalInfoHandled(false);
            XBinary::setInternalInfo(nullptr);
            return;
        }
        m_internalInfo = info;
        setIsInternalInfoHandled(true);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        setIsInternalInfoHandled(false);
        XBinary::setInternalInfo(nullptr);
    }
}

XBinary::FT XSFX::getFileType()
{
    FT arcType = m_requiredArcType;
    if ((arcType == FT_UNKNOWN) && m_internalInfo.bIsValid) {
        arcType = m_internalInfo.arcType;
    }

    XPE pe(getDevice());

    if (pe.isValid()) {
        const bool b64 = pe.is64();
        switch (arcType) {
            case FT_ZIP: return b64 ? FT_PE64_ZIPSFX : FT_PE32_ZIPSFX;
            case FT_RAR: return b64 ? FT_PE64_RARSFX : FT_PE32_RARSFX;
            case FT_CAB: return b64 ? FT_PE64_CABSFX : FT_PE32_CABSFX;
            case FT_FREEARC: return b64 ? FT_PE64_FREEARCSFX : FT_PE32_FREEARCSFX;
            case FT_ZPAQ: return b64 ? FT_PE64_ZPAQSFX : FT_PE32_ZPAQSFX;
            case FT_ARC: return FT_ARCSFX;
            case FT_ARJ: return FT_ARJSFX;
            case FT_LHA: return FT_LHASFX;
            case FT_GZIP: return FT_GZIPSFX;
            case FT_BZIP2: return FT_BZIP2SFX;
            case FT_KWAJ: return FT_KWAJSFX;
            case FT_SZDD: return FT_SZDDSFX;
            case FT_PYINSTALLER_SFX: return FT_PYINSTALLER_SFX;
            case FT_ARQ: return FT_ARQSFX;
            case FT_SQZ: return FT_SQZSFX;
            case FT_RTPATCH: return FT_RTPATCHSFX;
            case FT_BSN: return FT_BSNSFX;
            case FT_TGCF: return FT_TGCFSFX;
            case FT_RTA: return FT_RTASFX;
            case FT_ACE: return FT_ACESFX;
            case FT_ASYMETRIX: return FT_ASYMETRIXSFX;
            default: return b64 ? FT_PE64_SFX : FT_PE32_SFX;
        }
    }

    XELF elf(getDevice(), isImage(), getModuleAddress());
    if (elf.isValid()) {
        const bool b64 = elf.is64();
        switch (arcType) {
            case FT_ZIP: return b64 ? FT_ELF64_ZIPSFX : FT_ELF32_ZIPSFX;
            case FT_RAR: return b64 ? FT_ELF64_RARSFX : FT_ELF32_RARSFX;
            case FT_CAB: return b64 ? FT_ELF64_CABSFX : FT_ELF32_CABSFX;
            case FT_FREEARC: return b64 ? FT_ELF64_FREEARCSFX : FT_ELF32_FREEARCSFX;
            case FT_ZPAQ: return b64 ? FT_ELF64_ZPAQSFX : FT_ELF32_ZPAQSFX;
            case FT_ARC: return FT_ARCSFX;
            case FT_ARJ: return FT_ARJSFX;
            case FT_LHA: return FT_LHASFX;
            case FT_GZIP: return FT_GZIPSFX;
            case FT_BZIP2: return FT_BZIP2SFX;
            case FT_KWAJ: return FT_KWAJSFX;
            case FT_SZDD: return FT_SZDDSFX;
            case FT_PYINSTALLER_SFX: return FT_PYINSTALLER_SFX;
            case FT_ARQ: return FT_ARQSFX;
            case FT_SQZ: return FT_SQZSFX;
            case FT_RTPATCH: return FT_RTPATCHSFX;
            case FT_BSN: return FT_BSNSFX;
            case FT_TGCF: return FT_TGCFSFX;
            case FT_RTA: return FT_RTASFX;
            case FT_ACE: return FT_ACESFX;
            case FT_ASYMETRIX: return FT_ASYMETRIXSFX;
            default: return b64 ? FT_ELF64_SFX : FT_ELF32_SFX;
        }
    }

    switch (arcType) {
        case FT_ZIP: return FT_ZIPSFX;
        case FT_RAR: return FT_RARSFX;
        case FT_CAB: return FT_CABSFX;
        case FT_FREEARC: return FT_FREEARCSFX;
        case FT_ZPAQ: return FT_ZPAQSFX;
        case FT_ARC: return FT_ARCSFX;
        case FT_ARJ: return FT_ARJSFX;
        case FT_LHA: return FT_LHASFX;
        case FT_GZIP: return FT_GZIPSFX;
        case FT_BZIP2: return FT_BZIP2SFX;
        case FT_KWAJ: return FT_KWAJSFX;
        case FT_SZDD: return FT_SZDDSFX;
        case FT_PYINSTALLER_SFX: return FT_PYINSTALLER_SFX;
        case FT_ARQ: return FT_ARQSFX;
        case FT_SQZ: return FT_SQZSFX;
        case FT_RTPATCH: return FT_RTPATCHSFX;
        case FT_BSN: return FT_BSNSFX;
        case FT_TGCF: return FT_TGCFSFX;
        case FT_RTA: return FT_RTASFX;
        case FT_ACE: return FT_ACESFX;
        case FT_ASYMETRIX: return FT_ASYMETRIXSFX;
        default: break;
    }

    return FT_PE32_SFX;
}

QString XSFX::getArch()
{
    XNE ne(getDevice(), isImage(), getModuleAddress());
    if (ne.isValid()) {
        return ne.getArch();
    }
    XMSDOS msdos(getDevice(), isImage(), getModuleAddress());
    if (msdos.isValid()) {
        return msdos.getArch();
    }
    XELF elf(getDevice(), isImage(), getModuleAddress());
    if (elf.isValid()) return elf.getArch();
    XAtariST atariST(getDevice(), isImage(), getModuleAddress());
    if (atariST.isValid()) return atariST.getArch();
    return QString();
}

XBinary::MODE XSFX::getMode()
{
    return MODE_DATA;
}

QString XSFX::getMIMEString()
{
    return "application/x-sfx";
}

bool XSFX::_matchArchiveAt(qint64 nOffset, qint64 nSize, FT *pType, qint64 *pArchiveSize, PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache,
                           XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache, bool *pbProvisional, bool *pbResourceIndeterminate, bool *pbUseOuterDevice)
{
    if (pbProvisional) *pbProvisional = false;
    if (pbResourceIndeterminate) *pbResourceIndeterminate = false;
    if (pbUseOuterDevice) *pbUseOuterDevice = false;
    const qint64 nTotalSize = getSize();
    if (!pType || !pArchiveSize || !pFreeArcScanCache || !pbProvisional || !pbResourceIndeterminate || !pbUseOuterDevice || (nOffset < 0) || (nSize < 8) ||
        (nOffset > nTotalSize) || (nSize > nTotalSize - nOffset) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QByteArray baMagic = read_array_process(nOffset, qMin(nSize, (qint64)16), pPdStruct);
    if (baMagic.size() < 6) {
        return false;
    }
    const quint8 *p = (const quint8 *)baMagic.constData();

    FT candidate = FT_UNKNOWN;

    if ((baMagic.size() >= 14) && (p[4] == 0x00) && (p[7] == 0x2A) && (p[8] == 0x2A) && (p[9] == 0x41) && (p[10] == 0x43) && (p[11] == 0x45) && (p[12] == 0x2A) &&
        (p[13] == 0x2A)) {
        // ACE archive header: HEAD_TYPE 0 (archive header) at +4 and the
        // seven-byte ACESIGN "**ACE**" at +7.  XACE below walks the complete
        // block chain and verifies every header CRC, so this cheap eight-byte
        // test only decides which reader gets to answer.  It is tested first
        // because an ACE header begins with its own HEAD_CRC, whose two
        // arbitrary bytes can otherwise spell one of the short generic
        // prefixes below and lose the candidate to a family that then fails.
        candidate = FT_ACE;
    } else if ((p[0] == 0x37) && (p[1] == 0x7A) && (p[2] == 0xBC) && (p[3] == 0xAF) && (p[4] == 0x27) && (p[5] == 0x1C)) {
        candidate = FT_7Z;  // '7z' BC AF 27 1C
    } else if ((p[0] == 0x50) && (p[1] == 0x4B) && (((p[2] == 0x03) && (p[3] == 0x04)) || ((p[2] == 0x05) && (p[3] == 0x06)))) {
        candidate = FT_ZIP;  // local header or empty archive EOCD
    } else if ((p[0] == 0x52) && (p[1] == 0x61) && (p[2] == 0x72) && (p[3] == 0x21) && (p[4] == 0x1A) && (p[5] == 0x07)) {
        candidate = FT_RAR;  // 'Rar!' 1A 07 (RAR4/RAR5)
    } else if ((p[0] == 0x52) && (p[1] == 0x45) && (p[2] == 0x7E) && (p[3] == 0x5E)) {
        candidate = FT_RAR;  // 'RE~^' (RAR1.4)
    } else if ((p[0] == 0x4D) && (p[1] == 0x53) && (p[2] == 0x43) && (p[3] == 0x46)) {
        candidate = FT_CAB;  // 'MSCF'
    } else if ((baMagic.size() >= 13) && (p[0] == 0x41) && (p[1] == 0x72) && (p[2] == 0x43) && (p[3] == 0x01) && (p[8] == 0x41) && (p[9] == 0x72) && (p[10] == 0x43) &&
               (p[11] == 0x01) && (p[12] == 0x02)) {
        // Header followed by the first data block. A single ArC\x01 is common
        // inside the executable stubs and is deliberately not enough.
        candidate = FT_FREEARC;
    } else if ((baMagic.size() >= 16) && (p[0] == 0x37) && (p[1] == 0x6B) && (p[2] == 0x53) && (p[3] == 0x74) && (p[4] == 0xA0) && (p[5] == 0x31) && (p[6] == 0x83) &&
               (p[7] == 0xD3) && (p[8] == 0x8C) && (p[9] == 0xB2) && (p[10] == 0x28) && (p[11] == 0xB0) && (p[12] == 0xD3) && (p[13] == 0x7A) && (p[14] == 0x50) &&
               (p[15] == 0x51)) {
        candidate = FT_ZPAQ;  // 13-byte locator tag followed by 'zPQ'
    } else if ((p[0] == 0x7A) && (p[1] == 0x50) && (p[2] == 0x51)) {
        // Untagged zPQ is accepted only when the caller checks the exact
        // executable boundary. It is never one of the fallback scan patterns.
        candidate = FT_ZPAQ;
    } else if ((p[0] == 0x1A) && (p[1] >= 0x01) && (p[1] <= 0x0B)) {
        candidate = FT_ARC;
    } else if ((p[0] == 0x60) && (p[1] == 0xEA)) {
        candidate = FT_ARJ;
    } else if ((baMagic.size() >= 7) && (p[2] == 0x2D) && (p[6] == 0x2D) &&
               (((p[3] == 0x6C) && ((p[4] == 0x68) || (p[4] == 0x7A))) || ((p[3] == 0x70) && (p[4] == 0x6D)))) {
        candidate = FT_LHA;
    } else if ((baMagic.size() >= 10) && (p[0] == 0x1F) && (p[1] == 0x8B) && (p[2] == 0x08)) {
        candidate = FT_GZIP;
    } else if ((baMagic.size() >= 10) && (p[0] == 'B') && (p[1] == 'Z') && (p[2] == 'h') && (p[3] >= '1') && (p[3] <= '9') &&
               ((memcmp(p + 4, "1AY&SY", 6) == 0) || (memcmp(p + 4, "\x17\x72\x45\x38\x50\x90", 6) == 0))) {
        // The reference implementation SFX109 / archive17 share probe004330b0: BZh1..9 plus
        // the data-block or empty-stream marker. The payload is never run.
        candidate = FT_BZIP2;
    } else if ((baMagic.size() >= 8) && (p[0] == 0x4B) && (p[1] == 0x57) && (p[2] == 0x41) && (p[3] == 0x4A) && (p[4] == 0x88) &&
               (p[5] == 0xF0) && (p[6] == 0x27) && (p[7] == 0xD1)) {
        candidate = FT_KWAJ;
    } else if ((baMagic.size() >= 8) &&
               (((p[0] == 0x53) && (p[1] == 0x5A) && (p[2] == 0x44) && (p[3] == 0x44) && (p[4] == 0x88) && (p[5] == 0xF0) &&
                 (p[6] == 0x27) && ((p[7] == 0x33) || (p[7] == 0x3A))) ||
                ((p[0] == 0x5A) && (p[1] == 0x44) && (p[2] == 0x44) && (p[3] == 0x88) && (p[4] == 0xF0) && (p[5] == 0x27) &&
                 ((p[6] == 0x33) || (p[6] == 0x3A)) && (p[7] == 0x41)))) {
        candidate = FT_SZDD;
    } else if ((baMagic.size() >= 11) &&
               (p[0] == 0x01) && (p[1] == 0xCA) &&
               (memcmp(p + 2, "Copyright", 9) == 0)) {
        // NeoBook/NeoShow launchers append a complete NeoSoft GX Library.
        // The long copyright preamble is the format's native signature.
        candidate = FT_DEARK_LEGACY_ARCHIVE;
    } else if ((p[0] == 0x67) && (p[1] == 0x57) &&
               (p[2] == 0x04) && (p[3] == 0x02)) {
        candidate = FT_ARQ;
    } else if ((p[0] == 0x48) && (p[1] == 0x4c) &&
               (p[2] == 0x53) && (p[3] == 0x51) &&
               (p[4] == 0x5a)) {
        candidate = FT_SQZ;
    } else if ((p[0] == 0x4b) && (p[1] == 0x2a)) {
        const quint16 nVersion = quint16(p[2]) | (quint16(p[3]) << 8);
        if ((nVersion != 110) && (nVersion != 200) &&
            (nVersion != 211) && (nVersion != 320) &&
            (nVersion != 400) && (nVersion != 410) &&
            (nVersion != 500) && (nVersion != 650)) {
            return false;
        }
        candidate = FT_RTPATCH;
    } else if ((p[0] == 0xff) && (p[1] == 0x42) && (p[2] == 0x53) && (p[3] == 0x47)) {
        // 0xFF 'BSG' plus the big-endian container version. XBSN below walks the
        // complete member chain and verifies every member header's stored CRC32,
        // so this cheap prefix only decides which reader gets to answer.
        const quint16 nVersion = (quint16(p[4]) << 8) | quint16(p[5]);
        if (nVersion > 0x000f) {
            return false;
        }
        candidate = FT_BSN;
    } else if ((baMagic.size() >= 8) && (p[0] == 0x54) && (p[1] == 0x47) && (p[2] == 0x43) && (p[3] == 0x46) && (p[4] == 0x00) && (p[5] == 0x24)) {
        // "Setup Specialist": 'TGCF' plus the big-endian 0x0024 header-record
        // size and one of the three reader versions XTGCFArchive parses.  The
        // container walk below still has to accept every member record and
        // keep each payload extent inside the candidate, so this cheap prefix
        // only decides which reader gets to answer.
        const quint16 nTgcfVersion = ((quint16)p[6] << 8) | (quint16)p[7];
        if ((nTgcfVersion != 0x0130) && (nTgcfVersion != 0x0140) && (nTgcfVersion != 0x0160)) {
            return false;
        }
        candidate = FT_TGCF;
    } else if ((p[0] == 0x4B) && (p[1] == 0x4A) && (p[2] == 0x64) && (p[3] == 0x00) && (p[4] != 0x00)) {
        // "KJd\0" plus a non-empty first record name, which is what separates a
        // real RTA container from a file that merely opens with the magic: a
        // zero there is the archive's own end marker.  XRTA below walks the
        // complete record chain to that marker and requires every member stream
        // to open with the RTPatch codec's invariant prefix, so this cheap magic
        // only decides which reader gets to answer.
        candidate = FT_RTA;
    } else if ((baMagic.size() >= 8) && (p[0] == 0x60) && (p[1] == 0x22) && (p[2] == 0x13) && (p[3] == 0x63) && (p[4] == 0x6C) && (p[5] == 0x00) &&
               (p[6] == 0x00) && (p[7] == 0x00)) {
        // Asymetrix disk-set volume header: the 32-bit magic plus the 0x6C
        // directory-record stride, which is a hard constant of the writer - the
        // same eight bytes XAsymetrix itself gates on.  XAsymetrix below parses
        // the complete directory and walks every member's block chain, so this
        // cheap test only decides which reader gets to answer.
        candidate = FT_ASYMETRIX;
    } else if ((p[0] == 0x52) && (p[1] == 0x52) && (p[2] == 0x01) && (p[3] == 0x29)) {
        // Knowledge Dynamics Corp ".RED": 'RR', the version-1 record tag and
        // the 41-byte base header size.  Four bytes are far too weak on their
        // own, so require the complete member chain - every header's CRC-16 and
        // its multi-volume fields included - to cover this candidate exactly
        // before the bounded legacy decoder is even constructed.  This is one
        // walk for one offset the caller already chose, capped by
        // SFX_RED_MEMBER_LIMIT, so it needs no cross-candidate budget.
        if (!hasRedMemberChain(this, nOffset, nOffset + nSize, pPdStruct, nullptr)) {
            return false;
        }
        candidate = FT_DEARK_LEGACY_ARCHIVE;
    } else {
        return false;
    }

    if ((m_requiredArcType != FT_UNKNOWN) && (candidate != m_requiredArcType)) {
        return false;
    }
    if (candidate == FT_FREEARC) {
        const qint64 nMethodSize = qMin(nSize - 13, (qint64)256);
        if ((nMethodSize <= 0) || !hasFreeArcMethod(read_array_process(nOffset + 13, nMethodSize, pPdStruct))) {
            return false;
        }
    }

    // Some installer builders concatenate several complete, single-member
    // ZIP archives in one overlay. XZip intentionally locates the final EOCD,
    // which makes a view beginning at the first local header appear invalid.
    // Prefer the bounded concatenated adapter when the full sequence validates;
    // otherwise bound an ordinary candidate at the earliest authentic EOCD.
    if ((candidate == FT_ZIP) && (p[2] == 0x03) && (p[3] == 0x04)) {
        bool bConcatenated = false;
        SubDevice concatDevice(getDevice(), nOffset, nSize);
        if (concatDevice.open(QIODevice::ReadOnly)) {
            XConcatZipArchive concatZip(&concatDevice);
            if (concatZip.isValid(pPdStruct)) {
                const qint64 nConcatSize = concatZip.getFileFormatSize(pPdStruct);
                if ((nConcatSize > 0) && (nConcatSize <= nSize)) {
                    nSize = nConcatSize;
                    bConcatenated = true;
                }
            }
            concatDevice.close();
        }

        if (!bConcatenated) {
        const qint64 nCandidateEnd = nOffset + nSize;
        qint64 nSearchOffset = nOffset + 4;
        for (qint32 nAttempt = 0; (nAttempt < SFX_SIGNATURE_CANDIDATE_LIMIT) &&
                                   (nSearchOffset <= nCandidateEnd - 22) &&
                                   XBinary::isPdStructNotCanceled(pPdStruct);
             ++nAttempt) {
            const qint64 nEcdOffset = find_signature(nSearchOffset, nCandidateEnd - nSearchOffset,
                                                     "504B0506", nullptr, pPdStruct);
            if ((nEcdOffset < nSearchOffset) || (nEcdOffset > nCandidateEnd - 22)) break;
            const quint16 nCommentSize = read_uint16(nEcdOffset + 20);
            const qint64 nTrialEnd = nEcdOffset + 22 + nCommentSize;
            if ((nTrialEnd >= nEcdOffset + 22) && (nTrialEnd <= nCandidateEnd)) {
                const qint64 nTrialSize = nTrialEnd - nOffset;
                SubDevice trialDevice(getDevice(), nOffset, nTrialSize);
                if (trialDevice.open(QIODevice::ReadOnly)) {
                    XZip trialZip(&trialDevice);
                    UNPACK_STATE trialState = {};
                    QMap<UNPACK_PROP, QVariant> trialProperties;
                    bool bTrialValid = trialZip.initUnpack(&trialState, trialProperties, pPdStruct);
                    if (bTrialValid) bTrialValid = trialZip.finishUnpack(&trialState, pPdStruct);
                    if (bTrialValid && (trialZip.getFileFormatSize(pPdStruct) == nTrialSize)) {
                        nSize = nTrialSize;
                        trialDevice.close();
                        break;
                    }
                    trialDevice.close();
                }
            }
            nSearchOffset = nEcdOffset + 4;
        }
        }
    }

    // Confirm the candidate really is a valid archive at that offset.
    SubDevice sub(getDevice(), nOffset, nSize);
    if (!sub.open(QIODevice::ReadOnly)) {
        return false;
    }

    bool bValid = false;
    qint64 nLogicalSize = 0;
    // Standard LHA/LZH streams have a complete structural validator. Avoid
    // running the Deark adapter (which materializes a temporary ZIP) merely
    // to decide whether a candidate is an archive; the Deark reader remains
    // the extraction backend and the fallback for legacy dialects.
    if (candidate == FT_LHA) {
        XLHA lha(&sub);
        if (lha.isValid(pPdStruct)) {
            const qint64 nLhaSize = lha.getFileFormatSize(pPdStruct);
            // A few Atari installers contain several independently framed LHA
            // members separated by loader-owned gaps.  XLHA quite correctly
            // reports only the first contiguous archive there.  Do not let
            // that short extent suppress the Deark fallback: the later Atari
            // carrier gate would reject it for its large apparent trailer,
            // even though Deark can enumerate the complete framed sequence.
            // Ordinary SFX archives leave at most the same bounded trailer the
            // carrier policy already permits, and retain the cheap path.
            if ((nLhaSize > 0) && (nLhaSize <= nSize) &&
                ((nSize - nLhaSize) <= SFX_ATARIST_TRAILER_LIMIT)) {
                bValid = true;
                nLogicalSize = nLhaSize;
            }
        }
    }
    XArchive *pArc = bValid ? nullptr : _createArchive(candidate, &sub);
    if (pArc) {
        if (candidate == FT_FREEARC) {
            // Format probing must remain structural: archive initialization now
            // publishes only helper metadata, while the first member extraction
            // authenticates and stages the payload through the external adapter.
            XFREEARC *pFreeArc = static_cast<XFREEARC *>(pArc);
            FREEARC_LAYOUT_STATUS layoutStatus = FREEARC_LAYOUT_INVALID;
            if (pFreeArc->isValid(pPdStruct)) {
                layoutStatus = getFreeArcBlockLayoutStatus(pFreeArc, nOffset, pFreeArcScanCache, pPdStruct);
            }
            bValid = (layoutStatus == FREEARC_LAYOUT_AUTHENTICATED) || (layoutStatus == FREEARC_LAYOUT_PROVISIONAL);
            *pbProvisional = (layoutStatus == FREEARC_LAYOUT_PROVISIONAL);
            *pbResourceIndeterminate = (layoutStatus == FREEARC_LAYOUT_RESOURCE_LIMIT);
            nLogicalSize = bValid ? nSize : 0;
        } else if (candidate == FT_ZPAQ) {
            XZPAQ *pZpaq = static_cast<XZPAQ *>(pArc);
            bValid = pZpaq->isValid(pPdStruct) && hasZpaqBlockLayout(pZpaq, this, nOffset, pZpaqScanCache, pPdStruct);
            // Local parsing proves the first block's framing but deliberately
            // does not decompress/authenticate its data. If first extraction
            // rejects it, unpackCurrent may select a later manifest-equivalent
            // same-family candidate without exposing partial caller output.
            *pbProvisional = bValid;
            nLogicalSize = bValid ? nSize : 0;
        } else {
            UNPACK_STATE state = {};
            QMap<UNPACK_PROP, QVariant> properties;
            bValid = pArc->initUnpack(&state, properties, pPdStruct);
            if (bValid) {
                bValid = pArc->finishUnpack(&state, pPdStruct);
            } else if ((candidate == FT_7Z) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                // Preserve detection of password-protected encoded headers while
                // still requiring a structurally parsed encrypted stream map.
                XSevenZip *pSevenZip = static_cast<XSevenZip *>(pArc);
                bValid = pSevenZip->isValid(pPdStruct) && pSevenZip->isEncrypted();
            }
            if (bValid) nLogicalSize = pArc->getFileFormatSize(pPdStruct);
        }
        delete pArc;
    }
    sub.close();

    // Some ZIP SFX writers store central-directory and local-header offsets
    // as absolute offsets in the outer executable. Such archives cannot be
    // parsed through a rebased SubDevice. Validate them on the complete input,
    // then require the first authenticated local record (or an empty EOCD) to
    // begin at this exact overlay candidate before delegating extraction to the
    // same whole-device XZip view.
    if (!bValid && (candidate == FT_ZIP) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        XZip outerZip(getDevice());
        const qint64 nECDOffset = outerZip.findECDOffset(pPdStruct);
        bool bStartsAtCandidate = false;
        if (nECDOffset >= 0) {
            const quint16 nRecords = outerZip.read_uint16(nECDOffset + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));
            if (nRecords == 0) {
                bStartsAtCandidate = (nECDOffset == nOffset);
            } else {
                qint64 nCentralOffset = outerZip.read_uint32(nECDOffset + offsetof(XZip::ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));
                qint64 nFirstLocalOffset = (std::numeric_limits<qint64>::max)();
                bool bCentralValid = true;
                for (quint32 i = 0; i < nRecords; ++i) {
                    if ((nCentralOffset < 0) || ((nECDOffset - nCentralOffset) < (qint64)sizeof(XZip::CENTRALDIRECTORYFILEHEADER))) {
                        bCentralValid = false;
                        break;
                    }
                    const XZip::CENTRALDIRECTORYFILEHEADER header = outerZip.read_CENTRALDIRECTORYFILEHEADER(nCentralOffset, pPdStruct);
                    if (header.nSignature != XZip::SIGNATURE_CFD) {
                        bCentralValid = false;
                        break;
                    }
                    nFirstLocalOffset = qMin(nFirstLocalOffset, (qint64)header.nOffsetToLocalFileHeader);
                    const qint64 nRecordSize =
                        sizeof(XZip::CENTRALDIRECTORYFILEHEADER) + (qint64)header.nFileNameLength + (qint64)header.nExtraFieldLength + (qint64)header.nFileCommentLength;
                    if ((nRecordSize <= 0) || (nRecordSize > nECDOffset - nCentralOffset)) {
                        bCentralValid = false;
                        break;
                    }
                    nCentralOffset += nRecordSize;
                }
                bStartsAtCandidate = bCentralValid && (nFirstLocalOffset == nOffset);
            }
        }

        if (bStartsAtCandidate) {
            UNPACK_STATE state = {};
            QMap<UNPACK_PROP, QVariant> properties;
            bValid = outerZip.initUnpack(&state, properties, pPdStruct);
            if (bValid) {
                bValid = outerZip.finishUnpack(&state, pPdStruct);
            }
            const qint64 nOuterLogicalSize = bValid ? outerZip.getFileFormatSize(pPdStruct) : 0;
            if (bValid && (nOuterLogicalSize > nOffset) && (nOuterLogicalSize <= nTotalSize)) {
                nLogicalSize = nOuterLogicalSize - nOffset;
                *pbUseOuterDevice = true;
            } else {
                bValid = false;
            }
        }
    }

    // WinImage self-extractors write a hybrid footer: the end-of-central-
    // directory record addresses the central directory relative to the
    // appended archive, while every central record addresses its local header
    // by its absolute position in the outer executable. Neither the rebased
    // SubDevice view above nor the whole-device XZip branch can resolve that
    // mixture. The bounded adapter derives the archive's true start from the
    // footer arithmetic and then requires every absolute link to name the next
    // real, non-overlapping local header, so nothing about the delta is
    // guessed. Extraction uses the same whole-device view.
    if (!bValid && (candidate == FT_ZIP) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        XWinImageZipArchive winImageZip(getDevice());
        UNPACK_STATE winImageState = {};
        QMap<UNPACK_PROP, QVariant> winImageProperties;
        if (winImageZip.initUnpack(&winImageState, winImageProperties, pPdStruct)) {
            const qint64 nFirstHeaderOffset = winImageState.nCurrentOffset;
            const bool bWinImageFinished = winImageZip.finishUnpack(&winImageState, pPdStruct);
            const qint64 nWinImageLogicalSize = bWinImageFinished ? winImageZip.getFileFormatSize(pPdStruct) : 0;
            if (bWinImageFinished && (nFirstHeaderOffset == nOffset) &&
                (nWinImageLogicalSize > nOffset) && (nWinImageLogicalSize <= nTotalSize)) {
                nLogicalSize = nWinImageLogicalSize - nOffset;
                *pbUseOuterDevice = true;
                bValid = true;
            }
        }
    }

    if (bValid && (nLogicalSize > 0) && (nLogicalSize <= nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        *pType = candidate;
        *pArchiveSize = nLogicalSize;
        return true;
    }

    return false;
}

// Fresh XSFX-family instances (the probe chain, the listing phase, and the
// extract phase) each repeat the same whole-window scan.  The result depends
// only on the device content and the instance's required archive type, so it
// is cached per (device, required type) as a dynamic property on the device —
// the same pattern XFormats::_getFileTypes uses for "filetypes".  No result
// is ever inferred across required types: a specialized scan differs from the
// generic one in scan range (embedded-image types start at offset 0) and in
// per-family candidate treatment, so only an exact-type entry is authoritative.
XSFX::INTERNAL_INFO XSFX::_detect(PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache, XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache, qint64 nMinimumArchiveOffset)
{
    // The zpaq/freearc scan caches are pure performance helpers and never
    // change the scan result, so only a minimum-offset restriction (the
    // multi-payload retry path) disqualifies a call from the shared cache.
    const bool bDefaultCall = (nMinimumArchiveOffset < 0);
    QIODevice *pCacheDevice = getDevice();
    const qint64 nCacheDeviceSize = pCacheDevice ? getSize() : -1;
    const QString sCacheKey = QStringLiteral("xsfx_detect_t%1").arg(static_cast<qint32>(m_requiredArcType));

    if (bDefaultCall && pCacheDevice && (nCacheDeviceSize >= 0)) {
        const QVariantList listCached = pCacheDevice->property(sCacheKey.toLatin1().constData()).toList();
        if ((listCached.size() == 9) && (listCached.at(0).toLongLong() == nCacheDeviceSize)) {
            INTERNAL_INFO cached = {};
            cached.bIsValid = listCached.at(1).toBool();
            cached.arcType = static_cast<FT>(listCached.at(2).toInt());
            cached.nArchiveOffset = listCached.at(3).toLongLong();
            cached.nArchiveSize = listCached.at(4).toLongLong();
            cached.bProvisional = listCached.at(5).toBool();
            cached.bUseOuterDevice = listCached.at(6).toBool();
            cached.bResourceIndeterminate = listCached.at(7).toBool();
            cached.bAllowOpaqueZpaq = listCached.at(8).toBool();
            return cached;
        }
    }

    INTERNAL_INFO scanned = _detectScan(pPdStruct, pZpaqScanCache, pFreeArcScanCache, nMinimumArchiveOffset);

    // A canceled or resource-indeterminate scan is never stored.
    if (bDefaultCall && pCacheDevice && (nCacheDeviceSize >= 0) && XBinary::isPdStructNotCanceled(pPdStruct) && !scanned.bResourceIndeterminate) {
        QVariantList listStore;
        listStore << nCacheDeviceSize << scanned.bIsValid << static_cast<qint32>(scanned.arcType) << scanned.nArchiveOffset << scanned.nArchiveSize
                  << scanned.bProvisional << scanned.bUseOuterDevice << scanned.bResourceIndeterminate << scanned.bAllowOpaqueZpaq;
        pCacheDevice->setProperty(sCacheKey.toLatin1().constData(), listStore);
    }

    return scanned;
}

struct XSFX::SCAN_CANDIDATE_EVALUATOR {
    SCAN_CANDIDATE_EVALUATOR(
        XSFX *pOwner, INTERNAL_INFO *pResult,
        const QSet<qint64> *pTestedOverlayOffsets,
        qint64 nTotalSize, qint64 nScanStart, qint64 nScanEnd,
        qint64 nOverlayOffset, bool bAtariST,
        const char *const *ppSignatures,
        const qint32 *pCandidateAdjustments, qint32 nSignatureCount,
        qint64 *pNextCandidate, qint32 *pCandidateCounts,
        qint32 *pCandidateIndices, QList<qint64> *pCandidateLists,
        PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache,
        XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache)
        : m_pOwner(pOwner),
          m_pResult(pResult),
          m_pTestedOverlayOffsets(pTestedOverlayOffsets),
          m_nTotalSize(nTotalSize),
          m_nScanStart(nScanStart),
          m_nScanEnd(nScanEnd),
          m_nOverlayOffset(nOverlayOffset),
          m_bAtariST(bAtariST),
          m_ppSignatures(ppSignatures),
          m_pCandidateAdjustments(pCandidateAdjustments),
          m_nSignatureCount(nSignatureCount),
          m_pNextCandidate(pNextCandidate),
          m_pCandidateCounts(pCandidateCounts),
          m_pCandidateIndices(pCandidateIndices),
          m_pCandidateLists(pCandidateLists),
          m_pPdStruct(pPdStruct),
          m_pZpaqScanCache(pZpaqScanCache),
          m_pFreeArcScanCache(pFreeArcScanCache)
    {
    }

    void loadNextCandidate(qint32 nIndex)
    {
        m_pNextCandidate[nIndex] = -1;
        while ((m_pCandidateCounts[nIndex] <
                SFX_SIGNATURE_CANDIDATE_LIMIT) &&
               (m_pCandidateIndices[nIndex] <
                m_pCandidateLists[nIndex].count())) {
            const qint64 nCandidate = m_pCandidateLists[nIndex].at(
                m_pCandidateIndices[nIndex]);
            if ((nCandidate + m_pCandidateAdjustments[nIndex]) >=
                m_nScanStart) {
                m_pNextCandidate[nIndex] = nCandidate;
                break;
            }
            ++m_pCandidateCounts[nIndex];
            ++m_pCandidateIndices[nIndex];
        }
    }

    qint64 nextCollectedOffset(qint32 nSignatureIndex,
                               qint64 nNextSearchOffset)
    {
        const QList<qint64> &listOffsets =
            m_pCandidateLists[nSignatureIndex];
        const QList<qint64>::const_iterator it = std::lower_bound(
            listOffsets.cbegin(), listOffsets.cend(), nNextSearchOffset);
        if (it != listOffsets.cend()) return *it;
        // Collection deliberately stops one item past the evaluation limit.
        // Preserve exact behaviour for a pathologically dense signature by
        // falling back only when the retained list may have been truncated.
        if (listOffsets.count() >= (SFX_SIGNATURE_CANDIDATE_LIMIT + 1)) {
            return m_pOwner->find_signature(
                nNextSearchOffset, m_nScanEnd - nNextSearchOffset,
                m_ppSignatures[nSignatureIndex], nullptr, m_pPdStruct);
        }
        return -1;
    }

    bool evaluate(qint64 nEvaluationEnd)
    {
        for (qint32 i = 0; i < m_nSignatureCount; ++i) {
            if ((m_pNextCandidate[i] < 0) &&
                (m_pCandidateCounts[i] < SFX_SIGNATURE_CANDIDATE_LIMIT) &&
                (m_pCandidateIndices[i] < m_pCandidateLists[i].count())) {
                loadNextCandidate(i);
            }
        }

        while (XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            qint64 nPos = -1;
            for (qint32 i = 0; i < m_nSignatureCount; ++i) {
                const qint64 nAdjustedCandidate =
                    (m_pNextCandidate[i] >= 0)
                        ? (m_pNextCandidate[i] +
                           m_pCandidateAdjustments[i])
                        : -1;
                if ((nAdjustedCandidate >= m_nScanStart) &&
                    ((nPos == -1) || (nAdjustedCandidate < nPos))) {
                    nPos = nAdjustedCandidate;
                }
            }
            if ((nPos < m_nScanStart) || (nPos >= m_nScanEnd) ||
                (nPos >= nEvaluationEnd)) {
                break;
            }

            FT type = FT_UNKNOWN;
            qint64 nArchiveSize = 0;
            bool bProvisional = false;
            bool bResourceIndeterminate = false;
            bool bUseOuterDevice = false;
            if (!m_pTestedOverlayOffsets->contains(nPos)) {
                qint64 nCandidateSize = m_nTotalSize - nPos;
                // KWAJ and SZDD store the uncompressed length but not the
                // packed byte count. Resource-based installers concatenate
                // independently aligned streams, so cap the current view at
                // the next same-family header instead of treating the rest of
                // the executable as data.
                if ((m_pOwner->m_requiredArcType == FT_KWAJ) ||
                    (m_pOwner->m_requiredArcType == FT_SZDD)) {
                    qint64 nNextFamilyOffset = -1;
                    // Standard SZDD also contains the legacy magic beginning
                    // one byte later ("SZDD..." vs "ZDD..."). Skip the
                    // complete current signature so that overlap cannot
                    // truncate it to a one-byte candidate.
                    const qint64 nNextSearchOffset = nPos + 8;
                    if (nNextSearchOffset < m_nScanEnd) {
                        const qint32 nFirstIndex =
                            (m_pOwner->m_requiredArcType == FT_KWAJ) ? 24 : 25;
                        const qint32 nLastIndex =
                            (m_pOwner->m_requiredArcType == FT_KWAJ) ? 24 : 28;
                        for (qint32 nSignatureIndex = nFirstIndex;
                             nSignatureIndex <= nLastIndex;
                             ++nSignatureIndex) {
                            const qint64 nNext = nextCollectedOffset(
                                nSignatureIndex, nNextSearchOffset);
                            if ((nNext >= 0) &&
                                ((nNextFamilyOffset < 0) ||
                                 (nNext < nNextFamilyOffset))) {
                                nNextFamilyOffset = nNext;
                            }
                        }
                    }
                    if ((nNextFamilyOffset > nPos) &&
                        (nNextFamilyOffset <= m_nTotalSize)) {
                        nCandidateSize = nNextFamilyOffset - nPos;
                        // SZDD resource records are sector-aligned with zero
                        // fill, while the decoder stops at the declared output
                        // size. Keep the physical extent precise without
                        // applying this heuristic to KWAJ streams, whose valid
                        // compressed data may itself end in zero bytes.
                        if (m_pOwner->m_requiredArcType == FT_SZDD) {
                            const qint64 nTrimWindow = qMin<qint64>(
                                qMax<qint64>(0, nCandidateSize - 8), 4096);
                            if (nTrimWindow > 0) {
                                const QByteArray baTail =
                                    m_pOwner->read_array_process(
                                        nPos + nCandidateSize - nTrimWindow,
                                        nTrimWindow, m_pPdStruct);
                                if (!XBinary::isPdStructNotCanceled(
                                        m_pPdStruct) ||
                                    (baTail.size() != nTrimWindow)) {
                                    *m_pResult = INTERNAL_INFO();
                                    m_pResult->arcType = FT_UNKNOWN;
                                    return true;
                                }
                                qint64 nTrailingZeroes = 0;
                                while ((nTrailingZeroes < baTail.size()) &&
                                       (baTail.at(baTail.size() - 1 -
                                                  nTrailingZeroes) == 0)) {
                                    ++nTrailingZeroes;
                                }
                                nCandidateSize -= nTrailingZeroes;
                            }
                        }
                    }
                }
                bool bMatched = m_pOwner->_matchArchiveAt(
                    nPos, nCandidateSize, &type, &nArchiveSize, m_pPdStruct,
                    m_pZpaqScanCache, m_pFreeArcScanCache, &bProvisional,
                    &bResourceIndeterminate, &bUseOuterDevice);
                if (bMatched && m_bAtariST &&
                    (nPos < m_nOverlayOffset)) {
                    const qint64 nTrailerSize =
                        m_nTotalSize - nPos - nArchiveSize;
                    // Atari SFX images can keep relocation data after an
                    // archive embedded in the text image. Require both ends
                    // to remain near the executable boundaries so ordinary
                    // in-code tags or data resources cannot classify a
                    // program as an SFX container.
                    bMatched =
                        (nPos <= SFX_ATARIST_EMBEDDED_PREFIX_LIMIT) &&
                        (nTrailerSize >= 0) &&
                        (nTrailerSize <= SFX_ATARIST_TRAILER_LIMIT);
                }
                if (bMatched) {
                    m_pResult->bIsValid = true;
                    m_pResult->bProvisional = bProvisional;
                    m_pResult->arcType = type;
                    m_pResult->bUseOuterDevice = bUseOuterDevice;
                    m_pResult->nArchiveOffset = nPos;
                    m_pResult->nArchiveSize = nArchiveSize;
                    return true;
                }
            }
            if (bResourceIndeterminate) {
                m_pResult->bResourceIndeterminate = true;
                return true;
            }

            for (qint32 i = 0; i < m_nSignatureCount; ++i) {
                if ((m_pNextCandidate[i] >= 0) &&
                    ((m_pNextCandidate[i] + m_pCandidateAdjustments[i]) ==
                     nPos)) {
                    ++m_pCandidateCounts[i];
                    ++m_pCandidateIndices[i];
                    loadNextCandidate(i);
                }
            }
        }

        return false;
    }

private:
    XSFX *m_pOwner;
    INTERNAL_INFO *m_pResult;
    const QSet<qint64> *m_pTestedOverlayOffsets;
    qint64 m_nTotalSize;
    qint64 m_nScanStart;
    qint64 m_nScanEnd;
    qint64 m_nOverlayOffset;
    bool m_bAtariST;
    const char *const *m_ppSignatures;
    const qint32 *m_pCandidateAdjustments;
    qint32 m_nSignatureCount;
    qint64 *m_pNextCandidate;
    qint32 *m_pCandidateCounts;
    qint32 *m_pCandidateIndices;
    QList<qint64> *m_pCandidateLists;
    PDSTRUCT *m_pPdStruct;
    XSFX_ZPAQ_SCAN_CACHE *m_pZpaqScanCache;
    XSFX_FREEARC_SCAN_CACHE *m_pFreeArcScanCache;
};

// LHarc / LArc self-extractor stub locator.
//
// Two families were unreachable through the ordinary overlay scan:
//
//   * "LHARC SFX" carriers are raw DOS COM images.  The carrier gate below only
//     accepts a COM when the FILE NAME ends in ".COM", so a sample whose name
//     does not was rejected before any scanning began - copying each of the
//     eleven failing files to probe.COM was enough to make every one of them
//     list and extract.
//   * "SFX LHA" carriers are MZ executables whose header claims the WHOLE file
//     as the load image, so nOverlayOffset lands at EOF, nBaseScanStart is set
//     to it and the scan window is empty.
//
// One locator serves both, and it never depends on a file name.  The stub image
// starts at 0 for a raw COM and at e_cparhdr * 16 for a DOS MZ; the image's
// first byte is a short jump (0xEB), and past its displacement J sits the
// loader's fixed prologue, which names the family:
//
//   FC BC 00 01 BB 06 01 E8   LHarc, J is 0x60 or 0x6C
//   FC 8C C8 03 06 02 01 8E   LArc,  J is 0x1C
//
// The archive then begins at one of three fixed LHarc offsets (1260, 1263,
// 1290) or at LArc's single 594, whichever holds a "-l?" method tag at +2.
//
// Measured over the whole corpus: 33/33 and 436/436 hits in the two families,
// and ZERO hits across 11,622 files in 354 other families, so an early accept
// here cannot steal a file from another format.
static const qint64 SFX_LHA_STUB_WINDOW = 1300;

// The WinACE 32-bit self-extractor is the one ACE carrier that keeps its
// archive in the PE resource directory instead of behind the executable
// image: resource type "ARCDATA", resource name "DATA".  Its data entry gives
// both the exact offset and the exact size of the container, so the locator
// needs no scanning at all and cannot be confused by an ACE header that is
// merely a stored member of some other container.  The carrier has no overlay,
// which is exactly the shape the overlay probe and the overlay signature sweep
// below structurally cannot reach.
static const qint32 SFX_ACE_RESOURCE_LIMIT = 10000;

static bool getWinAceSfxResource(XPE *pPe, qint64 *pnOffset, qint64 *pnSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pPe || !pnOffset || !pnSize) return false;

    *pnOffset = -1;
    *pnSize = 0;

    QList<XPE::RESOURCE_RECORD> listRecords = pPe->getResources(SFX_ACE_RESOURCE_LIMIT, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const XPE::RESOURCE_RECORD record = XPE::getResourceRecord(QStringLiteral("ARCDATA"), QStringLiteral("DATA"), &listRecords);
    if ((record.nOffset < 0) || (record.nSize <= 0)) return false;

    *pnOffset = record.nOffset;
    *pnSize = record.nSize;

    return true;
}

static qint64 getLhaSfxStubBase(const QByteArray &baPrefix)
{
    if (baPrefix.size() < 10) return -1;
    const quint8 *pPrefix = (const quint8 *)baPrefix.constData();
    if (((pPrefix[0] == 'M') && (pPrefix[1] == 'Z')) || ((pPrefix[0] == 'Z') && (pPrefix[1] == 'M'))) {
        const qint64 nHeaderParagraphs = (qint64)pPrefix[8] | ((qint64)pPrefix[9] << 8);
        return nHeaderParagraphs * 16;
    }
    // A raw COM image: the loader is the file itself.
    return 0;
}

static bool isLhaSfxMethodTagAt(const QByteArray &baImage, qint64 nOffset)
{
    // "-l?-" style method tag: '-', 'l', 'h' or 'z', the method character and
    // a closing '-'.
    if ((nOffset < 0) || (nOffset > (baImage.size() - 5))) return false;
    const quint8 *pTag = (const quint8 *)baImage.constData() + nOffset;
    return (pTag[0] == '-') && (pTag[1] == 'l') && ((pTag[2] == 'h') || (pTag[2] == 'z')) && (pTag[4] == '-');
}

static qint64 getLhaSfxStubPayload(const QByteArray &baImage, qint64 nBase)
{
    static const quint8 anLHarcPrologue[8] = {0xfc, 0xbc, 0x00, 0x01, 0xbb, 0x06, 0x01, 0xe8};
    static const quint8 anLArcPrologue[8] = {0xfc, 0x8c, 0xc8, 0x03, 0x06, 0x02, 0x01, 0x8e};
    static const qint64 anLHarcPayloads[3] = {1260, 1263, 1290};

    if ((nBase < 0) || (nBase > (baImage.size() - 2))) return -1;
    const quint8 *pImage = (const quint8 *)baImage.constData();
    if (pImage[nBase] != 0xeb) return -1;

    const qint64 nDisplacement = (qint64)pImage[nBase + 1];
    const qint64 nPrologue = nBase + 2 + nDisplacement;
    if ((nPrologue < 0) || (nPrologue > (baImage.size() - 8))) return -1;

    const bool bLHarc = ((nDisplacement == 0x60) || (nDisplacement == 0x6c)) && (memcmp(pImage + nPrologue, anLHarcPrologue, 8) == 0);
    const bool bLArc = (nDisplacement == 0x1c) && (memcmp(pImage + nPrologue, anLArcPrologue, 8) == 0);
    if (!bLHarc && !bLArc) return -1;

    if (bLHarc) {
        for (qint32 i = 0; i < 3; ++i) {
            const qint64 nPayload = nBase + anLHarcPayloads[i];
            if (isLhaSfxMethodTagAt(baImage, nPayload + 2)) return nPayload;
        }
        return -1;
    }

    const qint64 nPayload = nBase + 594;

    return isLhaSfxMethodTagAt(baImage, nPayload + 2) ? nPayload : -1;
}

// RTPatch self-extractor payload locator.
//
// RTPatch 4.00 and later append the package at the carrier's overlay boundary,
// where the ordinary overlay probe already finds it.  3.20 instead links the
// package into the carrier itself, and neither shape of that carrier is
// reachable from the overlay scan: the two PE stubs claim the whole file as
// mapped image, so nOverlayOffset lands at EOF and the scan window is empty,
// and the six NE stubs keep the package inside the mapped image, which the
// overlay scan deliberately never searches.
//
// The generic signature table is deliberately left alone - an RTPatch magic is
// only two bytes, and adding it would make every unrelated SFX probe test every
// "K*" in its overlay.  This locator therefore runs ONLY for the explicit
// FT_RTPATCHSFX probe, and the offset it returns is still handed to
// _matchArchiveAt, so XRTPatch's complete container walk decides, exactly as it
// does for a scanned candidate.
//
// The predicate is the package header: "K*", a version word the reader actually
// parses, and for 3.20/4.00 the two invariants that separate a real header from
// compressed data that happens to spell the magic - the reserved uint32 at 0x14
// reads zero and the word at 0x18 reads the builder's fixed 4.  The invariants
// are applied to 3.20 and 4.00 only, so over the 73,823 files of the reference
// corpora - each read to this locator's own 16 MiB bound - the predicate
// selects 99 positions in 90 files: 69 genuine RTPatch packages, and 30
// positions in 21 unrelated files where an older version word happens to
// appear.  XRTPatch's container walk rejects all 21 of those, and every one of
// them keeps its existing identity.
static const qint32 SFX_RTPATCH_HEADER_SIZE = 0x1a;

static bool isRTPatchPackageHeaderAt(const QByteArray &baImage, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset > (baImage.size() - SFX_RTPATCH_HEADER_SIZE))) return false;
    const quint8 *pHeader = (const quint8 *)baImage.constData() + nOffset;
    if ((pHeader[0] != 0x4B) || (pHeader[1] != 0x2A)) return false;

    const quint16 nVersion = (quint16)pHeader[2] | ((quint16)pHeader[3] << 8);
    if ((nVersion != 110) && (nVersion != 200) &&
        (nVersion != 211) && (nVersion != 320) &&
        (nVersion != 400) && (nVersion != 410) &&
        (nVersion != 500) && (nVersion != 650)) {
        return false;
    }

    if ((nVersion == 320) || (nVersion == 400)) {
        for (qint32 i = 0x14; i < 0x18; ++i) {
            if (pHeader[i] != 0) return false;
        }
        if ((pHeader[0x18] != 4) || (pHeader[0x19] != 0)) return false;
    }

    return true;
}

bool XSFX::isLhaSfxStubCarrier(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guarded = pDevice;
    if (!guarded) return false;

    const qint64 nTotalSize = XBinary::getSize(guarded);
    if (nTotalSize < 16) return false;

    const QByteArray baPrefix = XBinary::read_array_process(guarded, 0, 16, pPdStruct);
    if (baPrefix.size() != 16) return false;
    const qint64 nBase = getLhaSfxStubBase(baPrefix);
    if ((nBase < 0) || (nBase >= nTotalSize)) return false;

    const qint64 nWindow = qMin<qint64>(nTotalSize, nBase + SFX_LHA_STUB_WINDOW);
    const QByteArray baImage = XBinary::read_array_process(guarded, 0, nWindow, pPdStruct);
    if (baImage.size() != nWindow) return false;

    const qint64 nPayload = getLhaSfxStubPayload(baImage, nBase);
    return (nPayload > 0) && (nPayload < nTotalSize);
}

XSFX::INTERNAL_INFO XSFX::_detectScan(PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache, XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache, qint64 nMinimumArchiveOffset)
{
    INTERNAL_INFO result = {};
    result.arcType = FT_UNKNOWN;

    const qint64 nTotalSize = getSize();
    if (nTotalSize < 0x40) {
        return result;
    }

    XSFX_ZPAQ_SCAN_CACHE localZpaqScanCache;
    if (!pZpaqScanCache) pZpaqScanCache = &localZpaqScanCache;
    pZpaqScanCache->bind(getDevice(), nTotalSize);
    XSFX_FREEARC_SCAN_CACHE localFreeArcScanCache;
    if (!pFreeArcScanCache) pFreeArcScanCache = &localFreeArcScanCache;
    pFreeArcScanCache->bind(getDevice(), nTotalSize);

    // Must be an executable stub. A bare archive is handled by its own class.
    XMSDOS msdos(getDevice(), isImage(), getModuleAddress());
    const bool bMSDOS = msdos.isValid(pPdStruct);
    XNE ne(getDevice(), isImage(), getModuleAddress());
    const bool bNE = ne.isValid(pPdStruct);
    XELF elf(getDevice(), isImage(), getModuleAddress());
    const bool bELF = elf.isValid(pPdStruct);
    XAtariST atariST(getDevice(), isImage(), getModuleAddress());
    const bool bAtariST = atariST.isValid(pPdStruct);
    const bool bCOM = !bMSDOS && (XBinary::getDeviceFileSuffix(getDevice()).compare(QStringLiteral("COM"), Qt::CaseInsensitive) == 0) &&
                      XCOM::isValid(getDevice(), isImage(), getModuleAddress(), pPdStruct);
    // A small family of Atari 68000 self-extractors uses the literal EXEC
    // loader header instead of the GEMDOS 0x601a executable header.  Accept
    // that carrier only for LHA probing; the embedded archive still has to
    // pass XLHA's complete structural validation below.
    const QByteArray baCarrierPrefix = read_array_process(0, 4, pPdStruct);
    const bool bExecLha = ((m_requiredArcType == FT_UNKNOWN) ||
                           (m_requiredArcType == FT_LHA)) &&
                          (baCarrierPrefix == QByteArrayLiteral("EXEC"));

    // The LHarc / LArc stub locator runs BEFORE the carrier gate and before the
    // overlay scan: its two families are precisely the ones neither of those
    // can reach (see getLhaSfxStubPayload above).  The offset it returns is
    // still handed to _matchArchiveAt, so XLHA's complete structural validation
    // decides, exactly as it does for a scanned candidate.
    if ((m_requiredArcType == FT_UNKNOWN) || (m_requiredArcType == FT_LHA)) {
        const QByteArray baStubPrefix = read_array_process(0, qMin<qint64>(nTotalSize, 16), pPdStruct);
        if (!isPdStructNotCanceled(pPdStruct)) return result;
        const qint64 nStubBase = getLhaSfxStubBase(baStubPrefix);
        if ((nStubBase >= 0) && (nStubBase < nTotalSize)) {
            const qint64 nStubWindow = qMin<qint64>(nTotalSize, nStubBase + SFX_LHA_STUB_WINDOW);
            const QByteArray baStubImage = read_array_process(0, nStubWindow, pPdStruct);
            if (!isPdStructNotCanceled(pPdStruct)) return result;
            const qint64 nStubPayload = getLhaSfxStubPayload(baStubImage, nStubBase);
            if ((nStubPayload > 0) && (nStubPayload < nTotalSize) && ((nMinimumArchiveOffset < 0) || (nStubPayload >= nMinimumArchiveOffset))) {
                FT stubType = FT_UNKNOWN;
                qint64 nStubArchiveSize = 0;
                bool bStubProvisional = false;
                bool bStubResourceIndeterminate = false;
                bool bStubUseOuterDevice = false;
                if (_matchArchiveAt(nStubPayload, nTotalSize - nStubPayload, &stubType, &nStubArchiveSize, pPdStruct, pZpaqScanCache, pFreeArcScanCache,
                                    &bStubProvisional, &bStubResourceIndeterminate, &bStubUseOuterDevice) &&
                    (stubType == FT_LHA)) {
                    result.bIsValid = true;
                    result.bProvisional = bStubProvisional;
                    result.arcType = stubType;
                    result.bUseOuterDevice = bStubUseOuterDevice;
                    result.nArchiveOffset = nStubPayload;
                    result.nArchiveSize = nStubArchiveSize;
                    return result;
                }
            }
        }
    }

    // The RTPatch locator runs BEFORE the carrier gate and before the overlay
    // scan, for the same reason the LHarc one does: the 3.20 carriers are
    // precisely the ones neither can reach (see isRTPatchPackageHeaderAt).
    // One buffered pass over the bounded image keeps a large carrier off the
    // repeated whole-window rescans a per-candidate signature search would cost.
    if (m_requiredArcType == FT_RTPATCH) {
        const QByteArray baPatchImage = read_array_process(0, qMin(nTotalSize, SFX_OVERLAY_SCAN_LIMIT), pPdStruct);
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return result;
        const qint32 nPatchLimit = baPatchImage.size() - SFX_RTPATCH_HEADER_SIZE;
        qint32 nPatchSearch = (nMinimumArchiveOffset > 0) ? (qint32)qMin<qint64>(nMinimumArchiveOffset, baPatchImage.size()) : 0;
        for (qint32 nAttempt = 0; (nAttempt < SFX_SIGNATURE_CANDIDATE_LIMIT) &&
                                  (nPatchSearch >= 0) && (nPatchSearch <= nPatchLimit) &&
                                  XBinary::isPdStructNotCanceled(pPdStruct);
             ++nAttempt) {
            const qint32 nPatchOffset = baPatchImage.indexOf(QByteArrayLiteral("K*"), nPatchSearch);
            if ((nPatchOffset < nPatchSearch) || (nPatchOffset > nPatchLimit)) break;
            nPatchSearch = nPatchOffset + 1;

            if (!isRTPatchPackageHeaderAt(baPatchImage, nPatchOffset)) continue;

            FT patchType = FT_UNKNOWN;
            qint64 nPatchArchiveSize = 0;
            bool bPatchProvisional = false;
            bool bPatchResourceIndeterminate = false;
            bool bPatchUseOuterDevice = false;
            if (_matchArchiveAt(nPatchOffset, nTotalSize - nPatchOffset, &patchType, &nPatchArchiveSize, pPdStruct, pZpaqScanCache, pFreeArcScanCache,
                                &bPatchProvisional, &bPatchResourceIndeterminate, &bPatchUseOuterDevice) &&
                (patchType == FT_RTPATCH)) {
                result.bIsValid = true;
                result.bProvisional = bPatchProvisional;
                result.arcType = patchType;
                result.bUseOuterDevice = bPatchUseOuterDevice;
                result.nArchiveOffset = nPatchOffset;
                result.nArchiveSize = nPatchArchiveSize;
                return result;
            }
        }
    }

    if (!bMSDOS && !bELF && !bAtariST && !bCOM && !bExecLha) {
        return result;
    }

    // PyInstaller CArchive is footer-indexed and spans the executable image,
    // so it cannot be represented by the ordinary overlay signature scan.
    if ((m_requiredArcType == FT_UNKNOWN ||
         m_requiredArcType == FT_PYINSTALLER_SFX) &&
        XPyInstallerCArchive::isValid(getDevice(), pPdStruct)) {
        result.bIsValid = true;
        result.bUseOuterDevice = true;
        result.arcType = FT_PYINSTALLER_SFX;
        result.nArchiveOffset = 0;
        result.nArchiveSize = nTotalSize;
        return result;
    }

    // 1) Preferred: the archive sits in the executable overlay.  XSFX itself
    // has XBinary's flat memory map, whose raw extent is the whole input, so
    // asking `this` for the overlay always returned EOF.  Use the parsed stub
    // map instead.
    qint64 nOverlayOffset = -1;
    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (pe.isValid(pPdStruct)) {
        nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    } else if (bELF) {
        nOverlayOffset = getExactELFExtent(&elf, nTotalSize);
    } else if (bAtariST) {
        nOverlayOffset = atariST.getOverlayOffset(pPdStruct);
    } else if (bNE) {
        nOverlayOffset = ne.getOverlayOffset(pPdStruct);
    } else if (bMSDOS) {
        nOverlayOffset = msdos.getOverlayOffset(pPdStruct);
    }
    // Current zpaqfranz SFX files place an encoded extraction command between
    // two fixed tags at the PE overlay boundary. Derive the authoritative
    // payload boundary from that framing before signature scanning: encrypted
    // payloads are intentionally opaque, while untagged plaintext archives
    // begin with raw zPQ and otherwise risk being carved from a later block.
    qint64 nFramedZpaqOffset = -1;
    qint64 nFramedZpaqSize = 0;
    if (((m_requiredArcType == FT_UNKNOWN) || (m_requiredArcType == FT_ZPAQ)) &&
        getZpaqFranzSfxPayload(this, nOverlayOffset, &nFramedZpaqOffset, &nFramedZpaqSize, pPdStruct) &&
        ((nMinimumArchiveOffset < 0) || (nFramedZpaqOffset >= nMinimumArchiveOffset))) {
        FT type = FT_UNKNOWN;
        qint64 nArchiveSize = 0;
        bool bProvisional = false;
        bool bResourceIndeterminate = false;
        bool bUseOuterDevice = false;
        if (_matchArchiveAt(nFramedZpaqOffset, nFramedZpaqSize, &type, &nArchiveSize, pPdStruct, pZpaqScanCache, pFreeArcScanCache, &bProvisional,
                            &bResourceIndeterminate, &bUseOuterDevice)) {
            if (type != FT_ZPAQ) return result;
            result.bIsValid = true;
            result.bProvisional = bProvisional;
            result.arcType = type;
            result.bUseOuterDevice = bUseOuterDevice;
            result.nArchiveOffset = nFramedZpaqOffset;
            result.nArchiveSize = nArchiveSize;
            return result;
        }
        if (bResourceIndeterminate) {
            result.bResourceIndeterminate = true;
            return result;
        }
        if (!isPdStructNotCanceled(pPdStruct)) return result;

        const QByteArray baPrefix = read_array_process(nFramedZpaqOffset, qMin<qint64>(nFramedZpaqSize, 16), pPdStruct);
        if (!isPdStructNotCanceled(pPdStruct) || resemblesPlainZpaqPrefix(baPrefix)) {
            // Do not reinterpret a damaged plaintext stream as encryption.
            return result;
        }

        result.bIsValid = true;
        result.bProvisional = true;
        result.bAllowOpaqueZpaq = true;
        result.arcType = FT_ZPAQ;
        result.bUseOuterDevice = false;
        result.nArchiveOffset = nFramedZpaqOffset;
        result.nArchiveSize = nFramedZpaqSize;
        return result;
    }

    QSet<qint64> setTestedOverlayOffsets;
    if ((nOverlayOffset > 0) && (nOverlayOffset < nTotalSize)) {
        const qint64 anOverlayCandidates[] = {
            nOverlayOffset,
            (nOverlayOffset + 1) & ~1LL,
            (nOverlayOffset + 3) & ~3LL,
            (nOverlayOffset + 15) & ~15LL,
            (nOverlayOffset + 511) & ~511LL,
        };
        for (qint64 nCandidateOffset : anOverlayCandidates) {
            if ((nCandidateOffset <= 0) || (nCandidateOffset >= nTotalSize) ||
                ((nMinimumArchiveOffset >= 0) && (nCandidateOffset < nMinimumArchiveOffset)) ||
                setTestedOverlayOffsets.contains(nCandidateOffset)) {
                continue;
            }
            setTestedOverlayOffsets.insert(nCandidateOffset);
            FT type = FT_UNKNOWN;
            qint64 nArchiveSize = 0;
            bool bProvisional = false;
            bool bResourceIndeterminate = false;
            bool bUseOuterDevice = false;
            if (_matchArchiveAt(nCandidateOffset, nTotalSize - nCandidateOffset, &type, &nArchiveSize, pPdStruct, pZpaqScanCache, pFreeArcScanCache, &bProvisional,
                                &bResourceIndeterminate, &bUseOuterDevice)) {
                result.bIsValid = true;
                result.bProvisional = bProvisional;
                result.arcType = type;
                result.bUseOuterDevice = bUseOuterDevice;
                result.nArchiveOffset = nCandidateOffset;
                result.nArchiveSize = nArchiveSize;
                return result;
            }
            if (bResourceIndeterminate) {
                result.bResourceIndeterminate = true;
                return result;
            }
        }
    }

    // The WinACE 32-bit stub publishes its container as the "ARCDATA"/"DATA"
    // PE resource. The directory entry is authoritative for both ends of the
    // archive, so no scan window is widened and no other family's candidate
    // can be displaced: the offset is only ever handed to _matchArchiveAt,
    // which lets XACE validate the complete block chain exactly as it does
    // for an overlay candidate.
    if (((m_requiredArcType == FT_UNKNOWN) || (m_requiredArcType == FT_ACE)) && pe.isValid(pPdStruct) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint64 nAceResourceOffset = -1;
        qint64 nAceResourceSize = 0;
        if (getWinAceSfxResource(&pe, &nAceResourceOffset, &nAceResourceSize, pPdStruct) && (nAceResourceOffset > 0) && (nAceResourceOffset < nTotalSize) &&
            (nAceResourceSize <= (nTotalSize - nAceResourceOffset)) && ((nMinimumArchiveOffset < 0) || (nAceResourceOffset >= nMinimumArchiveOffset)) &&
            !setTestedOverlayOffsets.contains(nAceResourceOffset)) {
            setTestedOverlayOffsets.insert(nAceResourceOffset);
            FT type = FT_UNKNOWN;
            qint64 nArchiveSize = 0;
            bool bProvisional = false;
            bool bResourceIndeterminate = false;
            bool bUseOuterDevice = false;
            if (_matchArchiveAt(nAceResourceOffset, nAceResourceSize, &type, &nArchiveSize, pPdStruct, pZpaqScanCache, pFreeArcScanCache, &bProvisional,
                                &bResourceIndeterminate, &bUseOuterDevice) &&
                (type == FT_ACE)) {
                result.bIsValid = true;
                result.bProvisional = bProvisional;
                result.arcType = type;
                result.bUseOuterDevice = bUseOuterDevice;
                result.nArchiveOffset = nAceResourceOffset;
                result.nArchiveSize = nArchiveSize;
                return result;
            }
        }
    }

    // A ZIP footer is both cheaper and more authoritative than a forward
    // signature sweep. Try its derived first-local-record offset even when it
    // lies beyond SFX_OVERLAY_SCAN_LIMIT (large PKSFX images commonly do).
    if (((m_requiredArcType == FT_UNKNOWN) || (m_requiredArcType == FT_ZIP)) &&
        XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nZipOffset = getZipSfxCandidate(this, pPdStruct);
        if ((nZipOffset > 0) && (nZipOffset < nTotalSize) &&
            ((nMinimumArchiveOffset < 0) || (nZipOffset >= nMinimumArchiveOffset)) &&
            !setTestedOverlayOffsets.contains(nZipOffset)) {
            setTestedOverlayOffsets.insert(nZipOffset);
            FT type = FT_UNKNOWN;
            qint64 nArchiveSize = 0;
            bool bProvisional = false;
            bool bResourceIndeterminate = false;
            bool bUseOuterDevice = false;
            if (_matchArchiveAt(nZipOffset, nTotalSize - nZipOffset, &type, &nArchiveSize, pPdStruct,
                                pZpaqScanCache, pFreeArcScanCache, &bProvisional,
                                &bResourceIndeterminate, &bUseOuterDevice) &&
                (type == FT_ZIP)) {
                result.bIsValid = true;
                result.bProvisional = bProvisional;
                result.arcType = type;
                result.bUseOuterDevice = bUseOuterDevice;
                result.nArchiveOffset = nZipOffset;
                result.nArchiveSize = nArchiveSize;
                return result;
            }
            if (bResourceIndeterminate) {
                result.bResourceIndeterminate = true;
                return result;
            }
        }
    }

    // Knowledge Dynamics Corp's INSTALL / wINSTALL "WinSFX" stubs append a
    // complete ".RED" archive after the loader.  XFU already READS that
    // container - it is one of the bounded legacy decoder's modules, and a
    // carved payload lists and extracts today - so only its position inside
    // the carrier was missing.  The locator is authoritative: it accepts the
    // one offset whose CRC-authenticated member chain ends exactly at end of
    // file, which is also what separates the payload from the loader-owned
    // engine blob the 16-bit builders store ahead of it.  It runs before the
    // generic sweep because a 16-bit carrier that reports no overlay returns
    // just below, and no scan pattern can reach these bytes anyway.
    //
    // Cost, stated plainly because this probe runs on every SFX-probed
    // executable: one buffered signature sweep of at most
    // SFX_OVERLAY_SCAN_LIMIT bytes, plus - only for a carrier that really
    // contains the four-byte tag - at most SFX_RED_SCAN_HEADER_BUDGET 41-byte
    // header reads.  That sweep is not free: it is one linear pass
    // proportional to the window, measured at about 40 ms for a full 16 MiB.
    // It is bounded, and it can no longer be multiplied by the candidate cap.
    // It starts at nMinimumArchiveOffset for the same reason the fallback
    // sweep below does: offsets below that bound are rejected anyway, so a
    // carrier re-probed for a further payload must not rescan them.
    if ((m_requiredArcType == FT_UNKNOWN) || (m_requiredArcType == FT_DEARK_LEGACY_ARCHIVE)) {
        const qint64 nRedScanStart = (nMinimumArchiveOffset > 0) ? nMinimumArchiveOffset : 0;
        const qint64 nRedOffset = getRedSfxCandidate(this, nRedScanStart, SFX_OVERLAY_SCAN_LIMIT, pPdStruct);
        if ((nRedOffset > 0) && (nRedOffset < nTotalSize) && ((nMinimumArchiveOffset < 0) || (nRedOffset >= nMinimumArchiveOffset)) &&
            !setTestedOverlayOffsets.contains(nRedOffset)) {
            setTestedOverlayOffsets.insert(nRedOffset);
            FT type = FT_UNKNOWN;
            qint64 nArchiveSize = 0;
            bool bProvisional = false;
            bool bResourceIndeterminate = false;
            bool bUseOuterDevice = false;
            if (_matchArchiveAt(nRedOffset, nTotalSize - nRedOffset, &type, &nArchiveSize, pPdStruct, pZpaqScanCache, pFreeArcScanCache, &bProvisional,
                                &bResourceIndeterminate, &bUseOuterDevice) &&
                (type == FT_DEARK_LEGACY_ARCHIVE)) {
                result.bIsValid = true;
                result.bProvisional = bProvisional;
                result.arcType = type;
                result.bUseOuterDevice = bUseOuterDevice;
                result.nArchiveOffset = nRedOffset;
                result.nArchiveSize = nArchiveSize;
                return result;
            }
        }
    }

    // 2) Fallback: scan only the executable overlay. Searching mapped PE/NE
    // sections classified ordinary programs containing CAB resources as SFXs.
    // Dedicated GZIP/KWAJ/SZDD wrappers are the exception: historical setup
    // builders deliberately place their compressed payloads in PE/NE resource
    // areas, so those required-family probes scan the bounded complete image.
    // Atari ST relocation/symbol data can precede the payload, so scanning from
    // the mapped image boundary also covers that executable metadata. A COM
    // image has no file-backed size metadata, so its complete (at most
    // 65280-byte) body is the only meaningful search range. Candidates still
    // have to initialize as complete archives before they are accepted.
    const bool bScanEmbeddedImage = (m_requiredArcType == FT_GZIP) || (m_requiredArcType == FT_KWAJ) || (m_requiredArcType == FT_SZDD);
    if (!bScanEmbeddedImage && !bCOM && !bAtariST && !bExecLha &&
        ((nOverlayOffset <= 0) || (nOverlayOffset >= nTotalSize))) return result;

    const char *apszSignatures[] = {"377ABCAF271C",
                                    "504B0304",
                                    "504B0506",
                                    "52617221",
                                    "52457E5E",
                                    "4D534346",
                                    "41724301........4172430102",         // FreeArc header + first data block
                                    "376B5374A03183D38CB228B0D37A5051",  // full ZPAQ locator
                                    "1A01",                              // ARC method 1
                                    "1A02",                              // ARC method 2
                                    "1A03",                              // ARC method 3
                                    "1A04",                              // ARC method 4
                                    "1A05",                              // ARC method 5
                                    "1A06",                              // ARC method 6
                                    "1A07",                              // ARC method 7
                                    "1A08",                              // ARC method 8
                                    "1A09",                              // ARC method 9
                                    "1A0A",                              // ARC method 10
                                    "1A0B",                              // ARC method 11
                                    "60EA",                              // ARJ basic-header marker
                                    "2D6C68..2D",                        // -lh?- (candidate begins two bytes earlier)
                                    "2D6C7A..2D",                        // -lz?-
                                    "2D706D..2D",                        // -pm?-
                                    "1F8B08",                            // GZIP
                                    "4B57414A88F027D1",                  // KWAJ
                                    "535A444488F02733",                  // SZDD mode A
                                    "535A444488F0273A",                  // SZDD mode B
                                    "5A444488F0273341",                  // legacy SZDD mode A
                                    "5A444488F0273A41",                  // legacy SZDD mode B
                                    "01CA436F70797269676874",            // NeoSoft GX Library
                                    "67570402",                          // Crusher ARQ
                                    "484C53515A",                        // HLSQZ
                                    "425A68",                            // BZIP2; full gate above
                                    "FF425347",                          // PTS BSA (.BSN) container header
                                    "544743460024",                      // "Setup Specialist" TGCF container header
                                    "602213636C000000"};                 // Asymetrix disk-set volume header + 0x6C record stride
    const qint32 anCandidateAdjustments[] = {0, 0, 0, 0, 0, 0, 0, 0,  // 7z through ZPAQ
                                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // ARC methods 1 through 11
                                             0,                                // ARJ
                                             -2, -2, -2,                       // LHA header markers
                                             0,                                // GZIP
                                             0,                                // KWAJ
                                             0, 0, 0, 0,                       // SZDD signatures
                                             0,                                // GX Library
                                             0, 0, 0, 0, 0,                    // ARQ, SQZ, BZIP2, BSN, TGCF
                                             0};                               // Asymetrix
    const FT anSignatureTypes[] = {FT_7Z, FT_ZIP, FT_ZIP, FT_RAR, FT_RAR, FT_CAB, FT_FREEARC, FT_ZPAQ,
                                        FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC, FT_ARC,
                                        FT_ARJ, FT_LHA, FT_LHA, FT_LHA, FT_GZIP, FT_KWAJ, FT_SZDD, FT_SZDD, FT_SZDD, FT_SZDD,
                                        FT_DEARK_LEGACY_ARCHIVE, FT_ARQ, FT_SQZ, FT_BZIP2, FT_BSN, FT_TGCF, FT_ASYMETRIX};
    static_assert((sizeof(apszSignatures) / sizeof(apszSignatures[0])) == (sizeof(anCandidateAdjustments) / sizeof(anCandidateAdjustments[0])),
                   "SFX signature and adjustment tables must stay aligned");
    static_assert((sizeof(apszSignatures) / sizeof(apszSignatures[0])) == (sizeof(anSignatureTypes) / sizeof(anSignatureTypes[0])),
                  "SFX signature and archive-type tables must stay aligned");
    const qint64 nBaseScanStart = bScanEmbeddedImage ? 0 :
        ((bCOM || bAtariST || bExecLha) ? 0 : nOverlayOffset);
    const qint64 nScanEnd = nBaseScanStart + qMin(nTotalSize - nBaseScanStart, SFX_OVERLAY_SCAN_LIMIT);
    const qint64 nScanStart = qMax(nBaseScanStart, (nMinimumArchiveOffset < 0) ? nBaseScanStart : nMinimumArchiveOffset);
    if (nScanStart >= nScanEnd) return result;
    const qint64 nScanSize = nScanEnd - nScanStart;
    Q_UNUSED(nScanSize)
    const int nSignatureCount = sizeof(apszSignatures) / sizeof(apszSignatures[0]);
    qint64 anNextCandidate[nSignatureCount];
    qint32 anCandidateCounts[nSignatureCount] = {};

    // One buffered pass over the scan window finds every signature without the
    // old per-pattern rescans.  Candidates are consumed as soon as a chunk is
    // complete, rather than waiting for the complete (up to 16 MiB) window.
    // The two-byte look-behind is needed because an LHA marker begins two bytes
    // after its archive header; retaining that boundary preserves exact global
    // offset order across chunks.  KWAJ/SZDD retain their complete candidate
    // map because framing one resource stream requires the next family header.
    QList<qint64> alistCandidates[nSignatureCount];
    qint32 anCandidateIndex[nSignatureCount] = {};
    std::fill(anNextCandidate, anNextCandidate + nSignatureCount, qint64(-1));

    struct SIG_PATTERN {
        QByteArray baBytes;
        QByteArray baMask;
    };
    SIG_PATTERN apatterns[nSignatureCount];
    qint32 nMaxPatternSize = 0;
    qint32 nMinimumCandidateAdjustment = 0;
    QList<qint32> aBuckets[256];
    for (int i = 0; i < nSignatureCount; i++) {
        if ((m_requiredArcType != FT_UNKNOWN) && (anSignatureTypes[i] != m_requiredArcType)) continue;
        const QByteArray baSignature(apszSignatures[i]);
        for (qint32 j = 0; (j + 1) < baSignature.size(); j += 2) {
            const char c1 = baSignature.at(j);
            const char c2 = baSignature.at(j + 1);
            if ((c1 == '.') && (c2 == '.')) {
                apatterns[i].baBytes.append('\0');
                apatterns[i].baMask.append('\0');
            } else {
                apatterns[i].baBytes.append(static_cast<char>(QByteArray(baSignature.constData() + j, 2).toUInt(nullptr, 16)));
                apatterns[i].baMask.append(static_cast<char>(0xff));
            }
        }
        nMaxPatternSize = qMax(nMaxPatternSize, apatterns[i].baBytes.size());
        nMinimumCandidateAdjustment = qMin(nMinimumCandidateAdjustment, anCandidateAdjustments[i]);
        // Every table entry starts with a fixed byte, so a first-byte bucket
        // dispatch keeps the inner loop cheap.
        aBuckets[static_cast<quint8>(apatterns[i].baBytes.at(0))].append(i);
    }

    // Examine signatures by file offset, not by format family. Otherwise a
    // later embedded archive or a run of one-family decoys can hide the real,
    // earlier payload of another type. nEvaluationEnd is exclusive and lets
    // the buffered collector defer only the cross-chunk look-behind.
    SCAN_CANDIDATE_EVALUATOR candidateEvaluator(
        this, &result, &setTestedOverlayOffsets, nTotalSize, nScanStart,
        nScanEnd, nOverlayOffset, bAtariST, apszSignatures,
        anCandidateAdjustments, nSignatureCount, anNextCandidate,
        anCandidateCounts, anCandidateIndex, alistCandidates, pPdStruct,
        pZpaqScanCache, pFreeArcScanCache);

    const qint32 nCollectLimit = SFX_SIGNATURE_CANDIDATE_LIMIT + 1;
    // Keep first-hit latency low for COM/Atari carriers whose embedded header
    // is near the start, while retaining large buffered reads versus the old
    // byte-at-a-time device scan.
    const qint64 nChunkSize = 64LL * 1024;
    const bool bNeedsCompleteCandidateMap = (m_requiredArcType == FT_KWAJ) || (m_requiredArcType == FT_SZDD);
    QByteArray baChunk;
    for (qint64 nChunkStart = nScanStart; (nChunkStart < nScanEnd) && isPdStructNotCanceled(pPdStruct); nChunkStart += nChunkSize) {
        const qint64 nReadSize = qMin(nChunkSize + nMaxPatternSize - 1, nScanEnd - nChunkStart);
        baChunk = read_array_process(nChunkStart, nReadSize, pPdStruct);
        if (baChunk.size() != nReadSize) break;
        const char *pChunk = baChunk.constData();
        const qint64 nPositions = qMin(nChunkSize, nScanEnd - nChunkStart);
        for (qint64 nOffsetInChunk = 0; nOffsetInChunk < nPositions; ++nOffsetInChunk) {
            const QList<qint32> &listBucket = aBuckets[static_cast<quint8>(pChunk[nOffsetInChunk])];
            if (listBucket.isEmpty()) continue;
            for (qint32 nBucketIndex = 0; nBucketIndex < listBucket.count(); ++nBucketIndex) {
                const qint32 i = listBucket.at(nBucketIndex);
                if (alistCandidates[i].count() >= nCollectLimit) continue;
                const SIG_PATTERN &pattern = apatterns[i];
                const qint32 nPatternSize = pattern.baBytes.size();
                if ((nOffsetInChunk + nPatternSize) > baChunk.size()) continue;
                bool bMatch = true;
                for (qint32 k = 1; k < nPatternSize; ++k) {
                    if ((pChunk[nOffsetInChunk + k] & pattern.baMask.at(k)) != pattern.baBytes.at(k)) {
                        bMatch = false;
                        break;
                    }
                }
                if (bMatch) alistCandidates[i].append(nChunkStart + nOffsetInChunk);
            }
        }

        const qint64 nNextChunkStart = nChunkStart + nPositions;
        const bool bFinalChunk = (nNextChunkStart >= nScanEnd);
        const qint64 nEvaluationEnd = (bFinalChunk || !bNeedsCompleteCandidateMap)
                                              ? (bFinalChunk ? nScanEnd : qMax(nScanStart, nNextChunkStart + nMinimumCandidateAdjustment))
                                              : nScanStart;
        if (candidateEvaluator.evaluate(nEvaluationEnd)) return result;
    }

    // A short read preserves the previous best-effort behaviour: consume the
    // candidates collected before the read failed. Normal completion has no
    // remaining entries because the final chunk used nScanEnd already.
    if (isPdStructNotCanceled(pPdStruct) &&
        candidateEvaluator.evaluate(nScanEnd)) {
        return result;
    }

    if (!isPdStructNotCanceled(pPdStruct)) {
        INTERNAL_INFO canceledResult = {};
        canceledResult.arcType = FT_UNKNOWN;
        return canceledResult;
    }
    return result;
}

XArchive *XSFX::_createArchive(FT arcType, QIODevice *pDevice, bool bAllowOpaqueZpaq)
{
    switch (arcType) {
        case FT_7Z: return new XSevenZip(pDevice);
        case FT_ZIP: {
            XConcatZipArchive *pConcat = new XConcatZipArchive(pDevice);
            if (pConcat->XGameStoreArchiveBase::isValid(nullptr)) return pConcat;
            delete pConcat;
            // Only the complete outer file can satisfy this reader: it derives
            // the archive's true start from the footer, so a rebased view
            // derives zero and falls straight through to XZip.
            XWinImageZipArchive *pWinImage = new XWinImageZipArchive(pDevice);
            if (pWinImage->XGameStoreArchiveBase::isValid(nullptr)) return pWinImage;
            delete pWinImage;
            return new XZip(pDevice);
        }
        case FT_RAR: return new XRar(pDevice);
        case FT_CAB: return new XCab(pDevice);
        case FT_ARC: return new XSEAARC(pDevice);
        case FT_ARJ: return new XARJ(pDevice);
        // Deark's LHA reader accepts the historical LArc/LZ5 headers used by
        // EXEC-format Atari self-extractors as well as ordinary LHA members.
        // The native XLHA reader remains the bare-archive implementation, but
        // the bounded SFX view benefits from Deark's wider header dialects.
        case FT_LHA: {
            // PMA uses the native PMarc decoders, including the reference implementation's
            // exact pms envelope. Other LHA SFX dialects retain Deark above.
            if (!pDevice) return nullptr;
            const qint64 savedPos = pDevice->pos();
            XBinary binary(pDevice);
            const QByteArray prefix = binary.read_array(0, 7);
            if (pDevice && savedPos >= 0) pDevice->seek(savedPos);
            const QByteArray method = prefix.mid(2, 5);
            if (prefix.size() == 7 && (method == "-pm0-" || method == "-pm1-" || method == "-pm2-" ||
                                      (method == "-pms-" && quint8(prefix.at(0)) == 0x18))) {
                // initUnpack/isValid still validate the complete header and
                // declared suffix before this backend can expose records.
                return new XLHA(pDevice);
            }
            return new XDearkArchive(pDevice);
        }
        case FT_GZIP: return new XGzip(pDevice);
        case FT_BZIP2: return new XBZIP2(pDevice);
        case FT_KWAJ: return new XKWAJ(pDevice);
        case FT_SZDD: return new XSZDD(pDevice);
        case FT_PYINSTALLER_SFX: return new XPyInstallerCArchive(pDevice);
        case FT_DEARK_LEGACY_ARCHIVE: return new XDearkArchive(pDevice);
        case FT_ARQ: return new XARQ(pDevice);
        case FT_SQZ: return new XSQZ(pDevice);
        case FT_RTPATCH: return new XRTPatch(pDevice);
        case FT_BSN: return new XBSN(pDevice);
        case FT_TGCF: return new XTGCFArchive(pDevice);
        case FT_RTA: return new XRTA(pDevice);
        case FT_ACE: return new XACE(pDevice);
        case FT_ASYMETRIX: return new XAsymetrix(pDevice);
        case FT_FREEARC: return new XFREEARC(pDevice);
        case FT_ZPAQ: {
            XZPAQ *pZpaq = new XZPAQ(pDevice);
            pZpaq->setAllowOpaqueEncrypted(bAllowOpaqueZpaq);
            return pZpaq;
        }
        default: return nullptr;
    }
}

QMap<XBinary::UNPACK_PROP, QVariant> XSFX::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    QIODevice *pDevice = getDevice();

    if (pDevice) {
        INTERNAL_INFO info = _detect(nullptr);

        if (info.bIsValid && (info.nArchiveOffset >= 0) && (info.nArchiveSize > 0)) {
            SubDevice subDevice(pDevice, info.nArchiveOffset, info.nArchiveSize);
            QIODevice *pArchiveDevice = info.bUseOuterDevice ? pDevice : static_cast<QIODevice *>(&subDevice);
            const bool bDeviceReady = info.bUseOuterDevice ? (pDevice->isOpen() && pDevice->isReadable()) : subDevice.open(QIODevice::ReadOnly);

            if (bDeviceReady) {
                XArchive *pArchive = _createArchive(info.arcType, pArchiveDevice, info.bAllowOpaqueZpaq);

                if (pArchive) {
                    QMap<UNPACK_PROP, QVariant> mapInnerProperties = pArchive->getDefaultUnpackProperties();

                    if (mapInnerProperties.contains(UNPACK_PROP_PASSWORD)) {
                        result.insert(UNPACK_PROP_PASSWORD, mapInnerProperties.value(UNPACK_PROP_PASSWORD));
                    }

                    for (QMap<UNPACK_PROP, QVariant>::const_iterator it = mapInnerProperties.constBegin(); it != mapInnerProperties.constEnd(); ++it) {
                        if (XBinary::isUnpackCRCProperty(it.key())) {
                            result.insert(it.key(), it.value());
                        }
                    }

                    delete pArchive;
                }

                if (!info.bUseOuterDevice) subDevice.close();
            }
        }
    }

    return result;
}

bool XSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;
    qint64 nOutputLimit = -1;
    if (!getUnpackOutputLimit(mapProperties, &nOutputLimit)) return false;
    Q_UNUSED(nOutputLimit)
    // Register the caller's operation budget before structural probing. The
    // FreeArc probes use a stricter internal ceiling, and the shared reservation
    // tracker clamps those nested allocations to this active caller limit too.
    UNPACK_MEMORY_RESERVATION operationMemoryBudget;
    if (!operationMemoryBudget.acquire(mapProperties, 0)) return false;
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (pOldContext->pOwnerState != pState) return false;
        pState->pContext = nullptr;
        bool bFinishOK = true;
        clearPrivateUnpackCredential(pOldContext);
        if (pOldContext->pArchive) {
            bFinishOK = pOldContext->pArchive->finishUnpack(&pOldContext->innerState, nullptr);
            delete pOldContext->pArchive;
        }
        if (pOldContext->pSubDevice) {
            pOldContext->pSubDevice->close();
            delete pOldContext->pSubDevice;
        }
        delete pOldContext;
        *pState = UNPACK_STATE();
        if (!bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;
    scrubPublicUnpackProperties(&pState->mapUnpackProperties);

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nTotalSize = guardedSource->size();
    if (nTotalSize < 0) return false;

    XSFX detector(guardedSource, isImage(), getModuleAddress(), m_requiredArcType);
    const QString sInitialError = XBinary::getPdStructErrorString(pPdStruct);
    QString sLastCandidateError;
    qint64 nMinimumArchiveOffset = -1;
    FT retryType = FT_UNKNOWN;
    UNPACK_CONTEXT *pContext = nullptr;
    XSFX_ZPAQ_SCAN_CACHE zpaqScanCache;
    XSFX_FREEARC_SCAN_CACHE freeArcScanCache;
    const QDeadlineTimer helperDeadline = XExternalArchive::createHelperDeadline();

    for (qint32 nAttempt = 0; (nAttempt < SFX_SIGNATURE_CANDIDATE_LIMIT) && XBinary::isPdStructNotCanceled(pPdStruct); nAttempt++) {
        const INTERNAL_INFO info = detector._detect(pPdStruct, &zpaqScanCache, &freeArcScanCache, nMinimumArchiveOffset);
        if (!guardedSource || !info.bIsValid) break;

        if ((retryType != FT_UNKNOWN) && (info.arcType != retryType)) {
            if (info.nArchiveOffset >= (std::numeric_limits<qint64>::max)()) break;
            nMinimumArchiveOffset = info.nArchiveOffset + 1;
            continue;
        }

        pContext = new UNPACK_CONTEXT;
        pContext->pOuterSourceDevice = guardedSource;
        pContext->nOwnerDeviceGeneration = getDeviceGeneration();
        pContext->info = info;
        pContext->pSubDevice = nullptr;
        pContext->pArchive = nullptr;
        pContext->innerState = UNPACK_STATE();
        if (info.bProvisional && isExternalSfxType(info.arcType)) {
            initializePrivateUnpackProperties(pContext, mapProperties, true);
        }

        bool bInitialized = (guardedSource != nullptr);
        QIODevice *pArchiveDevice = guardedSource;
        if (!info.bUseOuterDevice) {
            pContext->pSubDevice = new SubDevice(guardedSource, info.nArchiveOffset, info.nArchiveSize);
            bInitialized = pContext->pSubDevice->open(QIODevice::ReadOnly) && guardedSource;
            pArchiveDevice = pContext->pSubDevice;
        }
        XExternalArchive *pExternalArchive = nullptr;
        if (bInitialized) {
            pContext->pArchive = _createArchive(info.arcType, pArchiveDevice, info.bAllowOpaqueZpaq);
            pExternalArchive = externalArchiveFor(pContext->pArchive, info.arcType);
            if (pExternalArchive) pExternalArchive->setHelperDeadline(helperDeadline);
            bInitialized = pContext->pArchive && pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) && guardedSource;
        }
        XExternalArchive::EXTERNAL_FAILURE externalFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
        if (pExternalArchive) {
            externalFailure = pExternalArchive->getLastExternalFailure();
            pExternalArchive->clearHelperDeadline();
        }
        if (bInitialized) {
            if (pExternalArchive && pExternalArchive->isDeferredArchiveMaterialized(&pContext->innerState)) {
                clearPrivateUnpackCredential(pContext);
            }
            break;
        }

        clearPrivateUnpackCredential(pContext);
        if (pContext->pArchive) {
            pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }
        delete pContext;
        pContext = nullptr;

        const QString sCandidateError = XBinary::getPdStructErrorString(pPdStruct);
        if (!info.bProvisional || !isExternalSfxType(info.arcType) || (externalFailure != XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED) ||
            !guardedSource || (info.nArchiveOffset >= (std::numeric_limits<qint64>::max)())) {
            break;
        }

        // Only an authoritative archive rejection may advance to the next
        // same-family signature. Operational failures (timeout, cancellation,
        // policy, launch/containment, or password) stop immediately, and all
        // attempts share the single deadline created above.
        sLastCandidateError = sCandidateError;
        XBinary::setPdStructErrorString(pPdStruct, sInitialError);
        retryType = info.arcType;
        nMinimumArchiveOffset = info.nArchiveOffset + 1;
    }

    if (!pContext) {
        if ((XBinary::getPdStructErrorString(pPdStruct) == sInitialError) && !sLastCandidateError.isEmpty()) {
            XBinary::setPdStructErrorString(pPdStruct, sLastCandidateError);
        }
        return false;
    }

    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->nTotalSize = nTotalSize;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    return true;
}

XBinary::ARCHIVERECORD XSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) {
        return result;
    }

    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return ARCHIVERECORD();
    if (pContext->info.arcType == FT_BZIP2 && result.mapProperties.contains(FPART_PROP_ORIGINALNAME)) {
        // The reference implementation removes only the last extension of the outer basename;
        // the inner SubDevice deliberately carries no filename properties.
        QString name = QFileInfo(XBinary::getDeviceFileName(pContext->pOuterSourceDevice)).fileName();
        if (pState->pContext != pContext) return ARCHIVERECORD();
        const qsizetype dot = name.lastIndexOf('.');
        if (dot > 0) name.truncate(dot);
        // normalize one filename component.
        if (name.endsWith('\r')) name.chop(1);
        while (name.endsWith(' ') || name.endsWith('.')) name.chop(1);
        name = name.left(255);
        if (name.isEmpty()) name = QStringLiteral("_");
        const QString forbidden = QStringLiteral("\"*/:<>?\\|");
        for (qint32 i = 0; i < name.size(); ++i) {
            if (name.at(i).unicode() < 32 || forbidden.contains(name.at(i))) name[i] = QLatin1Char('_');
        }
        const QString stem = name.section('.', 0, 0).toUpper();
        if (stem == "AUX" || stem == "CON" || stem == "NUL" || stem == "PRN" || stem == "CLOCK$" ||
            (stem.size() == 4 && (stem.startsWith("COM") || stem.startsWith("LPT")) && stem.at(3) >= '1' && stem.at(3) <= '9')) name.prepend('_');
        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, name);
    }
    return result;
}

bool XSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedSource || !guardedOutput->isOpen() ||
        !guardedOutput->isWritable() || guardedOutput->isSequential() || !guardedOutput ||
        (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) {
        return false;
    }

    // XFU-015: thread the operation output budget into the inner archive's
    // state; the inner unpackCurrent performs its own entry accounting and
    // produced-byte debits (for the deferred route this charges production
    // into the private stage, so publishStage must not debit the copy).
    pContext->innerState.spOutputBudget = pState->spOutputBudget;

    const bool bDeferredCandidate = pContext->info.bProvisional && ((pContext->info.arcType == FT_ZPAQ) || (pContext->info.arcType == FT_FREEARC));
    if (!bDeferredCandidate) {
        const bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput, pPdStruct);
        if (!guardedOutput || (pState->pContext != pContext)) return false;
        pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
        pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
        return bResult;
    }

    const QDeadlineTimer helperDeadline = XExternalArchive::createHelperDeadline();
    XExternalArchive *pCurrentExternalArchive = externalArchiveFor(pContext->pArchive, pContext->info.arcType);
    if (!pCurrentExternalArchive) return false;

    const qint32 nRequestedIndex = pState->nCurrentIndex;
    const qint64 nOriginalOuterOffset = pState->nCurrentOffset;
    const QString sInitialError = XBinary::getPdStructErrorString(pPdStruct);
    class SFX_DEFERRED_CANDIDATE_HELPER {
    public:
        typedef XArchive *(XSFX::*CREATE_ARCHIVE_METHOD)(XBinary::FT, QIODevice *, bool);

        SFX_DEFERRED_CANDIDATE_HELPER(XSFX *guardedThis, QIODevice *guardedOutput,
                                      QIODevice *guardedSource,
                                      XSFX::UNPACK_CONTEXT **ppContext, XBinary::UNPACK_STATE *pState, qint32 nRequestedIndex,
                                      qint64 nOriginalOuterOffset, const QDeadlineTimer &helperDeadline, XBinary::PDSTRUCT *pPdStruct,
                                      CREATE_ARCHIVE_METHOD pCreateArchiveMethod)
            : m_guardedThis(guardedThis),
              m_guardedOutput(guardedOutput),
              m_guardedSource(guardedSource),
              m_ppContext(ppContext),
              m_pState(pState),
              m_nRequestedIndex(nRequestedIndex),
              m_nOriginalOuterOffset(nOriginalOuterOffset),
              m_helperDeadline(helperDeadline),
              m_pPdStruct(pPdStruct),
              m_pCreateArchiveMethod(pCreateArchiveMethod)
        {
        }

        bool recordsEqual(const XBinary::ARCHIVERECORD &a, const XBinary::ARCHIVERECORD &b) const
        {
            return (a.nStreamOffset == b.nStreamOffset) && (a.nStreamSize == b.nStreamSize) && (a.mapProperties == b.mapProperties);
        }

        bool contextIsCurrent() const
        {
            XSFX::UNPACK_CONTEXT *pContext = currentContext();
            return m_guardedThis && m_guardedOutput && m_guardedSource && pContext &&
                   (m_pState->pContext == pContext) && (pContext->pOwnerState == m_pState) && (pContext->pOuterSourceDevice == m_guardedSource) &&
                   (pContext->nOwnerDeviceGeneration == m_guardedThis->getDeviceGeneration()) && (m_pState->nCurrentIndex == m_nRequestedIndex) &&

                   (m_pState->nCurrentOffset == m_nOriginalOuterOffset);
        }

        void destroyDetachedContext(XSFX::UNPACK_CONTEXT *pDetached) const
        {
            if (!pDetached) return;
            clearPrivateUnpackCredential(pDetached);
            if (pDetached->pArchive) {
                pDetached->pArchive->finishUnpack(&pDetached->innerState, nullptr);
                delete pDetached->pArchive;
            }
            if (pDetached->pSubDevice) {
                pDetached->pSubDevice->close();
                delete pDetached->pSubDevice;
            }
            delete pDetached;
        }

        bool collectManifest(const XSFX::INTERNAL_INFO &info, QList<XBinary::ARCHIVERECORD> *pRecords,
                             QMap<XBinary::FPART_PROP, QVariant> *pProperties, XExternalArchive::EXTERNAL_FAILURE *pFailure) const
        {
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
            if (!pRecords || !pProperties || !m_guardedSource) return false;
            pRecords->clear();
            pProperties->clear();

            SubDevice *pSubDevice = new (std::nothrow) SubDevice(m_guardedSource, info.nArchiveOffset, info.nArchiveSize);
            XArchive *pArchive = nullptr;
            XExternalArchive *pExternalArchive = nullptr;
            XBinary::UNPACK_STATE state = {};
            bool bResult = pSubDevice && pSubDevice->open(QIODevice::ReadOnly) && m_guardedSource;
            if (bResult) {
                pArchive = createArchive(info.arcType, pSubDevice, info.bAllowOpaqueZpaq);
                pExternalArchive = externalArchiveFor(pArchive, info.arcType);
                if (pExternalArchive) pExternalArchive->setHelperDeadline(m_helperDeadline);
                QMap<XBinary::UNPACK_PROP, QVariant> attemptProperties = privateUnpackProperties(currentContext());
                bResult = pArchive && pExternalArchive && pArchive->initUnpack(&state, attemptProperties, m_pPdStruct) && m_guardedSource &&
                          (state.nNumberOfRecords >= 0);
                scrubPublicUnpackProperties(&attemptProperties);
                if (pExternalArchive && pFailure) *pFailure = pExternalArchive->getLastExternalFailure();
            }
            if (bResult) {
                *pProperties = state.mapArchiveProperties;
                pRecords->reserve(state.nNumberOfRecords);
                for (qint32 i = 0; bResult && (i < state.nNumberOfRecords); ++i) {
                    const XBinary::ARCHIVERECORD record = pArchive->infoCurrent(&state, m_pPdStruct);
                    bResult = m_guardedSource && record.mapProperties.contains(XBinary::FPART_PROP_ORIGINALNAME) && (state.nCurrentIndex == i);
                    if (bResult) pRecords->append(record);
                    if (bResult && (i + 1 < state.nNumberOfRecords)) {
                        bResult = pArchive->moveToNext(&state, m_pPdStruct) && m_guardedSource;
                    }
                }
                if (!bResult && pFailure) {
                    *pFailure = XBinary::isPdStructNotCanceled(m_pPdStruct) ? XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED
                                                                          : XExternalArchive::EXTERNAL_FAILURE_CANCELED;
                }
            }
            if (pExternalArchive) pExternalArchive->clearHelperDeadline();
            if (pArchive) {
                const bool bFinished = pArchive->finishUnpack(&state, nullptr);
                if (bResult && !bFinished && pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
                bResult = bResult && bFinished && m_guardedSource;
                delete pArchive;
            }
            if (pSubDevice) {
                pSubDevice->close();
                delete pSubDevice;
            }
            if (bResult && pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_NONE;
            return bResult && m_guardedSource;
        }

        XSFX::UNPACK_CONTEXT *initializeDetachedContext(const XSFX::INTERNAL_INFO &info, XExternalArchive::EXTERNAL_FAILURE *pFailure) const
        {
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
            XSFX::UNPACK_CONTEXT *pDetached = new (std::nothrow) XSFX::UNPACK_CONTEXT;
            if (!pDetached) return nullptr;
            pDetached->pOuterSourceDevice = m_guardedSource;
            pDetached->nOwnerDeviceGeneration = m_guardedThis->getDeviceGeneration();
            pDetached->pOwnerState = nullptr;
            pDetached->info = info;
            pDetached->pSubDevice = nullptr;
            pDetached->pArchive = nullptr;
            pDetached->innerState = XBinary::UNPACK_STATE();
            QMap<XBinary::UNPACK_PROP, QVariant> attemptProperties = privateUnpackProperties(currentContext());
            initializePrivateUnpackProperties(pDetached, attemptProperties, true);

            pDetached->pSubDevice = new (std::nothrow) SubDevice(m_guardedSource, info.nArchiveOffset, info.nArchiveSize);
            bool bInitialized = pDetached->pSubDevice && pDetached->pSubDevice->open(QIODevice::ReadOnly) && m_guardedSource;
            if (bInitialized) {
                pDetached->pArchive = createArchive(info.arcType, pDetached->pSubDevice, info.bAllowOpaqueZpaq);
                XExternalArchive *pExternalArchive = externalArchiveFor(pDetached->pArchive, info.arcType);
                if (pExternalArchive) pExternalArchive->setHelperDeadline(m_helperDeadline);
                bInitialized = pDetached->pArchive && pExternalArchive &&
                               pDetached->pArchive->initUnpack(&pDetached->innerState, attemptProperties, m_pPdStruct) && m_guardedSource &&
                               (pDetached->innerState.nNumberOfRecords == m_pState->nNumberOfRecords);
                if (pExternalArchive && pFailure) *pFailure = pExternalArchive->getLastExternalFailure();
                if (!bInitialized && pExternalArchive && pFailure && (*pFailure == XExternalArchive::EXTERNAL_FAILURE_NONE)) {
                    *pFailure = XBinary::isPdStructNotCanceled(m_pPdStruct) ? XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED
                                                                          : XExternalArchive::EXTERNAL_FAILURE_CANCELED;
                }
            }
            scrubPublicUnpackProperties(&attemptProperties);
            for (qint32 i = 0; bInitialized && (i < m_nRequestedIndex); ++i) {
                bInitialized = pDetached->pArchive->moveToNext(&pDetached->innerState, m_pPdStruct) && m_guardedSource;
            }
            bInitialized = bInitialized && (pDetached->innerState.nCurrentIndex == m_nRequestedIndex) &&
                           (pDetached->innerState.nCurrentOffset == m_nOriginalOuterOffset);
            if (!bInitialized) {
                if (pFailure && (*pFailure == XExternalArchive::EXTERNAL_FAILURE_NONE)) {
                    *pFailure = XBinary::isPdStructNotCanceled(m_pPdStruct) ? XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED
                                                                          : XExternalArchive::EXTERNAL_FAILURE_CANCELED;
                }
                destroyDetachedContext(pDetached);
                return nullptr;
            }
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_NONE;
            return pDetached;
        }

        bool publishStage(QIODevice *pStage, XSFX::UNPACK_CONTEXT *pDecodedContext, const XBinary::ARCHIVERECORD &expectedRecord) const
        {
            QIODevice *guardedStage = pStage;
            if (!guardedStage || !pDecodedContext || !pDecodedContext->pArchive || !guardedStage->isOpen() || !guardedStage->isReadable() ||
                guardedStage->isSequential() || !XBinary::isResizeEnable(m_guardedOutput) ||
                XBinary::devicesAlias(guardedStage, m_guardedOutput) || XBinary::devicesAlias(m_guardedSource, m_guardedOutput) ||
                !contextIsCurrent() || !XBinary::isPdStructNotCanceled(m_pPdStruct)) {
                return false;
            }

            const qint64 nStageSize = guardedStage->size();
            const qint64 nOriginalPosition = m_guardedOutput->pos();
            if (!guardedStage || !m_guardedOutput || (nStageSize < 0) || (nOriginalPosition < 0) || !guardedStage->seek(0)) return false;

            QByteArray baBuffer;
            baBuffer.resize(0x10000);
            if (baBuffer.size() != 0x10000) return false;
            bool bOutputCleared = false;

            if (!m_guardedOutput->seek(0) || !contextIsCurrent()) return false;
            if (!XBinary::resize(m_guardedOutput, 0)) {
                if (m_guardedOutput) m_guardedOutput->seek(nOriginalPosition);
                return false;
            }
            bOutputCleared = true;
            if (!m_guardedOutput || !XBinary::resize(m_guardedOutput, nStageSize) || !contextIsCurrent() || !m_guardedOutput->seek(0)) {
                return failPublication(bOutputCleared, nOriginalPosition);
            }

            qint64 nPublished = 0;
            while (nPublished < nStageSize) {
                if (!guardedStage || !m_guardedOutput || !contextIsCurrent() || !XBinary::isPdStructNotCanceled(m_pPdStruct) || !guardedStage->seek(nPublished)) {
                    return failPublication(bOutputCleared, nOriginalPosition);
                }
                const qint64 nRequest = qMin<qint64>(baBuffer.size(), nStageSize - nPublished);
                const qint64 nRead = guardedStage->read(baBuffer.data(), nRequest);
                if ((nRead <= 0) || (nRead > nRequest) || !contextIsCurrent() ||
                    (m_guardedThis->safeWriteData(m_guardedOutput, nPublished, baBuffer.constData(), nRead, m_pPdStruct) != nRead) || !contextIsCurrent()) {
                    return failPublication(bOutputCleared, nOriginalPosition);
                }
                nPublished += nRead;
            }

            if (!m_guardedOutput || !contextIsCurrent() || (m_guardedOutput->size() != nPublished) || !m_guardedOutput->seek(nPublished) ||
                !contextIsCurrent() || !XBinary::isPdStructNotCanceled(m_pPdStruct)) {
                return failPublication(bOutputCleared, nOriginalPosition);
            }

            // Output callbacks run after the helper authenticated the private
            // stage. Revalidate the inner source once more before success escapes.
            const XBinary::ARCHIVERECORD currentRecord = pDecodedContext->pArchive->infoCurrent(&pDecodedContext->innerState, m_pPdStruct);
            if (!contextIsCurrent() || !recordsEqual(currentRecord, expectedRecord)) {
                return failPublication(bOutputCleared, nOriginalPosition);
            }
            return true;
        }

    private:
        XSFX::UNPACK_CONTEXT *currentContext() const
        {
            return m_ppContext ? *m_ppContext : nullptr;
        }

        XArchive *createArchive(XBinary::FT arcType, QIODevice *pDevice, bool bAllowOpaqueZpaq) const
        {
            return m_guardedThis ? (m_guardedThis->*m_pCreateArchiveMethod)(arcType, pDevice, bAllowOpaqueZpaq) : nullptr;
        }

        bool failPublication(bool bOutputCleared, qint64 nOriginalPosition) const
        {
            if (m_guardedOutput && bOutputCleared) {
                XBinary::resize(m_guardedOutput, 0);
                if (m_guardedOutput) m_guardedOutput->seek(0);
            } else if (m_guardedOutput && (nOriginalPosition >= 0)) {
                m_guardedOutput->seek(nOriginalPosition);
            }
            return false;
        }

        XSFX *m_guardedThis;
        QIODevice *m_guardedOutput;
        QIODevice *m_guardedSource;
        XSFX::UNPACK_CONTEXT **m_ppContext;
        XBinary::UNPACK_STATE *m_pState;
        qint32 m_nRequestedIndex;
        qint64 m_nOriginalOuterOffset;
        QDeadlineTimer m_helperDeadline;
        XBinary::PDSTRUCT *m_pPdStruct;
        CREATE_ARCHIVE_METHOD m_pCreateArchiveMethod;
    };

    SFX_DEFERRED_CANDIDATE_HELPER helper(this, guardedOutput, guardedSource, &pContext, pState, nRequestedIndex,
                                         nOriginalOuterOffset, helperDeadline, pPdStruct, &XSFX::_createArchive);

    const ARCHIVERECORD expectedCurrent = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!helper.contextIsCurrent() || !expectedCurrent.mapProperties.contains(FPART_PROP_ORIGINALNAME)) {
        return false;
    }

    QTemporaryFile stagedOutput(QDir(QDir::tempPath()).filePath(QStringLiteral("xfileunpacker-sfx-member-XXXXXX")));
    if (!stagedOutput.open()) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Cannot create a private SFX member stage"));
        return false;
    }
    pCurrentExternalArchive->setHelperDeadline(helperDeadline);
    bool bDecoded = pContext->pArchive->unpackCurrent(&pContext->innerState, &stagedOutput, pPdStruct);
    const XExternalArchive::EXTERNAL_FAILURE decodeFailure = pCurrentExternalArchive->getLastExternalFailure();
    pCurrentExternalArchive->clearHelperDeadline();
    if (!helper.contextIsCurrent()) return false;
    if (bDecoded) {
        clearPrivateUnpackCredential(pContext);
        const bool bPublished = helper.publishStage(&stagedOutput, pContext, expectedCurrent);
        if (!helper.contextIsCurrent()) return false;
        if (!bPublished) {
            pContext->innerState.nCurrentOffset = nOriginalOuterOffset;
            return false;
        }
        pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
        pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
        return true;
    }
    pContext->innerState.nCurrentOffset = nOriginalOuterOffset;

    const QString sDecodeError = XBinary::getPdStructErrorString(pPdStruct);
    if (!pContext->info.bProvisional || (decodeFailure != XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // The first candidate has already published a manifest. A later candidate
    // may replace it only after its complete, ordered record set and archive
    // properties match exactly. This prevents a body failure from silently
    // changing what the caller inspected before requesting extraction.
    XBinary::setPdStructErrorString(pPdStruct, sInitialError);
    QList<ARCHIVERECORD> listExpectedManifest;
    QMap<FPART_PROP, QVariant> mapExpectedProperties;
    XExternalArchive::EXTERNAL_FAILURE manifestFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
    if (!helper.collectManifest(pContext->info, &listExpectedManifest, &mapExpectedProperties, &manifestFailure) || !helper.contextIsCurrent()) {
        return false;
    }

    XSFX detector(guardedSource, isImage(), getModuleAddress(), m_requiredArcType);
    XSFX_ZPAQ_SCAN_CACHE zpaqScanCache;
    XSFX_FREEARC_SCAN_CACHE freeArcScanCache;
    qint64 nMinimumArchiveOffset = pContext->info.nArchiveOffset + 1;
    const FT fallbackType = pContext->info.arcType;
    QString sLastDecodeError = sDecodeError;

    for (qint32 nAttempt = 0; (nAttempt < SFX_SIGNATURE_CANDIDATE_LIMIT) && helper.contextIsCurrent() && XBinary::isPdStructNotCanceled(pPdStruct); ++nAttempt) {
        XBinary::setPdStructErrorString(pPdStruct, sInitialError);
        const INTERNAL_INFO info = detector._detect(pPdStruct, &zpaqScanCache, &freeArcScanCache, nMinimumArchiveOffset);
        if (!helper.contextIsCurrent() || !info.bIsValid) break;
        if (info.nArchiveOffset >= (std::numeric_limits<qint64>::max)()) break;
        nMinimumArchiveOffset = info.nArchiveOffset + 1;
        if (info.arcType != fallbackType) continue;

        QList<ARCHIVERECORD> listCandidateManifest;
        QMap<FPART_PROP, QVariant> mapCandidateProperties;
        XExternalArchive::EXTERNAL_FAILURE candidateFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
        if (!helper.collectManifest(info, &listCandidateManifest, &mapCandidateProperties, &candidateFailure)) {
            sLastDecodeError = XBinary::getPdStructErrorString(pPdStruct);
            if (!helper.contextIsCurrent() || !info.bProvisional || (candidateFailure != XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED)) break;
            continue;
        }
        bool bEquivalent = (mapExpectedProperties == mapCandidateProperties) && (listExpectedManifest.size() == listCandidateManifest.size());
        for (qint32 i = 0; bEquivalent && (i < listExpectedManifest.size()); ++i) {
            bEquivalent = helper.recordsEqual(listExpectedManifest.at(i), listCandidateManifest.at(i));
        }
        if (!bEquivalent) continue;

        candidateFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
        UNPACK_CONTEXT *pCandidate = helper.initializeDetachedContext(info, &candidateFailure);
        if (!pCandidate || !helper.contextIsCurrent()) {
            helper.destroyDetachedContext(pCandidate);
            sLastDecodeError = XBinary::getPdStructErrorString(pPdStruct);
            if (!helper.contextIsCurrent() || !info.bProvisional || (candidateFailure != XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED)) break;
            continue;
        }
        // XFU-015: the candidate's inner unpackCurrent performs its own
        // entry accounting and debits its production into the stage.
        pCandidate->innerState.spOutputBudget = pState->spOutputBudget;
        QTemporaryFile candidateStage(QDir(QDir::tempPath()).filePath(QStringLiteral("xfileunpacker-sfx-fallback-XXXXXX")));
        XExternalArchive *pCandidateExternalArchive = externalArchiveFor(pCandidate->pArchive, info.arcType);
        bool bCandidateDecoded = candidateStage.open();
        candidateFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
        if (bCandidateDecoded && pCandidateExternalArchive) {
            bCandidateDecoded = pCandidate->pArchive->unpackCurrent(&pCandidate->innerState, &candidateStage, pPdStruct) && helper.contextIsCurrent();
            candidateFailure = pCandidateExternalArchive->getLastExternalFailure();
            pCandidateExternalArchive->clearHelperDeadline();
        } else {
            bCandidateDecoded = false;
        }
        if (!bCandidateDecoded) {
            sLastDecodeError = XBinary::getPdStructErrorString(pPdStruct);
            const bool bMayContinue = info.bProvisional && (candidateFailure == XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED);
            helper.destroyDetachedContext(pCandidate);
            if (!bMayContinue) break;
            continue;
        }
        clearPrivateUnpackCredential(pCandidate);

        const ARCHIVERECORD candidateCurrent = pCandidate->pArchive->infoCurrent(&pCandidate->innerState, pPdStruct);
        if (!helper.contextIsCurrent() || !helper.recordsEqual(candidateCurrent, expectedCurrent) || !helper.publishStage(&candidateStage, pCandidate, expectedCurrent)) {
            pCandidate->innerState.nCurrentOffset = nOriginalOuterOffset;
            helper.destroyDetachedContext(pCandidate);
            return false;
        }

        UNPACK_CONTEXT *pOldContext = pContext;
        pCandidate->pOwnerState = pState;
        pState->pContext = pCandidate;
        pContext = pCandidate;
        pState->nCurrentOffset = pCandidate->innerState.nCurrentOffset;
        pState->mapArchiveProperties = pCandidate->innerState.mapArchiveProperties;
        helper.destroyDetachedContext(pOldContext);
        return guardedOutput && guardedSource && (pState->pContext == pCandidate);
    }

    XBinary::setPdStructErrorString(pPdStruct, sLastDecodeError);
    return false;
}

bool XSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) {
        return false;
    }

    bool bResult = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return false;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;

    return bResult;
}

bool XSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (pContext->pOwnerState != pState) return false;
        pState->pContext = nullptr;

        if (pContext->pArchive) {
            bResult = pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
            pContext->pArchive = nullptr;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
            pContext->pSubDevice = nullptr;
        }

        clearPrivateUnpackCredential(pContext);
        delete pContext;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return bResult;
}
