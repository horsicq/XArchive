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
#include "xgamestorearchive_p.h"

#include <QtEndian>

#include <new>

namespace {
class GameDevicePositionGuard {
public:
    explicit GameDevicePositionGuard(QIODevice *pDevice)
        : m_pDevice(pDevice), m_nPosition(-1), m_bRestored(false)
    {
        if (!m_pDevice) return;
        const bool bSequential = m_pDevice->isSequential();
        if (!m_pDevice || bSequential) return;
        m_nPosition = m_pDevice->pos();
    }

    ~GameDevicePositionGuard()
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
        const bool bSeeked = m_pDevice->seek(m_nPosition);
        if (!m_pDevice || !bSeeked) return false;
        const qint64 nPosition = m_pDevice->pos();
        return m_pDevice && (nPosition == m_nPosition);
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

QString appendDuplicateSuffix(const QString &sComponent, qint32 nSuffix)
{
    const qint32 nDot = sComponent.lastIndexOf(QLatin1Char('.'));
    const QString sSuffix = QStringLiteral("_%1").arg(nSuffix);
    if (nDot > 0) {
        return sComponent.left(nDot) + sSuffix + sComponent.mid(nDot);
    }
    return sComponent + sSuffix;
}
}  // namespace

XGameStoreArchiveBase::XGameStoreArchiveBase(QIODevice *pDevice, FT fileType)
    : XArchive(pDevice), m_fileTypeHint(fileType)
{
    setFileType(fileType);
}

quint32 XGameStoreArchiveBase::readLE32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

bool XGameStoreArchiveBase::rangeWithin(qint64 nTotalSize, qint64 nOffset,
                                        qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool XGameStoreArchiveBase::rangesOverlap(qint64 nOffset1, qint64 nSize1,
                                          qint64 nOffset2, qint64 nSize2)
{
    if ((nSize1 <= 0) || (nSize2 <= 0)) return false;
    return (nOffset1 < (nOffset2 + nSize2)) &&
           (nOffset2 < (nOffset1 + nSize1));
}

bool XGameStoreArchiveBase::decodeName(const uchar *pData,
                                       qint32 nFieldSize, bool bAsciiOnly,
                                       QString *pName)
{
    if (!pData || !pName || (nFieldSize <= 0)) return false;

    qint32 nLength = 0;
    while ((nLength < nFieldSize) && pData[nLength]) nLength++;
    if (nLength == 0) return false;

    for (qint32 i = 0; i < nLength; ++i) {
        const quint8 nCharacter = pData[i];
        if ((nCharacter < 0x20) ||
            ((nCharacter >= 0x7f) && (nCharacter <= 0x9f)) ||
            (bAsciiOnly && (nCharacter > 0x7e))) {
            return false;
        }
    }

    QString sName = QString::fromLatin1(
        reinterpret_cast<const char *>(pData), nLength);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/'))) return false;

    const QStringList listParts =
        sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) ||
            (sPart == QLatin1String(".."))) {
            return false;
        }
    }

    sName = sName.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sName) != sName) return false;

    *pName = sName;
    return true;
}

bool XGameStoreArchiveBase::makeUniquePath(
    const QString &sSource, QSet<QString> *pUsedFiles,
    QSet<QString> *pUsedDirectories,
    QHash<QString, qint32> *pNextSuffixes,
    QHash<QString, QString> *pResolvedDirectories, QString *pResult)
{
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
        QString sComponent = sOriginalComponent;
        QString sPath;
        QString sKey;
        bool bFound = false;
        const QString sBasePath = listResolved.isEmpty()
            ? sOriginalComponent
            : (listResolved.join(QLatin1Char('/')) + QLatin1Char('/') +
               sOriginalComponent);
        const QString sBaseKey = sBasePath.toCaseFolded();
        qint32 nSuffix = 1;

        if (!bLeaf && pResolvedDirectories->contains(sBaseKey)) {
            sPath = pResolvedDirectories->value(sBaseKey);
            sKey = sPath.toCaseFolded();
            const qint32 nSeparator = sPath.lastIndexOf(QLatin1Char('/'));
            sComponent = sPath.mid(nSeparator + 1);
            if (sComponent.isEmpty() || pUsedFiles->contains(sKey) ||
                !pUsedDirectories->contains(sKey) ||
                (XBinary::fixFileName(sPath) != sPath)) {
                return false;
            }
            listResolved.append(sComponent);
            continue;
        }

        while (nSuffix <= (MAX_RECORDS + 1)) {
            sComponent = (nSuffix == 1)
                ? sOriginalComponent
                : appendDuplicateSuffix(sOriginalComponent, nSuffix);
            sPath = listResolved.isEmpty()
                ? sComponent
                : (listResolved.join(QLatin1Char('/')) +
                   QLatin1Char('/') + sComponent);
            sKey = sPath.toCaseFolded();
            const bool bCollision = pUsedFiles->contains(sKey) ||
                (bLeaf && pUsedDirectories->contains(sKey));
            if (!bCollision) {
                bFound = true;
                break;
            }
            nSuffix = (nSuffix == 1)
                ? pNextSuffixes->value(sBaseKey, 2)
                : (nSuffix + 1);
        }

        if (!bFound || (XBinary::fixFileName(sPath) != sPath)) return false;
        const qint32 nNextSuffix = (nSuffix <= MAX_RECORDS)
            ? qMax(2, nSuffix + 1) : (MAX_RECORDS + 1);
        pNextSuffixes->insert(sBaseKey, nNextSuffix);
        listResolved.append(sComponent);
        if (bLeaf) {
            pUsedFiles->insert(sKey);
        } else {
            pUsedDirectories->insert(sKey);
            pResolvedDirectories->insert(sBaseKey, sPath);
        }
    }

    *pResult = listResolved.join(QLatin1Char('/'));
    return !pResult->isEmpty();
}

