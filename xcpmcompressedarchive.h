// U3 Crunch/CPMLZH/Unix Compact handler port; see Algos/xu3cpmdecoder.PROVENANCE.md.
#ifndef XCPMCOMPRESSEDARCHIVE_H
#define XCPMCOMPRESSEDARCHIVE_H

#include "xarchive.h"
#include "Algos/xu3cpmdecoder.h"

class XCpmCompressedArchive final : public XArchive {
    Q_OBJECT
public:
    explicit XCpmCompressedArchive(QIODevice *pDevice = nullptr, FT fileType = FT_CPM_CRUNCH);
    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, FT fileType, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getVersion() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT { XU3CpmDecoder::Header header; QString sFileName; qint64 nUncompressedSize = 0; };
    bool readHeader(XU3CpmDecoder::Header *pHeader, PDSTRUCT *pPdStruct, qint64 *pUncompressedSize = nullptr,
                    qint64 nOutputLimit = XU3CpmDecoder::MaxOutput);
    const FT m_fileType;
};

#endif
