/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "xarchives.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopedPointer>
#include <QSet>
#include <QTemporaryDir>
#include <QTemporaryFile>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <algorithm>
#include <cstring>
#include <limits>

// MyInitGuid must precede the 7-Zip interface declarations in exactly one TU.
#include "Algos/sevenzip_api.h"

// ArchiveExports.cpp intentionally only declares this GUID in the static
// bundle. Define it in the same single INITGUID translation unit as the IIDs.
Z7_DEFINE_GUID(CLSID_CArchiveHandler,
               k_7zip_GUID_Data1,
               k_7zip_GUID_Data2,
               k_7zip_GUID_Data3_Common,
               0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);

STDAPI GetNumberOfFormats(UInt32 *numFormats);
STDAPI GetHandlerProperty2(UInt32 formatIndex, PROPID propID, PROPVARIANT *value);
STDAPI GetIsArc(UInt32 formatIndex, Func_IsArc *isArc);
STDAPI CreateArchiver(const GUID *clsid, const GUID *iid, void **outObject);

namespace {

const char *const UNSUPPORTED_FORMAT_ERROR = "Unsupported archive format";
const qint64 COPY_BUFFER_SIZE = 64 * 1024;
const UInt64 MAXIMUM_SCAN = (UInt64)1 << 23;
const UInt64 MAX_DECODER_MEMORY = (sizeof(void *) <= 4) ? ((UInt64)512 << 20) : ((UInt64)4 << 30);
const UInt64 RAW_BASE64_MAX_INPUT_SIZE = (UInt64)256 << 20;
const UInt32 MAX_ARCHIVE_ITEMS = 100000;
const qint32 MAX_PROPERTY_CHARS = 32768;
const qint64 MAX_METADATA_CHARS = 32LL * 1024 * 1024;

struct ProgressGuard {
    explicit ProgressGuard(XBinary::PDSTRUCT *pValue)
        : pPdStruct(pValue), lifetime(pValue ? XBinary::retainPdStructLifetime(pValue) : XBinary::PDSTRUCTLIFETIME())
    {
    }

    bool canContinue() const
    {
        return !pPdStruct || (XBinary::isPdStructLifetimeAlive(lifetime) && XBinary::isPdStructNotCanceled(pPdStruct));
    }

    XBinary::PDSTRUCT *pPdStruct;
    XBinary::PDSTRUCTLIFETIME lifetime;
};

struct TechnicalEntry {
    QMap<QString, QString> mapValues;
    QString sPath;
    QString sNormalizedPath;
    QString sKey;
    bool bIsFolder = false;
    UInt32 nIndex = 0;
};

struct StagedEntry {
    QString sRelativePath;
    QString sSourcePath;
    bool bIsFolder = false;
    QFile::Permissions permissions;
    QDateTime modified;
};

void setError(QString *pErrorString, const QString &sError)
{
    if (pErrorString) {
        *pErrorString = sError;
    }
}

QString normalizedArchivePath(QString sPath)
{
    sPath = QDir::fromNativeSeparators(sPath);
    while (sPath.endsWith(QLatin1Char('/'))) {
        sPath.chop(1);
    }
    return sPath;
}

bool isReservedName(QString sComponent)
{
    const qint32 nDot = sComponent.indexOf(QLatin1Char('.'));
    if (nDot >= 0) sComponent = sComponent.left(nDot);
    sComponent = sComponent.toUpper();
    if ((sComponent == QLatin1String("CON")) || (sComponent == QLatin1String("PRN")) ||
        (sComponent == QLatin1String("AUX")) || (sComponent == QLatin1String("NUL")) ||
        (sComponent == QLatin1String("CLOCK$")) || (sComponent == QLatin1String("CONIN$")) ||
        (sComponent == QLatin1String("CONOUT$"))) {
        return true;
    }
    if (sComponent.size() == 4) {
        const QString sPrefix = sComponent.left(3);
        const QChar cNumber = sComponent.at(3);
        const ushort nNumber = cNumber.unicode();
        // Win32 treats ISO-8859-1 superscript 1, 2 and 3 as device-name
        // digits too, so COM¹/LPT² and their extension forms are reserved.
        const bool bDeviceNumber =
            ((cNumber >= QLatin1Char('1')) &&
             (cNumber <= QLatin1Char('9'))) ||
            (nNumber == 0x00B9) || (nNumber == 0x00B2) ||
            (nNumber == 0x00B3);
        if (((sPrefix == QLatin1String("COM")) || (sPrefix == QLatin1String("LPT"))) &&
            bDeviceNumber) {
            return true;
        }
    }
    return false;
}

bool safeRelativePath(const QString &sInput, QString *pNormalized)
{
    const QString sPath = normalizedArchivePath(sInput);
    if (sPath.isEmpty() || sPath.startsWith(QLatin1Char('/')) || sPath.startsWith(QStringLiteral("//")) ||
        QDir::isAbsolutePath(sPath) || ((sPath.size() >= 2) && sPath.at(0).isLetter() && (sPath.at(1) == QLatin1Char(':')))) {
        return false;
    }

    const QStringList listParts = sPath.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String("..")) ||
            sPart.contains(QLatin1Char(':')) || sPart.contains(QRegularExpression(QStringLiteral("[<>\"|?*]"))) ||
            sPart.endsWith(QLatin1Char('.')) || sPart.endsWith(QLatin1Char(' ')) ||
            isReservedName(sPart)) {
            return false;
        }
        for (const QChar c : sPart) {
            if ((c.unicode() < 0x20) || (c.unicode() == 0x7f)) return false;
        }
    }
    if (QDir::cleanPath(sPath) != sPath) return false;
    if (pNormalized) *pNormalized = sPath;
    return true;
}

// Normalizes an archive ENTRY path for extraction.
//
// safeRelativePath() above stays strict because it also guards opening sibling
// files on disk for multi-volume archives.  Entry paths are different: an
// archive is allowed to be merely non-canonical without being dangerous.
// GNU tar stores "." and "./name", cpio and rpm commonly store absolute paths
// like "/home/user/file", and DOS-era archivers store "C:\name".  7-Zip,
// bsdtar and unzip all strip those prefixes and extract relative to the
// destination; refusing the whole archive instead loses ordinary data.
//
// Genuinely dangerous shapes are still rejected: ".." traversal, Windows
// reserved device names, control characters, and characters illegal in a path.
//
// *pSanitized may come back empty, which means the entry names the destination
// root itself (tar's leading ".") and carries no payload.  Callers must skip
// such entries rather than treat the empty string as a filename.
bool sanitizedArchiveEntryPath(const QString &sInput, QString *pSanitized)
{
    QString sPath = normalizedArchivePath(sInput);

    // Drop a drive-letter prefix ("C:" / "C:/").
    if ((sPath.size() >= 2) && sPath.at(0).isLetter() && (sPath.at(1) == QLatin1Char(':'))) {
        sPath = sPath.mid(2);
    }
    // Drop any leading separators, including the "//" of a UNC-style path.
    while (sPath.startsWith(QLatin1Char('/'))) {
        sPath.remove(0, 1);
    }

    QStringList listSafe;
    const QStringList listParts = sPath.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        // Empty components come from duplicated separators; "." is a no-op.
        if (sPart.isEmpty() || (sPart == QLatin1String("."))) continue;
        // Traversal is never sanitized away -- it is refused.
        if (sPart == QLatin1String("..")) return false;
        if (sPart.contains(QLatin1Char(':')) || sPart.contains(QRegularExpression(QStringLiteral("[<>\"|?*]"))) ||
            sPart.endsWith(QLatin1Char('.')) || sPart.endsWith(QLatin1Char(' ')) || isReservedName(sPart)) {
            return false;
        }
        for (const QChar c : sPart) {
            if ((c.unicode() < 0x20) || (c.unicode() == 0x7f)) return false;
        }
        listSafe.append(sPart);
    }

    const QString sResult = listSafe.join(QLatin1Char('/'));
    // Defence in depth: the rebuilt path must already be canonical.
    if (!sResult.isEmpty() && (QDir::cleanPath(sResult) != sResult)) return false;
    if (pSanitized) *pSanitized = sResult;
    return true;
}

bool isContainedPath(const QString &sPath, const QString &sRoot)
{
    const QString sCleanPath = QDir::cleanPath(QFileInfo(sPath).absoluteFilePath());
    const QString sCleanRoot = QDir::cleanPath(QFileInfo(sRoot).absoluteFilePath());
    const QString sPrefix = sCleanRoot + QLatin1Char('/');
#ifdef Q_OS_WIN
    return (sCleanPath.compare(sCleanRoot, Qt::CaseInsensitive) == 0) || sCleanPath.startsWith(sPrefix, Qt::CaseInsensitive);
#else
    return (sCleanPath == sCleanRoot) || sCleanPath.startsWith(sPrefix);
#endif
}

UString toUString(const QString &sValue)
{
    const std::wstring value = sValue.toStdWString();
    return UString(value.c_str());
}

QString fromBstr(BSTR value)
{
    return value ? QString::fromWCharArray(value, (int)::SysStringLen(value)) : QString();
}

bool propToUInt64(const PROPVARIANT &prop, quint64 *pValue)
{
    quint64 nValue = 0;
    switch (prop.vt) {
        case VT_UI1: nValue = prop.bVal; break;
        case VT_UI2: nValue = prop.uiVal; break;
        case VT_UI4: nValue = prop.ulVal; break;
        case VT_UI8: nValue = prop.uhVal.QuadPart; break;
        case VT_I1: if (prop.cVal < 0) return false; nValue = (quint64)prop.cVal; break;
        case VT_I2: if (prop.iVal < 0) return false; nValue = (quint64)prop.iVal; break;
        case VT_I4: if (prop.lVal < 0) return false; nValue = (quint64)prop.lVal; break;
        case VT_I8: if (prop.hVal.QuadPart < 0) return false; nValue = (quint64)prop.hVal.QuadPart; break;
        default: return false;
    }
    if (pValue) *pValue = nValue;
    return true;
}

bool propToBool(const PROPVARIANT &prop, bool *pValue)
{
    if (prop.vt == VT_BOOL) {
        if (pValue) *pValue = (prop.boolVal != VARIANT_FALSE);
        return true;
    }
    if (prop.vt == VT_EMPTY) {
        if (pValue) *pValue = false;
        return true;
    }
    return false;
}

QDateTime fileTimeToDateTime(const FILETIME &value)
{
    const quint64 nTicks = ((quint64)value.dwHighDateTime << 32) | value.dwLowDateTime;
    const qint64 nUnixMilliseconds = (qint64)(nTicks / 10000ULL) - 11644473600000LL;
    return QDateTime::fromMSecsSinceEpoch(nUnixMilliseconds, Qt::UTC);
}

QString operationResultText(Int32 nResult)
{
    using namespace NArchive::NExtract::NOperationResult;
    switch (nResult) {
        case kOK: return QString();
        case kUnsupportedMethod: return QStringLiteral("Unsupported compression method");
        case kDataError: return QStringLiteral("Archive data error or wrong password");
        case kCRCError: return QStringLiteral("Archive CRC error or wrong password");
        case kUnavailable: return QStringLiteral("Archive data is unavailable");
        case kUnexpectedEnd: return QStringLiteral("Unexpected end of archive data");
        case kDataAfterEnd: return QStringLiteral("Unexpected data after archive payload");
        case kIsNotArc: return QString::fromLatin1(UNSUPPORTED_FORMAT_ERROR);
        case kHeadersError: return QStringLiteral("Archive headers error or wrong password");
        case kWrongPassword: return QStringLiteral("Wrong archive password");
        default: return QStringLiteral("Archive extraction error %1").arg(nResult);
    }
}

class QtInStream final : public IInStream, public IStreamGetSize, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(IInStream, ISequentialInStream, IStreamGetSize)
    Z7_IFACE_COM7_IMP(ISequentialInStream)
    Z7_IFACE_COM7_IMP(IInStream)
    Z7_IFACE_COM7_IMP(IStreamGetSize)

public:
    QtInStream(const QString &sFileName, XBinary::PDSTRUCT *pPdStruct)
        : m_sFileName(sFileName), m_progress(pPdStruct)
    {
    }

    bool open()
    {
        m_file.setFileName(m_sFileName);
        return m_file.open(QIODevice::ReadOnly);
    }

private:
    QString m_sFileName;
    QFile m_file;
    QMutex m_mutex;
    ProgressGuard m_progress;
};

Z7_COM7F_IMF(QtInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
    if (processedSize) *processedSize = 0;
    if (!m_progress.canContinue()) return E_ABORT;
    if (!size) return S_OK;
    QMutexLocker locker(&m_mutex);
    const qint64 nRead = m_file.read((char *)data, size);
    if (nRead < 0) return E_FAIL;
    if (processedSize) *processedSize = (UInt32)nRead;
    return S_OK;
}

