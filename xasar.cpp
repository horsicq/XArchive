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
#include "xasar.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QVector>
#include <cmath>
#include <limits>
#include <new>

#ifdef Q_OS_WIN
#include <io.h>
#include <qt_windows.h>
#elif defined(Q_OS_MAC)
#include <fcntl.h>
#include <sys/param.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#endif

static XBinary::XCONVERT _TABLE_XASAR_STRUCTID[] = {{XASAR::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")}, {XASAR::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

static const qint64 ASAR_MAX_JSON_SIZE = Q_INT64_C(16) * 1024 * 1024;
static const qint64 ASAR_MAX_FILE_SIZE = Q_INT64_C(9007199254740991);
static const qint32 ASAR_MAX_LINK_DEPTH = 40;
static const qint32 ASAR_MAX_TREE_DEPTH = 256;
static const qint32 ASAR_MAX_RECORDS = 1000000;

class ASAR_DEVICE_POSITION_GUARD {
public:
    explicit ASAR_DEVICE_POSITION_GUARD(QIODevice *pDevice) : m_pDevice(pDevice), m_nPosition(pDevice ? pDevice->pos() : -1), m_bRestored(false)
    {
    }

    ~ASAR_DEVICE_POSITION_GUARD()
    {
        restore();
    }

    bool isValid() const
    {
        return m_pDevice && (m_nPosition >= 0);
    }

    bool restore()
    {
        if (m_bRestored) return !m_pDevice.isNull();
        m_bRestored = true;
        return m_pDevice && m_pDevice->seek(m_nPosition);
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

static Qt::CaseSensitivity asarPathCaseSensitivity()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

static bool asarPathsEqual(const QString &sFirst, const QString &sSecond)
{
    return !sFirst.isEmpty() && !sSecond.isEmpty() &&
           (QDir::fromNativeSeparators(QDir::cleanPath(sFirst)).compare(QDir::fromNativeSeparators(QDir::cleanPath(sSecond)), asarPathCaseSensitivity()) == 0);
}

static bool asarIsValidComponent(const QString &sComponent)
{
    return !sComponent.isEmpty() && (sComponent != QLatin1String(".")) && (sComponent != QLatin1String("..")) && !sComponent.contains(QChar(0)) &&
           !sComponent.contains(QLatin1Char('/')) && !sComponent.contains(QLatin1Char('\\'));
}

static bool asarNormalizeRelativePath(const QString &sPath, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pResult || sPath.isEmpty() || sPath.contains(QChar(0))) return false;

    QString sNormalized = sPath;
    sNormalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sNormalized.startsWith(QLatin1Char('/')) || QDir::isAbsolutePath(sNormalized) ||
        ((sNormalized.size() >= 2) && sNormalized.at(0).isLetter() && (sNormalized.at(1) == QLatin1Char(':')))) {
        return false;
    }

    const QStringList listParts = sNormalized.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    if (listParts.isEmpty()) return false;
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
    }

    *pResult = listParts.join(QLatin1Char('/'));
    return true;
}

static bool asarNormalizeLinkPath(const QString &sPath, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pResult || sPath.isEmpty() || sPath.contains(QChar(0))) return false;

    QString sNormalized = sPath;
    sNormalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sNormalized.startsWith(QLatin1Char('/')) || QDir::isAbsolutePath(sNormalized) ||
        ((sNormalized.size() >= 2) && sNormalized.at(0).isLetter() && (sNormalized.at(1) == QLatin1Char(':')))) {
        return false;
    }

    QStringList listResult;
    const QStringList listParts = sNormalized.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty()) return false;
        if (sPart == QLatin1String(".")) continue;
        if (sPart == QLatin1String("..")) {
            if (listResult.isEmpty()) return false;
            listResult.removeLast();
        } else {
            if (!asarIsValidComponent(sPart)) return false;
            listResult.append(sPart);
        }
    }
    if (listResult.isEmpty()) return false;
    *pResult = listResult.join(QLatin1Char('/'));
    return true;
}

static bool asarPathIsWithinRoot(const QString &sRoot, const QString &sPath)
{
    if (sRoot.isEmpty() || sPath.isEmpty()) return false;
    QString sPrefix = QDir::fromNativeSeparators(QDir::cleanPath(sRoot));
    const QString sCandidate = QDir::fromNativeSeparators(QDir::cleanPath(sPath));
    if (!sPrefix.endsWith(QLatin1Char('/'))) sPrefix.append(QLatin1Char('/'));
    return sCandidate.startsWith(sPrefix, asarPathCaseSensitivity());
}

static bool asarResolveExternalRoot(const QString &sRoot, QString *pCanonicalRoot)
{
    if (pCanonicalRoot) pCanonicalRoot->clear();
    if (!pCanonicalRoot || sRoot.isEmpty()) return false;
    QFileInfo rootInfo(sRoot);
    if (!rootInfo.exists() || !rootInfo.isDir() || rootInfo.isSymLink()
#ifdef Q_OS_WIN
        || rootInfo.isJunction()
#endif
    )
        return false;
    *pCanonicalRoot = QDir::fromNativeSeparators(rootInfo.canonicalFilePath());
    return !pCanonicalRoot->isEmpty();
}

