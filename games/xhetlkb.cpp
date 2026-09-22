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
#include "xhetlkb.h"

#include <cstring>

namespace {
const quint8 HE_XOR_KEY = 0x69;
const qint64 HE_CHUNK_HEADER_SIZE = 8;
// Root header plus one complete child chunk header.
const qint64 HE_MIN_FILE_SIZE = 16;
}  // namespace

XHETLKB::XHETLKB(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_HE_TLKB)
{
}

bool XHETLKB::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHETLKB archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHETLKB::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHETLKB(pDevice);
}

quint32 XHETLKB::readBE32(const uchar *pData)
{
    return ((quint32)pData[0] << 24) | ((quint32)pData[1] << 16) |
           ((quint32)pData[2] << 8) | (quint32)pData[3];
}

bool XHETLKB::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                         PDSTRUCT *pPdStruct)
{
    const qint64 nTotalSize = getSize();
    if ((nTotalSize < HE_MIN_FILE_SIZE) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QByteArray baRoot = read_array_process(0, HE_CHUNK_HEADER_SIZE, pPdStruct);
    if ((baRoot.size() != HE_CHUNK_HEADER_SIZE)) return false;
    for (qint32 i = 0; i < baRoot.size(); ++i) {
        baRoot[i] = char(quint8(baRoot.at(i)) ^ HE_XOR_KEY);
    }
    const uchar *pRoot = reinterpret_cast<const uchar *>(baRoot.constData());
    if (memcmp(pRoot, "TLKB", 4) != 0) return false;

    // The root size is the whole container. Requiring it to match the device
    // exactly is what makes a four-byte tag safe as a detector: a chance 'TLKB'
    // in obfuscated data will not also carry its own file size.
    const qint64 nRootSize = (qint64)readBE32(pRoot + 4);
    if (nRootSize != nTotalSize) return false;

    qint64 nOffset = HE_CHUNK_HEADER_SIZE;
    qint32 nIndex = 0;
    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;

    while (nOffset + HE_CHUNK_HEADER_SIZE <= nTotalSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nIndex >= MAX_RECORDS) return false;

        QByteArray baChunk =
            read_array_process(nOffset, HE_CHUNK_HEADER_SIZE, pPdStruct);
        if ((baChunk.size() != HE_CHUNK_HEADER_SIZE))
            return false;
        for (qint32 i = 0; i < baChunk.size(); ++i) {
            baChunk[i] = char(quint8(baChunk.at(i)) ^ HE_XOR_KEY);
        }
        const uchar *pChunk =
            reinterpret_cast<const uchar *>(baChunk.constData());

        // A well-formed chunk tag is four printable characters. The member tag
        // is NOT pinned to 'TALK': the container is a generic SCUMM chunk tree
        // and the root size check above already carries the detection, so a
        // pack holding some other child would be listed rather than rejected.
        QString sTag;
        for (qint32 i = 0; i < 4; ++i) {
            const quint8 nChar = pChunk[i];
            if ((nChar < 0x20) || (nChar > 0x7e)) return false;
            sTag.append(QChar::fromLatin1(char(nChar)));
        }

        const qint64 nChunkSize = (qint64)readBE32(pChunk + 4);
        // Sizes include the header, so anything under it cannot advance the
        // walk; without this a zero size would spin here forever.
        if (nChunkSize < HE_CHUNK_HEADER_SIZE) return false;
        if (nChunkSize > nTotalSize - nOffset) return false;

        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = nOffset;
            entry.nHeaderSize = HE_CHUNK_HEADER_SIZE;
            entry.nDataOffset = nOffset + HE_CHUNK_HEADER_SIZE;
            entry.nDataSize = nChunkSize - HE_CHUNK_HEADER_SIZE;
            // The 0x69 XOR is length-preserving, so the two sizes agree; the
            // method still has to be named, or the record would decode as STORE
            // and publish obfuscated bytes.
            entry.nUncompressedSize = entry.nDataSize;
            entry.handleMethod = HANDLE_METHOD_XOR_69;

            QString sName = QString("%1_%2.%3")
                                .arg(sTag.trimmed().toLower())
                                .arg(nIndex + 1, 4, 10, QChar::fromLatin1('0'))
                                .arg(sTag.trimmed().toLower());
            QString sUniqueName;
            if (!makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                                &mapNextSuffixes, &mapResolvedDirectories,
                                &sUniqueName)) {
                return false;
            }
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }

        nOffset += nChunkSize;
        ++nIndex;
    }

    // The root size is the file size, so a container that does not tile its
    // children exactly to the end is not one of these.
    if (nOffset != nTotalSize) return false;
    if (nIndex == 0) return false;

    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
