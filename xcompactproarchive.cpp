/*
 * Compact Pro catalog reader. The format description and decompression
 * behavior were translated from XADMaster (LGPL 2.1 or later); see
 * Algos/xadmaster/COPYING.
 */
#include "xcompactproarchive.h"

#include <QtEndian>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QTextCodec>

#include <functional>
#include <limits>

namespace {
const qint64 MAX_COMPACT_PRO_PARSE_SIZE = Q_INT64_C(512) * 1024 * 1024;

quint32 catalogCrc(const char *pData, qint64 nSize)
{
    quint32 nCrc = 0xffffffffU;
    for (qint64 i = 0; i < nSize; ++i) {
        nCrc ^= quint8(pData[i]);
        for (qint32 j = 0; j < 8; ++j)
            nCrc = (nCrc >> 1) ^ ((nCrc & 1U) ? 0xedb88320U : 0U);
    }
    return nCrc;
}

QString decodeMacName(const QByteArray &baName)
{
    QTextCodec *pCodec = QTextCodec::codecForName("macintosh");
    QString sName = pCodec ? pCodec->toUnicode(baName)
                           : QString::fromLatin1(baName);
    sName.replace(QLatin1Char('/'), QLatin1Char('_'));
    sName.replace(QLatin1Char(':'), QLatin1Char('_'));
    sName = sName.normalized(QString::NormalizationForm_C).trimmed();
    if (sName.isEmpty() || (sName == QLatin1String(".")) ||
        (sName == QLatin1String(".."))) return QString();
    return XBinary::fixFileName(sName);
}
}  // namespace

struct XCompactProArchive::PARSE_CONTEXT
{
    QPointer<XCompactProArchive> guardedArchive;
    qint64 nTotalSize;
    const QByteArray *pDataArray;
    const uchar *pData;
    PDSTRUCT *pPdStruct;
    qint64 *pPosition;
    qint32 *pParsedRecords;
    QSet<QString> *pUsedFiles;
    QSet<QString> *pUsedDirectories;
    QHash<QString, qint32> *pNextSuffixes;
    QHash<QString, QString> *pResolvedDirectories;
    QList<ENTRY> *pEntries;
};

XCompactProArchive::XCompactProArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_COMPACT_PRO)
{
}

bool XCompactProArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCompactProArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCompactProArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                            XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCompactProArchive(pDevice);
}

bool XCompactProArchive::appendFork(PARSE_CONTEXT *pContext,
                                    const QString &sPath,
                                    qint64 nRecordOffset,
                                    quint32 nStoredCrc,
                                    qint32 nForkCount, bool bResource,
                                    qint64 nOffset, qint64 nPacked,
                                    qint64 nRaw, bool bLzh)
{
    if (!pContext || !pContext->guardedArchive || !pContext->pEntries ||
        !pContext->pPosition || pContext->pEntries->count() >= MAX_RECORDS)
        return false;
    QString sOutputPath = sPath;
    if (bResource) sOutputPath += QStringLiteral(".rsrc");
    QString sUniquePath;
    if (!makeUniquePath(sOutputPath, pContext->pUsedFiles,
                        pContext->pUsedDirectories,
                        pContext->pNextSuffixes,
                        pContext->pResolvedDirectories, &sUniquePath))
        return false;
    ENTRY entry = {};
    entry.nHeaderOffset = nRecordOffset;
    entry.nHeaderSize = *pContext->pPosition - nRecordOffset;
    entry.nDataOffset = nOffset;
    entry.nDataSize = nPacked;
    entry.nUncompressedSize = nRaw;
    entry.handleMethod = nRaw == 0
        ? HANDLE_METHOD_STORE
        : (bLzh ? HANDLE_METHOD_COMPACT_PRO_LZH
                : HANDLE_METHOD_COMPACT_PRO_RLE);
    entry.sFileName = sUniquePath;
    if (nForkCount == 1) {
        entry.bCRC32Defined = true;
        entry.nCRC32 = ~nStoredCrc;
    }
    pContext->pEntries->append(entry);
    return true;
}

