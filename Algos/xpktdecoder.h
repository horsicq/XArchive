/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPKTDECODER_H
#define XPKTDECODER_H

#include <QByteArray>
#include <QtGlobal>

// FidoNet mail packet (FTS-0001 type-2 / FSC-0039 "type 2+") message renderer.
//
// A .PKT carries no files: it is a 58-byte packet header followed by packed
// messages, each of which is a 14-byte fixed header
//
//     +0x00 u16 messageType (always 2)
//     +0x02 u16 origNode      +0x04 u16 destNode
//     +0x06 u16 origNet       +0x08 u16 destNet
//     +0x0a u16 attribute     +0x0c u16 cost
//
// followed by five NUL-terminated strings: date/time, to-user, from-user,
// subject and the message body (which carries its own embedded CR line
// separators and the "\x01"-prefixed kludge lines).
//
// There is nothing to decompress; what a reader has to reproduce is the
// rendering, because that is the only thing a packet can be extracted "as".
// The reference implementation lays out
//
//     "Date    = " date    CR
//     "To      = " to      CR
//     "From    = " from    CR
//     "Subject = " subject CR
//                          CR
//     body                 CR
//
// with a bare CR (0x0d) as the line terminator throughout - not CR LF - which
// matches FidoNet's own line convention and keeps the body bytes verbatim.
//
// baPacked is exactly one message record: the 14 header bytes through the
// terminating NUL of the body.
class XPKTDecoder {
public:
    // Renders one packed message.  nUncompressedSize is the size XPKT
    // published for the member; the call fails if the rendering does not
    // produce exactly that many bytes.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pbaUnpacked);

    // Byte length the rendering of baRecord will have, or -1 when the record
    // is malformed.  XPKT uses this while listing so the published size and
    // the decoded size can never disagree.
    static qint64 renderedSize(const QByteArray &baRecord);
};

#endif  // XPKTDECODER_H
