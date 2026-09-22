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
#include "xsetupfactory.h"

#include "Algos/xdcldecoder.h"

#include <QDateTime>
#include <QUuid>

#include <cstring>
#include <memory>
#include <new>

namespace {
const uchar SETUPFACTORY_MAGIC[8] = {0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7};
const qint64 SETUPFACTORY_ENGINE_HEADER = 0x10c;
const qint64 SETUPFACTORY_MAX_PACKED = 512LL * 1024 * 1024;
const qint64 SETUPFACTORY_MAX_MANIFEST = 64LL * 1024 * 1024;
const qint64 SETUPFACTORY_MAX_TOTAL_OUTPUT = 2LL * 1024 * 1024 * 1024;

quint16 sfLe16(const char *p)
{
    const uchar *u = reinterpret_cast<const uchar *>(p);
    return quint16(u[0]) | (quint16(u[1]) << 8);
}

quint32 sfLe32(const char *p)
{
    const uchar *u = reinterpret_cast<const uchar *>(p);
    return quint32(u[0]) | (quint32(u[1]) << 8) | (quint32(u[2]) << 16) | (quint32(u[3]) << 24);
}

bool sfRange(qint64 size, qint64 offset, qint64 length)
{
    return (size >= 0) && (offset >= 0) && (length >= 0) && (offset <= size) && (length <= (size - offset));
}

bool sfCString(const QByteArray &data, qint64 *pOffset, QString *pValue)
{
    if (!pOffset || !pValue || !sfRange(data.size(), *pOffset, 1)) return false;
    qint64 offset = *pOffset;
    quint32 length = quint8(data.at(qint32(offset++)));
    if (length == 0xff) {
        if (!sfRange(data.size(), offset, 2)) return false;
        length = sfLe16(data.constData() + offset);
        offset += 2;
        if (length == 0xffff) {
            if (!sfRange(data.size(), offset, 4)) return false;
            length = sfLe32(data.constData() + offset);
            offset += 4;
        }
    }
    if ((length > 0x100000) || !sfRange(data.size(), offset, length)) return false;
    const QByteArray bytes = data.mid(qint32(offset), qint32(length));
    if (bytes.contains('\0')) return false;
    *pValue = QString::fromLatin1(bytes);
    *pOffset = offset + length;
    return true;
}

QString sfSafePath(QString directory, QString fileName)
{
    directory.replace('\\', '/');
    fileName.replace('\\', '/');
    directory = directory.trimmed();
    fileName = fileName.trimmed();
    if (directory.startsWith('%')) {
        const qint32 endVariable = directory.indexOf('%', 1);
        if (endVariable < 2) return QString();
        directory = directory.mid(endVariable + 1);
    }
    while (directory.startsWith('/')) directory.remove(0, 1);
    while (fileName.startsWith('/')) return QString();
    QString joined = directory.isEmpty() ? fileName : directory + QLatin1Char('/') + fileName;
    const QStringList parts = joined.split('/', Qt::SkipEmptyParts);
    QStringList safe;
    for (const QString &part : parts) {
        if (part == QLatin1String(".")) continue;
        if (part.isEmpty() || (part == QLatin1String("..")) || part.contains(':')) return QString();
        safe.append(part);
    }
    return safe.join('/');
}

quint32 sfCRC32(const QByteArray &data)
{
    return XBinary::_getCRC32(data, 0xffffffffU, XBinary::_getCRC32Table_EDB88320()) ^ 0xffffffffU;
}

class SETUPFACTORY_PARSE_FINISH {
public:
    SETUPFACTORY_PARSE_FINISH(QIODevice *pSource,
                              qint64 nSavedPosition)
        : m_pSource(pSource), m_nSavedPosition(nSavedPosition)
    {
    }

