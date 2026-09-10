/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrarsfx.h"

#include "xpe.h"

XRarSFX::XRarSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress, bool bRequireWinRarAttribution)
    : XSFX(pDevice, bIsImage, nModuleAddress, ARC_RAR), m_bRequireWinRarAttribution(bRequireWinRarAttribution)
{
}

bool XRarSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct) && (!m_bRequireWinRarAttribution || _isWinRarAttributed(pPdStruct));
}

bool XRarSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRarSFX sfx(pDevice);

    return sfx.isValid(pPdStruct);
}

bool XRarSFX::isWinRarValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRarSFX sfx(pDevice, false, -1, true);

    return sfx.isValid(pPdStruct);
}

XBinary::FT XRarSFX::getFileType()
{
    if (m_bRequireWinRarAttribution) {
        XPE pe(getDevice(), isImage(), getModuleAddress());
        return pe.is64() ? FT_PE64_WINRARSFX : FT_PE32_WINRARSFX;
    }

    return XSFX::getFileType();
}

QString XRarSFX::getVersion()
{
    QString sResult;

    if (m_bRequireWinRarAttribution && _isWinRarAttributed()) {
        XPE pe(getDevice(), isImage(), getModuleAddress());
        sResult = pe.getResourcesVersionValue("FileVersion").trimmed();
        if (sResult.isEmpty()) sResult = pe.getFileVersion().trimmed();
    }

    return sResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XRarSFX::getDefaultUnpackProperties()
{
    if (m_bRequireWinRarAttribution) {
        QPointer<XRarSFX> guardedThis(this);
        const bool bValid = guardedThis->isValid();
        if (!guardedThis) return QMap<UNPACK_PROP, QVariant>();
        if (!bValid) {
            return guardedThis->XBinary::getDefaultUnpackProperties();
        }
    }

    return XSFX::getDefaultUnpackProperties();
}

bool XRarSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (m_bRequireWinRarAttribution && !isValid(pPdStruct)) {
        return false;
    }

    return XSFX::initUnpack(pState, mapProperties, pPdStruct);
}

bool XRarSFX::_isWinRarAttributed(PDSTRUCT *pPdStruct)
{
    QPointer<XRarSFX> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    if (!guardedThis || !pInfo || !pInfo->bIsValid || (pInfo->arcType != ARC_RAR)) {
        return false;
    }

    XPE pe(guardedThis->getDevice(), guardedThis->isImage(), guardedThis->getModuleAddress());
    if (!pe.isValid(pPdStruct)) return false;

    const qint64 nTotalSize = guardedThis->getSize();
    const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if (!guardedThis || (pInfo->nArchiveOffset != nOverlayOffset) || (nOverlayOffset <= 0) || ((nTotalSize - nOverlayOffset) < 8)) {
        return false;
    }

    const QByteArray baHead = guardedThis->read_array_process(nOverlayOffset, 8, pPdStruct);
    if (!guardedThis || (baHead.size() != 8)) return false;
    const quint8 *p = reinterpret_cast<const quint8 *>(baHead.constData());

    // Preserve the WinRAR attribution contract: a PE stub, a
    // RAR4/RAR5 payload at the exact overlay boundary, and a WinRAR marker
    // inside the executable rather than inside an appended archive member.
    const bool bRar5 = (p[0] == 0x52) && (p[1] == 0x61) && (p[2] == 0x72) && (p[3] == 0x21) && (p[4] == 0x1A) && (p[5] == 0x07) && (p[6] == 0x01) && (p[7] == 0x00);
    const bool bRar4 = (p[0] == 0x52) && (p[1] == 0x61) && (p[2] == 0x72) && (p[3] == 0x21) && (p[4] == 0x1A) && (p[5] == 0x07) && (p[6] == 0x00);
    if (!bRar5 && !bRar4) return false;

    const bool bResult = (guardedThis->find_ansiString(0, nOverlayOffset, "name=\"WinRAR", pPdStruct) != -1) ||
                         (guardedThis && (guardedThis->find_ansiString(0, nOverlayOffset, "sfxrar", pPdStruct) != -1)) ||
                         (guardedThis && (guardedThis->find_ansiString(0, nOverlayOffset, "sfxcon", pPdStruct) != -1));

    return guardedThis && bResult && XBinary::isPdStructNotCanceled(pPdStruct);
}
