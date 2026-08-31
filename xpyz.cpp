/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xpyz.h"

#include "xzlib.h"

#include <QBuffer>
#include <QVector>
#include <QtEndian>

#include <cstring>
#include <limits>

namespace
{
class MarshalReader
{
public:
    explicit MarshalReader(const QByteArray &data)
        : m_data(data), m_position(0), m_objects(0)
    {
    }

    bool read(QVariant *pValue)
    {
        return readObject(pValue, 0) && (m_position == m_data.size());
    }

private:
    bool take(qint64 nSize, const char **ppData)
    {
        if (!ppData || (nSize < 0) || (m_position < 0) ||
            (m_position > m_data.size()) ||
            (nSize > (m_data.size() - m_position)))
            return false;
        *ppData = m_data.constData() + m_position;
        m_position += nSize;
        return true;
    }

    bool readLength(qint64 *pLength)
    {
        const char *pData = nullptr;
        if (!pLength || !take(4, &pData)) return false;
        const qint32 nLength = qFromLittleEndian<qint32>(
            reinterpret_cast<const uchar *>(pData));
        if ((nLength < 0) || (nLength > 200000)) return false;
        *pLength = nLength;
        return true;
    }

    bool readString(qint64 nLength, QVariant *pValue)
    {
        const char *pData = nullptr;
        if (!pValue || (nLength < 0) || (nLength > (16 * 1024 * 1024)) ||
            !take(nLength, &pData))
            return false;
        const QByteArray bytes(pData, static_cast<int>(nLength));
        QString text = QString::fromUtf8(bytes);
        if (text.contains(QChar::ReplacementCharacter))
            text = QString::fromLatin1(bytes);
        *pValue = text.normalized(QString::NormalizationForm_C);
        return true;
    }

    bool readObject(QVariant *pValue, qint32 nDepth)
    {
        if (!pValue || (nDepth > 64) || (++m_objects > 500000)) return false;
        const char *pCode = nullptr;
        if (!take(1, &pCode)) return false;
        const quint8 nCode = static_cast<quint8>(*pCode);
        const bool bStoreReference = (nCode & 0x80U) != 0;
        const char type = static_cast<char>(nCode & 0x7fU);
        const qint32 nReference = bStoreReference ? m_references.count() : -1;
        if (bStoreReference) m_references.append(QVariant());

        QVariant value;
        if ((type == 'N') || (type == '0'))
        {
            value = QVariant();
        }
        else if ((type == 'F') || (type == 'T'))
        {
            value = (type == 'T');
        }
        else if ((type == 'i') || (type == 'I'))
        {
            const qint64 nSize = (type == 'i') ? 4 : 8;
            const char *pData = nullptr;
            if (!take(nSize, &pData)) return false;
            value = (type == 'i')
                        ? static_cast<qint64>(qFromLittleEndian<qint32>(
                              reinterpret_cast<const uchar *>(pData)))
                        : qFromLittleEndian<qint64>(
                              reinterpret_cast<const uchar *>(pData));
        }
        else if (type == 'l')
        {
            const char *pData = nullptr;
            if (!take(4, &pData)) return false;
            const qint32 nDigits = qFromLittleEndian<qint32>(
                reinterpret_cast<const uchar *>(pData));
            if (nDigits == (std::numeric_limits<qint32>::min)()) return false;
            const qint32 nCount = qAbs(nDigits);
            if (nCount > 5) return false;
            quint64 nValue = 0;
            quint64 nMultiplier = 1;
            for (qint32 i = 0; i < nCount; ++i)
            {
                if (!take(2, &pData)) return false;
                const quint16 nDigit = qFromLittleEndian<quint16>(
                    reinterpret_cast<const uchar *>(pData));
                if (nDigit >= 0x8000U) return false;
                nValue += static_cast<quint64>(nDigit) * nMultiplier;
                nMultiplier <<= 15;
            }
            if (nValue > static_cast<quint64>((std::numeric_limits<qint64>::max)()))
                return false;
            value = nDigits < 0 ? -static_cast<qint64>(nValue)
                                : static_cast<qint64>(nValue);
        }
        else if ((type == 's') || (type == 't') || (type == 'u') ||
                 (type == 'a') || (type == 'A'))
        {
            qint64 nLength = 0;
            if (!readLength(&nLength) || !readString(nLength, &value))
                return false;
        }
        else if ((type == 'z') || (type == 'Z'))
        {
            const char *pLength = nullptr;
            if (!take(1, &pLength) ||
                !readString(static_cast<quint8>(*pLength), &value))
                return false;
        }
        else if ((type == '(') || (type == ')') || (type == '['))
        {
            qint64 nCount = 0;
            if (type == ')')
            {
                const char *pCount = nullptr;
                if (!take(1, &pCount)) return false;
                nCount = static_cast<quint8>(*pCount);
            }
            else if (!readLength(&nCount))
            {
                return false;
            }
            QVariantList list;
            list.reserve(static_cast<int>(nCount));
            for (qint64 i = 0; i < nCount; ++i)
            {
                QVariant item;
                if (!readObject(&item, nDepth + 1)) return false;
                list.append(item);
            }
            value = list;
        }
        else if (type == 'r')
        {
            const char *pData = nullptr;
            if (!take(4, &pData)) return false;
            const qint32 nIndex = qFromLittleEndian<qint32>(
                reinterpret_cast<const uchar *>(pData));
            if ((nIndex < 0) || (nIndex >= m_references.count())) return false;
            value = m_references.at(nIndex);
            if (!value.isValid()) return false;
        }
        else
        {
            return false;
        }

        if (bStoreReference) m_references[nReference] = value;
        *pValue = value;
        return true;
    }

private:
    const QByteArray &m_data;
    qint64 m_position;
    qint64 m_objects;
    QVector<QVariant> m_references;
};
} // namespace