Z7_COM7F_IMF(QtInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
    if (!m_progress.canContinue()) return E_ABORT;
    QMutexLocker locker(&m_mutex);
    qint64 nBase = 0;
    if (seekOrigin == STREAM_SEEK_CUR) nBase = m_file.pos();
    else if (seekOrigin == STREAM_SEEK_END) nBase = m_file.size();
    else if (seekOrigin != STREAM_SEEK_SET) return STG_E_INVALIDFUNCTION;
    if ((offset < 0 && offset < -nBase) || (offset > 0 && nBase > std::numeric_limits<qint64>::max() - offset)) {
        return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
    }
    const qint64 nPosition = nBase + offset;
    if (nPosition < 0) return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
    if (!m_file.seek(nPosition)) return E_FAIL;
    if (newPosition) *newPosition = (UInt64)nPosition;
    return S_OK;
}

Z7_COM7F_IMF(QtInStream::GetSize(UInt64 *size))
{
    if (!size) return E_INVALIDARG;
    QMutexLocker locker(&m_mutex);
    const qint64 nSize = m_file.size();
    if (nSize < 0) return E_FAIL;
    *size = (UInt64)nSize;
    return S_OK;
}

class QtOutStream final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_IFACES_IMP_UNK_1(ISequentialOutStream)

public:
    QtOutStream(QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
        : m_pDevice(pDevice), m_progress(pPdStruct)
    {
    }

private:
    QIODevice *m_pDevice;
    QMutex m_mutex;
    ProgressGuard m_progress;
};

class LimitedWriteDevice final : public QIODevice {
public:
    LimitedWriteDevice(QIODevice *pDestination, qint64 nLimit)
        : m_pDestination(pDestination), m_nLimit(nLimit)
    {
        open(QIODevice::WriteOnly);
    }

    bool isSequential() const override { return true; }
    bool isLimitExceeded() const { return m_bLimitExceeded; }

protected:
    qint64 readData(char *, qint64) override { return -1; }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        if (!m_pDestination || !m_pDestination->isOpen() || !m_pDestination->isWritable() || (nSize < 0) || ((nSize > 0) && !pData)) {
            return -1;
        }

        if ((m_nWritten > m_nLimit) || (nSize > (m_nLimit - m_nWritten))) {
            m_bLimitExceeded = true;
            return -1;
        }

        const qint64 nWritten = m_pDestination->write(pData, nSize);

        if (nWritten > 0) {
            m_nWritten += nWritten;
        }

        return nWritten;
    }

private:
    QIODevice *m_pDestination = nullptr;
    qint64 m_nLimit = 0;
    qint64 m_nWritten = 0;
    bool m_bLimitExceeded = false;
};

Z7_COM7F_IMF(QtOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
    if (processedSize) *processedSize = 0;
    if (!m_progress.canContinue()) return E_ABORT;
    if (!m_pDevice || !m_pDevice->isWritable()) return E_FAIL;
    if (!size) return S_OK;
    QMutexLocker locker(&m_mutex);
    if (!data) return E_INVALIDARG;
    UInt32 nTotalWritten = 0;
    while (nTotalWritten < size) {
        if (!m_progress.canContinue()) return E_ABORT;
        const qint64 nWritten = m_pDevice->write((const char *)data + nTotalWritten, size - nTotalWritten);
        if (nWritten <= 0) return E_FAIL;
        nTotalWritten += (UInt32)nWritten;
    }
    if (processedSize) *processedSize = nTotalWritten;
    return S_OK;
}

class ArchiveOpenCallback final : public IArchiveOpenCallback,
                                  public IArchiveOpenVolumeCallback,
                                  public ICryptoGetTextPassword,
                                  public CMyUnknownImp {
    Z7_IFACES_IMP_UNK_3(IArchiveOpenCallback, IArchiveOpenVolumeCallback, ICryptoGetTextPassword)

public:
    ArchiveOpenCallback(const QString &sPhysicalFileName, const QString &sReportedName, bool bSubArchive,
                        const QString &sPassword, XBinary::PDSTRUCT *pPdStruct)
        : m_sPhysicalFileName(QFileInfo(sPhysicalFileName).absoluteFilePath()),
          m_sFolder(QFileInfo(sPhysicalFileName).absolutePath()),
          m_sReportedName(sReportedName),
          m_bSubArchive(bSubArchive),
          m_password(toUString(sPassword)),
          m_progress(pPdStruct)
    {
    }

    bool passwordWasRequested() const { return m_bPasswordWasRequested; }
    bool isCanceled() const { return !m_progress.canContinue(); }

private:
    QString m_sPhysicalFileName;
    QString m_sFolder;
    QString m_sReportedName;
    bool m_bSubArchive = false;
    UString m_password;
    ProgressGuard m_progress;
    bool m_bPasswordWasRequested = false;
};

Z7_COM7F_IMF(ArchiveOpenCallback::SetTotal(const UInt64 *, const UInt64 *))
{
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveOpenCallback::SetCompleted(const UInt64 *, const UInt64 *))
{
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveOpenCallback::GetProperty(PROPID propID, PROPVARIANT *value))
{
    NWindows::NCOM::CPropVariant prop;
    if (propID == kpidName) prop = toUString(m_sReportedName);
    else if (!m_bSubArchive) {
        const QFileInfo info(m_sPhysicalFileName);
        if (propID == kpidSize) prop = (UInt64)qMax<qint64>(0, info.size());
        else if (propID == kpidIsDir) prop = false;
    }
    prop.Detach(value);
    return S_OK;
}