    bool operator()(bool bResult) const
    {
        return m_pSource && m_pSource->seek(m_nSavedPosition) && bResult;
    }

private:
    QIODevice *m_pSource;
    qint64 m_nSavedPosition;
};

class SETUPFACTORY_READ_AT {
public:
    SETUPFACTORY_READ_AT(QIODevice *pSource,
                         qint64 nSourceSize)
        : m_pSource(pSource), m_nSourceSize(nSourceSize)
    {
    }

    bool operator()(qint64 nOffset, qint64 nSize, QByteArray *pData) const
    {
        if (!pData || !m_pSource ||
            !sfRange(m_nSourceSize, nOffset, nSize) ||
            !m_pSource->seek(nOffset)) {
            return false;
        }
        *pData = m_pSource->read(nSize);
        return m_pSource && (pData->size() == nSize);
    }

private:
    QIODevice *m_pSource;
    qint64 m_nSourceSize;
};
}  // namespace

XSetupFactory::XSetupFactory(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    setIsArchive(true);
}

XSetupFactory::~XSetupFactory()
{
    const QSet<UNPACK_CONTEXT *> contexts = m_setContexts;
    m_setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) delete pContext;
}

bool XSetupFactory::_scanEngine(QList<ENGINE_ENTRY> *pEntries, qint64 *pnPayloadOffset, qint64 *pnSourceSize, bool *pbIs64, QString *psVersion,
                                PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pnPayloadOffset) *pnPayloadOffset = 0;
    if (pnSourceSize) *pnSourceSize = 0;
    if (pbIs64) *pbIs64 = false;
    if (psVersion) psVersion->clear();
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *source = getDevice();
    if (!source || !source->isOpen() || !source->isReadable() || source->isSequential()) return false;
    const qint64 sourceSize = source->size();
    const qint64 savedPosition = source->pos();
    if ((sourceSize < 0x200) || (savedPosition < 0)) return false;
    const SETUPFACTORY_PARSE_FINISH finish(source, savedPosition);
    const SETUPFACTORY_READ_AT readAt(source, sourceSize);

    QByteArray dos;
    if (!readAt(0, 0x40, &dos) || !dos.startsWith("MZ")) return finish(false);
    const qint64 peOffset = sfLe32(dos.constData() + 0x3c);
    QByteArray pe;
    if (!readAt(peOffset, 24, &pe) || (std::memcmp(pe.constData(), "PE\0\0", 4) != 0)) return finish(false);
    const quint16 sectionCount = sfLe16(pe.constData() + 6);
    const quint16 optionalSize = sfLe16(pe.constData() + 20);
    if (!sectionCount || (sectionCount > 96) || (optionalSize < 2)) return finish(false);
    QByteArray optional;
    if (!readAt(peOffset + 24, optionalSize, &optional)) return finish(false);
    const quint16 optionalMagic = sfLe16(optional.constData());
    const bool is64 = optionalMagic == 0x20b;
    if ((optionalMagic != 0x10b) && !is64) return finish(false);
    QByteArray sections;
    const qint64 sectionTable = peOffset + 24 + optionalSize;
    if (!readAt(sectionTable, qint64(sectionCount) * 40, &sections)) return finish(false);
    qint64 overlayOffset = 0;
    for (quint32 i = 0; i < sectionCount; ++i) {
        const char *row = sections.constData() + qint64(i) * 40;
        const qint64 rawSize = sfLe32(row + 16);
        const qint64 rawOffset = sfLe32(row + 20);
        if (!sfRange(sourceSize, rawOffset, rawSize)) return finish(false);
        overlayOffset = qMax(overlayOffset, rawOffset + rawSize);
    }
    QByteArray header;
    if (!readAt(overlayOffset, 12, &header) || (std::memcmp(header.constData(), SETUPFACTORY_MAGIC, 8) != 0)) return finish(false);
    const quint32 engineCount = sfLe32(header.constData() + 8);
    if (!engineCount || (engineCount > 64)) return finish(false);

    QList<ENGINE_ENTRY> entries;
    entries.reserve(engineCount);
    qint64 cursor = overlayOffset + 12;
    bool hasManifest = false;
    QString version;
    for (quint32 i = 0; i < engineCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return finish(false);
        QByteArray record;
        if (!readAt(cursor, SETUPFACTORY_ENGINE_HEADER, &record)) return finish(false);
        const qint32 terminator = record.left(260).indexOf('\0');
        if (terminator < 1) return finish(false);
        for (qint32 j = terminator; j < 260; ++j) {
            if (record.at(j) != 0) return finish(false);
        }
        const QByteArray nameBytes = record.left(terminator);
        for (char value : nameBytes) {
            const uchar u = uchar(value);
            if ((u < 0x20) || (u > 0x7e)) return finish(false);
        }
        ENGINE_ENTRY entry;
        entry.sName = QString::fromLatin1(nameBytes);
        entry.nPackedSize = sfLe32(record.constData() + 260);
        entry.nCRC32 = sfLe32(record.constData() + 264);
        entry.nDataOffset = cursor + SETUPFACTORY_ENGINE_HEADER;
        if ((entry.nPackedSize < 2) || (entry.nPackedSize > SETUPFACTORY_MAX_PACKED) || !sfRange(sourceSize, entry.nDataOffset, entry.nPackedSize)) {
            return finish(false);
        }
        const QString lower = entry.sName.toLower();
        if (lower == QLatin1String("irsetup.dat")) hasManifest = true;
        if (version.isEmpty() && lower.startsWith(QLatin1String("suf"))) {
            const qint32 lng = lower.indexOf(QLatin1String("lng"), 3);
            if (lng > 3) {
                bool ok = false;
                const qint32 major = lower.mid(3, lng - 3).toInt(&ok);
                if (ok && (major > 0) && (major < 100)) version = QStringLiteral("%1.0").arg(major);
            }
        }
        entries.append(entry);
        cursor = entry.nDataOffset + entry.nPackedSize;
    }
    if (!hasManifest || (cursor >= sourceSize)) return finish(false);
    if (pEntries) *pEntries = entries;
    if (pnPayloadOffset) *pnPayloadOffset = cursor;
    if (pnSourceSize) *pnSourceSize = sourceSize;
    if (pbIs64) *pbIs64 = is64;
    if (psVersion) *psVersion = version;
    return finish(true);
}

