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

#include "xzipaesdecoder.h"
#include <QCryptographicHash>
#include <cstring>

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

const qint32 N_BUFFER_SIZE = 65536;
const qint32 N_PBKDF2_ITERATIONS = 1000;
const qint32 N_PASSWORD_VERIFY_SIZE = 2;
const qint32 N_HMAC_SIZE = 10;
const qint32 N_AES_BLOCK_SIZE = 16;

//------------------------------------------------------------------------------
// Custom HMAC-SHA1 Implementation
//------------------------------------------------------------------------------

// Custom HMAC-SHA1: HMAC(key, message) = H((key XOR opad) || H((key XOR ipad) || message))
// where H is SHA-1, opad is 0x5c repeated, ipad is 0x36 repeated
QByteArray custom_hmac_sha1(const QByteArray &baKey, const QByteArray &baMessage)
{
    const qint32 BLOCK_SIZE = 64;  // SHA-1 block size is 64 bytes
    const quint8 IPAD = 0x36;
    const quint8 OPAD = 0x5c;

    // Prepare the key: if longer than block size, hash it; if shorter, pad with zeros
    QByteArray baKeyPadded;
    if (baKey.size() > BLOCK_SIZE) {
        baKeyPadded = QCryptographicHash::hash(baKey, QCryptographicHash::Sha1);
    } else {
        baKeyPadded = baKey;
    }

    // Pad key to block size with zeros
    while (baKeyPadded.size() < BLOCK_SIZE) {
        baKeyPadded.append((char)0);
    }

    // Create inner and outer padded keys
    QByteArray baInnerKey(BLOCK_SIZE, 0);
    QByteArray baOuterKey(BLOCK_SIZE, 0);

    for (qint32 i = 0; i < BLOCK_SIZE; i++) {
        baInnerKey[i] = baKeyPadded[i] ^ IPAD;
        baOuterKey[i] = baKeyPadded[i] ^ OPAD;
    }

    // Inner hash: H((key XOR ipad) || message)
    QCryptographicHash innerHash(QCryptographicHash::Sha1);
    innerHash.addData(baInnerKey);
    innerHash.addData(baMessage);
    QByteArray baInnerResult = innerHash.result();

    // Outer hash: H((key XOR opad) || inner_hash)
    QCryptographicHash outerHash(QCryptographicHash::Sha1);
    outerHash.addData(baOuterKey);
    outerHash.addData(baInnerResult);

    return outerHash.result();
}

XZipAESDecoder::XZipAESDecoder(QObject *pParent) : QObject(pParent)
{
    // Constructor - no initialization needed with OpenSSL
}

bool XZipAESDecoder::decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QString &sPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    return decrypt(pDecompressState, sPassword.toLatin1(), cryptoMethod, pPdStruct);
}

