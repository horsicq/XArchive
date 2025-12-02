// DIE's modification 2017-2025
// Copyright (c) 2020-2025 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#ifndef XDMG_H
#define XDMG_H

#include "xarchive.h"

class XDMG : public XArchive {
    Q_OBJECT

public:
    // DMG stripe types (from Apple Disk Image format)
    enum DMG_STRIPE_TYPE {
        DMG_STRIPE_EMPTY = 0x00000000,
        DMG_STRIPE_STORED = 0x00000001,
        DMG_STRIPE_ZEROES = 0x00000002,
        DMG_STRIPE_ADC = 0x80000004,
        DMG_STRIPE_DEFLATE = 0x80000005,
        DMG_STRIPE_BZ = 0x80000006,
        DMG_STRIPE_SKIP = 0x7FFFFFFE,
        DMG_STRIPE_END = 0xFFFFFFFF
    };

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_KOLY_BLOCK,
        STRUCTID_MISH_BLOCK,
        STRUCTID_STRIPE
    };

    // 512-byte koly block (trailer)
    struct KOLY_BLOCK {
        quint32 nMagic;           // 0x6b6f6c79 ("koly")
        quint32 nVersion;
        quint32 nHeaderLength;
        quint32 nFlags;
        quint64 nRunningOffset;
        quint64 nDataForkOffset;
        quint64 nDataForkLength;
        quint64 nResourceForkOffset;
        quint64 nResourceForkLength;
        quint32 nSegment;
        quint32 nSegmentCount;
        quint8 segmentID[16];
        quint32 dataChecksum[34];
        quint64 nXmlOffset;
        quint64 nXmlLength;
        quint8 padding[120];
        quint32 masterChecksum[34];
        quint32 nImageVariant;
        quint64 nSectorCount;
        quint32 reserved[3];
    };

    // 204-byte mish block
    struct MISH_BLOCK {
        quint32 nMagic;        // 0x6d697368 ("mish")
        quint32 nVersion;
        quint64 nStartSector;
        quint64 nSectorCount;
        quint64 nDataOffset;
        quint32 nBufferCount;
        quint32 nDescriptorBlocks;
        quint8 reserved[24];
        quint32 checksum[34];
        quint32 nBlockDataCount;
    };

    // 40-byte block data (stripe)
    struct BLOCK_DATA {
        quint32 nType;
        quint32 nReserved;
        quint64 nStartSector;
        quint64 nSectorCount;
        quint64 nDataOffset;
        quint64 nDataLength;
    };

    struct DMG_UNPACK_CONTEXT {
        QIODevice *pSourceDevice;
        QByteArray baXmlData;
        QList<MISH_BLOCK> listMishBlocks;
        QList<QList<BLOCK_DATA>> listStripes;
        qint32 nCurrentMishIndex;
        qint32 nCurrentStripeIndex;
        qint64 nCurrentFileIndex;
        QString sCurrentFileName;
    };

    explicit XDMG(QIODevice *pDevice = nullptr);
    virtual ~XDMG();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual QString getArch() override;
    virtual FT getFileType() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getVersion() override;
    virtual MODE getMode() override;
    virtual ENDIAN getEndian() override;
    virtual QString structIDToString(quint32 nID) override;

    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    // Archive methods
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;

    // Streaming unpack API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapOptions, PDSTRUCT *pPdStruct) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct) override;

    static KOLY_BLOCK readKolyBlock(QIODevice *pDevice, qint64 nOffset);
    static MISH_BLOCK readMishBlock(QIODevice *pDevice, qint64 nOffset);
    static BLOCK_DATA readBlockData(QIODevice *pDevice, qint64 nOffset);

private:
    QByteArray _parseXmlForMish(const QByteArray &baXml, PDSTRUCT *pPdStruct);
    bool _decompressStripe(const BLOCK_DATA &stripe, QIODevice *pDevice, PDSTRUCT *pPdStruct);
    bool _writeZeroes(QIODevice *pDevice, qint64 nSize);
};

#endif  // XDMG_H