bool XSetupFactory::_buildEntries(QList<FILE_ENTRY> *pEntries, qint64 *pnSourceSize, PDSTRUCT *pPdStruct)
{
    if (!pEntries || !pnSourceSize) return false;
    pEntries->clear();
    *pnSourceSize = 0;
    QList<ENGINE_ENTRY> engines;
    qint64 payloadOffset = 0;
    qint64 sourceSize = 0;
    if (!_scanEngine(&engines, &payloadOffset, &sourceSize, nullptr, nullptr, pPdStruct)) return false;
    ENGINE_ENTRY manifestEntry;
    bool foundManifest = false;
    for (const ENGINE_ENTRY &entry : engines) {
        if (entry.sName.compare(QLatin1String("irsetup.dat"), Qt::CaseInsensitive) == 0) {
            manifestEntry = entry;
            foundManifest = true;
            break;
        }
    }
    if (!foundManifest) return false;
    QIODevice *source = getDevice();
    if (!source || (source->size() != sourceSize)) return false;
    const qint64 savedPosition = source->pos();
    if ((savedPosition < 0) || !source->seek(manifestEntry.nDataOffset)) return false;
    const QByteArray packedManifest = source->read(manifestEntry.nPackedSize);
    const bool restored = source && source->seek(savedPosition);
    if (!restored || (packedManifest.size() != manifestEntry.nPackedSize)) return false;
    QByteArray manifest;
    qint64 consumed = 0;
    if (!XDclDecoder::decode(packedManifest, &manifest, SETUPFACTORY_MAX_MANIFEST, &consumed) || (consumed != manifestEntry.nPackedSize) ||
        (sfCRC32(manifest) != manifestEntry.nCRC32) || (manifest.size() < 2)) {
        return false;
    }

    qint64 offset = 0;
    const quint32 fileCount = sfLe16(manifest.constData());
    offset = 2;
    if (!fileCount || (fileCount > 10000)) return false;
    QList<FILE_ENTRY> entries;
    entries.reserve(fileCount);
    qint64 streamOffset = payloadOffset;
    qint64 totalRaw = 0;
    for (quint32 i = 0; i < fileCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !sfRange(manifest.size(), offset, 2)) return false;
        const quint16 classTag = sfLe16(manifest.constData() + offset);
        offset += 2;
        if (i == 0) {
            if ((classTag != 0xffff) || !sfRange(manifest.size(), offset, 4)) return false;
            const quint16 schema = sfLe16(manifest.constData() + offset);
            const quint16 classNameSize = sfLe16(manifest.constData() + offset + 2);
            offset += 4;
            if ((schema != 3) || (classNameSize != 9) || !sfRange(manifest.size(), offset, classNameSize) ||
                (manifest.mid(qint32(offset), classNameSize) != QByteArray("CFileInfo", 9))) {
                return false;
            }
            offset += classNameSize;
        } else if (classTag != 0x8001) {
            return false;
        }
        if (!sfRange(manifest.size(), offset, 4)) return false;
        offset += 4;  // serialized object/reference field
        QString sourcePath;
        QString fileName;
        QString sourceDirectory;
        QString extension;
        QString unused;
        if (!sfCString(manifest, &offset, &sourcePath) || !sfCString(manifest, &offset, &fileName) ||
            !sfCString(manifest, &offset, &sourceDirectory) || !sfCString(manifest, &offset, &extension) || !sfCString(manifest, &offset, &unused) ||
            !sfRange(manifest.size(), offset, 4 + 13 + 25)) {
            return false;
        }
        FILE_ENTRY entry;
        entry.nRawSize = sfLe32(manifest.constData() + offset);
        offset += 4;
        entry.nUnixTime = sfLe32(manifest.constData() + offset + 5);  // middle of three time_t fields
        offset += 13;
        offset += 25;
        QString destination;
        QString shortcutName;
        QString shortcutGroup;
        if (!sfCString(manifest, &offset, &destination) || !sfRange(manifest.size(), offset, 5)) return false;
        offset += 5;
        if (!sfCString(manifest, &offset, &shortcutName) || !sfRange(manifest.size(), offset, 9)) return false;
        offset += 9;
        if (!sfCString(manifest, &offset, &shortcutGroup) || !sfRange(manifest.size(), offset, 17 + 8 + 37)) return false;
        const quint8 compression = quint8(manifest.at(qint32(offset + 6)));
        offset += 17;
        entry.nPackedSize = sfLe32(manifest.constData() + offset);
        entry.nCRC32 = sfLe32(manifest.constData() + offset + 4);
        offset += 8;
        offset += 37;
        // Some schema-3 writers leave the compression marker set on stored
        // records; equal packed/raw sizes are the authoritative store shape.
        entry.bStored = (compression == 0) || (entry.nPackedSize == entry.nRawSize);
        entry.sName = sfSafePath(destination, fileName);
        entry.nDataOffset = streamOffset;
        if ((compression > 1) || entry.sName.isEmpty() || (entry.nRawSize > SETUPFACTORY_MAX_PACKED) ||
            (entry.nPackedSize > SETUPFACTORY_MAX_PACKED) || (entry.bStored && (entry.nPackedSize != entry.nRawSize)) ||
            !sfRange(sourceSize, entry.nDataOffset, entry.nPackedSize)) {
            return false;
        }
        streamOffset += entry.nPackedSize;
        totalRaw += entry.nRawSize;
        if (totalRaw > SETUPFACTORY_MAX_TOTAL_OUTPUT) return false;
        entries.append(entry);
    }
    if ((entries.size() != qint32(fileCount)) || (streamOffset != sourceSize)) return false;
    *pEntries = entries;
    *pnSourceSize = sourceSize;
    return true;
}

