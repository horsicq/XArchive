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
#include "xexternalarchive.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QVector>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <AclAPI.h>
#endif
#ifdef Q_OS_UNIX
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <limits>
#include <memory>
#include <new>
#include <utility>

namespace {
const qint64 EXTERNAL_CAPTURE_LIMIT = 64LL * 1024LL * 1024LL;
const qint64 EXTERNAL_COPY_BUFFER_SIZE = 1024LL * 1024LL;
const qint64 EXTERNAL_DEFAULT_TIMEOUT_MS = 10LL * 60LL * 1000LL;
const qint64 EXTERNAL_PIPE_READ_SIZE = 64LL * 1024LL;
#ifdef Q_OS_WIN
const DWORD EXTERNAL_JOB_PROCESS_LIMIT = 32;
#endif

struct ExternalRecord {
    QString sName;
    QString sStagedPath;
    QString sProvider;
    qint64 nCompressedSize = -1;
    qint64 nUncompressedSize = -1;
    QDateTime dateTime;
    bool bIsFolder = false;
};

struct StageScanResult {
    QList<ExternalRecord> listRecords;
    qint64 nTotalSize = 0;
    qint64 nEntryCount = 0;
};

bool externalRecordLess(const ExternalRecord &a, const ExternalRecord &b)
{
    const int nCompare = QString::compare(a.sName, b.sName, Qt::CaseSensitive);
    if (nCompare != 0) return nCompare < 0;
    return a.bIsFolder && !b.bIsFolder;
}

void sortExternalRecords(QList<ExternalRecord> *pRecords)
{
    if (!pRecords) return;
    std::sort(pRecords->begin(), pRecords->end(), externalRecordLess);
}

QString backendName(XExternalArchive::BACKEND backend)
{
    switch (backend) {
        case XExternalArchive::BACKEND_ZPAQ: return QStringLiteral("ZPAQ");
        case XExternalArchive::BACKEND_BCM: return QStringLiteral("BCM");
        case XExternalArchive::BACKEND_LPAQ8: return QStringLiteral("LPAQ8");
        case XExternalArchive::BACKEND_PEA: return QStringLiteral("PEA");
        case XExternalArchive::BACKEND_FREEARC: return QStringLiteral("FreeArc");
        default: return QStringLiteral("archive");
    }
}

QString helperRelativePath(XExternalArchive::BACKEND backend)
{
#ifdef Q_OS_WIN
    switch (backend) {
        case XExternalArchive::BACKEND_ZPAQ: return QStringLiteral("res/bin/zpaq/zpaq.exe");
        case XExternalArchive::BACKEND_BCM: return QStringLiteral("res/bin/quad/bcm.exe");
        case XExternalArchive::BACKEND_LPAQ8: return QStringLiteral("res/bin/lpaq/lpaq8.exe");
        case XExternalArchive::BACKEND_PEA: return QStringLiteral("pea.exe");
        case XExternalArchive::BACKEND_FREEARC: return QStringLiteral("res/bin/arc/Arc.exe");
        default: return QString();
    }
#else
    switch (backend) {
        case XExternalArchive::BACKEND_ZPAQ: return QStringLiteral("res/bin/zpaq/zpaq");
        case XExternalArchive::BACKEND_BCM: return QStringLiteral("res/bin/quad/bcm");
        case XExternalArchive::BACKEND_LPAQ8: return QStringLiteral("res/bin/lpaq/lpaq8");
        case XExternalArchive::BACKEND_PEA: return QStringLiteral("pea");
        case XExternalArchive::BACKEND_FREEARC: return QStringLiteral("res/bin/arc/arc");
        default: return QString();
    }
#endif
}

void appendRoot(QStringList *pRoots, const QString &sRoot)
{
    if (!pRoots || sRoot.trimmed().isEmpty()) return;
    const QString sAbsolute = QDir::cleanPath(QFileInfo(sRoot).absoluteFilePath());
    for (const QString &sExisting : *pRoots) {
#ifdef Q_OS_WIN
        if (sExisting.compare(sAbsolute, Qt::CaseInsensitive) == 0) return;
#else
        if (sExisting == sAbsolute) return;
#endif
    }
    pRoots->append(sAbsolute);
}

QString resolveHelper(XExternalArchive::BACKEND backend)
{
    const QString sRelative = helperRelativePath(backend);
    if (sRelative.isEmpty()) return QString();

    QStringList listRoots;
    appendRoot(&listRoots, qEnvironmentVariable("XFU_PEAZIP_ROOT"));
    appendRoot(&listRoots, qEnvironmentVariable("PEAZIP_ROOT"));

    const QString sApplicationDir = QCoreApplication::applicationDirPath();
    appendRoot(&listRoots, sApplicationDir);
    appendRoot(&listRoots, QDir(sApplicationDir).filePath(QStringLiteral("PeaZip")));
    appendRoot(&listRoots, QDir(sApplicationDir).filePath(QStringLiteral("tools/PeaZip")));

#ifdef Q_OS_WIN
    appendRoot(&listRoots, QDir(qEnvironmentVariable("ProgramFiles")).filePath(QStringLiteral("PeaZip")));
    appendRoot(&listRoots, QDir(qEnvironmentVariable("ProgramFiles(x86)")).filePath(QStringLiteral("PeaZip")));
#else
    appendRoot(&listRoots, QStringLiteral("/usr/lib/peazip"));
    appendRoot(&listRoots, QStringLiteral("/usr/share/peazip"));
    appendRoot(&listRoots, QStringLiteral("/opt/peazip"));
#endif

    for (const QString &sRoot : listRoots) {
        const QFileInfo fileInfo(QDir(sRoot).filePath(sRelative));
        const QString sCanonical = fileInfo.canonicalFilePath();
        if (fileInfo.exists() && fileInfo.isFile() && fileInfo.isExecutable() && !fileInfo.isSymLink() && !sCanonical.isEmpty()) {
            return sCanonical;
        }
    }
    return QString();
}

bool setExternalError(XBinary::PDSTRUCT *pPdStruct, const QString &sError)
{
    if (pPdStruct) XBinary::setPdStructErrorString(pPdStruct, sError);
    return false;
}

Qt::CaseSensitivity pathCaseSensitivity()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

bool isContainedPath(const QString &sCanonicalRoot, const QString &sCanonicalPath)
{
    if (sCanonicalRoot.isEmpty() || sCanonicalPath.isEmpty()) return false;
    QString sRoot = QDir::fromNativeSeparators(sCanonicalRoot);
    QString sPath = QDir::fromNativeSeparators(sCanonicalPath);
    while (sRoot.endsWith(QLatin1Char('/'))) sRoot.chop(1);
    return (sPath.compare(sRoot, pathCaseSensitivity()) == 0) || sPath.startsWith(sRoot + QLatin1Char('/'), pathCaseSensitivity());
}

bool normalizeRecordName(const QString &sValue, QString *pResult)
{
    if (!pResult || sValue.contains(QChar::Null)) return false;
    // Callers may normalize a QString in place. Keep an independent input
    // value before clearing the destination so aliasing cannot turn every
    // otherwise-safe path into an empty name.
    const QString sOriginal = sValue;
    pResult->clear();

    QString sName = QDir::fromNativeSeparators(sOriginal);
    while (sName.startsWith(QStringLiteral("./"))) sName.remove(0, 2);
    while (sName.endsWith(QLatin1Char('/'))) sName.chop(1);
    if (sName.isEmpty() || QDir::isAbsolutePath(sName)) return false;
    if ((sName.size() >= 2) && sName.at(0).isLetter() && (sName.at(1) == QLatin1Char(':'))) return false;

    const QStringList listParts = sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
    }

    const QString sClean = QDir::cleanPath(sName);
    if ((sClean != sName) || sClean.startsWith(QStringLiteral("../")) || (sClean == QLatin1String(".."))) return false;
    *pResult = sClean;
    return true;
}

bool normalizeZpaqExtractedName(const QString &sValue, QString *pResult)
{
    if (!pResult || sValue.contains(QChar::Null)) return false;

    // zpaq's append_path() makes stored absolute names relative to -to. On
    // Windows it removes a drive colon (C:/x -> C/x), then removes one leading
    // slash on every platform. The filesystem collapses any remaining leading
    // separators inside the destination, so mirror that mapping before the
    // normal strict relative-path validation and manifest reconciliation.
    QString sName = QDir::fromNativeSeparators(sValue);
#ifdef Q_OS_WIN
    if ((sName.size() > 1) && (sName.at(1) == QLatin1Char(':'))) {
        if ((sName.size() > 2) && (sName.at(2) != QLatin1Char('/'))) {
            sName[1] = QLatin1Char('/');
        } else {
            sName.remove(1, 1);
        }
    }
#endif
    while (sName.startsWith(QLatin1Char('/'))) sName.remove(0, 1);
    return normalizeRecordName(sName, pResult);
}

bool addWithLimit(qint64 nValue, qint64 *pnTotal)
{
    if (!pnTotal || (nValue < 0) || (*pnTotal < 0) || (nValue > (std::numeric_limits<qint64>::max)() - *pnTotal)) return false;
    *pnTotal += nValue;
    return true;
}

bool policyAllows(qint64 nEntrySize, qint64 nTotalSize, qint64 nEntryCount, const XBinary::OUTPUT_POLICY &policy)
{
    return (nEntrySize >= 0) && (nTotalSize >= 0) && (nEntryCount >= 0) && ((policy.nMaxEntryOutputSize < 0) || (nEntrySize <= policy.nMaxEntryOutputSize)) &&
           ((policy.nMaxTotalOutputSize < 0) || (nTotalSize <= policy.nMaxTotalOutputSize)) && ((policy.nMaxEntryCount < 0) || (nEntryCount <= policy.nMaxEntryCount));
}

bool scanStageTree(const QString &sRoot, const XBinary::OUTPUT_POLICY &policy, const QString &sProvider, bool bBuildRecords, StageScanResult *pResult,
                   XBinary::PDSTRUCT *pPdStruct = nullptr, const QDeadlineTimer *pDeadline = nullptr, bool *pbTimedOut = nullptr)
{
    if (pbTimedOut) *pbTimedOut = false;
    if (!pResult) return false;
    *pResult = StageScanResult();

    const QFileInfo rootInfo(sRoot);
    const QString sCanonicalRoot = rootInfo.canonicalFilePath();
    if (!rootInfo.exists() || !rootInfo.isDir() || rootInfo.isSymLink() || sCanonicalRoot.isEmpty()) return false;

    QSet<QString> setNames;
    QDirIterator iterator(sCanonicalRoot, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        if (pDeadline && !pDeadline->isForever() && pDeadline->hasExpired()) {
            if (pbTimedOut) *pbTimedOut = true;
            return false;
        }
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        iterator.next();
        const QFileInfo fileInfo = iterator.fileInfo();
        if (fileInfo.isSymLink() || (!fileInfo.isFile() && !fileInfo.isDir())) return false;

        const QString sCanonicalPath = fileInfo.canonicalFilePath();
        if (!isContainedPath(sCanonicalRoot, sCanonicalPath)) return false;

        QString sRelative = QDir::fromNativeSeparators(QDir(sCanonicalRoot).relativeFilePath(sCanonicalPath));
        if (!normalizeRecordName(sRelative, &sRelative)) return false;
        const QString sKey = (pathCaseSensitivity() == Qt::CaseInsensitive) ? sRelative.toCaseFolded() : sRelative;
        if (setNames.contains(sKey)) return false;
        setNames.insert(sKey);

        if (pResult->nEntryCount == (std::numeric_limits<qint64>::max)()) return false;
        ++pResult->nEntryCount;

        const qint64 nSize = fileInfo.isFile() ? fileInfo.size() : 0;
        if ((nSize < 0) || !addWithLimit(nSize, &pResult->nTotalSize) || !policyAllows(nSize, pResult->nTotalSize, pResult->nEntryCount, policy)) return false;

        if (bBuildRecords) {
            ExternalRecord record;
            record.sName = sRelative;
            record.sStagedPath = sCanonicalPath;
            record.sProvider = sProvider;
            record.nUncompressedSize = nSize;
            record.dateTime = fileInfo.lastModified();
            record.bIsFolder = fileInfo.isDir();
            pResult->listRecords.append(record);
        }
    }

    if (bBuildRecords) {
        sortExternalRecords(&pResult->listRecords);
    }
    return true;
}

qint64 externalTimeoutMs()
{
    bool bOK = false;
    const qint64 nValue = qEnvironmentVariable("XFU_EXTERNAL_TOOL_TIMEOUT_MS").toLongLong(&bOK);
    if (!bOK || (nValue < 1000) || (nValue > 60LL * 60LL * 1000LL)) return EXTERNAL_DEFAULT_TIMEOUT_MS;
    return nValue;
}

class ExternalHelperProcess : public QProcess {
public:
    using QProcess::QProcess;

#if defined(Q_OS_UNIX) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
protected:
    void setupChildProcess() override
    {
        // Qt 5 has no public child modifier. This hook runs after fork and
        // before exec, so no helper instruction can precede session/process-
        // group containment. _exit() is async-signal-safe on failure.
        if (::setsid() == -1) ::_exit(127);
    }
#endif
};

#ifdef Q_OS_WIN
bool allocateLowIntegritySid(PSID *ppSid, QString *pError)
{
    if (!ppSid) return false;
    *ppSid = nullptr;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_MANDATORY_LABEL_AUTHORITY;
    if (!AllocateAndInitializeSid(&authority, 1, SECURITY_MANDATORY_LOW_RID, 0, 0, 0, 0, 0, 0, 0, ppSid)) {
        if (pError) {
            *pError = QStringLiteral(
                          "Cannot allocate the external-helper Low-integrity SID "
                          "(Windows error %1)")
                          .arg(GetLastError());
        }
        return false;
    }
    return true;
}

