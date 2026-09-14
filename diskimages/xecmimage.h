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
#ifndef XECMIMAGE_H
#define XECMIMAGE_H

#include "xarchive.h"

// Error Code Modeler (.ecm) CD image stream: "ECM\0" followed by typed records
// (literal bytes, Mode 1, Mode 2 Form 1, Mode 2 Form 2 sectors stored without
// their sync/EDC/ECC) and a trailing 32-bit check value over the decoded
// output.  The single member is the original raw image with the .ecm suffix
// removed; EDC and P/Q parity are regenerated per ECMA-130.  Unrelated to
// FT_ECM_PACK (EmmaSetup), which shares the four magic bytes.
class XEcmImage : public XArchive {
    Q_OBJECT

public:
    explicit XEcmImage(QIODevice *pDevice = nullptr);
    ~XEcmImage() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nFileSize;
        qint64 nOutputSize;
        qint64 nRecordCount;
        qint64 nLiteralBytes;
        qint64 nMode1Sectors;
        qint64 nMode2Form1Sectors;
        qint64 nMode2Form2Sectors;
        quint32 nStoredCheck;
        QString sFileName;
    };

    struct STREAM;

    bool parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XECMIMAGE_H