Z7_COM7F_IMF(ArchiveOpenCallback::GetStream(const wchar_t *name, IInStream **inStream))
{
    if (!inStream) return E_INVALIDARG;
    *inStream = nullptr;
    if (m_bSubArchive) return S_FALSE;
    if (!m_progress.canContinue() || !name) return E_ABORT;
    QString sRelative = QString::fromWCharArray(name);
    QString sSafe;
    if (!safeRelativePath(sRelative, &sSafe)) return S_FALSE;
    // Official multi-volume handlers request sibling files. Restricting the
    // callback to one basename also prevents traversal through a junction in
    // an attacker-controlled subdirectory next to the archive.
    if (sSafe.contains(QLatin1Char('/'))) return S_FALSE;
    const QString sPath = QDir(m_sFolder).filePath(sSafe);
    if (!isContainedPath(sPath, m_sFolder)) return S_FALSE;
    const QFileInfo info(sPath);
    if (!info.isFile() || info.isSymLink()) return S_FALSE;
#ifdef Q_OS_WIN
    const DWORD nAttributes =
        GetFileAttributesW((LPCWSTR)XBinary::winExtendedNativePath(info.absoluteFilePath()).utf16());
    if (nAttributes == INVALID_FILE_ATTRIBUTES || (nAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return S_FALSE;
#endif
    QtInStream *pSpec = new QtInStream(info.absoluteFilePath(), m_progress.pPdStruct);
    CMyComPtr<IInStream> stream = pSpec;
    if (!pSpec->open()) return S_FALSE;
    *inStream = stream.Detach();
    return S_OK;
}

Z7_COM7F_IMF(ArchiveOpenCallback::CryptoGetTextPassword(BSTR *password))
{
    if (!password) return E_INVALIDARG;
    m_bPasswordWasRequested = true;
    if (!m_progress.canContinue()) return E_ABORT;
    return StringToBstr(m_password, password);
}

struct FormatInfo {
    UInt32 nIndex = 0;
    GUID classId = {};
    QString sName;
    QString sExtensions;
    UInt32 nFlags = 0;
    UInt64 nStreamOffset = 0;
    bool bExtensionMatch = false;
    qint32 nDetectionRank = 5;
};

QString handlerString(UInt32 nIndex, PROPID propId)
{
    NWindows::NCOM::CPropVariant prop;
    if (GetHandlerProperty2(nIndex, propId, &prop) != S_OK || prop.vt != VT_BSTR) return QString();
    return fromBstr(prop.bstrVal);
}

bool handlerUInt32(UInt32 nIndex, PROPID propId, UInt32 *pValue)
{
    NWindows::NCOM::CPropVariant prop;
    if (GetHandlerProperty2(nIndex, propId, &prop) != S_OK || prop.vt != VT_UI4) return false;
    if (pValue) *pValue = prop.ulVal;
    return true;
}

QByteArray handlerBinary(UInt32 nIndex, PROPID propId)
{
    NWindows::NCOM::CPropVariant prop;
    if (GetHandlerProperty2(nIndex, propId, &prop) != S_OK || prop.vt != VT_BSTR || !prop.bstrVal) return QByteArray();
    const UINT nSize = ::SysStringByteLen(prop.bstrVal);
    if (nSize > 4096) return QByteArray();
    return QByteArray(reinterpret_cast<const char *>(prop.bstrVal), (int)nSize);
}

QList<QByteArray> handlerSignatures(UInt32 nIndex, UInt32 nFlags)
{
    QList<QByteArray> result;
    if (!(nFlags & NArcInfoFlags::kMultiSignature)) {
        const QByteArray signature = handlerBinary(nIndex, NArchive::NHandlerPropID::kSignature);
        if (!signature.isEmpty()) result.append(signature);
        return result;
    }
    const QByteArray signatures = handlerBinary(nIndex, NArchive::NHandlerPropID::kMultiSignature);
    qint32 nOffset = 0;
    while (nOffset < signatures.size()) {
        const qint32 nLength = (quint8)signatures.at(nOffset++);
        if (!nLength || nLength > signatures.size() - nOffset) {
            result.clear();
            return result;
        }
        result.append(signatures.mid(nOffset, nLength));
        nOffset += nLength;
    }
    return result;
}

bool handlerClassId(UInt32 nIndex, GUID *pClassId)
{
    NWindows::NCOM::CPropVariant prop;
    if (!pClassId || GetHandlerProperty2(nIndex, NArchive::NHandlerPropID::kClassID, &prop) != S_OK ||
        prop.vt != VT_BSTR || ::SysStringByteLen(prop.bstrVal) != sizeof(GUID)) {
        return false;
    }
    memcpy(pClassId, prop.bstrVal, sizeof(GUID));
    return true;
}

bool extensionMatches(const QString &sFileName, const QString &sExtensions)
{
    const QString sLowerName = QFileInfo(sFileName).fileName().toLower();
    const QStringList listExtensions = sExtensions.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (QString sExtension : listExtensions) {
        if (sExtension.startsWith(QLatin1String("*."))) sExtension.remove(0, 2);
        else if (sExtension.startsWith(QLatin1Char('.'))) sExtension.remove(0, 1);
        else if (sExtension.startsWith(QLatin1Char('*'))) sExtension.remove(0, 1);
        if (!sExtension.isEmpty() && sLowerName.endsWith(QLatin1Char('.') + sExtension)) return true;
    }
    return false;
}

bool isSplitStartName(const QString &sFileName)
{
    const QString sLowerName = QFileInfo(sFileName).fileName().toLower();
    const qint32 nDot = sLowerName.lastIndexOf(QLatin1Char('.'));
    const QString sExtension =
        (nDot >= 0) ? sLowerName.mid(nDot + 1) : sLowerName;
    if (sExtension.size() < 2) return false;

    // Mirror SplitHandler's accepted first-volume syntax rather than only its
    // advertised "001" hint. Alphabetic sequences start with a run ending in
    // at least two 'a' characters (aa, aaa, partaa, ...). Numeric sequences
    // are at least two digits wide, all leading zeroes, and end in 0 or 1.
    if (sExtension.endsWith(QLatin1String("aa"))) return true;
    if (!sExtension.endsWith(QLatin1Char('0')) &&
        !sExtension.endsWith(QLatin1Char('1'))) {
        return false;
    }
    for (qint32 i = 0; i < sExtension.size() - 1; i++) {
        if (sExtension.at(i) != QLatin1Char('0')) return false;
    }
    return true;
}

struct StreamProbe {
    QByteArray prefix;
    UInt64 nSize = 0;
    bool bAtEnd = false;
};

HRESULT readStreamProbe(IInStream *pStream, StreamProbe *pProbe, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pStream || !pProbe) return E_INVALIDARG;
    pProbe->prefix.clear();
    pProbe->nSize = 0;
    pProbe->bAtEnd = false;
    ProgressGuard progress(pPdStruct);
    if (!progress.canContinue()) return E_ABORT;

    UInt64 nOldPosition = 0;
    HRESULT nResult = pStream->Seek(0, STREAM_SEEK_CUR, &nOldPosition);
    if (nResult != S_OK || nOldPosition > (UInt64)std::numeric_limits<Int64>::max()) return nResult == S_OK ? E_FAIL : nResult;

    UInt64 nSize = 0;
    CMyComPtr<IStreamGetSize> sizeStream;
    if (pStream->QueryInterface(IID_IStreamGetSize, (void **)&sizeStream) == S_OK && sizeStream) {
        nResult = sizeStream->GetSize(&nSize);
    } else {
        nResult = pStream->Seek(0, STREAM_SEEK_END, &nSize);
    }
    if (nResult == S_OK) nResult = pStream->Seek(0, STREAM_SEEK_SET, nullptr);
    if (nResult == S_OK) pProbe->nSize = nSize;

    HRESULT nWorkResult = nResult;
    if (nWorkResult == S_OK) {
        const qint32 nWanted = (qint32)qMin<UInt64>(nSize, MAXIMUM_SCAN);
        pProbe->prefix.resize(nWanted);
        qint32 nTotal = 0;
        while (nTotal < nWanted) {
            if (!progress.canContinue()) {
                nWorkResult = E_ABORT;
                break;
            }
            UInt32 nRead = 0;
            const HRESULT nReadResult = pStream->Read(pProbe->prefix.data() + nTotal, (UInt32)(nWanted - nTotal), &nRead);
            if ((nReadResult != S_OK && nReadResult != S_FALSE) || nRead > (UInt32)(nWanted - nTotal)) {
                nWorkResult = nReadResult == S_OK ? E_FAIL : nReadResult;
                break;
            }
            nTotal += (qint32)nRead;
            if (!nRead || nReadResult == S_FALSE) break;
        }
        pProbe->prefix.resize(nTotal);
        pProbe->bAtEnd = (UInt64)nTotal == nSize;
    }

    const HRESULT nRestoreResult = pStream->Seek((Int64)nOldPosition, STREAM_SEEK_SET, nullptr);
    if (nWorkResult != S_OK) return nWorkResult;
    return nRestoreResult;
}

QList<FormatInfo> enumerateFormats(const QString &sFileName, const StreamProbe &probe)
{
    QList<FormatInfo> result;
    const QByteArray &prefix = probe.prefix;
    const bool bAtEnd = probe.bAtEnd;
    UInt32 nFormats = 0;
    if (GetNumberOfFormats(&nFormats) != S_OK) return result;
    for (UInt32 i = 0; i < nFormats; i++) {
        FormatInfo info;
        info.nIndex = i;
        if (!handlerClassId(i, &info.classId)) continue;
        info.sName = handlerString(i, NArchive::NHandlerPropID::kName);
        info.sExtensions = handlerString(i, NArchive::NHandlerPropID::kExtension);
        handlerUInt32(i, NArchive::NHandlerPropID::kFlags, &info.nFlags);
        info.bExtensionMatch = extensionMatches(sFileName, info.sExtensions);
        if ((info.sName == QLatin1String("Split")) &&
            isSplitStartName(sFileName)) {
            info.bExtensionMatch = true;
        }
        if ((info.nFlags & NArcInfoFlags::kByExtOnlyOpen) && !info.bExtensionMatch) continue;

        UInt32 nProbeResult = k_IsArc_Res_NO;
        Func_IsArc isArc = nullptr;
        const bool bHasProbe = GetIsArc(i, &isArc) == S_OK && isArc;
        if (bHasProbe) {
            nProbeResult = isArc(reinterpret_cast<const Byte *>(prefix.constData()), (size_t)prefix.size());
        }

        UInt32 nSignatureOffset = 0;
        handlerUInt32(i, NArchive::NHandlerPropID::kSignatureOffset, &nSignatureOffset);
        bool bFixedSignature = false;
        bool bEmbeddedSignature = false;
        const QList<QByteArray> signatures = handlerSignatures(i, info.nFlags);
        for (const QByteArray &signature : signatures) {
            if (signature.isEmpty()) continue;
            if (nSignatureOffset <= (UInt32)prefix.size() &&
                signature.size() <= prefix.size() - (qint32)nSignatureOffset &&
                memcmp(prefix.constData() + nSignatureOffset, signature.constData(), (size_t)signature.size()) == 0) {
                bFixedSignature = true;
            }
            qint32 nPosition = prefix.indexOf(signature, (qint32)qMin<UInt32>(nSignatureOffset, (UInt32)std::numeric_limits<qint32>::max()));
            while (nPosition >= 0) {
                const UInt64 nArchiveOffset = (UInt64)(UInt32)nPosition - nSignatureOffset;
                if (nArchiveOffset > 0 && nArchiveOffset <= MAXIMUM_SCAN) {
                    bool bPlausible = true;
                    if (bHasProbe) {
                        const size_t nAvailable = (size_t)prefix.size() - (size_t)nArchiveOffset;
                        const UInt32 nEmbeddedProbe = isArc(
                            reinterpret_cast<const Byte *>(prefix.constData()) + (size_t)nArchiveOffset,
                            nAvailable);
                        bPlausible = (nEmbeddedProbe == k_IsArc_Res_YES) ||
                                     (nEmbeddedProbe == k_IsArc_Res_NEED_MORE && !bAtEnd);
                    }
                    if (bPlausible) {
                        bEmbeddedSignature = true;
                        // Handlers with kFindSignature already consume the
                        // complete stream and apply their own search limit.
                        // Other handlers (notably gzip) require a virtual
                        // stream rooted at the detected archive start.
                        if (!(info.nFlags & NArcInfoFlags::kFindSignature) &&
                            (!info.nStreamOffset || nArchiveOffset < info.nStreamOffset)) {
                            info.nStreamOffset = nArchiveOffset;
                        }
                        break;
                    }
                }
                nPosition = prefix.indexOf(signature, nPosition + 1);
            }
        }

        const bool bExactSplitExtension =
            info.bExtensionMatch && (info.sName == QLatin1String("Split"));
        if (bFixedSignature || (bHasProbe && nProbeResult == k_IsArc_Res_YES) ||
            bExactSplitExtension) {
            // A valid archive at the root always wins over matching byte
            // sequences that happen to occur later inside its payload. Split
            // volumes are the one extension-led peer at this rank: a numeric
            // zero/one start or alphabetic "aa" start can contain a valid ZIP
            // signature while the central directory lives in a sibling part.
            // The start-name tie-break tries Split before the truncated root.
            info.nStreamOffset = 0;
            info.nDetectionRank = 0;
        }
        else if (info.bExtensionMatch) info.nDetectionRank = 1;
        else if (bEmbeddedSignature) info.nDetectionRank = 2;
        else if (bHasProbe && nProbeResult == k_IsArc_Res_NEED_MORE && !bAtEnd) info.nDetectionRank = 3;
        else if (info.nFlags & NArcInfoFlags::kFindSignature) info.nDetectionRank = 4;
        result.append(info);
    }
    std::stable_sort(result.begin(), result.end(), [](const FormatInfo &a, const FormatInfo &b) {
        if (a.nDetectionRank != b.nDetectionRank) return a.nDetectionRank < b.nDetectionRank;
        if (a.bExtensionMatch != b.bExtensionMatch) return a.bExtensionMatch;
        return a.nIndex < b.nIndex;
    });
    return result;
}

struct ArchiveLayer {
    CMyComPtr<IInArchive> archive;
    CMyComPtr<IInStream> stream;
    CMyComPtr<IArchiveOpenCallback> callback;
    QString sFormat;
    QString sDisplayName;
};

struct OpenedArchive {
    CMyComPtr<IInArchive> archive;
    CMyComPtr<IInStream> stream;
    CMyComPtr<IArchiveOpenCallback> callback;
    QString sFormat;
    QString sDisplayName;
    QList<ArchiveLayer> parents;

    OpenedArchive() = default;

    void setLeaf(const ArchiveLayer &layer)
    {
        archive = layer.archive;
        stream = layer.stream;
        callback = layer.callback;
        sFormat = layer.sFormat;
        sDisplayName = layer.sDisplayName;
    }

    void pushLeaf(const ArchiveLayer &layer)
    {
        ArchiveLayer parent;
        parent.archive = archive;
        parent.stream = stream;
        parent.callback = callback;
        parent.sFormat = sFormat;
        parent.sDisplayName = sDisplayName;
        parents.append(parent);
        setLeaf(layer);
    }

    void close()
    {
        if (archive) archive->Close();
        for (qint32 i = parents.size() - 1; i >= 0; i--) {
            if (parents.at(i).archive) parents.at(i).archive->Close();
        }
        archive.Release();
        stream.Release();
        callback.Release();
        parents.clear();
        sFormat.clear();
        sDisplayName.clear();
    }

    ~OpenedArchive() { close(); }

private:
    Q_DISABLE_COPY(OpenedArchive)
};

quint64 archiveErrorFlags(IInArchive *pArchive);

constexpr bool isExactArchiveErrorFlag(quint64 nFlags,
                                       quint64 nExpectedFlag)
{
    return nFlags == nExpectedFlag;
}

static_assert(isExactArchiveErrorFlag(kpv_ErrorFlags_IsNotArc,
                                      kpv_ErrorFlags_IsNotArc),
              "A pure IsNotArc flag must remain eligible");
static_assert(!isExactArchiveErrorFlag(
                  kpv_ErrorFlags_IsNotArc | kpv_ErrorFlags_HeadersError,
                  kpv_ErrorFlags_IsNotArc),
              "Composite IsNotArc flags must remain terminal");
static_assert(!isExactArchiveErrorFlag(
                  kpv_ErrorFlags_UnsupportedMethod | kpv_ErrorFlags_DataError,
                  kpv_ErrorFlags_UnsupportedMethod),
              "Composite unsupported-method flags must remain terminal");

QString archiveFatalError(IInArchive *pArchive)
{
    if (!pArchive) return QStringLiteral("Archive open failed");
    const quint64 nFlags = archiveErrorFlags(pArchive);
    if (!nFlags) return QString();
    // Only a pure unsupported state may authorize native fallback. Composite
    // masks can also describe corruption or incomplete input and stay terminal.
    if (isExactArchiveErrorFlag(nFlags, kpv_ErrorFlags_IsNotArc)) return QString::fromLatin1(UNSUPPORTED_FORMAT_ERROR);
    if (nFlags & kpv_ErrorFlags_EncryptedHeadersError) return QStringLiteral("Wrong archive password or encrypted headers error");
    if (nFlags & kpv_ErrorFlags_HeadersError) return QStringLiteral("Archive headers error");
    if (nFlags & kpv_ErrorFlags_UnexpectedEnd) return QStringLiteral("Unexpected end of archive");
    if (isExactArchiveErrorFlag(nFlags, kpv_ErrorFlags_UnsupportedMethod)) return QStringLiteral("Unsupported archive method");
    if (nFlags & kpv_ErrorFlags_UnsupportedFeature) return QStringLiteral("Unsupported archive feature");
    if (nFlags & kpv_ErrorFlags_DataError) return QStringLiteral("Archive data error");
    if (nFlags & kpv_ErrorFlags_CrcError) return QStringLiteral("Archive CRC error");
    return QStringLiteral("Archive error flags 0x%1").arg(QString::number(nFlags, 16));
}

quint64 archiveErrorFlags(IInArchive *pArchive)
{
    NWindows::NCOM::CPropVariant prop;
    if (!pArchive || pArchive->GetArchiveProperty(kpidErrorFlags, &prop) != S_OK) return 0;
    quint64 nFlags = 0;
    return propToUInt64(prop, &nFlags) ? nFlags : 0;
}

bool itemString(IInArchive *pArchive, UInt32 nIndex, PROPID propId, QString *pValue);

enum class OpenStatus {
    Opened,
    Unsupported,
    Fatal,
    Canceled
};

OpenStatus openOneStream(const CMyComPtr<IInStream> &stream, const QString &sLogicalName, const QString &sPhysicalFileName,
                         bool bSubArchive, const QString &sPassword, ArchiveLayer *pLayer, QString *pErrorString,
                         XBinary::PDSTRUCT *pPdStruct)
{
    if (!stream || !pLayer) {
        setError(pErrorString, QStringLiteral("Invalid archive stream"));
        return OpenStatus::Fatal;
    }
    StreamProbe probe;
    const HRESULT nProbeResult = readStreamProbe(stream, &probe, pPdStruct);
    if (nProbeResult != S_OK) {
        if (nProbeResult == E_ABORT || !ProgressGuard(pPdStruct).canContinue()) {
            setError(pErrorString, QStringLiteral("Archive operation canceled"));
            return OpenStatus::Canceled;
        }
        setError(pErrorString, QStringLiteral("Cannot inspect archive stream (HRESULT 0x%1)")
                                   .arg((quint32)nProbeResult, 8, 16, QLatin1Char('0')));
        return OpenStatus::Fatal;
    }

    const QList<FormatInfo> formats = enumerateFormats(sLogicalName, probe);
    QString sDeferredError;
    for (const FormatInfo &format : formats) {
        ProgressGuard progress(pPdStruct);
        if (!progress.canContinue()) {
            setError(pErrorString, QStringLiteral("Archive operation canceled"));
            return OpenStatus::Canceled;
        }
        const HRESULT nSeekResult = stream->Seek(0, STREAM_SEEK_SET, nullptr);
        if (nSeekResult != S_OK) {
            if (nSeekResult == E_ABORT || !progress.canContinue()) {
                setError(pErrorString, QStringLiteral("Archive operation canceled"));
                return OpenStatus::Canceled;
            }
            setError(pErrorString, QStringLiteral("Cannot reset archive stream"));
            return OpenStatus::Fatal;
        }

        CMyComPtr<IInStream> candidateStream = stream;
        if (format.nStreamOffset) {
            if (format.nStreamOffset >= probe.nSize) continue;
            CLimitedInStream *pLimitedSpec = new CLimitedInStream;
            CMyComPtr<IInStream> limitedStream = pLimitedSpec;
            pLimitedSpec->SetStream(stream);
            const HRESULT nLimitedResult =
                pLimitedSpec->InitAndSeek(format.nStreamOffset, probe.nSize - format.nStreamOffset);
            if (nLimitedResult != S_OK) {
                if (nLimitedResult == E_ABORT || !progress.canContinue()) {
                    setError(pErrorString, QStringLiteral("Archive operation canceled"));
                    return OpenStatus::Canceled;
                }
                setError(pErrorString, QStringLiteral("Cannot position embedded archive stream"));
                return OpenStatus::Fatal;
            }
            candidateStream = limitedStream;
        }

        // The raw Base64 handler retains the complete encoded stream during
        // Open(), before extraction-time decoder limits can apply. Reject an
        // oversized extension-led input before handing it to that allocator.
        if ((format.sName == QLatin1String("Base64")) &&
            ((format.nStreamOffset > probe.nSize) ||
             ((probe.nSize - format.nStreamOffset) >
              RAW_BASE64_MAX_INPUT_SIZE))) {
            setError(pErrorString,
                     QStringLiteral("Raw Base64 input exceeds the 256 MiB safety limit"));
            return OpenStatus::Fatal;
        }

        CMyComPtr<IInArchive> archive;
        IInArchive *pRawArchive = nullptr;
        const HRESULT nCreateResult = CreateArchiver(&format.classId, &IID_IInArchive, (void **)&pRawArchive);
        if (nCreateResult != S_OK || !pRawArchive) continue;
        archive.Attach(pRawArchive);  // CreateArchiver already returned one owned reference.

        ArchiveOpenCallback *pCallbackSpec =
            new ArchiveOpenCallback(sPhysicalFileName, sLogicalName, bSubArchive, sPassword, pPdStruct);
        CMyComPtr<IArchiveOpenCallback> callback = pCallbackSpec;
        const UInt64 nMaximumScan = MAXIMUM_SCAN;
        const HRESULT nOpenResult = archive->Open(candidateStream, &nMaximumScan, callback);
        if (nOpenResult == S_OK) {
            // A pure false-positive signal may try the next handler. Any
            // composite error mask must reach the terminal-error mapping.
            if (isExactArchiveErrorFlag(archiveErrorFlags(archive),
                                        kpv_ErrorFlags_IsNotArc)) {
                archive->Close();
                continue;
            }
            const QString sFatal = archiveFatalError(archive);
            if (!sFatal.isEmpty()) {
                archive->Close();
                if (format.nDetectionRank <= 1) {
                    setError(pErrorString, sFatal);
                    return OpenStatus::Fatal;
                }
                if (sDeferredError.isEmpty()) sDeferredError = sFatal;
                continue;
            }
            pLayer->archive = archive;
            pLayer->stream = candidateStream;
            pLayer->callback = callback;
            pLayer->sFormat = format.sName;
            pLayer->sDisplayName = sLogicalName;
            return OpenStatus::Opened;
        }

        archive->Close();
        if (pCallbackSpec->isCanceled() || nOpenResult == E_ABORT) {
            setError(pErrorString, QStringLiteral("Archive operation canceled"));
            return OpenStatus::Canceled;
        }
        if (pCallbackSpec->passwordWasRequested()) {
            const QString sPasswordError = QStringLiteral("Wrong archive password or damaged encrypted archive");
            if (format.nDetectionRank <= 1) {
                setError(pErrorString, sPasswordError);
                return OpenStatus::Fatal;
            }
            if (sDeferredError.isEmpty()) sDeferredError = sPasswordError;
        }
        if (nOpenResult != S_FALSE) {
            QString sOpenError;
            if (nOpenResult == E_OUTOFMEMORY) sOpenError = QStringLiteral("Archive requires too much decoder memory");
            else if (nOpenResult == E_NOTIMPL) sOpenError = QStringLiteral("Archive format or method is not implemented");
            else if (nOpenResult == E_ACCESSDENIED) sOpenError = QStringLiteral("Access to the archive was denied");
            else sOpenError = QStringLiteral("Archive open failed (HRESULT 0x%1)").arg((quint32)nOpenResult, 8, 16, QLatin1Char('0'));
            if (format.nDetectionRank <= 1) {
                setError(pErrorString, sOpenError);
                return OpenStatus::Fatal;
            }
            if (sDeferredError.isEmpty()) sDeferredError = sOpenError;
        }
    }

    if (!bSubArchive && !sDeferredError.isEmpty()) {
        setError(pErrorString, sDeferredError);
        return OpenStatus::Fatal;
    }
    return OpenStatus::Unsupported;
}

bool openArchive(const QString &sFileName, const QString &sPassword, OpenedArchive *pOpened, QString *pErrorString,
                 XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOpened) return false;
    pOpened->close();
    const QString sPhysicalFileName = QFileInfo(sFileName).absoluteFilePath();
    QtInStream *pStreamSpec = new QtInStream(sPhysicalFileName, pPdStruct);
    CMyComPtr<IInStream> stream = pStreamSpec;
    if (!pStreamSpec->open()) {
        setError(pErrorString, QStringLiteral("Cannot open archive file"));
        return false;
    }

    ArchiveLayer firstLayer;
    const OpenStatus firstStatus = openOneStream(stream, QFileInfo(sFileName).fileName(), sPhysicalFileName, false,
                                                 sPassword, &firstLayer, pErrorString, pPdStruct);
    if (firstStatus != OpenStatus::Opened) {
        if (firstStatus == OpenStatus::Unsupported) setError(pErrorString, QString::fromLatin1(UNSUPPORTED_FORMAT_ERROR));
        return false;
    }
    pOpened->setLeaf(firstLayer);

    const qint32 MAX_ARCHIVE_LAYERS = 32;
    for (qint32 nLayers = 1;; nLayers++) {
        NWindows::NCOM::CPropVariant mainSubfile;
        const HRESULT nMainResult = pOpened->archive->GetArchiveProperty(kpidMainSubfile, &mainSubfile);
        if (nMainResult != S_OK) {
            setError(pErrorString, nMainResult == E_ABORT ? QStringLiteral("Archive operation canceled") :
                                                           QStringLiteral("Cannot query archive main subfile"));
            pOpened->close();
            return false;
        }
        if (mainSubfile.vt == VT_EMPTY || mainSubfile.vt != VT_UI4) break;
        if (nLayers >= MAX_ARCHIVE_LAYERS) {
            setError(pErrorString, QStringLiteral("Archive nesting exceeds the safety limit"));
            pOpened->close();
            return false;
        }

        UInt32 nItems = 0;
        if (pOpened->archive->GetNumberOfItems(&nItems) != S_OK || mainSubfile.ulVal >= nItems) {
            setError(pErrorString, QStringLiteral("Archive contains an invalid main subfile index"));
            pOpened->close();
            return false;
        }
        QString sChildName;
        if (!itemString(pOpened->archive, mainSubfile.ulVal, kpidPath, &sChildName)) {
            setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
            pOpened->close();
            return false;
        }
        if (sChildName.isEmpty()) sChildName = QStringLiteral("Content");

        CMyComPtr<IInArchiveGetStream> getStream;
        HRESULT nStreamResult = pOpened->archive->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream);
        if (nStreamResult != S_OK || !getStream) break;
        CMyComPtr<ISequentialInStream> sequentialStream;
        nStreamResult = getStream->GetStream(mainSubfile.ulVal, &sequentialStream);
        if (nStreamResult == E_ABORT || !ProgressGuard(pPdStruct).canContinue()) {
            setError(pErrorString, QStringLiteral("Archive operation canceled"));
            pOpened->close();
            return false;
        }
        if (nStreamResult != S_OK || !sequentialStream) break;
        CMyComPtr<IInStream> childStream;
        nStreamResult = sequentialStream.QueryInterface(IID_IInStream, (void **)&childStream);
        if (nStreamResult != S_OK || !childStream) break;

        ArchiveLayer childLayer;
        const OpenStatus childStatus = openOneStream(childStream, sChildName, sPhysicalFileName, true, sPassword,
                                                     &childLayer, pErrorString, pPdStruct);
        if (childStatus == OpenStatus::Unsupported) break;
        if (childStatus != OpenStatus::Opened) {
            pOpened->close();
            return false;
        }
        pOpened->pushLeaf(childLayer);
    }
    return true;
}

