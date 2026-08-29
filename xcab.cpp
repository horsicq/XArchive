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
#include "xcab.h"

#include <new>
#include "Algos/xdeflatedecoder.h"
#include "Algos/xlzhdecoder.h"
#include "subdevice.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <algorithm>
#include <limits>
#include <memory>

#ifdef Q_OS_WIN
#include <io.h>
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

static XBinary::XCONVERT _TABLE_XCAB_STRUCTID[] = {{XCab::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XCab::STRUCTID_CFHEADER, "CFHEADER", QString("CFHEADER")},
                                                   {XCab::STRUCTID_CFFOLDER, "CFFOLDER", QString("CFFOLDER")},
                                                   {XCab::STRUCTID_CFFILE, "CFFILE", QString("CFFILE")},
                                                   {XCab::STRUCTID_CFDATA, "CFDATA", QString("CFDATA")}};
static const qint64 CAB_MAX_FOLDER_SIZE = 0x7FFF8000LL;
static const quint16 CAB_MAX_DATA_BLOCK_SIZE = 0x9800;

struct CAB_DEVICE_CHAIN {
    QList<QIODevice *> listDevices;
    QIODevice *pRoot;
    bool bCycle;
};

static CAB_DEVICE_CHAIN cabGetDeviceChain(QIODevice *pDevice)
{
    CAB_DEVICE_CHAIN result = {};
    QSet<QIODevice *> setVisited;

    while (pDevice) {
        if (setVisited.contains(pDevice)) {
            result.bCycle = true;
            break;
        }

        setVisited.insert(pDevice);
        result.listDevices.append(pDevice);
        result.pRoot = pDevice;

        SubDevice *pSubDevice = dynamic_cast<SubDevice *>(pDevice);
        if (!pSubDevice) break;
        pDevice = pSubDevice->getOrigDevice();
    }

    return result;
}

static bool cabDevicesAlias(QIODevice *pSource, QIODevice *pDestination)
{
    const CAB_DEVICE_CHAIN sourceChain = cabGetDeviceChain(pSource);
    const CAB_DEVICE_CHAIN destinationChain = cabGetDeviceChain(pDestination);
    if (sourceChain.listDevices.isEmpty() || destinationChain.listDevices.isEmpty()) return false;

    QSet<QIODevice *> setSourceDevices;
    for (QIODevice *pDevice : sourceChain.listDevices) setSourceDevices.insert(pDevice);
    for (QIODevice *pDevice : destinationChain.listDevices) {
        if (setSourceDevices.contains(pDevice)) return true;
    }

    // A cyclic wrapper graph cannot be resolved to an independent backing
    // object safely.  Fail closed instead of risking a destructive alias.
    if (sourceChain.bCycle || destinationChain.bCycle) return true;

    QIODevice *pSourceRoot = sourceChain.pRoot;
    QIODevice *pDestinationRoot = destinationChain.pRoot;
    if (!pSourceRoot || !pDestinationRoot) return false;

    QBuffer *pSourceBuffer = dynamic_cast<QBuffer *>(pSourceRoot);
    QBuffer *pDestinationBuffer = dynamic_cast<QBuffer *>(pDestinationRoot);
    if (pSourceBuffer && pDestinationBuffer && (&pSourceBuffer->buffer() == &pDestinationBuffer->buffer())) {
        return true;
    }

    QFile *pSourceFile = dynamic_cast<QFile *>(pSourceRoot);
    QFile *pDestinationFile = dynamic_cast<QFile *>(pDestinationRoot);
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
        (QString::compare(QDir::fromNativeSeparators(sSourcePath), QDir::fromNativeSeparators(sDestinationPath), caseSensitivity) == 0)) {
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
    return (fstat(nSourceDescriptor, &sourceStatus) == 0) && (fstat(nDestinationDescriptor, &destinationStatus) == 0) &&
           (sourceStatus.st_dev == destinationStatus.st_dev) && (sourceStatus.st_ino == destinationStatus.st_ino);
#else
    return false;
#endif
}

static quint32 cabDataChecksum(const char *pData, qint32 nSize, quint32 nSeed = 0)
{
    quint32 nResult = nSeed;

    while (nSize >= 4) {
        nResult ^= (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) | ((quint32)(quint8)pData[2] << 16) | ((quint32)(quint8)pData[3] << 24);
        pData += 4;
        nSize -= 4;
    }

    // CAB's partial DWORD convention places the first remaining byte in the
    // most-significant occupied position (unlike the full little-endian words).
    quint32 nTail = 0;
    if (nSize == 3) {
        nTail |= (quint32)(quint8)*pData++ << 16;
    }
    if (nSize >= 2) {
        nTail |= (quint32)(quint8)*pData++ << 8;
    }
    if (nSize >= 1) {
        nTail |= (quint32)(quint8)*pData;
    }

    return nResult ^ nTail;
}

XCab::XCab(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XCab::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (getSize() < (qint64)sizeof(CFHEADER))) {
        return false;
    }

    QMap<UNPACK_PROP, QVariant> mapProperties = getDefaultUnpackProperties();
    UNPACK_STATE state = {};
    bool bResult = initUnpack(&state, mapProperties, pPdStruct);
    finishUnpack(&state, nullptr);

    if (!bResult && XBinary::isPdStructNotCanceled(pPdStruct)) {
        state = UNPACK_STATE();
        mapProperties.insert(UNPACK_PROP_METADATAONLY, true);
        bResult = initUnpack(&state, mapProperties, pPdStruct);
        finishUnpack(&state, nullptr);
    }

    return bResult && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XCab::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCab xcab(pDevice);

    return xcab.isValid(pPdStruct);
}

QString XCab::getVersion()
{
    return QString("%1.%2").arg(read_uint8(25)).arg(read_uint8(24), 2, 10, QChar('0'));
}

XCab::CFFILE XCab::readCFFILE(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    CFFILE result = {};
    _readCFFILEExact(nOffset, &result, pPdStruct);
    return result;
}

bool XCab::_readCFFILEExact(qint64 nOffset, CFFILE *pResult, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    if (!pResult) return false;
    *pResult = {};
    QByteArray baData(sizeof(CFFILE), 0);
    if (read_array_process(nOffset, baData.data(), baData.size(), pPdStruct) != baData.size() || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CFFILE result = {};
    result.cbFile = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + offsetof(CFFILE, cbFile)));
    result.uoffFolderStart = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + offsetof(CFFILE, uoffFolderStart)));
    result.iFolder = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + offsetof(CFFILE, iFolder)));
    result.date = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + offsetof(CFFILE, date)));
    result.time = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + offsetof(CFFILE, time)));
    result.attribs = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + offsetof(CFFILE, attribs)));

    *pResult = result;
    return true;
}

XCab::CFHEADER XCab::readCFHeader(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    CFHEADER result = {};
    _readCFHeaderExact(nOffset, &result, pPdStruct);
    return result;
}

static quint16 cabHeaderLE16(const QByteArray *pbaData, qint32 nFieldOffset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(pbaData->constData() + nFieldOffset));
}

static quint32 cabHeaderLE32(const QByteArray *pbaData, qint32 nFieldOffset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pbaData->constData() + nFieldOffset));
}

bool XCab::_readCFHeaderExact(qint64 nOffset, CFHEADER *pResult, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    if (!pResult) return false;
    *pResult = {};
    QByteArray baData(sizeof(CFHEADER), 0);
    if (read_array_process(nOffset, baData.data(), baData.size(), pPdStruct) != baData.size() || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CFHEADER result = {};
    memcpy(result.signature, baData.constData(), 4);
    result.reserved1 = cabHeaderLE32(&baData, offsetof(CFHEADER, reserved1));
    result.cbCabinet = cabHeaderLE32(&baData, offsetof(CFHEADER, cbCabinet));
    result.reserved2 = cabHeaderLE32(&baData, offsetof(CFHEADER, reserved2));
    result.coffFiles = cabHeaderLE32(&baData, offsetof(CFHEADER, coffFiles));
    result.reserved3 = cabHeaderLE32(&baData, offsetof(CFHEADER, reserved3));
    result.versionMinor = (quint8)baData.at(offsetof(CFHEADER, versionMinor));
    result.versionMajor = (quint8)baData.at(offsetof(CFHEADER, versionMajor));
    result.cFolders = cabHeaderLE16(&baData, offsetof(CFHEADER, cFolders));
    result.cFiles = cabHeaderLE16(&baData, offsetof(CFHEADER, cFiles));
    result.flags = cabHeaderLE16(&baData, offsetof(CFHEADER, flags));
    result.setID = cabHeaderLE16(&baData, offsetof(CFHEADER, setID));
    result.iCabinet = cabHeaderLE16(&baData, offsetof(CFHEADER, iCabinet));

    // if (result.flags & 0x0004)  // TODO const
    // {
    //     result.cbCFHeader = read_uint16(offsetof(CFHEADER, cbCFHeader));
    //     result.cbCFFolder = read_uint8(offsetof(CFHEADER, cbCFFolder));
    //     result.cbCFData = read_uint8(offsetof(CFHEADER, cbCFData));
    // }

    *pResult = result;
    return true;
}

XCab::CFFOLDER XCab::readCFFolder(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    CFFOLDER result = {};
    _readCFFolderExact(nOffset, &result, pPdStruct);
    return result;
}

bool XCab::_readCFFolderExact(qint64 nOffset, CFFOLDER *pResult, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    if (!pResult) return false;
    *pResult = {};
    QByteArray baData(sizeof(CFFOLDER), 0);
    if (read_array_process(nOffset, baData.data(), baData.size(), pPdStruct) != baData.size() || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CFFOLDER result = {};
    result.coffCabStart = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData()));
    result.cCFData = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + 4));
    result.typeCompress = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + 6));

    *pResult = result;
    return true;
}

