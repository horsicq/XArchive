/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpktdecoder.h"

#include <QList>

namespace {
const qint32 PKT_MESSAGE_HEADER_SIZE = 14;
const qint32 PKT_FIELD_COUNT = 5;
const char PKT_LINE_TERMINATOR = '\r';

const char *const PKT_LABELS[PKT_FIELD_COUNT] = {"Date    = ", "To      = ", "From    = ", "Subject = ", nullptr};

// Splits a message record into its five NUL-terminated strings.  Returns false
// when any of them is unterminated, which is what makes a truncated packet fail
// instead of rendering half a message.
bool pktSplit(const QByteArray &baRecord, QList<QByteArray> *pListFields)
{
    if (!pListFields) return false;
    pListFields->clear();
    if (baRecord.size() < PKT_MESSAGE_HEADER_SIZE) return false;
    qint32 nPosition = PKT_MESSAGE_HEADER_SIZE;
    for (qint32 i = 0; i < PKT_FIELD_COUNT; ++i) {
        const qint32 nEnd = baRecord.indexOf('\0', nPosition);
        if (nEnd < 0) return false;
        pListFields->append(baRecord.mid(nPosition, nEnd - nPosition));
        nPosition = nEnd + 1;
    }
    // The record must end exactly on the body's terminator; anything left over
    // means the caller measured the record wrong.
    return nPosition == baRecord.size();
}
}  // namespace

qint64 XPKTDecoder::renderedSize(const QByteArray &baRecord)
{
    QList<QByteArray> listFields;
    if (!pktSplit(baRecord, &listFields)) return -1;
    qint64 nResult = 0;
    for (qint32 i = 0; i < PKT_FIELD_COUNT; ++i) {
        if (PKT_LABELS[i]) nResult += qint64(qstrlen(PKT_LABELS[i]));
        nResult += listFields.at(i).size();
        nResult += 1;  // line terminator
    }
    nResult += 1;  // the blank line between the headers and the body
    return nResult;
}

bool XPKTDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;
    QList<QByteArray> listFields;
    if (!pktSplit(baPacked, &listFields)) return false;

    QByteArray baResult;
    if ((nUncompressedSize > 0) && (nUncompressedSize < 0x7fffffff)) {
        baResult.reserve(qint32(nUncompressedSize));
    }
    for (qint32 i = 0; i < PKT_FIELD_COUNT - 1; ++i) {
        baResult.append(PKT_LABELS[i]);
        baResult.append(listFields.at(i));
        baResult.append(PKT_LINE_TERMINATOR);
    }
    // Blank line, then the body verbatim (it keeps its own embedded CRs).
    baResult.append(PKT_LINE_TERMINATOR);
    baResult.append(listFields.at(PKT_FIELD_COUNT - 1));
    baResult.append(PKT_LINE_TERMINATOR);

    if ((nUncompressedSize >= 0) && (qint64(baResult.size()) != nUncompressedSize)) return false;
    *pbaUnpacked = baResult;
    return true;
}
