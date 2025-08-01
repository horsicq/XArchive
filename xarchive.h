/* Copyright (c) 2019-2025 hors<horsicq@gmail.com>
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
#ifndef XARCHIVE_H
#define XARCHIVE_H

#include "LzmaDec.h"
#include "bzlib.h"
#include "xbinary.h"
#include "zlib.h"
#ifdef PPMD_SUPPORT
#include "Ppmd7.h"
#endif
#include "xcompress.h"
#include "Compress/xlzhdecoder.h"
#include "Compress/xrardecoder.h"

class XArchive : public XBinary {
    Q_OBJECT

public:
    enum TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_ARCHIVE,
        TYPE_DOSEXTENDER
        // TODO more
    };

    struct COMPRESS_INFO {
        COMPRESS_METHOD compressMethod;
        QString sOptions;
    };

    struct SPINFO {
        QString sRecordName;
        quint32 nCRC32;
        qint64 nUncompressedSize;
        quint64 nWindowSize;
        bool bIsSolid;  // For RAR
        COMPRESS_METHOD compressMethod;
        FT fileType;
    };

    struct RECORD {
        SPINFO spInfo;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nOptHeaderOffset;
        qint64 nOptHeaderSize;
        QString sUUID;
        // for targz
        // qint64 nLayerOffset;
        // qint64 nLayerSize;
        // COMPRESS_METHOD layerCompressMethod;
    };

    enum COMPRESS_RESULT {
        COMPRESS_RESULT_UNKNOWN = 0,
        COMPRESS_RESULT_OK,
        COMPRESS_RESULT_DATAERROR,
        COMPRESS_RESULT_MEMORYERROR,
        COMPRESS_RESULT_BUFFERERROR,
        COMPRESS_RESULT_METHODERROR,
        COMPRESS_RESULT_READERROR,
        COMPRESS_RESULT_WRITEERROR
        // TODO more
    };

    static const qint32 COMPRESS_BUFFERSIZE = 0x4000;    // TODO Check mb set/get ???
    static const qint32 DECOMPRESS_BUFFERSIZE = 0x4000;  // TODO Check mb set/get ???

    explicit XArchive(QIODevice *pDevice = nullptr);

    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) = 0;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) = 0;

    struct DECOMPRESSSTRUCT {
        SPINFO spInfo;
        QIODevice *pSourceDevice;
        QIODevice *pDestDevice;
        qint64 nInSize;
        qint64 nOutSize;
        qint64 nDecompressedOffset;
        qint64 nDecompressedLimit;
        qint64 nDecompressedWrote;
        bool bLimit;
    };

    static COMPRESS_RESULT _decompress(DECOMPRESSSTRUCT *pDecompressStruct, PDSTRUCT *pPdStruct = nullptr);
    static bool _decompressRecord(const RECORD *pRecord, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset,
                                  qint64 nDecompressedLimit);
    static COMPRESS_RESULT _compress(COMPRESS_METHOD compressMethod, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct = nullptr);
    static COMPRESS_RESULT _compress_deflate(QIODevice *pSourceDevice, QIODevice *pDestDevice, qint32 nLevel, qint32 nMethod, qint32 nWindowsBits, qint32 nMemLevel,
                                             qint32 nStrategy, PDSTRUCT *pPdStruct = nullptr);
    QByteArray decompress(const RECORD *pRecord, PDSTRUCT *pPdStruct = nullptr, qint64 nDecompressedOffset = 0, qint64 nDecompressedLimit = -1);
    QByteArray decompress(QList<RECORD> *pListArchive, const QString &sRecordFileName, PDSTRUCT *pPdStruct = nullptr);
    QByteArray decompress(const QString &sRecordFileName, PDSTRUCT *pPdStruct = nullptr);
    bool decompressToFile(const RECORD *pRecord, const QString &sResultFileName, PDSTRUCT *pPdStruct = nullptr);
    bool decompressToDevice(const RECORD *pRecord, QIODevice *pDestDevice, PDSTRUCT *pPdStruct = nullptr);
    bool decompressToFile(QList<RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct = nullptr);
    bool decompressToPath(QList<RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultPathName, PDSTRUCT *pPdStruct = nullptr);
    bool decompressToFile(const QString &sArchiveFileName, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct = nullptr);
    bool decompressToPath(const QString &sArchiveFileName, const QString &sRecordPathName, const QString &sResultPathName, PDSTRUCT *pPdStruct = nullptr);
    bool dumpToFile(const RECORD *pRecord, const QString &sFileName, PDSTRUCT *pPdStruct = nullptr);
    static RECORD getArchiveRecord(const QString &sRecordFileName, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct = nullptr);
    static RECORD getArchiveRecordByUUID(const QString &sUUID, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct = nullptr);
    bool isArchiveRecordPresent(const QString &sRecordFileName, PDSTRUCT *pPdStruct = nullptr);
    static bool isArchiveRecordPresent(const QString &sRecordFileName, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct = nullptr);
    static bool isArchiveRecordPresentExp(const QString &sRecordFileName, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct = nullptr);
    static quint32 getCompressBufferSize();
    static quint32 getDecompressBufferSize();
    static void showRecords(QList<RECORD> *pListArchive);
    virtual MODE getMode();
    //    virtual _MEMORY_MAP getMemoryMap(); // TODO
    virtual qint32 getType();
    virtual QString typeIdToString(qint32 nType);
    virtual bool isArchive();

private:
    static bool _writeToDevice(char *pBuffer, qint32 nBufferSize, DECOMPRESSSTRUCT *pDecompressStruct);
};

#endif  // XARCHIVE_H
