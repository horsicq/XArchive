/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free Pocket Soft RTPatch reader.
 * MIT License
 */
#include "xrtpatch.h"

#include "Algos/xrtpatchdecoder.h"

#include <QtEndian>

#include <limits>

namespace {
const qint32 RTPATCH_HEADER_SIZE = 0x1a;
const qint32 RTPATCH_DESCRIPTOR_SIZE = 34;
const qint32 RTPATCH_AUGMENTED_PREFIX_SIZE = 42;
const qint32 RTPATCH_MAX_LIST_ITEMS = 4096;
// Bytes a self-contained delta program spends before its payload: the mode
// byte, a one-byte destination index and the fill opcode.  A fourth byte, the
// end opcode, follows the payload.
const qint32 RTPATCH_DELTA_PAYLOAD_OFFSET = 3;
const qint32 RTPATCH_DELTA_OVERHEAD = 4;
// How many source descriptors a delta record may name.  Only the layout of the
// block is at stake here; the reference reads the count from the record.
const qint32 RTPATCH_MAX_SOURCES = 8;
// A candidate program is decoded during the scan to prove its shape, so its
// length is bounded.  The whole corpus stays under 32 KiB.
const qint64 RTPATCH_MAX_DELTA_PROGRAM = 64LL * 1024 * 1024;
const QByteArray RTPATCH_STREAM_PREFIX =
    QByteArray::fromHex("B59C00FF040010");

struct RTPatchDescriptor {
    QString sName;
    qint64 nOffset = 0;
    qint64 nDataOffset = 0;
    qint64 nDataSize = -1;
    qint64 nUncompressedSize = 0;
    XBinary::HANDLE_METHOD handleMethod = XBinary::HANDLE_METHOD_UNKNOWN;
    QDateTime mtDateTime;
    // Set when the record is a binary delta whose program happens to carry the
    // complete destination.  nProgramSize is then the decoded program length,
    // of which nUncompressedSize bytes at RTPATCH_DELTA_PAYLOAD_OFFSET are the
    // member.
    bool bIsDelta = false;
    qint64 nProgramSize = 0;
};

struct RTPatchStringList {
    qint64 nOffset = 0;
    qint64 nEndOffset = 0;
    QList<QByteArray> listStrings;
};

bool isSupportedVersion(quint16 nVersion)
{
    return (nVersion == 110) || (nVersion == 200) ||
           (nVersion == 211) || (nVersion == 410) ||
           (nVersion == 500) ||
           (nVersion == 650);
}

bool parsePString(const QByteArray &baData, qint64 *pPosition,
                  bool bAllowEmpty, QByteArray *pString)
{
    if (!pPosition || !pString || (*pPosition < 0) ||
        (*pPosition >= baData.size())) {
        return false;
    }
    const uchar *pData =
        reinterpret_cast<const uchar *>(baData.constData());
    qint64 nPosition = *pPosition;
    const quint8 nLength = pData[nPosition++];
    if (nLength == 0) {
        if (!bAllowEmpty) return false;
        pString->clear();
        *pPosition = nPosition;
        return true;
    }
    if ((nPosition > baData.size() - nLength) ||
        (pData[nPosition + nLength - 1] != 0)) {
        return false;
    }
    for (quint8 i = 0; i + 1 < nLength; ++i) {
        if (pData[nPosition + i] == 0) return false;
    }
    *pString = QByteArray(baData.constData() + nPosition, nLength - 1);
    *pPosition = nPosition + nLength;
    return true;
}

bool decodeSafeName(const uchar *pData, qint32 nSize, bool bFixedField,
                    QString *pName)
{
    if (!pData || !pName || (nSize <= 0)) return false;

    qint32 nLength = 0;
    while ((nLength < nSize) && pData[nLength]) ++nLength;
    if (nLength == 0) return false;
    if (bFixedField && (nLength < nSize)) {
        for (qint32 i = nLength; i < nSize; ++i) {
            if (pData[i] != 0) return false;
        }
    }

    for (qint32 i = 0; i < nLength; ++i) {
        const quint8 nCharacter = pData[i];
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
    }

    QString sName = QString::fromLatin1(
        reinterpret_cast<const char *>(pData), nLength);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) ||
        XBinary::fixFileName(sName) != sName) {
        return false;
    }
    const QStringList listParts =
        sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) ||
            (sPart == QLatin1String(".."))) {
            return false;
        }
    }
    *pName = sName;
    return true;
}

