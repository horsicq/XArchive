/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xizpack.h"

#include <QDateTime>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// Java object-serialization tags.  Only the handful IzPack actually emits are
// implemented; anything else makes the walk fail closed.
const quint8 TC_NULL = 0x70;
const quint8 TC_REFERENCE = 0x71;
const quint8 TC_CLASSDESC = 0x72;
const quint8 TC_OBJECT = 0x73;
const quint8 TC_STRING = 0x74;
const quint8 TC_BLOCKDATA = 0x77;
const quint8 TC_ENDBLOCKDATA = 0x78;
const quint8 TC_BLOCKDATALONG = 0x7a;
const quint8 TC_LONGSTRING = 0x7c;
const qint32 IZPACK_BASE_WIRE_HANDLE = 0x7e0000;

// "AC ED 00 05" + "77 04" + int32 record count.  The class descriptor that
// follows is inspected in place, so the fixed part of the header is 10 bytes.
const qint64 IZPACK_HEADER_SIZE = 10;
const qint64 IZPACK_PROBE_SIZE = 0x34;
const char IZPACK_CLASS_NAME[] = "com.izforge.izpack.PackFile";
const qint32 IZPACK_CLASS_NAME_SIZE = 27;
const qint32 IZPACK_MAX_RECORDS = 1000000;
const qint32 IZPACK_MAX_HANDLES = 1 << 20;
const qint32 IZPACK_MAX_DEPTH = 64;
const qint32 IZPACK_MAX_FIELDS = 4096;
const qint64 IZPACK_MAX_MEMBER_SIZE = Q_INT64_C(0x40000000);  // 1 GiB
const qint64 IZPACK_BUFFER_SIZE = 0x10000;

// serialVersionUID (as stored, big endian) plus the declared field count is the
// only version marker in the stream.  The pairing matters: v5, v6 and v7 share
// one UID and differ solely in how many fields the descriptor declares.
struct IZPACK_VERSION_ENTRY {
    quint64 nUID;
    quint16 nFieldCount;
    qint32 nVersion;
};

const IZPACK_VERSION_ENTRY g_izpackVersions[] = {
    {Q_UINT64_C(0x76a523b293a5e5c3), 2, 1},
    {Q_UINT64_C(0x11e206a67610577a), 5, 2},
    {Q_UINT64_C(0x9856d67cc286140e), 7, 3},
    {Q_UINT64_C(0x99e53bc9ae638cb7), 8, 4},
    {Q_UINT64_C(0xf46bb277b6f32403), 10, 5},
    {Q_UINT64_C(0xf46bb277b6f32403), 11, 6},
    {Q_UINT64_C(0xf46bb277b6f32403), 12, 7},
};

qint32 izVersionFromClassDesc(quint64 nUID, quint16 nFieldCount)
{
    const qint32 nCount =
        static_cast<qint32>(sizeof(g_izpackVersions) / sizeof(g_izpackVersions[0]));
    for (qint32 i = 0; i < nCount; i++) {
        if ((g_izpackVersions[i].nUID == nUID) &&
            (g_izpackVersions[i].nFieldCount == nFieldCount)) {
            return g_izpackVersions[i].nVersion;
        }
    }
    return 0;
}

// A forward-only cursor with its own window over the device.  The record walk
// touches the stream a byte or a tag at a time; going through the device for
// each of those would make a five-megabyte pack unusable.
class IzCursor {
public:
    IzCursor(QIODevice *pDevice, qint64 nSize)
        : m_pDevice(pDevice), m_nSize(nSize), m_nPos(0), m_nBufferOffset(-1)
    {
    }

    qint64 pos() const
    {
        return m_nPos;
    }

    qint64 size() const
    {
        return m_nSize;
    }

    bool seek(qint64 nOffset)
    {
        if ((nOffset < 0) || (nOffset > m_nSize)) return false;
        m_nPos = nOffset;
        return true;
    }

