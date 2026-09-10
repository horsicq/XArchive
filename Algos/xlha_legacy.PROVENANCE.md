# LHA legacy decoder provenance

Source: [Lhasa](https://github.com/fragglet/lhasa), commit
`75ed83559f23e9538e0045c62f53f77ab03d03d6`, ISC license. The copyright and
permission notices are retained in every adapted source. `xlha_legacy.LICENSE`
contains the project's license inventory.

The `xlha_legacy_{lzs,lz5,lhx,lk7,pm1,pm2}_p.cpp` files correspond to
`lib/{lzs,lz5,lhx,lk7,pm1,pm2}_decoder.c`. The four `.inc` files correspond to
`lib/bit_stream_reader.c`, `lib/pma_common.c`, `lib/tree_decode.c` and
`lib/lh_new_decoder.c`. The six translation units are compiled as C++; the `.inc`
files are private implementation includes, not independent translation units.

Local adaptations:

- Renamed files, private adapter types, and exported decoder descriptors to
 the `xlha_legacy` prefix. Each codec retains its own static helper symbols.
- Built as C++ rather than C, so that XArchive has no C translation units of its
 own. The only source change this needed was an explicit cast at the 13 places
 where the upstream C relied on an implicit `void *` conversion
 (`LHAxxxDecoder *decoder = (LHAxxxDecoder *) data;`). `xlha_legacy_p.h`
 correspondingly dropped its `extern "C"` wrapper: the descriptor tables hold
 pointers to static functions in those C++ units, so the table type and the
 functions have to share one language linkage.
- `TreeBuildData.tree_len` is `size_t`, matching the function parameter and
 eliminating an unnecessary narrowing conversion on 64-bit builds.
- PM1's documented virtual zero tail is limited to 16 bytes instead of
 allowing indefinitely repeated zero reads. All ordinary PM1 corpus members
 pass with that allowance.
- `XLZHDecoder::decompressLegacyLha` supplies bounded input, fixed scratch
 storage, cancellation checks, and shared memory/output budget handling.
 Header sizes and CRC16 remain the responsibility of the archive layer.

The parser's level layouts, extended name/path fields, optional common-header
CRC, OS-9/68k `K` correction and LHARK method distinction follow
`lib/lha_file_header.c` / `lib/ext_header.c`. MorphOS comments inside a
NUL-terminated filename are described in
`test/archives/morphos_lha_2717/README.md`.

The the reference implementation reconstruction was consulted as independent behavioral evidence:
`the reference implementation` enumerates LHA header levels 0–3; PMA's `the reference implementation` delegates
to it. `the reference implementation` / `the reference implementation` identify stored PM0, PM2, LZS, LZ5 and
LH1/LH4–7 dispatch. The reference implementation does not provide a mapped PM1/LHX decoder and explicitly
returns unsupported for LH2/LH3. No the reference implementation executable was run and no decompiled
function was copied into these decoder implementations.