static bool asarResolveExternalFile(const QString &sRoot, const QString &sPinnedCanonicalRoot, const QString &sLogicalName, QString *pResult)
{
    if (pResult) pResult->clear();
    QString sNormalizedName;
    if (!pResult || sPinnedCanonicalRoot.isEmpty() || !asarNormalizeRelativePath(sLogicalName, &sNormalizedName)) return false;

    QString sCanonicalRoot;
    if (!asarResolveExternalRoot(sRoot, &sCanonicalRoot) || !asarPathsEqual(sCanonicalRoot, sPinnedCanonicalRoot)) return false;

    const QString sCandidate = QDir(sRoot).absoluteFilePath(sNormalizedName);
    QFileInfo candidateInfo(sCandidate);
    if (!candidateInfo.exists() || !candidateInfo.isFile() || candidateInfo.isSymLink()) return false;
    const QString sCanonicalCandidate = QDir::fromNativeSeparators(candidateInfo.canonicalFilePath());
    if (sCanonicalCandidate.isEmpty() || !asarPathIsWithinRoot(sCanonicalRoot, sCanonicalCandidate)) return false;

    *pResult = sCanonicalCandidate;
    return true;
}

static bool asarOpenedFileCanonicalPath(QFile *pFile, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pFile || !pResult || !pFile->isOpen() || (pFile->handle() < 0)) return false;

    QString sNativePath;
