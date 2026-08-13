/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "xrar.h"
#include "Algos/xrardecoder.h"
#include "Algos/xaesdecoder.h"
#include <QBuffer>
#include <QPointer>
#include <memory>
#include <new>

namespace {
const qint64 XRAR_MAX_RAR5_HEADER_SIZE = 4 * 1024 * 1024;
const qint32 XRAR_MAX_RECORDS = 1000000;
const quint8 XRAR_MAX_KDF_COUNT = 24;

quint16 xrarReadLe16(const QByteArray &baData, qint32 nOffset)
{
    return (quint16)(quint8)baData.at(nOffset) |
           ((quint16)(quint8)baData.at(nOffset + 1) << 8);
}

quint32 xrarReadLe32(const QByteArray &baData, qint32 nOffset)
{
    return (quint32)(quint8)baData.at(nOffset) |
           ((quint32)(quint8)baData.at(nOffset + 1) << 8) |
           ((quint32)(quint8)baData.at(nOffset + 2) << 16) |
           ((quint32)(quint8)baData.at(nOffset + 3) << 24);
}

bool xrarReadVInt(const QByteArray &baData, qint64 *pOffset,
                  qint64 nEndOffset, qint32 nMaxBytes, quint64 *pValue)
{
    if (!pOffset || !pValue || (*pOffset < 0) ||
        (nEndOffset < *pOffset) || (nEndOffset > baData.size()) ||
        (nMaxBytes <= 0) || (nMaxBytes > 10)) {
        return false;
    }

    quint64 nValue = 0;
    qint64 nCurrentOffset = *pOffset;

    for (qint32 i = 0;
         (i < nMaxBytes) && (nCurrentOffset < nEndOffset); i++) {
        const quint8 nByte = (quint8)baData.at((qint32)nCurrentOffset++);

        if ((i == 9) && (nByte & 0xFE)) {
            *pOffset = nCurrentOffset;
            return false;
        }

        nValue |= (quint64)(nByte & 0x7F) << (i * 7);
        *pOffset = nCurrentOffset;

        if ((nByte & 0x80) == 0) {
            *pValue = nValue;
            return true;
        }
    }

    return false;
}
}

static XBinary::XCONVERT _TABLE_XRAR_STRUCTID[] = {{XRar::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XRar::STRUCTID_RAR14_SIGNATURE, "RAR1.4SIGNATURE", QString("RAR 1.4 signature")},
                                                   {XRar::STRUCTID_RAR40_SIGNATURE, "RAR4.0SIGNATURE", QString("RAR 4.0 signature")},
                                                   {XRar::STRUCTID_RAR50_SIGNATURE, "RAR5.0SIGNATURE", QString("RAR 5.0 signature")},
                                                   {XRar::STRUCTID_RAR14_HEADER, "RAR1.4HEADER", QString("RAR 1.4 header")},
                                                   {XRar::STRUCTID_RAR40_HEADER, "RAR4.0HEADER", QString("RAR 4.0 header")},
                                                   {XRar::STRUCTID_RAR50_HEADER, "RAR5.0HEADER", QString("RAR 5.0 header")}};

static XBinary::PM_INFO createPMInfo(XBinary::HANDLE_METHOD hm0, XBinary::HANDLE_METHOD hm1 = XBinary::HANDLE_METHOD_UNKNOWN,
                                     XBinary::HANDLE_METHOD hm2 = XBinary::HANDLE_METHOD_UNKNOWN, XBinary::HANDLE_METHOD hm3 = XBinary::HANDLE_METHOD_UNKNOWN)
{
    XBinary::PM_INFO result = {};
    result.hm[0] = hm0;
    result.hm[1] = hm1;
    result.hm[2] = hm2;
    result.hm[3] = hm3;

    return result;
}

XRar::XRar(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRar::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> properties;
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const bool bInitialized = guardedArchive->initUnpack(
        &state, properties, pPdStruct);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        !bInitialized) {
        return false;
    }

    const bool bFinished = guardedArchive->finishUnpack(&state, pPdStruct);
    return guardedArchive && guardedSource &&
           (guardedArchive->getDevice() == guardedSource.data()) && bFinished;
}

bool XRar::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRar xrar(pDevice);

    return xrar.isValid(pPdStruct);
}

QString XRar::getVersion()
{
    return getFileFormatInfo(nullptr).sVersion;
}

bool XRar::isEncrypted()
{
    const qint64 nGenericBlock4Size = 7;
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint32 nVersion = guardedArchive->getInternVersion(nullptr);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return false;
    }
    qint64 nCurrentOffset = (nVersion == 1) ? 4 : ((nVersion == 4) ? 7 : 8);
    const qint64 nTotalSize = guardedArchive->getSize();

    if (nVersion == 1) {
        nCurrentOffset = 4;

        while (nCurrentOffset < nTotalSize) {
            const FILEBLOCK14 fileBlock = guardedArchive->readFileBlock14(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return false;
            }
            if (fileBlock.nHeaderSize == 0) {
                break;
            }

            if (fileBlock.nFlags & RAR4_FILE_PASSWORD) {
                return true;
            }

            quint64 nRecordSize = (quint64)fileBlock.nHeaderSize + (quint64)fileBlock.nPackSize;
            if (nRecordSize > (quint64)(nTotalSize - nCurrentOffset)) {
                break;
            }

            nCurrentOffset += (qint64)nRecordSize;
        }
    } else if (nVersion == 4) {
        while ((nCurrentOffset >= 0) && ((nCurrentOffset + nGenericBlock4Size) <= nTotalSize)) {
            const GENERICBLOCK4 genericBlock = guardedArchive->readGenericBlock4(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return false;
            }

            if (((qint64)genericBlock.nHeaderSize < nGenericBlock4Size) || (genericBlock.nType < BLOCKTYPE4_MARKER) ||
                (genericBlock.nType > BLOCKTYPE4_END) || ((qint64)genericBlock.nHeaderSize > (nTotalSize - nCurrentOffset))) {
                break;
            }

            if ((genericBlock.nType == BLOCKTYPE4_ARCHIVE) && (genericBlock.nFlags & RAR4_ARCHIVE_PASSWORD)) {
                return true;
            }

            quint64 nDataSize = 0;

            if ((genericBlock.nType == BLOCKTYPE4_FILE) || (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW)) {
                const FILEBLOCK4 fileBlock = guardedArchive->readFileBlock4(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }

                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_PASSWORD) {
                    return true;
                }

                nDataSize = (quint64)fileBlock.packSize;

                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                    nDataSize |= (quint64)fileBlock.highPackSize << 32;
                }
            } else if ((genericBlock.nFlags & RAR4_LONG_BLOCK) && ((qint64)genericBlock.nHeaderSize >= (nGenericBlock4Size + 4))) {
                nDataSize = guardedArchive->read_uint32(
                    nCurrentOffset + nGenericBlock4Size);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }
            }

            qint64 nDataOffset = nCurrentOffset + genericBlock.nHeaderSize;

            if (nDataSize > (quint64)(nTotalSize - nDataOffset)) {
                break;
            }

            nCurrentOffset = nDataOffset + (qint64)nDataSize;

            if (genericBlock.nType == BLOCKTYPE4_END) {
                break;
            }
        }
    } else if (nVersion == 5) {
        while ((nCurrentOffset >= 0) && ((nCurrentOffset + 5) <= nTotalSize)) {
            const GENERICHEADER5 genericHeader = guardedArchive->readGenericHeader5(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return false;
            }

            if ((genericHeader.nHeaderSize == 0) || (genericHeader.nHeaderSize > (quint64)(nTotalSize - nCurrentOffset))) {
                break;
            }

            if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
                return true;
            }

            if ((genericHeader.nType == HEADERTYPE5_FILE) || (genericHeader.nType == HEADERTYPE5_SERVICE)) {
                const FILEHEADER5 fileHeader = guardedArchive->readFileHeader5(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }

                if (guardedArchive->_readProperties(fileHeader)
                        .value(FPART_PROP_ENCRYPTED).toBool()) {
                    return true;
                }
            }

            qint64 nDataOffset = nCurrentOffset + (qint64)genericHeader.nHeaderSize;

            if (genericHeader.nDataSize > (quint64)(nTotalSize - nDataOffset)) {
                break;
            }

            nCurrentOffset = nDataOffset + (qint64)genericHeader.nDataSize;

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }
        }
    }

    return false;
}

bool XRar::isCommentPresent()
{
    bool bResult = false;
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint32 nVersion = guardedArchive->getInternVersion(nullptr);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return false;
    }

    if (nVersion == 4) {
        // RAR4: scan blocks for BLOCKTYPE4_COMMENT (0x75) or BLOCKTYPE4_SUBBLOCK_NEW (0x7A) with "CMT" name
        qint64 nCurrentOffset = 7;  // After signature
        qint64 nTotalSize = getSize();

        qint32 nBlockCount = 0;
        while ((nCurrentOffset < nTotalSize) && (nBlockCount < XRAR_MAX_RECORDS)) {
            const GENERICBLOCK4 genericBlock = guardedArchive->readGenericBlock4(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return false;
            }

            if (genericBlock.nHeaderSize == 0 || genericBlock.nType < 0x72 || genericBlock.nType > 0x7B) {
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_COMMENT) {
                bResult = true;
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                const FILEBLOCK4 fileBlock = guardedArchive->readFileBlock4(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }
                if (fileBlock.sFileName == "CMT") {
                    bResult = true;
                    break;
                }
            }

            if (genericBlock.nType == BLOCKTYPE4_END) {
                break;
            }

            quint64 nDataSize = 0;
            if (genericBlock.nType == BLOCKTYPE4_FILE || genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                const FILEBLOCK4 fileBlock = guardedArchive->readFileBlock4(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }
                if (fileBlock.genericBlock4.nHeaderSize == 0) break;
                nDataSize = (quint64)fileBlock.packSize;
                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                    nDataSize |= (quint64)fileBlock.highPackSize << 32;
                }
            } else if (genericBlock.nFlags & RAR4_LONG_BLOCK) {
                if (genericBlock.nHeaderSize < 11) break;
                nDataSize = guardedArchive->read_uint32(nCurrentOffset + 7);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }
            }

            quint64 nBlockSize = (quint64)genericBlock.nHeaderSize + nDataSize;
            if (nBlockSize > (quint64)(nTotalSize - nCurrentOffset)) break;
            nCurrentOffset += (qint64)nBlockSize;
            nBlockCount++;
        }
    } else if (nVersion == 5) {
        // RAR5: scan for service header (type 3) with name "CMT"
        qint64 nCurrentOffset = 8;  // After RAR5 signature
        qint64 nTotalSize = getSize();

        qint32 nHeaderCount = 0;
        while ((nCurrentOffset < nTotalSize) && (nHeaderCount < XRAR_MAX_RECORDS)) {
            const GENERICHEADER5 genericHeader = guardedArchive->readGenericHeader5(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return false;
            }

            if ((genericHeader.nHeaderSize == 0) ||
                (genericHeader.nDataSize > (quint64)(nTotalSize - nCurrentOffset - (qint64)genericHeader.nHeaderSize))) {
                break;
            }

            if (genericHeader.nType == HEADERTYPE5_SERVICE) {
                const FILEHEADER5 fileHeader = guardedArchive->readFileHeader5(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return false;
                }
                if (fileHeader.nHeaderSize == 0) break;
                if (fileHeader.sFileName == "CMT") {
                    bResult = true;
                    break;
                }
            }

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }

            nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
            nHeaderCount++;
        }
    }

    return bResult;
}

