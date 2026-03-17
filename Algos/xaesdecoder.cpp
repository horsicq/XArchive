/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xaesdecoder.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDebug>
#include <cstring>

XAESDecoder::XAESDecoder(QObject *parent) : QObject(parent)
{
}

void XAESDecoder::deriveKey(const QString &sPassword, const QByteArray &baSalt, quint8 nNumCyclesPower, quint8 *pKey)
{
    // Note: tiny-AES-c does not require table initialization

    // Convert password to UTF-16LE bytes (7-Zip uses UTF-16LE, not UTF-8)
    // Reference: 7zAes.cpp CalcKey() uses CBuffer<Byte> Password which stores UTF-16LE
    QByteArray baPassword;
    for (qint32 i = 0; i < sPassword.size(); i++) {
        ushort ch = sPassword[i].unicode();
        baPassword.append((char)(ch & 0xFF));
        baPassword.append((char)((ch >> 8) & 0xFF));
    }

    // Note: Test code removed to reduce debug output
    // AES-256-CBC and SHA256 implementations verified correct with NIST test vectors

    // 7z AES key derivation algorithm
    // Key = SHA256^(2^NumCyclesPower) (Salt + Password + Counter)

    const quint32 nKeySize = 32;  // AES-256 key size
    const quint32 nNumRounds = (quint32)1 << nNumCyclesPower;

    // Prepare buffer: Salt + Password + 8-byte counter field
    // NOTE: 7z allocates 8 bytes for counter but only uses 32-bit values (first 4 bytes)!
    QByteArray baBuffer;
    baBuffer.append(baSalt);
    baBuffer.append(baPassword);
    baBuffer.append(QByteArray(8, '\0'));  // 8-byte counter field (last 4 bytes stay zero)

    const qint32 nCounterOffset = baBuffer.size() - 8;

    // Hash iteratively: SHA256^(2^NumCyclesPower) means:
    // For each round: hash (Salt + Password + Counter)
    // All rounds feed into one continuous SHA context
    XSha256Decoder::Context sha;
    XSha256Decoder::init(&sha);

    for (quint32 nRound = 0; nRound < nNumRounds; nRound++) {
        // Update 32-bit counter at end of buffer (little-endian)
        // CRITICAL: 7z uses ONLY 32-bit counter, not 64-bit!
        baBuffer[nCounterOffset + 0] = (nRound >> 0) & 0xFF;
        baBuffer[nCounterOffset + 1] = (nRound >> 8) & 0xFF;
        baBuffer[nCounterOffset + 2] = (nRound >> 16) & 0xFF;
        baBuffer[nCounterOffset + 3] = (nRound >> 24) & 0xFF;
        // Bytes 4-7 remain zero

        // Hash: Salt + Password + Counter
        XSha256Decoder::update(&sha, (const quint8 *)baBuffer.constData(), baBuffer.size());
    }

    // Finalize and get the key
    XSha256Decoder::final(&sha, pKey);
}

