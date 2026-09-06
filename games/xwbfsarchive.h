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
#ifndef XWBFSARCHIVE_H
#define XWBFSARCHIVE_H

#include "xarchive.h"

// Native reader for Nintendo Wii Backup File System containers.  A WBFS
// member is a sparse block map rather than a contiguous payload, so each Wii
// disc is exposed as an archive-stream ISO record and reconstructed on demand.
class XWBFSArchive final : public XArchive
{
    Q_OBJECT

public:
    explicit XWBFSArchive(QIODevice *pDevice = nullptr);
    ~XWBFSArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct ENTRY {
        qint64 nDiscInfoOffset = 0;
        qint64 nDiscInfoSize = 0;
        qint64 nWbfsBlockSize = 0;
        qint64 nMapEntries = 0;
        qint64 nMappedSize = 0;
        qint64 nVirtualSize = 0;
        QByteArray baBlockMap;
        QString sFileName;
        QString sGameId;
        QString sGameTitle;
    };

    struct CONTEXT {
        QList<ENTRY> listEntries;
        qint64 nHeaderSize = 0;
        qint64 nArchiveEnd = 0;
        qint64 nSourceSize = 0;
        qint64 nWbfsBlockSize = 0;
        qint64 nMapEntries = 0;
        qint64 nDiscInfoSize = 0;
    };

    bool scanArchive(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XWBFSARCHIVE_H