    bool read(char *pBuffer, qint64 nSize)
    {
        if (nSize < 0) return false;
        if (nSize > (m_nSize - m_nPos)) return false;
        qint64 nLeft = nSize;
        char *pOut = pBuffer;
        while (nLeft > 0) {
            if (!fill(m_nPos)) return false;
            const qint64 nAvailable =
                m_baBuffer.size() - (m_nPos - m_nBufferOffset);
            if (nAvailable <= 0) return false;
            const qint64 nPortion = qMin(nAvailable, nLeft);
            if (pOut) {
                std::memcpy(pOut,
                            m_baBuffer.constData() + (m_nPos - m_nBufferOffset),
                            static_cast<size_t>(nPortion));
                pOut += nPortion;
            }
            m_nPos += nPortion;
            nLeft -= nPortion;
        }
        return true;
    }

    bool skip(qint64 nSize)
    {
        if ((nSize < 0) || (nSize > (m_nSize - m_nPos))) return false;
        m_nPos += nSize;
        return true;
    }

    bool u8(quint8 *pnValue)
    {
        char nByte = 0;
        if (!read(&nByte, 1)) return false;
        *pnValue = static_cast<quint8>(nByte);
        return true;
    }

    bool u16be(quint16 *pnValue)
    {
        char buffer[2] = {};
        if (!read(buffer, 2)) return false;
        *pnValue = qFromBigEndian<quint16>(reinterpret_cast<uchar *>(buffer));
        return true;
    }

    bool u32be(quint32 *pnValue)
    {
        char buffer[4] = {};
        if (!read(buffer, 4)) return false;
        *pnValue = qFromBigEndian<quint32>(reinterpret_cast<uchar *>(buffer));
        return true;
    }

    bool i32be(qint32 *pnValue)
    {
        quint32 nValue = 0;
        if (!u32be(&nValue)) return false;
        *pnValue = static_cast<qint32>(nValue);
        return true;
    }

    bool i64be(qint64 *pnValue)
    {
        char buffer[8] = {};
        if (!read(buffer, 8)) return false;
        *pnValue = static_cast<qint64>(
            qFromBigEndian<quint64>(reinterpret_cast<uchar *>(buffer)));
        return true;
    }

private:
    bool fill(qint64 nOffset)
    {
        if ((m_nBufferOffset >= 0) && (nOffset >= m_nBufferOffset) &&
            (nOffset < (m_nBufferOffset + m_baBuffer.size()))) {
            return true;
        }
        if (!m_pDevice) return false;
        const qint64 nPortion = qMin(IZPACK_BUFFER_SIZE, m_nSize - nOffset);
        if (nPortion <= 0) return false;
        m_baBuffer = XBinary::read_array(m_pDevice, nOffset, nPortion);
        if (m_baBuffer.size() != nPortion) {
            m_nBufferOffset = -1;
            return false;
        }
        m_nBufferOffset = nOffset;
        return true;
    }

    QPointer<QIODevice> m_pDevice;
    qint64 m_nSize;
    qint64 m_nPos;
    QByteArray m_baBuffer;
    qint64 m_nBufferOffset;
};

// One entry of the serialization handle table.  Only class descriptors carry
// anything worth remembering: their field type codes, needed to skip a
// back-referenced object's values.
struct IZ_HANDLE {
    bool bHasTypes;
    QByteArray baTypes;
};

class IzParser {
public:
    explicit IzParser(IzCursor *pCursor) : m_pCursor(pCursor), m_nDepth(0)
    {
    }

    IzCursor *cursor()
    {
        return m_pCursor;
    }

    bool appendHandle(const QByteArray &baTypes, bool bHasTypes)
    {
        if (m_listHandles.size() >= IZPACK_MAX_HANDLES) return false;
        IZ_HANDLE handle = {};
        handle.bHasTypes = bHasTypes;
        handle.baTypes = baTypes;
        m_listHandles.append(handle);
        return true;
    }

    qint32 handleCount() const
    {
        return m_listHandles.size();
    }