bool XAESDecoder::decrypt(XBinary::DATAPROCESS_STATE *pDecryptState, const QByteArray &baProperties, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecryptState || !pDecryptState->pDeviceInput || !pDecryptState->pDeviceOutput) {
        qWarning() << "[XAESDecoder] Invalid decrypt state";
        return false;
    }

    if (sPassword.isEmpty()) {
        qWarning() << "[XAESDecoder] Password is required for AES decryption";
        return false;
    }

    // Parse properties: FirstByte + Salt (0-16 bytes) + IV (16 bytes)
    // FirstByte format (from 7-Zip source):
    //   Bits 0-5: NumCyclesPower (0-63, typically 19 for 524288 iterations)
    //   Bit 6: IV flag
    //   Bit 7: Salt flag
    if (baProperties.size() < 1) {
        qWarning() << "[XAESDecoder] Invalid properties size:" << baProperties.size();
        return false;
    }

    quint8 nFirstByte = (quint8)baProperties[0];
    quint8 nNumCyclesPower = nFirstByte & 0x3F;  // Bits 0-5

    quint8 nSaltSize = 0;
    quint8 nIVSizeInProps = 0;

    if ((nFirstByte & 0xC0) == 0) {
        // OLD FORMAT: No salt, no IV in properties
        nSaltSize = 0;
        nIVSizeInProps = 0;
    } else {
        // NEW FORMAT (7-Zip 9.20+): Properties contain Salt and/or IV
        // Format: [FirstByte][SecondByte][Salt][IV]
        if (baProperties.size() < 2) {
            qWarning() << "[XAESDecoder] New format properties too small:" << baProperties.size();
            return false;
        }

        quint8 nSecondByte = (quint8)baProperties[1];

        // CRITICAL: 7-Zip SDK decoder formula (from 7zAes.cpp):
        // const unsigned saltSize = ((b0 >> 7) & 1) + (b1 >> 4);
        // const unsigned ivSize   = ((b0 >> 6) & 1) + (b1 & 0x0F);
        nSaltSize = ((nFirstByte >> 7) & 1) + (nSecondByte >> 4);
        nIVSizeInProps = ((nFirstByte >> 6) & 1) + (nSecondByte & 0x0F);

        qint32 nExpectedSize = 2 + nSaltSize + nIVSizeInProps;
        if (baProperties.size() < nExpectedSize) {
            qWarning() << "[XAESDecoder] New format size mismatch: got" << baProperties.size() << "expected at least" << nExpectedSize;
            return false;
        }
    }

    // Extract salt from properties
    qint32 nSaltOffset = ((nFirstByte & 0xC0) == 0) ? 1 : 2;
    QByteArray baSalt;
    if (nSaltSize > 0) {
        baSalt = baProperties.mid(nSaltOffset, nSaltSize);
    }

    // Extract IV
    QByteArray baIV;
    if (nIVSizeInProps > 0) {
        // NEW FORMAT: IV is in properties after salt
        qint32 nIVOffset = nSaltOffset + nSaltSize;
        baIV = baProperties.mid(nIVOffset, nIVSizeInProps);
    } else {
        // OLD FORMAT or SOLID: check if IV was appended to properties
        qint32 nExpectedWithIV = nSaltOffset + nSaltSize + 16;
        if (baProperties.size() >= nExpectedWithIV) {
            baIV = baProperties.mid(nSaltOffset + nSaltSize, 16);
        } else {
            // IV in stream
            baIV.resize(16);
            qint64 nIvRead = pDecryptState->pDeviceInput->read(baIV.data(), 16);
            if (nIvRead != 16) {
                qWarning() << "[XAESDecoder] Failed to read IV from stream: got" << nIvRead << "bytes";
                return false;
            }
            pDecryptState->nCountInput += 16;
        }
    }

    // Derive key from password
    quint8 aKey[32];  // AES-256 key
    deriveKey(sPassword, baSalt, nNumCyclesPower, aKey);

    // Read all remaining encrypted data
    qint64 nTotalEncrypted = pDecryptState->nInputLimit - pDecryptState->nCountInput;

    if (nTotalEncrypted <= 0 || (nTotalEncrypted % AES_BLOCK_SIZE) != 0) {
        qWarning() << "[XAESDecoder] Invalid encrypted data size:" << nTotalEncrypted;
        memset(aKey, 0, sizeof(aKey));
        return false;
    }

    QByteArray baEncrypted;
    baEncrypted.resize((qint32)nTotalEncrypted);
    qint32 nRead = XBinary::_readDevice(baEncrypted.data(), (qint32)nTotalEncrypted, pDecryptState);
    if (nRead != (qint32)nTotalEncrypted) {
        qWarning() << "[XAESDecoder] Failed to read encrypted data: got" << nRead << "expected" << nTotalEncrypted;
        memset(aKey, 0, sizeof(aKey));
        return false;
    }

    // AES-256-CBC decrypt
    QByteArray baKey32 = QByteArray((const char *)aKey, 32);
    QByteArray baDecrypted;
    baDecrypted.resize((qint32)nTotalEncrypted);

    if (!XAESDecoder::decryptAESCBC(baKey32, baIV, (const quint8 *)baEncrypted.constData(), (quint8 *)baDecrypted.data(), nTotalEncrypted)) {
        qWarning() << "[XAESDecoder] AES-CBC decryption failed";
        memset(aKey, 0, sizeof(aKey));
        return false;
    }

    // Write decrypted data to output
    if (!pDecryptState->pDeviceOutput) {
        qWarning() << "[XAESDecoder] No output device";
        memset(aKey, 0, sizeof(aKey));
        return false;
    }

    qint64 nTotalDecrypted = nTotalEncrypted;

    // 7z uses zero-padding (not PKCS#7): the expected output size is stored in FPART_PROP_UNCOMPRESSEDSIZE
    // Truncate the decrypted output to the expected size
    qint64 nExpectedSize = pDecryptState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    if (nExpectedSize > 0 && nExpectedSize < nTotalDecrypted) {
        baDecrypted.resize((qint32)nExpectedSize);
        nTotalDecrypted = nExpectedSize;
    } else {
        // Fallback: try PKCS#7 padding removal
        if (baDecrypted.size() > 0) {
            quint8 nPaddingLength = (quint8)baDecrypted.at(baDecrypted.size() - 1);

            if (nPaddingLength >= 1 && nPaddingLength <= AES_BLOCK_SIZE && nPaddingLength <= baDecrypted.size()) {
                bool bValidPadding = true;
                qint32 nPaddingStart = baDecrypted.size() - nPaddingLength;
                for (qint32 i = nPaddingStart; i < baDecrypted.size() && bValidPadding; i++) {
                    if ((quint8)baDecrypted.at(i) != nPaddingLength) {
                        bValidPadding = false;
                    }
                }

                if (bValidPadding) {
                    baDecrypted.resize(nPaddingStart);
                    nTotalDecrypted = nPaddingStart;
                } else {
                    qWarning() << "[XAESDecoder] Invalid PKCS#7 padding byte pattern, keeping all data";
                }
            }
        }
    }

    // Write to output device (works with any QIODevice: QBuffer, QFile, etc.)
    if (nTotalDecrypted > 0) {
        QBuffer *pBuffer = qobject_cast<QBuffer *>(pDecryptState->pDeviceOutput);
        if (pBuffer) {
            // For QBuffer: replace the buffer directly for efficiency
            pBuffer->buffer() = baDecrypted;
        } else {
            // For other devices (QFile, etc.): write directly
            qint64 nWritten = pDecryptState->pDeviceOutput->write(baDecrypted.constData(), nTotalDecrypted);
            if (nWritten != nTotalDecrypted) {
                qWarning() << "[XAESDecoder] Write error: wrote" << nWritten << "of" << nTotalDecrypted << "bytes";
                memset(aKey, 0, sizeof(aKey));
                return false;
            }
        }
        pDecryptState->nCountOutput += nTotalDecrypted;
    }

    // Zero out sensitive data
    memset(aKey, 0, sizeof(aKey));

    return nTotalDecrypted > 0;
}

//------------------------------------------------------------------------------
// ZIP AES (WinZip AES-256-CTR with PBKDF2-HMAC-SHA1)
//------------------------------------------------------------------------------

