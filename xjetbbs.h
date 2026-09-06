/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJETBBS_H
#define XJETBBS_H

#include "xlha.h"

// JetBBS distribution archive (JETBBS.DAT).
//
// This is an ordinary LHA archive whose five-character method tag has been
// re-spelled: "-mg5-" where LHA writes "-lh5-".  Nothing else about the
// container changes.  Verified over the whole reference corpus: the four
// JETBBS.DAT files hold 470 members between them, every one a level 1 header
// with a correct header checksum, a correct extended-header chain and a
// "-mg5-" payload that decodes byte-for-byte as LHA -lh5- (NC = 510 / CBIT = 9,
// NT = 19 / TBIT = 5, np = 14 / pbit = 4, THRESHOLD = 3, 8 KiB window), each
// archive ending on the usual single 0x00 end-of-archive marker.  470 of 470
// members matched the reference extractor bit for bit.
//
// U3's detector (FUN_0055a390) accepts "-mg0-", "-mg4-" and "-mg5-" and header
// levels 0 to 3, then hands the file to the very same worker its plain LHA
// handler uses (FUN_004f73a0).  This class mirrors that: it inherits the whole
// LHA member walk from XLHA - which already exposes _isMemberTag as a virtual
// hook for exactly this situation, XSAR being the other user - and only
// re-spells the tag and maps the three tags onto the decoders that already
// exist.  No new codec, and no HANDLE_METHOD of its own:
//
//     -mg0-  ->  HANDLE_METHOD_STORE
//     -mg4-  ->  HANDLE_METHOD_LZH4
//     -mg5-  ->  HANDLE_METHOD_LZH5
//
// Detection has to be strict, because byte 0 is a header length and byte 1 a
// checksum - the only fixed bytes in the file are "-mg" and the '-' at +6.
// isValid() therefore also demands a supported method character, a supported
// header level and a verified header checksum, the same bar XSAR sets.
class XJETBBS : public XLHA {
    Q_OBJECT

public:
    explicit XJETBBS(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;

    // XLHA sets FPART_PROP_HANDLEMETHOD from its own tag table, which does not
    // know "-mg?-" and would leave every member at HANDLE_METHOD_UNKNOWN (the
    // generic chain then refuses the member instead of silently storing it).
    // Both places that publish the property are re-spelled here.
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool _isMemberTag(const QByteArray &baHeader) override;

private:
    static HANDLE_METHOD _jetbbsMethodToHandle(const QString &sMethod);
};

#endif  // XJETBBS_H