bool itemString(IInArchive *pArchive, UInt32 nIndex, PROPID propId, QString *pValue)
{
    if (!pValue) return false;
    pValue->clear();
    NWindows::NCOM::CPropVariant prop;
    if (!pArchive || pArchive->GetProperty(nIndex, propId, &prop) != S_OK || prop.vt != VT_BSTR || !prop.bstrVal) return true;
    const UINT nLength = ::SysStringLen(prop.bstrVal);
    if (nLength > (UINT)MAX_PROPERTY_CHARS) return false;
    *pValue = QString::fromWCharArray(prop.bstrVal, (int)nLength);
    return true;
}

bool itemBool(IInArchive *pArchive, UInt32 nIndex, PROPID propId, bool *pValue)
{
    NWindows::NCOM::CPropVariant prop;
    return pArchive && pArchive->GetProperty(nIndex, propId, &prop) == S_OK && propToBool(prop, pValue);
}

bool itemUInt64(IInArchive *pArchive, UInt32 nIndex, PROPID propId, quint64 *pValue)
{
    NWindows::NCOM::CPropVariant prop;
    return pArchive && pArchive->GetProperty(nIndex, propId, &prop) == S_OK && propToUInt64(prop, pValue);
}

QDateTime itemDateTime(IInArchive *pArchive, UInt32 nIndex, PROPID propId)
{
    NWindows::NCOM::CPropVariant prop;
    if (!pArchive || pArchive->GetProperty(nIndex, propId, &prop) != S_OK || prop.vt != VT_FILETIME) return QDateTime();
    return fileTimeToDateTime(prop.filetime);
}

QString syntheticItemName(const QString &sFileName, UInt32 nIndex)
{
    QString sName = QFileInfo(sFileName).completeBaseName();
    if (sName.isEmpty()) sName = QStringLiteral("Content");
    if (nIndex) sName += QStringLiteral("_%1").arg(nIndex + 1);
    return sName;
}

