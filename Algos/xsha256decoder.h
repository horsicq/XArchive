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
#ifndef XSHA256DECODER_H
#define XSHA256DECODER_H

#include <QObject>

class XSha256Decoder : public QObject {
    Q_OBJECT

public:
    explicit XSha256Decoder(QObject *parent = nullptr);

    struct Context {
        quint32 state[8];
        quint64 count;
        quint8 buffer[64];
    };

    static void init(Context *pContext);
    static void update(Context *pContext, const quint8 *pData, qint32 nSize);
    static void final(Context *pContext, quint8 *pDigest);

private:
    static void transform(quint32 state[8], const quint8 data[64]);
    static quint32 rotr(quint32 x, quint32 n);
    static quint32 getBe32(const quint8 *p);
    static void setBe32(quint8 *p, quint32 v);
};

#endif  // XSHA256DECODER_H