XCab::CFDATA XCab::readCFData(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    CFDATA result = {};
    _readCFDataExact(nOffset, &result, pPdStruct);
    return result;
}

bool XCab::_readCFDataExact(qint64 nOffset, CFDATA *pResult, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    if (!pResult) return false;
    *pResult = {};
    QByteArray baData(sizeof(CFDATA), 0);
    if (read_array_process(nOffset, baData.data(), baData.size(), pPdStruct) != baData.size() || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CFDATA result = {};
    result.csum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData()));
    result.cbData = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + 4));
    result.cbUncomp = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + 6));

    *pResult = result;
    return true;
}

qint64 XCab::_getStreamSize(qint64 nOffset, qint32 nCount, qint32 nReservedSize, qint64 nCabinetSize, qint64 *pUncompressedSize, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    if ((nOffset < 0) || (nCount < 0) || (nReservedSize < 0) || (nCabinetSize < 0) || (nOffset > nCabinetSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return -1;
    }

    qint64 nCurrentOffset = nOffset;
    qint64 nUncompressedSize = 0;

    for (qint32 i = 0; i < nCount; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return -1;
        }

        if ((qint64)sizeof(CFDATA) + nReservedSize > nCabinetSize - nCurrentOffset) {
            return -1;
        }

        CFDATA cfData = {};
        const bool bDataRead = _readCFDataExact(nCurrentOffset, &cfData, pPdStruct);
        if (!guardedThis || !bDataRead) {
            return -1;
        }
        qint64 nBlockSize = (qint64)sizeof(CFDATA) + nReservedSize + (qint64)cfData.cbData;

        if ((cfData.cbData == 0) || (cfData.cbData > CAB_MAX_DATA_BLOCK_SIZE) || (nBlockSize > nCabinetSize - nCurrentOffset) ||
            ((qint64)cfData.cbUncomp > (std::numeric_limits<qint64>::max)() - nUncompressedSize)) {
            return -1;
        }

        nUncompressedSize += cfData.cbUncomp;
        nCurrentOffset += nBlockSize;
    }

    if (pUncompressedSize) {
        *pUncompressedSize = nUncompressedSize;
    }

    return nCurrentOffset - nOffset;
}

XBinary::FT XCab::getFileType()
{
    return FT_CAB;
}

QString XCab::getFileFormatExt()
{
    return "cab";
}

QString XCab::getFileFormatExtsString()
{
    return "CAB (*.cab)";
}

qint64 XCab::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    qint64 nResult = 0;

    nResult = readCFHeader(0).cbCabinet;  // TODO check mb _getRawSize !!!
    nResult = qMin(getSize(), nResult);

    return nResult;
}

QString XCab::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XCab::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XCab::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

static QList<qint64> cabSelectOffsets(const XBinary::XFSTRUCT *pXfStruct, const QList<qint64> &listAllOffsets, bool bDirect)
{
    if (!bDirect) return listAllOffsets;

    qint32 nStartIndex = 0;
    if (pXfStruct->xLoc.locType != XBinary::LT_UNKNOWN) {
        const qint64 nRequestedOffset = XBinary::locToOffset(pXfStruct->pMemoryMap, pXfStruct->xLoc);
        nStartIndex = listAllOffsets.indexOf(nRequestedOffset);
        if (nStartIndex < 0) return QList<qint64>();
    }

    qint32 nCount = listAllOffsets.size() - nStartIndex;
    if (pXfStruct->nCount > 0) nCount = qMin(nCount, pXfStruct->nCount);
    return listAllOffsets.mid(nStartIndex, nCount);
}

QList<XBinary::XFHEADER> XCab::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return listResult;

    UNPACK_STATE state = {};
    if (!initUnpack(&state, getDefaultUnpackProperties(), pPdStruct) || !state.pContext) {
        finishUnpack(&state, nullptr);
        return listResult;
    }

    CAB_UNPACK_CONTEXT *pContext = static_cast<CAB_UNPACK_CONTEXT *>(state.pContext);
    const quint32 nStructID = xfStruct.nStructID;
    QString sChildParent = xfStruct.sParent;

    if ((nStructID == STRUCTID_UNKNOWN) || (nStructID == STRUCTID_CFHEADER)) {
        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFHEADER);
        xfHeader.xLoc = offsetToLoc(0);
        xfHeader.nSize = sizeof(CFHEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFHEADER, xfHeader.xLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFHEADER), xfHeader.sParentTag);
        sChildParent = xfHeader.sTag;
        listResult.append(xfHeader);
    }

    const bool bIncludeChildren = xfStruct.bIsParent && ((nStructID == STRUCTID_UNKNOWN) || (nStructID == STRUCTID_CFHEADER));
    const bool bIncludeFolders = bIncludeChildren || (nStructID == STRUCTID_CFFOLDER);
    QList<qint32> listSelectedFolderIndexes;
    QString sFolderTag;
    QList<qint64> listFolderOffsets;
    if (bIncludeFolders) {
        listFolderOffsets = cabSelectOffsets(&xfStruct, pContext->listFolderOffsets, nStructID == STRUCTID_CFFOLDER);
        for (qint64 nOffset : qAsConst(listFolderOffsets)) {
            const qint32 nIndex = pContext->listFolderOffsets.indexOf(nOffset);
            if (nIndex >= 0) listSelectedFolderIndexes.append(nIndex);
        }
    }
    if (!listFolderOffsets.isEmpty()) {
        XFHEADER xfHeader = {};
        xfHeader.sParentTag = bIncludeChildren ? sChildParent : xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFFOLDER);
        xfHeader.xfType = XFTYPE_TABLE;
        for (qint64 nOffset : qAsConst(listFolderOffsets)) {
            xfHeader.listRowLocations.append((XADDR)nOffset);
        }
        xfHeader.xLoc = offsetToLoc(xfHeader.listRowLocations.constFirst());
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFFOLDER, xfHeader.xLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFFOLDER), xfHeader.sParentTag);
        sFolderTag = xfHeader.sTag;
        listResult.append(xfHeader);
    }

    const bool bIncludeFiles = bIncludeChildren || (nStructID == STRUCTID_CFFILE);
    const QList<qint64> listFileOffsets = bIncludeFiles ? cabSelectOffsets(&xfStruct, pContext->listFileOffsets, nStructID == STRUCTID_CFFILE) : QList<qint64>();
    if (!listFileOffsets.isEmpty()) {
        XFHEADER xfHeader = {};
        xfHeader.sParentTag = bIncludeChildren ? sChildParent : xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFFILE);
        xfHeader.xfType = XFTYPE_TABLE;
        for (qint64 nOffset : qAsConst(listFileOffsets)) {
            xfHeader.listRowLocations.append((XADDR)nOffset);
        }
        xfHeader.xLoc = offsetToLoc(xfHeader.listRowLocations.constFirst());
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFFILE, xfHeader.xLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFFILE), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    const bool bIncludeData = bIncludeChildren || (nStructID == STRUCTID_CFDATA) || ((nStructID == STRUCTID_CFFOLDER) && xfStruct.bIsParent);
    if (bIncludeData) {
        QList<qint64> listAllDataOffsets;
        QList<qint32> listDataFolderIndexes;
        if (nStructID == STRUCTID_CFFOLDER) {
            listDataFolderIndexes = listSelectedFolderIndexes;
        } else {
            for (qint32 i = 0; i < pContext->listFolders.size(); ++i) {
                listDataFolderIndexes.append(i);
            }
        }
        for (qint32 nFolderIndex : qAsConst(listDataFolderIndexes)) {
            listAllDataOffsets.append(pContext->mapFolderDataOffsets.value((quint16)nFolderIndex));
        }
        const QList<qint64> listDataOffsets = cabSelectOffsets(&xfStruct, listAllDataOffsets, nStructID == STRUCTID_CFDATA);
        if (!listDataOffsets.isEmpty()) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = (bIncludeChildren || (nStructID == STRUCTID_CFFOLDER)) ? sFolderTag : xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFDATA);
            xfHeader.xfType = XFTYPE_TABLE;
            for (qint64 nOffset : qAsConst(listDataOffsets)) {
                xfHeader.listRowLocations.append((XADDR)nOffset);
            }
            xfHeader.xLoc = offsetToLoc(listDataOffsets.constFirst());
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFDATA, xfHeader.xLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFDATA), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    const bool bFinished = finishUnpack(&state, nullptr);
    if (!bFinished || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
    }
    return listResult;
}

