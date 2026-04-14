// DIE's modification 2017-2025
// Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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
#include "xdmg.h"
#include <QBuffer>
#include <QRegularExpression>
#include <zlib.h>

XBinary::XCONVERT _TABLE_XDMG_STRUCTID[] = {
    {XDMG::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XDMG::STRUCTID_KOLY_BLOCK, "KOLY_BLOCK", QString("Koly block")},
    {XDMG::STRUCTID_MISH_BLOCK, "MISH_BLOCK", QString("Mish block")},
    {XDMG::STRUCTID_STRIPE, "STRIPE", QString("Stripe")},
};

XDMG::XDMG(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDMG::~XDMG()
{
}

bool XDMG::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    qint64 nSize = getSize();
    if (nSize >= 512) {
        KOLY_BLOCK kolyBlock = readKolyBlock(getDevice(), nSize - 512);

        if ((kolyBlock.nMagic == 0x6b6f6c79) && (kolyBlock.nHeaderLength == 512)) {  // "koly"
            bool bXmlRangeOk = true;

            if (kolyBlock.nXmlLength > 0) {
                bXmlRangeOk = ((kolyBlock.nXmlOffset >= 0) && (kolyBlock.nXmlOffset + kolyBlock.nXmlLength <= (quint64)nSize));
            }

            if (bXmlRangeOk) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XDMG::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDMG xdmg(pDevice);
    return xdmg.isValid();
}

QString XDMG::getArch()
{
    return QString("");
}

XBinary::FT XDMG::getFileType()
{
    return FT_DMG;
}

QString XDMG::getFileFormatExt()
{
    return QString("dmg");
}

QString XDMG::getFileFormatExtsString()
{
    return QString("*.dmg");
}

qint64 XDMG::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

QString XDMG::getVersion()
{
    QString sResult;

    qint64 nSize = getSize();
    if (nSize > 512) {
        KOLY_BLOCK kolyBlock = readKolyBlock(getDevice(), nSize - 512);
        sResult = QString::number(kolyBlock.nVersion);
    }

    return sResult;
}

XBinary::MODE XDMG::getMode()
{
    return MODE_UNKNOWN;
}

XBinary::ENDIAN XDMG::getEndian()
{
    return ENDIAN_BIG;
}

QString XDMG::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XDMG_STRUCTID, sizeof(_TABLE_XDMG_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XDMG::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XDMG_STRUCTID, sizeof(_TABLE_XDMG_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XDMG::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XDMG_STRUCTID, sizeof(_TABLE_XDMG_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::MAPMODE> XDMG::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);

    return listResult;
}

XBinary::_MEMORY_MAP XDMG::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_REGIONS;
    }

    qint64 nSize = getSize();

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return result;
}

quint64 XDMG::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    qint64 nSize = getSize();
    if (nSize > 512) {
        KOLY_BLOCK kolyBlock = readKolyBlock(getDevice(), nSize - 512);

        if (kolyBlock.nMagic == 0x6b6f6c79 && kolyBlock.nXmlLength > 0) {
            QByteArray baXml = read_array(kolyBlock.nXmlOffset, kolyBlock.nXmlLength);

            QList<DMG_PARTITION_INFO> listPartitions = _parseBlkxPartitions(baXml, pPdStruct);

            for (qint32 i = 0; i < listPartitions.count(); i++) {
                if (!isPdStructNotCanceled(pPdStruct)) {
                    break;
                }

                QByteArray baMishData = listPartitions.at(i).mishData;
                QBuffer buffer(&baMishData);
                if (buffer.open(QIODevice::ReadOnly)) {
                    MISH_BLOCK mishBlock = readMishBlock(&buffer, 0);
                    if (mishBlock.nMagic == 0x6d697368) {
                        nResult++;
                    }
                }
            }
        }
    }

    return nResult;
}