bool setLowIntegrityDirectoryTree(const QString &sCanonicalRoot, QString *pError)
{
    const QFileInfo rootInfo(sCanonicalRoot);
    if (!rootInfo.exists() || !rootInfo.isDir() || rootInfo.isSymLink() || (rootInfo.canonicalFilePath().compare(sCanonicalRoot, pathCaseSensitivity()) != 0)) {
        if (pError) *pError = QStringLiteral("Cannot label an invalid external-helper working directory");
        return false;
    }

    QStringList listDirectories;
    listDirectories.append(sCanonicalRoot);
    QDirIterator iterator(sCanonicalRoot, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString sPath = iterator.next();
        const QFileInfo directoryInfo = iterator.fileInfo();
        const QString sCanonicalPath = directoryInfo.canonicalFilePath();
        const QString sNativePath = QDir::toNativeSeparators(sPath);
        const DWORD nAttributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(sNativePath.utf16()));
        if (!directoryInfo.isDir() || directoryInfo.isSymLink() || sCanonicalPath.isEmpty() || !isContainedPath(sCanonicalRoot, sCanonicalPath) ||
            (nAttributes == INVALID_FILE_ATTRIBUTES) || (nAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            if (pError)
                *pError = QStringLiteral(
                    "External-helper working directory contains an unsafe "
                    "directory entry");
            return false;
        }
        listDirectories.append(sCanonicalPath);
    }

    PSID pLowSid = nullptr;
    if (!allocateLowIntegritySid(&pLowSid, pError)) return false;
    const DWORD nAclSize = sizeof(ACL) + sizeof(SYSTEM_MANDATORY_LABEL_ACE) - sizeof(DWORD) + GetLengthSid(pLowSid);
    QByteArray baAcl(static_cast<int>(nAclSize), 0);
    PACL pAcl = reinterpret_cast<PACL>(baAcl.data());
    bool bResult = InitializeAcl(pAcl, nAclSize, ACL_REVISION) != FALSE;
    if (bResult) {
        bResult = AddMandatoryAce(pAcl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, pLowSid) != FALSE;
    }
    if (!bResult && pError) {
        *pError = QStringLiteral(
                      "Cannot construct the external-helper Low-integrity label "
                      "(Windows error %1)")
                      .arg(GetLastError());
    }

    for (const QString &sDirectory : qAsConst(listDirectories)) {
        if (!bResult) break;
        QString sNativeDirectory = QDir::toNativeSeparators(sDirectory);
        const DWORD nStatus =
            SetNamedSecurityInfoW(reinterpret_cast<LPWSTR>(sNativeDirectory.data()), SE_FILE_OBJECT, LABEL_SECURITY_INFORMATION, nullptr, nullptr, nullptr, pAcl);
        if (nStatus != ERROR_SUCCESS) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot label the external-helper working directory "
                              "Low integrity (Windows error %1)")
                              .arg(nStatus);
            }
            bResult = false;
        }
    }
    FreeSid(pLowSid);
    return bResult;
}

bool lowerProcessIntegrity(PROCESS_INFORMATION *pProcessInformation, QString *pError)
{
    if (!pProcessInformation || !pProcessInformation->hProcess) return false;
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(pProcessInformation->hProcess, TOKEN_ADJUST_DEFAULT | TOKEN_QUERY, &hToken)) {
        if (pError) {
            *pError = QStringLiteral(
                          "Cannot open the external-helper process token "
                          "(Windows error %1)")
                          .arg(GetLastError());
        }
        return false;
    }

    PSID pLowSid = nullptr;
    bool bResult = allocateLowIntegritySid(&pLowSid, pError);
    if (bResult) {
        TOKEN_MANDATORY_LABEL label = {};
        label.Label.Attributes = SE_GROUP_INTEGRITY;
        label.Label.Sid = pLowSid;
        const DWORD nLabelSize = sizeof(label) + GetLengthSid(pLowSid);
        if (!SetTokenInformation(hToken, TokenIntegrityLevel, &label, nLabelSize)) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot lower the external-helper process to Low "
                              "integrity (Windows error %1)")
                              .arg(GetLastError());
            }
            bResult = false;
        }
    }
    if (pLowSid) FreeSid(pLowSid);
    CloseHandle(hToken);
    return bResult;
}

class RestrictedChildHandleInheritance {
public:
    ~RestrictedChildHandleInheritance()
    {
        if (m_bAttributeListInitialized) DeleteProcThreadAttributeList(m_pAttributeList);
        if (m_pAttributeStorage) HeapFree(GetProcessHeap(), 0, m_pAttributeStorage);
    }

    bool initialize(QString *pError)
    {
        SIZE_T nAttributeBytes = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &nAttributeBytes);
        if (!nAttributeBytes) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot size the external-helper handle allowlist "
                              "(Windows error %1)")
                              .arg(GetLastError());
            }
            return false;
        }
        m_pAttributeStorage = HeapAlloc(GetProcessHeap(), 0, nAttributeBytes);
        if (!m_pAttributeStorage) {
            if (pError) *pError = QStringLiteral("Cannot allocate the external-helper handle allowlist");
            return false;
        }
        m_pAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(m_pAttributeStorage);
        if (!InitializeProcThreadAttributeList(m_pAttributeList, 1, 0, &nAttributeBytes)) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot initialize the external-helper handle allowlist "
                              "(Windows error %1)")
                              .arg(GetLastError());
            }
            return false;
        }
        m_bAttributeListInitialized = true;
        return true;
    }

    bool configure(QProcess::CreateProcessArguments *pArguments, QString *pError)
    {
        if (!pArguments || !pArguments->startupInfo || !m_pAttributeList) {
            if (pError) {
                *pError = QStringLiteral(
                    "QProcess did not expose startup state for the "
                    "external-helper handle allowlist");
            }
            return false;
        }

        const STARTUPINFOW *pStartupInfo = reinterpret_cast<const STARTUPINFOW *>(pArguments->startupInfo);
        if (!(pStartupInfo->dwFlags & STARTF_USESTDHANDLES)) {
            if (pError) {
                *pError = QStringLiteral(
                    "QProcess did not configure explicit external-helper "
                    "standard streams");
            }
            return false;
        }

        m_listInheritedHandles.clear();
        if (!appendHandle(pStartupInfo->hStdInput, QStringLiteral("stdin"), pError) ||
            !appendHandle(pStartupInfo->hStdOutput, QStringLiteral("stdout"), pError) ||
            !appendHandle(pStartupInfo->hStdError, QStringLiteral("stderr"), pError))
            return false;

        m_startupInfo = {};
        m_startupInfo.StartupInfo = *pStartupInfo;
        m_startupInfo.StartupInfo.cb = sizeof(m_startupInfo);
        m_startupInfo.lpAttributeList = m_pAttributeList;
        if (!UpdateProcThreadAttribute(m_pAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, m_listInheritedHandles.data(),
                                       static_cast<SIZE_T>(m_listInheritedHandles.size()) * sizeof(HANDLE), nullptr, nullptr)) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot install the external-helper handle allowlist "
                              "(Windows error %1)")
                              .arg(GetLastError());
            }
            return false;
        }

        pArguments->startupInfo = reinterpret_cast<Q_STARTUPINFO *>(&m_startupInfo);
        pArguments->inheritHandles = true;
        pArguments->flags |= CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW;
        m_bConfigured = true;
        return true;
    }

    bool isConfigured() const
    {
        return m_bConfigured;
    }

private:
    bool appendHandle(HANDLE hHandle, const QString &sName, QString *pError)
    {
        if (!hHandle || (hHandle == INVALID_HANDLE_VALUE)) {
            if (pError) {
                *pError = QStringLiteral(
                              "QProcess supplied an invalid external-helper %1 "
                              "handle")
                              .arg(sName);
            }
            return false;
        }
        DWORD nFlags = 0;
        if (!GetHandleInformation(hHandle, &nFlags) ||
            !(nFlags & HANDLE_FLAG_INHERIT)) {
            if (pError) {
                *pError = QStringLiteral(
                              "QProcess supplied a non-inheritable external-helper "
                              "%1 handle (Windows error %2)")
                              .arg(sName)
                              .arg(GetLastError());
            }
            return false;
        }
        if (!m_listInheritedHandles.contains(hHandle))
            m_listInheritedHandles.append(hHandle);
        return true;
    }

    void *m_pAttributeStorage = nullptr;
    LPPROC_THREAD_ATTRIBUTE_LIST m_pAttributeList = nullptr;
    bool m_bAttributeListInitialized = false;
    STARTUPINFOEXW m_startupInfo = {};
    QVector<HANDLE> m_listInheritedHandles;
    bool m_bConfigured = false;
};

class RestrictedProcessArgumentsModifier
{
public:
    RestrictedProcessArgumentsModifier(void **ppNativeProcessInformation,
                                       RestrictedChildHandleInheritance *pInheritance,
                                       QString *pExecutionError)
        : m_ppNativeProcessInformation(ppNativeProcessInformation),
          m_pInheritance(pInheritance), m_pExecutionError(pExecutionError)
    {
    }

    void operator()(QProcess::CreateProcessArguments *pArguments) const
    {
        if (m_ppNativeProcessInformation)
            *m_ppNativeProcessInformation = pArguments ? pArguments->processInformation : nullptr;
        if (!m_pInheritance ||
            !m_pInheritance->configure(pArguments, m_pExecutionError)) {
            if (pArguments) {
                pArguments->flags |= CREATE_SUSPENDED | CREATE_NO_WINDOW;
                pArguments->inheritHandles = false;
            }
        }
    }

private:
    void **m_ppNativeProcessInformation;
    RestrictedChildHandleInheritance *m_pInheritance;
    QString *m_pExecutionError;
};

class ExternalProcessTreeGuard {
public:
    ExternalProcessTreeGuard()
    {
        m_hJob = CreateJobObjectW(nullptr, nullptr);
        if (!m_hJob) {
            m_sError = QStringLiteral(
                           "Cannot create the external-helper containment job "
                           "(Windows error %1)")
                           .arg(GetLastError());
            return;
        }

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
        limits.BasicLimitInformation.ActiveProcessLimit = EXTERNAL_JOB_PROCESS_LIMIT;
        if (!SetInformationJobObject(m_hJob, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
            m_sError = QStringLiteral(
                           "Cannot configure the external-helper containment job "
                           "(Windows error %1)")
                           .arg(GetLastError());
            CloseHandle(m_hJob);
            m_hJob = nullptr;
        }
    }

    ~ExternalProcessTreeGuard()
    {
        // KILL_ON_JOB_CLOSE is the final fail-closed cleanup if any return path
        // leaves a helper descendant alive.
        if (m_hJob) CloseHandle(m_hJob);
    }

    bool isValid() const
    {
        return m_hJob != nullptr;
    }
    QString errorString() const
    {
        return m_sError;
    }

    bool attachAndResume(void *pNativeProcessInformation, QString *pError)
    {
        PROCESS_INFORMATION *pProcessInformation = static_cast<PROCESS_INFORMATION *>(pNativeProcessInformation);
        if (!m_hJob || !pProcessInformation || !pProcessInformation->hProcess || !pProcessInformation->hThread) {
            if (pError) {
                *pError = m_sError.isEmpty() ? QStringLiteral(
                                                   "QProcess did not expose the suspended "
                                                   "external-helper process handles")
                                             : m_sError;
            }
            return false;
        }

        if (!AssignProcessToJobObject(m_hJob, pProcessInformation->hProcess)) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot assign the external helper to its containment "
                              "job (Windows error %1)")
                              .arg(GetLastError());
            }
            return false;
        }
        m_bProcessAssigned = true;
        if (!lowerProcessIntegrity(pProcessInformation, pError)) {
            TerminateJobObject(m_hJob, 1);
            return false;
        }
        if (ResumeThread(pProcessInformation->hThread) == static_cast<DWORD>(-1)) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot resume the contained external helper "
                              "(Windows error %1)")
                              .arg(GetLastError());
            }
            TerminateJobObject(m_hJob, 1);
            return false;
        }
        return true;
    }

    bool hasActiveProcesses(bool *pResult, QString *pError) const
    {
        if (!pResult || !m_hJob || !m_bProcessAssigned) {
            if (pError) *pError = QStringLiteral("The external-helper containment job is unavailable");
            return false;
        }
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting = {};
        if (!QueryInformationJobObject(m_hJob, JobObjectBasicAccountingInformation, &accounting, sizeof(accounting), nullptr)) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot query the external-helper containment job "
                              "(Windows error %1)")
                              .arg(GetLastError());
            }
            return false;
        }
        *pResult = accounting.ActiveProcesses != 0;
        return true;
    }

    bool terminateAndWait(QProcess *pProcess, void *pNativeProcessInformation, QString *pError)
    {
        PROCESS_INFORMATION *pProcessInformation = static_cast<PROCESS_INFORMATION *>(pNativeProcessInformation);
        if (!m_bProcessAssigned && m_hJob && pProcessInformation && pProcessInformation->hProcess) {
            if (AssignProcessToJobObject(m_hJob, pProcessInformation->hProcess)) {
                m_bProcessAssigned = true;
            }
        }

        bool bTerminationStarted = false;
        if (m_bProcessAssigned && m_hJob) {
            bTerminationStarted = TerminateJobObject(m_hJob, 1) != FALSE;
        } else if (pProcessInformation && pProcessInformation->hProcess) {
            // A handle-allowlist/setup failure leaves the root suspended, so
            // terminating that root is race-free even when Job assignment was
            // unavailable.
            bTerminationStarted = TerminateProcess(pProcessInformation->hProcess, 1) != FALSE;
        } else if (pProcess && (pProcess->state() != QProcess::NotRunning)) {
            pProcess->kill();
            bTerminationStarted = true;
        } else {
            bTerminationStarted = true;
        }

        if (!bTerminationStarted) {
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot terminate the external-helper process tree "
                              "(Windows error %1)")
                              .arg(GetLastError());
            }
            return false;
        }

        if (pProcess && (pProcess->state() != QProcess::NotRunning)) pProcess->waitForFinished(3000);

        if (m_bProcessAssigned && m_hJob) {
            QElapsedTimer timer;
            timer.start();
            for (;;) {
                JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting = {};
                if (!QueryInformationJobObject(m_hJob, JobObjectBasicAccountingInformation, &accounting, sizeof(accounting), nullptr)) {
                    if (pError) {
                        *pError = QStringLiteral(
                                      "Cannot verify external-helper process-tree "
                                      "termination (Windows error %1)")
                                      .arg(GetLastError());
                    }
                    return false;
                }
                if (accounting.ActiveProcesses == 0) return true;
                if (timer.elapsed() >= 3000) {
                    if (pError) {
                        *pError = QStringLiteral(
                            "The external-helper process tree did not stop "
                            "within 3000 ms");
                    }
                    return false;
                }
                Sleep(10);
            }
        }
        return !pProcess || (pProcess->state() == QProcess::NotRunning);
    }

