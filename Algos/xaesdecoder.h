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

// AES constants
#define AES_BLOCK_SIZE 16

class XAESDecoder : public QObject {
    Q_OBJECT

public:
    explicit XAESDecoder(QObject *parent = nullptr);

    // Decrypt AES-encrypted data from 7z archives
    // Properties format: NumCyclesPower (1 byte) + SaltSize (1 byte) + Salt (0-16 bytes) + IV (16 bytes)
    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecryptState, const QByteArray &baProperties, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    // Derive key from password using SHA-256 with salt and cycle power
    static void deriveKey(const QString &sPassword, const QByteArray &baSalt, quint8 nNumCyclesPower, quint8 *pKey);
};

#endif  // XAESDECODER_H
