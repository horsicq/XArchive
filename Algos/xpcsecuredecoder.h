/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPCSECUREDECODER_H
#define XPCSECUREDECODER_H

#include "xbinary.h"

// Central Point PCSECURE (PC Tools 5.x / 6.x / 7.x) protected-file codec.
//
// The member payload is DES-ECB encrypted and then, optionally, LZW-compressed
// UNDER the encryption - so the order on the way out is decrypt first, inflate
// second.  Both stages come straight from U3.unp.exe: FUN_006ef350 (the block
// cipher), FUN_006ef4f0 (the key schedule) and FUN_004c55f0 (the LZW engine
// EA's ".PEA" members share, run with a different configuration).
//
// DES, with two deviations from the textbook that both matter:
//
//   * the ROUND COUNT is a header field (0..16), not a constant.  The header
//     itself is always decrypted with 16 rounds (PCT7 falls back to 3), but
//     the payload uses whatever the header's +0x0c word says - the corpus has
//     both 16 and 2.  Only the first `rounds` subkeys are used, in reverse
//     order, exactly as a shortened decrypt would.
//   * when the round count is BELOW 3 the initial and final permutations are
//     SKIPPED entirely (U3's FUN_006ef350 replaces them with a plain 32-bit
//     endian swap, which is the identity once the halves are treated as
//     big-endian words).  Running standard IP/FP at 2 rounds decodes three of
//     the four extractable corpus files into garbage, so this is not cosmetic.
//
// Everything else is stock DES: PC-1, the 1/1/2... shift schedule, PC-2, the
// E expansion, the eight standard S-boxes and P.
//
// LZW stage (only when the header's flag word has bit 0 set): 12-to-14 bit
// LSB-first codes, width starting at 9 and widened by the reader before each
// fetch once maxCodeForWidth < nextFreeCode (no early change), CLEAR = 0x100,
// NO end-of-stream code, first assignable code 0x101, table capped at 16384.
// The stream opens with a bare code that is emitted as a literal and becomes
// the initial previous code, and every CLEAR restarts that same opening.
//
// Validated byte-exact against U3's own extraction over the whole PC Secure
// corpus: 4 of the 6 files (the other two are user-password protected, carry
// none of U3's four built-in keys, and U3 reports them as errors too).
class XPCSecureDecoder {
public:
    enum {
        PC_SECURE_BLOCK_SIZE = 8,
        PC_SECURE_MAX_ROUNDS = 16,
        // 8 key bytes | u8 rounds | u8 flags | u32 LE compressed size.
        PC_SECURE_PROPERTY_SIZE = 14
    };

    // U3's four built-in keys, in the order it tries them.  They are the
    // product keys PCSECURE uses when no user password was set; the bytes are
    // the little-endian image of the quad words U3 passes.
    static const quint64 *builtinKeys();
    static qint32 builtinKeyCount();

    // Runs the 68-byte header through the cipher with nRounds rounds and
    // reports whether the "SeaHawks" verifier came out.  On success the
    // decrypted header (with U3's byte-order fixups already applied) is
    // written to *pbaHeader.
    static bool tryHeader(const QByteArray &baHeader, quint64 nKey,
                          qint32 nRounds, QByteArray *pbaHeader);

    // Whole-payload entry point.  baProperty must be PC_SECURE_PROPERTY_SIZE
    // bytes as described above; baPacked is the raw member stream starting at
    // file offset 68.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       const QByteArray &baProperty, QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    XPCSecureDecoder() = delete;
};

#endif  // XPCSECUREDECODER_H
