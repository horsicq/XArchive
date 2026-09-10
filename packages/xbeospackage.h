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
#ifndef XBEOSPACKAGE_H
#define XBEOSPACKAGE_H

#include "xarchive.h"

// BeOS SoftwareValet installer package (.pkg).  Not the later Haiku .hpkg.
//
// After an eight-byte signature ("AlB", 0x1A, 0xFF, 0x0A, 0x0D, 0x00) the file
// is a tree of seven-byte tagged records:
//
//   +0  4  tag, four ASCII characters
//   +4  1  always zero
//   +5  2  payload type, little endian
//
// Payload types: 0x101 is four bytes, 0x102 is eight, 0x200 and 0x500 are a
// big-endian u32 length followed by that many bytes, 0x300 is a twenty-byte
// descriptor (u64 compressed size, u64 original size, u32 method) followed by
// the compressed bytes, and 0x400 opens a nested group that runs until a
// record whose tag and type are both zero.
//
// Everything numeric in this format is BIG endian, including the offsets,
// because the format predates the x86 port.
//
// The tree lives at the offset the top-level "COff" record gives; each "FilI"
// record carries "Name", "OffT" and "OrgS", and at OffT there is a "FiDa"
// group holding a "FiMF" descriptor whose method is 2, meaning zlib.  Note the
// descriptor tag is FiMF and not FiFM - the transposition costs an hour.
class XBeOSPackage : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;      // start of the FiDa group
        qint64 nStreamOffset;    // start of the deflate stream itself
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XBeOSPackage(QIODevice *pDevice = nullptr);
    ~XBeOSPackage() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct TAG {
        QByteArray baId;
        quint16 nType;
        qint64 nPayloadOffset;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool readTag(qint64 nOffset, TAG *pTag, PDSTRUCT *pPdStruct);
    bool skipPayload(qint64 nOffset, quint16 nType, qint64 *pnNext, qint32 nDepth, PDSTRUCT *pPdStruct);
    bool readFields(qint64 nOffset, QString *psName, qint64 *pnDataOffset, qint64 *pnOriginalSize, qint64 *pnNext, PDSTRUCT *pPdStruct);
    bool walkFolder(qint64 nOffset, const QString &sPrefix, CONTEXT *pContext, qint64 *pnNext, qint32 nDepth, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    qint64 m_nParseInputSize;
};

#endif  // XBEOSPACKAGE_H
