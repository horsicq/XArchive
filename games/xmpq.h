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
#ifndef XMPQ_H
#define XMPQ_H

#include "../xarchive.h"

// Native reader for classic Blizzard MPQ archives.  The implementation does
// not depend on StormLib: it validates and decrypts the classic hash/block
// tables, recovers sector keys where the MPQ format permits that, and decodes
// the PKWARE DCL (Implode) streams used by early Blizzard games.
class XMPQ final : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    explicit XMPQ(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice,
                        PDSTRUCT *pPdStruct = nullptr);

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
    struct MPQ_HEADER {
        qint64 nHeaderOffset;
        quint32 nHeaderSize;
        quint64 nArchiveSize;
        quint16 nFormatVersion;
        quint32 nSectorSize;
        quint64 nHashTableOffset;
        quint64 nBlockTableOffset;
        quint64 nHiBlockTableOffset;
        quint32 nHashTableEntries;
        quint32 nBlockTableEntries;
    };

    struct MPQ_HASH_ENTRY {
        quint32 nHashA;
        quint32 nHashB;
        quint16 nLocale;
        quint8 nPlatform;
        quint8 nReserved;
        quint32 nBlockIndex;
    };

    struct MPQ_BLOCK_ENTRY {
        quint64 nFileOffset;
        quint32 nCompressedSize;
        quint32 nUncompressedSize;
        quint32 nFlags;
    };

    struct MPQ_ENTRY {
        MPQ_BLOCK_ENTRY block;
        qint64 nHashEntryOffset;
        quint32 nHashIndex;
        quint32 nBlockIndex;
        quint16 nLocale;
        quint8 nPlatform;
        QString sFileName;
        bool bFileNameKnown;
    };

    struct MPQ_UNPACK_CONTEXT {
        MPQ_HEADER header;
        QList<MPQ_ENTRY> listEntries;
    };

    bool scanArchive(MPQ_HEADER *pHeader, QList<MPQ_ENTRY> *pEntries,
                     PDSTRUCT *pPdStruct);
    bool decodeEntry(const MPQ_HEADER &header, const MPQ_ENTRY &entry,
                     QIODevice *pOutputDevice,
                     const QMap<UNPACK_PROP, QVariant> &mapProperties,
                     PDSTRUCT *pPdStruct);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XMPQ_H
