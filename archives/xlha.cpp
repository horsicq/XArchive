/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xlha.h"
#include "xdecompress.h"

#include <limits>

XBinary::XCONVERT _TABLE_XLHA_STRUCTID[] = {
    {XLHA::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XLHA::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XLHA::STRUCTID_RECORD, "RECORD", QString("Record")},
};

static quint16 lhaReadLe16(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 2) > baData.size())) return 0;
    return static_cast<quint16>(static_cast<quint8>(baData.at(nOffset)) | (static_cast<quint16>(static_cast<quint8>(baData.at(nOffset + 1))) << 8));
}

static quint32 lhaReadLe32(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 4) > baData.size())) return 0;
    return static_cast<quint32>(static_cast<quint8>(baData.at(nOffset)) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
                                (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) |
                                (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24));
}

// The reference implementation: recognizes this exact CP/M PMA SFX envelope;
// 00522f80 delegates the suffix at stubSize+0x1a to the shared LHA reader.
// Physical offsets stay relative to the original source for every native API.
static qint64 lhaFirstMemberOffset(XLHA *pArchive, XBinary::PDSTRUCT *pPdStruct)
{
    XLHA *archive = pArchive;
    QIODevice *source = archive ? archive->getDevice() : nullptr;
    if (!archive || !source || !source->isOpen() || !source->isReadable() || source->isSequential() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return -1;
    if (!archive || !source) return -1;
    const qint64 size = archive->getSize();
    if (!archive || !source || size < 0) return -1;
    if (size < 11) return 0;
    const QByteArray prefix = archive->read_array_process(0, 11, pPdStruct);
    if (!archive || !source || prefix.size() != 11) return -1;
    if (prefix.mid(2, 5) != QByteArray("-pms-", 5)) return 0;
    const quint32 stubSize = lhaReadLe32(prefix, 7);
    const qint64 offset = qint64(stubSize) + 0x1a;
    if (quint8(prefix.at(0)) != 0x18 || !stubSize || stubSize >= 0x1000 || offset > size || size - offset < 22) return -1;
    return offset;
}

static bool lhaPmaSfxTailValid(XLHA *pArchive, qint64 offset, qint64 size, XBinary::PDSTRUCT *pPdStruct)
{
    XLHA *archive = pArchive;
    if (!archive || !XBinary::isPdStructNotCanceled(pPdStruct) || offset < 0 || offset > size) return false;
    if (offset == size) return true;
    // Ordinary PMarc2 writes its end marker and pads the final CP/M record
    // with 0x1a. Reject incomplete later members instead of accepting a prefix.
    if (size - offset > 128) return false;
    const QByteArray tail = archive->read_array_process(offset, size - offset, pPdStruct);
    if (!archive || tail.size() != size - offset || tail.at(0) != 0) return false;
    for (char value : tail) if (value != 0 && quint8(value) != 0x1a) return false;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

XLHA::XLHA(QIODevice *pDevice) : XArchive(pDevice)
{
}

// Level 1 archives store skip_sz at bytes 7-10 which equals ext_headers + csz.
// The extended header chain starts at nOffset+nBaseHeaderSize and each entry
// ends with a LE16 "next entry size" (0 = end of chain).
// Returns the total byte count occupied by extended headers.
qint64 XLHA::_getLevel1ExtHeadersSize(qint64 nOffset, qint64 nBaseHeaderSize)
{
    LHA_MEMBER member = {};
    if (!_readMember(nOffset, &member) || (member.nLevel != 1) ||
        (nBaseHeaderSize < 27) || (nBaseHeaderSize > member.nHeaderSize)) return -1;
    return member.nHeaderSize - nBaseHeaderSize;
}

bool XLHA::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedDevice = getDevice();
    const qint64 nSavedPos = guardedDevice->pos();
    const qint64 first = lhaFirstMemberOffset(this, pPdStruct);
    LHA_MEMBER member = {};
    bool bValid = first >= 0 && _readMember(first, &member, pPdStruct);
    if (bValid && first > 0) {
        const qint64 size = getSize();
        qint64 offset = first + member.nRecordSize;
        qint32 count = 1;
        while (count < 65536 && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (!_readMember(offset, &member, pPdStruct)) break;
            offset += member.nRecordSize; ++count;
        }
        bValid = count < 65536 && lhaPmaSfxTailValid(this, offset, size, pPdStruct);
    }
    if ((nSavedPos >= 0)) guardedDevice->seek(nSavedPos);
    return bValid;
}

bool XLHA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLHA xhla(pDevice);

    return xhla.isValid(pPdStruct);
}