QList<XBinary::XFRECORD> XCab::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_CFHEADER) {
        listResult.append({"signature", (qint32)offsetof(CFHEADER, signature), 4, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"reserved1", (qint32)offsetof(CFHEADER, reserved1), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"cbCabinet", (qint32)offsetof(CFHEADER, cbCabinet), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"reserved2", (qint32)offsetof(CFHEADER, reserved2), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"coffFiles", (qint32)offsetof(CFHEADER, coffFiles), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"reserved3", (qint32)offsetof(CFHEADER, reserved3), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"versionMinor", (qint32)offsetof(CFHEADER, versionMinor), 1, XFRECORD_FLAG_VERSION_MINOR, VT_UINT8});
        listResult.append({"versionMajor", (qint32)offsetof(CFHEADER, versionMajor), 1, XFRECORD_FLAG_VERSION_MAJOR, VT_UINT8});
        listResult.append({"cFolders", (qint32)offsetof(CFHEADER, cFolders), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"cFiles", (qint32)offsetof(CFHEADER, cFiles), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"flags", (qint32)offsetof(CFHEADER, flags), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"setID", (qint32)offsetof(CFHEADER, setID), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"iCabinet", (qint32)offsetof(CFHEADER, iCabinet), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
    } else if (nStructID == STRUCTID_CFFOLDER) {
        listResult.append({"coffCabStart", (qint32)offsetof(CFFOLDER, coffCabStart), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"cCFData", (qint32)offsetof(CFFOLDER, cCFData), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"typeCompress", (qint32)offsetof(CFFOLDER, typeCompress), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    } else if (nStructID == STRUCTID_CFFILE) {
        listResult.append({"cbFile", (qint32)offsetof(CFFILE, cbFile), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"uoffFolderStart", (qint32)offsetof(CFFILE, uoffFolderStart), 4, XFRECORD_FLAG_RELATIVE_OFFSET, VT_UINT32});
        listResult.append({"iFolder", (qint32)offsetof(CFFILE, iFolder), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"date", (qint32)offsetof(CFFILE, date), 2, XFRECORD_FLAG_DOSDATE, VT_UINT16});
        listResult.append({"time", (qint32)offsetof(CFFILE, time), 2, XFRECORD_FLAG_DOSTIME, VT_UINT16});
        listResult.append({"attribs", (qint32)offsetof(CFFILE, attribs), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        // Variable-length fields
        const QByteArray baName = read_array_process(xLoc.nLocation + sizeof(CFFILE), 257, nullptr);
        const qint32 nTerminator = baName.indexOf('\0');
        const qint32 nNameFieldSize = (nTerminator >= 0) ? (nTerminator + 1) : baName.size();
        listResult.append({"szName", (qint32)sizeof(CFFILE), nNameFieldSize, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    } else if (nStructID == STRUCTID_CFDATA) {
        listResult.append({"csum", (qint32)offsetof(CFDATA, csum), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"cbData", (qint32)offsetof(CFDATA, cbData), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        listResult.append({"cbUncomp", (qint32)offsetof(CFDATA, cbUncomp), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
    }

    return listResult;
}

// qint32 XCab::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues, void *pUserData,
//                           PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(nRow)
//     Q_UNUSED(locType)
//     Q_UNUSED(nLocation)
//     Q_UNUSED(dataRecordsOptions)
//     Q_UNUSED(pListValues)
//     Q_UNUSED(pUserData)
//     Q_UNUSED(pPdStruct)
//     // Not implemented for CAB
//     return 0;
// }

QString XCab::getMIMEString()
{
    return "application/vnd.ms-cab-compressed";
}

static bool cabPartsCanAppend(XBinary::PDSTRUCT *pPdStruct, qint32 nLimit, const QList<XBinary::FPART> *pListResult)
{
    return XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (pListResult->count() < nLimit));
}

QList<XBinary::FPART> XCab::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    // initUnpack is the authoritative CAB parser.  Running it for every part
    // mask keeps DATA/OVERLAY-only queries from accepting layouts that a
    // HEADER/STREAM query or extraction would reject.
    UNPACK_STATE state = {};
    if (!initUnpack(&state, getDefaultUnpackProperties(), pPdStruct) || !state.pContext) {
        finishUnpack(&state, nullptr);
        return listResult;
    }

    CAB_UNPACK_CONTEXT *pContext = static_cast<CAB_UNPACK_CONTEXT *>(state.pContext);
    const qint64 nFileSize = getSize();
    const qint64 nFileFormatSize = state.nTotalSize;
    bool bContextValid = (pContext->listFolderOffsets.size() == pContext->listFolders.size()) &&
                         (pContext->mapFolderDataOffsets.size() == pContext->listFolders.size()) && (pContext->listFileOffsets.size() == pContext->listFileEnds.size()) &&
                         (pContext->listFileOffsets.size() == pContext->listFileNames.size()) && (pContext->listFileOffsets.size() == pContext->listFiles.size());
    for (qint32 i = 0; bContextValid && (i < pContext->listFolders.size()); ++i) {
        bContextValid = (pContext->listFolderOffsets.at(i) >= 0) && (pContext->mapFolderDataOffsets.value((quint16)i).size() == pContext->listFolders.at(i).cCFData) &&
                        (pContext->mapFolderStreamSizes.value((quint16)i, -1) >= 0) && (pContext->mapFolderDataSizes.value((quint16)i, -1) >= 0);
    }
    for (qint32 i = 0; bContextValid && (i < pContext->listFileOffsets.size()); ++i) {
        bContextValid = (pContext->listFileOffsets.at(i) >= 0) && (pContext->listFileEnds.at(i) > pContext->listFileOffsets.at(i));
    }

    if (!bContextValid) {
        finishUnpack(&state, nullptr);
        return listResult;
    }

    if ((nFileParts & FILEPART_HEADER) && cabPartsCanAppend(pPdStruct, nLimit, &listResult)) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = pContext->nMainHeaderSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && cabPartsCanAppend(pPdStruct, nLimit, &listResult)) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nFileFormatSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_HEADER) {
        for (qint32 i = 0; (i < pContext->listFolders.size()) && cabPartsCanAppend(pPdStruct, nLimit, &listResult); ++i) {
            FPART record = {};
            record.filePart = FILEPART_HEADER;
            record.nFileOffset = pContext->listFolderOffsets.at(i);
            record.nFileSize = (qint64)sizeof(CFFOLDER) + pContext->nCbCFFolder;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = QString("CFFOLDER(%1)").arg(i);
            listResult.append(record);
        }

        for (qint32 i = 0; (i < pContext->listFileOffsets.size()) && cabPartsCanAppend(pPdStruct, nLimit, &listResult); ++i) {
            FPART record = {};
            record.filePart = FILEPART_HEADER;
            record.nFileOffset = pContext->listFileOffsets.at(i);
            record.nFileSize = pContext->listFileEnds.at(i) - record.nFileOffset;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = QString("CFFILE(%1)").arg(i);
            record.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->listFileNames.at(i));
            listResult.append(record);
        }
    }

    if (nFileParts & FILEPART_STREAM) {
        for (qint32 i = 0; (i < pContext->listFolders.size()) && cabPartsCanAppend(pPdStruct, nLimit, &listResult); ++i) {
            const CFFOLDER &folder = pContext->listFolders.at(i);
            const quint16 nFolderIndex = (quint16)i;
            const qint64 nStreamSize = pContext->mapFolderStreamSizes.value(nFolderIndex, -1);
            const qint64 nUncompressedSize = pContext->mapFolderDataSizes.value(nFolderIndex, -1);

            if ((nStreamSize >= 0) && (nUncompressedSize >= 0) && cabPartsCanAppend(pPdStruct, nLimit, &listResult)) {
                FPART record = {};
                record.filePart = FILEPART_STREAM;
                record.nFileOffset = folder.coffCabStart;
                record.nFileSize = nStreamSize;
                record.nVirtualAddress = XADDR_MAX;
                record.sName = tr("Stream") + QString(" (%1)").arg(i);
                record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nStreamSize);
                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
                record.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, nUncompressedSize);
                record.mapProperties.insert(FPART_PROP_TYPE, (quint32)folder.typeCompress);
                record.mapProperties.insert(FPART_PROP_OPTHEADER_SIZE, (qint64)pContext->nCbCFData);

                const quint16 nCompressionType = folder.typeCompress & 0x000F;
                if (nCompressionType == 0) {
                    record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
                } else if (nCompressionType == 1) {
                    record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
                } else if (nCompressionType == 3) {
                    record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
                    record.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((folder.typeCompress >> 8) & 0x1F));
                } else if (nCompressionType == 2) {
                    record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_QUANTUM_CAB);
                    record.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((folder.typeCompress >> 8) & 0x1F));
                } else {
                    record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
                }

                listResult.append(record);
            }
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && cabPartsCanAppend(pPdStruct, nLimit, &listResult)) {
        if (nFileFormatSize < nFileSize) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFileFormatSize;
            record.nFileSize = nFileSize - nFileFormatSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    const bool bFinished = finishUnpack(&state, nullptr);
    if (!bFinished || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XCab::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_CFHEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_CFHEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CFHEADER);
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, signature), 4, "signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, reserved1), 4, "reserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cbCabinet), 4, "cbCabinet", VT_UINT32, DRF_SIZE,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, reserved2), 4, "reserved2", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, coffFiles), 4, "coffFiles", VT_UINT32, DRF_OFFSET,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, reserved3), 4, "reserved3", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, versionMinor), 1, "versionMinor", VT_UINT8, DRF_VERSION, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, versionMajor), 1, "versionMajor", VT_UINT8, DRF_VERSION, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFolders), 2, "cFolders", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFiles), 2, "cFiles", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, flags), 2, "flags", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, setID), 2, "setID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, iCabinet), 2, "iCabinet", VT_UINT16, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian));
//                 // Optional fields not handled in this example

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren) {
//                     CFHEADER cfHeader = readCFHeader(nStartOffset);
//                     if (cfHeader.cFolders) {
//                         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                         _dataHeadersOptions.nLocation = nStartOffset + sizeof(CFHEADER);
//                         _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                         _dataHeadersOptions.nCount = cfHeader.cFolders;
//                         _dataHeadersOptions.nSize = sizeof(CFFOLDER) * cfHeader.cFolders;
//                         _dataHeadersOptions.nID = STRUCTID_CFFOLDER;
//                         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                     }

//                     if (cfHeader.coffFiles) {
//                         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                         _dataHeadersOptions.nLocation = nStartOffset + cfHeader.coffFiles;
//                         _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                         _dataHeadersOptions.nCount = cfHeader.cFiles;
//                         _dataHeadersOptions.nSize = sizeof(CFFILE) * cfHeader.cFiles;  // TODO Names and extra fields
//                         _dataHeadersOptions.nID = STRUCTID_CFFILE;
//                         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                     }
//                 }
//             } else if (dataHeadersOptions.nID == STRUCTID_CFFOLDER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CFFOLDER);
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFFOLDER, coffCabStart), 4, "coffCabStart", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFOLDER, cCFData), 2, "cCFData", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFFOLDER, typeCompress), 2, "typeCompress", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             } else if (dataHeadersOptions.nID == STRUCTID_CFFILE) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CFFILE);
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, cbFile), 4, "cbFile", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFFILE, uoffFolderStart), 4, "uoffFolderStart", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, iFolder), 2, "iFolder", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, date), 2, "date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, time), 2, "time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, attribs), 2, "attribs", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 QString szName = read_ansiString(nStartOffset + sizeof(CFFILE), 256);  // Limit to 256 chars for safety)
//                 dataHeader.listRecords.append(
//                     getDataRecord(sizeof(CFFILE), szName.size() + 1, "szName", VT_CHAR_ARRAY, DRF_VOLATILE, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::MAPMODE> XCab::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XCab::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;  // Default mode
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