private:
    HANDLE m_hJob = nullptr;
    bool m_bProcessAssigned = false;
    QString m_sError;
};
#endif

#ifdef Q_OS_UNIX
class ExternalPosixProcessTreeGuard {
public:
    ~ExternalPosixProcessTreeGuard()
    {
        bool bActive = false;
        QString sIgnoredError;
        if (hasActiveProcesses(&bActive, &sIgnoredError) && bActive) ::kill(-m_nProcessGroupId, SIGKILL);
    }

    bool capture(QProcess *pProcess, QString *pError)
    {
        if (!pProcess || (pProcess->processId() <= 0) || (static_cast<quint64>(pProcess->processId()) > static_cast<quint64>((std::numeric_limits<pid_t>::max)()))) {
            if (pError) *pError = QStringLiteral("Cannot retain the external-helper process-group ID");
            return false;
        }
        m_nProcessGroupId = static_cast<pid_t>(pProcess->processId());
        return true;
    }

    bool hasActiveProcesses(bool *pResult, QString *pError)
    {
        if (!pResult || (m_nProcessGroupId <= 0)) {
            if (pError) *pError = QStringLiteral("The external-helper process group is unavailable");
            return false;
        }
        errno = 0;
        if ((::kill(-m_nProcessGroupId, 0) == 0) || (errno == EPERM)) {
            *pResult = true;
            return true;
        }
        if (errno == ESRCH) {
            *pResult = false;
            // The group has been observed absent. Forget its numeric ID now:
            // retaining it until destruction could target an unrelated group
            // if the kernel reuses the ID before this guard leaves scope.
            m_nProcessGroupId = -1;
            return true;
        }
        if (pError) {
            *pError = QStringLiteral(
                          "Cannot query the external-helper process group "
                          "(errno %1)")
                          .arg(errno);
        }
        return false;
    }

    bool terminateAndWait(QProcess *pProcess, QString *pError)
    {
        if (m_nProcessGroupId <= 0) {
            if (pProcess && (pProcess->state() != QProcess::NotRunning)) {
                pProcess->kill();
                return pProcess->waitForFinished(3000);
            }
            return true;
        }

        if (::kill(-m_nProcessGroupId, SIGTERM) != 0) {
            if (errno == ESRCH) {
                m_nProcessGroupId = -1;
                if (pProcess && (pProcess->state() != QProcess::NotRunning)) pProcess->waitForFinished(100);
                return true;
            }
            if (pError) {
                *pError = QStringLiteral(
                              "Cannot terminate the external-helper process group "
                              "(errno %1)")
                              .arg(errno);
            }
            return false;
        }

        QElapsedTimer timer;
        timer.start();
        bool bSentKill = false;
        for (;;) {
            bool bActive = false;
            if (!hasActiveProcesses(&bActive, pError)) return false;
            if (!bActive) {
                if (pProcess && (pProcess->state() != QProcess::NotRunning)) pProcess->waitForFinished(100);
                return true;
            }
            if (!bSentKill && (timer.elapsed() >= 1000)) {
                if (::kill(-m_nProcessGroupId, SIGKILL) != 0) {
                    if (errno == ESRCH) {
                        m_nProcessGroupId = -1;
                        if (pProcess && (pProcess->state() != QProcess::NotRunning)) pProcess->waitForFinished(100);
                        return true;
                    }
                    if (pError) {
                        *pError = QStringLiteral(
                                      "Cannot kill the external-helper process group "
                                      "(errno %1)")
                                      .arg(errno);
                    }
                    return false;
                }
                bSentKill = true;
            }
            if (timer.elapsed() >= 3000) {
                if (pError)
                    *pError = QStringLiteral(
                        "The external-helper process group did not stop "
                        "within 3000 ms");
                return false;
            }
            if (pProcess && (pProcess->state() != QProcess::NotRunning)) pProcess->waitForFinished(10);
            else QThread::msleep(10);
        }
    }

private:
    pid_t m_nProcessGroupId = -1;
};
#endif

#if defined(Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)) && (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
void configureExternalChildSession()
{
    if (::setsid() == -1) ::_exit(127);
}
#endif

void stopProcess(QProcess *pProcess
#ifdef Q_OS_WIN
                 ,
                 ExternalProcessTreeGuard *pProcessTree, void *pNativeProcessInformation
#elif defined(Q_OS_UNIX)
                 ,
                 ExternalPosixProcessTreeGuard *pProcessTree
#endif
)
{
#ifdef Q_OS_WIN
    if (pProcessTree) {
        QString sIgnoredError;
        pProcessTree->terminateAndWait(pProcess, pNativeProcessInformation, &sIgnoredError);
        return;
    }
#elif defined(Q_OS_UNIX)
    if (pProcessTree) {
        QString sIgnoredError;
        pProcessTree->terminateAndWait(pProcess, &sIgnoredError);
        return;
    }
#endif
    if (!pProcess || (pProcess->state() == QProcess::NotRunning)) return;
    pProcess->terminate();
    if (!pProcess->waitForFinished(1000)) {
        pProcess->kill();
        pProcess->waitForFinished(3000);
    }
}

class ExternalProcessStopper
{
public:
#ifdef Q_OS_WIN
    ExternalProcessStopper(QProcess *pProcess,
                           ExternalProcessTreeGuard *pProcessTree,
                           void **ppNativeProcessInformation)
        : m_pProcess(pProcess), m_pProcessTree(pProcessTree),
          m_ppNativeProcessInformation(ppNativeProcessInformation)
    {
    }
#elif defined(Q_OS_UNIX)
    ExternalProcessStopper(QProcess *pProcess,
                           ExternalPosixProcessTreeGuard *pProcessTree)
        : m_pProcess(pProcess), m_pProcessTree(pProcessTree)
    {
    }
#else
    explicit ExternalProcessStopper(QProcess *pProcess)
        : m_pProcess(pProcess)
    {
    }
#endif

    void stop() const
    {
#ifdef Q_OS_WIN
        stopProcess(m_pProcess, m_pProcessTree,
                    m_ppNativeProcessInformation
                        ? *m_ppNativeProcessInformation : nullptr);
#elif defined(Q_OS_UNIX)
        stopProcess(m_pProcess, m_pProcessTree);
#else
        stopProcess(m_pProcess);
#endif
    }

private:
    QProcess *m_pProcess;
#ifdef Q_OS_WIN
    ExternalProcessTreeGuard *m_pProcessTree;
    void **m_ppNativeProcessInformation;
#elif defined(Q_OS_UNIX)
    ExternalPosixProcessTreeGuard *m_pProcessTree;
#endif
};

enum PEA_REPORT_STATUS {
    PEA_REPORT_ABSENT = 0,
    PEA_REPORT_INCOMPLETE,
    PEA_REPORT_SUCCESS,
    PEA_REPORT_FAILURE
};

PEA_REPORT_STATUS inspectPeaReport(const QString &sWorkingDirectory)
{
    QString sReportPath;
    QDirIterator iterator(sWorkingDirectory, QStringList() << QStringLiteral("*_Auto log UnPEA.txt"), QDir::Files | QDir::Hidden | QDir::System);
    while (iterator.hasNext()) {
        const QString sCandidate = iterator.next();
        if (!sReportPath.isEmpty()) return PEA_REPORT_FAILURE;
        sReportPath = sCandidate;
    }
    if (sReportPath.isEmpty()) return PEA_REPORT_ABSENT;

    const QFileInfo reportInfo(sReportPath);
    const QString sCanonicalRoot = QFileInfo(sWorkingDirectory).canonicalFilePath();
    const QString sCanonicalReport = reportInfo.canonicalFilePath();
    const qint64 nReportLimit = 4LL * 1024LL * 1024LL;
    if (!reportInfo.isFile() || reportInfo.isSymLink() || !isContainedPath(sCanonicalRoot, sCanonicalReport) || (reportInfo.size() < 0) ||
        (reportInfo.size() > nReportLimit)) {
        return PEA_REPORT_FAILURE;
    }

    QFile reportFile(sCanonicalReport);
    if (!reportFile.open(QIODevice::ReadOnly) || reportFile.isSequential()) return PEA_REPORT_INCOMPLETE;
    const QByteArray baReport = reportFile.readAll();
    if ((baReport.size() != reportInfo.size()) || baReport.contains('\0')) return PEA_REPORT_INCOMPLETE;

    const QByteArray baTerminal("Archive's stream correctly verified Done EXTRACT2DIR on archive");
    if (baReport.contains(baTerminal) && baReport.contains("Volume is OK") && !baReport.contains("Wrong tag!")) {
        return PEA_REPORT_SUCCESS;
    }
    if (baReport.contains("Done EXTRACT2DIR on archive")) return PEA_REPORT_FAILURE;
    return PEA_REPORT_INCOMPLETE;
}

bool validateToolExecutionPaths(const QString &sProgram, const QString &sWorkingDirectory, const QString &sAuditRoot, QString *pCanonicalProgram,
                                QString *pCanonicalWorkingDirectory, QString *pError)
{
    if (!pCanonicalProgram || !pCanonicalWorkingDirectory) return false;
    pCanonicalProgram->clear();
    pCanonicalWorkingDirectory->clear();

    const QFileInfo programInfo(sProgram);
    const QString sCanonicalProgram = programInfo.canonicalFilePath();
    if (!programInfo.exists() || !programInfo.isFile() || !programInfo.isExecutable() || programInfo.isSymLink() || sCanonicalProgram.isEmpty()) {
        if (pError) *pError = QStringLiteral("External archive helper is not an executable regular file");
        return false;
    }

    const QFileInfo workingInfo(sWorkingDirectory);
    const QString sCanonicalWorkingDirectory = workingInfo.canonicalFilePath();
    if (!workingInfo.exists() || !workingInfo.isDir() || workingInfo.isSymLink() || sCanonicalWorkingDirectory.isEmpty()) {
        if (pError) *pError = QStringLiteral("External archive helper working directory is invalid");
        return false;
    }

    if (!sAuditRoot.isEmpty()) {
        const QString sAbsoluteAuditRoot = QDir::cleanPath(QDir::isAbsolutePath(sAuditRoot) ? sAuditRoot : QDir(sCanonicalWorkingDirectory).absoluteFilePath(sAuditRoot));
        if (!isContainedPath(sCanonicalWorkingDirectory, sAbsoluteAuditRoot)) {
            if (pError)
                *pError = QStringLiteral(
                    "External archive audit root is outside its private "
                    "working directory");
            return false;
        }

        const QFileInfo auditInfo(sAbsoluteAuditRoot);
        const QFileInfo auditParentInfo(auditInfo.absolutePath());
        const bool bAuditIsWorkingDirectory = sAbsoluteAuditRoot.compare(sCanonicalWorkingDirectory, pathCaseSensitivity()) == 0;
        if ((auditInfo.exists() && (auditInfo.isSymLink() || !isContainedPath(sCanonicalWorkingDirectory, auditInfo.canonicalFilePath()))) ||
            (!bAuditIsWorkingDirectory && (!auditParentInfo.exists() || !auditParentInfo.isDir() || auditParentInfo.isSymLink() ||
                                           !isContainedPath(sCanonicalWorkingDirectory, auditParentInfo.canonicalFilePath())))) {
            if (pError)
                *pError = QStringLiteral(
                    "External archive audit root is not a contained regular "
                    "directory path");
            return false;
        }
    }

    *pCanonicalProgram = sCanonicalProgram;
    *pCanonicalWorkingDirectory = sCanonicalWorkingDirectory;
    return true;
}