bool XZipAESDecoder::decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baPassword, XBinary::HANDLE_METHOD cryptoMethod,
                             XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput && !baPassword.isEmpty()) {
        // Initialize error states
        pDecompressState->bReadError = false;
        pDecompressState->bWriteError = false;
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;

        // Get AES key size based on crypto method
        qint32 nKeySize = 0;
        qint32 nSaltSize = 0;

        if (cryptoMethod == XBinary::HANDLE_METHOD_AES256) {
            nKeySize = 32;   // 256 bits
            nSaltSize = 16;  // Salt size for AES-256
        } else {
            // Unsupported method
            return false;
        }

        // Read salt
        char bufferSalt[16];
        qint32 nSaltRead = XBinary::_readDevice(bufferSalt, nSaltSize, pDecompressState);
        if (nSaltRead != nSaltSize) {
            pDecompressState->bReadError = true;
            return false;
        }
        QByteArray baSalt(bufferSalt, nSaltSize);

        // Read password verification value
        char bufferPasswordVerify[N_PASSWORD_VERIFY_SIZE];
        qint32 nPasswordVerifyRead = XBinary::_readDevice(bufferPasswordVerify, N_PASSWORD_VERIFY_SIZE, pDecompressState);
        if (nPasswordVerifyRead != N_PASSWORD_VERIFY_SIZE) {
            pDecompressState->bReadError = true;
            return false;
        }
        QByteArray baPasswordVerifyExpected(bufferPasswordVerify, N_PASSWORD_VERIFY_SIZE);

        // Derive keys from password
        QByteArray baAESKey;
        QByteArray baHMACKey;
        QByteArray baPasswordVerifyKey;
        if (!deriveKeys(baPassword, baSalt, nKeySize, baAESKey, baPasswordVerifyKey, baHMACKey, pPdStruct)) {
            return false;
        }

        // Verify password
        if (baPasswordVerifyKey != baPasswordVerifyExpected) {
            // Wrong password
            return false;
        }

        // Calculate encrypted data size (total size - salt - password verify - HMAC)
        qint64 nEncryptedDataSize = pDecompressState->nInputLimit - nSaltSize - N_PASSWORD_VERIFY_SIZE - N_HMAC_SIZE;
        if (nEncryptedDataSize <= 0) {
            return false;
        }

        // Read encrypted data
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

        // Read HMAC-SHA1 authentication code
        char bufferHmac[N_HMAC_SIZE];
        qint32 nHmacRead = XBinary::_readDevice(bufferHmac, N_HMAC_SIZE, pDecompressState);
        if (nHmacRead != N_HMAC_SIZE) {
            pDecompressState->bReadError = true;
            return false;
        }
        QByteArray baExpectedHmac(bufferHmac, N_HMAC_SIZE);

        // Verify HMAC before decryption
        QByteArray baComputedHmac = custom_hmac_sha1(baHMACKey, baEncryptedData);
        if (baComputedHmac.left(N_HMAC_SIZE) != baExpectedHmac) {
            // HMAC verification failed
            return false;
        }

        // Decrypt data using AES-CTR mode
        QByteArray baDecryptedData;
        baDecryptedData.resize((qint32)nEncryptedDataSize);
        QByteArray baNonce;  // Not used in WinZip AES
        bResult = decryptAESCTR(baAESKey, baNonce, baEncryptedData.constData(), baDecryptedData.data(), nEncryptedDataSize, pPdStruct);

        if (bResult) {
            // Write decrypted data to output
            XBinary::_writeDevice(baDecryptedData.data(), (qint32)nEncryptedDataSize, pDecompressState);
        }

        bResult = !pDecompressState->bReadError && !pDecompressState->bWriteError;
    }

    return bResult;
}

void XZipAESDecoder::pbkdf2(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nIterations, qint32 nKeyLength, QByteArray &baResult)
{
    // PBKDF2 implementation using HMAC-SHA1
    baResult.clear();

    qint32 nHashLength = 20;  // SHA1 produces 20 bytes
    qint32 nBlocks = (nKeyLength + nHashLength - 1) / nHashLength;

    for (qint32 nBlock = 1; nBlock <= nBlocks; nBlock++) {
        // Prepare block index (big-endian)
        QByteArray baBlockIndex(4, 0);
        baBlockIndex[0] = (char)((nBlock >> 24) & 0xFF);
        baBlockIndex[1] = (char)((nBlock >> 16) & 0xFF);
        baBlockIndex[2] = (char)((nBlock >> 8) & 0xFF);
        baBlockIndex[3] = (char)(nBlock & 0xFF);

        // U1 = HMAC(password, salt || block_index)
        QByteArray baU = custom_hmac_sha1(baPassword, baSalt + baBlockIndex);
        QByteArray baT = baU;

        // Iterate: U2 = HMAC(password, U1), U3 = HMAC(password, U2), ...
        for (qint32 i = 1; i < nIterations; i++) {
            baU = custom_hmac_sha1(baPassword, baU);

            // XOR with accumulated result
            for (qint32 j = 0; j < baT.size(); j++) {
                baT.data()[j] ^= baU.at(j);
            }
        }

        baResult.append(baT);
    }

    // Truncate to desired length
    baResult.resize(nKeyLength);
}

