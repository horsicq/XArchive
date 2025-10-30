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
#ifndef XZIPCRYPTODECODER_H
#define XZIPCRYPTODECODER_H

#include "xbinary.h"

class XZipCryptoDecoder : public QObject {
    Q_OBJECT

public:
    explicit XZipCryptoDecoder(QObject *pParent = nullptr);

    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decrypt(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baPassword, XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QString &sPassword, quint32 nCRC32, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool encrypt(XBinary::DATAPROCESS_STATE *pCompressState, const QByteArray &baPassword, quint32 nCRC32, XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    static void initKeys(quint32 *pnKeys, const QByteArray &baPassword);
    static void updateKeys(quint32 *pnKeys, quint8 nByte);
    static quint8 decryptByte(const quint32 *pnKeys);
};

#endif  // XZIPCRYPTODECODER_H
