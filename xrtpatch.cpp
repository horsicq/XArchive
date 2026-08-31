/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free Pocket Soft RTPatch reader.
 * MIT License
 */
#include "xrtpatch.h"

#include <QtEndian>

#include <limits>

namespace {
const qint32 RTPATCH_HEADER_SIZE = 0x1a;
const qint32 RTPATCH_DESCRIPTOR_SIZE = 34;
const qint32 RTPATCH_AUGMENTED_PREFIX_SIZE = 42;
const qint32 RTPATCH_MAX_LIST_ITEMS = 4096;
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
            classifyWholeFileStream(baData, nDescriptorOffset,
                                    nStreamOffset, &descriptor);
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

    // Generation-1 packages may start with a directory table, a banner, or
    // both.  Both use the same counted-string encoding.  Directory strings
    // are printable non-space paths; the corpus banner classifier below is
    // exact for all 26 independently verified U3 Comments.txt members.
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
            pEntries->append(entry);
        }
    }

    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