bool parseDescriptor(const QByteArray &baData, qint64 nOffset,
                     RTPatchDescriptor *pDescriptor)
{
    if (!pDescriptor || (nOffset < 0) ||
        (nOffset > baData.size() - RTPATCH_DESCRIPTOR_SIZE)) {
        return false;
    }
    const uchar *pData = reinterpret_cast<const uchar *>(
        baData.constData() + nOffset);
    QString sName;
    if (!decodeSafeName(pData, 14, true, &sName)) return false;

    const quint16 nAttributes = qFromLittleEndian<quint16>(pData + 14);
    const quint32 nSize = qFromLittleEndian<quint32>(pData + 16);
    const quint16 nDosDate = qFromLittleEndian<quint16>(pData + 20);
    const quint16 nDosTime = qFromLittleEndian<quint16>(pData + 22);
    const QDateTime mtDateTime =
        XBinary::dosDateTimeToQDateTime(nDosDate, nDosTime);
    if (((nAttributes != 0) && (nAttributes != 0x20) &&
         (nAttributes != 0x21)) || (nSize == 0) ||
        (nSize > 0x7fffffffU) || !mtDateTime.isValid()) {
        return false;
    }

    pDescriptor->sName = sName;
    pDescriptor->nOffset = nOffset;
    pDescriptor->nUncompressedSize = nSize;
    pDescriptor->mtDateTime = mtDateTime;
    return true;
}

// Whole-file records have no source descriptor.  Immediately before their
// single destination descriptor they carry the count byte, the uncompressed
// size, and the exact compressed extent.  Delta records instead place a
// source descriptor in this position, so this invariant distinguishes the two
// without attempting to interpret or execute the delta opcode program.
bool classifyWholeFileStream(const QByteArray &baData,
                             qint64 nDescriptorOffset,
                             qint64 nStreamOffset,
                             RTPatchDescriptor *pDescriptor)
{
    if (!pDescriptor || (nDescriptorOffset < 9) ||
        (nStreamOffset < nDescriptorOffset + RTPATCH_DESCRIPTOR_SIZE) ||
        (nStreamOffset > baData.size() - 8)) {
        return false;
    }

    const uchar *pData =
        reinterpret_cast<const uchar *>(baData.constData());
    const quint32 nUncompressedSize =
        qFromLittleEndian<quint32>(pData + nDescriptorOffset - 8);
    const quint32 nCompressedSize =
        qFromLittleEndian<quint32>(pData + nDescriptorOffset - 4);
    if ((pData[nDescriptorOffset - 9] != 1) ||
        (nUncompressedSize != pDescriptor->nUncompressedSize) ||
        (nCompressedSize < 8) ||
        (quint64(nCompressedSize) >
         quint64(baData.size() - nStreamOffset)) ||
        (memcmp(pData + nStreamOffset, RTPATCH_STREAM_PREFIX.constData(),
                RTPATCH_STREAM_PREFIX.size()) != 0) ||
        ((pData[nStreamOffset + 7] & 0xf0) != 0x80)) {
        return false;
    }

    pDescriptor->nDataSize = nCompressedSize;
    pDescriptor->handleMethod = XBinary::HANDLE_METHOD_RTPATCH;
    return true;
}