QList<TechnicalEntry> readEntries(IInArchive *pArchive, const QString &sFileName, QString *pErrorString,
                                  XBinary::PDSTRUCT *pPdStruct)
{
    QList<TechnicalEntry> result;
    UInt32 nItems = 0;
    if (!pArchive || pArchive->GetNumberOfItems(&nItems) != S_OK) {
        setError(pErrorString, QStringLiteral("Cannot enumerate archive items"));
        return result;
    }
    if (nItems > MAX_ARCHIVE_ITEMS) {
        setError(pErrorString, QStringLiteral("Archive contains too many items"));
        return result;
    }
    CMyComPtr<IArchiveGetRawProps> rawProps;
    pArchive->QueryInterface(IID_IArchiveGetRawProps, (void **)&rawProps);
    ProgressGuard progress(pPdStruct);
    qint64 nMetadataChars = 0;
    for (UInt32 i = 0; i < nItems; i++) {
        if (!progress.canContinue()) {
            setError(pErrorString, QStringLiteral("Archive operation canceled"));
            result.clear();
            return result;
        }
        TechnicalEntry entry;
        entry.nIndex = i;
        if (!itemString(pArchive, i, kpidPath, &entry.sPath)) {
            setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
            result.clear();
            return result;
        }
        if (entry.sPath.isEmpty()) entry.sPath = syntheticItemName(sFileName, i);
        // Use the sanitized form: it is both what validateEntries() accepts and
        // where 7-Zip actually stages the item, so an absolute or "./"-prefixed
        // entry resolves inside the stage root instead of escaping it.
        if (!sanitizedArchiveEntryPath(entry.sPath, &entry.sNormalizedPath)) {
            entry.sNormalizedPath = normalizedArchivePath(entry.sPath);
        }
        entry.sKey = entry.sNormalizedPath.normalized(QString::NormalizationForm_C).toCaseFolded();
        itemBool(pArchive, i, kpidIsDir, &entry.bIsFolder);
        entry.mapValues.insert(QStringLiteral("Path"), entry.sPath);
        entry.mapValues.insert(QStringLiteral("Folder"), entry.bIsFolder ? QStringLiteral("+") : QStringLiteral("-"));

        quint64 nValue = 0;
        if (itemUInt64(pArchive, i, kpidSize, &nValue)) entry.mapValues.insert(QStringLiteral("Size"), QString::number(nValue));
        if (itemUInt64(pArchive, i, kpidPackSize, &nValue)) entry.mapValues.insert(QStringLiteral("Packed Size"), QString::number(nValue));
        if (itemUInt64(pArchive, i, kpidPosition, &nValue) || itemUInt64(pArchive, i, kpidOffset, &nValue))
            entry.mapValues.insert(QStringLiteral("Offset"), QString::number(nValue));
        if (itemUInt64(pArchive, i, kpidCRC, &nValue)) entry.mapValues.insert(QStringLiteral("CRC"), QString::number(nValue, 16));
        if (itemUInt64(pArchive, i, kpidAttrib, &nValue)) {
            QString sAttributes;
            if (entry.bIsFolder || (nValue & 0x10)) sAttributes += QLatin1Char('D');
            if (nValue & 0x01) sAttributes += QLatin1Char('R');
            if (nValue & 0x02) sAttributes += QLatin1Char('H');
            if (nValue & 0x04) sAttributes += QLatin1Char('S');
            if (nValue & 0x20) sAttributes += QLatin1Char('A');
            entry.mapValues.insert(QStringLiteral("Attributes"), sAttributes);
        }
        if (itemUInt64(pArchive, i, kpidPosixAttrib, &nValue)) {
            entry.mapValues.insert(QStringLiteral("POSIX Attributes"),
                                   QString::number(nValue));
        }
        bool bValue = false;
        if (itemBool(pArchive, i, kpidEncrypted, &bValue) && bValue) entry.mapValues.insert(QStringLiteral("Encrypted"), QStringLiteral("+"));
        if (itemBool(pArchive, i, kpidIsAltStream, &bValue) && bValue) entry.mapValues.insert(QStringLiteral("Alternate Stream"), QStringLiteral("+"));
        if (itemBool(pArchive, i, kpidIsAnti, &bValue) && bValue) entry.mapValues.insert(QStringLiteral("Anti"), QStringLiteral("+"));
        if (itemBool(pArchive, i, kpidIsDeleted, &bValue) && bValue) entry.mapValues.insert(QStringLiteral("Deleted"), QStringLiteral("+"));
        QString sProperty;
        if (!itemString(pArchive, i, kpidSymLink, &sProperty)) {
            setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
            result.clear();
            return result;
        }
        entry.mapValues.insert(QStringLiteral("Symbolic Link"), sProperty);
        if (!itemString(pArchive, i, kpidHardLink, &sProperty)) {
            setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
            result.clear();
            return result;
        }
        entry.mapValues.insert(QStringLiteral("Hard Link"), sProperty);
        if (!itemString(pArchive, i, kpidMethod, &sProperty)) {
            setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
            result.clear();
            return result;
        }
        entry.mapValues.insert(QStringLiteral("Method"), sProperty);
        if (!itemString(pArchive, i, kpidHostOS, &sProperty)) {
            setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
            result.clear();
            return result;
        }
        if (!sProperty.isEmpty()) {
            entry.mapValues.insert(QStringLiteral("Host OS"), sProperty);
        }
        if (itemBool(pArchive, i, kpidSolid, &bValue) && bValue) {
            entry.mapValues.insert(QStringLiteral("Solid"), QStringLiteral("+"));
        }
        if (itemUInt64(pArchive, i, kpidBlock, &nValue)) {
            entry.mapValues.insert(QStringLiteral("Block"), QString::number(nValue));
        }
        if (rawProps) {
            const void *pRawData = nullptr;
            UInt32 nRawSize = 0;
            UInt32 nRawType = 0;
            if (rawProps->GetRawProp(i, kpidNtReparse, &pRawData, &nRawSize, &nRawType) == S_OK && pRawData && nRawSize) {
                entry.mapValues.insert(QStringLiteral("Reparse"), QStringLiteral("+"));
            }
        }

        const QDateTime modified = itemDateTime(pArchive, i, kpidMTime);
        const QDateTime created = itemDateTime(pArchive, i, kpidCTime);
        const QDateTime accessed = itemDateTime(pArchive, i, kpidATime);
        if (modified.isValid()) entry.mapValues.insert(QStringLiteral("Modified"), modified.toString(Qt::ISODateWithMs));
        if (created.isValid()) entry.mapValues.insert(QStringLiteral("Created"), created.toString(Qt::ISODateWithMs));
        if (accessed.isValid()) entry.mapValues.insert(QStringLiteral("Accessed"), accessed.toString(Qt::ISODateWithMs));
        for (auto it = entry.mapValues.constBegin(); it != entry.mapValues.constEnd(); ++it) {
            if (it.value().size() > MAX_PROPERTY_CHARS || nMetadataChars > MAX_METADATA_CHARS - it.value().size()) {
                setError(pErrorString, QStringLiteral("Archive metadata exceeds safety limits"));
                result.clear();
                return result;
            }
            nMetadataChars += it.value().size();
        }
        result.append(entry);
    }
    return result;
}

bool fieldIsTrue(const QString &sValue)
{
    return (sValue == QLatin1String("+")) || (sValue == QLatin1String("1")) ||
           (sValue.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0);
}

bool entryIsUnsafeSpecial(const TechnicalEntry &entry)
{
    if (fieldIsTrue(entry.mapValues.value(QStringLiteral("Alternate Stream"))) ||
        fieldIsTrue(entry.mapValues.value(QStringLiteral("Anti"))) || fieldIsTrue(entry.mapValues.value(QStringLiteral("Deleted"))) ||
        fieldIsTrue(entry.mapValues.value(QStringLiteral("Reparse")))) return true;
    const QString sSymLink = entry.mapValues.value(QStringLiteral("Symbolic Link"));
    const QString sHardLink = entry.mapValues.value(QStringLiteral("Hard Link"));
    return (!sSymLink.isEmpty() && sSymLink != QLatin1String("-")) || (!sHardLink.isEmpty() && sHardLink != QLatin1String("-"));
}

bool validateEntries(const QList<TechnicalEntry> &entries, QString *pErrorString)
{
    QMap<QString, bool> paths;
    for (const TechnicalEntry &entry : entries) {
        QString sSafe;
        if (!sanitizedArchiveEntryPath(entry.sPath, &sSafe)) {
            setError(pErrorString, QStringLiteral("Archive contains an unsafe path: %1").arg(entry.sPath));
            return false;
        }
        if (entryIsUnsafeSpecial(entry)) {
            setError(pErrorString, QStringLiteral("Archive contains an unsupported link, stream, anti, or deleted item: %1").arg(entry.sPath));
            return false;
        }
        // An empty result means the entry names the destination root itself,
        // e.g. tar's leading ".".  It carries no payload, so it is skipped
        // instead of being registered as a (necessarily duplicate) path.
        if (sSafe.isEmpty()) continue;
        const QString sKey = sSafe.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (paths.contains(sKey)) {
            setError(pErrorString, QStringLiteral("Archive contains duplicate paths: %1").arg(entry.sPath));
            return false;
        }
        paths.insert(sKey, entry.bIsFolder);
    }
    for (auto it = paths.constBegin(); it != paths.constEnd(); ++it) {
        const QStringList parts = it.key().split(QLatin1Char('/'));
        QString ancestor;
        for (qint32 i = 0; i + 1 < parts.size(); i++) {
            if (!ancestor.isEmpty()) ancestor += QLatin1Char('/');
            ancestor += parts.at(i);
            if (paths.contains(ancestor) && !paths.value(ancestor)) {
                setError(pErrorString, QStringLiteral("Archive contains a file/directory path collision"));
                return false;
            }
        }
    }
    return true;
}

QDateTime parseDateTime(const QString &sValue)
{
    return sValue.isEmpty() ? QDateTime() : QDateTime::fromString(sValue, Qt::ISODateWithMs);
}

XBinary::ARCHIVERECORD technicalToRecord(const TechnicalEntry &entry)
{
    XBinary::ARCHIVERECORD record = {};
    bool bOk = false;
    const qint64 nSize = entry.mapValues.value(QStringLiteral("Size")).toLongLong(&bOk);
    if (bOk) record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nSize);
    const qint64 nPacked = entry.mapValues.value(QStringLiteral("Packed Size")).toLongLong(&bOk);
    if (bOk) record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nPacked);
    const qint64 nOffset = entry.mapValues.value(QStringLiteral("Offset")).toLongLong(&bOk);
    record.nStreamOffset = bOk ? nOffset : 0;
    record.nStreamSize = record.mapProperties.value(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)0).toLongLong();
    record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, entry.sPath);
    record.mapProperties.insert(XBinary::FPART_PROP_ISFOLDER, entry.bIsFolder);
    record.mapProperties.insert(XBinary::FPART_PROP_STREAMOFFSET, record.nStreamOffset);
    record.mapProperties.insert(XBinary::FPART_PROP_STREAMSIZE, record.nStreamSize);
    record.mapProperties.insert(XBinary::FPART_PROP_ENCRYPTED, fieldIsTrue(entry.mapValues.value(QStringLiteral("Encrypted"))));
    const QString sMethod = entry.mapValues.value(QStringLiteral("Method"));
    if (!sMethod.isEmpty()) {
        // kpidMethod is provider text (often a complete pipeline such as
        // "LZMA2:12 BCJ"), not an XBinary::HANDLE_METHOD value.  Keep the
        // exact text separately and leave the numeric routing property typed.
        record.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD,
                                    (quint32)XBinary::HANDLE_METHOD_UNKNOWN);
        record.mapProperties.insert(XBinary::FPART_PROP_REPORTEDMETHOD, sMethod);
    }
    const qint64 nBlock = entry.mapValues.value(QStringLiteral("Block")).toLongLong(&bOk);
    if (bOk) {
        record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, nBlock);
    }
    if (fieldIsTrue(entry.mapValues.value(QStringLiteral("Solid")))) {
        record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
    }
    const QString sHostOS = entry.mapValues.value(QStringLiteral("Host OS"));
    if (!sHostOS.isEmpty()) {
        record.mapProperties.insert(XBinary::FPART_PROP_HOSTOS, sHostOS);
    }
    const QDateTime modified = parseDateTime(entry.mapValues.value(QStringLiteral("Modified")));
    const QDateTime created = parseDateTime(entry.mapValues.value(QStringLiteral("Created")));
    const QDateTime accessed = parseDateTime(entry.mapValues.value(QStringLiteral("Accessed")));
    if (modified.isValid()) record.mapProperties.insert(XBinary::FPART_PROP_MTIME, modified);
    if (created.isValid()) record.mapProperties.insert(XBinary::FPART_PROP_CTIME, created);
    if (accessed.isValid()) record.mapProperties.insert(XBinary::FPART_PROP_ATIME, accessed);
    const QString sAttributes = entry.mapValues.value(QStringLiteral("Attributes"));
    record.mapProperties.insert(XBinary::FPART_PROP_ISREADONLY, sAttributes.contains(QLatin1Char('R')));
    record.mapProperties.insert(XBinary::FPART_PROP_ISHIDDEN, sAttributes.contains(QLatin1Char('H')));
    record.mapProperties.insert(XBinary::FPART_PROP_ISSYSTEM, sAttributes.contains(QLatin1Char('S')));
    record.mapProperties.insert(XBinary::FPART_PROP_ISARCHIVE, sAttributes.contains(QLatin1Char('A')));
    const quint32 nPosixAttributes = entry.mapValues.value(
        QStringLiteral("POSIX Attributes")).toUInt(&bOk);
    if (bOk) {
        record.mapProperties.insert(XBinary::FPART_PROP_FILEMODE,
                                    nPosixAttributes);
        if ((nPosixAttributes & 0222U) == 0) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISREADONLY, true);
        }
    }
    const QString sCrc = entry.mapValues.value(QStringLiteral("CRC"));
    const quint32 nCrc = sCrc.toUInt(&bOk, 16);
    if (bOk && !sCrc.isEmpty()) {
        record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDCRC, nCrc);
        record.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, (quint32)XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    }
    return record;
}