QList<XArchive::RECORD> XDMG::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;

    qint64 nSize = getSize();
    if (nSize > 512) {
        KOLY_BLOCK kolyBlock = readKolyBlock(getDevice(), nSize - 512);

        if (kolyBlock.nMagic == 0x6b6f6c79 && kolyBlock.nXmlLength > 0) {
            QByteArray baXml = read_array(kolyBlock.nXmlOffset, kolyBlock.nXmlLength);

            QList<DMG_PARTITION_INFO> listPartitions = _parseBlkxPartitions(baXml, pPdStruct);
            qint32 nRecordIndex = 0;

            for (qint32 i = 0; (i < listPartitions.count()) && isPdStructNotCanceled(pPdStruct); i++) {
                if (nLimit != -1 && nRecordIndex >= nLimit) {
                    break;
                }

                QByteArray baMishData = listPartitions.at(i).mishData;
                QBuffer buffer(&baMishData);
                if (!buffer.open(QIODevice::ReadOnly)) {
                    continue;
                }

                MISH_BLOCK mishBlock = readMishBlock(&buffer, 0);
                if (mishBlock.nMagic != 0x6d697368) {
                    continue;
                }

                RECORD record = {};
                QString sName = listPartitions.at(i).sName;
                if (sName.isEmpty()) {
                    sName = QString("partition%1").arg(nRecordIndex);
                }

                record.spInfo.sRecordName = sName;
                record.spInfo.nUncompressedSize = mishBlock.nSectorCount * 512;
                record.spInfo.compressMethod = HANDLE_METHOD_UNKNOWN;
                record.nDataOffset = 0;
                record.nDataSize = 0;

                listResult.append(record);
                nRecordIndex++;
            }
        }
    }

    return listResult;
}

bool XDMG::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapOptions)

    bool bResult = false;

    if (pState) {
        DMG_UNPACK_CONTEXT *pContext = new DMG_UNPACK_CONTEXT;
        pContext->pSourceDevice = getDevice();
        pContext->nDataForkOffset = 0;
        pContext->nCurrentMishIndex = 0;
        pContext->nCurrentStripeIndex = 0;
        pContext->nCurrentFileIndex = 0;

        qint64 nSize = getSize();
        if (nSize > 512) {
            KOLY_BLOCK kolyBlock = readKolyBlock(getDevice(), nSize - 512);

            if (kolyBlock.nMagic == 0x6b6f6c79 && kolyBlock.nXmlLength > 0) {
                pContext->baXmlData = read_array(kolyBlock.nXmlOffset, kolyBlock.nXmlLength);

                pContext->nDataForkOffset = kolyBlock.nDataForkOffset;

                QList<DMG_PARTITION_INFO> listPartitions = _parseBlkxPartitions(pContext->baXmlData, pPdStruct);

                for (qint32 i = 0; (i < listPartitions.count()) && isPdStructNotCanceled(pPdStruct); i++) {
                    QByteArray baMishData = listPartitions.at(i).mishData;
                    QBuffer buffer(&baMishData);
                    if (!buffer.open(QIODevice::ReadOnly)) {
                        continue;
                    }

                    MISH_BLOCK mishBlock = readMishBlock(&buffer, 0);
                    if (mishBlock.nMagic != 0x6d697368) {
                        continue;
                    }

                    qint64 nExpectedSize = 204 + ((qint64)mishBlock.nBlockDataCount) * 40;
                    if (listPartitions.at(i).mishData.size() < nExpectedSize) {
                        continue;
                    }

                    QList<BLOCK_DATA> listCurrentStripes;
                    for (qint32 j = 0; j < (qint32)mishBlock.nBlockDataCount; j++) {
                        BLOCK_DATA stripe = readBlockData(&buffer, 204 + ((qint64)j) * 40);
                        listCurrentStripes.append(stripe);
                    }

                    pContext->listMishBlocks.append(mishBlock);
                    pContext->listStripes.append(listCurrentStripes);
                    pContext->listPartitionNames.append(listPartitions.at(i).sName);
                }

                pState->nNumberOfRecords = pContext->listMishBlocks.count();
                pState->nCurrentIndex = 0;
                pState->pContext = pContext;
                bResult = (pState->nNumberOfRecords > 0);
            }
        }

        if (!bResult) {
            delete pContext;
        }
    }

    return bResult;
}