QString XRar::getComment()
{
    QString sResult;
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return sResult;

    const qint32 nVersion = guardedArchive->getInternVersion(nullptr);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return QString();
    }

    if (nVersion == 4) {
        // RAR4: find COMMENT block (0x75) or SUBBLOCK_NEW (0x7A) with "CMT" name
        qint64 nCurrentOffset = 7;
        qint64 nTotalSize = getSize();

        qint32 nBlockCount = 0;
        while ((nCurrentOffset < nTotalSize) && (nBlockCount < XRAR_MAX_RECORDS)) {
            const GENERICBLOCK4 genericBlock = guardedArchive->readGenericBlock4(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return QString();
            }

            if (genericBlock.nHeaderSize == 0 || genericBlock.nType < 0x72 || genericBlock.nType > 0x7B) {
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_COMMENT) {
                // Old-style RAR4 comment block after generic header (7 bytes):
                //   2 bytes: unpacked comment size
                //   1 byte: version needed to extract
                //   1 byte: packing method
                //   2 bytes: comment CRC16
                // Remaining header data = comment (compressed or stored)
                if (genericBlock.nHeaderSize < 13) {
                    break;
                }

                const QByteArray baHeader = guardedArchive->readBlock4Snapshot(
                    nCurrentOffset);
                GENERICBLOCK4 checkedBlock = {};
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data()) ||
                    !parseGenericBlock4Snapshot(baHeader, &checkedBlock) ||
                    (checkedBlock.nCRC16 != genericBlock.nCRC16) ||
                    (checkedBlock.nType != genericBlock.nType) ||
                    (checkedBlock.nFlags != genericBlock.nFlags) ||
                    (checkedBlock.nHeaderSize != genericBlock.nHeaderSize)) {
                    return QString();
                }

                qint32 nBodyOffset = 7;
                const quint16 nUnpSize = xrarReadLe16(baHeader,
                                                      nBodyOffset);
                nBodyOffset += 2;
                const quint8 nUnpVer = (quint8)baHeader.at(nBodyOffset++);
                const quint8 nMethod = (quint8)baHeader.at(nBodyOffset++);

                Q_UNUSED(nUnpVer)

                if (nMethod == RAR_METHOD_STORE) {
                    nBodyOffset += 2;  // Skip CRC16
                    const qint64 nCommentDataSize =
                        genericBlock.nHeaderSize - nBodyOffset;
                    if (nCommentDataSize > 0 && nCommentDataSize <= nUnpSize) {
                        const QByteArray baComment = baHeader.mid(
                            nBodyOffset, (qint32)nCommentDataSize);
                        if (baComment.size() == nCommentDataSize) {
                            sResult = QString::fromUtf8(baComment);
                        }
                    }
                }
                // Compressed comments (method != STORE) are not supported for reading yet
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                const FILEBLOCK4 fileBlock = guardedArchive->readFileBlock4(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return QString();
                }
                if (fileBlock.genericBlock4.nHeaderSize == 0) break;
                if (fileBlock.sFileName == "CMT") {
                    // RAR3/4 new-style comment sub-block (same structure as FILE block)
                    // Data after header is the comment (compressed or stored)
                    qint64 nDataOffset = nCurrentOffset + genericBlock.nHeaderSize;
                    quint64 nPackSize = (quint64)fileBlock.packSize;
                    if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                        nPackSize |= (quint64)fileBlock.highPackSize << 32;
                    }

                    if ((nPackSize > (quint64)(nTotalSize - nDataOffset)) || (nPackSize > (quint64)XRAR_MAX_RAR5_HEADER_SIZE)) {
                        break;
                    }

                    if (fileBlock.method == RAR_METHOD_STORE && nPackSize > 0) {
                        const QByteArray baComment = guardedArchive->read_array(
                            nDataOffset, (qint64)nPackSize);
                        if (!guardedArchive || !guardedSource ||
                            (guardedArchive->getDevice() != guardedSource.data())) {
                            return QString();
                        }
                        if ((quint64)baComment.size() == nPackSize) {
                            sResult = QString::fromUtf8(baComment);
                        }
                    }
                    // Compressed comments (method != STORE) require RAR decompression engine
                    break;
                }
            }

            if (genericBlock.nType == BLOCKTYPE4_END) {
                break;
            }

            quint64 nDataSize = 0;
            if (genericBlock.nType == BLOCKTYPE4_FILE || genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                const FILEBLOCK4 fileBlock = guardedArchive->readFileBlock4(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return QString();
                }
                if (fileBlock.genericBlock4.nHeaderSize == 0) break;
                nDataSize = (quint64)fileBlock.packSize;
                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                    nDataSize |= (quint64)fileBlock.highPackSize << 32;
                }
            } else if (genericBlock.nFlags & RAR4_LONG_BLOCK) {
                if (genericBlock.nHeaderSize < 11) break;
                nDataSize = guardedArchive->read_uint32(nCurrentOffset + 7);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return QString();
                }
            }

            quint64 nBlockSize = (quint64)genericBlock.nHeaderSize + nDataSize;
            if (nBlockSize > (quint64)(nTotalSize - nCurrentOffset)) break;
            nCurrentOffset += (qint64)nBlockSize;
            nBlockCount++;
        }
    } else if (nVersion == 5) {
        // RAR5: find service header "CMT" and read comment data area
        qint64 nCurrentOffset = 8;
        qint64 nTotalSize = getSize();

        qint32 nHeaderCount = 0;
        while ((nCurrentOffset < nTotalSize) && (nHeaderCount < XRAR_MAX_RECORDS)) {
            const GENERICHEADER5 genericHeader = guardedArchive->readGenericHeader5(
                nCurrentOffset);
            if (!guardedArchive || !guardedSource ||
                (guardedArchive->getDevice() != guardedSource.data())) {
                return QString();
            }

            if ((genericHeader.nHeaderSize == 0) ||
                (genericHeader.nDataSize > (quint64)(nTotalSize - nCurrentOffset - (qint64)genericHeader.nHeaderSize))) {
                break;
            }

            if (genericHeader.nType == HEADERTYPE5_SERVICE) {
                const FILEHEADER5 fileHeader = guardedArchive->readFileHeader5(
                    nCurrentOffset);
                if (!guardedArchive || !guardedSource ||
                    (guardedArchive->getDevice() != guardedSource.data())) {
                    return QString();
                }
                if (fileHeader.nHeaderSize == 0) break;
                if (fileHeader.sFileName == "CMT" && fileHeader.nDataSize > 0) {
                    // RAR5 comment data area follows the header
                    qint64 nDataOffset = nCurrentOffset + fileHeader.nHeaderSize;
                    quint64 nMethod = fileHeader.nCompInfo & 0x003f;

                    if ((nMethod == 0) && (fileHeader.nDataSize <= (quint64)XRAR_MAX_RAR5_HEADER_SIZE)) {
                        // Store method (version 0) — read directly
                        const QByteArray baComment = guardedArchive->read_array(
                            nDataOffset, (qint64)fileHeader.nDataSize);
                        if (!guardedArchive || !guardedSource ||
                            (guardedArchive->getDevice() != guardedSource.data())) {
                            return QString();
                        }
                        if ((quint64)baComment.size() == fileHeader.nDataSize) {
                            sResult = QString::fromUtf8(baComment);
                        }
                    }
                    // Compressed CMT data is not supported for reading yet
                    break;
                }
            }

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }

            nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
            nHeaderCount++;
        }
    }

    return sResult;
}

QString XRar::getFileFormatExt()
{
    return "rar";
}

QString XRar::getFileFormatExtsString()
{
    return "RAR (*.rar)";
}

qint64 XRar::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return getFileFormatInfo(pPdStruct).nSize;
}

QString XRar::blockType4ToString(BLOCKTYPE4 type)
{
    QString sResult;

    switch (type) {
        case BLOCKTYPE4_MARKER: sResult = QString("Marker block"); break;
        case BLOCKTYPE4_ARCHIVE: sResult = QString("Archive header"); break;
        case BLOCKTYPE4_FILE: sResult = QString("File header"); break;
        case BLOCKTYPE4_COMMENT: sResult = QString("Comment header"); break;
        case BLOCKTYPE4_EXTRA: sResult = QString("Extra information"); break;
        case BLOCKTYPE4_SUBBLOCK: sResult = QString("Subblock"); break;
        case BLOCKTYPE4_RECOVERY: sResult = QString("Recovery record"); break;
        case BLOCKTYPE4_AUTH: sResult = QString("Archive authentication"); break;
        case BLOCKTYPE4_SUBBLOCK_NEW: sResult = QString("Subblock"); break;
        case BLOCKTYPE4_END: sResult = QString("End of archive"); break;
        default: sResult = QString("Unknown (%1)").arg(type, 0, 16);
    }

    return sResult;
}

QString XRar::headerType5ToString(HEADERTYPE5 type)
{
    QString sResult;

    switch (type) {
        case HEADERTYPE5_MAIN: sResult = QString("Main archive header"); break;
        case HEADERTYPE5_FILE: sResult = QString("File header"); break;
        case HEADERTYPE5_SERVICE: sResult = QString("Service header"); break;
        case HEADERTYPE5_ENCRYPTION: sResult = QString("Archive encryption header"); break;
        case HEADERTYPE5_ENDARC: sResult = QString("End of archive header"); break;
        default: sResult = QString("Unknown (%1)").arg(type, 0, 16); break;
    }

    return sResult;
}

QString XRar::getMIMEString()
{
    return "application/x-rar-compressed";
}

QString XRar::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XRAR_STRUCTID, sizeof(_TABLE_XRAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XRar::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XRAR_STRUCTID, sizeof(_TABLE_XRAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XRar::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XRAR_STRUCTID, sizeof(_TABLE_XRAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XRar::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        qint32 nVersion = getInternVersion(pPdStruct);
        quint32 nSigID = STRUCTID_UNKNOWN;
        qint64 nSigSize = 0;

        if (nVersion == 1) {
            nSigID = STRUCTID_RAR14_SIGNATURE;
            nSigSize = 4;
        } else if (nVersion == 4) {
            nSigID = STRUCTID_RAR40_SIGNATURE;
            nSigSize = 7;
        } else if (nVersion == 5) {
            nSigID = STRUCTID_RAR50_SIGNATURE;
            nSigSize = 8;
        }

        if (nSigID != STRUCTID_UNKNOWN) {
            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.nStructID = nSigID;
            _xfStruct.xLoc = offsetToLoc(0);
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if ((nStructID == STRUCTID_RAR40_SIGNATURE) || (nStructID == STRUCTID_RAR14_SIGNATURE) || (nStructID == STRUCTID_RAR50_SIGNATURE)) {
        qint64 nOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);
        if (nOffset == -1) nOffset = 0;

        qint64 nSigSize = (nStructID == STRUCTID_RAR50_SIGNATURE) ? 8 : (nStructID == STRUCTID_RAR14_SIGNATURE) ? 4 : 7;

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(nStructID);
        xfHeader.xLoc = xfStruct.xLoc;
        xfHeader.nSize = nSigSize;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, nStructID, xfStruct.xLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(nStructID), xfHeader.sParentTag);
        listResult.append(xfHeader);

        if (xfStruct.bIsParent && (nStructID == STRUCTID_RAR40_SIGNATURE)) {
            // Enumerate RAR4 block offsets for the table
            qint64 nCurrentOffset = nSigSize;
            qint64 nTotalSize = getSize();
            QList<XADDR> listBlockOffsets;

            while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                if (nCurrentOffset >= nTotalSize - (qint64)sizeof(GENERICBLOCK4)) break;

                GENERICBLOCK4 block = readGenericBlock4(nCurrentOffset);
                if (block.nHeaderSize == 0 || block.nType < 0x72 || block.nType > 0x7B) break;

                listBlockOffsets.append(nCurrentOffset);

                if (block.nType == BLOCKTYPE4_FILE) {
                    FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);
                    qint64 nPackSize = fileBlock4.packSize;
                    if (fileBlock4.genericBlock4.nFlags & RAR4_FILE_LARGE) nPackSize |= ((qint64)fileBlock4.highPackSize << 32);
                    nCurrentOffset += block.nHeaderSize + nPackSize;
                } else {
                    nCurrentOffset += block.nHeaderSize;
                }

                if (block.nType == BLOCKTYPE4_END) break;
            }

            if (!listBlockOffsets.isEmpty()) {
                XFHEADER xfTable = {};
                xfTable.sParentTag = xfHeader.sTag;
                xfTable.fileType = xfStruct.fileType;
                xfTable.structID = static_cast<XBinary::STRUCTID>(STRUCTID_RAR40_HEADER);
                xfTable.xLoc = offsetToLoc(listBlockOffsets.first());
                xfTable.xfType = XFTYPE_TABLE;
                xfTable.listFields = getXFRecords(xfStruct.fileType, STRUCTID_RAR40_HEADER, xfTable.xLoc);
                xfTable.listRowLocations = listBlockOffsets;
                xfTable.sTag = xfHeaderToTag(xfTable, structIDToString(STRUCTID_RAR40_HEADER), xfTable.sParentTag);
                listResult.append(xfTable);
            }
        } else if (xfStruct.bIsParent && (nStructID == STRUCTID_RAR50_SIGNATURE)) {
            // Enumerate RAR5 block offsets
            qint64 nCurrentOffset = nSigSize;
            qint64 nTotalSize = getSize();
            QList<XADDR> listHeaderOffsets;

            while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                if (nCurrentOffset >= nTotalSize) break;
                GENERICHEADER5 hdr = readGenericHeader5(nCurrentOffset);
                if (hdr.nHeaderSize == 0 || hdr.nType < 1 || hdr.nType > 5) break;
                listHeaderOffsets.append(nCurrentOffset);
                nCurrentOffset += hdr.nHeaderSize + hdr.nDataSize;
                if (hdr.nType == HEADERTYPE5_ENDARC || hdr.nType == HEADERTYPE5_ENCRYPTION) break;
            }

            if (!listHeaderOffsets.isEmpty()) {
                XFHEADER xfTable = {};
                xfTable.sParentTag = xfHeader.sTag;
                xfTable.fileType = xfStruct.fileType;
                xfTable.structID = static_cast<XBinary::STRUCTID>(STRUCTID_RAR50_HEADER);
                xfTable.xLoc = offsetToLoc(listHeaderOffsets.first());
                xfTable.xfType = XFTYPE_TABLE;
                xfTable.listFields = getXFRecords(xfStruct.fileType, STRUCTID_RAR50_HEADER, xfTable.xLoc);
                xfTable.listRowLocations = listHeaderOffsets;
                xfTable.sTag = xfHeaderToTag(xfTable, structIDToString(STRUCTID_RAR50_HEADER), xfTable.sParentTag);
                listResult.append(xfTable);
            }
        }
    } else if ((nStructID == STRUCTID_RAR40_HEADER) || (nStructID == STRUCTID_RAR50_HEADER)) {
        // Direct lookup: xLoc = first block offset, nCount = number of blocks
        qint64 nStartOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);
        qint32 nCount = xfStruct.nCount;

        if ((nStartOffset != -1) && (nCount > 0)) {
            QList<XADDR> listOffsets;
            qint64 nCurrentOffset = nStartOffset;
            qint64 nTotalSize = getSize();

            if (nStructID == STRUCTID_RAR40_HEADER) {
                for (qint32 i = 0; i < nCount; i++) {
                    if (nCurrentOffset >= nTotalSize - (qint64)sizeof(GENERICBLOCK4)) break;
                    GENERICBLOCK4 block = readGenericBlock4(nCurrentOffset);
                    if (block.nHeaderSize == 0 || block.nType < 0x72 || block.nType > 0x7B) break;
                    listOffsets.append(nCurrentOffset);
                    if (block.nType == BLOCKTYPE4_FILE) {
                        FILEBLOCK4 fb4 = readFileBlock4(nCurrentOffset);
                        qint64 nPackSize = fb4.packSize;
                        if (fb4.genericBlock4.nFlags & RAR4_FILE_LARGE) nPackSize |= ((qint64)fb4.highPackSize << 32);
                        nCurrentOffset += block.nHeaderSize + nPackSize;
                    } else {
                        nCurrentOffset += block.nHeaderSize;
                    }
                    if (block.nType == BLOCKTYPE4_END) break;
                }
            } else {
                for (qint32 i = 0; i < nCount; i++) {
                    if (nCurrentOffset >= nTotalSize) break;
                    GENERICHEADER5 hdr = readGenericHeader5(nCurrentOffset);
                    if (hdr.nHeaderSize == 0 || hdr.nType < 1 || hdr.nType > 5) break;
                    listOffsets.append(nCurrentOffset);
                    nCurrentOffset += hdr.nHeaderSize + hdr.nDataSize;
                    if (hdr.nType == HEADERTYPE5_ENDARC || hdr.nType == HEADERTYPE5_ENCRYPTION) break;
                }
            }

            if (!listOffsets.isEmpty()) {
                XFHEADER xfTable = {};
                xfTable.sParentTag = xfStruct.sParent;
                xfTable.fileType = xfStruct.fileType;
                xfTable.structID = static_cast<XBinary::STRUCTID>(nStructID);
                xfTable.xLoc = xfStruct.xLoc;
                xfTable.xfType = XFTYPE_TABLE;
                xfTable.listFields = getXFRecords(xfStruct.fileType, nStructID, xfStruct.xLoc);
                xfTable.listRowLocations = listOffsets;
                xfTable.sTag = xfHeaderToTag(xfTable, structIDToString(nStructID), xfTable.sParentTag);
                listResult.append(xfTable);
            }
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XRar::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_RAR40_SIGNATURE) || (nStructID == STRUCTID_RAR14_SIGNATURE) || (nStructID == STRUCTID_RAR50_SIGNATURE)) {
        qint32 nSigSize = (nStructID == STRUCTID_RAR50_SIGNATURE) ? 8 : (nStructID == STRUCTID_RAR14_SIGNATURE) ? 4 : 7;
        listResult.append({"Signature", 0, nSigSize, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
    } else if (nStructID == STRUCTID_RAR40_HEADER) {
        listResult.append({"CRC16", 0, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Type", 2, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"Flags", 3, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"HeaderSize", 5, 2, XFRECORD_FLAG_SIZE, VT_UINT16});
    } else if (nStructID == STRUCTID_RAR50_HEADER) {
        listResult.append({"CRC32", 0, 4, XFRECORD_FLAG_NONE, VT_UINT32});
    }

    return listResult;
}

// qint32 XRar::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
//                           void *pUserData, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(locType)
//     Q_UNUSED(nLocation)
//     Q_UNUSED(dataRecordsOptions)
//     Q_UNUSED(pUserData)

//     qint32 nResult = 0;

//     if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_RAR40_HEADER) {
//         XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);

//         qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, locType, nLocation);

//         quint8 nType = read_uint8(nStartOffset + 2);
//         nResult = read_uint16(nStartOffset + 5);

//         if (nType == BLOCKTYPE4_FILE) {
//             FILEBLOCK4 fileBlock4 = readFileBlock4(nStartOffset);

//             qint64 nFileSize = fileBlock4.packSize;
//             nFileSize |= ((qint64)fileBlock4.highPackSize) << 32;

//             nResult += nFileSize;
//         }
//     } else {
//         nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
//     }

//     return nResult;
// }

static bool rarCanAppend(qint32 nLimit, const QList<XBinary::FPART> *pListResult)
{
    return (nLimit == -1) || (pListResult->size() < nLimit);
}

QList<XBinary::FPART> XRar::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> properties;

    if (!initUnpack(&state, properties, pPdStruct)) {
        return listResult;
    }

    RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)state.pContext;
    if (!pContext) {
        finishUnpack(&state, nullptr);
        return listResult;
    }

    const qint32 nInternVersion = pContext->nVersion;
    const qint64 nSignatureSize = (nInternVersion == 1) ? 4 : ((nInternVersion == 4) ? 7 : 8);
    const qint64 nTotalSize = getSize();
    const qint64 nMaxOffset = qMin(pContext->nArchiveEnd, nTotalSize);

    if ((nFileParts & FILEPART_SIGNATURE) && rarCanAppend(nLimit, &listResult)) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_SIGNATURE;
        record.nFileOffset = 0;
        record.nFileSize = nSignatureSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Signature");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_HEADER) {
        if (nInternVersion == 1) {
            for (qint32 i = 0; (i < pContext->listFileOffsets.count()) && rarCanAppend(nLimit, &listResult) && isPdStructNotCanceled(pPdStruct); i++) {
                XBinary::FPART record = {};
                record.filePart = FILEPART_HEADER;
                record.nFileOffset = pContext->listFileOffsets.at(i);
                record.nFileSize = pContext->listFileBlocks14.at(i).nHeaderSize;
                record.nVirtualAddress = XADDR_MAX;
                record.sName = tr("File header");
                listResult.append(record);
            }
        } else {
            qint64 nCurrentOffset = nSignatureSize;
            qint32 nHeaderCount = 0;

            while ((nCurrentOffset < nMaxOffset) && rarCanAppend(nLimit, &listResult) && isPdStructNotCanceled(pPdStruct) &&
                   (nHeaderCount < XRAR_MAX_RECORDS)) {
                qint64 nBlockSize = 0;
                QString sName;

                if (nInternVersion == 4) {
                    GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);
                    if (genericBlock.nHeaderSize == 0) break;

                    quint64 nDataSize = 0;
                    if ((genericBlock.nType == BLOCKTYPE4_FILE) || (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW)) {
                        FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);
                        if (fileBlock.genericBlock4.nHeaderSize == 0) break;
                        nDataSize = (quint64)fileBlock.packSize;
                        if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                            nDataSize |= (quint64)fileBlock.highPackSize << 32;
                        }
                    } else if (genericBlock.nFlags & RAR4_LONG_BLOCK) {
                        if (genericBlock.nHeaderSize < 11) break;
                        nDataSize = read_uint32(nCurrentOffset + 7);
                    }

                    quint64 nSize = (quint64)genericBlock.nHeaderSize + nDataSize;
                    if (nSize > (quint64)(nMaxOffset - nCurrentOffset)) break;
                    nBlockSize = (qint64)nSize;
                    sName = blockType4ToString((BLOCKTYPE4)genericBlock.nType);

                    XBinary::FPART record = {};
                    record.filePart = FILEPART_HEADER;
                    record.nFileOffset = nCurrentOffset;
                    record.nFileSize = genericBlock.nHeaderSize;
                    record.nVirtualAddress = XADDR_MAX;
                    record.sName = sName;
                    listResult.append(record);

                    if (genericBlock.nType == BLOCKTYPE4_END) break;
                } else {
                    GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);
                    if ((genericHeader.nHeaderSize == 0) ||
                        (genericHeader.nDataSize > (quint64)(nMaxOffset - nCurrentOffset - (qint64)genericHeader.nHeaderSize))) {
                        break;
                    }

                    nBlockSize = (qint64)genericHeader.nHeaderSize + (qint64)genericHeader.nDataSize;
                    sName = headerType5ToString((HEADERTYPE5)genericHeader.nType);

                    XBinary::FPART record = {};
                    record.filePart = FILEPART_HEADER;
                    record.nFileOffset = nCurrentOffset;
                    record.nFileSize = genericHeader.nHeaderSize;
                    record.nVirtualAddress = XADDR_MAX;
                    record.sName = sName;
                    listResult.append(record);

                    if ((genericHeader.nType == HEADERTYPE5_ENCRYPTION) || (genericHeader.nType == HEADERTYPE5_ENDARC)) break;
                }

                if (nBlockSize <= 0) break;
                nCurrentOffset += nBlockSize;
                nHeaderCount++;
            }
        }
    }

    if (nFileParts & FILEPART_STREAM) {
        for (qint32 i = 0; (i < state.nNumberOfRecords) && rarCanAppend(nLimit, &listResult) && isPdStructNotCanceled(pPdStruct); i++) {
            state.nCurrentIndex = i;
            ARCHIVERECORD archiveRecord = infoCurrent(&state, pPdStruct);

            XBinary::FPART record = {};
            record.filePart = FILEPART_STREAM;
            record.nFileOffset = archiveRecord.nStreamOffset;
            record.nFileSize = archiveRecord.nStreamSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = "Stream";
            record.mapProperties = archiveRecord.mapProperties;
            listResult.append(record);
        }
    }

    if ((nFileParts & FILEPART_DATA) && rarCanAppend(nLimit, &listResult)) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nMaxOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_OVERLAY) && rarCanAppend(nLimit, &listResult) && (nMaxOffset < nTotalSize)) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nMaxOffset;
        record.nFileSize = nTotalSize - nMaxOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Overlay");
        listResult.append(record);
    }

    finishUnpack(&state, pPdStruct);
    return listResult;
}