// Methods without a decoder (including lh2/lh3) resolve to UNKNOWN. Leaving
// the property unset would make the shared decompressor default to STORE.
XBinary::HANDLE_METHOD XLHA::_methodToHandle(const QString &sMethod)
{
    if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-") || (sMethod == "-pm0-")) return HANDLE_METHOD_STORE;
    if ((sMethod == "-lzs-") || (sMethod == "-lz5-") || (sMethod == "-lhx-") ||
        (sMethod == "-pm1-") || (sMethod == "-pm2-") || (sMethod == "-lk7-")) return HANDLE_METHOD_LHA_LEGACY;
    if (sMethod == "-lh1-") return HANDLE_METHOD_LZH1;  // LArc-compatible: adaptive Huffman + 4 KiB LZSS (LZHUF)
    if (sMethod == "-lh4-") return HANDLE_METHOD_LZH4;
    if (sMethod == "-lh5-") return HANDLE_METHOD_LZH5;
    if (sMethod == "-lh6-") return HANDLE_METHOD_LZH6;
    if (sMethod == "-lh7-") return HANDLE_METHOD_LZH7;

    // Streamline SAR spells the same tags with spaces instead of hyphens and
    // upper-cases them. Its payloads are bit-exact standard LHA streams, so they
    // map to the same decoders. SAR.DOC documents exactly three methods - LH5,
    // LH4 and LH0 - and anything else stays unknown so it fails closed rather
    // than being decoded on an assumption.
    if (sMethod == " LH0 ") return HANDLE_METHOD_STORE;
    if (sMethod == " LH4 ") return HANDLE_METHOD_LZH4;
    if (sMethod == " LH5 ") return HANDLE_METHOD_LZH5;

    return HANDLE_METHOD_UNKNOWN;
}

// Does this 21-byte header prefix start a member of this format?  LHA writes
// "-lh?-", "-lz?-" or "-pm?-"; SAR writes the same tags spaced and upper-cased.
// Everything else about the walk is identical, so only the spelling varies.
bool XLHA::_isMemberTag(const QByteArray &baHeader)
{
    if (baHeader.size() < 21) return false;

    const QByteArray baPrefix = baHeader.mid(2, 3);

    return ((baPrefix == "-lh") || (baPrefix == "-lz") || (baPrefix == "-pm")) && (baHeader.at(6) == '-');
}