XPYZ::XPYZ(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_PYINSTALLER_PYZ)
{
}

bool XPYZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPYZ archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPYZ::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPYZ(pDevice);
}

bool XPYZ::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XPYZ> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 16) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    const QByteArray header = read_array_process(0, 12, pPdStruct);
    if (!guardedThis || (header.size() != 12) ||
        (std::memcmp(header.constData(), "PYZ\0", 4) != 0))
        return false;
    const qint64 nTocOffset = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar *>(header.constData()) + 8);
    if ((nTocOffset < 12) || (nTocOffset >= nTotalSize) ||
        ((nTotalSize - nTocOffset) > (64LL * 1024LL * 1024LL)))
        return false;

    const QByteArray tocData = read_array_process(
        nTocOffset, nTotalSize - nTocOffset, pPdStruct);
    if (!guardedThis || (tocData.size() != (nTotalSize - nTocOffset)))
        return false;
    QVariant root;
    MarshalReader reader(tocData);
    if (!reader.read(&root) || (root.type() != QVariant::List)) return false;
    const QVariantList toc = root.toList();
    if (toc.isEmpty() || (toc.count() > MAX_RECORDS)) return false;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    QList<QPair<qint64, qint64> > ranges;
    for (const QVariant &itemValue : toc)
    {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            (itemValue.type() != QVariant::List))
            return false;
        const QVariantList item = itemValue.toList();
        if ((item.count() != 2) || (item.at(0).type() != QVariant::String) ||
            (item.at(1).type() != QVariant::List))
            return false;
        const QVariantList descriptor = item.at(1).toList();
        if (descriptor.count() != 3) return false;
        bool okType = false;
        bool okOffset = false;
        bool okSize = false;
        const qint64 nType = descriptor.at(0).toLongLong(&okType);
        const qint64 nDataOffset = descriptor.at(1).toLongLong(&okOffset);
        const qint64 nDataSize = descriptor.at(2).toLongLong(&okSize);
        if (!okType || !okOffset || !okSize || (nType < 0) || (nType > 3) ||
            (nDataOffset < 12) || (nDataSize < 0) ||
            !rangeWithin(nTocOffset, nDataOffset, nDataSize))
            return false;
        if (nType == 3)
        {
            if (nDataSize != 0) return false;
            continue; // Namespace package marker; it has no byte stream.
        }
        if ((nType > 1) || (nDataSize < 6)) return false;
        for (const QPair<qint64, qint64> &range : ranges)
            if (rangesOverlap(nDataOffset, nDataSize, range.first, range.second))
                return false;
        ranges.append(qMakePair(nDataOffset, nDataSize));

        const QByteArray compressed = read_array_process(
            nDataOffset, nDataSize, pPdStruct);
        if (!guardedThis || (compressed.size() != nDataSize)) return false;
        QBuffer buffer;
        buffer.setData(compressed);
        if (!buffer.open(QIODevice::ReadOnly)) return false;
        XZlib zlib(&buffer);
        const qint64 nZlibSize = zlib.getFileFormatSize(pPdStruct);
        const QList<XArchive::RECORD> records =
            (nZlibSize == nDataSize) ? zlib.getRecords(1, pPdStruct)
                                     : QList<XArchive::RECORD>();
        buffer.close();
        if (!guardedThis || (records.count() != 1) ||
            (records.constFirst().spInfo.nUncompressedSize < 0))
            return false;

        QString path = item.at(0).toString();
        path.replace(QLatin1Char('.'), QLatin1Char('/'));
        path += nType == 1 ? QStringLiteral("/__init__.pyc.marshal")
                           : QStringLiteral(".pyc.marshal");
        path = XBinary::fixFileName(path);
        QString uniquePath;
        if (path.isEmpty() ||
            !makeUniquePath(path, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &uniquePath))
            return false;

        if (pEntries)
        {
            ENTRY entry = {};
            entry.nHeaderOffset = 0;
            entry.nHeaderSize = 12;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.nUncompressedSize =
                records.constFirst().spInfo.nUncompressedSize;
            entry.handleMethod = HANDLE_METHOD_ZLIB;
            entry.sFileName = uniquePath;
            pEntries->append(entry);
        }
    }

    if (ranges.isEmpty()) return false;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