class ArchiveExtractCallback final : public IArchiveExtractCallback,
                                     public IArchiveExtractCallbackMessage2,
                                     public IArchiveRequestMemoryUseCallback,
                                     public ICryptoGetTextPassword,
                                     public CMyUnknownImp {
    Z7_IFACES_IMP_UNK_4(IArchiveExtractCallback, IArchiveExtractCallbackMessage2,
                        IArchiveRequestMemoryUseCallback, ICryptoGetTextPassword)
    Z7_IFACE_COM7_IMP(IProgress)

public:
    ArchiveExtractCallback(IInArchive *pArchive, const QList<TechnicalEntry> *pEntries, const QString &sStageRoot,
                           QIODevice *pSelectedOutput, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct,
                           qint64 nMaxOutputSize)
        : m_archive(pArchive),
          m_pEntries(pEntries),
          m_sStageRoot(sStageRoot),
          m_pSelectedOutput(pSelectedOutput),
          m_password(toUString(sPassword)),
          m_progress(pPdStruct),
          m_nMaxOutputSize(nMaxOutputSize)
    {
    }

    QString errorString() const
    {
        QMutexLocker locker(&m_errorMutex);
        return m_sError;
    }

    bool isCanceled() const { return !m_progress.canContinue(); }

private:
    void recordError(const QString &sError)
    {
        QMutexLocker locker(&m_errorMutex);
        if (m_sError.isEmpty()) m_sError = sError;
    }

    void recordResult(Int32 nResult)
    {
        const QString sResult = operationResultText(nResult);
        if (!sResult.isEmpty()) {
            QMutexLocker locker(&m_errorMutex);
            if (m_sError.isEmpty()) m_sError = sResult;
        }
    }

    CMyComPtr<IInArchive> m_archive;
    const QList<TechnicalEntry> *m_pEntries;
    QString m_sStageRoot;
    QIODevice *m_pSelectedOutput;
    UString m_password;
    ProgressGuard m_progress;
    mutable QMutex m_errorMutex;
    QString m_sError;
    UInt32 m_nCurrentIndex = (UInt32)-1;
    Int32 m_nAskMode = NArchive::NExtract::NAskMode::kSkip;
    QString m_sCurrentPath;
    QScopedPointer<QFile> m_currentFile;
    QScopedPointer<LimitedWriteDevice> m_currentLimitedOutput;
    CMyComPtr<ISequentialOutStream> m_currentStream;
    qint64 m_nMaxOutputSize = -1;
};

Z7_COM7F_IMF(ArchiveExtractCallback::SetTotal(UInt64))
{
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveExtractCallback::SetCompleted(const UInt64 *))
{
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode))
{
    if (!outStream) return E_INVALIDARG;
    *outStream = nullptr;
    m_currentStream.Release();
    m_currentLimitedOutput.reset();
    m_currentFile.reset();
    m_sCurrentPath.clear();
    m_nCurrentIndex = index;
    m_nAskMode = askExtractMode;
    if (!m_progress.canContinue()) return E_ABORT;
    if (askExtractMode != NArchive::NExtract::NAskMode::kExtract) return S_OK;
    if (!m_pEntries || index >= (UInt32)m_pEntries->size()) return E_FAIL;

    const TechnicalEntry &entry = m_pEntries->at((qint32)index);
    if (entry.bIsFolder) {
        if (!m_sStageRoot.isEmpty()) {
            const QString sPath = QDir(m_sStageRoot).filePath(entry.sNormalizedPath);
            if (!isContainedPath(sPath, m_sStageRoot) || !QDir().mkpath(sPath)) return E_FAIL;
        }
        return S_OK;
    }

    if (m_nMaxOutputSize >= 0) {
        bool bSizeValid = false;
        const qulonglong nDeclaredSize = entry.mapValues
            .value(QStringLiteral("Size")).toULongLong(&bSizeValid);
        if (bSizeValid &&
            (nDeclaredSize > (qulonglong)m_nMaxOutputSize)) {
            recordError(QStringLiteral(
                "Archive member exceeds the output-size limit"));
            return E_FAIL;
        }
    }

    QIODevice *pDevice = m_pSelectedOutput;
    if (!pDevice) {
        if (m_sStageRoot.isEmpty()) return E_FAIL;
        m_sCurrentPath = QDir(m_sStageRoot).filePath(entry.sNormalizedPath);
        if (!isContainedPath(m_sCurrentPath, m_sStageRoot) || !QDir().mkpath(QFileInfo(m_sCurrentPath).absolutePath())) return E_FAIL;
        m_currentFile.reset(new QFile(m_sCurrentPath));
        if (!m_currentFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) return E_FAIL;
        pDevice = m_currentFile.data();
    }
    if (!pDevice || !pDevice->isOpen() || !pDevice->isWritable()) return E_FAIL;
    // The runtime bound has to wrap whichever device was chosen, not just the
    // staging file. The declared-size check above is skipped whenever the
    // handler reports no trustworthy size - which is exactly the single-stream
    // gzip/xz/bzip2 case - so a caller-supplied output device would otherwise
    // be left with no bound at all.
    if (m_nMaxOutputSize >= 0) {
        m_currentLimitedOutput.reset(
            new LimitedWriteDevice(pDevice, m_nMaxOutputSize));
        pDevice = m_currentLimitedOutput.data();
        if (!pDevice->isOpen() || !pDevice->isWritable()) return E_FAIL;
    }

    QtOutStream *pSpec = new QtOutStream(pDevice, m_progress.pPdStruct);
    CMyComPtr<ISequentialOutStream> stream = pSpec;
    m_currentStream = stream;
    *outStream = stream.Detach();
    return S_OK;
}

