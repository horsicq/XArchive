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
#include "xwim.h"
#include "Algos/xlzxdecoder.h"
#include "Algos/xxpressdecoder.h"
#include "subdevice.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryFile>
#include <new>
#include <limits>

#ifdef Q_OS_WIN
#include <io.h>
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

static const qint32 WIM_MAX_IMAGES = 65536;
static const qint32 WIM_MAX_RECORDS = 1000000;
static const qint32 WIM_MAX_DIRECTORY_DEPTH = 256;
static const qint32 WIM_MAX_PATH_LENGTH = 32768;
static const quint64 WIM_MAX_BUFFERED_RESOURCE_SIZE = 256ULL * 1024ULL * 1024ULL;
static const quint32 WIM_MAX_CHUNK_SIZE = 0x40000000U;
static const quint32 WIM_MAX_XPRESS_CHUNK_SIZE = 64U * 1024U;

static XBinary::XCONVERT _TABLE_XWIM_STRUCTID[] = {{XWIM::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XWIM::STRUCTID_WIM_HEADER, "WIM_HEADER", QString("WIM header")}};

static bool wimWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nSize < 0) || ((nSize > 0) && !pData)) return false;

    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return false;
    const bool bSeekable = !guardedDevice->isSequential();
    if (!guardedDevice) return false;
    const qint64 nStart = bSeekable ? guardedDevice->pos() : -1;
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!guardedDevice || (bSeekable && (nStart < 0))) return false;

    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!guardedDevice ||
            (bSeekable &&
             ((nDone > nMax - nStart) ||
              !guardedDevice->seek(nStart + nDone)))) return false;
        const qint64 nWritten = guardedDevice->write(
            pData + nDone, nSize - nDone);
        if (!guardedDevice) return false;
        if ((nWritten <= 0) || (nWritten > (nSize - nDone))) return false;
        nDone += nWritten;
    }

    if (!guardedDevice || (nDone != nSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (bSeekable &&
        ((nSize > nMax - nStart) ||
         !guardedDevice->seek(nStart + nSize))) return false;
    return true;
}

static QByteArray wimSha1(const QByteArray &baData, XBinary::PDSTRUCT *pPdStruct)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    qint64 nOffset = 0;
    while ((nOffset < baData.size()) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)qMin<qint64>(1 << 20, (qint64)baData.size() - nOffset);
        hash.addData(baData.constData() + nOffset, nChunk);
        nOffset += nChunk;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return QByteArray();
    return hash.result();
}

static QIODevice *wimUnwrapDevice(QIODevice *pDevice)
{
    QSet<QIODevice *> stVisited;
    while (pDevice && !stVisited.contains(pDevice)) {
        stVisited.insert(pDevice);
        SubDevice *pSubDevice = dynamic_cast<SubDevice *>(pDevice);
        if (!pSubDevice) break;
        QIODevice *pOriginal = pSubDevice->getOrigDevice();
        if (!pOriginal || (pOriginal == pDevice)) break;
        pDevice = pOriginal;
    }
    return pDevice;
}

static bool wimDevicesAlias(QIODevice *pSource, QIODevice *pDestination)
{
    pSource = wimUnwrapDevice(pSource);
    pDestination = wimUnwrapDevice(pDestination);
    if (!pSource || !pDestination) return false;
    if (pSource == pDestination) return true;

    QBuffer *pSourceBuffer = dynamic_cast<QBuffer *>(pSource);
    QBuffer *pDestinationBuffer = dynamic_cast<QBuffer *>(pDestination);
    if (pSourceBuffer && pDestinationBuffer &&
        (&pSourceBuffer->buffer() == &pDestinationBuffer->buffer())) {
        return true;
    }

    QFile *pSourceFile = dynamic_cast<QFile *>(pSource);
    QFile *pDestinationFile = dynamic_cast<QFile *>(pDestination);
    if (!pSourceFile || !pDestinationFile) return false;

    QPointer<QFile> guardedSourceFile(pSourceFile);
    QPointer<QFile> guardedDestinationFile(pDestinationFile);
    if (!guardedSourceFile || !guardedDestinationFile) return true;

    const QString sSourceFileName = guardedSourceFile->fileName();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    const QString sDestinationFileName = guardedDestinationFile->fileName();
    if (!guardedSourceFile || !guardedDestinationFile) return true;

    const QFileInfo sourceInfo(sSourceFileName);
    const QFileInfo destinationInfo(sDestinationFileName);
    QString sSourcePath = sourceInfo.canonicalFilePath();
    QString sDestinationPath = destinationInfo.canonicalFilePath();
    if (sSourcePath.isEmpty()) sSourcePath = QDir::cleanPath(sourceInfo.absoluteFilePath());
    if (sDestinationPath.isEmpty()) sDestinationPath = QDir::cleanPath(destinationInfo.absoluteFilePath());
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif
    if (!sSourcePath.isEmpty() && !sDestinationPath.isEmpty() &&
        (QString::compare(QDir::fromNativeSeparators(sSourcePath),
                          QDir::fromNativeSeparators(sDestinationPath), caseSensitivity) == 0)) {
        return true;
    }

    const int nSourceDescriptor = guardedSourceFile->handle();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    const int nDestinationDescriptor = guardedDestinationFile->handle();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    if ((nSourceDescriptor < 0) || (nDestinationDescriptor < 0)) return false;
#ifdef Q_OS_WIN
    const intptr_t nSourceHandle = _get_osfhandle(nSourceDescriptor);
    const intptr_t nDestinationHandle = _get_osfhandle(nDestinationDescriptor);
    if ((nSourceHandle == -1) || (nDestinationHandle == -1)) return false;
    BY_HANDLE_FILE_INFORMATION sourceFileInformation = {};
    BY_HANDLE_FILE_INFORMATION destinationFileInformation = {};
    return GetFileInformationByHandle(reinterpret_cast<HANDLE>(nSourceHandle), &sourceFileInformation) &&
           GetFileInformationByHandle(reinterpret_cast<HANDLE>(nDestinationHandle), &destinationFileInformation) &&
           (sourceFileInformation.dwVolumeSerialNumber == destinationFileInformation.dwVolumeSerialNumber) &&
           (sourceFileInformation.nFileIndexHigh == destinationFileInformation.nFileIndexHigh) &&
           (sourceFileInformation.nFileIndexLow == destinationFileInformation.nFileIndexLow);
#elif defined(Q_OS_UNIX)
    struct stat sourceStatus = {};
    struct stat destinationStatus = {};
    return (fstat(nSourceDescriptor, &sourceStatus) == 0) &&
           (fstat(nDestinationDescriptor, &destinationStatus) == 0) &&
           (sourceStatus.st_dev == destinationStatus.st_dev) &&
           (sourceStatus.st_ino == destinationStatus.st_ino);
#else
    return false;
#endif
}

static quint16 read_uint16_le(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset > ((qint64)baData.size() - 2))) {
        return 0;
    }

    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);

    return (quint16)p[0] | ((quint16)p[1] << 8);
}

static quint32 read_uint32_le(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset > ((qint64)baData.size() - 4))) {
        return 0;
    }

    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);

    return (quint32)p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24);
}

static quint64 read_uint64_le(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset > ((qint64)baData.size() - 8))) {
        return 0;
    }

    quint64 nResult = 0;
    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);

    for (qint32 i = 7; i >= 0; i--) {
        nResult <<= 8;
        nResult |= p[i];
    }

    return nResult;
}

static bool isWimUtf16LEValid(const QByteArray &baData, qint64 nOffset, qint32 nSize)
{
    if ((nSize < 0) || ((nSize & 1) != 0) || (nOffset < 0) ||
        (nOffset > ((qint64)baData.size() - nSize))) return false;

    for (qint32 i = 0; i < nSize; i += 2) {
        const quint16 nCodeUnit = read_uint16_le(baData, nOffset + i);
        if (nCodeUnit == 0) return false;
        if ((nCodeUnit >= 0xD800) && (nCodeUnit <= 0xDBFF)) {
            if ((i + 2) >= nSize) return false;
            const quint16 nLow = read_uint16_le(baData, nOffset + i + 2);
            if ((nLow < 0xDC00) || (nLow > 0xDFFF)) return false;
            i += 2;
        } else if ((nCodeUnit >= 0xDC00) && (nCodeUnit <= 0xDFFF)) {
            return false;
        }
    }
    return true;
}

static bool isWimResourceExtentValid(const XWIM::RESOURCE_INFO &resourceInfo, qint64 nFileSize)
{
    if ((nFileSize < 0) || (resourceInfo.nOffset > (quint64)nFileSize) ||
        (resourceInfo.nPackSize > (quint64)nFileSize - resourceInfo.nOffset) ||
        (resourceInfo.nUnpackSize > (quint64)(std::numeric_limits<qint64>::max)())) {
        return false;
    }

    return true;
}

static bool isWimSameResource(const XWIM::RESOURCE_INFO &resourceInfo1,
                              const XWIM::RESOURCE_INFO &resourceInfo2)
{
    return (resourceInfo1.nPackSize == resourceInfo2.nPackSize) &&
           (resourceInfo1.nOffset == resourceInfo2.nOffset) &&
           (resourceInfo1.nUnpackSize == resourceInfo2.nUnpackSize) &&
           (resourceInfo1.nFlags == resourceInfo2.nFlags);
}

static bool insertWimResourceRange(QMap<quint64, quint64> *pRanges,
                                   quint64 nStart, quint64 nSize)
{
    if (!pRanges) return false;
    if (nSize == 0) return true;
    if (nStart > (std::numeric_limits<quint64>::max)() - nSize) return false;

    const quint64 nEnd = nStart + nSize;
    QMap<quint64, quint64>::const_iterator it = pRanges->lowerBound(nStart);
    if ((it != pRanges->constEnd()) && (nEnd > it.key())) return false;
    if (it != pRanges->constBegin()) {
        --it;
        if (it.value() > nStart) return false;
    }
    pRanges->insert(nStart, nEnd);
    return true;
}

static bool isWimResourceDescriptorSupported(
    const XWIM::RESOURCE_INFO &resourceInfo, quint32 nVersion,
    bool bLiveResource)
{
    static const quint8 WIM_RESOURCE_KNOWN_FLAGS = 0x1F;
    if (resourceInfo.nFlags & ~WIM_RESOURCE_KNOWN_FLAGS) return false;
    if ((resourceInfo.nFlags & 0x10) &&
        ((nVersion != 0x00000E00) || bLiveResource)) {
        return false;
    }
    if (!(resourceInfo.nFlags & 0x10) &&
        (((resourceInfo.nUnpackSize == 0) && (resourceInfo.nPackSize != 0)) ||
         ((resourceInfo.nUnpackSize != 0) && (resourceInfo.nPackSize == 0)))) {
        return false;
    }
    return true;
}

static XBinary::PM_INFO createPMInfo(XBinary::HANDLE_METHOD hm0, XBinary::HANDLE_METHOD hm1 = XBinary::HANDLE_METHOD_UNKNOWN,
                                     XBinary::HANDLE_METHOD hm2 = XBinary::HANDLE_METHOD_UNKNOWN, XBinary::HANDLE_METHOD hm3 = XBinary::HANDLE_METHOD_UNKNOWN)
{
    XBinary::PM_INFO result = {};

    result.hm[0] = hm0;
    result.hm[1] = hm1;
    result.hm[2] = hm2;
    result.hm[3] = hm3;

    return result;
}

