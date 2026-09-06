/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPCSECURE_H
#define XPCSECURE_H

#include "xarchive.h"

// Central Point PCSECURE protected file (PC Tools 5.x / 6.x / 7.x).
//
// The container holds exactly one file behind a 68-byte header whose middle is
// itself encrypted:
//
//   +0x00  4 bytes  "PCT5" | "PCT6" | "PCT7" | "AfoS"   (plaintext)
//   +0x04  56 bytes DES-ECB encrypted header body, seven blocks
//   +0x3c  8 bytes  password verifier (plaintext; zero when no user password
//                   was set, in which case one of the four product keys opens
//                   the file)
//   +0x44  ...      the encrypted, optionally LZW-compressed payload
//
// Decrypted header body (after the byte-order fixups the original applies):
//
//   +0x08  u32      flags; bit 0 = the payload is LZW-compressed
//   +0x0c  u16      DES round count for the PAYLOAD, 0..16 - the header itself
//                   is always 16 rounds (3 for PCT7)
//   +0x0e  10 bytes ASCII "DOS " plus the original extension
//   +0x12  4 bytes  the original extension including its dot, e.g. ".COM"
//   +0x18  u32      uncompressed size
//   +0x20  u32      compressed size
//   +0x28/+0x30 and +0x2c/+0x34  XOR to the ASCII "SeaHawks" - the only
//                   integrity check the format has, and what tells a correct
//                   key from a wrong one
//   +0x38  u32      DOS timestamp (PCT7 only)
//
// There is no stored name: the member is the archive's own file name with the
// extension replaced by the one at +0x12, which is what U3 does.
//
// A file no built-in key opens can be listed but not extracted; such a member is
// published with HANDLE_METHOD_UNKNOWN and FPART_PROP_ENCRYPTED so the unpack
// path fails cleanly instead of writing ciphertext.  Two cases reach it and the
// reported method distinguishes them: a non-zero verifier means a real user
// password was set, while a zero verifier that still opens under none of the
// four product keys is a container variant this reader does not implement (every
// PCT6 sample in the reference corpus is one - its header body is not DES-ECB at
// all: two PCT6 files whose plaintext differs only in the last byte of a block
// have ciphertext that differs only in that same byte, which no block cipher can
// produce.  U3 does not read them either).
class XPCSecure final : public XArchive {
    Q_OBJECT

public:
    explicit XPCSecure(QIODevice *pDevice = nullptr);
    ~XPCSecure() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
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
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nSignature;
        quint32 nFlags;
        quint32 nDosTime;
        qint32 nRounds;
        bool bKeyFound;
        bool bUserPassword;  // the plaintext verifier at +0x3c is non-zero
        bool bCompressed;
        QByteArray baProperty;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString memberName(const QByteArray &baExtension);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPCSECURE_H
