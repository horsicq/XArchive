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
#include "xrpgmvresource.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include <cstring>
#include <new>

#include "xcompanionfile.h"

namespace {
const qint64 HeaderSize = 16;
const qint64 KeySize = 16;
const qint64 MaxOutputSize = 4LL * 1024 * 1024 * 1024;
const qint64 MaxSystemJsonSize = 16LL * 1024 * 1024;
const qint32 ChunkSize = 1024 * 1024;

bool isHeaderValid(const QByteArray &baHeader)
{
    if (baHeader.size() < HeaderSize) return false;
    if (memcmp(baHeader.constData(), "RPGMV\0\0\0", 8) != 0) return false;
    for (qint32 i = 11; i < HeaderSize; i++) {
        if (baHeader.at(i) != 0) return false;
    }
    return true;
}

struct RPGMV_CANCELED {
    const QPointer<XRpgmvResource> &owner;
    const QPointer<QIODevice> &source;
    const QPointer<QIODevice> &output;
    XBinary::PDSTRUCT *pPdStruct;
    RPGMV_CANCELED(const QPointer<XRpgmvResource> &ownerRef, const QPointer<QIODevice> &sourceRef, const QPointer<QIODevice> &outputRef,
                   XBinary::PDSTRUCT *pPd)
        : owner(ownerRef), source(sourceRef), output(outputRef), pPdStruct(pPd)
    {
    }
    bool operator()() const
    {
        return !owner || !source || !output || !XBinary::isPdStructNotCanceled(pPdStruct);
    }
};
}  // namespace

XRpgmvResource::XRpgmvResource(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRpgmvResource::~XRpgmvResource()
{
}

XBinary::FT XRpgmvResource::getFileType()
{
    return FT_RPGMV_RESOURCE;
}

XBinary::MODE XRpgmvResource::getMode()
{
    return MODE_DATA;
}

qint32 XRpgmvResource::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XRpgmvResource::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRpgmvResource::getArch()
{
    return QString();
}

QString XRpgmvResource::getVersion()
{
    const QByteArray baHeader = read_array(0, HeaderSize);
    if (!isHeaderValid(baHeader)) return QString();
    return QString("%1.%2.%3").arg((uchar)baHeader.at(8)).arg((uchar)baHeader.at(9)).arg((uchar)baHeader.at(10));
}

QString XRpgmvResource::getFileFormatExt()
{
    const QString sExt = QFileInfo(XCompanionFile::sourcePath(getDevice())).suffix().toLower();
    if ((sExt == QLatin1String("rpgmvo")) || (sExt == QLatin1String("rpgmvm"))) return sExt;
    return QStringLiteral("rpgmvp");
}

QString XRpgmvResource::getFileFormatExtsString()
{
    return QStringLiteral("RPG Maker MV encrypted resource (*.rpgmvp *.rpgmvo *.rpgmvm)");
}

QString XRpgmvResource::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

qint64 XRpgmvResource::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return readContext(&context, QString(), pPdStruct) ? HeaderSize + context.nPayloadSize : 0;
}

QList<QString> XRpgmvResource::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'RPGMV'000000"));
    return listResult;
}

XBinary *XRpgmvResource::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRpgmvResource(pDevice);
}

bool XRpgmvResource::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XRpgmvResource> owner(this);
    QPointer<QIODevice> source(getDevice());
    if (!source || !source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(pPdStruct)) return false;
    const qint64 nTotalSize = getSize();
    if (!owner || (nTotalSize < HeaderSize + KeySize)) return false;
    const QByteArray baHeader = read_array_process(0, HeaderSize, pPdStruct);
    return owner && isHeaderValid(baHeader);
}

bool XRpgmvResource::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRpgmvResource resource(pDevice);
    return resource.isValid(pPdStruct);
}

QByteArray XRpgmvResource::parseKey(const QString &sText)
{
    const QString sTrimmed = sText.trimmed();
    if (sTrimmed.size() != 32) return QByteArray();
    for (qint32 i = 0; i < 32; i++) {
        const QChar c = sTrimmed.at(i);
        const bool bHex = ((c >= QLatin1Char('0')) && (c <= QLatin1Char('9'))) || ((c >= QLatin1Char('a')) && (c <= QLatin1Char('f'))) ||
                          ((c >= QLatin1Char('A')) && (c <= QLatin1Char('F')));
        if (!bHex) return QByteArray();
    }
    const QByteArray baKey = QByteArray::fromHex(sTrimmed.toLatin1());
    return (baKey.size() == KeySize) ? baKey : QByteArray();
}

QByteArray XRpgmvResource::readSystemJsonKey(const QString &sPath)
{
    QFile file(sPath);
    if (sPath.isEmpty() || (file.size() > MaxSystemJsonSize) || !file.open(QIODevice::ReadOnly)) return QByteArray();
    QByteArray baJson = file.readAll();
    file.close();
    if (baJson.startsWith("\xEF\xBB\xBF")) baJson = baJson.mid(3);
    const QJsonDocument document = QJsonDocument::fromJson(baJson);
    if (!document.isObject()) return QByteArray();
    return parseKey(document.object().value(QLatin1String("encryptionKey")).toString());
}