// Header layouts and the OS-9/68k level-2 size correction are documented in
// Lhasa lib/lha_file_header.c and lib/ext_header.c (.).
// Keep one checked member parser for detection, enumeration and file maps.
bool XLHA::_readMember(qint64 nOffset, LHA_MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const qint64 nFileSize = getSize();
    if ((nOffset < 0) || (nOffset > nFileSize) || (nFileSize - nOffset < 22)) return false;
    const QByteArray baPrefix = read_array(nOffset, qMin<qint64>(32, nFileSize - nOffset));
    if ((baPrefix.size() < 22) || !_isMemberTag(baPrefix)) return false;

    LHA_MEMBER member = {};
    member.nLevel = static_cast<quint8>(baPrefix.at(20));
    member.sMethod = QString::fromLatin1(baPrefix.constData() + 2, 5);
    member.nCompressedSize = lhaReadLe32(baPrefix, 7);
    member.nUncompressedSize = lhaReadLe32(baPrefix, 11);
    const qint64 nHeaderLimit = 1024 * 1024;
    qint64 nBaseSize = 0;
    if (member.nLevel <= 1) {
        nBaseSize = static_cast<quint8>(baPrefix.at(0)) + 2;
        if (nBaseSize < ((member.nLevel == 0) ? 24 : 27)) return false;
    } else if (member.nLevel == 2) {
        if (baPrefix.size() < 26) return false;
        nBaseSize = lhaReadLe16(baPrefix, 0);
        if (nBaseSize < 26) return false;
        if (baPrefix.at(23) == 'K') nBaseSize += 2;
    } else if (member.nLevel == 3) {
        if ((baPrefix.size() < 32) || (lhaReadLe16(baPrefix, 0) != 4)) return false;
        nBaseSize = lhaReadLe32(baPrefix, 24);
        if (nBaseSize < 32) return false;
    } else {
        return false;
    }
    if ((nBaseSize > nHeaderLimit) || (nBaseSize > nFileSize - nOffset)) return false;
    QByteArray baHeader = read_array(nOffset, nBaseSize);
    if ((baHeader.size() != nBaseSize)) return false;

    QByteArray baName;
    QByteArray baPath;
    qint32 nExtPos = -1;
    quint16 nUnixMode = 0;
    quint8 nOS = 0;
    if (member.nLevel <= 1) {
        if (!_isHeaderChecksumValid(baHeader)) return false;
        const qint32 nNameLength = static_cast<quint8>(baHeader.at(21));
        if (((member.nLevel == 0 ? 24 : 27) + nNameLength) > nBaseSize) return false;
        baName = baHeader.mid(22, nNameLength);
        member.nCRC16 = lhaReadLe16(baHeader, 22 + nNameLength);
        if (member.nLevel == 1) {
            nOS = static_cast<quint8>(baHeader.at(24 + nNameLength));
            nExtPos = static_cast<qint32>(nBaseSize) - 2;
            // Level 1's packed length includes the extended headers, whose
            // first length field is the final word of the base header.
            qint64 nExtTotal = 0;
            for (;;) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                const qint32 nNextSize = lhaReadLe16(baHeader, baHeader.size() - 2);
                if (nNextSize == 0) break;
                if ((nNextSize < 3) || (nNextSize > nHeaderLimit - baHeader.size()) ||
                    (nNextSize > member.nCompressedSize - nExtTotal) ||
                    (nNextSize > nFileSize - nOffset - baHeader.size())) return false;
                const QByteArray baExtra = read_array(nOffset + baHeader.size(), nNextSize);
                if ((baExtra.size() != nNextSize)) return false;
                baHeader.append(baExtra);
                nExtTotal += nNextSize;
            }
            member.nCompressedSize -= nExtTotal;
        }
    } else {
        member.nCRC16 = lhaReadLe16(baHeader, 21);
        nOS = static_cast<quint8>(baHeader.at(23));
        nExtPos = (member.nLevel == 2) ? 24 : 28;
    }

    qint32 nCommonCRCOffset = -1;
    quint16 nCommonCRC = 0;
    if (nExtPos >= 0) {
        const qint32 nWordSize = (member.nLevel == 3) ? 4 : 2;
        for (;;) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nExtPos > baHeader.size() - nWordSize)) return false;
            const quint32 nExtSize = (nWordSize == 4) ? lhaReadLe32(baHeader, nExtPos) : lhaReadLe16(baHeader, nExtPos);
            if (nExtSize == 0) break;
            if ((nExtSize < static_cast<quint32>(nWordSize + 1)) ||
                (nExtSize > static_cast<quint32>(baHeader.size() - nExtPos - nWordSize))) return false;
            const qint32 nTypePos = nExtPos + nWordSize;
            const quint8 nType = static_cast<quint8>(baHeader.at(nTypePos));
            const qint32 nDataPos = nTypePos + 1;
            const qint32 nDataSize = static_cast<qint32>(nExtSize) - nWordSize - 1;
            if (nType == 0) {
                if ((nDataSize < 2) || (nCommonCRCOffset >= 0)) return false;
                nCommonCRCOffset = nDataPos;
                nCommonCRC = lhaReadLe16(baHeader, nDataPos);
            } else if (nType == 1) {
                baName = baHeader.mid(nDataPos, nDataSize);
            } else if (nType == 2) {
                baPath = baHeader.mid(nDataPos, nDataSize);
                baPath.replace(static_cast<char>(0xff), '/');
                if (!baPath.isEmpty() && !baPath.endsWith('/')) baPath.append('/');
            } else if (nType == 0x50) {
                if (nDataSize < 2) return false;
                nUnixMode = lhaReadLe16(baHeader, nDataPos);
            } else if (nType == 0x42) {
                // Large-file extensions are not safely represented by the
                // existing 32-bit member fields. Refuse contradictory sizes.
                if (nDataSize < 16) return false;
                if (lhaReadLe32(baHeader, nDataPos + 4) || lhaReadLe32(baHeader, nDataPos + 12) ||
                    (lhaReadLe32(baHeader, nDataPos) != member.nCompressedSize) ||
                    (lhaReadLe32(baHeader, nDataPos + 8) != member.nUncompressedSize)) return false;
            }
            nExtPos += static_cast<qint32>(nExtSize);
        }
    }
    if (nCommonCRCOffset >= 0) {
        baHeader[nCommonCRCOffset] = 0;
        baHeader[nCommonCRCOffset + 1] = 0;
        quint16 nCRC = 0;
        for (qint32 i = 0; i < baHeader.size(); ++i) {
            nCRC ^= static_cast<quint8>(baHeader.at(i));
            for (qint32 j = 0; j < 8; ++j) nCRC = static_cast<quint16>((nCRC >> 1) ^ ((nCRC & 1) ? 0xa001 : 0));
        }
        if (nCRC != nCommonCRC) return false;
    }
    member.nHeaderSize = baHeader.size();
    if (member.nCompressedSize > nFileSize - nOffset - member.nHeaderSize) return false;
    member.nRecordSize = member.nHeaderSize + member.nCompressedSize;
    // MorphOS appends a comment after a NUL inside the declared filename.
    // The CRC and extent still cover the full field; only the name ends here.
    const qint32 nNameEnd = baName.indexOf('\0');
    if (nNameEnd >= 0) baName.truncate(nNameEnd);
    if (baPath.contains('\0')) return false;
    member.sFileName = QString::fromLatin1(baPath + baName).replace('\\', '/');
    member.bDirectory = (member.sMethod == "-lhd-") && ((nUnixMode & 0170000) != 0120000);
    member.bSymbolicLink = ((nUnixMode & 0170000) == 0120000);
    if (member.sFileName.isEmpty() && !member.bDirectory) return false;
    // LHARK uses a different -lh7- bitstream. Keep its method explicit so it
    // can be dispatched independently from ordinary LHA's -lh7-.
    if ((member.nLevel == 1) && (nOS == 0x20) && (member.sMethod == "-lh7-")) member.sMethod = "-lk7-";
    *pMember = member;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XLHA::_isHeaderChecksumValid(const QByteArray &baHeader)
{
    if (baHeader.size() < 3) return false;
    const qint32 nHeaderSize = (quint8)baHeader.at(0);
    if (nHeaderSize < 2) return false;
    if (baHeader.size() < (2 + nHeaderSize)) return false;

    quint32 nSum = 0;
    for (qint32 i = 2; i < (2 + nHeaderSize); i++) {
        nSum += (quint8)baHeader.at(i);
    }

    return ((nSum & 0xFF) == (quint32)(quint8)baHeader.at(1));
}