bool drainProcessOutput(QProcess *pProcess, qint64 *pnCaptured, QByteArray *pDiagnosticTail, QByteArray *pCapturedOutput, QString *pError)
{
    if (!pProcess || !pnCaptured || !pDiagnosticTail) return false;
    while (pProcess->bytesAvailable() > 0) {
        const qint64 nRemaining = EXTERNAL_CAPTURE_LIMIT - *pnCaptured;
        const qint64 nRequest = qMin<qint64>(EXTERNAL_PIPE_READ_SIZE, qMin<qint64>(pProcess->bytesAvailable(), nRemaining + 1));
        const QByteArray baChunk = pProcess->read(nRequest);
        if (baChunk.isEmpty()) break;
        if (baChunk.size() > nRemaining) {
            if (pError) *pError = QStringLiteral("External archive helper produced excessive output");
            return false;
        }

        *pnCaptured += baChunk.size();
        if (pCapturedOutput) pCapturedOutput->append(baChunk);
        pDiagnosticTail->append(baChunk);
        if (pDiagnosticTail->size() > 1024) {
            pDiagnosticTail->remove(0, pDiagnosticTail->size() - 1024);
        }
        if (pDiagnosticTail->contains("Enter decryption password:")) {
            if (pError) *pError = QStringLiteral("External archive password is missing or incorrect");
            return false;
        }
    }
    return true;
}

XExternalArchive::EXTERNAL_FAILURE classifyHelperExit(XExternalArchive::BACKEND backend, const QByteArray &baDiagnosticTail)
{
    const QByteArray baLower = baDiagnosticTail.toLower();
    if (baLower.contains("password") || baLower.contains("archive seems encrypted")) {
        return XExternalArchive::EXTERNAL_FAILURE_PASSWORD;
    }

    bool bArchiveRejected = false;
    if (backend == XExternalArchive::BACKEND_FREEARC) {
        bArchiveRejected = baLower.contains("isn't archive or this archive is corrupt") || baLower.contains("archive signature not found") ||
                           baLower.contains("archive is corrupt") || baLower.contains("crc failed in") || baLower.contains("data error in") ||
                           baLower.contains("file is broken");
    } else if (backend == XExternalArchive::BACKEND_ZPAQ) {
        bArchiveRejected = baLower.contains("not a zpaq") || baLower.contains("fragment checksum failed") || baLower.contains("bad checksum") ||
                           baLower.contains("unexpected end of compressed data") || baLower.contains("incomplete decompression") ||
                           baLower.contains("zpaqfranz error: empty block") || baLower.contains("archive block not found") || baLower.contains("archive corrupted") ||
                           baLower.contains("archive seems corrupt") || baLower.contains("unexpected eof");
    }
    return bArchiveRejected ? XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED : XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
}

bool runTool(XExternalArchive::BACKEND backend, const QString &sProgram, const QStringList &listArguments, const QString &sWorkingDirectory, const QString &sAuditRoot,
             const XBinary::OUTPUT_POLICY &policy, bool bMonitorPeaReport, XBinary::PDSTRUCT *pPdStruct, QByteArray *pCapturedOutput,
             XExternalArchive::EXTERNAL_FAILURE *pFailure, const QDeadlineTimer &aggregateDeadline, const QProcessEnvironment *pChildEnvironment = nullptr)
{
    if (pCapturedOutput) pCapturedOutput->clear();
    if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_CANCELED;
        return false;
    }
    qint64 nTimeoutMs = externalTimeoutMs();
    if (!aggregateDeadline.isForever()) {
        const qint64 nRemainingMs = aggregateDeadline.remainingTime();
        if (nRemainingMs <= 0) {
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_TIMEOUT;
            return setExternalError(pPdStruct, QStringLiteral("External archive helper operation timed out"));
        }
        nTimeoutMs = qMin(nTimeoutMs, nRemainingMs);
    }
    const QDeadlineTimer toolDeadline(nTimeoutMs, Qt::PreciseTimer);
    QElapsedTimer timer;
    timer.start();
    QString sCanonicalProgram;
    QString sCanonicalWorkingDirectory;
    QString sExecutionError;
    if (!validateToolExecutionPaths(sProgram, sWorkingDirectory, sAuditRoot, &sCanonicalProgram, &sCanonicalWorkingDirectory, &sExecutionError)) {
        return setExternalError(pPdStruct, sExecutionError);
    }

#ifdef Q_OS_WIN
    if (!setLowIntegrityDirectoryTree(sCanonicalWorkingDirectory, &sExecutionError)) return setExternalError(pPdStruct, sExecutionError);
    RestrictedChildHandleInheritance restrictedInheritance;
    if (!restrictedInheritance.initialize(&sExecutionError)) return setExternalError(pPdStruct, sExecutionError);
#endif

    ExternalHelperProcess process;
#ifdef Q_OS_WIN
    // Declared after QProcess so kill-on-close runs before QProcess's own
    // destructor fallback on every early return.
    ExternalProcessTreeGuard processTree;
    if (!processTree.isValid()) return setExternalError(pPdStruct, processTree.errorString());
    void *pNativeProcessInformation = nullptr;
#elif defined(Q_OS_UNIX)
    // Declared after QProcess so the process group is killed before QProcess's
    // destructor fallback on every early return.
    ExternalPosixProcessTreeGuard processTree;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QProcess::UnixProcessParameters unixParameters;
    unixParameters.flags = QProcess::UnixProcessFlag::CreateNewSession | QProcess::UnixProcessFlag::CloseFileDescriptors | QProcess::UnixProcessFlag::ResetSignalHandlers;
    unixParameters.lowestFileDescriptorToClose = 3;
    process.setUnixProcessParameters(unixParameters);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    process.setChildProcessModifier(configureExternalChildSession);
#endif
#endif

#ifdef Q_OS_WIN
    ExternalProcessStopper processStopper(&process, &processTree,
                                          &pNativeProcessInformation);
#elif defined(Q_OS_UNIX)
    ExternalProcessStopper processStopper(&process, &processTree);
#else
    ExternalProcessStopper processStopper(&process);
#endif

    process.setProgram(sCanonicalProgram);
    process.setArguments(listArguments);
    process.setWorkingDirectory(sCanonicalWorkingDirectory);
    process.setProcessChannelMode(QProcess::MergedChannels);

    const QProcessEnvironment sourceEnvironment = pChildEnvironment ? *pChildEnvironment : QProcessEnvironment::systemEnvironment();
    QProcessEnvironment childEnvironment;
    const QString sHelperDirectory = QFileInfo(sCanonicalProgram).absolutePath();
#ifdef Q_OS_WIN
    wchar_t awcWindowsDirectory[32768] = {};
    wchar_t awcSystemDirectory[32768] = {};
    const UINT nWindowsDirectoryLength = GetWindowsDirectoryW(awcWindowsDirectory, static_cast<UINT>(sizeof(awcWindowsDirectory) / sizeof(awcWindowsDirectory[0])));
    const UINT nSystemDirectoryLength = GetSystemDirectoryW(awcSystemDirectory, static_cast<UINT>(sizeof(awcSystemDirectory) / sizeof(awcSystemDirectory[0])));
    if (!nWindowsDirectoryLength || !nSystemDirectoryLength || (nWindowsDirectoryLength >= sizeof(awcWindowsDirectory) / sizeof(awcWindowsDirectory[0])) ||
        (nSystemDirectoryLength >= sizeof(awcSystemDirectory) / sizeof(awcSystemDirectory[0]))) {
        return setExternalError(pPdStruct, QStringLiteral("Cannot construct a trusted helper environment"));
    }
    const QString sWindowsDirectory = QDir::cleanPath(QString::fromWCharArray(awcWindowsDirectory, nWindowsDirectoryLength));
    const QString sSystemDirectory = QDir::cleanPath(QString::fromWCharArray(awcSystemDirectory, nSystemDirectoryLength));
    childEnvironment.insert(QStringLiteral("SystemRoot"), sWindowsDirectory);
    childEnvironment.insert(QStringLiteral("WINDIR"), sWindowsDirectory);
    QString sSystemDrive = QDir(sWindowsDirectory).rootPath();
    while (sSystemDrive.endsWith(QLatin1Char('/')) || sSystemDrive.endsWith(QLatin1Char('\\'))) sSystemDrive.chop(1);
    childEnvironment.insert(QStringLiteral("SystemDrive"), sSystemDrive);
    childEnvironment.insert(QStringLiteral("COMSPEC"), QDir(sSystemDirectory).filePath(QStringLiteral("cmd.exe")));
    childEnvironment.insert(QStringLiteral("PATHEXT"), QStringLiteral(".COM;.EXE;.BAT;.CMD"));
    childEnvironment.insert(QStringLiteral("PATH"), QStringList({sHelperDirectory, sSystemDirectory, sWindowsDirectory}).join(QDir::listSeparator()));
#else
    childEnvironment.insert(QStringLiteral("PATH"), QStringList({sHelperDirectory, QStringLiteral("/usr/bin"), QStringLiteral("/bin")}).join(QDir::listSeparator()));
    childEnvironment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    childEnvironment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
#endif
    // FRANZKEY is forwarded only from an explicit per-child environment.
    // Default ambient invocations drop it, FREEARC, FRANZFRANZEN, and every
    // other arbitrary parent variable.
    if (pChildEnvironment && sourceEnvironment.contains(QStringLiteral("FRANZKEY"))) {
        childEnvironment.insert(QStringLiteral("FRANZKEY"), sourceEnvironment.value(QStringLiteral("FRANZKEY")));
    }
    childEnvironment.insert(QStringLiteral("TEMP"), sCanonicalWorkingDirectory);
    childEnvironment.insert(QStringLiteral("TMP"), sCanonicalWorkingDirectory);
#ifndef Q_OS_WIN
    childEnvironment.insert(QStringLiteral("TMPDIR"), sCanonicalWorkingDirectory);
#endif
    process.setProcessEnvironment(childEnvironment);

#ifdef Q_OS_WIN
    const RestrictedProcessArgumentsModifier argumentsModifier(
        &pNativeProcessInformation, &restrictedInheritance, &sExecutionError);
    process.setCreateProcessArgumentsModifier(argumentsModifier);
#endif

    process.start(QIODevice::ReadOnly);

    bool bStarted = process.state() == QProcess::Running;
    while (!bStarted && (process.state() == QProcess::Starting) && (timer.elapsed() < qMin<qint64>(5000, nTimeoutMs))) {
        bStarted = process.waitForStarted(50) || (process.state() == QProcess::Running);
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_CANCELED;
            processStopper.stop();
            return false;
        }
    }
    bStarted = bStarted || (process.state() == QProcess::Running);
    if (!bStarted) {
        processStopper.stop();
        if (timer.elapsed() >= nTimeoutMs) {
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_TIMEOUT;
            return setExternalError(pPdStruct, QStringLiteral("External archive helper operation timed out"));
        }
        return setExternalError(pPdStruct, QStringLiteral("Cannot start external archive helper"));
    }

#ifdef Q_OS_WIN
    if (!restrictedInheritance.isConfigured()) {
        processStopper.stop();
        return setExternalError(pPdStruct, sExecutionError.isEmpty() ? QStringLiteral("Cannot configure the external-helper handle allowlist") : sExecutionError);
    }
    if (!processTree.attachAndResume(pNativeProcessInformation, &sExecutionError)) {
        processStopper.stop();
        return setExternalError(pPdStruct, sExecutionError);
    }
#elif defined(Q_OS_UNIX)
    if (!processTree.capture(&process, &sExecutionError)) {
        processStopper.stop();
        return setExternalError(pPdStruct, sExecutionError);
    }
