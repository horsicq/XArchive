/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Format knowledge: the block/tag layout of Qt's .qm files as documented by
 * the Qt sources (LGPL/GPL). Layout only - no code, comments or tables were
 * taken; the parser and the .ts writer below are original.
 */
#include "xqtqm.h"

#include <QFileInfo>
#include <QLocale>
#include <QtEndian>

#include <limits>
#include <memory>
#include <new>

namespace {

const qint64 QM_MAX_SOURCE = Q_INT64_C(64) * 1024 * 1024;
const qint64 QM_MAX_TEXT = Q_INT64_C(256) * 1024 * 1024;
const qint32 QM_MAX_MESSAGES = 1000000;
const qint32 QM_MAX_TRANSLATIONS = 64;

const quint8 QM_MAGIC[16] = {0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95, 0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd};

enum BLOCK_TAG {
    BLOCK_CONTEXTS = 0x2f,
    BLOCK_HASHES = 0x42,
    BLOCK_MESSAGES = 0x69,
    BLOCK_NUMERUSRULES = 0x88,
    BLOCK_DEPENDENCIES = 0x96,
    BLOCK_LANGUAGE = 0xa7
};

enum MESSAGE_TAG {
    TAG_END = 1,
    TAG_SOURCETEXT16 = 2,
    TAG_TRANSLATION = 3,
    TAG_CONTEXT16 = 4,
    TAG_OBSOLETE1 = 5,
    TAG_SOURCETEXT = 6,
    TAG_CONTEXT = 7,
    TAG_COMMENT = 8
};

quint32 be32(const QByteArray &baData, qint32 nOffset)
{
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

// Reads a 32-bit big-endian length at *pnPos and the payload that follows it.
bool takeSized(const QByteArray &baData, qint32 *pnPos, QByteArray *pPayload, bool *pbNull)
{
    if (!pnPos || !pPayload || (*pnPos < 0) || ((baData.size() - *pnPos) < 4)) return false;
    const quint32 nLength = be32(baData, *pnPos);
    *pnPos += 4;
    if (pbNull) *pbNull = false;
    if (nLength == 0xFFFFFFFFU) {
        // QDataStream's null-string marker: no payload bytes follow.
        pPayload->clear();
        if (pbNull) *pbNull = true;
        return true;
    }
    if (nLength > static_cast<quint32>(baData.size() - *pnPos)) return false;
    *pPayload = baData.mid(*pnPos, static_cast<qint32>(nLength));
    *pnPos += static_cast<qint32>(nLength);
    return true;
}

}  // namespace

XQtQM::XQtQM(QIODevice *pDevice) : XArchive(pDevice)
{
}

QString XQtQM::utf16BE(const QByteArray &baData)
{
    QString sResult;
    const qint32 nUnits = baData.size() / 2;
    sResult.resize(nUnits);
    for (qint32 i = 0; i < nUnits; ++i) {
        const quint16 nUnit = static_cast<quint16>((static_cast<quint8>(baData.at(2 * i)) << 8) | static_cast<quint8>(baData.at(2 * i + 1)));
        sResult[i] = QChar(nUnit);
    }
    return sResult;
}

bool XQtQM::parseMessages(const QByteArray &baBlock, QList<MESSAGE> *pList, QList<qint32> *pOffsets, PDSTRUCT *pPdStruct)
{
    if (!pList || !pOffsets) return false;
    const qint32 nSize = baBlock.size();
    qint32 nPos = 0;
    while (nPos < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        pOffsets->append(nPos);
        MESSAGE message;
        bool bEnded = false;
        while (nPos < nSize) {
            const quint8 nTag = static_cast<quint8>(baBlock.at(nPos++));
            QByteArray baPayload;
            bool bNull = false;
            if (nTag == TAG_END) {
                bEnded = true;
                break;
            } else if (nTag == TAG_OBSOLETE1) {
                if ((nSize - nPos) < 4) return false;
                nPos += 4;
            } else if ((nTag == TAG_SOURCETEXT16) || (nTag == TAG_CONTEXT16) || (nTag == TAG_TRANSLATION)) {
                if (!takeSized(baBlock, &nPos, &baPayload, &bNull) || (baPayload.size() & 1)) return false;
                const QString sText = utf16BE(baPayload);
                if (nTag == TAG_SOURCETEXT16) message.sSource = sText;
                else if (nTag == TAG_CONTEXT16) message.sContext = sText;
                else {
                    if (message.listTranslations.size() >= QM_MAX_TRANSLATIONS) return false;
                    message.listTranslations.append(sText);
                }
            } else if ((nTag == TAG_SOURCETEXT) || (nTag == TAG_CONTEXT) || (nTag == TAG_COMMENT)) {
                if (!takeSized(baBlock, &nPos, &baPayload, &bNull)) return false;
                const QString sText = QString::fromUtf8(baPayload);
                if (nTag == TAG_SOURCETEXT) message.sSource = sText;
                else if (nTag == TAG_CONTEXT) message.sContext = sText;
                else message.sComment = sText;
            } else {
                return false;  // unknown or unsupported tag: fail closed
            }
        }
        if (!bEnded) return false;
        if (pList->size() >= QM_MAX_MESSAGES) return false;
        pList->append(message);
    }
    return true;
}

bool XQtQM::parseCatalog(const QByteArray &baSource, CATALOG *pCatalog, PDSTRUCT *pPdStruct)
{
    if (!pCatalog) return false;
    *pCatalog = CATALOG();
    const qint32 nSize = baSource.size();
    if (nSize < 16) return false;
    for (qint32 i = 0; i < 16; ++i) {
        if (static_cast<quint8>(baSource.at(i)) != QM_MAGIC[i]) return false;
    }
    qint32 nPos = 16;
    bool bMessagesSeen = false;
    QList<MESSAGE> listBlockOrder;
    QList<qint32> listMessageOffsets;
    QByteArray baHashes;
    bool bHashesSeen = false;
    while (nPos < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if ((nSize - nPos) < 5) return false;
        const quint8 nTag = static_cast<quint8>(baSource.at(nPos));
        const quint32 nLength = be32(baSource, nPos + 1);
        nPos += 5;
        if (nLength > static_cast<quint32>(nSize - nPos)) return false;
        const QByteArray baBlock = baSource.mid(nPos, static_cast<qint32>(nLength));
        nPos += static_cast<qint32>(nLength);
        ++pCatalog->nBlocks;
        if (nTag == BLOCK_MESSAGES) {
            if (bMessagesSeen || !parseMessages(baBlock, &listBlockOrder, &listMessageOffsets, pPdStruct)) return false;
            bMessagesSeen = true;
        } else if (nTag == BLOCK_HASHES) {
            if (bHashesSeen) return false;
            baHashes = baBlock;
            bHashesSeen = true;
        } else if (nTag == BLOCK_LANGUAGE) {
            pCatalog->sLanguage = QString::fromUtf8(baBlock);
        } else if (nTag == BLOCK_DEPENDENCIES) {
            qint32 nDepPos = 0;
            while (nDepPos < baBlock.size()) {
                QByteArray baDependency;
                bool bNull = false;
                if (!takeSized(baBlock, &nDepPos, &baDependency, &bNull) || (baDependency.size() & 1)) return false;
                pCatalog->listDependencies.append(utf16BE(baDependency));
                if (pCatalog->listDependencies.size() > 4096) return false;
            }
        } else if ((nTag == BLOCK_CONTEXTS) || (nTag == BLOCK_NUMERUSRULES)) {
            // Context lookup acceleration and plural rules: not needed for the .ts text.
        } else {
            return false;
        }
    }
    // The Hashes block lists (hash, message offset) pairs sorted by hash; that
    // is the order in which the messages are published (and the order the
    // reference converter uses).  Every offset must start a parsed message.
    if (bHashesSeen && bMessagesSeen && !listBlockOrder.isEmpty()) {
        if (baHashes.size() % 8) return false;
        QMap<qint32, qint32> mapOffsetToIndex;
        for (qint32 i = 0; i < listMessageOffsets.size(); ++i) mapOffsetToIndex.insert(listMessageOffsets.at(i), i);
        const qint32 nEntries = baHashes.size() / 8;
        if (nEntries != listBlockOrder.size()) return false;
        for (qint32 i = 0; i < nEntries; ++i) {
            const quint32 nOffset = be32(baHashes, i * 8 + 4);
            if ((nOffset > static_cast<quint32>((std::numeric_limits<qint32>::max)())) || !mapOffsetToIndex.contains(static_cast<qint32>(nOffset))) return false;
            pCatalog->listMessages.append(listBlockOrder.at(mapOffsetToIndex.value(static_cast<qint32>(nOffset))));
        }
    } else {
        pCatalog->listMessages = listBlockOrder;
    }
    return pCatalog->nBlocks > 0;
}

// The reference converter reports QLocale's canonical "language_COUNTRY" name
// for the catalog language, and guesses the language from the file name when
// the catalog carries none ("qt_sv.qm" -> sv -> sv_SE).
QString XQtQM::normalizeLanguage(const QString &sLanguage)
{
    if (sLanguage.isEmpty()) return QString();
    const QLocale locale(sLanguage);
    if (locale.language() == QLocale::C) return sLanguage;
    return locale.name();
}

QString XQtQM::languageFromFileName(const QString &sFileName)
{
    if (sFileName.isEmpty()) return QString();
    QString sStem = QFileInfo(sFileName).completeBaseName();
    while (!sStem.isEmpty()) {
        const QLocale locale(sStem);
        if (locale.language() != QLocale::C) return locale.name();
        const qint32 nUnderscore = sStem.indexOf(QLatin1Char('_'));
        if (nUnderscore < 0) break;
        sStem = sStem.mid(nUnderscore + 1);
    }
    return QString();
}

QString XQtQM::xmlEscape(const QString &sText)
{
    QString sResult;
    sResult.reserve(sText.size() + 16);
    for (qint32 i = 0; i < sText.size(); ++i) {
        const QChar c = sText.at(i);
        const ushort nUnit = c.unicode();
        if (nUnit == '&') sResult += QStringLiteral("&amp;");
        else if (nUnit == '<') sResult += QStringLiteral("&lt;");
        else if (nUnit == '>') sResult += QStringLiteral("&gt;");
        else if (nUnit == '"') sResult += QStringLiteral("&quot;");
        else if (nUnit == '\'') sResult += QStringLiteral("&apos;");
        else if ((nUnit < 0x20) && (nUnit != '\t') && (nUnit != '\n') && (nUnit != '\r')) sResult += QStringLiteral("&#x%1;").arg(nUnit, 0, 16);
        else sResult += c;
    }
    return sResult;
}

QByteArray XQtQM::renderTS(const CATALOG &catalog)
{
    QString sText;
    sText += QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE TS>\n<TS version=\"2.1\"");
    if (!catalog.sLanguage.isEmpty()) sText += QStringLiteral(" language=\"%1\"").arg(xmlEscape(catalog.sLanguage));
    sText += QStringLiteral(">\n");
    if (!catalog.listDependencies.isEmpty()) {
        sText += QStringLiteral("<dependencies>\n");
        for (qint32 i = 0; i < catalog.listDependencies.size(); ++i) {
            sText += QStringLiteral("<dependency catalog=\"%1\"/>\n").arg(xmlEscape(catalog.listDependencies.at(i)));
        }
        sText += QStringLiteral("</dependencies>\n");
    }
    // Contexts in order of first appearance, messages in catalog order.
    QStringList listContextOrder;
    QMap<QString, QList<qint32> > mapContextMessages;
    for (qint32 i = 0; i < catalog.listMessages.size(); ++i) {
        const QString &sContext = catalog.listMessages.at(i).sContext;
        if (!mapContextMessages.contains(sContext)) listContextOrder.append(sContext);
        mapContextMessages[sContext].append(i);
    }
    for (qint32 c = 0; c < listContextOrder.size(); ++c) {
        const QString &sContext = listContextOrder.at(c);
        sText += QStringLiteral("<context>\n    <name>%1</name>\n").arg(xmlEscape(sContext));
        const QList<qint32> &listIndexes = mapContextMessages.value(sContext);
        for (qint32 m = 0; m < listIndexes.size(); ++m) {
            const MESSAGE &message = catalog.listMessages.at(listIndexes.at(m));
            const bool bNumerus = message.listTranslations.size() > 1;
            sText += bNumerus ? QStringLiteral("    <message numerus=\"yes\">\n") : QStringLiteral("    <message>\n");
            sText += QStringLiteral("        <source>%1</source>\n").arg(xmlEscape(message.sSource));
            if (!message.sComment.isEmpty()) sText += QStringLiteral("        <comment>%1</comment>\n").arg(xmlEscape(message.sComment));
            if (bNumerus) {
                sText += QStringLiteral("        <translation>\n");
                for (qint32 t = 0; t < message.listTranslations.size(); ++t) {
                    sText += QStringLiteral("            <numerusform>%1</numerusform>\n").arg(xmlEscape(message.listTranslations.at(t)));
                }
                sText += QStringLiteral("        </translation>\n");
            } else {
                const QString sTranslation = message.listTranslations.isEmpty() ? QString() : message.listTranslations.first();
                sText += QStringLiteral("        <translation>%1</translation>\n").arg(xmlEscape(sTranslation));
            }
            sText += QStringLiteral("    </message>\n");
        }
        sText += QStringLiteral("</context>\n");
    }
    sText += QStringLiteral("</TS>\n");
    return sText.toUtf8();
}