static const qint32 N_BUFFER_SIZE = 65536;
static const qint32 N_PBKDF2_ITERATIONS = 1000;
static const qint32 N_PASSWORD_VERIFY_SIZE = 2;
static const qint32 N_HMAC_SIZE = 10;
static const qint32 N_AES_BLOCK_SIZE = 16;

static QByteArray custom_hmac_sha1(const QByteArray &baKey, const QByteArray &baMessage)
{
    const qint32 BLOCK_SIZE = 64;
    const quint8 IPAD = 0x36;
    const quint8 OPAD = 0x5c;

    QByteArray baKeyPadded;
    if (baKey.size() > BLOCK_SIZE) {
        baKeyPadded = QCryptographicHash::hash(baKey, QCryptographicHash::Sha1);
    } else {
        baKeyPadded = baKey;
    }
    while (baKeyPadded.size() < BLOCK_SIZE) {
        baKeyPadded.append((char)0);
    }

    QByteArray baInnerKey(BLOCK_SIZE, 0);
    QByteArray baOuterKey(BLOCK_SIZE, 0);
    for (qint32 i = 0; i < BLOCK_SIZE; i++) {
        baInnerKey[i] = baKeyPadded[i] ^ IPAD;
        baOuterKey[i] = baKeyPadded[i] ^ OPAD;
    }

    QCryptographicHash innerHash(QCryptographicHash::Sha1);
    innerHash.addData(baInnerKey);
    innerHash.addData(baMessage);
    QByteArray baInnerResult = innerHash.result();

    QCryptographicHash outerHash(QCryptographicHash::Sha1);
    outerHash.addData(baOuterKey);
    outerHash.addData(baInnerResult);
    return outerHash.result();
}

bool XAESDecoder::decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QString &sPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    return decrypt(pDecompressState, sPassword.toLatin1(), cryptoMethod, pPdStruct);
}

bool XAESDecoder::decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput && !baPassword.isEmpty()) {
        pDecompressState->bReadError = false;
        pDecompressState->bWriteError = false;
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;

        qint32 nKeySize = 0;
        qint32 nSaltSize = 0;

        if (cryptoMethod == XBinary::HANDLE_METHOD_ZIP_AES256) {
            nKeySize = 32;
            nSaltSize = 16;
        } else {
            return false;
        }

        char bufferSalt[16];
        qint32 nSaltRead = XBinary::_readDevice(bufferSalt, nSaltSize, pDecompressState);
        if (nSaltRead != nSaltSize) {
            pDecompressState->bReadError = true;
            return false;
        }
        QByteArray baSalt(bufferSalt, nSaltSize);

        char bufferPasswordVerify[N_PASSWORD_VERIFY_SIZE];
        qint32 nPasswordVerifyRead = XBinary::_readDevice(bufferPasswordVerify, N_PASSWORD_VERIFY_SIZE, pDecompressState);
        if (nPasswordVerifyRead != N_PASSWORD_VERIFY_SIZE) {
            pDecompressState->bReadError = true;
            return false;
        }
        QByteArray baPasswordVerifyExpected(bufferPasswordVerify, N_PASSWORD_VERIFY_SIZE);

        QByteArray baAESKey;
        QByteArray baHMACKey;
        QByteArray baPasswordVerifyKey;
        if (!deriveKeys(baPassword, baSalt, nKeySize, baAESKey, baPasswordVerifyKey, baHMACKey, pPdStruct)) {
            return false;
        }

        if (baPasswordVerifyKey != baPasswordVerifyExpected) {
            return false;
        }

        qint64 nEncryptedDataSize = pDecompressState->nInputLimit - nSaltSize - N_PASSWORD_VERIFY_SIZE - N_HMAC_SIZE;
        if (nEncryptedDataSize <= 0) {
            return false;
        }

        QByteArray baEncryptedData;
        baEncryptedData.resize((qint32)nEncryptedDataSize);
        qint64 nReadTotal = 0;
        while (nReadTotal < nEncryptedDataSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint32 nToRead = qMin((qint32)(nEncryptedDataSize - nReadTotal), N_BUFFER_SIZE);
            qint32 nRead = XBinary::_readDevice(baEncryptedData.data() + nReadTotal, nToRead, pDecompressState);
            if (nRead <= 0) {
                pDecompressState->bReadError = true;
                return false;
            }
            nReadTotal += nRead;
        }

        char bufferHmac[N_HMAC_SIZE];
        qint32 nHmacRead = XBinary::_readDevice(bufferHmac, N_HMAC_SIZE, pDecompressState);
        if (nHmacRead != N_HMAC_SIZE) {
            pDecompressState->bReadError = true;
            return false;
        }
        QByteArray baExpectedHmac(bufferHmac, N_HMAC_SIZE);

        QByteArray baComputedHmac = custom_hmac_sha1(baHMACKey, baEncryptedData);
        if (baComputedHmac.left(N_HMAC_SIZE) != baExpectedHmac) {
            return false;
        }

        QByteArray baDecryptedData;
        baDecryptedData.resize((qint32)nEncryptedDataSize);
        QByteArray baNonce;
        bResult = decryptAESCTR(baAESKey, baNonce, baEncryptedData.constData(), baDecryptedData.data(), nEncryptedDataSize, pPdStruct);

        if (bResult) {
            XBinary::_writeDevice(baDecryptedData.data(), (qint32)nEncryptedDataSize, pDecompressState);
        }

        bResult = !pDecompressState->bReadError && !pDecompressState->bWriteError;
    }

    return bResult;
}