qint64 XLHA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XLHA::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XLHA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XLHA::getFileType()
{
    return FT_LHA;
}

QString XLHA::getFileFormatExt()
{
    const qint64 first = lhaFirstMemberOffset(this, nullptr);
    if (first > 0) return QStringLiteral("com");
    QString sResult = "lha";
    QString _sVersion = getVersion().left(2);

    if (_sVersion == "lh") {
        sResult = "lha";
    } else if (_sVersion == "lz") {
        sResult = "lzs";
    } else if (_sVersion == "pm") {
        sResult = "pma";
    }

    return sResult;
}

QString XLHA::getFileFormatExtsString()
{
    return "LHA(lha, lzs, pma, com)";
}

QString XLHA::getMIMEString()
{
    return "application/x-lzh-compressed";
}

QString XLHA::getVersion()
{
    // The SFX envelope does not have a numeric creator-version field.
    const qint64 first = lhaFirstMemberOffset(this, nullptr);
    if (first > 0) return QString();
    return read_ansiString(3, 3);
}

QString XLHA::getArch()
{
    return QString();
}

XBinary::ENDIAN XLHA::getEndian()
{
    return ENDIAN_LITTLE;  // LHA is little-endian
}

XBinary::MODE XLHA::getMode()
{
    return MODE_DATA;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLHA::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XLHA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const bool bBound = bindUnpackSource(pState, pPdStruct);
        if (!bBound) return false;

        pState->mapUnpackProperties = mapProperties;
        const qint64 first = lhaFirstMemberOffset(this, pPdStruct);
        if (first < 0) { releaseUnpackSource(pState); *pState = UNPACK_STATE(); return false; }
        pState->nCurrentOffset = first;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;

        qint64 nOffset = first;
        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            LHA_MEMBER member = {};
            if (!_readMember(nOffset, &member, pPdStruct)) break;
            if ((pState->nNumberOfRecords == (std::numeric_limits<qint32>::max)()) ||
                (first > 0 && pState->nNumberOfRecords >= 65536)) break;
            ++pState->nNumberOfRecords;
            nOffset += member.nRecordSize;
        }
        bResult = (pState->nNumberOfRecords > 0) && XBinary::isPdStructNotCanceled(pPdStruct);
        if (bResult && first > 0) {
            bResult = pState->nNumberOfRecords < 65536 && lhaPmaSfxTailValid(this, nOffset, pState->nTotalSize, pPdStruct);
        }
        if (bResult) {
            bResult = validateAndFinalizeUnpackSource(pState, pPdStruct);
        }
        if (!bResult) {
            releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XLHA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    XBinary::ARCHIVERECORD result = {};
    if (pState && isUnpackSourceCurrent(pState, pPdStruct) && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        LHA_MEMBER member = {};
        if (!_readMember(pState->nCurrentOffset, &member, pPdStruct)) return result;
        result.nStreamOffset = pState->nCurrentOffset + member.nHeaderSize;
        result.nStreamSize = member.nCompressedSize;
        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
        result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bDirectory);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bSymbolicLink ? HANDLE_METHOD_UNKNOWN : _methodToHandle(member.sMethod));
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.sMethod.toLatin1());
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
        result.mapProperties.insert(FPART_PROP_RESULTCRC, static_cast<quint32>(member.nCRC16));
        result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16);
    }
    return result;
}

