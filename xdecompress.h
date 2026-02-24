/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#ifndef XDECOMPRESS_H
#define XDECOMPRESS_H

#include "xzipaesdecoder.h"
#include "xzipcryptodecoder.h"
#include "xlzhdecoder.h"
#include "xrardecoder.h"
#include "xit214decoder.h"
#include "xdeflatedecoder.h"
#include "ximplodedecoder.h"
#include "xlzmadecoder.h"
#include "xlzwdecoder.h"
#include "xascii85decoder.h"
#include "xbzip2decoder.h"
#include "xshrinkdecoder.h"
#include "xreducedecoder.h"
#include "xthreadobject.h"
#include "xstoredecoder.h"
#include "xppmddecoder.h"

class XDecompress : public QObject {
    Q_OBJECT

public:
    explicit XDecompress(QObject *parent = nullptr);
    bool decompressFPART(const XBinary::FPART &fPart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, XBinary::PDSTRUCT *pPdStruct);
    bool decompressArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct);
    bool checkCRC(const QMap<XBinary::FPART_PROP, QVariant> &mapProperties, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct);
    bool _decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct);
    bool decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nIndex);
    QByteArray decomressToByteArray(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct);
    qint64 getCompressedDataSize(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct);

signals:
    void completed(qint64 nElapsedTime);
    void errorMessage(const QString &sErrorMessage);
    void warningMessage(const QString &sWarningMessage);
    void infoMessage(const QString &sInfoMessage);
};

#endif  // XDECOMPRESS_H
