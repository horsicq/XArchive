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
#include "xace.h"

#include <cstring>
#include <new>

const quint8 XACE::MAGIC[7] = {0x2A, 0x2A, 0x41, 0x43, 0x45, 0x2A, 0x2A};

XBinary::XCONVERT _TABLE_XACE_STRUCTID[] = {
    {XACE::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XACE::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XACE::STRUCTID_RECORD, "RECORD", QString("Record")},
};

namespace {
const quint16 ACE1_MAX_FILENAME = 512;
const quint16 ACE1_MAX_COMMENT = 2048;
const quint16 ACE1_MAIN_MIN_HEAD_SIZE = 27;
const quint16 ACE1_FILE_MIN_HEAD_SIZE = 31;
const quint16 ACE1_RECOVERY_MIN_HEAD_SIZE = 28;
const qint32 ACE1_MAX_BLOCKS = 1000000;
const quint16 ACE1_MAIN_ALLOWED_FLAGS = 0xFE02;
const quint16 ACE1_FILE_ALLOWED_FLAGS = 0xF003;
const qint32 ACE1_CRC_BUFFER_SIZE = 0x10000;

QString decodeAceOemName(const QByteArray &baData, const QString &sCodePage)
{
    // Raw ACE 1.x names are locale-dependent DOS OEM bytes.  CP850 matches
    // the original UnACE compatibility fallback; callers can select CP437 or
    // another Qt-supported OEM codec for archives created in other locales.
    static const quint16 g_anCP437HighBytes[128] = {
        0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, 0x00C9, 0x00E6, 0x00C6,
        0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, 0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1,
        0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555,
        0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, 0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566,
        0x2560, 0x2550, 0x256C, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590,
        0x2580, 0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x03BC, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229, 0x2261, 0x00B1,
        0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
    };

    static const quint16 g_anCP850HighBytes[128] = {
        0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, 0x00C9, 0x00E6, 0x00C6,
        0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192, 0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1,
        0x00AA, 0x00BA, 0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0, 0x00A9,
        0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510, 0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3, 0x255A, 0x2554, 0x2569, 0x2566,
        0x2560, 0x2550, 0x256C, 0x00A4, 0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x0131, 0x00CD, 0x00CE, 0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC,
        0x2580, 0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE, 0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4, 0x00AD, 0x00B1,
        0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8, 0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0,
    };

    const bool bCP437 = (sCodePage.compare(QStringLiteral("CP437"), Qt::CaseInsensitive) == 0) || (sCodePage.compare(QStringLiteral("IBM437"), Qt::CaseInsensitive) == 0);
    const bool bCP850 = sCodePage.isEmpty() || (sCodePage.compare(QStringLiteral("CP850"), Qt::CaseInsensitive) == 0) ||
                        (sCodePage.compare(QStringLiteral("IBM850"), Qt::CaseInsensitive) == 0);

#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    if (!bCP437 && !bCP850) {
        QTextCodec *pCodec = QTextCodec::codecForName(sCodePage.toLatin1());
        if (pCodec) {
            return pCodec->toUnicode(baData);
        }
    }
#endif

    const quint16 *pHighBytes = bCP437 ? g_anCP437HighBytes : g_anCP850HighBytes;

    QString sResult;
    sResult.reserve(baData.size());

    for (char cValue : baData) {
        const quint8 nValue = static_cast<quint8>(cValue);
        const quint16 nUnicode = (nValue < 0x80) ? nValue : pHighBytes[nValue - 0x80];
        sResult.append(QChar(nUnicode));
    }

    return sResult;
}
}  // namespace

XACE::XACE(QIODevice *pDevice) : XArchive(pDevice), m_sFileNameCodePage(QStringLiteral("IBM850"))
{
}

void XACE::setFileNameCodePage(const QString &sCodePage)
{
    m_sFileNameCodePage = sCodePage.trimmed();
}

QString XACE::getFileNameCodePage() const
{
    return m_sFileNameCodePage;
}

qint64 XACE::_blockEnd(const BLOCK_INFO &info)
{
    return info.nDataOffset + info.nAddSize;
}

