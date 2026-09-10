/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbinshsfx.h"

#include "subdevice.h"
#include "../Formats/xformats.h"

#include <QPointer>
#include <QTemporaryFile>

#include <memory>
#include <new>

namespace {
// The wrapper script is always plain ASCII and the carve directive sits in the
// first kilobyte of every observed revision; 64 KiB is a generous bound that
// still keeps detection cheap.
const qint64 BINSH_MAX_PREAMBLE = 65536;
// "tail +N" is a LINE number, so the byte offset can only be found by counting
// newlines.  All real payloads start below 20 KiB; the cap stops a crafted
// script with a huge N from turning detection into a full-file scan.
const qint64 BINSH_MAX_LINE_SCAN = Q_INT64_C(1) * 1024 * 1024;
const qint64 BINSH_MIN_FILE_SIZE = 4096;
const qint64 BINSH_TAR_BLOCK_SIZE = 512;
const qint32 BINSH_MIN_LINE_NUMBER = 2;
const qint32 BINSH_MAX_LINE_NUMBER = 100000;
const qint32 BINSH_MAX_LINE_DIGITS = 6;
const qint32 BINSH_USTAR_MAGIC_OFFSET = 257;
const qint32 BINSH_USTAR_SIZE_OFFSET = 124;
const qint32 BINSH_USTAR_SIZE_LENGTH = 12;
const qint32 BINSH_USTAR_CHECKSUM_OFFSET = 148;
const qint32 BINSH_USTAR_CHECKSUM_LENGTH = 8;
const char BINSH_RECURSION_PROPERTY[] = "_xfileunpacker_binsh_sfx_skip";

bool binshHasShebang(const QByteArray &baHead)
{
    // Sun ships "#!/bin/sh"; the two spelling variants are accepted because
    // the same generator family emits them and the carve directive is what
    // actually authenticates the file.
    return baHead.startsWith("#!/bin/sh") || baHead.startsWith("#! /bin/sh") ||
           baHead.startsWith("#!/sbin/sh");
}

bool binshIsWordCharacter(char cCharacter)
{
    const quint8 nCharacter = static_cast<quint8>(cCharacter);
    return ((nCharacter >= '0') && (nCharacter <= '9')) ||
           ((nCharacter >= 'A') && (nCharacter <= 'Z')) ||
           ((nCharacter >= 'a') && (nCharacter <= 'z')) ||
           (nCharacter == '_');
}

bool binshSkipBlanks(const QByteArray &baHead, qint32 *pnPosition,
                     bool bRequired)
{
    const qint32 nStart = *pnPosition;
    while (*pnPosition < baHead.size()) {
        const char cCharacter = baHead.at(*pnPosition);
        if ((cCharacter != ' ') && (cCharacter != '\t')) break;
        ++(*pnPosition);
    }
    return !bRequired || (*pnPosition > nStart);
}

// Matches "tail [-n] +<N> $0" starting at nStart, which the caller has already
// positioned on the literal "tail".  Returns the parsed line number and the
// offset just past the directive.
bool binshMatchTailDirective(const QByteArray &baHead, qint32 nStart,
                             qint32 *pnLineNumber, qint32 *pnMatchEnd)
{
    if ((nStart > 0) && binshIsWordCharacter(baHead.at(nStart - 1))) {
        return false;
    }
    qint32 nPosition = nStart + 4;
    if (!binshSkipBlanks(baHead, &nPosition, true)) return false;

    // GNU tail spells the same request "tail -n +N"; both forms appear in
    // shell self-extractors of this era.
    if (((nPosition + 1) < baHead.size()) && (baHead.at(nPosition) == '-') &&
        (baHead.at(nPosition + 1) == 'n')) {
        nPosition += 2;
        binshSkipBlanks(baHead, &nPosition, false);
    }

    // The '+' is the whole point: "tail +N" starts AT line N, while "tail -N"
    // means the last N lines and carries no offset information at all.
    if ((nPosition >= baHead.size()) || (baHead.at(nPosition) != '+')) {
        return false;
    }
    ++nPosition;

    qint32 nDigits = 0;
    qint64 nLineNumber = 0;
    while ((nPosition < baHead.size()) && (baHead.at(nPosition) >= '0') &&
           (baHead.at(nPosition) <= '9')) {
        if (nDigits >= BINSH_MAX_LINE_DIGITS) return false;
        nLineNumber = (nLineNumber * 10) + (baHead.at(nPosition) - '0');
        ++nPosition;
        ++nDigits;
    }
    if ((nDigits == 0) || (nLineNumber < BINSH_MIN_LINE_NUMBER) ||
        (nLineNumber > BINSH_MAX_LINE_NUMBER)) {
        return false;
    }
    if (!binshSkipBlanks(baHead, &nPosition, true)) return false;

    qint32 nArgumentSize = 0;
    if (baHead.mid(nPosition, 4) == "\"$0\"") {
        nArgumentSize = 4;
    } else if (baHead.mid(nPosition, 4) == "${0}") {
        nArgumentSize = 4;
    } else if (baHead.mid(nPosition, 2) == "$0") {
        nArgumentSize = 2;
    } else {
        return false;
    }

    *pnLineNumber = static_cast<qint32>(nLineNumber);
    *pnMatchEnd = nPosition + nArgumentSize;
    return true;
}

bool binshIsCompressHeader(const uchar *pHeader)
{
    // Identical to XCompressZ::isValid's own header test, so a payload this
    // class accepts is one the compress reader will also accept.
    const qint32 nMaxBits = pHeader[2] & 0x1f;
    return (pHeader[0] == 0x1f) && (pHeader[1] == 0x9d) &&
           ((pHeader[2] & 0x60) == 0) && (nMaxBits >= 9) && (nMaxBits <= 16);
}

bool binshParseOctalField(const uchar *pField, qint32 nSize, quint64 *pnValue)
{
    quint64 nValue = 0;
    qint32 nPosition = 0;
    while ((nPosition < nSize) && (pField[nPosition] == ' ')) ++nPosition;

    qint32 nDigits = 0;
    while ((nPosition < nSize) && (pField[nPosition] >= '0') &&
           (pField[nPosition] <= '7')) {
        if (nValue > ((~Q_UINT64_C(0)) >> 3)) return false;
        nValue = (nValue << 3) | static_cast<quint64>(pField[nPosition] - '0');
        ++nPosition;
        ++nDigits;
    }
    if (nDigits == 0) return false;

    // Writers terminate the field with NUL, space, or both; anything else
    // means this is not an octal tar field.
    while (nPosition < nSize) {
        if ((pField[nPosition] != '\0') && (pField[nPosition] != ' ')) {
            return false;
        }
        ++nPosition;
    }
    *pnValue = nValue;
    return true;
}

bool binshIsUstarHeader(const QByteArray &baBlock)
{
    if (baBlock.size() != BINSH_TAR_BLOCK_SIZE) return false;
    const uchar *pBlock = reinterpret_cast<const uchar *>(baBlock.constData());
    if (pBlock[0] == 0) return false;  // the end-of-archive block, not a member

    const QByteArray baMagic =
        baBlock.mid(BINSH_USTAR_MAGIC_OFFSET, 6);
    if ((baMagic != QByteArray("ustar\0", 6)) && (baMagic != "ustar ")) {
        return false;
    }

    quint64 nSize = 0;
    if (!binshParseOctalField(pBlock + BINSH_USTAR_SIZE_OFFSET,
                              BINSH_USTAR_SIZE_LENGTH, &nSize)) {
        return false;
    }
    quint64 nStoredChecksum = 0;
    if (!binshParseOctalField(pBlock + BINSH_USTAR_CHECKSUM_OFFSET,
                              BINSH_USTAR_CHECKSUM_LENGTH,
                              &nStoredChecksum)) {
        return false;
    }

    // The unsigned 8-bit sum over the header with the checksum field blanked
    // is the strong half of the gate: it authenticates all 512 bytes instead
    // of sniffing a magic string that any text file could contain.
    quint64 nCalculatedChecksum = 0;
    for (qint32 i = 0; i < BINSH_TAR_BLOCK_SIZE; i++) {
        const bool bIsChecksumByte =
            (i >= BINSH_USTAR_CHECKSUM_OFFSET) &&
            (i < (BINSH_USTAR_CHECKSUM_OFFSET + BINSH_USTAR_CHECKSUM_LENGTH));
        nCalculatedChecksum += bIsChecksumByte ? 0x20U : pBlock[i];
    }
    return nCalculatedChecksum == nStoredChecksum;
}

// Scoped dynamic property on a device, used to fence off the payload view
// while XFormats decides what it is.
class DevicePropertyOverride {
public:
    DevicePropertyOverride(QIODevice *pDevice, const char *pName,
                           const QVariant &value)
        : m_pDevice(pDevice),
          m_name(pName),
          m_oldValue(),
          m_bHadProperty(false),
          m_bApplied(false)
    {
        if (!m_pDevice || m_name.isEmpty()) return;
        m_bHadProperty = m_pDevice->dynamicPropertyNames().contains(m_name);
        m_oldValue = m_pDevice->property(m_name.constData());
        m_pDevice->setProperty(m_name.constData(), value);
        m_bApplied = m_pDevice &&
                     (m_pDevice->property(m_name.constData()) == value);
    }