XWIM::XWIM(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWIM::~XWIM()
{
}

bool XWIM::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    bool bResult = false;
    qint64 nSize = getSize();

    if (nSize >= WIM_HEADER_SIZE_OLD) {
        QByteArray baSignature = read_array(0, WIM_SIGNATURE_SIZE);

        if ((baSignature.size() == WIM_SIGNATURE_SIZE) && (baSignature == QByteArray("MSWIM\0\0\0", WIM_SIGNATURE_SIZE))) {
            quint32 nHeaderSize = read_uint32(8);
            quint32 nVersion = read_uint32(0x0C);
            const quint32 nFlags = read_uint32(0x10);
            const quint32 nChunkSize = read_uint32(0x14);

            if ((nHeaderSize >= WIM_HEADER_SIZE_OLD) && (nHeaderSize <= WIM_HEADER_SIZE_NEW) && ((qint64)nHeaderSize <= nSize) &&
                _isSupportedVersion(nVersion, nHeaderSize) &&
                _isCompressionConfigurationValid(nFlags, nChunkSize, false)) {
                if (nHeaderSize == WIM_HEADER_SIZE_OLD) {
                    bResult = true;
                } else {
                    quint16 nPartNumber = read_uint16(0x28);
                    quint16 nNumberOfParts = read_uint16(0x2A);
                    bResult = (nPartNumber != 0) && (nNumberOfParts != 0) && (nPartNumber <= nNumberOfParts);
                }
            }
        }
    }

    return bResult;
}

bool XWIM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWIM xwim(pDevice);

    return xwim.isValid(pPdStruct);
}

XBinary::FT XWIM::getFileType()
{
    return FT_WIM;
}

QString XWIM::getMIMEString()
{
    return "application/x-ms-wim";
}

QString XWIM::getFileFormatExt()
{
    return "wim";
}

QString XWIM::getFileFormatExtsString()
{
    return "WIM (*.wim *.swm *.esd *.ppkg)";
}

qint64 XWIM::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return getSize();
}

XBinary::MODE XWIM::getMode()
{
    return MODE_DATA;
}

qint32 XWIM::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XWIM::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::OSNAME XWIM::getOsName()
{
    return OSNAME_WINDOWS;
}

QString XWIM::getVersion()
{
    QString sResult;

    if (getSize() >= WIM_HEADER_SIZE_OLD) {
        quint32 nVersion = read_uint32(0x0C);
        sResult = QString("%1.%2.%3").arg((nVersion >> 16) & 0xFF).arg((nVersion >> 8) & 0xFF).arg(nVersion & 0xFF);
    }

    return sResult;
}

QString XWIM::getArch()
{
    return QString();
}

QList<XBinary::MAPMODE> XWIM::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XWIM::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    return _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
}