bool XACE::_readBlock(qint64 nOffset, BLOCK_INFO *pInfo, PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    if (!pInfo || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pInfo = BLOCK_INFO();

    const qint64 nFileSize = getSize();
    if (!guardedThis) return false;

    if ((nOffset < 0) || (nOffset > nFileSize) || ((nFileSize - nOffset) < 4)) {
        return false;
    }

    BLOCK_INFO info = {};
    info.nOffset = nOffset;
    info.nHeadCRC = read_uint16(nOffset, false);
    if (!guardedThis) return false;
    info.nHeadSize = read_uint16(nOffset + 2, false);
    if (!guardedThis) return false;

    // HEAD_SIZE covers bytes starting at HEAD_TYPE.  A generic header contains
    // at least HEAD_TYPE and HEAD_FLAGS (three bytes).
    if ((info.nHeadSize < 3) || (info.nHeadSize > (nFileSize - nOffset - 4))) {
        return false;
    }

    info.nHeaderSize = 4 + info.nHeadSize;
    info.nDataOffset = nOffset + info.nHeaderSize;
    info.nHeadType = read_uint8(nOffset + 4);
    if (!guardedThis) return false;
    info.nHeadFlags = read_uint16(nOffset + 5, false);
    if (!guardedThis) return false;

    if ((info.nHeadType != HEADTYPE_ARCHIVE) && (info.nHeadType != HEADTYPE_FILE) && (info.nHeadType != HEADTYPE_RECOVERY)) {
        return false;
    }

    QByteArray baHeader = read_array(nOffset + 4, info.nHeadSize);
    if (!guardedThis) return false;

    if (baHeader.size() != info.nHeadSize) {
        return false;
    }

    const quint32 nHeaderCRC = XBinary::_getCRC32(baHeader, 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320());

    if (info.nHeadCRC != static_cast<quint16>(nHeaderCRC & 0xFFFFU)) {
        return false;
    }

    if (info.nHeadFlags & FLAG_ADDSIZE) {
        if (info.nHeadSize < 7) {
            return false;
        }

        info.nAddSize = read_uint32(nOffset + 7, false);
        if (!guardedThis) return false;
    }

    if (info.nAddSize > static_cast<quint64>(nFileSize - info.nDataOffset)) {
        return false;
    }

    if (info.nHeadType == HEADTYPE_ARCHIVE) {
        // A main header never has generic ADD_SIZE; at +7 it contains ACESIGN.
        if ((info.nHeadFlags & static_cast<quint16>(~ACE1_MAIN_ALLOWED_FLAGS)) || (info.nHeadSize < ACE1_MAIN_MIN_HEAD_SIZE)) {
            return false;
        }

        QByteArray baMagic = read_array(nOffset + MAGIC_OFFSET, 7);
        if (!guardedThis) return false;

        if ((baMagic.size() != 7) || (std::memcmp(baMagic.constData(), MAGIC, 7) != 0)) {
            return false;
        }

        info.nVersionExtract = read_uint8(nOffset + 14);
        if (!guardedThis) return false;
        info.nVersionCreated = read_uint8(nOffset + 15);
        if (!guardedThis) return false;
        info.nHostCreated = read_uint8(nOffset + 16);
        if (!guardedThis) return false;
        info.nVolumeNumber = read_uint8(nOffset + 17);
        if (!guardedThis) return false;
        info.nTimeCreated = read_uint32(nOffset + 18, false);
        if (!guardedThis) return false;
        info.nMainReserved1 = read_uint16(nOffset + 22, false);
        if (!guardedThis) return false;
        info.nMainReserved2 = read_uint16(nOffset + 24, false);
        if (!guardedThis) return false;
        info.nMainReserved = read_uint32(nOffset + 26, false);
        if (!guardedThis) return false;
        info.nAVSize = read_uint8(nOffset + 30);
        if (!guardedThis) return false;
        info.nAVOffset = nOffset + 31;

        if (((info.nHeadFlags & ARCHFLAG_AV) != 0) != (info.nAVSize != 0)) {
            return false;
        }

        quint32 nVariableEnd = 27U + info.nAVSize;

        if (nVariableEnd > info.nHeadSize) {
            return false;
        }

        if (info.nHeadFlags & FLAG_COMMENT) {
            if ((info.nHeadSize - nVariableEnd) < 2U) {
                return false;
            }

            const qint64 nCommentSizeOffset = nOffset + 4 + nVariableEnd;
            info.nCommentSize = read_uint16(nCommentSizeOffset, false);
            if (!guardedThis) return false;
            info.nCommentOffset = nCommentSizeOffset + 2;

            if ((info.nCommentSize > ACE1_MAX_COMMENT) || (info.nCommentSize > (info.nHeadSize - nVariableEnd - 2U))) {
                return false;
            }
        }
    } else if (info.nHeadType == HEADTYPE_FILE) {
        if ((info.nHeadFlags & static_cast<quint16>(~ACE1_FILE_ALLOWED_FLAGS)) || !(info.nHeadFlags & FILEFLAG_ADDSIZE) || (info.nHeadSize < ACE1_FILE_MIN_HEAD_SIZE)) {
            return false;
        }

        info.nPackedSize = read_uint32(nOffset + 7, false);
        if (!guardedThis) return false;
        info.nUnpackedSize = read_uint32(nOffset + 11, false);
        if (!guardedThis) return false;
        info.nFileTime = read_uint32(nOffset + 15, false);
        if (!guardedThis) return false;
        info.nAttributes = read_uint32(nOffset + 19, false);
        if (!guardedThis) return false;
        info.nFileCRC = read_uint32(nOffset + 23, false);
        if (!guardedThis) return false;
        info.nTechType = read_uint8(nOffset + 27);
        if (!guardedThis) return false;
        info.nTechQuality = read_uint8(nOffset + 28);
        if (!guardedThis) return false;
        info.nTechParameter = read_uint16(nOffset + 29, false);
        if (!guardedThis) return false;
        info.nReserved = read_uint16(nOffset + 31, false);
        if (!guardedThis) return false;
        info.nFileNameSize = read_uint16(nOffset + 33, false);
        if (!guardedThis) return false;
        info.nFileNameOffset = nOffset + 35;

        if ((info.nFileNameSize > ACE1_MAX_FILENAME) || (info.nFileNameSize > (info.nHeadSize - ACE1_FILE_MIN_HEAD_SIZE))) {
            return false;
        }

        QByteArray baFileName = read_array(info.nFileNameOffset, info.nFileNameSize);
        if (!guardedThis) return false;

        if ((baFileName.size() != info.nFileNameSize) || baFileName.contains('\0')) {
            return false;
        }

        info.sFileName = decodeAceOemName(baFileName, m_sFileNameCodePage);

        const quint32 nVariableEnd = ACE1_FILE_MIN_HEAD_SIZE + info.nFileNameSize;

        if (info.nHeadFlags & FLAG_COMMENT) {
            if ((info.nHeadSize - nVariableEnd) < 2U) {
                return false;
            }

            const qint64 nCommentSizeOffset = nOffset + 4 + nVariableEnd;
            info.nCommentSize = read_uint16(nCommentSizeOffset, false);
            if (!guardedThis) return false;
            info.nCommentOffset = nCommentSizeOffset + 2;

            if ((info.nCommentSize > ACE1_MAX_COMMENT) || (info.nCommentSize > (info.nHeadSize - nVariableEnd - 2U))) {
                return false;
            }
        }

    } else {
        // ACE 1.x recovery record (32-bit form): REC_BLK_SIZE aliases ADD_SIZE,
        // followed by ACESIGN, REL_STRT, CLUSTER, CL_SIZE and REC_CRC.
        if ((info.nHeadFlags != FLAG_ADDSIZE) || (info.nHeadSize != ACE1_RECOVERY_MIN_HEAD_SIZE)) {
            return false;
        }

        QByteArray baMagic = read_array(nOffset + 11, 7);
        if (!guardedThis) return false;

        if ((baMagic.size() != 7) || (std::memcmp(baMagic.constData(), MAGIC, 7) != 0)) {
            return false;
        }

        info.nRecoveryRelativeStart = read_uint32(nOffset + 18, false);
        if (!guardedThis) return false;
        info.nRecoveryBlockCount = read_uint32(nOffset + 22, false);
        if (!guardedThis) return false;
        info.nRecoveryClusterSize = read_uint32(nOffset + 26, false);
        if (!guardedThis) return false;
        info.nRecoveryCRC = read_uint16(nOffset + 30, false);
        if (!guardedThis) return false;

        const quint64 nExpectedRecoverySize = static_cast<quint64>(info.nRecoveryBlockCount) * 2U + static_cast<quint64>(info.nRecoveryClusterSize);
        if (nExpectedRecoverySize != info.nAddSize) {
            return false;
        }

        quint32 nRecoveryCRC = 0xFFFFFFFFU;
        qint64 nCRCOffset = info.nDataOffset;
        qint64 nCRCRemaining = info.nAddSize;
        QByteArray baCRCBuffer;
        if (nCRCRemaining > 0) {
            baCRCBuffer.resize(static_cast<qint32>(qMin(nCRCRemaining, (qint64)ACE1_CRC_BUFFER_SIZE)));
        }

        while (nCRCRemaining > 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }

            const qint32 nChunk = static_cast<qint32>(qMin(nCRCRemaining, (qint64)baCRCBuffer.size()));
            if (nChunk <= 0) return false;
            const qint64 nRead = read_array(nCRCOffset, baCRCBuffer.data(), nChunk);
            if (!guardedThis || (nRead != nChunk)) {
                return false;
            }
            nRecoveryCRC = XBinary::_getCRC32(baCRCBuffer.constData(), nChunk, nRecoveryCRC, XBinary::_getCRC32Table_EDB88320());
            nCRCOffset += nChunk;
            nCRCRemaining -= nChunk;
        }

        if (info.nRecoveryCRC != static_cast<quint16>(nRecoveryCRC & 0xFFFFU)) {
            return false;
        }
    }

    *pInfo = info;

    return true;
}