Z7_COM7F_IMF(ArchiveExtractCallback::PrepareOperation(Int32 askExtractMode))
{
    m_nAskMode = askExtractMode;
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveExtractCallback::SetOperationResult(Int32 opRes))
{
    m_currentStream.Release();
    if (m_currentLimitedOutput &&
        m_currentLimitedOutput->isLimitExceeded()) {
        recordError(QStringLiteral(
            "Archive member exceeds the output-size limit"));
        opRes = NArchive::NExtract::NOperationResult::kDataError;
    }
    m_currentLimitedOutput.reset();
    if (m_currentFile) {
        if (opRes == NArchive::NExtract::NOperationResult::kOK) {
            if (m_pEntries && m_nCurrentIndex < (UInt32)m_pEntries->size()) {
                const QDateTime modified = parseDateTime(m_pEntries->at((qint32)m_nCurrentIndex).mapValues.value(QStringLiteral("Modified")));
                if (modified.isValid()) m_currentFile->setFileTime(modified, QFileDevice::FileModificationTime);
            }
            if (!m_currentFile->flush()) opRes = NArchive::NExtract::NOperationResult::kDataError;
        }
        m_currentFile->close();
        if (opRes != NArchive::NExtract::NOperationResult::kOK && !m_sCurrentPath.isEmpty()) QFile::remove(m_sCurrentPath);
        m_currentFile.reset();
    } else if (m_pSelectedOutput) {
        QFileDevice *pFileDevice = qobject_cast<QFileDevice *>(m_pSelectedOutput);
        if (pFileDevice && !pFileDevice->flush()) opRes = NArchive::NExtract::NOperationResult::kDataError;
    }
    recordResult(opRes);
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveExtractCallback::ReportExtractResult(UInt32, UInt32, Int32 opRes))
{
    recordResult(opRes);
    return m_progress.canContinue() ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(ArchiveExtractCallback::RequestMemoryUse(UInt32 flags, UInt32, UInt32, const wchar_t *,
                                                      UInt64 requiredSize, UInt64 *allowedSize, UInt32 *answerFlags))
{
    if (!allowedSize || !answerFlags) return E_INVALIDARG;
    if (!m_progress.canContinue()) {
        *answerFlags = NRequestMemoryAnswerFlags::k_Stop;
        return E_ABORT;
    }

    // A 32-bit Qt process cannot safely satisfy the RAR5 handler's 4 GiB default.
    // Keep enough address-space headroom for Qt, archive metadata, and decoder overhead.
    if (*allowedSize > MAX_DECODER_MEMORY) *allowedSize = MAX_DECODER_MEMORY;
    if (flags & NRequestMemoryUseFlags::k_IsReport) return S_OK;
    if (requiredSize <= *allowedSize) {
        *answerFlags = NRequestMemoryAnswerFlags::k_Allow;
        return S_OK;
    }

    *answerFlags = NRequestMemoryAnswerFlags::k_Limit_Exceeded;
    if (flags & NRequestMemoryUseFlags::k_SkipArc_IsExpected) {
        *answerFlags |= NRequestMemoryAnswerFlags::k_SkipArc;
    }
    {
        QMutexLocker locker(&m_errorMutex);
        if (m_sError.isEmpty()) {
            m_sError = QStringLiteral("Archive extraction requires %1 MiB of decoder memory; the safe process limit is %2 MiB")
                           .arg((requiredSize + ((UInt64)1 << 20) - 1) >> 20)
                           .arg((*allowedSize + ((UInt64)1 << 20) - 1) >> 20);
        }
    }
    return S_OK;
}

Z7_COM7F_IMF(ArchiveExtractCallback::CryptoGetTextPassword(BSTR *password))
{
    if (!password) return E_INVALIDARG;
    if (!m_progress.canContinue()) return E_ABORT;
    return StringToBstr(m_password, password);
}

#ifdef Q_OS_WIN
bool isReparsePoint(const QString &sPath, bool *pbValid = nullptr)
{
    // Extended-length form, or a staged item past MAX_PATH reports as invalid
    // and the caller then refuses the extraction with "Staged archive item is
    // a reparse point" - the failure mode behind the long-destination reports.
    const DWORD nAttributes =
        GetFileAttributesW((LPCWSTR)XBinary::winExtendedNativePath(QFileInfo(sPath).absoluteFilePath()).utf16());
    if (nAttributes == INVALID_FILE_ATTRIBUTES) {
        if (pbValid) *pbValid = false;
        return false;
    }
    if (pbValid) *pbValid = true;
    return (nAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}
#endif

bool checkExistingSafe(const QString &sPath, bool bExpectDirectory, QString *pErrorString)
{
    const QFileInfo info(sPath);
    if (!info.exists()) return true;
    if (info.isSymLink() || (bExpectDirectory ? !info.isDir() : !info.isFile())) {
        setError(pErrorString, QStringLiteral("Unsafe existing destination path: %1").arg(sPath));
        return false;
    }
#ifdef Q_OS_WIN
    bool bValid = false;
    if (isReparsePoint(info.absoluteFilePath(), &bValid) || !bValid) {
        setError(pErrorString, QStringLiteral("Unsafe existing destination reparse path: %1").arg(sPath));
        return false;
    }
#endif
    return true;
}

bool ensureSafeDirectory(const QString &sRoot, const QString &sRelativeDirectory,
                         XBinary::UNPACK_FOLDER_TRANSACTION *pTransaction,
                         QString *pErrorString)
{
    if (!pTransaction) {
        setError(pErrorString, QStringLiteral("Archive publication transaction is unavailable"));
        return false;
    }
    if (sRelativeDirectory.isEmpty() || sRelativeDirectory == QLatin1String(".")) return checkExistingSafe(sRoot, true, pErrorString);
    QString sCurrent = sRoot;
    const QStringList parts = QDir::fromNativeSeparators(sRelativeDirectory).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        sCurrent = QDir(sCurrent).filePath(part);
        if (!isContainedPath(sCurrent, sRoot)) {
            setError(pErrorString, QStringLiteral("Destination path escapes its root"));
            return false;
        }
        if (QFileInfo::exists(sCurrent)) {
            if (!checkExistingSafe(sCurrent, true, pErrorString)) return false;
        } else {
            if (!pTransaction->ensureDirectory(sCurrent)) {
                const QString sTransactionError = pTransaction->errorString();
                setError(pErrorString, sTransactionError.isEmpty()
                                           ? QStringLiteral("Cannot create destination directory: %1").arg(sCurrent)
                                           : sTransactionError);
                return false;
            }
            if (!checkExistingSafe(sCurrent, true, pErrorString)) return false;
        }
    }
    return true;
}

bool prepareDestinationRoot(const QString &sResultFolder, QString *pRoot, QString *pErrorString)
{
    if (sResultFolder.isEmpty()) return false;
    const QString sAbsolute = QFileInfo(sResultFolder).absoluteFilePath();
    if (QDir(sAbsolute).isRoot()) {
        setError(pErrorString, QStringLiteral("Refusing to extract directly to a filesystem root"));
        return false;
    }
    if (QFileInfo::exists(sAbsolute)) {
        if (!checkExistingSafe(sAbsolute, true, pErrorString)) return false;
    } else if (!QDir().mkpath(sAbsolute)) {
        setError(pErrorString, QStringLiteral("Cannot create destination directory"));
        return false;
    }
    const QString sCanonical = QFileInfo(sAbsolute).canonicalFilePath();
    if (sCanonical.isEmpty() || !checkExistingSafe(sCanonical, true, pErrorString)) return false;
    if (pRoot) *pRoot = sCanonical;
    return true;
}

bool buildStagedEntries(const QString &sStageRoot, const QList<TechnicalEntry> &technical, QList<StagedEntry> *pStaged,
                        QString *pErrorString)
{
    if (!pStaged) return false;
    pStaged->clear();
    for (const TechnicalEntry &entry : technical) {
        // Entries naming the destination root itself (tar's leading ".") have
        // no staged file of their own and nothing to place.
        if (entry.sNormalizedPath.isEmpty()) continue;
        StagedEntry staged;
        staged.sRelativePath = entry.sNormalizedPath;
        staged.sSourcePath = QDir(sStageRoot).filePath(staged.sRelativePath);
        staged.bIsFolder = entry.bIsFolder;
        staged.modified = parseDateTime(entry.mapValues.value(QStringLiteral("Modified")));
        const QFileInfo info(staged.sSourcePath);
        if (!info.exists() || info.isSymLink() || (staged.bIsFolder ? !info.isDir() : !info.isFile())) {
            setError(pErrorString, QStringLiteral("Staged archive item is missing or unsafe: %1").arg(entry.sPath));
            return false;
        }
#ifdef Q_OS_WIN
        bool bValid = false;
        if (isReparsePoint(info.absoluteFilePath(), &bValid) || !bValid) {
            setError(pErrorString, QStringLiteral("Staged archive item is a reparse point: %1").arg(entry.sPath));
            return false;
        }
#endif
        const QString sCanonical = info.canonicalFilePath();
        if (sCanonical.isEmpty() || !isContainedPath(sCanonical, sStageRoot)) {
            setError(pErrorString, QStringLiteral("Staged archive item escapes its root: %1").arg(entry.sPath));
            return false;
        }
        staged.permissions = info.permissions();
        pStaged->append(staged);
    }
    return true;
}

bool preflightDestination(const QString &sRoot, const QList<StagedEntry> &entries, QString *pErrorString)
{
    for (const StagedEntry &entry : entries) {
        const QString sTarget = QDir(sRoot).filePath(entry.sRelativePath);
        if (!isContainedPath(sTarget, sRoot)) return false;
        QString sParent = QFileInfo(sTarget).absolutePath();
        while (isContainedPath(sParent, sRoot) && sParent.compare(sRoot, Qt::CaseInsensitive) != 0) {
            if (QFileInfo::exists(sParent) && !checkExistingSafe(sParent, true, pErrorString)) return false;
            const QString sNext = QFileInfo(sParent).absolutePath();
            if (sNext == sParent) break;
            sParent = sNext;
        }
        if (QFileInfo::exists(sTarget) && !checkExistingSafe(sTarget, entry.bIsFolder, pErrorString)) return false;
    }
    return true;
}

bool copyFileTransactional(const StagedEntry &entry, const QString &sTarget,
                           XBinary::UNPACK_FOLDER_TRANSACTION *pTransaction,
                           QString *pErrorString, const ProgressGuard &progress)
{
    if (!pTransaction) {
        setError(pErrorString, QStringLiteral("Archive publication transaction is unavailable"));
        return false;
    }
    QFile source(entry.sSourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        setError(pErrorString, QStringLiteral("Cannot read staged archive item"));
        return false;
    }
    QSaveFile target(sTarget);
    target.setDirectWriteFallback(false);
    if (!target.open(QIODevice::WriteOnly)) {
        setError(pErrorString, QStringLiteral("Cannot create destination file: %1").arg(sTarget));
        return false;
    }
    QByteArray buffer(COPY_BUFFER_SIZE, 0);
    while (progress.canContinue()) {
        const qint64 nRead = source.read(buffer.data(), buffer.size());
        if (nRead < 0) break;
        if (!nRead) {
            if (!progress.canContinue()) {
                target.cancelWriting();
                setError(pErrorString, QStringLiteral("Archive operation canceled"));
                return false;
            }
            // Re-validate the destination at the publication boundary.
            // preflightDestination checked it once before extraction began, so
            // on its own the window between that check and this replacement
            // spans the whole operation: anything could have swapped the target
            // for a symlink, a reparse point or a directory in the meantime.
            // The native route already re-checks immediately before its commit;
            // this route did not, which made it the weaker of the two.
            if (!checkExistingSafe(sTarget, false, pErrorString)) {
                target.cancelWriting();
                return false;
            }
            // Keep the existing destination visible while the verified bytes
            // are copied into QSaveFile's private temporary.  Journal/backup
            // the destination only at the publication boundary immediately
            // before the atomic replacement.
            if (!pTransaction->prepareFile(sTarget)) {
                target.cancelWriting();
                const QString sTransactionError = pTransaction->errorString();
                setError(pErrorString, sTransactionError.isEmpty()
                                           ? QStringLiteral("Cannot prepare destination file: %1").arg(sTarget)
                                           : sTransactionError);
                return false;
            }
            if (!progress.canContinue()) {
                target.cancelWriting();
                setError(pErrorString, QStringLiteral("Archive operation canceled"));
                return false;
            }
            if (!target.commit()) {
                setError(pErrorString, QStringLiteral("Cannot finalize destination file: %1").arg(sTarget));
                return false;
            }
            // Record the now-visible file before doing metadata work or
            // observing cancellation so every later failure can remove it or
            // restore the journaled predecessor.
            if (!pTransaction->markFilePublished(sTarget)) {
                const QString sTransactionError = pTransaction->errorString();
                setError(pErrorString, sTransactionError.isEmpty()
                                           ? QStringLiteral("Cannot journal published destination file: %1").arg(sTarget)
                                           : sTransactionError);
                return false;
            }
            if (!progress.canContinue()) {
                setError(pErrorString, QStringLiteral("Archive operation canceled"));
                return false;
            }
            return true;
        }
        if (target.write(buffer.constData(), nRead) != nRead) break;
    }
    target.cancelWriting();
    setError(pErrorString, progress.canContinue() ? QStringLiteral("Cannot copy staged archive item") : QStringLiteral("Archive operation canceled"));
    return false;
}

bool rollbackFolderPublication(XBinary::UNPACK_FOLDER_TRANSACTION *pTransaction,
                               const QString &sFailure,
                               QString *pErrorString)
{
    QString sMessage = sFailure;
    if (sMessage.isEmpty()) sMessage = QStringLiteral("Archive publication failed");

    const QString sTransactionErrorBefore = pTransaction ? pTransaction->errorString() : QString();
    const bool bRolledBack = pTransaction && pTransaction->rollback();
    const QString sTransactionErrorAfter = pTransaction ? pTransaction->errorString() : QString();

    const auto appendDetail = [&sMessage](const QString &sDetail) {
        if (!sDetail.isEmpty() && !sMessage.contains(sDetail)) {
            sMessage += QStringLiteral("; ") + sDetail;
        }
    };
    appendDetail(sTransactionErrorBefore);

    if (!bRolledBack) {
        appendDetail(QStringLiteral("Extraction rollback is incomplete"));
        appendDetail(sTransactionErrorAfter);
        const QString sRecoveryPath = pTransaction ? pTransaction->recoveryPath() : QString();
        appendDetail(QStringLiteral("Rollback recovery path: %1")
                         .arg(sRecoveryPath.isEmpty()
                                  ? QStringLiteral("<unavailable>")
                                  : QDir::toNativeSeparators(sRecoveryPath)));
    }

    setError(pErrorString, sMessage);
    return false;
}

bool mergeStagedEntries(const QString &sRoot, QList<StagedEntry> entries, QString *pErrorString, XBinary::PDSTRUCT *pPdStruct)
{
    std::stable_sort(entries.begin(), entries.end(), [](const StagedEntry &a, const StagedEntry &b) {
        if (a.bIsFolder != b.bIsFolder) return a.bIsFolder;
        return a.sRelativePath.count(QLatin1Char('/')) < b.sRelativePath.count(QLatin1Char('/'));
    });
    ProgressGuard progress(pPdStruct);
    XBinary::UNPACK_FOLDER_TRANSACTION transaction(sRoot);
    QString sPublicationError;
    const auto failPublication = [&](const QString &sFailure) {
        return rollbackFolderPublication(&transaction, sFailure, pErrorString);
    };

    if (!transaction.isValid()) {
        const QString sTransactionError = transaction.errorString();
        // Construction has not published or journaled anything, so there is
        // nothing to roll back and no recovery directory to report.
        setError(pErrorString, sTransactionError.isEmpty()
                                   ? QStringLiteral("Cannot initialize archive publication transaction")
                                   : sTransactionError);
        return false;
    }

    for (const StagedEntry &entry : entries) {
        if (!progress.canContinue()) {
            return failPublication(QStringLiteral("Archive operation canceled"));
        }
        const QString sTarget = QDir(sRoot).filePath(entry.sRelativePath);
        if (!isContainedPath(sTarget, sRoot)) {
            return failPublication(QStringLiteral("Destination path escapes its root"));
        }
        if (entry.bIsFolder) {
            sPublicationError.clear();
            if (!ensureSafeDirectory(sRoot, entry.sRelativePath, &transaction,
                                     &sPublicationError)) {
                return failPublication(sPublicationError);
            }
        } else {
            const QString sParentRelative = QDir(sRoot).relativeFilePath(QFileInfo(sTarget).absolutePath());
            sPublicationError.clear();
            if (!ensureSafeDirectory(sRoot, sParentRelative, &transaction,
                                     &sPublicationError) ||
                (QFileInfo::exists(sTarget) &&
                 !checkExistingSafe(sTarget, false, &sPublicationError)) ||
                !copyFileTransactional(entry, sTarget, &transaction,
                                       &sPublicationError, progress)) {
                return failPublication(sPublicationError);
            }
        }
    }

    if (!progress.canContinue()) {
        return failPublication(QStringLiteral("Archive operation canceled"));
    }
    if (!transaction.commit()) {
        const QString sTransactionError = transaction.errorString();
        return failPublication(sTransactionError.isEmpty()
                                   ? QStringLiteral("Cannot commit archive publication transaction")
                                   : sTransactionError);
    }
    if (!transaction.errorString().isEmpty()) {
        QString sWarning = transaction.errorString();
        if (!transaction.recoveryPath().isEmpty() &&
            !sWarning.contains(transaction.recoveryPath())) {
            sWarning += QStringLiteral("; rollback-data recovery path: %1")
                            .arg(QDir::toNativeSeparators(
                                transaction.recoveryPath()));
        }
        // Publication is committed, so retain success while exposing the
        // cleanup/recovery warning to callers that inspect diagnostics.
        setError(pErrorString, sWarning);
    }

    // Permissions can make a file non-deletable on Windows.  Apply all staged
    // metadata only after the journal has committed, when rollback is no
    // longer required.  These best-effort operations preserve the previous
    // behavior, which did not fail extraction on a metadata-set error.
    for (const StagedEntry &entry : entries) {
        if (entry.bIsFolder) continue;
        const QString sTarget = QDir(sRoot).filePath(entry.sRelativePath);
        QFile::setPermissions(sTarget, entry.permissions);
        if (entry.modified.isValid()) {
            QFile output(sTarget);
            if (output.open(QIODevice::ReadWrite)) {
                output.setFileTime(entry.modified,
                                   QFileDevice::FileModificationTime);
            }
        }
    }
    return true;
}

bool copyDevice(QIODevice *pSource, QIODevice *pTarget, QString *pErrorString, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSource || !pTarget || !pSource->seek(0)) {
        setError(pErrorString, QStringLiteral("Cannot read extracted archive item"));
        return false;
    }
    ProgressGuard progress(pPdStruct);
    QByteArray buffer(COPY_BUFFER_SIZE, 0);
    while (progress.canContinue()) {
        const qint64 nRead = pSource->read(buffer.data(), buffer.size());
        if (nRead < 0) break;
        if (!nRead) return true;
        qint64 nOffset = 0;
        while (nOffset < nRead) {
            const qint64 nWritten = pTarget->write(buffer.constData() + nOffset, nRead - nOffset);
            if (nWritten <= 0) {
                setError(pErrorString, QStringLiteral("Cannot write extracted archive item"));
                return false;
            }
            nOffset += nWritten;
        }
    }
    setError(pErrorString, progress.canContinue() ? QStringLiteral("Cannot read extracted archive item") : QStringLiteral("Archive operation canceled"));
    return false;
}

