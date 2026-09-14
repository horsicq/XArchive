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
#ifndef XRENPYRPA_H
#define XRENPYRPA_H

#include "xarchive.h"

// Ren'Py RPA archives (UniExtract parity gap G51).  Layout as documented by
// GARbro's RPA reader (MIT) and the Ren'Py archiver, verified against unrpa:
//
//   first line  "RPA-2.0 <16 hex index offset>\n"
//               "RPA-3.0 <16 hex index offset> <8 hex key>\n"
//               "RPA-3.2 <16 hex index offset> <8 hex key> <extra>\n"
//   the index is a zlib stream (to EOF) of a Python pickle (protocol 2, or 3
//   from Python 3 writers) dict: name -> list of (offset, length[, prefix]);
//   in 3.0/3.2 offset and length are XORed with the key.  A member is the
//   concatenation of its segments, each prefix + file[offset : offset +
//   length - len(prefix)].
//
// The pickle reader below is a minimal stack machine for exactly the opcodes
// those indexes use (PROTO, MARK, EMPTY_DICT/LIST/TUPLE, BINUNICODE,
// SHORT_BINSTRING, BINSTRING, BINBYTES, SHORT_BINBYTES, BININT/1/2, LONG1,
// TUPLE/1/2/3, BINPUT, LONG_BINPUT, BINGET, LONG_BINGET, APPEND(S),
// SETITEM(S), STOP); any other opcode refuses the archive.
class XRenpyRpa : public XArchive {
    Q_OBJECT

public:
    explicit XRenpyRpa(QIODevice *pDevice = nullptr);
    ~XRenpyRpa() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    struct SEGMENT {
        qint64 nDataOffset = 0;
        qint64 nDataSize = 0;  // bytes taken from the file (length - prefix)
        QByteArray baPrefix;
    };

    struct MEMBER {
        QString sName;
        QList<SEGMENT> listSegments;
        qint64 nSize = 0;
    };

    struct HEADER {
        QString sVersion;  // "2.0", "3.0", "3.2"
        qint64 nIndexOffset = 0;
        quint32 nKey = 0;
        qint64 nHeaderSize = 0;
    };

    struct CONTEXT {
        HEADER header;
        QList<MEMBER> listMembers;
        qint64 nArchiveEnd = 0;
    };

    // Pickle value tree.
    struct PVALUE {
        enum TYPE {
            TYPE_NONE = 0,
            TYPE_INT,
            TYPE_TEXT,   // BINUNICODE (UTF-8 in baData)
            TYPE_BYTES,  // SHORT_BINSTRING / BINSTRING / (SHORT_)BINBYTES (raw)
            TYPE_LIST,
            TYPE_TUPLE,
            TYPE_DICT,
            TYPE_MARK
        };
        TYPE type = TYPE_NONE;
        qint64 nInt = 0;
        QByteArray baData;
        QList<PVALUE> listItems;  // list/tuple items, dict values
        QList<PVALUE> listKeys;   // dict keys
    };

    static bool parseHeader(const QByteArray &baHead, qint64 nTotalSize, HEADER *pHeader);
    static bool unpickle(const QByteArray &baPickle, PVALUE *pResult);

private:
    bool readContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool inflateIndex(const HEADER &header, qint64 nTotalSize, QByteArray *pIndex, PDSTRUCT *pPdStruct);
    static bool buildMembers(const PVALUE &index, const HEADER &header, qint64 nTotalSize, QList<MEMBER> *pMembers);
};

#endif  // XRENPYRPA_H
