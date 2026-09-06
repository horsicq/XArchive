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
#include "xgob.h"

#include <QPointer>
#include <QSet>

#include <cstring>
#include <new>

namespace {
// "GOB\n" - the LucasArts Dark Forces container signature.
const qint64 GOB_HEADER_SIZE = 8;
// {quint32 offset, quint32 size, char name[13]}
const qint64 GOB_RECORD_SIZE = 21;
const qint32 GOB_NAME_FIELD = 13;

// The directory record carries a fixed 13-byte name field; the archiver
// leaves whatever was in its buffer after the terminator, so only the bytes
// up to the first NUL are part of the name.  The producing tool also allows a
// name to fill the whole field, in which case the last byte is treated as the
// terminator (this mirrors the reference implementation, which clears it).
bool decodeGobName(const uchar *pField, QString *pName)
{
    if (!pField || !pName) return false;

    qint32 nLength = 0;
    while ((nLength < (GOB_NAME_FIELD - 1)) && pField[nLength]) nLength++;
    if (nLength == 0) return false;

    for (qint32 i = 0; i < nLength; ++i) {
        const quint8 nCharacter = pField[i];
        // Dark Forces names are plain printable ASCII 8.3 leaf names.
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') ||
            (nCharacter == ':')) {
            return false;
        }
    }

    QString sName = QString::fromLatin1(
        reinterpret_cast<const char *>(pField), nLength);
    if ((sName == QLatin1String(".")) || (sName == QLatin1String(".."))) {
        return false;
    }

    // A handful of shipped archives store names containing characters that no
    // Windows path may carry (for example "?EXT.MSG").  Map them the same way
    // the reference extractor does rather than rejecting the whole container.
    sName = XBinary::fixFileName(sName);
    if (sName.isEmpty() || sName.contains(QLatin1Char('/'))) return false;

    *pName = sName;
    return true;
}
}  // namespace

XGob::XGob(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_GOB)
{
}

bool XGob::scanGob(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                   PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pArchiveEnd) *pArchiveEnd = 0;

    QPointer<XGob> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedDevice || guardedDevice->isSequential() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Probing must leave the device exactly where it was found: the detection
    // chain hands the same device to the next candidate class.
    const qint64 nSavedPosition = guardedDevice->pos();
    if (nSavedPosition < 0) return false;

    QList<ENTRY> listEntries;
    qint64 nArchiveEnd = 0;
    const bool bResult = scanFormat(
        pEntries ? &listEntries : nullptr, &nArchiveEnd, pPdStruct);

    bool bRestored = false;
    if (guardedDevice) {
        bRestored = guardedDevice->seek(nSavedPosition) && guardedDevice &&
                    (guardedDevice->pos() == nSavedPosition);
    }

    if (!guardedThis || !bRestored || !bResult || (getFileType() != FT_GOB) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}

bool XGob::isValid(PDSTRUCT *pPdStruct)
{
    return scanGob(nullptr, nullptr, pPdStruct);
}

bool XGob::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGob archive(pDevice);
    return archive.isValid(pPdStruct);
}

qint64 XGob::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    qint64 nArchiveEnd = 0;
    if (!scanGob(nullptr, &nArchiveEnd, pPdStruct)) return 0;
    return nArchiveEnd;
}

XBinary *XGob::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGob(pDevice);
}

QString XGob::getFileFormatExt()
{
    return QStringLiteral("gob");
}

QString XGob::getFileFormatExtsString()
{
    return QStringLiteral("LucasArts GOB archive (*.gob)");
}

QString XGob::getMIMEString()
{
    return QStringLiteral("application/x-lucasarts-gob");
}

QList<QString> XGob::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'GOB'0A"));
    return listResult;
}