void XAESDecoder::pbkdf2(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nIterations, qint32 nKeyLength, QByteArray &baResult)
{
    baResult.clear();

    qint32 nHashLength = 20;
    qint32 nBlocks = (nKeyLength + nHashLength - 1) / nHashLength;

    for (qint32 nBlock = 1; nBlock <= nBlocks; nBlock++) {
        QByteArray baBlockIndex(4, 0);
        baBlockIndex[0] = (char)((nBlock >> 24) & 0xFF);
        baBlockIndex[1] = (char)((nBlock >> 16) & 0xFF);
        baBlockIndex[2] = (char)((nBlock >> 8) & 0xFF);
        baBlockIndex[3] = (char)(nBlock & 0xFF);

        QByteArray baU = custom_hmac_sha1(baPassword, baSalt + baBlockIndex);
        QByteArray baT = baU;

        for (qint32 i = 1; i < nIterations; i++) {
            baU = custom_hmac_sha1(baPassword, baU);
            for (qint32 j = 0; j < baT.size(); j++) {
                baT.data()[j] ^= baU.at(j);
            }
        }

        baResult.append(baT);
    }

    baResult.resize(nKeyLength);
}

bool XAESDecoder::deriveKeys(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nKeySize, QByteArray &baAESKey, QByteArray &baPasswordVerify,
                             QByteArray &baHMACKey, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint32 nTotalKeySize = nKeySize + N_PASSWORD_VERIFY_SIZE + nKeySize;

    QByteArray baDerivedKeys;
    pbkdf2(baPassword, baSalt, N_PBKDF2_ITERATIONS, nTotalKeySize, baDerivedKeys);

    if (baDerivedKeys.size() != nTotalKeySize) {
        return false;
    }

    baAESKey = baDerivedKeys.left(nKeySize);
    baHMACKey = baDerivedKeys.mid(nKeySize, nKeySize);
    baPasswordVerify = baDerivedKeys.right(N_PASSWORD_VERIFY_SIZE);

    return true;
}

//------------------------------------------------------------------------------
// Custom AES Key Expansion
//------------------------------------------------------------------------------

static const quint8 s_aes_sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4,
    0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3,
    0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3,
    0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c,
    0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5,
    0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e,
    0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

static const quint32 s_aes_rcon[10] = {0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x0000001b, 0x00000036};

static quint32 SubWord(quint32 nWord)
{
    return ((quint32)s_aes_sbox[(nWord >> 0) & 0xFF] << 0) | ((quint32)s_aes_sbox[(nWord >> 8) & 0xFF] << 8) | ((quint32)s_aes_sbox[(nWord >> 16) & 0xFF] << 16) |
           ((quint32)s_aes_sbox[(nWord >> 24) & 0xFF] << 24);
}

static quint32 RotWord(quint32 nWord)
{
    return (nWord >> 8) | (nWord << 24);
}

qint32 XAESDecoder::custom_aes_set_encrypt_key(const quint8 *pUserKey, qint32 nBits, CUSTOM_AES_KEY *pKey)
{
    if (!pUserKey || !pKey) {
        return -1;
    }

    qint32 nKeyWords = 0;
    qint32 nRounds = 0;

    switch (nBits) {
        case 128:
            nKeyWords = 4;
            nRounds = 10;
            break;
        case 192:
            nKeyWords = 6;
            nRounds = 12;
            break;
        case 256:
            nKeyWords = 8;
            nRounds = 14;
            break;
        default: return -2;
    }

    pKey->rounds = nRounds;

    for (qint32 i = 0; i < nKeyWords; i++) {
        pKey->rd_key[i] =
            ((quint32)pUserKey[4 * i + 0] << 0) | ((quint32)pUserKey[4 * i + 1] << 8) | ((quint32)pUserKey[4 * i + 2] << 16) | ((quint32)pUserKey[4 * i + 3] << 24);
    }

    qint32 nTotalWords = 4 * (nRounds + 1);
    for (qint32 i = nKeyWords; i < nTotalWords; i++) {
        quint32 nTemp = pKey->rd_key[i - 1];

        if (i % nKeyWords == 0) {
            nTemp = SubWord(RotWord(nTemp)) ^ s_aes_rcon[i / nKeyWords - 1];
        } else if (nKeyWords > 6 && i % nKeyWords == 4) {
            nTemp = SubWord(nTemp);
        }

        pKey->rd_key[i] = pKey->rd_key[i - nKeyWords] ^ nTemp;
    }

    return 0;
}

//------------------------------------------------------------------------------
// Custom AES Encryption (forward cipher)
//------------------------------------------------------------------------------

static const quint8 s_aes_inv_sbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e,
    0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, 0x08, 0x2e, 0xa1, 0x66,
    0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65,
    0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
    0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91,
    0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
    0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2,
    0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb,
    0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d};

static quint8 gf_mul(quint8 a, quint8 b)
{
    quint8 p = 0;
    quint8 hi_bit_set;
    for (qint32 i = 0; i < 8; i++) {
        if (b & 1) {
            p ^= a;
        }
        hi_bit_set = (a & 0x80);
        a <<= 1;
        if (hi_bit_set) {
            a ^= 0x1b;
        }
        b >>= 1;
    }
    return p;
}

static void SubBytes(quint8 *pState)
{
    for (qint32 i = 0; i < 16; i++) {
        pState[i] = s_aes_sbox[pState[i]];
    }
}

static void ShiftRows(quint8 *pState)
{
    quint8 temp;
    temp = pState[1];
    pState[1] = pState[5];
    pState[5] = pState[9];
    pState[9] = pState[13];
    pState[13] = temp;
    temp = pState[2];
    pState[2] = pState[10];
    pState[10] = temp;
    temp = pState[6];
    pState[6] = pState[14];
    pState[14] = temp;
    temp = pState[15];
    pState[15] = pState[11];
    pState[11] = pState[7];
    pState[7] = pState[3];
    pState[3] = temp;
}

