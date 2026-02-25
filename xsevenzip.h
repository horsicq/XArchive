/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#ifndef XSEVENZIP_H
#define XSEVENZIP_H

#include "xarchive.h"
#include "xdecompress.h"
#include "xcompress.h"
#include "Algos/xlzmadecoder.h"
#include "Algos/xaesdecoder.h"
#include "Algos/xppmddecoder.h"
#include "Algos/xbzip2decoder.h"
#include "Algos/xdeflatedecoder.h"
#include <QBuffer>
#include <QFileInfo>
#include <QDir>

// TODO https://py7zr.readthedocs.io/en/latest/archive_format.html
class XSevenZip : public XArchive {
    Q_OBJECT

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_SIGNATUREHEADER,
        STRUCTID_HEADER
    };

    enum EIdEnum {
        k7zIdEnd = 0,
        k7zIdHeader,
        k7zIdArchiveProperties,
        k7zIdAdditionalStreamsInfo,
        k7zIdMainStreamsInfo,
        k7zIdFilesInfo,
        k7zIdPackInfo,
        k7zIdUnpackInfo,
        k7zIdSubStreamsInfo,
        k7zIdSize,
        k7zIdCRC,
        k7zIdFolder,
        k7zIdCodersUnpackSize,
        k7zIdNumUnpackStream,
        k7zIdEmptyStream,
        k7zIdEmptyFile,
        k7zIdAnti,
        k7zIdName,
        k7zIdCTime,
        k7zIdATime,
        k7zIdMTime,
        k7zIdWinAttrib,
        k7zIdComment,
        k7zIdEncodedHeader,
        k7zIdStartPos,
        k7zIdDummy
        // k7zNtSecure,
        // k7zParent,
        // k7zIsReal
        // Test
    };
#pragma pack(push)
#pragma pack(1)
    struct SIGNATUREHEADER {
        quint8 kSignature[6];  // {'7','z',0xBC,0xAF,0x27,0x1C}
        quint8 Major;          // now = 0
        quint8 Minor;          // now = 4
        quint32 StartHeaderCRC;
        quint64 NextHeaderOffset;
        quint64 NextHeaderSize;
        quint32 NextHeaderCRC;
    };
#pragma pack(pop)

    explicit XSevenZip(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual QString getVersion();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);
    virtual QString getFileFormatExt();
    virtual QString getFileFormatExtsString();
    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    virtual qint64 getImageSize() override;
    virtual FT getFileType();
    virtual ENDIAN getEndian();
    virtual MODE getMode();
    virtual QString getArch();
    virtual QString getMIMEString();

    SIGNATUREHEADER _read_SIGNATUREHEADER(qint64 nOffset);
    static QString idToSring(EIdEnum id);
    static HANDLE_METHOD coderToCompressMethod(const QByteArray &baCodec);
    static void _applyBCJFilter(QByteArray &baData, qint32 nOffset);

    static const QString PREFIX_k7zId;

    static QMap<quint64, QString> get_k7zId();
    static QMap<quint64, QString> get_k7zId_s();

    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct SEVENZ_UNPACK_CONTEXT {
        QList<ARCHIVERECORD> listArchiveRecords;  // Pre-parsed archive records
        QMap<QString, QIODevice *> mapDevices;
    };

    bool _decompress(QIODevice *pDevice, HANDLE_METHOD compressMethod, const QByteArray &baProperty, qint64 nOffset, qint64 nSize, qint64 nUncompressedSize,
                     PDSTRUCT *pPdStruct);

    enum SRTYPE {
        SRTYPE_UNKNOWN = 0,
        SRTYPE_ID,
        SRTYPE_NUMBER,
        SRTYPE_BYTE,
        SRTYPE_UINT32,
        SRTYPE_ARRAY
    };

    enum IMPTYPE {
        IMPTYPE_UNKNOWN = 0,
        IMPTYPE_NUMBEROFFOLDERS,
        IMPTYPE_NUMBEROFFILES,  // Number of files in archive
        IMPTYPE_NUMBEROFCODERS,
        IMPTYPE_STREAMCRC,
        IMPTYPE_STREAMOFFSET,
        IMPTYPE_STREAMSIZE,
        IMPTYPE_CODERUNPACKEDSIZE,
        IMPTYPE_STREAMUNPACKEDCRC,
        IMPTYPE_NUMBEROFPACKSTREAMS,
        IMPTYPE_CODER,
        IMPTYPE_CODERPROPERTY,
        IMPTYPE_FILENAME,
        IMPTYPE_FILECRC,
        IMPTYPE_FILEATTRIBUTES,
        IMPTYPE_FILETIME,
        IMPTYPE_FILEPACKEDSIZE,
        IMPTYPE_FILEUNPACKEDSIZE,
        IMPTYPE_NUMBEROFUNPACKSTREAM,  // Number of files in each folder (NumUnpackStream)
        IMPTYPE_EMPTYSTREAMDATA,
        IMPTYPE_EMPTYFILEDATA,
        IMPTYPE_CTIMEDATA,
        IMPTYPE_ATIMEDATA,
        IMPTYPE_MTIMEDATA,
        IMPTYPE_WINATTRIBDATA
    };

    struct SZRECORD {
        qint32 nRelOffset;
        qint32 nSize;
        QString sName;
        QVariant varValue;
        SRTYPE srType;
        VT valType;
        quint32 nFlags;
        IMPTYPE impType;
    };

    struct SZBOND {
        qint32 nPackIndex;
        qint32 nUnpackIndex;
    };

    struct SZCODER {
        QByteArray baCoder;
        QByteArray baProperty;
        bool bIsComplex;
        qint32 nNumInStreams;
        qint32 nNumOutStreams;
    };

    struct SZINSTREAM {
        qint64 nOffset;
        qint64 nSize;
        quint32 nCRC;
        // QList<qint64> listSubSreamSizes;
        // QList<quint32> listSubSreamCRC;
    };

    struct SZOUTSTREAM {
        qint64 nSize;
        quint32 nCRC;
    };

    struct SZFOLDER {
        QList<SZCODER> listCoders;
        QList<SZBOND> listBonds;
        QList<qint32> listStreamIndexes;
    };

    struct SZSTATE {
        char *pData;
        qint64 nCurrentOffset;
        qint64 nSize;
        bool bIsError;
        QString sErrorString;
        qint64 nStreamsBegin;
        quint64 nNumberOfFolders;  // Track folder count for SubStreamsInfo
        quint64 nNumberOfFiles;  // Track file count from FilesInfo (including extended count)
        quint64 nNumberOfCoders;
        QList<SZINSTREAM> listInStreams;
        QList<SZOUTSTREAM> listOutStreams;
        QList<SZFOLDER> listFolders;
        QByteArray baEmptyStreams;
        QByteArray baEmptyFiles;
        QList<QString> listFileNames;
        qint32 nCurrentStream;
        qint32 nCurrentSubstream;
    };

    QList<SZRECORD> _handleData(char *pData, qint64 nSize, PDSTRUCT *pPdStruct);
    bool _handleId(QList<SZRECORD> *pListRecords, EIdEnum id, SZSTATE *pState, qint32 nCount, bool bCheck, PDSTRUCT *pPdStruct, IMPTYPE impType);
    quint64 _handleNumber(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, quint32 nFlags, IMPTYPE impType);
    quint8 _handleByte(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType);
    quint32 _handleUINT32(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType);
    QByteArray _handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType);
};

#endif  // XSEVENZIP_H
