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
#ifndef XSETUPFACTORY_H
#define XSETUPFACTORY_H

#include <QPointer>
#include <QSet>

#include "xbinary.h"

// Setup Factory 6 installers use a PE overlay containing a short engine-file
// chain followed by headerless payload streams. irsetup.dat is a DCL-compressed
// MFC CArchive manifest that supplies the payload boundaries and names.
class XSetupFactory : public XBinary {
    Q_OBJECT

public:
    struct ENGINE_ENTRY {
        QString sName;
        qint64 nDataOffset = 0;
        qint64 nPackedSize = 0;
        quint32 nCRC32 = 0;
    };

    struct FILE_ENTRY {
        QString sName;
        qint64 nDataOffset = 0;
        qint64 nPackedSize = 0;
        qint64 nRawSize = 0;
        quint32 nCRC32 = 0;
        quint32 nUnixTime = 0;
        bool bStored = false;
    };

    struct UNPACK_CONTEXT {
        QList<FILE_ENTRY> listEntries;
        QPointer<QIODevice> pSourceDevice;
        UNPACK_STATE *pOwnerState = nullptr;
        QByteArray baToken;
        quint64 nDeviceGeneration = 0;
        qint64 nSourceSize = 0;
        qint32 nCurrentIndex = 0;
        qint64 nCurrentOffset = 0;
    };

    explicit XSetupFactory(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XSetupFactory() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    FT getFileType() override;
    MODE getMode() override;
    QString getVersion() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override;

private:
    bool _scanEngine(QList<ENGINE_ENTRY> *pEntries, qint64 *pnPayloadOffset, qint64 *pnSourceSize, bool *pbIs64, QString *psVersion,
                     PDSTRUCT *pPdStruct);
    bool _buildEntries(QList<FILE_ENTRY> *pEntries, qint64 *pnSourceSize, PDSTRUCT *pPdStruct);
    bool _isContextCurrent(const UNPACK_STATE *pState, const UNPACK_CONTEXT *pContext);

    QSet<UNPACK_CONTEXT *> m_setContexts;
};

#endif  // XSETUPFACTORY_H