    // TC_CLASSDESC body: class name, serialVersionUID, flags, field table,
    // TC_ENDBLOCKDATA, superclass (always TC_NULL in this stream).
    bool classDesc()
    {
        if (!skipUtf()) return false;
        if (!m_pCursor->skip(9)) return false;  // uid(8) + classDescFlags(1)
        quint16 nFieldCount = 0;
        if (!m_pCursor->u16be(&nFieldCount)) return false;
        if (nFieldCount > IZPACK_MAX_FIELDS) return false;
        // The handle is claimed before the field table is read, because a field
        // type string inside it may already reference this descriptor.
        if (!appendHandle(QByteArray(), true)) return false;
        const qint32 nHandleIndex = m_listHandles.size() - 1;
        QByteArray baTypes;
        baTypes.reserve(nFieldCount);
        for (quint16 i = 0; i < nFieldCount; i++) {
            quint8 nTypeCode = 0;
            if (!m_pCursor->u8(&nTypeCode)) return false;
            if (!skipUtf()) return false;
            if ((nTypeCode == 'L') || (nTypeCode == '[')) {
                if (!readContent()) return false;
            } else if ((nTypeCode != 'B') && (nTypeCode != 'C') &&
                       (nTypeCode != 'D') && (nTypeCode != 'F') &&
                       (nTypeCode != 'I') && (nTypeCode != 'J') &&
                       (nTypeCode != 'S') && (nTypeCode != 'Z')) {
                return false;
            }
            baTypes.append(static_cast<char>(nTypeCode));
        }
        m_listHandles[nHandleIndex].baTypes = baTypes;
        quint8 nTag = 0;
        if (!m_pCursor->u8(&nTag) || (nTag != TC_ENDBLOCKDATA)) return false;
        if (!m_pCursor->u8(&nTag) || (nTag != TC_NULL)) return false;
        return true;
    }

    // Skip the field values of an object whose descriptor is already known.
    bool skipFields(const QByteArray &baTypes)
    {
        for (qint32 i = 0; i < baTypes.size(); i++) {
            const char nTypeCode = baTypes.at(i);
            if ((nTypeCode == 'B') || (nTypeCode == 'C') ||
                (nTypeCode == 'Z')) {
                if (!m_pCursor->skip(1)) return false;
            } else if (nTypeCode == 'S') {
                if (!m_pCursor->skip(2)) return false;
            } else if ((nTypeCode == 'F') || (nTypeCode == 'I')) {
                if (!m_pCursor->skip(4)) return false;
            } else if ((nTypeCode == 'J') || (nTypeCode == 'D')) {
                if (!m_pCursor->skip(8)) return false;
            } else if ((nTypeCode == 'L') || (nTypeCode == '[')) {
                if (!readContent()) return false;
            } else {
                return false;
            }
        }
        return true;
    }

    // Skip one serialized value of any shape IzPack can put in a PackFile
    // field.  Nothing is materialised: the walk only has to land on the byte
    // after the value.
    bool readContent()
    {
        if (m_nDepth >= IZPACK_MAX_DEPTH) return false;
        m_nDepth++;
        const bool bResult = readContentBody();
        m_nDepth--;
        return bResult;
    }

    // TC_STRING / TC_LONGSTRING / TC_NULL, materialised.  Used for the
    // sourcePath, relativePath and targetPath fields.
    bool readString(QString *psResult)
    {
        psResult->clear();
        quint8 nTag = 0;
        if (!m_pCursor->u8(&nTag)) return false;
        if (nTag == TC_STRING) {
            quint16 nLength = 0;
            if (!m_pCursor->u16be(&nLength)) return false;
            if (!readUtfBody(nLength, psResult)) return false;
        } else if (nTag == TC_LONGSTRING) {
            // The reference implementation reads a 32-bit length here; a real TC_LONGSTRING is 64-bit,
            // but IzPack never emits one and matching the reference implementation keeps the byte
            // positions identical.
            quint32 nLength = 0;
            if (!m_pCursor->u32be(&nLength)) return false;
            if (!readUtfBody(nLength, psResult)) return false;
        } else if (nTag == TC_NULL) {
            return true;  // no handle is allocated for a null
        } else {
            return false;
        }
        return appendHandle(QByteArray(), false);
    }

