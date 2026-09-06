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
#ifndef XHUFFDECODER_H
#define XHUFFDECODER_H

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

// Codec of the "HUF" multi-file Huffman archive (magic BD 01).
//
// The archive carries ONE Huffman tree that every member - and every member
// NAME - is coded with.  The tree lives in the archive header, not in the
// member stream, which is why the tree has to travel to the decompressor as a
// property blob instead of as part of the member's bytes.
//
// Tree encoding (bit stream, LSB-first within each byte, a fresh byte is
// consumed whenever all 8 bits are used up):
//
//   bit 0 -> leaf; its symbol is the next unused byte of the archive's
//            frequency-ordered symbol table
//   bit 1 -> internal node; the two subtrees follow immediately, child0 first
//            and then child1, both encoded the same way
//
// Decoding a symbol walks from the root: a SET bit selects child0, a CLEAR bit
// selects child1 (this asymmetry is the original's, not a transcription slip),
// and the walk stops on a node whose child0 is absent.
//
// Every stream - each member's name and each member's data - restarts the bit
// reader byte-aligned at its own file offset; there is no end marker, the
// caller stops after the expected number of symbols (or, for a name, at the
// first NUL).
class XHuffDecoder {
public:
    struct NODE {
        qint32 nChild0;  // -1 when absent; absent means "this node is a leaf"
        qint32 nChild1;
        quint8 nSymbol;
    };

    // Blob handed over as FPART_PROP_COMPRESSPROPERTIES:
    //   u16 symbolCount, symbolCount bytes of symbol table, then the raw bytes
    //   that hold the tree bit stream.
    static QByteArray packTree(const QByteArray &baSymbols,
                               const QByteArray &baTreeBits);
    static bool unpackTree(const QByteArray &baProperty, QByteArray *pbaSymbols,
                           QByteArray *pbaTreeBits);

    // Rebuilds the shared tree.  Fails unless every symbol-table entry is used
    // exactly once, which is what makes a stray BD 01 file fail detection.
    static bool buildTree(const QByteArray &baTreeBits,
                          const QByteArray &baSymbols,
                          QVector<NODE> *pListNodes, qint64 *pnBytesUsed);

    // nCount < 0 means "until the first NUL"; nLimit then caps the length.
    static bool decodeSymbols(const QVector<NODE> &listNodes,
                              const QByteArray &baInput, qint64 nCount,
                              qint64 nLimit, QByteArray *pbaResult);

    static bool decodeWithProperty(const QByteArray &baProperty,
                                   const QByteArray &baInput,
                                   qint64 nUncompressedSize,
                                   QByteArray *pbaResult);

    static const qint32 MAX_SYMBOLS = 256;
    static const qint32 MAX_NODES = 1024;
    static const qint32 MAX_NAME_SIZE = 256;
};

#endif  // XHUFFDECODER_H