#ifdef Q_OS_WIN
    const intptr_t nNativeHandle = _get_osfhandle(pFile->handle());
    if (nNativeHandle == -1) return false;
    HANDLE hFile = reinterpret_cast<HANDLE>(nNativeHandle);
    const DWORD nRequired = GetFinalPathNameByHandleW(hFile, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if ((nRequired == 0) || (nRequired > (1u << 20))) return false;
    QVector<wchar_t> buffer((qint32)nRequired + 1);
    const DWORD nLength = GetFinalPathNameByHandleW(hFile, buffer.data(), (DWORD)buffer.size(), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if ((nLength == 0) || (nLength >= (DWORD)buffer.size())) return false;
    sNativePath = QString::fromWCharArray(buffer.constData(), (qint32)nLength);
    if (sNativePath.startsWith(QLatin1String("\\\\?\\UNC\\"), Qt::CaseInsensitive)) {
        sNativePath = QLatin1String("\\\\") + sNativePath.mid(8);
    } else if (sNativePath.startsWith(QLatin1String("\\\\?\\"), Qt::CaseInsensitive)) {
        sNativePath = sNativePath.mid(4);
    }
#elif defined(Q_OS_MAC)
    QByteArray buffer(MAXPATHLEN, '\0');
    if (::fcntl(pFile->handle(), F_GETPATH, buffer.data()) != 0) return false;
    sNativePath = QString::fromLocal8Bit(buffer.constData());
#elif defined(Q_OS_LINUX)
    const QByteArray fdPath = QByteArrayLiteral("/proc/self/fd/") + QByteArray::number(pFile->handle());
    QByteArray buffer(4096, '\0');
    for (;;) {
        const ssize_t nLength = ::readlink(fdPath.constData(), buffer.data(), (size_t)buffer.size());
        if (nLength < 0) return false;
        if (nLength < buffer.size()) {
            sNativePath = QString::fromLocal8Bit(buffer.constData(), (qint32)nLength);
            break;
        }
        if (buffer.size() >= (1 << 20)) return false;
        buffer.resize(buffer.size() * 2);
    }
#else
    // External sidecars fail closed on platforms where QFile cannot expose a
    // trustworthy final path for the already-open handle.
    return false;
#endif

    const QString sCanonicalPath = QDir::fromNativeSeparators(QFileInfo(sNativePath).canonicalFilePath());
    if (sCanonicalPath.isEmpty()) return false;
    *pResult = sCanonicalPath;
    return true;
}

static bool asarOpenedFileIsContained(QFile *pFile, const QString &sCanonicalRoot, const QString &sExpectedCanonicalPath)
{
    QString sOpenedPath;
    return asarOpenedFileCanonicalPath(pFile, &sOpenedPath) && asarPathIsWithinRoot(sCanonicalRoot, sOpenedPath) && asarPathsEqual(sOpenedPath, sExpectedCanonicalPath);
}

static bool asarHashExternalFile(const QString &sFileName, const QString &sCanonicalRoot, qint64 nExpectedSize, QByteArray *pHash, XBinary::PDSTRUCT *pPdStruct)
{
    if (pHash) pHash->clear();
    if (!pHash || (nExpectedSize < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly) || file.isSequential() || (file.size() != nExpectedSize) || !asarOpenedFileIsContained(&file, sCanonicalRoot, sFileName))
        return false;

    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray baBuffer;
    baBuffer.resize(1 << 20);
    if (baBuffer.size() != (1 << 20)) return false;

    qint64 nReadTotal = 0;
    while ((nReadTotal < nExpectedSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRequest = qMin<qint64>(baBuffer.size(), nExpectedSize - nReadTotal);
        const qint64 nRead = file.read(baBuffer.data(), nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) return false;
        hash.addData(baBuffer.constData(), nRead);
        nReadTotal += nRead;
    }

    if ((nReadTotal != nExpectedSize) || !XBinary::isPdStructNotCanceled(pPdStruct) || (file.size() != nExpectedSize) || !file.atEnd()) {
        return false;
    }

    *pHash = hash.result();
    return pHash->size() == 32;
}

static bool asarIsSHA256Hex(const QByteArray &baHex)
{
    if (baHex.size() != 64) return false;
    for (char c : baHex) {
        if (!(((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))) return false;
    }
    return true;
}

static bool asarReadSHA256(const QJsonObject &objEntry, qint64 nFileSize, QByteArray *pHash)
{
    if (pHash) pHash->clear();
    if (!pHash || (nFileSize < 0)) return false;
    if (!objEntry.contains(QLatin1String("integrity"))) return true;

    const QJsonValue integrityValue = objEntry.value(QLatin1String("integrity"));
    if (!integrityValue.isObject()) return false;
    const QJsonObject integrity = integrityValue.toObject();
    if (!integrity.value(QLatin1String("algorithm")).isString() || (integrity.value(QLatin1String("algorithm")).toString() != QLatin1String("SHA256")) ||
        !integrity.value(QLatin1String("hash")).isString() || !integrity.value(QLatin1String("blockSize")).isDouble() ||
        !integrity.value(QLatin1String("blocks")).isArray())
        return false;

    const QByteArray baHex = integrity.value(QLatin1String("hash")).toString().toLatin1();
    if (!asarIsSHA256Hex(baHex)) return false;
    const double dBlockSize = integrity.value(QLatin1String("blockSize")).toDouble();
    if (!std::isfinite(dBlockSize) || (dBlockSize <= 0) || (dBlockSize > (double)ASAR_MAX_FILE_SIZE) || (std::floor(dBlockSize) != dBlockSize)) return false;
    const qint64 nBlockSize = (qint64)dBlockSize;
    const qint64 nExpectedBlocks = (nFileSize == 0) ? 1 : (((nFileSize - 1) / nBlockSize) + 1);
    const QJsonArray blocks = integrity.value(QLatin1String("blocks")).toArray();
    if ((nExpectedBlocks > (std::numeric_limits<qint32>::max)()) || (blocks.size() != (qint32)nExpectedBlocks)) return false;
    for (const QJsonValue &block : blocks) {
        if (!block.isString() || !asarIsSHA256Hex(block.toString().toLatin1())) return false;
    }
    *pHash = QByteArray::fromHex(baHex);
    return pHash->size() == 32;
}

XASAR::XASAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

struct XASAR::LINK_CONTEXT
{
    ASAR_UNPACK_CONTEXT *pUnpackContext;
    const QHash<QString, qint32> *pRecordMap;
    QSet<QString> *pAllNames;
    qint32 nOriginalRecordCount;
};

bool XASAR::_readHeader(qint64 *pnJsonOffset, qint64 *pnJsonSize, qint64 *pnBlobOffset)
{
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid()) return false;
    const qint64 nFileSize = getSize();
    if (!guardedThis || (nFileSize < 16)) {
        return false;
    }

    // Pickle: sizeOfHeaderPickle(4)=4, sizeOfHeader(4), jsonStrLenField(4), jsonByteLen(4)
    const QByteArray baHeader = read_array(0, 16);
    if (!guardedThis || (baHeader.size() != 16)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    const quint32 nField0 = qFromLittleEndian<quint32>(pHeader);
    const quint32 nHeaderSize = qFromLittleEndian<quint32>(pHeader + 4);
    const quint32 nJsonStrSize = qFromLittleEndian<quint32>(pHeader + 8);
    const quint32 nJsonSize = qFromLittleEndian<quint32>(pHeader + 12);

    // The first pickle payload is always 4 (it encodes a single uint32).
    if (nField0 != 4) {
        return false;
    }

    if ((nJsonSize == 0) || ((qint64)nJsonSize > ASAR_MAX_JSON_SIZE)) {
        return false;
    }

    // A Pickle string has a four-byte length followed by its bytes padded to a
    // four-byte boundary. The second Pickle contains exactly that string and
    // the first Pickle contains exactly the second Pickle's byte length.
    const quint64 nAlignedJsonSize = ((quint64)nJsonSize + 3u) & ~Q_UINT64_C(3);
    const quint64 nExpectedJsonStringPayload = 4u + nAlignedJsonSize;
    const quint64 nExpectedHeaderSize = 4u + nExpectedJsonStringPayload;
    if (((quint64)nJsonStrSize != nExpectedJsonStringPayload) || ((quint64)nHeaderSize != nExpectedHeaderSize)) {
        return false;
    }

    const qint64 nJsonOffset = 16;
    const qint64 nBlobOffset = 8 + (qint64)nHeaderSize;
    if ((nJsonOffset + (qint64)nJsonSize > nBlobOffset) || (nBlobOffset > nFileSize)) {
        return false;
    }

    if (pnJsonOffset) *pnJsonOffset = nJsonOffset;
    if (pnJsonSize) *pnJsonSize = (qint64)nJsonSize;
    if (pnBlobOffset) *pnBlobOffset = nBlobOffset;

    return positionGuard.restore();
}

bool XASAR::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid()) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    if (!_readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset) || !guardedThis) {
        return false;
    }

    // The JSON directory must parse and contain a "files" object.
    QByteArray baJson = read_array(nJsonOffset, nJsonSize);
    if (!guardedThis) return false;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(baJson, &parseError);

    if ((parseError.error != QJsonParseError::NoError) || !doc.isObject()) {
        return false;
    }

    return doc.object().value(QLatin1String("files")).isObject() && positionGuard.restore();
}