QString XWIM::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XWIM_STRUCTID, sizeof(_TABLE_XWIM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XWIM::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XWIM_STRUCTID, sizeof(_TABLE_XWIM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XWIM::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XWIM_STRUCTID, sizeof(_TABLE_XWIM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

static XBinary::XIDSTRING _TABLE_XWIM_HeaderFlags[] = {{XWIM::HEADER_FLAG_COMPRESSION, "COMPRESSION"},
                                                       {XWIM::HEADER_FLAG_READONLY, "READONLY"},
                                                       {XWIM::HEADER_FLAG_SPANNED, "SPANNED"},
                                                       {XWIM::HEADER_FLAG_RESOURCE_ONLY, "RESOURCE_ONLY"},
                                                       {XWIM::HEADER_FLAG_METADATA_ONLY, "METADATA_ONLY"},
                                                       {XWIM::HEADER_FLAG_WRITE_IN_PROGRESS, "WRITE_IN_PROGRESS"},
                                                       {XWIM::HEADER_FLAG_REPARSE_POINT_FIXUP, "REPARSE_POINT_FIXUP"},
                                                       {XWIM::HEADER_FLAG_XPRESS, "XPRESS"},
                                                       {XWIM::HEADER_FLAG_LZX, "LZX"},
                                                       {XWIM::HEADER_FLAG_LZMS, "LZMS"},
                                                       {XWIM::HEADER_FLAG_XPRESS2, "XPRESS2"}};

QList<XBinary::XFHEADER> XWIM::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_WIM_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_WIM_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        quint32 nHeaderSize = read_uint32(headerLoc.nLocation + 0x08);

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_WIM_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = nHeaderSize;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_WIM_HEADER, headerLoc);
        // Field 3 = Flags
        xfHeader.listDataSt.append({3, 0xFFFFFFFF, XFDATASTYPE_FLAGS, _TABLE_XWIM_HeaderFlags, sizeof(_TABLE_XWIM_HeaderFlags) / sizeof(XBinary::XIDSTRING)});
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_WIM_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

// Append the three fields (PackSize/Offset/UnpackSize) of one WIM reshdr_disk_short resource.
static void wimAddResourceRecords(QList<XBinary::XFRECORD> *pList, const QString &sName, qint32 nOffset)
{
    pList->append({sName + ".PackSize", nOffset, 8, XBinary::XFRECORD_FLAG_SIZE, XBinary::VT_UINT64});
    pList->append({sName + ".Offset", nOffset + 8, 8, XBinary::XFRECORD_FLAG_OFFSET, XBinary::VT_UINT64});
    pList->append({sName + ".UnpackSize", nOffset + 16, 8, XBinary::XFRECORD_FLAG_SIZE, XBinary::VT_UINT64});
}

QList<XBinary::XFRECORD> XWIM::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_WIM_HEADER) {
        listResult.append({"Signature", 0, 8, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"HeaderSize", 0x08, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"Version", 0x0C, 4, XFRECORD_FLAG_VERSION, VT_UINT32});
        listResult.append({"Flags", 0x10, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"ChunkSize", 0x14, 4, XFRECORD_FLAG_SIZE, VT_UINT32});

        quint32 nHeaderSize = read_uint32(xLoc.nLocation + 0x08);
        quint32 nVersion = read_uint32(xLoc.nLocation + 0x0C);

        if (nHeaderSize == WIM_HEADER_SIZE_OLD) {
            wimAddResourceRecords(&listResult, "OffsetTable", 0x18);
            wimAddResourceRecords(&listResult, "XmlData", 0x30);
            wimAddResourceRecords(&listResult, "BootMetadata", 0x48);
        } else if (nHeaderSize >= 0x74) {
            listResult.append({"Guid", 0x18, 16, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
            listResult.append({"PartNumber", 0x28, 2, XFRECORD_FLAG_COUNT, VT_UINT16});
            listResult.append({"NumberOfParts", 0x2A, 2, XFRECORD_FLAG_COUNT, VT_UINT16});

            qint32 nResourceOffset = 0x2C;
            const bool bIsNew = (nVersion == 0x00000E00) || (nVersion >= 0x00010D00);

            if (bIsNew) {
                listResult.append({"NumberOfImages", 0x2C, 4, XFRECORD_FLAG_COUNT, VT_UINT32});
                nResourceOffset = 0x30;
            }

            wimAddResourceRecords(&listResult, "OffsetTable", nResourceOffset);
            wimAddResourceRecords(&listResult, "XmlData", nResourceOffset + 0x18);
            wimAddResourceRecords(&listResult, "BootMetadata", nResourceOffset + 0x30);

            if (bIsNew) {
                listResult.append({"BootIndex", nResourceOffset + 0x48, 4, XFRECORD_FLAG_NONE, VT_UINT32});
                wimAddResourceRecords(&listResult, "Integrity", nResourceOffset + 0x4C);
            }
        }
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XWIM::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap ? dataHeadersOptions.pMemoryMap->fileType : getFileType();
//         _dataHeadersOptions.nID = STRUCTID_WIM_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else if (dataHeadersOptions.nID == STRUCTID_WIM_HEADER) {
//         qint64 nStartOffset = -1;

//         if (dataHeadersOptions.pMemoryMap) {
//             nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);
//         } else if (dataHeadersOptions.locType == LT_OFFSET) {
//             nStartOffset = dataHeadersOptions.nLocation;
//         }

//         if (nStartOffset != -1) {
//             quint32 nHeaderSize = read_uint32(nStartOffset + 8);

//             if ((nHeaderSize >= WIM_HEADER_SIZE_OLD) && (nHeaderSize <= WIM_HEADER_SIZE_NEW)) {
//                 ENDIAN endian = dataHeadersOptions.pMemoryMap ? dataHeadersOptions.pMemoryMap->endian : getEndian();
//                 quint32 nVersion = read_uint32(nStartOffset + 0x0C);
//                 bool bHasNewFields = (nVersion == 0x00000E00) || (nVersion >= 0x00010D00) || (nHeaderSize == WIM_HEADER_SIZE_NEW);
//                 DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XWIM::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = nHeaderSize;

//                 dataHeader.listRecords.append(getDataRecord(0x00, 8, "signature", VT_CHAR_ARRAY, DRF_UNKNOWN, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x08, 4, "headerSize", VT_UINT32, DRF_SIZE, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x0C, 4, "version", VT_UINT32, DRF_VERSION, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x10, 4, "flags", VT_UINT32, DRF_UNKNOWN, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x14, 4, "chunkSize", VT_UINT32, DRF_SIZE, endian));

//                 if (nHeaderSize > WIM_HEADER_SIZE_OLD) {
//                     dataHeader.listRecords.append(getDataRecord(0x18, 16, "guid", VT_BYTE_ARRAY, DRF_UNKNOWN, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x28, 2, "partNumber", VT_UINT16, DRF_UNKNOWN, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x2A, 2, "numberOfParts", VT_UINT16, DRF_COUNT, endian));

//                     if (bHasNewFields) {
//                         dataHeader.listRecords.append(getDataRecord(0x2C, 4, "numberOfImages", VT_UINT32, DRF_COUNT, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x30, WIM_RESOURCE_SIZE, "offsetTableResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x48, WIM_RESOURCE_SIZE, "xmlResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x60, WIM_RESOURCE_SIZE, "bootMetadataResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x78, 4, "bootIndex", VT_UINT32, DRF_UNKNOWN, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x7C, WIM_RESOURCE_SIZE, "integrityResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     } else {
//                         dataHeader.listRecords.append(getDataRecord(0x2C, WIM_RESOURCE_SIZE, "offsetTableResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x44, WIM_RESOURCE_SIZE, "xmlResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x5C, WIM_RESOURCE_SIZE, "bootMetadataResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     }
//                 } else {
//                     dataHeader.listRecords.append(getDataRecord(0x18, WIM_RESOURCE_SIZE, "offsetTableResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x30, WIM_RESOURCE_SIZE, "xmlResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x48, WIM_RESOURCE_SIZE, "bootMetadataResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                 }

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

static bool wimCanAppend(XBinary::PDSTRUCT *pPdStruct, qint32 nLimit, const QList<XBinary::FPART> *pListResult)
{
    return XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (pListResult->count() < nLimit));
}

bool XWIM::_processResourceFilePart(QList<FPART> *pListResult, quint32 nFileParts, qint32 nLimit, qint64 nFileSize, qint64 *pnMaxKnownEnd,
                                    QMap<quint64, QSet<quint64> > *pMapKnownResourceExtents, const RESOURCE_INFO &resourceInfo, const QString &sName,
                                    PDSTRUCT *pPdStruct)
{
    if (!isWimResourceExtentValid(resourceInfo, nFileSize)) return false;
    if (resourceInfo.nPackSize == 0) return true;

    const quint64 nEnd = resourceInfo.nOffset + resourceInfo.nPackSize;
    *pnMaxKnownEnd = qMax<qint64>(*pnMaxKnownEnd, (qint64)nEnd);

    QSet<quint64> &stSizes = (*pMapKnownResourceExtents)[resourceInfo.nOffset];
    if (stSizes.contains(resourceInfo.nPackSize)) return true;
    stSizes.insert(resourceInfo.nPackSize);

    if (wimCanAppend(pPdStruct, nLimit, pListResult)) {
        _appendResourcePart(pListResult, nFileParts, resourceInfo, sName, nLimit);
    }
    return true;
}

QList<XBinary::FPART> XWIM::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    qint64 nFileSize = getSize();

    if (nFileSize <= 0) {
        return listResult;
    }

    WIM_HEADER header = readWIMHeader();
    qint64 nHeaderSize = header.nHeaderSize;

    if ((nHeaderSize < WIM_HEADER_SIZE_OLD) || (nHeaderSize > WIM_HEADER_SIZE_NEW) || (nHeaderSize > nFileSize)) {
        nHeaderSize = qMin<qint64>(WIM_HEADER_SIZE_OLD, nFileSize);
    }

    qint64 nMaxKnownEnd = nHeaderSize;
    QMap<quint64, QSet<quint64> > mapKnownResourceExtents;

    if ((nFileParts & FILEPART_HEADER) && wimCanAppend(pPdStruct, nLimit, &listResult)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    _processResourceFilePart(&listResult, nFileParts, nLimit, nFileSize, &nMaxKnownEnd, &mapKnownResourceExtents, header.offsetTableResource, tr("Offset Table"),
                             pPdStruct);
    _processResourceFilePart(&listResult, nFileParts, nLimit, nFileSize, &nMaxKnownEnd, &mapKnownResourceExtents, header.xmlResource, tr("XML Data"), pPdStruct);
    _processResourceFilePart(&listResult, nFileParts, nLimit, nFileSize, &nMaxKnownEnd, &mapKnownResourceExtents, header.bootMetadataResource,
                             tr("Boot Metadata"), pPdStruct);
    _processResourceFilePart(&listResult, nFileParts, nLimit, nFileSize, &nMaxKnownEnd, &mapKnownResourceExtents, header.integrityResource, tr("Integrity"),
                             pPdStruct);

    // Lookup-table entries describe the remaining physical WIM resources.  Use
    // the same validated parser as extraction and expose each local extent at
    // most once; the boot-metadata header commonly aliases one of these entries.
    if (wimCanAppend(pPdStruct, nLimit, &listResult) && (nFileParts & (FILEPART_REGION | FILEPART_OVERLAY))) {
        bool bStreamListOk = false;
        const QList<STREAM_INFO> listStreams = _readStreamInfoList(header, &bStreamListOk, pPdStruct);

        if (bStreamListOk) {
            for (qint32 i = 0; (i < listStreams.count()) && wimCanAppend(pPdStruct, nLimit, &listResult); i++) {
                const STREAM_INFO &streamInfo = listStreams.at(i);
                if (streamInfo.nPartNumber != header.nPartNumber) continue;

                const QString sName = (streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA)
                                          ? tr("Metadata Resource %1").arg(i + 1)
                                          : tr("Data Resource %1").arg(i + 1);
                _processResourceFilePart(&listResult, nFileParts, nLimit, nFileSize, &nMaxKnownEnd, &mapKnownResourceExtents, streamInfo.resourceInfo, sName,
                                         pPdStruct);
            }
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && wimCanAppend(pPdStruct, nLimit, &listResult) && (nMaxKnownEnd < nFileSize)) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = nMaxKnownEnd;
        part.nFileSize = nFileSize - nMaxKnownEnd;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
    }

    return listResult;
}

QList<XBinary::PM_INFO> XWIM::unpackImplemented()
{
    QList<PM_INFO> listResult;

    listResult.append(createPMInfo(HANDLE_METHOD_STORE));
    listResult.append(createPMInfo(HANDLE_METHOD_LZX));
    listResult.append(createPMInfo(HANDLE_METHOD_XPRESS_HUFF));

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XWIM::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

static bool wimFailSource(QPointer<XWIM> *pGuardedArchive, XBinary::UNPACK_STATE *pState)
{
    if (*pGuardedArchive) (*pGuardedArchive)->releaseUnpackSource(pState);
    return false;
}

bool XWIM::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XWIM> guardedArchive(this);
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress) {
        return false;
    }

    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;
    SOURCE_DEVICE_SNAPSHOT sourceSnapshot = {};
    if (!guardedArchive->getBoundUnpackSourceSnapshot(pState, &sourceSnapshot)) {
        return wimFailSource(&guardedArchive, pState);
    }
    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) return false;
    if (!bValid) {
        return wimFailSource(&guardedArchive, pState);
    }

    WIM_HEADER header = guardedArchive->readWIMHeader();
    if (!guardedArchive) return false;
    // This class has no sibling-volume resolver.  Accepting a split WIM here
    // would make foreign-part resources look like extents in the current file.
    if ((header.nPartNumber != 1) || (header.nNumberOfParts != 1)) {
        return wimFailSource(&guardedArchive, pState);
    }
    const qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive) return false;
    if (!_isCompressionConfigurationValid(header.nFlags, header.nChunkSize, true) ||
        !isWimResourceExtentValid(header.offsetTableResource, nFileSize) ||
        !isWimResourceExtentValid(header.xmlResource, nFileSize) ||
        !isWimResourceExtentValid(header.bootMetadataResource, nFileSize) ||
        !isWimResourceExtentValid(header.integrityResource, nFileSize) ||
        !isWimResourceDescriptorSupported(header.offsetTableResource,
                                          header.nVersion, true) ||
        !isWimResourceDescriptorSupported(header.xmlResource,
                                          header.nVersion,
                                          header.xmlResource.nPackSize != 0) ||
        !isWimResourceDescriptorSupported(header.bootMetadataResource,
                                          header.nVersion,
                                          header.bootMetadataResource.nPackSize != 0) ||
        !isWimResourceDescriptorSupported(header.integrityResource,
                                          header.nVersion,
                                          header.integrityResource.nPackSize != 0)) {
        return wimFailSource(&guardedArchive, pState);
    }

    WIM_UNPACK_CONTEXT *pContext = new (std::nothrow) WIM_UNPACK_CONTEXT;
    if (!pContext) {
        return wimFailSource(&guardedArchive, pState);
    }
    pContext->sourceSnapshot = sourceSnapshot;
    pContext->bLegacy = (header.nHeaderSize == WIM_HEADER_SIZE_OLD);
    pContext->nHeaderFlags = header.nFlags;
    pContext->nChunkSize = header.nChunkSize;
    pContext->compressedHandleMethod = _getCompressionHandleMethod(header.nFlags);
    bool bStreamListOk = false;
    QList<STREAM_INFO> listStreams = guardedArchive->_readStreamInfoList(
        header, &bStreamListOk, pPdStruct);
    if (!guardedArchive) {
        delete pContext;
        return false;
    }
    QMap<QByteArray, STREAM_INFO> mapStreamsByHash;
    QMap<quint32, STREAM_INFO> mapStreamsById;
    QMap<QByteArray, quint64> mapActualHashRefCounts;
    QMap<quint32, quint64> mapActualIdRefCounts;
    const bool bLegacy = (header.nHeaderSize == WIM_HEADER_SIZE_OLD);

    for (qint32 i = 0; bStreamListOk && (i < listStreams.count()) &&
                          XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const STREAM_INFO &streamInfo = listStreams.at(i);
        if ((streamInfo.nRefCount != 0) &&
            (streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_SOLID)) {
            bStreamListOk = false;
            break;
        }
        if ((streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA) || (streamInfo.nRefCount == 0)) continue;
        if (bLegacy) {
            if ((streamInfo.nId == 0) || (streamInfo.baHash.size() != WIM_HASH_SIZE) ||
                mapStreamsById.contains(streamInfo.nId)) {
                bStreamListOk = false;
                break;
            }
            mapStreamsById.insert(streamInfo.nId, streamInfo);
        } else {
            if (_isEmptyHash(streamInfo.baHash) || mapStreamsByHash.contains(streamInfo.baHash)) {
                bStreamListOk = false;
                break;
            }
            mapStreamsByHash.insert(streamInfo.baHash, streamInfo);
        }
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) bStreamListOk = false;
    bool bMetadataFound = false;
    bool bMetadataParsed = false;
    QList<QList<WIM_RECORD> > listImages;
    QList<RESOURCE_INFO> listLiveMetadataResources;
    qint64 nPendingRecords = 0;

    for (qint32 i = 0; bStreamListOk && (i < listStreams.count()) &&
                          XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const STREAM_INFO &streamInfo = listStreams.at(i);

        if (streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA) {
            if (streamInfo.nRefCount == 0) continue;  // deleted image metadata
            bMetadataFound = true;
            if ((streamInfo.nRefCount != 1) || (listImages.count() >= WIM_MAX_IMAGES)) {
                bStreamListOk = false;
                break;
            }
            QByteArray baMetadata = guardedArchive->_readResource(
                streamInfo.resourceInfo, header.nFlags, header.nChunkSize,
                WIM_MAX_BUFFERED_RESOURCE_SIZE, pPdStruct);
            if (!guardedArchive) {
                delete pContext;
                return false;
            }
            QList<WIM_RECORD> listMetadataRecords;
            const QByteArray baMetadataDigest = wimSha1(baMetadata, pPdStruct);
            const bool bMetadataHashMatches = (streamInfo.baHash.size() == WIM_HASH_SIZE) &&
                                              (baMetadataDigest == streamInfo.baHash);

            if (!XBinary::isPdStructNotCanceled(pPdStruct) || baMetadata.isEmpty() ||
                (!bMetadataHashMatches && !(bLegacy && _isEmptyHash(streamInfo.baHash))) ||
                !guardedArchive->_parseMetadata(
                    baMetadata, mapStreamsByHash, mapStreamsById, header,
                    &listMetadataRecords, &mapActualHashRefCounts,
                    &mapActualIdRefCounts, pPdStruct)) {
                bStreamListOk = false;
                break;
            }
            if (!guardedArchive) {
                delete pContext;
                return false;
            }

            QSet<QString> stImageNames;
            for (qint32 nRecord = 0; (nRecord < listMetadataRecords.count()) &&
                                         XBinary::isPdStructNotCanceled(pPdStruct); nRecord++) {
                const QString &sName = listMetadataRecords.at(nRecord).sFileName;
                if (sName.isEmpty() || stImageNames.contains(sName)) {
                    bStreamListOk = false;
                    break;
                }
                stImageNames.insert(sName);
            }
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) bStreamListOk = false;
            if (!bStreamListOk) break;
            if (listMetadataRecords.count() > (WIM_MAX_RECORDS - nPendingRecords)) {
                bStreamListOk = false;
                break;
            }
            nPendingRecords += listMetadataRecords.count();
            listImages.append(listMetadataRecords);
            listLiveMetadataResources.append(streamInfo.resourceInfo);
        }
    }

    const bool bHasImageCount = (header.nVersion == 0x00000E00) ||
                                (header.nVersion >= 0x00010D00);
    if (bStreamListOk && bHasImageCount &&
        ((header.nNumberOfImages == 0) || (header.nNumberOfImages > WIM_MAX_IMAGES) ||
         (header.nNumberOfImages != (quint32)listImages.count()))) {
        bStreamListOk = false;
    }

    if (bStreamListOk) {
        if (header.bootMetadataResource.nUnpackSize == 0) {
            if ((header.bootMetadataResource.nPackSize != 0) ||
                (bHasImageCount && (header.nBootIndex != 0))) {
                bStreamListOk = false;
            }
        } else {
            qint32 nBootImage = -1;
            for (qint32 i = 0; (i < listLiveMetadataResources.count()) &&
                                  XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                if (isWimSameResource(header.bootMetadataResource,
                                      listLiveMetadataResources.at(i))) {
                    if (nBootImage != -1) {
                        bStreamListOk = false;
                        break;
                    }
                    nBootImage = i;
                }
            }
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) bStreamListOk = false;
            if (bStreamListOk && ((nBootImage < 0) ||
                                  (bHasImageCount &&
                                   (header.nBootIndex != (quint32)(nBootImage + 1))))) {
                bStreamListOk = false;
            }
        }
    }

    if (bStreamListOk) {
        for (qint32 i = 0; (i < listStreams.count()) &&
                              XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            const STREAM_INFO &streamInfo = listStreams.at(i);
            if (streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA) continue;
            const quint64 nActualRefCount = bLegacy
                                                ? mapActualIdRefCounts.value(streamInfo.nId, 0)
                                                : mapActualHashRefCounts.value(streamInfo.baHash, 0);
            if (nActualRefCount != streamInfo.nRefCount) {
                bStreamListOk = false;
                break;
            }
        }
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) bStreamListOk = false;
    }

    if (bStreamListOk && !listImages.isEmpty()) {
        const bool bUseImagePrefix = listImages.count() > 1;
        const qint32 nFirstImage = (header.nVersion == 0x00010900) ? 0 : 1;
        QSet<QString> stFinalNames;

        for (qint32 nImage = 0; bStreamListOk && (nImage < listImages.count()) &&
                                     XBinary::isPdStructNotCanceled(pPdStruct); nImage++) {
            QList<WIM_RECORD> listMetadataRecords = listImages.at(nImage);
            const QString sPrefix = bUseImagePrefix
                                        ? QStringLiteral("image_%1/").arg(nFirstImage + nImage)
                                        : QString();
            for (qint32 nRecord = 0; (nRecord < listMetadataRecords.count()) &&
                                         XBinary::isPdStructNotCanceled(pPdStruct); nRecord++) {
                WIM_RECORD record = listMetadataRecords.at(nRecord);
                record.sFileName.prepend(sPrefix);
                if ((record.handleMethod == HANDLE_METHOD_UNKNOWN) &&
                    (record.resourceInfo.nFlags & RESOURCE_FLAG_COMPRESSED) &&
                    !(record.resourceInfo.nFlags & RESOURCE_FLAG_SOLID)) {
                    record.handleMethod = pContext->compressedHandleMethod;
                }
                if ((record.sFileName.size() > WIM_MAX_PATH_LENGTH) ||
                    stFinalNames.contains(record.sFileName) ||
                    (pContext->listRecords.count() >= WIM_MAX_RECORDS)) {
                    bStreamListOk = false;
                    break;
                }
                stFinalNames.insert(record.sFileName);
                pContext->listRecords.append(record);
            }
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) bStreamListOk = false;
        }
        bMetadataParsed = bStreamListOk;
    }

    if (bStreamListOk && bMetadataFound && bMetadataParsed &&
        XBinary::isPdStructNotCanceled(pPdStruct) &&
        guardedArchive->isSourceDeviceSnapshotCurrent(
            pContext->sourceSnapshot, guardedArchive->getDevice(),
            pPdStruct)) {
        if (!guardedArchive) return false;
        pState->nCurrentOffset = 0;
        pState->nTotalSize = nFileSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = pContext->listRecords.count();
        pState->mapUnpackProperties = mapProperties;
        pState->pContext = pContext;
        bResult = guardedArchive->validateAndFinalizeUnpackSource(
            pState, pContext, pPdStruct);
        if (!guardedArchive) return false;
        if (!bResult) {
            pState->pContext = nullptr;
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            pState->nCurrentOffset = 0;
            pState->nTotalSize = 0;
            pState->nCurrentIndex = 0;
            pState->nNumberOfRecords = 0;
            pState->mapUnpackProperties.clear();
            pState->mapArchiveProperties.clear();
        }
    } else {
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XWIM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return result;
    QPointer<XWIM> guardedArchive(this);

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    WIM_UNPACK_CONTEXT *pContext = (WIM_UNPACK_CONTEXT *)pState->pContext;

    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedArchive || !guardedSource ||
        !guardedArchive->isSourceDeviceSnapshotCurrent(
            pContext->sourceSnapshot, guardedSource.data(), pPdStruct) ||
        !guardedArchive || !guardedSource) {
        return result;
    }

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const WIM_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nStreamOffset;
    result.nStreamSize = record.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, record.handleMethod);

    if (record.bIsFolder) {
        result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    }

    if (record.mtDateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, record.mtDateTime);
    }

    if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive || !guardedSource) {
        return ARCHIVERECORD();
    }
    return result;
}

