/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPKT_H
#define XPKT_H

#include "xarchive.h"

// FidoNet mail packet (FTS-0001 type-2, and the FSC-0039 "type 2+" superset
// that only widens reserved fields).  These are the .PKT bundles a mailer
// exchanges; they carry messages, not files.
//
// Packet header, 58 bytes, all little endian:
//
//     +0x00 u16 origNode     +0x02 u16 destNode
//     +0x04 u16 year         +0x06 u16 month (0-11!)   +0x08 u16 day
//     +0x0a u16 hour         +0x0c u16 minute          +0x0e u16 second
//     +0x10 u16 baud         +0x12 u16 packetType (always 2)
//     +0x14 u16 origNet      +0x16 u16 destNet
//     +0x18 u8  productCode  +0x19 u8  serialNumber
//     +0x1a char password[8]
//     +0x22 u16 qOrigZone    +0x24 u16 qDestZone
//     +0x26 char filler[20]
//
// The format has no magic, so detection is a set of cross-checks rather than a
// signature: the packet type must be 2, the first message must also open with
// type 2, and the first message's orig/dest node and net must repeat the
// packet header's - four independent 16-bit equalities that a random file is
// not going to satisfy.  On top of that the packed date has to be a real one
// (1900..2999, month 0..11, day 1..31, 0..23:0..59:0..59) and destNode,
// origNet and destNet must be non-zero.  This is the same predicate U3 uses;
// it is deliberately strict because the container is headerless.
//
// Messages start at +0x3a and run back to back until a u16 0 (or EOF).  Each
// is a 14-byte fixed header followed by five NUL-terminated strings; see
// Algos/xpktdecoder.h for the layout and for the rendering this class
// publishes, which is what HANDLE_METHOD_PKT produces.  Members are named
// "001.txt", "002.txt", ... in packet order - a packed message has no filename
// of its own.
class XPKT final : public XArchive {
    Q_OBJECT

public:
    struct MESSAGE {
        qint64 nRecordOffset;    // the 14-byte message header
        qint64 nRecordSize;      // through the body's terminating NUL
        qint64 nUncompressedSize;  // size of the rendered text
        quint16 nOrigNode;
        quint16 nDestNode;
        quint16 nOrigNet;
        quint16 nDestNet;
        quint16 nAttribute;
        quint16 nCost;
        QString sFileName;
    };

    explicit XPKT(QIODevice *pDevice = nullptr);
    ~XPKT() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

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
        qint64 nInputSize;
        qint64 nArchiveSize;
        quint16 nYear;
        quint16 nMonth;
        quint16 nDay;
        quint16 nHour;
        quint16 nMinute;
        quint16 nSecond;
        quint16 nOrigNode;
        quint16 nDestNode;
        quint16 nOrigNet;
        quint16 nDestNet;
        QList<MESSAGE> listMessages;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPKT_H