static void MixColumns(quint8 *pState)
{
    quint8 temp[4];
    for (qint32 i = 0; i < 4; i++) {
        temp[0] = pState[i * 4 + 0];
        temp[1] = pState[i * 4 + 1];
        temp[2] = pState[i * 4 + 2];
        temp[3] = pState[i * 4 + 3];
        pState[i * 4 + 0] = gf_mul(temp[0], 2) ^ gf_mul(temp[1], 3) ^ temp[2] ^ temp[3];
        pState[i * 4 + 1] = temp[0] ^ gf_mul(temp[1], 2) ^ gf_mul(temp[2], 3) ^ temp[3];
        pState[i * 4 + 2] = temp[0] ^ temp[1] ^ gf_mul(temp[2], 2) ^ gf_mul(temp[3], 3);
        pState[i * 4 + 3] = gf_mul(temp[0], 3) ^ temp[1] ^ temp[2] ^ gf_mul(temp[3], 2);
    }
}

static void AddRoundKey(quint8 *pState, const quint32 *pRoundKey)
{
    for (qint32 i = 0; i < 4; i++) {
        quint32 nKey = pRoundKey[i];
        pState[i * 4 + 0] ^= (quint8)(nKey >> 0);
        pState[i * 4 + 1] ^= (quint8)(nKey >> 8);
        pState[i * 4 + 2] ^= (quint8)(nKey >> 16);
        pState[i * 4 + 3] ^= (quint8)(nKey >> 24);
    }
}

void XAESDecoder::custom_aes_encrypt(const quint8 *pInput, quint8 *pOutput, const CUSTOM_AES_KEY *pKey)
{
    if (!pInput || !pOutput || !pKey) {
        return;
    }

    quint8 state[16];
    for (qint32 i = 0; i < 16; i++) {
        state[i] = pInput[i];
    }

    AddRoundKey(state, pKey->rd_key);

    for (qint32 nRound = 1; nRound < pKey->rounds; nRound++) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, pKey->rd_key + (nRound * 4));
    }

    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, pKey->rd_key + (pKey->rounds * 4));

    for (qint32 i = 0; i < 16; i++) {
        pOutput[i] = state[i];
    }
}

//------------------------------------------------------------------------------
// AES Inverse Cipher (for CBC decryption)
//------------------------------------------------------------------------------

static void InvSubBytes(quint8 *pState)
{
    for (qint32 i = 0; i < 16; i++) {
        pState[i] = s_aes_inv_sbox[pState[i]];
    }
}

static void InvShiftRows(quint8 *pState)
{
    quint8 temp;
    temp = pState[13];
    pState[13] = pState[9];
    pState[9] = pState[5];
    pState[5] = pState[1];
    pState[1] = temp;
    temp = pState[2];
    pState[2] = pState[10];
    pState[10] = temp;
    temp = pState[6];
    pState[6] = pState[14];
    pState[14] = temp;
    temp = pState[3];
    pState[3] = pState[7];
    pState[7] = pState[11];
    pState[11] = pState[15];
    pState[15] = temp;
}

static void InvMixColumns(quint8 *pState)
{
    quint8 a0, a1, a2, a3;
    for (qint32 i = 0; i < 4; i++) {
        a0 = pState[i * 4 + 0];
        a1 = pState[i * 4 + 1];
        a2 = pState[i * 4 + 2];
        a3 = pState[i * 4 + 3];
        pState[i * 4 + 0] = gf_mul(a0, 0x0e) ^ gf_mul(a1, 0x0b) ^ gf_mul(a2, 0x0d) ^ gf_mul(a3, 0x09);
        pState[i * 4 + 1] = gf_mul(a0, 0x09) ^ gf_mul(a1, 0x0e) ^ gf_mul(a2, 0x0b) ^ gf_mul(a3, 0x0d);
        pState[i * 4 + 2] = gf_mul(a0, 0x0d) ^ gf_mul(a1, 0x09) ^ gf_mul(a2, 0x0e) ^ gf_mul(a3, 0x0b);
        pState[i * 4 + 3] = gf_mul(a0, 0x0b) ^ gf_mul(a1, 0x0d) ^ gf_mul(a2, 0x09) ^ gf_mul(a3, 0x0e);
    }
}

static void custom_aes_decrypt_block(const quint8 *pInput, quint8 *pOutput, const CUSTOM_AES_KEY *pKey)
{
    quint8 state[16];
    qint32 nRound;

    for (qint32 i = 0; i < 16; i++) {
        state[i] = pInput[i];
    }

    AddRoundKey(state, pKey->rd_key + pKey->rounds * 4);

    for (nRound = pKey->rounds - 1; nRound >= 1; nRound--) {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey(state, pKey->rd_key + nRound * 4);
        InvMixColumns(state);
    }

    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(state, pKey->rd_key);

    for (qint32 i = 0; i < 16; i++) {
        pOutput[i] = state[i];
    }
}

bool XAESDecoder::decryptAESCBC(const QByteArray &baKey, const QByteArray &baIV, const quint8 *pInputData, quint8 *pOutputData, qint64 nSize)
{
    if (!pInputData || !pOutputData || nSize <= 0 || (nSize % N_AES_BLOCK_SIZE) != 0) {
        return false;
    }
    if (baKey.isEmpty() || baIV.size() < N_AES_BLOCK_SIZE) {
        return false;
    }

    CUSTOM_AES_KEY customKey;
    if (custom_aes_set_encrypt_key((const quint8 *)baKey.constData(), baKey.size() * 8, &customKey) != 0) {
        return false;
    }

    quint8 prevBlock[N_AES_BLOCK_SIZE];
    memcpy(prevBlock, baIV.constData(), N_AES_BLOCK_SIZE);

    for (qint64 nOffset = 0; nOffset + N_AES_BLOCK_SIZE <= nSize; nOffset += N_AES_BLOCK_SIZE) {
        quint8 decryptedBlock[N_AES_BLOCK_SIZE];
        custom_aes_decrypt_block(pInputData + nOffset, decryptedBlock, &customKey);

        for (qint32 i = 0; i < N_AES_BLOCK_SIZE; i++) {
            pOutputData[nOffset + i] = decryptedBlock[i] ^ prevBlock[i];
        }

        memcpy(prevBlock, pInputData + nOffset, N_AES_BLOCK_SIZE);
    }

    return true;
}

