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
#ifndef XNERODISCIMAGE_H
#define XNERODISCIMAGE_H

#include "xarchive.h"

// Nero Burning ROM disc image (.nrg).  The track data comes first and a chunk
// list ("CUEX"/"DAOX"/"ETN2"/"SINF"/... terminated by "END!") is addressed by
// the trailing "NERO"+BE32 (v1) or "NER5"+BE64 (v2) footer.  Every data track
// is published as Track<NN>.iso with its sectors reduced to 2048-byte user
// data; audio tracks are listed but not extractable.
class XNeroDiscImage : public XArchive {
    Q_OBJECT

public:
    explicit XNeroDiscImage(QIODevice *pDevice = nullptr);
    ~XNeroDiscImage() override;

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

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct TRACK {
        QString sName;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint32 nSectorSize;
        qint32 nUserOffset;   // offset of the 2048 user bytes inside a sector
        qint64 nOutputSize;   // sectors * 2048, 0 for audio
        bool bAudio;
        QString sMethod;
    };

    struct CONTEXT {
        qint64 nFileSize;
        qint32 nFooterVersion;
        qint64 nChunkListOffset;
        QList<TRACK> listTracks;
    };

    bool parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool describeTrack(TRACK *pTrack, qint64 nOffset, qint64 nSize, qint32 nSectorSize, qint32 nModeCode, qint64 nChunkListOffset, PDSTRUCT *pPdStruct);
};

#endif  // XNERODISCIMAGE_H
