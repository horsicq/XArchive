/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xckp.h"

#include <QHash>
#include <QSet>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <new>

namespace {
const qint64 CKPEDP_HEADER_SIZE = 14;
const qint64 CKPEDP_FIXED_RECORD_SIZE = 22;
const quint32 CKPEDP_MAX_RECORDS = 100000;

class CKPEDPDevicePositionGuard {
public:
    explicit CKPEDPDevicePositionGuard(QIODevice *pDevice)
        : m_pDevice(pDevice), m_nPosition(-1), m_bRestored(false)
    {
        if (m_pDevice && !m_pDevice->isSequential()) {
            m_nPosition = m_pDevice->pos();
        }
    }

    ~CKPEDPDevicePositionGuard()
    {
        restore();
    }

    bool isValid() const
    {
        return m_pDevice && (m_nPosition >= 0);
    }

    bool restore()
    {
        if (m_bRestored) return true;
        m_bRestored = true;
        if (!m_pDevice || (m_nPosition < 0)) return false;
        if (!m_pDevice->seek(m_nPosition)) return false;
        return m_pDevice && (m_pDevice->pos() == m_nPosition);
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

quint16 ckpEdpReadLE16(const uchar *pData)
{
    return qFromLittleEndian<quint16>(pData);
}

quint32 ckpEdpReadLE32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

quint64 ckpEdpReadLE64(const uchar *pData)
{
    return qFromLittleEndian<quint64>(pData);
}

bool ckpEdpRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool ckpEdpDecodeName(const uchar *pData, quint16 nCharacterCount,
                      bool bUtf16, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pData || !pResult || !nCharacterCount) return false;

    QString sName;
    sName.reserve(nCharacterCount);
    for (quint32 i = 0; i < nCharacterCount; ++i) {
        quint32 nCharacter = bUtf16
            ? ckpEdpReadLE16(pData + ((qint64)i * 2))
            : pData[i];
        nCharacter = (~nCharacter) & (bUtf16 ? 0xffffu : 0xffu);
        if (nCharacter < 0x40) nCharacter += 0x20;
        sName.append(QChar((ushort)nCharacter));
    }

    // The archive names are Windows-style.  fixFileName makes them relative,
    // removes traversal semantics, handles reserved device names and bounds
    // every component while preserving readable Unicode.
    sName = XBinary::fixFileName(sName);
    if (sName.isEmpty()) return false;
    *pResult = sName;
    return true;
}

QString ckpEdpAppendDuplicateSuffix(const QString &sComponent,
                                    qint32 nSuffix)
{
    const qint32 nDot = sComponent.lastIndexOf(QLatin1Char('.'));
    const QString sSuffix = QStringLiteral("_%1").arg(nSuffix);
    if (nDot > 0) {
        return sComponent.left(nDot) + sSuffix + sComponent.mid(nDot);
    }
    return sComponent + sSuffix;
}

bool ckpEdpMakeUniquePath(const QString &sSource,
                          QSet<QString> *pUsedFiles,
                          QSet<QString> *pUsedDirectories,
                          QHash<QString, qint32> *pNextSuffixes,
                          QHash<QString, QString> *pResolvedDirectories,
                          QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pUsedFiles || !pUsedDirectories || !pNextSuffixes ||
        !pResolvedDirectories || !pResult || sSource.isEmpty()) {
        return false;
    }

    const QStringList listSource =
        sSource.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    if (listSource.isEmpty()) return false;

