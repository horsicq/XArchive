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
#include "xrgssad.h"

#include <QTemporaryFile>
#include <QTextCodec>
#include <QtEndian>
#include <cstring>
#include <new>

namespace {
const qint64 MaxOutputSize = 4LL * 1024 * 1024 * 1024;
const qint32 MaxNameLength = 1024;
const qint32 MaxMembers = 100000;
const qint32 ChunkSize = 1024 * 1024;  // multiple of 4: the key stream stays aligned
const quint32 KeyVersion1 = 0xDEADCAFE;

quint32 le32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

quint32 advanceKey(quint32 nKey)
{
    return nKey * 7 + 3;
}

bool isNameByteAllowed(uchar nByte)
{
    return (nByte >= 0x20) && (nByte != 0x7F);
}

QString decodeMemberName(const QByteArray &baName)
{
    QTextCodec *pCodec = QTextCodec::codecForName("Shift-JIS");
    QString sName = pCodec ? pCodec->toUnicode(baName) : QString::fromLatin1(baName);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return sName;
}

bool isMemberNameSafe(const QString &sName)
{
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) || sName.contains(QLatin1Char(':'))) return false;
    const QStringList listParts = sName.split(QLatin1Char('/'));
    const qint32 nCount = listParts.size();
    for (qint32 i = 0; i < nCount; i++) {
        const QString &sPart = listParts.at(i);
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
    }
    return true;
}

// XOR one chunk with the running data key.  nSize is a multiple of 4 except
// for the member's final chunk.
void decryptChunk(uchar *pData, qint32 nSize, quint32 *pnKey)
{
    quint32 nKey = *pnKey;
    qint32 i = 0;
    for (; i + 4 <= nSize; i += 4) {
        const quint32 nWord = le32(pData + i) ^ nKey;
        qToLittleEndian<quint32>(nWord, pData + i);
        nKey = advanceKey(nKey);
    }
    if (i < nSize) {
        for (qint32 j = 0; i + j < nSize; j++) pData[i + j] ^= (uchar)(nKey >> (8 * j));
        nKey = advanceKey(nKey);
    }
    *pnKey = nKey;
}

struct RGSSAD_CANCELED {
    const QPointer<XRgssad> &owner;
    const QPointer<QIODevice> &source;
    const QPointer<QIODevice> &output;
    XBinary::PDSTRUCT *pPdStruct;
    RGSSAD_CANCELED(const QPointer<XRgssad> &ownerRef, const QPointer<QIODevice> &sourceRef, const QPointer<QIODevice> &outputRef, XBinary::PDSTRUCT *pPd)
        : owner(ownerRef), source(sourceRef), output(outputRef), pPdStruct(pPd)
    {
    }
    bool operator()() const
    {
        return !owner || !source || !output || !XBinary::isPdStructNotCanceled(pPdStruct);
    }
};
}  // namespace

XRgssad::XRgssad(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRgssad::~XRgssad()
{
}

XBinary::FT XRgssad::getFileType()
{
    return FT_RGSSAD;
}

XBinary::MODE XRgssad::getMode()
{
    return MODE_DATA;
}

qint32 XRgssad::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XRgssad::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRgssad::getArch()
{
    return QString();
}

qint32 XRgssad::readVersionByte(const QByteArray &baHeader)
{
    if ((baHeader.size() < 8) || (memcmp(baHeader.constData(), "RGSSAD\0", 7) != 0)) return 0;
    const qint32 nVersion = (uchar)baHeader.at(7);
    return ((nVersion == 1) || (nVersion == 3)) ? nVersion : 0;
}

QString XRgssad::getVersion()
{
    const qint32 nVersion = readVersionByte(read_array(0, 8));
    return nVersion ? QString::number(nVersion) : QString();
}

QString XRgssad::getFileFormatExt()
{
    return (readVersionByte(read_array(0, 8)) == 3) ? QStringLiteral("rgss3a") : QStringLiteral("rgssad");
}

QString XRgssad::getFileFormatExtsString()
{
    return QStringLiteral("RPG Maker RGSSAD archive (*.rgssad *.rgss2a *.rgss3a)");
}

QString XRgssad::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

qint64 XRgssad::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return readContext(&context, pPdStruct) ? context.nArchiveEnd : 0;
}

QList<QString> XRgssad::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'RGSSAD'00"));
    return listResult;
}

XBinary *XRgssad::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRgssad(pDevice);
}

bool XRgssad::isValid(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return readContext(&context, pPdStruct);
}

