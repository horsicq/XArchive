# U3 Crunch, CPMLZH and Unix Compact reconstruction

The reconstruction basis is the user-provided U3.unp.exe, SHA-256
5cbca424fb83e7b962a255ece35693cfc2791a7e365617d50f5e73475563a59b.
Recovered source is under F:\utils\U3\src. Addresses below are preferred
virtual addresses in that exact image. U3 and corpus programs were never run.

Catalog indices are **zero based**: Crunch 50, CPMLZH 51, Unix Compact 681.
Displayed one-based entry numbers are 51, 52 and 682. Original Delphi
names/types remain unknown; the class and member names are new C++ names.

| U3 VA | Recovered role | C++ implementation |
|---|---|---|
| 004f43f0, 004f45a0 | Crunch recognition/header/dispatch | parseHeader; archive adapter |
| 004f49e0, 004f4bd0 | CPMLZH recognition/header/dispatch | parseHeader; archive adapter |
| 00432950, 004327d0 | Filename, BBS stamp/comment, high-bit attributes | parseHeader |
| 004f4480, 004f4a70 | Decode and little-endian additive16 footer | decode |
| 004f3fc0 | Fixed/variable-width Crunch loop, reset, EOF, special code | Crunch::decode |
| 004f3d70 | MSB code reader, v2 fillers 258/259 | Bits; Crunch::decode |
| 004f3b10 | Initial dictionary/reset | Crunch::reset |
| 004f38c0, 004f38e0 | v2 hash and v1 hashed code-slot selection | Crunch::hash, oldSlot |
| 004f39e0, 004f3c90 | Dictionary insertion/full-table reuse | Crunch::insert, replace |
| 004f3ea0, 004f3f80 | Prefix expansion and first byte | Crunch::emit |
| 004c50b0 | RLE90, count includes the preceding emitted byte | Output::rle |
| 004edae0 | Adaptive Huffman/LZSS stream engine | Crlzh::decode |
| 004ed030, 004ed220, 004ed5a0, 004ed820 | Tree initialization/reconstruction/update/decode | Crlzh |
| 004ed8e0 | Version-selected position coding | Crlzh::position |
| 004ecd90, 004ecef0 | MSB bit/byte input | Bits |
| 006d8800, 006d84b0 | Compact recognition, EOF, zero pad bits | parseHeader; discard decode; Bits::finishedWithZeroPadding |
| 006d8850, 006d8220 | Compact first byte, escape/new literal, duplicate rejection | compact; archive adapter |
| 006d8040, 006d7d00, 006d7b70, 006d7c70 | Compact ordered-frequency tree and insertion | Existing Ancient primitive, below |

At 004f4ac9–004f4b0f, disassembly passes maximum match length 60, stop mode 1
and **RLE mode 0** to CPMLZH's shared engine. The port therefore does not apply
Crunch's RLE stage to CPMLZH. Its ring retains U3's space-initialized 8192-byte
layout, with v1 12-bit and v2 11-bit distance coding.

Crlzh::position generates the canonical tables instead of embedding address
references. Its bytes equal U3 007d14c0[256] and 007d15c0[16].
The reconstruction artifact u3_provenance.json records both tables and
SHA-256 identities for 37 inspected decompiled functions.

## Existing library reuse

Compact's stream flow and strict EOF/padding checks follow U3. The existing
XAncientPrivate::DynamicHuffmanDecoder<258> in xancientdynamichuffman_p.h
represents its equivalent ordered-frequency tree. This is **not a literal
port of U3's raw-pointer/bucket storage**. U3's initial paths are literal 1,
escape 00, EOF 01; branch polarity is inverted at the existing primitive's
input boundary. The port rejects an escape followed by a previously seen byte.

The Ancient helper retains its existing Teemu Suutari copyright/license
notices. No vendor source was modified or recopied. The primary reference is
[pinned Ancient CompactDecompressor.cpp](https://github.com/temisu/ancient/blob/d52dc0c1eec35f14e0da78dd48836ac9542f2f0f/src/CompactDecompressor.cpp).

## Deliberate C++ adaptations

- Dictionaries, trees, bit and RLE state are operation-local. No Delphi globals,
  absolute pointers, inferred object layouts or external callbacks were pasted.
- Packed input is capped at 64 MiB, output at 128 MiB or the lower caller entry,
  aggregate and memory limit. Missing input, impossible codes, dictionary
  cycles, cancellation and allocation failure cause failure.
- Discard measurement validates terminators/checksums and obtains the full
  output size without retaining output. Initialization uses it for exact
  record sizes; Compact recognition uses it instead of accepting just magic.
- Unpacking validates privately, stages under the shared budget, then publishes
  once through existing source-binding and output-lifecycle checks.
- CP/M supports only standard additive16 checksum selector 0. A dangling RLE
  marker is rejected. Optional trailing bytes are accepted only as 0–127 bytes
  completing the final 128-byte CP/M record; padding is **not authenticated
  payload**. Additional records or arbitrary suffixes are rejected.
- Compact has no embedded output checksum. Exact output is independently
  verified against its pinned raw fixture.

## Verification evidence

All eight ordinary corpus fixtures decode. Both full translation units compile
with real MSVC/Qt console flags and /W3 /WX; the codec also passed /W4 /WX.
The private API executable links real console dependencies and the new
class/moc/codec, with no stubs or second full project build.

**81/81 API checks pass**: native/deprecated extraction, exact output accounting,
entry/aggregate/memory ceilings, cancellation, CP/M unpadded streams and bad
footers, Compact truncation/trailing bytes, and a four-byte Compact positive
with its zero-padding negative.

CPMLZH 0372–0374 match the pinned
[80un decoder](https://github.com/avwohl/80un/blob/132368f413be79e27f7d17c3248a1db6c5555b1f/src/un80/crlzh.py)
byte-for-byte and match stored checksums. Compact 1069 matches Ancient's
test_C1.raw: 152089 bytes, SHA-256
7467306ee0feed4971260f3c87421154a05be571d944e9cb021a5713700c38f0.

Crunch 0368–0371 reach explicit EOF and match all four stored additive16
checksums. They are **not claimed as independently verified against 80un**:
its pinned Python Crunch output differs in every case and fails those stored
checksums (v1 gives empty output). That incomplete decoder is not an acceptance
oracle for the port.

The reconstruction artifacts expected_outputs.json and
prototype/archive-results/results.json retain names, sizes, hashes, consumed
extents, padding and evidence levels. Root owns final integrated CLI
verification. No text conversion or execution of extracted programs occurs.
