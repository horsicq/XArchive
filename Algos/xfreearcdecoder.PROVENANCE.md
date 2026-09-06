# Native FreeArc method-chain decoder provenance

The `XFreeArcDecoder` adapter implements the observed FreeArc chain subset. Container parsing, block/member CRC validation, filenames, solid slicing, output staging, and helper fallback belong to `XFreeArcNative` and its caller.

## Recovered U3 identity and functions

U3 archive entry **268, FreeArc**, and SFX entry **72, SFX FreeArc**, share VMT **00595f98**. These are zero-based catalogue indices from `F:/utils/U3/src/recovered_data/format_catalog.json`. A shared decoder does not by itself prove every SFX/container variant.

| Native code | Recovered U3 VA | Translation and adaptation |
| --- | --- | --- |
| method parsing / reverse chain | 00585cf0, 00586150, 00595340, 00595ce0 | Parse the supported method subset and decode stages in reverse order; use bounded sequential byte arrays instead of U3 pipes/worker streams. |
| storing | 00594de0 | Exact byte transfer. |
| raw LZMA properties / decode | 00583840, 00594e90, 00438400, 004382e0 | Reuse existing `XLZMADecoder` and LZMA SDK; construct the five raw properties from method text. |
| REP | 00584c70, 00587580, 00587200, 005871e0 | Translate LE32 window/block/table/literal framing and overlapping history copies. Previously emitted bounded output supplies history instead of an allocated ring. |
| DELTA | 00583e60, 00587190, 00586dd0, 00586c00, 00586cc0, 00586c40 | Translate framed data/tables, type bits, immutable-column unshuffle, and little-endian cumulative addition with carry reset at immutable columns. |
| EXE | 00584550, 00438a20, 00438670, 00438650 | Translate 64 KiB stream/suffix behavior and adapt the matching public-domain Bra86 state machine. The final suffix of at most five bytes is passed unchanged. |

The implementation was written as typed C++, with explicit extents, cancellation, EOF, output, and memory bounds. It does not mechanically include decompiler pseudocode. U3 was read as data; this work did not execute U3 or archive member programs.

## Primary reference source and notices

Reference tree: [FreeArc original-source mirror](https://github.com/mirror/freearc/tree/71f3ab36df26401fff4301b4c8600a31a90d8da9), pinned commit `71f3ab36df26401fff4301b4c8600a31a90d8da9`.

- `Compression/REP/rep.cpp` and `C_REP.cpp`: consulted to corroborate framing/options; the native REP loop was translated from the recovered U3 functions. No upstream REP compilation unit is incorporated in production.
- `Compression/Delta/Delta.cpp` and `C_Delta.cpp`: corroborate the recovered table representation and arithmetic. The native implementation is the FreeArc decompression portion only. The upstream notice is retained below and in the production source.
- `Compression/LZMA2/C/Bra86.c`: the EXE conversion state machine is adapted from this source and cross-checked against U3. Notice: **Bra86.c -- Converter for x86 code (BCJ), 2008-10-04, Igor Pavlov, Public domain**. The unchanged upstream C file is compiled separately for ordinary encoding/decoding byte-reference tests, not added to production build lists.
- Existing XFU `XLZMADecoder`/LZMA SDK are reused under their existing source notices; this port adds no replacement LZMA implementation.

DELTA notice, retained verbatim:

> Delta: binary tables preprocessor v1.51 (c) Bulat.Ziganshin@gmail.com 2013-09-18
> All rights reserved. You can for free use decompression part of the algorithm for decompression of FreeArc archives. For any other usage ask me for the license.

The new interface and framing glue use the project copyright; that does not replace the DELTA algorithm notice or the Bra86 public-domain provenance. No DELTA compressor is shipped or copied into tests. Generated DELTA fixtures independently form specified numeric row differences and byte-column groups.

Exact downloaded source URLs, sizes, and SHA-256 hashes are in the work artifact `freearc-codec/upstream/provenance.json`.

## Tested scope and limits

Supported methods are `storing`, raw `lzma`, `rep`, `exe`, and `delta`, with at most 16 stages and 1024 method-text bytes. Known LZMA encoder options are accepted conservatively; unknown methods/options return unsupported for caller fallback. REP/DELTA accept no option or one bounded buffer-size option. LZMA dictionaries are at most 64 MiB, and lc+lp is at most 4. Each compressed input and each output stage is bounded to 256 MiB, with lower caller caps enforced. A composed chain can need an intermediate stage larger than its final payload.

`memoryRequired` conservatively returns caller input capacity + six times the selected maximum stage size + maximum LZMA dictionary/probability/I/O workspace + 64 KiB overhead. The container separately charges its metadata and retained buffers. Stages are sequential; temporary DELTA block/shuffle arrays and old allocations during output reserve are covered. A caller can choose a smaller intermediate cap to fit its remaining memory policy. This is a byte-buffer/codec-workspace accounting bound, not a claim about total process resident memory or allocator internals.

REP requires the exact zero-block EOF and rejects trailing bytes. DELTA permits clean EOF only at a frame boundary and rejects truncated following headers. Raw LZMA requires complete bounded input consumption and valid end/size status. No stage publishes partial decoded bytes on failure. The native parser intentionally rejects unknown options, invalid extents, and incomplete framing even where U3's old stream helpers were permissive.

This is a verified method subset, not complete FreeArc codec or format parity. PPMd, TOR, LZP, SREP, MM, TTA, encryption, and other unimplemented chains remain outside this decoder.

## Ordinary byte evidence

ARC3 sample **0724**, `sources/patool/samples/t.freearc.arc`, has solid bytes at offset 31, length 25, and method `rep:1kb+exe+delta+lzma:32kb:normal:16:mc8`. Python's independent raw LZMA decoder produces 32 bytes; DELTA unwraps 24 bytes; EXE leaves those particular bytes unchanged; REP yields ASCII `4242` (four bytes). Original-U3 output evidence belongs to `u3-direct/comparison_review.json` and the parent's immutable CLI verifier.

Generated fixtures exercise real REP overlapping matches and window-boundary references; DELTA mutable/immutable columns, carries, multiple tables and frames, widths 1 through 31; EXE conversion and suffixes around 64 KiB boundaries; and a complete non-noop composed chain. The test harness also checks unsupported options, ordinary truncations/invalid extents, cancellation, output limits, aliasing, and empty streams. Exact executable/source hashes and counts are recorded in the final work report rather than implying exhaustive coverage.

A bounded peer review found no ordinary REP/DELTA/EXE byte or memory-estimate mismatch. Its cancellation observation was resolved by polling PDSTRUCT at each DELTA table, including one-row tables that need no inner shuffle or undiff loop. The unchanged 130-check codec suite passed again after this single-line change. This table-boundary cancellation behavior was source-reviewed; a delayed cancellation callback was not reproduced.