    // The osConstraints java.util.ArrayList: descriptor, size, the writeObject
    // capacity block, the elements, TC_ENDBLOCKDATA.
    bool readArrayList()
    {
        quint8 nTag = 0;
        if (!m_pCursor->u8(&nTag)) return false;
        if (nTag == TC_REFERENCE) {
            if (!m_pCursor->seek(m_pCursor->pos() - 1)) return false;
            return readContent();
        }
        if (nTag != TC_OBJECT) return false;
        if (!readContent()) return false;  // the class descriptor
        if (!appendHandle(QByteArray(), false)) return false;
        qint32 nSize = 0;
        if (!m_pCursor->i32be(&nSize)) return false;
        if ((nSize < 0) || (nSize > IZPACK_MAX_FIELDS)) return false;
        if (!readContent()) return false;  // the capacity block-data record
        for (qint32 i = 0; i < nSize; i++) {
            if (!readContent()) return false;
        }
        if (!m_pCursor->u8(&nTag) || (nTag != TC_ENDBLOCKDATA)) return false;
        return true;
    }

    // Walk the block-data framing that carries a member's bytes.
    //
    // The last chunk may run past the member: ObjectOutputStream had already
    // buffered the next few primitive writes when it flushed. The reference implementation accepts an
    // overrun of exactly 4, 8 or 12 bytes and skips it, and the record layout
    // only stays aligned if we do the same.
    bool scanBlockData(qint64 nSize, qint64 *pnFirstDataOffset,
                       qint32 *pnChunks, qint64 *pnExcess)
    {
        *pnFirstDataOffset = m_pCursor->pos();
        *pnChunks = 0;
        *pnExcess = 0;
        qint64 nLeft = nSize;
        bool bFirst = true;
        while (nLeft > 0) {
            quint8 nTag = 0;
            if (!m_pCursor->u8(&nTag)) return false;
            qint64 nChunkSize = 0;
            if (nTag == TC_BLOCKDATALONG) {
                quint32 nValue = 0;
                if (!m_pCursor->u32be(&nValue)) return false;
                nChunkSize = static_cast<qint64>(nValue);
            } else if (nTag == TC_BLOCKDATA) {
                quint8 nValue = 0;
                if (!m_pCursor->u8(&nValue)) return false;
                nChunkSize = static_cast<qint64>(nValue);
            } else {
                return false;
            }
            if (nChunkSize <= 0) return false;
            if (nLeft < nChunkSize) {
                const qint64 nExcess = nChunkSize - nLeft;
                if ((nExcess != 4) && (nExcess != 8) && (nExcess != 12)) {
                    return false;
                }
                *pnExcess = nExcess;
                nChunkSize = nLeft;
            }
            if (bFirst) {
                *pnFirstDataOffset = m_pCursor->pos();
                bFirst = false;
            }
            if (!m_pCursor->skip(nChunkSize)) return false;
            nLeft -= nChunkSize;
            (*pnChunks)++;
        }
        if (*pnExcess > 0) {
            if (!m_pCursor->skip(*pnExcess)) return false;
        }
        return true;
    }

private:
    bool skipUtf()
    {
        quint16 nLength = 0;
        if (!m_pCursor->u16be(&nLength)) return false;
        if (nLength) {
            if (!m_pCursor->skip(nLength)) return false;
        }
        return true;
    }

    bool readUtfBody(qint64 nLength, QString *psResult)
    {
        if ((nLength < 0) || (nLength > IZPACK_MAX_MEMBER_SIZE)) return false;
        if (nLength == 0) {
            psResult->clear();
            return true;
        }
        QByteArray baValue(static_cast<qint32>(nLength), 0);
        if (!m_pCursor->read(baValue.data(), nLength)) return false;
        *psResult = QString::fromUtf8(baValue);
        return true;
    }

