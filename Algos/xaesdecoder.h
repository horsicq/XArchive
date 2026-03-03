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
#ifndef XAESDECODER_H
#define XAESDECODER_H

#include "xbinary.h"
#include "xsha256decoder.h"
#include <QCryptographicHash>

// AES constants
#define AES_BLOCK_SIZE 16

// Custom AES key structure
struct CUSTOM_AES_KEY {
    quint32 rd_key[60];  // Round keys (max 14 rounds * 4 = 56, plus padding)
    qint32 rounds;       // Number of rounds
};

class XAESDecoder : public QObject {
    Q_OBJECT

public:
    explicit XAESDecoder(QObject *parent = nullptr);

    // 7z AES-256-CBC decrypt (properties-based key derivation)
    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecryptState, const QByteArray &baProperties, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // ZIP AES decrypt (AES-CTR, PBKDF2-HMAC-SHA1 key derivation)
    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QString &sPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // ZIP AES encrypt
    static bool encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QString &sPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QByteArray &baPassword, XBinary::HANDLE_METHOD cryptoMethod, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // AES-256-CBC decryption (used by 7z)
    static bool decryptAESCBC(const QByteArray &baKey, const QByteArray &baIV, const quint8 *pInputData, quint8 *pOutputData, qint64 nSize);

    // Custom AES block cipher
    static qint32 custom_aes_set_encrypt_key(const quint8 *pUserKey, qint32 nBits, CUSTOM_AES_KEY *pKey);
    static void custom_aes_encrypt(const quint8 *pInput, quint8 *pOutput, const CUSTOM_AES_KEY *pKey);

private:
    // 7z key derivation (SHA-256 iterated)
    static void deriveKey(const QString &sPassword, const QByteArray &baSalt, quint8 nNumCyclesPower, quint8 *pKey);

    // ZIP key derivation helpers (PBKDF2-HMAC-SHA1)
    static void pbkdf2(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nIterations, qint32 nKeyLength, QByteArray &baResult);
    static bool deriveKeys(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nKeySize, QByteArray &baAESKey, QByteArray &baPasswordVerify,
                           QByteArray &baHMACKey, XBinary::PDSTRUCT *pPdStruct);
    static bool decryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct);
    static bool encryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct);
};

#endif  // XAESDECODER_H