bool XSetupFactory::isValid(PDSTRUCT *pPdStruct)
{
    return _scanEngine(nullptr, nullptr, nullptr, nullptr, nullptr, pPdStruct);
}

bool XSetupFactory::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSetupFactory x(pDevice);
    return x.isValid(pPdStruct);
}

XBinary::FT XSetupFactory::getFileType()
{
    bool is64 = false;
    return _scanEngine(nullptr, nullptr, nullptr, &is64, nullptr, nullptr) && is64 ? FT_PE64_SETUPFACTORY : FT_PE32_SETUPFACTORY;
}

XBinary::MODE XSetupFactory::getMode()
{
    return getFileType() == FT_PE64_SETUPFACTORY ? MODE_64 : MODE_32;
}

QString XSetupFactory::getVersion()
{
    QString version;
    _scanEngine(nullptr, nullptr, nullptr, nullptr, &version, nullptr);
    return version;
}

QString XSetupFactory::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XSetupFactory::getFileFormatExtsString()
{
    return QStringLiteral("Setup Factory installer (*.exe)");
}

QString XSetupFactory::getMIMEString()
{
    return QStringLiteral("application/vnd.microsoft.portable-executable");
}

QList<QString> XSetupFactory::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("4D5A");
}

XBinary *XSetupFactory::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    return new XSetupFactory(pDevice, bIsImage, nModuleAddress);
}