QString XRpgmvResource::memberName()
{
    const QString sPath = XCompanionFile::sourcePath(getDevice());
    if (sPath.isEmpty()) return QStringLiteral("resource.bin");
    const QFileInfo fileInfo(sPath);
    const QString sSuffix = fileInfo.suffix().toLower();
    QString sMapped;
    if (sSuffix == QLatin1String("rpgmvp")) sMapped = QStringLiteral("png");
    else if (sSuffix == QLatin1String("rpgmvo")) sMapped = QStringLiteral("ogg");
    else if (sSuffix == QLatin1String("rpgmvm")) sMapped = QStringLiteral("m4a");
    if (sMapped.isEmpty()) return fileInfo.fileName();
    return fileInfo.completeBaseName() + QLatin1Char('.') + sMapped;
}

bool XRpgmvResource::readContext(CONTEXT *pContext, const QString &sPassword, PDSTRUCT *pPdStruct)
{
    QPointer<XRpgmvResource> owner(this);
    if (!pContext || !isValid(pPdStruct) || !owner) return false;
    CONTEXT parsed;
    parsed.nPayloadSize = getSize() - HeaderSize;
    if (!owner) return false;
    parsed.sMemberName = memberName();
    if (!owner) return false;
    parsed.baKey = parseKey(sPassword);
    if (!parsed.baKey.isEmpty()) {
        parsed.sKeySource = QStringLiteral("password");
    } else {
        const QString sSystemJson = XCompanionFile::resolveInAncestors(getDevice(), QStringLiteral("data/System.json"), 4);
        if (!owner) return false;
        if (!sSystemJson.isEmpty()) {
            parsed.baKey = readSystemJsonKey(sSystemJson);
            if (!parsed.baKey.isEmpty()) parsed.sKeySource = sSystemJson;
        }
    }
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    *pContext = parsed;
    return true;
}

bool XRpgmvResource::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XRpgmvResource> owner(this);
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
    const QString sPassword = mapProperties.value(UNPACK_PROP_PASSWORD).toString();
    const bool bValid = pContext && resolveUnpackOutputPolicy(mapProperties, &policy) && readContext(pContext, sPassword, pPdStruct);
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
    pState->nNumberOfRecords = 1;
    pState->nCurrentIndex = 0;
    pState->nTotalSize = HeaderSize + pContext->nPayloadSize;
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

XBinary::ARCHIVERECORD XRpgmvResource::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XRpgmvResource> owner(this);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !owner) return record;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if ((pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1)) return record;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sMemberName);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nPayloadSize);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nPayloadSize);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    QString sMethod;
    if (pContext->baKey.isEmpty()) {
        sMethod = QStringLiteral("RPGMV XOR - encryption key (System.json) not found");
    } else if (pContext->sKeySource == QLatin1String("password")) {
        sMethod = QStringLiteral("RPGMV XOR (key: password)");
    } else {
        sMethod = QStringLiteral("RPGMV XOR (key: System.json)");
    }
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sMethod);
    if (!markArchiveStreamRecord(&record, pState->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XRpgmvResource::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XRpgmvResource> owner(this);
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
    if ((pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1)) return false;
    // No key: fail closed.  Guessing a key would only ever produce a file
    // with 16 wrong bytes, which is worse than no file.
    if (pContext->baKey.size() != KeySize) return false;
    const qint64 nSize = pContext->nPayloadSize;
    const QString sName = pContext->sMemberName;
    const QByteArray baKey = pContext->baKey;
    const QMap<UNPACK_PROP, QVariant> mapProperties = pState->mapUnpackProperties;
    const QSharedPointer<OUTPUT_BUDGET> spBudget = pState->spOutputBudget;
    if ((nSize < KeySize) || (nSize > MaxOutputSize)) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy) || ((policy.nMaxEntryOutputSize >= 0) && (nSize > policy.nMaxEntryOutputSize))) return false;
    if (spBudget && !spBudget->beginEntry(pState->nCurrentIndex, sName)) {
        if (spBudget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(spBudget.data());
    }
    if (spBudget && spBudget->isEnforcing() && (spBudget->totalLimit() >= 0) &&
        ((spBudget->totalWritten() > spBudget->totalLimit()) || (nSize > spBudget->totalLimit() - spBudget->totalWritten()))) {
        return false;
    }

    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writer = {};
    writer.pDeviceOutput = &stage;
    writer.nProcessedLimit = -1;
    writer.mapUnpackProperties = mapProperties;
    writer.spOutputBudget = spBudget;
    const RPGMV_CANCELED canceled(owner, source, output, pPdStruct);
    try {
        for (qint64 nDone = 0; nDone < nSize;) {
            const qint32 nTake = (qint32)qMin<qint64>(ChunkSize, nSize - nDone);
            QByteArray baChunk = read_array_process(HeaderSize + nDone, nTake, pPdStruct);
            if (canceled() || (baChunk.size() != nTake)) return false;
            if (nDone == 0) {
                char *pChunk = baChunk.data();
                for (qint32 i = 0; i < KeySize; i++) pChunk[i] = (char)(pChunk[i] ^ baKey.at(i));
            }
            if (XBinary::_writeDevice(baChunk.constData(), nTake, &writer) != nTake) return false;
            nDone += nTake;
        }
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (canceled() || (stage.size() != nSize) || !stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(pState, pPdStruct) || canceled()) return false;
    const bool bPublished = publishUnpackOutput(&stage, output.data(), pState, pPdStruct);
    return owner && output && bPublished;
}

bool XRpgmvResource::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XRpgmvResource> owner(this);
    if (!guard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !owner) return false;
    if ((pState->nNumberOfRecords != 1) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= 1)) return false;
    ++pState->nCurrentIndex;
    return pState->nCurrentIndex < 1;
}

bool XRpgmvResource::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