// QList<XBinary::DATA_HEADER> XRar::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         qint32 nVersion = getInternVersion(pPdStruct);

//         if (nVersion == 1) {
//             _dataHeadersOptions.nID = STRUCTID_RAR14_SIGNATURE;
//             _dataHeadersOptions.nSize = 4;
//         } else if (nVersion == 4) {
//             _dataHeadersOptions.nID = STRUCTID_RAR40_SIGNATURE;
//             _dataHeadersOptions.nSize = 7;
//         } else if (nVersion == 5) {
//             _dataHeadersOptions.nID = STRUCTID_RAR50_SIGNATURE;
//             _dataHeadersOptions.nSize = 8;
//         }

//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_RAR14_SIGNATURE) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.listRecords.append(getDataRecord(0, 4, "Signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             } else if (dataHeadersOptions.nID == STRUCTID_RAR40_SIGNATURE) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.listRecords.append(getDataRecord(0, 7, "Signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren) {
//                     // Count RAR 4.0 blocks for table
//                     qint64 nCurrentOffset = 7;
//                     qint64 nTotalSize = getSize();
//                     qint32 nNumberOfBlocks = 0;

//                     while (XBinary::isPdStructNotCanceled(pPdStruct)) {
//                         if (nCurrentOffset >= nTotalSize - sizeof(GENERICBLOCK4)) {
//                             break;
//                         }

//                         GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

//                         if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {
//                             nNumberOfBlocks++;

//                             if (genericBlock.nType == BLOCKTYPE4_FILE) {
//                                 FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);
//                                 qint64 nPackSize = fileBlock4.packSize;
//                                 if (fileBlock4.genericBlock4.nFlags & RAR4_FILE_LARGE) {
//                                     nPackSize |= ((qint64)fileBlock4.highPackSize << 32);
//                                 }
//                                 nCurrentOffset += fileBlock4.genericBlock4.nHeaderSize + nPackSize;
//                             } else {
//                                 nCurrentOffset += genericBlock.nHeaderSize;
//                             }

//                             if (genericBlock.nType == BLOCKTYPE4_END) {
//                                 break;
//                             }
//                         } else {
//                             break;
//                         }
//                     }

//                     // Create table of RAR 4.0 blocks
//                     DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                     _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                     _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                     _dataHeadersOptions.nID = STRUCTID_RAR40_HEADER;
//                     _dataHeadersOptions.nLocation += 7;
//                     _dataHeadersOptions.nCount = nNumberOfBlocks;
//                     _dataHeadersOptions.nSize = nCurrentOffset - 7;

//                     listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                 }
//             } else if (dataHeadersOptions.nID == STRUCTID_RAR50_SIGNATURE) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.listRecords.append(getDataRecord(0, 8, "Signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren) {
//                     // Count RAR 5.0 headers for table
//                     qint64 nCurrentOffset = 8;
//                     qint32 nNumberOfHeaders = 0;
//                     qint64 nFileSize = getSize();

//                     while (XBinary::isPdStructNotCanceled(pPdStruct)) {
//                         if (nCurrentOffset >= nFileSize) break;

//                         GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

//                         if (genericHeader.nHeaderSize == 0) break;

//                         // Stop at encryption header
//                         if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
//                             nNumberOfHeaders++;
//                             nCurrentOffset += genericHeader.nHeaderSize;
//                             break;
//                         }

//                         if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
//                             nNumberOfHeaders++;
//                             nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;

//                             if (genericHeader.nType == HEADERTYPE5_ENDARC) {
//                                 break;
//                             }
//                         } else {
//                             break;
//                         }
//                     }

//                     // Create table of RAR 5.0 headers
//                     DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                     _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                     _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                     _dataHeadersOptions.nID = STRUCTID_RAR50_HEADER;
//                     _dataHeadersOptions.nLocation += 8;
//                     _dataHeadersOptions.nCount = nNumberOfHeaders;
//                     _dataHeadersOptions.nSize = nCurrentOffset - 8;

//                     listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                 }
//             } else if (dataHeadersOptions.nID == STRUCTID_RAR40_HEADER) {
//                 GENERICBLOCK4 genericBlock = readGenericBlock4(nStartOffset);

//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = genericBlock.nHeaderSize;

//                 dataHeader.listRecords.append(getDataRecord(0, 2, "CRC16", XBinary::VT_UINT16, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
//                 dataHeader.listRecords.append(getDataRecord(2, 1, "Type", XBinary::VT_UINT8, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
//                 dataHeader.listRecords.append(getDataRecord(3, 2, "Flags", XBinary::VT_UINT16, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
//                 dataHeader.listRecords.append(getDataRecord(5, 2, "Header Size", XBinary::VT_UINT16, DRF_SIZE, XBinary::ENDIAN_LITTLE));

//                 listResult.append(dataHeader);
//             } else if (dataHeadersOptions.nID == STRUCTID_RAR50_HEADER) {
//                 GENERICHEADER5 genericHeader = readGenericHeader5(nStartOffset);

//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = genericHeader.nHeaderSize;

//                 qint64 nOffset = 0;
//                 dataHeader.listRecords.append(getDataRecord(nOffset, 4, "CRC32", XBinary::VT_UINT32, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
//                 nOffset += 4;

//                 // Variable-length fields (ULEB128)
//                 PACKED_UINT packeInt = read_uleb128(nStartOffset + nOffset, 4);
//                 dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Header Size", XBinary::VT_ULEB128, DRF_SIZE, XBinary::ENDIAN_LITTLE));
//                 nOffset += packeInt.nByteSize;

//                 packeInt = read_uleb128(nStartOffset + nOffset, 4);
//                 dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Type", XBinary::VT_ULEB128, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
//                 nOffset += packeInt.nByteSize;

//                 packeInt = read_uleb128(nStartOffset + nOffset, 4);
//                 dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Flags", XBinary::VT_ULEB128, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
//                 nOffset += packeInt.nByteSize;

//                 if (genericHeader.nFlags & 0x0001) {
//                     packeInt = read_uleb128(nStartOffset + nOffset, 4);
//                     dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Extra Area Size", XBinary::VT_ULEB128, DRF_SIZE,
//                     XBinary::ENDIAN_LITTLE)); nOffset += packeInt.nByteSize;
//                 }

//                 if (genericHeader.nFlags & 0x0002) {
//                     packeInt = read_uleb128(nStartOffset + nOffset, 8);
//                     dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Data Size", XBinary::VT_ULEB128, DRF_SIZE, XBinary::ENDIAN_LITTLE));
//                     nOffset += packeInt.nByteSize;
//                 }

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