    QStringList listResolved;
    for (qint32 i = 0; i < listSource.count(); ++i) {
        const bool bLeaf = (i + 1 == listSource.count());
        const QString sOriginalComponent = listSource.at(i);
        if (sOriginalComponent.isEmpty()) return false;

        const QString sOriginalPath = listResolved.isEmpty()
            ? sOriginalComponent
            : listResolved.join(QLatin1Char('/')) + QLatin1Char('/') +
                  sOriginalComponent;
        const QString sOriginalKey = sOriginalPath.toCaseFolded();

        if (!bLeaf && pResolvedDirectories->contains(sOriginalKey)) {
            const QString sResolvedPath =
                pResolvedDirectories->value(sOriginalKey);
            const QString sResolvedKey = sResolvedPath.toCaseFolded();
            const qint32 nSlash =
                sResolvedPath.lastIndexOf(QLatin1Char('/'));
            const QString sResolvedComponent = sResolvedPath.mid(nSlash + 1);
            if (sResolvedComponent.isEmpty() ||
                pUsedFiles->contains(sResolvedKey) ||
                !pUsedDirectories->contains(sResolvedKey)) {
                return false;
            }
            listResolved.append(sResolvedComponent);
            continue;
        }

        qint32 nSuffix = 1;
        QString sComponent;
        QString sPath;
        QString sKey;
        bool bFound = false;
        while (nSuffix <= ((qint32)CKPEDP_MAX_RECORDS + 1)) {
            sComponent = (nSuffix == 1)
                ? sOriginalComponent
                : ckpEdpAppendDuplicateSuffix(sOriginalComponent, nSuffix);
            sPath = listResolved.isEmpty()
                ? sComponent
                : listResolved.join(QLatin1Char('/')) + QLatin1Char('/') +
                      sComponent;
            sKey = sPath.toCaseFolded();
            const bool bCollision = pUsedFiles->contains(sKey) ||
                (bLeaf && pUsedDirectories->contains(sKey));
            if (!bCollision && (XBinary::fixFileName(sPath) == sPath)) {
                bFound = true;
                break;
            }
            nSuffix = (nSuffix == 1)
                ? pNextSuffixes->value(sOriginalKey, 2)
                : nSuffix + 1;
        }
        if (!bFound) return false;

        pNextSuffixes->insert(
            sOriginalKey,
            (nSuffix <= (qint32)CKPEDP_MAX_RECORDS)
                ? qMax(2, nSuffix + 1)
                : ((qint32)CKPEDP_MAX_RECORDS + 1));
        listResolved.append(sComponent);
        if (bLeaf) {
            pUsedFiles->insert(sKey);
        } else {
            pUsedDirectories->insert(sKey);
            pResolvedDirectories->insert(sOriginalKey, sPath);
        }
    }

    *pResult = listResolved.join(QLatin1Char('/'));
    return !pResult->isEmpty();
}
}  // namespace

XCKPEDPBase::XCKPEDPBase(QIODevice *pDevice, FT fileType)
    : XArchive(pDevice), m_fileTypeHint(fileType)
{
    setFileType(fileType);
}

bool XCKPEDPBase::isSupportedFileType(FT fileType)
{
    return (fileType == FT_CKP) || (fileType == FT_EDP);
}

bool XCKPEDPBase::isValid(PDSTRUCT *pPdStruct)
{
    return scanArchive(nullptr, nullptr, pPdStruct);
}

