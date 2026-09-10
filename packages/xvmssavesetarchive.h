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
#ifndef XVMSSAVESETARCHIVE_H
#define XVMSSAVESETARCHIVE_H

#include "xarchive.h"

#include "Algos/xvmssavesetdecoder.h"

// OpenVMS BACKUP save set.
//
// THERE IS NO CODEC HERE AND NO KEY MATERIAL.  A save set is block and record
// CHAINING, nothing more; the "DCX compressed variant" this family is often
// assumed to need does not exist, and the one transform that does exist is the
// VMS variable-length-record to CRLF conversion, which is a text reformat, not
// a compressor.
//
// The block/record layout, the file-attribute parser, the member-assembly
// pipeline and - importantly - the SPAN AND RESUME design that makes a member
// addressable at all live in Algos/xvmssavesetdecoder.h, which is the single
// source of truth for the 24-byte FPART_PROP_COMPRESSPROPERTIES blob.  They sit
// there rather than here because the dispatch that calls
// XVMSSaveSetDecoder::decode() is part of the shared core, which has to link
// for targets built without the archive classes.
//
// What is left in this class is the container plumbing: detection off the first
// block header, the member list, and the streaming contract.  A record
// publishes the member's SPAN (first body record header to last body byte)
// rather than the whole file, because a member does not occupy one contiguous
// range - block and record headers sit inside it - and re-walking from byte
// zero for every member would cost O(archive) each time.
//
// DIRECTORY ENTRIES ARE NOT PUBLISHED, matching the reference; their body
// records are still consumed during the walk or the chain desynchronises.
class XVMSSaveSetArchive : public XArchive {
    Q_OBJECT

public:
    explicit XVMSSaveSetArchive(QIODevice *pDevice = nullptr);
    ~XVMSSaveSetArchive() override;

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
        QList<XVMSSaveSetDecoder::MEMBER> listMembers;
    };

    // bWalkMembers enumerates the whole save set; detection only checks the
    // first block header and must not pay for the walk.
    bool parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct);
    static QString methodToString(const XVMSSaveSetDecoder::MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XVMSSAVESETARCHIVE_H