// A delta record (reference type 0x4000) carries the same descriptor block as a
// whole-file one, optionally preceded by a block of source descriptors.  The
// first alternative of a chain states both counts, the alternatives after it -
// and every whole-file record - state only the destination count, so those two
// forms are byte-identical and are told apart by a size relation instead: a
// whole-file record's uncompressed size IS the destination size, a delta
// record's is the length of its opcode program.
//
// The stream of a delta is normally unusable on its own, because the program
// copies from a checksum-matched source file.  One shape is the exception: the
// reference's interpreter reads a mode byte, a varint
// destination index and then opcodes, of which opcode 5 fills the whole
// remaining destination from the program itself and opcode 1/2 ends it.  With
// one destination and a one-byte index such a program is exactly the
// destination plus RTPATCH_DELTA_OVERHEAD bytes - a relation the record header
// states before anything is decoded, and the only case where the record is a
// plain archive member.
bool classifyDeltaStream(const QByteArray &baData, qint64 nDescriptorOffset,
                         qint64 nStreamOffset,
                         RTPatchDescriptor *pDescriptor)
{
    if (!pDescriptor || (nDescriptorOffset < 0) ||
        (nStreamOffset != nDescriptorOffset + RTPATCH_DESCRIPTOR_SIZE) ||
        (nStreamOffset > baData.size() - 8) ||
        (pDescriptor->nUncompressedSize <= 0) ||
        (pDescriptor->nUncompressedSize > RTPATCH_MAX_DELTA_PROGRAM)) {
        return false;
    }

    const uchar *pData =
        reinterpret_cast<const uchar *>(baData.constData());
    if ((memcmp(pData + nStreamOffset, RTPATCH_STREAM_PREFIX.constData(),
                RTPATCH_STREAM_PREFIX.size()) != 0) ||
        ((pData[nStreamOffset + 7] & 0xf0) != 0x80)) {
        return false;
    }
    const qint64 nProgramSize =
        pDescriptor->nUncompressedSize + RTPATCH_DELTA_OVERHEAD;

    // The two-count form is the more specific match - it also demands a whole
    // valid source-descriptor block - so it is tried first.
    qint64 nHeaderOffset = -1;
    for (qint32 nSources = 1;
         (nHeaderOffset < 0) && (nSources <= RTPATCH_MAX_SOURCES); ++nSources) {
        const qint64 nBlockOffset =
            nDescriptorOffset - qint64(RTPATCH_DESCRIPTOR_SIZE) * nSources;
        if (nBlockOffset < 10) continue;
        bool bSourcesValid = true;
        for (qint32 i = 0; bSourcesValid && (i < nSources); ++i) {
            RTPatchDescriptor source;
            bSourcesValid = parseDescriptor(
                baData,
                nBlockOffset + qint64(RTPATCH_DESCRIPTOR_SIZE) * i, &source);
        }
        if (!bSourcesValid || (pData[nBlockOffset - 10] != nSources) ||
            (pData[nBlockOffset - 9] != 1)) {
            continue;
        }
        nHeaderOffset = nBlockOffset - 8;
    }
    if ((nHeaderOffset < 0) && (nDescriptorOffset >= 9) &&
        (pData[nDescriptorOffset - 9] == 1)) {
        nHeaderOffset = nDescriptorOffset - 8;
    }
    if (nHeaderOffset < 0) return false;

    const quint32 nUncompressedSize =
        qFromLittleEndian<quint32>(pData + nHeaderOffset);
    const quint32 nCompressedSize =
        qFromLittleEndian<quint32>(pData + nHeaderOffset + 4);
    if ((qint64(nUncompressedSize) != nProgramSize) ||
        (nCompressedSize < 8) ||
        (quint64(nCompressedSize) >
         quint64(baData.size() - nStreamOffset))) {
        return false;
    }

    pDescriptor->nDataSize = nCompressedSize;
    pDescriptor->nProgramSize = nProgramSize;
    pDescriptor->bIsDelta = true;
    pDescriptor->handleMethod = XBinary::HANDLE_METHOD_RTPATCH;
    return true;
}