XArchive::ARCHIVERECORD XDMG::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (pState && pState->pContext) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

        if ((pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pContext->listMishBlocks.count())) {
            QString sPartitionName;
            if (pState->nCurrentIndex < pContext->listPartitionNames.count()) {
                sPartitionName = pContext->listPartitionNames.at(pState->nCurrentIndex);
            }

            if (sPartitionName.isEmpty()) {
                sPartitionName = QString("partition%1").arg(pContext->nCurrentFileIndex);
            }

            qint64 nUncompressedSize = pContext->listMishBlocks.at(pState->nCurrentIndex).nSectorCount * 512;

            result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sPartitionName + ".img");
            result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
            result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, 0);
            result.mapProperties.insert(FPART_PROP_EXT, "img");
        }

        result.nStreamOffset = 0;
        result.nStreamSize = 0;
    }

    return result;
}

bool XDMG::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState && pState->pContext && pDevice) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

        if ((pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pContext->listStripes.count())) {
            QList<BLOCK_DATA> listCurrentStripes = pContext->listStripes.at(pState->nCurrentIndex);
            bResult = true;

            for (qint32 i = 0; (i < listCurrentStripes.count()) && bResult && isPdStructNotCanceled(pPdStruct); i++) {
                const BLOCK_DATA stripe = listCurrentStripes.at(i);

                if ((stripe.nType == DMG_STRIPE_END) || (stripe.nType == DMG_STRIPE_SKIP)) {
                    continue;
                }

                bResult = _decompressStripe(stripe, pContext->nDataForkOffset, pDevice, pPdStruct);
            }
        }
    }

    return bResult;
}

bool XDMG::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

        pContext->nCurrentFileIndex++;
        pState->nCurrentIndex++;

        if (pState->nCurrentIndex < pState->nNumberOfRecords) {
            bResult = true;
        }
    }

    return bResult;
}

bool XDMG::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
        bResult = true;
    }

    return bResult;
}

XDMG::KOLY_BLOCK XDMG::readKolyBlock(QIODevice *pDevice, qint64 nOffset)
{
    KOLY_BLOCK result = {};

    if (pDevice) {
        XBinary binary(pDevice);

        result.nMagic = binary.read_uint32(nOffset, true);
        result.nVersion = binary.read_uint32(nOffset + 4, true);
        result.nHeaderLength = binary.read_uint32(nOffset + 8, true);
        result.nFlags = binary.read_uint32(nOffset + 12, true);
        result.nRunningOffset = binary.read_uint64(nOffset + 16, true);
        result.nDataForkOffset = binary.read_uint64(nOffset + 24, true);
        result.nDataForkLength = binary.read_uint64(nOffset + 32, true);
        result.nResourceForkOffset = binary.read_uint64(nOffset + 40, true);
        result.nResourceForkLength = binary.read_uint64(nOffset + 48, true);
        result.nSegment = binary.read_uint32(nOffset + 56, true);
        result.nSegmentCount = binary.read_uint32(nOffset + 60, true);

        // Skip segmentID (16 bytes) and dataChecksum (136 bytes)
        result.nXmlOffset = binary.read_uint64(nOffset + 216, true);
        result.nXmlLength = binary.read_uint64(nOffset + 224, true);

        // Skip padding and masterChecksum
        result.nImageVariant = binary.read_uint32(nOffset + 488, true);
        result.nSectorCount = binary.read_uint64(nOffset + 492, true);
    }

    return result;
}