qint64 XCab::getImageSize()
{
    return (qint64)readCFHeader(0).cbCabinet;
}

XBinary::MODE XCab::getMode()
{
    return MODE_DATA;
}

QString XCab::getArch()
{
    return QString("Generic");
}

XBinary::ENDIAN XCab::getEndian()
{
    return ENDIAN_LITTLE;
}

// Streaming unpacking API implementation
QMap<XBinary::UNPACK_PROP, QVariant> XCab::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

static bool cabFailUnpackSource(QPointer<XCab> *pGuardedThis, XBinary::UNPACK_STATE *pState)
{
    if (pGuardedThis->isNull()) return false;
    (*pGuardedThis)->releaseUnpackSource(pState);
    return false;
}

static bool cabFailUnpackInit(QPointer<XCab> *pGuardedThis, XBinary::UNPACK_STATE *pState, XCab::CAB_UNPACK_CONTEXT *pContext)
{
    if (pGuardedThis->isNull()) return false;
    (*pGuardedThis)->releaseUnpackSource(pState);
    *pState = XBinary::UNPACK_STATE();
    delete pContext;
    return false;
}

// CAB strings are byte-counted by their terminating NUL, not by the
// decoded QString length.  Requiring the terminator also prevents a
// malformed field from walking into the folder/file tables.
static bool cabReadCabString(XCab *pThis, QPointer<XCab> *pGuardedThis, qint64 nCabinetSize, XBinary::PDSTRUCT *pPdStruct, qint64 *pOffset, QByteArray *pBytes,
                             qint32 nMaximumFieldSize)
{
    if (!pOffset || !pBytes || (*pOffset < 0) || (*pOffset >= nCabinetSize)) {
        return false;
    }

    qint32 nMaximum = (qint32)qMin<qint64>(nMaximumFieldSize, nCabinetSize - *pOffset);
    QByteArray baValue = pThis->read_array_process(*pOffset, nMaximum, pPdStruct);
    if (pGuardedThis->isNull() || (baValue.size() != nMaximum) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint32 nTerminator = baValue.indexOf('\0');
    if (nTerminator < 0) {
        return false;
    }

    *pBytes = baValue.left(nTerminator);
    *pOffset += (qint64)nTerminator + 1;
    return true;
}

static bool cabStreamRangeLessThan(const QPair<qint64, qint64> &a, const QPair<qint64, qint64> &b)
{
    return (a.first < b.first) || ((a.first == b.first) && (a.second < b.second));
}

bool XCab::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress) {
        return false;
    }

    const bool bMetadataOnly = mapProperties.value(UNPACK_PROP_METADATAONLY, false).toBool();

    const bool bFinished = finishUnpack(pState, nullptr);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const bool bOpen = guardedSource->isOpen();
    if (!guardedThis || !guardedSource || !bOpen) return false;
    const bool bReadable = guardedSource->isReadable();
    if (!guardedThis || !guardedSource || !bReadable) return false;
    const bool bSequential = guardedSource->isSequential();
    if (!guardedThis || !guardedSource || bSequential) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    SOURCE_DEVICE_SNAPSHOT sourceSnapshot = {};
    if (!getBoundUnpackSourceSnapshot(pState, &sourceSnapshot)) {
        return cabFailUnpackSource(&guardedThis, pState);
    }

    qint64 nFileSize = getSize();
    if (!guardedThis) return false;

    if (nFileSize < (qint64)sizeof(CFHEADER)) {
        return cabFailUnpackSource(&guardedThis, pState);
    }

    // Read CAB header
    CFHEADER cfHeader = {};
    const bool bHeaderRead = _readCFHeaderExact(0, &cfHeader, pPdStruct);
    if (!guardedThis) return false;
    if (!bHeaderRead) return cabFailUnpackSource(&guardedThis, pState);
    if (cfHeader.signature[0] != 'M' || cfHeader.signature[1] != 'S' || cfHeader.signature[2] != 'C' || cfHeader.signature[3] != 'F') {
        return cabFailUnpackSource(&guardedThis, pState);  // Invalid CAB signature
    }

    // cbCabinet is the authoritative end of the cabinet.  A PE resource may
    // contain alignment padding after it, but no CAB structure may reference
    // that padding.
    qint64 nCabinetSize = cfHeader.cbCabinet;
    if ((cfHeader.reserved1 != 0) || (cfHeader.reserved2 != 0) || (cfHeader.reserved3 != 0) || (cfHeader.versionMinor != 3) || (cfHeader.versionMajor != 1) ||
        ((cfHeader.flags & 0xFFF8U) != 0) || ((((cfHeader.flags & 0x0001) != 0) != (cfHeader.iCabinet != 0))) || (cfHeader.cFolders == 0) || (cfHeader.cFiles == 0) ||
        (nCabinetSize <= (qint64)sizeof(CFHEADER)) || (nCabinetSize > 0x7FFFFFFFLL) || (nCabinetSize > nFileSize)) {
        return cabFailUnpackSource(&guardedThis, pState);
    }

    // Create unpack context
    CAB_UNPACK_CONTEXT *pContext = new (std::nothrow) CAB_UNPACK_CONTEXT;
    if (!pContext) {
        return cabFailUnpackSource(&guardedThis, pState);
    }
    pContext->nCurrentFileIndex = 0;
    pContext->nCbCFHeader = 0;
    pContext->nCbCFFolder = 0;
    pContext->nCbCFData = 0;
    pContext->nMainHeaderSize = 0;
    pContext->sourceSnapshot = sourceSnapshot;

    pState->pContext = pContext;
    if (!registerUnpackContextCleanup(pState, pContext, &deleteUnpackContext<CAB_UNPACK_CONTEXT>)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        delete pContext;
        return false;
    }

    // Handle reserved fields (flag 0x0004 = cfhdrRESERVE_PRESENT)
    qint64 nFolderOffset = sizeof(CFHEADER);

    if (cfHeader.flags & 0x0004) {
        if (4 > nCabinetSize - nFolderOffset) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        QByteArray baReserveSizes(sizeof(quint16) + 2, 0);
        if ((read_array_process(nFolderOffset, baReserveSizes.data(), baReserveSizes.size(), pPdStruct) != baReserveSizes.size()) || !guardedThis ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        pContext->nCbCFHeader = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baReserveSizes.constData()));
        pContext->nCbCFFolder = (quint8)baReserveSizes.at(2);
        pContext->nCbCFData = (quint8)baReserveSizes.at(3);
        nFolderOffset += 4;

        if ((pContext->nCbCFHeader > 60000) || ((qint64)pContext->nCbCFHeader > nCabinetSize - nFolderOffset)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        nFolderOffset += pContext->nCbCFHeader;
    }

    // Handle optional previous cabinet name (flag 0x0001 = cfhdrPREV_CABINET)
    if (cfHeader.flags & 0x0001) {
        QByteArray baPreviousCabinet;
        QByteArray baPreviousDisk;
        if (!cabReadCabString(this, &guardedThis, nCabinetSize, pPdStruct, &nFolderOffset, &baPreviousCabinet, 256) ||
            !cabReadCabString(this, &guardedThis, nCabinetSize, pPdStruct, &nFolderOffset, &baPreviousDisk, 256)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
    }

    // Handle optional next cabinet name (flag 0x0002 = cfhdrNEXT_CABINET)
    if (cfHeader.flags & 0x0002) {
        QByteArray baNextCabinet;
        QByteArray baNextDisk;
        if (!cabReadCabString(this, &guardedThis, nCabinetSize, pPdStruct, &nFolderOffset, &baNextCabinet, 256) ||
            !cabReadCabString(this, &guardedThis, nCabinetSize, pPdStruct, &nFolderOffset, &baNextDisk, 256)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
    }

    pContext->nMainHeaderSize = nFolderOffset;

    // Parse folders (each CFFOLDER may have per-folder reserved area)
    qint64 nFolderStructSize = (qint64)sizeof(CFFOLDER) + pContext->nCbCFFolder;
    if ((qint64)cfHeader.cFolders * nFolderStructSize > nCabinetSize - nFolderOffset) {
        return cabFailUnpackInit(&guardedThis, pState, pContext);
    }

    qint64 nAggregateStreamSize = 0;
    QSet<qint64> setNonemptyStreamStarts;
    for (quint16 i = 0; i < cfHeader.cFolders; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        CFFOLDER cfFolder = {};
        if (!_readCFFolderExact(nFolderOffset, &cfFolder, pPdStruct)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        quint16 nCompressionType = cfFolder.typeCompress & 0x000F;
        const quint16 nCompressionLevel = cfFolder.typeCompress & 0x00F0;
        const qint32 nWindowBits = (cfFolder.typeCompress >> 8) & 0x1F;
        if ((cfFolder.typeCompress & 0xE000) || (nCompressionType > 3) || (((nCompressionType == 0) || (nCompressionType == 1)) && (cfFolder.typeCompress & 0x1FF0)) ||
            ((nCompressionType == 2) && ((nCompressionLevel < 0x0010) || (nCompressionLevel > 0x0070) || (nWindowBits < 10) || (nWindowBits > 21))) ||
            ((nCompressionType == 3) && ((nCompressionLevel != 0) || (nWindowBits < 15) || (nWindowBits > 21)))) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        if (bMetadataOnly) {
            // CFFILE directory metadata is independent of the CFDATA stream.
            // Listing must not decompress or checksum the payload; extraction
            // creates a fresh strict state and performs the full validation.
            pContext->listFolders.append(cfFolder);
            pContext->listFolderOffsets.append(nFolderOffset);
            pContext->setUnvalidatedFolderStreams.insert(i);
            nFolderOffset += nFolderStructSize;
            continue;
        }

        qint64 nFolderDataSize = 0;
        qint64 nStreamSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData, pContext->nCbCFData, nCabinetSize, &nFolderDataSize, pPdStruct);
        if (!guardedThis) return false;
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        bool bFolderStreamValid = (nStreamSize >= 0) && (nFolderDataSize <= CAB_MAX_FOLDER_SIZE);

        // STORE blocks are byte-for-byte and every CAB data block represents
        // at most 32 KiB of uncompressed folder data.
        qint64 nBlockOffset = cfFolder.coffCabStart;
        QList<qint64> listDataOffsets;
        for (quint16 nBlock = 0; bFolderStreamValid && (nBlock < cfFolder.cCFData); nBlock++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return cabFailUnpackInit(&guardedThis, pState, pContext);
            }

            CFDATA cfData = {};
            if (!_readCFDataExact(nBlockOffset, &cfData, pPdStruct)) {
                if (!guardedThis) return false;
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return cabFailUnpackInit(&guardedThis, pState, pContext);
                }
                bFolderStreamValid = false;
                break;
            }
            listDataOffsets.append(nBlockOffset);
            const bool bContinuesToNext = (i + 1 == cfHeader.cFolders) && (nBlock + 1 == cfFolder.cCFData) && (cfData.cbUncomp == 0) && ((cfHeader.flags & 0x0002) != 0);
            const bool bContinuesFromPrevious = (i == 0) && (nBlock == 0) && ((cfHeader.flags & 0x0001) != 0);
            if ((cfData.cbUncomp > 32768) || ((nBlock + 1 < cfFolder.cCFData) && (cfData.cbUncomp != 32768)) || ((cfData.cbUncomp == 0) && !bContinuesToNext) ||
                ((nCompressionType == 0) && (cfData.cbData != cfData.cbUncomp) && !bContinuesToNext && !bContinuesFromPrevious) ||
                ((nCompressionType == 1) && (cfData.cbData < 2) && !bContinuesToNext && !bContinuesFromPrevious)) {
                bFolderStreamValid = false;
                break;
            }

            if (cfData.csum != 0) {
                QByteArray baHeaderAndReserve = read_array_process(nBlockOffset + 4, 4 + pContext->nCbCFData, pPdStruct);
                if (!guardedThis) return false;
                QByteArray baPayload = read_array_process(nBlockOffset + (qint64)sizeof(CFDATA) + pContext->nCbCFData, cfData.cbData, pPdStruct);
                if (!guardedThis) return false;
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return cabFailUnpackInit(&guardedThis, pState, pContext);
                }
                if ((baHeaderAndReserve.size() != 4 + pContext->nCbCFData) || (baPayload.size() != cfData.cbData)) {
                    bFolderStreamValid = false;
                    break;
                }

                quint32 nCalculatedChecksum = cabDataChecksum(baPayload.constData(), baPayload.size());
                nCalculatedChecksum = cabDataChecksum(baHeaderAndReserve.constData(), baHeaderAndReserve.size(), nCalculatedChecksum);
                if (nCalculatedChecksum != cfData.csum) {
                    bFolderStreamValid = false;
                    break;
                }
            }

            nBlockOffset += (qint64)sizeof(CFDATA) + pContext->nCbCFData + cfData.cbData;
        }

        if (!bFolderStreamValid) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        if ((nStreamSize > nCabinetSize - nAggregateStreamSize) || ((nStreamSize > 0) && setNonemptyStreamStarts.contains((qint64)cfFolder.coffCabStart))) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        nAggregateStreamSize += nStreamSize;
        if (nStreamSize > 0) {
            setNonemptyStreamStarts.insert((qint64)cfFolder.coffCabStart);
        }

        pContext->listFolders.append(cfFolder);
        pContext->listFolderOffsets.append(nFolderOffset);
        pContext->mapFolderDataOffsets.insert(i, listDataOffsets);
        pContext->mapFolderStreamSizes.insert(i, nStreamSize);
        pContext->mapFolderDataSizes.insert(i, nFolderDataSize);
        nFolderOffset += nFolderStructSize;
    }

    // CFFILE entries and their variable-length names must end before the
    // earliest non-empty folder stream.  This keeps directory-only parsing
    // from interpreting compressed bytes as additional metadata.
    qint64 nFileTableLimit = nCabinetSize;
    for (const CFFOLDER &folder : qAsConst(pContext->listFolders)) {
        if (folder.cCFData != 0) {
            if ((qint64)folder.coffCabStart > nCabinetSize) {
                return cabFailUnpackInit(&guardedThis, pState, pContext);
            }
            nFileTableLimit = qMin(nFileTableLimit, (qint64)folder.coffCabStart);
        }
    }

    // Parse file offsets starting at coffFiles
    qint64 nFileOffset = cfHeader.coffFiles;
    if ((nFileOffset < nFolderOffset) || (nFileOffset > nFileTableLimit) || ((cfHeader.cFiles > 0) && (nFileOffset == nFileTableLimit))) {
        return cabFailUnpackInit(&guardedThis, pState, pContext);
    }

    quint32 nPreviousLogicalFolder = 0;
    quint32 nPreviousFolderOffset = 0;
    bool bHavePreviousFile = false;
    quint32 nPreviousNonemptyLogicalFolder = 0;
    quint64 nPreviousNonemptyStart = 0;
    quint64 nPreviousNonemptyEnd = 0;
    bool bHavePreviousNonemptyFile = false;
    for (quint16 i = 0; i < cfHeader.cFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        if ((qint64)sizeof(CFFILE) > nFileTableLimit - nFileOffset) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        CFFILE cfFile = {};
        if (!_readCFFILEExact(nFileOffset, &cfFile, pPdStruct)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        if (cfFile.cbFile > 0x7FFF8000U) return cabFailUnpackInit(&guardedThis, pState, pContext);
        qint64 nNameOffset = nFileOffset + sizeof(CFFILE);
        QByteArray baFileName;
        if (!cabReadCabString(this, &guardedThis, nFileTableLimit, pPdStruct, &nNameOffset, &baFileName, 257) || baFileName.isEmpty()) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        QString sFileName;
        if (cfFile.attribs & 0x0080) {
            sFileName = QString::fromUtf8(baFileName);
            if (sFileName.toUtf8() != baFileName) {
                return cabFailUnpackInit(&guardedThis, pState, pContext);
            }
        } else {
            sFileName = QString::fromLocal8Bit(baFileName);
        }

        const bool bSpecialFolder = cfFile.iFolder >= 0xFFFD;
        if (!bSpecialFolder && (cfFile.iFolder < (quint16)pContext->listFolders.size())) {
            qint64 nFileEnd = (qint64)cfFile.uoffFolderStart + (qint64)cfFile.cbFile;
            const bool bStreamUnvalidated = pContext->setUnvalidatedFolderStreams.contains(cfFile.iFolder);
            const qint64 nMaximumFolderExtent = (qint64)pContext->listFolders.at(cfFile.iFolder).cCFData * 32768;
            const bool bFolderContinuesFromPrevious = ((cfHeader.flags & 0x0001) != 0) && (cfFile.iFolder == 0);
            if ((nFileEnd > CAB_MAX_FOLDER_SIZE) || (bStreamUnvalidated && !bFolderContinuesFromPrevious && (nFileEnd > nMaximumFolderExtent)) ||
                (!bStreamUnvalidated && (nFileEnd > pContext->mapFolderDataSizes.value(cfFile.iFolder, -1)))) {
                return cabFailUnpackInit(&guardedThis, pState, pContext);
            }
            pContext->mapFolderUncompressedSizes[cfFile.iFolder] = qMax(pContext->mapFolderUncompressedSizes.value(cfFile.iFolder, 0), nFileEnd);
        } else {
            const bool bHasPrevious = (cfHeader.flags & 0x0001) != 0;
            const bool bHasNext = (cfHeader.flags & 0x0002) != 0;
            const bool bSpecialFolderValid =
                !pContext->listFolders.isEmpty() && (((cfFile.iFolder == 0xFFFD) && bHasPrevious) || ((cfFile.iFolder == 0xFFFE) && bHasNext) ||
                                                     ((cfFile.iFolder == 0xFFFF) && bHasPrevious && bHasNext && (pContext->listFolders.size() == 1)));
            if (!bSpecialFolderValid) return cabFailUnpackInit(&guardedThis, pState, pContext);
        }

        quint32 nLogicalFolder = cfFile.iFolder;
        if (cfFile.iFolder == 0xFFFD) nLogicalFolder = 0;
        else if (cfFile.iFolder == 0xFFFE) {
            nLogicalFolder = (quint32)pContext->listFolders.size() - 1;
        } else if (cfFile.iFolder == 0xFFFF) nLogicalFolder = 0;
        if (bHavePreviousFile &&
            ((nLogicalFolder < nPreviousLogicalFolder) || ((nLogicalFolder == nPreviousLogicalFolder) && (cfFile.uoffFolderStart < nPreviousFolderOffset)))) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        if (cfFile.cbFile != 0) {
            const quint64 nCurrentStart = cfFile.uoffFolderStart;
            const quint64 nCurrentEnd = nCurrentStart + cfFile.cbFile;
            if (bHavePreviousNonemptyFile && (nLogicalFolder == nPreviousNonemptyLogicalFolder) && (nCurrentStart < nPreviousNonemptyEnd) &&
                ((nCurrentStart != nPreviousNonemptyStart) || (nCurrentEnd != nPreviousNonemptyEnd))) {
                // CAB permits duplicate file entries to alias one exact
                // logical range, but a partial overlap makes extraction order
                // ambiguous and is rejected by established CAB readers.
                return cabFailUnpackInit(&guardedThis, pState, pContext);
            }
            nPreviousNonemptyLogicalFolder = nLogicalFolder;
            nPreviousNonemptyStart = nCurrentStart;
            nPreviousNonemptyEnd = nCurrentEnd;
            bHavePreviousNonemptyFile = true;
        }
        bool bEarlierIsNormalOrNext = false;
        if (i > 0) {
            const CFFILE previousFile = pContext->listFiles.constLast();
            bEarlierIsNormalOrNext = (previousFile.iFolder != 0xFFFD) && (previousFile.iFolder != 0xFFFF);
        }
        if (((cfFile.iFolder == 0xFFFD) || (cfFile.iFolder == 0xFFFF)) && bEarlierIsNormalOrNext) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        if (bHavePreviousFile && ((nPreviousLogicalFolder == (quint32)pContext->listFolders.size() - 1)) && (cfFile.iFolder != 0xFFFE) && (cfFile.iFolder != 0xFFFF) &&
            (pContext->listFiles.constLast().iFolder >= 0xFFFE)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        bHavePreviousFile = true;
        nPreviousLogicalFolder = nLogicalFolder;
        nPreviousFolderOffset = cfFile.uoffFolderStart;

        pContext->listFileOffsets.append(nFileOffset);
        pContext->listFiles.append(cfFile);
        pContext->listFileEnds.append(nNameOffset);
        pContext->listFileNames.append(sFileName);
        nFileOffset = nNameOffset;
    }

    const qint64 nMetadataEnd = qMax(nFolderOffset, nFileOffset);

    // The variable-length file table must end before every non-empty folder
    // stream, and folder streams may not overlap one another.
    QList<QPair<qint64, qint64> > listStreamRanges;
    QList<QPair<qint64, qint64> > listUnvalidatedStreamMinimums;
    for (qint32 i = 0; i < pContext->listFolders.size(); ++i) {
        const CFFOLDER &folder = pContext->listFolders.at(i);
        const qint64 nStreamOffset = folder.coffCabStart;
        if (pContext->setUnvalidatedFolderStreams.contains((quint16)i)) {
            if ((nStreamOffset < nMetadataEnd) || (nStreamOffset > nCabinetSize) || ((folder.cCFData != 0) && (nStreamOffset >= nCabinetSize))) {
                return cabFailUnpackInit(&guardedThis, pState, pContext);
            }
            if (folder.cCFData != 0) {
                const qint64 nMinimumStreamSize = (qint64)folder.cCFData * ((qint64)sizeof(CFDATA) + pContext->nCbCFData + 1);
                listUnvalidatedStreamMinimums.append(qMakePair(nStreamOffset, nMinimumStreamSize));
            }
            continue;
        }
        const qint64 nStreamSize = pContext->mapFolderStreamSizes.value((quint16)i, -1);
        if ((nStreamSize < 0) || (nStreamOffset < nMetadataEnd)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
        if (nStreamSize > 0) {
            listStreamRanges.append(qMakePair(nStreamOffset, nStreamOffset + nStreamSize));
        }
    }

    std::sort(listStreamRanges.begin(), listStreamRanges.end(), cabStreamRangeLessThan);
    for (qint32 i = 1; i < listStreamRanges.size(); ++i) {
        if (listStreamRanges.at(i).first < listStreamRanges.at(i - 1).second) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
    }

    // Without reading CFDATA, each non-empty folder still needs a distinct
    // physical start and enough space for its declared fixed-size block
    // headers before the next folder stream (or the end of the cabinet).
    std::sort(listUnvalidatedStreamMinimums.begin(), listUnvalidatedStreamMinimums.end(), cabStreamRangeLessThan);
    for (qint32 i = 0; i < listUnvalidatedStreamMinimums.size(); ++i) {
        const qint64 nStreamOffset = listUnvalidatedStreamMinimums.at(i).first;
        const qint64 nMinimumStreamSize = listUnvalidatedStreamMinimums.at(i).second;
        const qint64 nStreamLimit = (i + 1 < listUnvalidatedStreamMinimums.size()) ? listUnvalidatedStreamMinimums.at(i + 1).first : nCabinetSize;
        if (((i > 0) && (nStreamOffset == listUnvalidatedStreamMinimums.at(i - 1).first)) || (nStreamLimit < nStreamOffset) ||
            (nMinimumStreamSize > nStreamLimit - nStreamOffset)) {
            return cabFailUnpackInit(&guardedThis, pState, pContext);
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return cabFailUnpackInit(&guardedThis, pState, pContext);
    const bool bSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis) return false;
    if (!bSnapshotCurrent) {
        return cabFailUnpackInit(&guardedThis, pState, pContext);
    }
    // Initialize state
    pState->nCurrentOffset = cfHeader.coffFiles;
    pState->nTotalSize = nCabinetSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listFiles.size();
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        return cabFailUnpackInit(&guardedThis, pState, pContext);
    }

    return true;
}

XBinary::ARCHIVERECORD XCab::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
    const bool bSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis || !bSnapshotCurrent) {
        return result;
    }
    if (pState->nCurrentIndex >= pContext->listFiles.size()) return result;
    const CFFILE cfFile = pContext->listFiles.at(pState->nCurrentIndex);
    QString sFileName = pContext->listFileNames.value(pState->nCurrentIndex);

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)cfFile.cbFile);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)(cfFile.attribs & ~0x0080U));

    const QDateTime dateTime = XBinary::dosDateTimeToQDateTime(cfFile.date, cfFile.time);
    if (dateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
    }

    if ((cfFile.iFolder < 0xFFFD) && (cfFile.iFolder < (quint16)pContext->listFolders.size())) {
        CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);
        const bool bStreamValidated = !pContext->setUnvalidatedFolderStreams.contains(cfFile.iFolder);

        result.nStreamOffset = cfFolder.coffCabStart;
        result.nStreamSize = bStreamValidated ? pContext->mapFolderStreamSizes.value(cfFile.iFolder, -1) : 0;

        if (bStreamValidated) {
            result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);
            result.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE,
                                        pContext->mapFolderDataSizes.value(cfFile.iFolder, (qint64)cfFile.uoffFolderStart + cfFile.cbFile));
        }
        result.mapProperties.insert(FPART_PROP_TYPE, (quint32)cfFolder.typeCompress);
        result.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)cfFile.iFolder);
        result.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, (qint64)cfFile.uoffFolderStart);
        result.mapProperties.insert(FPART_PROP_OPTHEADER_OFFSET, (qint64)cfFile.uoffFolderStart);
        result.mapProperties.insert(FPART_PROP_OPTHEADER_SIZE, (qint64)pContext->nCbCFData);
        // Set compression method
        quint16 nCompressType = cfFolder.typeCompress & 0x000F;

        if (nCompressType == 0x0000) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
        } else if (nCompressType == 0x0001) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
        } else if (nCompressType == 0x0003) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
            // LZX window size is stored in the upper bits of typeCompress: (typeCompress >> 8) & 0x1F
            result.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((cfFolder.typeCompress >> 8) & 0x1F));
        } else if (nCompressType == 0x0002) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_QUANTUM_CAB);
            result.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((cfFolder.typeCompress >> 8) & 0x1F));
        } else {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
        }
    }

    const bool bFinalSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bFinalSourceCurrent) {
        return XBinary::ARCHIVERECORD();
    }
    return result;
}

