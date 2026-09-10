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
#ifndef XTNEFARCHIVE_H
#define XTNEFARCHIVE_H

#include "xarchive.h"

// TNEF - Microsoft Transport Neutral Encapsulation Format (reference
// The reference implementation detect / the reference implementation walk). Public format; the reference
// walker only confirms which attributes it actually acts on.
//
//   u32 signature = 0x223E9F78
//   u16 key, never zero
//   then a stream of attributes
//      u8  level     1 message, 2 attachment
//      u32 attId     (attType << 16) | attName
//      i32 length
//      u8  data[length]
//      u16 checksum  = sum(data) & 0xFFFF
//
// Attributes that matter here, all at attachment level
//   0x00069002 attAttachRenddata  starts a new attachment (flush the previous)
//   0x00018010 attAttachTitle     the 8.3 name (PT_STRING8)
//   0x0006800F attAttachData      the payload, stored verbatim
//   0x00069005 attAttachment      a MAPI property stream: PR_ATTACH_DATA_OBJ
//                                 (0x3701) supplies the payload when
//                                 attAttachData is absent, and
//                                 PR_ATTACH_LONG_FILENAME (0x3707) the name
//
// MAPI property stream: u32 count, then per property u16 type, u16 id; a named
// id (>= 0x8000) is followed by a 16-byte GUID, a u32 kind and either a u32 lid
// or a u32 length plus a name padded to four; multi-valued (type & 0x1000) and
// variable-length types carry a u32 value count, fixed types are padded to a
// multiple of four and variable ones are u32 length plus bytes padded the same
// way.  A PT_OBJECT value carries a 16-byte interface GUID in front of its real
// payload, which the reference strips.
//
// TEN of the seventeen reference archives hold NO attachments at all.  That is
// the correct answer for them, not a failure, so an attachment-free TNEF still
// opens - with zero members.
//
// The message body is not an attachment: it lives in the message-level
// attributes, as attBody, or in attMAPIProps as PR_BODY, PR_BODY_HTML or
// PR_RTF_COMPRESSED (LZFu, MS-OXRTFCP).  It is rendered to plain text and
// published as ONE synthetic member named "Content.txt", appended after every
// attachment so that no attachment name, index, offset or byte moves.  Its
// bytes are computed rather than stored, so it carries no extent in the file
// and unpackCurrent() serves it from memory.
class XTNEFArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
        // The rendered message body: not a region of the file, so nDataOffset
        // and nSize describe no extent and the bytes below are authoritative.
        bool bSynthetic;
        QByteArray baInlineData;
    };

    explicit XTNEFArchive(QIODevice *pDevice = nullptr);
    ~XTNEFArchive() override;

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
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    // What one walk of a MAPI property stream should collect.  A null pointer
    // means "not interested", so the attachment call site and the message-body
    // call site share the single walk without either seeing the other's ids.
    struct PROPERTY_SINK {
        qint64 *pnDataOffset;
        qint64 *pnDataSize;
        QString *psFileName;
        QByteArray *pbaRtfCompressed;
        QByteArray *pbaBodyHtml;
        QByteArray *pbaBody;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool scanProperties(const QByteArray &baStream, const PROPERTY_SINK &sink);
    static bool scanAttachmentProperties(const QByteArray &baStream, qint64 *pnDataOffset, qint64 *pnDataSize, QString *psFileName);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTNEFARCHIVE_H