bool XZipAESDecoder::deriveKeys(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nKeySize, QByteArray &baAESKey, QByteArray &baPasswordVerify,
                                QByteArray &baHMACKey, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // Total key material = AES key + password verify (2 bytes) + HMAC key
    // HMAC key size must match AES key size for WinZip AES
    qint32 nTotalKeySize = nKeySize + N_PASSWORD_VERIFY_SIZE + nKeySize;

    // Derive keys using PBKDF2-HMAC-SHA1
    QByteArray baDerivedKeys;
    pbkdf2(baPassword, baSalt, N_PBKDF2_ITERATIONS, nTotalKeySize, baDerivedKeys);

    if (baDerivedKeys.size() != nTotalKeySize) {
        return false;
    }

    // Split derived key material
    // WinZip AES format: AES_key + HMAC_key + PWD_VERIFY (last 2 bytes)
    baAESKey = baDerivedKeys.left(nKeySize);
    baHMACKey = baDerivedKeys.mid(nKeySize, nKeySize);
    baPasswordVerify = baDerivedKeys.right(N_PASSWORD_VERIFY_SIZE);

    return true;
}

//------------------------------------------------------------------------------
// Custom AES Key Expansion Implementation
//------------------------------------------------------------------------------

// AES S-box (forward substitution box)
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

// AES round constants (for little-endian: XOR with LSB)
static const quint32 s_aes_rcon[10] = {0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x0000001b, 0x00000036};

// SubWord transformation (works on little-endian words)
static quint32 SubWord(quint32 nWord)
{
    return ((quint32)s_aes_sbox[(nWord >> 0) & 0xFF] << 0) | ((quint32)s_aes_sbox[(nWord >> 8) & 0xFF] << 8) | ((quint32)s_aes_sbox[(nWord >> 16) & 0xFF] << 16) |
           ((quint32)s_aes_sbox[(nWord >> 24) & 0xFF] << 24);
}

// RotWord transformation (rotate left by one byte for little-endian)
static quint32 RotWord(quint32 nWord)
{
    return (nWord >> 8) | (nWord << 24);
}

// Custom AES key expansion
qint32 XZipAESDecoder::custom_aes_set_encrypt_key(const quint8 *pUserKey, qint32 nBits, CUSTOM_AES_KEY *pKey)
{
    if (!pUserKey || !pKey) {
        return -1;
    }

    qint32 nKeyWords = 0;
    qint32 nRounds = 0;

    // Determine key size and number of rounds
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
        default: return -2;  // Invalid key size
    }

    pKey->rounds = nRounds;

    // Copy user key into first words of round key array
    // ENDIANNESS NOTE: Using little-endian byte order for word formation
    // This matches x86/x64 architecture and ensures compatibility
    for (qint32 i = 0; i < nKeyWords; i++) {
        pKey->rd_key[i] =
            ((quint32)pUserKey[4 * i + 0] << 0) | ((quint32)pUserKey[4 * i + 1] << 8) | ((quint32)pUserKey[4 * i + 2] << 16) | ((quint32)pUserKey[4 * i + 3] << 24);
    }

    // Key expansion
    qint32 nTotalWords = 4 * (nRounds + 1);
    for (qint32 i = nKeyWords; i < nTotalWords; i++) {
        quint32 nTemp = pKey->rd_key[i - 1];

        if (i % nKeyWords == 0) {
            // RotWord + SubWord + Rcon
            nTemp = SubWord(RotWord(nTemp)) ^ s_aes_rcon[i / nKeyWords - 1];
        } else if (nKeyWords > 6 && i % nKeyWords == 4) {
            // Additional SubWord for AES-256
            nTemp = SubWord(nTemp);
        }

        pKey->rd_key[i] = pKey->rd_key[i - nKeyWords] ^ nTemp;
    }

    return 0;
}

//------------------------------------------------------------------------------
// Custom AES Encryption Implementation
//------------------------------------------------------------------------------

// Inverse S-box for decryption (not needed for CTR mode, but kept for completeness)
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

// Galois Field multiplication (for MixColumns)
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
            a ^= 0x1b;  // x^8 + x^4 + x^3 + x + 1
        }
        b >>= 1;
    }

    return p;
}

// SubBytes transformation
static void SubBytes(quint8 *pState)
{
    for (qint32 i = 0; i < 16; i++) {
        pState[i] = s_aes_sbox[pState[i]];
    }
}

