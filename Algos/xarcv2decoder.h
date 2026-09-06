/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARCV2DECODER_H
#define XARCV2DECODER_H

#include "xbinary.h"

// Eschalon Setup ARCV 2.00 ships a plain writer and two "scrambled" ones.  A
// scrambled writer runs a prefix-XOR filter over the whole member payload
// before it is stored, and the container carries no flag that tells any of
// them apart: the archive headers are byte-identical and all seven block
// dwords overlap.  XARCV2 therefore probes the archive once and applies the
// result archive-wide.  A stored member needs only this filter; a compressed
// member needs it in front of the ordinary ARCV LZHUF decoder.
class XARCV2Decoder {
public:
    // The filter is out[i] = in[i] XOR out[i - 1]; only the value standing in
    // for out[-1] separates the two scrambled writers, and it is the authoring
    // tool's edition -- not anything in the archive -- that picks it.  The
    // Release Edition seeds with 0x56, the Trial Edition with 0xab, and the
    // seed is uniform across an archive.  Proof the two are the same codec and
    // not two encodings: CTL3DV2.DLL is stored 0x56-scrambled in one archive
    // and 0xab-scrambled in another, and the two packed payloads differ in
    // exactly one byte -- the first, which is the only place the seed lands.
    //
    // The seed is the ONLY degree of freedom, and it moves the whole output:
    // descrambling with seed s equals descrambling with seed 0 XOR s on every
    // byte.  So a wrong seed does not degrade gracefully -- it destroys the
    // LZHUF bitstream from the first symbol -- which is what lets the probe in
    // XARCV2::probeScramble tell the seeds apart from the stream alone.
    enum SEED : quint8 {
        SEED_RELEASE = 0x56U,
        SEED_TRIAL = 0xabU
    };

    // out[i] = in[i] XOR out[i - 1], with out[-1] = nSeed.
    static bool descramble(const QByteArray &packed, quint8 nSeed,
                           QByteArray *output);

    // Entry point for a scrambled STORED member, where the filter is the whole
    // codec.  nUncompressedSize must equal the packed size; a mismatch means
    // the caller mixed a compressed member into this path.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       quint8 nSeed, QByteArray *output);
};

#endif  // XARCV2DECODER_H
