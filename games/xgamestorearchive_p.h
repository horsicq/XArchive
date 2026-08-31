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
#ifndef XGAMESTOREARCHIVE_P_H
#define XGAMESTOREARCHIVE_P_H

#include "xarchive.h"

#include <QHash>
#include <QSet>

// Shared implementation detail for simple, store-only archive readers. It
// originated with the game formats, but also serves structurally equivalent
// legacy containers. It is deliberately non-instantiable: format recognition
// and table parsing remain in the concrete classes.
class XGameStoreArchiveBase : public XArchive {
public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
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
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override = 0;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

protected:
    struct ENTRY {
        qint64 nHeaderOffset = 0;
        qint64 nHeaderSize = 0;
        qint64 nDataOffset = 0;
        qint64 nDataSize = 0;
        qint64 nUncompressedSize = -1;
        HANDLE_METHOD handleMethod = HANDLE_METHOD_STORE;
        qint64 nSubstreamOffset = -1;
        qint64 nStreamUnpackedSize = -1;
        qint64 nSolidFolderIndex = -1;
        bool bIsSolid = false;
        bool bCRC32Defined = false;
        quint32 nCRC32 = 0;
        QString sChecksum;
        QString sChecksumType;
        QDateTime mtDateTime;
        QString sFileName;
    };

    enum { MAX_RECORDS = 100000 };

    explicit XGameStoreArchiveBase(QIODevice *pDevice, FT fileType);

    static quint32 readLE32(const uchar *pData);
    static bool rangeWithin(qint64 nTotalSize, qint64 nOffset,
                            qint64 nSize);
    static bool rangesOverlap(qint64 nOffset1, qint64 nSize1,
                              qint64 nOffset2, qint64 nSize2);
    static bool decodeName(const uchar *pData, qint32 nFieldSize,
                           bool bAsciiOnly, QString *pName);
    static bool makeUniquePath(
        const QString &sSource, QSet<QString> *pUsedFiles,
        QSet<QString> *pUsedDirectories,
        QHash<QString, qint32> *pNextSuffixes,
        QHash<QString, QString> *pResolvedDirectories, QString *pResult);

private:
    struct UNPACK_CONTEXT {
        QList<ENTRY> listEntries;
        FT fileType;
    };

    bool scanArchive(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                     PDSTRUCT *pPdStruct);
    virtual bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                            PDSTRUCT *pPdStruct) = 0;

private:
    INTERNAL_INFO m_internalInfo;
    // XBinary::setData() resets its generic file-type field.  Keep the
    // concrete reader identity when the object is rebound to another device.
    FT m_fileTypeHint;
};

#endif  // XGAMESTOREARCHIVE_P_H