bool XSetupFactory::_isContextCurrent(const UNPACK_STATE *pState, const UNPACK_CONTEXT *pContext)
{
    return pState && pContext && m_setContexts.contains(const_cast<UNPACK_CONTEXT *>(pContext)) && (pState->pContext == pContext) &&
           (pContext->pOwnerState == pState) && !pState->baUnpackSourceToken.isEmpty() && (pState->baUnpackSourceToken == pContext->baToken) &&
           (pContext->pSourceDevice == getDevice()) && (pContext->nDeviceGeneration == getDeviceGeneration()) &&
           (pState->nTotalSize == pContext->nSourceSize) && (pState->nNumberOfRecords == pContext->listEntries.size()) &&
           (pState->nCurrentIndex == pContext->nCurrentIndex) && (pState->nCurrentOffset == pContext->nCurrentOffset);
}

bool XSetupFactory::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (pState->pContext || !pState->baUnpackSourceToken.isEmpty()) {
        UNPACK_CONTEXT *pOld = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!_isContextCurrent(pState, pOld)) return false;
        m_setContexts.remove(pOld);
        delete pOld;
    }
    *pState = UNPACK_STATE();
    QList<FILE_ENTRY> entries;
    qint64 sourceSize = 0;
    if (!_buildEntries(&entries, &sourceSize, pPdStruct) || entries.isEmpty()) return false;
    std::unique_ptr<UNPACK_CONTEXT> context(new (std::nothrow) UNPACK_CONTEXT);
    if (!context) return false;
    context->listEntries = entries;
    context->pSourceDevice = getDevice();
    context->pOwnerState = pState;
    context->nDeviceGeneration = getDeviceGeneration();
    context->nSourceSize = sourceSize;
    context->baToken = QUuid::createUuid().toRfc4122();
    if (!context->pSourceDevice || context->baToken.isEmpty()) return false;
    pState->nTotalSize = sourceSize;
    pState->nNumberOfRecords = entries.size();
    pState->mapUnpackProperties = mapProperties;
    pState->pContext = context.get();
    pState->baUnpackSourceToken = context->baToken;
    m_setContexts.insert(context.release());
    return true;
}