bool XWIM::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XWIM> guardedArchive(this);

    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext) return false;

    WIM_UNPACK_CONTEXT *pContext = (WIM_UNPACK_CONTEXT *)pState->pContext;

    // The parsed context belongs to the source device that was active during
    // initUnpack().  Reject a later setDevice() replacement before touching
    // either iterator state or the caller's output device.
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedArchive || !guardedSource ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !guardedArchive->isSourceDeviceSnapshotCurrent(
            pContext->sourceSnapshot, guardedSource.data(), pPdStruct) ||
        !guardedArchive || !guardedSource) return false;

    // Archive payloads are byte streams.  Text mode can transparently
    // translate CR/LF on Windows while still reporting successful writes.
    if (!guardedOutput ||
        !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) ||
        !guardedArchive) {
        return false;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return false;
    }

    const WIM_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);
    if (!guardedOutput || wimDevicesAlias(
            pContext->sourceSnapshot.pSourceDevice.data(),
            guardedOutput.data())) return false;

    if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !guardedArchive->isSourceDeviceSnapshotCurrent(
            pContext->sourceSnapshot, guardedSource.data(), pPdStruct) ||
        !guardedArchive || !guardedSource) {
        return false;
    }

    if ((record.nUncompressedSize < 0) ||
        (record.resourceInfo.nUnpackSize >
         (quint64)(std::numeric_limits<qint64>::max)()) ||
        !XBinary::isUnpackOutputSizeAllowed(
            pState->mapUnpackProperties, record.nUncompressedSize) ||
        !XBinary::isUnpackOutputSizeAllowed(
            pState->mapUnpackProperties,
            (qint64)record.resourceInfo.nUnpackSize)) {
        return false;
    }

    // This override bypasses the base decode chain: the resource is
    // reassembled chunk-wise into a private stage and published as one
    // copy.  Account the member and its exact produced size here; the
    // publish copy itself is never charged.
    if (!record.bIsFolder && pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, record.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(record.nUncompressedSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    if (record.nUncompressedSize == 0) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            (record.resourceInfo.nUnpackSize != 0) || (record.resourceInfo.nPackSize != 0) ||
            (record.resourceInfo.nFlags & RESOURCE_FLAG_SOLID) ||
            !isWimResourceExtentValid(record.resourceInfo,
                                      guardedArchive->getSize()) ||
            !guardedArchive ||
            (!record.baHash.isEmpty() && (record.baHash.size() != WIM_HASH_SIZE)) ||
             ((record.baHash.size() == WIM_HASH_SIZE) && !_isEmptyHash(record.baHash) &&
              (wimSha1(QByteArray(), pPdStruct) != record.baHash))) {
            return false;
        }
        if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
            !guardedArchive->isSourceDeviceSnapshotCurrent(
                pContext->sourceSnapshot, guardedSource.data(), pPdStruct) ||
            !guardedArchive || !guardedSource || !guardedOutput) {
            return false;
        }
        QBuffer emptyStage;
        return emptyStage.open(QIODevice::ReadWrite) &&
               guardedArchive->publishUnpackOutput(
                   &emptyStage, guardedOutput.data(), pState, pPdStruct);
    }

    QTemporaryFile stageFile;
    if (!stageFile.open()) {
        return false;
    }

    QByteArray baDigest;
    const bool bDigestRequired = !(pContext->bLegacy && _isEmptyHash(record.baHash));
    if (!guardedArchive->_stageResource(
            record, *pContext, &stageFile, &baDigest, pPdStruct) ||
        !guardedArchive || !guardedSource || !guardedOutput ||
        (bDigestRequired && (baDigest != record.baHash)) ||
        (stageFile.size() != record.nUncompressedSize) ||
        !stageFile.seek(0)) {
        return false;
    }
    if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !guardedArchive->isSourceDeviceSnapshotCurrent(
            pContext->sourceSnapshot, guardedSource.data(), pPdStruct) ||
        !guardedArchive || !guardedSource || !guardedOutput) {
        return false;
    }

    if (!guardedArchive->publishUnpackOutput(
            &stageFile, guardedOutput.data(), pState, pPdStruct) ||
        !guardedArchive) return false;

    pState->nCurrentOffset = record.nUncompressedSize;
    return true;
}

bool XWIM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XWIM> guardedArchive(this);

    if (XBinary::isPdStructNotCanceled(pPdStruct) && pState && pState->pContext && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        WIM_UNPACK_CONTEXT *pContext = (WIM_UNPACK_CONTEXT *)pState->pContext;
        QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
        if (!guardedArchive || !guardedSource ||
            !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
            !guardedArchive->isSourceDeviceSnapshotCurrent(
                pContext->sourceSnapshot, guardedSource.data(), pPdStruct) ||
            !guardedArchive || !guardedSource) {
            return false;
        }

        pState->nCurrentIndex++;
        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XWIM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XWIM> guardedArchive(this);

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) return false;
    guardedArchive->releaseUnpackSource(pState);

    if (pState->pContext) {
        WIM_UNPACK_CONTEXT *pContext = (WIM_UNPACK_CONTEXT *)pState->pContext;
        pState->pContext = nullptr;
        delete pContext;
        if (!guardedArchive) return false;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QList<XBinary::FPART_PROP> XWIM::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_ISFOLDER);
    listResult.append(FPART_PROP_DATETIME);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}

XWIM::WIM_HEADER XWIM::readWIMHeader(qint64 nOffset)
{
    WIM_HEADER result = {};

    const qint64 nFileSize = getSize();
    if ((nOffset < 0) || (nFileSize < WIM_HEADER_SIZE_OLD) ||
        (nOffset > (nFileSize - WIM_HEADER_SIZE_OLD))) {
        return result;
    }

    result.nHeaderSize = read_uint32(nOffset + 0x08);
    result.nVersion = read_uint32(nOffset + 0x0C);
    result.nFlags = read_uint32(nOffset + 0x10);
    result.nChunkSize = read_uint32(nOffset + 0x14);

    if ((result.nHeaderSize < WIM_HEADER_SIZE_OLD) ||
        (result.nHeaderSize > WIM_HEADER_SIZE_NEW) ||
        ((quint64)result.nHeaderSize > (quint64)(nFileSize - nOffset)) ||
        !_isSupportedVersion(result.nVersion, result.nHeaderSize) ||
        !_isCompressionConfigurationValid(result.nFlags, result.nChunkSize, false)) {
        return WIM_HEADER();
    }

    if (result.nHeaderSize == WIM_HEADER_SIZE_OLD) {
        result.nPartNumber = 1;
        result.nNumberOfParts = 1;
        result.offsetTableResource = readResourceInfo(nOffset + 0x18);
        result.xmlResource = readResourceInfo(nOffset + 0x30);
        result.bootMetadataResource = readResourceInfo(nOffset + 0x48);
    } else if (result.nHeaderSize >= 0x74) {
        result.baGuid = read_array(nOffset + 0x18, 16);
        result.nPartNumber = read_uint16(nOffset + 0x28);
        result.nNumberOfParts = read_uint16(nOffset + 0x2A);

        qint64 nResourceOffset = nOffset + 0x2C;

        const bool bIsNew = (result.nVersion == 0x00000E00) || (result.nVersion >= 0x00010D00);
        if (bIsNew) {
            result.nNumberOfImages = read_uint32(nOffset + 0x2C);
            nResourceOffset = nOffset + 0x30;
        }

        result.offsetTableResource = readResourceInfo(nResourceOffset);
        result.xmlResource = readResourceInfo(nResourceOffset + 0x18);
        result.bootMetadataResource = readResourceInfo(nResourceOffset + 0x30);

        if (bIsNew) {
            result.nBootIndex = read_uint32(nResourceOffset + 0x48);
            result.integrityResource = readResourceInfo(nResourceOffset + 0x4C);
        }
    }

    return result;
}

