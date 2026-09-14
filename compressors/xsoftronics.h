/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSOFTRONICS_H
#define XSOFTRONICS_H

#include "xarchive.h"

// Softronics "Compressed File" Version 2.00 - the single-file container the
// Softronics Softerm/TSU distribution disks ship every file in.  One member per
// file, and the member carries the name the installer has to write out, which is
// the only place that name survives: the carrier on the disk is the same name
// with one letter of the extension replaced.
//
//   0x00  quint8  nHeaderSize        offset of the compressed data, 0x38..0x43
//   0x01  quint8  0
//   0x02  char    "Softronics Compressed File" + NUL   (27 bytes)
//   0x1d  char    "Version 2.00" + NUL                 (13 bytes)
//   0x2a  quint32 nCompressedSize    bytes of code stream, to end of file
//   0x2e  char    szFileName[]       the original 8.3 name, ASCIIZ, 1..12 chars
//   ....  quint32 nUncompressedSize
//   ....  quint32 nDosDateTime       packed DOS time in the low word
//   ....  the LZW code stream
//
// nHeaderSize IS the data offset, so the whole layout is cross-checked by two
// independent identities that a random file cannot satisfy at once:
//
//     nHeaderSize == 0x2e + strlen(name) + 1 + 8
//     nHeaderSize + nCompressedSize == file size
//
// The codec is the GIF dialect of LZW and needs no decoder of its own: 9..12 bit
// codes, LSB-first, clear = 0x100, end = 0x101, first free slot = 0x102, no early
// change and no block padding - i.e. XSharedLZWDecoder with bHasClearCode and
// bHasEndCode set and everything else off.  The stream ALWAYS opens with a clear
// code, which is what makes the shared decoder's "read the first code after a
// clear as a literal" path the one that initialises the KwKwK state; a decoder
// that instead consumes an initial code before the loop emits one wrong byte per
// KwKwK that follows a clear, and nothing else, so the mistake survives every
// small member.
class XSoftronics final : public XArchive {
    Q_OBJECT

public:
    explicit XSoftronics(QIODevice *pDevice = nullptr);
    ~XSoftronics() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
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

private:
    struct CONTEXT {
        qint64 nArchiveSize;
        qint64 nHeaderSize;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint32 nDosDateTime;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    // The stored name is a bare DOS 8.3 name in every sample, but it is raw
    // bytes: path separators are escaped rather than honoured, because a
    // single-member container must never write outside the extraction root.
    static QString sanitizeName(const QByteArray &baRawName);
};

#endif  // XSOFTRONICS_H
