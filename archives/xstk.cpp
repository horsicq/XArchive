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
#include "xstk.h"

#include <new>

static XBinary::XCONVERT _TABLE_XSTK_STRUCTID[] = {{XStk::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")}, {XStk::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

static quint32 stkReadLe32(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 4) > baData.size())) return 0;
    return static_cast<quint32>(static_cast<quint8>(baData.at(nOffset)) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
                                (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) |
                                (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24));
}

static QString stkEntryName(const QByteArray &baEntry, qint32 nBase)
{
    QByteArray baName = baEntry.mid(nBase, 13);
    qint32 nEnd = baName.indexOf('\0');
    return QString::fromLatin1(baName.constData(), (nEnd >= 0) ? nEnd : baName.size());
}

XStk::XStk(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XStk::_isStk2(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (getSize() < N_STK2_HEADER_SIZE) return false;

    // The later generation is the only one with a magic: "STK2." + a version digit.
    const QByteArray baSig = read_array(0, 5);

    return (baSig == QByteArray("STK2.", 5));
}

bool XStk::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nSize = getSize();

    // The STK2.1 generation has a signature and a completely different layout.
    if (_isStk2(pPdStruct)) {
        return _isValidStk2(pPdStruct);
    }

    // Minimum: uint16 count + one 22-byte entry.
    if (nSize < (2 + N_STK_ENTRY_SIZE)) {
        return false;
    }

    const quint16 nNumFiles = read_uint16(0);

    // STK is headerless; keep the count within a sane bound to reject noise.
    if ((nNumFiles == 0) || (nNumFiles > 20000)) {
        return false;
    }

    const qint64 nDirEnd = 2 + (qint64)nNumFiles * N_STK_ENTRY_SIZE;
    if (nDirEnd > nSize) {
        return false;
    }

    const QByteArray baDir = read_array(2, (qint32)(nNumFiles * N_STK_ENTRY_SIZE));
    if (baDir.size() != (qint32)(nNumFiles * N_STK_ENTRY_SIZE)) {
        return false;
    }

    // Every entry must have a printable DOS name, a compression flag of 0/1, and an in-bounds
    // data offset that starts after the directory. This combination is discriminating enough for
    // a format with no magic bytes.
    for (quint16 i = 0; i < nNumFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const qint32 nBase = i * N_STK_ENTRY_SIZE;

        QByteArray baName = baDir.mid(nBase, 13);
        qint32 nNameEnd = baName.indexOf('\0');
        qint32 nNameLen = (nNameEnd >= 0) ? nNameEnd : 13;
        if (nNameLen == 0) {
            return false;
        }
        for (qint32 k = 0; k < nNameLen; k++) {
            quint8 c = static_cast<quint8>(baName.at(k));
            if ((c < 0x20) || (c > 0x7E)) {
                return false;
            }
        }

        const qint64 nEntrySize = stkReadLe32(baDir, nBase + 13);
        const qint64 nOffset = stkReadLe32(baDir, nBase + 17);
        const quint8 nCompress = static_cast<quint8>(baDir.at(nBase + 21));

        if (nCompress > 1) {
            return false;
        }
        if ((nOffset < nDirEnd) || (nOffset > nSize)) {
            return false;
        }
        if (nCompress == 0) {
            if (nOffset + nEntrySize > nSize) {
                return false;
            }
        } else {
            if (nOffset + 4 > nSize) {
                return false;
            }
        }
    }

    return true;
}

bool XStk::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XStk xstk(pDevice);

    return xstk.isValid(pPdStruct);
}

bool XStk::_isValidStk2(PDSTRUCT *pPdStruct)
{
    const qint64 nSize = getSize();
    if (nSize < N_STK2_HEADER_SIZE) return false;

    // Directory offset is the last header field; the directory sits after the header and in-bounds.
    const qint64 nDirOffset = read_uint32(28);
    if ((nDirOffset < N_STK2_HEADER_SIZE) || ((nDirOffset + 8) > nSize)) return false;

    const quint32 nNumFiles = read_uint32(nDirOffset);
    if ((nNumFiles == 0) || (nNumFiles > 65535)) return false;

    const qint64 nMetaOffset = read_uint32(nDirOffset + 4);
    // The name list fills [dirOffset+8, metaOffset); the fixed-size metadata records must fit
    // between metaOffset and EOF.
    if ((nMetaOffset < (nDirOffset + 8)) || (nMetaOffset > nSize)) return false;
    const qint64 nMetaBytes = (qint64)nNumFiles * N_STK2_RECORD_SIZE;
    if ((nMetaOffset + nMetaBytes) > nSize) return false;

    // Decisive structural invariant: members are stored back-to-back from the end of the header, so
    // the cumulative stored sizes must land exactly on the directory offset.
    qint64 nCum = N_STK2_HEADER_SIZE;
    for (quint32 i = 0; i < nNumFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nStored = read_uint32(nMetaOffset + (qint64)i * N_STK2_RECORD_SIZE + 40);

        nCum += nStored;
        if (nCum > nDirOffset) return false;  // data would spill into the directory
    }

    return (nCum == nDirOffset);
}