bool XGameStoreArchiveBase::isValid(PDSTRUCT *pPdStruct)
{
    return scanArchive(nullptr, nullptr, pPdStruct);
}

bool XGameStoreArchiveBase::scanArchive(QList<ENTRY> *pEntries,
                                        qint64 *pArchiveEnd,
                                        PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pArchiveEnd) *pArchiveEnd = 0;

    QPointer<XGameStoreArchiveBase> guardedThis(this);
    const FT fileType = getFileType();
    if (((fileType != FT_ZIP) &&
         (fileType != FT_QUAKE_PAK) && (fileType != FT_DOOM_WAD) &&
         (fileType != FT_BUILD_GRP) && (fileType != FT_DESCENT_HOG) &&
         (fileType != FT_C64_T64) && (fileType != FT_APPLESINGLE) &&
         (fileType != FT_APPLE_2IMG) &&
         (fileType != FT_WINTERMUTE_DCP) &&
         (fileType != FT_PYINSTALLER_PYZ) &&
         (fileType != FT_AMIGA_LZX) &&
         (fileType != FT_WOLF_VSWAP) &&
         (fileType != FT_COMPACT_PRO) &&
         (fileType != FT_DISK_DOUBLER) &&
         (fileType != FT_DISK_DOUBLER_DDA2) &&
         (fileType != FT_PYINSTALLER_SFX) &&
         (fileType != FT_LEGACY_CAT) &&
         (fileType != FT_KA_ARCHIVE) &&
         (fileType != FT_MLB_ARCHIVE) &&
         (fileType != FT_LEGACY_RES) &&
         (fileType != FT_LEGACY_RSC) &&
         (fileType != FT_SHRINKWRAP_IMAGE) &&
         (fileType != FT_LPAK) &&
         (fileType != FT_DISKJUGGLER_CDI) &&
         (fileType != FT_INSTALLSHIELD_BOOT) &&
         (fileType != FT_SABDU_IMAGE) &&
         (fileType != FT_COMPAQ_LZH) &&
         (fileType != FT_INSA) &&
         (fileType != FT_WISE_SFX) &&
         (fileType != FT_INSTALLSHIELD3_SFX) &&
         (fileType != FT_IS14_SFX) &&
         (fileType != FT_GPINSTALL_SFX) &&
         (fileType != FT_INSTALLSHIELD_LAUNCHER) &&
         (fileType != FT_EPFS_ARCHIVE) &&
         (fileType != FT_STUNTS_DSI) &&
         (fileType != FT_FINSTALL_ARCHIVE) &&
         (fileType != FT_IS_STORED) &&
         (fileType != FT_INSTALLSHIELD3_ARCHIVE) &&
         (fileType != FT_EMT_IMAGE) &&
         (fileType != FT_GPFPACK) &&
         (fileType != FT_PAX) &&
         (fileType != FT_SCF) &&
         (fileType != FT_SOLITAIRE_DELUXE) &&
         (fileType != FT_INSTALIT_DATA) &&
         (fileType != FT_ARCV) &&
         (fileType != FT_PIMP_SFX) &&
         (fileType != FT_VISE_SFX) &&
         (fileType != FT_FTCOMP) &&
         (fileType != FT_FLS) &&
         (fileType != FT_RTPATCH) &&
         (fileType != FT_RNC) &&
         (fileType != FT_MI10) &&
         (fileType != FT_DN_ARCHIVE) &&
         (fileType != FT_FPAK) &&
         (fileType != FT_SOFTPAQ1_SFX) &&
         (fileType != FT_INSTALIT_SFX) &&
         (fileType != FT_LIF_COMPRESSED) &&
         (fileType != FT_JASC_ARCHIVE) &&
         (fileType != FT_SSM_MODULE) &&
         (fileType != FT_SSBOB) &&
         (fileType != FT_IS_SKIN) &&
         (fileType != FT_MACBINARY) && (fileType != FT_RESOURCE_FORK) &&
         (fileType != FT_CPM_LBR) &&
         (fileType != FT_PARSEC_ARCHIVE) && (fileType != FT_PMM)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    GameDevicePositionGuard positionGuard(getDevice());
    if (!guardedThis || !positionGuard.isValid()) return false;

    QList<ENTRY> listEntries;
    qint64 nArchiveEnd = 0;
    const bool bResult = scanFormat(
        pEntries ? &listEntries : nullptr, &nArchiveEnd, pPdStruct);

    const bool bRestored = positionGuard.restore();
    if (!guardedThis || !bRestored || !bResult ||
        (getFileType() != fileType) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}

XBinary::FT XGameStoreArchiveBase::getFileType()
{
    return m_fileTypeHint;
}

XBinary::MODE XGameStoreArchiveBase::getMode()
{
    return MODE_DATA;
}

qint32 XGameStoreArchiveBase::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XGameStoreArchiveBase::getEndian()
{
    const FT fileType = getFileType();
    if ((fileType == FT_APPLESINGLE) || (fileType == FT_MACBINARY) ||
        (fileType == FT_RESOURCE_FORK) || (fileType == FT_COMPACT_PRO) ||
        (fileType == FT_DISK_DOUBLER) ||
        (fileType == FT_DISK_DOUBLER_DDA2) ||
        (fileType == FT_SHRINKWRAP_IMAGE) ||
        (fileType == FT_LPAK) || (fileType == FT_PAX) ||
        (fileType == FT_RNC) || (fileType == FT_MI10)) return ENDIAN_BIG;
    return ENDIAN_LITTLE;
}

QString XGameStoreArchiveBase::getArch()
{
    return QString();
}

QString XGameStoreArchiveBase::getFileFormatExt()
{
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK) return QStringLiteral("pak");
    if (fileType == FT_DOOM_WAD) return QStringLiteral("wad");
    if (fileType == FT_BUILD_GRP) return QStringLiteral("grp");
    if (fileType == FT_DESCENT_HOG) return QStringLiteral("hog");
    if (fileType == FT_C64_T64) return QStringLiteral("t64");
    if (fileType == FT_APPLESINGLE) return QStringLiteral("as");
    if (fileType == FT_APPLE_2IMG) return QStringLiteral("2mg");
    if (fileType == FT_WINTERMUTE_DCP) return QStringLiteral("dcp");
    if (fileType == FT_PYINSTALLER_PYZ) return QStringLiteral("pyz");
    if (fileType == FT_AMIGA_LZX) return QStringLiteral("lzx");
    if (fileType == FT_WOLF_VSWAP) return QStringLiteral("vswap");
    if (fileType == FT_COMPACT_PRO) return QStringLiteral("cpt");
    if (fileType == FT_DISK_DOUBLER) return QStringLiteral("dd");
    if (fileType == FT_DISK_DOUBLER_DDA2) return QStringLiteral("dda2");
    if (fileType == FT_PYINSTALLER_SFX) return QStringLiteral("exe");
    if (fileType == FT_LEGACY_CAT) return QStringLiteral("cat");
    if (fileType == FT_KA_ARCHIVE) return QStringLiteral("arc");
    if (fileType == FT_MLB_ARCHIVE) return QStringLiteral("mlb");
    if (fileType == FT_LEGACY_RES) return QStringLiteral("res");
    if (fileType == FT_LEGACY_RSC) return QStringLiteral("rsc");
    if (fileType == FT_SHRINKWRAP_IMAGE) return QStringLiteral("image");
    if (fileType == FT_LPAK) return QStringLiteral("cmp");
    if (fileType == FT_DISKJUGGLER_CDI) return QStringLiteral("cdi");
    if (fileType == FT_INSTALLSHIELD_BOOT) return QStringLiteral("boot");
    if (fileType == FT_SABDU_IMAGE) return QStringLiteral("sdu");
    if (fileType == FT_COMPAQ_LZH) return QStringLiteral("lzh");
    if (fileType == FT_INSA) return QStringLiteral("dat");
    if (fileType == FT_WISE_SFX) return QStringLiteral("exe");
    if (fileType == FT_INSTALLSHIELD3_SFX) return QStringLiteral("exe");
    if (fileType == FT_IS14_SFX) return QStringLiteral("exe");
    if (fileType == FT_GPINSTALL_SFX) return QStringLiteral("exe");
    if (fileType == FT_INSTALLSHIELD_LAUNCHER) return QStringLiteral("exe");
    if (fileType == FT_EPFS_ARCHIVE) return QStringLiteral("epf");
    if (fileType == FT_STUNTS_DSI) return QStringLiteral("pes");
    if (fileType == FT_FINSTALL_ARCHIVE) return QStringLiteral("disk");
    if (fileType == FT_IS_STORED) return QStringLiteral("bin");
    if (fileType == FT_INSTALLSHIELD3_ARCHIVE) return QStringLiteral("z");
    if (fileType == FT_EMT_IMAGE) return QStringLiteral("emt");
    if (fileType == FT_GPFPACK) return QStringLiteral("gpf");
    if (fileType == FT_PAX) return QStringLiteral("pax");
    if (fileType == FT_SCF) return QStringLiteral("scf");
    if (fileType == FT_SOLITAIRE_DELUXE) return QStringLiteral("1");
    if (fileType == FT_INSTALIT_DATA) return QStringLiteral("001");
    if (fileType == FT_ARCV) return QStringLiteral("arv");
    if (fileType == FT_PIMP_SFX) return QStringLiteral("exe");
    if (fileType == FT_VISE_SFX) return QStringLiteral("exe");
    if (fileType == FT_FTCOMP) return QStringLiteral("_");
    if (fileType == FT_FLS) return QStringLiteral("fls");
    if (fileType == FT_RTPATCH) return QStringLiteral("rtp");
    if (fileType == FT_RNC) return QStringLiteral("rnc");
    if (fileType == FT_MI10) return QStringLiteral("mi");
    if (fileType == FT_DN_ARCHIVE) return QStringLiteral("138");
    if (fileType == FT_FPAK) return QStringLiteral("pak");
    if (fileType == FT_SOFTPAQ1_SFX) return QStringLiteral("exe");
    if (fileType == FT_INSTALIT_SFX) return QStringLiteral("exe");
    if (fileType == FT_LIF_COMPRESSED) return QStringLiteral("lif");
    if (fileType == FT_JASC_ARCHIVE) return QStringLiteral("cmp");
    if (fileType == FT_SSM_MODULE) return QStringLiteral("ssm");
    if (fileType == FT_SSBOB) return QStringLiteral("fss");
    if (fileType == FT_IS_SKIN) return QStringLiteral("skin");
    if (fileType == FT_MACBINARY) return QStringLiteral("bin");
    if (fileType == FT_RESOURCE_FORK) return QStringLiteral("rsrc");
    if (fileType == FT_CPM_LBR) return QStringLiteral("lbr");
    if (fileType == FT_PARSEC_ARCHIVE) return QStringLiteral("dat");
    if (fileType == FT_PMM) return QStringLiteral("pmm");
    return QString();
}

QString XGameStoreArchiveBase::getFileFormatExtsString()
{
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK)
        return QStringLiteral("Quake PAK (*.pak)");
    if (fileType == FT_DOOM_WAD)
        return QStringLiteral("Doom WAD (*.wad)");
    if (fileType == FT_BUILD_GRP)
        return QStringLiteral("Build GRP (*.grp)");
    if (fileType == FT_DESCENT_HOG)
        return QStringLiteral("Descent HOG (*.hog)");
    if (fileType == FT_C64_T64)
        return QStringLiteral("Commodore T64 tape image (*.t64)");
    if (fileType == FT_APPLESINGLE)
        return QStringLiteral("AppleSingle/AppleDouble (*.as;._*)");
    if (fileType == FT_APPLE_2IMG)
        return QStringLiteral("Apple II 2IMG disk image (*.2mg;*.2img)");
    if (fileType == FT_WINTERMUTE_DCP)
        return QStringLiteral("Wintermute Engine package (*.dcp)");
    if (fileType == FT_PYINSTALLER_PYZ)
        return QStringLiteral("PyInstaller PYZ archive (*.pyz)");
    if (fileType == FT_AMIGA_LZX)
        return QStringLiteral("Amiga LZX archive (*.lzx)");
    if (fileType == FT_WOLF_VSWAP)
        return QStringLiteral("Wolfenstein VSWAP resource archive (VSWAP.*)");
    if (fileType == FT_COMPACT_PRO)
        return QStringLiteral("Compact Pro archive (*.cpt;*.sea)");
    if (fileType == FT_DISK_DOUBLER)
        return QStringLiteral("DiskDoubler compressed file (*.dd)");
    if (fileType == FT_DISK_DOUBLER_DDA2)
        return QStringLiteral("DiskDoubler DDA2 archive (*.dda2;*.sea)");
    if (fileType == FT_PYINSTALLER_SFX)
        return QStringLiteral("PyInstaller executable archive (*.exe)");
    if (fileType == FT_LEGACY_CAT)
        return QStringLiteral("CAT resource archive (*.cat)");
    if (fileType == FT_KA_ARCHIVE)
        return QStringLiteral("KA archive (*.arc)");
    if (fileType == FT_MLB_ARCHIVE)
        return QStringLiteral("MLB resource archive (*.mlb)");
    if (fileType == FT_LEGACY_RES)
        return QStringLiteral("RES resource archive (*.res)");
    if (fileType == FT_LEGACY_RSC)
        return QStringLiteral("RSC resource archive (*.rsc;*.dat;*.gsl)");
    if (fileType == FT_SHRINKWRAP_IMAGE)
        return QStringLiteral("ShrinkWrap disk image");
    if (fileType == FT_LPAK)
        return QStringLiteral("Atari LPAK compressed stream (*.cmp)");
    if (fileType == FT_DISKJUGGLER_CDI)
        return QStringLiteral("DiscJuggler CDI disc image (*.cdi)");
    if (fileType == FT_INSTALLSHIELD_BOOT)
        return QStringLiteral("InstallShield 7 bootstrap archive (*.boot)");
    if (fileType == FT_SABDU_IMAGE)
        return QStringLiteral("SAB Diskette Utility image (*.sdu)");
    if (fileType == FT_COMPAQ_LZH)
        return QStringLiteral("Compaq LZH compressed file (*.*!;*.lzh)");
    if (fileType == FT_INSA)
        return QStringLiteral("INSA installer data archive (*.dat)");
    if (fileType == FT_WISE_SFX)
        return QStringLiteral("Wise installer executable archive (*.exe)");
    if (fileType == FT_INSTALLSHIELD3_SFX)
        return QStringLiteral("InstallShield 3 SFX (*.exe)");
    if (fileType == FT_IS14_SFX)
        return QStringLiteral("InstallShield Setup Player 2K2 SFX (*.exe)");
    if (fileType == FT_GPINSTALL_SFX)
        return QStringLiteral("GP-Install self-extracting installer (*.exe)");
    if (fileType == FT_INSTALLSHIELD_LAUNCHER)
        return QStringLiteral("InstallShield Setup Launcher self-extracting archive (*.exe)");
    if (fileType == FT_EPFS_ARCHIVE)
        return QStringLiteral("East Point Software EPFS archive (*.epf)");
    if (fileType == FT_STUNTS_DSI)
        return QStringLiteral("Stunts/4D Sports Driving compressed resource (*.pes;*.pvs;*.p3s;*.pre)");
    if (fileType == FT_FINSTALL_ARCHIVE)
        return QStringLiteral("F Install 2 archive (DISK*)");
    if (fileType == FT_IS_STORED)
        return QStringLiteral("IS stored/XOR data");
    if (fileType == FT_INSTALLSHIELD3_ARCHIVE)
        return QStringLiteral("InstallShield 3 archive (*.z;*.lib;*.1;*.2;*.3;*.4;*.5;*.6)");
    if (fileType == FT_EMT_IMAGE)
        return QStringLiteral("EMT compressed disk image (*.emt;*.img)");
    if (fileType == FT_GPFPACK)
        return QStringLiteral("GPFPACK compressed file (*.*#)");
    if (fileType == FT_PAX)
        return QStringLiteral("GEM-View PAX archive (*.pax)");
    if (fileType == FT_SCF)
        return QStringLiteral("Microsoft SCF compressed archive (*.scf)");
    if (fileType == FT_SOLITAIRE_DELUXE)
        return QStringLiteral("Solitaire Deluxe installation archive (DISK.1)");
    if (fileType == FT_INSTALIT_DATA)
        return QStringLiteral("Instalit concatenated DCL archive (*.001)");
    if (fileType == FT_ARCV)
        return QStringLiteral("Eschalon Setup ARCV archive (*.arv;*.bin;*$00;*$01)");
    if (fileType == FT_PIMP_SFX)
        return QStringLiteral("Nullsoft PiMP self-extracting archive (*.exe)");
    if (fileType == FT_VISE_SFX)
        return QStringLiteral("Windows Installer VISE self-extracting archive (*.exe)");
    if (fileType == FT_FTCOMP)
        return QStringLiteral("IBM OS/2 PACK2 archive (*._*;*.___)");
    if (fileType == FT_FLS)
        return QStringLiteral("IBM SaveRam/SaveRam2 FLS archive (*.fls;*._*)");
    if (fileType == FT_RTPATCH)
        return QStringLiteral("Pocket Soft RTPatch package (*.rtp;*.stp)");
    if (fileType == FT_RNC)
        return QStringLiteral("Rob Northen multi-file archive (*.rnc)");
    if (fileType == FT_MI10)
        return QStringLiteral("Amiga MI10 crunched block chain (*.mi)");
    if (fileType == FT_DN_ARCHIVE)
        return QStringLiteral("DOS Navigator installer archive (*.138)");
    if (fileType == FT_FPAK)
        return QStringLiteral("FoxPro Distribution Kit archive (*.pak;*.pa?)");
    if (fileType == FT_SOFTPAQ1_SFX)
        return QStringLiteral("Compaq SoftPaq version 1 archive (*.exe)");
    if (fileType == FT_INSTALIT_SFX)
        return QStringLiteral("Instalit self-extracting archive (*.exe)");
    if (fileType == FT_LIF_COMPRESSED)
        return QStringLiteral("LIF/DC LZW compressed file (*.lif;*.cmp;*.ser)");
    if (fileType == FT_JASC_ARCHIVE)
        return QStringLiteral("JASC installer archive (*.cmp;*.inf)");
    if (fileType == FT_SSM_MODULE)
        return QStringLiteral("PICTools SSM compressed module (*.ssm)");
    if (fileType == FT_SSBOB)
        return QStringLiteral("SSBOB slideshow package (*.fss)");
    if (fileType == FT_IS_SKIN)
        return QStringLiteral("InstallShield setup skin (skin, isn)");
    if (fileType == FT_MACBINARY)
        return QStringLiteral("MacBinary (*.bin;*.macbin;*.mac)");
    if (fileType == FT_RESOURCE_FORK)
        return QStringLiteral("Macintosh resource fork (*.rsrc;*.rez)");
    if (fileType == FT_CPM_LBR)
        return QStringLiteral("CP/M LBR library (*.lbr)");
    if (fileType == FT_PARSEC_ARCHIVE)
        return QStringLiteral("Parsec resource archive (*.dat)");
    if (fileType == FT_PMM)
        return QStringLiteral("Parsec PSM music module (*.pmm)");
    return QString();
}