    ~DevicePropertyOverride()
    {
        if (!m_pDevice || m_name.isEmpty()) return;
        m_pDevice->setProperty(m_name.constData(),
                               m_bHadProperty ? m_oldValue : QVariant());
    }

    bool isApplied() const
    {
        return m_bApplied && m_pDevice;
    }

private:
    QPointer<QIODevice> m_pDevice;
    QByteArray m_name;
    QVariant m_oldValue;
    bool m_bHadProperty;
    bool m_bApplied;
};

bool binshIsReadableSeekableDevice(const QPointer<QIODevice> &guardedDevice)
{
    if (!guardedDevice) return false;
    const bool bSequential = guardedDevice->isSequential();
    if (!guardedDevice || bSequential) return false;
    const bool bOpen = guardedDevice->isOpen();
    if (!guardedDevice || !bOpen) return false;
    const bool bReadable = guardedDevice->isReadable();
    return guardedDevice && bReadable;
}
}  // namespace

XBinShSFX::CONTEXT::CONTEXT()
    : pPayloadDevice(nullptr),
      pInnerArchive(nullptr),
      innerState(),
      carve(),
      innerFileType(FT_UNKNOWN),
      bInnerInitialized(false)
{
    carve.nInputSize = -1;
    carve.nCarveOffset = -1;
    carve.nPayloadSize = -1;
    carve.nDirectiveOffset = -1;
    carve.nLineNumber = 0;
    carve.payloadKind = PAYLOAD_KIND_UNKNOWN;
}