// ShiftRows transformation
static void ShiftRows(quint8 *pState)
{
    quint8 temp;

    // Row 1: shift left by 1
    temp = pState[1];
    pState[1] = pState[5];
    pState[5] = pState[9];
    pState[9] = pState[13];
    pState[13] = temp;

    // Row 2: shift left by 2
    temp = pState[2];
    pState[2] = pState[10];
    pState[10] = temp;
    temp = pState[6];
    pState[6] = pState[14];
    pState[14] = temp;

    // Row 3: shift left by 3
    temp = pState[15];
    pState[15] = pState[11];
    pState[11] = pState[7];
    pState[7] = pState[3];
    pState[3] = temp;
}

// MixColumns transformation
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

// AddRoundKey transformation
static void AddRoundKey(quint8 *pState, const quint32 *pRoundKey)
{
    // ENDIANNESS NOTE: Extract bytes from little-endian round keys and XOR with state
    // The round keys were created in little-endian format during key expansion
    for (qint32 i = 0; i < 4; i++) {
        quint32 nKey = pRoundKey[i];
        pState[i * 4 + 0] ^= (quint8)(nKey >> 0);
        pState[i * 4 + 1] ^= (quint8)(nKey >> 8);
        pState[i * 4 + 2] ^= (quint8)(nKey >> 16);
        pState[i * 4 + 3] ^= (quint8)(nKey >> 24);
    }
}

// Custom AES block encryption
void XZipAESDecoder::custom_aes_encrypt(const quint8 *pInput, quint8 *pOutput, const CUSTOM_AES_KEY *pKey)
{
    if (!pInput || !pOutput || !pKey) {
        return;
    }

    // Copy input to state (column-major order for AES)
    quint8 state[16];
    for (qint32 i = 0; i < 16; i++) {
        state[i] = pInput[i];
    }

    // Initial round key addition
    AddRoundKey(state, pKey->rd_key);

    // Main rounds
    for (qint32 nRound = 1; nRound < pKey->rounds; nRound++) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, pKey->rd_key + (nRound * 4));
    }

    // Final round (no MixColumns)
    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, pKey->rd_key + (pKey->rounds * 4));

    // Copy state to output
    for (qint32 i = 0; i < 16; i++) {
        pOutput[i] = state[i];
    }
}

bool XZipAESDecoder::decryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize,
                                   XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(baNonce)  // Nonce is not used in WinZip AES; counter starts at 1
    Q_UNUSED(pPdStruct)

    // Setup AES key for encryption (CTR mode uses encryption even for decryption)
    CUSTOM_AES_KEY customKey;
    qint32 nKeyBits = baKey.size() * 8;  // Convert bytes to bits

    // Use custom key expansion
    qint32 nResult = custom_aes_set_encrypt_key((const quint8 *)baKey.constData(), nKeyBits, &customKey);
    if (nResult != 0) {
        return false;
    }

    // WinZip AES-CTR: counter starts at 1 (little-endian)
    unsigned char counter[N_AES_BLOCK_SIZE];
    memset(counter, 0, N_AES_BLOCK_SIZE);
    counter[0] = 1;  // Little-endian counter = 1

    // Process data in 16-byte blocks
    qint64 nOffset = 0;
    unsigned char encryptedCounter[N_AES_BLOCK_SIZE];

    while (nOffset < nSize) {
        // Encrypt the counter to get the keystream using custom AES implementation
        custom_aes_encrypt(counter, encryptedCounter, &customKey);

        // XOR encrypted counter with input data to get output
        qint64 nBlockSize = qMin((qint64)N_AES_BLOCK_SIZE, nSize - nOffset);
        for (qint64 i = 0; i < nBlockSize; i++) {
            pOutputData[nOffset + i] = pInputData[nOffset + i] ^ encryptedCounter[i];
        }

        // Increment counter (little-endian 64-bit)
        for (qint32 j = 0; j < 8; j++) {
            if (++counter[j] != 0) {
                break;  // No carry needed
            }
        }

        nOffset += nBlockSize;
    }

    return true;
}