// The length relation is necessary but not sufficient, so the program is
// decoded and its shape proved before the record is offered as a member.  Mode
// 0x15 selects the complemented opcode alphabet: the interpreter reads every
// opcode as 0x17 - byte, while the payload bytes stay literal.
bool verifyDeltaProgram(const QByteArray &baData,
                        const RTPatchDescriptor &descriptor,
                        XBinary::PDSTRUCT *pPdStruct)
{
    if (!descriptor.bIsDelta || (descriptor.nUncompressedSize <= 0) ||
        (descriptor.nUncompressedSize > RTPATCH_MAX_DELTA_PROGRAM) ||
        (descriptor.nProgramSize !=
         descriptor.nUncompressedSize + RTPATCH_DELTA_OVERHEAD) ||
        (descriptor.nDataOffset < 0) || (descriptor.nDataSize <= 0) ||
        (descriptor.nDataSize > baData.size() - descriptor.nDataOffset)) {
        return false;
    }

    QByteArray baProgram;
    if (!XRTPatchDecoder::decode(
            baData.mid(qint32(descriptor.nDataOffset),
                       qint32(descriptor.nDataSize)),
            descriptor.nProgramSize, &baProgram, pPdStruct) ||
        (baProgram.size() != descriptor.nProgramSize)) {
        return false;
    }

    const uchar *pProgram =
        reinterpret_cast<const uchar *>(baProgram.constData());
    quint8 nFillOpcode = 5;
    quint8 nEndOpcodeA = 1;
    quint8 nEndOpcodeB = 2;
    if (pProgram[0] == 0x15) {
        nFillOpcode = 0x17 - 5;
        nEndOpcodeA = 0x17 - 1;
        nEndOpcodeB = 0x17 - 2;
    } else if (pProgram[0] != 2) {
        return false;
    }
    // destination index 0 as a one-byte varint, then "fill the destination
    // from the stream"
    if ((pProgram[1] != 0) || (pProgram[2] != nFillOpcode)) return false;
    const quint8 nTerminator =
        pProgram[descriptor.nUncompressedSize + RTPATCH_DELTA_PAYLOAD_OFFSET];
    return (nTerminator == nEndOpcodeA) || (nTerminator == nEndOpcodeB);
}