XBinShSFX::CONTEXT::~CONTEXT()
{
    finishInner(nullptr);
    delete pInnerArchive;
    pInnerArchive = nullptr;
    // The inner archive reads through this view, so it must outlive nothing.
    delete pPayloadDevice;
    pPayloadDevice = nullptr;
}

bool XBinShSFX::CONTEXT::finishInner(PDSTRUCT *pPdStruct)
{
    if (!bInnerInitialized) return true;
    if (!pInnerArchive) {
        bInnerInitialized = false;
        innerState = UNPACK_STATE();
        return false;
    }

    QPointer<XArchive> guardedInner(pInnerArchive);
    const bool bResult =
        guardedInner->finishUnpack(&innerState, pPdStruct) && guardedInner;
    if (!guardedInner) pInnerArchive = nullptr;
    bInnerInitialized = false;
    innerState = UNPACK_STATE();
    return bResult;
}

XBinShSFX::XBinShSFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBinShSFX::~XBinShSFX()
{
}

const char *XBinShSFX::recursionPropertyName()
{
    return BINSH_RECURSION_PROPERTY;
}

bool XBinShSFX::isRecursionSuppressed(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    return guardedDevice &&
           guardedDevice->property(BINSH_RECURSION_PROPERTY).toBool();
}

bool XBinShSFX::parseCarve(CARVE *pCarve, PDSTRUCT *pPdStruct)
{
    if (!pCarve || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XBinShSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;
    const bool bSuppressed = isRecursionSuppressed(guardedSource.data());
    if (!guardedThis || !guardedSource || bSuppressed) return false;

    CARVE carve = {};
    carve.payloadKind = PAYLOAD_KIND_UNKNOWN;
    carve.nInputSize = guardedSource->size();
    if (!guardedThis || !guardedSource ||
        (carve.nInputSize < BINSH_MIN_FILE_SIZE)) {
        return false;
    }

    const qint64 nHeadSize = qMin(carve.nInputSize, BINSH_MAX_PREAMBLE);
    const QByteArray baHead = read_array_process(0, nHeadSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHead.size() != static_cast<int>(nHeadSize)) ||
        !binshHasShebang(baHead)) {
        return false;
    }

    QByteArray baScan;
    bool bScanLoaded = false;
    qint32 nSearchFrom = 0;
    while (isPdStructNotCanceled(pPdStruct)) {
        const int nTailPosition = baHead.indexOf("tail", nSearchFrom);
        if (nTailPosition < 0) return false;
        nSearchFrom = nTailPosition + 1;

        qint32 nLineNumber = 0;
        qint32 nMatchEnd = 0;
        if (!binshMatchTailDirective(baHead, nTailPosition, &nLineNumber,
                                     &nMatchEnd)) {
            continue;
        }

        if (!bScanLoaded) {
            const qint64 nScanSize =
                qMin(carve.nInputSize, BINSH_MAX_LINE_SCAN);
            baScan = read_array_process(0, nScanSize, pPdStruct);
            if (!guardedThis || !guardedSource ||
                (baScan.size() != static_cast<int>(nScanSize))) {
                return false;
            }
            bScanLoaded = true;
        }

        // Historic BSD semantics: "+N" is 1-based "begin at line N", so the
        // payload starts after the (N-1)th newline.  Taking N newlines instead
        // lands one line late and the header probe below then fails silently,
        // which is exactly how an off-by-one here would hide.
        qint32 nOffset = 0;
        qint32 nNewlines = 0;
        bool bResolved = true;
        while (nNewlines < (nLineNumber - 1)) {
            const int nIndex = baScan.indexOf('\n', nOffset);
            if (nIndex < 0) {
                bResolved = false;
                break;
            }
            nOffset = nIndex + 1;
            ++nNewlines;
        }
        if (!bResolved) continue;

        const qint64 nCarveOffset = nOffset;
        // The payload must live after the directive that describes it, and a
        // whole probe block must be addressable inside the real file.
        if ((nCarveOffset <= nMatchEnd) ||
            (nCarveOffset > (carve.nInputSize - BINSH_TAR_BLOCK_SIZE))) {
            continue;
        }

        const QByteArray baProbe = read_array_process(
            nCarveOffset, BINSH_TAR_BLOCK_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baProbe.size() != static_cast<int>(BINSH_TAR_BLOCK_SIZE))) {
            return false;
        }

        PAYLOAD_KIND payloadKind = PAYLOAD_KIND_UNKNOWN;
        if (binshIsCompressHeader(
                reinterpret_cast<const uchar *>(baProbe.constData()))) {
            payloadKind = PAYLOAD_KIND_COMPRESS;
        } else if (binshIsUstarHeader(baProbe)) {
            payloadKind = PAYLOAD_KIND_TAR;
        } else {
            continue;
        }

        carve.nCarveOffset = nCarveOffset;
        carve.nPayloadSize = carve.nInputSize - nCarveOffset;
        carve.nDirectiveOffset = nTailPosition;
        carve.nLineNumber = nLineNumber;
        carve.payloadKind = payloadKind;
        *pCarve = carve;
        return guardedThis && guardedSource &&
               isPdStructNotCanceled(pPdStruct);
    }

    return false;
}