#endif
    process.closeWriteChannel();

    QElapsedTimer auditTimer;
    auditTimer.start();
    qint64 nCaptured = 0;
    QByteArray baDiagnosticTail;
    bool bResult = true;

    for (;;) {
        if (process.state() != QProcess::NotRunning) process.waitForFinished(50);
        else QThread::msleep(10);

        QString sOutputError;
        if (!drainProcessOutput(&process, &nCaptured, &baDiagnosticTail, pCapturedOutput, &sOutputError)) {
            bResult = false;
            if (pFailure) {
                *pFailure = sOutputError.contains(QStringLiteral("password"), Qt::CaseInsensitive) ? XExternalArchive::EXTERNAL_FAILURE_PASSWORD
                                                                                                   : XExternalArchive::EXTERNAL_FAILURE_RESOURCE_LIMIT;
            }
            setExternalError(pPdStruct, sOutputError);
            break;
        }

        bool bTreeRunning = process.state() != QProcess::NotRunning;
#ifdef Q_OS_WIN
        if (!processTree.hasActiveProcesses(&bTreeRunning, &sExecutionError)) {
            bResult = false;
            setExternalError(pPdStruct, sExecutionError);
            break;
        }
#elif defined(Q_OS_UNIX)
        if (!processTree.hasActiveProcesses(&bTreeRunning, &sExecutionError)) {
            bResult = false;
            setExternalError(pPdStruct, sExecutionError);
            break;
        }
#endif
        if (!bTreeRunning) break;

        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_CANCELED;
            break;
        }
        if (timer.elapsed() >= nTimeoutMs) {
            bResult = false;
            if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_TIMEOUT;
            setExternalError(pPdStruct, QStringLiteral("External archive helper operation timed out"));
            break;
        }
        if (auditTimer.elapsed() >= 250) {
            if (bMonitorPeaReport && (inspectPeaReport(sWorkingDirectory) == PEA_REPORT_FAILURE)) {
                bResult = false;
                if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_ARCHIVE_REJECTED;
                setExternalError(pPdStruct, QStringLiteral("PEA archive integrity verification failed"));
                break;
            }
            if (!sAuditRoot.isEmpty() && QFileInfo::exists(sAuditRoot)) {
                StageScanResult scanResult;
                bool bScanTimedOut = false;
                if (!scanStageTree(sAuditRoot, policy, QString(), false, &scanResult, pPdStruct, &toolDeadline, &bScanTimedOut)) {
                    bResult = false;
                    const bool bScanCanceled = !XBinary::isPdStructNotCanceled(pPdStruct);
                    if (pFailure) {
                        *pFailure = bScanCanceled ? XExternalArchive::EXTERNAL_FAILURE_CANCELED
                                                  : (bScanTimedOut ? XExternalArchive::EXTERNAL_FAILURE_TIMEOUT : XExternalArchive::EXTERNAL_FAILURE_RESOURCE_LIMIT);
                    }
                    if (!bScanCanceled) {
                        setExternalError(pPdStruct, bScanTimedOut ? QStringLiteral("External archive helper operation timed out")
                                                                  : QStringLiteral("External archive output failed safety limits"));
                    }
                    break;
                }
            }
            auditTimer.restart();
        }
    }

    if (!bResult) {
        processStopper.stop();
        while (process.bytesAvailable() > 0) process.read(qMin<qint64>(process.bytesAvailable(), EXTERNAL_PIPE_READ_SIZE));
        return false;
    }

    QString sOutputError;
    if (!drainProcessOutput(&process, &nCaptured, &baDiagnosticTail, pCapturedOutput, &sOutputError)) {
        if (pFailure) {
            *pFailure = sOutputError.contains(QStringLiteral("password"), Qt::CaseInsensitive) ? XExternalArchive::EXTERNAL_FAILURE_PASSWORD
                                                                                               : XExternalArchive::EXTERNAL_FAILURE_RESOURCE_LIMIT;
        }
        return setExternalError(pPdStruct, sOutputError);
    }

    if ((process.exitStatus() != QProcess::NormalExit) || (process.exitCode() != 0)) {
        if (pFailure) {
            if (process.exitStatus() != QProcess::NormalExit) {
                *pFailure = XExternalArchive::EXTERNAL_FAILURE_INFRASTRUCTURE;
            } else {
                *pFailure = classifyHelperExit(backend, baDiagnosticTail);
            }
        }
        return setExternalError(pPdStruct, QStringLiteral("External archive helper failed (exit code %1)").arg(process.exitCode()));
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_CANCELED;
        return false;
    }

    if (!sAuditRoot.isEmpty()) {
        StageScanResult scanResult;
        bool bScanTimedOut = false;
        if (!scanStageTree(sAuditRoot, policy, QString(), false, &scanResult, pPdStruct, &toolDeadline, &bScanTimedOut)) {
            const bool bScanCanceled = !XBinary::isPdStructNotCanceled(pPdStruct);
            if (pFailure) {
                *pFailure = bScanCanceled ? XExternalArchive::EXTERNAL_FAILURE_CANCELED
                                          : (bScanTimedOut ? XExternalArchive::EXTERNAL_FAILURE_TIMEOUT : XExternalArchive::EXTERNAL_FAILURE_RESOURCE_LIMIT);
            }
            if (bScanCanceled) return false;
            return setExternalError(pPdStruct, bScanTimedOut ? QStringLiteral("External archive helper operation timed out")
                                                             : QStringLiteral("External archive output failed safety limits"));
        }
    }
    if (pFailure) *pFailure = XExternalArchive::EXTERNAL_FAILURE_NONE;
    return true;
}

bool verifyPeaReport(const QString &sWorkingDirectory)
{
    return inspectPeaReport(sWorkingDirectory) == PEA_REPORT_SUCCESS;
}

enum EXTERNAL_PARSE_RESULT {
    EXTERNAL_PARSE_OK = 0,
    EXTERNAL_PARSE_INVALID,
    EXTERNAL_PARSE_RESOURCE_LIMIT
};

EXTERNAL_PARSE_RESULT parseZpaqList(const QByteArray &baOutput, const XBinary::OUTPUT_POLICY &policy, QList<ExternalRecord> *pRecords)
{
    if (!pRecords) return EXTERNAL_PARSE_INVALID;
    pRecords->clear();

    const QString sOutput = QString::fromUtf8(baOutput);
    if (sOutput.contains(QChar::ReplacementCharacter)) return EXTERNAL_PARSE_INVALID;

    // -noattributes makes the stable 7.15 record grammar independent of the
    // archived platform. Without it, the attribute field is variable width
    // (Windows flags or a five-character Unix mode). There is exactly one
    // separator after the size; preserve any leading spaces in the filename.
    const QRegularExpression expression(
        QStringLiteral("^- (\\d{4}-\\d{2}-\\d{2}) "
                       "(\\d{2}:\\d{2}:\\d{2})\\s+"
                       "(\\d+) (.*)$"));
    QSet<QString> setNames;
    qint64 nTotalSize = 0;
    const QStringList listLines = sOutput.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (const QString &sLine : listLines) {
        const QRegularExpressionMatch match = expression.match(sLine);
        if (!match.hasMatch()) continue;

        bool bSizeOK = false;
        const qint64 nListedSize = match.captured(3).toLongLong(&bSizeOK);
        if (!bSizeOK || (nListedSize < 0)) return EXTERNAL_PARSE_INVALID;

        const QString sRawName = match.captured(4);
        const bool bIsFolder = sRawName.endsWith(QLatin1Char('/'));
        QString sName;
        if (!normalizeZpaqExtractedName(sRawName, &sName)) return EXTERNAL_PARSE_INVALID;
        const QString sKey = (pathCaseSensitivity() == Qt::CaseInsensitive) ? sName.toCaseFolded() : sName;
        if (setNames.contains(sKey)) return EXTERNAL_PARSE_INVALID;
        setNames.insert(sKey);

        ExternalRecord record;
        record.sName = sName;
        record.sProvider = QStringLiteral("zpaqfranz");
        record.bIsFolder = bIsFolder;
        record.nUncompressedSize = record.bIsFolder ? 0 : nListedSize;
        record.dateTime = QDateTime::fromString(match.captured(1) + QLatin1Char('T') + match.captured(2) + QLatin1Char('Z'), Qt::ISODate);
        if (!record.dateTime.isValid()) return EXTERNAL_PARSE_INVALID;

        if (!record.bIsFolder && !addWithLimit(record.nUncompressedSize, &nTotalSize)) return EXTERNAL_PARSE_RESOURCE_LIMIT;
        const qint64 nCount = pRecords->size() + 1LL;
        if (!policyAllows(record.nUncompressedSize, nTotalSize, nCount, policy)) return EXTERNAL_PARSE_RESOURCE_LIMIT;
        pRecords->append(record);
    }

    if (!pRecords->isEmpty()) {
        // initUnpack historically exposed the extracted stage's sorted order.
        // Keep that order even though listing no longer creates the stage.
        sortExternalRecords(pRecords);
        return EXTERNAL_PARSE_OK;
    }

    // A successful empty journal has no per-entry lines. Require both of the
    // stable 7.15 summary statements so arbitrary/partial helper output cannot
    // be mistaken for an authoritative empty archive.
    const QRegularExpression emptyArchiveExpression(
        QStringLiteral("(?m)^.+: \\d+ versions, 0 files, "
                       "\\d+ fragments, \\d+\\.\\d+ MB\\r?$"));
    const QRegularExpression emptyListingExpression(
        QStringLiteral("(?m)^\\d+\\.\\d+ MB of \\d+\\.\\d+ MB "
                       "\\(0 files\\) shown\\r?$"));
    return (emptyArchiveExpression.match(sOutput).hasMatch() && emptyListingExpression.match(sOutput).hasMatch()) ? EXTERNAL_PARSE_OK : EXTERNAL_PARSE_INVALID;
}

bool decodeFreeArcOutput(const QByteArray &baOutput, QString *pOutput)
{
    if (!pOutput) return false;
#ifdef Q_OS_WIN
    if (baOutput.isEmpty()) {
        pOutput->clear();
        return true;
    }
    const int nLength = MultiByteToWideChar(CP_OEMCP, 0, baOutput.constData(), baOutput.size(), nullptr, 0);
    if (nLength <= 0) return false;
    QVector<wchar_t> listCharacters(nLength);
    if (MultiByteToWideChar(CP_OEMCP, 0, baOutput.constData(), baOutput.size(), listCharacters.data(), nLength) != nLength) return false;
    *pOutput = QString::fromWCharArray(listCharacters.constData(), nLength);
#else
    *pOutput = QString::fromLocal8Bit(baOutput);
#endif
    return !pOutput->contains(QChar::Null);
}

bool parseFreeArcNumber(const QString &sValue, qint64 *pnValue)
{
    if (!pnValue || sValue.isEmpty()) return false;
    const QStringList listGroups = sValue.split(QLatin1Char(','));
    if (listGroups.size() > 1) {
        if (listGroups.first().isEmpty() || (listGroups.first().size() > 3)) return false;
        for (qint32 i = 1; i < listGroups.size(); ++i) {
            if (listGroups.at(i).size() != 3) return false;
        }
    }
    QString sDigits = sValue;
    sDigits.remove(QLatin1Char(','));
    bool bOK = false;
    const qint64 nValue = sDigits.toLongLong(&bOK);
    if (!bOK || (nValue < 0)) return false;
    *pnValue = nValue;
    return true;
}

EXTERNAL_PARSE_RESULT parseFreeArcList(const QByteArray &baOutput, const XBinary::OUTPUT_POLICY &policy, QList<ExternalRecord> *pRecords)
{
    if (!pRecords) return EXTERNAL_PARSE_INVALID;
    pRecords->clear();

    QString sOutput;
    if (!decodeFreeArcOutput(baOutput, &sOutput)) return EXTERNAL_PARSE_INVALID;
    const QRegularExpression recordExpression(
        QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}) "
                       "(\\d{2}:\\d{2}:\\d{2})\\s+"
                       "(\\S{7})\\s+"
                       "((?:\\d+|\\d{1,3}(?:,\\d{3})+))\\s+"
                       "((?:\\d+|\\d{1,3}(?:,\\d{3})+))\\s+"
                       "([0-9A-Fa-f]{8})([ *])(.*)$"));
    const QRegularExpression summaryExpression(
        QStringLiteral("^((?:\\d+|\\d{1,3}(?:,\\d{3})+)) files?, "
                       "((?:\\d+|\\d{1,3}(?:,\\d{3})+)) bytes, "
                       "((?:\\d+|\\d{1,3}(?:,\\d{3})+)) compressed$"));
    const QRegularExpression headerExpression(QStringLiteral("^Date/time\\s+Attr\\s+Size\\s+Packed\\s+CRC Filename$"));

    QSet<QString> setNames;
    qint64 nTotalSize = 0;
    qint64 nSummaryCount = -1;
    qint64 nSummarySize = -1;
    bool bSawHeader = false;
    bool bSawSuccess = false;
    const QStringList listLines = sOutput.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (const QString &sLine : listLines) {
        if (headerExpression.match(sLine).hasMatch()) {
            if (bSawHeader) return EXTERNAL_PARSE_INVALID;
            bSawHeader = true;
            continue;
        }
        if (sLine == QLatin1String("All OK")) {
            if (bSawSuccess) return EXTERNAL_PARSE_INVALID;
            bSawSuccess = true;
            continue;
        }

        const QRegularExpressionMatch summaryMatch = summaryExpression.match(sLine);
        if (summaryMatch.hasMatch()) {
            if (nSummaryCount >= 0 || !parseFreeArcNumber(summaryMatch.captured(1), &nSummaryCount) || !parseFreeArcNumber(summaryMatch.captured(2), &nSummarySize))
                return EXTERNAL_PARSE_INVALID;
            qint64 nSummaryCompressed = 0;
            if (!parseFreeArcNumber(summaryMatch.captured(3), &nSummaryCompressed)) return EXTERNAL_PARSE_INVALID;
            continue;
        }

        const QRegularExpressionMatch match = recordExpression.match(sLine);
        if (!match.hasMatch()) continue;

        qint64 nSize = 0;
        qint64 nCompressedSize = 0;
        if (!parseFreeArcNumber(match.captured(4), &nSize) || !parseFreeArcNumber(match.captured(5), &nCompressedSize)) {
            return EXTERNAL_PARSE_INVALID;
        }

        QString sName;
        if (!normalizeRecordName(match.captured(8), &sName)) return EXTERNAL_PARSE_INVALID;
        const QString sKey = (pathCaseSensitivity() == Qt::CaseInsensitive) ? sName.toCaseFolded() : sName;
        if (setNames.contains(sKey)) return EXTERNAL_PARSE_INVALID;
        setNames.insert(sKey);

        ExternalRecord record;
        record.sName = sName;
        record.sProvider = QStringLiteral("FreeArc");
        record.bIsFolder = match.captured(3).contains(QLatin1Char('D'));
        if (record.bIsFolder && (nSize != 0)) return EXTERNAL_PARSE_INVALID;
        record.nUncompressedSize = record.bIsFolder ? 0 : nSize;
        record.nCompressedSize = nCompressedSize;
        record.dateTime = QDateTime::fromString(match.captured(1) + QLatin1Char('T') + match.captured(2), Qt::ISODate);
        if (!record.dateTime.isValid()) return EXTERNAL_PARSE_INVALID;

        if (!record.bIsFolder && !addWithLimit(record.nUncompressedSize, &nTotalSize)) return EXTERNAL_PARSE_RESOURCE_LIMIT;
        const qint64 nCount = pRecords->size() + 1LL;
        if (!policyAllows(record.nUncompressedSize, nTotalSize, nCount, policy)) return EXTERNAL_PARSE_RESOURCE_LIMIT;
        pRecords->append(record);
    }

    if (!bSawHeader || !bSawSuccess || (nSummaryCount < 0) || (nSummarySize < 0) || (nSummaryCount != pRecords->size()) || (nSummarySize != nTotalSize))
        return EXTERNAL_PARSE_INVALID;
    sortExternalRecords(pRecords);
    return EXTERNAL_PARSE_OK;
}