bool XASAR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XASAR xasar(pDevice);

    return xasar.isValid(pPdStruct);
}

bool XASAR::_walkTree(const QJsonObject &objFiles, const QString &sParent, qint64 nBlobOffset, QList<ASAR_RECORD> *pListRecords, PDSTRUCT *pPdStruct, qint32 nDepth,
                      bool bParentUnpacked)
{
    if (!pListRecords || (nDepth < 0) || (nDepth > ASAR_MAX_TREE_DEPTH) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QStringList listKeys = objFiles.keys();

    for (const QString &sKey : listKeys) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (pListRecords->count() >= ASAR_MAX_RECORDS) || !asarIsValidComponent(sKey)) {
            return false;
        }

        const QJsonValue entryValue = objFiles.value(sKey);
        if (!entryValue.isObject()) return false;
        const QJsonObject objEntry = entryValue.toObject();
        const QString sPath = sParent.isEmpty() ? sKey : (sParent + QLatin1Char('/') + sKey);
        const bool bHasFiles = objEntry.contains(QLatin1String("files"));
        const bool bHasLink = objEntry.contains(QLatin1String("link"));
        const bool bHasOffset = objEntry.contains(QLatin1String("offset"));
        const bool bHasSize = objEntry.contains(QLatin1String("size"));
        const bool bHasIntegrity = objEntry.contains(QLatin1String("integrity"));
        const bool bHasExecutable = objEntry.contains(QLatin1String("executable"));
        const bool bHasUnpacked = objEntry.contains(QLatin1String("unpacked"));
        if (bHasUnpacked && !objEntry.value(QLatin1String("unpacked")).isBool()) return false;
        const bool bOwnUnpacked = bHasUnpacked && objEntry.value(QLatin1String("unpacked")).toBool();
        const bool bUnpacked = bParentUnpacked || bOwnUnpacked;

        if (bHasFiles) {
            if (bHasLink || bHasOffset || bHasSize || bHasIntegrity || bHasExecutable || !objEntry.value(QLatin1String("files")).isObject()) return false;
            ASAR_RECORD folderRecord = {};
            folderRecord.sFileName = sPath;
            folderRecord.nOffset = 0;
            folderRecord.nSize = 0;
            folderRecord.bIsFolder = true;
            pListRecords->append(folderRecord);

            if (!_walkTree(objEntry.value("files").toObject(), sPath, nBlobOffset, pListRecords, pPdStruct, nDepth + 1, bUnpacked)) {
                return false;
            }
        } else if (bHasLink) {
            if (bHasOffset || bHasSize || bHasIntegrity || bHasExecutable || !objEntry.value(QLatin1String("link")).isString()) return false;
            QString sNormalizedLink;
            if (!asarNormalizeLinkPath(objEntry.value(QLatin1String("link")).toString(), &sNormalizedLink)) return false;
            ASAR_RECORD linkRecord = {};
            linkRecord.sFileName = sPath;
            linkRecord.sLinkName = sNormalizedLink;
            linkRecord.bIsLink = true;
            pListRecords->append(linkRecord);
        } else {
            if (!bHasSize || !objEntry.value(QLatin1String("size")).isDouble() || (bUnpacked ? bHasOffset : !bHasOffset) ||
                (bHasExecutable && !objEntry.value(QLatin1String("executable")).isBool()))
                return false;
            const double dSize = objEntry.value(QLatin1String("size")).toDouble(-1);
            if (!std::isfinite(dSize) || (dSize < 0) || (dSize > (double)ASAR_MAX_FILE_SIZE) || (std::floor(dSize) != dSize)) return false;

            ASAR_RECORD fileRecord = {};
            fileRecord.sFileName = sPath;
            fileRecord.nSize = (qint64)dSize;
            fileRecord.bIsExternal = bUnpacked;

            if (bUnpacked) {
                if (!asarReadSHA256(objEntry, fileRecord.nSize, &fileRecord.baExternalSHA256)) return false;
            } else {
                QByteArray baUnusedHash;
                if (!asarReadSHA256(objEntry, fileRecord.nSize, &baUnusedHash) || !objEntry.value(QLatin1String("offset")).isString()) return false;
                const QString sOffset = objEntry.value(QLatin1String("offset")).toString();
                if (sOffset.isEmpty()) return false;
                for (const QChar c : sOffset) {
                    if (!c.isDigit() || (c.unicode() > 0x7f)) return false;
                }
                bool bOk = false;
                const qulonglong nUnsignedOffset = sOffset.toULongLong(&bOk, 10);
                if (!bOk || (nUnsignedOffset > (qulonglong)(std::numeric_limits<qint64>::max)()) || (nBlobOffset < 0)) return false;
                const qint64 nRelOffset = (qint64)nUnsignedOffset;
                if ((nRelOffset < 0) || (nRelOffset > (std::numeric_limits<qint64>::max)() - nBlobOffset)) {
                    return false;
                }
                fileRecord.nOffset = nBlobOffset + nRelOffset;
            }
            pListRecords->append(fileRecord);
        }
    }

    return true;
}

