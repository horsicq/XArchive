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
#ifndef XXEDITPACKARCHIVE_H
#define XXEDITPACKARCHIVE_H

#include "xarchive.h"

// XEDIT PACK - the container written by the CMS COPYFILE PACK / XEDIT PACK
// command.  One member, an 8-byte header, no directory:
//
//   +0x00 u16 BE  0x0001
//   +0x02 u8      0x40           the EBCDIC blank
//   +0x03 u8      record format: 0xC6 = EBCDIC 'F' (fixed) or 0xE5 = 'V'
//   +0x04 u32 BE  logical record length / record count
//   +0x08         the packed byte stream (see XXEditPackDecoder)
//
// Everything here is BIG ENDIAN - it is a mainframe format - and the payload
// stays in EBCDIC, untranslated, exactly as the reference extractor leaves it.
//
// TWO THINGS THAT COST TIME:
//
//  * THE PLAINTEXT LENGTH IS STORED NOWHERE.  The u32 at +0x04 looks like a
//    size and is not one: across the reference corpus it holds values such as
//    0x52, 0x228 and 0xFFFF for members of 2084, 692 and 83680 bytes.  The
//    only way to the real length is to walk the opcode stream, so parseContext
//    takes a bMeasure flag and does that walk exclusively on the paths that
//    need it (getFileParts / initUnpack), never on isValid.
//  * THE DETECTOR IS FOUR BYTES WIDE and three of them are constant, so the
//    record-format byte is what carries most of the discrimination.  Only 0xC6
//    and 0xE5 appear; accepting any byte there would turn a two-byte magic
//    into a false-positive generator.
//
// The member carries no name; it is named after the archive, which is what the
// reference extractor does.
class XXEditPackArchive : public XArchive {
    Q_OBJECT

public:
    explicit XXEditPackArchive(QIODevice *pDevice = nullptr);
    ~XXEditPackArchive() override;

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
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        // False until XXEditPackDecoder::measure() has walked the stream.
        bool bUncompressedSizeKnown;
        quint8 nRecordFormat;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XXEDITPACKARCHIVE_H