bool configureDecoderMemoryLimit(OpenedArchive *pOpened, QString *pErrorString)
{
    if (!pOpened || !pOpened->archive) return false;
    if (pOpened->sFormat != QLatin1String("Rar5")) return true;

    CMyComPtr<ISetProperties> properties;
    const HRESULT nQueryResult = pOpened->archive->QueryInterface(IID_ISetProperties, (void **)&properties);
    if (nQueryResult != S_OK || !properties) {
        setError(pErrorString, QStringLiteral("RAR5 decoder memory limit is unavailable"));
        return false;
    }
    const wchar_t *names[] = {L"memx"};
    NWindows::NCOM::CPropVariant value;
    value = MAX_DECODER_MEMORY;
    if (properties->SetProperties(names, &value, 1) != S_OK) {
        setError(pErrorString, QStringLiteral("Cannot enforce the RAR5 decoder memory limit"));
        return false;
    }
    return true;
}

bool runExtract(OpenedArchive *pOpened, const QList<TechnicalEntry> &entries, const QString &sPassword,
                const UInt32 *pIndices, UInt32 nItems, bool bTest, const QString &sStageRoot, QIODevice *pSelectedOutput,
                QString *pErrorString, XBinary::PDSTRUCT *pPdStruct,
                qint64 nMaxOutputSize = -1)
{
    if (!configureDecoderMemoryLimit(pOpened, pErrorString)) return false;
    ArchiveExtractCallback *pCallbackSpec = new ArchiveExtractCallback(
        pOpened->archive, &entries, sStageRoot, pSelectedOutput, sPassword,
        pPdStruct, nMaxOutputSize);
    CMyComPtr<IArchiveExtractCallback> callback = pCallbackSpec;
    const HRESULT nResult = pOpened->archive->Extract(pIndices, nItems, bTest ? 1 : 0, callback);
    const QString sCallbackError = pCallbackSpec->errorString();
    if (nResult != S_OK || pCallbackSpec->isCanceled()) {
        setError(pErrorString, pCallbackSpec->isCanceled() ? QStringLiteral("Archive operation canceled") :
                                                            (sCallbackError.isEmpty() ? QStringLiteral("Archive extraction failed") : sCallbackError));
        return false;
    }
    if (!sCallbackError.isEmpty()) {
        setError(pErrorString, sCallbackError);
        return false;
    }
    const QString sFatal = archiveFatalError(pOpened->archive);
    if (!sFatal.isEmpty()) {
        setError(pErrorString, sFatal);
        return false;
    }
    return true;
}

}  // namespace

bool XArchives::isIp7zSourceAvailable()
{
    UInt32 nFormats = 0;
    return GetNumberOfFormats(&nFormats) == S_OK && nFormats != 0;
}

bool XArchives::isIp7zUnsupportedFormatError(const QString &sErrorString)
{
    return (sErrorString == QString::fromLatin1(UNSUPPORTED_FORMAT_ERROR)) ||
           (sErrorString == QStringLiteral("Unsupported compression method")) ||
           (sErrorString == QStringLiteral("Unsupported archive method")) ||
           (sErrorString == QStringLiteral("Archive format or method is not implemented"));
}

bool XArchives::listArchiveWithIp7zSource(const QString &sFileName, const QString &sPassword,
                                         QList<XBinary::ARCHIVERECORD> *pListRecords,
                                         QString *pErrorString, XBinary::PDSTRUCT *pPdStruct)
{
    setError(pErrorString, QString());
    if (!pListRecords) return false;
    pListRecords->clear();
    if (!QFileInfo(sFileName).isFile()) {
        setError(pErrorString, QStringLiteral("Archive file does not exist"));
        return false;
    }
    OpenedArchive opened;
    if (!openArchive(sFileName, sPassword, &opened, pErrorString, pPdStruct)) return false;
    QString sReadError;
    const QList<TechnicalEntry> entries = readEntries(opened.archive, opened.sDisplayName, &sReadError, pPdStruct);
    opened.close();
    if (!sReadError.isEmpty()) {
        setError(pErrorString, sReadError);
        return false;
    }
    // kpidBlock is the folder-level accounting key.  7-Zip reports a packed
    // size only on the first member of a solid block; mark every member of a
    // shared block so callers can count that size once without treating the
    // other members as missing metadata.
    QMap<QString, qint32> mapBlockCounts;
    for (const TechnicalEntry &entry : entries) {
        const QString sBlock = entry.mapValues.value(QStringLiteral("Block"));
        if (!entry.bIsFolder && !sBlock.isEmpty()) {
            mapBlockCounts[sBlock] = mapBlockCounts.value(sBlock) + 1;
        }
    }
    for (const TechnicalEntry &entry : entries) {
        XBinary::ARCHIVERECORD record = technicalToRecord(entry);
        const QString sBlock = entry.mapValues.value(QStringLiteral("Block"));
        if (!entry.bIsFolder && !sBlock.isEmpty() && (mapBlockCounts.value(sBlock) > 1)) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }
        pListRecords->append(record);
    }
    return true;
}

bool XArchives::testArchiveWithIp7zSource(const QString &sFileName, const QString &sPassword,
                                         QString *pErrorString, XBinary::PDSTRUCT *pPdStruct)
{
    setError(pErrorString, QString());
    if (!QFileInfo(sFileName).isFile()) {
        setError(pErrorString, QStringLiteral("Archive file does not exist"));
        return false;
    }
    OpenedArchive opened;
    if (!openArchive(sFileName, sPassword, &opened, pErrorString, pPdStruct)) return false;
    QString sReadError;
    const QList<TechnicalEntry> entries = readEntries(opened.archive, opened.sDisplayName, &sReadError, pPdStruct);
    bool bResult = sReadError.isEmpty() && runExtract(&opened, entries, sPassword, nullptr, (UInt32)(Int32)-1, true,
                                                     QString(), nullptr, pErrorString, pPdStruct);
    opened.close();
    if (!sReadError.isEmpty()) setError(pErrorString, sReadError);
    return bResult;
}

bool XArchives::extractArchiveWithIp7zSource(const QString &sFileName, const QString &sPassword,
                                            const QString &sResultFolder, QString *pErrorString,
                                            XBinary::PDSTRUCT *pPdStruct)
{
    return extractArchiveWithIp7zSource(
        sFileName, sPassword, sResultFolder, pErrorString, pPdStruct, -1);
}

bool XArchives::extractArchiveWithIp7zSource(const QString &sFileName, const QString &sPassword,
                                            const QString &sResultFolder, QString *pErrorString,
                                            XBinary::PDSTRUCT *pPdStruct,
                                            qint64 nMaxOutputSize)
{
    setError(pErrorString, QString());
    if (!QFileInfo(sFileName).isFile() || sResultFolder.isEmpty() ||
        (nMaxOutputSize < -1)) {
        setError(pErrorString, QStringLiteral("Invalid archive or destination directory"));
        return false;
    }
    OpenedArchive opened;
    if (!openArchive(sFileName, sPassword, &opened, pErrorString, pPdStruct)) return false;
    QString sReadError;
    const QList<TechnicalEntry> entries = readEntries(opened.archive, opened.sDisplayName, &sReadError, pPdStruct);
    if (!sReadError.isEmpty() || !validateEntries(entries, pErrorString)) {
        opened.close();
        if (!sReadError.isEmpty()) setError(pErrorString, sReadError);
        return false;
    }
    QString sDestinationRoot;
    if (!prepareDestinationRoot(sResultFolder, &sDestinationRoot, pErrorString)) {
        opened.close();
        return false;
    }
    const QString sStageParent = QFileInfo(sDestinationRoot).absolutePath();
    QTemporaryDir temporaryDir(QDir(sStageParent).filePath(QStringLiteral(".xfileunpacker-staging-XXXXXX")));
    if (!temporaryDir.isValid()) {
        opened.close();
        setError(pErrorString, QStringLiteral("Cannot create private extraction staging directory"));
        return false;
    }
    const QString sStageRoot = QFileInfo(temporaryDir.path()).canonicalFilePath();
    if (!runExtract(&opened, entries, sPassword, nullptr,
                    (UInt32)(Int32)-1, false, sStageRoot, nullptr,
                    pErrorString, pPdStruct, nMaxOutputSize)) {
        opened.close();
        return false;
    }
    opened.close();
    QList<StagedEntry> staged;
    if (!buildStagedEntries(sStageRoot, entries, &staged, pErrorString)) return false;
    if (!preflightDestination(sDestinationRoot, staged, pErrorString)) return false;
    return mergeStagedEntries(sDestinationRoot, staged, pErrorString, pPdStruct);
}

bool XArchives::extractArchiveRecordWithIp7zSource(const QString &sFileName, const QString &sRecordName,
                                                  const QString &sPassword, QIODevice *pOutputDevice,
                                                  QString *pErrorString, XBinary::PDSTRUCT *pPdStruct)
{
    return extractArchiveRecordWithIp7zSource(sFileName, sRecordName, sPassword, pOutputDevice, pErrorString, pPdStruct, -1);
}

bool XArchives::extractArchiveRecordWithIp7zSource(const QString &sFileName, const QString &sRecordName,
                                                  const QString &sPassword, QIODevice *pOutputDevice,
                                                  QString *pErrorString, XBinary::PDSTRUCT *pPdStruct,
                                                  qint64 nMaxOutputSize)
{
    setError(pErrorString, QString());
    if (!QFileInfo(sFileName).isFile() || sRecordName.isEmpty() || !pOutputDevice || !pOutputDevice->isOpen() || !pOutputDevice->isWritable() ||
        (nMaxOutputSize < -1)) {
        setError(pErrorString, QStringLiteral("Invalid archive record or output device"));
        return false;
    }
    OpenedArchive opened;
    if (!openArchive(sFileName, sPassword, &opened, pErrorString, pPdStruct)) return false;
    QString sReadError;
    const QList<TechnicalEntry> entries = readEntries(opened.archive, opened.sDisplayName, &sReadError, pPdStruct);
    if (!sReadError.isEmpty() || !validateEntries(entries, pErrorString)) {
        opened.close();
        if (!sReadError.isEmpty()) setError(pErrorString, sReadError);
        return false;
    }
    const TechnicalEntry *pMatch = nullptr;
    for (const TechnicalEntry &entry : entries) {
        if (entry.sPath == sRecordName) {
            if (pMatch) {
                setError(pErrorString, QStringLiteral("Selected archive record is ambiguous"));
                opened.close();
                return false;
            }
            pMatch = &entry;
        }
    }
    if (!pMatch || pMatch->bIsFolder || entryIsUnsafeSpecial(*pMatch)) {
        setError(pErrorString, QStringLiteral("Selected archive record is missing or is not a regular file"));
        opened.close();
        return false;
    }
    if (nMaxOutputSize >= 0) {
        bool bSizeValid = false;
        const qulonglong nDeclaredSize = pMatch->mapValues.value(QStringLiteral("Size")).toULongLong(&bSizeValid);

        if (bSizeValid && (nDeclaredSize > (qulonglong)nMaxOutputSize)) {
            setError(pErrorString, QStringLiteral("Selected archive record exceeds the output-size limit"));
            opened.close();
            return false;
        }
    }
    QTemporaryFile stagedFile(QDir(QDir::tempPath()).filePath(QStringLiteral("xfileunpacker-record-XXXXXX")));
    if (!stagedFile.open()) {
        setError(pErrorString, QStringLiteral("Cannot create private record staging file"));
        opened.close();
        return false;
    }
    const UInt32 nIndex = pMatch->nIndex;
    QScopedPointer<LimitedWriteDevice> limitedOutput;
    QIODevice *pStagedOutput = &stagedFile;

    if (nMaxOutputSize >= 0) {
        limitedOutput.reset(new LimitedWriteDevice(&stagedFile, nMaxOutputSize));
        pStagedOutput = limitedOutput.data();
    }

    const bool bExtracted = runExtract(&opened, entries, sPassword, &nIndex, 1, false, QString(), pStagedOutput, pErrorString, pPdStruct);
    opened.close();
    if (limitedOutput && limitedOutput->isLimitExceeded()) {
        setError(pErrorString, QStringLiteral("Selected archive record exceeds the output-size limit"));
        return false;
    }
    if (!bExtracted) return false;
    if (!stagedFile.flush()) {
        setError(pErrorString, QStringLiteral("Cannot flush private record staging file"));
        return false;
    }
    return copyDevice(&stagedFile, pOutputDevice, pErrorString, pPdStruct);
}