    bool readContentBody()
    {
        quint8 nTag = 0;
        if (!m_pCursor->u8(&nTag)) return false;
        if (nTag == TC_NULL) return true;
        if (nTag == TC_STRING) {
            if (!skipUtf()) return false;
            return appendHandle(QByteArray(), false);
        }
        if (nTag == TC_LONGSTRING) {
            quint32 nLength = 0;
            if (!m_pCursor->u32be(&nLength)) return false;
            if (nLength && !m_pCursor->skip(nLength)) return false;
            return appendHandle(QByteArray(), false);
        }
        if (nTag == TC_REFERENCE) {
            qint32 nHandle = 0;
            if (!m_pCursor->i32be(&nHandle)) return false;
            nHandle -= IZPACK_BASE_WIRE_HANDLE;
            return (nHandle >= 0) && (nHandle < m_listHandles.size());
        }
        if (nTag == TC_CLASSDESC) return classDesc();
        if (nTag == TC_BLOCKDATA) {
            quint8 nLength = 0;
            if (!m_pCursor->u8(&nLength)) return false;
            if (nLength == 0) return true;
            return m_pCursor->skip(nLength);
        }
        if (nTag == TC_ENDBLOCKDATA) return true;
        if (nTag == TC_OBJECT) {
            quint8 nInner = 0;
            if (!m_pCursor->u8(&nInner)) return false;
            qint32 nDescriptorIndex = -1;
            if (nInner == TC_CLASSDESC) {
                if (!classDesc()) return false;
                nDescriptorIndex = m_listHandles.size() - 1;
            } else if (nInner == TC_REFERENCE) {
                qint32 nHandle = 0;
                if (!m_pCursor->i32be(&nHandle)) return false;
                nDescriptorIndex = nHandle - IZPACK_BASE_WIRE_HANDLE;
            } else {
                return false;
            }
            if (!appendHandle(QByteArray(), false)) return false;
            if ((nDescriptorIndex < 0) ||
                (nDescriptorIndex >= m_listHandles.size())) {
                return false;
            }
            const IZ_HANDLE &descriptor = m_listHandles.at(nDescriptorIndex);
            if (!descriptor.bHasTypes) return false;
            return skipFields(descriptor.baTypes);
        }
        return false;
    }

    IzCursor *m_pCursor;
    QList<IZ_HANDLE> m_listHandles;
    qint32 m_nDepth;
};
}  // namespace

XIzPack::XIzPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIzPack::~XIzPack()
{
}