QVector<XBinary::XMETADATA_STRUCT> XWIM::getMetadataStructs()
{
    QVector<XMETADATA_STRUCT> listResult = XArchive::getMetadataStructs();
    const WIM_HEADER header = readWIMHeader();

    if ((header.baGuid.size() == 16) && (header.baGuid != QByteArray(16, '\0')) && checkOffsetSize(0x18, 16)) {
        XMETADATA_STRUCT record = {};
        record.nOffset = 0x18;
        record.nSize = 16;
        record.nAddress = offsetToAddress(0x18);
        record.id = XMETADATA_ID_UUID;
        record.sName = QString("WIM GUID");
        record.varValue = read_UUID(0x18);
        listResult.prepend(record);
    }

    return listResult;
}

XWIM::RESOURCE_INFO XWIM::readResourceInfo(qint64 nOffset)
{
    RESOURCE_INFO result = {};

    const qint64 nFileSize = getSize();
    if ((nOffset >= 0) && (nFileSize >= WIM_RESOURCE_SIZE) &&
        (nOffset <= (nFileSize - WIM_RESOURCE_SIZE))) {
        quint64 nPackSizeAndFlags = read_uint64(nOffset);
        result.nFlags = (quint8)(nPackSizeAndFlags >> 56);
        result.nPackSize = nPackSizeAndFlags & 0x00FFFFFFFFFFFFFFULL;
        result.nOffset = read_uint64(nOffset + 8);
        result.nUnpackSize = read_uint64(nOffset + 16);
    }

    return result;
}

QString XWIM::compressionMethodToString()
{
    QString sResult = "Store";
    quint32 nFlags = read_uint32(0x10);

    if (nFlags & HEADER_FLAG_COMPRESSION) {
        if (nFlags & HEADER_FLAG_LZX) {
            sResult = "LZX";
        } else if (nFlags & HEADER_FLAG_LZMS) {
            sResult = "LZMS";
        } else if ((nFlags & HEADER_FLAG_XPRESS) || (nFlags & HEADER_FLAG_XPRESS2)) {
            sResult = "XPRESS";
        } else {
            sResult = tr("Compressed");
        }
    }

    return sResult;
}

bool XWIM::_isSupportedVersion(quint32 nVersion, quint32 nHeaderSize) const
{
    bool bResult = false;

    if ((nVersion == 0x00000E00) && (nHeaderSize == WIM_HEADER_SIZE_NEW)) {
        bResult = true;
    } else if ((nVersion >= 0x00010900) && (nVersion <= 0x00020E00)) {
        if (nVersion <= 0x00010A00) {
            bResult = (nHeaderSize == WIM_HEADER_SIZE_OLD);
        } else if (nVersion == 0x00010B00) {
            bResult = (nHeaderSize == WIM_HEADER_SIZE_OLD) ||
                      ((nHeaderSize >= 0x74) && (nHeaderSize <= WIM_HEADER_SIZE_NEW));
        } else if (nVersion == 0x00010C00) {
            bResult = (nHeaderSize >= 0x74) && (nHeaderSize <= WIM_HEADER_SIZE_NEW);
        } else {
            bResult = (nHeaderSize == WIM_HEADER_SIZE_NEW);
        }
    }

    return bResult;
}

bool XWIM::_isCompressionConfigurationValid(quint32 nHeaderFlags, quint32 nChunkSize,
                                             bool bRequireImplemented) const
{
    if ((nChunkSize != 0) &&
        ((nChunkSize < 4096) || (nChunkSize > WIM_MAX_CHUNK_SIZE) ||
         ((nChunkSize & (nChunkSize - 1)) != 0))) {
        return false;
    }

    const quint32 nMethodMask = nHeaderFlags & 0xFFFE0000U;
    if (!(nHeaderFlags & HEADER_FLAG_COMPRESSION)) {
        return nMethodMask == 0;
    }

    if ((nMethodMask != HEADER_FLAG_XPRESS) && (nMethodMask != HEADER_FLAG_XPRESS2) &&
        (nMethodMask != HEADER_FLAG_LZX) && (nMethodMask != HEADER_FLAG_LZMS)) {
        return false;
    }

    if (!bRequireImplemented) return true;
    if (nMethodMask == HEADER_FLAG_LZMS) return false;

    const quint32 nEffectiveChunkSize = nChunkSize ? nChunkSize : 32768U;
    if ((nMethodMask == HEADER_FLAG_LZX) &&
        ((nEffectiveChunkSize < (1U << 15)) || (nEffectiveChunkSize > (1U << 21)))) return false;
    if (((nMethodMask == HEADER_FLAG_XPRESS) || (nMethodMask == HEADER_FLAG_XPRESS2)) &&
        (nEffectiveChunkSize > WIM_MAX_XPRESS_CHUNK_SIZE)) return false;
    return true;
}

void XWIM::_appendResourcePart(QList<FPART> *pListResult, quint32 nFileParts, const RESOURCE_INFO &resourceInfo, const QString &sName, qint32 nLimit)
{
    if (!pListResult) {
        return;
    }

    if (!(nFileParts & FILEPART_REGION)) {
        return;
    }

    if ((nLimit != -1) && (pListResult->count() >= nLimit)) {
        return;
    }

    qint64 nFileSize = getSize();

    if ((resourceInfo.nPackSize > 0) && (resourceInfo.nOffset < (quint64)nFileSize) &&
        (resourceInfo.nUnpackSize <= (quint64)(std::numeric_limits<qint64>::max)())) {
        const quint64 nAvailable = (quint64)nFileSize - resourceInfo.nOffset;
        if (resourceInfo.nPackSize > nAvailable) {
            return;
        }
        qint64 nPartSize = (qint64)resourceInfo.nPackSize;

        if (nPartSize > 0) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = (qint64)resourceInfo.nOffset;
            part.nFileSize = nPartSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = sName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nPartSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)resourceInfo.nUnpackSize);

            pListResult->append(part);
        }
    }
}

bool XWIM::_isResourceStored(const RESOURCE_INFO &resourceInfo) const
{
    return ((resourceInfo.nFlags & (RESOURCE_FLAG_COMPRESSED | RESOURCE_FLAG_SOLID)) == 0) && (resourceInfo.nPackSize == resourceInfo.nUnpackSize);
}

QByteArray XWIM::_readStoredResource(const RESOURCE_INFO &resourceInfo, PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    if (!_isResourceStored(resourceInfo) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return baResult;
    }

    if (resourceInfo.nPackSize == 0) return baResult;
    if ((resourceInfo.nPackSize > WIM_MAX_BUFFERED_RESOURCE_SIZE) ||
        (resourceInfo.nPackSize > (quint64)INT_MAX)) {
        return baResult;
    }

    if ((resourceInfo.nOffset >= (quint64)getSize()) || (resourceInfo.nPackSize > (quint64)getSize() - resourceInfo.nOffset)) {
        return baResult;
    }

    baResult.resize((qint32)resourceInfo.nPackSize);
    if (read_array_process((qint64)resourceInfo.nOffset, baResult.data(), baResult.size(), pPdStruct) !=
            baResult.size() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        baResult.clear();
    }

    return baResult;
}

XWIM::WIM_COMPRESSION XWIM::_getCompressionType(quint32 nHeaderFlags) const
{
    if (!(nHeaderFlags & HEADER_FLAG_COMPRESSION)) {
        return WIM_COMPRESSION_NONE;
    }

    if (nHeaderFlags & HEADER_FLAG_LZX) {
        return WIM_COMPRESSION_LZX;
    } else if (nHeaderFlags & HEADER_FLAG_LZMS) {
        return WIM_COMPRESSION_LZMS;
    } else if ((nHeaderFlags & HEADER_FLAG_XPRESS) || (nHeaderFlags & HEADER_FLAG_XPRESS2)) {
        return WIM_COMPRESSION_XPRESS;
    }

    return WIM_COMPRESSION_NONE;
}

XBinary::HANDLE_METHOD XWIM::_getCompressionHandleMethod(quint32 nHeaderFlags) const
{
    const WIM_COMPRESSION compression = _getCompressionType(nHeaderFlags);

    if (compression == WIM_COMPRESSION_LZX) return HANDLE_METHOD_LZX;
    if (compression == WIM_COMPRESSION_XPRESS) return HANDLE_METHOD_XPRESS_HUFF;
    return HANDLE_METHOD_UNKNOWN;
}

qint32 XWIM::_getChunkSize(quint32 nChunkSize) const
{
    // WIM v1 uses a fixed 32768-byte chunk; the header field is normally 0 there.
    if (nChunkSize == 0) {
        return 32768;
    }

    if ((nChunkSize < 4096) || (nChunkSize > WIM_MAX_CHUNK_SIZE) ||
        ((nChunkSize & (nChunkSize - 1)) != 0) ||
        (nChunkSize > (quint32)(std::numeric_limits<qint32>::max)())) return 0;
    return (qint32)nChunkSize;
}

QByteArray XWIM::_readResource(const RESOURCE_INFO &resourceInfo, quint32 nHeaderFlags, quint32 nChunkSize,
                               quint64 nMaxBufferSize, PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
        !isWimResourceExtentValid(resourceInfo, getSize()) || (nMaxBufferSize == 0) ||
        (resourceInfo.nPackSize > nMaxBufferSize) ||
        (resourceInfo.nUnpackSize > nMaxBufferSize)) return QByteArray();

    // Equal packed and unpacked sizes do not override the resource flags: a
    // compressed resource still has a chunk table and must be decoded.
    if (_isResourceStored(resourceInfo)) return _readStoredResource(resourceInfo, pPdStruct);

    if (!(resourceInfo.nFlags & RESOURCE_FLAG_COMPRESSED) ||
        (resourceInfo.nFlags & RESOURCE_FLAG_SOLID)) return QByteArray();

    WIM_COMPRESSION compression = _getCompressionType(nHeaderFlags);

    if ((compression == WIM_COMPRESSION_LZX) || (compression == WIM_COMPRESSION_XPRESS)) {
        return _decompressChunkedResource(resourceInfo, compression, _getChunkSize(nChunkSize), pPdStruct);
    }

    // LZMS and unknown compression are not yet supported
    return QByteArray();
}

QByteArray XWIM::_decompressChunkedResource(const RESOURCE_INFO &resourceInfo, WIM_COMPRESSION compression, qint32 nChunkSize, PDSTRUCT *pPdStruct)
{
    QByteArray baResult;
    if ((resourceInfo.nUnpackSize == 0) ||
        (resourceInfo.nUnpackSize > WIM_MAX_BUFFERED_RESOURCE_SIZE) ||
        (resourceInfo.nUnpackSize > (quint64)INT_MAX)) return baResult;

    QBuffer buffer(&baResult);
    if (!buffer.open(QIODevice::WriteOnly) ||
        !_stageChunkedResource(resourceInfo, compression, nChunkSize, &buffer, nullptr, pPdStruct) ||
        ((quint64)baResult.size() != resourceInfo.nUnpackSize)) {
        baResult.clear();
    }
    return baResult;
}