bool XZipAESDecoder::encryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize,
                                   XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(baNonce)  // Nonce is not used in WinZip AES; counter starts at 1
    Q_UNUSED(pPdStruct)

    // Setup AES key for encryption (CTR mode uses encryption for both encryption and decryption)
    CUSTOM_AES_KEY customKey;
    qint32 nKeyBits = baKey.size() * 8;  // Convert bytes to bits

    // Use custom key expansion
    qint32 nResult = custom_aes_set_encrypt_key((const quint8 *)baKey.constData(), nKeyBits, &customKey);
    if (nResult != 0) {
        return false;
    }

    // WinZip AES-CTR: counter starts at 1 (little-endian)
    unsigned char counter[N_AES_BLOCK_SIZE];
    memset(counter, 0, N_AES_BLOCK_SIZE);
    counter[0] = 1;  // Little-endian counter = 1

    // Process data in 16-byte blocks
    qint64 nOffset = 0;
    unsigned char encryptedCounter[N_AES_BLOCK_SIZE];

    while (nOffset < nSize) {
        // Encrypt the counter to get the keystream using custom AES implementation
        custom_aes_encrypt(counter, encryptedCounter, &customKey);

        // XOR encrypted counter with input data to get output
        qint64 nBlockSize = qMin((qint64)N_AES_BLOCK_SIZE, nSize - nOffset);
        for (qint64 i = 0; i < nBlockSize; i++) {
            pOutputData[nOffset + i] = pInputData[nOffset + i] ^ encryptedCounter[i];
        }

        // Increment counter (little-endian 64-bit)
        for (qint32 j = 0; j < 8; j++) {
            if (++counter[j] != 0) {
                break;  // No carry needed
            }
        }

        nOffset += nBlockSize;
    }

    return true;
}

bool XZipAESDecoder::encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QString &sPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    return encrypt(pCompressState, sPassword.toLatin1(), cryptoMethod, pPdStruct);
}

bool XZipAESDecoder::encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QByteArray &baPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pCompressState && pCompressState->pDeviceInput && pCompressState->pDeviceOutput && !baPassword.isEmpty()) {
        // Initialize error states
        pCompressState->bReadError = false;
        pCompressState->bWriteError = false;
        pCompressState->nCountInput = 0;
        pCompressState->nCountOutput = 0;

        // Get AES key size based on crypto method
        qint32 nKeySize = 0;
        qint32 nSaltSize = 0;

        if (cryptoMethod == XBinary::HANDLE_METHOD_AES256) {
            nKeySize = 32;   // 256 bits
            nSaltSize = 16;  // Salt size for AES-256
        } else {
            // Unsupported method
            return false;
        }

        // Generate random salt
        QByteArray baSalt;
        baSalt.resize(nSaltSize);
        for (qint32 i = 0; i < nSaltSize; i++) {
            baSalt.data()[i] = (char)(rand() % 256);
        }

        // Derive keys from password
        QByteArray baAESKey;
        QByteArray baHMACKey;
        QByteArray baPasswordVerify;
        if (!deriveKeys(baPassword, baSalt, nKeySize, baAESKey, baPasswordVerify, baHMACKey, pPdStruct)) {
            return false;
        }

        // Write salt
        XBinary::_writeDevice((char *)baSalt.constData(), nSaltSize, pCompressState);
        if (pCompressState->bWriteError) {
            return false;
        }

        // Write password verification value
        XBinary::_writeDevice((char *)baPasswordVerify.constData(), N_PASSWORD_VERIFY_SIZE, pCompressState);
        if (pCompressState->bWriteError) {
            return false;
        }

        // Read input data
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

        // Encrypt data using AES-CTR mode
        QByteArray baEncryptedData;
        baEncryptedData.resize((qint32)nPlainDataSize);
        QByteArray baNonce;  // Not used in WinZip AES
        bResult = encryptAESCTR(baAESKey, baNonce, baPlainData.constData(), baEncryptedData.data(), nPlainDataSize, pPdStruct);

        if (bResult) {
            // Calculate HMAC
            QByteArray baComputedHmac = custom_hmac_sha1(baHMACKey, baEncryptedData);

            // Write encrypted data
            XBinary::_writeDevice((char *)baEncryptedData.constData(), (qint32)nPlainDataSize, pCompressState);

            // Write HMAC
            XBinary::_writeDevice((char *)baComputedHmac.constData(), N_HMAC_SIZE, pCompressState);
        }

        bResult = !pCompressState->bReadError && !pCompressState->bWriteError;
    }

    return bResult;
}
