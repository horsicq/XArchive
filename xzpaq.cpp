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
#include "xzpaq.h"

#include <cstring>

namespace {
const char ZPAQ_TAG[] = {0x37, 0x6B, 0x53, 0x74, (char)0xA0, 0x31, (char)0x83,
                         (char)0xD3, (char)0x8C, (char)0xB2, 0x28, (char)0xB0,
                         (char)0xD3};
}

XZPAQ::XZPAQ(QIODevice *pDevice)
    : XExternalArchive(pDevice, BACKEND_ZPAQ)
{
}

qint64 XZPAQ::getFirstBlockOffset() const
{
    XZPAQ *pThis = const_cast<XZPAQ *>(this);
    const qint64 nFileSize = pThis->getSize();
    if (nFileSize < ZPAQ_BLOCK_PREFIX_SIZE) return -1;

    const QByteArray baPrefix = pThis->read_array(0, qMin(nFileSize, (qint64)(ZPAQ_TAG_SIZE + 3)));
    if ((baPrefix.size() >= ZPAQ_TAG_SIZE + 3) &&
        (std::memcmp(baPrefix.constData(), ZPAQ_TAG, ZPAQ_TAG_SIZE) == 0) &&
        (std::memcmp(baPrefix.constData() + ZPAQ_TAG_SIZE, "zPQ", 3) == 0)) {
        return ZPAQ_TAG_SIZE;
    }

    if ((baPrefix.size() >= 3) && (std::memcmp(baPrefix.constData(), "zPQ", 3) == 0)) {
        return 0;
    }

    return -1;
}

quint8 XZPAQ::getLevel() const
{
    XZPAQ *pThis = const_cast<XZPAQ *>(this);
    const qint64 nOffset = getFirstBlockOffset();
    const qint64 nFileSize = pThis->getSize();
    return ((nOffset >= 0) && (nOffset <= nFileSize - ZPAQ_BLOCK_PREFIX_SIZE))
               ? pThis->read_uint8(nOffset + 3)
               : 0;
}

quint16 XZPAQ::getHeaderSize() const
{
    XZPAQ *pThis = const_cast<XZPAQ *>(this);
    const qint64 nOffset = getFirstBlockOffset();
    const qint64 nFileSize = pThis->getSize();
    return ((nOffset >= 0) && (nOffset <= nFileSize - ZPAQ_BLOCK_PREFIX_SIZE))
               ? pThis->read_uint16(nOffset + 5, false)
               : 0;
}

void XZPAQ::setAllowOpaqueEncrypted(bool bAllow)
{
    m_bAllowOpaqueEncrypted = bAllow;
}

bool XZPAQ::isValid(PDSTRUCT *pPdStruct)
{
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nBlockOffset = getFirstBlockOffset();
    const qint64 nFileSize = getSize();
    if ((nBlockOffset < 0) ||
        (nBlockOffset > nFileSize - ZPAQ_BLOCK_PREFIX_SIZE)) {
        // Encrypted ZPAQ has no plaintext signature: a 32-byte salt is
        // followed by AES-CTR ciphertext. This opt-in is set only after XSFX
        // validates zpaqfranz's exact executable-overlay framing. The helper
        // must still authenticate the password and archive before records are
        // exposed.
        return m_bAllowOpaqueEncrypted && (nFileSize > 32) &&
               isPdStructNotCanceled(pPdStruct);
    }

    const quint8 nLevel = read_uint8(nBlockOffset + 3);
    const quint8 nType = read_uint8(nBlockOffset + 4);
    const quint16 nHeaderSize = read_uint16(nBlockOffset + 5, false);
    const qint64 nRemaining = nFileSize - nBlockOffset - ZPAQ_BLOCK_PREFIX_SIZE;

    return isPdStructNotCanceled(pPdStruct) && ((nLevel == 1) || (nLevel == 2)) &&
           (nType == 1) && (nHeaderSize >= 7) && (nHeaderSize <= nRemaining) &&
           (read_uint8(nBlockOffset + ZPAQ_BLOCK_PREFIX_SIZE + nHeaderSize - 1) == 0);
}

bool XZPAQ::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    if (m_bAllowOpaqueEncrypted &&
        mapProperties.value(UNPACK_PROP_PASSWORD).toString().isEmpty() &&
        mapProperties.value(UNPACK_PROP_PASSWORD_BYTES).toByteArray().isEmpty()) {
        setLastExternalFailure(EXTERNAL_FAILURE_PASSWORD);
        XBinary::setPdStructErrorString(
            pPdStruct, tr("Encrypted ZPAQ archive password is required"));
        return false;
    }
    return XExternalArchive::initUnpack(
        pState, mapProperties, pPdStruct);
}

bool XZPAQ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZPAQ zpaq(pDevice);
    return zpaq.isValid(pPdStruct);
}

XBinary::MODE XZPAQ::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZPAQ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZPAQ::getFileFormatExt()
{
    return "zpaq";
}

QString XZPAQ::getFileFormatExtsString()
{
    return "ZPAQ (*.zpaq)";
}

XBinary::FT XZPAQ::getFileType()
{
    return FT_ZPAQ;
}

qint64 XZPAQ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

QString XZPAQ::getMIMEString()
{
    return "application/x-zpaq";
}

XBinary::OSNAME XZPAQ::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XZPAQ::getVersion()
{
    const quint8 nLevel = getLevel();
    return nLevel ? QString::number(nLevel) : QString();
}

QList<QString> XZPAQ::getSearchSignatures()
{
    return {"376B5374A03183D38CB228B0D3'zPQ'"};
}

XBinary *XZPAQ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZPAQ(pDevice);
}