bool XBinShSFX::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition =
        guardedSource ? guardedSource->pos() : -1;
    CARVE carve = {};
    const bool bResult = parseCarve(&carve, pPdStruct);
    // parseCarve probes deep inside the file; leaving the cursor there would
    // change what the next class in the detection chain reads.
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XBinShSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBinShSFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBinShSFX::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBinShSFX(pDevice);
}

QList<QString> XBinShSFX::getSearchSignatures()
{
    return {QStringLiteral("'#!/bin/sh'"), QStringLiteral("'#! /bin/sh'"),
            QStringLiteral("'#!/sbin/sh'")};
}

XBinary::FT XBinShSFX::getFileType()
{
    return FT_BINSH_SFX;
}

XBinary::MODE XBinShSFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBinShSFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBinShSFX::getArch()
{
    return QString();
}

QString XBinShSFX::getFileFormatExt()
{
    return QStringLiteral("sh");
}

QString XBinShSFX::getFileFormatExtsString()
{
    return QStringLiteral("Shell self-extractor (*.sh *.bin)");
}

QString XBinShSFX::getMIMEString()
{
    return QStringLiteral("application/x-shellscript");
}

QString XBinShSFX::getVersion()
{
    return QString();
}

XBinary::OSNAME XBinShSFX::getOsName()
{
    return OSNAME_UNIX;
}

qint64 XBinShSFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CARVE carve = {};
    // The payload runs to EOF in every observed revision, so the format size
    // is the whole file.
    return parseCarve(&carve, pPdStruct) ? carve.nInputSize : 0;
}

QList<XBinary::MAPMODE> XBinShSFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBinShSFX::getMemoryMap(MAPMODE mapMode,
                                             PDSTRUCT *pPdStruct)
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

