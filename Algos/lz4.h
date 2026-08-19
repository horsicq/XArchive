/* Compatibility shim.
 *
 * 7-Zip's SquashfsHandler.cpp includes "lz4.h" to decode LZ4-compressed
 * SquashFS data; it uses only LZ4_decompress_safe.  The vendored LZ4 tree has
 * been folded into Algos/lz4declib.cpp, so this forwards to the amalgamation's
 * public surface and keeps the upstream 7-Zip source unmodified.
 */
#ifndef XARCHIVE_LZ4_COMPAT_H
#define XARCHIVE_LZ4_COMPAT_H

#include "lz4declib.h"

#endif  // XARCHIVE_LZ4_COMPAT_H
