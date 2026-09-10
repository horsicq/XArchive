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
#ifndef XSFPACKARCHIVE_H
#define XSFPACKARCHIVE_H

#include "xarchive.h"

#include "Algos/xsfpackdecoder.h"

// SFPack (.sfpack) - a SoundFont 2 compressor.
//
// SFPACK IS NOT AN ARCHIVE.  It stores the pieces of ONE .sf2 (RIFF/sfbk) - an
// INFO list, a pdta list and one compressed stream per sample - and the
// reference extractor rebuilds that single file, relaying out the sample data
// and rewriting the dwStart / dwEnd / dwStartloop / dwEndloop fields of every
// shdr record to match.  So this reader publishes exactly ONE member, the
// rebuilt .sf2, and the whole container is handed to XSFPACKDecoder; see that
// header for the two codecs and the reassembly rules.
//
// The container stores no member name, so the name comes from the ARCHIVE's own
// file name with a .sf2 extension, which is what the reference extractor does.
//
// The i32 at +0x08 claims to be the size of the .sf2 that will be produced and
// IS NOT RELIABLE - 5 of the 7 reference-corpus files declare between 468 and
// 1170 bytes more than the reference actually writes.  The uncompressed size
// published here is therefore MEASURED (XSFPACKDecoder::measure decompresses
// the pdta chunk and adds up the sample lengths); the declared value is only
// reported as the version string.  Measuring costs one pdta decompression, so
// isValid() deliberately does not do it: validation stops at the chunk headers
// and the sample offset table.
class XSFPACKArchive : public XArchive {
    Q_OBJECT

public:
    explicit XSFPACKArchive(QIODevice *pDevice = nullptr);
    ~XSFPACKArchive() override;

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
        qint64 nUncompressedSize;  // measured, -1 when not measured
        XSFPACKDecoder::HEADER header;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSFPACKARCHIVE_H
