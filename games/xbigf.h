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
#ifndef XBIGF_H
#define XBIGF_H

#include "xarchive.h"

// Ptero-Engine BIGF/ZBL resource archive, normally stored as .CBF.
//
// The archive has a 64-byte header, a data area and a variable-length directory
// at EOF.  Version zero stores directory names and uncompressed members as-is;
// later versions obfuscate directory records and stored member data.  A member
// beginning with "[..]" contains the engine's LSB-packed variable-width LZW
// stream.
class XBIGF final : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    explicit XBIGF(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
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
    struct BIGF_HEADER {
        quint8 nVersion;
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint64 nDataOffset;
        quint32 nNumberOfRecords;
    };

    struct BIGF_BLOCK {
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
    };

    struct BIGF_ENTRY {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nStoredSize;
        qint64 nUncompressedSize;
        quint64 nFileTime;
        QString sFileName;
        QList<BIGF_BLOCK> listBlocks;
        bool bCompressed;
    };

    struct BIGF_UNPACK_CONTEXT {
        QList<BIGF_ENTRY> listEntries;
        BIGF_HEADER header;
        qint64 nDeviceSize;
    };

    struct LZW_CONTEXT;

    static bool entryOffsetLess(const BIGF_ENTRY &a, const BIGF_ENTRY &b);
    bool resetLzwDictionary(LZW_CONTEXT *pContext);
    bool readLzwByte(LZW_CONTEXT *pContext, quint8 *pValue);
    bool readLzwBits(LZW_CONTEXT *pContext, qint32 nWidth, quint32 *pCode);
    bool alignLzwInput(LZW_CONTEXT *pContext);
    bool appendLzwDictionary(LZW_CONTEXT *pContext, quint32 nPrefix,
                             quint8 nSuffix);
    bool decodeLzwPhrase(LZW_CONTEXT *pContext, quint32 nCode,
                         quint8 *pFirstCharacter);

    bool scanArchive(BIGF_HEADER *pHeader, QList<BIGF_ENTRY> *pEntries,
                     PDSTRUCT *pPdStruct);
    bool readZeroTerminatedName(qint64 nOffset, qint64 nLimit,
                                QByteArray *pName, qint64 *pBytesConsumed,
                                PDSTRUCT *pPdStruct);
    bool unpackStoredRecord(const BIGF_ENTRY &entry, quint8 nVersion,
                            QIODevice *pOutput, UNPACK_STATE *pState,
                            PDSTRUCT *pPdStruct);
    bool unpackLzwRecord(const BIGF_ENTRY &entry, QIODevice *pOutput,
                         UNPACK_STATE *pState, PDSTRUCT *pPdStruct);
    bool unpackLzwBlock(const BIGF_BLOCK &block, qint64 nOutputBase,
                        QIODevice *pOutput, UNPACK_STATE *pState,
                        PDSTRUCT *pPdStruct);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XBIGF_H