bool XACE::_isRawAce1Main(const BLOCK_INFO &info) const
{
    return (info.nOffset == 0) && (info.nHeadType == HEADTYPE_ARCHIVE) && !(info.nHeadFlags & ARCHFLAG_V20FORMAT) && (info.nVersionExtract >= 10) &&
           (info.nVersionExtract < 20);
}

bool XACE::_collectBlocks(QList<BLOCK_INFO> *pListBlocks, PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    if (!pListBlocks) {
        return false;
    }

    pListBlocks->clear();

    BLOCK_INFO mainInfo = {};

    const bool bMainRead = _readBlock(0, &mainInfo, pPdStruct);
    if (!guardedThis || !bMainRead || !_isRawAce1Main(mainInfo)) {
        return false;
    }

    pListBlocks->append(mainInfo);

    qint64 nOffset = _blockEnd(mainInfo);
    const qint64 nFileSize = getSize();
    if (!guardedThis) return false;
    bool bHasRecoveryRecord = false;

    if (!(mainInfo.nHeadFlags & ARCHFLAG_MULTIVOLUME) && (mainInfo.nVolumeNumber != 0)) {
        pListBlocks->clear();
        return false;
    }

    while (nOffset < nFileSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            pListBlocks->clear();
            return false;
        }

        if (pListBlocks->count() >= ACE1_MAX_BLOCKS) {
            pListBlocks->clear();
            return false;
        }

        BLOCK_INFO info = {};

        const bool bBlockRead = _readBlock(nOffset, &info, pPdStruct);
        if (!guardedThis || !bBlockRead || (info.nHeadType == HEADTYPE_ARCHIVE)) {
            pListBlocks->clear();
            return false;
        }

        if (info.nHeadType == HEADTYPE_FILE) {
            const quint16 nSplitFlags = info.nHeadFlags & (FILEFLAG_SPLIT_BEFORE | FILEFLAG_SPLIT_AFTER);
            if ((nSplitFlags && !(mainInfo.nHeadFlags & ARCHFLAG_MULTIVOLUME)) || ((info.nHeadFlags & FILEFLAG_SPLIT_BEFORE) && (mainInfo.nVolumeNumber == 0)) ||
                ((info.nHeadFlags & FILEFLAG_SOLID) && !(mainInfo.nHeadFlags & ARCHFLAG_SOLID))) {
                pListBlocks->clear();
                return false;
            }
        } else if (bHasRecoveryRecord || (_blockEnd(info) != nFileSize)) {
            // An ACE1 recovery record describes the preceding archive blocks;
            // it is unique and terminal.
            pListBlocks->clear();
            return false;
        }

        pListBlocks->append(info);
        if (info.nHeadType == HEADTYPE_RECOVERY) {
            bHasRecoveryRecord = true;
        }
        nOffset = _blockEnd(info);
    }

    const bool bRecoveryDeclared = (mainInfo.nHeadFlags & ARCHFLAG_RECOVERY) != 0;
    if ((nOffset != nFileSize) || (bRecoveryDeclared != bHasRecoveryRecord) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pListBlocks->clear();
        return false;
    }

    return true;
}