bool XASAR::_prepareExternalRecords(ASAR_UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    bool bHasExternalRecords = false;
    for (const ASAR_RECORD &record : pContext->listRecords) {
        if (record.bIsExternal) {
            bHasExternalRecords = true;
            break;
        }
    }
    if (!bHasExternalRecords) return true;

    QFile *pArchiveFile = dynamic_cast<QFile *>(getDevice());
    QPointer<QFile> guardedArchiveFile(pArchiveFile);
    if (!guardedArchiveFile || guardedArchiveFile->fileName().isEmpty()) return false;
    const QString sArchivePath = QDir::fromNativeSeparators(QFileInfo(guardedArchiveFile->fileName()).absoluteFilePath());
    QString sOpenedArchivePath;
    if (sArchivePath.isEmpty() || !asarOpenedFileCanonicalPath(guardedArchiveFile.data(), &sOpenedArchivePath) ||
        !asarPathsEqual(QFileInfo(sArchivePath).canonicalFilePath(), sOpenedArchivePath))
        return false;
    pContext->sExternalRoot = QDir::fromNativeSeparators(sArchivePath + QLatin1String(".unpacked"));
    if (!asarResolveExternalRoot(pContext->sExternalRoot, &pContext->sExternalCanonicalRoot)) return false;

    for (ASAR_RECORD &record : pContext->listRecords) {
        if (!record.bIsExternal) continue;
        record.sExternalLogicalName = record.sFileName;
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            !asarResolveExternalFile(pContext->sExternalRoot, pContext->sExternalCanonicalRoot, record.sExternalLogicalName, &record.sExternalFileName)) {
            return false;
        }

        const QByteArray baHeaderSHA256 = record.baExternalSHA256;
        QByteArray baActualSHA256;
        if (!asarHashExternalFile(record.sExternalFileName, pContext->sExternalCanonicalRoot, record.nSize, &baActualSHA256, pPdStruct) ||
            (!baHeaderSHA256.isEmpty() && (baActualSHA256 != baHeaderSHA256))) {
            return false;
        }
        record.baExternalSHA256 = baActualSHA256;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XASAR::_resolvePath(LINK_CONTEXT *pLinkContext,
                         const QString &sInputPath,
                         qint32 nInitialLinkDepth,
                         qint32 *pnTargetIndex, QString *pFinalPath)
{
    if (pnTargetIndex) *pnTargetIndex = -1;
    if (pFinalPath) pFinalPath->clear();
    if (!pLinkContext || !pLinkContext->pUnpackContext ||
        !pLinkContext->pRecordMap || !pnTargetIndex || !pFinalPath ||
        (nInitialLinkDepth < 0) ||
        (nInitialLinkDepth > ASAR_MAX_LINK_DEPTH)) return false;
    QString sCurrentPath;
    if (!asarNormalizeLinkPath(sInputPath, &sCurrentPath)) return false;

    QSet<QString> setVisited;
    qint32 nLinkDepth = nInitialLinkDepth;
    for (;;) {
        if (setVisited.contains(sCurrentPath)) return false;
        setVisited.insert(sCurrentPath);

        const qint32 nExactIndex = pLinkContext->pRecordMap->value(sCurrentPath, -1);
        if (nExactIndex >= 0) {
            const ASAR_RECORD &record =
                pLinkContext->pUnpackContext->listRecords.at(nExactIndex);
            if (!record.bIsLink) {
                *pnTargetIndex = nExactIndex;
                *pFinalPath = sCurrentPath;
                return true;
            }
            if (++nLinkDepth > ASAR_MAX_LINK_DEPTH) return false;
            sCurrentPath = record.sLinkName;
            continue;
        }

        bool bRewritten = false;
        qint32 nSlash = sCurrentPath.lastIndexOf(QLatin1Char('/'));
        while (nSlash > 0) {
            const QString sPrefix = sCurrentPath.left(nSlash);
            const qint32 nPrefixIndex =
                pLinkContext->pRecordMap->value(sPrefix, -1);
            if ((nPrefixIndex >= 0) &&
                pLinkContext->pUnpackContext->listRecords.at(nPrefixIndex).bIsLink) {
                if (++nLinkDepth > ASAR_MAX_LINK_DEPTH) return false;
                QString sRewrittenPath;
                if (!asarNormalizeLinkPath(
                        pLinkContext->pUnpackContext->listRecords.at(nPrefixIndex).sLinkName +
                            QLatin1Char('/') + sCurrentPath.mid(nSlash + 1),
                        &sRewrittenPath))
                    return false;
                sCurrentPath = sRewrittenPath;
                bRewritten = true;
                break;
            }
            nSlash = sCurrentPath.lastIndexOf(QLatin1Char('/'), nSlash - 1);
        }
        if (!bRewritten) return false;
    }
}

bool XASAR::_expandDirectory(LINK_CONTEXT *pLinkContext,
                             const QString &sAliasPath,
                             const QString &sTargetFolderPath,
                             QSet<QString> *pAncestry, qint32 nDepth)
{
    if (!pLinkContext || !pLinkContext->pUnpackContext ||
        !pLinkContext->pAllNames || !pAncestry ||
        (nDepth > ASAR_MAX_TREE_DEPTH) ||
        pAncestry->contains(sTargetFolderPath)) return false;
    pAncestry->insert(sTargetFolderPath);
    const QString sPrefix = sTargetFolderPath + QLatin1Char('/');

    for (qint32 j = 0; j < pLinkContext->nOriginalRecordCount; ++j) {
        const QString sSourcePath =
            pLinkContext->pUnpackContext->listRecords.at(j).sFileName;
        if (!sSourcePath.startsWith(sPrefix)) continue;
        const QString sSuffix = sSourcePath.mid(sPrefix.size());
        if (sSuffix.isEmpty() || sSuffix.contains(QLatin1Char('/'))) continue;

        qint32 nTargetIndex = -1;
        QString sResolvedChildPath;
        if (!_resolvePath(pLinkContext, sSourcePath, 0, &nTargetIndex,
                          &sResolvedChildPath) || (nTargetIndex < 0))
            return false;
        const QString sDestinationPath =
            sAliasPath + QLatin1Char('/') + sSuffix;
        if (pLinkContext->pAllNames->contains(sDestinationPath) ||
            (pLinkContext->pUnpackContext->listRecords.size() >= ASAR_MAX_RECORDS))
            return false;

        ASAR_RECORD materialized =
            pLinkContext->pUnpackContext->listRecords.at(nTargetIndex);
        materialized.sFileName = sDestinationPath;
        materialized.sLinkName = sSourcePath;
        materialized.bIsLink = true;
        pLinkContext->pUnpackContext->listRecords.append(materialized);
        pLinkContext->pAllNames->insert(sDestinationPath);

        if (materialized.bIsFolder &&
            !_expandDirectory(pLinkContext, sDestinationPath,
                              sResolvedChildPath, pAncestry, nDepth + 1))
            return false;
    }

    pAncestry->remove(sTargetFolderPath);
    return true;
}

bool XASAR::_resolveLinks(ASAR_UNPACK_CONTEXT *pContext)
{
    if (!pContext) return false;

    QHash<QString, qint32> mapRecords;
    const qint32 nOriginalRecordCount = pContext->listRecords.size();
    for (qint32 i = 0; i < nOriginalRecordCount; ++i) {
        QString sNormalizedPath;
        if (!asarNormalizeRelativePath(pContext->listRecords.at(i).sFileName, &sNormalizedPath) || (sNormalizedPath != pContext->listRecords.at(i).sFileName))
            return false;
        if (mapRecords.contains(sNormalizedPath)) return false;
        mapRecords.insert(sNormalizedPath, i);
    }

    LINK_CONTEXT linkContext = {pContext, &mapRecords, nullptr,
                                nOriginalRecordCount};

    for (qint32 i = 0; i < nOriginalRecordCount; ++i) {
        ASAR_RECORD &linkRecord = pContext->listRecords[i];
        if (!linkRecord.bIsLink) continue;

        QString sTargetPath;
        if (!asarNormalizeLinkPath(linkRecord.sLinkName, &sTargetPath)) return false;
        linkRecord.sLinkName = sTargetPath;

        qint32 nTargetIndex = -1;
        QString sFinalPath;
        if (!_resolvePath(&linkContext, sTargetPath, 1, &nTargetIndex,
                          &sFinalPath) || (nTargetIndex < 0)) return false;
        const ASAR_RECORD target = pContext->listRecords.at(nTargetIndex);
        linkRecord.nOffset = target.nOffset;
        linkRecord.nSize = target.nSize;
        linkRecord.bIsFolder = target.bIsFolder;
        linkRecord.bIsExternal = target.bIsExternal;
        linkRecord.sExternalLogicalName = target.sExternalLogicalName;
        linkRecord.sExternalFileName = target.sExternalFileName;
        linkRecord.baExternalSHA256 = target.baExternalSHA256;
    }

    const QStringList listRecordNames = mapRecords.keys();
    QSet<QString> setAllNames(listRecordNames.cbegin(), listRecordNames.cend());
    linkContext.pAllNames = &setAllNames;

    for (qint32 i = 0; i < nOriginalRecordCount; ++i) {
        const ASAR_RECORD linkRecord = pContext->listRecords.at(i);
        if (!linkRecord.bIsLink || !linkRecord.bIsFolder) continue;
        qint32 nTargetIndex = -1;
        QString sFinalPath;
        if (!_resolvePath(&linkContext, linkRecord.sLinkName, 1,
                          &nTargetIndex, &sFinalPath) || (nTargetIndex < 0) ||
            !pContext->listRecords.at(nTargetIndex).bIsFolder) return false;
        QSet<QString> setAncestry;
        if (!_expandDirectory(&linkContext, linkRecord.sFileName, sFinalPath,
                              &setAncestry, 0)) return false;
    }

    return true;
}

XBinary::FT XASAR::getFileType()
{
    return FT_ASAR;
}

XBinary::MODE XASAR::getMode()
{
    return MODE_DATA;
}

QString XASAR::getMIMEString()
{
    return "application/x-asar";
}

qint32 XASAR::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XASAR::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XASAR::getArch()
{
    return QString();
}

QString XASAR::getFileFormatExt()
{
    return "asar";
}

QString XASAR::getFileFormatExtsString()
{
    return "ASAR (*.asar)";
}

qint64 XASAR::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XASAR::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XASAR::getVersion()
{
    return QString();
}

QList<XBinary::MAPMODE> XASAR::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XASAR::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)
    Q_UNUSED(pPdStruct)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;
    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    _readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset);

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = (nBlobOffset > 0) ? nBlobOffset : 16;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("ASAR ") + tr("Header");
    result.listRecords.append(recHeader);

    if ((nBlobOffset > 0) && (nBlobOffset < getSize())) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = nBlobOffset;
        recData.nSize = getSize() - nBlobOffset;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Files");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XASAR::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XASAR_STRUCTID, sizeof(_TABLE_XASAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XASAR::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XASAR_STRUCTID, sizeof(_TABLE_XASAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XASAR::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XASAR_STRUCTID, sizeof(_TABLE_XASAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XASAR::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = 16;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XASAR::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_HEADER) {
        listResult.append({"pickle_size", 0, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"header_size", 4, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"json_str_size", 8, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"json_size", 12, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
    }

    return listResult;
}