bool XQtQM::readSource(QByteArray *pData, PDSTRUCT *pPdStruct)
{
    if (!pData || !isPdStructNotCanceled(pPdStruct)) return false;
    const qint64 nSize = getSize();
    if ((nSize < 16) || (nSize > QM_MAX_SOURCE) || (nSize > (std::numeric_limits<int>::max)())) return false;
    const QByteArray baMagic = read_array_process(0, 16, pPdStruct);
    if ((baMagic.size() != 16)) return false;
    for (qint32 i = 0; i < 16; ++i) {
        if (static_cast<quint8>(baMagic.at(i)) != QM_MAGIC[i]) return false;
    }
    *pData = read_array_process(0, nSize, pPdStruct);
    return (pData->size() == nSize) && isPdStructNotCanceled(pPdStruct);
}

QString XQtQM::memberName()
{
    const QString sFileName = XBinary::getDeviceFileName(getDevice());
    QString sStem;
    if (!sFileName.isEmpty()) sStem = QFileInfo(sFileName).completeBaseName();
    sStem = XBinary::fixFileName(sStem);
    if (sStem.isEmpty() || sStem.contains(QLatin1Char('/'))) sStem = QStringLiteral("translation");
    return sStem + QStringLiteral(".ts");
}

bool XQtQM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQtQM archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XQtQM::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baSource;
    CATALOG catalog;
    return readSource(&baSource, pPdStruct) && parseCatalog(baSource, &catalog, pPdStruct) && isPdStructNotCanceled(pPdStruct);
}