bool XLHA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    bool bResult = false;

    if (pState && isUnpackSourceCurrent(pState, pPdStruct) && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        LHA_MEMBER member = {};
        if (!_readMember(pState->nCurrentOffset, &member, pPdStruct)) return false;
        pState->nCurrentOffset += member.nRecordSize;
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XLHA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    releaseUnpackSource(pState);
    *pState = UNPACK_STATE();

    return true;
}

QString XLHA::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLHA::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLHA::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XLHA::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const qint64 first = lhaFirstMemberOffset(this, pPdStruct);
    if (first < 0) return listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(first);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(first);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if (nHeaderOffset != -1) {
            LHA_MEMBER member = {};
            if (!_readMember(nHeaderOffset, &member, pPdStruct)) return listResult;
            const qint64 nHeaderSize = member.nHeaderSize;

            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = nHeaderSize;
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);

            if (xfStruct.bIsParent) {
                XFSTRUCT _xfStruct = xfStruct;
                _xfStruct.sParent = xfHeader.sTag;
                _xfStruct.nStructID = STRUCTID_RECORD;
                _xfStruct.xLoc = offsetToLoc(first);
                listResult.append(getXFHeaders(_xfStruct, pPdStruct));
            }
        }
    } else if (nStructID == STRUCTID_RECORD) {
        qint64 nStartOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);

        if (nStartOffset == -1) {
            nStartOffset = first;
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_RECORD);
        xfHeader.xLoc = offsetToLoc(nStartOffset);
        xfHeader.xfType = XFTYPE_TABLE;

        qint64 nFileSize = getSize();
        qint64 nCurrentOffset = nStartOffset;

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            LHA_MEMBER member = {};
            if (!_readMember(nCurrentOffset, &member, pPdStruct)) break;
            xfHeader.listRowLocations.append(nCurrentOffset);
            nCurrentOffset += member.nRecordSize;
            if (nCurrentOffset >= nFileSize) break;
        }

        if (!xfHeader.listRowLocations.isEmpty()) {
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_RECORD, offsetToLoc(xfHeader.listRowLocations.first()));
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_RECORD), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLHA::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_RECORD)) {
        quint8 nLevel = read_uint8(xLoc.nLocation + 20);

        if (nLevel >= 2) {
            listResult.append({(nLevel == 3) ? "WordSize" : "HeaderSize", 0, 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        } else {
            listResult.append({"HeaderSize", 0, 1, XFRECORD_FLAG_SIZE, VT_UINT8});
            listResult.append({"HeaderChecksum", 1, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        }

        listResult.append({"Method", 2, 5, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"CompressedSize", 7, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"UncompressedSize", 11, 4, XFRECORD_FLAG_SIZE, VT_UINT32});

        if (nLevel >= 2) {
            listResult.append({"LastModTime", 15, 4, XFRECORD_FLAG_UNIXTIME, VT_UINT32});
        } else {
            listResult.append({"LastModTime", 15, 2, XFRECORD_FLAG_DOSTIME, VT_UINT16});
            listResult.append({"LastModDate", 17, 2, XFRECORD_FLAG_DOSDATE, VT_UINT16});
        }

        listResult.append({"Attribute", 19, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"Level", 20, 1, XFRECORD_FLAG_NONE, VT_UINT8});

        if (nLevel <= 1) {
            quint8 nNameLength = read_uint8(xLoc.nLocation + 21);
            listResult.append({"NameLength", 21, 1, XFRECORD_FLAG_SIZE, VT_UINT8});
            listResult.append({"FileName", 22, (qint32)nNameLength, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
            listResult.append({"CRC16", 22 + (qint32)nNameLength, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        } else {
            listResult.append({"CRC16", 21, 2, XFRECORD_FLAG_NONE, VT_UINT16});
            listResult.append({"OSID", 23, 1, XFRECORD_FLAG_NONE, VT_UINT8});
            if (nLevel == 3) listResult.append({"HeaderSize", 24, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        }
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XLHA::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         // Count records for table
//         qint64 nRealSize = 0;
//         qint32 nCount = 0;

//         qint64 nFileSize = getSize();
//         qint64 nOffset = 0;

//         while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//             if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nOffset) || compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nOffset)
//             ||
//                 compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nOffset)) {
//                 quint8 nLevel = read_uint8(nOffset + 20);
//                 qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nOffset) : (qint64)(read_uint8(nOffset) + 2);
//                 qint64 nDataSize = read_uint32(nOffset + 7);

//                 if (nHeaderSize < 21) {
//                     break;
//                 }

//                 nCount++;
//                 nRealSize = nOffset + nHeaderSize + nDataSize;

//                 nOffset += (nHeaderSize + nDataSize);
//                 nFileSize -= (nHeaderSize + nDataSize);
//             } else {
//                 break;
//             }
//         }

//         _dataHeadersOptions.nID = STRUCTID_RECORD;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;
//         _dataHeadersOptions.nCount = nCount;
//         _dataHeadersOptions.nSize = nRealSize;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_RECORD) {
//                 // Table of records
//                 qint64 nCurrentOffset = nStartOffset;
//                 qint32 nCount = 0;

//                 while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//                     if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nCurrentOffset) ||
//                         compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nCurrentOffset) ||
//                         compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nCurrentOffset)) {
//                         quint8 nLevel = read_uint8(nCurrentOffset + 20);
//                         qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nCurrentOffset) : (qint64)(read_uint8(nCurrentOffset) + 2);
//                         qint64 nSkipSize = (qint64)(quint32)read_uint32(nCurrentOffset + 7);
//                         qint64 nExtSize = (nLevel == 1) ? _getLevel1ExtHeadersSize(nCurrentOffset, nHeaderSize) : 0;
//                         qint64 nDataSize = nSkipSize - nExtSize;
//                         QString sFileName = read_ansiString(nCurrentOffset + 22, read_uint8(nCurrentOffset + 21));

//                         DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
//                         dataHeader.nSize = nHeaderSize + nSkipSize;

//                         // Record header fields
//                         dataHeader.listRecords.append(getDataRecord(0, 1, "Header Size", VT_UINT8, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(1, 1, "Header CRC", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(2, 5, "Compression Method", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(7, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(11, 4, "Uncompressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(15, 2, "Last Mod Time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(17, 2, "Last Mod Date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(19, 1, "File Attribute", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(20, 1, "Name Length", VT_UINT8, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(
//                             getDataRecord(21, read_uint8(nCurrentOffset + 21), "File Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                         if (nHeaderSize > 22 + read_uint8(nCurrentOffset + 21)) {
//                             dataHeader.listRecords.append(getDataRecord(22 + read_uint8(nCurrentOffset + 21), nHeaderSize - (22 + read_uint8(nCurrentOffset + 21)),
//                                                                         "Extended Header", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         }

//                         if (nExtSize > 0) {
//                             dataHeader.listRecords.append(
//                                 getDataRecord(nHeaderSize, nExtSize, "Extended Headers", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         }

//                         if (nDataSize > 0) {
//                             dataHeader.listRecords.append(
//                                 getDataRecord(nHeaderSize + nExtSize, nDataSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN,
//                                 dataHeadersOptions.pMemoryMap->endian));
//                         }

//                         listResult.append(dataHeader);

//                         nCurrentOffset += (nHeaderSize + nSkipSize);
//                         nCount++;
//                     } else {
//                         break;
//                     }
//                 }
//             }
//         }
//     }

//     return listResult;
// }

static bool lhaCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XLHA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const qint64 first = lhaFirstMemberOffset(this, pPdStruct);
    if (first < 0) return listResult;
    qint64 nFileSize = getSize() - first;
    qint64 nCurrentOffset = first;
    qint64 nMaxOffset = first;
    if (first > 0 && (nFileParts & FILEPART_HEADER) && lhaCanAppendPart(nLimit, listResult)) {
        listResult.append(getFPART(FILEPART_HEADER, tr("PMA SFX stub"), 0, first, XADDR_MAX, 0));
    }
    while ((nFileSize > 0) && lhaCanAppendPart(nLimit, listResult) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        LHA_MEMBER member = {};
        if (!_readMember(nCurrentOffset, &member, pPdStruct)) break;
        if ((nFileParts & FILEPART_HEADER) && lhaCanAppendPart(nLimit, listResult)) {
            listResult.append(getFPART(FILEPART_HEADER, tr("Header"), nCurrentOffset, member.nHeaderSize, XADDR_MAX, 0));
        }
        if ((nFileParts & FILEPART_STREAM) && lhaCanAppendPart(nLimit, listResult)) {
            FPART record = getFPART(FILEPART_STREAM, member.sFileName, nCurrentOffset + member.nHeaderSize, member.nCompressedSize, XADDR_MAX, 0);
            record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bSymbolicLink ? HANDLE_METHOD_UNKNOWN : _methodToHandle(member.sMethod));
            record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.sMethod.toLatin1());
            record.mapProperties.insert(FPART_PROP_ISFOLDER, member.bDirectory);
            listResult.append(record);
        }
        if ((nFileParts & FILEPART_REGION) && lhaCanAppendPart(nLimit, listResult)) {
            listResult.append(getFPART(FILEPART_REGION, member.sFileName, nCurrentOffset, member.nRecordSize, XADDR_MAX, 0));
        }
        nCurrentOffset += member.nRecordSize;
        nMaxOffset = nCurrentOffset;
        nFileSize -= member.nRecordSize;
    }

    // Data part (all archive data)
    if ((nFileParts & FILEPART_DATA) && lhaCanAppendPart(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nMaxOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");

        listResult.append(record);
    }

    // Overlay part (any trailing data)
    if ((nFileParts & FILEPART_OVERLAY) && lhaCanAppendPart(nLimit, listResult)) {
        if (nMaxOffset < getSize()) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nMaxOffset;
            record.nFileSize = getSize() - nMaxOffset;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QList<QString> XLHA::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("....'-lh'..2d");
    listResult.append("....'-lz'..2d");
    listResult.append("....'-pm'..2d");
    listResult.append("18..'-pms-'");

    return listResult;
}

XBinary *XLHA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLHA(pDevice);
}

bool XLHA::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        if (!bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
        if (!pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pInfo;
    }

    return bResult;
}

void *XLHA::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XLHA::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
