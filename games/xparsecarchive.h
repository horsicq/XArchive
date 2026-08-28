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
#ifndef XPARSECARCHIVE_H
#define XPARSECARCHIVE_H

#include "xgamestorearchive_p.h"

// Parsec engine's unnamed resource list.  The first offset determines the
// number of records; an offset array, a zero sentinel and a size array are
// followed by contiguous RIB or SM8 members.
class XParsecArchive final : public XGameStoreArchiveBase {
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XParsecArchive(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XPARSECARCHIVE_H