QString XGameStoreArchiveBase::getMIMEString()
{
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK)
        return QStringLiteral("application/x-quake-pak");
    if (fileType == FT_DOOM_WAD)
        return QStringLiteral("application/x-doom-wad");
    if (fileType == FT_BUILD_GRP)
        return QStringLiteral("application/x-build-grp");
    if (fileType == FT_DESCENT_HOG)
        return QStringLiteral("application/x-descent-hog");
    if (fileType == FT_C64_T64)
        return QStringLiteral("application/x-c64-t64");
    if (fileType == FT_APPLESINGLE)
        return QStringLiteral("application/applefile");
    if (fileType == FT_APPLE_2IMG)
        return QStringLiteral("application/x-apple-2img");
    if (fileType == FT_WINTERMUTE_DCP)
        return QStringLiteral("application/x-wintermute-dcp");
    if (fileType == FT_PYINSTALLER_PYZ)
        return QStringLiteral("application/x-pyinstaller-pyz");
    if (fileType == FT_AMIGA_LZX)
        return QStringLiteral("application/x-amiga-lzx");
    if (fileType == FT_WOLF_VSWAP)
        return QStringLiteral("application/x-wolf-vswap");
    if (fileType == FT_COMPACT_PRO)
        return QStringLiteral("application/x-compact-pro");
    if ((fileType == FT_DISK_DOUBLER) ||
        (fileType == FT_DISK_DOUBLER_DDA2))
        return QStringLiteral("application/x-diskdoubler");
    if (fileType == FT_PYINSTALLER_SFX)
        return QStringLiteral("application/x-pyinstaller");
    if (fileType == FT_LEGACY_CAT)
        return QStringLiteral("application/x-cat-archive");
    if (fileType == FT_KA_ARCHIVE)
        return QStringLiteral("application/x-ka-archive");
    if (fileType == FT_MLB_ARCHIVE)
        return QStringLiteral("application/x-mlb-archive");
    if (fileType == FT_LEGACY_RES)
        return QStringLiteral("application/x-res-archive");
    if (fileType == FT_LEGACY_RSC)
        return QStringLiteral("application/x-rsc-archive");
    if (fileType == FT_SHRINKWRAP_IMAGE)
        return QStringLiteral("application/x-shrinkwrap-image");
    if (fileType == FT_LPAK)
        return QStringLiteral("application/x-lpak");
    if (fileType == FT_DISKJUGGLER_CDI)
        return QStringLiteral("application/x-discjuggler-cdi");
    if (fileType == FT_INSTALLSHIELD_BOOT)
        return QStringLiteral("application/x-installshield-boot");
    if (fileType == FT_SABDU_IMAGE)
        return QStringLiteral("application/x-sabdu-image");
    if (fileType == FT_COMPAQ_LZH)
        return QStringLiteral("application/x-compaq-lzh");
    if (fileType == FT_INSA)
        return QStringLiteral("application/x-insa");
    if (fileType == FT_WISE_SFX)
        return QStringLiteral("application/x-wise-installer");
    if (fileType == FT_INSTALLSHIELD3_SFX)
        return QStringLiteral("application/x-installshield3-sfx");
    if (fileType == FT_IS14_SFX)
        return QStringLiteral("application/x-installshield-setup-player-sfx");
    if (fileType == FT_GPINSTALL_SFX)
        return QStringLiteral("application/x-gpinstall-sfx");
    if (fileType == FT_INSTALLSHIELD_LAUNCHER)
        return QStringLiteral("application/x-installshield-launcher");
    if (fileType == FT_EPFS_ARCHIVE)
        return QStringLiteral("application/x-epfs-archive");
    if (fileType == FT_STUNTS_DSI)
        return QStringLiteral("application/x-stunts-resource");
    if (fileType == FT_FINSTALL_ARCHIVE)
        return QStringLiteral("application/x-finstall-archive");
    if (fileType == FT_IS_STORED)
        return QStringLiteral("application/x-is-stored");
    if (fileType == FT_INSTALLSHIELD3_ARCHIVE)
        return QStringLiteral("application/x-installshield3-archive");
    if (fileType == FT_EMT_IMAGE)
        return QStringLiteral("application/x-emt-disk-image");
    if (fileType == FT_GPFPACK)
        return QStringLiteral("application/x-gpfpack");
    if (fileType == FT_PAX)
        return QStringLiteral("application/x-gemview-pax");
    if (fileType == FT_SCF)
        return QStringLiteral("application/x-microsoft-scf");
    if (fileType == FT_SOLITAIRE_DELUXE)
        return QStringLiteral("application/x-solitaire-deluxe-installer");
    if (fileType == FT_INSTALIT_DATA)
        return QStringLiteral("application/x-instalit-data");
    if (fileType == FT_ARCV)
        return QStringLiteral("application/x-arcv");
    if (fileType == FT_PIMP_SFX)
        return QStringLiteral("application/x-pimp-sfx");
    if (fileType == FT_VISE_SFX)
        return QStringLiteral("application/x-vise-sfx");
    if (fileType == FT_FTCOMP)
        return QStringLiteral("application/x-os2-pack2");
    if (fileType == FT_FLS)
        return QStringLiteral("application/x-saveram-fls");
    if (fileType == FT_RTPATCH)
        return QStringLiteral("application/x-rtpatch");
    if (fileType == FT_RNC)
        return QStringLiteral("application/x-rnc");
    if (fileType == FT_MI10)
        return QStringLiteral("application/x-amiga-mi10");
    if (fileType == FT_DN_ARCHIVE)
        return QStringLiteral("application/x-dos-navigator-installer");
    if (fileType == FT_FPAK)
        return QStringLiteral("application/x-foxpro-fpak");
    if (fileType == FT_SOFTPAQ1_SFX)
        return QStringLiteral("application/x-compaq-softpaq1");
    if (fileType == FT_INSTALIT_SFX)
        return QStringLiteral("application/x-instalit-sfx");
    if (fileType == FT_LIF_COMPRESSED)
        return QStringLiteral("application/x-lif-compressed");
    if (fileType == FT_JASC_ARCHIVE)
        return QStringLiteral("application/x-jasc-installer-archive");
    if (fileType == FT_SSM_MODULE)
        return QStringLiteral("application/x-pictools-ssm");
    if (fileType == FT_SSBOB)
        return QStringLiteral("application/x-ssbob-slideshow");
    if (fileType == FT_IS_SKIN)
        return QStringLiteral("application/x-installshield-skin");
    if (fileType == FT_MACBINARY)
        return QStringLiteral("application/x-macbinary");
    if (fileType == FT_RESOURCE_FORK)
        return QStringLiteral("application/x-mac-resource-fork");
    if (fileType == FT_CPM_LBR)
        return QStringLiteral("application/x-cpm-lbr");
    if (fileType == FT_PARSEC_ARCHIVE)
        return QStringLiteral("application/x-parsec-resource-archive");
    if (fileType == FT_PMM)
        return QStringLiteral("audio/x-parsec-pmm");
    return QStringLiteral("application/octet-stream");
}