bool parseVersion500(const QByteArray &baData,
                     QList<RTPatchDescriptor> *pDescriptors,
                     qint32 nMaximumRecords)
{
    if (!pDescriptors || (baData.size() < 0x29) ||
        (nMaximumRecords <= 0)) {
        return false;
    }
    const uchar *pData =
        reinterpret_cast<const uchar *>(baData.constData());
    const quint16 nRecordCount =
        qFromLittleEndian<quint16>(pData + 0x1a);
    if ((nRecordCount == 0) || (nRecordCount > nMaximumRecords)) {
        return false;
    }

    // The version-5 header has four counted registry strings, followed by a
    // counted long/short directory-name table. Empty registry fields use a
    // zero length byte rather than a one-byte NUL string.
    qint64 nPosition = 0x27;
    QByteArray baString;
    for (qint32 i = 0; i < 4; ++i) {
        if (!parsePString(baData, &nPosition, true, &baString)) return false;
        if ((i < 2) && baString.isEmpty()) return false;
    }
    if (nPosition > baData.size() - 2) return false;
    const quint16 nDirectoryCount =
        qFromLittleEndian<quint16>(pData + nPosition);
    nPosition += 2;
    if (nDirectoryCount > 256) return false;

    QList<QString> listDirectories;
    listDirectories.reserve(nDirectoryCount);
    for (quint16 i = 0; i < nDirectoryCount; ++i) {
        if (!parsePString(baData, &nPosition, false, &baString)) return false;
        QString sDirectory;
        if (!decodeSafeName(
                reinterpret_cast<const uchar *>(baString.constData()),
                baString.size(), false, &sDirectory)) {
            return false;
        }
        listDirectories.append(sDirectory);
    }

    QList<RTPatchDescriptor> listDescriptors;
    listDescriptors.reserve(nRecordCount);
    for (quint16 i = 0; i < nRecordCount; ++i) {
        const qint64 nRecordOffset = nPosition;
        if (nPosition > baData.size() - 2) return false;
        const quint16 nTag =
            qFromLittleEndian<quint16>(pData + nPosition);
        nPosition += 2;
        const bool bSubdirectory = (nTag == 0x2446);
        if (!bSubdirectory && (nTag != 0x2444)) return false;
        if (bSubdirectory) {
            if ((nPosition > baData.size() - 2) ||
                (qFromLittleEndian<quint16>(pData + nPosition) != 0x01b8)) {
                return false;
            }
            nPosition += 2;
        }

        QByteArray baFullPath;
        if (!parsePString(baData, &nPosition, false, &baFullPath)) return false;
        QString sFullPath;
        if (!decodeSafeName(
                reinterpret_cast<const uchar *>(baFullPath.constData()),
                baFullPath.size(), false, &sFullPath)) {
            return false;
        }
        if (bSubdirectory) {
            if ((nPosition > baData.size() - 2) ||
                (pData[nPosition] >= nDirectoryCount) ||
                (pData[nPosition + 1] >= nDirectoryCount)) {
                return false;
            }
            const quint8 nLongDirectoryIndex = pData[nPosition + 1];
            nPosition += 2;
            if (sFullPath.section(QLatin1Char('/'), 0, 0) !=
                listDirectories.at(nLongDirectoryIndex)) {
                return false;
            }
        } else if (sFullPath.contains(QLatin1Char('/'))) {
            return false;
        }

        // Whole-file add records have no old-file identity and set flag 1.
        if (nPosition > baData.size() - 19) return false;
        for (qint32 j = 0; j < 10; ++j) {
            if (pData[nPosition + j] != 0) return false;
        }
        nPosition += 10;
        if (pData[nPosition++] != 1) return false;
        const quint32 nNewSize =
            qFromLittleEndian<quint32>(pData + nPosition);
        const quint32 nCompressedSize =
            qFromLittleEndian<quint32>(pData + nPosition + 4);
        nPosition += 8;

        const qint64 nDescriptorOffset = nPosition;
        RTPatchDescriptor descriptor;
        if (!parseDescriptor(baData, nDescriptorOffset, &descriptor) ||
            (descriptor.nUncompressedSize != nNewSize) ||
            (qFromLittleEndian<quint16>(pData + nDescriptorOffset + 14) !=
             0x20)) {
            return false;
        }
        // Version 5 extends the common 24-byte name/size/time prefix with an
        // 18-byte opaque checksum/bookkeeping tail.
        if (nPosition > baData.size() - 42) return false;
        nPosition += 42;

        QByteArray baBaseName;
        if (!parsePString(baData, &nPosition, false, &baBaseName)) return false;
        QString sBaseName;
        if (!decodeSafeName(
                reinterpret_cast<const uchar *>(baBaseName.constData()),
                baBaseName.size(), false, &sBaseName) ||
            sBaseName.contains(QLatin1Char('/')) ||
            (sFullPath.section(QLatin1Char('/'), -1, -1,
                               QString::SectionSkipEmpty)
                 .compare(sBaseName, Qt::CaseInsensitive) != 0)) {
            return false;
        }

        const qint64 nDataOffset = nPosition;
        if ((nCompressedSize < 8) || (nDataOffset < 0) ||
            (nDataOffset > baData.size()) ||
            (nCompressedSize > quint64(baData.size() - nDataOffset)) ||
            (memcmp(pData + nDataOffset, RTPATCH_STREAM_PREFIX.constData(),
                    RTPATCH_STREAM_PREFIX.size()) != 0)) {
            return false;
        }
        const quint8 nModelByte = pData[nDataOffset + 7];
        if ((nModelByte != 0x81) && (nModelByte != 0x83) &&
            (nModelByte != 0x88)) {
            return false;
        }

        descriptor.sName = sFullPath;
        descriptor.nOffset = nRecordOffset;
        descriptor.nDataOffset = nDataOffset;
        descriptor.nDataSize = nCompressedSize;
        descriptor.handleMethod = XBinary::HANDLE_METHOD_RTPATCH;
        listDescriptors.append(descriptor);
        nPosition += nCompressedSize;
    }

    static const QByteArray baTrailer =
        QByteArray::fromHex("001000C00300444B4E4A");
    if ((nPosition != baData.size()) &&
        ((nPosition > baData.size() - baTrailer.size()) ||
         (nPosition + baTrailer.size() != baData.size()) ||
         (memcmp(pData + nPosition, baTrailer.constData(),
                 baTrailer.size()) != 0))) {
        return false;
    }

    *pDescriptors = listDescriptors;
    return true;
}