QString XBinShSFX::payloadName(const CARVE &carve)
{
    // The script's own name for the payload is a shell variable ("$outname")
    // or an outright lie ("temp.tar.Z" for what is a bare tar), so the name is
    // derived from the bytes instead.
    if (carve.payloadKind == PAYLOAD_KIND_COMPRESS) {
        return QStringLiteral("payload.tar.Z");
    }
    if (carve.payloadKind == PAYLOAD_KIND_TAR) {
        return QStringLiteral("payload.tar");
    }
    return QStringLiteral("payload");
}

QString XBinShSFX::payloadDescription(const CARVE &carve)
{
    return QStringLiteral("Shell SFX carve: tail +%1 (offset 0x%2), %3")
        .arg(carve.nLineNumber)
        .arg(carve.nCarveOffset, 0, 16)
        .arg(payloadName(carve));
}

bool XBinShSFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBinShSFX::getFileParts(quint32 nFileParts,
                                              qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CARVE carve = {};
    if (!parseCarve(&carve, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = carve.nCarveOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Shell extractor script");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        // The carve is a verbatim byte range: the wrapper adds no header, no
        // padding and no checksum around the payload.
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = carve.nCarveOffset;
        part.nFileSize = carve.nPayloadSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = payloadName(carve);
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  carve.nPayloadSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  carve.nPayloadSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        part.mapProperties.insert(FPART_PROP_ORIGINALNAME, payloadName(carve));
        part.mapProperties.insert(FPART_PROP_INFO,
                                  payloadDescription(carve));
        result.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = carve.nCarveOffset;
        part.nFileSize = carve.nPayloadSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = payloadName(carve);
        result.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = carve.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }

    return result;
}

XBinary::FT XBinShSFX::detectPayloadFileType(QIODevice *pDevice,
                                             PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    const bool bUsableDevice = binshIsReadableSeekableDevice(guardedDevice);
    if (!guardedDevice || !bUsableDevice ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FT_UNKNOWN;
    }

    const qint64 nOriginalPosition = guardedDevice->pos();
    if (!guardedDevice || (nOriginalPosition < 0)) return FT_UNKNOWN;

    FT result = FT_UNKNOWN;
    {
        // Detection of the payload runs the whole archive chain, which
        // includes this class.  The property keeps that chain from claiming
        // the view back and recursing.
        DevicePropertyOverride recursionGuard(guardedDevice.data(),
                                              BINSH_RECURSION_PROPERTY, true);
        if (recursionGuard.isApplied() && guardedDevice) {
            result = XFormats::getPrefFileType(guardedDevice.data(),
                                               FT_FLAG_ARCHIVES, pPdStruct);
        }
    }

    if (!guardedDevice || !guardedDevice->seek(nOriginalPosition) ||
        !guardedDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FT_UNKNOWN;
    }
    return result;
}

XArchive *XBinShSFX::createPayloadArchive(FT fileType, QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return nullptr;

    XBinary *pBinary = nullptr;
    {
        DevicePropertyOverride recursionGuard(guardedDevice.data(),
                                              BINSH_RECURSION_PROPERTY, true);
        if (!recursionGuard.isApplied() || !guardedDevice) return nullptr;
        pBinary = XFormats::createClass(fileType, guardedDevice.data());
    }

    if (!guardedDevice || !pBinary) {
        delete pBinary;
        return nullptr;
    }

    XArchive *pArchive = dynamic_cast<XArchive *>(pBinary);
    // A payload that somehow resolved back to this class would recurse on the
    // same backing file.
    if (!pArchive || dynamic_cast<XBinShSFX *>(pArchive)) {
        delete pBinary;
        return nullptr;
    }
    return pArchive;
}

bool XBinShSFX::publicStateMatchesInner(const UNPACK_STATE *pState,
                                        const CONTEXT *pContext)
{
    if (!pState || !pContext || (pState->pContext != pContext) ||
        (pContext->carve.nInputSize < 0) || (pContext->carve.nCarveOffset < 0) ||
        (pContext->innerState.nNumberOfRecords < 0) ||
        (pContext->innerState.nCurrentIndex < 0) ||
        (pContext->innerState.nCurrentIndex >
         pContext->innerState.nNumberOfRecords)) {
        return false;
    }

    const qint64 nExpectedOffset =
        (pContext->innerState.nCurrentIndex <
         pContext->innerState.nNumberOfRecords)
            ? pContext->carve.nCarveOffset
            : pContext->carve.nInputSize;
    return (pState->nCurrentOffset == nExpectedOffset) &&
           (pState->nTotalSize == pContext->carve.nInputSize) &&
           (pState->nCurrentIndex == pContext->innerState.nCurrentIndex) &&
           (pState->nNumberOfRecords ==
            pContext->innerState.nNumberOfRecords);
}

void XBinShSFX::copyInnerState(UNPACK_STATE *pState, const CONTEXT *pContext)
{
    if (!pState || !pContext) return;
    // The public offsets stay in OUR file: the payload start while records
    // remain, EOF once the walk is done.
    pState->nCurrentOffset = (pContext->innerState.nCurrentIndex <
                              pContext->innerState.nNumberOfRecords)
                                 ? pContext->carve.nCarveOffset
                                 : pContext->carve.nInputSize;
    pState->nTotalSize = pContext->carve.nInputSize;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->mapUnpackProperties = pContext->innerState.mapUnpackProperties;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO,
                                        payloadDescription(pContext->carve));
}