bool XACE::isValid(PDSTRUCT *pPdStruct)
{
    QList<BLOCK_INFO> listBlocks;

    return _collectBlocks(&listBlocks, pPdStruct);
}

bool XACE::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XACE xace(pDevice);

    return xace.isValid(pPdStruct);
}

qint64 XACE::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    QList<BLOCK_INFO> listBlocks;

    if (!_collectBlocks(&listBlocks, pPdStruct) || listBlocks.isEmpty()) {
        return 0;
    }

    return _blockEnd(listBlocks.constLast());
}

QList<XBinary::MAPMODE> XACE::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XACE::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;
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

XBinary::FT XACE::getFileType()
{
    return FT_ACE;
}

QString XACE::getFileFormatExt()
{
    return QStringLiteral("ace");
}

QString XACE::getFileFormatExtsString()
{
    return QStringLiteral("ACE (*.ace)");
}

QString XACE::getMIMEString()
{
    return QStringLiteral("application/x-ace");
}

QString XACE::getVersion()
{
    QList<BLOCK_INFO> listBlocks;

    if (!_collectBlocks(&listBlocks, nullptr) || listBlocks.isEmpty()) {
        return QString();
    }

    const quint8 nVersion = listBlocks.constFirst().nVersionCreated;

    return QStringLiteral("%1.%2").arg(nVersion / 10).arg(nVersion % 10);
}

