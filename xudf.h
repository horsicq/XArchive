/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#ifndef XUDF_H
#define XUDF_H

#include "xarchive.h"

class XUDF : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct UDF_TAG {
        quint16 nTagIdentifier;
        quint16 nDescriptorVersion;
        quint8 nChecksum;
        quint8 nReserved;
        quint16 nTagSerialNumber;
        quint16 nDescriptorCRC;
        quint16 nDescriptorCRCLength;
        quint32 nTagLocation;
    };

    struct UDF_EXTENT_AD {
        quint32 nLength;
        quint32 nLocation;
    };

    struct UDF_PRIMARY_VOLUME_DESCRIPTOR {
        UDF_TAG tag;
        quint32 nVolumeDescriptorSequenceNumber;
        quint32 nPrimaryVolumeDescriptorNumber;
        char szVolumeIdentifier[32];
        quint16 nVolumeSequenceNumber;
        quint16 nMaximumVolumeSequenceNumber;
        quint16 nInterchangeLevel;
        quint16 nMaximumInterchangeLevel;
        quint32 nCharacterSetList;
        quint32 nMaximumCharacterSetList;
        char szVolumeSetIdentifier[128];
        quint8 nDescriptorCharacterSet[64];
        quint8 nExplanatoryCharacterSet[64];
        UDF_EXTENT_AD volumeAbstract;
        UDF_EXTENT_AD volumeCopyrightNotice;
        quint8 nApplicationIdentifier[32];
        quint8 nRecordingDateAndTime[12];
        quint8 nImplementationIdentifier[32];
        quint8 nImplementationUse[64];
        quint32 nPredecessorVolumeDescriptorSequenceLocation;
        quint16 nFlags;
        quint8 nReserved[22];
    };

    struct UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER {
        UDF_TAG tag;
        UDF_EXTENT_AD mainVolumeDescriptorSequenceExtent;
        UDF_EXTENT_AD reserveVolumeDescriptorSequenceExtent;
        quint8 nReserved[480];
    };

    struct UDF_FILE_ENTRY {
        UDF_TAG tag;
        quint32 nICBTag;
        quint32 nUid;
        quint32 nGid;
        quint32 nPermissions;
        quint16 nFileLinkCount;
        quint8 nRecordFormat;
        quint8 nRecordDisplayAttributes;
        quint32 nRecordLength;
        quint64 nInformationLength;
        quint64 nLogicalBlocksRecorded;
        quint8 nAccessTime[12];
        quint8 nModificationTime[12];
        quint8 nAttributeTime[12];
        quint32 nCheckpoint;
        quint32 nExtendedAttributeICB;
        quint32 nImplementationIdentifier;
        quint64 nUniqueID;
        quint32 nLengthOfExtendedAttributes;
        quint32 nLengthOfAllocationDescriptors;
        // Followed by extended attributes and allocation descriptors
    };
#pragma pack(pop)

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_TAG,
        STRUCTID_ANCHOR_VOLUME_DESCRIPTOR,
        STRUCTID_PRIMARY_VOLUME_DESCRIPTOR,
        STRUCTID_FILE_ENTRY
    };

    enum UDF_TAG_IDENTIFIER {
        TAG_PRIMARY_VOLUME_DESCRIPTOR = 1,
        TAG_ANCHOR_VOLUME_DESCRIPTOR_POINTER = 2,
        TAG_VOLUME_DESCRIPTOR_POINTER = 3,
        TAG_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR = 4,
        TAG_PARTITION_DESCRIPTOR = 5,
        TAG_LOGICAL_VOLUME_DESCRIPTOR = 6,
        TAG_UNALLOCATED_SPACE_DESCRIPTOR = 7,
        TAG_TERMINATING_DESCRIPTOR = 8,
        TAG_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR = 9,
        TAG_FILE_SET_DESCRIPTOR = 256,
        TAG_FILE_IDENTIFIER_DESCRIPTOR = 257,
        TAG_ALLOCATION_EXTENT_DESCRIPTOR = 258,
        TAG_INDIRECT_ENTRY = 259,
        TAG_TERMINAL_ENTRY = 260,
        TAG_FILE_ENTRY = 261,
        TAG_EXTENDED_ATTRIBUTE_HEADER_DESCRIPTOR = 262,
        TAG_UNALLOCATED_SPACE_ENTRY = 263,
        TAG_SPACE_BITMAP_DESCRIPTOR = 264,
        TAG_PARTITION_INTEGRITY_ENTRY = 265,
        TAG_EXTENDED_FILE_ENTRY = 266
    };

    explicit XUDF(QIODevice *pDevice = nullptr);
    virtual ~XUDF();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getMIMEString() override;
    virtual FT getFileType() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    UDF_TAG _readTag(qint64 nOffset);
    UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER _readAnchorVolumeDescriptor(qint64 nOffset);
    UDF_PRIMARY_VOLUME_DESCRIPTOR _readPrimaryVolumeDescriptor(qint64 nOffset);

    QString getVolumeIdentifier();
    QString getVolumeSetIdentifier();

private:
    struct UDF_SCAN_CONTEXT {
        qint32 nBlockSize;
        qint64 nVolumeDescriptorSequenceOffset;
        qint64 nVolumeDescriptorSequenceSize;
    };

    struct UDF_UNPACK_CONTEXT {
        qint32 nBlockSize;
        QList<ARCHIVERECORD> listRecords;
        qint32 nCurrentRecordIndex;
    };

    qint32 _getBlockSize();
    qint64 _getAnchorVolumeDescriptorOffset();
    bool _isValidTag(qint64 nOffset, PDSTRUCT *pPdStruct);
    QList<ARCHIVERECORD> _parseFileSystem(qint32 nBlockSize, PDSTRUCT *pPdStruct);
    QString _cleanFileName(const QString &sFileName);

    QString m_sVolumeIdentifier;
    QString m_sVolumeSetIdentifier;
};

#endif  // XUDF_H