bool XBinShSFX::failUnpackInitialization(XBinShSFX *pArchive,
                                         UNPACK_STATE *pState)
{
    if (pArchive) pArchive->releaseUnpackSource(pState);
    if (pState) *pState = UNPACK_STATE();
    return false;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBinShSFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBinShSFX::initUnpack(UNPACK_STATE *pState,
                           const QMap<UNPACK_PROP, QVariant> &mapProperties,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XBinShSFX> guardedThis(this);
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
    if (!operationGuard.isAcquired()) return false;

    const bool bSuppressed = isRecursionSuppressed(guardedSource.data());
    if (!guardedThis || !guardedSource || bSuppressed) return false;
    const qint64 nOriginalPosition = guardedSource->pos();
    if (!guardedThis || !guardedSource || (nOriginalPosition < 0)) return false;

    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis ||
        !guardedSource) {
        return false;
    }

    std::unique_ptr<CONTEXT> pContext(new (std::nothrow) CONTEXT());
    if (!pContext) return failUnpackInitialization(guardedThis.data(), pState);
    if (!parseCarve(&pContext->carve, pPdStruct) || !guardedThis ||
        !guardedSource) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    // SubDevice re-seeks its backing device on every read, so the payload view
    // and this class can share the one open file safely.
    pContext->pPayloadDevice = new (std::nothrow) SubDevice(
        guardedSource.data(), pContext->carve.nCarveOffset,
        pContext->carve.nPayloadSize);
    QPointer<QIODevice> guardedPayload(pContext->pPayloadDevice);
    if (!guardedPayload || !guardedThis || !guardedSource) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }
    const bool bPayloadOpened =
        guardedPayload->open(QIODevice::ReadOnly);
    if (!guardedThis || !guardedSource || !guardedPayload || !bPayloadOpened ||
        (guardedPayload->size() != pContext->carve.nPayloadSize)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    const FT payloadFileType =
        detectPayloadFileType(guardedPayload.data(), pPdStruct);
    if (!guardedThis || !guardedSource || !guardedPayload ||
        (payloadFileType == FT_UNKNOWN) ||
        !XFormats::isArchive(payloadFileType)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }
    pContext->innerFileType = payloadFileType;
    pContext->pInnerArchive =
        createPayloadArchive(payloadFileType, guardedPayload.data());
    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    if (!guardedInner || !guardedThis || !guardedSource || !guardedPayload) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    bool bResult =
        guardedInner->initUnpack(&pContext->innerState, mapProperties,
                                 pPdStruct);
    if (bResult && guardedInner) pContext->bInnerInitialized = true;
    if (!bResult || !guardedThis || !guardedSource || !guardedPayload ||
        !guardedInner || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedSource) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    // The inner total size is the inner archive's own coordinate space - for a
    // compressed TAR that is the DECOMPRESSED size, which is deliberately not
    // compared against the payload view.
    if ((pContext->innerState.nCurrentIndex != 0) ||
        (pContext->innerState.nNumberOfRecords < 0) ||
        (pContext->innerState.nCurrentOffset < 0) ||
        (pContext->innerState.nTotalSize < 0) ||
        (pContext->innerState.nCurrentOffset >
         pContext->innerState.nTotalSize)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    if (pContext->innerState.nNumberOfRecords > 0) {
        const ARCHIVERECORD firstRecord =
            guardedInner->infoCurrent(&pContext->innerState, pPdStruct);
        if (!guardedThis || !guardedSource || !guardedPayload ||
            !guardedInner || firstRecord.mapProperties.isEmpty() ||
            (pContext->innerState.nCurrentIndex != 0) ||
            !XBinary::isPdStructNotCanceled(pPdStruct) ||
            !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
            !guardedThis || !guardedSource) {
            return failUnpackInitialization(guardedThis.data(), pState);
        }
    }

    // Opening the view and probing the payload moved the shared file cursor.
    const bool bPositionRestored = guardedSource->seek(nOriginalPosition);
    if (!guardedThis || !guardedSource || !guardedPayload || !guardedInner ||
        !bPositionRestored ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedSource) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    copyInnerState(pState, pContext.get());
    CONTEXT *pRawContext = pContext.release();
    pState->pContext = pRawContext;
    // Binding alone only STAGES the source; without this the listing works
    // and extraction silently produces nothing.
    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pRawContext, pPdStruct);
    if (!guardedThis) {
        *pState = UNPACK_STATE();
        return false;
    }
    if (!bFinalized) {
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pRawContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XBinShSFX::infoCurrent(UNPACK_STATE *pState,
                                              PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();
    QPointer<XBinShSFX> guardedThis(this);
    if (!pState || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!publicStateMatchesInner(pState, pContext) ||
        !pContext->bInnerInitialized || !pContext->pInnerArchive ||
        !pContext->pPayloadDevice) {
        return ARCHIVERECORD();
    }

    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedPayload(pContext->pPayloadDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedInner || !guardedPayload || !guardedSource) {
        return ARCHIVERECORD();
    }

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const qint32 nIndex = pContext->innerState.nCurrentIndex;
    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    ARCHIVERECORD result =
        guardedInner->infoCurrent(&pContext->innerState, pPdStruct);
    const qint64 nInnerSize = pContext->innerState.nTotalSize;
    if (!guardedThis || !guardedInner || !guardedPayload || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedInner || !guardedPayload || !guardedSource ||
        result.mapProperties.isEmpty() ||
        (pContext->innerState.nCurrentIndex != nIndex) ||
        (pContext->innerState.nNumberOfRecords != nRecords)) {
        return ARCHIVERECORD();
    }

    // A compressed-TAR payload already republishes its members as index-paired
    // archive-stream records with no extent.  Re-marking such a record would
    // rewrite an identity token that is already correct, so it is passed
    // through untouched; anything still carrying coordinates is bounded
    // against the inner archive's own address space and then marked, because
    // those coordinates do NOT resolve on this file (variant A's members live
    // in a decompressed buffer that has no counterpart here).
    qint32 nStreamIndex = -1;
    if (XBinary::getArchiveStreamRecordIndex(result, &nStreamIndex)) {
        if (nStreamIndex != nIndex) return ARCHIVERECORD();
        return result;
    }

    if ((result.nStreamOffset < 0) || (result.nStreamSize < 0) ||
        (nInnerSize < 0) || (result.nStreamOffset > nInnerSize) ||
        (result.nStreamSize > (nInnerSize - result.nStreamOffset))) {
        return ARCHIVERECORD();
    }
    if (!XBinary::markArchiveStreamRecord(&result, nIndex)) {
        return ARCHIVERECORD();
    }
    return result;
}

bool XBinShSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                              PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XBinShSFX> guardedThis(this);
    if (!pState || !pDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedOutput || !guardedSource ||
        !guardedThis->isUnpackOutputSupported(guardedOutput.data()) ||
        !guardedThis || !guardedOutput || !guardedSource ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !guardedThis || !guardedOutput || !guardedSource ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedOutput || !guardedSource) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!publicStateMatchesInner(pState, pContext) ||
        !pContext->bInnerInitialized || !pContext->pInnerArchive ||
        !pContext->pPayloadDevice) {
        return false;
    }

    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedPayload(pContext->pPayloadDevice);
    if (!guardedInner || !guardedPayload ||
        XBinary::devicesAlias(guardedPayload.data(), guardedOutput.data()) ||
        !guardedThis || !guardedInner || !guardedPayload || !guardedOutput ||
        !guardedSource) {
        return false;
    }

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const qint32 nIndex = pContext->innerState.nCurrentIndex;
    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    const ARCHIVERECORD record =
        guardedInner->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !guardedInner || !guardedPayload || !guardedOutput ||
        !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedInner || !guardedPayload || !guardedOutput ||
        !guardedSource || record.mapProperties.isEmpty() ||
        (pContext->innerState.nCurrentIndex != nIndex) ||
        (pContext->innerState.nNumberOfRecords != nRecords)) {
        return false;
    }

    qint64 nExpectedSize = -1;
    if (record.mapProperties.contains(FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bSizeOk = false;
        nExpectedSize =
            record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE)
                .toLongLong(&bSizeOk);
        if (!bSizeOk || (nExpectedSize < 0)) return false;
    }

    // The member is decoded into a private stage first: publishUnpackOutput is
    // the only path that can guarantee exact replacement of the caller's
    // destination.
    QIODevice *pStage = nullptr;
    if (nExpectedSize >= 0) {
        if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                                nExpectedSize)) {
            return false;
        }
        pStage = XBinary::createFileBuffer(nExpectedSize, pPdStruct);
    } else {
        QTemporaryFile *pTemporaryFile = new (std::nothrow) QTemporaryFile();
        if (pTemporaryFile && pTemporaryFile->open()) {
            pStage = pTemporaryFile;
        } else {
            delete pTemporaryFile;
        }
    }
    QPointer<QIODevice> guardedStage(pStage);
    if (!guardedThis || !guardedInner || !guardedPayload || !guardedOutput ||
        !guardedSource || !guardedStage ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis) {
        if (!guardedStage) pStage = nullptr;
        XBinary::freeFileBuffer(&pStage);
        return false;
    }

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    // The inner session does its own per-entry and produced-byte accounting,
    // so the budget is threaded through rather than debited twice here.
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = guardedInner->unpackCurrent(&pContext->innerState,
                                               guardedStage.data(), pPdStruct);
    if (!guardedThis || !guardedInner || !guardedPayload || !guardedOutput ||
        !guardedSource || !guardedStage) {
        bResult = false;
    }
    if (bResult) {
        const qint64 nStageSize = guardedStage->size();
        bResult = (nStageSize >= 0) &&
                  XBinary::isUnpackOutputSizeAllowed(
                      pState->mapUnpackProperties, nStageSize) &&
                  ((nExpectedSize < 0) || (nStageSize == nExpectedSize)) &&
                  (pContext->innerState.nCurrentIndex == nIndex) &&
                  (pContext->innerState.nNumberOfRecords == nRecords) &&
                  XBinary::isPdStructNotCanceled(pPdStruct) &&
                  guardedThis->isUnpackSourceCurrent(pState, pPdStruct) &&
                  guardedThis && guardedInner && guardedPayload &&
                  guardedOutput && guardedSource && guardedStage;
    }
    if (bResult) {
        bResult = guardedThis->publishUnpackOutput(
            guardedStage.data(), guardedOutput.data(), pState, pPdStruct);
    }
    if (bResult && guardedThis && guardedInner && guardedPayload &&
        guardedOutput && guardedSource && guardedStage) {
        copyInnerState(pState, pContext);
    } else {
        bResult = false;
    }

    if (!guardedStage) pStage = nullptr;
    XBinary::freeFileBuffer(&pStage);
    return bResult;
}