bool XCab::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || !guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const bool bOutputSupported = isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedOutput || !bOutputSupported) return false;

    CAB_UNPACK_CONTEXT *pContext = static_cast<CAB_UNPACK_CONTEXT *>(pState->pContext);
    QPointer<QIODevice> guardedSource(pContext->sourceSnapshot.pSourceDevice.data());
    if (!guardedSource || !guardedOutput) return false;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;
    const bool bSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis || !bSnapshotCurrent || !guardedSource || !guardedOutput) return false;
    const bool bAliases = cabDevicesAlias(guardedSource.data(), guardedOutput.data());
    if (!guardedThis || !guardedSource || !guardedOutput || bAliases || (pState->nCurrentIndex >= pContext->listFiles.size())) return false;

    const CFFILE cfFile = pContext->listFiles.at(pState->nCurrentIndex);
    if ((cfFile.iFolder >= 0xFFFD) || (cfFile.iFolder >= (quint16)pContext->listFolders.size())) {
        // Multi-cabinet spanning records require neighboring cabinet streams
        // and cannot be decoded atomically by this single-device context.
        return false;
    }

    const bool bRecordSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis || !bRecordSnapshotCurrent) return false;

    const bool bRecordSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bRecordSourceCurrent) return false;
    const bool bSecondSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis || !bSecondSnapshotCurrent || !guardedOutput) return false;

    const quint16 nFolderIndex = cfFile.iFolder;
    if (pContext->setUnvalidatedFolderStreams.contains(nFolderIndex)) {
        return false;
    }
    if (!pContext->mapFolderCache.contains(nFolderIndex)) {
        const CFFOLDER &folder = pContext->listFolders.at(nFolderIndex);
        const qint64 nStreamSize = pContext->mapFolderStreamSizes.value(nFolderIndex, -1);
        const qint64 nFolderSize = pContext->mapFolderDataSizes.value(nFolderIndex, -1);
        if ((nStreamSize < 0) || (nFolderSize < 0) || (nFolderSize > CAB_MAX_FOLDER_SIZE) ||
            !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nFolderSize))
            return false;

        FPART streamPart = {};
        streamPart.filePart = FILEPART_STREAM;
        streamPart.nFileOffset = folder.coffCabStart;
        streamPart.nFileSize = nStreamSize;
        streamPart.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nStreamSize);
        streamPart.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nFolderSize);
        streamPart.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, nFolderSize);
        streamPart.mapProperties.insert(FPART_PROP_OPTHEADER_SIZE, (qint64)pContext->nCbCFData);
        const quint16 nCompressionType = folder.typeCompress & 0x000F;
        if (nCompressionType == 0) {
            streamPart.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
        } else if (nCompressionType == 1) {
            streamPart.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
        } else if (nCompressionType == 3) {
            streamPart.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
            streamPart.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((folder.typeCompress >> 8) & 0x1F));
        } else if (nCompressionType == 2) {
            streamPart.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_QUANTUM_CAB);
            streamPart.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((folder.typeCompress >> 8) & 0x1F));
        } else {
            return false;
        }

        QByteArray baFolderData;
        QBuffer folderBuffer(&baFolderData);
        if (!folderBuffer.open(QIODevice::ReadWrite)) return false;
        XDecompress decompressor;
        const bool bDecoded = decompressor.decompressFPART(streamPart, guardedSource.data(), &folderBuffer, pState->mapUnpackProperties, pPdStruct);
        folderBuffer.close();
        if (!guardedThis || !guardedSource || !bDecoded || !XBinary::isPdStructNotCanceled(pPdStruct) || (baFolderData.size() != nFolderSize)) {
            return false;
        }
        const bool bDecodedSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
        if (!guardedThis || !bDecodedSnapshotCurrent) {
            return false;
        }
        // Retain only the active solid folder.  CFFILE entries are ordered by
        // folder, so this prevents aggregate cache growth without re-decoding
        // adjacent files from the same folder.
        pContext->mapFolderCache.clear();
        pContext->mapFolderCache.insert(nFolderIndex, baFolderData);
    }

    const QByteArray &baFolderData = pContext->mapFolderCache.value(nFolderIndex);
    const qint64 nSubstreamOffset = cfFile.uoffFolderStart;
    const qint64 nSubstreamSize = cfFile.cbFile;
    if ((nSubstreamOffset < 0) || (nSubstreamSize < 0) || (nSubstreamOffset > baFolderData.size()) || (nSubstreamSize > baFolderData.size() - nSubstreamOffset) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nSubstreamSize)) {
        return false;
    }
    // A callback/custom source can mutate or rebind the archive after the
    // initial preflight.  Never publish cached or freshly decoded bytes from a
    // stale context.
    const bool bPreStageSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bPreStageSourceCurrent) return false;
    const bool bPreStageSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis || !bPreStageSnapshotCurrent || !guardedOutput) return false;

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here.  Produced bytes are charged by _writeDevice through
    // writeState.spOutputBudget; the folder-cache decode and the
    // publishUnpackOutput copy are never charged.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->listFileNames.value(pState->nCurrentIndex))) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(nSubstreamSize, pPdStruct));
    if (!guardedThis || !pStage || !guardedOutput || !guardedSource) return false;
    const bool bStageSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bStageSourceCurrent) return false;
    DATAPROCESS_STATE writeState = {};
    writeState.mapUnpackProperties = pState->mapUnpackProperties;
    writeState.spOutputBudget = pState->spOutputBudget;
    writeState.pDeviceOutput = pStage.get();
    writeState.nProcessedLimit = -1;
    qint64 nWritten = 0;
    while ((nWritten < nSubstreamSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)qMin<qint64>(0x10000, nSubstreamSize - nWritten);
        const qint32 nResult = XBinary::_writeDevice(baFolderData.constData() + nSubstreamOffset + nWritten, nChunk, &writeState);
        if (!guardedOutput || (nResult != nChunk)) break;
        nWritten += nResult;
    }
    if ((nWritten != nSubstreamSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bFinalSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bFinalSourceCurrent) return false;
    const bool bFinalSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
    if (!guardedThis || !bFinalSnapshotCurrent || !guardedOutput) return false;
    const bool bPublished = publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (!guardedThis || !bPublished) return false;
    pState->nCurrentOffset = nSubstreamSize;
    return true;
}