XDMG::MISH_BLOCK XDMG::readMishBlock(QIODevice *pDevice, qint64 nOffset)
{
    MISH_BLOCK result = {};

    if (pDevice) {
        XBinary binary(pDevice);

        result.nMagic = binary.read_uint32(nOffset, true);
        result.nVersion = binary.read_uint32(nOffset + 4, true);
        result.nStartSector = binary.read_uint64(nOffset + 8, true);
        result.nSectorCount = binary.read_uint64(nOffset + 16, true);
        result.nDataOffset = binary.read_uint64(nOffset + 24, true);
        result.nBufferCount = binary.read_uint32(nOffset + 32, true);
        result.nDescriptorBlocks = binary.read_uint32(nOffset + 36, true);
        // Skip reserved (24 bytes) and checksum (136 bytes)
        result.nBlockDataCount = binary.read_uint32(nOffset + 200, true);
    }

    return result;
}

XDMG::BLOCK_DATA XDMG::readBlockData(QIODevice *pDevice, qint64 nOffset)
{
    BLOCK_DATA result = {};

    if (pDevice) {
        XBinary binary(pDevice);

        result.nType = binary.read_uint32(nOffset, true);
        result.nReserved = binary.read_uint32(nOffset + 4, true);
        result.nStartSector = binary.read_uint64(nOffset + 8, true);
        result.nSectorCount = binary.read_uint64(nOffset + 16, true);
        result.nDataOffset = binary.read_uint64(nOffset + 24, true);
        result.nDataLength = binary.read_uint64(nOffset + 32, true);
    }

    return result;
}

QList<XDMG::DMG_PARTITION_INFO> XDMG::_parseBlkxPartitions(const QByteArray &baXml, PDSTRUCT *pPdStruct)
{
    QList<DMG_PARTITION_INFO> listResult;

    QString sXml = QString::fromUtf8(baXml);
    qint64 nCurrentNameIndex = 0;

    qint32 nBlkxPos = sXml.indexOf("<key>blkx</key>");
    if (nBlkxPos == -1) {
        return listResult;
    }

    qint32 nArrayStart = sXml.indexOf("<array>", nBlkxPos);
    if (nArrayStart == -1) {
        return listResult;
    }

    qint32 nArrayEnd = sXml.indexOf("</array>", nArrayStart);
    if (nArrayEnd == -1) {
        return listResult;
    }

    QString sArray = sXml.mid(nArrayStart, nArrayEnd - nArrayStart);

    QRegularExpression reDict("<dict>([\\s\\S]*?)</dict>");
    QRegularExpressionMatchIterator itDict = reDict.globalMatch(sArray);

    while (itDict.hasNext() && isPdStructNotCanceled(pPdStruct)) {
        QRegularExpressionMatch matchDict = itDict.next();
        QString sDict = matchDict.captured(1);

        QString sName;

        QRegularExpression reName("<key>Name</key>\\s*<string>([^<]*)</string>");
        QRegularExpressionMatch matchName = reName.match(sDict);
        if (matchName.hasMatch()) {
            sName = matchName.captured(1).trimmed();
        }

        if (sName.isEmpty()) {
            QRegularExpression reCFName("<key>CFName</key>\\s*<string>([^<]*)</string>");
            QRegularExpressionMatch matchCFName = reCFName.match(sDict);
            if (matchCFName.hasMatch()) {
                sName = matchCFName.captured(1).trimmed();
            }
        }

        if (sName.isEmpty()) {
            sName = QString("partition%1").arg(nCurrentNameIndex);
        }

        QRegularExpression reData("<key>Data</key>\\s*<data>([\\s\\S]*?)</data>");
        QRegularExpressionMatch matchData = reData.match(sDict);
        if (matchData.hasMatch()) {
            QString sBase64 = matchData.captured(1);
            sBase64.remove(QRegularExpression("\\s+"));

            QByteArray baMish = QByteArray::fromBase64(sBase64.toLatin1());
            if (!baMish.isEmpty()) {
                DMG_PARTITION_INFO info = {};
                info.sName = sName;
                info.mishData = baMish;
                listResult.append(info);
            }
        }

        nCurrentNameIndex++;
    }

    return listResult;
}