bool XStk::_parseEntries(QList<STK_RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    if (!pListRecords) return false;

    // Delegate the signed generation to its own parser.
    if (_isStk2(pPdStruct)) {
        return _parseEntriesStk2(pListRecords, pPdStruct);
    }

    const qint64 nSize = getSize();

    const quint16 nNumFiles = read_uint16(0);

    const qint64 nDirEnd = 2 + (qint64)nNumFiles * N_STK_ENTRY_SIZE;
    if ((nNumFiles == 0) || (nDirEnd > nSize)) {
        return false;
    }

    const QByteArray baDir = read_array(2, (qint32)(nNumFiles * N_STK_ENTRY_SIZE));
    if (baDir.size() != (qint32)(nNumFiles * N_STK_ENTRY_SIZE)) {
        return false;
    }

    for (quint16 i = 0; i < nNumFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            break;
        }

        const qint32 nBase = i * N_STK_ENTRY_SIZE;

        QString sName = stkEntryName(baDir, nBase);
        const qint64 nEntrySize = stkReadLe32(baDir, nBase + 13);
        const qint64 nOffset = stkReadLe32(baDir, nBase + 17);
        const quint8 nCompress = static_cast<quint8>(baDir.at(nBase + 21));

        if (sName.isEmpty() || (nOffset < 0) || (nOffset > nSize)) {
            continue;
        }

        // Normalise DOS backslashes to forward slashes for portable listing.
        sName.replace(QChar('\\'), QChar('/'));

        STK_RECORD record = {};
        record.sFileName = sName;

        if (nCompress == 1) {
            // Compressed chunk: uint32 uncompressed size + Coktel LZSS stream.
            const quint32 nUncomp = read_uint32(nOffset);

            record.bCompressed = true;
            record.nDataOffset = nOffset + 4;
            record.nUncompressedSize = nUncomp;
            // The LZSS stream fills the entry slot after the 4-byte prefix. The stored size field
            // is (stream size + 3); clamp to the file just in case.
            qint64 nStream = (nEntrySize > 3) ? (nEntrySize - 3) : 0;
            if (record.nDataOffset + nStream > nSize) {
                nStream = nSize - record.nDataOffset;
            }
            record.nStreamSize = (nStream > 0) ? nStream : 0;
        } else {
            record.bCompressed = false;
            record.nDataOffset = nOffset;
            record.nStreamSize = nEntrySize;
            record.nUncompressedSize = nEntrySize;
        }

        pListRecords->append(record);
    }

    return !pListRecords->isEmpty();
}

bool XStk::_parseEntriesStk2(QList<STK_RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    if (!pListRecords) return false;

    const qint64 nSize = getSize();

    const qint64 nDirOffset = read_uint32(28);
    if ((nDirOffset < N_STK2_HEADER_SIZE) || ((nDirOffset + 8) > nSize)) return false;

    const quint32 nNumFiles = read_uint32(nDirOffset);
    if ((nNumFiles == 0) || (nNumFiles > 65535)) return false;

    const qint64 nMetaOffset = read_uint32(nDirOffset + 4);
    if ((nMetaOffset < (nDirOffset + 8)) || (nMetaOffset > nSize)) return false;
    const qint64 nMetaBytes = (qint64)nNumFiles * N_STK2_RECORD_SIZE;
    if ((nMetaOffset + nMetaBytes) > nSize) return false;

    // The name list is a packed run of NUL-terminated names in [dirOffset+8, metaOffset). Read it
    // once; each record's name pointer is an absolute file offset into this blob.
    const qint64 nNameBase = nDirOffset + 8;
    const qint64 nNameBytes = nMetaOffset - nNameBase;
    const QByteArray baNames = read_array(nNameBase, (qint32)nNameBytes);
    if (baNames.size() != (qint32)nNameBytes) return false;

    const QByteArray baMeta = read_array(nMetaOffset, (qint32)nMetaBytes);
    if (baMeta.size() != (qint32)nMetaBytes) return false;

    qint64 nCum = N_STK2_HEADER_SIZE;
    for (quint32 i = 0; i < nNumFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            break;
        }

        const qint32 nRecBase = (qint32)((qint64)i * N_STK2_RECORD_SIZE);
        const quint32 nNamePtr = stkReadLe32(baMeta, nRecBase + 0);
        const qint64 nStored = stkReadLe32(baMeta, nRecBase + 40);
        const qint64 nUncomp = stkReadLe32(baMeta, nRecBase + 44);

        // Resolve the name from the blob; a bad pointer falls back to an index name rather than
        // dropping the member (its data offset is positional, not name-derived).
        QString sName;
        if (((qint64)nNamePtr >= nNameBase) && ((qint64)nNamePtr < nMetaOffset)) {
            const qint32 nRel = (qint32)((qint64)nNamePtr - nNameBase);
            qint32 nEnd = baNames.indexOf('\0', nRel);
            if (nEnd < 0) nEnd = baNames.size();
            sName = QString::fromLatin1(baNames.constData() + nRel, nEnd - nRel);
        }
        if (sName.isEmpty()) {
            sName = QString("file%1").arg(i);
        }
        sName.replace(QChar('\\'), QChar('/'));

        const qint64 nChunkOffset = nCum;
        nCum += nStored;
        if ((nChunkOffset + nStored) > nSize) {
            break;  // corrupt: data spills past EOF
        }

        STK_RECORD record = {};
        record.sFileName = sName;

        if ((nUncomp != nStored) && (nStored >= 4)) {
            // Compressed chunk: uint32 uncompressed size prefix + Coktel LZSS stream.
            record.bCompressed = true;
            record.nDataOffset = nChunkOffset + 4;
            record.nStreamSize = nStored - 4;
            record.nUncompressedSize = nUncomp;
        } else {
            record.bCompressed = false;
            record.nDataOffset = nChunkOffset;
            record.nStreamSize = nStored;
            record.nUncompressedSize = nStored;
        }

        pListRecords->append(record);
    }

    return !pListRecords->isEmpty();
}

