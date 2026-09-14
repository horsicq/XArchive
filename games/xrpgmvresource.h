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
#ifndef XRPGMVRESOURCE_H
#define XRPGMVRESOURCE_H

#include "xarchive.h"

// RPG Maker MV encrypted resource (UniExtract parity gap G53): a 16-byte
// header "RPGMV\0\0\0" + three version bytes + five zero bytes, then the
// original file whose first 16 bytes are XORed with the 16-byte key (the
// 32-hex-digit "encryptionKey" of the game's data/System.json); the rest is
// stored unchanged.  .rpgmvp -> .png, .rpgmvo -> .ogg, .rpgmvm -> .m4a.
//
// The key file lives in a sibling tree (www/data) of the resource
// (www/img/..., www/audio/...), so it is resolved through
// XCompanionFile::resolveInAncestors("data/System.json", 4 levels).  A
// 32-hex-digit unpack password is accepted as the key instead.  Without a
// key the resource still lists, but extraction fails closed: no guessing.
class XRpgmvResource : public XArchive {
    Q_OBJECT

public:
    explicit XRpgmvResource(QIODevice *pDevice = nullptr);
    ~XRpgmvResource() override;

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

    struct CONTEXT {
        QString sMemberName;
        qint64 nPayloadSize = 0;
        QByteArray baKey;     // 16 bytes, or empty when no key was found
        QString sKeySource;   // "password", the System.json path, or empty
    };

    // Parses a 32-hex-digit key; empty result when the text is not one.
    static QByteArray parseKey(const QString &sText);
    // Reads "encryptionKey" out of a System.json file; empty when absent.
    static QByteArray readSystemJsonKey(const QString &sPath);

private:
    bool readContext(CONTEXT *pContext, const QString &sPassword, PDSTRUCT *pPdStruct);
    QString memberName();
};

#endif  // XRPGMVRESOURCE_H