static bool asarCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XASAR::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    _readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset);

    if ((nFileParts & FILEPART_HEADER) && asarCanAppendPart(nLimit, listResult)) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = (nBlobOffset > 0) ? nBlobOffset : 16;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && asarCanAppendPart(nLimit, listResult) && (nBlobOffset > 0) && (nBlobOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nBlobOffset;
        record.nFileSize = getSize() - nBlobOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Files");
        listResult.append(record);
    }

    return listResult;
}

QList<QString> XASAR::getSearchSignatures()
{
    return {};
}

XBinary *XASAR::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XASAR(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XASAR::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XASAR::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid()) return false;
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    ASAR_UNPACK_CONTEXT *pOldContext = static_cast<ASAR_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis) return false;
    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) {
        return false;
    }

    const bool bValid = isValid(pPdStruct);
    if (!guardedThis) return false;
    if (!bValid) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    const bool bHeaderRead = _readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset);
    if (!guardedThis) return false;
    if (!bHeaderRead) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    QByteArray baJson = read_array(nJsonOffset, nJsonSize);
    if (!guardedThis) return false;
    QJsonDocument doc = QJsonDocument::fromJson(baJson);

    if (!doc.isObject() || !doc.object().value(QLatin1String("files")).isObject()) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;

    ASAR_UNPACK_CONTEXT *pContext = new (std::nothrow) ASAR_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    if (!_walkTree(doc.object().value("files").toObject(), QString(), nBlobOffset, &(pContext->listRecords), pPdStruct, 0) ||
        !_prepareExternalRecords(pContext, pPdStruct) || !_resolveLinks(pContext)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    for (const ASAR_RECORD &record : pContext->listRecords) {
        if (!record.bIsFolder && !record.bIsExternal &&
            ((record.nOffset < nBlobOffset) || (record.nOffset > nTotalSize) || (record.nSize < 0) || (record.nSize > nTotalSize - record.nOffset))) {
            releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
    }

    if (pContext->listRecords.isEmpty() || !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nTotalSize = nTotalSize;
    pState->nCurrentOffset = 0;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return positionGuard.restore();
}

XBinary::ARCHIVERECORD XASAR::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid()) return ARCHIVERECORD();
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return result;

    ASAR_UNPACK_CONTEXT *pContext = (ASAR_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const ASAR_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nOffset;
    result.nStreamSize = record.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);

    if (record.bIsFolder) {
        result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    }
    if (record.bIsLink) {
        result.mapProperties.insert(FPART_PROP_INFO, QStringLiteral("ASAR symbolic link -> %1 (materialized)").arg(record.sLinkName));
    }
    if (record.bIsExternal) {
        if (record.baExternalSHA256.size() != 32) return ARCHIVERECORD();
        result.mapProperties.insert(FPART_PROP_CHECKSUM, QString::fromLatin1(record.baExternalSHA256.toHex()));
        result.mapProperties.insert(FPART_PROP_CHECKSUMTYPE, QStringLiteral("SHA256"));
        if (!record.bIsLink) result.mapProperties.insert(FPART_PROP_INFO, QStringLiteral("External ASAR sidecar"));
        if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    }

    return positionGuard.restore() ? result : ARCHIVERECORD();
}