bool XDMG::_decompressStripe(const BLOCK_DATA &stripe, qint64 nDataForkOffset, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = true;
    qint64 nSectorSize = 512;  // DMG sector size
    qint64 nExpectedSize = stripe.nSectorCount * nSectorSize;

    switch (stripe.nType) {
        case DMG_STRIPE_EMPTY:
            // Empty stripe - no data to write
            break;

        case DMG_STRIPE_ZEROES:
            // Write zeroes
            bResult = _writeZeroes(pDevice, nExpectedSize);
            break;

        case DMG_STRIPE_STORED:
            // Uncompressed data - direct copy
            if (stripe.nDataLength > 0) {
                QByteArray baData = read_array(nDataForkOffset + stripe.nDataOffset, stripe.nDataLength);
                if (!baData.isEmpty()) {
                    qint64 nWritten = pDevice->write(baData);
                    if (nWritten != baData.size()) {
                        bResult = false;
                    }
                } else {
                    bResult = false;
                }
            }
            break;

        case DMG_STRIPE_DEFLATE:
            // ZLIB compressed data
            if (stripe.nDataLength > 0) {
                QByteArray baCompressed = read_array(nDataForkOffset + stripe.nDataOffset, stripe.nDataLength);
                if (!baCompressed.isEmpty()) {
                    z_stream strm = {};
                    strm.next_in = (Bytef *)baCompressed.data();
                    strm.avail_in = baCompressed.size();

                    if (inflateInit(&strm) == Z_OK) {
                        QByteArray baBuffer(16384, 0);  // 16KB buffer
                        qint64 nTotalWritten = 0;

                        while (strm.avail_in > 0 && nTotalWritten < nExpectedSize) {
                            strm.next_out = (Bytef *)baBuffer.data();
                            strm.avail_out = baBuffer.size();

                            qint32 nRet = inflate(&strm, Z_NO_FLUSH);
                            if (nRet != Z_OK && nRet != Z_STREAM_END) {
                                bResult = false;
                                break;
                            }

                            qint64 nHave = baBuffer.size() - strm.avail_out;
                            if (nHave > 0) {
                                qint64 nWritten = pDevice->write(baBuffer.constData(), nHave);
                                if (nWritten != nHave) {
                                    bResult = false;
                                    break;
                                }
                                nTotalWritten += nWritten;
                            }

                            if (nRet == Z_STREAM_END) {
                                break;
                            }
                        }

                        inflateEnd(&strm);
                    } else {
                        bResult = false;
                    }
                } else {
                    bResult = false;
                }
            }
            break;

        case DMG_STRIPE_BZ:
            // BZ2 compressed - not implemented yet
            // For now, write zeroes as placeholder
            bResult = _writeZeroes(pDevice, nExpectedSize);
            break;

        case DMG_STRIPE_ADC:
            // ADC compressed - not implemented yet
            // For now, write zeroes as placeholder
            bResult = _writeZeroes(pDevice, nExpectedSize);
            break;

        case DMG_STRIPE_SKIP:
        case DMG_STRIPE_END:
            // Control markers - no data
            break;

        default:
            // Unknown stripe type
            bResult = false;
            break;
    }

    return bResult;
}

bool XDMG::_writeZeroes(QIODevice *pDevice, qint64 nSize)
{
    bool bResult = true;

    QByteArray baZeroes(4096, 0);
    while (nSize > 0 && bResult) {
        qint64 nToWrite = qMin(nSize, (qint64)baZeroes.size());
        qint64 nWritten = pDevice->write(baZeroes.constData(), nToWrite);

        if (nWritten != nToWrite) {
            bResult = false;
        }

        nSize -= nWritten;
    }

    return bResult;
}

QList<QString> XDMG::getSearchSignatures()
{
    QList<QString> listResult;

    // UDIF DMG trailer signature.
    listResult.append("'koly'");

    return listResult;
}

XBinary *XDMG::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XDMG(pDevice);
}