XBinary::FILEFORMATINFO XRar::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    FILEFORMATINFO result = {};
    result.nSize = getSize();

    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> properties;

    if (initUnpack(&state, properties, pPdStruct)) {
        RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)state.pContext;

        if (pContext) {
            result.nSize = pContext->nArchiveEnd;

            if (pContext->nVersion == 1) {
                result.sVersion = "1.4";
            } else if (pContext->nVersion == 4) {
                result.sVersion = "1.5-4.X";

                if (!pContext->listFileBlocks4.isEmpty()) {
                    quint8 nUnpackVersion = pContext->listFileBlocks4.first().unpVer;
                    if (nUnpackVersion == 15) {
                        result.sVersion = "1.5";
                    } else if ((nUnpackVersion == 20) || (nUnpackVersion == 26)) {
                        result.sVersion = "2.X";
                    } else if (nUnpackVersion == 29) {
                        result.sVersion = "3.X-4.X";
                    }
                }
            } else if (pContext->nVersion == 5) {
                result.sVersion = "5.X-7.X";

                if (!pContext->listFileHeaders5.isEmpty()) {
                    quint8 nUnpackVersion = pContext->listFileHeaders5.first().nCompInfo & 0x003f;
                    if (nUnpackVersion == 0) {
                        result.sVersion = "5.X";
                    } else if (nUnpackVersion == 1) {
                        result.sVersion = "7.X";
                    }
                }
            }

            result.bIsValid = (result.nSize > 0) && (result.nSize <= getSize());
        }

        finishUnpack(&state, pPdStruct);
    }

    if (result.bIsValid) {
        result.fileType = getFileType();
        result.sExt = getFileFormatExt();
        result.sInfo = getInfo();
        result.sMIME = getMIMEString();
    }

    return result;
}

qint32 XRar::getInternVersion(PDSTRUCT *pPdStruct)
{
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return 0;

    _MEMORY_MAP memoryMap = XBinary::getSimpleMemoryMap();

    // Each signature probe can enter a caller-provided QIODevice. Check both
    // the parser and the exact bound source before attempting the next probe.
    const bool bRar14 = guardedArchive->compareSignature(
        &memoryMap, "'RE~^'", 0, pPdStruct);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return 0;
    }
    if (bRar14) return 1;

    const bool bRar4 = guardedArchive->compareSignature(
        &memoryMap, "'Rar!'1A0700", 0, pPdStruct);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return 0;
    }
    if (bRar4) return 4;

    const bool bRar5 = guardedArchive->compareSignature(
        &memoryMap, "'Rar!'1A070100", 0, pPdStruct);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return 0;
    }

    return bRar5 ? 5 : 0;
}

bool XRar::readVIntBounded(qint64 *pOffset, qint64 nEndOffset, qint32 nMaxBytes, quint64 *pValue)
{
    if ((!pOffset) || (!pValue) || (*pOffset < 0) || (nEndOffset < *pOffset) || (nMaxBytes <= 0) || (nMaxBytes > 10)) {
        return false;
    }

    const qint64 nStartOffset = *pOffset;
    const qint64 nReadSize = qMin<qint64>(nMaxBytes, nEndOffset - nStartOffset);
    if (nReadSize <= 0) return false;

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baVInt = guardedArchive->read_array(nStartOffset, nReadSize);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baVInt.size() != nReadSize)) {
        return false;
    }

    qint64 nLocalOffset = 0;
    const bool bResult = xrarReadVInt(baVInt, &nLocalOffset,
                                     baVInt.size(), nMaxBytes, pValue);
    *pOffset = nStartOffset + nLocalOffset;
    return bResult;
}

bool XRar::isRangeValid(qint64 nOffset, quint64 nSize)
{
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nTotalSize = getSize();
    return guardedArchive && guardedSource &&
           (guardedArchive->getDevice() == guardedSource.data()) &&
           (nOffset >= 0) && (nOffset <= nTotalSize) &&
           (nSize <= (quint64)(nTotalSize - nOffset));
}

bool XRar::isHeaderCRCValid4(qint64 nOffset, qint64 nHeaderSize, quint16 nExpectedCRC)
{
    if ((nHeaderSize < 7) || (nHeaderSize > 0xFFFF)) return false;

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baHeader = guardedArchive->read_array(
        nOffset + 2, nHeaderSize - 2);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baHeader.size() != (nHeaderSize - 2))) {
        return false;
    }

    const quint32 nCRC = XBinary::_getCRC32(
        baHeader, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320()) ^
        0xFFFFFFFF;
    return (quint16)nCRC == nExpectedCRC;
}

bool XRar::isHeaderCRCValid5(qint64 nOffset, qint64 nHeaderSize, quint32 nExpectedCRC)
{
    if ((nHeaderSize < 7) || (nHeaderSize > XRAR_MAX_RAR5_HEADER_SIZE))
        return false;

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baHeader = guardedArchive->read_array(
        nOffset + 4, nHeaderSize - 4);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baHeader.size() != (nHeaderSize - 4))) {
        return false;
    }

    const quint32 nCRC = XBinary::_getCRC32(
        baHeader, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320()) ^
        0xFFFFFFFF;
    return nCRC == nExpectedCRC;
}

QByteArray XRar::readBlock4Snapshot(qint64 nOffset)
{
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || (nOffset < 0)) return QByteArray();

    const QByteArray baFixed = guardedArchive->read_array(nOffset, 7);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baFixed.size() != 7)) {
        return QByteArray();
    }

    const quint16 nHeaderSize = xrarReadLe16(baFixed, 5);
    if (nHeaderSize < 7) return QByteArray();
    if (nHeaderSize == 7) return baFixed;

    const QByteArray baHeader = guardedArchive->read_array(
        nOffset, nHeaderSize);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baHeader.size() != nHeaderSize)) {
        return QByteArray();
    }

    return baHeader;
}

QByteArray XRar::readHeader5Snapshot(qint64 nOffset)
{
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || (nOffset < 0)) return QByteArray();

    const qint64 nTotalSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (nOffset > nTotalSize) || ((nTotalSize - nOffset) < 7)) {
        return QByteArray();
    }

    const qint64 nPrefixSize = qMin<qint64>(14, nTotalSize - nOffset);
    const QByteArray baPrefix = guardedArchive->read_array(
        nOffset, nPrefixSize);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baPrefix.size() != nPrefixSize)) {
        return QByteArray();
    }

    qint64 nCurrentOffset = 4;
    quint64 nHeaderDataSize = 0;
    if (!xrarReadVInt(baPrefix, &nCurrentOffset, baPrefix.size(), 10,
                      &nHeaderDataSize) ||
        (nHeaderDataSize < 2) ||
        (nHeaderDataSize > (quint64)XRAR_MAX_RAR5_HEADER_SIZE)) {
        return QByteArray();
    }

    const quint64 nTotalHeaderSize =
        4 + (quint64)(nCurrentOffset - 4) + nHeaderDataSize;
    if ((nTotalHeaderSize > (quint64)XRAR_MAX_RAR5_HEADER_SIZE) ||
        (nTotalHeaderSize > (quint64)(nTotalSize - nOffset))) {
        return QByteArray();
    }

    if (nTotalHeaderSize <= (quint64)baPrefix.size()) {
        return baPrefix.left((qint32)nTotalHeaderSize);
    }

    const QByteArray baHeader = guardedArchive->read_array(
        nOffset, (qint64)nTotalHeaderSize);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        ((quint64)baHeader.size() != nTotalHeaderSize)) {
        return QByteArray();
    }

    return baHeader;
}

bool XRar::parseGenericBlock4Snapshot(const QByteArray &baHeader,
                                      GENERICBLOCK4 *pResult)
{
    if (!pResult || (baHeader.size() < 7) ||
        (baHeader.size() > 0xFFFF)) {
        return false;
    }

    GENERICBLOCK4 parsed = {};
    parsed.nCRC16 = xrarReadLe16(baHeader, 0);
    parsed.nType = (quint8)baHeader.at(2);
    parsed.nFlags = xrarReadLe16(baHeader, 3);
    parsed.nHeaderSize = xrarReadLe16(baHeader, 5);
    if (parsed.nHeaderSize != baHeader.size()) return false;

    const QByteArray baCrcData = baHeader.mid(2);
    const quint32 nCRC = XBinary::_getCRC32(
        baCrcData, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320()) ^
        0xFFFFFFFF;
    if ((quint16)nCRC != parsed.nCRC16) return false;

    *pResult = parsed;
    return true;
}

bool XRar::parseGenericHeader5Snapshot(const QByteArray &baHeader,
                                       GENERICHEADER5 *pResult,
                                       qint64 *pBodyOffset)
{
    if (!pResult || (baHeader.size() < 7) ||
        (baHeader.size() > XRAR_MAX_RAR5_HEADER_SIZE)) {
        return false;
    }

    GENERICHEADER5 parsed = {};
    parsed.nCRC32 = xrarReadLe32(baHeader, 0);

    qint64 nCurrentOffset = 4;
    quint64 nHeaderDataSize = 0;
    if (!xrarReadVInt(baHeader, &nCurrentOffset, baHeader.size(), 10,
                      &nHeaderDataSize) ||
        (nHeaderDataSize < 2) ||
        (nHeaderDataSize > (quint64)XRAR_MAX_RAR5_HEADER_SIZE)) {
        return false;
    }

    const quint64 nTotalHeaderSize =
        4 + (quint64)(nCurrentOffset - 4) + nHeaderDataSize;
    if (nTotalHeaderSize != (quint64)baHeader.size()) return false;

    parsed._nHeaderSize = nHeaderDataSize;
    parsed.nHeaderSize = nTotalHeaderSize;
    if (!xrarReadVInt(baHeader, &nCurrentOffset, baHeader.size(), 10,
                      &parsed.nType) ||
        !xrarReadVInt(baHeader, &nCurrentOffset, baHeader.size(), 10,
                      &parsed.nFlags)) {
        return false;
    }

    if (parsed.nFlags & 0x0001) {
        if (!xrarReadVInt(baHeader, &nCurrentOffset, baHeader.size(), 10,
                          &parsed.nExtraAreaSize)) {
            return false;
        }
    }
    if (parsed.nFlags & 0x0002) {
        if (!xrarReadVInt(baHeader, &nCurrentOffset, baHeader.size(), 10,
                          &parsed.nDataSize)) {
            return false;
        }
    }

    if (parsed.nExtraAreaSize >
        (quint64)(baHeader.size() - nCurrentOffset)) {
        return false;
    }

    const QByteArray baCrcData = baHeader.mid(4);
    const quint32 nCRC = XBinary::_getCRC32(
        baCrcData, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320()) ^
        0xFFFFFFFF;
    if (nCRC != parsed.nCRC32) return false;

    *pResult = parsed;
    if (pBodyOffset) *pBodyOffset = nCurrentOffset;
    return true;
}

bool XRar::isMainOrEndHeader5Valid(qint64 nOffset, const GENERICHEADER5 &genericHeader)
{
    if ((genericHeader.nType != HEADERTYPE5_MAIN) && (genericHeader.nType != HEADERTYPE5_ENDARC)) {
        return false;
    }

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baHeader = guardedArchive->readHeader5Snapshot(nOffset);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return false;
    }

    GENERICHEADER5 checkedHeader = {};
    qint64 nCurrentOffset = 0;
    if (!parseGenericHeader5Snapshot(baHeader, &checkedHeader,
                                     &nCurrentOffset) ||
        (checkedHeader.nCRC32 != genericHeader.nCRC32) ||
        (checkedHeader._nHeaderSize != genericHeader._nHeaderSize) ||
        (checkedHeader.nHeaderSize != genericHeader.nHeaderSize) ||
        (checkedHeader.nType != genericHeader.nType) ||
        (checkedHeader.nFlags != genericHeader.nFlags) ||
        (checkedHeader.nExtraAreaSize != genericHeader.nExtraAreaSize) ||
        (checkedHeader.nDataSize != genericHeader.nDataSize)) {
        return false;
    }

    const qint64 nBodyEnd =
        baHeader.size() - (qint64)checkedHeader.nExtraAreaSize;

    quint64 nArchiveFlags = 0;
    if (!xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &nArchiveFlags)) {
        return false;
    }

    // MAIN_HEAD stores the volume number only when MHFL_VOLNUMBER is set.
    if ((checkedHeader.nType == HEADERTYPE5_MAIN) &&
        (nArchiveFlags & 0x0002)) {
        quint64 nValue = 0;
        if (!xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                          &nValue)) {
            return false;
        }
    }

    return nCurrentOffset == nBodyEnd;
}

XRar::FILEHEADER5 XRar::readFileHeader5(qint64 nOffset)
{
    FILEHEADER5 result = {};
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return result;

    const QByteArray baHeader = guardedArchive->readHeader5Snapshot(nOffset);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return result;
    }

    GENERICHEADER5 genericHeader = {};
    qint64 nCurrentOffset = 0;
    if (!parseGenericHeader5Snapshot(baHeader, &genericHeader,
                                     &nCurrentOffset)) {
        return result;
    }

    if ((genericHeader.nHeaderSize == 0) ||
        ((genericHeader.nType != HEADERTYPE5_FILE) && (genericHeader.nType != HEADERTYPE5_SERVICE))) {
        return result;
    }

    const qint64 nBodyEnd =
        baHeader.size() - (qint64)genericHeader.nExtraAreaSize;

    FILEHEADER5 parsed = {};
    parsed.nCRC32 = genericHeader.nCRC32;
    parsed._nHeaderSize = genericHeader._nHeaderSize;
    parsed.nHeaderSize = genericHeader.nHeaderSize;
    parsed.nType = genericHeader.nType;
    parsed.nFlags = genericHeader.nFlags;
    parsed.nExtraAreaSize = genericHeader.nExtraAreaSize;
    parsed.nDataSize = genericHeader.nDataSize;

    if (!xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &parsed.nFileFlags) ||
        !xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &parsed.nUnpackedSize) ||
        !xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &parsed.nAttributes)) {
        return result;
    }

    if (parsed.nFileFlags & 0x0002) {
        if ((nBodyEnd - nCurrentOffset) < 4) {
            return result;
        }
        parsed.nMTime = xrarReadLe32(baHeader, (qint32)nCurrentOffset);
        nCurrentOffset += 4;
    }

    if (parsed.nFileFlags & 0x0004) {
        if ((nBodyEnd - nCurrentOffset) < 4) {
            return result;
        }
        parsed.nDataCRC32 = xrarReadLe32(baHeader,
                                        (qint32)nCurrentOffset);
        nCurrentOffset += 4;
    }

    if (!xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &parsed.nCompInfo) ||
        !xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &parsed.nHostOS) ||
        !xrarReadVInt(baHeader, &nCurrentOffset, nBodyEnd, 10,
                      &parsed.nNameLength) ||
        (parsed.nNameLength > (quint64)(nBodyEnd - nCurrentOffset))) {
        return result;
    }

    const QByteArray baName = baHeader.mid(
        (qint32)nCurrentOffset, (qint32)parsed.nNameLength);
    if ((quint64)baName.size() != parsed.nNameLength) {
        return result;
    }
    if (!decodeRar5Name(baName, &parsed.sFileName)) {
        return result;
    }
    nCurrentOffset += (qint64)parsed.nNameLength;

    if (nCurrentOffset != nBodyEnd) {
        return result;
    }

    if (parsed.nExtraAreaSize > 0) {
        parsed.baExtraArea = baHeader.mid(
            (qint32)nBodyEnd, (qint32)parsed.nExtraAreaSize);
        if ((quint64)parsed.baExtraArea.size() != parsed.nExtraAreaSize) {
            return result;
        }
    }

    return parsed;
}