bool XASAR::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid() || !operationGuard.isAcquired() || !pState || !pDevice || !guardedThis) return false;
    pState->nCurrentOffset = 0;

    const bool bInitialSourceCurrent = pState->pContext && guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!pState->pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) || !bInitialSourceCurrent || !guardedThis) {
        return false;
    }

    ASAR_UNPACK_CONTEXT *pContext = static_cast<ASAR_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nCurrentIndex >= pContext->listRecords.size()) || !guardedThis) return false;
    const ASAR_RECORD record = pContext->listRecords.at(pState->nCurrentIndex);

    if (!record.bIsExternal) {
        operationGuard.release();
        const bool bResult = guardedThis && guardedThis->XArchive::unpackCurrent(pState, pDevice, pPdStruct);
        return bResult && positionGuard.restore();
    }

    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedOutput || !guardedSource || !guardedThis->isUnpackOutputSupported(guardedOutput.data()) ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, record.nSize) ||
        (record.baExternalSHA256.size() != 32)) {
        return false;
    }

    // The external-sidecar route bypasses the base decode chain, so it must
    // charge the operation budget itself: one entry, record.nSize produced
    // bytes (the copy loop below reads exactly that much, hash-verified).
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, record.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(record.nSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    QString sCurrentExternalFile;
    if (!asarResolveExternalFile(pContext->sExternalRoot, pContext->sExternalCanonicalRoot, record.sExternalLogicalName, &sCurrentExternalFile) ||
        !asarPathsEqual(sCurrentExternalFile, record.sExternalFileName)) {
        return false;
    }

    QFile sidecarFile(sCurrentExternalFile);
    if (!sidecarFile.open(QIODevice::ReadOnly) || sidecarFile.isSequential() || (sidecarFile.size() != record.nSize) ||
        !asarOpenedFileIsContained(&sidecarFile, pContext->sExternalCanonicalRoot, record.sExternalFileName) ||
        XBinary::devicesAlias(&sidecarFile, guardedOutput.data())) {
        return false;
    }

    QIODevice *pWorkDevice = XBinary::createFileBuffer(record.nSize, pPdStruct);
    if (!pWorkDevice) {
        return false;
    }

    bool bResult = (pWorkDevice->size() == record.nSize) && pWorkDevice->seek(0);
    QByteArray baBuffer;
    if (bResult) {
        baBuffer.resize(1 << 20);
        bResult = (baBuffer.size() == (1 << 20));
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 nReadTotal = 0;
    while (bResult && guardedThis && guardedOutput && guardedSource && (nReadTotal < record.nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRequest = qMin<qint64>(baBuffer.size(), record.nSize - nReadTotal);
        const qint64 nRead = sidecarFile.read(baBuffer.data(), nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) {
            bResult = false;
            break;
        }
        hash.addData(baBuffer.constData(), nRead);
        const qint64 nWritten = guardedThis->safeWriteData(pWorkDevice, nReadTotal, baBuffer.constData(), nRead, pPdStruct);
        if (!guardedThis || !guardedOutput || !guardedSource || (nWritten != nRead)) {
            bResult = false;
            break;
        }
        nReadTotal += nRead;
    }

    if (bResult) {
        bResult = guardedThis && guardedOutput && guardedSource && XBinary::isPdStructNotCanceled(pPdStruct) && (nReadTotal == record.nSize) && sidecarFile.atEnd() &&
                  (sidecarFile.size() == record.nSize) && (pWorkDevice->size() == record.nSize) && (hash.result() == record.baExternalSHA256);
    }
    if (bResult) {
        QString sFinalExternalFile;
        bResult = asarResolveExternalFile(pContext->sExternalRoot, pContext->sExternalCanonicalRoot, record.sExternalLogicalName, &sFinalExternalFile) &&
                  asarPathsEqual(sFinalExternalFile, record.sExternalFileName) &&
                  asarOpenedFileIsContained(&sidecarFile, pContext->sExternalCanonicalRoot, record.sExternalFileName) &&
                  guardedThis->isUnpackSourceCurrent(pState, pPdStruct) && guardedThis && guardedOutput && guardedSource;
    }
    if (bResult) {
        bResult = guardedThis->publishUnpackOutput(pWorkDevice, guardedOutput.data(), pState, pPdStruct);
    }

    XBinary::freeFileBuffer(&pWorkDevice);
    if (bResult && guardedThis && pState) pState->nCurrentOffset = record.nSize;
    return bResult && guardedThis && positionGuard.restore();
}

bool XASAR::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid()) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    pState->nCurrentIndex++;

    const bool bHasNext = pState->nCurrentIndex < pState->nNumberOfRecords;
    return positionGuard.restore() && bHasNext;
}

bool XASAR::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    ASAR_DEVICE_POSITION_GUARD positionGuard(getDevice());
    if (!positionGuard.isValid()) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    ASAR_UNPACK_CONTEXT *pContext = static_cast<ASAR_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    delete pContext;
    Q_UNUSED(guardedThis)
    return positionGuard.restore();
}

bool XASAR::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XASAR::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XASAR> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XASAR::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
