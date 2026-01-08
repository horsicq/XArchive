# BZIP2 Decompression Infinite Loop Issue

## Problem Description

**Date Discovered**: November 20, 2025

**Test Failure**: XSevenZip tests hang indefinitely when processing BZIP2-compressed archives during `testArchiveRoundTrip()`.

**Symptoms**:
- Tests pass for LZMA and LZMA2 compression
- Tests hang (infinite loop) on BZIP2 compression
- Last output before hang: "FAILED: Could not extract file test_file_bzip2_10_5925.txt for BZIP2"
- Process must be killed after 60+ seconds

## Root Cause

**File**: `c:\tmp_build\qt5\_mylibs\XArchive\Algos\xbzip2decoder.cpp`
**Function**: `bool XBZIP2Decoder::decompress()`
**Lines**: 77-99

### Problematic Code

```cpp
// If we have input or previous buffered data, decompress
if (strm.avail_in > 0 || bReadMore == false) {
    strm.total_in_hi32 = 0;
    strm.total_in_lo32 = 0;
    strm.total_out_hi32 = 0;
    strm.total_out_lo32 = 0;
    strm.avail_out = N_BUFFER_SIZE;
    strm.next_out = bufferOut;
    ret = BZ2_bzDecompress(&strm);

    if ((ret != BZ_STREAM_END) && (ret != BZ_OK)) {
        break;
    }

    qint32 nTemp = N_BUFFER_SIZE - strm.avail_out;

    if (nTemp > 0) {
        if (!XBinary::_writeDevice((char *)bufferOut, nTemp, pDecompressState)) {
            ret = BZ_MEM_ERROR;
            break;
        }
    }
} else {
    // No input and nothing to read
    ret = BZ_MEM_ERROR;
    break;
}
```

### The Problem

The condition `if (strm.avail_in > 0 || bReadMore == false)` has a logic error:

1. When all input has been read (`bReadMore = false`)
2. And the buffer is empty (`strm.avail_in == 0`)
3. The condition `bReadMore == false` is `true`
4. So the if-statement executes
5. `BZ2_bzDecompress()` is called with no input data (`strm.avail_in == 0`)
6. The decompressor may not return `BZ_STREAM_END` without data
7. Loop continues indefinitely

**Correct Logic**: Only decompress when there is actual input data available:
- `strm.avail_in > 0` → Decompress buffered data
- `strm.avail_in == 0 && bReadMore == true` → Read more data first
- `strm.avail_in == 0 && bReadMore == false` → **Should exit loop, not decompress!**

## Proposed Fix

### Option 1: Change Loop Condition (Recommended)

```cpp
// If we have input data, decompress it
if (strm.avail_in > 0) {
    strm.total_in_hi32 = 0;
    strm.total_in_lo32 = 0;
    strm.total_out_hi32 = 0;
    strm.total_out_lo32 = 0;
    strm.avail_out = N_BUFFER_SIZE;
    strm.next_out = bufferOut;
    ret = BZ2_bzDecompress(&strm);

    if ((ret != BZ_STREAM_END) && (ret != BZ_OK)) {
        break;
    }

    qint32 nTemp = N_BUFFER_SIZE - strm.avail_out;

    if (nTemp > 0) {
        if (!XBinary::_writeDevice((char *)bufferOut, nTemp, pDecompressState)) {
            ret = BZ_MEM_ERROR;
            break;
        }
    }
} else if (!bReadMore) {
    // No more data to read and buffer is empty - check if stream ended properly
    if (ret == BZ_OK) {
        // Stream didn't end cleanly - this is an error
        ret = BZ_UNEXPECTED_EOF;
    }
    break;
}
```

### Option 2: Add Safety Check Before Decompression

```cpp
// Only decompress if we have input data available
if (strm.avail_in > 0) {
    strm.total_in_hi32 = 0;
    strm.total_in_lo32 = 0;
    strm.total_out_hi32 = 0;
    strm.total_out_lo32 = 0;
    strm.avail_out = N_BUFFER_SIZE;
    strm.next_out = bufferOut;
    ret = BZ2_bzDecompress(&strm);

    if ((ret != BZ_STREAM_END) && (ret != BZ_OK)) {
        break;
    }

    qint32 nTemp = N_BUFFER_SIZE - strm.avail_out;

    if (nTemp > 0) {
        if (!XBinary::_writeDevice((char *)bufferOut, nTemp, pDecompressState)) {
            ret = BZ_MEM_ERROR;
            break;
        }
    }
} else {
    // No input data available and no more to read
    if (!bReadMore) {
        // End of input - exit loop
        break;
    }
    // Otherwise continue loop to read more data
}
```

## Testing Requirements

After applying the fix, verify:

1. **BZIP2 Decompression Works**: 
   ```powershell
   # Run the full test suite
   .\FormatsTestsXSevenZip.exe
   ```

2. **Specific Test**: Run `testArchiveRoundTrip()` which creates and extracts BZIP2 archives

3. **No Timeout**: Tests should complete in < 10 seconds (currently hangs at 60+ seconds)

4. **All Compression Methods Pass**:
   - LZMA ✓
   - LZMA2 ✓
   - BZIP2 ✗ → ✓ (after fix)
   - DEFLATE
   - PPMD
   - STORE

## Related Code

**Similar Implementations** (for reference):
- `xlzmadecoder.cpp` - LZMA decompression (working correctly)
- `xdeflatedecoder.cpp` - DEFLATE decompression (working correctly)

**Test File**: `c:\tmp_build\qt5\_mylibs\Formats\tests\XSevenZip\test_xsevenzip.cpp`
- Line ~650-750: `testArchiveRoundTrip()` method
- Creates test files, compresses with system 7z, then decompresses with XSevenZip

## Impact

**Severity**: High - Blocks all BZIP2 decompression in 7z archives

**Affected Operations**:
- Extracting BZIP2-compressed files from 7z archives
- Any solid 7z archive using BZIP2 compression
- Archive validation/testing operations

**Not Affected**:
- LZMA/LZMA2 decompression (working)
- DEFLATE decompression (working)
- Pure BZIP2 files (not tested, but likely affected)

## Implementation Notes

1. The fix should preserve the existing cancellation check via `XBinary::isPdStructStopped(pPdStruct)`
2. Ensure proper cleanup with `BZ2_bzDecompressEnd()` in all exit paths
3. Consider adding debug logging to track decompression progress
4. Test with various BZIP2 archive sizes and compression levels

## History

- **2025-11-20**: Issue discovered during XSevenZip test suite execution
- **Root cause**: Logic error in input availability check
- **Fix pending**: Awaiting code modification and validation
