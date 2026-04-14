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
#ifndef XFREEARC_H
#define XFREEARC_H

#include "xarchive.h"

class XFREEARC : public XArchive {
    Q_OBJECT
public:
    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_ARCHIVE_HEADER,
        STRUCTID_BLOCK,
    };

    // FreeARC block types
    enum BLOCKTYPE {
        BLOCKTYPE_HEADER = 0x00,
        BLOCKTYPE_DATA = 0x02,
        BLOCKTYPE_DIR = 0x06,
        BLOCKTYPE_FOOTER = 0x08,
    };

    struct BLOCK {
        qint64 nOffset;
        qint64 nSize;
        quint8 nType;
        QString sCompressor;
    };

    explicit XFREEARC(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual FT getFileType() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QString getMIMEString() override;
    virtual QString getVersion() override;
    virtual QString getArch() override;
    virtual MODE getMode() override;
    virtual ENDIAN getEndian() override;

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QList<BLOCK> getBlocks(PDSTRUCT *pPdStruct = nullptr);
    static QString blockTypeToString(quint8 nType);

private:
    static const quint32 FREEARC_MAGIC = 0x01437241;  // "ArC\x01" LE
    static const qint32 FREEARC_SIGNATURE_SIZE = 4;
    static const qint32 FREEARC_HEADER_SIZE = 8;  // magic(4) + flags(2) + version(2)

    qint64 _findNextBlock(qint64 nOffset, qint64 nFileSize);
    QString _readCompressorString(qint64 nOffset, qint64 nMaxSize);
};

#endif  // XFREEARC_H