XBinary::FT XStk::getFileType()
{
    return FT_STK;
}

XBinary::MODE XStk::getMode()
{
    return MODE_DATA;
}

QString XStk::getMIMEString()
{
    return "application/x-coktel-stk";
}

qint32 XStk::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XStk::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XStk::getArch()
{
    return QString();
}

QString XStk::getFileFormatExt()
{
    return "stk";
}

QString XStk::getFileFormatExtsString()
{
    return "Coktel Vision (*.stk *.itk *.ltk *.jtk)";
}

qint64 XStk::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XStk::getOsName()
{
    return OSNAME_MSDOS;
}

QString XStk::getVersion()
{
    // The signed generation embeds "STK2." + a version digit at offset 0.
    if (_isStk2(nullptr)) {
        const QByteArray baSig = read_array(3, 3);  // "2.1"
        return QString::fromLatin1(baSig.constData(), baSig.size());
    }

    return QString();
}

QList<XBinary::MAPMODE> XStk::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XStk::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)
    Q_UNUSED(pPdStruct)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;
    const quint16 nNumFiles = read_uint16(0);
    qint64 nDirEnd = 2 + (qint64)nNumFiles * N_STK_ENTRY_SIZE;
    if ((nDirEnd <= 0) || (nDirEnd > getSize())) {
        nDirEnd = 2;
    }

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = nDirEnd;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("STK ") + tr("Header");
    result.listRecords.append(recHeader);

    if (nDirEnd < getSize()) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = nDirEnd;
        recData.nSize = getSize() - nDirEnd;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XStk::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSTK_STRUCTID, sizeof(_TABLE_XSTK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XStk::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSTK_STRUCTID, sizeof(_TABLE_XSTK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XStk::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSTK_STRUCTID, sizeof(_TABLE_XSTK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XStk::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = 2;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XStk::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_HEADER) {
        listResult.append({"numFiles", 0, 2, XFRECORD_FLAG_NONE, VT_UINT16});
    }

    return listResult;
}

static bool stkCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XStk::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const quint16 nNumFiles = read_uint16(0);
    qint64 nDirEnd = 2 + (qint64)nNumFiles * N_STK_ENTRY_SIZE;
    if ((nDirEnd <= 0) || (nDirEnd > getSize())) {
        nDirEnd = 2;
    }

    if ((nFileParts & FILEPART_HEADER) && stkCanAppend(nLimit, listResult)) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = nDirEnd;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && stkCanAppend(nLimit, listResult) && (nDirEnd < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nDirEnd;
        record.nFileSize = getSize() - nDirEnd;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

XBinary *XStk::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XStk(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XStk::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XStk::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || m_bUnpackOperationInProgress || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) {
        return false;
    }
    if (!finishUnpack(pState, nullptr)) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!bBound) return false;

    const bool bValid = isValid(pPdStruct);
    if (!bValid) {
        releaseUnpackSource(pState);
        return false;
    }

    STK_UNPACK_CONTEXT *pContext = new (std::nothrow) STK_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    const bool bParsed = _parseEntries(&(pContext->listRecords), pPdStruct);
    if (!bParsed || !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        delete pContext;
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nTotalSize = getSize();
    pState->nCurrentOffset = 0;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XStk::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    STK_UNPACK_CONTEXT *pContext = (STK_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const STK_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nDataOffset;
    result.nStreamSize = record.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nStreamSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, record.bCompressed ? HANDLE_METHOD_COKTEL_LZ : HANDLE_METHOD_STORE);

    return result;
}

bool XStk::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XStk::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    STK_UNPACK_CONTEXT *pContext = static_cast<STK_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

bool XStk::handleInternalInfo(PDSTRUCT *pPdStruct)
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

void *XStk::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XStk::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