// bool XCab::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
// {
//     if (!pState || !pState->pContext || !pDevice) {
//         return false;
//     }

//     if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
//         return false;
//     }

//     PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
//     if (!pPdStruct) {
//         pPdStruct = &pdStructEmpty;
//     }

//     CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
//     qint64 nFileOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);

//     CFFILE cfFile = readCFFILE(nFileOffset);

//     if (cfFile.iFolder >= (quint16)pContext->listFolders.size()) {
//         return false;
//     }

//     CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

//     // Determine compression type (lower 4 bits)
//     quint16 nCompressionType = cfFolder.typeCompress & 0x000F;
//     qint64 nTotalSize = getSize();
//     // Per-datablock reserved area size
//     qint64 nDataReservedSize = (qint64)pContext->nCbCFData;

//     if (nCompressionType == 0x0000) {
//         // STORE method (no compression) - direct copy from data blocks
//         qint64 nDataOffset = cfFolder.coffCabStart;
//         qint64 nCurrentUncompressedOffset = 0;
//         qint64 nTargetOffset = cfFile.uoffFolderStart;
//         qint64 nRemainingSize = cfFile.cbFile;
//         qint64 nWriteOffset = 0;

//         for (quint16 i = 0; (i < cfFolder.cCFData) && (nRemainingSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
//             if ((nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize) > nTotalSize) {
//                 break;
//             }

