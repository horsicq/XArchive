/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native reader for the documented PyInstaller CArchive cookie and TOC.
 * MIT License
 */
#include "xpyinstallercarchive.h"

#include <QtEndian>
#include <QHash>
#include <QPointer>
#include <QSet>

#include <limits>

namespace {
const char PYINSTALLER_COOKIE_MAGIC[] = {'M', 'E', 'I', '\x0c', '\x0b',
                                         '\x0a', '\x0b', '\x0e'};
const qint64 PYINSTALLER_COOKIE_V20_SIZE = 24;
const qint64 PYINSTALLER_COOKIE_V21_SIZE = 88;
const qint64 PYINSTALLER_COOKIE_SEARCH_SIZE = Q_INT64_C(1024) * 1024;
}

XPyInstallerCArchive::XPyInstallerCArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_PYINSTALLER_SFX)
{
}

bool XPyInstallerCArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPyInstallerCArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPyInstallerCArchive::createInstance(QIODevice *pDevice,
                                              bool bIsImage,
                                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPyInstallerCArchive(pDevice);
}

bool XPyInstallerCArchive::scanFormat(QList<ENTRY> *pEntries,
                                      qint64 *pArchiveEnd,
                                      PDSTRUCT *pPdStruct)
{
    QPointer<XPyInstallerCArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < PYINSTALLER_COOKIE_V20_SIZE + 18 ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nSearchSize = qMin(nTotalSize, PYINSTALLER_COOKIE_SEARCH_SIZE);
    const qint64 nSearchOffset = nTotalSize - nSearchSize;
    const QByteArray baTail = read_array_process(nSearchOffset, nSearchSize,
                                                 pPdStruct);
    if (!guardedThis || baTail.size() != nSearchSize) return false;
    const qint32 nRelativeCookie = baTail.lastIndexOf(
        QByteArray(PYINSTALLER_COOKIE_MAGIC, 8));
    if (nRelativeCookie < 0) return false;
    const qint64 nCookieOffset = nSearchOffset + nRelativeCookie;
    if (!rangeWithin(nTotalSize, nCookieOffset,
                     PYINSTALLER_COOKIE_V20_SIZE)) return false;
    const uchar *pCookie = reinterpret_cast<const uchar *>(
        baTail.constData() + nRelativeCookie);
    const qint64 nPackageLength = qFromBigEndian<quint32>(pCookie + 8);
    const qint64 nTocRelative = qFromBigEndian<quint32>(pCookie + 12);
    const qint64 nTocSize = qFromBigEndian<quint32>(pCookie + 16);

    qint64 nCookieSize = 0;
    qint64 nArchiveStart = -1;
    qint64 nTocOffset = -1;
    for (const qint64 nCandidateCookieSize :
         {PYINSTALLER_COOKIE_V21_SIZE, PYINSTALLER_COOKIE_V20_SIZE}) {
        if (!rangeWithin(nTotalSize, nCookieOffset, nCandidateCookieSize) ||
            nPackageLength < nCandidateCookieSize ||
            nPackageLength > nCookieOffset + nCandidateCookieSize)
            continue;
        const qint64 nCandidateStart =
            nCookieOffset + nCandidateCookieSize - nPackageLength;
        const qint64 nCandidateToc = nCandidateStart + nTocRelative;
        if (nCandidateStart < 0 || nCandidateToc < nCandidateStart ||
            !rangeWithin(nCookieOffset, nCandidateToc, nTocSize) ||
            nCandidateToc + nTocSize != nCookieOffset) continue;
        nCookieSize = nCandidateCookieSize;
        nArchiveStart = nCandidateStart;
        nTocOffset = nCandidateToc;
        break;
    }
    if (!nCookieSize || nTocSize < 18 || nCookieOffset + nCookieSize != nTotalSize)
        return false;

    const QByteArray baToc = read_array_process(nTocOffset, nTocSize,
                                                pPdStruct);
    if (!guardedThis || baToc.size() != nTocSize) return false;
    const uchar *pToc = reinterpret_cast<const uchar *>(baToc.constData());
    qint64 nPosition = 0;
    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;

    while (nPosition < nTocSize) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            nPosition > nTocSize - 18 || entries.count() >= MAX_RECORDS)
            return false;
        const qint64 nEntrySize = qFromBigEndian<quint32>(pToc + nPosition);
        if (nEntrySize < 18 || nEntrySize > nTocSize - nPosition)
            return false;
        const qint64 nDataRelative =
            qFromBigEndian<quint32>(pToc + nPosition + 4);
        const qint64 nPackedSize =
            qFromBigEndian<quint32>(pToc + nPosition + 8);
        const qint64 nRawSize =
            qFromBigEndian<quint32>(pToc + nPosition + 12);
        const quint8 nCompressed = pToc[nPosition + 16];
        const char cType = char(pToc[nPosition + 17]);
        const qint64 nNameFieldSize = nEntrySize - 18;
        const char *pName = reinterpret_cast<const char *>(pToc + nPosition + 18);
        qint64 nNameSize = 0;
        while (nNameSize < nNameFieldSize && pName[nNameSize]) ++nNameSize;
        if (nNameSize == nNameFieldSize || nCompressed > 1 ||
            nDataRelative < 0 ||
            nDataRelative > nTocRelative ||
            nPackedSize > nTocRelative - nDataRelative ||
            (nCompressed == 0 && nPackedSize != nRawSize)) return false;

        QString sName = QString::fromUtf8(pName, qint32(nNameSize));
        sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (sName.isEmpty())
            sName = QStringLiteral("entry_%1_%2.bin")
                        .arg(entries.count(), 5, 10, QLatin1Char('0'))
                        .arg(QChar::fromLatin1(cType));
        sName = XBinary::fixFileName(sName);
        QString sUniqueName;
        if (sName.isEmpty() ||
            !makeUniquePath(sName, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniqueName)) return false;

        ENTRY entry = {};
        entry.nHeaderOffset = nTocOffset + nPosition;
        entry.nHeaderSize = nEntrySize;
        entry.nDataOffset = nArchiveStart + nDataRelative;
        entry.nDataSize = nPackedSize;
        entry.nUncompressedSize = nRawSize;
        entry.handleMethod = nCompressed ? HANDLE_METHOD_ZLIB
                                         : HANDLE_METHOD_STORE;
        entry.sFileName = sUniqueName;
        entries.append(entry);
        nPosition += nEntrySize;
    }

    if (entries.isEmpty() || nPosition != nTocSize) return false;
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
