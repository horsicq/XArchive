# U3 ARC hash-table decoder port

Source binary: `F:/utils/U3/U3.unp.exe`, SHA-256
`5cbca424fb83e7b962a255ece35693cfc2791a7e365617d50f5e73475563a59b`.
Evidence: `F:/utils/U3/src/decompiled/functions` and
`F:/utils/U3/src/disassembly/runtime_ranges.asm`. No source binary was executed.
Catalog archive index **35** (zero-based; display row **36**), exact name **ARC**,
VMT `004e97f8`. Recognition `004ea1e0 -> 004e98c0`; record traversal
`004ea200 -> 004e9db0`; method dispatch `004e9a20`.

| U3 VA | Reconstructed behavior in `xarcdecoder.cpp` |
| --- | --- |
| `004e86d0` | Method 5: fixed 12-bit old-hash LZW, raw output |
| `004e8730` | Method 6: old-hash LZW followed by RLE90 |
| `004e8790` | Method 7: new-hash LZW followed by RLE90 |
| `004e8260` | Old and new hash formulas, including signed literal prefix |
| `004e82b0` | Follow collision links to the last occupied slot |
| `004e82f0` | Probe from collision tail plus 101, then linearly wrap |
| `004e8360` | Insert occupied/link/prefix/suffix dictionary entry |
| `004e83a0` | Initialize 4096 entries by hashing all 256 literals |
| `004e83f0` | Reset dictionary capacity and nibble-reader state |
| `004e8430` | Read MSB-first packed 12-bit codes |
| `004e8500`, `004e8530`, `004e8570` | Unwind prefixes and handle next-entry references |

These codes identify **hash-table slots**, so methods 5/6/7 cannot use the
sequential-code dictionary of methods 8/9. Old hash:
`((((prefix + suffix) & 0xffff) | 0x800) ** 2 modulo 2^32) >> 6 & 0xfff`.
New hash: `(prefix + suffix) * 0x3ae1 & 0xfff`.
Literal prefix is signed 16-bit `-1`. The apparently void decompilation of
`004e82b0` is misleading: assembly returns the collision tail in AX. This
return value is necessary to reconstruct the on-disk code assignments.

Global U3 arrays are local per-decode state. Bounds, cancellation and cycle
checks reject failed decoding; the existing XArchive size, CRC and output
budget contract remains responsible for successful publication. We do not
reproduce unchecked U3 memory accesses or silent error paths.

`sample-0136` (`crunch.arc`) supplies methods 5, 6 and 7 with separate stored
CRC-16 and output sizes. The Python trace and C++ validation results are in
`F:/ownCloud/_build/xfileunpacker/u3-handler-port/arc/`. Methods 3/4/8/9 remain
on the existing decoders and are included in regression validation.

The ARC **0x7f Unix compress extension is a separate compatibility addition**;
U3's method mask does not recognize it. It reuses the existing group-aligned
LZW reader with flag-prefixed width 9 through 16 and block-mode reset, without
RLE90. Fixtures 0134/0135 and expected bytes are from the pinned
[newtua-formats source](https://github.com/new-the-unarchiver/newtua-formats/blob/b4c4582d13bbd8a692238875f75e1e4b21177929/crates/newtua-dos/src/arc.rs).
Method 8 retains its original flag/width contract.