bool externalCanContinue(XBinary::PDSTRUCT *pPdStruct,
                         const QDeadlineTimer *pDeadline, bool *pbTimedOut)
{
    if (pDeadline && !pDeadline->isForever() && pDeadline->hasExpired()) {
        if (pbTimedOut) *pbTimedOut = true;
        return false;
    }
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool reconcileListedStage(const QList<ExternalRecord> &listManifest, QList<ExternalRecord> *pStageRecords, XBinary::PDSTRUCT *pPdStruct, const QDeadlineTimer *pDeadline,
                          bool *pbTimedOut)
{
    if (pbTimedOut) *pbTimedOut = false;
    if (!pStageRecords || (pStageRecords->size() < listManifest.size())) return false;
    if (listManifest.isEmpty()) return pStageRecords->isEmpty();

    if (!externalCanContinue(pPdStruct, pDeadline, pbTimedOut)) return false;

    QList<ExternalRecord> listExpected = listManifest;
    std::sort(listExpected.begin(), listExpected.end(), externalRecordLess);
    std::sort(pStageRecords->begin(), pStageRecords->end(), externalRecordLess);
    if (!externalCanContinue(pPdStruct, pDeadline, pbTimedOut)) return false;

    QSet<QString> setStrictAncestorFolders;
    for (const ExternalRecord &expected : qAsConst(listExpected)) {
        if (!externalCanContinue(pPdStruct, pDeadline, pbTimedOut)) return false;
        qint32 nSeparator = expected.sName.indexOf(QLatin1Char('/'));
        while (nSeparator > 0) {
            setStrictAncestorFolders.insert(expected.sName.left(nSeparator));
            nSeparator = expected.sName.indexOf(QLatin1Char('/'), nSeparator + 1);
        }
    }

    QList<ExternalRecord> listReconciled;
    listReconciled.reserve(listExpected.size());
    qint32 nExpectedIndex = 0;
    for (ExternalRecord actual : qAsConst(*pStageRecords)) {
        if (!externalCanContinue(pPdStruct, pDeadline, pbTimedOut)) return false;
        if ((nExpectedIndex < listExpected.size()) && (actual.sName == listExpected.at(nExpectedIndex).sName)) {
            const ExternalRecord &expected = listExpected.at(nExpectedIndex++);
            if ((actual.bIsFolder != expected.bIsFolder) || (actual.nUncompressedSize != expected.nUncompressedSize)) {
                return false;
            }
            actual.sProvider = expected.sProvider;
            actual.nCompressedSize = expected.nCompressedSize;
            if (expected.dateTime.isValid()) actual.dateTime = expected.dateTime;
            listReconciled.append(actual);
            continue;
        }

        // External helpers do not always list parent folders explicitly,
        // although the filesystem necessarily creates them while extracting a
        // nested member. Accept only such implicit folders and omit them from
        // the archive record list; every file and every non-ancestor directory
        // must still have an exact manifest entry.
        if (!actual.bIsFolder) return false;
        if (!setStrictAncestorFolders.contains(actual.sName)) return false;
    }
    if (nExpectedIndex != listExpected.size()) return false;
    *pStageRecords = listReconciled;
    return true;
}

QString singleRecordName(QIODevice *pDevice)
{
    QString sName = XBinary::getDeviceFileBaseName(pDevice);
    if (sName.isEmpty()) sName = QStringLiteral("data");
    QString sNormalized;
    if (!normalizeRecordName(sName, &sNormalized)) sNormalized = QStringLiteral("data");
    return sNormalized;
}

quint32 readBigEndian32(const QByteArray &baData)
{
    if (baData.size() < 4) return 0;
    return (quint32)(quint8)baData.at(0) << 24 | (quint32)(quint8)baData.at(1) << 16 | (quint32)(quint8)baData.at(2) << 8 | (quint32)(quint8)baData.at(3);
}

QString withTrailingSeparator(const QString &sPath)
{
    QString sResult = QDir::toNativeSeparators(sPath);
    if (!sResult.endsWith(QDir::separator())) sResult.append(QDir::separator());
    return sResult;
}

bool helperContainsAsciiMarker(const QString &sPath, const QByteArray &baMarker)
{
    const qint64 nMaximumHelperSize = 128LL * 1024LL * 1024LL;
    const qint64 nReadSize = 64LL * 1024LL;
    if (baMarker.isEmpty()) return false;

    QFile file(sPath);
    const qint64 nSize = file.size();
    if ((nSize < baMarker.size()) || (nSize > nMaximumHelperSize) || !file.open(QIODevice::ReadOnly) || file.isSequential()) {
        return false;
    }

    QByteArray baOverlap;
    while (!file.atEnd()) {
        const QByteArray baChunk = file.read(nReadSize);
        if (baChunk.isEmpty()) return false;
        const QByteArray baWindow = baOverlap + baChunk;
        if (baWindow.contains(baMarker)) return true;
        baOverlap = baWindow.right(baMarker.size() - 1);
    }
    return false;
}
}  // namespace

struct XExternalArchive::EXTERNAL_UNPACK_CONTEXT {
    ~EXTERNAL_UNPACK_CONTEXT()
    {
        pFreeArcPasswordFile.reset();
        if (!sPassword.isEmpty()) {
            sPassword.fill(QChar::Null);
            sPassword.clear();
        }
    }

    std::unique_ptr<QTemporaryDir> pTemporaryDir;
    std::unique_ptr<QTemporaryDir> pArchiveStageDir;
    std::unique_ptr<QTemporaryFile> pFreeArcPasswordFile;
    QString sInputPath;
    QString sHelperPath;
    QString sPassword;
    QList<ExternalRecord> listRecords;
    XBinary::OUTPUT_POLICY outputPolicy = {};
    bool bZpaqEnvironmentPassword = false;
};

XExternalArchive::XExternalArchive(QIODevice *pDevice, BACKEND backend)
    : XArchive(pDevice), m_backend(backend), m_helperDeadline(QDeadlineTimer::Forever), m_lastExternalFailure(EXTERNAL_FAILURE_NONE)
{
}

XExternalArchive::BACKEND XExternalArchive::getExternalBackend() const
{
    return m_backend;
}

qint64 XExternalArchive::getConfiguredHelperTimeoutMs()
{
    return externalTimeoutMs();
}

QDeadlineTimer XExternalArchive::createHelperDeadline()
{
    return QDeadlineTimer(getConfiguredHelperTimeoutMs(), Qt::PreciseTimer);
}

void XExternalArchive::setHelperDeadline(const QDeadlineTimer &deadline)
{
    m_helperDeadline = deadline;
}

void XExternalArchive::clearHelperDeadline()
{
    m_helperDeadline = QDeadlineTimer(QDeadlineTimer::Forever);
}

XExternalArchive::EXTERNAL_FAILURE XExternalArchive::getLastExternalFailure() const
{
    return m_lastExternalFailure;
}

bool XExternalArchive::isDeferredArchiveMaterialized(const UNPACK_STATE *pState) const
{
    if (!pState || !pState->pContext || ((m_backend != BACKEND_ZPAQ) && (m_backend != BACKEND_FREEARC))) return false;

    const EXTERNAL_UNPACK_CONTEXT *pContext = static_cast<const EXTERNAL_UNPACK_CONTEXT *>(pState->pContext);
    return pContext->pArchiveStageDir != nullptr;
}

void XExternalArchive::setLastExternalFailure(EXTERNAL_FAILURE failure)
{
    m_lastExternalFailure = failure;
}

