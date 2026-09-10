/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRARSFX_H
#define XRARSFX_H

#include "xsfx.h"

class XRarSFX : public XSFX {
    Q_OBJECT

public:
    explicit XRarSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1, bool bRequireWinRarAttribution = false);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    static bool isWinRarValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    QString getVersion() override;
    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;

private:
    bool _isWinRarAttributed(PDSTRUCT *pPdStruct = nullptr);
    bool m_bRequireWinRarAttribution;
};

#endif  // XRARSFX_H
