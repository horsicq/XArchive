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
#ifndef XSAR_H
#define XSAR_H

#include "xlha.h"

/* Streamline Design's SAR.
 *
 * SAR is LHA with a differently spelled method tag. A byte comparison of a SAR
 * archive against an LHA 2.13 archive of the same payload showed the two differ
 * in ten bytes: the method tag (" LH5 " instead of "-lh5-"), the header
 * checksum that follows from it, the OS identifier, and a zeroed timestamp.
 * Header size, both sizes, attribute byte, level, name, CRC-16, extended-header
 * size, the compressed stream and the archive terminator were identical, and the
 * compressed stream decoded to the same SHA-256 as an independent copy of the
 * payload. The container is therefore inherited rather than reimplemented.
 *
 * Detection has to be stricter than LHA's. SAR carries no magic number - byte 0
 * is a header length and byte 1 a checksum - so the only fixed bytes are " LH"
 * and the trailing space of the tag. The extension is also badly overloaded: of
 * 34 files named *.sar on the development machine, 30 were unrelated (game
 * resources, bitmaps, plain text) and two more were ZIP archives. isValid()
 * therefore also requires a supported header level and a verified header
 * checksum. */

class XSAR : public XLHA {
    Q_OBJECT

public:
    explicit XSAR(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    virtual QList<QString> getSearchSignatures() override;

    virtual FT getFileType() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QString getMIMEString() override;
    virtual QString getVersion() override;

protected:
    virtual bool _isMemberTag(const QByteArray &baHeader) override;
};

#endif  // XSAR_H
