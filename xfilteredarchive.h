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
#ifndef XFILTEREDARCHIVE_H
#define XFILTEREDARCHIVE_H

#include "xarchive.h"

class QBuffer;

// Adapts one or more native single-stream compression formats to the archive
// streaming API.  Every outer layer is authenticated and materialized into a
// bounded private buffer; the final native archive owns a persistent streaming
// state for the lifetime of the caller's UNPACK_STATE.
class XFilteredArchive : public XArchive {
    Q_OBJECT

public:
    explicit XFilteredArchive(QIODevice *pDevice = nullptr,
                              FT outerFileTypeHint = FT_UNKNOWN);
    ~XFilteredArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice,
                        PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice, FT outerFileTypeHint,
                        PDSTRUCT *pPdStruct = nullptr);

    // XFormats must suppress this adapter while it asks for the native type of
    // a layer.  These helpers keep that routing contract in one place.
    static const char *recursionPropertyName();
    static const char *depthPropertyName();
    static bool isRecursionSuppressed(QIODevice *pDevice);
    static bool isFilterFileType(FT fileType);
    static bool isDedicatedCompressedTarFileType(FT fileType);

    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct FILTERED_UNPACK_CONTEXT {
        QBuffer *pDecodedDevice;
        XArchive *pInnerArchive;
        UNPACK_STATE innerState;
        FT outerFileType;
        FT innerFileType;
        qint32 nFilterDepth;
        qint64 nOuterSize;
        bool bInnerInitialized;

        FILTERED_UNPACK_CONTEXT();
        ~FILTERED_UNPACK_CONTEXT();
        bool finishInner(PDSTRUCT *pPdStruct);
    };

    static FT detectNativeFileType(QIODevice *pDevice,
                                   PDSTRUCT *pPdStruct);
    static XArchive *createNativeArchive(FT fileType, QIODevice *pDevice);
    static bool readFilterDepth(QIODevice *pDevice, qint32 *pnDepth);
    static bool publicStateMatchesInner(
        const UNPACK_STATE *pState,
        const FILTERED_UNPACK_CONTEXT *pContext);
    static void copyInnerState(UNPACK_STATE *pState,
                               const FILTERED_UNPACK_CONTEXT *pContext);

    QBuffer *materializeLayer(
        QIODevice *pInputDevice, FT fileType,
        const QMap<UNPACK_PROP, QVariant> &mapProperties,
        UNPACK_STATE *pOuterState, PDSTRUCT *pPdStruct);

    FT m_outerFileTypeHint;
};

#endif  // XFILTEREDARCHIVE_H