bool XRgssad::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRgssad archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XRgssad::readContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XRgssad> owner(this);
    QPointer<QIODevice> source(getDevice());
    if (!pContext || !source || !source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const qint64 nTotalSize = getSize();
    if (!owner || !source || (nTotalSize < 8)) return false;
    const QByteArray baHeader = read_array_process(0, 8, pPdStruct);
    if (!owner) return false;
    const qint32 nVersion = readVersionByte(baHeader);
    if (nVersion == 0) return false;

    CONTEXT parsed;
    parsed.nVersion = nVersion;
    bool bResult = false;
    try {
        bResult = (nVersion == 1) ? readVersion1(nTotalSize, &parsed, pPdStruct) : readVersion3(nTotalSize, &parsed, pPdStruct);
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (!owner || !bResult || !isPdStructNotCanceled(pPdStruct)) return false;
    *pContext = parsed;
    return true;
}

bool XRgssad::readVersion1(qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XRgssad> owner(this);
    quint32 nKey = KeyVersion1;
    qint64 nPos = 8;
    QList<MEMBER> listMembers;
    while (nPos < nTotalSize) {
        if (!isPdStructNotCanceled(pPdStruct) || (listMembers.size() >= MaxMembers)) return false;
        if (nTotalSize - nPos < 4) return false;
        QByteArray baField = read_array_process(nPos, 4, pPdStruct);
        if (!owner || (baField.size() != 4)) return false;
        const qint64 nNameLength = le32(reinterpret_cast<const uchar *>(baField.constData())) ^ nKey;
        nKey = advanceKey(nKey);
        nPos += 4;
        if ((nNameLength < 1) || (nNameLength > MaxNameLength) || (nTotalSize - nPos < nNameLength + 4)) return false;
        QByteArray baName = read_array_process(nPos, nNameLength, pPdStruct);
        if (!owner || (baName.size() != nNameLength)) return false;
        uchar *pName = reinterpret_cast<uchar *>(baName.data());
        for (qint64 i = 0; i < nNameLength; i++) {
            pName[i] ^= (uchar)(nKey & 0xFF);
            nKey = advanceKey(nKey);
            if (!isNameByteAllowed(pName[i])) return false;
        }
        nPos += nNameLength;
        baField = read_array_process(nPos, 4, pPdStruct);
        if (!owner || (baField.size() != 4)) return false;
        const qint64 nSize = le32(reinterpret_cast<const uchar *>(baField.constData())) ^ nKey;
        nKey = advanceKey(nKey);
        nPos += 4;
        if (nSize > nTotalSize - nPos) return false;
        MEMBER member;
        member.sName = decodeMemberName(baName);
        if (!isMemberNameSafe(member.sName)) return false;
        member.nDataOffset = nPos;
        member.nSize = nSize;
        member.nDataKey = nKey;
        listMembers.append(member);
        nPos += nSize;
    }
    if (listMembers.isEmpty() || (nPos != nTotalSize)) return false;
    pContext->listMembers = listMembers;
    pContext->nArchiveEnd = nTotalSize;
    return true;
}

bool XRgssad::readVersion3(qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XRgssad> owner(this);
    if (nTotalSize < 12 + 4) return false;
    const QByteArray baBase = read_array_process(8, 4, pPdStruct);
    if (!owner || (baBase.size() != 4)) return false;
    const quint32 nKey = le32(reinterpret_cast<const uchar *>(baBase.constData())) * 9 + 3;
    qint64 nPos = 12;
    qint64 nArchiveEnd = 12;
    QList<MEMBER> listMembers;
    for (;;) {
        if (!isPdStructNotCanceled(pPdStruct) || (listMembers.size() >= MaxMembers)) return false;
        if (nTotalSize - nPos < 4) return false;
        const QByteArray baOffset = read_array_process(nPos, 4, pPdStruct);
        if (!owner || (baOffset.size() != 4)) return false;
        const qint64 nOffset = le32(reinterpret_cast<const uchar *>(baOffset.constData())) ^ nKey;
        nPos += 4;
        if (nOffset == 0) break;
        if (nTotalSize - nPos < 12) return false;
        const QByteArray baFields = read_array_process(nPos, 12, pPdStruct);
        if (!owner || (baFields.size() != 12)) return false;
        const uchar *pFields = reinterpret_cast<const uchar *>(baFields.constData());
        const qint64 nSize = le32(pFields) ^ nKey;
        const quint32 nDataKey = le32(pFields + 4) ^ nKey;
        const qint64 nNameLength = le32(pFields + 8) ^ nKey;
        nPos += 12;
        if ((nNameLength < 1) || (nNameLength > MaxNameLength) || (nTotalSize - nPos < nNameLength)) return false;
        QByteArray baName = read_array_process(nPos, nNameLength, pPdStruct);
        if (!owner || (baName.size() != nNameLength)) return false;
        uchar *pName = reinterpret_cast<uchar *>(baName.data());
        for (qint64 i = 0; i < nNameLength; i++) {
            pName[i] ^= (uchar)(nKey >> (8 * (i % 4)));
            if (!isNameByteAllowed(pName[i])) return false;
        }
        nPos += nNameLength;
        if ((nOffset < 12) || (nOffset > nTotalSize) || (nSize > nTotalSize - nOffset)) return false;
        MEMBER member;
        member.sName = decodeMemberName(baName);
        if (!isMemberNameSafe(member.sName)) return false;
        member.nDataOffset = nOffset;
        member.nSize = nSize;
        member.nDataKey = nDataKey;
        listMembers.append(member);
        nArchiveEnd = qMax(nArchiveEnd, nOffset + nSize);
    }
    // Member data must lie behind the table.
    const qint32 nCount = listMembers.size();
    for (qint32 i = 0; i < nCount; i++) {
        if (listMembers.at(i).nDataOffset < nPos) return false;
    }
    if (listMembers.isEmpty()) return false;
    pContext->listMembers = listMembers;
    pContext->nArchiveEnd = qMax(nArchiveEnd, nPos);
    return true;
}

bool XRgssad::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XRgssad> owner(this);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *pOld = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    delete pOld;
    *pState = UNPACK_STATE();
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!owner || !bBound) return false;
    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    OUTPUT_POLICY policy = {};
    const bool bValid = pContext && resolveUnpackOutputPolicy(mapProperties, &policy) && readContext(pContext, pPdStruct);
    if (!owner) {
        delete pContext;
        return false;
    }
    if (!bValid) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    pState->pContext = pContext;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->nCurrentIndex = 0;
    pState->nTotalSize = pContext->nArchiveEnd;
    pState->mapUnpackProperties = mapProperties;
    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!owner) return false;
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XRgssad::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XRgssad> owner(this);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !owner) return record;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint32 nCount = pContext->listMembers.size();
    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount) || (pState->nNumberOfRecords != nCount)) return record;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sName);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QString("RGSSAD v%1 XOR").arg(pContext->nVersion));
    if (!markArchiveStreamRecord(&record, pState->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XRgssad::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XRgssad> owner(this);
    QPointer<QIODevice> source(getDevice());
    QPointer<QIODevice> output(pDevice);
    if (!guard.isAcquired() || !pState || !pState->pContext || !source || !output || !isUnpackSourceCurrent(pState, pPdStruct) || !owner || !source ||
        !output) {
        return false;
    }
    const bool bSupported = isUnpackOutputSupported(output.data());
    if (!owner || !source || !output || !bSupported) return false;
    const bool bAliases = devicesAlias(source.data(), output.data());
    if (!owner || !source || !output || bAliases) return false;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint32 nCount = pContext->listMembers.size();
    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount) || (pState->nNumberOfRecords != nCount)) return false;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    const QMap<UNPACK_PROP, QVariant> mapProperties = pState->mapUnpackProperties;
    const QSharedPointer<OUTPUT_BUDGET> spBudget = pState->spOutputBudget;
    if ((member.nSize < 0) || (member.nSize > MaxOutputSize)) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy) || ((policy.nMaxEntryOutputSize >= 0) && (member.nSize > policy.nMaxEntryOutputSize))) {
        return false;
    }
    if (spBudget && !spBudget->beginEntry(pState->nCurrentIndex, member.sName)) {
        if (spBudget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(spBudget.data());
    }
    if (spBudget && spBudget->isEnforcing() && (spBudget->totalLimit() >= 0) &&
        ((spBudget->totalWritten() > spBudget->totalLimit()) || (member.nSize > spBudget->totalLimit() - spBudget->totalWritten()))) {
        return false;
    }

    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writer = {};
    writer.pDeviceOutput = &stage;
    writer.nProcessedLimit = -1;
    writer.mapUnpackProperties = mapProperties;
    writer.spOutputBudget = spBudget;
    const RGSSAD_CANCELED canceled(owner, source, output, pPdStruct);
    quint32 nKey = member.nDataKey;
    try {
        for (qint64 nDone = 0; nDone < member.nSize;) {
            const qint32 nTake = (qint32)qMin<qint64>(ChunkSize, member.nSize - nDone);
            QByteArray baChunk = read_array_process(member.nDataOffset + nDone, nTake, pPdStruct);
            if (canceled() || (baChunk.size() != nTake)) return false;
            decryptChunk(reinterpret_cast<uchar *>(baChunk.data()), nTake, &nKey);
            if (XBinary::_writeDevice(baChunk.constData(), nTake, &writer) != nTake) return false;
            nDone += nTake;
        }
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (canceled() || (stage.size() != member.nSize) || !stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(pState, pPdStruct) || canceled()) {
        return false;
    }
    const bool bPublished = publishUnpackOutput(&stage, output.data(), pState, pPdStruct);
    return owner && output && bPublished;
}

bool XRgssad::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XRgssad> owner(this);
    if (!guard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !owner) return false;
    const qint64 nCount = static_cast<CONTEXT *>(pState->pContext)->listMembers.size();
    if ((pState->nNumberOfRecords != nCount) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount)) return false;
    ++pState->nCurrentIndex;
    return pState->nCurrentIndex < nCount;
}

bool XRgssad::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
