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
#ifndef XRGSSAD_H
#define XRGSSAD_H

#include "xarchive.h"

// RPG Maker XP / VX / VX Ace encrypted archives (UniExtract parity gap
// G52): "RGSSAD\0" + version byte 1 (.rgssad / .rgss2a) or 3 (.rgss3a).
// Key streams as documented by RgssDecrypter (MIT) and verified against it:
//
//   v1: key = 0xDEADCAFE; per entry name_len = u32 ^ key, key = key*7+3;
//       each name byte ^ (key & 0xFF) with key = key*7+3 per byte; size =
//       u32 ^ key, key = key*7+3; the entry's data key is the key at that
//       point (the table key is not advanced by the data); entries follow
//       each other to EOF.
//   v3: key = u32(at 8)*9+3; entries {offset, size, data key, name_len}
//       each ^ key, name bytes ^ (key >> (8*(i%4))); offset 0 ends the table;
//       offsets are absolute.
//   Data: little-endian u32 chunks ^ data key, data key = data key*7+3
//       after each; a partial last chunk uses the key's low bytes.
class XRgssad : public XArchive {
    Q_OBJECT

public:
    explicit XRgssad(QIODevice *pDevice = nullptr);
    ~XRgssad() override;

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

    struct MEMBER {
        QString sName;
        qint64 nDataOffset = 0;
        qint64 nSize = 0;
        quint32 nDataKey = 0;
    };

    struct CONTEXT {
        qint32 nVersion = 0;
        QList<MEMBER> listMembers;
        qint64 nArchiveEnd = 0;
    };

private:
    bool readContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readVersion1(qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readVersion3(qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static qint32 readVersionByte(const QByteArray &baHeader);
};

#endif  // XRGSSAD_H
