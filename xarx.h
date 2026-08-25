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
#ifndef XARX_H
#define XARX_H

#include "xarchive.h"

/* ARX.
 *
 * ARX reuses LHA's method tag - the fixtures carry a literal "-lh1-" - and even
 * produces a valid LHA header checksum, but it is not LHA: one byte is inserted
 * at offset 7, so every field from there on is shifted by one. Read as LHA the
 * compressed size lands on the wrong bytes and comes out far larger than the
 * file, which is why these archives used to report as LHA and then refuse to
 * open. XLHA now rejects them and this class handles them.
 *
 * Member layout, verified against both fixtures:
 *
 *   +0   u8    header size          total header = this + 2; 0 ends the archive
 *   +1   u8    header checksum      sum of the header bytes that follow
 *   +2   5     method tag           "-lh1-" in every known sample
 *   +7   u8    zero pad             the byte LHA does not have
 *   +8   u32   compressed size      0 means the member is stored
 *   +12  u32   original size
 *   +16  u16   DOS time
 *   +18  u16   DOS date
 *   +20  u8    DOS attribute
 *   +21  u8    header level
 *   +22  u8    file name length
 *   +23  n     file name
 *   last u8    low byte of the CRC-16 only
 *
 * The stored CRC is one byte, not sixteen, so no checksum is published: a
 * truncated value cannot be validated by any CRC_TYPE_* the record model has,
 * and advertising it as a CRC-16 would be a false claim. */

class XARX : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sMethod;
        QString sFileName;
        quint32 nDosDateTime;
        bool bStored;
    };

    explicit XARX(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    virtual QList<QString> getSearchSignatures() override;

    virtual FT getFileType() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QString getMIMEString() override;
    virtual QString getVersion() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

private:
    // Reads the member starting at nOffset. Returns false at the terminator or
    // on anything that does not fit inside the file.
    bool _readMember(qint64 nOffset, MEMBER *pMember, PDSTRUCT *pPdStruct = nullptr);
    QList<MEMBER> _collectMembers(qint32 nLimit, PDSTRUCT *pPdStruct = nullptr);
    static HANDLE_METHOD _methodToHandle(const MEMBER &member);

    INTERNAL_INFO m_internalInfo;
};

#endif  // XARX_H