//             CFDATA cfData = readCFData(nDataOffset);
//             qint64 nPayloadOffset = nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize;

//             if (nCurrentUncompressedOffset + cfData.cbUncomp <= nTargetOffset) {
//                 // Skip this block entirely
//                 nCurrentUncompressedOffset += cfData.cbUncomp;
//                 nDataOffset = nPayloadOffset + cfData.cbData;
//                 continue;
//             }

//             // This block contains data we need
//             qint64 nBlockStart = nTargetOffset - nCurrentUncompressedOffset;
//             qint64 nCopySize = qMin(nRemainingSize, (qint64)cfData.cbUncomp - nBlockStart);

//             if (nCopySize > 0) {
//                 qint64 nSourceOffset = nPayloadOffset + nBlockStart;

//                 if ((nSourceOffset + nCopySize) > nTotalSize) {
//                     return false;
//                 }

//                 if (!copyDeviceMemory(getDevice(), nSourceOffset, pDevice, nWriteOffset, nCopySize)) {
//                     return false;
//                 }

//                 nWriteOffset += nCopySize;
//             }

//             nTargetOffset += nCopySize;
//             nRemainingSize -= nCopySize;
//             nCurrentUncompressedOffset += cfData.cbUncomp;
//             nDataOffset = nPayloadOffset + cfData.cbData;
//         }