bool XAESDecoder::decryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(baNonce)
    Q_UNUSED(pPdStruct)

    CUSTOM_AES_KEY customKey;
    if (custom_aes_set_encrypt_key((const quint8 *)baKey.constData(), baKey.size() * 8, &customKey) != 0) {
        return false;
    }

    unsigned char counter[N_AES_BLOCK_SIZE];
    memset(counter, 0, N_AES_BLOCK_SIZE);
    counter[0] = 1;

    qint64 nOffset = 0;
    unsigned char encryptedCounter[N_AES_BLOCK_SIZE];

    while (nOffset < nSize) {
        custom_aes_encrypt(counter, encryptedCounter, &customKey);

        qint64 nBlockSize = qMin((qint64)N_AES_BLOCK_SIZE, nSize - nOffset);
        for (qint64 i = 0; i < nBlockSize; i++) {
            pOutputData[nOffset + i] = pInputData[nOffset + i] ^ encryptedCounter[i];
        }

        for (qint32 j = 0; j < 8; j++) {
            if (++counter[j] != 0) {
                break;
            }
        }

        nOffset += nBlockSize;
    }

    return true;
}

bool XAESDecoder::encryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(baNonce)
    Q_UNUSED(pPdStruct)

    CUSTOM_AES_KEY customKey;
    if (custom_aes_set_encrypt_key((const quint8 *)baKey.constData(), baKey.size() * 8, &customKey) != 0) {
        return false;
    }

    unsigned char counter[N_AES_BLOCK_SIZE];
    memset(counter, 0, N_AES_BLOCK_SIZE);
    counter[0] = 1;

    qint64 nOffset = 0;
    unsigned char encryptedCounter[N_AES_BLOCK_SIZE];

    while (nOffset < nSize) {
        custom_aes_encrypt(counter, encryptedCounter, &customKey);

        qint64 nBlockSize = qMin((qint64)N_AES_BLOCK_SIZE, nSize - nOffset);
        for (qint64 i = 0; i < nBlockSize; i++) {
            pOutputData[nOffset + i] = pInputData[nOffset + i] ^ encryptedCounter[i];
        }

        for (qint32 j = 0; j < 8; j++) {
            if (++counter[j] != 0) {
                break;
            }
        }

        nOffset += nBlockSize;
    }

    return true;
}

bool XAESDecoder::encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QString &sPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    return encrypt(pCompressState, sPassword.toLatin1(), cryptoMethod, pPdStruct);
}

bool XAESDecoder::encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QByteArray &baPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pCompressState && pCompressState->pDeviceInput && pCompressState->pDeviceOutput && !baPassword.isEmpty()) {
        pCompressState->bReadError = false;
        pCompressState->bWriteError = false;
        pCompressState->nCountInput = 0;
        pCompressState->nCountOutput = 0;

        qint32 nKeySize = 0;
        qint32 nSaltSize = 0;

        if (cryptoMethod == XBinary::HANDLE_METHOD_ZIP_AES256) {
            nKeySize = 32;
            nSaltSize = 16;
        } else {
            return false;
        }

        QByteArray baSalt;
        baSalt.resize(nSaltSize);
        for (qint32 i = 0; i < nSaltSize; i++) {
            baSalt.data()[i] = (char)(rand() % 256);
        }

        QByteArray baAESKey;
        QByteArray baHMACKey;
        QByteArray baPasswordVerify;
        if (!deriveKeys(baPassword, baSalt, nKeySize, baAESKey, baPasswordVerify, baHMACKey, pPdStruct)) {
            return false;
        }

        XBinary::_writeDevice((char *)baSalt.constData(), nSaltSize, pCompressState);
        if (pCompressState->bWriteError) {
            return false;
        }

        XBinary::_writeDevice((char *)baPasswordVerify.constData(), N_PASSWORD_VERIFY_SIZE, pCompressState);
        if (pCompressState->bWriteError) {
            return false;
        }

        qint64 nPlainDataSize = pCompressState->nInputLimit;
        if (nPlainDataSize <= 0) {
            return false;
        }

        QByteArray baPlainData;
        baPlainData.resize((qint32)nPlainDataSize);
        qint64 nReadTotal = 0;
        while (nReadTotal < nPlainDataSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint32 nToRead = qMin((qint32)(nPlainDataSize - nReadTotal), N_BUFFER_SIZE);
            qint32 nRead = XBinary::_readDevice(baPlainData.data() + nReadTotal, nToRead, pCompressState);
            if (nRead <= 0) {
                pCompressState->bReadError = true;
                return false;
            }
            nReadTotal += nRead;
        }

        QByteArray baEncryptedData;
        baEncryptedData.resize((qint32)nPlainDataSize);
        QByteArray baNonce;
        bResult = encryptAESCTR(baAESKey, baNonce, baPlainData.constData(), baEncryptedData.data(), nPlainDataSize, pPdStruct);

        if (bResult) {
            QByteArray baComputedHmac = custom_hmac_sha1(baHMACKey, baEncryptedData);
            XBinary::_writeDevice((char *)baEncryptedData.constData(), (qint32)nPlainDataSize, pCompressState);
            XBinary::_writeDevice((char *)baComputedHmac.constData(), N_HMAC_SIZE, pCompressState);
        }

        bResult = !pCompressState->bReadError && !pCompressState->bWriteError;
    }

    return bResult;
}

