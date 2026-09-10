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
#ifndef XCOPYQM_H
#define XCOPYQM_H

#include <QPointer>
#include <QSet>

#include "xbinary.h"

// Sydex CopyQM-family DOS tools keep their help/menu screens in a TX overlay.
// Each line is an independently framed bit stream using a serialized binary
// Huffman tree.  The decoded screens are exposed as 01.txt, 02.txt, ... .
class XCopyQM : public XBinary {
    Q_OBJECT

public:
    struct FILE_ENTRY {
        QString sName;
        QByteArray baData;
        qint64 nStreamOffset = 0;
        qint64 nStreamSize = 0;
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

    explicit XCopyQM(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XCopyQM() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    FT getFileType() override;
    MODE getMode() override;
    QString getArch() override;
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
    bool _parse(QList<FILE_ENTRY> *pEntries, qint64 *pnSourceSize, PDSTRUCT *pPdStruct);
    bool _isContextCurrent(const UNPACK_STATE *pState, const UNPACK_CONTEXT *pContext);

    QSet<UNPACK_CONTEXT *> m_setContexts;
};

#endif  // XCOPYQM_H