bool parseStringList(const QByteArray &baData, qint64 nOffset,
                     RTPatchStringList *pList)
{
    if (!pList || (nOffset < 0) || (nOffset > baData.size() - 2))
        return false;
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData());
    const quint16 nCount = qFromLittleEndian<quint16>(pData + nOffset);
    if ((nCount == 0) || (nCount > RTPATCH_MAX_LIST_ITEMS)) return false;

    qint64 nPosition = nOffset + 2;
    QList<QByteArray> listStrings;
    for (quint16 i = 0; i < nCount; ++i) {
        if (nPosition >= baData.size()) return false;
        const quint8 nLength = pData[nPosition++];
        if ((nLength == 0) || (nPosition > baData.size() - nLength) ||
            (pData[nPosition + nLength - 1] != 0)) {
            return false;
        }
        for (quint8 j = 0; j + 1 < nLength; ++j) {
            if (pData[nPosition + j] == 0) return false;
        }
        listStrings.append(QByteArray(
            baData.constData() + nPosition, nLength - 1));
        nPosition += nLength;
    }

    pList->nOffset = nOffset;
    pList->nEndOffset = nPosition;
    pList->listStrings = listStrings;
    return true;
}

bool looksLikeDirectoryList(const QList<QByteArray> &listStrings)
{
    if (listStrings.isEmpty()) return false;
    for (const QByteArray &baString : listStrings) {
        if (baString.isEmpty()) return false;
        for (char c : baString) {
            const quint8 nCharacter = static_cast<quint8>(c);
            if ((nCharacter < 0x21) || (nCharacter > 0x7e) ||
                (c == '"') || (c == '<') || (c == '>') ||
                (c == '|') || (c == '?') || (c == '*')) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace

XRTPatch::XRTPatch(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_RTPATCH)
{
}

bool XRTPatch::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRTPatch archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRTPatch::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRTPatch(pDevice);
}

bool XRTPatch::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                          PDSTRUCT *pPdStruct)
{
    QPointer<XRTPatch> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < RTPATCH_HEADER_SIZE) ||
        (nTotalSize > (std::numeric_limits<qint32>::max)()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baData = read_array_process(0, nTotalSize, pPdStruct);
    if (!guardedThis || (baData.size() != nTotalSize)) return false;
    const uchar *pData =
        reinterpret_cast<const uchar *>(baData.constData());
    const quint16 nVersion = qFromLittleEndian<quint16>(pData + 2);
    if ((pData[0] != 'K') || (pData[1] != '*') ||
        !isSupportedVersion(nVersion)) {
        return false;
    }
    if (nVersion <= 211) {
        for (qint32 i = 0x10; i < RTPATCH_HEADER_SIZE; ++i) {
            if (pData[i] != 0) return false;
        }
    }

    QList<RTPatchDescriptor> listDescriptors;
    if (nVersion == 500) {
        if (!parseVersion500(baData, &listDescriptors, MAX_RECORDS)) {
            return false;
        }
    } else {
        qint32 nSearchOffset = RTPATCH_HEADER_SIZE;
        while ((nSearchOffset >= 0) &&
               (nSearchOffset < baData.size()) &&
               (listDescriptors.size() < MAX_RECORDS) &&
               XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nStreamOffset =
                baData.indexOf(RTPATCH_STREAM_PREFIX, nSearchOffset);
            if (nStreamOffset < 0) break;
            nSearchOffset = nStreamOffset + 1;
            if ((nStreamOffset > baData.size() - 8) ||
                ((static_cast<quint8>(baData.at(nStreamOffset + 7)) &
                  0xf0) != 0x80)) {
                continue;
            }

            RTPatchDescriptor descriptor;
            qint64 nDescriptorOffset = -1;
            bool bDescriptorValid = false;
            if (nVersion <= 211) {
                nDescriptorOffset = nStreamOffset - RTPATCH_DESCRIPTOR_SIZE;
                bDescriptorValid = parseDescriptor(
                    baData, nDescriptorOffset, &descriptor);
            } else {
                // Version 4.x/6.x keeps the 8.3 descriptor, adds eight opaque
                // bytes, then places a length-prefixed long name immediately
                // before the stream.  Search backwards and prefer the nearest
                // fully validating candidate; compressed bytes can accidentally
                // mimic an earlier length byte.
                for (qint64 nNameOffset = nStreamOffset - 2;
                     nNameOffset >= qMax<qint64>(RTPATCH_AUGMENTED_PREFIX_SIZE,
                                                nStreamOffset - 256);
                     --nNameOffset) {
                    const quint8 nNameLength = pData[nNameOffset];
                    if ((nNameLength < 2) ||
                        (nNameOffset + 1 + nNameLength != nStreamOffset) ||
                        (pData[nStreamOffset - 1] != 0)) {
                        continue;
                    }
                    bool bEmbeddedNul = false;
                    for (qint64 j = nNameOffset + 1;
                         j < nStreamOffset - 1; ++j) {
                        if (pData[j] == 0) {
                            bEmbeddedNul = true;
                            break;
                        }
                    }
                    if (bEmbeddedNul) continue;

                    nDescriptorOffset =
                        nNameOffset - RTPATCH_AUGMENTED_PREFIX_SIZE;
                    RTPatchDescriptor candidate;
                    QString sLongName;
                    if (!parseDescriptor(baData, nDescriptorOffset, &candidate) ||
                        !decodeSafeName(pData + nNameOffset + 1,
                                        nNameLength - 1, false,
                                        &sLongName)) {
                        continue;
                    }
                    candidate.sName = sLongName;
                    descriptor = candidate;
                    bDescriptorValid = true;
                    break;
                }
            }

            if (!bDescriptorValid) continue;
            descriptor.nDataOffset = nStreamOffset;
            if (!classifyWholeFileStream(baData, nDescriptorOffset,
                                         nStreamOffset, &descriptor) &&
                (nVersion <= 211)) {
                // The 4.x/6.x descriptor block is reached through a long-name
                // search rather than a fixed offset, so the delta header is not
                // at a known distance there and the form is not attempted.
                if (classifyDeltaStream(baData, nDescriptorOffset,
                                        nStreamOffset, &descriptor) &&
                    !verifyDeltaProgram(baData, descriptor, pPdStruct)) {
                    // the program does need its source file after all
                    descriptor.nDataSize = -1;
                    descriptor.nProgramSize = 0;
                    descriptor.bIsDelta = false;
                    descriptor.handleMethod = HANDLE_METHOD_UNKNOWN;
                }
            }
            listDescriptors.append(descriptor);
        }
    }

    if (listDescriptors.isEmpty() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    bool bHasSourceFreeRecord = false;

    for (const RTPatchDescriptor &descriptor : listDescriptors) {
        if (descriptor.handleMethod != HANDLE_METHOD_UNKNOWN) {
            bHasSourceFreeRecord = true;
            break;
        }
    }

    // Generation-1 packages may start with a directory table, a banner, or
    // both.  Both use the same counted-string encoding.  Directory strings
    // are printable non-space paths; the corpus banner classifier below is
    // exact for all 26 independently verified the reference implementation Comments.txt members.
    qint64 nListOffset = RTPATCH_HEADER_SIZE;
    for (qint32 i = 0; i < 2; ++i) {
        RTPatchStringList stringList;
        if ((nVersion > 211) ||
            !parseStringList(baData, nListOffset, &stringList) ||
            (stringList.nEndOffset > listDescriptors.constFirst().nOffset)) {
            break;
        }
        nListOffset = stringList.nEndOffset;
        if (looksLikeDirectoryList(stringList.listStrings)) continue;

        qint64 nDecodedSize = 0;
        for (const QByteArray &baLine : stringList.listStrings) {
            if (nDecodedSize > (std::numeric_limits<qint64>::max)() -
                                   baLine.size() - 2) {
                return false;
            }
            nDecodedSize += baLine.size() + 2;
        }
        QString sUniqueName;
        if (!makeUniquePath(QStringLiteral("Comments.txt"), &stUsedFiles,
                            &stUsedDirectories, &mapNextSuffixes,
                            &mapResolvedDirectories, &sUniqueName)) {
            return false;
        }
        bHasSourceFreeRecord = true;
        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = stringList.nOffset;
            entry.nHeaderSize = 2;
            entry.nDataOffset = stringList.nOffset;
            entry.nDataSize =
                stringList.nEndOffset - stringList.nOffset;
            entry.nUncompressedSize = nDecodedSize;
            entry.handleMethod = HANDLE_METHOD_RTPATCH_TEXT;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        break;
    }

    for (qint32 i = 0; i < listDescriptors.size(); ++i) {
        const RTPatchDescriptor &descriptor = listDescriptors.at(i);
        const qint64 nDataEnd = (i + 1 < listDescriptors.size())
            ? listDescriptors.at(i + 1).nOffset : nTotalSize;
        const qint64 nDataSize = (descriptor.nDataSize >= 0)
            ? descriptor.nDataSize : nDataEnd - descriptor.nDataOffset;
        if ((descriptor.nOffset < RTPATCH_HEADER_SIZE) ||
            (descriptor.nDataOffset < descriptor.nOffset) ||
            (nDataSize <= 0) ||
            !rangeWithin(nTotalSize, descriptor.nDataOffset,
                         nDataSize)) {
            return false;
        }

        // A patch operation is not an archive member: its byte stream is an
        // opcode program that needs a checksum-matched source file.  Mixed
        // packages therefore expose only their independently decodable banner
        // and whole-file records, matching archive-tool semantics without
        // manufacturing patched targets.  A delta-only package deliberately
        // retains its unsupported records so an extraction attempt fails
        // closed instead of reporting an empty success.
        if (bHasSourceFreeRecord &&
            (descriptor.handleMethod == HANDLE_METHOD_UNKNOWN)) {
            continue;
        }

        QString sUniqueName;
        if (!makeUniquePath(descriptor.sName, &stUsedFiles,
                            &stUsedDirectories, &mapNextSuffixes,
                            &mapResolvedDirectories, &sUniqueName)) {
            return false;
        }
        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = descriptor.nOffset;
            entry.nHeaderSize =
                descriptor.nDataOffset - descriptor.nOffset;
            entry.nDataOffset = descriptor.nDataOffset;
            entry.nDataSize = nDataSize;
            entry.nUncompressedSize = descriptor.nUncompressedSize;
            entry.handleMethod = descriptor.handleMethod;
            entry.mtDateTime = descriptor.mtDateTime;
            entry.sFileName = sUniqueName;
            if (descriptor.bIsDelta) {
                // The decoded stream is the opcode program; the member is the
                // window inside it that the fill opcode wrote.  The shared
                // sub-stream path decodes the block once and emits that window.
                entry.bIsSolid = true;
                entry.nSubstreamOffset = RTPATCH_DELTA_PAYLOAD_OFFSET;
                entry.nStreamUnpackedSize = descriptor.nProgramSize;
                entry.nSolidFolderIndex = i;
            }
            pEntries->append(entry);
        }
    }

    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
