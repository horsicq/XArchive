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
#include "xarx.h"

namespace {
const qint64 ARX_MIN_HEADER = 24;       // through the name-length byte plus the checksum
const qint32 ARX_MAX_MEMBERS = 100000;  // the same ceiling the other linked-list readers use
}  // namespace

XARX::XARX(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XARX::_readMember(qint64 nOffset, MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pMember) return false;

    const qint64 nTotalSize = getSize();
    if ((nOffset < 0) || (nOffset >= nTotalSize)) return false;

    const quint8 nHeaderSizeByte = read_uint8(nOffset);
    if (nHeaderSizeByte == 0) return false;  // end-of-archive terminator

    const qint64 nHeaderSize = (qint64)nHeaderSizeByte + 2;
    if ((nHeaderSize < ARX_MIN_HEADER) || ((nOffset + nHeaderSize) > nTotalSize)) return false;

    const QString sMethod = read_ansiString(nOffset + 2, 5);
    if (sMethod.size() != 5) return false;
    const QString sPrefix = sMethod.left(3);
    if (((sPrefix != "-lh") && (sPrefix != "-lz")) || (sMethod.at(4) != QLatin1Char('-'))) return false;

    // The byte LHA does not have. Requiring it to be zero is what separates a
    // genuine ARX member from an LHA one, which carries the low byte of its
    // compressed size here.
    if (read_uint8(nOffset + 7) != 0) return false;

    const quint32 nCompressed = read_uint32(nOffset + 8);
    const quint32 nOriginal = read_uint32(nOffset + 12);
    const quint8 nNameLength = read_uint8(nOffset + 22);
    if ((qint64)23 + nNameLength > nHeaderSize) return false;

    // A stored member declares no compressed size; its data is the original.
    const bool bStored = (nCompressed == 0);
    const qint64 nDataSize = bStored ? (qint64)nOriginal : (qint64)nCompressed;
    if ((nDataSize < 0) || ((nOffset + nHeaderSize + nDataSize) > nTotalSize)) return false;

    pMember->nHeaderOffset = nOffset;
    pMember->nHeaderSize = nHeaderSize;
    pMember->nDataOffset = nOffset + nHeaderSize;
    pMember->nCompressedSize = nDataSize;
    pMember->nUncompressedSize = (qint64)nOriginal;
    pMember->sMethod = sMethod;
    pMember->sFileName = read_ansiString(nOffset + 23, nNameLength).replace(QLatin1Char('\\'), QLatin1Char('/'));
    pMember->nDosDateTime = ((quint32)read_uint16(nOffset + 18) << 16) | (quint32)read_uint16(nOffset + 16);
    pMember->bStored = bStored;

    return true;
}

QList<XARX::MEMBER> XARX::_collectMembers(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<MEMBER> listResult;
    qint64 nOffset = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        MEMBER member = {};
        if (!_readMember(nOffset, &member, pPdStruct)) break;
        listResult.append(member);
        if ((nLimit > 0) && (listResult.size() >= nLimit)) break;
        if (listResult.size() >= ARX_MAX_MEMBERS) break;
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    return listResult;
}

// A stored member is tagged with the same compression method as a compressed
// one - the fixtures use "-lh1-" for both - and is distinguished only by a zero
// compressed size. Mapping it through the tag would feed verbatim bytes to the
// LZH1 decoder.
XBinary::HANDLE_METHOD XARX::_methodToHandle(const MEMBER &member)
{
    if (member.bStored) return HANDLE_METHOD_STORE;
    if (member.sMethod == "-lh0-") return HANDLE_METHOD_STORE;
    if (member.sMethod == "-lh1-") return HANDLE_METHOD_LZH1;
    if (member.sMethod == "-lh4-") return HANDLE_METHOD_LZH4;
    if (member.sMethod == "-lh5-") return HANDLE_METHOD_LZH5;

    return HANDLE_METHOD_UNKNOWN;
}

bool XARX::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    // Same contract as XLHA::isValid: the caller's device cursor is restored,
    // because detection probes a device the caller still owns.
    QIODevice *pSourceDevice = getDevice();
    const qint64 nSavedPos = pSourceDevice ? pSourceDevice->pos() : -1;

    if (XBinary::isPdStructNotCanceled(pPdStruct) && (getSize() >= (ARX_MIN_HEADER + 1))) {
        MEMBER member = {};
        // One well-formed member that fits is the whole test: the tag alone is
        // shared with LHA, and the zero byte at offset 7 plus a record that fits
        // the file is what distinguishes the two.
        bResult = _readMember(0, &member, pPdStruct);
    }

    if (pSourceDevice && (nSavedPos >= 0)) {
        pSourceDevice->seek(nSavedPos);
    }

    return bResult;
}

