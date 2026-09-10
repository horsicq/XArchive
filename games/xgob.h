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
#ifndef XGOB_H
#define XGOB_H

#include "games/xgamestorearchive_p.h"

// Native, store-only reader for the LucasArts Dark Forces GOB resource
// container ("GOB\n" magic, trailing directory of {offset, size, 13-byte
// name} records).  Every member is stored verbatim, so the generic extraction
// chain handles the payload through HANDLE_METHOD_STORE.
//
// XGameStoreArchiveBase supplies the record model and the name helpers, but it
// only drives scanFormat() for the file types enumerated inside its private
// scanArchive() gate, and FT_GOB is not one of them.  Until that list carries
// FT_GOB, every base entry point that funnels through scanArchive() -
// isValid(), getFileFormatSize() and initUnpack() - refuses the container
// before scanFormat() is ever reached, which is why the reader detected and
// enumerated nothing.  The overrides below run the same scan for FT_GOB
// without touching the shared gate; they become redundant (but stay correct)
// the moment FT_GOB is added to it.
class XGob final : public XGameStoreArchiveBase {
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XGob(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QList<QString> getSearchSignatures() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;

    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct GOB_CONTEXT {
        QList<ENTRY> listEntries;
        qint64 nArchiveEnd = 0;
    };

    // Runs scanFormat() behind the same device-position snapshot and file-type
    // re-check the shared gate applies to the formats it does accept.
    bool scanGob(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                 PDSTRUCT *pPdStruct);

    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XGOB_H