XRar::FILEBLOCK4 XRar::readFileBlock4(qint64 nOffset)
{
    FILEBLOCK4 result = {};
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return result;

    const QByteArray baHeader = guardedArchive->readBlock4Snapshot(nOffset);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data())) {
        return result;
    }

    GENERICBLOCK4 genericBlock = {};
    if (!parseGenericBlock4Snapshot(baHeader, &genericBlock)) return result;

    if ((genericBlock.nHeaderSize < 32) ||
        ((genericBlock.nType != BLOCKTYPE4_FILE) && (genericBlock.nType != BLOCKTYPE4_SUBBLOCK_NEW))) {
        return result;
    }

    qint64 nCurrentOffset = 7;
    const qint64 nHeaderEnd = baHeader.size();

    // Read header fields
    result.genericBlock4 = genericBlock;

    // Continue reading file block specific fields
    result.packSize = xrarReadLe32(baHeader, (qint32)nCurrentOffset);
    nCurrentOffset += 4;
    result.unpSize = xrarReadLe32(baHeader, (qint32)nCurrentOffset);
    nCurrentOffset += 4;
    result.hostOS = (quint8)baHeader.at((qint32)nCurrentOffset);
    nCurrentOffset++;
    result.fileCRC = xrarReadLe32(baHeader, (qint32)nCurrentOffset);
    nCurrentOffset += 4;
    result.fileTime = xrarReadLe32(baHeader, (qint32)nCurrentOffset);
    nCurrentOffset += 4;
    result.unpVer = (quint8)baHeader.at((qint32)nCurrentOffset);
    nCurrentOffset++;
    result.method = (quint8)baHeader.at((qint32)nCurrentOffset);
    nCurrentOffset++;
    result.nameSize = xrarReadLe16(baHeader, (qint32)nCurrentOffset);
    nCurrentOffset += 2;
    result.fileAttr = xrarReadLe32(baHeader, (qint32)nCurrentOffset);
    nCurrentOffset += 4;

    // Read high bits of pack/unpack size if large file flag is set
    if (result.genericBlock4.nFlags & RAR4_FILE_LARGE) {
        if ((nHeaderEnd - nCurrentOffset) < 8) {
            return FILEBLOCK4();
        }
        result.highPackSize = xrarReadLe32(baHeader,
                                          (qint32)nCurrentOffset);
        nCurrentOffset += 4;
        result.highUnpSize = xrarReadLe32(baHeader,
                                         (qint32)nCurrentOffset);
        nCurrentOffset += 4;
    } else {
        result.highPackSize = 0;
        result.highUnpSize = 0;
    }

    // Read filename
    if ((qint64)result.nameSize > (nHeaderEnd - nCurrentOffset)) {
        return FILEBLOCK4();
    }

    if (result.nameSize > 0) {
        const QByteArray nameData = baHeader.mid(
            (qint32)nCurrentOffset, result.nameSize);
        if (nameData.size() != result.nameSize) {
            return FILEBLOCK4();
        }
        nCurrentOffset += result.nameSize;

        // Handle Unicode filenames
        if (result.genericBlock4.nFlags & RAR4_FILE_UNICODE_FILENAME) {
            if (!decodeRar4UnicodeName(nameData, &result.sFileName)) {
                return FILEBLOCK4();
            }
        } else {
            result.sFileName = QString::fromLatin1(nameData);
        }
    }

    return result;
}

XRar::GENERICBLOCK4 XRar::readGenericBlock4(qint64 nOffset)
{
    GENERICBLOCK4 result = {};
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return result;

    const QByteArray baHeader = guardedArchive->readBlock4Snapshot(nOffset);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        !parseGenericBlock4Snapshot(baHeader, &result)) {
        return GENERICBLOCK4();
    }

    return result;
}

XRar::FILEBLOCK14 XRar::readFileBlock14(qint64 nOffset)
{
    FILEBLOCK14 result = {};
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || (nOffset < 0)) return result;

    const QByteArray baFixed = guardedArchive->read_array(nOffset, 24);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baFixed.size() != 24) || ((quint8)baFixed.at(0) != 0x07) ||
        ((quint8)baFixed.at(1) != 0x00)) {
        return result;
    }

    const quint8 nNameLen = (quint8)baFixed.at(22);
    const qint64 nHeaderSize = 24 + (qint64)nNameLen;
    QByteArray baHeader = baFixed;
    if (nHeaderSize > baFixed.size()) {
        baHeader = guardedArchive->read_array(nOffset, nHeaderSize);
        if (!guardedArchive || !guardedSource ||
            (guardedArchive->getDevice() != guardedSource.data()) ||
            (baHeader.size() != nHeaderSize)) {
            return result;
        }
    }

    qint32 nCurrentOffset = 0;

    /* byte [0]: unknown byte (0x07 in all known samples) */
    nCurrentOffset += 1;
    /* byte [1]: unknown byte (0x00 in all known samples) */
    nCurrentOffset += 1;
    result.nFlags = (quint8)baHeader.at(nCurrentOffset);  // flags (bit 0x08 = solid)
    nCurrentOffset += 1;
    result.nPackSize = xrarReadLe32(baHeader, nCurrentOffset);  // packed size
    nCurrentOffset += 4;
    result.nUnpSize = xrarReadLe32(baHeader, nCurrentOffset);  // unpacked size
    nCurrentOffset += 4;
    result.nFileCRC16 = xrarReadLe16(baHeader, nCurrentOffset);  // RAR 1.4 checksum of unpacked data
    nCurrentOffset += 2;
    result.nFileTime = xrarReadLe32(baHeader, nCurrentOffset);  // DOS date/time
    nCurrentOffset += 4;
    /* bytes [17-18]: additional time / unknown (2 bytes) */
    nCurrentOffset += 2;
    result.nFileAttr = xrarReadLe16(baHeader, nCurrentOffset);  // file attributes
    nCurrentOffset += 2;
    /* byte [21]: unknown byte (0x02 in all known samples) */
    nCurrentOffset += 1;
    result.nNameLen = (quint8)baHeader.at(nCurrentOffset);  // filename length
    nCurrentOffset += 1;
    result.nMethod = (quint8)baHeader.at(nCurrentOffset);  // packing method (0=store, 1-5=compress)
    nCurrentOffset += 1;

    result.nHeaderSize = nHeaderSize;

    if ((result.nNameLen != nNameLen) || (result.nMethod > 5)) {
        result.nHeaderSize = 0;
        return result;
    }

    if (result.nNameLen > 0) {
        const QByteArray nameData = baHeader.mid(nCurrentOffset,
                                                 result.nNameLen);
        if (nameData.size() != result.nNameLen) {
            result.nHeaderSize = 0;
            return result;
        }
        result.sFileName = QString::fromLatin1(nameData);
    }

    return result;
}

bool XRar::decodeRar5Name(const QByteArray &nameData, QString *pResult)
{
    if (!pResult || nameData.isEmpty() || nameData.contains('\0')) return false;

    // RAR 5 names are UTF-8. A round trip is a compact strict validator on
    // Qt 5: malformed, overlong, surrogate, and truncated sequences decode to
    // replacement characters and cannot reproduce the original bytes. A real
    // encoded U+FFFD (EF BF BD) remains valid because it round-trips exactly.
    const QString sResult = QString::fromUtf8(nameData.constData(), nameData.size());
    if (sResult.toUtf8() != nameData) return false;

    *pResult = sResult;
    return true;
}

bool XRar::decodeRar4UnicodeName(const QByteArray &nameData, QString *pResult)
{
    if (!pResult) return false;

    const qint32 nSeparator = nameData.indexOf('\0');
    if ((nSeparator <= 0) || (nSeparator >= (nameData.size() - 1))) return false;

    const QByteArray baAnsiName = nameData.left(nSeparator);
    const QByteArray baEncoded = nameData.mid(nSeparator + 1);
    qint32 nEncodedPosition = 0;
    qint32 nDecodedPosition = 0;
    const quint16 nHighByte = (quint16)(quint8)baEncoded.at(nEncodedPosition++) << 8;
    QString sDecoded;
    sDecoded.reserve(baAnsiName.size());

    while (nEncodedPosition < baEncoded.size()) {
        quint8 nFlags = (quint8)baEncoded.at(nEncodedPosition++);
        bool bDecodedFromThisFlag = false;

        for (qint32 nFlagIndex = 0; nFlagIndex < 4; nFlagIndex++, nFlags <<= 2) {
            if (nEncodedPosition >= baEncoded.size()) break;

            const quint8 nMode = nFlags >> 6;
            if (nMode == 0) {
                sDecoded.append(QChar((quint8)baEncoded.at(nEncodedPosition++)));
                nDecodedPosition++;
            } else if (nMode == 1) {
                sDecoded.append(QChar(nHighByte | (quint8)baEncoded.at(nEncodedPosition++)));
                nDecodedPosition++;
            } else if (nMode == 2) {
                if ((baEncoded.size() - nEncodedPosition) < 2) return false;
                const quint16 nCodeUnit = (quint8)baEncoded.at(nEncodedPosition) |
                                          ((quint16)(quint8)baEncoded.at(nEncodedPosition + 1) << 8);
                nEncodedPosition += 2;
                sDecoded.append(QChar(nCodeUnit));
                nDecodedPosition++;
            } else {
                const quint8 nLengthByte = (quint8)baEncoded.at(nEncodedPosition++);
                const qint32 nRunLength = (nLengthByte & 0x7F) + 2;
                quint8 nCorrection = 0;
                if (nLengthByte & 0x80) {
                    if (nEncodedPosition >= baEncoded.size()) return false;
                    nCorrection = (quint8)baEncoded.at(nEncodedPosition++);
                }
                if ((nDecodedPosition > baAnsiName.size()) ||
                    (nRunLength > (baAnsiName.size() - nDecodedPosition))) {
                    return false;
                }

                for (qint32 i = 0; i < nRunLength; i++, nDecodedPosition++) {
                    quint16 nCodeUnit = (quint8)baAnsiName.at(nDecodedPosition);
                    if (nLengthByte & 0x80) {
                        nCodeUnit = nHighByte | ((nCodeUnit + nCorrection) & 0xFF);
                    }
                    sDecoded.append(QChar(nCodeUnit));
                }
            }

            bDecodedFromThisFlag = true;
        }

        if (!bDecodedFromThisFlag) return false;
    }

    if (sDecoded.isEmpty() || sDecoded.contains(QChar(0))) return false;

    *pResult = sDecoded;
    return true;
}

quint16 XRar::calculateCRC16(const QByteArray &data)
{
    // RAR 1.5-4.x stores the low 16 bits of the standard finalized CRC32.
    quint32 nCRC = XBinary::_getCRC32(data, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
    return (quint16)nCRC;
}

QByteArray XRar::createFileBlock4(const QString &sFileName, qint64 nFileSize, quint32 nFileCRC, quint32 nFileTime, quint32 nAttributes)
{
    QByteArray baResult;
    QByteArray baHeader;
    QDataStream ds(&baHeader, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    // Convert filename to bytes (ASCII/UTF-8)
    QByteArray baFileName = sFileName.toUtf8();
    quint16 nNameSize = baFileName.size();

    // Build header (without CRC16 at the beginning)
    ds << (quint8)BLOCKTYPE4_FILE;  // Type
    ds << (quint16)0x8000;          // Flags (0x8000 = has data)

    // Calculate header size (no high size fields for files < 4GB)
    quint16 nHeaderSize = 7 + 25 + nNameSize;  // 7 (generic) + 25 (file-specific) + name
    ds << nHeaderSize;

    // File-specific fields
    ds << (quint32)nFileSize;        // packSize (low 32 bits)
    ds << (quint32)nFileSize;        // unpSize (low 32 bits)
    ds << (quint8)RAR_OS_WIN32;      // hostOS
    ds << nFileCRC;                  // fileCRC
    ds << nFileTime;                 // fileTime (MS-DOS format)
    ds << (quint8)0x14;              // unpVer (2.0)
    ds << (quint8)RAR_METHOD_STORE;  // method (0x30 = STORE)
    ds << nNameSize;                 // nameSize
    ds << nAttributes;               // fileAttr

    // Note: For files < 4GB, we don't write highPackSize/highUnpSize fields
    // The RAR4_FILE_LARGE flag (0x0100) is not set, so reader won't expect these fields

    // Append filename
    baHeader.append(baFileName);

    // Calculate CRC16 and prepend
    quint16 nCRC16 = calculateCRC16(baHeader);
    QByteArray baCRC;
    QDataStream dsCRC(&baCRC, QIODevice::WriteOnly);
    dsCRC.setByteOrder(QDataStream::LittleEndian);
    dsCRC << nCRC16;

    baResult = baCRC + baHeader;

    return baResult;
}

QList<XBinary::PM_INFO> XRar::unpackImplemented()
{
    QList<PM_INFO> listResult;

    listResult.append(createPMInfo(HANDLE_METHOD_STORE));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_15));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_20));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_29));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_50));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_70));
    listResult.append(createPMInfo(HANDLE_METHOD_STORE, HANDLE_METHOD_RAR5_AES));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_50, HANDLE_METHOD_RAR5_AES));
    listResult.append(createPMInfo(HANDLE_METHOD_RAR_70, HANDLE_METHOD_RAR5_AES));

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XRar::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    if (isEncrypted()) {
        result.insert(XBinary::UNPACK_PROP_PASSWORD, QString());
    }

    return result;
}

