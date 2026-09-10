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
#ifndef XINSTALLSHIELD_H
#define XINSTALLSHIELD_H

#include "xiscab.h"

// InstallShield installer executables that embed a complete ISc( media set
// (one catalog blob plus its data volumes) inside the PE image — the layout
// used by single-file InstallShield setups and by ISSetupFile payload members.
// Detection requires a PE host and a structurally valid embedded catalog with
// at least one embedded volume; the InstallShield engine's own "ISc(" compare
// immediates are rejected by the header sanity checks.  Extraction reuses the
// XISCab streaming machinery: the embedded blobs are published to the shared
// unpack context as container-relative volume windows.
class XInstallShield : public XISCab {
    Q_OBJECT

public:
    struct MEDIA_LAYOUT {
        bool bIsValid = false;
        bool bIs64 = false;
        quint64 nCatalogOffset = 0;
        quint64 nCatalogSize = 0;
        qint32 nMajorVersion = 0;
        QMap<quint32, EMBEDDED_VOLUME> mapVolumes;
    };

    explicit XInstallShield(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);

    // Note: the inherited XISCab internal-info cache probes ISc( at device
    // offset 0 and therefore records bIsValid=false for a PE host.  That
    // cache is only consumed by XISCab::isValid()/getVersion(), both of which
    // are overridden here with media-scan-based implementations.
    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    FT getFileType() override;
    MODE getMode() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

protected:
    bool _loadCatalog(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct) const override;

private:
    MEDIA_LAYOUT _scanMedia(PDSTRUCT *pPdStruct) const;
};

#endif  // XINSTALLSHIELD_H
