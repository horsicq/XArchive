# U3 RZIP functional translation

U3 archive catalogue zero-based index **390**, exact name **RZIP**, VMT **005ef2b8**. Original executable SHA-256: `5cbca424fb83e7b962a255ece35693cfc2791a7e365617d50f5e73475563a59b`.

The new Qt/C++ implementation translates the recovered container and token behavior, replacing decompiler-inferred pointers, callbacks, allocation, and stream state with typed, bounded native operations. No U3 executable or corpus program was run. It is not a byte-for-byte recreation of U3 implementation details.

| U3 VA | Recovered responsibility | Native location |
|---|---|---|
| 005ef330 | RZIP magic and reserved-header recognition | parseHeader |
| 005efbc0 | 24-byte header, advertised size, one output member | parseHeader / XRzipArchive |
| 005ef920 | chunk sequence, two empty stream heads, old extra-empty-head compatibility | RzipStreams::open / decode |
| 005ef4c0 | relative linked-block offsets and 13-byte block headers | RzipStreams::load |
| 005ef360 | type 3 STORE / type 4 BZip2 decoding | RzipStreams::load / existing XBZIP2Decoder |
| 005ef5d0 | exact reads crossing blocks of one logical stream | RzipStreams::read |
| 005ef6e0 | literal/copy tokens, distance overlap, zero-length chunk terminator, checksum | decode |
| 00416bb0 | CRC32 update using table 007a9a4c | existing _getCRC32 / EDB88320 table |

Recovered functions are under `F:/utils/U3/src/decompiled/functions/<first-four-VA-digits>/<VA>.c`; machine-readable hashes are in the review artifact `u3_source_identities.json`. The disassembly in `F:/utils/U3/src/disassembly/runtime_ranges.asm` resolves CRC behavior omitted by the decompiler. Each chunk starts CRC at zero and has no final XOR. The output length stores the low 32-bit big-endian word before the high big-endian word; it is not an ordinary 64-bit big-endian field.

Independent corroboration and test oracle: Andrew Tridgell's original [rzip 2.1](https://download.samba.org/pub/rzip/rzip-2.1.tar.gz), SHA-256 `4bb96f4d58ccf16749ed3f836957ce97dbcff3e3ee5fd50266229a48f89815b7`. The retained upstream source is GPL-2.0-or-later with its original COPYING and notices. It is used only by the independent test reference executable; production does not compile or mechanically paste those source files. The reference compiles unmodified stream.c, runzip.c and crc32.c with a small Windows file-I/O shim and existing libbzip2. Source and reference hashes are retained with the test artifacts.

Validated scope: RZIP 2.0/2.1, STORE/BZip2 linked streams, overlapping copies, interleaved blocks, multiple checksummed chunks, and the historical empty-header close workaround. Both stream payloads must be fully consumed; the union of physical blocks must exactly cover each chunk, and the final chunk must end at EOF. All output is staged in a private disk-backed file and only published after complete decoding and checksums.

Deliberate limits: compressed input 256 MiB, logical output 1 GiB, decoded or compressed substream block 16 MiB, at most 100000 blocks/chunks and 1024 historical empty headers per chunk. Lower user entry/aggregate limits and block-memory limits are enforced. These are bounded implementation limits, not claims about the format's maximums. No whole-file checksum exists beyond the verified per-chunk CRC. Recognition and listing validate header metadata; extraction performs full stream validation.