XBinary::FT XQtQM::getFileType()
{
    return FT_QT_QM;
}
XBinary::MODE XQtQM::getMode()
{
    return MODE_DATA;
}
qint32 XQtQM::getType()
{
    return TYPE_ARCHIVE;
}
XBinary::ENDIAN XQtQM::getEndian()
{
    return ENDIAN_BIG;
}
QString XQtQM::getFileFormatExt()
{
    return QStringLiteral("qm");
}
QString XQtQM::getFileFormatExtsString()
{
    return QStringLiteral("Qt compiled translation (*.qm)");
}
QString XQtQM::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}
qint64 XQtQM::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}
XBinary::OSNAME XQtQM::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}
QString XQtQM::getVersion()
{
    return QString();
}
QList<QString> XQtQM::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("3CB86418CAEF9C95CD211CBF60A1BDDD");
}
XBinary *XQtQM::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQtQM(pDevice);
}

bool XQtQM::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!bindUnpackSource(pState, pPdStruct)) return false;

    QByteArray baSource;
    CATALOG catalog;
    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    bool bResult = pContext && readSource(&baSource, pPdStruct) && parseCatalog(baSource, &catalog, pPdStruct);
    if (bResult) {
        pContext->sName = memberName();
        catalog.sLanguage = normalizeLanguage(catalog.sLanguage);
        if (catalog.sLanguage.isEmpty()) catalog.sLanguage = languageFromFileName(XBinary::getDeviceFileName(getDevice()));
        pContext->baText = renderTS(catalog);
        pContext->nMessages = catalog.listMessages.size();
        pContext->sLanguage = catalog.sLanguage;
        bResult = (pContext->baText.size() <= QM_MAX_TEXT);
    }
    if (!bResult) {
        delete pContext;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = baSource.size();
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

XBinary::ARCHIVERECORD XQtQM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1))
        return ARCHIVERECORD();
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(pContext->baText.size()));
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Qt Linguist TS (synthesised, %1 messages)").arg(pContext->nMessages));
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XQtQM::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1) || devicesAlias(getDevice(), pDevice))
        return false;

    QIODevice *guardedOutput = pDevice;
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nSize = pContext->baText.size();
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nSize)) return false;

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->sName) && pState->spOutputBudget->isEnforcing()) return false;
        if (!pState->spOutputBudget->debit(nSize) && pState->spOutputBudget->isEnforcing()) return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nSize, pPdStruct));
    if (!pStage || !guardedOutput || ((nSize > 0) && (pStage->write(pContext->baText) != nSize)) || !pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
    if (bResult) pState->nCurrentOffset = nSize;
    return bResult;
}

bool XQtQM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex == pState->nNumberOfRecords) ? pState->nTotalSize : 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XQtQM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XQtQM::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD};
}
