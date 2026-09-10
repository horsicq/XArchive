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
#ifndef XRIB_H
#define XRIB_H

#include "xarchive.h"

// Parsec engine's backwards-decoded RIB resource wrapper.  RIB is a single
// compressed stream rather than a named multi-file archive; exposing it through
// XArchive gives callers the same transactional extraction API as gzip and the
// other one-stream formats.
class XRIB final : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {
        qint64 nPackedSize = 0;
        qint64 nUncompressedSize = 0;
    };

    explicit XRIB(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    void setInternalInfo(void *pInternalInfo) override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    OSNAME getOsName() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

    // Decode the bytes following the eight-byte RIB header.  This helper is
    // public so the codec can be covered independently of archive discovery
    // and reused by containers whose members are bounded SubDevices.
    static bool decompress(const QByteArray &baPackedData, qint64 nUncompressedSize, QByteArray *pUncompressedData, PDSTRUCT *pPdStruct = nullptr);

private:
    struct RIB_HEADER_INFO {
        qint64 nFileSize = 0;
        qint64 nPackedSize = 0;
        qint64 nUncompressedSize = 0;
        QByteArray baPayloadPrefix;
    };

    struct RIB_UNPACK_CONTEXT {
        qint64 nFileSize = 0;
        qint64 nPackedSize = 0;
        qint64 nUncompressedSize = 0;
        QString sFileName;
    };

    bool readHeaderInfo(RIB_HEADER_INFO *pInfo, PDSTRUCT *pPdStruct = nullptr);
    static QString payloadExtension(const QByteArray &baPrefix);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XRIB_H