QString XACE::getArch()
{
    return QString();
}

XBinary::ENDIAN XACE::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XACE::getMode()
{
    return MODE_DATA;
}

QMap<XBinary::UNPACK_PROP, QVariant> XACE::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XACE::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState) {
        return false;
    }

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    // Only a state authenticated by this archive object's private source
    // session can own a context created by an earlier successful init.
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();
    if (!guardedThis) return false;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    QList<BLOCK_INFO> listBlocks;

    const bool bCollected = _collectBlocks(&listBlocks, pPdStruct);
    if (!guardedThis) return false;
    if (!bCollected || listBlocks.isEmpty()) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->nArchiveFlags = listBlocks.constFirst().nHeadFlags;
    pContext->nArchiveSize = _blockEnd(listBlocks.constLast());
    pContext->nVolumeNumber = listBlocks.constFirst().nVolumeNumber;

    for (const BLOCK_INFO &info : listBlocks) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        if (info.nHeadType == HEADTYPE_FILE) {
            pContext->listFileBlocks.append(info);
        }
    }

    pState->pContext = pContext;
    pState->nNumberOfRecords = pContext->listFileBlocks.count();
    pState->nCurrentOffset = pContext->listFileBlocks.isEmpty() ? pContext->nArchiveSize : pContext->listFileBlocks.constFirst().nOffset;
    pState->mapArchiveProperties.insert(FPART_PROP_FLAGS, pContext->nArchiveFlags);

    if (pContext->nArchiveFlags & ARCHFLAG_SOLID) {
        pState->mapArchiveProperties.insert(FPART_PROP_ISSOLID, true);
    }

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XACE::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || (pState->nCurrentIndex < 0)) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);

    if (pState->nCurrentIndex >= pContext->listFileBlocks.count()) {
        return result;
    }

    const BLOCK_INFO &info = pContext->listFileBlocks.at(pState->nCurrentIndex);
    QString sFileName = info.sFileName;
    sFileName.replace(QLatin1Char('\\'), QLatin1Char('/'));

    result.nStreamOffset = info.nDataOffset;
    result.nStreamSize = info.nAddSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);
    result.mapProperties.insert(FPART_PROP_STREAMOFFSET, result.nStreamOffset);
    result.mapProperties.insert(FPART_PROP_STREAMSIZE, result.nStreamSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, static_cast<qint64>(info.nAddSize));
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(info.nUnpackedSize));
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, info.nOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, info.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FLAGS, info.nHeadFlags);
    result.mapProperties.insert(FPART_PROP_TYPE, static_cast<quint32>(info.nTechType));
    result.mapProperties.insert(FPART_PROP_RESULTCRC, info.nFileCRC);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_00000000);

    const qint32 nDictionaryBits = (info.nTechParameter & 15) + 10;
    result.mapProperties.insert(FPART_PROP_WINDOWSIZE, static_cast<qint64>(quint64(1) << nDictionaryBits));

    if (info.nHeadFlags & FILEFLAG_PASSWORD) {
        result.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
    }

    // ACE marks every member of a solid archive as solid, including the first
    // one - which by definition cannot continue an earlier member's decoder
    // state. Treating the archive flag as decisive refused even a single-entry
    // stored archive, which made the method-1 decoder unreachable entirely.
    //
    // Volume 0 is part of the test: the first file block of a continuation
    // volume also has index 0, but it does depend on the previous volume's
    // dictionary, so it is genuinely solid.
    const bool bSolidArchive = (pContext->nArchiveFlags & ARCHFLAG_SOLID) || (info.nHeadFlags & FILEFLAG_SOLID);
    const bool bFirstInSolidChain = (pState->nCurrentIndex == 0) && (pContext->nVolumeNumber == 0);
    const bool bSolid = bSolidArchive && (!bFirstInSolidChain);

    if (bSolid) {
        result.mapProperties.insert(FPART_PROP_ISSOLID, true);
    }

    if (info.nHeadFlags & FILEFLAG_COMMENT) {
        result.mapProperties.insert(FPART_PROP_ISCOMMENTPRESENT, true);
        result.mapProperties.insert(FPART_PROP_FILECOMMENTOFFSET, info.nCommentOffset);
        result.mapProperties.insert(FPART_PROP_FILECOMMENTLENGTH, info.nCommentSize);
    }

    if (info.nAttributes & 0x01U) {
        result.mapProperties.insert(FPART_PROP_ISREADONLY, true);
    }
    if (info.nAttributes & 0x02U) {
        result.mapProperties.insert(FPART_PROP_ISHIDDEN, true);
    }
    if (info.nAttributes & 0x04U) {
        result.mapProperties.insert(FPART_PROP_ISSYSTEM, true);
    }
    if (info.nAttributes & 0x10U) {
        result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    }
    if (info.nAttributes & 0x20U) {
        result.mapProperties.insert(FPART_PROP_ISARCHIVE, true);
    }

    const quint16 nDosTime = static_cast<quint16>(info.nFileTime & 0xFFFFU);
    const quint16 nDosDate = static_cast<quint16>(info.nFileTime >> 16);
    const QDateTime dtMTime(QDate(((nDosDate >> 9) & 0x7F) + 1980, (nDosDate >> 5) & 0x0F, nDosDate & 0x1F),
                            QTime((nDosTime >> 11) & 0x1F, (nDosTime >> 5) & 0x3F, (nDosTime & 0x1F) * 2));

    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
        result.mapProperties.insert(FPART_PROP_MTIME, dtMTime);
    }

    HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
    const bool bUnsupportedFlags =
        bSolid || (info.nHeadFlags & FILEFLAG_PASSWORD) || (info.nHeadFlags & FILEFLAG_SPLIT_BEFORE) || (info.nHeadFlags & FILEFLAG_SPLIT_AFTER);

    if (!bUnsupportedFlags) {
        if (info.nTechType == CTYPE_STORED) {
            compressMethod = HANDLE_METHOD_STORE;
        } else if (info.nTechType == CTYPE_LZ_HUFFMAN) {
            compressMethod = HANDLE_METHOD_ACE;
        }
    }

    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, compressMethod);

    return result;
}