//         return (nRemainingSize == 0);
//     } else if (nCompressionType == 0x0001) {
//         // MSZIP method (DEFLATE with 2-byte "CK" signature per block)
//         // Use folder cache to avoid re-decompressing the same folder for multiple files
//         quint16 nFolderIndex = cfFile.iFolder;

//         if (!pContext->mapFolderCache.contains(nFolderIndex)) {
//             // Decompress entire folder into cache
//             QByteArray baFolderData;

//             qint64 nDataOffset = cfFolder.coffCabStart;

//             for (quint16 i = 0; (i < cfFolder.cCFData) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
//                 if ((nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize) > nTotalSize) {
//                     break;
//                 }

//                 CFDATA cfData = readCFData(nDataOffset);
//                 qint64 nPayloadOffset = nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize;

//                 if (cfData.cbData < 2) {
//                     return false;  // Invalid block (needs at least 2-byte "CK" signature)
//                 }

//                 // Verify MSZIP signature "CK" (0x43 0x4B)
//                 quint8 nSig0 = read_uint8(nPayloadOffset);
//                 quint8 nSig1 = read_uint8(nPayloadOffset + 1);

//                 if (nSig0 != 0x43 || nSig1 != 0x4B) {
//                     return false;  // Invalid MSZIP block signature
//                 }

//                 qint64 nCompressedDataOffset = nPayloadOffset + 2;
//                 qint64 nCompressedDataSize = cfData.cbData - 2;

//                 if (nCompressedDataSize <= 0) {
//                     return false;
//                 }

//                 if ((nCompressedDataOffset + nCompressedDataSize) > nTotalSize) {
//                     return false;
//                 }

//                 QByteArray baCompressedData = read_array(nCompressedDataOffset, nCompressedDataSize);

//                 if (baCompressedData.size() != nCompressedDataSize) {
//                     return false;
//                 }

//                 QBuffer bufferCompressed(&baCompressedData);
//                 if (!bufferCompressed.open(QIODevice::ReadOnly)) {
//                     return false;
//                 }

//                 QByteArray baUncompressedBlock;
//                 QBuffer bufferUncompressed(&baUncompressedBlock);
//                 if (!bufferUncompressed.open(QIODevice::WriteOnly)) {
//                     bufferCompressed.close();
//                     return false;
//                 }

//                 DATAPROCESS_STATE decompressState = {};
//                 decompressState.pDeviceInput = &bufferCompressed;
//                 decompressState.pDeviceOutput = &bufferUncompressed;
//                 decompressState.nInputOffset = 0;
//                 decompressState.nInputLimit = nCompressedDataSize;
//                 decompressState.nProcessedOffset = 0;
//                 decompressState.nProcessedLimit = cfData.cbUncomp;

//                 bool bDecompressResult = XDeflateDecoder::decompress(&decompressState, pPdStruct);

//                 bufferCompressed.close();
//                 bufferUncompressed.close();

//                 if (!bDecompressResult) {
//                     return false;
//                 }

//                 if (baUncompressedBlock.size() != (qint32)cfData.cbUncomp) {
//                     return false;
//                 }

//                 baFolderData.append(baUncompressedBlock);
//                 nDataOffset = nPayloadOffset + cfData.cbData;
//             }

//             pContext->mapFolderCache.insert(nFolderIndex, baFolderData);
//         }

//         // Extract file data from the cached folder
//         const QByteArray &baFolderData = pContext->mapFolderCache.value(nFolderIndex);

//         if (baFolderData.size() < (qint64)cfFile.uoffFolderStart + (qint64)cfFile.cbFile) {
//             return false;
//         }

//         qint64 nBytesWritten = pDevice->write(baFolderData.constData() + cfFile.uoffFolderStart, cfFile.cbFile);
//         return (nBytesWritten == (qint64)cfFile.cbFile);
//     } else if (nCompressionType == 0x0003) {
//         // LZX method - not yet implemented (requires specialized LZX decoder)
//         // LZX window size is encoded in upper bits: (typeCompress >> 8) & 0x1F
//         return false;
//     }

//     return false;  // Unsupported compression type
// }

bool XCab::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    bool bResult = false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (XBinary::isPdStructNotCanceled(pPdStruct) && pState && pState->pContext && (pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
        const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
        if (!guardedThis || !bSourceCurrent) return false;
        const bool bSnapshotCurrent = isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, getDevice(), pPdStruct);
        if (!guardedThis || !bSnapshotCurrent) return false;

        pState->nCurrentIndex++;

        if (pState->nCurrentIndex < pState->nNumberOfRecords) {
            pState->nCurrentOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);
            bResult = true;
        }
    }

    return bResult;
}

bool XCab::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    CAB_UNPACK_CONTEXT *pContext = static_cast<CAB_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    delete pContext;
    Q_UNUSED(guardedThis)
    return true;
}

quint64 XCab::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return 0;

    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> mapProperties = getDefaultUnpackProperties();
    mapProperties.insert(UNPACK_PROP_METADATAONLY, true);

    if (!initUnpack(&state, mapProperties, pPdStruct)) return 0;

    const bool bStateValid = (state.nCurrentIndex == 0) && (state.nNumberOfRecords >= 0) && (state.nCurrentIndex <= state.nNumberOfRecords);
    const quint64 nResult = bStateValid ? (quint64)state.nNumberOfRecords : 0;
    const bool bFinished = finishUnpack(&state, nullptr);

    return (bStateValid && bFinished && XBinary::isPdStructNotCanceled(pPdStruct)) ? nResult : 0;
}

QList<QString> XCab::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'MSCF'");
    return listResult;
}

XBinary *XCab::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCab(pDevice);
}

bool XCab::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XCab::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCab> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XCab::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