bool XIzPack::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XIzPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < IZPACK_PROBE_SIZE) return false;

    const QByteArray baProbe =
        read_array_process(0, IZPACK_PROBE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baProbe.size() != IZPACK_PROBE_SIZE)) {
        return false;
    }
    const uchar *pProbe = reinterpret_cast<const uchar *>(baProbe.constData());

    // Java stream magic and version, then the record-count block-data record.
    if ((pProbe[0] != 0xac) || (pProbe[1] != 0xed) || (pProbe[2] != 0x00) ||
        (pProbe[3] != 0x05)) {
        return false;
    }
    if ((pProbe[4] != TC_BLOCKDATA) || (pProbe[5] != 0x04)) return false;
    // The first record's descriptor is inlined right after the count, and its
    // class name is the discriminator that keeps this out of every other Java
    // serialization stream in the world.
    if ((pProbe[10] != TC_OBJECT) || (pProbe[11] != TC_CLASSDESC)) return false;
    if (qFromBigEndian<quint16>(pProbe + 12) != IZPACK_CLASS_NAME_SIZE) {
        return false;
    }
    if (std::memcmp(pProbe + 14, IZPACK_CLASS_NAME,
                    IZPACK_CLASS_NAME_SIZE) != 0) {
        return false;
    }

    const quint64 nUID = qFromBigEndian<quint64>(pProbe + 0x29);
    const quint16 nFieldCount = qFromBigEndian<quint16>(pProbe + 0x32);
    context.nVersion = izVersionFromClassDesc(nUID, nFieldCount);
    if (context.nVersion == 0) return false;

    context.nDeclaredCount =
        static_cast<qint32>(qFromBigEndian<quint32>(pProbe + 6));
    if ((context.nDeclaredCount < 0) ||
        (context.nDeclaredCount > IZPACK_MAX_RECORDS)) {
        return false;
    }
    context.nHeaderSize = IZPACK_HEADER_SIZE;

    IzCursor cursor(guardedSource, context.nInputSize);
    if (!cursor.seek(IZPACK_HEADER_SIZE)) return false;
    IzParser parser(&cursor);

    const qint32 nVersion = context.nVersion;
    const quint32 nMask = 1U << nVersion;  // nVersion is 1..7

    for (qint32 nIndex = 0; nIndex < context.nDeclaredCount; nIndex++) {
        if (!isPdStructNotCanceled(pPdStruct) || !guardedThis ||
            !guardedSource) {
            return false;
        }

        RECORD record = {};
        record.nRecordOffset = cursor.pos();

        quint8 nTag = 0;
        if (!cursor.u8(&nTag) || (nTag != TC_OBJECT)) return false;
        if (!cursor.u8(&nTag)) return false;
        if (nTag == TC_CLASSDESC) {
            if (!parser.classDesc()) return false;
        } else if (nTag == TC_REFERENCE) {
            qint32 nHandle = 0;
            if (!cursor.i32be(&nHandle)) return false;
            // Every later record points back at the very first descriptor.
            if (nHandle != IZPACK_BASE_WIRE_HANDLE) return false;
        } else {
            return false;
        }
        if (!parser.appendHandle(QByteArray(), false)) return false;

        quint8 nIsDirectory = 0;
        if (!(nMask & 0x0e)) {
            if (!cursor.u8(&nIsDirectory)) return false;
        }
        qint64 nLength = 0;
        if (!cursor.i64be(&nLength)) return false;
        qint64 nMTime = 0;
        if (nVersion != 1) {
            if (!cursor.i64be(&nMTime)) return false;
        }
        qint64 nOffsetInPreviousPack = -1;
        if (!(nMask & 0x06)) {
            if (!cursor.i64be(&nOffsetInPreviousPack)) return false;
        }
        // override / previousPackNumber: two ints up to v5, one from v6 on,
        // none at all in v1.
        qint64 nIntFieldsSize = 8;
        if (nVersion == 1) nIntFieldsSize = 0;
        else if ((nVersion == 2) || (nVersion == 6) || (nVersion == 7)) nIntFieldsSize = 4;
        if (!cursor.skip(nIntFieldsSize)) return false;

        bool bPack200Jar = false;
        if (nVersion == 7) {
            quint8 nPack200 = 0;
            if (!cursor.u8(&nPack200)) return false;
            bPack200Jar = (nPack200 != 0);
        }
        if (nMask & 0xe0) {
            if (!parser.readContent()) return false;  // additionals (Map)
        }
        if (nMask & 0xc0) {
            if (!parser.readContent()) return false;  // condition (String)
        }
        if (nVersion != 1) {
            if (!parser.readArrayList()) return false;  // osConstraints
        }
        if (nMask & 0xc0) {
            if (!parser.readContent()) return false;  // previousPackId
        }
        if (nMask & 0xe0) {
            if (!parser.readString(&record.sSourcePath)) return false;
        }
        QString sTargetPath;
        if (!parser.readString(&sTargetPath)) return false;

        qint64 nFirstDataOffset = 0;
        qint32 nChunks = 0;
        qint64 nExcess = 0;

        if (nOffsetInPreviousPack != -1) {
            // The bytes live in an earlier pack of the same set; this record is
            // a pointer, not a member, and carries no block data at all.
            continue;
        }
        if (bPack200Jar) {
            // A Pack200-compressed JAR: the payload is not the file, so the reference implementation
            // steps over exactly four bytes of it and moves on.
            if (!parser.scanBlockData(4, &nFirstDataOffset, &nChunks,
                                      &nExcess)) {
                return false;
            }
            continue;
        }

        record.bIsDirectory = (nIsDirectory != 0);
        if (!record.bIsDirectory && (nLength < 0)) return false;
        record.nUncompressedSize = record.bIsDirectory ? 0 : nLength;
        if (record.nUncompressedSize > IZPACK_MAX_MEMBER_SIZE) return false;

        const qint64 nBlockStart = cursor.pos();
        if (!parser.scanBlockData(record.nUncompressedSize, &nFirstDataOffset,
                                  &nChunks, &nExcess)) {
            return false;
        }

        if ((nChunks <= 1) && (nExcess == 0)) {
            // A member that fits in one frame is a plain contiguous run, so it
            // is published as a stored stream and needs no codec at all.
            record.nStreamOffset = nFirstDataOffset;
            record.nStreamSize = record.nUncompressedSize;
            record.handleMethod = HANDLE_METHOD_STORE;
        } else {
            record.nStreamOffset = nBlockStart;
            record.nStreamSize = cursor.pos() - nBlockStart;
            record.handleMethod = HANDLE_METHOD_IZPACK;
        }
        record.nMTime = nMTime;
        record.nRecordSize = cursor.pos() - record.nRecordOffset;
        record.sFileName = sTargetPath;
        record.sFileName.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (record.sFileName.isEmpty()) {
            record.sFileName = QString("izpack_%1").arg(nIndex);
        }
        context.listRecords.append(record);
    }

    context.nArchiveSize = cursor.pos();
    if (!guardedThis || !guardedSource) return false;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XIzPack::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XIzPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIzPack archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIzPack::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIzPack(pDevice);
}