// HMAC-SHA256: initialize inner and outer contexts with key (standard ipad/opad construction)
void XAESDecoder::hmacSha256SetKey(XSha256Decoder::Context *pInnerCtx, XSha256Decoder::Context *pOuterCtx, const quint8 *pKey, qint32 nKeySize)
{
    quint8 aKeyBlock[64];
    memset(aKeyBlock, 0, 64);

    if (nKeySize > 64) {
        // Hash the key if longer than block size
        XSha256Decoder::Context hashCtx;
        XSha256Decoder::init(&hashCtx);
        XSha256Decoder::update(&hashCtx, pKey, nKeySize);
        XSha256Decoder::final(&hashCtx, aKeyBlock);
    } else {
        memcpy(aKeyBlock, pKey, nKeySize);
    }

    quint8 aInnerPad[64];
    quint8 aOuterPad[64];

    for (qint32 i = 0; i < 64; i++) {
        aInnerPad[i] = aKeyBlock[i] ^ 0x36;
        aOuterPad[i] = aKeyBlock[i] ^ 0x5C;
    }

    XSha256Decoder::init(pInnerCtx);
    XSha256Decoder::update(pInnerCtx, aInnerPad, 64);

    XSha256Decoder::init(pOuterCtx);
    XSha256Decoder::update(pOuterCtx, aOuterPad, 64);

    memset(aKeyBlock, 0, 64);
    memset(aInnerPad, 0, 64);
    memset(aOuterPad, 0, 64);
}

// HMAC-SHA256: finalize - produces 32-byte digest
void XAESDecoder::hmacSha256Final(XSha256Decoder::Context *pInnerCtx, XSha256Decoder::Context *pOuterCtx, quint8 *pDigest)
{
    quint8 aInnerDigest[32];
    XSha256Decoder::final(pInnerCtx, aInnerDigest);
    XSha256Decoder::update(pOuterCtx, aInnerDigest, 32);
    XSha256Decoder::final(pOuterCtx, pDigest);
    memset(aInnerDigest, 0, 32);
}

// RAR5 key derivation: PBKDF2-HMAC-SHA256 producing 3 keys (AES key, hash key, password check)
// Follows the RAR5 spec: 2^nCnt main iterations, then 16 extra per additional key
void XAESDecoder::deriveRar5Keys(const QByteArray &baPassword, const quint8 *pSalt, quint8 nCnt, quint8 *pAesKey, quint8 *pHashKey, quint8 *pPswCheck)
{
    // Set up base HMAC context with password as key
    XSha256Decoder::Context baseInner, baseOuter;
    hmacSha256SetKey(&baseInner, &baseOuter, (const quint8 *)baPassword.constData(), baPassword.size());

    // First HMAC round: HMAC(password, salt || 0x00000001_BE)
    XSha256Decoder::Context ctxInner = baseInner;
    XSha256Decoder::Context ctxOuter = baseOuter;
    XSha256Decoder::update(&ctxInner, pSalt, 16);

    quint8 aCounter[4] = {0x00, 0x00, 0x00, 0x01};  // Big-endian 1
    XSha256Decoder::update(&ctxInner, aCounter, 4);

    quint8 aU[32];    // Current HMAC result
    quint8 aKey[32];  // XOR accumulator
    hmacSha256Final(&ctxInner, &ctxOuter, aU);
    memcpy(aKey, aU, 32);

    // Main iterations: 2^nCnt - 1 (already did 1 above)
    quint32 nIterations = ((quint32)1 << nCnt) - 1;

    // 3 keys: i=0 → AES key, i=1 → hashKey, i=2 → pswCheck
    for (qint32 i = 0; i < 3; i++) {
        for (; nIterations != 0; nIterations--) {
            // u = HMAC(password, u)
            ctxInner = baseInner;
            ctxOuter = baseOuter;
            XSha256Decoder::update(&ctxInner, aU, 32);
            hmacSha256Final(&ctxInner, &ctxOuter, aU);

            // key ^= u
            for (qint32 s = 0; s < 32; s++) {
                aKey[s] ^= aU[s];
            }
        }

        // Store derived key
        if (i == 0) {
            memcpy(pAesKey, aKey, 32);
        } else if (i == 1) {
            memcpy(pHashKey, aKey, 32);
        } else {
            memcpy(pPswCheck, aKey, 32);
        }

        // RAR uses 16 additional iterations for subsequent keys
        nIterations = 16;
    }

    memset(aU, 0, 32);
    memset(aKey, 0, 32);
}

QByteArray XAESDecoder::deriveRar5HeaderKey(const QString &sPassword, const QByteArray &baSalt, quint8 nKdfCount)
{
    if (sPassword.isEmpty() || baSalt.size() < 16) {
        return QByteArray();
    }

    QByteArray baPassword = sPassword.toUtf8();
    quint8 aAesKey[32], aHashKey[32], aPswCheck[32];
    deriveRar5Keys(baPassword, (const quint8 *)baSalt.constData(), nKdfCount, aAesKey, aHashKey, aPswCheck);

    QByteArray baResult((const char *)aAesKey, 32);

    memset(aAesKey, 0, 32);
    memset(aHashKey, 0, 32);
    memset(aPswCheck, 0, 32);

    return baResult;
}