qint64 XGameStoreArchiveBase::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    qint64 nArchiveEnd = 0;
    if (!scanArchive(nullptr, &nArchiveEnd, pPdStruct)) return 0;
    return nArchiveEnd;
}

XBinary::OSNAME XGameStoreArchiveBase::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XGameStoreArchiveBase::getVersion()
{
    if (getFileType() == FT_INSTALLSHIELD3_SFX) return QStringLiteral("3");
    if (getFileType() == FT_IS14_SFX) return QStringLiteral("7");
    if (getFileType() == FT_RTPATCH) {
        const quint16 nVersion = read_uint16(2);
        if (nVersion >= 100) {
            return QStringLiteral("%1.%2")
                .arg(nVersion / 100)
                .arg(nVersion % 100, 2, 10, QLatin1Char('0'));
        }
    }
    return QString();
}

QList<QString> XGameStoreArchiveBase::getSearchSignatures()
{
    QList<QString> listResult;
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK) {
        listResult.append(QStringLiteral("'PACK'"));
    } else if (fileType == FT_DOOM_WAD) {
        listResult.append(QStringLiteral("'IWAD'"));
        listResult.append(QStringLiteral("'PWAD'"));
    } else if (fileType == FT_BUILD_GRP) {
        listResult.append(QStringLiteral("'KenSilverman'"));
    } else if (fileType == FT_DESCENT_HOG) {
        listResult.append(QStringLiteral("'DHF'"));
    } else if (fileType == FT_C64_T64) {
        listResult.append(QStringLiteral("'C64 tape image file'"));
        listResult.append(QStringLiteral("'C64S tape image file'"));
    } else if (fileType == FT_APPLESINGLE) {
        listResult.append(QStringLiteral("00051600"));
        listResult.append(QStringLiteral("00051607"));
    } else if (fileType == FT_APPLE_2IMG) {
        listResult.append(QStringLiteral("'2IMG'"));
    } else if (fileType == FT_DISK_DOUBLER) {
        listResult.append(QStringLiteral("ABCD0054"));
    } else if (fileType == FT_DISK_DOUBLER_DDA2) {
        listResult.append(QStringLiteral("'DDA2'"));
    } else if (fileType == FT_WINTERMUTE_DCP) {
        listResult.append(QStringLiteral("DEADC0DE'JUNK'"));
    } else if (fileType == FT_PYINSTALLER_PYZ) {
        listResult.append(QStringLiteral("'PYZ'00"));
    } else if (fileType == FT_AMIGA_LZX) {
        listResult.append(QStringLiteral("'LZX'"));
    } else if (fileType == FT_KA_ARCHIVE) {
        listResult.append(QStringLiteral("'KA Archive'00"));
    } else if (fileType == FT_LPAK) {
        listResult.append(QStringLiteral("'LPAK'"));
    } else if (fileType == FT_INSTALLSHIELD_BOOT) {
        listResult.append(QStringLiteral("'SZDD'88F027'33'"));
    } else if (fileType == FT_SABDU_IMAGE) {
        listResult.append(QStringLiteral("'SAB Diskette Utility'00"));
    } else if (fileType == FT_COMPAQ_LZH) {
        listResult.append(QStringLiteral("'CPQ_LZH'"));
    } else if (fileType == FT_WISE_SFX) {
        listResult.append(QStringLiteral("'WiseMain'"));
    } else if (fileType == FT_INSTALLSHIELD3_SFX) {
        listResult.append(QStringLiteral("9401000006000000"));
    } else if (fileType == FT_IS14_SFX) {
        listResult.append(QStringLiteral("4D5A*'InstallShield Setup Player 2K2'"));
    } else if (fileType == FT_GPINSTALL_SFX) {
        listResult.append(QStringLiteral("4D5A*'SPIS'1A'LH5'"));
    } else if (fileType == FT_INSTALLSHIELD_LAUNCHER) {
        listResult.append(QStringLiteral("4D5A*'InstallShield'00"));
    } else if (fileType == FT_EPFS_ARCHIVE) {
        listResult.append(QStringLiteral("'EPFS'"));
    } else if (fileType == FT_FINSTALL_ARCHIVE) {
        listResult.append(QStringLiteral("01'F Install 2'"));
    } else if (fileType == FT_INSTALLSHIELD3_ARCHIVE) {
        listResult.append(QStringLiteral("135D658C3A0102"));
    } else if (fileType == FT_EMT_IMAGE) {
        listResult.append(QStringLiteral("5C5C7AC5D4E340F0F0F1F0F0F1"));
    } else if (fileType == FT_GPFPACK) {
        listResult.append(QStringLiteral("C0000000'GPFPACK'00"));
    } else if (fileType == FT_PAX) {
        listResult.append(QStringLiteral("'LZF0'"));
    } else if (fileType == FT_SCF) {
        listResult.append(QStringLiteral("04000000"));
    } else if (fileType == FT_SOLITAIRE_DELUXE) {
        listResult.append(QStringLiteral("'Solitaire Deluxe'"));
    } else if (fileType == FT_INSTALIT_DATA) {
        listResult.append(QStringLiteral("00(04|05|06)"));
    } else if (fileType == FT_ARCV) {
        listResult.append(QStringLiteral("'ARCV'1001"));
    } else if (fileType == FT_PIMP_SFX) {
        listResult.append(QStringLiteral("4D5A*'PIMPFILE'"));
    } else if (fileType == FT_VISE_SFX) {
        listResult.append(QStringLiteral("4D5A*'ESIV'"));
    } else if (fileType == FT_FTCOMP) {
        listResult.append(QStringLiteral("A596FDFF"));
    } else if (fileType == FT_FLS) {
        listResult.append(QStringLiteral("'SaveRam'"));
    } else if (fileType == FT_RTPATCH) {
        listResult.append(QStringLiteral("'K*'"));
    } else if (fileType == FT_RNC) {
        listResult.append(QStringLiteral("'RNCA'"));
    } else if (fileType == FT_MI10) {
        listResult.append(QStringLiteral("'MI10'"));
    } else if (fileType == FT_DN_ARCHIVE) {
        listResult.append(QStringLiteral("848D0102"));
    } else if (fileType == FT_FPAK) {
        listResult.append(QStringLiteral("'FPAK'|'FPAC'"));
    } else if (fileType == FT_SOFTPAQ1_SFX) {
        listResult.append(QStringLiteral("4D5A*'FIT002'"));
    } else if (fileType == FT_INSTALIT_SFX) {
        listResult.append(QStringLiteral("4D5A*'934730434875'"));
    } else if (fileType == FT_LIF_COMPRESSED) {
        listResult.append(QStringLiteral("'DC'02000100"));
    } else if (fileType == FT_JASC_ARCHIVE) {
        listResult.append(QStringLiteral("(16|17|18|19|1A|1B)*"));
    } else if (fileType == FT_SSM_MODULE) {
        listResult.append(QStringLiteral("'SSM'00"));
    } else if (fileType == FT_SSBOB) {
        listResult.append(QStringLiteral("'SSBOB'"));
    } else if (fileType == FT_CPM_LBR) {
        listResult.append(QStringLiteral("00'           '0000"));
    } else if (fileType == FT_PMM) {
        listResult.append(QStringLiteral("'MTCVTS PSM 2.00'00"));
    }
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant>
XGameStoreArchiveBase::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGameStoreArchiveBase::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
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
    if (!bScanned || (getFileType() != fileType) ||
        !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis || !rangeWithin(nTotalSize, 0, nArchiveEnd)) {
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

XBinary::ARCHIVERECORD XGameStoreArchiveBase::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext)
        return ARCHIVERECORD();

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) return ARCHIVERECORD();

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (pContext->fileType != getFileType()) ||
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                entry.handleMethod);
    if (entry.bIsSolid) {
        if ((entry.nSubstreamOffset < 0) ||
            (entry.nStreamUnpackedSize < 0) ||
            (entry.nSolidFolderIndex < 0) ||
            (entry.nSubstreamOffset > entry.nStreamUnpackedSize) ||
            (entry.nUncompressedSize < 0) ||
            (entry.nUncompressedSize >
             (entry.nStreamUnpackedSize - entry.nSubstreamOffset))) {
            return ARCHIVERECORD();
        }
        result.mapProperties.insert(FPART_PROP_ISSOLID, true);
        result.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET,
                                    entry.nSubstreamOffset);
        result.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE,
                                    entry.nStreamUnpackedSize);
        result.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX,
                                    entry.nSolidFolderIndex);
    }
    if (entry.bCRC32Defined) {
        result.mapProperties.insert(
            FPART_PROP_CRC_TYPE,
            static_cast<quint32>(CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF));
        result.mapProperties.insert(FPART_PROP_RESULTCRC, entry.nCRC32);
    }
    if (!entry.sChecksum.isEmpty() && !entry.sChecksumType.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_CHECKSUM, entry.sChecksum);
        result.mapProperties.insert(FPART_PROP_CHECKSUMTYPE,
                                    entry.sChecksumType);
    }
    if (entry.mtDateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, entry.mtDateTime);
        result.mapProperties.insert(FPART_PROP_MTIME, entry.mtDateTime);
    }
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XGameStoreArchiveBase::moveToNext(UNPACK_STATE *pState,
                                       PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext)
        return false;

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) return false;

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
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XGameStoreArchiveBase::finishUnpack(UNPACK_STATE *pState,
                                         PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP>
XGameStoreArchiveBase::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);
    listResult.append(FPART_PROP_HEADER_OFFSET);
    listResult.append(FPART_PROP_HEADER_SIZE);
    listResult.append(FPART_PROP_FILEMODE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_ISSOLID);
    listResult.append(FPART_PROP_SUBSTREAMOFFSET);
    listResult.append(FPART_PROP_STREAMUNPACKEDSIZE);
    listResult.append(FPART_PROP_SOLIDFOLDERINDEX);
    listResult.append(FPART_PROP_CRC_TYPE);
    listResult.append(FPART_PROP_RESULTCRC);
    listResult.append(FPART_PROP_DATETIME);
    listResult.append(FPART_PROP_MTIME);
    return listResult;
}

bool XGameStoreArchiveBase::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
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

void *XGameStoreArchiveBase::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XGameStoreArchiveBase::setInternalInfo(void *pInternalInfo)
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