bool XGob::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XGob> guardedThis(this);
    const qint64 nTotalSize = getSize();
    // Header plus a directory holding at least one record.
    if (!guardedThis ||
        (nTotalSize < (GOB_HEADER_SIZE + 4 + GOB_RECORD_SIZE)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, GOB_HEADER_SIZE, pPdStruct);
    if (!guardedThis || (baHeader.size() != GOB_HEADER_SIZE) ||
        (memcmp(baHeader.constData(), "GOB\x0a", 4) != 0)) {
        return false;
    }

    const qint64 nDirectoryOffset = (qint64)readLE32(
        reinterpret_cast<const uchar *>(baHeader.constData()) + 4);
    // The first member always starts right behind the header, so the
    // directory can never point into it.
    if ((nDirectoryOffset < GOB_HEADER_SIZE) ||
        ((nDirectoryOffset + 4 + GOB_RECORD_SIZE) > nTotalSize)) {
        return false;
    }

    // The directory is the archive tail: 4-byte count plus fixed records.
    const qint64 nDirectoryBytes = nTotalSize - nDirectoryOffset - 4;
    if ((nDirectoryBytes % GOB_RECORD_SIZE) != 0) return false;
    const qint64 nRecordCount = nDirectoryBytes / GOB_RECORD_SIZE;
    if ((nRecordCount < 1) || (nRecordCount > (qint64)MAX_RECORDS)) {
        return false;
    }

    const qint64 nDirectorySize = 4 + nRecordCount * GOB_RECORD_SIZE;
    if (!rangeWithin(nTotalSize, nDirectoryOffset, nDirectorySize)) {
        return false;
    }

    const QByteArray baDirectory =
        read_array_process(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedThis || (baDirectory.size() != nDirectorySize)) return false;

    const uchar *pDirectory =
        reinterpret_cast<const uchar *>(baDirectory.constData());
    // A stored count that disagrees with the physical directory length means
    // this is not a GOB, whatever the magic says.
    if ((qint64)readLE32(pDirectory) != nRecordCount) return false;

    QList<ENTRY> listEntries;
    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;

    for (qint64 i = 0; i < nRecordCount; ++i) {
        if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint64 nRecordOffset = 4 + i * GOB_RECORD_SIZE;
        const uchar *pRecord = pDirectory + nRecordOffset;
        const quint32 nDataOffsetValue = readLE32(pRecord);
        const quint32 nDataSizeValue = readLE32(pRecord + 4);
        // Both fields are signed in the producing tool; negatives are junk.
        if ((nDataOffsetValue > 0x7fffffffU) ||
            (nDataSizeValue > 0x7fffffffU)) {
            return false;
        }
        const qint64 nDataOffset = (qint64)nDataOffsetValue;
        const qint64 nDataSize = (qint64)nDataSizeValue;
        // Member payloads live strictly between the header and the directory.
        if ((nDataOffset < GOB_HEADER_SIZE) ||
            !rangeWithin(nDirectoryOffset, nDataOffset, nDataSize)) {
            return false;
        }

        QString sName;
        QString sUniqueName;
        if (!decodeGobName(pRecord + 8, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName)) {
            return false;
        }

        ENTRY entry = {};
        entry.nHeaderOffset = nDirectoryOffset + nRecordOffset;
        entry.nHeaderSize = GOB_RECORD_SIZE;
        entry.nDataOffset = nDataOffset;
        entry.nDataSize = nDataSize;
        entry.nUncompressedSize = nDataSize;
        entry.handleMethod = HANDLE_METHOD_STORE;
        entry.sFileName = sUniqueName;
        listEntries.append(entry);
    }

    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (listEntries.count() != (qint32)nRecordCount)) {
        return false;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}

bool XGob::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XGob> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    GOB_CONTEXT *pOldContext = static_cast<GOB_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    QList<ENTRY> listEntries;
    qint64 nArchiveEnd = 0;
    const bool bScanned = scanGob(&listEntries, &nArchiveEnd, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || listEntries.isEmpty() ||
        !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis || !rangeWithin(nTotalSize, 0, nArchiveEnd)) {
        if (guardedThis) {
            releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
        }
        return false;
    }

    GOB_CONTEXT *pContext = new (std::nothrow) GOB_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pContext->listEntries = listEntries;
    pContext->nArchiveEnd = nArchiveEnd;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.constFirst().nHeaderOffset;
    pState->nTotalSize = nTotalSize;
    pState->mapUnpackProperties = mapProperties;

    // Paired with the bindUnpackSource() above; on failure the source has to
    // be released before the context dies or the token outlives its owner.
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

XBinary::ARCHIVERECORD XGob::infoCurrent(UNPACK_STATE *pState,
                                         PDSTRUCT *pPdStruct)
{
    QPointer<XGob> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext) {
        return ARCHIVERECORD();
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) return ARCHIVERECORD();

    GOB_CONTEXT *pContext = static_cast<GOB_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (getFileType() != FT_GOB) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return ARCHIVERECORD();
    }

    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!rangeWithin(nCurrentSize, entry.nHeaderOffset, entry.nHeaderSize) ||
        !rangeWithin(nCurrentSize, entry.nDataOffset, entry.nDataSize) ||
        (entry.nUncompressedSize < -1) || entry.sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                entry.nUncompressedSize >= 0
                                    ? entry.nUncompressedSize
                                    : entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, entry.handleMethod);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XGob::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XGob> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) {
        return false;
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    GOB_CONTEXT *pContext = static_cast<GOB_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (getFileType() != FT_GOB) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    // The cursor has to come to rest one past the last record, so the walk is
    // driven by the >= guard above and never by (count - 1).
    pState->nCurrentIndex++;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XGob::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    GOB_CONTEXT *pContext = static_cast<GOB_CONTEXT *>(pState->pContext);
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