bool XRar::_initUnpackFail(QPointer<XRar> *pGuardedArchive, XBinary::UNPACK_STATE *pUnpackState, RAR_UNPACK_CONTEXT *pContext)
{
    if (pUnpackState->pContext == pContext) {
        pUnpackState->pContext = nullptr;
    }
    if (*pGuardedArchive) {
        (*pGuardedArchive)->releaseUnpackSource(pUnpackState);
    }
    delete pContext;
    if (!(*pGuardedArchive)) {
        *pUnpackState = UNPACK_STATE();
        return false;
    }
    pUnpackState->nCurrentOffset = 0;
    pUnpackState->nCurrentIndex = 0;
    pUnpackState->nNumberOfRecords = 0;
    pUnpackState->nTotalSize = 0;
    pUnpackState->mapUnpackProperties.clear();
    pUnpackState->mapArchiveProperties.clear();
    pUnpackState->pContext = nullptr;
    return false;
}

bool XRar::initUnpack(XBinary::UNPACK_STATE *pUnpackState, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XRar> guardedArchive(this);
    if (!pUnpackState || m_bUnpackOperationInProgress) return false;
    if ((pUnpackState->pContext ||
         !pUnpackState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pUnpackState)) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pUnpackState, nullptr) ||
        !guardedArchive) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) {
        return false;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = guardedArchive->bindUnpackSource(
        pUnpackState, pPdStruct);
    if (!guardedArchive || !bBound) return false;

    pUnpackState->mapUnpackProperties = mapProperties;
    pUnpackState->nTotalSize = guardedArchive->getSize();
    if (!guardedArchive) return false;

    RAR_UNPACK_CONTEXT *pContext = new (std::nothrow) RAR_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pUnpackState);
        pUnpackState->mapUnpackProperties.clear();
        pUnpackState->nTotalSize = 0;
        return false;
    }
    pContext->nVersion = guardedArchive->getInternVersion(pPdStruct);
    if (!guardedArchive) {
        delete pContext;
        *pUnpackState = UNPACK_STATE();
        return false;
    }
    pContext->bArchiveIsSolid = false;
    pContext->bHeadersEncrypted = false;
    pContext->nArchiveEnd = 0;

    if ((pContext->nVersion == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
    }

    const qint64 nTotalSize = pUnpackState->nTotalSize;
    qint64 nCurrentOffset = (pContext->nVersion == 1) ? 4 : ((pContext->nVersion == 4) ? 7 : 8);

    if (pContext->nVersion == 1) {
        qint32 nBlockCount = 0;

        while (nCurrentOffset < nTotalSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nBlockCount >= XRAR_MAX_RECORDS)) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            FILEBLOCK14 fileBlock = guardedArchive->readFileBlock14(
                nCurrentOffset);
            if (!guardedArchive) {
                delete pContext;
                *pUnpackState = UNPACK_STATE();
                return false;
            }

            if (fileBlock.nHeaderSize == 0) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            quint64 nRecordSize = (quint64)fileBlock.nHeaderSize + (quint64)fileBlock.nPackSize;
            if ((nCurrentOffset > nTotalSize) || (nRecordSize > (quint64)(nTotalSize - nCurrentOffset))) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            pContext->listFileOffsets.append(nCurrentOffset);
            pContext->listFileBlocks14.append(fileBlock);
            pUnpackState->nNumberOfRecords++;
            nBlockCount++;

            nCurrentOffset += (qint64)nRecordSize;
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nCurrentOffset != nTotalSize) || pContext->listFileBlocks14.isEmpty()) {
            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
        }

        pContext->nArchiveEnd = nCurrentOffset;
        pContext->bArchiveIsSolid = (pContext->listFileBlocks14.at(0).nFlags & 0x08) != 0;

        qint32 nFolderIndex = 0;
        for (qint32 i = 0; i < pContext->listFileBlocks14.count(); i++) {
            if ((i > 0) && !pContext->bArchiveIsSolid) {
                nFolderIndex++;
            }
            pContext->listSolidFolderIndex.append(nFolderIndex);
        }
    } else if (pContext->nVersion == 4) {
        GENERICBLOCK4 archiveBlock = guardedArchive->readGenericBlock4(
            nCurrentOffset);
        if (!guardedArchive) {
            delete pContext;
            *pUnpackState = UNPACK_STATE();
            return false;
        }

        if ((archiveBlock.nType != BLOCKTYPE4_ARCHIVE) || (archiveBlock.nHeaderSize < 13)) {
            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
        }

        pContext->bArchiveIsSolid = (archiveBlock.nFlags & 0x0008) != 0;
        nCurrentOffset += archiveBlock.nHeaderSize;

        // RAR 2.x-4.x header encryption covers all blocks following the
        // plaintext archive header. This reader can still identify such an
        // archive without pretending that encrypted bytes are normal headers.
        if (archiveBlock.nFlags & RAR4_ARCHIVE_PASSWORD) {
            if (nCurrentOffset >= nTotalSize) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }
            pContext->bHeadersEncrypted = true;
            pContext->nArchiveEnd = nTotalSize;
        } else {
            qint32 nBlockCount = 0;
            bool bReachedEnd = false;

            while (nCurrentOffset < nTotalSize) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nBlockCount >= XRAR_MAX_RECORDS)) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                GENERICBLOCK4 genericBlock = guardedArchive->readGenericBlock4(
                    nCurrentOffset);
                if (!guardedArchive) {
                    delete pContext;
                    *pUnpackState = UNPACK_STATE();
                    return false;
                }
                if ((genericBlock.nHeaderSize == 0) || (genericBlock.nType < BLOCKTYPE4_MARKER) || (genericBlock.nType > BLOCKTYPE4_END)) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                quint64 nDataSize = 0;

                if ((genericBlock.nType == BLOCKTYPE4_FILE) || (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW)) {
                    FILEBLOCK4 fileBlock = guardedArchive->readFileBlock4(
                        nCurrentOffset);
                    if (!guardedArchive) {
                        delete pContext;
                        *pUnpackState = UNPACK_STATE();
                        return false;
                    }
                    if (fileBlock.genericBlock4.nHeaderSize == 0) {
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    nDataSize = (quint64)fileBlock.packSize;
                    if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                        nDataSize |= (quint64)fileBlock.highPackSize << 32;
                    }

                    if (genericBlock.nType == BLOCKTYPE4_FILE) {
                        if (pUnpackState->nNumberOfRecords >= XRAR_MAX_RECORDS) {
                            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                        }
                        pContext->listFileOffsets.append(nCurrentOffset);
                        pContext->listFileBlocks4.append(fileBlock);
                        pUnpackState->nNumberOfRecords++;
                    }
                } else if (genericBlock.nFlags & RAR4_LONG_BLOCK) {
                    if (genericBlock.nHeaderSize < 11) {
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }
                    nDataSize = guardedArchive->read_uint32(
                        nCurrentOffset + 7);
                    if (!guardedArchive) {
                        delete pContext;
                        *pUnpackState = UNPACK_STATE();
                        return false;
                    }
                }

                quint64 nBlockSize = (quint64)genericBlock.nHeaderSize + nDataSize;
                if (nBlockSize > (quint64)(nTotalSize - nCurrentOffset)) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                nCurrentOffset += (qint64)nBlockSize;
                nBlockCount++;

                if (genericBlock.nType == BLOCKTYPE4_END) {
                    bReachedEnd = true;
                    break;
                }
            }

            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            // Early RAR generations can end exactly after the last complete
            // data block. Newer archives normally carry ENDARC_HEAD.
            if (!bReachedEnd) {
                if ((nCurrentOffset != nTotalSize) || pContext->listFileBlocks4.isEmpty()) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                for (qint32 i = 0; i < pContext->listFileBlocks4.count(); i++) {
                    if (pContext->listFileBlocks4.at(i).unpVer >= 29) {
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }
                }
            }
            pContext->nArchiveEnd = nCurrentOffset;

            qint32 nFolderIndex = 0;
            for (qint32 i = 0; i < pContext->listFileBlocks4.count(); i++) {
                bool bPerFileSolid = (pContext->listFileBlocks4.at(i).genericBlock4.nFlags & 0x0010) != 0;
                if (!bPerFileSolid && (i > 0)) {
                    nFolderIndex++;
                }
                pContext->listSolidFolderIndex.append(nFolderIndex);
            }
        }
    } else if (pContext->nVersion == 5) {
        qint32 nHeaderCount = 0;
        bool bSawMainHeader = false;
        bool bReachedEnd = false;
        bool bStoppedAtEncryptedHeaders = false;

        while (nCurrentOffset < nTotalSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nHeaderCount >= XRAR_MAX_RECORDS)) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            GENERICHEADER5 genericHeader = guardedArchive->readGenericHeader5(
                nCurrentOffset);
            if (!guardedArchive) {
                delete pContext;
                *pUnpackState = UNPACK_STATE();
                return false;
            }
            if (genericHeader.nHeaderSize == 0) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            qint64 nHeaderEnd = nCurrentOffset + (qint64)genericHeader.nHeaderSize;
            if ((genericHeader.nDataSize > (quint64)(nTotalSize - nHeaderEnd)) ||
                ((genericHeader.nType > HEADERTYPE5_ENDARC) && ((genericHeader.nFlags & 0x0004) == 0)) ||
                (genericHeader.nType == 0)) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            if (genericHeader.nType == HEADERTYPE5_MAIN) {
                if (bSawMainHeader || (nHeaderCount != 0) || (genericHeader.nDataSize != 0) ||
                    !guardedArchive->isMainOrEndHeader5Valid(
                        nCurrentOffset, genericHeader) || !guardedArchive) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
                bSawMainHeader = true;
            } else if (!bSawMainHeader && !((genericHeader.nType == HEADERTYPE5_ENCRYPTION) && (nHeaderCount == 0))) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            } else if (genericHeader.nType == HEADERTYPE5_FILE) {
                FILEHEADER5 fileHeader = guardedArchive->readFileHeader5(
                    nCurrentOffset);
                if (!guardedArchive) {
                    delete pContext;
                    *pUnpackState = UNPACK_STATE();
                    return false;
                }
                if (fileHeader.nHeaderSize == 0) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
                if (pUnpackState->nNumberOfRecords >= XRAR_MAX_RECORDS) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
                pContext->listFileOffsets.append(nCurrentOffset);
                pContext->listFileHeaders5.append(fileHeader);
                pUnpackState->nNumberOfRecords++;
            } else if (genericHeader.nType == HEADERTYPE5_SERVICE) {
                const FILEHEADER5 checkedHeader =
                    guardedArchive->readFileHeader5(nCurrentOffset);
                if (!guardedArchive) {
                    delete pContext;
                    *pUnpackState = UNPACK_STATE();
                    return false;
                }
                if (checkedHeader.nHeaderSize == 0) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
            } else if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
                qint64 nBodyOffset = nCurrentOffset + 4;
                quint64 nValue = 0;

                if (!guardedArchive->readVIntBounded(&nBodyOffset, nHeaderEnd, 10, &nValue) ||
                    !guardedArchive ||
                    !guardedArchive->readVIntBounded(&nBodyOffset, nHeaderEnd, 10, &nValue) ||
                    !guardedArchive ||
                    !guardedArchive->readVIntBounded(&nBodyOffset, nHeaderEnd, 10, &nValue) ||
                    !guardedArchive) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
                if (genericHeader.nFlags & 0x0001) {
                    if (!guardedArchive->readVIntBounded(&nBodyOffset, nHeaderEnd, 10, &nValue) ||
                        !guardedArchive) return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
                if (genericHeader.nFlags & 0x0002) {
                    if (!guardedArchive->readVIntBounded(&nBodyOffset, nHeaderEnd, 10, &nValue) ||
                        !guardedArchive) return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                qint64 nBodyEnd = nHeaderEnd - (qint64)genericHeader.nExtraAreaSize;
                quint64 nEncVersion = 0;
                quint64 nEncFlags = 0;
                if (!guardedArchive->readVIntBounded(&nBodyOffset, nBodyEnd, 10, &nEncVersion) ||
                    !guardedArchive ||
                    !guardedArchive->readVIntBounded(&nBodyOffset, nBodyEnd, 10, &nEncFlags) ||
                    !guardedArchive || ((nBodyEnd - nBodyOffset) < 17)) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                quint8 nKdfCount = guardedArchive->read_uint8(nBodyOffset);
                if (!guardedArchive) {
                    delete pContext;
                    *pUnpackState = UNPACK_STATE();
                    return false;
                }
                nBodyOffset++;
                QByteArray baSalt = guardedArchive->read_array(
                    nBodyOffset, 16);
                if (!guardedArchive) {
                    delete pContext;
                    *pUnpackState = UNPACK_STATE();
                    return false;
                }
                nBodyOffset += 16;

                qint64 nExpectedTailSize = (nEncFlags & 0x0001) ? 12 : 0;
                if ((baSalt.size() != 16) || (nEncVersion != 0) || (nEncFlags & ~((quint64)0x0001)) ||
                    (nKdfCount > XRAR_MAX_KDF_COUNT) || ((nBodyEnd - nBodyOffset) != nExpectedTailSize)) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                pContext->bHeadersEncrypted = true;
                QString sPassword = pUnpackState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
                nCurrentOffset = nHeaderEnd + (qint64)genericHeader.nDataSize;

                if (sPassword.isEmpty()) {
                    if (nCurrentOffset >= nTotalSize) {
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }
                    pContext->nArchiveEnd = nTotalSize;
                    bStoppedAtEncryptedHeaders = true;
                    break;
                }

                QByteArray baHeaderAesKey = XAESDecoder::deriveRar5HeaderKey(sPassword, baSalt, nKdfCount);
                if (baHeaderAesKey.isEmpty()) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }

                qint32 nEncryptedHeaderCount = 0;
                bool bSawEncryptedMainHeader = false;

                while (nCurrentOffset < nTotalSize) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nHeaderCount >= XRAR_MAX_RECORDS)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    qint64 nConsumed = 0;
                    QByteArray baDecHeader = guardedArchive->decryptRar5HeaderBlock(
                        nCurrentOffset, baHeaderAesKey, &nConsumed);
                    if (!guardedArchive) {
                        baHeaderAesKey.fill(0);
                        delete pContext;
                        *pUnpackState = UNPACK_STATE();
                        return false;
                    }
                    if (baDecHeader.isEmpty() || (nConsumed <= 0)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    QBuffer bufHeader(&baDecHeader);
                    if (!bufHeader.open(QIODevice::ReadOnly)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    XRar rarTemp(&bufHeader);
                    GENERICHEADER5 decGeneric = rarTemp.readGenericHeader5(0);
                    if ((decGeneric.nHeaderSize != (quint64)baDecHeader.size()) ||
                        ((decGeneric.nType > HEADERTYPE5_ENDARC) && ((decGeneric.nFlags & 0x0004) == 0)) ||
                        (decGeneric.nType == 0) || (decGeneric.nType == HEADERTYPE5_ENCRYPTION)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    if ((quint64)nConsumed > (quint64)(nTotalSize - nCurrentOffset)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }
                    qint64 nDataOffset = nCurrentOffset + nConsumed;
                    if (decGeneric.nDataSize > (quint64)(nTotalSize - nDataOffset)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    if (decGeneric.nType == HEADERTYPE5_MAIN) {
                        if (bSawEncryptedMainHeader || (nEncryptedHeaderCount != 0) || (decGeneric.nDataSize != 0)) {
                            baHeaderAesKey.fill(0);
                            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                        }
                        if (!rarTemp.isMainOrEndHeader5Valid(0, decGeneric)) {
                            baHeaderAesKey.fill(0);
                            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                        }
                        bSawEncryptedMainHeader = true;
                    } else if (!bSawEncryptedMainHeader) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    } else if (decGeneric.nType == HEADERTYPE5_FILE) {
                        FILEHEADER5 fileHeader = rarTemp.readFileHeader5(0);
                        if (fileHeader.nHeaderSize == 0) {
                            baHeaderAesKey.fill(0);
                            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                        }
                        fileHeader.nHeaderSize = nConsumed;
                        pContext->listFileOffsets.append(nCurrentOffset);
                        pContext->listFileHeaders5.append(fileHeader);
                        pUnpackState->nNumberOfRecords++;
                    } else if (decGeneric.nType == HEADERTYPE5_SERVICE) {
                        if (rarTemp.readFileHeader5(0).nHeaderSize == 0) {
                            baHeaderAesKey.fill(0);
                            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                        }
                    } else if ((decGeneric.nType == HEADERTYPE5_ENDARC) &&
                               !rarTemp.isMainOrEndHeader5Valid(0, decGeneric)) {
                        baHeaderAesKey.fill(0);
                        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                    }

                    nCurrentOffset = nDataOffset + (qint64)decGeneric.nDataSize;
                    nHeaderCount++;
                    nEncryptedHeaderCount++;

                    if (decGeneric.nType == HEADERTYPE5_ENDARC) {
                        bReachedEnd = true;
                        pContext->nArchiveEnd = nCurrentOffset;
                        break;
                    }
                }

                baHeaderAesKey.fill(0);
                if (!bReachedEnd) {
                    return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
                }
                break;
            }

            if ((genericHeader.nType == HEADERTYPE5_ENDARC) &&
                (!guardedArchive->isMainOrEndHeader5Valid(
                     nCurrentOffset, genericHeader) ||
                 !guardedArchive)) {
                return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
            }

            nCurrentOffset = nHeaderEnd + (qint64)genericHeader.nDataSize;
            nHeaderCount++;

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                bReachedEnd = true;
                pContext->nArchiveEnd = nCurrentOffset;
                break;
            }
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (!bReachedEnd && !bStoppedAtEncryptedHeaders)) {
            return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
        }

        for (qint32 i = 0; i < pContext->listFileHeaders5.count(); i++) {
            if ((pContext->listFileHeaders5.at(i).nCompInfo >> 6) & 1) {
                pContext->bArchiveIsSolid = true;
                break;
            }
        }

        // Compute solid folder indices for RAR5: increment when per-file solid flag is false
        {
            qint32 nFolderIndex = 0;
            for (qint32 i = 0; i < pContext->listFileHeaders5.count(); i++) {
                bool bPerFileSolid = (pContext->listFileHeaders5.at(i).nCompInfo >> 6) & 1;
                if (!bPerFileSolid && (i > 0)) {
                    nFolderIndex++;
                }
                pContext->listSolidFolderIndex.append(nFolderIndex);
            }
        }
    }

    pUnpackState->pContext = pContext;
    pUnpackState->nCurrentOffset = (pUnpackState->nNumberOfRecords > 0) ? pContext->listFileOffsets.at(0) : 0;

    if (!guardedArchive->validateAndFinalizeUnpackSource(
            pUnpackState, pContext, pPdStruct)) {
        if (!guardedArchive) return false;
        return _initUnpackFail(&guardedArchive, pUnpackState, pContext);
    }

    return true;
}

