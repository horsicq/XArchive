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
#ifndef XWARC_H
#define XWARC_H

#include "xarchive.h"

class XWARC : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    explicit XWARC(QIODevice *pDevice = nullptr);
    ~XWARC();

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    FT getFileType() override;
    ENDIAN getEndian() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct WARC_ENTRY {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nNextOffset;
        QString sFileName;
        QDateTime created;
        QDateTime modified;
    };

    struct WARC_UNPACK_CONTEXT {
        QList<WARC_ENTRY> listEntries;
        qint32 nCurrentRecord;
    };

    bool _readHeader(qint64 nOffset, QByteArray *pHeader, qint64 *pDataOffset, PDSTRUCT *pPdStruct);
    bool _parseRecord(qint64 nOffset, WARC_ENTRY *pEntry, bool *pVisible, PDSTRUCT *pPdStruct);
    bool _scanArchive(QList<WARC_ENTRY> *pEntries, qint64 *pArchiveEnd, PDSTRUCT *pPdStruct);

    static bool _parseVersion(const QByteArray &line);
    static bool _parseUnsignedDecimal(const QByteArray &value, qint64 *pResult);
    static bool _parseDate(const QByteArray &value, QDateTime *pResult);
    static bool _mapTargetURI(const QByteArray &value, QString *pResult);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XWARC_H