bool XCKPEDPBase::scanArchive(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                              PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pArchiveEnd) *pArchiveEnd = 0;

    QPointer<XCKPEDPBase> guardedThis(this);
    const FT fileType = getFileType();
    if (!isSupportedFileType(fileType) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CKPEDPDevicePositionGuard positionGuard(getDevice());
    if (!guardedThis || !positionGuard.isValid()) return false;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < CKPEDP_HEADER_SIZE)) return false;

    const QByteArray baHeader =
        read_array_process(0, CKPEDP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || (baHeader.size() != CKPEDP_HEADER_SIZE)) return false;

    const char *pExpectedSignature =
        (fileType == FT_CKP) ? ".CKP" : ".EDP";
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if ((memcmp(pHeader, pExpectedSignature, 4) != 0) ||
        (pHeader[4] != 0) || (pHeader[5] != 1)) {
        return false;
    }

    const quint32 nRecordCount = ckpEdpReadLE32(pHeader + 6);
    const bool bUtf16 = (fileType == FT_EDP);
    const qint64 nMinimumRecordSize = CKPEDP_FIXED_RECORD_SIZE +
                                      (bUtf16 ? 2 : 1);
    if ((nRecordCount > CKPEDP_MAX_RECORDS) ||
        ((qint64)nRecordCount >
         ((nTotalSize - CKPEDP_HEADER_SIZE) / nMinimumRecordSize))) {
        return false;
    }
    if ((nRecordCount == 0) && (nTotalSize != CKPEDP_HEADER_SIZE)) {
        return false;
    }

    QList<ENTRY> listEntries;
    listEntries.reserve((qint32)nRecordCount);
    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    qint64 nCursor = CKPEDP_HEADER_SIZE;

    for (quint32 i = 0; i < nRecordCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            !ckpEdpRangeWithin(nTotalSize, nCursor, 2)) {
            return false;
        }

        const QByteArray baLength = read_array_process(nCursor, 2, pPdStruct);
        if (!guardedThis || (baLength.size() != 2)) return false;
        const quint16 nCharacterCount = ckpEdpReadLE16(
            reinterpret_cast<const uchar *>(baLength.constData()));
        if (!nCharacterCount) return false;

        const qint64 nNameSize =
            (qint64)nCharacterCount * (bUtf16 ? 2 : 1);
        const qint64 nRecordSize = CKPEDP_FIXED_RECORD_SIZE + nNameSize;
        if (!ckpEdpRangeWithin(nTotalSize, nCursor, nRecordSize)) {
            return false;
        }

        const QByteArray baRecord =
            read_array_process(nCursor, nRecordSize, pPdStruct);
        if (!guardedThis || (baRecord.size() != nRecordSize)) return false;
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());
        const uchar *pTail = pRecord + 2 + nNameSize;

        QString sFileName;
        QString sUniqueName;
        if (!ckpEdpDecodeName(pRecord + 2, nCharacterCount, bUtf16,
                              &sFileName) ||
            !ckpEdpMakeUniquePath(sFileName, &stUsedFiles,
                                  &stUsedDirectories, &mapNextSuffixes,
                                  &mapResolvedDirectories, &sUniqueName) ||
            (ckpEdpReadLE32(pTail + 8) != 0)) {
            return false;
        }

        ENTRY entry = {};
        entry.nHeaderOffset = nCursor;
        entry.nHeaderSize = nRecordSize;
        entry.nDataOffset = ckpEdpReadLE32(pTail + 12);
        entry.nDataSize = ckpEdpReadLE32(pTail + 16);
        entry.nResourceId = ckpEdpReadLE64(pTail);
        entry.sFileName = sUniqueName;
        if (!ckpEdpRangeWithin(nTotalSize, entry.nDataOffset,
                               entry.nDataSize)) {
            return false;
        }
        listEntries.append(entry);
        nCursor += nRecordSize;
    }

    // Entries may be stored in a non-physical order, but their payloads must
    // not alias the index or one another.  Sorting ranges validates both forms
    // without assuming that offsets are monotonic in the table.
    QList<QPair<qint64, qint64> > listRanges;
    listRanges.reserve(listEntries.count());
    qint64 nLogicalEnd = nCursor;
    for (const ENTRY &entry : listEntries) {
        if (entry.nDataOffset < nCursor) return false;
        nLogicalEnd = qMax(nLogicalEnd, entry.nDataOffset + entry.nDataSize);
        if (entry.nDataSize > 0) {
            listRanges.append(qMakePair(entry.nDataOffset,
                                        entry.nDataOffset + entry.nDataSize));
        }
    }
    std::sort(listRanges.begin(), listRanges.end(),
              [](const QPair<qint64, qint64> &a,
                 const QPair<qint64, qint64> &b) {
                  return (a.first < b.first) ||
                         ((a.first == b.first) && (a.second < b.second));
              });
    for (qint32 i = 1; i < listRanges.count(); ++i) {
        if (listRanges.at(i).first < listRanges.at(i - 1).second) {
            return false;
        }
    }

    const bool bRestored = positionGuard.restore();
    if (!guardedThis || !bRestored || (getFileType() != fileType) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nLogicalEnd;
    return true;
}

XBinary::FT XCKPEDPBase::getFileType()
{
    return m_fileTypeHint;
}

XBinary::MODE XCKPEDPBase::getMode()
{
    return MODE_DATA;
}

qint32 XCKPEDPBase::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XCKPEDPBase::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCKPEDPBase::getArch()
{
    return QString();
}

