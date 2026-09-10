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
#ifndef XIS7INX_H
#define XIS7INX_H

#include "xarchive.h"

// InstallShield 7 obfuscated compiled InstallScript (setup.inx / Binary.InstallScript).
//
// There is no container and no header: the whole file is a single byte stream
// put through a length-preserving, position-dependent obfuscation.  Decoding
// byte i of the file is
//
//     v = ror8(byte ^ 0xF1, 2) - (i % 0x47)
//
// where i is a 32-bit counter that starts at 0 at file offset 0 and never
// resets.  The result is the ordinary InstallShield compiled-script image,
// whose own magic is "aLuZ" followed by 00 00 and the
// "Copyright (c) 1990-2002 InstallShield Software Corp." banner.
//
// Because the first 16 plaintext bytes ("aLuZ\0\0Copyright ") are constant
// across every build, and the obfuscation of a given offset is deterministic,
// the first 16 bytes of the obfuscated file are the same fixed sequence in
// every sample:
//
//     74 C4 2C 84 E1 E5 D4 28 10 FB 00 20 3C 24 FB 4D
//
// That sequence is the detection signature; it is what the reference tool
// (the reference implementation, class "kia", entry A028) tests as four little-endian dwords,
// and it holds for all 51 corpus samples.
//
// One member per file.  The format stores no name; the reference tool labels
// the output "Setup.ini" unconditionally, and that label is reproduced here so
// that extraction matches the oracle byte for byte AND name for name (the
// payload is really a compiled .inx, not an .ini - the name is simply what the
// obfuscated container has always been unpacked as).
class XIS7Inx : public XArchive {
    Q_OBJECT
public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_IS7INX_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct IS7INX_HEADER {
        quint8 signature[16];
    };
#pragma pack(pop)

    explicit XIS7Inx(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    bool isSigned() override;
    OSNAME getOsName() override;
    QString getOsVersion() override;
    QString getVersion() override;
    bool isEncrypted() override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    IS7INX_HEADER _read_IS7INX_HEADER(qint64 nOffset);

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    bool _readAndCheckHeader(PDSTRUCT *pPdStruct);

    struct IS7INX_UNPACK_CONTEXT {
        qint64 nTotalSize;
        QString sFileName;
    };

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XIS7INX_H
