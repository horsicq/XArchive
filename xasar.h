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
#ifndef XASAR_H
#define XASAR_H

#include "xarchive.h"

// Electron ASAR archive. Layout: a Chromium "Pickle" header (uint32 = 4,
// uint32 = header-payload size + 4, uint32 = json string length, uint32 =
// json byte length), the JSON directory, then the raw file blob. File offsets
// in the JSON are relative to the start of the blob.
class XASAR : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER
    };

    explicit XASAR(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType() override;
    virtual MODE getMode() override;
    virtual QString getMIMEString() override;
    virtual qint32 getType() override;
    virtual ENDIAN getEndian() override;
    virtual QString getArch() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual OSNAME getOsName() override;
    virtual QString getVersion() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    virtual QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct LINK_CONTEXT;

    struct ASAR_RECORD {
        QString sFileName;
        QString sLinkName;
        QString sExternalLogicalName;
        QString sExternalFileName;
        QByteArray baExternalSHA256;
        qint64 nOffset;  // absolute file offset
        qint64 nSize;
        bool bIsFolder;
        bool bIsExternal;
        bool bIsLink;
    };

    struct ASAR_UNPACK_CONTEXT {
        QList<ASAR_RECORD> listRecords;
        QString sExternalRoot;
        QString sExternalCanonicalRoot;
    };

    // Returns [jsonLength, jsonStringOffset] via out params; false if not ASAR.
    bool _readHeader(qint64 *pnJsonOffset, qint64 *pnJsonSize, qint64 *pnBlobOffset);
    bool _walkTree(const class QJsonObject &objFiles, const QString &sParent, qint64 nBlobOffset, QList<ASAR_RECORD> *pListRecords, PDSTRUCT *pPdStruct, qint32 nDepth,
                   bool bParentUnpacked = false);
    bool _prepareExternalRecords(ASAR_UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool _resolvePath(LINK_CONTEXT *pLinkContext, const QString &sInputPath,
                      qint32 nInitialLinkDepth, qint32 *pnTargetIndex,
                      QString *pFinalPath);
    bool _expandDirectory(LINK_CONTEXT *pLinkContext,
                          const QString &sAliasPath,
                          const QString &sTargetFolderPath,
                          QSet<QString> *pAncestry, qint32 nDepth);
    bool _resolveLinks(ASAR_UNPACK_CONTEXT *pContext);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XASAR_H