bool XACE::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->listFileBlocks.count())) {
        return false;
    }

    ++pState->nCurrentIndex;

    if (pState->nCurrentIndex < pContext->listFileBlocks.count()) {
        pState->nCurrentOffset = pContext->listFileBlocks.at(pState->nCurrentIndex).nOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XACE::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
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

QString XACE::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XACE_STRUCTID, sizeof(_TABLE_XACE_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XACE::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XACE_STRUCTID, sizeof(_TABLE_XACE_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XACE::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XACE_STRUCTID, sizeof(_TABLE_XACE_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XACE::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;
    QList<BLOCK_INFO> listBlocks;

    if (!_collectBlocks(&listBlocks, pPdStruct) || listBlocks.isEmpty()) {
        return listResult;
    }

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT child = xfStruct;
        child.nStructID = STRUCTID_HEADER;
        child.xLoc = offsetToLoc(0);
        return getXFHeaders(child, pPdStruct);
    }

    if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;

        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        const qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if (nHeaderOffset != 0) {
            return listResult;
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = listBlocks.constFirst().nHeaderSize;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);

        if (xfStruct.bIsParent) {
            XFSTRUCT child = xfStruct;
            child.sParent = xfHeader.sTag;
            child.nStructID = STRUCTID_RECORD;
            child.xLoc = offsetToLoc(_blockEnd(listBlocks.constFirst()));
            listResult.append(getXFHeaders(child, pPdStruct));
        }

        return listResult;
    }

    if (nStructID == STRUCTID_RECORD) {
        qint64 nStartOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);

        if (nStartOffset == -1) {
            nStartOffset = _blockEnd(listBlocks.constFirst());
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_RECORD);
        xfHeader.xfType = XFTYPE_TABLE;

        for (const BLOCK_INFO &info : listBlocks) {
            if ((info.nHeadType == HEADTYPE_FILE) && (info.nOffset >= nStartOffset)) {
                xfHeader.listRowLocations.append(info.nOffset);
            }
        }

        if (!xfHeader.listRowLocations.isEmpty()) {
            xfHeader.xLoc = offsetToLoc(xfHeader.listRowLocations.constFirst());
            xfHeader.nSize = _blockEnd(listBlocks.constLast()) - xfHeader.listRowLocations.constFirst();
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_RECORD, xfHeader.xLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_RECORD), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XACE::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;
    qint64 nOffset = xLoc.nLocation;

    if (xLoc.locType == LT_UNKNOWN) {
        nOffset = 0;
    }

    BLOCK_INFO info = {};

    if (!_readBlock(nOffset, &info, nullptr)) {
        return listResult;
    }

    if ((nStructID == STRUCTID_HEADER) && (info.nHeadType == HEADTYPE_ARCHIVE) && _isRawAce1Main(info)) {
        listResult.append({"HeadCRC", 0, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"HeadSize", 2, 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        listResult.append({"HeadType", 4, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"HeadFlags", 5, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Magic", 7, 7, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"VersionExtract", 14, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"VersionCreated", 15, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"HostCreated", 16, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"VolumeNumber", 17, 1, XFRECORD_FLAG_COUNT, VT_UINT8});
        listResult.append({"TimeCreated", 18, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"Reserved1", 22, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Reserved2", 24, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Reserved", 26, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"AVSize", 30, 1, XFRECORD_FLAG_SIZE, VT_UINT8});

        if (info.nAVSize) {
            listResult.append({"AuthenticityVerification", 31, info.nAVSize, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        }

        if (info.nHeadFlags & FLAG_COMMENT) {
            const qint32 nCommentSizeOffset = 31 + info.nAVSize;
            listResult.append({"CommentSize", nCommentSizeOffset, 2, XFRECORD_FLAG_SIZE, VT_UINT16});

            if (info.nCommentSize) {
                listResult.append({"CompressedComment", nCommentSizeOffset + 2, info.nCommentSize, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
            }
        }
    } else if ((nStructID == STRUCTID_RECORD) && (info.nHeadType == HEADTYPE_FILE)) {
        listResult.append({"HeadCRC", 0, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"HeadSize", 2, 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        listResult.append({"HeadType", 4, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"HeadFlags", 5, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"PackedSize", 7, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"UnpackedSize", 11, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"FileTime", 15, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"Attributes", 19, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"CRC32", 23, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"TechType", 27, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"TechQuality", 28, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"TechParameter", 29, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Reserved", 31, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"FilenameSize", 33, 2, XFRECORD_FLAG_SIZE, VT_UINT16});

        if (info.nFileNameSize) {
            listResult.append({"Filename", 35, info.nFileNameSize, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        }

        if (info.nHeadFlags & FLAG_COMMENT) {
            const qint32 nCommentSizeOffset = 35 + info.nFileNameSize;
            listResult.append({"CommentSize", nCommentSizeOffset, 2, XFRECORD_FLAG_SIZE, VT_UINT16});

            if (info.nCommentSize) {
                listResult.append({"CompressedComment", nCommentSizeOffset + 2, info.nCommentSize, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
            }
        }
    }

    return listResult;
}

static bool aceCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XACE::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    QList<BLOCK_INFO> listBlocks;

    if (nLimit < -1 || nLimit == 0) {
        return listResult;
    }

    if (!_collectBlocks(&listBlocks, pPdStruct) || listBlocks.isEmpty()) {
        return listResult;
    }

    const quint16 nArchiveFlags = listBlocks.constFirst().nHeadFlags;
    const quint8 nMainVolumeNumber = listBlocks.constFirst().nVolumeNumber;
    qint32 nFileBlockIndex = 0;

    for (const BLOCK_INFO &info : listBlocks) {
        if (!aceCanAppendPart(nLimit, listResult)) {
            break;
        }

        QString sName;

        if (info.nHeadType == HEADTYPE_ARCHIVE) {
            sName = tr("Archive header");
        } else if (info.nHeadType == HEADTYPE_RECOVERY) {
            sName = tr("Recovery record");
        } else {
            sName = info.sFileName;
            sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
        }

        if ((nFileParts & FILEPART_HEADER) && aceCanAppendPart(nLimit, listResult)) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = info.nOffset;
            part.nFileSize = info.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = sName;
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_STREAM) && aceCanAppendPart(nLimit, listResult) && (info.nHeadType == HEADTYPE_FILE)) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = info.nDataOffset;
            part.nFileSize = info.nAddSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = sName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, sName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, static_cast<qint64>(info.nAddSize));
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(info.nUnpackedSize));
            part.mapProperties.insert(FPART_PROP_FLAGS, info.nHeadFlags);
            part.mapProperties.insert(FPART_PROP_RESULTCRC, info.nFileCRC);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_00000000);
            part.mapProperties.insert(FPART_PROP_WINDOWSIZE, static_cast<qint64>(quint64(1) << ((info.nTechParameter & 15) + 10)));

            HANDLE_METHOD method = HANDLE_METHOD_UNKNOWN;
            const bool bFirstInSolidChain = (nFileBlockIndex == 0) && (nMainVolumeNumber == 0);
            const bool bUnsupported = (((nArchiveFlags & ARCHFLAG_SOLID) || (info.nHeadFlags & FILEFLAG_SOLID)) && (!bFirstInSolidChain)) ||
                                      (info.nHeadFlags & (FILEFLAG_PASSWORD | FILEFLAG_SPLIT_BEFORE | FILEFLAG_SPLIT_AFTER));

            if (!bUnsupported) {
                if (info.nTechType == CTYPE_STORED) {
                    method = HANDLE_METHOD_STORE;
                } else if (info.nTechType == CTYPE_LZ_HUFFMAN) {
                    method = HANDLE_METHOD_ACE;
                }
            }

            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, method);
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_REGION) && aceCanAppendPart(nLimit, listResult)) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = info.nOffset;
            part.nFileSize = info.nHeaderSize + info.nAddSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = sName;
            listResult.append(part);
        }

        if (info.nHeadType == HEADTYPE_FILE) {
            nFileBlockIndex++;
        }
    }

    const qint64 nArchiveSize = _blockEnd(listBlocks.constLast());

    if ((nFileParts & FILEPART_DATA) && aceCanAppendPart(nLimit, listResult)) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_OVERLAY) && aceCanAppendPart(nLimit, listResult) && (nArchiveSize < getSize())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = nArchiveSize;
        part.nFileSize = getSize() - nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

qint64 XACE::_getBlockHeaderSize(qint64 nOffset)
{
    BLOCK_INFO info = {};

    return _readBlock(nOffset, &info, nullptr) ? info.nHeaderSize : -1;
}

bool XACE::_isFileBlock(qint64 nOffset)
{
    BLOCK_INFO info = {};

    return _readBlock(nOffset, &info, nullptr) && (info.nHeadType == HEADTYPE_FILE);
}

bool XACE::_isValidBlock(qint64 nOffset)
{
    BLOCK_INFO info = {};

    return _readBlock(nOffset, &info, nullptr);
}

quint32 XACE::_getCompressedSize(qint64 nOffset)
{
    BLOCK_INFO info = {};

    if (!_readBlock(nOffset, &info, nullptr) || (info.nHeadType != HEADTYPE_FILE)) {
        return 0;
    }

    return info.nAddSize;
}

QString XACE::_getFileName(qint64 nOffset)
{
    BLOCK_INFO info = {};

    if (!_readBlock(nOffset, &info, nullptr) || (info.nHeadType != HEADTYPE_FILE)) {
        return QString();
    }

    return info.sFileName;
}

QList<QString> XACE::getSearchSignatures()
{
    // Magic begins seven bytes from the raw ACE header start: fourteen wildcard
    // nibbles followed by the seven-byte ASCII signature.
    return {QStringLiteral("..............'**ACE**'")};
}

XBinary *XACE::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    XACE *pResult = new XACE(pDevice);
    pResult->setFileNameCodePage(m_sFileNameCodePage);
    return pResult;
}

bool XACE::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
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

void *XACE::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XACE> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XACE::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
