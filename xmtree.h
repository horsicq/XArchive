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
#ifndef XMTREE_H
#define XMTREE_H

#include "xarchive.h"

class XMTree : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    explicit XMTree(QIODevice *pDevice = nullptr);
    ~XMTree();

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
    struct MTREE_OPTION {
        bool bHasValue;
        QByteArray baValue;
    };

    struct MTREE_ENTRY {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nStreamOffset;
        QString sFileName;
        bool bIsFolder;
        bool bHasMode;
        quint32 nMode;
        bool bHasUID;
        quint32 nUID;
        bool bHasGID;
        quint32 nGID;
        bool bHasMTime;
        QDateTime modified;
    };

    struct MTREE_UNPACK_CONTEXT {
        QList<MTREE_ENTRY> listEntries;
        qint32 nCurrentRecord;
    };

    bool _readPhysicalLine(qint64 nOffset, QByteArray *pLine, qint64 *pNextOffset, bool *pHadNewline, PDSTRUCT *pPdStruct);
    bool _readLogicalLine(qint64 nOffset, QByteArray *pLine, qint64 *pNextOffset, PDSTRUCT *pPdStruct);
    bool _scanArchive(QList<MTREE_ENTRY> *pEntries, qint64 *pArchiveEnd, PDSTRUCT *pPdStruct);

    static QList<QByteArray> _splitTokens(const QByteArray &line);
    static bool _decodeEscapes(const QByteArray &value, QByteArray *pResult);
    static bool _parseOption(const QByteArray &token, MTREE_OPTION *pOption, QByteArray *pKey);
    static bool _validateOption(const QByteArray &key, const MTREE_OPTION &option);
    static bool _parseUnsigned(const QByteArray &value, quint32 nBase, quint64 nMaximum, quint64 *pResult);
    static bool _parseTime(const QByteArray &value, QDateTime *pResult);
    static bool _makeSafePath(const QByteArray &rawName, const QString &sCurrentDirectory, QString *pPath, bool *pIsFull);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XMTREE_H