bool XWIM::_stageChunkedResource(const RESOURCE_INFO &resourceInfo, WIM_COMPRESSION compression,
                                 qint32 nChunkSize, QIODevice *pStageDevice,
                                 QCryptographicHash *pHash, PDSTRUCT *pPdStruct)
{
    const quint64 nUnpackSize = resourceInfo.nUnpackSize;
    const quint64 nPackSize = resourceInfo.nPackSize;
    if (!pStageDevice || !pStageDevice->isOpen() || !pStageDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (nChunkSize <= 0) ||
        (compression == WIM_COMPRESSION_NONE) || (compression == WIM_COMPRESSION_LZMS) ||
        (nUnpackSize == 0) || (nUnpackSize > (quint64)(std::numeric_limits<qint64>::max)()) ||
        (nPackSize == 0) || !isWimResourceExtentValid(resourceInfo, getSize())) {
        return false;
    }
    if ((compression == WIM_COMPRESSION_LZX) && (nChunkSize > (1 << 21))) return false;
    if ((compression == WIM_COMPRESSION_XPRESS) &&
        ((quint32)nChunkSize > WIM_MAX_XPRESS_CHUNK_SIZE)) return false;

    qint32 nWindowBits = 0;
    for (quint32 nValue = (quint32)nChunkSize; nValue > 1; nValue >>= 1) nWindowBits++;

    const quint64 nNumChunks = 1 + ((nUnpackSize - 1) / (quint64)nChunkSize);
    const quint32 nEntrySize = (nUnpackSize > 0xFFFFFFFFULL) ? 8 : 4;
    const quint64 nTableEntries = nNumChunks - 1;
    if (nTableEntries > ((quint64)(std::numeric_limits<qint64>::max)() / nEntrySize)) return false;
    const quint64 nTableSize = nTableEntries * nEntrySize;
    if (nTableSize >= nPackSize) return false;

    const quint64 nCompressedTotal = nPackSize - nTableSize;
    quint64 nChunkStart = 0;
    quint64 nOutputDone = 0;
    QByteArray baEntry((qint32)nEntrySize, 0);

    for (quint64 i = 0; (i < nNumChunks) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        quint64 nChunkEnd = nCompressedTotal;
        if (i + 1 < nNumChunks) {
            const quint64 nEntryOffset = resourceInfo.nOffset + i * nEntrySize;
            if ((nEntryOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
                (read_array_process((qint64)nEntryOffset, baEntry.data(), baEntry.size(), pPdStruct) != baEntry.size())) {
                return false;
            }
            nChunkEnd = (nEntrySize == 4) ? read_uint32_le(baEntry, 0) : read_uint64_le(baEntry, 0);
        }
        if ((nChunkEnd <= nChunkStart) || (nChunkEnd > nCompressedTotal)) return false;

        const quint64 nChunkCompressed64 = nChunkEnd - nChunkStart;
        const qint32 nChunkUncompressed = (qint32)qMin<quint64>((quint64)nChunkSize,
                                                                nUnpackSize - nOutputDone);
        if ((nChunkCompressed64 > (quint64)INT_MAX) ||
            (nChunkCompressed64 > (quint64)nChunkSize)) return false;
        const qint32 nChunkCompressed = (qint32)nChunkCompressed64;
        QByteArray baChunk(nChunkCompressed, 0);
        const quint64 nDataOffset = resourceInfo.nOffset + nTableSize + nChunkStart;
        if ((nDataOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
            (read_array_process((qint64)nDataOffset, baChunk.data(), baChunk.size(), pPdStruct) != baChunk.size())) {
            return false;
        }

        QByteArray baChunkOut;
        const QByteArray *pOutput = &baChunk;
        if (nChunkCompressed != nChunkUncompressed) {
            if (nChunkCompressed >= nChunkSize) return false;
            const bool bDecoded = (compression == WIM_COMPRESSION_LZX)
                                      ? XLZXDecoder::decompressWIMChunk(baChunk, &baChunkOut,
                                                                        nChunkUncompressed, nWindowBits, pPdStruct)
                                      : XXPressDecoder::decompressHuffman(baChunk, &baChunkOut,
                                                                          nChunkUncompressed, pPdStruct);
            if (!bDecoded || (baChunkOut.size() != nChunkUncompressed)) return false;
            pOutput = &baChunkOut;
        }

        if (pHash) pHash->addData(*pOutput);
        if (!wimWriteAll(pStageDevice, pOutput->constData(), pOutput->size(), pPdStruct)) return false;
        nOutputDone += (quint64)pOutput->size();
        nChunkStart = nChunkEnd;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct) &&
           (nChunkStart == nCompressedTotal) && (nOutputDone == nUnpackSize);
}

bool XWIM::_stageResource(const WIM_RECORD &record, const WIM_UNPACK_CONTEXT &context,
                          QIODevice *pStageDevice, QByteArray *pDigest, PDSTRUCT *pPdStruct)
{
    if (pDigest) pDigest->clear();
    if (!pStageDevice || !pDigest || !pStageDevice->isOpen() || !pStageDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (record.nUncompressedSize <= 0) ||
        ((quint64)record.nUncompressedSize != record.resourceInfo.nUnpackSize) ||
        (record.baHash.size() != WIM_HASH_SIZE) ||
        (!context.bLegacy && _isEmptyHash(record.baHash)) ||
        !isWimResourceExtentValid(record.resourceInfo, getSize())) return false;

    QCryptographicHash hash(QCryptographicHash::Sha1);
    bool bResult = false;
    if (_isResourceStored(record.resourceInfo)) {
        QByteArray baBuffer(1 << 20, 0);
        quint64 nDone = 0;
        while ((nDone < record.resourceInfo.nUnpackSize) &&
               XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nChunk = (qint32)qMin<quint64>((quint64)baBuffer.size(),
                                                        record.resourceInfo.nUnpackSize - nDone);
            const quint64 nReadOffset = record.resourceInfo.nOffset + nDone;
            if ((nReadOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
                (read_array_process((qint64)nReadOffset, baBuffer.data(), nChunk, pPdStruct) != nChunk)) {
                return false;
            }
            hash.addData(baBuffer.constData(), nChunk);
            if (!wimWriteAll(pStageDevice, baBuffer.constData(), nChunk, pPdStruct)) return false;
            nDone += nChunk;
        }
        bResult = (nDone == record.resourceInfo.nUnpackSize);
    } else if ((record.resourceInfo.nFlags & RESOURCE_FLAG_COMPRESSED) &&
               !(record.resourceInfo.nFlags & RESOURCE_FLAG_SOLID)) {
        const WIM_COMPRESSION compression = _getCompressionType(context.nHeaderFlags);
        bResult = _stageChunkedResource(record.resourceInfo, compression,
                                        _getChunkSize(context.nChunkSize), pStageDevice,
                                        &hash, pPdStruct);
    }

    if (!bResult || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pDigest = hash.result();
    return pStageDevice->pos() == record.nUncompressedSize;
}

QList<XWIM::STREAM_INFO> XWIM::_readStreamInfoList(const WIM_HEADER &header, bool *pOk, PDSTRUCT *pPdStruct)
{
    QList<STREAM_INFO> listResult;
    if (pOk) *pOk = false;
    if (!pOk || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (header.nPartNumber == 0) || (header.nNumberOfParts == 0) ||
        (header.nPartNumber > header.nNumberOfParts) ||
        !isWimResourceExtentValid(header.offsetTableResource, getSize()) ||
        !isWimResourceExtentValid(header.xmlResource, getSize()) ||
        !isWimResourceExtentValid(header.bootMetadataResource, getSize()) ||
        !isWimResourceExtentValid(header.integrityResource, getSize()) ||
        !isWimResourceDescriptorSupported(header.offsetTableResource,
                                          header.nVersion, true) ||
        !isWimResourceDescriptorSupported(header.xmlResource,
                                          header.nVersion,
                                          header.xmlResource.nPackSize != 0) ||
        !isWimResourceDescriptorSupported(header.bootMetadataResource,
                                          header.nVersion,
                                          header.bootMetadataResource.nPackSize != 0) ||
        !isWimResourceDescriptorSupported(header.integrityResource,
                                          header.nVersion,
                                          header.integrityResource.nPackSize != 0)) {
        return listResult;
    }

    // The boot descriptor is intentionally omitted: when present it must
    // exactly alias one live metadata entry and is checked after metadata is
    // parsed.  Every other control resource and retained lookup descriptor
    // owns a unique physical range outside the header.
    QMap<quint64, quint64> mapResourceRanges;
    if (!insertWimResourceRange(&mapResourceRanges, 0, header.nHeaderSize) ||
        !insertWimResourceRange(&mapResourceRanges,
                                header.offsetTableResource.nOffset,
                                header.offsetTableResource.nPackSize) ||
        !insertWimResourceRange(&mapResourceRanges,
                                header.xmlResource.nOffset,
                                header.xmlResource.nPackSize) ||
        !insertWimResourceRange(&mapResourceRanges,
                                header.integrityResource.nOffset,
                                header.integrityResource.nPackSize)) {
        return listResult;
    }

    const bool bLegacy = (header.nHeaderSize == WIM_HEADER_SIZE_OLD);
    const qint32 nStreamInfoSize = bLegacy ? WIM_STREAM_INFO_SIZE_OLD : WIM_STREAM_INFO_SIZE;
    const quint64 nMaxOffsetTableSize =
        (quint64)(WIM_MAX_RECORDS + WIM_MAX_IMAGES) * (quint64)nStreamInfoSize;
    if ((header.offsetTableResource.nUnpackSize == 0) ||
        (header.offsetTableResource.nUnpackSize > nMaxOffsetTableSize) ||
        ((header.offsetTableResource.nUnpackSize % (quint64)nStreamInfoSize) != 0)) {
        return listResult;
    }
    const quint64 nOffsetTableBufferLimit =
        nMaxOffsetTableSize + ((nMaxOffsetTableSize + 4095) / 4096) * 8;
    QByteArray baOffsetTable = _readResource(header.offsetTableResource, header.nFlags, header.nChunkSize,
                                             nOffsetTableBufferLimit, pPdStruct);
    if (baOffsetTable.isEmpty() || ((baOffsetTable.size() % nStreamInfoSize) != 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return listResult;
    const qint32 nNumberOfStreams = baOffsetTable.size() / nStreamInfoSize;
    if ((nNumberOfStreams <= 0) || (nNumberOfStreams > (WIM_MAX_RECORDS + WIM_MAX_IMAGES))) return listResult;

    QSet<quint32> stLegacyIds;
    QSet<QByteArray> stModernHashes;

    for (qint32 i = 0; (i < nNumberOfStreams) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const qint64 nOffset = (qint64)i * nStreamInfoSize;
        STREAM_INFO streamInfo = {};
        const quint64 nPackSizeAndFlags = read_uint64_le(baOffsetTable, nOffset);

        streamInfo.resourceInfo.nFlags = (quint8)(nPackSizeAndFlags >> 56);
        streamInfo.resourceInfo.nPackSize = nPackSizeAndFlags & 0x00FFFFFFFFFFFFFFULL;
        streamInfo.resourceInfo.nOffset = read_uint64_le(baOffsetTable, nOffset + 8);
        streamInfo.resourceInfo.nUnpackSize = read_uint64_le(baOffsetTable, nOffset + 16);
        if (bLegacy) {
            streamInfo.nPartNumber = 1;
            streamInfo.nId = read_uint32_le(baOffsetTable, nOffset + 24);
            streamInfo.nRefCount = read_uint32_le(baOffsetTable, nOffset + 28);
            streamInfo.baHash = baOffsetTable.mid(nOffset + 32, WIM_HASH_SIZE);
        } else {
            streamInfo.nPartNumber = read_uint16_le(baOffsetTable, nOffset + 24);
            streamInfo.nRefCount = read_uint32_le(baOffsetTable, nOffset + 26);
            streamInfo.baHash = baOffsetTable.mid(nOffset + 30, WIM_HASH_SIZE);
        }

        const bool bLiveResource = streamInfo.nRefCount != 0;
        if ((streamInfo.baHash.size() != WIM_HASH_SIZE) ||
            (streamInfo.resourceInfo.nUnpackSize > (quint64)(std::numeric_limits<qint64>::max)()) ||
            (streamInfo.nPartNumber == 0) || (streamInfo.nPartNumber > header.nNumberOfParts) ||
            !isWimResourceDescriptorSupported(streamInfo.resourceInfo,
                                              header.nVersion,
                                              bLiveResource) ||
            ((streamInfo.nPartNumber == header.nPartNumber) &&
             !isWimResourceExtentValid(streamInfo.resourceInfo, getSize()))) {
            listResult.clear();
            return listResult;
        }

        const bool bDataStream = !(streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA);

        if (bLegacy && bDataStream) {
            if (stLegacyIds.contains(streamInfo.nId)) return QList<STREAM_INFO>();
            stLegacyIds.insert(streamInfo.nId);
        }
        if (bLegacy && (streamInfo.nRefCount != 0) &&
            bDataStream && (streamInfo.nId == 0)) {
            return QList<STREAM_INFO>();
        }
        if (!bLegacy && bDataStream) {
            if (_isEmptyHash(streamInfo.baHash) || stModernHashes.contains(streamInfo.baHash)) {
                return QList<STREAM_INFO>();
            }
            stModernHashes.insert(streamInfo.baHash);
        }

        // Retain all local non-metadata ranges, including FREE/deleted data,
        // because they still describe physical bytes.  Deleted metadata is a
        // compatibility exception used by real WAIK images.  A dead SOLID
        // descriptor in pipable v0xE00 is similarly ignored because this
        // reader cannot interpret its solid-resource framing.
        const bool bDeletedMetadata = !bDataStream && !bLiveResource;
        const bool bIgnoredDeadSolid =
            !bLiveResource && (header.nVersion == 0x00000E00) &&
            (streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_SOLID);
        if ((streamInfo.nPartNumber == header.nPartNumber) &&
            !bDeletedMetadata && !bIgnoredDeadSolid &&
            !insertWimResourceRange(&mapResourceRanges,
                                    streamInfo.resourceInfo.nOffset,
                                    streamInfo.resourceInfo.nPackSize)) {
            return QList<STREAM_INFO>();
        }
        listResult.append(streamInfo);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || listResult.isEmpty()) {
        listResult.clear();
        return listResult;
    }
    *pOk = true;
    return listResult;
}

bool XWIM::_parseMetadata(const QByteArray &baMetadata, const QMap<QByteArray, STREAM_INFO> &mapStreamsByHash,
                          const QMap<quint32, STREAM_INFO> &mapStreamsById, const WIM_HEADER &header,
                          QList<WIM_RECORD> *pListRecords, QMap<QByteArray, quint64> *pHashReferenceCounts,
                          QMap<quint32, quint64> *pIdReferenceCounts, PDSTRUCT *pPdStruct)
{
    if (!pListRecords || !pHashReferenceCounts || !pIdReferenceCounts || (baMetadata.size() < 8)) {
        return false;
    }

    WIM_METADATA_CONTEXT context = {};
    context.bLegacy = (header.nHeaderSize == WIM_HEADER_SIZE_OLD);
    context.nAlignMask = (header.nVersion == 0x00010900) ? 3 : 7;
    context.nDirEntrySize = context.bLegacy ? WIM_DIR_ENTRY_SIZE_OLD : WIM_DIR_ENTRY_SIZE;
    context.mapStreamsByHash = mapStreamsByHash;
    context.mapStreamsById = mapStreamsById;
    context.pListRecords = pListRecords;
    context.pHashReferenceCounts = pHashReferenceCounts;
    context.pIdReferenceCounts = pIdReferenceCounts;
    context.pPdStruct = pPdStruct;

    const quint32 nTotalLength = read_uint32_le(baMetadata, 0);
    qint64 nDirOffset = 8;

    if (context.bLegacy) {
        const quint32 nNumberOfSecurityEntries = read_uint32_le(baMetadata, 4);
        if ((nNumberOfSecurityEntries > (1U << 28)) ||
            (nNumberOfSecurityEntries > ((quint32)baMetadata.size() >> 3))) return false;

        qint64 nSecurityEnd = nNumberOfSecurityEntries ? (qint64)nNumberOfSecurityEntries * 8 : 8;
        if (nSecurityEnd > baMetadata.size()) return false;
        for (quint32 i = 0; (i < nNumberOfSecurityEntries) &&
                                  XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            const qint64 nEntryOffset = (qint64)i * 8;
            const quint32 nSecuritySize = read_uint32_le(baMetadata, nEntryOffset);
            if ((i != 0) && (read_uint32_le(baMetadata, nEntryOffset + 4) != 0)) return false;
            if ((quint64)nSecuritySize > (quint64)baMetadata.size() - (quint64)nSecurityEnd) return false;
            nSecurityEnd += nSecuritySize;
        }
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        nDirOffset = (nSecurityEnd + context.nAlignMask) & ~((qint64)context.nAlignMask);
    } else if (nTotalLength != 0) {
        if ((nTotalLength < 8) || (nTotalLength > (quint32)baMetadata.size())) return false;

        const quint32 nNumberOfSecurityEntries = read_uint32_le(baMetadata, 4);
        if (nNumberOfSecurityEntries > ((nTotalLength - 8) >> 3)) return false;

        qint64 nEntryOffset = 8;
        quint64 nSecurityEnd = 8 + (quint64)nNumberOfSecurityEntries * 8;
        for (quint32 i = 0; (i < nNumberOfSecurityEntries) &&
                                  XBinary::isPdStructNotCanceled(pPdStruct); i++, nEntryOffset += 8) {
            const quint64 nSecuritySize = read_uint64_le(baMetadata, nEntryOffset);
            if (nSecuritySize > (quint64)nTotalLength - nSecurityEnd) return false;
            nSecurityEnd += nSecuritySize;
        }
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const quint64 nAlignedSecurityEnd = (nSecurityEnd + 7) & ~7ULL;
        const quint64 nAlignedTotalLength = ((quint64)nTotalLength + 7) & ~7ULL;
        if (nAlignedSecurityEnd != nAlignedTotalLength) return false;
        nDirOffset = (qint64)nAlignedSecurityEnd;
    }

    if ((nDirOffset < 8) || (nDirOffset > ((qint64)baMetadata.size() - 8))) return false;
    context.nDirStart = nDirOffset;
    return XBinary::isPdStructNotCanceled(pPdStruct) &&
           _parseMetadataDir(baMetadata, nDirOffset, QString(), &context, 0);
}

bool XWIM::_reserveMetadataRange(WIM_METADATA_CONTEXT *pContext, qint64 nOffset, qint64 nSize, qint64 nMetadataSize)
{
    if (!pContext || (nOffset < pContext->nDirStart) || (nSize <= 0) ||
        (nOffset > (nMetadataSize - nSize)) ||
        (pContext->mapParsedRanges.size() >= WIM_MAX_RECORDS * 3)) return false;

    const qint64 nEnd = nOffset + nSize;
    QMap<qint64, qint64>::const_iterator it = pContext->mapParsedRanges.lowerBound(nOffset);
    if ((it != pContext->mapParsedRanges.constEnd()) && (nEnd > it.key())) return false;
    if (it != pContext->mapParsedRanges.constBegin()) {
        --it;
        if (it.value() > nOffset) return false;
    }
    pContext->mapParsedRanges.insert(nOffset, nEnd);
    return true;
}

void XWIM::_countStreamReference(WIM_METADATA_CONTEXT *pContext, const WIM_RECORD &streamRecord)
{
    if (pContext->bLegacy) {
        if (streamRecord.nStreamId != 0) {
            (*pContext->pIdReferenceCounts)[streamRecord.nStreamId]++;
        }
    } else if ((streamRecord.baHash.size() == WIM_HASH_SIZE) &&
               !XWIM::_isEmptyHash(streamRecord.baHash)) {
        (*pContext->pHashReferenceCounts)[streamRecord.baHash]++;
    }
}

bool XWIM::_parseMetadataDir(const QByteArray &baMetadata, qint64 nOffset, const QString &sParent,
                             WIM_METADATA_CONTEXT *pContext, qint32 nDepth)
{
    if (!pContext || !pContext->pListRecords || !pContext->pHashReferenceCounts ||
        !pContext->pIdReferenceCounts || !XBinary::isPdStructNotCanceled(pContext->pPdStruct) ||
        (nDepth > WIM_MAX_DIRECTORY_DEPTH) ||
        (sParent.size() > WIM_MAX_PATH_LENGTH)) return false;

    const qint32 nAlignMask = pContext->nAlignMask;
    const qint32 nDirEntrySize = pContext->nDirEntrySize;

    if ((nOffset < pContext->nDirStart) || ((nOffset & nAlignMask) != 0) ||
        (nOffset > ((qint64)baMetadata.size() - 8))) {
        return false;
    }

    qint64 nCurrentOffset = nOffset;

    while ((nCurrentOffset >= pContext->nDirStart) &&
           XBinary::isPdStructNotCanceled(pContext->pPdStruct)) {
        if (nCurrentOffset > ((qint64)baMetadata.size() - 8)) return false;

        const quint64 nLength = read_uint64_le(baMetadata, nCurrentOffset);

        if (nLength == 0) {
            return _reserveMetadataRange(pContext, nCurrentOffset, 8, baMetadata.size());
        }

        if ((nLength & nAlignMask) || (nLength < (quint64)nDirEntrySize) ||
            (nLength > (quint64)baMetadata.size() - (quint64)nCurrentOffset) ||
            !_reserveMetadataRange(pContext, nCurrentOffset, (qint64)nLength, baMetadata.size())) {
            return false;
        }

        const quint16 nAltStreams = read_uint16_le(baMetadata, nCurrentOffset + nDirEntrySize - 6);
        const quint16 nShortNameSize = read_uint16_le(baMetadata, nCurrentOffset + nDirEntrySize - 4);
        const quint16 nFileNameSize = read_uint16_le(baMetadata, nCurrentOffset + nDirEntrySize - 2);

        if ((nShortNameSize & 1) || (nFileNameSize & 1)) {
            return false;
        }

        quint32 nShortNameSizeWithTerminator = nShortNameSize ? (quint32)nShortNameSize + 2 : 0;
        quint32 nFileNameSizeWithTerminator = nFileNameSize ? (quint32)nFileNameSize + 2 : 0;

        if ((((quint64)nDirEntrySize + nFileNameSizeWithTerminator + nShortNameSizeWithTerminator + nAlignMask) &
             ~(quint64)nAlignMask) > nLength) {
            return false;
        }

        const qint64 nFileNameOffset = nCurrentOffset + nDirEntrySize;
        const qint64 nShortNameOffset = nFileNameOffset + nFileNameSizeWithTerminator;
        if (!isWimUtf16LEValid(baMetadata, nFileNameOffset, nFileNameSize) ||
            (read_uint16_le(baMetadata, nFileNameOffset + nFileNameSize) != 0) ||
            (nShortNameSize && (!isWimUtf16LEValid(baMetadata, nShortNameOffset, nShortNameSize) ||
                                (read_uint16_le(baMetadata, nShortNameOffset + nShortNameSize) != 0)))) {
            return false;
        }

        WIM_RECORD record = _createRecordFromMetadataItem(baMetadata, nCurrentOffset, sParent, *pContext);

        const bool bLegacyMainMayFollow = pContext->bLegacy && !record.bIsFolder && (nAltStreams != 0);
        if ((!record.bValid && !bLegacyMainMayFollow) ||
            (record.sFileName.isEmpty() && !record.bIsFolder)) return false;

        const quint32 nAttributes = read_uint32_le(baMetadata, nCurrentOffset + 8);
        const QByteArray baMainHash = pContext->bLegacy ? QByteArray() : baMetadata.mid(nCurrentOffset + 0x40, WIM_HASH_SIZE);
        const quint32 nMainId = pContext->bLegacy && !record.bIsFolder ? read_uint32_le(baMetadata, nCurrentOffset + 0x10) : 0;
        bool bMainReferenceEmpty = pContext->bLegacy ? (nMainId == 0) : _isEmptyHash(baMainHash);
        QList<WIM_RECORD> listAltRecords;
        QSet<QString> stAltNames;

        const quint64 nSubdirOffsetValue = read_uint64_le(baMetadata, nCurrentOffset + 0x10);
        if (pContext->bLegacy && !record.bIsFolder &&
            (read_uint32_le(baMetadata, nCurrentOffset + 0x14) != 0)) return false;
        if (nSubdirOffsetValue > (quint64)(std::numeric_limits<qint64>::max)()) return false;
        qint64 nSubdirOffset = (qint64)nSubdirOffsetValue;
        if (!pContext->bLegacy && !record.bIsFolder && (nSubdirOffset != 0)) return false;
        qint64 nNextOffset = nCurrentOffset + (qint64)nLength;

        for (quint16 i = 0; (i < nAltStreams) &&
                              XBinary::isPdStructNotCanceled(pContext->pPdStruct); i++) {
            if ((nNextOffset < 0) || (nNextOffset > ((qint64)baMetadata.size() - 8))) {
                return false;
            }

            const quint64 nAltLength = read_uint64_le(baMetadata, nNextOffset);
            const quint32 nAltMinSize = pContext->bLegacy ? 0x18 : 0x28;

            if ((nAltLength & nAlignMask) || (nAltLength < nAltMinSize) ||
                (nAltLength > (quint64)baMetadata.size() - (quint64)nNextOffset) ||
                !_reserveMetadataRange(pContext, nNextOffset, (qint64)nAltLength, baMetadata.size())) {
                return false;
            }

            const quint32 nAltId = pContext->bLegacy ? read_uint32_le(baMetadata, nNextOffset + 8) : 0;
            if (pContext->bLegacy) {
                if (read_uint32_le(baMetadata, nNextOffset + 0x0C) != 0) return false;
            } else if (read_uint64_le(baMetadata, nNextOffset + 8) != 0) {
                return false;
            }

            const QByteArray baAltHash = pContext->bLegacy ? QByteArray() : baMetadata.mid(nNextOffset + 0x10, WIM_HASH_SIZE);
            const qint64 nAltNameLengthOffset = pContext->bLegacy ? nNextOffset + 0x10 : nNextOffset + 0x24;
            const quint16 nAltNameSize = read_uint16_le(baMetadata, nAltNameLengthOffset);
            const qint64 nAltNameOffset = nAltNameLengthOffset + 2;
            if ((nAltNameSize & 1) ||
                ((((quint64)(nAltNameOffset - nNextOffset) + (quint64)nAltNameSize + 2ULL + nAlignMask) &
                  ~(quint64)nAlignMask) > nAltLength) ||
                !isWimUtf16LEValid(baMetadata, nAltNameOffset, nAltNameSize) ||
                (read_uint16_le(baMetadata, nAltNameOffset + nAltNameSize) != 0)) {
                return false;
            }

            QString sAltName = _readUTF16LEString(baMetadata, nAltNameOffset, nAltNameSize);
            sAltName.replace(QLatin1Char('/'), QLatin1Char('_'));
            sAltName.replace(QLatin1Char('\\'), QLatin1Char('_'));
            if ((sAltName == QLatin1String(".")) || (sAltName == QLatin1String(".."))) return false;

            const bool bRepresentsMain = sAltName.isEmpty() &&
                                         (pContext->bLegacy || bMainReferenceEmpty) &&
                                         (((nAttributes & 0x400) != 0) || !record.bIsFolder);
            if (bRepresentsMain) {
                // Old WIMs always store the authoritative unnamed stream ID here,
                // even when the directory entry already contains a nonzero ID.
                if (pContext->bLegacy && (nAltId == 0)) {
                    record.nStreamOffset = 0;
                    record.nStreamSize = 0;
                    record.nUncompressedSize = 0;
                    record.resourceInfo = RESOURCE_INFO();
                    record.baHash.clear();
                    record.nStreamId = 0;
                    record.handleMethod = HANDLE_METHOD_STORE;
                }
                if (!_applyStreamReference(baAltHash, nAltId, *pContext, &record)) return false;
                record.bValid = true;
                if (!pContext->bLegacy) bMainReferenceEmpty = _isEmptyHash(baAltHash);
            } else {
                if (record.sFileName.isEmpty()) return false;
                if (sAltName.isEmpty()) sAltName = QString("unnamed_%1").arg(i);
                if (stAltNames.contains(sAltName)) return false;
                stAltNames.insert(sAltName);

                WIM_RECORD altRecord = {};
                altRecord.sFileName = record.sFileName + QStringLiteral(".__streams__/") + sAltName;
                altRecord.bIsFolder = false;
                altRecord.handleMethod = HANDLE_METHOD_STORE;
                altRecord.mtDateTime = record.mtDateTime;
                altRecord.bValid = _applyStreamReference(baAltHash, nAltId, *pContext, &altRecord);
                if (!altRecord.bValid) return false;
                listAltRecords.append(altRecord);
            }

            nNextOffset += (qint64)nAltLength;
        }
        if (!XBinary::isPdStructNotCanceled(pContext->pPdStruct)) return false;

        if (!record.bValid) return false;

        _countStreamReference(pContext, record);
        for (qint32 i = 0; (i < listAltRecords.count()) &&
                              XBinary::isPdStructNotCanceled(pContext->pPdStruct); i++) {
            _countStreamReference(pContext, listAltRecords.at(i));
        }
        if (!XBinary::isPdStructNotCanceled(pContext->pPdStruct)) return false;

        // Some old DISM/Longhorn images put otherwise hidden root entries directly
        // after the empty root terminator while pointing SubdirOffset past them.
        // Follow the same narrowly gated correction used by the bundled reference.
        if ((nDepth == 0) && (nCurrentOffset == nOffset) && record.bIsFolder &&
            (nShortNameSize == 0) && (nFileNameSize == 0) && (nSubdirOffset != 0) &&
            (nNextOffset <= ((qint64)baMetadata.size() - 16)) &&
            (read_uint64_le(baMetadata, nNextOffset) == 0) &&
            (read_uint64_le(baMetadata, nNextOffset + 8) != 0) &&
            ((nNextOffset + 8) < nSubdirOffset)) {
            nSubdirOffset = nNextOffset + 8;
        }

        if ((pContext->pListRecords->count() + listAltRecords.count() + (record.sFileName.isEmpty() ? 0 : 1)) > WIM_MAX_RECORDS) return false;
        if (!record.sFileName.isEmpty()) pContext->pListRecords->append(record);
        pContext->pListRecords->append(listAltRecords);

        if (record.bIsFolder && (nSubdirOffset != 0)) {
            QString sChildParent = record.sFileName.isEmpty() ? sParent : record.sFileName;

            if ((nSubdirOffset < pContext->nDirStart) || ((nSubdirOffset & nAlignMask) != 0) ||
                (nSubdirOffset > ((qint64)baMetadata.size() - 8)) ||
                !_parseMetadataDir(baMetadata, nSubdirOffset, sChildParent, pContext, nDepth + 1)) {
                return false;
            }
        }

        nCurrentOffset = nNextOffset;
    }

    return XBinary::isPdStructNotCanceled(pContext->pPdStruct);
}

XWIM::WIM_RECORD XWIM::_createRecordFromMetadataItem(const QByteArray &baMetadata, qint64 nOffset, const QString &sParent,
                                                       const WIM_METADATA_CONTEXT &context)
{
    WIM_RECORD record = {};

    const quint32 nAttributes = read_uint32_le(baMetadata, nOffset + 8);
    const quint16 nFileNameSize = read_uint16_le(baMetadata, nOffset + context.nDirEntrySize - 2);
    QString sName = _readUTF16LEString(baMetadata, nOffset + context.nDirEntrySize, nFileNameSize);

    sName.replace(QLatin1Char('/'), QLatin1Char('_'));
    sName.replace(QLatin1Char('\\'), QLatin1Char('_'));

    if ((sName == QLatin1String(".")) || (sName == QLatin1String(".."))) return record;

    if (!sName.isEmpty()) {
        record.sFileName = sParent.isEmpty() ? sName : (sParent + QLatin1Char('/') + sName);
    } else {
        record.sFileName = sParent;
    }

    if (record.sFileName.size() > WIM_MAX_PATH_LENGTH) return WIM_RECORD();

    record.bIsFolder = (nAttributes & 0x10) != 0;
    record.nStreamOffset = 0;
    record.nStreamSize = 0;
    record.nUncompressedSize = 0;
    record.handleMethod = HANDLE_METHOD_STORE;
    record.mtDateTime = winFileTimeToQDateTime(read_uint64_le(baMetadata, nOffset + (context.bLegacy ? 0x28 : 0x38)));

    if (context.bLegacy) {
        record.bValid = record.bIsFolder ||
                        _applyStreamReference(QByteArray(), read_uint32_le(baMetadata, nOffset + 0x10), context, &record);
    } else {
        record.bValid = _applyStreamReference(baMetadata.mid(nOffset + 0x40, WIM_HASH_SIZE), 0, context, &record);
    }

    return record;
}

bool XWIM::_applyStreamReference(const QByteArray &baHash, quint32 nId, const WIM_METADATA_CONTEXT &context,
                                 WIM_RECORD *pRecord)
{
    if (!pRecord) return false;

    if (context.bLegacy) {
        if (nId == 0) return true;
        if (!context.mapStreamsById.contains(nId)) return false;
        return _applyStreamInfo(context.mapStreamsById.value(nId), pRecord);
    }

    if (baHash.size() != WIM_HASH_SIZE) return false;
    if (_isEmptyHash(baHash)) return true;
    if (!context.mapStreamsByHash.contains(baHash)) return false;
    return _applyStreamInfo(context.mapStreamsByHash.value(baHash), pRecord);
}

bool XWIM::_applyStreamInfo(const STREAM_INFO &streamInfo, WIM_RECORD *pRecord)
{
    if (!pRecord || (streamInfo.resourceInfo.nOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
        (streamInfo.resourceInfo.nPackSize > (quint64)(std::numeric_limits<qint64>::max)()) ||
        (streamInfo.resourceInfo.nUnpackSize > (quint64)(std::numeric_limits<qint64>::max)())) return false;
    pRecord->nStreamOffset = (qint64)streamInfo.resourceInfo.nOffset;
    pRecord->nStreamSize = (qint64)streamInfo.resourceInfo.nPackSize;
    pRecord->nUncompressedSize = (qint64)streamInfo.resourceInfo.nUnpackSize;
    pRecord->resourceInfo = streamInfo.resourceInfo;
    pRecord->baHash = streamInfo.baHash;
    pRecord->nStreamId = streamInfo.nId;
    pRecord->handleMethod = _isResourceStored(streamInfo.resourceInfo) ? HANDLE_METHOD_STORE : HANDLE_METHOD_UNKNOWN;
    return true;
}

QString XWIM::_readUTF16LEString(const QByteArray &baData, qint64 nOffset, qint32 nSize)
{
    QString sResult;

    if ((nSize > 0) && ((nSize & 1) == 0) && (nOffset >= 0) &&
        (nOffset <= ((qint64)baData.size() - nSize))) {
        QByteArray baString = baData.mid(nOffset, nSize);
        sResult = QString::fromUtf16(
            reinterpret_cast<const char16_t *>(baString.constData()), nSize / 2);
    }

    return sResult;
}

bool XWIM::_isEmptyHash(const QByteArray &baHash)
{
    if (baHash.size() < WIM_HASH_SIZE) {
        return true;
    }

    for (qint32 i = 0; i < WIM_HASH_SIZE; i++) {
        if (baHash.at(i) != 0) {
            return false;
        }
    }

    return true;
}

QList<QString> XWIM::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'MSWIM'000000");

    return listResult;
}

XBinary *XWIM::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XWIM(pDevice);
}

bool XWIM::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XWIM> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XWIM::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XWIM> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XWIM::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