bool XARX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARX xarx(pDevice);

    return xarx.isValid(pPdStruct);
}

XBinary *XARX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XARX(pDevice);
}

QList<QString> XARX::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("....'-lh'..2d00");
    listResult.append("....'-lz'..2d00");

    return listResult;
}

XBinary::FT XARX::getFileType()
{
    return FT_ARX;
}

QString XARX::getFileFormatExt()
{
    return "arx";
}

QString XARX::getFileFormatExtsString()
{
    return "ARX(arx)";
}

QString XARX::getMIMEString()
{
    return "application/x-lzh-compressed";
}

QString XARX::getVersion()
{
    return read_ansiString(3, 3);
}

qint64 XARX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    const QList<MEMBER> listMembers = _collectMembers(-1, pPdStruct);
    if (listMembers.isEmpty()) return 0;

    const MEMBER &last = listMembers.constLast();
    qint64 nResult = last.nDataOffset + last.nCompressedSize;
    // The terminator byte, when the file actually has one.
    if (nResult < getSize()) nResult++;

    return nResult;
}

bool XARX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XARX> guardedArchive(this);
    if (!pState || m_bUnpackOperationInProgress || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    // The state has to be bound to the source device: XArchive::unpackCurrent
    // authenticates it through isUnpackSourceCurrent/getBoundUnpackSourceSnapshot
    // and refuses an unbound state, so listing would work while extraction
    // silently failed.
    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;

    const QList<MEMBER> listMembers = guardedArchive->_collectMembers(-1, pPdStruct);
    if (!guardedArchive) return false;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listMembers.size();
    pState->nTotalSize = guardedArchive->getSize();
    pState->pContext = nullptr;

    bool bResult = false;

    // A bound source is not usable until it is finalized: isUnpackSourceCurrent
    // and getBoundUnpackSourceSnapshot both authenticate against the finalized
    // token, so skipping this leaves every later record check failing.
    if ((pState->nNumberOfRecords > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        bResult = guardedArchive->validateAndFinalizeUnpackSource(pState, pPdStruct);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
    }

    if (!bResult) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
    }

    return bResult;
}

XBinary::ARCHIVERECORD XARX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    // The nested-authorized form: XArchive::unpackCurrent already holds the
    // operation guard when it calls this, so the single-argument form would
    // refuse and hand back an empty record - listing would work while
    // extraction quietly produced nothing.
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    ARCHIVERECORD result = {};

    if (!pState || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return result;
    if (!isUnpackSourceCurrent(pState, pPdStruct)) return result;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    MEMBER member = {};
    if (!_readMember(pState->nCurrentOffset, &member, pPdStruct)) return result;

    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, _methodToHandle(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    // MS-DOS packed date/time, decoded the same way XSEAARC does it: there is
    // no single DT_TYPE for the combined value.
    const quint16 nDosDate = (quint16)(member.nDosDateTime >> 16);
    const quint16 nDosTime = (quint16)(member.nDosDateTime & 0xFFFF);
    const QDate dateModified(((nDosDate >> 9) & 0x7F) + 1980, (nDosDate >> 5) & 0x0F, nDosDate & 0x1F);
    const QTime timeModified((nDosTime >> 11) & 0x1F, (nDosTime >> 5) & 0x3F, (nDosTime & 0x1F) * 2);
    const QDateTime dtModified(dateModified, timeModified);
    if (dtModified.isValid()) result.mapProperties.insert(FPART_PROP_MTIME, dtModified);

    // Deliberately no FPART_PROP_RESULTCRC: only the low byte of the CRC-16 is
    // stored, and there is no record-model checksum type for a truncated value.

    return result;
}

bool XARX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;
    if (!isUnpackSourceCurrent(pState, pPdStruct)) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    MEMBER member = {};
    if (!_readMember(pState->nCurrentOffset, &member, pPdStruct)) return false;

    pState->nCurrentOffset = member.nDataOffset + member.nCompressedSize;
    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XARX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    releaseUnpackSource(pState);

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QList<XBinary::FPART> XARX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    const QList<MEMBER> listMembers = _collectMembers(-1, pPdStruct);
    qint64 nMaxOffset = 0;

    for (const MEMBER &member : listMembers) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
        if ((nLimit > 0) && (listResult.size() >= nLimit)) break;

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_STREAM) && ((nLimit <= 0) || (listResult.size() < nLimit))) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, _methodToHandle(member));
            listResult.append(part);
        }

        nMaxOffset = member.nDataOffset + member.nCompressedSize;
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nMaxOffset > 0) && ((nMaxOffset + 1) < getSize()) && ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = nMaxOffset + 1;  // past the terminator byte
        part.nFileSize = getSize() - (nMaxOffset + 1);
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}