QString XCKPEDPBase::getFileFormatExt()
{
    if (getFileType() == FT_CKP) return QStringLiteral("ckp");
    if (getFileType() == FT_EDP) return QStringLiteral("edp");
    return QString();
}

QString XCKPEDPBase::getFileFormatExtsString()
{
    if (getFileType() == FT_CKP)
        return QStringLiteral("CKP game archive (*.ckp)");
    if (getFileType() == FT_EDP)
        return QStringLiteral("EdgeDataPak archive (*.edp)");
    return QString();
}

QString XCKPEDPBase::getMIMEString()
{
    if (getFileType() == FT_CKP)
        return QStringLiteral("application/x-ckp");
    if (getFileType() == FT_EDP)
        return QStringLiteral("application/x-edgedatapak");
    return QStringLiteral("application/octet-stream");
}

qint64 XCKPEDPBase::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    qint64 nArchiveEnd = 0;
    return scanArchive(nullptr, &nArchiveEnd, pPdStruct) ? nArchiveEnd : 0;
}

XBinary::OSNAME XCKPEDPBase::getOsName()
{
    return OSNAME_WINDOWS;
}

QString XCKPEDPBase::getVersion()
{
    return QStringLiteral("1");
}

QList<QString> XCKPEDPBase::getSearchSignatures()
{
    if (getFileType() == FT_CKP)
        return QList<QString>() << QStringLiteral("'.CKP'0001");
    if (getFileType() == FT_EDP)
        return QList<QString>() << QStringLiteral("'.EDP'0001");
    return QList<QString>();
}

QMap<XBinary::UNPACK_PROP, QVariant>
XCKPEDPBase::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCKPEDPBase::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XCKPEDPBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    UNPACK_CONTEXT *pOldContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    const FT fileType = getFileType();
    QList<ENTRY> listEntries;
    qint64 nArchiveEnd = 0;
    const bool bScanned = scanArchive(&listEntries, &nArchiveEnd, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || !isSupportedFileType(fileType) ||
        (getFileType() != fileType) ||
        !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis || !ckpEdpRangeWithin(nTotalSize, 0, nArchiveEnd)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->listEntries = listEntries;
    pContext->fileType = fileType;
    pContext->nArchiveEnd = nArchiveEnd;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.isEmpty()
        ? nArchiveEnd : listEntries.constFirst().nHeaderOffset;
    pState->nTotalSize = nTotalSize;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XCKPEDPBase::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCKPEDPBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext) {
        return ARCHIVERECORD();
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (pContext->fileType != getFileType()) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        !ckpEdpRangeWithin(nCurrentSize, 0, pContext->nArchiveEnd) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return ARCHIVERECORD();
    }

    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!ckpEdpRangeWithin(nCurrentSize, entry.nHeaderOffset,
                           entry.nHeaderSize) ||
        !ckpEdpRangeWithin(nCurrentSize, entry.nDataOffset,
                           entry.nDataSize) ||
        entry.sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE,
                                entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_RESOURCEID,
                                QVariant::fromValue(entry.nResourceId));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, false);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XCKPEDPBase::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCKPEDPBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) {
        return false;
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (pContext->fileType != getFileType()) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listEntries.at(
            pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveEnd;
    return false;
}

bool XCKPEDPBase::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;
    return true;
}

QList<XBinary::FPART_PROP> XCKPEDPBase::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);
    listResult.append(FPART_PROP_HEADER_OFFSET);
    listResult.append(FPART_PROP_HEADER_SIZE);
    listResult.append(FPART_PROP_RESOURCEID);
    listResult.append(FPART_PROP_ENCRYPTED);
    listResult.append(FPART_PROP_FILEMODE);
    return listResult;
}

bool XCKPEDPBase::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCKPEDPBase> guardedThis(this);
    bool bResult = true;
    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }
    return guardedThis && bResult;
}

void *XCKPEDPBase::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCKPEDPBase> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XCKPEDPBase::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(
            static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}

XCKP::XCKP(QIODevice *pDevice)
    : XCKPEDPBase(pDevice, FT_CKP)
{
}

bool XCKP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCKP archive(pDevice);
    return archive.XCKPEDPBase::isValid(pPdStruct);
}

XBinary *XCKP::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCKP(pDevice);
}
