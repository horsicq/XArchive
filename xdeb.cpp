/* Copyright (c) 2022-2026 hors<horsicq@gmail.com>
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
#include "xdeb.h"

XBinary::XCONVERT _TABLE_XDEB_STRUCTID[] = {
    {XDEB::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
};

namespace {
static const qint32 N_DEB_MAX_MEMBERS = 0x10000;

// Same guard the package readers use: getRecords() leaves the cursor wherever
// its last read ended, and detection probes a device the caller still owns.
class DevicePositionGuard {
public:
    explicit DevicePositionGuard(QIODevice *pDevice) : m_pDevice(pDevice), m_nPosition(-1)
    {
        if (m_pDevice && !m_pDevice->isSequential()) {
            m_nPosition = m_pDevice->pos();
        }
    }

    ~DevicePositionGuard()
    {
        if (m_pDevice && (m_nPosition >= 0) && m_pDevice->isOpen()) {
            m_pDevice->seek(m_nPosition);
        }
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
};

bool isDebTarMember(const QString &sName, const QString &sPrefix)
{
    if (sName == (sPrefix + QStringLiteral(".tar"))) {
        return true;
    }

    static const char *const pSuffixes[] = {".tar.gz", ".tar.xz", ".tar.zst", ".tar.bz2", ".tar.lzma"};
    for (qint32 i = 0; i < (qint32)(sizeof(pSuffixes) / sizeof(pSuffixes[0])); i++) {
        if (sName == (sPrefix + QLatin1String(pSuffixes[i]))) {
            return true;
        }
    }

    return false;
}
}  // namespace

XDEB::XDEB(QIODevice *pDevice) : X_Ar(pDevice)
{
}

bool XDEB::isValid(PDSTRUCT *pPdStruct)
{
    DevicePositionGuard positionGuard(getDevice());

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    X_Ar xar(getDevice());
    if (!xar.isValid(pPdStruct)) {
        return false;
    }

    QList<XArchive::RECORD> listArchiveRecords = xar.getRecords(N_DEB_MAX_MEMBERS + 1, pPdStruct);
    if (!isValid(&listArchiveRecords, pPdStruct)) {
        return false;
    }

    const QByteArray baVersion = xar.decompress(&listArchiveRecords[0], pPdStruct);
    return XBinary::isPdStructNotCanceled(pPdStruct) && (baVersion == QByteArrayLiteral("2.0\n"));
}

bool XDEB::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDEB xdeb(pDevice);

    return xdeb.isValid(pPdStruct);
}

bool XDEB::isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    if (!pListRecords || (pListRecords->count() < 3) || (pListRecords->count() > N_DEB_MAX_MEMBERS) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const RECORD &versionRecord = pListRecords->at(0);
    if ((versionRecord.spInfo.sRecordName != QStringLiteral("debian-binary")) ||
        (versionRecord.spInfo.nUncompressedSize != 4)) {
        return false;
    }

    bool bControlSeen = false;
    bool bDataSeen = false;
    for (qint32 i = 1; i < pListRecords->count(); i++) {
        const QString &sName = pListRecords->at(i).spInfo.sRecordName;
        if (bDataSeen) {
            // The format explicitly reserves members after data.tar for
            // future extensions; current readers must ignore them.
            continue;
        }
        if (sName.startsWith(QLatin1Char('_'))) {
            // Ignorable extension members may appear between the required
            // members when their names begin with an underscore.
            continue;
        }
        if (!bControlSeen) {
            if (!isDebTarMember(sName, QStringLiteral("control"))) return false;
            bControlSeen = true;
        } else {
            if (!isDebTarMember(sName, QStringLiteral("data"))) return false;
            bDataSeen = true;
        }
    }

    return bControlSeen && bDataSeen;
}

QString XDEB::getVersion()
{
    return getFileFormatInfo(nullptr).sVersion;
}

QString XDEB::getFileFormatExt()
{
    return "deb";
}

QString XDEB::getMIMEString()
{
    return "application/vnd.debian.binary-package";
}

XBinary::FILEFORMATINFO XDEB::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    DevicePositionGuard positionGuard(getDevice());

    XBinary::FILEFORMATINFO result = {};

    QList<XArchive::RECORD> listArchiveRecords = getRecords(N_DEB_MAX_MEMBERS + 1, pPdStruct);

    if (isValid(&listArchiveRecords, pPdStruct)) {
        RECORD record = getArchiveRecord("debian-binary", &listArchiveRecords);
        QByteArray baVersion = decompress(&record, pPdStruct);

        if ((baVersion == QByteArrayLiteral("2.0\n")) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            result.bIsValid = true;
            result.nSize = getSize();
            result.sExt = "deb";
            result.fileType = FT_DEB;
            result.sVersion = QStringLiteral("2.0");

            result.osName = OSNAME_DEBIANLINUX;

            result.sArch = getArch();
            result.mode = getMode();
            result.sType = typeIdToString(getType());
            result.endian = getEndian();
        }
    }

    return result;
}

XBinary::FT XDEB::getFileType()
{
    return FT_DEB;
}

QString XDEB::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XDEB_STRUCTID, sizeof(_TABLE_XDEB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XDEB::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XDEB_STRUCTID, sizeof(_TABLE_XDEB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XDEB::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XDEB_STRUCTID, sizeof(_TABLE_XDEB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

bool XDEB::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XDEB> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->X_Ar::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        X_Ar::INTERNAL_INFO *pInfo =
            static_cast<X_Ar::INTERNAL_INFO *>(
                guardedThis->X_Ar::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<X_Ar::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XDEB::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XDEB> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XDEB::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        X_Ar::setInternalInfo(static_cast<X_Ar::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        X_Ar::setInternalInfo(nullptr);
    }
}