XBinary::ARCHIVERECORD XRar::infoCurrent(XBinary::UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    ARCHIVERECORD record = {};

    if (!pUnpackState || !pUnpackState->pContext || (pUnpackState->nCurrentIndex < 0) ||
        (pUnpackState->nCurrentIndex >= pUnpackState->nNumberOfRecords) ||
        !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return record;
    }
    if (!guardedArchive->isUnpackSourceCurrent(pUnpackState, pPdStruct) ||
        !guardedArchive || !guardedSource) return record;

    RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
    qint32 nIndex = pUnpackState->nCurrentIndex;

    if (pContext->nVersion == 1) {
        const FILEBLOCK14 &fileBlock = pContext->listFileBlocks14.at(nIndex);

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileBlock.nHeaderSize;
        record.nStreamSize = (qint64)fileBlock.nPackSize;

        record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, fileBlock.sFileName);
        record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)fileBlock.nPackSize);
        record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)fileBlock.nUnpSize);
        record.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, (quint32)fileBlock.nFileCRC16);
        record.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_RAR14);

        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
        if (fileBlock.nMethod == 0) {
            compressMethod = HANDLE_METHOD_STORE;
        } else {
            // Methods 1-5 all use the same RAR 1.5 algorithm
            compressMethod = HANDLE_METHOD_RAR_15;
            record.mapProperties.insert(XBinary::FPART_PROP_WINDOWSIZE, (qint32)0x10000);
        }
        record.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        bool bIsFolder = (fileBlock.nFileAttr & 0x10) != 0;
        record.mapProperties.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

        if (pContext->bArchiveIsSolid) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if (nIndex < pContext->listSolidFolderIndex.count()) {
            record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)pContext->listSolidFolderIndex.at(nIndex));
        }

    } else if (pContext->nVersion == 4) {
        const FILEBLOCK4 &fileBlock = pContext->listFileBlocks4.at(nIndex);

        qint64 nPackSize = fileBlock.packSize;
        qint64 nUnpSize = fileBlock.unpSize;

        if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
            nPackSize |= ((qint64)fileBlock.highPackSize << 32);
            nUnpSize |= ((qint64)fileBlock.highUnpSize << 32);
        }

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileBlock.genericBlock4.nHeaderSize;
        record.nStreamSize = nPackSize;

        record.mapProperties = guardedArchive->_readProperties(fileBlock);
        if (!guardedArchive) return XBinary::ARCHIVERECORD();

        // For solid archives, mark ALL files as solid so decompressArchiveRecord
        // routes them through the persistent decoder path
        if (pContext->bArchiveIsSolid) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if (nIndex < pContext->listSolidFolderIndex.count()) {
            record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)pContext->listSolidFolderIndex.at(nIndex));
        }

    } else if (pContext->nVersion == 5) {
        const FILEHEADER5 &fileHeader = pContext->listFileHeaders5.at(nIndex);

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileHeader.nHeaderSize;
        record.nStreamSize = fileHeader.nDataSize;

        record.mapProperties = guardedArchive->_readProperties(fileHeader);
        if (!guardedArchive) return XBinary::ARCHIVERECORD();

        // For solid archives, mark ALL files as solid so decompressArchiveRecord
        // routes them through the persistent decoder path
        if (pContext->bArchiveIsSolid) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if (nIndex < pContext->listSolidFolderIndex.count()) {
            record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)pContext->listSolidFolderIndex.at(nIndex));
        }
    }

    if (!guardedArchive->isUnpackSourceCurrent(pUnpackState, pPdStruct) ||
        !guardedArchive || !guardedSource) {
        return XBinary::ARCHIVERECORD();
    }
    return record;
}

bool XRar::unpackCurrent(XBinary::UNPACK_STATE *pUnpackState, QIODevice *pOutputDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedOutput(pOutputDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pUnpackState || !pUnpackState->pContext || !guardedSource ||
        !guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (!guardedArchive->isUnpackOutputSupported(guardedOutput.data()) ||
        !guardedArchive || !guardedOutput || !guardedSource) return false;
    if (!guardedArchive->isUnpackSourceCurrent(pUnpackState, pPdStruct) ||
        !guardedArchive || !guardedSource || !guardedOutput) return false;

    if ((pUnpackState->nCurrentIndex < 0) || (pUnpackState->nCurrentIndex >= pUnpackState->nNumberOfRecords)) {
        return false;
    }

    RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
    UNPACK_INFO_AUTHORIZATION infoAuthorization(m_pUnpackGuardState);
    if (!infoAuthorization.isAuthorized()) return false;
    ARCHIVERECORD archiveRecord = guardedArchive->infoCurrent(
        pUnpackState, pPdStruct);
    if (!guardedArchive || !guardedSource || !guardedOutput ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data())) {
        return false;
    }

    const bool bIsFolder =
        archiveRecord.mapProperties.value(FPART_PROP_ISFOLDER).toBool();
    const qint64 nExpectedSize = bIsFolder ? 0 :
        archiveRecord.mapProperties
            .value(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1)
            .toLongLong();
    if (nExpectedSize < 0) return false;

    std::unique_ptr<QIODevice> pStage(
        XBinary::createFileBuffer(nExpectedSize, pPdStruct));
    if (!pStage || !guardedArchive || !guardedSource || !guardedOutput)
        return false;
    if (!guardedArchive->isUnpackSourceCurrent(pUnpackState, pPdStruct) ||
        !guardedArchive || !guardedSource || !guardedOutput) return false;

    bool bResult = bIsFolder ||
        pContext->decompress.decompressArchiveRecord(
            archiveRecord, guardedSource.data(), pStage.get(),
            pUnpackState->mapUnpackProperties, pPdStruct);

    if (!guardedArchive || !guardedSource || !guardedOutput || !bResult)
        return false;

    // bool bResult = false;
    // // For solid archives: first file is not solid (bIsSolid=false), subsequent files are solid (bIsSolid=true)
    // bool bIsSolid = (pUnpackState->nCurrentIndex > 0);

    // SubDevice sd(getDevice(), record.nStreamOffset, record.nStreamSize);

    // if (sd.open(QIODevice::ReadOnly)) {
    //     if (compressMethod == HANDLE_METHOD_STORE) {
    //         qint64 nDataOffset = record.nStreamOffset;
    //         qint64 nDataSize = record.nStreamSize;

    //         bResult = XBinary::copyDeviceMemory(getDevice(), nDataOffset, pOutputDevice, 0, nDataSize);  // TODO
    //     } else if ((compressMethod == HANDLE_METHOD_RAR_15) || (compressMethod == HANDLE_METHOD_RAR_20) || (compressMethod == HANDLE_METHOD_RAR_29) ||
    //                (compressMethod == HANDLE_METHOD_RAR_50) || (compressMethod == HANDLE_METHOD_RAR_70)) {
    //         qint32 nWindowSize = record.mapProperties.value(FPART_PROP_WINDOWSIZE).toInt();
    //         qint64 nUncompressedSize = record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    //         pContext->rarUnpacker.setDevices(&sd, pOutputDevice);
    //         qint32 nInit = pContext->rarUnpacker.Init(nWindowSize, bIsSolid);

    //         if (nInit > 0) {
    //             pContext->rarUnpacker.SetDestSize(nUncompressedSize);
    //             if (compressMethod == HANDLE_METHOD_RAR_15) {
    //                 pContext->rarUnpacker.Unpack15(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             } else if (compressMethod == HANDLE_METHOD_RAR_20) {
    //                 pContext->rarUnpacker.Unpack20(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             } else if (compressMethod == HANDLE_METHOD_RAR_29) {
    //                 pContext->rarUnpacker.Unpack29(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             } else if ((compressMethod == HANDLE_METHOD_RAR_50) || (compressMethod == HANDLE_METHOD_RAR_70)) {
    //                 pContext->rarUnpacker.Unpack5(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             }
    //         }
    //     }

    //     sd.close();
    // }

    if (!guardedArchive->isUnpackSourceCurrent(pUnpackState, pPdStruct) ||
        !guardedArchive || !guardedSource || !guardedOutput) return false;
    const bool bPublished = guardedArchive->publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pUnpackState, pPdStruct);
    return bPublished && guardedArchive && guardedSource && guardedOutput;
}

bool XRar::moveToNext(XBinary::UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pUnpackState || !pUnpackState->pContext || !guardedSource ||
        (pUnpackState->nCurrentIndex < 0) ||
        (pUnpackState->nCurrentIndex >= pUnpackState->nNumberOfRecords) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (!guardedArchive->isUnpackSourceCurrent(pUnpackState, pPdStruct) ||
        !guardedArchive || !guardedSource) return false;

    pUnpackState->nCurrentIndex++;

    if (pUnpackState->nCurrentIndex < pUnpackState->nNumberOfRecords) {
        RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
        pUnpackState->nCurrentOffset = pContext->listFileOffsets.at(pUnpackState->nCurrentIndex);

        return true;
    } else {
        return false;
    }
}

bool XRar::finishUnpack(XBinary::UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    QPointer<XRar> guardedArchive(this);
    if (!pUnpackState) {
        return false;
    }

    if ((pUnpackState->pContext ||
         !pUnpackState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pUnpackState)) {
        return false;
    }
    guardedArchive->releaseUnpackSource(pUnpackState);

    if (pUnpackState->pContext) {
        RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
        pUnpackState->pContext = nullptr;
        delete pContext;
        if (!guardedArchive) return false;
    }

    pUnpackState->nCurrentOffset = 0;
    pUnpackState->nTotalSize = 0;
    pUnpackState->nCurrentIndex = 0;
    pUnpackState->nNumberOfRecords = 0;
    pUnpackState->mapUnpackProperties.clear();
    pUnpackState->mapArchiveProperties.clear();
    return true;
}

QList<XBinary::FPART_PROP> XRar::getAvailableFPARTProperties()
{
    QList<XBinary::FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}

