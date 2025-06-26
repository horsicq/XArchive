#ifndef XRAR1DECODER_H
#define XRAR1DECODER_H

#include <QtCore>
#include <QIODevice>

// RAR1 stream structure
struct RAR1_stream {
    char *next_in;
    qint64 avail_in;
    qint64 total_in;
    char *next_out;
    qint64 avail_out;
    qint64 total_out;
};

class XRar1Decoder
{
public:
    XRar1Decoder();
    ~XRar1Decoder();

    // Initialize the decoder
    int decodeInit(RAR1_stream *strm);

    // Decode data from the stream
    int decode(RAR1_stream *strm);

    // End the decoding process
    int decodeEnd(RAR1_stream *strm);
};

#endif // XRAR1DECODER_H