QList<QString> XIzPack::getSearchSignatures()
{
    // The Java magic alone matches every serialized object ever written; the
    // class name pinned at offset 14 is what makes this an IzPack pack.
    return {QStringLiteral(
        "ACED00057704........7372001B'com.izforge.izpack.PackFile'")};
}

XBinary::FT XIzPack::getFileType()
{
    return FT_IZPACK;
}

XBinary::MODE XIzPack::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIzPack::getEndian()
{
    // Java serialization is big endian throughout.
    return ENDIAN_BIG;
}

QString XIzPack::getArch()
{
    return QString();
}

QString XIzPack::getFileFormatExt()
{
    return QStringLiteral("pack");
}

QString XIzPack::getFileFormatExtsString()
{
    return QStringLiteral("IzPack pack (*.pack)");
}

QString XIzPack::getMIMEString()
{
    return QStringLiteral("application/x-izpack");
}

QString XIzPack::versionToString(qint32 nVersion)
{
    if ((nVersion >= 1) && (nVersion <= 7)) {
        return QString("PackFile v%1").arg(nVersion);
    }
    return QString();
}

QString XIzPack::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return versionToString(context.nVersion);
}

qint64 XIzPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return 0;
    // The tail after the last record still belongs to the same serialization
    // stream (the installer's parsable/executable lists), so the format spans
    // the whole file rather than stopping at the last member.
    return context.nInputSize;
}

QList<XBinary::MAPMODE> XIzPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIzPack::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XIzPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIzPack::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    const qint32 nCount = context.listRecords.size();
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        const RECORD &record = context.listRecords.at(i);
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = record.nStreamOffset;
            part.nFileSize = record.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = record.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, record.handleMethod);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      (record.handleMethod == HANDLE_METHOD_STORE)
                                          ? QString("Stored")
                                          : QString("Stored (block data framed)"));
            part.mapProperties.insert(FPART_PROP_ISFOLDER, record.bIsDirectory);
            if (record.nMTime > 0) {
                part.mapProperties.insert(
                    FPART_PROP_MTIME,
                    QDateTime::fromMSecsSinceEpoch(record.nMTime));
            }
            if (!record.sSourcePath.isEmpty()) {
                // The build-machine path the file came from.  It is not an
                // install directory, so it is published as free text rather
                // than as FPART_PROP_OPTIONAL_PATH.
                part.mapProperties.insert(FPART_PROP_INFO,
                                          QString("Source: %1").arg(record.sSourcePath));
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = record.nRecordOffset;
            part.nFileSize = record.nRecordSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = record.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XIzPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIzPack::initUnpack(UNPACK_STATE *pState,
                         const QMap<UNPACK_PROP, QVariant> &mapProperties,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XIzPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listRecords.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("IzPack pack file; stored members inside a Java serialization stream"));
    pState->mapArchiveProperties.insert(FPART_PROP_VERSION,
                                        versionToString(pContext->nVersion));
    pState->nCurrentOffset = pContext->listRecords.first().nRecordOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XIzPack::infoCurrent(UNPACK_STATE *pState,
                                            PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listRecords.size())) {
        return ARCHIVERECORD();
    }
    const RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != record.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = record.nStreamOffset;
    result.nStreamSize = record.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, record.handleMethod);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                (record.handleMethod == HANDLE_METHOD_STORE)
                                    ? QString("Stored")
                                    : QString("Stored (block data framed)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, record.bIsDirectory);
    if (record.nMTime > 0) {
        result.mapProperties.insert(
            FPART_PROP_MTIME, QDateTime::fromMSecsSinceEpoch(record.nMTime));
    }
    if (!record.sSourcePath.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO,
                                    QString("Source: %1").arg(record.sSourcePath));
    }
    return result;
}

bool XIzPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listRecords.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listRecords.at(pState->nCurrentIndex).nRecordOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XIzPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