bool XBinShSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XBinShSFX> guardedThis(this);
    if (!pState || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!publicStateMatchesInner(pState, pContext) ||
        !pContext->bInnerInitialized || !pContext->pInnerArchive ||
        !pContext->pPayloadDevice) {
        return false;
    }

    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedPayload(pContext->pPayloadDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedInner || !guardedPayload || !guardedSource) return false;

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const qint32 nPreviousIndex = pContext->innerState.nCurrentIndex;
    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    if ((nPreviousIndex < 0) || (nPreviousIndex >= nRecords)) return false;

    // The inner class advances first and only then compares; verifying both
    // here is what keeps a "false without advancing" inner implementation from
    // silently truncating the listing to nothing.
    const bool bMoved =
        guardedInner->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !guardedInner || !guardedPayload || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedInner || !guardedPayload || !guardedSource ||
        (pContext->innerState.nNumberOfRecords != nRecords) ||
        (pContext->innerState.nCurrentIndex != (nPreviousIndex + 1)) ||
        (bMoved != (pContext->innerState.nCurrentIndex < nRecords))) {
        return false;
    }

    copyInnerState(pState, pContext);
    return bMoved;
}

bool XBinShSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    QPointer<XBinShSFX> guardedThis(this);

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedThis->ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    guardedThis->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    const bool bInnerFinished = pContext ? pContext->finishInner(nullptr) : true;
    delete pContext;
    if (!guardedThis) return false;
    *pState = UNPACK_STATE();
    return bInnerFinished;
}