QMap<XBinary::UNPACK_PROP, QVariant> XExternalArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XExternalArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XExternalArchive> guardedThis(this);
    m_lastExternalFailure = EXTERNAL_FAILURE_INFRASTRUCTURE;
    if (!pState || (m_backend == BACKEND_UNKNOWN) || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)))
        return false;

    if (!finishUnpack(pState, nullptr) || !guardedThis) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;
    if (!m_helperDeadline.isForever() && m_helperDeadline.hasExpired()) {
        m_lastExternalFailure = EXTERNAL_FAILURE_TIMEOUT;
        return setExternalError(pPdStruct, tr("External archive helper operation timed out"));
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !guardedThis->isValid(pPdStruct) || !guardedThis) return false;

    XBinary::OUTPUT_POLICY outputPolicy = {};
    if (!XBinary::resolveUnpackOutputPolicy(mapProperties, &outputPolicy)) {
        m_lastExternalFailure = EXTERNAL_FAILURE_RESOURCE_LIMIT;
        return setExternalError(pPdStruct, tr("Invalid unpacked-output policy"));
    }

    const QString sHelperPath = resolveHelper(m_backend);
    if (sHelperPath.isEmpty()) {
        m_lastExternalFailure = EXTERNAL_FAILURE_INFRASTRUCTURE;
        return setExternalError(pPdStruct, tr("%1 helper was not found. Install PeaZip or set XFU_PEAZIP_ROOT.").arg(backendName(m_backend)));
    }

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    std::unique_ptr<EXTERNAL_UNPACK_CONTEXT> pContext(new (std::nothrow) EXTERNAL_UNPACK_CONTEXT);
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    pContext->pTemporaryDir.reset(new (std::nothrow) QTemporaryDir(QDir(QDir::tempPath()).filePath(QStringLiteral("xfileunpacker-external-XXXXXX"))));
    if (!pContext->pTemporaryDir || !pContext->pTemporaryDir->isValid()) {
        releaseUnpackSource(pState);
        return setExternalError(pPdStruct, tr("Cannot create a private extraction directory"));
    }
    pContext->sHelperPath = sHelperPath;
    const QString sTextPassword = mapProperties.value(UNPACK_PROP_PASSWORD).toString();
    const QByteArray baPassword = mapProperties.value(UNPACK_PROP_PASSWORD_BYTES).toByteArray();
    if (!sTextPassword.isEmpty() && !baPassword.isEmpty()) {
        releaseUnpackSource(pState);
        return setExternalError(pPdStruct, tr("Specify either a text or byte archive password, not both"));
    }
    if (!baPassword.isEmpty()) {
        if (baPassword.contains('\0')) {
            releaseUnpackSource(pState);
            return setExternalError(pPdStruct, tr("External archive helper passwords cannot contain NUL bytes"));
        }
        pContext->sPassword = QString::fromUtf8(baPassword.constData(), baPassword.size());
        if (pContext->sPassword.toUtf8() != baPassword) {
            releaseUnpackSource(pState);
            return setExternalError(pPdStruct, tr("External archive helper password bytes must be valid UTF-8"));
        }
    } else {
        if (sTextPassword.contains(QChar::Null)) {
            releaseUnpackSource(pState);
            return setExternalError(pPdStruct, tr("External archive helper passwords cannot contain NUL characters"));
        }
        pContext->sPassword = sTextPassword;
    }
    pContext->outputPolicy = outputPolicy;

    if ((m_backend == BACKEND_FREEARC) && !pContext->sPassword.isEmpty()) {
        // FreeArc derives the same key from an empty command-line password
        // followed by key-file bytes as it does from those bytes supplied via
        // -p. Keep the secret out of the process command line while preserving
        // the helper's native local-8-bit argv semantics.
        QByteArray baFreeArcPassword = pContext->sPassword.toLocal8Bit();
        pContext->pFreeArcPasswordFile.reset(new (std::nothrow)
                                                 QTemporaryFile(QDir(pContext->pTemporaryDir->path()).filePath(QStringLiteral("freearc-password-XXXXXX.key"))));
        bool bPasswordFileReady = !baFreeArcPassword.isEmpty() && pContext->pFreeArcPasswordFile && pContext->pFreeArcPasswordFile->open();
        qint64 nPasswordWritten = 0;
        while (bPasswordFileReady && (nPasswordWritten < baFreeArcPassword.size())) {
            const qint64 nWritten = pContext->pFreeArcPasswordFile->write(baFreeArcPassword.constData() + nPasswordWritten, baFreeArcPassword.size() - nPasswordWritten);
            if (nWritten <= 0) {
                bPasswordFileReady = false;
                break;
            }
            nPasswordWritten += nWritten;
        }
        bPasswordFileReady = bPasswordFileReady && (nPasswordWritten == baFreeArcPassword.size()) && pContext->pFreeArcPasswordFile->flush();
#ifndef Q_OS_WIN
        if (bPasswordFileReady) {
            bPasswordFileReady = QFile::setPermissions(pContext->pFreeArcPasswordFile->fileName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
#endif
        if (pContext->pFreeArcPasswordFile) pContext->pFreeArcPasswordFile->close();
        if (bPasswordFileReady) {
            const QFileInfo passwordFileInfo(pContext->pFreeArcPasswordFile->fileName());
            bPasswordFileReady = passwordFileInfo.isFile() && !passwordFileInfo.isSymLink() && (passwordFileInfo.size() == baFreeArcPassword.size()) &&
                                 isContainedPath(QFileInfo(pContext->pTemporaryDir->path()).canonicalFilePath(), passwordFileInfo.canonicalFilePath());
        }
        // Minimize additional plaintext copies. The context retains the
        // password only until the lazy archive stage has been materialized.
        baFreeArcPassword.fill('\0');
        baFreeArcPassword.clear();
        if (!bPasswordFileReady) {
            releaseUnpackSource(pState);
            return setExternalError(pPdStruct, tr("Cannot create a private FreeArc password file"));
        }
    }
    if ((m_backend == BACKEND_ZPAQ) && !pContext->sPassword.isEmpty()) {
        // zpaqfranz 64.8 introduced FRANZKEY as an exact fallback for -key.
        // The bundled PeaZip 62.x helper lacks it, so retain a compatible
        // command-line fallback only when the capability marker is absent.
        pContext->bZpaqEnvironmentPassword = helperContainsAsciiMarker(pContext->sHelperPath, QByteArray("FRANZKEY"));
    }

    qint64 nInputSize = getFileFormatSize(pPdStruct);
    const qint64 nDeviceSize = getSize();
    if (!guardedThis || (nInputSize <= 0) || (nInputSize > nDeviceSize)) nInputSize = nDeviceSize;
    if ((nInputSize < 0) || !guardedThis) {
        releaseUnpackSource(pState);
        return false;
    }
    const QString sSuffix = getFileFormatExt().isEmpty() ? QStringLiteral("bin") : getFileFormatExt();
    pContext->sInputPath = QDir(pContext->pTemporaryDir->path()).filePath(QStringLiteral("input.%1").arg(sSuffix));

    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) {
        releaseUnpackSource(pState);
        return setExternalError(pPdStruct, tr("External archive input must be seekable"));
    }
    const qint64 nOriginalPosition = guardedSource->pos();
    QFile inputFile(pContext->sInputPath);
    bool bPrepared = inputFile.open(QIODevice::WriteOnly | QIODevice::Truncate) && guardedSource->seek(0);
    QByteArray baBuffer;
    if (bPrepared) {
        baBuffer.resize((int)EXTERNAL_COPY_BUFFER_SIZE);
        bPrepared = (baBuffer.size() == EXTERNAL_COPY_BUFFER_SIZE);
    }
    qint64 nCopied = 0;
    bool bCopyTimedOut = false;
    while (bPrepared && guardedThis && guardedSource && (nCopied < nInputSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!m_helperDeadline.isForever() && m_helperDeadline.hasExpired()) {
            bCopyTimedOut = true;
            bPrepared = false;
            break;
        }
        const qint64 nRequest = qMin<qint64>(baBuffer.size(), nInputSize - nCopied);
        const qint64 nRead = guardedSource->read(baBuffer.data(), nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) {
            bPrepared = false;
            break;
        }
        qint64 nWrittenTotal = 0;
        while (bPrepared && (nWrittenTotal < nRead)) {
            const qint64 nWritten = inputFile.write(baBuffer.constData() + nWrittenTotal, nRead - nWrittenTotal);
            if ((nWritten <= 0) || (nWritten > nRead - nWrittenTotal)) {
                bPrepared = false;
                break;
            }
            nWrittenTotal += nWritten;
        }
        nCopied += nRead;
    }
    bPrepared = bPrepared && guardedThis && guardedSource && (nCopied == nInputSize) && inputFile.flush();
    inputFile.close();
    if (guardedSource && (nOriginalPosition >= 0)) bPrepared = guardedSource->seek(nOriginalPosition) && bPrepared;
    bPrepared =
        bPrepared && QFileInfo(pContext->sInputPath).isFile() && (QFileInfo(pContext->sInputPath).size() == nInputSize) && XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bPrepared || !guardedThis || !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct)) {
        if (guardedThis) releaseUnpackSource(pState);
        if (bCopyTimedOut) {
            m_lastExternalFailure = EXTERNAL_FAILURE_TIMEOUT;
            return setExternalError(pPdStruct, tr("External archive helper operation timed out"));
        }
        return setExternalError(pPdStruct, tr("Cannot materialize archive input"));
    }

    bool bInitialized = false;
    const QString sWorkDir = pContext->pTemporaryDir->path();
    if (m_backend == BACKEND_ZPAQ) {
        QByteArray baListOutput;
        QStringList listArguments;
        listArguments << QStringLiteral("l") << pContext->sInputPath << QStringLiteral("-715") << QStringLiteral("-noattributes");
        QProcessEnvironment passwordEnvironment;
        const QProcessEnvironment *pPasswordEnvironment = nullptr;
        if (!pContext->sPassword.isEmpty()) {
            if (pContext->bZpaqEnvironmentPassword) {
                passwordEnvironment.insert(QStringLiteral("FRANZKEY"), pContext->sPassword);
                pPasswordEnvironment = &passwordEnvironment;
            } else {
                listArguments << QStringLiteral("-key") << pContext->sPassword;
            }
        }
        QList<ExternalRecord> listManifest;
        bInitialized = runTool(m_backend, pContext->sHelperPath, listArguments, sWorkDir, QString(), outputPolicy, false, pPdStruct, &baListOutput,
                               &m_lastExternalFailure, m_helperDeadline, pPasswordEnvironment);
        if (bInitialized) {
            const EXTERNAL_PARSE_RESULT parseResult = parseZpaqList(baListOutput, outputPolicy, &listManifest);
            if (parseResult != EXTERNAL_PARSE_OK) {
                m_lastExternalFailure = (parseResult == EXTERNAL_PARSE_RESOURCE_LIMIT) ? EXTERNAL_FAILURE_RESOURCE_LIMIT : EXTERNAL_FAILURE_ARCHIVE_REJECTED;
                bInitialized = setExternalError(
                    pPdStruct, (parseResult == EXTERNAL_PARSE_RESOURCE_LIMIT) ? tr("ZPAQ directory exceeds safety limits") : tr("Cannot read ZPAQ directory"));
            }
        }
        // zpaqfranz exposes an authoritative manifest. Keep init/listing
        // metadata-only and create the private extraction stage on the first
        // file request instead.
        if (bInitialized) pContext->listRecords = listManifest;
    } else if ((m_backend == BACKEND_BCM) || (m_backend == BACKEND_LPAQ8)) {
        ExternalRecord record;
        record.sName = singleRecordName(guardedSource.data());
        record.sProvider = backendName(m_backend);
        record.nCompressedSize = nInputSize;
        bool bRecordValid = true;
        QString sRecordError;
        if (m_backend == BACKEND_LPAQ8) {
            QFile headerFile(pContext->sInputPath);
            const bool bHeaderReady = headerFile.open(QIODevice::ReadOnly) && headerFile.seek(3);
            const QByteArray baHeader = bHeaderReady ? headerFile.read(5) : QByteArray();
            if (baHeader.size() != 5 || (baHeader.at(0) < '0') || (baHeader.at(0) > '9')) {
                bRecordValid = false;
                sRecordError = tr("Cannot read the LPAQ8 header");
            } else {
                record.nUncompressedSize = readBigEndian32(baHeader.mid(1, 4));
                const qint32 nLevel = baHeader.at(0) - '0';
                const qint64 nRequiredMemory = (6LL + 3LL * (1LL << nLevel)) * 1024LL * 1024LL;
                if ((outputPolicy.nMaxMemoryOutputSize >= 0) && (nRequiredMemory > outputPolicy.nMaxMemoryOutputSize)) {
                    bRecordValid = false;
                    sRecordError = tr("LPAQ8 memory requirement exceeds the configured limit");
                }
            }
        }

        if (bRecordValid) {
            const qint64 nKnownSize = (record.nUncompressedSize < 0) ? 0 : record.nUncompressedSize;
            bRecordValid = policyAllows(nKnownSize, nKnownSize, 1, outputPolicy);
            if (!bRecordValid) sRecordError = tr("Declared output exceeds safety limits");
        }
        bInitialized = bRecordValid;
        if (bInitialized) pContext->listRecords.append(record);
        else setExternalError(pPdStruct, sRecordError);
    } else if (m_backend == BACKEND_FREEARC) {
        QByteArray baListOutput;
        QStringList listArguments;
        listArguments << QStringLiteral("v") << QStringLiteral("-i0");
        if (pContext->pFreeArcPasswordFile) {
            listArguments << (QStringLiteral("-kf") + QDir::toNativeSeparators(pContext->pFreeArcPasswordFile->fileName()));
        }
        listArguments << QStringLiteral("--") << pContext->sInputPath;

        QList<ExternalRecord> listManifest;
        bInitialized = runTool(m_backend, pContext->sHelperPath, listArguments, sWorkDir, QString(), outputPolicy, false, pPdStruct, &baListOutput,
                               &m_lastExternalFailure, m_helperDeadline);
        if (bInitialized) {
            const EXTERNAL_PARSE_RESULT parseResult = parseFreeArcList(baListOutput, outputPolicy, &listManifest);
            if (parseResult != EXTERNAL_PARSE_OK) {
                m_lastExternalFailure = (parseResult == EXTERNAL_PARSE_RESOURCE_LIMIT) ? EXTERNAL_FAILURE_RESOURCE_LIMIT : EXTERNAL_FAILURE_ARCHIVE_REJECTED;
                bInitialized = setExternalError(
                    pPdStruct, (parseResult == EXTERNAL_PARSE_RESOURCE_LIMIT) ? tr("FreeArc directory exceeds safety limits") : tr("Cannot read FreeArc directory"));
            }
        }
        // Header-encrypted archives require the password above because Arc
        // cannot expose their manifest. Data-only encryption deliberately
        // retains ordinary archive-browser semantics: listing does not decode
        // or authenticate file bodies; first extraction does.
        if (bInitialized) pContext->listRecords = listManifest;
    } else if (m_backend == BACKEND_PEA) {
        const QString sOutputRoot = QDir(sWorkDir).filePath(QStringLiteral("expanded"));
        QStringList listArguments;
        listArguments << QStringLiteral("UNPEA") << pContext->sInputPath << QDir::toNativeSeparators(sOutputRoot) << QStringLiteral("RESETDATE")
                      << QStringLiteral("RESETATTR") << QStringLiteral("EXTRACT2DIR") << QStringLiteral("HIDDEN_REPORT") << pContext->sPassword
                      << QStringLiteral("NOKEYFILE");

        QByteArray baHelperOutput;
        bInitialized = runTool(m_backend, pContext->sHelperPath, listArguments, sWorkDir, sOutputRoot, outputPolicy, true, pPdStruct, &baHelperOutput,
                               &m_lastExternalFailure, m_helperDeadline);
        if (bInitialized && !verifyPeaReport(sWorkDir)) {
            m_lastExternalFailure = EXTERNAL_FAILURE_ARCHIVE_REJECTED;
            bInitialized = setExternalError(pPdStruct, tr("PEA helper did not verify the archive stream"));
        }
        StageScanResult scanResult;
        if (bInitialized) {
            bool bScanTimedOut = false;
            bInitialized = scanStageTree(sOutputRoot, outputPolicy, backendName(m_backend), true, &scanResult, pPdStruct, &m_helperDeadline, &bScanTimedOut);
            if (!bInitialized) {
                const bool bScanCanceled = !XBinary::isPdStructNotCanceled(pPdStruct);
                m_lastExternalFailure = bScanCanceled ? EXTERNAL_FAILURE_CANCELED : (bScanTimedOut ? EXTERNAL_FAILURE_TIMEOUT : EXTERNAL_FAILURE_RESOURCE_LIMIT);
                if (bScanTimedOut && !bScanCanceled) setExternalError(pPdStruct, tr("External archive helper operation timed out"));
            }
        }
        if (bInitialized) pContext->listRecords = scanResult.listRecords;
        else if (XBinary::isPdStructNotCanceled(pPdStruct) && XBinary::getPdStructErrorString(pPdStruct).isEmpty())
            setExternalError(pPdStruct, tr("Cannot expand %1 archive directory").arg(backendName(m_backend)));
        if (bInitialized && !pContext->sPassword.isEmpty()) {
            // PEA performs its only helper invocation during init. It has no
            // secure password transport, so do not retain a second plaintext
            // copy after that process and its report verification complete.
            pContext->sPassword.fill(QChar::Null);
            pContext->sPassword.clear();
        }
    }

    if (bInitialized && ((m_backend == BACKEND_ZPAQ) || (m_backend == BACKEND_FREEARC))) {
        bool bHasFileRecord = false;
        for (const ExternalRecord &record : qAsConst(pContext->listRecords)) {
            if (!record.bIsFolder) {
                bHasFileRecord = true;
                break;
            }
        }
        // Empty and directory-only manifests have no later file-body call
        // capable of authenticating a provisional SFX candidate. Verify them
        // now, while init can still reject it and try a later candidate.
        if (!bHasFileRecord) {
            bInitialized = _materializeDeferredArchive(pContext.get(), pState, pPdStruct);
        }
    }

    if (!bInitialized || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || !isUnpackSourceCurrent(pState, pPdStruct)) {
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    m_lastExternalFailure = EXTERNAL_FAILURE_NONE;

    pState->mapUnpackProperties = mapProperties;
    // External helpers keep the credential in their private context only for
    // as long as another helper invocation can still be required. The generic
    // state needs output limits and policy flags, not a duplicate password.
    pState->mapUnpackProperties.remove(UNPACK_PROP_PASSWORD);
    pState->mapUnpackProperties.remove(UNPACK_PROP_PASSWORD_BYTES);
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.size();
    pState->pContext = pContext.release();
    if (!validateAndFinalizeUnpackSource(pState, static_cast<EXTERNAL_UNPACK_CONTEXT *>(pState->pContext), pPdStruct)) {
        if (!guardedThis) return false;
        EXTERNAL_UNPACK_CONTEXT *pFailedContext = static_cast<EXTERNAL_UNPACK_CONTEXT *>(pState->pContext);
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pFailedContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XExternalArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XExternalArchive> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;

    EXTERNAL_UNPACK_CONTEXT *pContext = static_cast<EXTERNAL_UNPACK_CONTEXT *>(pState->pContext);
    if (pState->nCurrentIndex >= pContext->listRecords.size()) return result;
    const ExternalRecord &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = pState->nCurrentIndex;
    result.nStreamSize = qMax<qint64>(record.nUncompressedSize, 0);
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sName);
    if (record.nCompressedSize >= 0) result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nCompressedSize);
    if (record.nUncompressedSize >= 0) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
    if (record.bIsFolder) result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    if (record.dateTime.isValid()) result.mapProperties.insert(FPART_PROP_MTIME, record.dateTime);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("%1 (PeaZip external helper)").arg(record.sProvider.isEmpty() ? backendName(m_backend) : record.sProvider));
    if (!XBinary::markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XExternalArchive::_materializeDeferredArchive(EXTERNAL_UNPACK_CONTEXT *pContext, UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    m_lastExternalFailure = EXTERNAL_FAILURE_INFRASTRUCTURE;
    if (!pContext || !pState || ((m_backend != BACKEND_ZPAQ) && (m_backend != BACKEND_FREEARC)) || (pState->pContext && (pState->pContext != pContext))) return false;
    if (pContext->pArchiveStageDir) {
        m_lastExternalFailure = EXTERNAL_FAILURE_NONE;
        return true;
    }

    QPointer<XExternalArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) m_lastExternalFailure = EXTERNAL_FAILURE_CANCELED;
        return false;
    }

    std::unique_ptr<QTemporaryDir> pArchiveStage(new (std::nothrow) QTemporaryDir(QDir(pContext->pTemporaryDir->path()).filePath(QStringLiteral("expanded-XXXXXX"))));
    if (!pArchiveStage || !pArchiveStage->isValid()) {
        return setExternalError(pPdStruct, tr("Cannot create archive extraction stage"));
    }

    const QString sArchiveStageRoot = pArchiveStage->path();
    const QString sWorkDir = pContext->pTemporaryDir->path();
    QStringList listArguments;
    QByteArray baHelperOutput;
    QProcessEnvironment passwordEnvironment;
    const QProcessEnvironment *pPasswordEnvironment = nullptr;

    if (m_backend == BACKEND_ZPAQ) {
        listArguments << QStringLiteral("x") << pContext->sInputPath << QStringLiteral("-to") << withTrailingSeparator(sArchiveStageRoot) << QStringLiteral("-find")
                      << QStringLiteral("./") << QStringLiteral("-replace") << QString();
        if (!pContext->sPassword.isEmpty()) {
            if (pContext->bZpaqEnvironmentPassword) {
                passwordEnvironment.insert(QStringLiteral("FRANZKEY"), pContext->sPassword);
                pPasswordEnvironment = &passwordEnvironment;
            } else {
                listArguments << QStringLiteral("-key") << pContext->sPassword;
            }
        }
        // Keep listing and extraction on the same stable 7.15 command
        // grammar. No archive-provided member name is passed to the helper.
        listArguments << QStringLiteral("-715");
    } else {
        // Prompt mode plus a closed stdin makes any path collision an
        // authoritative helper failure instead of silently overwriting a
        // previously extracted member.
        listArguments << QStringLiteral("x") << QStringLiteral("-op") << QStringLiteral("-i0");
        if (pContext->pFreeArcPasswordFile) {
            listArguments << (QStringLiteral("-kf") + QDir::toNativeSeparators(pContext->pFreeArcPasswordFile->fileName()));
        }
        listArguments << (QStringLiteral("-dp") + QDir::toNativeSeparators(sArchiveStageRoot)) << QStringLiteral("--") << pContext->sInputPath;
    }

    if (!runTool(m_backend, pContext->sHelperPath, listArguments, sWorkDir, sArchiveStageRoot, pContext->outputPolicy, false, pPdStruct, &baHelperOutput,
                 &m_lastExternalFailure, m_helperDeadline, pPasswordEnvironment))
        return false;
    if ((m_backend == BACKEND_FREEARC) && !baHelperOutput.contains("All OK")) {
        m_lastExternalFailure = EXTERNAL_FAILURE_ARCHIVE_REJECTED;
        return setExternalError(pPdStruct, tr("FreeArc helper did not verify the archive"));
    }

    StageScanResult scanResult;
    bool bScanTimedOut = false;
    if (!scanStageTree(sArchiveStageRoot, pContext->outputPolicy, backendName(m_backend), true, &scanResult, pPdStruct, &m_helperDeadline, &bScanTimedOut)) {
        const bool bScanCanceled = !XBinary::isPdStructNotCanceled(pPdStruct);
        m_lastExternalFailure = bScanCanceled ? EXTERNAL_FAILURE_CANCELED : (bScanTimedOut ? EXTERNAL_FAILURE_TIMEOUT : EXTERNAL_FAILURE_RESOURCE_LIMIT);
        if (bScanCanceled) return false;
        return setExternalError(pPdStruct, bScanTimedOut ? tr("External archive helper operation timed out") : tr("Extracted archive directory failed safety checks"));
    }
    bool bReconcileTimedOut = false;
    if (!reconcileListedStage(pContext->listRecords, &scanResult.listRecords, pPdStruct, &m_helperDeadline, &bReconcileTimedOut)) {
        m_lastExternalFailure =
            bReconcileTimedOut ? EXTERNAL_FAILURE_TIMEOUT : (XBinary::isPdStructNotCanceled(pPdStruct) ? EXTERNAL_FAILURE_ARCHIVE_REJECTED : EXTERNAL_FAILURE_CANCELED);
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        return setExternalError(pPdStruct,
                                bReconcileTimedOut ? tr("External archive helper operation timed out") : tr("Extracted archive directory does not match its listing"));
    }
    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || !isUnpackSourceCurrent(pState, pPdStruct)) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) m_lastExternalFailure = EXTERNAL_FAILURE_CANCELED;
        return false;
    }

    pContext->listRecords = scanResult.listRecords;
    pContext->pArchiveStageDir = std::move(pArchiveStage);
    // No later record requires another helper invocation or its secret.
    pContext->pFreeArcPasswordFile.reset();
    if (!pContext->sPassword.isEmpty()) {
        pContext->sPassword.fill(QChar::Null);
        pContext->sPassword.clear();
    }
    m_lastExternalFailure = EXTERNAL_FAILURE_NONE;
    return true;
}