QMap<XBinary::FPART_PROP, QVariant> XRar::_readProperties(const FILEBLOCK4 &fileBlock4)
{
    QMap<XBinary::FPART_PROP, QVariant> mapResult;

    qint64 nPackSize = fileBlock4.packSize;
    qint64 nUnpSize = fileBlock4.unpSize;

    if (fileBlock4.genericBlock4.nFlags & RAR4_FILE_LARGE) {
        nPackSize |= ((qint64)fileBlock4.highPackSize << 32);
        nUnpSize |= ((qint64)fileBlock4.highUnpSize << 32);
    }

    mapResult.insert(XBinary::FPART_PROP_ORIGINALNAME, fileBlock4.sFileName);
    mapResult.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nPackSize);
    mapResult.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUnpSize);
    mapResult.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    mapResult.insert(XBinary::FPART_PROP_RESULTCRC, fileBlock4.fileCRC);

    HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

    if (fileBlock4.method == RAR_METHOD_STORE) {
        compressMethod = HANDLE_METHOD_STORE;
    } else if (fileBlock4.unpVer == 15) {
        compressMethod = HANDLE_METHOD_RAR_15;
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, 0x10000);
    } else if ((fileBlock4.unpVer == 20) || (fileBlock4.unpVer == 26)) {
        compressMethod = HANDLE_METHOD_RAR_20;
    } else if (fileBlock4.unpVer == 29) {
        compressMethod = HANDLE_METHOD_RAR_29;
    }

    mapResult.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

    // Solid flag: RAR4 file header nFlags bit 0x0010
    bool bIsSolid = (fileBlock4.genericBlock4.nFlags & 0x0010) != 0;
    mapResult.insert(XBinary::FPART_PROP_ISSOLID, bIsSolid);

    // Directory flag: RAR4 uses dictionary size field (bits 7-5) == 7 or fileAttr & 0x10
    bool bIsFolder = ((fileBlock4.genericBlock4.nFlags & 0x00E0) == 0x00E0) || (fileBlock4.fileAttr & 0x10);
    mapResult.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

    // Extract DOS date and time from 32-bit fileTime field (date in high word, time in low word)
    quint16 nDosTime = fileBlock4.fileTime & 0xFFFF;
    quint16 nDosDate = (fileBlock4.fileTime >> 16) & 0xFFFF;
    QDateTime dateTime = XBinary::dosDateTimeToQDateTime(nDosDate, nDosTime);
    mapResult.insert(XBinary::FPART_PROP_DATETIME, dateTime);

    return mapResult;
}

static bool rarReadExtraVInt(const char *pExtraData, qint64 nExtraSize, qint64 *pOffset, qint64 nEndOffset, quint64 *pValue)
{
    if (!pOffset || !pValue || (*pOffset < 0) || (nEndOffset < *pOffset) || (nEndOffset > nExtraSize)) {
        return false;
    }

    quint64 nValue = 0;
    for (qint32 i = 0; (i < 10) && (*pOffset < nEndOffset); i++) {
        quint8 nByte = (quint8)pExtraData[*pOffset];
        (*pOffset)++;
        if ((i == 9) && (nByte & 0xFE)) {
            return false;
        }
        nValue |= (quint64)(nByte & 0x7F) << (i * 7);
        if ((nByte & 0x80) == 0) {
            *pValue = nValue;
            return true;
        }
    }
    return false;
}

QMap<XBinary::FPART_PROP, QVariant> XRar::_readProperties(const FILEHEADER5 &fileHeader5)
{
    QMap<XBinary::FPART_PROP, QVariant> mapResult;

    mapResult.insert(XBinary::FPART_PROP_ORIGINALNAME, fileHeader5.sFileName);
    mapResult.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, fileHeader5.nDataSize);
    mapResult.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, fileHeader5.nUnpackedSize);

    // RAR5 stores nDataCRC32 only when the CRC-present flag is set. A zero
    // value is still a valid CRC and must not be used as a presence sentinel.
    if (fileHeader5.nFileFlags & 0x0004) {
        mapResult.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
        mapResult.insert(XBinary::FPART_PROP_RESULTCRC, fileHeader5.nDataCRC32);
    }

    quint8 nVer = fileHeader5.nCompInfo & 0x003f;
    quint8 nMethod = (fileHeader5.nCompInfo >> 7) & 7;

    HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

    if (nMethod == RAR5_METHOD_STORE) {
        compressMethod = HANDLE_METHOD_STORE;
    } else if (nVer == 0) {
        compressMethod = HANDLE_METHOD_RAR_50;
    } else if (nVer == 1) {
        compressMethod = HANDLE_METHOD_RAR_70;
    }

    mapResult.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

    // Solid flag: bit 6 of nCompInfo
    bool bIsSolid = (fileHeader5.nCompInfo >> 6) & 1;
    mapResult.insert(XBinary::FPART_PROP_ISSOLID, bIsSolid);

    // Directory flag: RAR5 nFileFlags bit 0 = directory
    bool bIsFolder = (fileHeader5.nFileFlags & 0x0001) != 0;
    mapResult.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

    // Calculate window (dictionary) size from compression info
    if (nVer == 0) {
        // RAR 5.0: bits 10-14 encode dictionary size as 128KB << dictBits
        quint8 nDictBits = (fileHeader5.nCompInfo >> 10) & 0x1F;
        quint64 nWindowSize = (quint64)0x20000 << nDictBits;
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, nWindowSize);
    } else if (nVer == 1) {
        // RAR 7.0: bits 10-14 = d, bit 15 = fraction flag
        // If fraction flag is 0: window_size = 128KB << d
        // If fraction flag is 1: window_size = 3 * (128KB << (d-1))  (1.5x rounding)
        quint8 nDictBits = (fileHeader5.nCompInfo >> 10) & 0x1F;
        bool bFraction = (fileHeader5.nCompInfo >> 15) & 1;
        quint64 nWindowSize;
        if (!bFraction) {
            nWindowSize = (quint64)0x20000 << nDictBits;
        } else {
            nWindowSize = 3 * ((quint64)0x20000 << (nDictBits > 0 ? nDictBits - 1 : 0));
        }
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, nWindowSize);
    }

    if (fileHeader5.nFileFlags & 0x0002) {
        QDateTime dateTime = XBinary::valueToTime(fileHeader5.nMTime, XBinary::DT_TYPE_UNIXTIME);
        mapResult.insert(XBinary::FPART_PROP_DATETIME, dateTime);
    }

    // Parse extra area for encryption record (id=1)
    if (!fileHeader5.baExtraArea.isEmpty()) {
        qint64 nExtraOffset = 0;
        qint64 nExtraSize = fileHeader5.baExtraArea.size();
        const char *pExtraData = fileHeader5.baExtraArea.constData();

        while (nExtraOffset < nExtraSize) {
            quint64 nRecSize = 0;
            if (!rarReadExtraVInt(pExtraData, nExtraSize, &nExtraOffset, nExtraSize, &nRecSize) || (nRecSize == 0) ||
                (nRecSize > (quint64)(nExtraSize - nExtraOffset))) {
                break;
            }

            qint64 nRecEnd = nExtraOffset + (qint64)nRecSize;
            quint64 nRecId = 0;
            if (!rarReadExtraVInt(pExtraData, nExtraSize, &nExtraOffset, nRecEnd, &nRecId)) {
                break;
            }

            if (nRecId == 1) {
                // Encryption record: [varint version=0][varint flags][byte cnt][16 salt][16 IV][opt 12 pswCheck]
                quint64 nCryptoVersion = 0;
                quint64 nCryptoFlags = 0;

                if (rarReadExtraVInt(pExtraData, nExtraSize, &nExtraOffset, nRecEnd, &nCryptoVersion) && (nCryptoVersion == 0) &&
                    rarReadExtraVInt(pExtraData, nExtraSize, &nExtraOffset, nRecEnd, &nCryptoFlags) && ((nCryptoFlags & ~((quint64)0x0003)) == 0)) {
                    bool bHasPswCheck = (nCryptoFlags & 0x0001) != 0;  // flag bit 0 = password check present

                    if (((nRecEnd - nExtraOffset) >= (1 + 16 + 16)) &&
                        (!bHasPswCheck || ((nRecEnd - nExtraOffset) >= (1 + 16 + 16 + 12)))) {
                        quint8 nCnt = (quint8)pExtraData[nExtraOffset];
                        nExtraOffset += 1;

                        QByteArray baSalt(pExtraData + nExtraOffset, 16);
                        nExtraOffset += 16;

                        QByteArray baIV(pExtraData + nExtraOffset, 16);
                        nExtraOffset += 16;

                        QByteArray baPswCheck;
                        if (bHasPswCheck) {
                            baPswCheck = QByteArray(pExtraData + nExtraOffset, 12);
                            nExtraOffset += 12;
                        }

                        if ((nCnt <= XRAR_MAX_KDF_COUNT) && (nExtraOffset == nRecEnd)) {
                            // Pack crypto params: [1 byte cnt][16 salt][16 IV][opt 12 pswCheck]
                            QByteArray baAESKey;
                            baAESKey.append((char)nCnt);
                            baAESKey.append(baSalt);
                            baAESKey.append(baIV);
                            if (!baPswCheck.isEmpty()) {
                                baAESKey.append(baPswCheck);
                            }

                            mapResult.insert(XBinary::FPART_PROP_ENCRYPTED, true);
                            mapResult.insert(XBinary::FPART_PROP_AESKEY, baAESKey);

                            // RAR5_AES is the outer (decryption) method; original compress method stays as HANDLEMETHOD (inner)
                            mapResult.insert(XBinary::FPART_PROP_HANDLEMETHOD2, (quint32)HANDLE_METHOD_RAR5_AES);

                            // Encryption flag bit 1 replaces the stored CRC32 with
                            // a keyed value derived from the plaintext CRC32. Keep
                            // the field and mark it so extraction authenticates it
                            // with the RAR5 hash key after decryption/decompression.
                            if (nCryptoFlags & 0x0002) {
                                mapResult.insert(XBinary::FPART_PROP_RAR5_HASHMAC, true);
                            }
                        }
                    }
                }
            }

            nExtraOffset = nRecEnd;
        }
    }

    return mapResult;
}

QList<XBinary::MAPMODE> XRar::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XRar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_REGIONS;  // Default mode
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

XBinary::FT XRar::getFileType()
{
    return FT_RAR;
}

QByteArray XRar::decryptRar5HeaderBlock(qint64 nOffset, const QByteArray &baAesKey, qint64 *pConsumedSize)
{
    if (!pConsumedSize || (baAesKey.size() != 32)) {
        return QByteArray();
    }

    *pConsumedSize = 0;
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || (nOffset < 0)) {
        return QByteArray();
    }

    // Capture the IV and the first encrypted block atomically from the parser's
    // point of view. A caller-controlled device may synchronously delete or
    // rebind the archive during the read callback.
    const QByteArray baPrefix = guardedArchive->read_array(nOffset, 32);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (baPrefix.size() != 32)) {
        return QByteArray();
    }

    const QByteArray baIV = baPrefix.left(16);
    const QByteArray baFirstCipher = baPrefix.mid(16, 16);

    // Decrypt first block to get CRC and headerSize
    QByteArray baFirstPlain(16, 0);

    if (!XAESDecoder::decryptAESCBC(baAesKey, baIV, (const quint8 *)baFirstCipher.constData(), (quint8 *)baFirstPlain.data(), 16)) {
        return QByteArray();
    }

    quint64 nHeaderDataSize = 0;
    qint32 nSizeFieldLength = 0;
    bool bTerminated = false;

    for (qint32 i = 0; i < 10; i++) {
        quint8 nByte = (quint8)baFirstPlain.at(4 + i);
        if ((i == 9) && (nByte & 0xFE)) {
            return QByteArray();
        }
        nHeaderDataSize |= (quint64)(nByte & 0x7F) << (i * 7);
        nSizeFieldLength++;
        if ((nByte & 0x80) == 0) {
            bTerminated = true;
            break;
        }
    }

    if (!bTerminated || (nHeaderDataSize < 2) || (nHeaderDataSize > (quint64)XRAR_MAX_RAR5_HEADER_SIZE)) {
        return QByteArray();
    }

    quint64 nPlainSize = 4 + (quint64)nSizeFieldLength + nHeaderDataSize;
    if (nPlainSize > (quint64)XRAR_MAX_RAR5_HEADER_SIZE) {
        return QByteArray();
    }

    quint64 nEncSize = ((nPlainSize + 15) / 16) * 16;

    const qint64 nTotalSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        (nOffset > nTotalSize) ||
        ((16 + nEncSize) > (quint64)(nTotalSize - nOffset))) {
        return QByteArray();
    }

    if (nEncSize <= 16) {
        // First AES block was enough
        *pConsumedSize = 16 + (qint64)nEncSize;
        return baFirstPlain.left((qint32)nPlainSize);
    }

    // Need more data — capture the IV and complete encrypted header together,
    // then re-decrypt from that single bounded snapshot.
    const QByteArray baEncrypted = guardedArchive->read_array(
        nOffset, (qint64)(16 + nEncSize));
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        ((quint64)baEncrypted.size() != (16 + nEncSize)) ||
        (baEncrypted.left(16) != baIV)) {
        return QByteArray();
    }
    const QByteArray baAllCipher = baEncrypted.mid(16);

    QByteArray baAllPlain((qint32)nEncSize, 0);

    if (!XAESDecoder::decryptAESCBC(baAesKey, baIV, (const quint8 *)baAllCipher.constData(), (quint8 *)baAllPlain.data(), nEncSize)) {
        return QByteArray();
    }

    *pConsumedSize = 16 + (qint64)nEncSize;
    return baAllPlain.left((qint32)nPlainSize);
}

XRar::GENERICHEADER5 XRar::readGenericHeader5(qint64 nOffset)
{
    GENERICHEADER5 result = {};
    QPointer<XRar> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return result;

    const QByteArray baHeader = guardedArchive->readHeader5Snapshot(nOffset);
    if (!guardedArchive || !guardedSource ||
        (guardedArchive->getDevice() != guardedSource.data()) ||
        !parseGenericHeader5Snapshot(baHeader, &result)) {
        return GENERICHEADER5();
    }

    return result;
}

QList<QString> XRar::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'RE~^'");
    listResult.append("'Rar!'1A07");
    listResult.append("'Rar!'1A0700");
    listResult.append("'Rar!'1A070100");

    return listResult;
}

XBinary *XRar::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XRar(pDevice);
}

bool XRar::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XRar> guardedThis(this);
    bool bResult = true;

    if (!guardedThis->isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!bResult || !guardedThis) {
            return false;
        }

        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) {
            return false;
        }

        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) =
            *pInfo;
    }

    return guardedThis && bResult;
}

void *XRar::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XRar> guardedThis(this);
    if (!guardedThis->handleInternalInfo(pPdStruct) || !guardedThis) {
        return nullptr;
    }

    return &guardedThis->m_internalInfo;
}

void XRar::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
