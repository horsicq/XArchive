/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free reader for InstallShield 7.x "Setup Player 2K2"
 * self-extractors (called IS14 by the reference corpus).
 * MIT License
 */
#include "xis14sfxarchive.h"

#include <QHash>
#include <QPointer>
#include <QSet>

namespace {
const qint64 IS14_MAX_CSTRING = 260;
const qint64 IS14_MAX_SECTIONS = 96;

bool isDottedVersion(const QByteArray &baValue)
{
    if (baValue.isEmpty()) return false;

    qint32 nParts = 1;
    bool bHaveDigit = false;
    for (char cValue : baValue) {
        if (cValue >= '0' && cValue <= '9') {
            bHaveDigit = true;
        } else if (cValue == '.' && bHaveDigit && nParts < 4) {
            ++nParts;
            bHaveDigit = false;
        } else {
            return false;
        }
    }

    return bHaveDigit;
}

bool isSafeRelativePath(const QString &sPath)
{
    if (sPath.isEmpty() || sPath.startsWith(QLatin1Char('/')) ||
        sPath.contains(QLatin1Char(':'))) {
        return false;
    }

    const QStringList listParts = sPath.split(QLatin1Char('/'));
    if (listParts.isEmpty()) return false;
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || sPart == QStringLiteral(".") ||
            sPart == QStringLiteral("..")) {
            return false;
        }
    }

    return true;
}
}  // namespace

XIS14SFXArchive::XIS14SFXArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_IS14_SFX)
{
}

bool XIS14SFXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIS14SFXArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIS14SFXArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                         XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIS14SFXArchive(pDevice);
}

bool XIS14SFXArchive::readCString(qint64 nTotalSize, qint64 *pPosition,
                                  QByteArray *pValue,
                                  PDSTRUCT *pPdStruct)
{
    QPointer<XIS14SFXArchive> guardedThis(this);
    if (!pPosition || !pValue || !guardedThis ||
        !rangeWithin(nTotalSize, *pPosition, 1)) {
        return false;
    }
    const qint64 nProbeSize = qMin(IS14_MAX_CSTRING + 1,
                                   nTotalSize - *pPosition);
    const QByteArray baProbe = read_array_process(
        *pPosition, nProbeSize, pPdStruct);
    if (!guardedThis || (baProbe.size() != nProbeSize)) return false;
    const qint32 nTerminator = baProbe.indexOf('\0');
    if ((nTerminator < 1) || (nTerminator > IS14_MAX_CSTRING)) {
        return false;
    }
    for (qint32 i = 0; i < nTerminator; ++i) {
        const uchar nValue = static_cast<uchar>(baProbe.at(i));
        if ((nValue < 0x20) || (nValue > 0x7e)) return false;
    }
    *pValue = baProbe.left(nTerminator);
    *pPosition += nTerminator + 1;
    return true;
}

bool XIS14SFXArchive::scanFormat(QList<ENTRY> *pEntries,
                                 qint64 *pArchiveEnd,
                                 PDSTRUCT *pPdStruct)
{
    QPointer<XIS14SFXArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < 0x40 ||
        !isPdStructNotCanceled(pPdStruct) || read_uint16(0) != 0x5a4dU) {
        return false;
    }

    const qint64 nPEOffset = read_uint32(0x3c);
    if (!rangeWithin(nTotalSize, nPEOffset, 24) ||
        read_uint32(nPEOffset) != 0x00004550U) {
        return false;
    }

    const qint64 nSections = read_uint16(nPEOffset + 6);
    const qint64 nOptionalSize = read_uint16(nPEOffset + 20);
    const qint64 nOptionalOffset = nPEOffset + 24;
    if (nSections < 1 || nSections > IS14_MAX_SECTIONS ||
        nOptionalSize < 2 ||
        !rangeWithin(nTotalSize, nOptionalOffset, nOptionalSize)) {
        return false;
    }
    const quint16 nOptionalMagic = read_uint16(nOptionalOffset);
    if (nOptionalMagic != 0x010bU && nOptionalMagic != 0x020bU) {
        return false;
    }

    const qint64 nSectionTable = nOptionalOffset + nOptionalSize;
    if (!rangeWithin(nTotalSize, nSectionTable, nSections * 40)) return false;

    qint64 nOverlayOffset = 0;
    for (qint64 i = 0; i < nSections; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nSectionOffset = nSectionTable + i * 40;
        const qint64 nRawSize = read_uint32(nSectionOffset + 16);
        const qint64 nRawOffset = read_uint32(nSectionOffset + 20);
        if (nRawSize == 0) continue;
        if (!rangeWithin(nTotalSize, nRawOffset, nRawSize)) return false;
        nOverlayOffset = qMax(nOverlayOffset, nRawOffset + nRawSize);
    }
    if (nOverlayOffset <= 0 || nOverlayOffset >= nTotalSize ||
        nTotalSize - nOverlayOffset < 16) {
        return false;
    }

    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    qint64 nPosition = nOverlayOffset;

    while (nPosition < nTotalSize && entries.size() < MAX_RECORDS &&
           isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRecordOffset = nPosition;
        QByteArray baBaseName;
        QByteArray baPath;
        QByteArray baVersion;
        QByteArray baSize;
        if (!readCString(nTotalSize, &nPosition, &baBaseName, pPdStruct) ||
            !readCString(nTotalSize, &nPosition, &baPath, pPdStruct) ||
            !readCString(nTotalSize, &nPosition, &baVersion, pPdStruct) ||
            !readCString(nTotalSize, &nPosition, &baSize, pPdStruct) ||
            !isDottedVersion(baVersion) || baSize.size() > 12) {
            return false;
        }

        for (char cValue : baSize) {
            if (cValue < '0' || cValue > '9') return false;
        }
        bool bSizeValid = false;
        const qulonglong nUnsignedSize = baSize.toULongLong(&bSizeValid, 10);
        if (!bSizeValid ||
            nUnsignedSize > static_cast<qulonglong>(nTotalSize - nPosition)) {
            return false;
        }
        const qint64 nDataSize = static_cast<qint64>(nUnsignedSize);

        const QString sBaseName = QString::fromLatin1(baBaseName);
        QString sPath = QString::fromLatin1(baPath);
        sPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (sBaseName.contains(QLatin1Char('/')) ||
            sBaseName.contains(QLatin1Char('\\')) ||
            !isSafeRelativePath(sPath) ||
            sPath.section(QLatin1Char('/'), -1).compare(
                sBaseName, Qt::CaseInsensitive) != 0) {
            return false;
        }

        QString sUniquePath;
        if (!makeUniquePath(sPath, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniquePath)) {
            return false;
        }

        ENTRY entry = {};
        entry.nHeaderOffset = nRecordOffset;
        entry.nHeaderSize = nPosition - nRecordOffset;
        entry.nDataOffset = nPosition;
        entry.nDataSize = nDataSize;
        entry.nUncompressedSize = nDataSize;
        entry.handleMethod = HANDLE_METHOD_STORE;
        entry.sFileName = sUniquePath;
        entries.append(entry);

        nPosition += nDataSize;
    }

    if (!guardedThis || !isPdStructNotCanceled(pPdStruct) ||
        entries.isEmpty() || nPosition != nTotalSize) {
        return false;
    }

    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