// RAR5 AES-256-CBC decrypt
bool XAESDecoder::decryptRar5(XBinary::DATAPROCESS_STATE *pDecryptState, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pDecryptState || !pDecryptState->pDeviceInput || !pDecryptState->pDeviceOutput) {
        qWarning() << "[XAESDecoder::decryptRar5] Invalid state or devices";
        return false;
    }

    if (sPassword.isEmpty()) {
        qWarning() << "[XAESDecoder::decryptRar5] Password is required for RAR5 AES decryption";
        return false;
    }

    // Get crypto parameters from AESKEY property: [1 cnt][16 salt][16 IV][opt 12 pswCheck]
    QByteArray baAESKeyProp = pDecryptState->mapProperties.value(XBinary::FPART_PROP_AESKEY).toByteArray();

    if (baAESKeyProp.size() < 33) {  // 1 + 16 + 16 = 33 minimum
        qWarning() << "[XAESDecoder::decryptRar5] Invalid AESKEY property size:" << baAESKeyProp.size();
        return false;
    }

    quint8 nCnt = (quint8)baAESKeyProp.at(0);
    QByteArray baSalt = baAESKeyProp.mid(1, 16);
    QByteArray baIV = baAESKeyProp.mid(17, 16);

    // Convert password to UTF-8 (RAR5 uses UTF-8 encoding)
    QByteArray baPassword = sPassword.toUtf8();

    // Derive 3 keys via PBKDF2-HMAC-SHA256
    quint8 aAesKey[32];
    quint8 aHashKey[32];
    quint8 aPswCheck[32];
    deriveRar5Keys(baPassword, (const quint8 *)baSalt.constData(), nCnt, aAesKey, aHashKey, aPswCheck);

    // Optional password verification (if pswCheck present in AESKEY)
    if (baAESKeyProp.size() >= 45) {  // 33 + 12 = 45 (8 bytes pswCheck + 4 bytes checksum)
        // RAR5 password check: XOR-fold 32-byte pswCheck to 8 bytes
        quint8 aCheckCalced[8];
        for (qint32 i = 0; i < 4; i++) {
            quint32 nVal = 0;
            for (qint32 j = 0; j < 8; j++) {
                nVal ^= (quint32)aPswCheck[i * 4 + j + (j >= 4 ? 12 : 0)];
                // XOR pairs: check[0]^check[2]^check[4]^check[6] and check[1]^check[3]^check[5]^check[7]
            }
        }
        // Simplified: XOR 32 bytes down to 8 bytes
        for (qint32 i = 0; i < 8; i++) {
            aCheckCalced[i] = aPswCheck[i] ^ aPswCheck[i + 8] ^ aPswCheck[i + 16] ^ aPswCheck[i + 24];
        }

        const quint8 *pExpectedCheck = (const quint8 *)baAESKeyProp.constData() + 33;
        if (memcmp(aCheckCalced, pExpectedCheck, 8) != 0) {
            qWarning() << "[XAESDecoder::decryptRar5] Password check failed - wrong password";
            memset(aAesKey, 0, 32);
            memset(aHashKey, 0, 32);
            memset(aPswCheck, 0, 32);
            return false;
        }
    }

    // Read all encrypted data
    qint64 nTotalEncrypted = pDecryptState->nInputLimit - pDecryptState->nCountInput;

    if (nTotalEncrypted <= 0 || (nTotalEncrypted % AES_BLOCK_SIZE) != 0) {
        qWarning() << "[XAESDecoder::decryptRar5] Invalid encrypted data size:" << nTotalEncrypted;
        memset(aAesKey, 0, 32);
        memset(aHashKey, 0, 32);
        memset(aPswCheck, 0, 32);
        return false;
    }

    QByteArray baEncrypted;
    baEncrypted.resize((qint32)nTotalEncrypted);
    qint32 nRead = XBinary::_readDevice(baEncrypted.data(), (qint32)nTotalEncrypted, pDecryptState);
    if (nRead != (qint32)nTotalEncrypted) {
        qWarning() << "[XAESDecoder::decryptRar5] Failed to read encrypted data: got" << nRead << "expected" << nTotalEncrypted;
        memset(aAesKey, 0, 32);
        memset(aHashKey, 0, 32);
        memset(aPswCheck, 0, 32);
        return false;
    }

    // AES-256-CBC decryption
    QByteArray baKey32 = QByteArray((const char *)aAesKey, 32);
    QByteArray baDecrypted;
    baDecrypted.resize((qint32)nTotalEncrypted);

    if (!XAESDecoder::decryptAESCBC(baKey32, baIV, (const quint8 *)baEncrypted.constData(), (quint8 *)baDecrypted.data(), nTotalEncrypted)) {
        qWarning() << "[XAESDecoder::decryptRar5] AES-CBC decryption failed";
        memset(aAesKey, 0, 32);
        memset(aHashKey, 0, 32);
        memset(aPswCheck, 0, 32);
        return false;
    }

    // Write decrypted data - only the uncompressed size portion (RAR5 pads encrypted data to AES block boundary)
    qint64 nOutputSize = pDecryptState->mapProperties.value(XBinary::FPART_PROP_COMPRESSEDSIZE, nTotalEncrypted).toLongLong();
    // For encrypted RAR5, the actual compressed data may be smaller than the encrypted block
    // Use the full decrypted size since the decompressor will handle the actual data length
    qint64 nToWrite = nTotalEncrypted;

    pDecryptState->pDeviceOutput->seek(0);
    qint64 nWritten = pDecryptState->pDeviceOutput->write(baDecrypted.constData(), nToWrite);
    pDecryptState->nCountOutput = nWritten;

    // Clear sensitive data
    memset(aAesKey, 0, 32);
    memset(aHashKey, 0, 32);
    memset(aPswCheck, 0, 32);

    return (nWritten == nToWrite);
}