bool XCompactProArchive::parseDirectory(PARSE_CONTEXT *pContext,
                                        const QString &sParent,
                                        qint32 nRecords, qint32 nDepth)
{
    if (!pContext || !pContext->pPosition || !pContext->pParsedRecords ||
        !pContext->pDataArray || !pContext->pEntries || nRecords < 0 ||
        nDepth > 128 || *pContext->pParsedRecords > MAX_RECORDS - nRecords)
        return false;
    qint32 nRemaining = nRecords;
    while (nRemaining > 0) {
        if (!pContext->guardedArchive ||
            !isPdStructNotCanceled(pContext->pPdStruct) ||
            !rangeWithin(pContext->nTotalSize, *pContext->pPosition, 1))
            return false;
        const qint64 nRecordOffset = *pContext->pPosition;
        const quint8 nNameField = pContext->pData[(*pContext->pPosition)++];
        const qint32 nNameSize = nNameField & 0x7f;
        if (nNameSize <= 0 ||
            !rangeWithin(pContext->nTotalSize, *pContext->pPosition, nNameSize))
            return false;
        const QString sComponent = decodeMacName(
            pContext->pDataArray->mid(*pContext->pPosition, nNameSize));
        *pContext->pPosition += nNameSize;
        if (sComponent.isEmpty()) return false;
        const QString sPath = sParent.isEmpty()
                                  ? sComponent
                                  : sParent + QLatin1Char('/') + sComponent;

        if (nNameField & 0x80) {
            if (!rangeWithin(pContext->nTotalSize, *pContext->pPosition, 2))
                return false;
            const qint32 nChildren = qFromBigEndian<quint16>(
                pContext->pData + *pContext->pPosition);
            *pContext->pPosition += 2;
            if (nChildren <= 0 || nChildren >= nRemaining) return false;
            ++*pContext->pParsedRecords;
            if (!parseDirectory(pContext, sPath, nChildren, nDepth + 1))
                return false;
            nRemaining -= nChildren + 1;
            continue;
        }

        if (!rangeWithin(pContext->nTotalSize, *pContext->pPosition, 45))
            return false;
        const uchar *pMeta = pContext->pData + *pContext->pPosition;
        const qint64 nFileOffset = qFromBigEndian<quint32>(pMeta + 1);
        const quint32 nStoredCrc = qFromBigEndian<quint32>(pMeta + 23);
        const quint16 nFlags = qFromBigEndian<quint16>(pMeta + 27);
        const qint64 nResourceSize = qFromBigEndian<quint32>(pMeta + 29);
        const qint64 nDataSize = qFromBigEndian<quint32>(pMeta + 33);
        const qint64 nResourcePacked = qFromBigEndian<quint32>(pMeta + 37);
        const qint64 nDataPacked = qFromBigEndian<quint32>(pMeta + 41);
        *pContext->pPosition += 45;
        ++*pContext->pParsedRecords;
        --nRemaining;

        if ((nFlags & 1) ||
            !rangeWithin(pContext->nTotalSize, nFileOffset, nResourcePacked) ||
            !rangeWithin(pContext->nTotalSize,
                         nFileOffset + nResourcePacked, nDataPacked))
            return false;

        const qint32 nForkCount = (nResourceSize ? 1 : 0) +
                                  ((nDataSize || !nResourceSize) ? 1 : 0);
        if (nResourceSize &&
            !appendFork(pContext, sPath, nRecordOffset, nStoredCrc,
                        nForkCount, true, nFileOffset, nResourcePacked,
                        nResourceSize, nFlags & 2))
            return false;
        if ((nDataSize || !nResourceSize) &&
            !appendFork(pContext, sPath, nRecordOffset, nStoredCrc,
                        nForkCount, false, nFileOffset + nResourcePacked,
                        nDataPacked, nDataSize, nFlags & 4))
            return false;
    }
    return true;
}

bool XCompactProArchive::scanFormat(QList<ENTRY> *pEntries,
                                    qint64 *pArchiveEnd,
                                    PDSTRUCT *pPdStruct)
{
    QPointer<XCompactProArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < 15 ||
        nTotalSize > MAX_COMPACT_PRO_PARSE_SIZE ||
        nTotalSize > (std::numeric_limits<int>::max)() ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baData = read_array_process(0, nTotalSize, pPdStruct);
    if (!guardedThis || baData.size() != nTotalSize ||
        quint8(baData.at(0)) != 1) return false;
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData());
    const qint64 nCatalogOffset = qFromBigEndian<quint32>(pData + 4);
    if (!rangeWithin(nTotalSize, nCatalogOffset, 7)) return false;

    const quint32 nStoredCatalogCrc =
        qFromBigEndian<quint32>(pData + nCatalogOffset);
    qint64 nPosition = nCatalogOffset + 4;
    const qint32 nRootRecords = qFromBigEndian<quint16>(pData + nPosition);
    const qint32 nCommentSize = pData[nPosition + 2];
    nPosition += 3;
    if (nRootRecords <= 0 || nRootRecords > MAX_RECORDS ||
        !rangeWithin(nTotalSize, nPosition, nCommentSize)) return false;
    nPosition += nCommentSize;

    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    qint32 nParsedRecords = 0;
    QList<ENTRY> entries;
    PARSE_CONTEXT parseContext = {guardedThis, nTotalSize, &baData, pData,
                                  pPdStruct, &nPosition, &nParsedRecords,
                                  &usedFiles, &usedDirectories, &nextSuffixes,
                                  &resolvedDirectories, &entries};

    if (!parseDirectory(&parseContext, QString(), nRootRecords, 0) ||
        nParsedRecords != nRootRecords || entries.isEmpty() ||
        nPosition <= nCatalogOffset + 4 ||
        catalogCrc(baData.constData() + nCatalogOffset + 4,
                   nPosition - nCatalogOffset - 4) != nStoredCatalogCrc ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
