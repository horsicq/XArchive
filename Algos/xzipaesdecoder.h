/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#ifndef XZIPAESDECODER_H
#define XZIPAESDECODER_H

#include "xbinary.h"
#include <QCryptographicHash>
#include <QDebug>

// Custom AES key structure (compatible with OpenSSL AES_KEY)
struct CUSTOM_AES_KEY {
    quint32 rd_key[60];  // Round keys (max 14 rounds * 4 = 56, plus padding)
    qint32 rounds;       // Number of rounds
};

class XZipAESDecoder : public QObject {
    Q_OBJECT

public:
    explicit XZipAESDecoder(QObject *pParent = nullptr);

    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QString &sPassword, XBinary::CRYPTO_METHOD cryptoMethod,
                        XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baPassword, XBinary::CRYPTO_METHOD cryptoMethod,
                        XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QString &sPassword, XBinary::CRYPTO_METHOD cryptoMethod,
                        XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QByteArray &baPassword, XBinary::CRYPTO_METHOD cryptoMethod,
                        XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    static void pbkdf2(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nIterations, qint32 nKeyLength, QByteArray &baResult);
    static bool deriveKeys(const QByteArray &baPassword, const QByteArray &baSalt, qint32 nKeySize, QByteArray &baAESKey, QByteArray &baPasswordVerify,
                           QByteArray &baHMACKey, XBinary::PDSTRUCT *pPdStruct);
    static bool decryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct);
    static bool encryptAESCTR(const QByteArray &baKey, const QByteArray &baNonce, const char *pInputData, char *pOutputData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct);

    // Custom AES implementation
    static qint32 custom_aes_set_encrypt_key(const quint8 *pUserKey, qint32 nBits, CUSTOM_AES_KEY *pKey);
    static void custom_aes_encrypt(const quint8 *pInput, quint8 *pOutput, const CUSTOM_AES_KEY *pKey);
};

#endif  // XZIPAESDECODER_H