XBinary::ARCHIVERECORD XSetupFactory::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext) return result;
    UNPACK_CONTEXT *context = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, context) || (context->nCurrentIndex < 0) || (context->nCurrentIndex >= context->listEntries.size())) return result;
    const FILE_ENTRY &entry = context->listEntries.at(context->nCurrentIndex);
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nPackedSize;
    result.mapProperties[FPART_PROP_ORIGINALNAME] = entry.sName;
    result.mapProperties[FPART_PROP_COMPRESSEDSIZE] = entry.nPackedSize;
    result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = entry.nRawSize;
    result.mapProperties[FPART_PROP_HANDLEMETHOD] = entry.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    result.mapProperties[FPART_PROP_CRC_TYPE] = CRC_TYPE_EDB88320;
    result.mapProperties[FPART_PROP_RESULTCRC] = entry.nCRC32;
    result.mapProperties[FPART_PROP_DATETIME] = QDateTime::fromSecsSinceEpoch(entry.nUnixTime);
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    return result;
}

bool XSetupFactory::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pDevice || !pDevice->isOpen() || !pDevice->isWritable() || pDevice->isSequential() ||
        (pDevice->openMode() & (QIODevice::Append | QIODevice::Text)) || !isResizeEnable(pDevice) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    UNPACK_CONTEXT *context = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, context) || devicesAlias(context->pSourceDevice, pDevice) || (context->nCurrentIndex < 0) ||
        (context->nCurrentIndex >= context->listEntries.size())) {
        return false;
    }
    const FILE_ENTRY entry = context->listEntries.at(context->nCurrentIndex);
    QIODevice *source = context->pSourceDevice;
    const qint64 savedPosition = source ? source->pos() : -1;
    if (!source || (savedPosition < 0) || !source->seek(entry.nDataOffset)) return false;
    const QByteArray packed = source->read(entry.nPackedSize);
    const bool restored = source && source->seek(savedPosition);
    if (!restored || !_isContextCurrent(pState, context) || (packed.size() != entry.nPackedSize)) return false;
    QByteArray raw;
    if (entry.bStored) {
        raw = packed;
    } else {
        qint64 consumed = 0;
        if (!XDclDecoder::decode(packed, &raw, entry.nRawSize, &consumed) || (consumed != entry.nPackedSize)) return false;
    }
    if ((raw.size() != entry.nRawSize) || (sfCRC32(raw) != entry.nCRC32)) return false;
    if (pState->spOutputBudget && !pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sName) && pState->spOutputBudget->isEnforcing()) {
        setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (!resize(pDevice, 0) || !pDevice->seek(0)) return false;
    UNPACK_STATE writeState = *pState;
    writeState.nCurrentOffset = 0;
    const bool written = writeUnpackData(&writeState, pDevice, raw, pPdStruct);
    if (!written || !_isContextCurrent(pState, context) || (pDevice->size() != raw.size())) {
        resize(pDevice, 0);
        pDevice->seek(0);
        return false;
    }
    context->nCurrentOffset = writeState.nCurrentOffset;
    pState->nCurrentOffset = writeState.nCurrentOffset;
    return true;
}

bool XSetupFactory::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext) return false;
    UNPACK_CONTEXT *context = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, context) || (context->nCurrentIndex < 0) || (context->nCurrentIndex >= context->listEntries.size())) return false;
    ++context->nCurrentIndex;
    context->nCurrentOffset = 0;
    pState->nCurrentIndex = context->nCurrentIndex;
    pState->nCurrentOffset = 0;
    return context->nCurrentIndex < context->listEntries.size();
}

bool XSetupFactory::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    if (!pState->pContext && pState->baUnpackSourceToken.isEmpty()) {
        *pState = UNPACK_STATE();
        return true;
    }
    UNPACK_CONTEXT *context = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, context)) return false;
    m_setContexts.remove(context);
    *pState = UNPACK_STATE();
    delete context;
    return true;
}