bool XExternalArchive::verifyDeferredArchive(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct)) {
        m_lastExternalFailure = EXTERNAL_FAILURE_INFRASTRUCTURE;
        return false;
    }
    return _materializeDeferredArchive(static_cast<EXTERNAL_UNPACK_CONTEXT *>(pState->pContext), pState, pPdStruct);
}

bool XExternalArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XExternalArchive> guardedThis(this);
    m_lastExternalFailure = EXTERNAL_FAILURE_INFRASTRUCTURE;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !pDevice || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    pState->nCurrentOffset = 0;

    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedOutput || !guardedSource || !isUnpackOutputSupported(guardedOutput.data()) ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !isUnpackSourceCurrent(pState, pPdStruct))
        return false;

    EXTERNAL_UNPACK_CONTEXT *pContext = static_cast<EXTERNAL_UNPACK_CONTEXT *>(pState->pContext);
    if (pState->nCurrentIndex >= pContext->listRecords.size()) return false;
    ExternalRecord record = pContext->listRecords.at(pState->nCurrentIndex);

    if (((m_backend == BACKEND_ZPAQ) || (m_backend == BACKEND_FREEARC)) && !pContext->pArchiveStageDir) {
        if (!_materializeDeferredArchive(pContext, pState, pPdStruct) || !guardedThis || !guardedOutput || !guardedSource ||
            (pState->nCurrentIndex >= pContext->listRecords.size()))
            return false;
        record = pContext->listRecords.at(pState->nCurrentIndex);
    }

    if (record.bIsFolder) {
        QBuffer emptyStage;
        const bool bOpen = emptyStage.open(QIODevice::ReadWrite);
        const bool bResult = bOpen && guardedThis && guardedOutput && publishUnpackOutput(&emptyStage, guardedOutput.data(), pState, pPdStruct);
        if (bResult && guardedThis) pState->nCurrentOffset = 0;
        if (bResult && guardedThis) m_lastExternalFailure = EXTERNAL_FAILURE_NONE;
        return bResult && guardedThis;
    }

    QString sStagePath = record.sStagedPath;
    std::unique_ptr<QTemporaryDir> pRunDirectory;
    if (sStagePath.isEmpty()) {
        if ((m_backend == BACKEND_ZPAQ) || (m_backend == BACKEND_FREEARC)) {
            if (!_materializeDeferredArchive(pContext, pState, pPdStruct) || !guardedThis || !guardedOutput || !guardedSource) return false;
            if (pState->nCurrentIndex >= pContext->listRecords.size()) return false;
            record = pContext->listRecords.at(pState->nCurrentIndex);
            sStagePath = record.sStagedPath;
        } else {
            pRunDirectory.reset(new (std::nothrow) QTemporaryDir(QDir(QDir::tempPath()).filePath(QStringLiteral("xfileunpacker-member-XXXXXX"))));
            if (!pRunDirectory || !pRunDirectory->isValid()) return setExternalError(pPdStruct, tr("Cannot create member extraction stage"));
            const QString sRunRoot = pRunDirectory->path();
            QStringList listArguments;

            if (m_backend == BACKEND_BCM) {
                sStagePath = QDir(sRunRoot).filePath(QStringLiteral("output"));
                listArguments << QStringLiteral("-d") << QStringLiteral("-f") << pContext->sInputPath << sStagePath;
            } else if (m_backend == BACKEND_LPAQ8) {
                sStagePath = QDir(sRunRoot).filePath(QStringLiteral("output"));
                listArguments << QStringLiteral("d") << pContext->sInputPath << sStagePath;
            } else {
                return false;
            }

            if (!runTool(m_backend, pContext->sHelperPath, listArguments, sRunRoot, sRunRoot, pContext->outputPolicy, false, pPdStruct, nullptr, &m_lastExternalFailure,
                         m_helperDeadline))
                return false;

            StageScanResult scanResult;
            bool bScanTimedOut = false;
            if (!scanStageTree(sRunRoot, pContext->outputPolicy, backendName(m_backend), true, &scanResult, pPdStruct, &m_helperDeadline, &bScanTimedOut)) {
                const bool bScanCanceled = !XBinary::isPdStructNotCanceled(pPdStruct);
                m_lastExternalFailure = bScanCanceled ? EXTERNAL_FAILURE_CANCELED : (bScanTimedOut ? EXTERNAL_FAILURE_TIMEOUT : EXTERNAL_FAILURE_RESOURCE_LIMIT);
                if (bScanCanceled) return false;
                return setExternalError(pPdStruct, bScanTimedOut ? tr("External archive helper operation timed out") : tr("Extracted member failed safety checks"));
            }
            QList<ExternalRecord> listFiles;
            for (const ExternalRecord &candidate : scanResult.listRecords) {
                if (!candidate.bIsFolder) listFiles.append(candidate);
            }
            if (listFiles.size() != 1) {
                m_lastExternalFailure = EXTERNAL_FAILURE_ARCHIVE_REJECTED;
                return setExternalError(pPdStruct, tr("External helper returned an unexpected member set"));
            }
            sStagePath = listFiles.first().sStagedPath;
        }
    }

    const QFileInfo stageInfo(sStagePath);
    if (!stageInfo.isFile() || stageInfo.isSymLink() || (stageInfo.size() < 0) || ((record.nUncompressedSize >= 0) && (stageInfo.size() != record.nUncompressedSize)) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, stageInfo.size())) {
        return setExternalError(pPdStruct, tr("Extracted member size is invalid"));
    }

    const QString sExpectedRoot =
        pRunDirectory ? pRunDirectory->path() : (pContext->pArchiveStageDir ? pContext->pArchiveStageDir->path() : QFileInfo(pContext->sInputPath).absolutePath());
    const QString sCanonicalRoot = QFileInfo(sExpectedRoot).canonicalFilePath();
    const QString sCanonicalStage = stageInfo.canonicalFilePath();
    if (!isContainedPath(sCanonicalRoot, sCanonicalStage)) return setExternalError(pPdStruct, tr("Extracted member escaped its private stage"));

    QFile stageFile(sCanonicalStage);
    if (!stageFile.open(QIODevice::ReadOnly) || stageFile.isSequential() || (stageFile.size() != stageInfo.size()) || !stageFile.seek(0) || !guardedThis ||
        !guardedOutput || !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct))
        return false;

    const qint64 nSize = stageFile.size();
    // The external-helper route bypasses the base decode chain: the staged
    // bytes were produced by the sandboxed helper, so charge the operation
    // budget here - one entry, nSize produced bytes - before publication.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, record.sName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }
    const bool bResult = publishUnpackOutput(&stageFile, guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedThis) {
        pState->nCurrentOffset = nSize;
        m_lastExternalFailure = EXTERNAL_FAILURE_NONE;
    }
    return bResult && guardedThis;
}

bool XExternalArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XExternalArchive> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XExternalArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    EXTERNAL_UNPACK_CONTEXT *pContext = static_cast<EXTERNAL_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XExternalArchive::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD,
            FPART_PROP_ISFOLDER,     FPART_PROP_MTIME};
}
