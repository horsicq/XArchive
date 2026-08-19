/* XArchive amalgamation of 7-Zip 26.01 -- 7-Zip C codecs.
 *
 * 15 upstream translation units folded into one. Code is verbatim;
 * only the file boundaries are gone.
 *
 * Layout: every local header the group needs is emitted once, unconditionally,
 * in dependency order, then the unit bodies with their #includes stripped. That
 * avoids the trap where a header whose only occurrence sits inside an inactive
 * #if never reaches the compiler.
 *
 * Units whose REGISTER_* macro fires at global scope (the *Reg.cpp hashers)
 * are never merged: REGISTER_HASHER emits CreateHasherSpec / g_HasherInfo /
 * g_RegisterHasher with identical names in every file. REGISTER_ARC_* and
 * REGISTER_CODEC are safe -- they fire inside per-format namespaces or mangle
 * by class.
 *
 * 7-Zip is LGPL-2.1-or-later with unRAR restrictions; see licenses/7-Zip.
 *
 * This file is generated -- do not edit by hand.
 */

/* Converted from C to C++ on 2026-08-17 and must stay C++. Several of the
 * internal symbols defined below are declared in no header at all, so their
 * linkage names follow whatever language this file is compiled as; building
 * it as C would not fail here, it would fail much later at link, in whichever
 * unit calls them. Fail now instead.
 */
#ifndef __cplusplus
#error "XArchive: this amalgamation must be compiled as C++, not C. Its contract-free \
internal symbols (LzmaEnc_*, Sha*_UpdateBlocks, Ppmd7/8_UpdateModel, ZSTDv0*_*, ...) have \
no header declaration, so C and C++ builds of these units cannot be mixed."
#endif

/* ================ prologue: headers, dependency order ================ */

/* ---- C/Compiler.h ---- */
/* Compiler.h : Compiler specific defines and pragmas
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_COMPILER_H
#define ZIP7_INC_COMPILER_H

#if defined(__clang__)
# define Z7_CLANG_VERSION  (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif
#if defined(__clang__) && defined(__apple_build_version__)
# define Z7_APPLE_CLANG_VERSION   Z7_CLANG_VERSION
#elif defined(__clang__)
# define Z7_LLVM_CLANG_VERSION    Z7_CLANG_VERSION
#elif defined(__GNUC__)
# define Z7_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#ifdef _MSC_VER
#if !defined(__clang__) && !defined(__GNUC__)
#define Z7_MSC_VER_ORIGINAL _MSC_VER
#endif
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#define Z7_MINGW
#endif

#if defined(__LCC__) && (defined(__MCST__) || defined(__e2k__))
#define Z7_MCST_LCC
#define Z7_MCST_LCC_VERSION (__LCC__ * 100 + __LCC_MINOR__)
#endif

/*
#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
    #define Z7_COMPILER_AVX2_SUPPORTED
  #endif
#endif
*/

// #pragma GCC diagnostic ignored "-Wunknown-pragmas"

#ifdef __clang__
// padding size of '' with 4 bytes to alignment boundary
#pragma GCC diagnostic ignored "-Wpadded"

#if defined(Z7_LLVM_CLANG_VERSION) && (__clang_major__ == 13) \
  && defined(__FreeBSD__)
// freebsd:
#pragma GCC diagnostic ignored "-Wexcess-padding"
#endif

#if defined(Z7_APPLE_CLANG_VERSION) && __clang_major__ >= 21
// warning: function MyAlloc might be an allocator wrapper
// clang in xcode: clang 21.0.0
#pragma GCC diagnostic ignored "-Wallocator-wrappers"
#endif

#if __clang_major__ >= 16
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#endif

#if __clang_major__ == 13
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 16)
// cheri
#pragma GCC diagnostic ignored "-Wcapability-to-integer-cast"
#endif
#endif

#if __clang_major__ == 13
  // for <arm_neon.h>
  #pragma GCC diagnostic ignored "-Wreserved-identifier"
#endif

#endif // __clang__

#if defined(__clang__) && __clang_major__ >= 16
// #pragma GCC diagnostic ignored "-Wcast-function-type-strict"
#define Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION \
  _Pragma("GCC diagnostic ignored \"-Wcast-function-type-strict\"")
#else
#define Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION
#endif

typedef void (*Z7_void_Function)(void);
#if defined(__clang__) || defined(__GNUC__)
#define Z7_CAST_FUNC_C  (Z7_void_Function)
#elif defined(_MSC_VER) && _MSC_VER > 1920
#define Z7_CAST_FUNC_C  (void *)
// #pragma warning(disable : 4191) // 'type cast': unsafe conversion from 'FARPROC' to 'void (__cdecl *)()'
#else
#define Z7_CAST_FUNC_C
#endif
/*
#if (defined(__GNUC__) && (__GNUC__ >= 8)) || defined(__clang__)
  // #pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
*/
#ifdef __GNUC__
#if defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40000) && (Z7_GCC_VERSION < 70000)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#endif


#ifdef _MSC_VER

  #ifdef UNDER_CE
    #define RPC_NO_WINDOWS_H
    /* #pragma warning(disable : 4115) // '_RPC_ASYNC_STATE' : named type definition in parentheses */
    #pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
    #pragma warning(disable : 4214) // nonstandard extension used : bit field types other than int
  #endif

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif

// == 1200 : -O1 : for __forceinline
// >= 1900 : -O1 : for printf
#pragma warning(disable : 4710) // function not inlined

#if _MSC_VER < 1900
// winnt.h: 'Int64ShllMod32'
#pragma warning(disable : 4514) // unreferenced inline function has been removed
#endif
    
#if _MSC_VER < 1300
// #pragma warning(disable : 4702) // unreachable code
// Bra.c : -O1:
#pragma warning(disable : 4714) // function marked as __forceinline not inlined
#endif

/*
#if _MSC_VER > 1400 && _MSC_VER <= 1900
// strcat: This function or variable may be unsafe
// sysinfoapi.h: kit10: GetVersion was declared deprecated
#pragma warning(disable : 4996)
#endif
*/

#if _MSC_VER > 1200
// -Wall warnings

#pragma warning(disable : 4711) // function selected for automatic inline expansion
#pragma warning(disable : 4820) // '2' bytes padding added after data member

#if _MSC_VER >= 1400 && _MSC_VER < 1920
// 1400: string.h: _DBG_MEMCPY_INLINE_
// 1600 - 191x : smmintrin.h __cplusplus'
// is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable : 4668)

// 1400 - 1600 : WinDef.h : 'FARPROC' :
// 1900 - 191x : immintrin.h: _readfsbase_u32
// no function prototype given : converting '()' to '(void)'
#pragma warning(disable : 4255)
#endif

#if _MSC_VER >= 1914
// Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#pragma warning(disable : 5045)
#endif

#endif // _MSC_VER > 1200
#endif // _MSC_VER


#if defined(__clang__) && (__clang_major__ >= 4)
  #define Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
    _Pragma("clang loop unroll(disable)") \
    _Pragma("clang loop vectorize(disable)")
  #define Z7_ATTRIB_NO_VECTORIZE
#elif defined(__GNUC__) && (__GNUC__ >= 5) \
    && (!defined(Z7_MCST_LCC_VERSION) || (Z7_MCST_LCC_VERSION >= 12610))
  #define Z7_ATTRIB_NO_VECTORIZE __attribute__((optimize("no-tree-vectorize")))
  // __attribute__((optimize("no-unroll-loops")));
  #define Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
#elif defined(_MSC_VER) && (_MSC_VER >= 1920)
  #define Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
    _Pragma("loop( no_vector )")
  #define Z7_ATTRIB_NO_VECTORIZE
#else
  #define Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  #define Z7_ATTRIB_NO_VECTORIZE
#endif

#if defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1920)
  #define Z7_PRAGMA_OPTIMIZE_FOR_CODE_SIZE _Pragma("optimize ( \"s\", on )")
  #define Z7_PRAGMA_OPTIMIZE_DEFAULT       _Pragma("optimize ( \"\", on )")
#else
  #define Z7_PRAGMA_OPTIMIZE_FOR_CODE_SIZE
  #define Z7_PRAGMA_OPTIMIZE_DEFAULT
#endif



#if defined(MY_CPU_X86_OR_AMD64) && ( \
       defined(__clang__) && (__clang_major__ >= 4) \
    || defined(__GNUC__) && (__GNUC__ >= 5))
  #define Z7_ATTRIB_NO_SSE  __attribute__((__target__("no-sse")))
#else
  #define Z7_ATTRIB_NO_SSE
#endif

#define Z7_ATTRIB_NO_VECTOR \
  Z7_ATTRIB_NO_VECTORIZE \
  Z7_ATTRIB_NO_SSE


#if defined(__clang__) && (__clang_major__ >= 8) \
  || defined(__GNUC__) && (__GNUC__ >= 1000) \
  /* || defined(_MSC_VER) && (_MSC_VER >= 1920) */
  // GCC is not good for __builtin_expect()
  #define Z7_LIKELY(x)   (__builtin_expect((x), 1))
  #define Z7_UNLIKELY(x) (__builtin_expect((x), 0))
  // #define Z7_unlikely [[unlikely]]
  // #define Z7_likely [[likely]]
#else
  #define Z7_LIKELY(x)   (x)
  #define Z7_UNLIKELY(x) (x)
  // #define Z7_likely
#endif


#if (defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30600))

#if (Z7_CLANG_VERSION < 130000)
#define Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wreserved-id-macro\"")
#else
#define Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wreserved-macro-identifier\"")
#endif

#define Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER \
  _Pragma("GCC diagnostic pop")
#else
#define Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
#define Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif

#define UNUSED_VAR(x) (void)x;
/* #define UNUSED_VAR(x) x=x; */

#endif

/* ---- C/Precomp.h ---- */
/* Precomp.h -- precompilation file
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_PRECOMP_H
#define ZIP7_INC_PRECOMP_H

/*
  this file must be included before another *.h files and before <windows.h>.
  this file is included from the following files:
    C\*.c
    C\Util\*\Precomp.h   <-  C\Util\*\*.c
    CPP\Common\Common.h  <-  *\StdAfx.h    <-  *\*.cpp

  this file can set the following macros:
    Z7_LARGE_PAGES 1
    Z7_LONG_PATH 1
    Z7_WIN32_WINNT_MIN  0x0500 (or higher) : we require at least win2000+ for 7-Zip
    _WIN32_WINNT        0x0500 (or higher)
    WINVER  _WIN32_WINNT
    UNICODE 1
    _UNICODE 1
*/

// amalgamation: header emitted in prologue

#ifdef _MSC_VER
// #pragma warning(disable : 4206) // nonstandard extension used : translation unit is empty
#if _MSC_VER >= 1912
// #pragma warning(disable : 5039) // pointer or reference to potentially throwing function passed to 'extern "C"' function under - EHc.Undefined behavior may occur if this function throws an exception.
#endif
#endif

/*
// for debug:
#define UNICODE 1
#define _UNICODE 1
#define  _WIN32_WINNT  0x0500  // win2000
#ifndef WINVER
  #define WINVER  _WIN32_WINNT
#endif
*/

#ifndef Z7_LARGE_PAGES
#if !defined(Z7_NO_LARGE_PAGES) && !defined(UNDER_CE)
#define Z7_LARGE_PAGES 1
#endif
#endif

#ifdef _WIN32
/*
  this "Precomp.h" file must be included before <windows.h>,
  if we want to define _WIN32_WINNT before <windows.h>.
*/

#ifndef Z7_LONG_PATH
#ifndef Z7_NO_LONG_PATH
#define Z7_LONG_PATH 1
#endif
#endif

#ifndef Z7_DEVICE_FILE
#ifndef Z7_NO_DEVICE_FILE
// #define Z7_DEVICE_FILE 1
#endif
#endif

// we don't change macros if included after <windows.h>
#ifndef _WINDOWS_

#ifndef Z7_WIN32_WINNT_MIN
  #if defined(_M_ARM64) || defined(__aarch64__)
    // #define Z7_WIN32_WINNT_MIN  0x0a00  // win10
    #define Z7_WIN32_WINNT_MIN  0x0600  // vista
  #elif defined(_M_ARM) && defined(_M_ARMT) && defined(_M_ARM_NT)
    // #define Z7_WIN32_WINNT_MIN  0x0602  // win8
    #define Z7_WIN32_WINNT_MIN  0x0600  // vista
  #elif defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(_M_IA64)
    #define Z7_WIN32_WINNT_MIN  0x0503  // win2003
  // #elif defined(_M_IX86) || defined(__i386__)
  //   #define Z7_WIN32_WINNT_MIN  0x0500  // win2000
  #else // x86 and another(old) systems
    #define Z7_WIN32_WINNT_MIN  0x0500  // win2000
    // #define Z7_WIN32_WINNT_MIN  0x0502  // win2003 // for debug
  #endif
#endif // Z7_WIN32_WINNT_MIN


#ifndef Z7_DO_NOT_DEFINE_WIN32_WINNT
#ifdef _WIN32_WINNT
  // #error Stop_Compiling_Bad_WIN32_WINNT
#else
  #ifndef Z7_NO_DEFINE_WIN32_WINNT
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #define _WIN32_WINNT  Z7_WIN32_WINNT_MIN
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
  #endif
#endif // _WIN32_WINNT

#ifndef WINVER
  #define WINVER  _WIN32_WINNT
#endif
#endif // Z7_DO_NOT_DEFINE_WIN32_WINNT


#ifndef _MBCS
#ifndef Z7_NO_UNICODE
// UNICODE and _UNICODE are used by <windows.h> and by 7-zip code.

#ifndef UNICODE
#define UNICODE 1
#endif

#ifndef _UNICODE
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
#define _UNICODE 1
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif

#endif // Z7_NO_UNICODE
#endif // _MBCS
#endif // _WINDOWS_

// #include "7zWindows.h"

#endif // _WIN32

#endif

/* ---- C/7zTypes.h ---- */
/* 7zTypes.h -- Basic types
: Igor Pavlov : Public domain */

#ifndef ZIP7_7Z_TYPES_H
#define ZIP7_7Z_TYPES_H

#ifdef _WIN32
/* #include <windows.h> */
#else
#include <errno.h>
#endif

#include <stddef.h>

#ifndef EXTERN_C_BEGIN
#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif
#endif

EXTERN_C_BEGIN

#define SZ_OK 0

#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12

#define SZ_ERROR_ARCHIVE 16
#define SZ_ERROR_NO_ARCHIVE 17

typedef int SRes;


#ifdef _MSC_VER
  #define MY_ALIGN_IN_STRUCT(n) __declspec(align(n))
  #if _MSC_VER > 1200
    #define MY_ALIGN(n) MY_ALIGN_IN_STRUCT(n)
  #else
    #define MY_ALIGN(n)
  #endif
#else
  /*
  // C11/C++11:
  #include <stdalign.h>
  #define MY_ALIGN(n) alignas(n)
  */
  #define MY_ALIGN(n) __attribute__ ((aligned(n)))
  #define MY_ALIGN_IN_STRUCT(n) MY_ALIGN(n)
#endif


#ifdef _WIN32

/* typedef DWORD WRes; */
typedef unsigned WRes;
#define MY_SRes_HRESULT_FROM_WRes(x) HRESULT_FROM_WIN32(x)

// #define MY_HRES_ERROR_INTERNAL_ERROR  MY_SRes_HRESULT_FROM_WRes(ERROR_INTERNAL_ERROR)

#else // _WIN32

// #define ENV_HAVE_LSTAT
typedef int WRes;

// (FACILITY_ERRNO = 0x800) is 7zip's FACILITY constant to represent (errno) errors in HRESULT
#define MY_FACILITY_ERRNO  0x800
#define MY_FACILITY_WIN32  7
#define MY_FACILITY_WRes  MY_FACILITY_ERRNO

#define MY_HRESULT_FROM_errno_CONST_ERROR(x) ((HRESULT)( \
          ( (HRESULT)(x) & 0x0000FFFF) \
          | (MY_FACILITY_WRes << 16)  \
          | (HRESULT)0x80000000 ))

#define MY_SRes_HRESULT_FROM_WRes(x) \
  ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : MY_HRESULT_FROM_errno_CONST_ERROR(x))

// we call macro HRESULT_FROM_WIN32 for system errors (WRes) that are (errno)
#define HRESULT_FROM_WIN32(x) MY_SRes_HRESULT_FROM_WRes(x)

/*
#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_ACCESS_DENIED              5L
#define ERROR_NO_MORE_FILES              18L
#define ERROR_LOCK_VIOLATION             33L
#define ERROR_FILE_EXISTS                80L
#define ERROR_DISK_FULL                  112L
#define ERROR_NEGATIVE_SEEK              131L
#define ERROR_ALREADY_EXISTS             183L
#define ERROR_DIRECTORY                  267L
#define ERROR_TOO_MANY_POSTS             298L

#define ERROR_INTERNAL_ERROR             1359L
#define ERROR_INVALID_REPARSE_DATA       4392L
#define ERROR_REPARSE_TAG_INVALID        4393L
#define ERROR_REPARSE_TAG_MISMATCH       4394L
*/

// we use errno equivalents for some WIN32 errors:

#define ERROR_INVALID_PARAMETER     EINVAL
#define ERROR_INVALID_FUNCTION      EINVAL
#define ERROR_ALREADY_EXISTS        EEXIST
#define ERROR_FILE_EXISTS           EEXIST
#define ERROR_PATH_NOT_FOUND        ENOENT
#define ERROR_FILE_NOT_FOUND        ENOENT
#define ERROR_DISK_FULL             ENOSPC
// #define ERROR_INVALID_HANDLE        EBADF

// we use FACILITY_WIN32 for errors that has no errno equivalent
// Too many posts were made to a semaphore.
#define ERROR_TOO_MANY_POSTS        ((HRESULT)0x8007012AL)
#define ERROR_INVALID_REPARSE_DATA  ((HRESULT)0x80071128L)
#define ERROR_REPARSE_TAG_INVALID   ((HRESULT)0x80071129L)

// if (MY_FACILITY_WRes != FACILITY_WIN32),
// we use FACILITY_WIN32 for COM errors:
#define E_OUTOFMEMORY               ((HRESULT)0x8007000EL)
#define E_INVALIDARG                ((HRESULT)0x80070057L)
#define MY_E_ERROR_NEGATIVE_SEEK    ((HRESULT)0x80070083L)

/*
// we can use FACILITY_ERRNO for some COM errors, that have errno equivalents:
#define E_OUTOFMEMORY             MY_HRESULT_FROM_errno_CONST_ERROR(ENOMEM)
#define E_INVALIDARG              MY_HRESULT_FROM_errno_CONST_ERROR(EINVAL)
#define MY_E_ERROR_NEGATIVE_SEEK  MY_HRESULT_FROM_errno_CONST_ERROR(EINVAL)
*/

#define TEXT(quote) quote

#define FILE_ATTRIBUTE_READONLY       0x0001
#define FILE_ATTRIBUTE_HIDDEN         0x0002
#define FILE_ATTRIBUTE_SYSTEM         0x0004
#define FILE_ATTRIBUTE_DIRECTORY      0x0010
#define FILE_ATTRIBUTE_ARCHIVE        0x0020
#define FILE_ATTRIBUTE_DEVICE         0x0040
#define FILE_ATTRIBUTE_NORMAL         0x0080
#define FILE_ATTRIBUTE_TEMPORARY      0x0100
#define FILE_ATTRIBUTE_SPARSE_FILE    0x0200
#define FILE_ATTRIBUTE_REPARSE_POINT  0x0400
#define FILE_ATTRIBUTE_COMPRESSED     0x0800
#define FILE_ATTRIBUTE_OFFLINE        0x1000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x2000
#define FILE_ATTRIBUTE_ENCRYPTED      0x4000

#define FILE_ATTRIBUTE_UNIX_EXTENSION 0x8000   /* trick for Unix */

#endif


#ifndef RINOK
#define RINOK(x) { const int _result_ = (x); if (_result_ != 0) return _result_; }
#endif

#ifndef RINOK_WRes
#define RINOK_WRes(x) { const WRes _result_ = (x); if (_result_ != 0) return _result_; }
#endif

typedef unsigned char Byte;
typedef short Int16;
typedef unsigned short UInt16;

#ifdef Z7_DECL_Int32_AS_long
typedef long Int32;
typedef unsigned long UInt32;
#else
typedef int Int32;
typedef unsigned int UInt32;
#endif


#ifndef _WIN32

typedef int INT;
typedef Int32 INT32;
typedef unsigned int UINT;
typedef UInt32 UINT32;
typedef INT32 LONG;   // LONG, ULONG and DWORD must be 32-bit for _WIN32 compatibility
typedef UINT32 ULONG;

#undef DWORD
typedef UINT32 DWORD;

#define VOID void

#define HRESULT LONG

typedef void *LPVOID;
// typedef void VOID;
// typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
// gcc / clang on Unix  : sizeof(long==sizeof(void*) in 32 or 64 bits)
typedef          long  INT_PTR;
typedef unsigned long  UINT_PTR;
typedef          long  LONG_PTR;
typedef unsigned long  DWORD_PTR;

typedef size_t SIZE_T;

#endif //  _WIN32


#define MY_HRES_ERROR_INTERNAL_ERROR  ((HRESULT)0x8007054FL)


#ifdef Z7_DECL_Int64_AS_long

typedef long Int64;
typedef unsigned long UInt64;

#else

#if (defined(_MSC_VER) || defined(__BORLANDC__)) && !defined(__clang__)
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#else
#if defined(__clang__) || defined(__GNUC__)
#include <stdint.h>
typedef int64_t Int64;
typedef uint64_t UInt64;
#else
typedef long long int Int64;
typedef unsigned long long int UInt64;
// #define UINT64_CONST(n) n ## ULL
#endif
#endif

#endif

#define UINT64_CONST(n) n


#ifdef Z7_DECL_SizeT_AS_unsigned_int
typedef unsigned int SizeT;
#else
typedef size_t SizeT;
#endif

/*
#if (defined(_MSC_VER) && _MSC_VER <= 1200)
typedef size_t MY_uintptr_t;
#else
#include <stdint.h>
typedef uintptr_t MY_uintptr_t;
#endif
*/

typedef int BoolInt;
/* typedef BoolInt Bool; */
#define True 1
#define False 0


#ifdef _WIN32
#define Z7_STDCALL __stdcall
#else
#define Z7_STDCALL
#endif

#ifdef _MSC_VER

#if _MSC_VER >= 1300
#define Z7_NO_INLINE __declspec(noinline)
#else
#define Z7_NO_INLINE
#endif

#define Z7_FORCE_INLINE __forceinline

#define Z7_CDECL      __cdecl
#define Z7_FASTCALL  __fastcall

#else //  _MSC_VER

#if (defined(__GNUC__) && (__GNUC__ >= 4)) \
    || (defined(__clang__) && (__clang_major__ >= 4)) \
    || defined(__INTEL_COMPILER) \
    || defined(__xlC__)
#define Z7_NO_INLINE      __attribute__((noinline))
#define Z7_FORCE_INLINE   __attribute__((always_inline)) inline
#else
#define Z7_NO_INLINE
#define Z7_FORCE_INLINE
#endif

#define Z7_CDECL

#if  defined(_M_IX86) \
  || defined(__i386__)
// #define Z7_FASTCALL __attribute__((fastcall))
// #define Z7_FASTCALL __attribute__((cdecl))
#define Z7_FASTCALL
#elif defined(MY_CPU_AMD64)
// #define Z7_FASTCALL __attribute__((ms_abi))
#define Z7_FASTCALL
#else
#define Z7_FASTCALL
#endif

#endif //  _MSC_VER


/* The following interfaces use first parameter as pointer to structure */

// #define Z7_C_IFACE_CONST_QUAL
#define Z7_C_IFACE_CONST_QUAL const

#define Z7_C_IFACE_DECL(a) \
  struct a ## _; \
  typedef Z7_C_IFACE_CONST_QUAL struct a ## _ * a ## Ptr; \
  typedef struct a ## _ a; \
  struct a ## _


Z7_C_IFACE_DECL (IByteIn)
{
  Byte (*Read)(IByteInPtr p); /* reads one byte, returns 0 in case of EOF or error */
};
#define IByteIn_Read(p) (p)->Read(p)


Z7_C_IFACE_DECL (IByteOut)
{
  void (*Write)(IByteOutPtr p, Byte b);
};
#define IByteOut_Write(p, b) (p)->Write(p, b)


Z7_C_IFACE_DECL (ISeqInStream)
{
  SRes (*Read)(ISeqInStreamPtr p, void *buf, size_t *size);
    /* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
       (output(*size) < input(*size)) is allowed */
};
#define ISeqInStream_Read(p, buf, size) (p)->Read(p, buf, size)

/* try to read as much as avail in stream and limited by (*processedSize) */
SRes SeqInStream_ReadMax(ISeqInStreamPtr stream, void *buf, size_t *processedSize);
/* it can return SZ_ERROR_INPUT_EOF */
// SRes SeqInStream_Read(ISeqInStreamPtr stream, void *buf, size_t size);
// SRes SeqInStream_Read2(ISeqInStreamPtr stream, void *buf, size_t size, SRes errorType);
SRes SeqInStream_ReadByte(ISeqInStreamPtr stream, Byte *buf);


Z7_C_IFACE_DECL (ISeqOutStream)
{
  size_t (*Write)(ISeqOutStreamPtr p, const void *buf, size_t size);
    /* Returns: result - the number of actually written bytes.
       (result < size) means error */
};
#define ISeqOutStream_Write(p, buf, size) (p)->Write(p, buf, size)

typedef enum
{
  SZ_SEEK_SET = 0,
  SZ_SEEK_CUR = 1,
  SZ_SEEK_END = 2
} ESzSeek;


Z7_C_IFACE_DECL (ISeekInStream)
{
  SRes (*Read)(ISeekInStreamPtr p, void *buf, size_t *size);  /* same as ISeqInStream::Read */
  SRes (*Seek)(ISeekInStreamPtr p, Int64 *pos, ESzSeek origin);
};
#define ISeekInStream_Read(p, buf, size)   (p)->Read(p, buf, size)
#define ISeekInStream_Seek(p, pos, origin) (p)->Seek(p, pos, origin)


Z7_C_IFACE_DECL (ILookInStream)
{
  SRes (*Look)(ILookInStreamPtr p, const void **buf, size_t *size);
    /* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
       (output(*size) > input(*size)) is not allowed
       (output(*size) < input(*size)) is allowed */
  SRes (*Skip)(ILookInStreamPtr p, size_t offset);
    /* offset must be <= output(*size) of Look */
  SRes (*Read)(ILookInStreamPtr p, void *buf, size_t *size);
    /* reads directly (without buffer). It's same as ISeqInStream::Read */
  SRes (*Seek)(ILookInStreamPtr p, Int64 *pos, ESzSeek origin);
};

#define ILookInStream_Look(p, buf, size)   (p)->Look(p, buf, size)
#define ILookInStream_Skip(p, offset)      (p)->Skip(p, offset)
#define ILookInStream_Read(p, buf, size)   (p)->Read(p, buf, size)
#define ILookInStream_Seek(p, pos, origin) (p)->Seek(p, pos, origin)


SRes LookInStream_LookRead(ILookInStreamPtr stream, void *buf, size_t *size);
SRes LookInStream_SeekTo(ILookInStreamPtr stream, UInt64 offset);

/* reads via ILookInStream::Read */
SRes LookInStream_Read2(ILookInStreamPtr stream, void *buf, size_t size, SRes errorType);
SRes LookInStream_Read(ILookInStreamPtr stream, void *buf, size_t size);


typedef struct
{
  ILookInStream vt;
  ISeekInStreamPtr realStream;
 
  size_t pos;
  size_t size; /* it's data size */
  
  /* the following variables must be set outside */
  Byte *buf;
  size_t bufSize;
} CLookToRead2;

void LookToRead2_CreateVTable(CLookToRead2 *p, int lookahead);

#define LookToRead2_INIT(p) { (p)->pos = (p)->size = 0; }


typedef struct
{
  ISeqInStream vt;
  ILookInStreamPtr realStream;
} CSecToLook;

void SecToLook_CreateVTable(CSecToLook *p);



typedef struct
{
  ISeqInStream vt;
  ILookInStreamPtr realStream;
} CSecToRead;

void SecToRead_CreateVTable(CSecToRead *p);


Z7_C_IFACE_DECL (ICompressProgress)
{
  SRes (*Progress)(ICompressProgressPtr p, UInt64 inSize, UInt64 outSize);
    /* Returns: result. (result != SZ_OK) means break.
       Value (UInt64)(Int64)-1 for size means unknown value. */
};

#define ICompressProgress_Progress(p, inSize, outSize) (p)->Progress(p, inSize, outSize)



typedef struct ISzAlloc ISzAlloc;
typedef const ISzAlloc * ISzAllocPtr;

struct ISzAlloc
{
  void *(*Alloc)(ISzAllocPtr p, size_t size);
  void (*Free)(ISzAllocPtr p, void *address); /* address can be 0 */
};

#define ISzAlloc_Alloc(p, size) (p)->Alloc(p, size)
#define ISzAlloc_Free(p, a) (p)->Free(p, a)

/* deprecated */
#define IAlloc_Alloc(p, size) ISzAlloc_Alloc(p, size)
#define IAlloc_Free(p, a) ISzAlloc_Free(p, a)





#ifndef MY_offsetof
  #ifdef offsetof
    #define MY_offsetof(type, m) offsetof(type, m)
    /*
    #define MY_offsetof(type, m) FIELD_OFFSET(type, m)
    */
  #else
    #define MY_offsetof(type, m) ((size_t)&(((type *)0)->m))
  #endif
#endif



#ifndef Z7_container_of

/*
#define Z7_container_of(ptr, type, m) container_of(ptr, type, m)
#define Z7_container_of(ptr, type, m) CONTAINING_RECORD(ptr, type, m)
#define Z7_container_of(ptr, type, m) ((type *)((char *)(ptr) - offsetof(type, m)))
#define Z7_container_of(ptr, type, m) (&((type *)0)->m == (ptr), ((type *)(((char *)(ptr)) - MY_offsetof(type, m))))
*/

/*
  GCC shows warning: "perhaps the 'offsetof' macro was used incorrectly"
    GCC 3.4.4 : classes with constructor
    GCC 4.8.1 : classes with non-public variable members"
*/

#define Z7_container_of(ptr, type, m) \
  ((type *)(void *)((char *)(void *) \
  (1 ? (ptr) : &((type *)NULL)->m) - MY_offsetof(type, m)))

#define Z7_container_of_CONST(ptr, type, m) \
  ((const type *)(const void *)((const char *)(const void *) \
  (1 ? (ptr) : &((type *)NULL)->m) - MY_offsetof(type, m)))

/*
#define Z7_container_of_NON_CONST_FROM_CONST(ptr, type, m) \
  ((type *)(void *)(const void *)((const char *)(const void *) \
  (1 ? (ptr) : &((type *)NULL)->m) - MY_offsetof(type, m)))
*/

#endif

#define Z7_CONTAINER_FROM_VTBL_SIMPLE(ptr, type, m) ((type *)(void *)(ptr))

// #define Z7_CONTAINER_FROM_VTBL(ptr, type, m) Z7_CONTAINER_FROM_VTBL_SIMPLE(ptr, type, m)
#define Z7_CONTAINER_FROM_VTBL(ptr, type, m) Z7_container_of(ptr, type, m)
// #define Z7_CONTAINER_FROM_VTBL(ptr, type, m) Z7_container_of_NON_CONST_FROM_CONST(ptr, type, m)

#define Z7_CONTAINER_FROM_VTBL_CONST(ptr, type, m) Z7_container_of_CONST(ptr, type, m)

#define Z7_CONTAINER_FROM_VTBL_CLS(ptr, type, m) Z7_CONTAINER_FROM_VTBL_SIMPLE(ptr, type, m)
/*
#define Z7_CONTAINER_FROM_VTBL_CLS(ptr, type, m) Z7_CONTAINER_FROM_VTBL(ptr, type, m)
*/
#if defined (__clang__) || defined(__GNUC__)
#define Z7_DIAGNOSTIC_IGNORE_BEGIN_CAST_QUAL \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#define Z7_DIAGNOSTIC_IGNORE_END_CAST_QUAL \
  _Pragma("GCC diagnostic pop")
#else
#define Z7_DIAGNOSTIC_IGNORE_BEGIN_CAST_QUAL
#define Z7_DIAGNOSTIC_IGNORE_END_CAST_QUAL
#endif

#define Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR(ptr, type, m, p) \
  Z7_DIAGNOSTIC_IGNORE_BEGIN_CAST_QUAL \
  type *p = Z7_CONTAINER_FROM_VTBL(ptr, type, m); \
  Z7_DIAGNOSTIC_IGNORE_END_CAST_QUAL

#define Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(type) \
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR(pp, type, vt, p)


// #define ZIP7_DECLARE_HANDLE(name)  typedef void *name;
#define Z7_DECLARE_HANDLE(name)  struct name##_dummy{int unused;}; typedef struct name##_dummy *name;


#define Z7_memset_0_ARRAY(a)  memset((a), 0, sizeof(a))

#ifndef Z7_ARRAY_SIZE
#define Z7_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif


#ifdef _WIN32

#define CHAR_PATH_SEPARATOR '\\'
#define WCHAR_PATH_SEPARATOR L'\\'
#define STRING_PATH_SEPARATOR "\\"
#define WSTRING_PATH_SEPARATOR L"\\"

#else

#define CHAR_PATH_SEPARATOR '/'
#define WCHAR_PATH_SEPARATOR L'/'
#define STRING_PATH_SEPARATOR "/"
#define WSTRING_PATH_SEPARATOR L"/"

#endif

#define k_PropVar_TimePrec_0        0
#define k_PropVar_TimePrec_Unix     1
#define k_PropVar_TimePrec_DOS      2
#define k_PropVar_TimePrec_HighPrec 3
#define k_PropVar_TimePrec_Base     16
#define k_PropVar_TimePrec_100ns (k_PropVar_TimePrec_Base + 7)
#define k_PropVar_TimePrec_1ns   (k_PropVar_TimePrec_Base + 9)

EXTERN_C_END

#endif

/*
#ifndef Z7_ST
#ifdef _7ZIP_ST
#define Z7_ST
#endif
#endif
*/

/* ---- C/CpuArch.h ---- */
/* CpuArch.h -- CPU specific code
Igor Pavlov : Public domain */

#ifndef ZIP7_INC_CPU_ARCH_H
#define ZIP7_INC_CPU_ARCH_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/*
MY_CPU_LE means that CPU is LITTLE ENDIAN.
MY_CPU_BE means that CPU is BIG ENDIAN.
If MY_CPU_LE and MY_CPU_BE are not defined, we don't know about ENDIANNESS of platform.

MY_CPU_LE_UNALIGN means that CPU is LITTLE ENDIAN and CPU supports unaligned memory accesses.

MY_CPU_64BIT means that processor can work with 64-bit registers.
  MY_CPU_64BIT can be used to select fast code branch
  MY_CPU_64BIT doesn't mean that (sizeof(void *) == 8)
*/

#if !defined(_M_ARM64EC)
#if  defined(_M_X64) \
  || defined(_M_AMD64) \
  || defined(__x86_64__) \
  || defined(__AMD64__) \
  || defined(__amd64__)
  #define MY_CPU_AMD64
  #ifdef __ILP32__
    #define MY_CPU_NAME "x32"
    #define MY_CPU_SIZEOF_POINTER 4
  #else
    #if defined(__APX_EGPR__) || defined(__EGPR__)
      #define MY_CPU_NAME "x64-apx"
      #define MY_CPU_AMD64_APX
    #else
      #define MY_CPU_NAME "x64"
    #endif
    #define MY_CPU_SIZEOF_POINTER 8
  #endif
  #define MY_CPU_64BIT
#endif
#endif


#if  defined(_M_IX86) \
  || defined(__i386__)
  #define MY_CPU_X86
  #define MY_CPU_NAME "x86"
  /* #define MY_CPU_32BIT */
  #define MY_CPU_SIZEOF_POINTER 4
#endif

#if defined(__SSE2__) \
    || defined(MY_CPU_AMD64) \
    || defined(_M_IX86_FP) && (_M_IX86_FP >= 2)
#define MY_CPU_SSE2
#endif


#if  defined(_M_ARM64) \
  || defined(_M_ARM64EC) \
  || defined(__AARCH64EL__) \
  || defined(__AARCH64EB__) \
  || defined(__aarch64__)
  #define MY_CPU_ARM64
#if   defined(__ILP32__) \
   || defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)
    #define MY_CPU_NAME "arm64-32"
    #define MY_CPU_SIZEOF_POINTER 4
#elif defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 16)
    #define MY_CPU_NAME "arm64-128"
    #define MY_CPU_SIZEOF_POINTER 16
#else
#if defined(_M_ARM64EC)
    #define MY_CPU_NAME "arm64ec"
#else
    #define MY_CPU_NAME "arm64"
#endif
    #define MY_CPU_SIZEOF_POINTER 8
#endif
  #define MY_CPU_64BIT
#endif


#if  defined(_M_ARM) \
  || defined(_M_ARM_NT) \
  || defined(_M_ARMT) \
  || defined(__arm__) \
  || defined(__thumb__) \
  || defined(__ARMEL__) \
  || defined(__ARMEB__) \
  || defined(__THUMBEL__) \
  || defined(__THUMBEB__)
  #define MY_CPU_ARM

  #if defined(__thumb__) || defined(__THUMBEL__) || defined(_M_ARMT)
    #define MY_CPU_ARMT
    #define MY_CPU_NAME "armt"
  #else
    #define MY_CPU_ARM32
    #define MY_CPU_NAME "arm"
  #endif
  /* #define MY_CPU_32BIT */
  #define MY_CPU_SIZEOF_POINTER 4
#endif


#if  defined(_M_IA64) \
  || defined(__ia64__)
  #define MY_CPU_IA64
  #define MY_CPU_NAME "ia64"
  #define MY_CPU_64BIT
#endif


#if  defined(__mips64) \
  || defined(__mips64__) \
  || (defined(__mips) && (__mips == 64 || __mips == 4 || __mips == 3))
  #define MY_CPU_NAME "mips64"
  #define MY_CPU_64BIT
#elif defined(__mips__)
  #define MY_CPU_NAME "mips"
  /* #define MY_CPU_32BIT */
#endif


#if  defined(__ppc64__) \
  || defined(__powerpc64__) \
  || defined(__ppc__) \
  || defined(__powerpc__) \
  || defined(__PPC__) \
  || defined(_POWER)

#define MY_CPU_PPC_OR_PPC64

#if  defined(__ppc64__) \
  || defined(__powerpc64__) \
  || defined(_LP64) \
  || defined(__64BIT__)
  #ifdef __ILP32__
    #define MY_CPU_NAME "ppc64-32"
    #define MY_CPU_SIZEOF_POINTER 4
  #else
    #define MY_CPU_NAME "ppc64"
    #define MY_CPU_SIZEOF_POINTER 8
  #endif
  #define MY_CPU_64BIT
#else
  #define MY_CPU_NAME "ppc"
  #define MY_CPU_SIZEOF_POINTER 4
  /* #define MY_CPU_32BIT */
#endif
#endif


#if   defined(__sparc__) \
   || defined(__sparc)
  #define MY_CPU_SPARC
  #if  defined(__LP64__) \
    || defined(_LP64) \
    || defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    #define MY_CPU_NAME "sparcv9"
    #define MY_CPU_SIZEOF_POINTER 8
    #define MY_CPU_64BIT
  #elif defined(__sparc_v9__) \
     || defined(__sparcv9)
    #define MY_CPU_64BIT
    #if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)
      #define MY_CPU_NAME "sparcv9-32"
    #else
      #define MY_CPU_NAME "sparcv9m"
    #endif
  #elif defined(__sparc_v8__) \
     || defined(__sparcv8)
    #define MY_CPU_NAME "sparcv8"
    #define MY_CPU_SIZEOF_POINTER 4
  #else
    #define MY_CPU_NAME "sparc"
  #endif
#endif


#if  defined(__riscv) \
  || defined(__riscv__)
    #define MY_CPU_RISCV
  #if __riscv_xlen == 32
    #define MY_CPU_NAME "riscv32"
  #elif __riscv_xlen == 64
    #define MY_CPU_NAME "riscv64"
  #else
    #define MY_CPU_NAME "riscv"
  #endif
#endif


#if defined(__loongarch__)
  #define MY_CPU_LOONGARCH
  #if defined(__loongarch64) || defined(__loongarch_grlen) && (__loongarch_grlen == 64)
  #define MY_CPU_64BIT
  #endif
  #if defined(__loongarch64)
  #define MY_CPU_NAME "loongarch64"
  #define MY_CPU_LOONGARCH64
  #else
  #define MY_CPU_NAME "loongarch"
  #endif
#endif


// #undef MY_CPU_NAME
// #undef MY_CPU_SIZEOF_POINTER
// #define __e2k__
// #define __SIZEOF_POINTER__ 4
#if  defined(__e2k__)
  #define MY_CPU_E2K
  #if defined(__ILP32__) || defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)
    #define MY_CPU_NAME "e2k-32"
    #define MY_CPU_SIZEOF_POINTER 4
  #else
    #define MY_CPU_NAME "e2k"
    #if defined(__LP64__) || defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
      #define MY_CPU_SIZEOF_POINTER 8
    #endif
  #endif
  #define MY_CPU_64BIT
#endif


#if defined(MY_CPU_X86) || defined(MY_CPU_AMD64)
#define MY_CPU_X86_OR_AMD64
#endif

#if defined(MY_CPU_ARM) || defined(MY_CPU_ARM64)
#define MY_CPU_ARM_OR_ARM64
#endif


#ifdef _WIN32

  #ifdef MY_CPU_ARM
  #define MY_CPU_ARM_LE
  #endif

  #ifdef MY_CPU_ARM64
  #define MY_CPU_ARM64_LE
  #endif

  #ifdef _M_IA64
  #define MY_CPU_IA64_LE
  #endif

#endif


// _LITTLE_ENDIAN macro can be defined for big-endian platform with some compilers
 
#if defined(MY_CPU_X86_OR_AMD64) \
    || defined(MY_CPU_ARM_LE) \
    || defined(MY_CPU_ARM64_LE) \
    || defined(MY_CPU_IA64_LE) \
    || defined(__LITTLE_ENDIAN__) \
    || defined(__ARMEL__) \
    || defined(__THUMBEL__) \
    || defined(__AARCH64EL__) \
    || defined(__MIPSEL__) \
    || defined(__MIPSEL) \
    || defined(_MIPSEL) \
    || defined(__BFIN__) \
    || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
  #define MY_CPU_LE
#endif

#if defined(__BIG_ENDIAN__) \
    || defined(__ARMEB__) \
    || defined(__THUMBEB__) \
    || defined(__AARCH64EB__) \
    || defined(__MIPSEB__) \
    || defined(__MIPSEB) \
    || defined(_MIPSEB) \
    || defined(__m68k__) \
    || defined(__s390__) \
    || defined(__s390x__) \
    || defined(__zarch__) \
    || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
  #define MY_CPU_BE
#endif


#if defined(MY_CPU_LE) && defined(MY_CPU_BE)
  #error Stop_Compiling_Bad_Endian
#endif

#if !defined(MY_CPU_LE) && !defined(MY_CPU_BE)
  #error Stop_Compiling_CPU_ENDIAN_must_be_detected_at_compile_time
#endif

#if defined(MY_CPU_32BIT) && defined(MY_CPU_64BIT)
  #error Stop_Compiling_Bad_32_64_BIT
#endif

#ifdef __SIZEOF_POINTER__
  #ifdef MY_CPU_SIZEOF_POINTER
    #if MY_CPU_SIZEOF_POINTER != __SIZEOF_POINTER__
      #error Stop_Compiling_Bad_MY_CPU_PTR_SIZE
    #endif
  #else
    #define MY_CPU_SIZEOF_POINTER  __SIZEOF_POINTER__
  #endif
#endif

#if defined(MY_CPU_SIZEOF_POINTER) && (MY_CPU_SIZEOF_POINTER == 4)
#if defined (_LP64)
      #error Stop_Compiling_Bad_MY_CPU_PTR_SIZE
#endif
#endif

#ifdef _MSC_VER
  #if _MSC_VER >= 1300
    #define MY_CPU_pragma_pack_push_1   __pragma(pack(push, 1))
    #define MY_CPU_pragma_pop           __pragma(pack(pop))
  #else
    #define MY_CPU_pragma_pack_push_1
    #define MY_CPU_pragma_pop
  #endif
#else
  #ifdef __xlC__
    #define MY_CPU_pragma_pack_push_1   _Pragma("pack(1)")
    #define MY_CPU_pragma_pop           _Pragma("pack()")
  #else
    #define MY_CPU_pragma_pack_push_1   _Pragma("pack(push, 1)")
    #define MY_CPU_pragma_pop           _Pragma("pack(pop)")
  #endif
#endif


#ifndef MY_CPU_NAME
  // #define MY_CPU_IS_UNKNOWN
  #ifdef MY_CPU_LE
    #define MY_CPU_NAME "LE"
  #elif defined(MY_CPU_BE)
    #define MY_CPU_NAME "BE"
  #else
    /*
    #define MY_CPU_NAME ""
    */
  #endif
#endif





#ifdef __has_builtin
  #define Z7_has_builtin(x)  __has_builtin(x)
#else
  #define Z7_has_builtin(x)  0
#endif


#define Z7_BSWAP32_CONST(v) \
       ( (((UInt32)(v) << 24)                   ) \
       | (((UInt32)(v) <<  8) & (UInt32)0xff0000) \
       | (((UInt32)(v) >>  8) & (UInt32)0xff00  ) \
       | (((UInt32)(v) >> 24)                   ))


#if defined(_MSC_VER) && (_MSC_VER >= 1300)

#include <stdlib.h>

/* Note: these macros will use bswap instruction (486), that is unsupported in 386 cpu */

#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

#define Z7_BSWAP16(v)  _byteswap_ushort(v)
#define Z7_BSWAP32(v)  _byteswap_ulong (v)
#define Z7_BSWAP64(v)  _byteswap_uint64(v)
#define Z7_CPU_FAST_BSWAP_SUPPORTED

/* GCC can generate slow code that calls function for __builtin_bswap32() for:
     - GCC for RISCV, if Zbb/XTHeadBb extension is not used.
     - GCC for SPARC.
   The code from CLANG for SPARC also is not fastest.
   So we don't define Z7_CPU_FAST_BSWAP_SUPPORTED in some cases.
*/
#elif (!defined(MY_CPU_RISCV) || defined (__riscv_zbb) || defined(__riscv_xtheadbb)) \
    && !defined(MY_CPU_SPARC) \
    && ( \
       (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))) \
    || (defined(__clang__) && Z7_has_builtin(__builtin_bswap16)) \
    )

#define Z7_BSWAP16(v)  __builtin_bswap16(v)
#define Z7_BSWAP32(v)  __builtin_bswap32(v)
#define Z7_BSWAP64(v)  __builtin_bswap64(v)
#define Z7_CPU_FAST_BSWAP_SUPPORTED

#else

#define Z7_BSWAP16(v) ((UInt16) \
       ( ((UInt32)(v) << 8) \
       | ((UInt32)(v) >> 8) \
       ))

#define Z7_BSWAP32(v) Z7_BSWAP32_CONST(v)

#define Z7_BSWAP64(v) \
       ( ( ( (UInt64)(v)                           ) << 8 * 7 ) \
       | ( ( (UInt64)(v) & ((UInt32)0xff << 8 * 1) ) << 8 * 5 ) \
       | ( ( (UInt64)(v) & ((UInt32)0xff << 8 * 2) ) << 8 * 3 ) \
       | ( ( (UInt64)(v) & ((UInt32)0xff << 8 * 3) ) << 8 * 1 ) \
       | ( ( (UInt64)(v) >> 8 * 1 ) & ((UInt32)0xff << 8 * 3) ) \
       | ( ( (UInt64)(v) >> 8 * 3 ) & ((UInt32)0xff << 8 * 2) ) \
       | ( ( (UInt64)(v) >> 8 * 5 ) & ((UInt32)0xff << 8 * 1) ) \
       | ( ( (UInt64)(v) >> 8 * 7 )                           ) \
       )

#endif



#ifdef MY_CPU_LE
  #if defined(MY_CPU_X86_OR_AMD64) \
      || defined(MY_CPU_ARM64) \
      || defined(MY_CPU_RISCV) && defined(__riscv_misaligned_fast) \
      || defined(MY_CPU_E2K) && defined(__iset__) && (__iset__ >= 6)
    #define MY_CPU_LE_UNALIGN
    #define MY_CPU_LE_UNALIGN_64
  #elif defined(__ARM_FEATURE_UNALIGNED)
/* === ALIGNMENT on 32-bit arm and LDRD/STRD/LDM/STM instructions.
  Description of problems:
problem-1 : 32-bit ARM architecture:
  multi-access (pair of 32-bit accesses) instructions (LDRD/STRD/LDM/STM)
  require 32-bit (WORD) alignment (by 32-bit ARM architecture).
  So there is "Alignment fault exception", if data is not aligned for 32-bit.

problem-2 : 32-bit kernels and arm64 kernels:
  32-bit linux kernels provide fixup for these "paired" instruction "Alignment fault exception".
  So unaligned paired-access instructions work via exception handler in kernel in 32-bit linux.
 
  But some arm64 kernels do not handle these faults in 32-bit programs.
  So we have unhandled exception for such instructions.
  Probably some new arm64 kernels have fixed it, and unaligned
  paired-access instructions work in new kernels?

problem-3 : compiler for 32-bit arm:
  Compilers use LDRD/STRD/LDM/STM for UInt64 accesses
  and for another cases where two 32-bit accesses are fused
  to one multi-access instruction.
  So UInt64 variables must be aligned for 32-bit, and each
  32-bit access must be aligned for 32-bit, if we want to
  avoid "Alignment fault" exception (handled or unhandled).

problem-4 : performace:
  Even if unaligned access is handled by kernel, it will be slow.
  So if we allow unaligned access, we can get fast unaligned
  single-access, and slow unaligned paired-access.

  We don't allow unaligned access on 32-bit arm, because compiler
  genarates paired-access instructions that require 32-bit alignment,
  and some arm64 kernels have no handler for these instructions.
  Also unaligned paired-access instructions will be slow, if kernel handles them.
*/
    // it must be disabled:
    // #define MY_CPU_LE_UNALIGN
  #endif
#endif


#ifdef MY_CPU_LE_UNALIGN

#define GetUi16(p) (*(const UInt16 *)(const void *)(p))
#define GetUi32(p) (*(const UInt32 *)(const void *)(p))
#ifdef MY_CPU_LE_UNALIGN_64
#define GetUi64(p) (*(const UInt64 *)(const void *)(p))
#define SetUi64(p, v) { *(UInt64 *)(void *)(p) = (v); }
#endif

#define SetUi16(p, v) { *(UInt16 *)(void *)(p) = (v); }
#define SetUi32(p, v) { *(UInt32 *)(void *)(p) = (v); }

#else

#define GetUi16(p) ( (UInt16) ( \
             ((const Byte *)(p))[0] | \
    ((UInt16)((const Byte *)(p))[1] << 8) ))

#define GetUi32(p) ( \
             ((const Byte *)(p))[0]        | \
    ((UInt32)((const Byte *)(p))[1] <<  8) | \
    ((UInt32)((const Byte *)(p))[2] << 16) | \
    ((UInt32)((const Byte *)(p))[3] << 24))

#define SetUi16(p, v) { Byte *_ppp_ = (Byte *)(p); UInt32 _vvv_ = (v); \
    _ppp_[0] = (Byte)_vvv_; \
    _ppp_[1] = (Byte)(_vvv_ >> 8); }

#define SetUi32(p, v) { Byte *_ppp_ = (Byte *)(p); UInt32 _vvv_ = (v); \
    _ppp_[0] = (Byte)_vvv_; \
    _ppp_[1] = (Byte)(_vvv_ >> 8); \
    _ppp_[2] = (Byte)(_vvv_ >> 16); \
    _ppp_[3] = (Byte)(_vvv_ >> 24); }

#endif


#ifndef GetUi64
#define GetUi64(p) (GetUi32(p) | ((UInt64)GetUi32(((const Byte *)(p)) + 4) << 32))
#endif

#ifndef SetUi64
#define SetUi64(p, v) { Byte *_ppp2_ = (Byte *)(p); UInt64 _vvv2_ = (v); \
    SetUi32(_ppp2_    , (UInt32)_vvv2_) \
    SetUi32(_ppp2_ + 4, (UInt32)(_vvv2_ >> 32)) }
#endif


#if defined(MY_CPU_LE_UNALIGN) && defined(Z7_CPU_FAST_BSWAP_SUPPORTED)

#if 0
// Z7_BSWAP16 can be slow for x86-msvc
#define GetBe16_to32(p)  (Z7_BSWAP16 (*(const UInt16 *)(const void *)(p)))
#else
#define GetBe16_to32(p)  (Z7_BSWAP32 (*(const UInt16 *)(const void *)(p)) >> 16)
#endif

#define GetBe32(p)  Z7_BSWAP32 (*(const UInt32 *)(const void *)(p))
#define SetBe32(p, v) { (*(UInt32 *)(void *)(p)) = Z7_BSWAP32(v); }

#if defined(MY_CPU_LE_UNALIGN_64)
#define GetBe64(p)  Z7_BSWAP64 (*(const UInt64 *)(const void *)(p))
#define SetBe64(p, v) { (*(UInt64 *)(void *)(p)) = Z7_BSWAP64(v); }
#endif

#else

#define GetBe32(p) ( \
    ((UInt32)((const Byte *)(p))[0] << 24) | \
    ((UInt32)((const Byte *)(p))[1] << 16) | \
    ((UInt32)((const Byte *)(p))[2] <<  8) | \
             ((const Byte *)(p))[3] )

#define SetBe32(p, v) { Byte *_ppp_ = (Byte *)(p); UInt32 _vvv_ = (v); \
    _ppp_[0] = (Byte)(_vvv_ >> 24); \
    _ppp_[1] = (Byte)(_vvv_ >> 16); \
    _ppp_[2] = (Byte)(_vvv_ >> 8); \
    _ppp_[3] = (Byte)_vvv_; }

#endif

#ifndef GetBe64
#define GetBe64(p) (((UInt64)GetBe32(p) << 32) | GetBe32(((const Byte *)(p)) + 4))
#endif

#ifndef SetBe64
#define SetBe64(p, v) { Byte *_ppp_ = (Byte *)(p); UInt64 _vvv_ = (v); \
    _ppp_[0] = (Byte)(_vvv_ >> 56); \
    _ppp_[1] = (Byte)(_vvv_ >> 48); \
    _ppp_[2] = (Byte)(_vvv_ >> 40); \
    _ppp_[3] = (Byte)(_vvv_ >> 32); \
    _ppp_[4] = (Byte)(_vvv_ >> 24); \
    _ppp_[5] = (Byte)(_vvv_ >> 16); \
    _ppp_[6] = (Byte)(_vvv_ >> 8); \
    _ppp_[7] = (Byte)_vvv_; }
#endif

#ifndef GetBe16
#ifdef GetBe16_to32
#define GetBe16(p) ( (UInt16) GetBe16_to32(p))
#else
#define GetBe16(p) ( (UInt16) ( \
    ((UInt16)((const Byte *)(p))[0] << 8) | \
             ((const Byte *)(p))[1] ))
#endif
#endif


#if defined(MY_CPU_BE)
#define Z7_CONV_BE_TO_NATIVE_CONST32(v)  (v)
#define Z7_CONV_LE_TO_NATIVE_CONST32(v)  Z7_BSWAP32_CONST(v)
#define Z7_CONV_NATIVE_TO_BE_32(v)       (v)
// #define Z7_GET_NATIVE16_FROM_2_BYTES(b0, b1)  ((b1) | ((b0) << 8))
#elif defined(MY_CPU_LE)
#define Z7_CONV_BE_TO_NATIVE_CONST32(v)  Z7_BSWAP32_CONST(v)
#define Z7_CONV_LE_TO_NATIVE_CONST32(v)  (v)
#define Z7_CONV_NATIVE_TO_BE_32(v)       Z7_BSWAP32(v)
// #define Z7_GET_NATIVE16_FROM_2_BYTES(b0, b1)  ((b0) | ((b1) << 8))
#else
#error Stop_Compiling_Unknown_Endian_CONV
#endif


#if defined(MY_CPU_BE)

#define GetBe64a(p)      (*(const UInt64 *)(const void *)(p))
#define GetBe32a(p)      (*(const UInt32 *)(const void *)(p))
#define GetBe16a(p)      (*(const UInt16 *)(const void *)(p))
#define SetBe32a(p, v)   { *(UInt32 *)(void *)(p) = (v); }
#define SetBe16a(p, v)   { *(UInt16 *)(void *)(p) = (v); }

// gcc and clang for powerpc can transform load byte access to load reverse word access.
// sp we can use byte access instead of word access. Z7_BSWAP64 cab be slow
#if 1 && defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_64BIT)
#define GetUi64a(p)   Z7_BSWAP64 (*(const UInt64 *)(const void *)(p))
#else
#define GetUi64a(p)      GetUi64(p)
#endif

#if 1 && defined(Z7_CPU_FAST_BSWAP_SUPPORTED)
#define GetUi32a(p)   Z7_BSWAP32 (*(const UInt32 *)(const void *)(p))
#else
#define GetUi32a(p)      GetUi32(p)
#endif

#define GetUi16a(p)      GetUi16(p)
#define SetUi32a(p, v)   SetUi32(p, v)
#define SetUi16a(p, v)   SetUi16(p, v)

#elif defined(MY_CPU_LE)

#define GetUi64a(p)      (*(const UInt64 *)(const void *)(p))
#define GetUi32a(p)      (*(const UInt32 *)(const void *)(p))
#define GetUi16a(p)      (*(const UInt16 *)(const void *)(p))
#define SetUi32a(p, v)   { *(UInt32 *)(void *)(p) = (v); }
#define SetUi16a(p, v)   { *(UInt16 *)(void *)(p) = (v); }

#define GetBe64a(p)      GetBe64(p)
#define GetBe32a(p)      GetBe32(p)
#define GetBe16a(p)      GetBe16(p)
#define SetBe32a(p, v)   SetBe32(p, v)
#define SetBe16a(p, v)   SetBe16(p, v)

#else
#error Stop_Compiling_Unknown_Endian_CPU_a
#endif


#ifndef GetBe16_to32
#define GetBe16_to32(p) GetBe16(p)
#endif


#if defined(MY_CPU_X86_OR_AMD64) \
  || defined(MY_CPU_ARM_OR_ARM64) \
  || defined(MY_CPU_PPC_OR_PPC64)
  #define Z7_CPU_FAST_ROTATE_SUPPORTED
#endif


#ifdef MY_CPU_X86_OR_AMD64

void Z7_FASTCALL z7_x86_cpuid(UInt32 a[4], UInt32 function);
UInt32 Z7_FASTCALL z7_x86_cpuid_GetMaxFunc(void);
#if defined(MY_CPU_AMD64)
#define Z7_IF_X86_CPUID_SUPPORTED
#else
#define Z7_IF_X86_CPUID_SUPPORTED if (z7_x86_cpuid_GetMaxFunc())
#endif

BoolInt CPU_IsSupported_AES(void);
BoolInt CPU_IsSupported_AVX(void);
BoolInt CPU_IsSupported_AVX2(void);
BoolInt CPU_IsSupported_AVX512F_AVX512VL(void);
BoolInt CPU_IsSupported_VAES_AVX2(void);
BoolInt CPU_IsSupported_CMOV(void);
BoolInt CPU_IsSupported_SSE(void);
BoolInt CPU_IsSupported_SSE2(void);
BoolInt CPU_IsSupported_SSSE3(void);
BoolInt CPU_IsSupported_SSE41(void);
BoolInt CPU_IsSupported_SHA(void);
BoolInt CPU_IsSupported_SHA512(void);
BoolInt CPU_IsSupported_PageGB(void);

#elif defined(MY_CPU_ARM_OR_ARM64)

BoolInt CPU_IsSupported_CRC32(void);
BoolInt CPU_IsSupported_NEON(void);

#if defined(_WIN32)
BoolInt CPU_IsSupported_CRYPTO(void);
#define CPU_IsSupported_SHA1  CPU_IsSupported_CRYPTO
#define CPU_IsSupported_SHA2  CPU_IsSupported_CRYPTO
#define CPU_IsSupported_AES   CPU_IsSupported_CRYPTO
#else
BoolInt CPU_IsSupported_SHA1(void);
BoolInt CPU_IsSupported_SHA2(void);
BoolInt CPU_IsSupported_AES(void);
#endif
BoolInt CPU_IsSupported_SHA512(void);

#endif

#if defined(__APPLE__)
int z7_sysctlbyname_Get(const char *name, void *buf, size_t *bufSize);
int z7_sysctlbyname_Get_UInt32(const char *name, UInt32 *val);
#endif

EXTERN_C_END

#endif

/* ---- C/Sha3.h ---- */
/* Sha3.h -- SHA-3 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_MD5_H
#define ZIP7_INC_MD5_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define SHA3_NUM_STATE_WORDS  25

#define SHA3_BLOCK_SIZE_FROM_DIGEST_SIZE(digestSize) \
    (SHA3_NUM_STATE_WORDS * 8 - (digestSize) * 2)

typedef struct
{
  UInt32 count;     // < blockSize
  UInt32 blockSize; // <= SHA3_NUM_STATE_WORDS * 8
  UInt64 _pad1[3];
  // we want 32-bytes alignment here
  UInt64 state[SHA3_NUM_STATE_WORDS];
  UInt64 _pad2[3];
  // we want 64-bytes alignment here
  Byte buffer[SHA3_NUM_STATE_WORDS * 8]; // last bytes will be unused with predefined blockSize values
} CSha3;

#define Sha3_SET_blockSize(p, blockSize) { (p)->blockSize = (blockSize); }

void Sha3_Init(CSha3 *p);
void Sha3_Update(CSha3 *p, const Byte *data, size_t size);
void Sha3_Final(CSha3 *p, Byte *digest, unsigned digestSize, unsigned shake);

EXTERN_C_END

#endif

/* ---- C/RotateDefs.h ---- */
/* RotateDefs.h -- Rotate functions
2023-06-18 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_ROTATE_DEFS_H
#define ZIP7_INC_ROTATE_DEFS_H

#ifdef _MSC_VER

#include <stdlib.h>

/* don't use _rotl with old MINGW. It can insert slow call to function. */
 
/* #if (_MSC_VER >= 1200) */
#pragma intrinsic(_rotl)
#pragma intrinsic(_rotr)
/* #endif */

#define rotlFixed(x, n) _rotl((x), (n))
#define rotrFixed(x, n) _rotr((x), (n))

#if (_MSC_VER >= 1300)
#define Z7_ROTL64(x, n) _rotl64((x), (n))
#define Z7_ROTR64(x, n) _rotr64((x), (n))
#else
#define Z7_ROTL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define Z7_ROTR64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#endif

#else

/* new compilers can translate these macros to fast commands. */

#if defined(__clang__) && (__clang_major__ >= 4) \
  || defined(__GNUC__) && (__GNUC__ >= 5)
/* GCC 4.9.0 and clang 3.5 can recognize more correct version: */
#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (-(n) & 31)))
#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (-(n) & 31)))
#define Z7_ROTL64(x, n) (((x) << (n)) | ((x) >> (-(n) & 63)))
#define Z7_ROTR64(x, n) (((x) >> (n)) | ((x) << (-(n) & 63)))
#else
/* for old GCC / clang: */
#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define Z7_ROTL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define Z7_ROTR64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#endif

#endif

#endif

/* ---- C/Sha512.h ---- */
/* Sha512.h -- SHA-512 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SHA512_H
#define ZIP7_INC_SHA512_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define SHA512_NUM_BLOCK_WORDS  16
#define SHA512_NUM_DIGEST_WORDS  8

#define SHA512_BLOCK_SIZE   (SHA512_NUM_BLOCK_WORDS * 8)
#define SHA512_DIGEST_SIZE  (SHA512_NUM_DIGEST_WORDS * 8)
#define SHA512_224_DIGEST_SIZE  (224 / 8)
#define SHA512_256_DIGEST_SIZE  (256 / 8)
#define SHA512_384_DIGEST_SIZE  (384 / 8)

typedef void (Z7_FASTCALL *SHA512_FUNC_UPDATE_BLOCKS)(UInt64 state[8], const Byte *data, size_t numBlocks);

/*
  if (the system supports different SHA512 code implementations)
  {
    (CSha512::func_UpdateBlocks) will be used
    (CSha512::func_UpdateBlocks) can be set by
       Sha512_Init()        - to default (fastest)
       Sha512_SetFunction() - to any algo
  }
  else
  {
    (CSha512::func_UpdateBlocks) is ignored.
  }
*/

typedef struct
{
  union
  {
    struct
    {
      SHA512_FUNC_UPDATE_BLOCKS func_UpdateBlocks;
      UInt64 count;
    } vars;
    UInt64 _pad_64bit[8];
    void *_pad_align_ptr[2];
  } v;
  UInt64 state[SHA512_NUM_DIGEST_WORDS];
  
  Byte buffer[SHA512_BLOCK_SIZE];
} CSha512;


#define SHA512_ALGO_DEFAULT 0
#define SHA512_ALGO_SW      1
#define SHA512_ALGO_HW      2

/*
Sha512_SetFunction()
return:
  0 - (algo) value is not supported, and func_UpdateBlocks was not changed
  1 - func_UpdateBlocks was set according (algo) value.
*/

BoolInt Sha512_SetFunction(CSha512 *p, unsigned algo);
// we support only these (digestSize) values: 224/8, 256/8, 384/8, 512/8
void Sha512_InitState(CSha512 *p, unsigned digestSize);
void Sha512_Init(CSha512 *p, unsigned digestSize);
void Sha512_Update(CSha512 *p, const Byte *data, size_t size);
void Sha512_Final(CSha512 *p, Byte *digest, unsigned digestSize);




// void Z7_FASTCALL Sha512_UpdateBlocks(UInt64 state[8], const Byte *data, size_t numBlocks);

/*
call Sha512Prepare() once at program start.
It prepares all supported implementations, and detects the fastest implementation.
*/

void Sha512Prepare(void);

EXTERN_C_END

#endif

/* ---- C/7zWindows.h ---- */
/* 7zWindows.h -- Windows.h and related code
Igor Pavlov : Public domain */

#ifndef ZIP7_INC_7Z_WINDOWS_H
#define ZIP7_INC_7Z_WINDOWS_H

#ifdef _WIN32

#if defined(_MSC_VER) && _MSC_VER >= 1950 && !defined(__clang__) // VS2026
// <Windows.h> and some another windows files need that option
// VS2026: wtypesbase.h: warning C4865: 'tagCLSCTX': the underlying type will change from 'int' to 'unsigned int' when '/Zc:enumTypes' is specified on the command line
#pragma warning(disable : 4865)
#endif

#if defined(__clang__)
# pragma clang diagnostic push
#endif

#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4668) // '_WIN32_WINNT' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'

#if _MSC_VER == 1900
// for old kit10 versions
// #pragma warning(disable : 4255) // winuser.h(13979): warning C4255: 'GetThreadDpiAwarenessContext':
#endif
// win10 Windows Kit:
#endif // _MSC_VER

#if defined(_MSC_VER) && _MSC_VER <= 1200 && !defined(_WIN64)
// for msvc6 without sdk2003
#define RPC_NO_WINDOWS_H
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
// #if defined(__GNUC__) && !defined(__clang__)
#include <windows.h>
#else
#include <Windows.h>
#endif
// #include <basetsd.h>
// #include <wtypes.h>

// but if precompiled with clang-cl then we need
// #include <windows.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200 && !defined(_WIN64)
#ifndef _W64

typedef long LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#define Z7_OLD_WIN_SDK
#endif // _W64
#endif // _MSC_VER == 1200

#ifdef Z7_OLD_WIN_SDK

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
#ifndef FILE_SPECIAL_ACCESS
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#endif

// ShlObj.h:
// #define BIF_NEWDIALOGSTYLE     0x0040

#pragma warning(disable : 4201)
// #pragma warning(disable : 4115)

#undef  VARIANT_TRUE
#define VARIANT_TRUE ((VARIANT_BOOL)-1)
#endif

#endif // Z7_OLD_WIN_SDK

#ifdef UNDER_CE
#undef  VARIANT_TRUE
#define VARIANT_TRUE ((VARIANT_BOOL)-1)
#endif


#if defined(_MSC_VER)
#if _MSC_VER >= 1400 && _MSC_VER <= 1600
  // BaseTsd.h(148) : 'HandleToULong' : unreferenced inline function has been removed
  // string.h
  // #pragma warning(disable : 4514)
#endif
#endif


/* #include "7zTypes.h" */

#endif

/* ---- C/Sort.h ---- */
/* Sort.h -- Sort functions
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SORT_H
#define ZIP7_INC_SORT_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

void Z7_FASTCALL HeapSort(UInt32 *p, size_t size);

EXTERN_C_END

#endif

/* ---- C/SwapBytes.h ---- */
/* SwapBytes.h -- Byte Swap conversion filter
2023-04-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SWAP_BYTES_H
#define ZIP7_INC_SWAP_BYTES_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

void z7_SwapBytes2(UInt16 *data, size_t numItems);
void z7_SwapBytes4(UInt32 *data, size_t numItems);
void z7_SwapBytesPrepare(void);

EXTERN_C_END

#endif

/* ---- C/Threads.h ---- */
/* Threads.h -- multithreading library
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_THREADS_H
#define ZIP7_INC_THREADS_H

#ifdef _WIN32
// amalgamation: header emitted in prologue

#else

// amalgamation: header emitted in prologue

// #define Z7_AFFINITY_DISABLE
#if defined(__linux__)
#if !defined(__APPLE__) && !defined(_AIX) && !defined(__ANDROID__)
#ifndef Z7_AFFINITY_DISABLE
#define Z7_AFFINITY_SUPPORTED
// #pragma message(" ==== Z7_AFFINITY_SUPPORTED")
#if !defined(_GNU_SOURCE)
// #pragma message(" ==== _GNU_SOURCE set")
// we need _GNU_SOURCE for cpu_set_t, if we compile for MUSL
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
#define _GNU_SOURCE
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif
#endif
#endif

#include <pthread.h>

#endif

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#ifdef _WIN32

WRes HandlePtr_Close(HANDLE *h);
WRes Handle_WaitObject(HANDLE h);

typedef HANDLE CThread;

#define Thread_CONSTRUCT(p) { *(p) = NULL; }
#define Thread_WasCreated(p) (*(p) != NULL)
#define Thread_Close(p) HandlePtr_Close(p)
// #define Thread_Wait(p) Handle_WaitObject(*(p))

#ifdef UNDER_CE
  // if (USE_THREADS_CreateThread is      defined), we use _beginthreadex()
  // if (USE_THREADS_CreateThread is not definned), we use CreateThread()
  #define USE_THREADS_CreateThread
#endif

typedef
    #ifdef USE_THREADS_CreateThread
      DWORD
    #else
      unsigned
    #endif
    THREAD_FUNC_RET_TYPE;

#define THREAD_FUNC_RET_ZERO  0

typedef DWORD_PTR CAffinityMask;
typedef DWORD_PTR CCpuSet;

#define CpuSet_Zero(p)        *(p) = (0)
#define CpuSet_Set(p, cpu)    *(p) |= ((DWORD_PTR)1 << (cpu))

#else //  _WIN32

typedef struct
{
  pthread_t _tid;
  int _created;
} CThread;

#define Thread_CONSTRUCT(p)   { (p)->_tid = 0;  (p)->_created = 0; }
#define Thread_WasCreated(p)  ((p)->_created != 0)
WRes Thread_Close(CThread *p);
// #define Thread_Wait Thread_Wait_Close

typedef void * THREAD_FUNC_RET_TYPE;
#define THREAD_FUNC_RET_ZERO  NULL


typedef UInt64 CAffinityMask;

#ifdef Z7_AFFINITY_SUPPORTED

typedef cpu_set_t CCpuSet;
#define CpuSet_Zero(p)        CPU_ZERO(p)
#define CpuSet_Set(p, cpu)    CPU_SET(cpu, p)
#define CpuSet_IsSet(p, cpu)  CPU_ISSET(cpu, p)

#else

typedef UInt64 CCpuSet;
#define CpuSet_Zero(p)        *(p) = (0)
#define CpuSet_Set(p, cpu)    *(p) |= ((UInt64)1 << (cpu))
#define CpuSet_IsSet(p, cpu)  ((*(p) & ((UInt64)1 << (cpu))) != 0)

#endif


#endif //  _WIN32


#define THREAD_FUNC_CALL_TYPE Z7_STDCALL

#if defined(_WIN32) && defined(__GNUC__)
/* GCC compiler for x86 32-bit uses the rule:
   the stack is 16-byte aligned before CALL instruction for function calling.
   But only root function main() contains instructions that
   set 16-byte alignment for stack pointer. And another functions
   just keep alignment, if it was set in some parent function.
   
   The problem:
    if we create new thread in MinGW (GCC) 32-bit x86 via _beginthreadex() or CreateThread(),
       the root function of thread doesn't set 16-byte alignment.
       And stack frames in all child functions also will be unaligned in that case.
   
   Here we set (force_align_arg_pointer) attribute for root function of new thread.
   Do we need (force_align_arg_pointer) also for another systems?  */
  
  #define THREAD_FUNC_ATTRIB_ALIGN_ARG __attribute__((force_align_arg_pointer))
  // #define THREAD_FUNC_ATTRIB_ALIGN_ARG // for debug : bad alignment in SSE functions
#else
  #define THREAD_FUNC_ATTRIB_ALIGN_ARG
#endif

#define THREAD_FUNC_DECL  THREAD_FUNC_ATTRIB_ALIGN_ARG THREAD_FUNC_RET_TYPE THREAD_FUNC_CALL_TYPE

typedef THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE * THREAD_FUNC_TYPE)(void *);
WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param);
WRes Thread_Create_With_Affinity(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, CAffinityMask affinity);
WRes Thread_Wait_Close(CThread *p);

#ifdef _WIN32
WRes Thread_Create_With_Group(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, unsigned group, CAffinityMask affinityMask);
#define Thread_Create_With_CpuSet(p, func, param, cs) \
  Thread_Create_With_Affinity(p, func, param, *cs)
#else
WRes Thread_Create_With_CpuSet(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, const CCpuSet *cpuSet);
#endif

typedef struct
{
  unsigned NumGroups;
  unsigned NextGroup;
} CThreadNextGroup;

void ThreadNextGroup_Init(CThreadNextGroup *p, unsigned numGroups, unsigned startGroup);
unsigned ThreadNextGroup_GetNext(CThreadNextGroup *p);


#ifdef _WIN32

typedef HANDLE CEvent;
typedef CEvent CAutoResetEvent;
typedef CEvent CManualResetEvent;
#define Event_Construct(p) *(p) = NULL
#define Event_IsCreated(p) (*(p) != NULL)
#define Event_Close(p) HandlePtr_Close(p)
#define Event_Wait(p) Handle_WaitObject(*(p))
WRes Event_Set(CEvent *p);
WRes Event_Reset(CEvent *p);
WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled);
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p);
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled);
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p);

typedef HANDLE CSemaphore;
#define Semaphore_Construct(p) *(p) = NULL
#define Semaphore_IsCreated(p) (*(p) != NULL)
#define Semaphore_Close(p) HandlePtr_Close(p)
#define Semaphore_Wait(p) Handle_WaitObject(*(p))
WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount);
WRes Semaphore_OptCreateInit(CSemaphore *p, UInt32 initCount, UInt32 maxCount);
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num);
WRes Semaphore_Release1(CSemaphore *p);

typedef CRITICAL_SECTION CCriticalSection;
WRes CriticalSection_Init(CCriticalSection *p);
#define CriticalSection_Delete(p) DeleteCriticalSection(p)
#define CriticalSection_Enter(p) EnterCriticalSection(p)
#define CriticalSection_Leave(p) LeaveCriticalSection(p)


#else // _WIN32

typedef struct
{
  int _created;
  int _manual_reset;
  int _state;
  pthread_mutex_t _mutex;
  pthread_cond_t _cond;
} CEvent;

typedef CEvent CAutoResetEvent;
typedef CEvent CManualResetEvent;

#define Event_Construct(p) (p)->_created = 0
#define Event_IsCreated(p) ((p)->_created)

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled);
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p);
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled);
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p);

WRes Event_Set(CEvent *p);
WRes Event_Reset(CEvent *p);
WRes Event_Wait(CEvent *p);
WRes Event_Close(CEvent *p);


typedef struct
{
  int _created;
  UInt32 _count;
  UInt32 _maxCount;
  pthread_mutex_t _mutex;
  pthread_cond_t _cond;
} CSemaphore;

#define Semaphore_Construct(p) (p)->_created = 0
#define Semaphore_IsCreated(p) ((p)->_created)

WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount);
WRes Semaphore_OptCreateInit(CSemaphore *p, UInt32 initCount, UInt32 maxCount);
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num);
#define Semaphore_Release1(p) Semaphore_ReleaseN(p, 1)
WRes Semaphore_Wait(CSemaphore *p);
WRes Semaphore_Close(CSemaphore *p);


typedef struct
{
  pthread_mutex_t _mutex;
} CCriticalSection;

WRes CriticalSection_Init(CCriticalSection *p);
void CriticalSection_Delete(CCriticalSection *cs);
void CriticalSection_Enter(CCriticalSection *cs);
void CriticalSection_Leave(CCriticalSection *cs);

LONG InterlockedIncrement(LONG volatile *addend);
LONG InterlockedDecrement(LONG volatile *addend);

#endif  // _WIN32

WRes AutoResetEvent_OptCreate_And_Reset(CAutoResetEvent *p);

EXTERN_C_END

#endif

/* ---- C/Xxh64.h ---- */
/* Xxh64.h -- XXH64 hash calculation interfaces
2023-08-18 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_XXH64_H
#define ZIP7_INC_XXH64_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define Z7_XXH64_BLOCK_SIZE  (4 * 8)

typedef struct
{
  UInt64 v[4];
} CXxh64State;

void Xxh64State_Init(CXxh64State *p);

// end != data && end == data + Z7_XXH64_BLOCK_SIZE * numBlocks
void Z7_FASTCALL Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end);

/*
Xxh64State_Digest():
data:
  the function processes only
    (totalCount & (Z7_XXH64_BLOCK_SIZE - 1)) bytes in (data): (smaller than 32 bytes).
totalCount: total size of hashed stream:
  it includes total size of data processed by previous Xxh64State_UpdateBlocks() calls,
  and it also includes current processed size in (data).
*/
UInt64 Xxh64State_Digest(const CXxh64State *p, const void *data, UInt64 totalCount);


typedef struct
{
  CXxh64State state;
  UInt64 count;
  UInt64 buf64[4];
} CXxh64;

void Xxh64_Init(CXxh64 *p);
void Xxh64_Update(CXxh64 *p, const void *data, size_t size);

#define Xxh64_Digest(p) \
  Xxh64State_Digest(&(p)->state, (p)->buf64, (p)->count)

EXTERN_C_END

#endif

/* ---- C/7zCrc.h ---- */
/* 7zCrc.h -- CRC32 calculation
2024-01-22 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_7Z_CRC_H
#define ZIP7_INC_7Z_CRC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

extern UInt32 g_CrcTable[];

/* Call CrcGenerateTable one time before other CRC functions */
void Z7_FASTCALL CrcGenerateTable(void);

#define CRC_INIT_VAL 0xFFFFFFFF
#define CRC_GET_DIGEST(crc) ((crc) ^ CRC_INIT_VAL)
#define CRC_UPDATE_BYTE(crc, b) (g_CrcTable[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

UInt32 Z7_FASTCALL CrcUpdate(UInt32 crc, const void *data, size_t size);
UInt32 Z7_FASTCALL CrcCalc(const void *data, size_t size);

typedef UInt32 (Z7_FASTCALL *Z7_CRC_UPDATE_FUNC)(UInt32 v, const void *data, size_t size);
Z7_CRC_UPDATE_FUNC z7_GetFunc_CrcUpdate(unsigned algo);

EXTERN_C_END

#endif

/* ---- C/Sha256.h ---- */
/* Sha256.h -- SHA-256 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SHA256_H
#define ZIP7_INC_SHA256_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define SHA256_NUM_BLOCK_WORDS  16
#define SHA256_NUM_DIGEST_WORDS  8

#define SHA256_BLOCK_SIZE   (SHA256_NUM_BLOCK_WORDS * 4)
#define SHA256_DIGEST_SIZE  (SHA256_NUM_DIGEST_WORDS * 4)




typedef void (Z7_FASTCALL *SHA256_FUNC_UPDATE_BLOCKS)(UInt32 state[8], const Byte *data, size_t numBlocks);

/*
  if (the system supports different SHA256 code implementations)
  {
    (CSha256::func_UpdateBlocks) will be used
    (CSha256::func_UpdateBlocks) can be set by
       Sha256_Init()        - to default (fastest)
       Sha256_SetFunction() - to any algo
  }
  else
  {
    (CSha256::func_UpdateBlocks) is ignored.
  }
*/

typedef struct
{
  union
  {
    struct
    {
      SHA256_FUNC_UPDATE_BLOCKS func_UpdateBlocks;
      UInt64 count;
    } vars;
    UInt64 _pad_64bit[4];
    void *_pad_align_ptr[2];
  } v;
  UInt32 state[SHA256_NUM_DIGEST_WORDS];

  Byte buffer[SHA256_BLOCK_SIZE];
} CSha256;


#define SHA256_ALGO_DEFAULT 0
#define SHA256_ALGO_SW      1
#define SHA256_ALGO_HW      2

/*
Sha256_SetFunction()
return:
  0 - (algo) value is not supported, and func_UpdateBlocks was not changed
  1 - func_UpdateBlocks was set according (algo) value.
*/

BoolInt Sha256_SetFunction(CSha256 *p, unsigned algo);

void Sha256_InitState(CSha256 *p);
void Sha256_Init(CSha256 *p);
void Sha256_Update(CSha256 *p, const Byte *data, size_t size);
void Sha256_Final(CSha256 *p, Byte *digest);




// void Z7_FASTCALL Sha256_UpdateBlocks(UInt32 state[8], const Byte *data, size_t numBlocks);

/*
call Sha256Prepare() once at program start.
It prepares all supported implementations, and detects the fastest implementation.
*/

void Sha256Prepare(void);

EXTERN_C_END

#endif

/* ---- C/Delta.h ---- */
/* Delta.h -- Delta converter
2023-03-03 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_DELTA_H
#define ZIP7_INC_DELTA_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define DELTA_STATE_SIZE 256

void Delta_Init(Byte *state);
void Delta_Encode(Byte *state, unsigned delta, Byte *data, SizeT size);
void Delta_Decode(Byte *state, unsigned delta, Byte *data, SizeT size);

EXTERN_C_END

#endif

/* ---- C/Xz.h ---- */
/* Xz.h - Xz interface
Igor Pavlov : Public domain */

#ifndef ZIP7_INC_XZ_H
#define ZIP7_INC_XZ_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define XZ_ID_Subblock 1
#define XZ_ID_Delta 3
#define XZ_ID_X86   4
#define XZ_ID_PPC   5
#define XZ_ID_IA64  6
#define XZ_ID_ARM   7
#define XZ_ID_ARMT  8
#define XZ_ID_SPARC 9
#define XZ_ID_ARM64 0xa
#define XZ_ID_RISCV 0xb
#define XZ_ID_LZMA2 0x21

unsigned Xz_ReadVarInt(const Byte *p, size_t maxSize, UInt64 *value);
unsigned Xz_WriteVarInt(Byte *buf, UInt64 v);

/* ---------- xz block ---------- */

#define XZ_BLOCK_HEADER_SIZE_MAX 1024

#define XZ_NUM_FILTERS_MAX 4
#define XZ_BF_NUM_FILTERS_MASK 3
#define XZ_BF_PACK_SIZE (1 << 6)
#define XZ_BF_UNPACK_SIZE (1 << 7)

#define XZ_FILTER_PROPS_SIZE_MAX 20

typedef struct
{
  UInt64 id;
  UInt32 propsSize;
  Byte props[XZ_FILTER_PROPS_SIZE_MAX];
} CXzFilter;

typedef struct
{
  UInt64 packSize;
  UInt64 unpackSize;
  Byte flags;
  CXzFilter filters[XZ_NUM_FILTERS_MAX];
} CXzBlock;

#define XzBlock_GetNumFilters(p) (((unsigned)(p)->flags & XZ_BF_NUM_FILTERS_MASK) + 1)
#define XzBlock_HasPackSize(p)   (((p)->flags & XZ_BF_PACK_SIZE) != 0)
#define XzBlock_HasUnpackSize(p) (((p)->flags & XZ_BF_UNPACK_SIZE) != 0)
#define XzBlock_HasUnsupportedFlags(p) (((p)->flags & ~(XZ_BF_NUM_FILTERS_MASK | XZ_BF_PACK_SIZE | XZ_BF_UNPACK_SIZE)) != 0)

SRes XzBlock_Parse(CXzBlock *p, const Byte *header);
SRes XzBlock_ReadHeader(CXzBlock *p, ISeqInStreamPtr inStream, BoolInt *isIndex, UInt32 *headerSizeRes);

/* ---------- xz stream ---------- */

#define XZ_SIG_SIZE 6
#define XZ_FOOTER_SIG_SIZE 2

extern const Byte XZ_SIG[XZ_SIG_SIZE];

/*
extern const Byte XZ_FOOTER_SIG[XZ_FOOTER_SIG_SIZE];
*/

#define XZ_FOOTER_SIG_0 'Y'
#define XZ_FOOTER_SIG_1 'Z'

#define XZ_STREAM_FLAGS_SIZE 2
#define XZ_STREAM_CRC_SIZE 4

#define XZ_STREAM_HEADER_SIZE (XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE + XZ_STREAM_CRC_SIZE)
#define XZ_STREAM_FOOTER_SIZE (XZ_FOOTER_SIG_SIZE + XZ_STREAM_FLAGS_SIZE + XZ_STREAM_CRC_SIZE + 4)

#define XZ_CHECK_MASK 0xF
#define XZ_CHECK_NO 0
#define XZ_CHECK_CRC32 1
#define XZ_CHECK_CRC64 4
#define XZ_CHECK_SHA256 10

typedef struct
{
  unsigned mode;
  UInt32 crc;
  UInt64 crc64;
  CSha256 sha;
} CXzCheck;

void XzCheck_Init(CXzCheck *p, unsigned mode);
void XzCheck_Update(CXzCheck *p, const void *data, size_t size);
int XzCheck_Final(CXzCheck *p, Byte *digest);

typedef UInt16 CXzStreamFlags;

#define XzFlags_IsSupported(f) ((f) <= XZ_CHECK_MASK)
#define XzFlags_GetCheckType(f) ((f) & XZ_CHECK_MASK)
#define XzFlags_HasDataCrc32(f) (Xz_GetCheckType(f) == XZ_CHECK_CRC32)
unsigned XzFlags_GetCheckSize(CXzStreamFlags f);

SRes Xz_ParseHeader(CXzStreamFlags *p, const Byte *buf);
SRes Xz_ReadHeader(CXzStreamFlags *p, ISeqInStreamPtr inStream);

typedef struct
{
  UInt64 unpackSize;
  UInt64 totalSize;
} CXzBlockSizes;

typedef struct
{
  CXzStreamFlags flags;
  // Byte _pad[6];
  size_t numBlocks;
  CXzBlockSizes *blocks;
  UInt64 startOffset;
} CXzStream;

#define Xz_CONSTRUCT(p) { (p)->numBlocks = 0;  (p)->blocks = NULL;  (p)->flags = 0; }
void Xz_Construct(CXzStream *p);
void Xz_Free(CXzStream *p, ISzAllocPtr alloc);

#define XZ_SIZE_OVERFLOW ((UInt64)(Int64)-1)

UInt64 Xz_GetUnpackSize(const CXzStream *p);
UInt64 Xz_GetPackSize(const CXzStream *p);

typedef struct
{
  size_t num;
  size_t numAllocated;
  CXzStream *streams;
} CXzs;

#define Xzs_CONSTRUCT(p) { (p)->num = 0;  (p)->numAllocated = 0;  (p)->streams = NULL; }
void Xzs_Construct(CXzs *p);
void Xzs_Free(CXzs *p, ISzAllocPtr alloc);
/*
Xzs_ReadBackward() must be called for empty CXzs object.
Xzs_ReadBackward() can return non empty object with (p->num != 0) even in case of error.
*/
SRes Xzs_ReadBackward(CXzs *p, ILookInStreamPtr inStream, Int64 *startOffset, ICompressProgressPtr progress, ISzAllocPtr alloc);

UInt64 Xzs_GetNumBlocks(const CXzs *p);
UInt64 Xzs_GetUnpackSize(const CXzs *p);


// ECoderStatus values are identical to ELzmaStatus values of LZMA2 decoder

typedef enum
{
  CODER_STATUS_NOT_SPECIFIED,               /* use main error code instead */
  CODER_STATUS_FINISHED_WITH_MARK,          /* stream was finished with end mark. */
  CODER_STATUS_NOT_FINISHED,                /* stream was not finished */
  CODER_STATUS_NEEDS_MORE_INPUT             /* you must provide more input bytes */
} ECoderStatus;


// ECoderFinishMode values are identical to ELzmaFinishMode

typedef enum
{
  CODER_FINISH_ANY,   /* finish at any point */
  CODER_FINISH_END    /* block must be finished at the end */
} ECoderFinishMode;


typedef struct
{
  void *p; // state object;
  void (*Free)(void *p, ISzAllocPtr alloc);
  SRes (*SetProps)(void *p, const Byte *props, size_t propSize, ISzAllocPtr alloc);
  void (*Init)(void *p);
  SRes (*Code2)(void *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
      int srcWasFinished, ECoderFinishMode finishMode,
      // int *wasFinished,
      ECoderStatus *status);
  SizeT (*Filter)(void *p, Byte *data, SizeT size);
} IStateCoder;


typedef struct
{
  UInt32 methodId;
  UInt32 delta;
  UInt32 ip;
  UInt32 X86_State;
  Byte delta_State[DELTA_STATE_SIZE];
} CXzBcFilterStateBase;

typedef SizeT (*Xz_Func_BcFilterStateBase_Filter)(CXzBcFilterStateBase *p, Byte *data, SizeT size);

SRes Xz_StateCoder_Bc_SetFromMethod_Func(IStateCoder *p, UInt64 id,
    Xz_Func_BcFilterStateBase_Filter func, ISzAllocPtr alloc);


#define MIXCODER_NUM_FILTERS_MAX 4

typedef struct
{
  ISzAllocPtr alloc;
  Byte *buf;
  unsigned numCoders;

  Byte *outBuf;
  size_t outBufSize;
  size_t outWritten; // is equal to lzmaDecoder.dicPos (in outBuf mode)
  BoolInt wasFinished;
  SRes res;
  ECoderStatus status;
  // BoolInt SingleBufMode;
  
  int finished[MIXCODER_NUM_FILTERS_MAX - 1];
  size_t pos[MIXCODER_NUM_FILTERS_MAX - 1];
  size_t size[MIXCODER_NUM_FILTERS_MAX - 1];
  UInt64 ids[MIXCODER_NUM_FILTERS_MAX];
  SRes results[MIXCODER_NUM_FILTERS_MAX];
  IStateCoder coders[MIXCODER_NUM_FILTERS_MAX];
} CMixCoder;


typedef enum
{
  XZ_STATE_STREAM_HEADER,
  XZ_STATE_STREAM_INDEX,
  XZ_STATE_STREAM_INDEX_CRC,
  XZ_STATE_STREAM_FOOTER,
  XZ_STATE_STREAM_PADDING,
  XZ_STATE_BLOCK_HEADER,
  XZ_STATE_BLOCK,
  XZ_STATE_BLOCK_FOOTER
} EXzState;


typedef struct
{
  EXzState state;
  unsigned pos;
  unsigned alignPos;
  unsigned indexPreSize;

  CXzStreamFlags streamFlags;
  
  unsigned blockHeaderSize;
  UInt64 packSize;
  UInt64 unpackSize;

  UInt64 numBlocks; // number of finished blocks in current stream
  UInt64 indexSize;
  UInt64 indexPos;
  UInt64 padSize;

  UInt64 numStartedStreams;
  UInt64 numFinishedStreams;
  UInt64 numTotalBlocks;

  UInt32 crc;
  CMixCoder decoder;
  CXzBlock block;
  CXzCheck check;
  CSha256 sha;

  BoolInt parseMode;
  BoolInt headerParsedOk;
  BoolInt decodeToStreamSignature;
  unsigned decodeOnlyOneBlock;

  Byte *outBuf;
  size_t outBufSize;
  size_t outDataWritten; // the size of data in (outBuf) that were fully unpacked

  UInt32 shaDigest32[SHA256_DIGEST_SIZE / 4];
  Byte buf[XZ_BLOCK_HEADER_SIZE_MAX]; // it must be aligned for 4-bytes
} CXzUnpacker;

/* alloc : aligned for cache line allocation is better */
void XzUnpacker_Construct(CXzUnpacker *p, ISzAllocPtr alloc);
void XzUnpacker_Init(CXzUnpacker *p);
void XzUnpacker_SetOutBuf(CXzUnpacker *p, Byte *outBuf, size_t outBufSize);
void XzUnpacker_Free(CXzUnpacker *p);

/*
  XzUnpacker
  The sequence for decoding functions:
  {
    XzUnpacker_Construct()
    [Decoding_Calls]
    XzUnpacker_Free()
  }

  [Decoding_Calls]

  There are 3 types of interfaces for [Decoding_Calls] calls:

  Interface-1 : Partial output buffers:
    {
      XzUnpacker_Init()
      for()
      {
        XzUnpacker_Code();
      }
      XzUnpacker_IsStreamWasFinished()
    }
    
  Interface-2 : Direct output buffer:
    Use it, if you know exact size of decoded data, and you need
    whole xz unpacked data in one output buffer.
    xz unpacker doesn't allocate additional buffer for lzma2 dictionary in that mode.
    {
      XzUnpacker_Init()
      XzUnpacker_SetOutBufMode(); // to set output buffer and size
      for()
      {
        XzUnpacker_Code(); // (dest = NULL) in XzUnpacker_Code()
      }
      XzUnpacker_IsStreamWasFinished()
    }

  Interface-3 : Direct output buffer : One call full decoding
    It unpacks whole input buffer to output buffer in one call.
    It uses Interface-2 internally.
    {
      XzUnpacker_CodeFull()
      XzUnpacker_IsStreamWasFinished()
    }
*/

/*
finishMode:
  It has meaning only if the decoding reaches output limit (*destLen).
  CODER_FINISH_ANY - use smallest number of input bytes
  CODER_FINISH_END - read EndOfStream marker after decoding

Returns:
  SZ_OK
    status:
      CODER_STATUS_NOT_FINISHED,
      CODER_STATUS_NEEDS_MORE_INPUT - the decoder can return it in two cases:
         1) it needs more input data to finish current xz stream
         2) xz stream was finished successfully. But the decoder supports multiple
            concatented xz streams. So it expects more input data for new xz streams.
         Call XzUnpacker_IsStreamWasFinished() to check that latest xz stream was finished successfully.

  SZ_ERROR_MEM  - Memory allocation error
  SZ_ERROR_DATA - Data error
  SZ_ERROR_UNSUPPORTED - Unsupported method or method properties
  SZ_ERROR_CRC  - CRC error
  // SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).

  SZ_ERROR_NO_ARCHIVE - the error with xz Stream Header with one of the following reasons:
     - xz Stream Signature failure
     - CRC32 of xz Stream Header is failed
     - The size of Stream padding is not multiple of four bytes.
    It's possible to get that error, if xz stream was finished and the stream
    contains some another data. In that case you can call XzUnpacker_GetExtraSize()
    function to get real size of xz stream.
*/


SRes XzUnpacker_Code(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, int srcFinished,
    ECoderFinishMode finishMode, ECoderStatus *status);

SRes XzUnpacker_CodeFull(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen,
    ECoderFinishMode finishMode, ECoderStatus *status);

/*
If you decode full xz stream(s), then you can call XzUnpacker_IsStreamWasFinished()
after successful XzUnpacker_CodeFull() or after last call of XzUnpacker_Code().
*/

BoolInt XzUnpacker_IsStreamWasFinished(const CXzUnpacker *p);

/*
XzUnpacker_GetExtraSize() returns then number of unconfirmed bytes,
 if it's in (XZ_STATE_STREAM_HEADER) state or in (XZ_STATE_STREAM_PADDING) state.
These bytes can be some data after xz archive, or
it can be start of new xz stream.
 
Call XzUnpacker_GetExtraSize() after XzUnpacker_Code() function to detect real size of
xz stream in two cases, if XzUnpacker_Code() returns:
  res == SZ_OK && status == CODER_STATUS_NEEDS_MORE_INPUT
  res == SZ_ERROR_NO_ARCHIVE
*/

UInt64 XzUnpacker_GetExtraSize(const CXzUnpacker *p);


/*
  for random block decoding:
    XzUnpacker_Init();
    set CXzUnpacker::streamFlags
    XzUnpacker_PrepareToRandomBlockDecoding()
    loop
    {
      XzUnpacker_Code()
      XzUnpacker_IsBlockFinished()
    }
*/

void XzUnpacker_PrepareToRandomBlockDecoding(CXzUnpacker *p);
BoolInt XzUnpacker_IsBlockFinished(const CXzUnpacker *p);

#define XzUnpacker_GetPackSizeForIndex(p) ((p)->packSize + (p)->blockHeaderSize + XzFlags_GetCheckSize((p)->streamFlags))






/* ---- Single-Thread and Multi-Thread xz Decoding with Input/Output Streams ---- */

/*
  if (CXzDecMtProps::numThreads > 1), the decoder can try to use
  Multi-Threading. The decoder analyses xz block header, and if
  there are pack size and unpack size values stored in xz block header,
  the decoder reads compressed data of block to internal buffers,
  and then it can start parallel decoding, if there are another blocks.
  The decoder can switch back to Single-Thread decoding after some conditions.

  The sequence of calls for xz decoding with in/out Streams:
  {
    XzDecMt_Create()
    XzDecMtProps_Init(XzDecMtProps) to set default values of properties
    // then you can change some XzDecMtProps parameters with required values
    // here you can set the number of threads and (memUseMax) - the maximum
    Memory usage for multithreading decoding.
    for()
    {
      XzDecMt_Decode() // one call per one file
    }
    XzDecMt_Destroy()
  }
*/


typedef struct
{
  size_t inBufSize_ST;    // size of input buffer for Single-Thread decoding
  size_t outStep_ST;      // size of output buffer for Single-Thread decoding
  BoolInt ignoreErrors;   // if set to 1, the decoder can ignore some errors and it skips broken parts of data.
  
  #ifndef Z7_ST
  unsigned numThreads;    // the number of threads for Multi-Thread decoding. if (umThreads == 1) it will use Single-thread decoding
  size_t inBufSize_MT;    // size of small input data buffers for Multi-Thread decoding. Big number of such small buffers can be created
  size_t memUseMax;       // the limit of total memory usage for Multi-Thread decoding.
                          // it's recommended to set (memUseMax) manually to value that is smaller of total size of RAM in computer.
  #endif
} CXzDecMtProps;

void XzDecMtProps_Init(CXzDecMtProps *p);

typedef struct CXzDecMt CXzDecMt;
typedef CXzDecMt * CXzDecMtHandle;
// Z7_DECLARE_HANDLE(CXzDecMtHandle)

/*
  alloc    : XzDecMt uses CAlignOffsetAlloc internally for addresses allocated by (alloc).
  allocMid : for big allocations, aligned allocation is better
*/

CXzDecMtHandle XzDecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid);
void XzDecMt_Destroy(CXzDecMtHandle p);


typedef struct
{
  Byte UnpackSize_Defined;
  Byte NumStreams_Defined;
  Byte NumBlocks_Defined;

  Byte DataAfterEnd;      // there are some additional data after good xz streams, and that data is not new xz stream.
  Byte DecodingTruncated; // Decoding was Truncated, we need only partial output data

  UInt64 InSize;          // pack size processed. That value doesn't include the data after
                          // end of xz stream, if that data was not correct
  UInt64 OutSize;

  UInt64 NumStreams;
  UInt64 NumBlocks;

  SRes DecodeRes;         // the error code of xz streams data decoding
  SRes ReadRes;           // error code from ISeqInStream:Read()
  SRes ProgressRes;       // error code from ICompressProgress:Progress()

  SRes CombinedRes;       // Combined result error code that shows main rusult
                          // = S_OK, if there is no error.
                          // but check also (DataAfterEnd) that can show additional minor errors.
 
  SRes CombinedRes_Type;  // = SZ_ERROR_READ,     if error from ISeqInStream
                          // = SZ_ERROR_PROGRESS, if error from ICompressProgress
                          // = SZ_ERROR_WRITE,    if error from ISeqOutStream
                          // = SZ_ERROR_* codes for decoding
} CXzStatInfo;

void XzStatInfo_Clear(CXzStatInfo *p);

/*

XzDecMt_Decode()
SRes: it's combined decoding result. It also is equal to stat->CombinedRes.

  SZ_OK               - no error
                        check also output value in (stat->DataAfterEnd)
                        that can show additional possible error

  SZ_ERROR_MEM        - Memory allocation error
  SZ_ERROR_NO_ARCHIVE - is not xz archive
  SZ_ERROR_ARCHIVE    - Headers error
  SZ_ERROR_DATA       - Data Error
  SZ_ERROR_UNSUPPORTED - Unsupported method or method properties
  SZ_ERROR_CRC        - CRC Error
  SZ_ERROR_INPUT_EOF  - it needs more input data
  SZ_ERROR_WRITE      - ISeqOutStream error
  (SZ_ERROR_READ)     - ISeqInStream errors
  (SZ_ERROR_PROGRESS) - ICompressProgress errors
  // SZ_ERROR_THREAD     - error in multi-threading functions
  MY_SRes_HRESULT_FROM_WRes(WRes_error) - error in multi-threading function
*/

SRes XzDecMt_Decode(CXzDecMtHandle p,
    const CXzDecMtProps *props,
    const UInt64 *outDataSize, // NULL means undefined
    int finishMode,            // 0 - partial unpacking is allowed, 1 - xz stream(s) must be finished
    ISeqOutStreamPtr outStream,
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    // const Byte *inData, size_t inDataSize,
    CXzStatInfo *stat,         // out: decoding results and statistics
    int *isMT,                 // out: 0 means that ST (Single-Thread) version was used
                               //      1 means that MT (Multi-Thread) version was used
    ICompressProgressPtr progress);

EXTERN_C_END

#endif

/* ---- C/XzCrc64.h ---- */
/* XzCrc64.h -- CRC64 calculation
2023-12-08 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_XZ_CRC64_H
#define ZIP7_INC_XZ_CRC64_H

#include <stddef.h>

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

// extern UInt64 g_Crc64Table[];

void Z7_FASTCALL Crc64GenerateTable(void);

#define CRC64_INIT_VAL UINT64_CONST(0xFFFFFFFFFFFFFFFF)
#define CRC64_GET_DIGEST(crc) ((crc) ^ CRC64_INIT_VAL)
// #define CRC64_UPDATE_BYTE(crc, b) (g_Crc64Table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

UInt64 Z7_FASTCALL Crc64Update(UInt64 crc, const void *data, size_t size);
// UInt64 Z7_FASTCALL Crc64Calc(const void *data, size_t size);

EXTERN_C_END

#endif

/* ---- C/Alloc.h ---- */
/* Alloc.h -- Memory allocation functions
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_ALLOC_H
#define ZIP7_INC_ALLOC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/*
  MyFree(NULL)        : is allowed, as free(NULL)
  MyAlloc(0)          : returns NULL : but malloc(0)        is allowed to return NULL or non_NULL
  MyRealloc(NULL, 0)  : returns NULL : but realloc(NULL, 0) is allowed to return NULL or non_NULL
MyRealloc() is similar to realloc() for the following cases:
  MyRealloc(non_NULL, 0)         : returns NULL and always calls MyFree(ptr)
  MyRealloc(NULL, non_ZERO)      : returns NULL, if allocation failed
  MyRealloc(non_NULL, non_ZERO)  : returns NULL, if reallocation failed
*/

void *MyAlloc(size_t size);
void MyFree(void *address);
void *MyRealloc(void *address, size_t size);

void *z7_AlignedAlloc(size_t size);
void  z7_AlignedFree(void *p);

extern const ISzAlloc g_Alloc;
extern const ISzAlloc g_AlignedAlloc;

#ifdef _WIN32
  void *MidAlloc(size_t size);
  void MidFree(void *address);
  extern const ISzAlloc g_MidAlloc;
#else
  #define MidAlloc(size)    z7_AlignedAlloc(size)
  #define MidFree(address)  z7_AlignedFree(address)
  #define g_MidAlloc g_AlignedAlloc
#endif

#ifdef Z7_LARGE_PAGES

#define Z7_LARGE_PAGES_FLAG_USE_HUGEPAGE  (1 << 0)  //    PAGE_ALIGNED / MADV_HUGEPAGE
#define Z7_LARGE_PAGES_FLAG_NO_PAGECODE   (1 << 1)  // no PAGE_ALIGNED / no madvise
#define Z7_LARGE_PAGES_FLAG_NO_MADVISE    (1 << 2)  //    PAGE_ALIGNED / no madvise : for THP=always
#define Z7_LARGE_PAGES_FLAG_NO_HUGEPAGE   (1 << 3)  //    PAGE_ALIGNED / MADV_NOHUGEPAGE
#define Z7_LARGE_PAGES_FLAG_FAIL_STOP     (1 << 15) // for benchmarks
#define Z7_LARGE_PAGES_FLAG_DIRECT_PAGE_SIZE  (1 << 16)
#define Z7_LARGE_PAGES_FLAG_DIRECT_THRESHOLD  (1 << 17)

void z7_LargePage_Set(UInt32 flags, size_t pageSize, size_t threshold);
  
  void *BigAlloc(size_t size);
  void BigFree(void *address);
  extern const ISzAlloc g_BigAlloc;
#else
  #define BigAlloc(size)    MidAlloc(size)
  #define BigFree(address)  MidFree(address)
  #define g_BigAlloc g_MidAlloc
#endif


typedef struct
{
  ISzAlloc vt;
  ISzAllocPtr baseAlloc;
  unsigned numAlignBits; /* ((1 << numAlignBits) >= sizeof(void *)) */
  size_t offset;         /* (offset == (k * sizeof(void *)) && offset < (1 << numAlignBits) */
} CAlignOffsetAlloc;

void AlignOffsetAlloc_CreateVTable(CAlignOffsetAlloc *p);


EXTERN_C_END

#endif

/* ---- C/Bra.h ---- */
/* Bra.h -- Branch converters for executables
2024-01-20 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BRA_H
#define ZIP7_INC_BRA_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/* #define PPC BAD_PPC_11 // for debug */

#define Z7_BRANCH_CONV_DEC_2(name)  z7_ ## name ## _Dec
#define Z7_BRANCH_CONV_ENC_2(name)  z7_ ## name ## _Enc
#define Z7_BRANCH_CONV_DEC(name)    Z7_BRANCH_CONV_DEC_2(BranchConv_ ## name)
#define Z7_BRANCH_CONV_ENC(name)    Z7_BRANCH_CONV_ENC_2(BranchConv_ ## name)
#define Z7_BRANCH_CONV_ST_DEC(name) z7_BranchConvSt_ ## name ## _Dec
#define Z7_BRANCH_CONV_ST_ENC(name) z7_BranchConvSt_ ## name ## _Enc

#define Z7_BRANCH_CONV_DECL(name)    Byte * name(Byte *data, SizeT size, UInt32 pc)
#define Z7_BRANCH_CONV_ST_DECL(name) Byte * name(Byte *data, SizeT size, UInt32 pc, UInt32 *state)

typedef Z7_BRANCH_CONV_DECL(   (*z7_Func_BranchConv));
typedef Z7_BRANCH_CONV_ST_DECL((*z7_Func_BranchConvSt));

#define Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL 0
Z7_BRANCH_CONV_ST_DECL (Z7_BRANCH_CONV_ST_DEC(X86));
Z7_BRANCH_CONV_ST_DECL (Z7_BRANCH_CONV_ST_ENC(X86));

#define Z7_BRANCH_FUNCS_DECL(name) \
Z7_BRANCH_CONV_DECL (Z7_BRANCH_CONV_DEC_2(name)); \
Z7_BRANCH_CONV_DECL (Z7_BRANCH_CONV_ENC_2(name));

Z7_BRANCH_FUNCS_DECL (BranchConv_ARM64)
Z7_BRANCH_FUNCS_DECL (BranchConv_ARM)
Z7_BRANCH_FUNCS_DECL (BranchConv_ARMT)
Z7_BRANCH_FUNCS_DECL (BranchConv_PPC)
Z7_BRANCH_FUNCS_DECL (BranchConv_SPARC)
Z7_BRANCH_FUNCS_DECL (BranchConv_IA64)
Z7_BRANCH_FUNCS_DECL (BranchConv_RISCV)

/*
These functions convert data that contain CPU instructions.
Each such function converts relative addresses to absolute addresses in some
branch instructions: CALL (in all converters) and JUMP (X86 converter only).
Such conversion allows to increase compression ratio, if we compress that data.

There are 2 types of converters:
  Byte * Conv_RISC (Byte *data, SizeT size, UInt32 pc);
  Byte * ConvSt_X86(Byte *data, SizeT size, UInt32 pc, UInt32 *state);
Each Converter supports 2 versions: one for encoding
and one for decoding (_Enc/_Dec postfixes in function name).

In params:
  data  : data buffer
  size  : size of data
  pc    : current virtual Program Counter (Instruction Pointer) value
In/Out param:
  state : pointer to state variable (for X86 converter only)

Return:
  The pointer to position in (data) buffer after last byte that was processed.
  If the caller calls converter again, it must call it starting with that position.
  But the caller is allowed to move data in buffer. So pointer to
  current processed position also will be changed for next call.
  Also the caller must increase internal (pc) value for next call.
  
Each converter has some characteristics: Endian, Alignment, LookAhead.
  Type   Endian  Alignment  LookAhead
  
  X86    little      1          4
  ARMT   little      2          2
  RISCV  little      2          6
  ARM    little      4          0
  ARM64  little      4          0
  PPC     big        4          0
  SPARC   big        4          0
  IA64   little     16          0

  (data) must be aligned for (Alignment).
  processed size can be calculated as:
    SizeT processed = Conv(data, size, pc) - data;
  if (processed == 0)
    it means that converter needs more data for processing.
  If (size < Alignment + LookAhead)
    then (processed == 0) is allowed.

Example code for conversion in loop:
  UInt32 pc = 0;
  size = 0;
  for (;;)
  {
    size += Load_more_input_data(data + size);
    SizeT processed = Conv(data, size, pc) - data;
    if (processed == 0 && no_more_input_data_after_size)
      break; // we stop convert loop
    data += processed;
    size -= processed;
    pc += processed;
  }
*/

EXTERN_C_END

#endif

/* ---- C/LzmaDec.h ---- */
/* LzmaDec.h -- LZMA Decoder
2023-04-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZMA_DEC_H
#define ZIP7_INC_LZMA_DEC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/* #define Z7_LZMA_PROB32 */
/* Z7_LZMA_PROB32 can increase the speed on some CPUs,
   but memory usage for CLzmaDec::probs will be doubled in that case */

typedef
#ifdef Z7_LZMA_PROB32
  UInt32
#else
  UInt16
#endif
  CLzmaProb;


/* ---------- LZMA Properties ---------- */

#define LZMA_PROPS_SIZE 5

typedef struct
{
  Byte lc;
  Byte lp;
  Byte pb;
  Byte _pad_;
  UInt32 dicSize;
} CLzmaProps;

/* LzmaProps_Decode - decodes properties
Returns:
  SZ_OK
  SZ_ERROR_UNSUPPORTED - Unsupported properties
*/

SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size);


/* ---------- LZMA Decoder state ---------- */

/* LZMA_REQUIRED_INPUT_MAX = number of required input bytes for worst case.
   Num bits = log2((2^11 / 31) ^ 22) + 26 < 134 + 26 = 160; */

#define LZMA_REQUIRED_INPUT_MAX 20

typedef struct
{
  /* Don't change this structure. ASM code can use it. */
  CLzmaProps prop;
  CLzmaProb *probs;
  CLzmaProb *probs_1664;
  Byte *dic;
  SizeT dicBufSize;
  SizeT dicPos;
  const Byte *buf;
  UInt32 range;
  UInt32 code;
  UInt32 processedPos;
  UInt32 checkDicSize;
  UInt32 reps[4];
  UInt32 state;
  UInt32 remainLen;

  UInt32 numProbs;
  unsigned tempBufSize;
  Byte tempBuf[LZMA_REQUIRED_INPUT_MAX];
} CLzmaDec;

#define LzmaDec_CONSTRUCT(p) { (p)->dic = NULL; (p)->probs = NULL; }
#define LzmaDec_Construct(p) LzmaDec_CONSTRUCT(p)

void LzmaDec_Init(CLzmaDec *p);

/* There are two types of LZMA streams:
     - Stream with end mark. That end mark adds about 6 bytes to compressed size.
     - Stream without end mark. You must know exact uncompressed size to decompress such stream. */

typedef enum
{
  LZMA_FINISH_ANY,   /* finish at any point */
  LZMA_FINISH_END    /* block must be finished at the end */
} ELzmaFinishMode;

/* ELzmaFinishMode has meaning only if the decoding reaches output limit !!!

   You must use LZMA_FINISH_END, when you know that current output buffer
   covers last bytes of block. In other cases you must use LZMA_FINISH_ANY.

   If LZMA decoder sees end marker before reaching output limit, it returns SZ_OK,
   and output value of destLen will be less than output buffer size limit.
   You can check status result also.

   You can use multiple checks to test data integrity after full decompression:
     1) Check Result and "status" variable.
     2) Check that output(destLen) = uncompressedSize, if you know real uncompressedSize.
     3) Check that output(srcLen) = compressedSize, if you know real compressedSize.
        You must use correct finish mode in that case. */

typedef enum
{
  LZMA_STATUS_NOT_SPECIFIED,               /* use main error code instead */
  LZMA_STATUS_FINISHED_WITH_MARK,          /* stream was finished with end mark. */
  LZMA_STATUS_NOT_FINISHED,                /* stream was not finished */
  LZMA_STATUS_NEEDS_MORE_INPUT,            /* you must provide more input bytes */
  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK  /* there is probability that stream was finished without end mark */
} ELzmaStatus;

/* ELzmaStatus is used only as output value for function call */


/* ---------- Interfaces ---------- */

/* There are 3 levels of interfaces:
     1) Dictionary Interface
     2) Buffer Interface
     3) One Call Interface
   You can select any of these interfaces, but don't mix functions from different
   groups for same object. */


/* There are two variants to allocate state for Dictionary Interface:
     1) LzmaDec_Allocate / LzmaDec_Free
     2) LzmaDec_AllocateProbs / LzmaDec_FreeProbs
   You can use variant 2, if you set dictionary buffer manually.
   For Buffer Interface you must always use variant 1.

LzmaDec_Allocate* can return:
  SZ_OK
  SZ_ERROR_MEM         - Memory allocation error
  SZ_ERROR_UNSUPPORTED - Unsupported properties
*/
   
SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc);
void LzmaDec_FreeProbs(CLzmaDec *p, ISzAllocPtr alloc);

SRes LzmaDec_Allocate(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc);
void LzmaDec_Free(CLzmaDec *p, ISzAllocPtr alloc);

/* ---------- Dictionary Interface ---------- */

/* You can use it, if you want to eliminate the overhead for data copying from
   dictionary to some other external buffer.
   You must work with CLzmaDec variables directly in this interface.

   STEPS:
     LzmaDec_Construct()
     LzmaDec_Allocate()
     for (each new stream)
     {
       LzmaDec_Init()
       while (it needs more decompression)
       {
         LzmaDec_DecodeToDic()
         use data from CLzmaDec::dic and update CLzmaDec::dicPos
       }
     }
     LzmaDec_Free()
*/

/* LzmaDec_DecodeToDic
   
   The decoding to internal dictionary buffer (CLzmaDec::dic).
   You must manually update CLzmaDec::dicPos, if it reaches CLzmaDec::dicBufSize !!!

finishMode:
  It has meaning only if the decoding reaches output limit (dicLimit).
  LZMA_FINISH_ANY - Decode just dicLimit bytes.
  LZMA_FINISH_END - Stream must be finished after dicLimit.

Returns:
  SZ_OK
    status:
      LZMA_STATUS_FINISHED_WITH_MARK
      LZMA_STATUS_NOT_FINISHED
      LZMA_STATUS_NEEDS_MORE_INPUT
      LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
  SZ_ERROR_DATA - Data error
  SZ_ERROR_FAIL - Some unexpected error: internal error of code, memory corruption or hardware failure
*/

SRes LzmaDec_DecodeToDic(CLzmaDec *p, SizeT dicLimit,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);


/* ---------- Buffer Interface ---------- */

/* It's zlib-like interface.
   See LzmaDec_DecodeToDic description for information about STEPS and return results,
   but you must use LzmaDec_DecodeToBuf instead of LzmaDec_DecodeToDic and you don't need
   to work with CLzmaDec variables manually.

finishMode:
  It has meaning only if the decoding reaches output limit (*destLen).
  LZMA_FINISH_ANY - Decode just destLen bytes.
  LZMA_FINISH_END - Stream must be finished after (*destLen).
*/

SRes LzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);


/* ---------- One Call Interface ---------- */

/* LzmaDecode

finishMode:
  It has meaning only if the decoding reaches output limit (*destLen).
  LZMA_FINISH_ANY - Decode just destLen bytes.
  LZMA_FINISH_END - Stream must be finished after (*destLen).

Returns:
  SZ_OK
    status:
      LZMA_STATUS_FINISHED_WITH_MARK
      LZMA_STATUS_NOT_FINISHED
      LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
  SZ_ERROR_DATA - Data error
  SZ_ERROR_MEM  - Memory allocation error
  SZ_ERROR_UNSUPPORTED - Unsupported properties
  SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).
  SZ_ERROR_FAIL - Some unexpected error: internal error of code, memory corruption or hardware failure
*/

SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode,
    ELzmaStatus *status, ISzAllocPtr alloc);

EXTERN_C_END

#endif

/* ---- C/Lzma2Dec.h ---- */
/* Lzma2Dec.h -- LZMA2 Decoder
2023-03-03 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZMA2_DEC_H
#define ZIP7_INC_LZMA2_DEC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/* ---------- State Interface ---------- */

typedef struct
{
  unsigned state;
  Byte control;
  Byte needInitLevel;
  Byte isExtraMode;
  Byte _pad_;
  UInt32 packSize;
  UInt32 unpackSize;
  CLzmaDec decoder;
} CLzma2Dec;

#define Lzma2Dec_CONSTRUCT(p)  LzmaDec_CONSTRUCT(&(p)->decoder)
#define Lzma2Dec_Construct(p)  Lzma2Dec_CONSTRUCT(p)
#define Lzma2Dec_FreeProbs(p, alloc)  LzmaDec_FreeProbs(&(p)->decoder, alloc)
#define Lzma2Dec_Free(p, alloc)  LzmaDec_Free(&(p)->decoder, alloc)

SRes Lzma2Dec_AllocateProbs(CLzma2Dec *p, Byte prop, ISzAllocPtr alloc);
SRes Lzma2Dec_Allocate(CLzma2Dec *p, Byte prop, ISzAllocPtr alloc);
void Lzma2Dec_Init(CLzma2Dec *p);

/*
finishMode:
  It has meaning only if the decoding reaches output limit (*destLen or dicLimit).
  LZMA_FINISH_ANY - use smallest number of input bytes
  LZMA_FINISH_END - read EndOfStream marker after decoding

Returns:
  SZ_OK
    status:
      LZMA_STATUS_FINISHED_WITH_MARK
      LZMA_STATUS_NOT_FINISHED
      LZMA_STATUS_NEEDS_MORE_INPUT
  SZ_ERROR_DATA - Data error
*/

SRes Lzma2Dec_DecodeToDic(CLzma2Dec *p, SizeT dicLimit,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);

SRes Lzma2Dec_DecodeToBuf(CLzma2Dec *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);


/* ---------- LZMA2 block and chunk parsing ---------- */

/*
Lzma2Dec_Parse() parses compressed data stream up to next independent block or next chunk data.
It can return LZMA_STATUS_* code or LZMA2_PARSE_STATUS_* code:
  - LZMA2_PARSE_STATUS_NEW_BLOCK - there is new block, and 1 additional byte (control byte of next block header) was read from input.
  - LZMA2_PARSE_STATUS_NEW_CHUNK - there is new chunk, and only lzma2 header of new chunk was read.
                                   CLzma2Dec::unpackSize contains unpack size of that chunk
*/

typedef enum
{
/*
  LZMA_STATUS_NOT_SPECIFIED                 // data error
  LZMA_STATUS_FINISHED_WITH_MARK
  LZMA_STATUS_NOT_FINISHED                  //
  LZMA_STATUS_NEEDS_MORE_INPUT
  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK   // unused
*/
  LZMA2_PARSE_STATUS_NEW_BLOCK = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK + 1,
  LZMA2_PARSE_STATUS_NEW_CHUNK
} ELzma2ParseStatus;

ELzma2ParseStatus Lzma2Dec_Parse(CLzma2Dec *p,
    SizeT outSize,   // output size
    const Byte *src, SizeT *srcLen,
    int checkFinishBlock   // set (checkFinishBlock = 1), if it must read full input data, if decoder.dicPos reaches blockMax position.
    );

/*
LZMA2 parser doesn't decode LZMA chunks, so we must read
  full input LZMA chunk to decode some part of LZMA chunk.

Lzma2Dec_GetUnpackExtra() returns the value that shows
    max possible number of output bytes that can be output by decoder
    at current input positon.
*/

#define Lzma2Dec_GetUnpackExtra(p)  ((p)->isExtraMode ? (p)->unpackSize : 0)


/* ---------- One Call Interface ---------- */

/*
finishMode:
  It has meaning only if the decoding reaches output limit (*destLen).
  LZMA_FINISH_ANY - use smallest number of input bytes
  LZMA_FINISH_END - read EndOfStream marker after decoding

Returns:
  SZ_OK
    status:
      LZMA_STATUS_FINISHED_WITH_MARK
      LZMA_STATUS_NOT_FINISHED
  SZ_ERROR_DATA - Data error
  SZ_ERROR_MEM  - Memory allocation error
  SZ_ERROR_UNSUPPORTED - Unsupported properties
  SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).
*/

SRes Lzma2Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    Byte prop, ELzmaFinishMode finishMode, ELzmaStatus *status, ISzAllocPtr alloc);

EXTERN_C_END

#endif

/* ---- C/MtDec.h ---- */
/* MtDec.h -- Multi-thread Decoder
2023-04-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_MT_DEC_H
#define ZIP7_INC_MT_DEC_H

// amalgamation: header emitted in prologue

#ifndef Z7_ST
// amalgamation: header emitted in prologue
#endif

EXTERN_C_BEGIN

#ifndef Z7_ST

#ifndef Z7_ST
  #define MTDEC_THREADS_MAX 32
#else
  #define MTDEC_THREADS_MAX 1
#endif


typedef struct
{
  ICompressProgressPtr progress;
  SRes res;
  UInt64 totalInSize;
  UInt64 totalOutSize;
  CCriticalSection cs;
} CMtProgress;

void MtProgress_Init(CMtProgress *p, ICompressProgressPtr progress);
SRes MtProgress_Progress_ST(CMtProgress *p);
SRes MtProgress_ProgressAdd(CMtProgress *p, UInt64 inSize, UInt64 outSize);
SRes MtProgress_GetError(CMtProgress *p);
void MtProgress_SetError(CMtProgress *p, SRes res);

struct CMtDec_;

typedef struct
{
  struct CMtDec_ *mtDec;
  unsigned index;
  void *inBuf;

  size_t inDataSize_Start; // size of input data in start block
  UInt64 inDataSize;       // total size of input data in all blocks

  CThread thread;
  CAutoResetEvent canRead;
  CAutoResetEvent canWrite;
  void  *allocaPtr;
} CMtDecThread;

void MtDecThread_FreeInBufs(CMtDecThread *t);


typedef enum
{
  MTDEC_PARSE_CONTINUE, // continue this block with more input data
  MTDEC_PARSE_OVERFLOW, // MT buffers overflow, need switch to single-thread
  MTDEC_PARSE_NEW,      // new block
  MTDEC_PARSE_END       // end of block threading. But we still can return to threading after Write(&needContinue)
} EMtDecParseState;

typedef struct
{
  // in
  int startCall;
  const Byte *src;
  size_t srcSize;
      // in  : (srcSize == 0) is allowed
      // out : it's allowed to return less that actually was used ?
  int srcFinished;

  // out
  EMtDecParseState state;
  BoolInt canCreateNewThread;
  UInt64 outPos; // check it (size_t)
} CMtDecCallbackInfo;


typedef struct
{
  void (*Parse)(void *p, unsigned coderIndex, CMtDecCallbackInfo *ci);
  
  // PreCode() and Code():
  // (SRes_return_result != SZ_OK) means stop decoding, no need another blocks
  SRes (*PreCode)(void *p, unsigned coderIndex);
  SRes (*Code)(void *p, unsigned coderIndex,
      const Byte *src, size_t srcSize, int srcFinished,
      UInt64 *inCodePos, UInt64 *outCodePos, int *stop);
  // stop - means stop another Code calls


  /* Write() must be called, if Parse() was called
      set (needWrite) if
      {
         && (was not interrupted by progress)
         && (was not interrupted in previous block)
      }

    out:
      if (*needContinue), decoder still need to continue decoding with new iteration,
         even after MTDEC_PARSE_END
      if (*canRecode), we didn't flush current block data, so we still can decode current block later.
  */
  SRes (*Write)(void *p, unsigned coderIndex,
      BoolInt needWriteToStream,
      const Byte *src, size_t srcSize, BoolInt isCross,
      // int srcFinished,
      BoolInt *needContinue,
      BoolInt *canRecode);

} IMtDecCallback2;



typedef struct CMtDec_
{
  /* input variables */
  
  size_t inBufSize;        /* size of input block */
  unsigned numThreadsMax;
  // size_t inBlockMax;
  unsigned numThreadsMax_2;

  ISeqInStreamPtr inStream;
  // const Byte *inData;
  // size_t inDataSize;

  ICompressProgressPtr progress;
  ISzAllocPtr alloc;

  IMtDecCallback2 *mtCallback;
  void *mtCallbackObject;

  
  /* internal variables */
  
  size_t allocatedBufsSize;

  BoolInt exitThread;
  WRes exitThreadWRes;

  UInt64 blockIndex;
  BoolInt isAllocError;
  BoolInt overflow;
  SRes threadingErrorSRes;

  BoolInt needContinue;

  // CAutoResetEvent finishedEvent;

  SRes readRes;
  SRes codeRes;

  BoolInt wasInterrupted;

  unsigned numStartedThreads_Limit;
  unsigned numStartedThreads;

  Byte *crossBlock;
  size_t crossStart;
  size_t crossEnd;
  UInt64 readProcessed;
  BoolInt readWasFinished;
  UInt64 inProcessed;

  unsigned filledThreadStart;
  unsigned numFilledThreads;

  #ifndef Z7_ST
  BoolInt needInterrupt;
  UInt64 interruptIndex;
  CMtProgress mtProgress;
  CMtDecThread threads[MTDEC_THREADS_MAX];
  #endif
} CMtDec;


void MtDec_Construct(CMtDec *p);
void MtDec_Destruct(CMtDec *p);

/*
MtDec_Code() returns:
  SZ_OK - in most cases
  MY_SRes_HRESULT_FROM_WRes(WRes_error) - in case of unexpected error in threading function
*/
  
SRes MtDec_Code(CMtDec *p);
Byte *MtDec_GetCrossBuff(CMtDec *p);

int MtDec_PrepareRead(CMtDec *p);
const Byte *MtDec_Read(CMtDec *p, size_t *inLim);

#endif

EXTERN_C_END

#endif

/* ---- C/LzmaEnc.h ---- */
/*  LzmaEnc.h -- LZMA Encoder
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZMA_ENC_H
#define ZIP7_INC_LZMA_ENC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define LZMA_PROPS_SIZE 5

typedef struct
{
  int level;       /* 0 <= level <= 9 */
  UInt32 dictSize; /* (1 << 12) <= dictSize <= (1 << 27) for 32-bit version
                      (1 << 12) <= dictSize <= (3 << 29) for 64-bit version
                      default = (1 << 24) */
  int lc;          /* 0 <= lc <= 8, default = 3 */
  int lp;          /* 0 <= lp <= 4, default = 0 */
  int pb;          /* 0 <= pb <= 4, default = 2 */
  int algo;        /* 0 - fast, 1 - normal, default = 1 */
  int fb;          /* 5 <= fb <= 273, default = 32 */
  int btMode;      /* 0 - hashChain Mode, 1 - binTree mode - normal, default = 1 */
  int numHashBytes; /* 2, 3 or 4, default = 4 */
  unsigned numHashOutBits;  /* default = ? */
  UInt32 mc;       /* 1 <= mc <= (1 << 30), default = 32 */
  unsigned writeEndMark;  /* 0 - do not write EOPM, 1 - write EOPM, default = 0 */
  int numThreads;  /* 1 or 2, default = 2 */

  // int _pad;
  Int32 affinityGroup;

  UInt64 reduceSize; /* estimated size of data that will be compressed. default = (UInt64)(Int64)-1.
                        Encoder uses this value to reduce dictionary size */

  UInt64 affinity;
  UInt64 affinityInGroup;
} CLzmaEncProps;

void LzmaEncProps_Init(CLzmaEncProps *p);
void LzmaEncProps_Normalize(CLzmaEncProps *p);
UInt32 LzmaEncProps_GetDictSize(const CLzmaEncProps *props2);


/* ---------- CLzmaEncHandle Interface ---------- */

/* LzmaEnc* functions can return the following exit codes:
SRes:
  SZ_OK           - OK
  SZ_ERROR_MEM    - Memory allocation error
  SZ_ERROR_PARAM  - Incorrect paramater in props
  SZ_ERROR_WRITE  - ISeqOutStream write callback error
  SZ_ERROR_OUTPUT_EOF - output buffer overflow - version with (Byte *) output
  SZ_ERROR_PROGRESS - some break from progress callback
  SZ_ERROR_THREAD - error in multithreading functions (only for Mt version)
*/

typedef struct CLzmaEnc CLzmaEnc;
typedef CLzmaEnc * CLzmaEncHandle;
// Z7_DECLARE_HANDLE(CLzmaEncHandle)

CLzmaEncHandle LzmaEnc_Create(ISzAllocPtr alloc);
void LzmaEnc_Destroy(CLzmaEncHandle p, ISzAllocPtr alloc, ISzAllocPtr allocBig);

SRes LzmaEnc_SetProps(CLzmaEncHandle p, const CLzmaEncProps *props);
void LzmaEnc_SetDataSize(CLzmaEncHandle p, UInt64 expectedDataSiize);
SRes LzmaEnc_WriteProperties(CLzmaEncHandle p, Byte *properties, SizeT *size);
unsigned LzmaEnc_IsWriteEndMark(CLzmaEncHandle p);

SRes LzmaEnc_Encode(CLzmaEncHandle p, ISeqOutStreamPtr outStream, ISeqInStreamPtr inStream,
    ICompressProgressPtr progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_MemEncode(CLzmaEncHandle p, Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
    int writeEndMark, ICompressProgressPtr progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);


/* ---------- One Call Interface ---------- */

SRes LzmaEncode(Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
    const CLzmaEncProps *props, Byte *propsEncoded, SizeT *propsSize, int writeEndMark,
    ICompressProgressPtr progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);

EXTERN_C_END

#endif

/* ---- C/Lzma2Enc.h ---- */
/* Lzma2Enc.h -- LZMA2 Encoder
2023-04-13 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZMA2_ENC_H
#define ZIP7_INC_LZMA2_ENC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO   0
#define LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID  ((UInt64)(Int64)-1)

typedef struct
{
  CLzmaEncProps lzmaProps;
  UInt64 blockSize;
  int numBlockThreads_Reduced;
  int numBlockThreads_Max;
  int numTotalThreads;
  unsigned numThreadGroups; // 0 : no groups
} CLzma2EncProps;

void Lzma2EncProps_Init(CLzma2EncProps *p);
void Lzma2EncProps_Normalize(CLzma2EncProps *p);

/* ---------- CLzmaEnc2Handle Interface ---------- */

/* Lzma2Enc_* functions can return the following exit codes:
SRes:
  SZ_OK           - OK
  SZ_ERROR_MEM    - Memory allocation error
  SZ_ERROR_PARAM  - Incorrect paramater in props
  SZ_ERROR_WRITE  - ISeqOutStream write callback error
  SZ_ERROR_OUTPUT_EOF - output buffer overflow - version with (Byte *) output
  SZ_ERROR_PROGRESS - some break from progress callback
  SZ_ERROR_THREAD - error in multithreading functions (only for Mt version)
*/

typedef struct CLzma2Enc CLzma2Enc;
typedef CLzma2Enc * CLzma2EncHandle;
// Z7_DECLARE_HANDLE(CLzma2EncHandle)

CLzma2EncHandle Lzma2Enc_Create(ISzAllocPtr alloc, ISzAllocPtr allocBig);
void Lzma2Enc_Destroy(CLzma2EncHandle p);
SRes Lzma2Enc_SetProps(CLzma2EncHandle p, const CLzma2EncProps *props);
void Lzma2Enc_SetDataSize(CLzma2EncHandle p, UInt64 expectedDataSiize);
Byte Lzma2Enc_WriteProperties(CLzma2EncHandle p);
SRes Lzma2Enc_Encode2(CLzma2EncHandle p,
    ISeqOutStreamPtr outStream,
    Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    const Byte *inData, size_t inDataSize,
    ICompressProgressPtr progress);

EXTERN_C_END

#endif

/* ---- C/XzEnc.h ---- */
/* XzEnc.h -- Xz Encode
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_XZ_ENC_H
#define ZIP7_INC_XZ_ENC_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN


#define XZ_PROPS_BLOCK_SIZE_AUTO   LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO
#define XZ_PROPS_BLOCK_SIZE_SOLID  LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID


typedef struct
{
  UInt32 id;
  UInt32 delta;
  UInt32 ip;
  int ipDefined;
} CXzFilterProps;

void XzFilterProps_Init(CXzFilterProps *p);


typedef struct
{
  CLzma2EncProps lzma2Props;
  CXzFilterProps filterProps;
  unsigned checkId;
  unsigned numThreadGroups; // 0 : no groups
  UInt64 blockSize;
  int numBlockThreads_Reduced;
  int numBlockThreads_Max;
  int numTotalThreads;
  int forceWriteSizesInHeader;
  UInt64 reduceSize;
} CXzProps;

void XzProps_Init(CXzProps *p);

typedef struct CXzEnc CXzEnc;
typedef CXzEnc * CXzEncHandle;
// Z7_DECLARE_HANDLE(CXzEncHandle)

CXzEncHandle XzEnc_Create(ISzAllocPtr alloc, ISzAllocPtr allocBig);
void XzEnc_Destroy(CXzEncHandle p);
SRes XzEnc_SetProps(CXzEncHandle p, const CXzProps *props);
void XzEnc_SetDataSize(CXzEncHandle p, UInt64 expectedDataSiize);
SRes XzEnc_Encode(CXzEncHandle p, ISeqOutStreamPtr outStream, ISeqInStreamPtr inStream, ICompressProgressPtr progress);

SRes Xz_Encode(ISeqOutStreamPtr outStream, ISeqInStreamPtr inStream,
    const CXzProps *props, ICompressProgressPtr progress);

SRes Xz_EncodeEmpty(ISeqOutStreamPtr outStream);

EXTERN_C_END

#endif

/* ---- C/MtCoder.h ---- */
/* MtCoder.h -- Multi-thread Coder
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_MT_CODER_H
#define ZIP7_INC_MT_CODER_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/*
  if (    defined MTCODER_USE_WRITE_THREAD) : main thread writes all data blocks to output stream
  if (not defined MTCODER_USE_WRITE_THREAD) : any coder thread can write data blocks to output stream
*/
/* #define MTCODER_USE_WRITE_THREAD */

#ifndef Z7_ST
  #define MTCODER_GET_NUM_BLOCKS_FROM_THREADS(numThreads) ((numThreads) + (numThreads) / 8 + 1)
  #define MTCODER_THREADS_MAX 256
  #define MTCODER_BLOCKS_MAX (MTCODER_GET_NUM_BLOCKS_FROM_THREADS(MTCODER_THREADS_MAX) + 3)
#else
  #define MTCODER_THREADS_MAX 1
  #define MTCODER_BLOCKS_MAX 1
#endif


#ifndef Z7_ST


typedef struct
{
  ICompressProgress vt;
  CMtProgress *mtProgress;
  UInt64 inSize;
  UInt64 outSize;
} CMtProgressThunk;

void MtProgressThunk_CreateVTable(CMtProgressThunk *p);
    
#define MtProgressThunk_INIT(p) { (p)->inSize = 0; (p)->outSize = 0; }


struct CMtCoder_;


typedef struct
{
  struct CMtCoder_ *mtCoder;
  unsigned index;
  int stop;
  Byte *inBuf;

  CAutoResetEvent startEvent;
  CThread thread;
} CMtCoderThread;


typedef struct
{
  SRes (*Code)(void *p, unsigned coderIndex, unsigned outBufIndex,
      const Byte *src, size_t srcSize, int finished);
  SRes (*Write)(void *p, unsigned outBufIndex);
} IMtCoderCallback2;


typedef struct
{
  SRes res;
  unsigned bufIndex;
  BoolInt finished;
} CMtCoderBlock;


typedef struct CMtCoder_
{
  /* input variables */
  
  size_t blockSize;        /* size of input block */
  unsigned numThreadsMax;
  unsigned numThreadGroups;
  UInt64 expectedDataSize;

  ISeqInStreamPtr inStream;
  const Byte *inData;
  size_t inDataSize;

  ICompressProgressPtr progress;
  ISzAllocPtr allocBig;

  IMtCoderCallback2 *mtCallback;
  void *mtCallbackObject;

  
  /* internal variables */
  
  size_t allocatedBufsSize;

  CAutoResetEvent readEvent;
  CSemaphore blocksSemaphore;

  BoolInt stopReading;
  SRes readRes;

  #ifdef MTCODER_USE_WRITE_THREAD
    CAutoResetEvent writeEvents[MTCODER_BLOCKS_MAX];
  #else
    CAutoResetEvent finishedEvent;
    SRes writeRes;
    unsigned writeIndex;
    Byte ReadyBlocks[MTCODER_BLOCKS_MAX];
    LONG numFinishedThreads;
  #endif

  unsigned numStartedThreadsLimit;
  unsigned numStartedThreads;

  unsigned numBlocksMax;
  unsigned blockIndex;
  UInt64 readProcessed;

  CCriticalSection cs;

  unsigned freeBlockHead;
  unsigned freeBlockList[MTCODER_BLOCKS_MAX];

  CMtProgress mtProgress;
  CMtCoderBlock blocks[MTCODER_BLOCKS_MAX];
  CMtCoderThread threads[MTCODER_THREADS_MAX];

  CThreadNextGroup nextGroup;
} CMtCoder;


void MtCoder_Construct(CMtCoder *p);
void MtCoder_Destruct(CMtCoder *p);
SRes MtCoder_Code(CMtCoder *p);


#endif


EXTERN_C_END

#endif

/* ---- C/ZstdDec.h ---- */
/* ZstdDec.h -- Zstd Decoder interfaces
2024-01-21 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_ZSTD_DEC_H
#define ZIP7_INC_ZSTD_DEC_H

EXTERN_C_BEGIN

typedef struct CZstdDec CZstdDec;
typedef CZstdDec * CZstdDecHandle;

CZstdDecHandle ZstdDec_Create(ISzAllocPtr alloc_Small, ISzAllocPtr alloc_Big);
void ZstdDec_Destroy(CZstdDecHandle p);

typedef enum
{
  ZSTD_STATUS_NOT_SPECIFIED,    /* use main error code instead */
  ZSTD_STATUS_FINISHED_FRAME,   /* data frame or skip frame was finished */
  ZSTD_STATUS_NOT_FINISHED,     /* just finished non-empty block or unfinished RAW/RLE block */
  ZSTD_STATUS_NEEDS_MORE_INPUT, /* the callee needs more input bytes. It has more priority over ZSTD_STATUS_NOT_FINISHED */
  ZSTD_STATUS_OUT_REACHED       /* is not finihed frame and ((outProcessed > outSize) || (outProcessed == outSize && unfinished RAW/RLE block) */
} enum_ZstdStatus_Dummy;

#define ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(p) \
    ((p)->status & ZSTD_STATUS_FINISHED_FRAME)
/*
    ((p)->status == ZSTD_STATUS_NEEDS_MORE_INPUT || \
     (p)->status == ZSTD_STATUS_FINISHED_FRAME)
*/

typedef Byte enum_ZstdStatus;


void ZstdDec_Init(CZstdDecHandle p);

typedef struct
{
  UInt64 num_Blocks;
  Byte descriptor_OR;
  Byte descriptor_NOT_OR;
  Byte are_ContentSize_Unknown;
  Byte windowDescriptor_MAX;

  // Byte are_ContentSize_Known;
  // Byte are_SingleSegments;
  // Byte are_WindowDescriptors;
  Byte checksum_Defined;
  // Byte are_Checksums;
  // Byte are_Non_Checksums;
  
  // Byte are_DictionaryId;
  Byte are_DictionaryId_Different;
  
  // Byte reserved[3];

  UInt32 checksum;        // checksum of last data frame
  /// UInt32 dictionaryId_Cur;
  UInt32 dictionaryId; // if there are non-zero dictionary IDs, then it's first dictionaryId
  
  UInt64 num_DataFrames;
  UInt64 num_SkipFrames;
  UInt64 skipFrames_Size;
  UInt64 contentSize_Total;
  UInt64 contentSize_MAX;
  // UInt64 num_Checksums;
  // UInt64 num_Non_Checksums; // frames without checksum
  // UInt64 num_WindowDescriptors;
  // UInt64 num_SingleSegments;
  // UInt64 num_Frames_with_ContentSize;
  // UInt64 num_Frames_without_ContentSize;
  UInt64 windowSize_MAX;
  UInt64 windowSize_Allocate_MAX;
  // UInt64 num_DictionaryIds;
  // UInt64 num_Blocks_forType[4];
  // UInt64 num_BlockBytes_forType[4];
  // UInt64 num_SingleSegments;
  // UInt64 singleSegment_ContentSize_MAX;
} CZstdDecInfo;

#define ZstdDecInfo_CLEAR(p)  { memset(p, 0, sizeof(*(p))); }

#define ZstdDecInfo_GET_NUM_FRAMES(p)  ((p)->num_DataFrames + (p)->num_SkipFrames)


typedef struct CZstdDecState
{
  enum_ZstdStatus status; // out
  Byte disableHash;
  // Byte mustBeFinished;
  Byte outSize_Defined;
  // Byte isAfterSizeMode;
  // UInt64 inProcessed;
  // SRes codeRes;
  // Byte needWrite_IsStrong;

  const Byte *inBuf;
  size_t inPos;           // in/out
  size_t inLim;

  const Byte *win;        // out
  size_t winPos;          // out
  size_t wrPos;           // in/out
  // size_t cycSize;      // out : if (!outBuf_fromCaller)
  size_t needWrite_Size;  // out

  Byte *outBuf_fromCaller;
  size_t outBufSize_fromCaller;
  /* (outBufSize_fromCaller >= full_uncompressed_size_of_all_frames) is required
     for success decoding.
     If outBufSize_fromCaller < full_uncompressed_size_of_all_frames),
     decoding can give error message, because we decode per block basis.
  */

  // size_t outStep;
  UInt64 outSize;         // total in all frames
  UInt64 outProcessed;    // out decoded in all frames (it can be >= outSize)

  CZstdDecInfo info;
} CZstdDecState;

void ZstdDecState_Clear(CZstdDecState *p);

/*
ZstdDec_Decode()
return:
  SZ_OK                 - no error
  SZ_ERROR_DATA         - Data Error
  SZ_ERROR_MEM          - Memory allocation error
  SZ_ERROR_UNSUPPORTED  - Unsupported method or method properties
  SZ_ERROR_CRC          - XXH hash Error
  // SZ_ERROR_ARCHIVE   - Headers error (not used now)
*/
SRes ZstdDec_Decode(CZstdDecHandle dec, CZstdDecState *p);

/*
ZstdDec_ReadUnusedFromInBuf():
returns: the number of bytes that were read from InBuf
(*afterDecoding_tempPos) must be set to zero before first call of ZstdDec_ReadUnusedFromInBuf()
*/
size_t ZstdDec_ReadUnusedFromInBuf(
    CZstdDecHandle dec,
    size_t afterDecoding_tempPos, // in/out
    void *data, size_t size);

typedef struct
{
  SRes decode_SRes;   // error code of data decoding
  Byte is_NonFinishedFrame;  // there is unfinished decoding for data frame or skip frame
  Byte extraSize;
} CZstdDecResInfo;

/*
#define ZstdDecResInfo_CLEAR(p) \
{ (p)->decode_SRes = 0; \
  (p)->is_NonFinishedFrame; \
  (p)->extraSize = 0; \
}
// memset(p, 0, sizeof(*p));
*/

/*
additional error codes for CZstdDecResInfo::decode_SRes:
  SZ_ERROR_NO_ARCHIVE - is not zstd stream (no frames)
  SZ_ERROR_INPUT_EOF  - need more data in input stream
*/
void ZstdDec_GetResInfo(const CZstdDec *dec,
    const CZstdDecState *p,
    SRes res, // it's result from ZstdDec_Decode()
    CZstdDecResInfo *info);

EXTERN_C_END

#endif

/* ================ unit bodies ================ */

/* ================ unit: C/Sha256Opt.c ================ */
/* Sha256Opt.c -- SHA-256 optimized code for SHA-256 hardware instructions
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// #define Z7_USE_HW_SHA_STUB // for debug
#ifdef MY_CPU_X86_OR_AMD64
  #if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1600) // fix that check
      #define USE_HW_SHA
  #elif defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 30800) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 50100) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 40900)
      #define USE_HW_SHA
      #if !defined(__INTEL_COMPILER)
      // icc defines __GNUC__, but icc doesn't support __attribute__(__target__)
      #if !defined(__SHA__) || !defined(__SSSE3__)
        #define ATTRIB_SHA __attribute__((__target__("sha,ssse3")))
      #endif
      #endif
  #elif defined(_MSC_VER)
    #if (_MSC_VER >= 1900)
      #define USE_HW_SHA
    #else
      #define Z7_USE_HW_SHA_STUB
    #endif
  #endif
// #endif // MY_CPU_X86_OR_AMD64
#ifndef USE_HW_SHA
  // #define Z7_USE_HW_SHA_STUB // for debug
#endif

#ifdef USE_HW_SHA

// #pragma message("Sha256 HW")




// sse/sse2/ssse3:
#include <tmmintrin.h>
// sha*:
#include <immintrin.h>

#if defined (__clang__) && defined(_MSC_VER)
  #if !defined(__SHA__)
    #include <shaintrin.h>
  #endif
#else

#endif

/*
SHA256 uses:
SSE2:
  _mm_loadu_si128
  _mm_storeu_si128
  _mm_set_epi32
  _mm_add_epi32
  _mm_shuffle_epi32 / pshufd


  
SSSE3:
  _mm_shuffle_epi8 / pshufb
  _mm_alignr_epi8
SHA:
  _mm_sha256*
*/

// K array must be aligned for 16-bytes at least.
// The compiler can look align attribute and selects
//   movdqu - for code without align attribute
//   movdqa - for code with    align attribute
extern
MY_ALIGN(64)
const UInt32 SHA256_K_ARRAY[64];
#define K SHA256_K_ARRAY


#define ADD_EPI32(dest, src)      dest = _mm_add_epi32(dest, src);
#define SHA256_MSG1(dest, src)    dest = _mm_sha256msg1_epu32(dest, src);
#define SHA256_MSG2(dest, src)    dest = _mm_sha256msg2_epu32(dest, src);

#define LOAD_SHUFFLE(m, k) \
    m = _mm_loadu_si128((const __m128i *)(const void *)(data + (k) * 16)); \
    m = _mm_shuffle_epi8(m, mask); \

#define NNN(m0, m1, m2, m3)

#define SM1(m1, m2, m3, m0) \
    SHA256_MSG1(m0, m1); \

#define SM2(m2, m3, m0, m1) \
    ADD_EPI32(m0, _mm_alignr_epi8(m3, m2, 4)) \
    SHA256_MSG2(m0, m3); \

#define RND2(t0, t1) \
    t0 = _mm_sha256rnds2_epu32(t0, t1, msg);



#define R4(k, m0, m1, m2, m3, OP0, OP1) \
    msg = _mm_add_epi32(m0, *(const __m128i *) (const void *) &K[(k) * 4]); \
    RND2(state0, state1); \
    msg = _mm_shuffle_epi32(msg, 0x0E); \
    OP0(m0, m1, m2, m3) \
    RND2(state1, state0); \
    OP1(m0, m1, m2, m3) \

#define R16(k, OP0, OP1, OP2, OP3, OP4, OP5, OP6, OP7) \
    R4 ( (k)*4+0, m0,m1,m2,m3, OP0, OP1 ) \
    R4 ( (k)*4+1, m1,m2,m3,m0, OP2, OP3 ) \
    R4 ( (k)*4+2, m2,m3,m0,m1, OP4, OP5 ) \
    R4 ( (k)*4+3, m3,m0,m1,m2, OP6, OP7 ) \

#define PREPARE_STATE \
    tmp    = _mm_shuffle_epi32(state0, 0x1B); /* abcd */ \
    state0 = _mm_shuffle_epi32(state1, 0x1B); /* efgh */ \
    state1 = state0; \
    state0 = _mm_unpacklo_epi64(state0, tmp); /* cdgh */ \
    state1 = _mm_unpackhi_epi64(state1, tmp); /* abef */ \


void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  const __m128i mask = _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
   
  
  __m128i tmp, state0, state1;

  if (numBlocks == 0)
    return;

  state0 = _mm_loadu_si128((const __m128i *) (const void *) &state[0]);
  state1 = _mm_loadu_si128((const __m128i *) (const void *) &state[4]);
  
  PREPARE_STATE

  do
  {
    __m128i state0_save, state1_save;
    __m128i m0, m1, m2, m3;
    __m128i msg;
    // #define msg tmp

    state0_save = state0;
    state1_save = state1;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)



    R16 ( 0, NNN, NNN, SM1, NNN, SM1, SM2, SM1, SM2 )
    R16 ( 1, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 2, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 3, SM1, SM2, NNN, SM2, NNN, NNN, NNN, NNN )
    
    ADD_EPI32(state0, state0_save)
    ADD_EPI32(state1, state1_save)
    
    data += 64;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm_storeu_si128((__m128i *) (void *) &state[0], state0);
  _mm_storeu_si128((__m128i *) (void *) &state[4], state1);
}

#endif // USE_HW_SHA

#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE)
  
  #if   defined(__ARM_FEATURE_SHA2) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define USE_HW_SHA
  #else
    #if  defined(MY_CPU_ARM64) \
      || defined(__ARM_ARCH) && (__ARM_ARCH >= 4) \
      || defined(Z7_MSC_VER_ORIGINAL)
    #if  defined(__ARM_FP) && \
          (   defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30800) \
           || defined(__GNUC__) && (__GNUC__ >= 6) \
          ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1910)
    #if  defined(MY_CPU_ARM64) \
      || !defined(Z7_CLANG_VERSION) \
      || defined(__ARM_NEON) && \
          (Z7_CLANG_VERSION < 170000 || \
           Z7_CLANG_VERSION > 170001)
      #define USE_HW_SHA
    #endif
    #endif
    #endif
  #endif

#ifdef USE_HW_SHA

// #pragma message("=== Sha256 HW === ")


#if defined(__clang__) || defined(__GNUC__)
#if !defined(__ARM_FEATURE_SHA2) && \
    !defined(__ARM_FEATURE_CRYPTO)
  #ifdef MY_CPU_ARM64
#if defined(__clang__)
    #define ATTRIB_SHA __attribute__((__target__("crypto")))
#else
    #define ATTRIB_SHA __attribute__((__target__("+crypto")))
#endif
  #else
#if defined(__clang__) && (__clang_major__ >= 1)
    #define ATTRIB_SHA __attribute__((__target__("armv8-a,sha2")))
#else
    #define ATTRIB_SHA __attribute__((__target__("fpu=crypto-neon-fp-armv8")))
#endif
  #endif
#endif
#else
  // _MSC_VER
  // for arm32
  #define _ARM_USE_NEW_NEON_INTRINSICS
#endif

#if defined(Z7_MSC_VER_ORIGINAL) && defined(MY_CPU_ARM64)
#include <arm64_neon.h>
#else

#if defined(__clang__) && __clang_major__ < 16
#if !defined(__ARM_FEATURE_SHA2) && \
    !defined(__ARM_FEATURE_CRYPTO)
//     #pragma message("=== we set __ARM_FEATURE_CRYPTO 1 === ")
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #define Z7_ARM_FEATURE_CRYPTO_WAS_SET 1
// #if defined(__clang__) && __clang_major__ < 13
    #define __ARM_FEATURE_CRYPTO 1
// #else
    #define __ARM_FEATURE_SHA2 1
// #endif
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif // clang

#if defined(__clang__)

#if defined(__ARM_ARCH) && __ARM_ARCH < 8
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
//    #pragma message("#define __ARM_ARCH 8")
    #undef  __ARM_ARCH
    #define __ARM_ARCH 8
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif

#endif // clang

#include <arm_neon.h>

#if defined(Z7_ARM_FEATURE_CRYPTO_WAS_SET) && \
    defined(__ARM_FEATURE_CRYPTO) && \
    defined(__ARM_FEATURE_SHA2)
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #undef __ARM_FEATURE_CRYPTO
    #undef __ARM_FEATURE_SHA2
    #undef Z7_ARM_FEATURE_CRYPTO_WAS_SET
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
//    #pragma message("=== we undefine __ARM_FEATURE_CRYPTO === ")
#endif

#endif // Z7_MSC_VER_ORIGINAL

typedef uint32x4_t v128;
// typedef __n128 v128; // MSVC

#ifdef MY_CPU_BE
  #define MY_rev32_for_LE(x) x
#else
  #define MY_rev32_for_LE(x) vrev32q_u8(x)
#endif

#if 1 // 0 for debug
// for arm32: it works slower by some reason than direct code
/*
for arm32 it generates:
MSVC-2022, GCC-9:
    vld1.32 {d18,d19}, [r10]
    vst1.32 {d4,d5}, [r3]
    vld1.8  {d20-d21}, [r4]
there is no align hint (like [r10:128]).  So instruction allows unaligned access
*/
#define LOAD_128_32(_p)       vld1q_u32(_p)
#define LOAD_128_8(_p)        vld1q_u8 (_p)
#define STORE_128_32(_p, _v)  vst1q_u32(_p, _v)
#else
/*
for arm32:
MSVC-2022:
    vldm r10,{d18,d19}
    vstm r3,{d4,d5}
    does it require strict alignment?
GCC-9:
    vld1.64 {d30-d31}, [r0:64]
    vldr  d28, [r0, #16]
    vldr  d29, [r0, #24]
    vst1.64 {d30-d31}, [r0:64]
    vstr  d28, [r0, #16]
    vstr  d29, [r0, #24]
there is hint [r0:64], so does it requires 64-bit alignment.
*/
#define LOAD_128_32(_p)       (*(const v128 *)(const void *)(_p))
#define LOAD_128_8(_p)        vreinterpretq_u8_u32(*(const v128 *)(const void *)(_p))
#define STORE_128_32(_p, _v)  *(v128 *)(void *)(_p) = (_v)
#endif

#define LOAD_SHUFFLE(m, k) \
    m = vreinterpretq_u32_u8( \
        MY_rev32_for_LE( \
        LOAD_128_8(data + (k) * 16))); \

// K array must be aligned for 16-bytes at least.
extern
MY_ALIGN(64)
const UInt32 SHA256_K_ARRAY[64];
#define K SHA256_K_ARRAY

#define SHA256_SU0(dest, src)        dest = vsha256su0q_u32(dest, src);
#define SHA256_SU1(dest, src2, src3) dest = vsha256su1q_u32(dest, src2, src3);

#define SM1(m0, m1, m2, m3)  SHA256_SU0(m3, m0)
#define SM2(m0, m1, m2, m3)  SHA256_SU1(m2, m0, m1)
#define NNN(m0, m1, m2, m3)

#define R4(k, m0, m1, m2, m3, OP0, OP1) \
    msg = vaddq_u32(m0, *(const v128 *) (const void *) &K[(k) * 4]); \
    tmp = state0; \
    state0 = vsha256hq_u32( state0, state1, msg ); \
    state1 = vsha256h2q_u32( state1, tmp, msg ); \
    OP0(m0, m1, m2, m3); \
    OP1(m0, m1, m2, m3); \


#define R16(k, OP0, OP1, OP2, OP3, OP4, OP5, OP6, OP7) \
    R4 ( (k)*4+0, m0, m1, m2, m3, OP0, OP1 ) \
    R4 ( (k)*4+1, m1, m2, m3, m0, OP2, OP3 ) \
    R4 ( (k)*4+2, m2, m3, m0, m1, OP4, OP5 ) \
    R4 ( (k)*4+3, m3, m0, m1, m2, OP6, OP7 ) \


void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  v128 state0, state1;

  if (numBlocks == 0)
    return;

  state0 = LOAD_128_32(&state[0]);
  state1 = LOAD_128_32(&state[4]);
  
  do
  {
    v128 state0_save, state1_save;
    v128 m0, m1, m2, m3;
    v128 msg, tmp;

    state0_save = state0;
    state1_save = state1;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)

    R16 ( 0, NNN, NNN, SM1, NNN, SM1, SM2, SM1, SM2 )
    R16 ( 1, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 2, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 3, SM1, SM2, NNN, SM2, NNN, NNN, NNN, NNN )
    
    state0 = vaddq_u32(state0, state0_save);
    state1 = vaddq_u32(state1, state1_save);
    
    data += 64;
  }
  while (--numBlocks);

  STORE_128_32(&state[0], state0);
  STORE_128_32(&state[4], state1);
}

#endif // USE_HW_SHA

#endif // MY_CPU_ARM_OR_ARM64


#if !defined(USE_HW_SHA) && defined(Z7_USE_HW_SHA_STUB)
// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>
// We can compile this file with another C compiler,
// or we can compile asm version.
// So we can generate real code instead of this stub function.
// #include "Sha256.h"
// #if defined(_MSC_VER)
#pragma message("Sha256 HW-SW stub was used")
// #endif
void Z7_FASTCALL Sha256_UpdateBlocks   (UInt32 state[8], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  Sha256_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}
#endif


#undef K
#undef RND2
#undef MY_rev32_for_LE

#undef NNN
#undef LOAD_128
#undef STORE_128
#undef LOAD_SHUFFLE
#undef SM1
#undef SM2


#undef R4
#undef R16
#undef PREPARE_STATE
#undef USE_HW_SHA
#undef ATTRIB_SHA
#undef USE_VER_MIN
#undef Z7_USE_HW_SHA_STUB

/* ================ unit: C/Sha3.c ================ */
/* Sha3.c -- SHA-3 Hash
: Igor Pavlov : Public domain
This code is based on public domain code from Wei Dai's Crypto++ library. */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define U64C(x) UINT64_CONST(x)

static
MY_ALIGN(64)
const UInt64 SHA3_K_ARRAY[24] =
{
  U64C(0x0000000000000001), U64C(0x0000000000008082),
  U64C(0x800000000000808a), U64C(0x8000000080008000),
  U64C(0x000000000000808b), U64C(0x0000000080000001),
  U64C(0x8000000080008081), U64C(0x8000000000008009),
  U64C(0x000000000000008a), U64C(0x0000000000000088),
  U64C(0x0000000080008009), U64C(0x000000008000000a),
  U64C(0x000000008000808b), U64C(0x800000000000008b),
  U64C(0x8000000000008089), U64C(0x8000000000008003),
  U64C(0x8000000000008002), U64C(0x8000000000000080),
  U64C(0x000000000000800a), U64C(0x800000008000000a),
  U64C(0x8000000080008081), U64C(0x8000000000008080),
  U64C(0x0000000080000001), U64C(0x8000000080008008)
};

void Sha3_Init(CSha3 *p)
{
  p->count = 0;
  memset(p->state, 0, sizeof(p->state));
}

#define GET_state(i, a)   UInt64 a = state[i];
#define SET_state(i, a)   state[i] = a;

#define LS_5(M, i, a0,a1,a2,a3,a4) \
        M ((i) * 5    , a0) \
        M ((i) * 5 + 1, a1) \
        M ((i) * 5 + 2, a2) \
        M ((i) * 5 + 3, a3) \
        M ((i) * 5 + 4, a4) \

#define LS_25(M) \
        LS_5 (M, 0, a50, a51, a52, a53, a54) \
        LS_5 (M, 1, a60, a61, a62, a63, a64) \
        LS_5 (M, 2, a70, a71, a72, a73, a74) \
        LS_5 (M, 3, a80, a81, a82, a83, a84) \
        LS_5 (M, 4, a90, a91, a92, a93, a94) \


#define XOR_1(i, a0) \
        a0 ^= GetUi64(data + (i) * 8); \

#define XOR_4(i, a0,a1,a2,a3) \
        XOR_1 ((i)    , a0); \
        XOR_1 ((i) + 1, a1); \
        XOR_1 ((i) + 2, a2); \
        XOR_1 ((i) + 3, a3); \

#define D(d,b1,b2) \
        d = b1 ^ Z7_ROTL64(b2, 1);

#define D5 \
        D (d0, c4, c1) \
        D (d1, c0, c2) \
        D (d2, c1, c3) \
        D (d3, c2, c4) \
        D (d4, c3, c0) \

#define C0(c,a,d) \
        c = a ^ d; \

#define C(c,a,d,k) \
        c = a ^ d; \
        c = Z7_ROTL64(c, k); \

#define E4(e1,e2,e3,e4) \
        e1 = c1 ^ (~c2 & c3); \
        e2 = c2 ^ (~c3 & c4); \
        e3 = c3 ^ (~c4 & c0); \
        e4 = c4 ^ (~c0 & c1); \

#define CK(   v0,w0,    \
              v1,w1,k1, \
              v2,w2,k2, \
              v3,w3,k3, \
              v4,w4,k4, e0,e1,e2,e3,e4, keccak_c) \
        C0(c0,v0,w0)    \
        C (c1,v1,w1,k1) \
        C (c2,v2,w2,k2) \
        C (c3,v3,w3,k3) \
        C (c4,v4,w4,k4) \
        e0 = c0 ^ (~c1 & c2) ^ keccak_c; \
        E4(e1,e2,e3,e4) \

#define CE(   v0,w0,k0, \
              v1,w1,k1, \
              v2,w2,k2, \
              v3,w3,k3, \
              v4,w4,k4, e0,e1,e2,e3,e4) \
        C (c0,v0,w0,k0) \
        C (c1,v1,w1,k1) \
        C (c2,v2,w2,k2) \
        C (c3,v3,w3,k3) \
        C (c4,v4,w4,k4) \
        e0 = c0 ^ (~c1 & c2); \
        E4(e1,e2,e3,e4) \

// numBlocks != 0
static
Z7_NO_INLINE
void Z7_FASTCALL Sha3_UpdateBlocks(UInt64 state[SHA3_NUM_STATE_WORDS],
    const Byte *data, size_t numBlocks, size_t blockSize)
{
  LS_25 (GET_state)

  do
  {
    unsigned round;
                              XOR_4 ( 0, a50, a51, a52, a53)
                              XOR_4 ( 4, a54, a60, a61, a62)
                              XOR_1 ( 8, a63)
    if (blockSize > 8 *  9) { XOR_4 ( 9, a64, a70, a71, a72)  // sha3-384
    if (blockSize > 8 * 13) { XOR_4 (13, a73, a74, a80, a81)  // sha3-256
    if (blockSize > 8 * 17) { XOR_1 (17, a82)                 // sha3-224
    if (blockSize > 8 * 18) { XOR_1 (18, a83)                 // shake128
                              XOR_1 (19, a84)
                              XOR_1 (20, a90) }}}}
    data += blockSize;

    for (round = 0; round < 24; round += 2)
    {
      UInt64 c0, c1, c2, c3, c4;
      UInt64 d0, d1, d2, d3, d4;
      UInt64 e50, e51, e52, e53, e54;
      UInt64 e60, e61, e62, e63, e64;
      UInt64 e70, e71, e72, e73, e74;
      UInt64 e80, e81, e82, e83, e84;
      UInt64 e90, e91, e92, e93, e94;

      c0 = a50^a60^a70^a80^a90;
      c1 = a51^a61^a71^a81^a91;
      c2 = a52^a62^a72^a82^a92;
      c3 = a53^a63^a73^a83^a93;
      c4 = a54^a64^a74^a84^a94;
      D5
      CK( a50, d0,
          a61, d1, 44,
          a72, d2, 43,
          a83, d3, 21,
          a94, d4, 14, e50, e51, e52, e53, e54, SHA3_K_ARRAY[round])
      CE( a53, d3, 28,
          a64, d4, 20,
          a70, d0,  3,
          a81, d1, 45,
          a92, d2, 61, e60, e61, e62, e63, e64)
      CE( a51, d1,  1,
          a62, d2,  6,
          a73, d3, 25,
          a84, d4,  8,
          a90, d0, 18, e70, e71, e72, e73, e74)
      CE( a54, d4, 27,
          a60, d0, 36,
          a71, d1, 10,
          a82, d2, 15,
          a93, d3, 56, e80, e81, e82, e83, e84)
      CE( a52, d2, 62,
          a63, d3, 55,
          a74, d4, 39,
          a80, d0, 41,
          a91, d1,  2, e90, e91, e92, e93, e94)
      
      // ---------- ROUND + 1 ----------

      c0 = e50^e60^e70^e80^e90;
      c1 = e51^e61^e71^e81^e91;
      c2 = e52^e62^e72^e82^e92;
      c3 = e53^e63^e73^e83^e93;
      c4 = e54^e64^e74^e84^e94;
      D5
      CK( e50, d0,
          e61, d1, 44,
          e72, d2, 43,
          e83, d3, 21,
          e94, d4, 14, a50, a51, a52, a53, a54, SHA3_K_ARRAY[(size_t)round + 1])
      CE( e53, d3, 28,
          e64, d4, 20,
          e70, d0,  3,
          e81, d1, 45,
          e92, d2, 61, a60, a61, a62, a63, a64)
      CE( e51, d1,  1,
          e62, d2,  6,
          e73, d3, 25,
          e84, d4,  8,
          e90, d0, 18, a70, a71, a72, a73, a74)
      CE (e54, d4, 27,
          e60, d0, 36,
          e71, d1, 10,
          e82, d2, 15,
          e93, d3, 56, a80, a81, a82, a83, a84)
      CE (e52, d2, 62,
          e63, d3, 55,
          e74, d4, 39,
          e80, d0, 41,
          e91, d1,  2, a90, a91, a92, a93, a94)
    }
  }
  while (--numBlocks);

  LS_25 (SET_state)
}


#define Sha3_UpdateBlock(p) \
        Sha3_UpdateBlocks(p->state, p->buffer, 1, p->blockSize)

void Sha3_Update(CSha3 *p, const Byte *data, size_t size)
{
/*
  for (;;)
  {
    if (size == 0)
      return;
    unsigned cur = p->blockSize - p->count;
    if (cur > size)
      cur = (unsigned)size;
    size -= cur;
    unsigned pos = p->count;
    p->count = pos + cur;
    while (pos & 7)
    {
      if (cur == 0)
        return;
      Byte *pb = &(((Byte *)p->state)[pos]);
      *pb = (Byte)(*pb ^ *data++);
      cur--;
      pos++;
    }
    if (cur >= 8)
    {
      do
      {
        *(UInt64 *)(void *)&(((Byte *)p->state)[pos]) ^= GetUi64(data);
        data += 8;
        pos += 8;
        cur -= 8;
      }
      while (cur >= 8);
    }
    if (pos != p->blockSize)
    {
      if (cur)
      {
        Byte *pb = &(((Byte *)p->state)[pos]);
        do
        {
          *pb = (Byte)(*pb ^ *data++);
          pb++;
        }
        while (--cur);
      }
      return;
    }
    Sha3_UpdateBlock(p->state);
    p->count = 0;
  }
*/
  if (size == 0)
    return;
  {
    const unsigned pos = p->count;
    const unsigned num = p->blockSize - pos;
    if (num > size)
    {
      p->count = pos + (unsigned)size;
      memcpy(p->buffer + pos, data, size);
      return;
    }
    if (pos != 0)
    {
      size -= num;
      memcpy(p->buffer + pos, data, num);
      data += num;
      Sha3_UpdateBlock(p);
    }
  }
  if (size >= p->blockSize)
  {
    const size_t numBlocks = size / p->blockSize;
    const Byte *dataOld = data;
    data += numBlocks * p->blockSize;
    size = (size_t)(dataOld + size - data);
    Sha3_UpdateBlocks(p->state, dataOld, numBlocks, p->blockSize);
  }
  p->count = (unsigned)size;
  if (size)
    memcpy(p->buffer, data, size);
}


// we support only (digestSize % 4 == 0) cases
void Sha3_Final(CSha3 *p, Byte *digest, unsigned digestSize, unsigned shake)
{
  memset(p->buffer + p->count, 0, p->blockSize - p->count);
  // we write bits markers from low to higher in current byte:
  //   - if sha-3 : 2 bits : 0,1
  //   - if shake : 4 bits : 1111
  // then we write bit 1 to same byte.
  // And we write bit 1 to highest bit of last byte of block.
  p->buffer[p->count] = (Byte)(shake ? 0x1f : 0x06);
  // we need xor operation (^= 0x80) here because we must write 0x80 bit
  // to same byte as (0x1f : 0x06), if (p->count == p->blockSize - 1) !!!
  p->buffer[p->blockSize - 1] ^= 0x80;
/*
  ((Byte *)p->state)[p->count] ^= (Byte)(shake ? 0x1f : 0x06);
  ((Byte *)p->state)[p->blockSize - 1] ^= 0x80;
*/
  Sha3_UpdateBlock(p);
#if 1 && defined(MY_CPU_LE)
  memcpy(digest, p->state, digestSize);
#else
  {
    const unsigned numWords = digestSize >> 3;
    unsigned i;
    for (i = 0; i < numWords; i++)
    {
      const UInt64 v = p->state[i];
      SetUi64(digest, v)
      digest += 8;
    }
    if (digestSize & 4) // for SHA3-224
    {
      const UInt32 v = (UInt32)p->state[numWords];
      SetUi32(digest, v)
    }
  }
#endif
  Sha3_Init(p);
}

#undef GET_state
#undef SET_state
#undef LS_5
#undef LS_25
#undef XOR_1
#undef XOR_4
#undef D
#undef D5
#undef C0
#undef C
#undef E4
#undef CK
#undef CE

/* ================ unit: C/Sha512.c ================ */
/* Sha512.c -- SHA-512 Hash
: Igor Pavlov : Public domain
This code is based on public domain code from Wei Dai's Crypto++ library. */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef MY_CPU_X86_OR_AMD64
  #if   defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 170001) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 170001) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 140000) \
     || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 2400) && (__INTEL_COMPILER <= 9900) \
     || defined(_MSC_VER) && (_MSC_VER >= 1940)
      #define Z7_COMPILER_SHA512_SUPPORTED
  #endif
#elif defined(MY_CPU_ARM64) && defined(MY_CPU_LE)
  #if defined(__ARM_FEATURE_SHA512)
    #define Z7_COMPILER_SHA512_SUPPORTED
  #else
    #if (defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 130000) \
           || defined(__GNUC__) && (__GNUC__ >= 9) \
        ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1940) // fix it
      #define Z7_COMPILER_SHA512_SUPPORTED
    #endif
  #endif
#endif














void Z7_FASTCALL Sha512_UpdateBlocks(UInt64 state[8], const Byte *data, size_t numBlocks);

#ifdef Z7_COMPILER_SHA512_SUPPORTED
  void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);

  static SHA512_FUNC_UPDATE_BLOCKS g_SHA512_FUNC_UPDATE_BLOCKS = Sha512_UpdateBlocks;
  static SHA512_FUNC_UPDATE_BLOCKS g_SHA512_FUNC_UPDATE_BLOCKS_HW;

  #define SHA512_UPDATE_BLOCKS(p) p->v.vars.func_UpdateBlocks
#else
  #define SHA512_UPDATE_BLOCKS(p) Sha512_UpdateBlocks
#endif


BoolInt Sha512_SetFunction(CSha512 *p, unsigned algo)
{
  SHA512_FUNC_UPDATE_BLOCKS func = Sha512_UpdateBlocks;
  
  #ifdef Z7_COMPILER_SHA512_SUPPORTED
    if (algo != SHA512_ALGO_SW)
    {
      if (algo == SHA512_ALGO_DEFAULT)
        func = g_SHA512_FUNC_UPDATE_BLOCKS;
      else
      {
        if (algo != SHA512_ALGO_HW)
          return False;
        func = g_SHA512_FUNC_UPDATE_BLOCKS_HW;
        if (!func)
          return False;
      }
    }
  #else
    if (algo > 1)
      return False;
  #endif

  p->v.vars.func_UpdateBlocks = func;
  return True;
}


/* define it for speed optimization */

#if 0 // 1 for size optimization
  #define STEP_PRE 1
  #define STEP_MAIN 1
#else
  #define STEP_PRE 2
  #define STEP_MAIN 4
  // #define Z7_SHA512_UNROLL
#endif

#undef Z7_SHA512_BIG_W
#if STEP_MAIN != 16
  #define Z7_SHA512_BIG_W
#endif


#define U64C(x) UINT64_CONST(x)

static MY_ALIGN(64) const UInt64 SHA512_INIT_ARRAYS[4][8] = {
{ U64C(0x8c3d37c819544da2), U64C(0x73e1996689dcd4d6), U64C(0x1dfab7ae32ff9c82), U64C(0x679dd514582f9fcf),
  U64C(0x0f6d2b697bd44da8), U64C(0x77e36f7304c48942), U64C(0x3f9d85a86a1d36c8), U64C(0x1112e6ad91d692a1)
},
{ U64C(0x22312194fc2bf72c), U64C(0x9f555fa3c84c64c2), U64C(0x2393b86b6f53b151), U64C(0x963877195940eabd),
  U64C(0x96283ee2a88effe3), U64C(0xbe5e1e2553863992), U64C(0x2b0199fc2c85b8aa), U64C(0x0eb72ddc81c52ca2)
},
{ U64C(0xcbbb9d5dc1059ed8), U64C(0x629a292a367cd507), U64C(0x9159015a3070dd17), U64C(0x152fecd8f70e5939),
  U64C(0x67332667ffc00b31), U64C(0x8eb44a8768581511), U64C(0xdb0c2e0d64f98fa7), U64C(0x47b5481dbefa4fa4)
},
{ U64C(0x6a09e667f3bcc908), U64C(0xbb67ae8584caa73b), U64C(0x3c6ef372fe94f82b), U64C(0xa54ff53a5f1d36f1),
  U64C(0x510e527fade682d1), U64C(0x9b05688c2b3e6c1f), U64C(0x1f83d9abfb41bd6b), U64C(0x5be0cd19137e2179)
}};

void Sha512_InitState(CSha512 *p, unsigned digestSize)
{
  p->v.vars.count = 0;
  memcpy(p->state, SHA512_INIT_ARRAYS[(size_t)(digestSize >> 4) - 1], sizeof(p->state));
}

void Sha512_Init(CSha512 *p, unsigned digestSize)
{
  p->v.vars.func_UpdateBlocks =
  #ifdef Z7_COMPILER_SHA512_SUPPORTED
      g_SHA512_FUNC_UPDATE_BLOCKS;
  #else
      NULL;
  #endif
  Sha512_InitState(p, digestSize);
}

#define S0(x) (Z7_ROTR64(x,28) ^ Z7_ROTR64(x,34) ^ Z7_ROTR64(x,39))
#define S1(x) (Z7_ROTR64(x,14) ^ Z7_ROTR64(x,18) ^ Z7_ROTR64(x,41))
#define s0(x) (Z7_ROTR64(x, 1) ^ Z7_ROTR64(x, 8) ^ (x >> 7))
#define s1(x) (Z7_ROTR64(x,19) ^ Z7_ROTR64(x,61) ^ (x >> 6))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))


#define W_PRE(i) (W[(i) + (size_t)(j)] = GetBe64(data + ((size_t)(j) + i) * 8))

#define blk2_main(j, i)  s1(w(j, (i)-2)) + w(j, (i)-7) + s0(w(j, (i)-15))

#ifdef Z7_SHA512_BIG_W
    // we use +i instead of +(i) to change the order to solve CLANG compiler warning for signed/unsigned.
    #define w(j, i)     W[(size_t)(j) + i]
    #define blk2(j, i)  (w(j, i) = w(j, (i)-16) + blk2_main(j, i))
#else
    #if STEP_MAIN == 16
        #define w(j, i)  W[(i) & 15]
    #else
        #define w(j, i)  W[((size_t)(j) + (i)) & 15]
    #endif
    #define blk2(j, i)  (w(j, i) += blk2_main(j, i))
#endif

#define W_MAIN(i)  blk2(j, i)


#define T1(wx, i) \
    tmp = h + S1(e) + Ch(e,f,g) + K[(i)+(size_t)(j)] + wx(i); \
    h = g; \
    g = f; \
    f = e; \
    e = d + tmp; \
    tmp += S0(a) + Maj(a, b, c); \
    d = c; \
    c = b; \
    b = a; \
    a = tmp; \

#define R1_PRE(i)  T1( W_PRE, i)
#define R1_MAIN(i) T1( W_MAIN, i)

#if (!defined(Z7_SHA512_UNROLL) || STEP_MAIN < 8) && (STEP_MAIN >= 4)
#define R2_MAIN(i) \
    R1_MAIN(i) \
    R1_MAIN(i + 1) \

#endif



#if defined(Z7_SHA512_UNROLL) && STEP_MAIN >= 8

#define T4( a,b,c,d,e,f,g,h, wx, i) \
    h += S1(e) + Ch(e,f,g) + K[(i)+(size_t)(j)] + wx(i); \
    tmp = h; \
    h += d; \
    d = tmp + S0(a) + Maj(a, b, c); \

#define R4( wx, i) \
    T4 ( a,b,c,d,e,f,g,h, wx, (i  )); \
    T4 ( d,a,b,c,h,e,f,g, wx, (i+1)); \
    T4 ( c,d,a,b,g,h,e,f, wx, (i+2)); \
    T4 ( b,c,d,a,f,g,h,e, wx, (i+3)); \

#define R4_PRE(i)  R4( W_PRE, i)
#define R4_MAIN(i) R4( W_MAIN, i)


#define T8( a,b,c,d,e,f,g,h, wx, i) \
    h += S1(e) + Ch(e,f,g) + K[(i)+(size_t)(j)] + wx(i); \
    d += h; \
    h += S0(a) + Maj(a, b, c); \

#define R8( wx, i) \
    T8 ( a,b,c,d,e,f,g,h, wx, i  ); \
    T8 ( h,a,b,c,d,e,f,g, wx, i+1); \
    T8 ( g,h,a,b,c,d,e,f, wx, i+2); \
    T8 ( f,g,h,a,b,c,d,e, wx, i+3); \
    T8 ( e,f,g,h,a,b,c,d, wx, i+4); \
    T8 ( d,e,f,g,h,a,b,c, wx, i+5); \
    T8 ( c,d,e,f,g,h,a,b, wx, i+6); \
    T8 ( b,c,d,e,f,g,h,a, wx, i+7); \

#define R8_PRE(i)  R8( W_PRE, i)
#define R8_MAIN(i) R8( W_MAIN, i)

#endif


extern
MY_ALIGN(64) const UInt64 SHA512_K_ARRAY[80];
MY_ALIGN(64) const UInt64 SHA512_K_ARRAY[80] = {
  U64C(0x428a2f98d728ae22), U64C(0x7137449123ef65cd), U64C(0xb5c0fbcfec4d3b2f), U64C(0xe9b5dba58189dbbc),
  U64C(0x3956c25bf348b538), U64C(0x59f111f1b605d019), U64C(0x923f82a4af194f9b), U64C(0xab1c5ed5da6d8118),
  U64C(0xd807aa98a3030242), U64C(0x12835b0145706fbe), U64C(0x243185be4ee4b28c), U64C(0x550c7dc3d5ffb4e2),
  U64C(0x72be5d74f27b896f), U64C(0x80deb1fe3b1696b1), U64C(0x9bdc06a725c71235), U64C(0xc19bf174cf692694),
  U64C(0xe49b69c19ef14ad2), U64C(0xefbe4786384f25e3), U64C(0x0fc19dc68b8cd5b5), U64C(0x240ca1cc77ac9c65),
  U64C(0x2de92c6f592b0275), U64C(0x4a7484aa6ea6e483), U64C(0x5cb0a9dcbd41fbd4), U64C(0x76f988da831153b5),
  U64C(0x983e5152ee66dfab), U64C(0xa831c66d2db43210), U64C(0xb00327c898fb213f), U64C(0xbf597fc7beef0ee4),
  U64C(0xc6e00bf33da88fc2), U64C(0xd5a79147930aa725), U64C(0x06ca6351e003826f), U64C(0x142929670a0e6e70),
  U64C(0x27b70a8546d22ffc), U64C(0x2e1b21385c26c926), U64C(0x4d2c6dfc5ac42aed), U64C(0x53380d139d95b3df),
  U64C(0x650a73548baf63de), U64C(0x766a0abb3c77b2a8), U64C(0x81c2c92e47edaee6), U64C(0x92722c851482353b),
  U64C(0xa2bfe8a14cf10364), U64C(0xa81a664bbc423001), U64C(0xc24b8b70d0f89791), U64C(0xc76c51a30654be30),
  U64C(0xd192e819d6ef5218), U64C(0xd69906245565a910), U64C(0xf40e35855771202a), U64C(0x106aa07032bbd1b8),
  U64C(0x19a4c116b8d2d0c8), U64C(0x1e376c085141ab53), U64C(0x2748774cdf8eeb99), U64C(0x34b0bcb5e19b48a8),
  U64C(0x391c0cb3c5c95a63), U64C(0x4ed8aa4ae3418acb), U64C(0x5b9cca4f7763e373), U64C(0x682e6ff3d6b2b8a3),
  U64C(0x748f82ee5defb2fc), U64C(0x78a5636f43172f60), U64C(0x84c87814a1f0ab72), U64C(0x8cc702081a6439ec),
  U64C(0x90befffa23631e28), U64C(0xa4506cebde82bde9), U64C(0xbef9a3f7b2c67915), U64C(0xc67178f2e372532b),
  U64C(0xca273eceea26619c), U64C(0xd186b8c721c0c207), U64C(0xeada7dd6cde0eb1e), U64C(0xf57d4f7fee6ed178),
  U64C(0x06f067aa72176fba), U64C(0x0a637dc5a2c898a6), U64C(0x113f9804bef90dae), U64C(0x1b710b35131c471b),
  U64C(0x28db77f523047d84), U64C(0x32caab7b40c72493), U64C(0x3c9ebe0a15c9bebc), U64C(0x431d67c49c100d4c),
  U64C(0x4cc5d4becb3e42b6), U64C(0x597f299cfc657e2a), U64C(0x5fcb6fab3ad6faec), U64C(0x6c44198c4a475817)
};

#define K SHA512_K_ARRAY

Z7_NO_INLINE
void Z7_FASTCALL Sha512_UpdateBlocks(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  UInt64 W
#ifdef Z7_SHA512_BIG_W
      [80];
#else
      [16];
#endif
  unsigned j;
  UInt64 a,b,c,d,e,f,g,h;
#if !defined(Z7_SHA512_UNROLL) || (STEP_MAIN <= 4) || (STEP_PRE <= 4)
  UInt64 tmp;
#endif

  if (numBlocks == 0) return;
  
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  f = state[5];
  g = state[6];
  h = state[7];

  do
  {

  for (j = 0; j < 16; j += STEP_PRE)
  {
    #if STEP_PRE > 4

      #if STEP_PRE < 8
      R4_PRE(0);
      #else
      R8_PRE(0);
      #if STEP_PRE == 16
      R8_PRE(8);
      #endif
      #endif

    #else

      R1_PRE(0)
      #if STEP_PRE >= 2
      R1_PRE(1)
      #if STEP_PRE >= 4
      R1_PRE(2)
      R1_PRE(3)
      #endif
      #endif
    
    #endif
  }

  for (j = 16; j < 80; j += STEP_MAIN)
  {
    #if defined(Z7_SHA512_UNROLL) && STEP_MAIN >= 8

      #if STEP_MAIN < 8
      R4_MAIN(0)
      #else
      R8_MAIN(0)
      #if STEP_MAIN == 16
      R8_MAIN(8)
      #endif
      #endif

    #else
      
      R1_MAIN(0)
      #if STEP_MAIN >= 2
      R1_MAIN(1)
      #if STEP_MAIN >= 4
      R2_MAIN(2)
      #if STEP_MAIN >= 8
      R2_MAIN(4)
      R2_MAIN(6)
      #if STEP_MAIN >= 16
      R2_MAIN(8)
      R2_MAIN(10)
      R2_MAIN(12)
      R2_MAIN(14)
      #endif
      #endif
      #endif
      #endif
    #endif
  }

  a += state[0]; state[0] = a;
  b += state[1]; state[1] = b;
  c += state[2]; state[2] = c;
  d += state[3]; state[3] = d;
  e += state[4]; state[4] = e;
  f += state[5]; state[5] = f;
  g += state[6]; state[6] = g;
  h += state[7]; state[7] = h;

  data += SHA512_BLOCK_SIZE;
  }
  while (--numBlocks);
}


#define Sha512_UpdateBlock(p) SHA512_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Sha512_Update(CSha512 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->v.vars.count & (SHA512_BLOCK_SIZE - 1);
    const unsigned num = SHA512_BLOCK_SIZE - pos;
    p->v.vars.count += size;
    if (num > size)
    {
      memcpy(p->buffer + pos, data, size);
      return;
    }
    if (pos != 0)
    {
      size -= num;
      memcpy(p->buffer + pos, data, num);
      data += num;
      Sha512_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 7;
    // if (numBlocks)
    SHA512_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= SHA512_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 7);
    memcpy(p->buffer, data, size);
  }
}


void Sha512_Final(CSha512 *p, Byte *digest, unsigned digestSize)
{
  unsigned pos = (unsigned)p->v.vars.count & (SHA512_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (SHA512_BLOCK_SIZE - 8 * 2))
  {
    while (pos != SHA512_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, SHA512_BLOCK_SIZE - pos);
    Sha512_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (SHA512_BLOCK_SIZE - 8 * 2) - pos);
  {
    const UInt64 numBits = p->v.vars.count << 3;
    SetBe64(p->buffer + SHA512_BLOCK_SIZE - 8 * 2, 0) // = (p->v.vars.count >> (64 - 3)); (high 64-bits)
    SetBe64(p->buffer + SHA512_BLOCK_SIZE - 8 * 1, numBits)
  }
  Sha512_UpdateBlock(p);
#if 1 && defined(MY_CPU_BE)
  memcpy(digest, p->state, digestSize);
#else
  {
    const unsigned numWords = digestSize >> 3;
    unsigned i;
    for (i = 0; i < numWords; i++)
    {
      const UInt64 v = p->state[i];
      SetBe64(digest, v)
      digest += 8;
    }
    if (digestSize & 4) // digestSize == SHA512_224_DIGEST_SIZE
    {
      const UInt32 v = (UInt32)((p->state[numWords]) >> 32);
      SetBe32(digest, v)
    }
  }
#endif
  Sha512_InitState(p, digestSize);
}



// #define Z7_SHA512_PROBE_DEBUG // for debug

#if defined(Z7_SHA512_PROBE_DEBUG) || defined(Z7_COMPILER_SHA512_SUPPORTED)

#if defined(Z7_SHA512_PROBE_DEBUG) \
    || defined(_WIN32) && defined(MY_CPU_ARM64)
#ifndef Z7_SHA512_USE_PROBE
#define Z7_SHA512_USE_PROBE
#endif
#endif

#ifdef Z7_SHA512_USE_PROBE

#ifdef Z7_SHA512_PROBE_DEBUG
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

#if 0 || !defined(_MSC_VER) // 1 || : for debug LONGJMP mode
// MINGW doesn't support __try. So we use signal() / longjmp().
// Note: signal() / longjmp() probably is not thread-safe.
// So we must call Sha512Prepare() from main thread at program start.
#ifndef Z7_SHA512_USE_LONGJMP
#define Z7_SHA512_USE_LONGJMP
#endif
#endif

#ifdef Z7_SHA512_USE_LONGJMP
#include <signal.h>
#include <setjmp.h>
static jmp_buf g_Sha512_jmp_buf;
// static int g_Sha512_Unsupported;

#if defined(__GNUC__) && (__GNUC__ >= 8) \
    || defined(__clang__) && (__clang_major__ >= 3)
  __attribute__((noreturn))
#endif
static void Z7_CDECL Sha512_signal_Handler(int v)
{
  PRF(printf("======== Sha512_signal_Handler = %x\n", (unsigned)v);)
  // g_Sha512_Unsupported = 1;
  longjmp(g_Sha512_jmp_buf, 1);
}
#endif // Z7_SHA512_USE_LONGJMP


#if defined(_WIN32)
// amalgamation: header emitted in prologue
#endif

#if defined(MY_CPU_ARM64)
// #define Z7_SHA512_USE_SIMPLIFIED_PROBE // for debug
#endif

#ifdef Z7_SHA512_USE_SIMPLIFIED_PROBE
#include <arm_neon.h>
#if defined(__clang__)
  __attribute__((__target__("sha3")))
#elif !defined(_MSC_VER)
  __attribute__((__target__("arch=armv8.2-a+sha3")))
#endif
#endif
static BoolInt CPU_IsSupported_SHA512_Probe(void)
{
  PRF(printf("\n== CPU_IsSupported_SHA512_Probe\n");)
#if defined(_WIN32) && defined(MY_CPU_ARM64)
  // we have no SHA512 flag for IsProcessorFeaturePresent() still.
  if (!CPU_IsSupported_CRYPTO())
    return False;
  PRF(printf("==== Registry check\n");)
  {
    // we can't read ID_AA64ISAR0_EL1 register from application.
    // but ID_AA64ISAR0_EL1 register is mapped to "CP 4030" registry value.
    HKEY key = NULL;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        0, KEY_READ, &key);
    if (res != ERROR_SUCCESS)
      return False;
    {
      DWORD type = 0;
      DWORD count = sizeof(UInt64);
      UInt64 val = 0;
      res = RegQueryValueEx(key, TEXT("CP 4030"), NULL,
          &type, (LPBYTE)&val, &count);
      RegCloseKey(key);
      if (res != ERROR_SUCCESS
          || type != REG_QWORD
          || count != sizeof(UInt64)
          || ((unsigned)(val >> 12) & 0xf) != 2)
        return False;
      // we parse SHA2 field of ID_AA64ISAR0_EL1 register:
      //   0 : No SHA2 instructions implemented
      //   1 : SHA256 implemented
      //   2 : SHA256 and SHA512 implemented
    }
  }
#endif // defined(_WIN32) && defined(MY_CPU_ARM64)


#if 1  // 0 for debug to disable SHA512 PROBE code

/*
----- SHA512 PROBE -----

We suppose that "CP 4030" registry reading is enough.
But we use additional SHA512 PROBE code, because
we can catch exception here, and we don't catch exceptions,
if we call Sha512 functions from main code.

NOTE: arm64 PROBE code doesn't work, if we call it via Wine in linux-arm64.
The program just stops.
Also x64 version of PROBE code doesn't work, if we run it via Intel SDE emulator
without SHA512 support (-skl switch),
The program stops, and we have message from SDE:
  TID 0 SDE-ERROR: Executed instruction not valid for specified chip (SKYLAKE): vsha512msg1
But we still want to catch that exception instead of process stopping.
Does this PROBE code work in native Windows-arm64 (with/without sha512 hw instructions)?
Are there any ways to fix the problems with arm64-wine and x64-SDE cases?
*/

  PRF(printf("==== CPU_IsSupported_SHA512 PROBE\n");)
  {
    BoolInt isSupported = False;
#ifdef Z7_SHA512_USE_LONGJMP
    void (Z7_CDECL *signal_prev)(int);
    /*
    if (g_Sha512_Unsupported)
    {
      PRF(printf("==== g_Sha512_Unsupported\n");)
      return False;
    }
    */
    printf("====== signal(SIGILL)\n");
    signal_prev = signal(SIGILL, Sha512_signal_Handler);
    if (signal_prev == SIG_ERR)
    {
      PRF(printf("====== signal fail\n");)
      return False;
    }
    // PRF(printf("==== signal_prev = %p\n", (void *)signal_prev);)
    // docs: Before the specified function is executed,
    // the value of func is set to SIG_DFL.
    // So we can exit if (setjmp(g_Sha512_jmp_buf) != 0).
    PRF(printf("====== setjmp\n");)
    if (!setjmp(g_Sha512_jmp_buf))
#else //  Z7_SHA512_USE_LONGJMP

#ifdef _MSC_VER
#ifdef __clang_major__
  #pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#endif
    __try
#endif
#endif //  Z7_SHA512_USE_LONGJMP

    {
#if defined(Z7_COMPILER_SHA512_SUPPORTED)
#ifdef Z7_SHA512_USE_SIMPLIFIED_PROBE
      // simplified sha512 check for arm64:
      const uint64x2_t a = vdupq_n_u64(1);
      const uint64x2_t b = vsha512hq_u64(a, a, a);
      PRF(printf("======== vsha512hq_u64 probe\n");)
      if ((UInt32)vgetq_lane_u64(b, 0) == 0x11800002)
#else
      MY_ALIGN(16)
      UInt64 temp[SHA512_NUM_DIGEST_WORDS + SHA512_NUM_BLOCK_WORDS];
      memset(temp, 0x5a, sizeof(temp));
      PRF(printf("======== Sha512_UpdateBlocks_HW\n");)
      Sha512_UpdateBlocks_HW(temp,
          (const Byte *)(const void *)(temp + SHA512_NUM_DIGEST_WORDS), 1);
      // PRF(printf("======== t = %x\n", (UInt32)temp[0]);)
      if ((UInt32)temp[0] == 0xa33cfdf7)
#endif
      {
        PRF(printf("======== PROBE SHA512: SHA512 is supported\n");)
        isSupported = True;
      }
#else // Z7_COMPILER_SHA512_SUPPORTED
      // for debug : we generate bad instrction or raise exception.
      // __except() doesn't catch raise() calls.
#ifdef Z7_SHA512_USE_LONGJMP
      PRF(printf("====== raise(SIGILL)\n");)
      raise(SIGILL);
#else
#if defined(_MSC_VER) && defined(MY_CPU_X86)
      __asm  ud2
#endif
#endif // Z7_SHA512_USE_LONGJMP
#endif // Z7_COMPILER_SHA512_SUPPORTED
    }

#ifdef Z7_SHA512_USE_LONGJMP
    PRF(printf("====== restore signal SIGILL\n");)
    signal(SIGILL, signal_prev);
#elif _MSC_VER
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      PRF(printf("==== CPU_IsSupported_SHA512 __except(EXCEPTION_EXECUTE_HANDLER)\n");)
    }
#endif
    PRF(printf("== return (sha512 supported) = %d\n", isSupported);)
    return isSupported;
  }
#else
  // without SHA512 PROBE code
  return True;
#endif
}

#endif // Z7_SHA512_USE_PROBE
#endif // defined(Z7_SHA512_PROBE_DEBUG) || defined(Z7_COMPILER_SHA512_SUPPORTED)


void Sha512Prepare(void)
{
#ifdef Z7_COMPILER_SHA512_SUPPORTED
  SHA512_FUNC_UPDATE_BLOCKS f, f_hw;
  f = Sha512_UpdateBlocks;
  f_hw = NULL;
#ifdef Z7_SHA512_USE_PROBE
  if (CPU_IsSupported_SHA512_Probe())
#elif defined(MY_CPU_X86_OR_AMD64)
  if (CPU_IsSupported_SHA512() && CPU_IsSupported_AVX2())
#else
  if (CPU_IsSupported_SHA512())
#endif
  {
    // printf("\n========== HW SHA512 ======== \n");
    f = f_hw = Sha512_UpdateBlocks_HW;
  }
  g_SHA512_FUNC_UPDATE_BLOCKS    = f;
  g_SHA512_FUNC_UPDATE_BLOCKS_HW = f_hw;
#elif defined(Z7_SHA512_PROBE_DEBUG)
  CPU_IsSupported_SHA512_Probe(); // for debug
#endif
}


#undef K
#undef S0
#undef S1
#undef s0
#undef s1
#undef Ch
#undef Maj
#undef W_MAIN
#undef W_PRE
#undef w
#undef blk2_main
#undef blk2
#undef T1
#undef T4
#undef T8
#undef R1_PRE
#undef R1_MAIN
#undef R2_MAIN
#undef R4
#undef R4_PRE
#undef R4_MAIN
#undef R8
#undef R8_PRE
#undef R8_MAIN
#undef STEP_PRE
#undef STEP_MAIN
#undef Z7_SHA512_BIG_W
#undef Z7_SHA512_UNROLL
#undef Z7_COMPILER_SHA512_SUPPORTED

/* ================ unit: C/Sha512Opt.c ================ */
/* Sha512Opt.c -- SHA-512 optimized code for SHA-512 hardware instructions
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// #define Z7_USE_HW_SHA_STUB // for debug
#ifdef MY_CPU_X86_OR_AMD64
  #if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 2400) && (__INTEL_COMPILER <= 9900) // fix it
      #define USE_HW_SHA
  #elif defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 170001) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 170001) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 140000)
      #define USE_HW_SHA
      #if !defined(__INTEL_COMPILER)
      // icc defines __GNUC__, but icc doesn't support __attribute__(__target__)
      #if !defined(__SHA512__) || !defined(__AVX2__)
        #define ATTRIB_SHA512 __attribute__((__target__("sha512,avx2")))
      #endif
      #endif
  #elif defined(Z7_MSC_VER_ORIGINAL)
    #if (_MSC_VER >= 1940)
      #define USE_HW_SHA
    #else
      // #define Z7_USE_HW_SHA_STUB
    #endif
  #endif
// #endif // MY_CPU_X86_OR_AMD64
#ifndef USE_HW_SHA
  // #define Z7_USE_HW_SHA_STUB // for debug
#endif

#ifdef USE_HW_SHA

// #pragma message("Sha512 HW")

#include <immintrin.h>

#if defined (__clang__) && defined(_MSC_VER)
  #if !defined(__AVX__)
    #include <avxintrin.h>
  #endif
  #if !defined(__AVX2__)
    #include <avx2intrin.h>
  #endif
  #if !defined(__SHA512__)
    #include <sha512intrin.h>
  #endif
#else

#endif

/*
SHA512 uses:
AVX:
  _mm256_loadu_si256  (vmovdqu)
  _mm256_storeu_si256
  _mm256_set_epi32    (unused)
AVX2:
  _mm256_add_epi64     : vpaddq
  _mm256_shuffle_epi8  : vpshufb
  _mm256_shuffle_epi32 : pshufd
  _mm256_blend_epi32   : vpblendd
  _mm256_permute4x64_epi64 : vpermq     : 3c
  _mm256_permute2x128_si256: vperm2i128 : 3c
  _mm256_extracti128_si256 : vextracti128  : 3c
SHA512:
  _mm256_sha512*
*/

// K array must be aligned for 32-bytes at least.
// The compiler can look align attribute and selects
//  vmovdqu - for code without align attribute
//  vmovdqa - for code with    align attribute
extern
MY_ALIGN(64)
const UInt64 SHA512_K_ARRAY[80];
#define K SHA512_K_ARRAY


#define ADD_EPI64(dest, src)      dest = _mm256_add_epi64(dest, src);
#define SHA512_MSG1(dest, src)    dest = _mm256_sha512msg1_epi64(dest, _mm256_extracti128_si256(src, 0));
#define SHA512_MSG2(dest, src)    dest = _mm256_sha512msg2_epi64(dest, src);

#define LOAD_SHUFFLE(m, k) \
    m = _mm256_loadu_si256((const __m256i *)(const void *)(data + (k) * 32)); \
    m = _mm256_shuffle_epi8(m, mask); \

#define NNN(m0, m1, m2, m3)

#define SM1(m1, m2, m3, m0) \
    SHA512_MSG1(m0, m1); \
            
#define SM2(m2, m3, m0, m1) \
    ADD_EPI64(m0, _mm256_permute4x64_epi64(_mm256_blend_epi32(m2, m3, 3), 0x39)); \
    SHA512_MSG2(m0, m3); \

#define RND2(t0, t1, lane) \
    t0 = _mm256_sha512rnds2_epi64(t0, t1, _mm256_extracti128_si256(msg, lane));



#define R4(k, m0, m1, m2, m3, OP0, OP1) \
    msg = _mm256_add_epi64(m0, *(const __m256i *) (const void *) &K[(k) * 4]); \
    RND2(state0, state1, 0);  OP0(m0, m1, m2, m3) \
    RND2(state1, state0, 1);  OP1(m0, m1, m2, m3) \




#define R16(k, OP0, OP1, OP2, OP3, OP4, OP5, OP6, OP7) \
    R4 ( (k)*4+0, m0,m1,m2,m3, OP0, OP1 ) \
    R4 ( (k)*4+1, m1,m2,m3,m0, OP2, OP3 ) \
    R4 ( (k)*4+2, m2,m3,m0,m1, OP4, OP5 ) \
    R4 ( (k)*4+3, m3,m0,m1,m2, OP6, OP7 ) \

#define PREPARE_STATE \
    state0 = _mm256_shuffle_epi32(state0, 0x4e);              /* cdab */ \
    state1 = _mm256_shuffle_epi32(state1, 0x4e);              /* ghef */ \
    tmp = state0; \
    state0 = _mm256_permute2x128_si256(state0, state1, 0x13); /* cdgh */ \
    state1 = _mm256_permute2x128_si256(tmp,    state1, 2);    /* abef */ \


void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA512
ATTRIB_SHA512
#endif
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  const __m256i mask = _mm256_set_epi32(
      0x08090a0b,0x0c0d0e0f, 0x00010203,0x04050607,
      0x08090a0b,0x0c0d0e0f, 0x00010203,0x04050607);
  __m256i tmp, state0, state1;

  if (numBlocks == 0)
    return;

  state0 = _mm256_loadu_si256((const __m256i *) (const void *) &state[0]);
  state1 = _mm256_loadu_si256((const __m256i *) (const void *) &state[4]);
  
  PREPARE_STATE

  do
  {
    __m256i state0_save, state1_save;
    __m256i m0, m1, m2, m3;
    __m256i msg;
    // #define msg tmp

    state0_save = state0;
    state1_save = state1;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)



    R16 ( 0, NNN, NNN, SM1, NNN, SM1, SM2, SM1, SM2 )
    R16 ( 1, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 2, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 3, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 4, SM1, SM2, NNN, SM2, NNN, NNN, NNN, NNN )
    ADD_EPI64(state0, state0_save)
    ADD_EPI64(state1, state1_save)
    
    data += 128;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm256_storeu_si256((__m256i *) (void *) &state[0], state0);
  _mm256_storeu_si256((__m256i *) (void *) &state[4], state1);
}

#endif // USE_HW_SHA

// gcc 8.5 also supports sha512, but we need also support in assembler that is called by gcc
#elif defined(MY_CPU_ARM64) && defined(MY_CPU_LE)
  
  #if defined(__ARM_FEATURE_SHA512)
    #define USE_HW_SHA
  #else
    #if (defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 130000) \
           || defined(__GNUC__) && (__GNUC__ >= 9) \
          ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1940) // fix it
      #define USE_HW_SHA
    #endif
  #endif

#ifdef USE_HW_SHA

// #pragma message("=== Sha512 HW === ")


#if defined(__clang__) || defined(__GNUC__)
#if !defined(__ARM_FEATURE_SHA512)
// #pragma message("=== we define SHA3 ATTRIB_SHA512 === ")
#if defined(__clang__)
    #define ATTRIB_SHA512 __attribute__((__target__("sha3"))) // "armv8.2-a,sha3"
#else
    #define ATTRIB_SHA512 __attribute__((__target__("arch=armv8.2-a+sha3")))
#endif
#endif
#endif


#if defined(Z7_MSC_VER_ORIGINAL)
#include <arm64_neon.h>
#else

#if defined(__clang__) && __clang_major__ < 16
#if !defined(__ARM_FEATURE_SHA512)
// #pragma message("=== we set __ARM_FEATURE_SHA512 1 === ")
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #define Z7_ARM_FEATURE_SHA512_WAS_SET 1
    #define __ARM_FEATURE_SHA512 1
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif // clang

#include <arm_neon.h>

#if defined(Z7_ARM_FEATURE_SHA512_WAS_SET) && \
    defined(__ARM_FEATURE_SHA512)
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #undef __ARM_FEATURE_SHA512
    #undef Z7_ARM_FEATURE_SHA512_WAS_SET
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
// #pragma message("=== we undefine __ARM_FEATURE_CRYPTO === ")
#endif

#endif // Z7_MSC_VER_ORIGINAL

typedef uint64x2_t v128_64;
// typedef __n128 v128_64; // MSVC

#ifdef MY_CPU_BE
  #define MY_rev64_for_LE(x) x
#else
  #define MY_rev64_for_LE(x) vrev64q_u8(x)
#endif

#define LOAD_128_64(_p)       vld1q_u64(_p)
#define LOAD_128_8(_p)        vld1q_u8 (_p)
#define STORE_128_64(_p, _v)  vst1q_u64(_p, _v)

#define LOAD_SHUFFLE(m, k) \
    m = vreinterpretq_u64_u8( \
        MY_rev64_for_LE( \
        LOAD_128_8(data + (k) * 16))); \

// K array must be aligned for 16-bytes at least.
extern
MY_ALIGN(64)
const UInt64 SHA512_K_ARRAY[80];
#define K SHA512_K_ARRAY

#define NN(m0, m1, m4, m5, m7)
#define SM(m0, m1, m4, m5, m7) \
    m0 = vsha512su1q_u64(vsha512su0q_u64(m0, m1), m7, vextq_u64(m4, m5, 1));

#define R2(k, m0,m1,m2,m3,m4,m5,m6,m7, a0,a1,a2,a3, OP) \
    OP(m0, m1, m4, m5, m7) \
    t = vaddq_u64(m0, vld1q_u64(k)); \
    t = vaddq_u64(vextq_u64(t, t, 1), a3); \
    t = vsha512hq_u64(t, vextq_u64(a2, a3, 1), vextq_u64(a1, a2, 1)); \
    a3 = vsha512h2q_u64(t, a1, a0); \
    a1 = vaddq_u64(a1, t); \

#define R8(k,     m0,m1,m2,m3,m4,m5,m6,m7, OP) \
    R2 ( (k)+0*2, m0,m1,m2,m3,m4,m5,m6,m7, a0,a1,a2,a3, OP ) \
    R2 ( (k)+1*2, m1,m2,m3,m4,m5,m6,m7,m0, a3,a0,a1,a2, OP ) \
    R2 ( (k)+2*2, m2,m3,m4,m5,m6,m7,m0,m1, a2,a3,a0,a1, OP ) \
    R2 ( (k)+3*2, m3,m4,m5,m6,m7,m0,m1,m2, a1,a2,a3,a0, OP ) \

#define R16(k, OP) \
    R8 ( (k)+0*2, m0,m1,m2,m3,m4,m5,m6,m7, OP ) \
    R8 ( (k)+4*2, m4,m5,m6,m7,m0,m1,m2,m3, OP ) \


void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA512
ATTRIB_SHA512
#endif
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  v128_64 a0, a1, a2, a3;

  if (numBlocks == 0)
    return;
  a0 = LOAD_128_64(&state[0]);
  a1 = LOAD_128_64(&state[2]);
  a2 = LOAD_128_64(&state[4]);
  a3 = LOAD_128_64(&state[6]);
  do
  {
    v128_64 a0_save, a1_save, a2_save, a3_save;
    v128_64 m0, m1, m2, m3, m4, m5, m6, m7;
    v128_64 t;
    unsigned i;
    const UInt64 *k_ptr;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)
    LOAD_SHUFFLE (m4, 4)
    LOAD_SHUFFLE (m5, 5)
    LOAD_SHUFFLE (m6, 6)
    LOAD_SHUFFLE (m7, 7)

    a0_save = a0;
    a1_save = a1;
    a2_save = a2;
    a3_save = a3;
    
    R16 ( K, NN )
    k_ptr = K + 16;
    for (i = 0; i < 4; i++)
    {
      R16 ( k_ptr, SM )
      k_ptr += 16;
    }
    
    a0 = vaddq_u64(a0, a0_save);
    a1 = vaddq_u64(a1, a1_save);
    a2 = vaddq_u64(a2, a2_save);
    a3 = vaddq_u64(a3, a3_save);

    data += 128;
  }
  while (--numBlocks);

  STORE_128_64(&state[0], a0);
  STORE_128_64(&state[2], a1);
  STORE_128_64(&state[4], a2);
  STORE_128_64(&state[6], a3);
}

#endif // USE_HW_SHA

#endif // MY_CPU_ARM_OR_ARM64


#if !defined(USE_HW_SHA) && defined(Z7_USE_HW_SHA_STUB)
// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>
// We can compile this file with another C compiler,
// or we can compile asm version.
// So we can generate real code instead of this stub function.
// #include "Sha512.h"
// #if defined(_MSC_VER)
#pragma message("Sha512 HW-SW stub was used")
// #endif
void Z7_FASTCALL Sha512_UpdateBlocks   (UInt64 state[8], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  Sha512_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}
#endif


#undef K
#undef RND2
#undef MY_rev64_for_LE
#undef NN
#undef NNN
#undef LOAD_128
#undef STORE_128
#undef LOAD_SHUFFLE
#undef SM1
#undef SM2
#undef SM
#undef R2
#undef R4
#undef R16
#undef PREPARE_STATE
#undef USE_HW_SHA
#undef ATTRIB_SHA512
#undef USE_VER_MIN
#undef Z7_USE_HW_SHA_STUB

/* ================ unit: C/Sort.c ================ */
/* Sort.c -- Sort functions
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#if (  (defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))) \
    || (defined(__clang__) && Z7_has_builtin(__builtin_prefetch)) \
    )
// the code with prefetch is slow for small arrays on x86.
// So we disable prefetch for x86.
#ifndef MY_CPU_X86
  // #pragma message("Z7_PREFETCH : __builtin_prefetch")
  #define Z7_PREFETCH(a)  __builtin_prefetch((a))
#endif

#elif defined(_WIN32) // || defined(_MSC_VER) && (_MSC_VER >= 1200)

// amalgamation: header emitted in prologue

// NOTE: CLANG/GCC/MSVC can define different values for _MM_HINT_T0 / PF_TEMPORAL_LEVEL_1.
// For example, clang-cl can generate "prefetcht2" instruction for
// PreFetchCacheLine(PF_TEMPORAL_LEVEL_1) call.
// But we want to generate "prefetcht0" instruction.
// So for CLANG/GCC we must use __builtin_prefetch() in code branch above
// instead of PreFetchCacheLine() / _mm_prefetch().

// New msvc-x86 compiler generates "prefetcht0" instruction for PreFetchCacheLine() call.
// But old x86 cpus don't support "prefetcht0".
// So we will use PreFetchCacheLine(), only if we are sure that
// generated instruction is supported by all cpus of that isa.
#if defined(MY_CPU_AMD64) \
    || defined(MY_CPU_ARM64) \
    || defined(MY_CPU_IA64)
// we need to use additional braces for (a) in PreFetchCacheLine call, because
// PreFetchCacheLine macro doesn't use braces:
//   #define PreFetchCacheLine(l, a)  _mm_prefetch((CHAR CONST *) a, l)
  // #pragma message("Z7_PREFETCH : PreFetchCacheLine")
  #define Z7_PREFETCH(a)  PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, (a))
#endif

#endif // _WIN32


#define PREFETCH_NO(p,k,s,size)

#ifndef Z7_PREFETCH
  #define SORT_PREFETCH(p,k,s,size)
#else

// #define PREFETCH_LEVEL 2  // use it if cache line is 32-bytes
#define PREFETCH_LEVEL 3  // it is fast for most cases (64-bytes cache line prefetch)
// #define PREFETCH_LEVEL 4  // it can be faster for big array (128-bytes prefetch)

#if PREFETCH_LEVEL == 0

  #define SORT_PREFETCH(p,k,s,size)

#else // PREFETCH_LEVEL != 0

/*
if  defined(USE_PREFETCH_FOR_ALIGNED_ARRAY)
    we prefetch one value per cache line.
    Use it if array is aligned for cache line size (64 bytes)
    or if array is small (less than L1 cache size).

if !defined(USE_PREFETCH_FOR_ALIGNED_ARRAY)
    we perfetch all cache lines that can be required.
    it can be faster for big unaligned arrays.
*/
  #define USE_PREFETCH_FOR_ALIGNED_ARRAY

// s == k * 2
#if 0 && PREFETCH_LEVEL <= 3 && defined(MY_CPU_X86_OR_AMD64)
  // x86 supports (lea r1*8+offset)
  #define PREFETCH_OFFSET(k,s)  ((s) << PREFETCH_LEVEL)
#else
  #define PREFETCH_OFFSET(k,s)  ((k) << (PREFETCH_LEVEL + 1))
#endif

#if 1 && PREFETCH_LEVEL <= 3 && defined(USE_PREFETCH_FOR_ALIGNED_ARRAY)
  #define PREFETCH_ADD_OFFSET   0
#else
  // last offset that can be reqiured in PREFETCH_LEVEL step:
  #define PREFETCH_RANGE        ((2 << PREFETCH_LEVEL) - 1)
  #define PREFETCH_ADD_OFFSET   PREFETCH_RANGE / 2
#endif

#if PREFETCH_LEVEL <= 3

#ifdef USE_PREFETCH_FOR_ALIGNED_ARRAY
  #define SORT_PREFETCH(p,k,s,size) \
  { const size_t s2 = PREFETCH_OFFSET(k,s) + PREFETCH_ADD_OFFSET; \
    if (s2 <= size) { \
      Z7_PREFETCH((p + s2)); \
  }}
#else /* for unaligned array */
  #define SORT_PREFETCH(p,k,s,size) \
  { const size_t s2 = PREFETCH_OFFSET(k,s) + PREFETCH_RANGE; \
    if (s2 <= size) { \
      Z7_PREFETCH((p + s2 - PREFETCH_RANGE)); \
      Z7_PREFETCH((p + s2)); \
  }}
#endif

#else // PREFETCH_LEVEL > 3

#ifdef USE_PREFETCH_FOR_ALIGNED_ARRAY
  #define SORT_PREFETCH(p,k,s,size) \
  { const size_t s2 = PREFETCH_OFFSET(k,s) + PREFETCH_RANGE - 16 / 2; \
    if (s2 <= size) { \
      Z7_PREFETCH((p + s2 - 16)); \
      Z7_PREFETCH((p + s2)); \
  }}
#else /* for unaligned array */
  #define SORT_PREFETCH(p,k,s,size) \
  { const size_t s2 = PREFETCH_OFFSET(k,s) + PREFETCH_RANGE; \
    if (s2 <= size) { \
      Z7_PREFETCH((p + s2 - PREFETCH_RANGE)); \
      Z7_PREFETCH((p + s2 - PREFETCH_RANGE / 2)); \
      Z7_PREFETCH((p + s2)); \
  }}
#endif

#endif // PREFETCH_LEVEL > 3
#endif // PREFETCH_LEVEL != 0
#endif // Z7_PREFETCH


#if defined(MY_CPU_ARM64) \
    /* || defined(MY_CPU_AMD64) */ \
    /* || defined(MY_CPU_ARM) && !defined(_MSC_VER) */
  // we want to use cmov, if cmov is very fast:
  // - this cmov version is slower for clang-x64.
  // - this cmov version is faster for gcc-arm64 for some fast arm64 cpus.
  #define Z7_FAST_CMOV_SUPPORTED
#endif
 
#ifdef Z7_FAST_CMOV_SUPPORTED
  // we want to use cmov here, if cmov is fast: new arm64 cpus.
  // we want the compiler to use conditional move for this branch
  #define GET_MAX_VAL(n0, n1, max_val_slow)  if (n0 < n1) n0 = n1;
#else
  // use this branch, if cpu doesn't support fast conditional move.
  // it uses slow array access reading:
  #define GET_MAX_VAL(n0, n1, max_val_slow)  n0 = max_val_slow;
#endif

#define HeapSortDown(p, k, size, temp, macro_prefetch) \
{ \
  for (;;) { \
    UInt32 n0, n1; \
    size_t s = k * 2; \
    if (s >= size) { \
      if (s == size) { \
        n0 = p[s]; \
        p[k] = n0; \
        if (temp < n0) k = s; \
      } \
      break; \
    } \
    n0 = p[k * 2]; \
    n1 = p[k * 2 + 1]; \
    s += n0 < n1; \
    GET_MAX_VAL(n0, n1, p[s]) \
    if (temp >= n0) break; \
    macro_prefetch(p, k, s, size) \
    p[k] = n0; \
    k = s; \
  } \
  p[k] = temp; \
}


/*
stage-1 : O(n) :
  we generate intermediate partially sorted binary tree:
  p[0]  : it's additional item for better alignment of tree structure in memory.
  p[1]
  p[2]       p[3]
  p[4] p[5]  p[6] p[7]
  ...
  p[x] >= p[x * 2]
  p[x] >= p[x * 2 + 1]
  
stage-2 : O(n)*log2(N):
  we move largest item p[0] from head of tree to the end of array
  and insert last item to sorted binary tree.
*/

// (p) must be aligned for cache line size (64-bytes) for best performance

void Z7_FASTCALL HeapSort(UInt32 *p, size_t size)
{
  if (size < 2)
    return;
  if (size == 2)
  {
    const UInt32 a0 = p[0];
    const UInt32 a1 = p[1];
    const unsigned k = a1 < a0;
    p[k] = a0;
    p[k ^ 1] = a1;
    return;
  }
  {
    // stage-1 : O(n)
    // we transform array to partially sorted binary tree.
    size_t i = --size / 2;
    // (size) now is the index of the last item in tree,
    // if (i)
    {
      do
      {
        const UInt32 temp = p[i];
        size_t k = i;
        HeapSortDown(p, k, size, temp, PREFETCH_NO)
      }
      while (--i);
    }
    {
      const UInt32 temp = p[0];
      const UInt32 a1 = p[1];
      if (temp < a1)
      {
        size_t k = 1;
        p[0] = a1;
        HeapSortDown(p, k, size, temp, PREFETCH_NO)
      }
    }
  }

  if (size < 3)
  {
    // size == 2
    const UInt32 a0 = p[0];
    p[0] = p[2];
    p[2] = a0;
    return;
  }
  if (size != 3)
  {
    // stage-2 : O(size) * log2(size):
    // we move largest item p[0] from head to the end of array,
    // and insert last item to sorted binary tree.
    do
    {
      const UInt32 temp = p[size];
      size_t k = p[2] < p[3] ? 3 : 2;
      p[size--] = p[0];
      p[0] = p[1];
      p[1] = p[k];
      HeapSortDown(p, k, size, temp, SORT_PREFETCH) // PREFETCH_NO
    }
    while (size != 3);
  }
  {
    const UInt32 a2 = p[2];
    const UInt32 a3 = p[3];
    const size_t k = a2 < a3;
    p[2] = p[1];
    p[3] = p[0];
    p[k] = a3;
    p[k ^ 1] = a2;
  }
}

/* ================ unit: C/SwapBytes.c ================ */
/* SwapBytes.c -- Byte Swap conversion filter
2024-03-01 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

typedef UInt16 CSwapUInt16;
typedef UInt32 CSwapUInt32;

// #define k_SwapBytes_Mode_BASE   0

#ifdef MY_CPU_X86_OR_AMD64

#define k_SwapBytes_Mode_SSE2   1
#define k_SwapBytes_Mode_SSSE3  2
#define k_SwapBytes_Mode_AVX2   3

  // #if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1900)
  #if defined(__clang__) && (__clang_major__ >= 4) \
      || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40701)
      #define k_SwapBytes_Mode_MAX  k_SwapBytes_Mode_AVX2
      #define SWAP_ATTRIB_SSE2  __attribute__((__target__("sse2")))
      #define SWAP_ATTRIB_SSSE3 __attribute__((__target__("ssse3")))
      #define SWAP_ATTRIB_AVX2  __attribute__((__target__("avx2")))
  #elif defined(_MSC_VER)
    #if (_MSC_VER == 1900)
      #pragma warning(disable : 4752) // found Intel(R) Advanced Vector Extensions; consider using /arch:AVX
    #endif
    #if (_MSC_VER >= 1900)
      #define k_SwapBytes_Mode_MAX  k_SwapBytes_Mode_AVX2
    #elif (_MSC_VER >= 1500)  // (VS2008)
      #define k_SwapBytes_Mode_MAX  k_SwapBytes_Mode_SSSE3
    #elif (_MSC_VER >= 1310)  // (VS2003)
      #define k_SwapBytes_Mode_MAX  k_SwapBytes_Mode_SSE2
    #endif
  #endif // _MSC_VER

/*
// for debug
#ifdef k_SwapBytes_Mode_MAX
#undef k_SwapBytes_Mode_MAX
#endif
*/

#ifndef k_SwapBytes_Mode_MAX
#define k_SwapBytes_Mode_MAX 0
#endif

#if (k_SwapBytes_Mode_MAX != 0) && defined(MY_CPU_AMD64)
  #define k_SwapBytes_Mode_MIN  k_SwapBytes_Mode_SSE2
#else
  #define k_SwapBytes_Mode_MIN  0
#endif

#if (k_SwapBytes_Mode_MAX >= k_SwapBytes_Mode_AVX2)
  #define USE_SWAP_AVX2
#endif
#if (k_SwapBytes_Mode_MAX >= k_SwapBytes_Mode_SSSE3)
  #define USE_SWAP_SSSE3
#endif
#if (k_SwapBytes_Mode_MAX >= k_SwapBytes_Mode_SSE2)
  #define USE_SWAP_128
#endif

#if k_SwapBytes_Mode_MAX <= k_SwapBytes_Mode_MIN || !defined(USE_SWAP_128)
#define FORCE_SWAP_MODE
#endif


#ifdef USE_SWAP_128
/*
 <mmintrin.h> MMX
<xmmintrin.h> SSE
<emmintrin.h> SSE2
<pmmintrin.h> SSE3
<tmmintrin.h> SSSE3
<smmintrin.h> SSE4.1
<nmmintrin.h> SSE4.2
<ammintrin.h> SSE4A
<wmmintrin.h> AES
<immintrin.h> AVX, AVX2, FMA
*/

#include <emmintrin.h> // sse2
// typedef __m128i v128;

#define SWAP2_128(i) { \
  const __m128i v = *(const __m128i *)(const void *)(items + (i) * 8); \
                    *(      __m128i *)(      void *)(items + (i) * 8) = \
    _mm_or_si128( \
      _mm_slli_epi16(v, 8), \
      _mm_srli_epi16(v, 8)); }
// _mm_or_si128() has more ports to execute than _mm_add_epi16().

static
#ifdef SWAP_ATTRIB_SSE2
SWAP_ATTRIB_SSE2
#endif
void
Z7_FASTCALL
SwapBytes2_128(CSwapUInt16 *items, const CSwapUInt16 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP2_128(0)  SWAP2_128(1)  items += 2 * 8;
    SWAP2_128(0)  SWAP2_128(1)  items += 2 * 8;
  }
  while (items != lim);
}

/*
// sse2
#define SWAP4_128_pack(i) { \
  __m128i v = *(const __m128i *)(const void *)(items + (i) * 4); \
  __m128i v0 = _mm_unpacklo_epi8(v, mask); \
  __m128i v1 = _mm_unpackhi_epi8(v, mask); \
  v0 = _mm_shufflelo_epi16(v0, 0x1b); \
  v1 = _mm_shufflelo_epi16(v1, 0x1b); \
  v0 = _mm_shufflehi_epi16(v0, 0x1b); \
  v1 = _mm_shufflehi_epi16(v1, 0x1b); \
  *(__m128i *)(void *)(items + (i) * 4) = _mm_packus_epi16(v0, v1); }

static
#ifdef SWAP_ATTRIB_SSE2
SWAP_ATTRIB_SSE2
#endif
void
Z7_FASTCALL
SwapBytes4_128_pack(CSwapUInt32 *items, const CSwapUInt32 *lim)
{
  const __m128i mask = _mm_setzero_si128();
  // const __m128i mask = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 0);
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP4_128_pack(0); items += 1 * 4;
    // SWAP4_128_pack(0); SWAP4_128_pack(1); items += 2 * 4;
  }
  while (items != lim);
}

// sse2
#define SWAP4_128_shift(i) { \
  __m128i v = *(const __m128i *)(const void *)(items + (i) * 4); \
  __m128i v2; \
  v2 = _mm_or_si128( \
        _mm_slli_si128(_mm_and_si128(v, mask), 1), \
        _mm_and_si128(_mm_srli_si128(v, 1), mask)); \
  v = _mm_or_si128( \
        _mm_slli_epi32(v, 24), \
        _mm_srli_epi32(v, 24)); \
  *(__m128i *)(void *)(items + (i) * 4) = _mm_or_si128(v2, v); }

static
#ifdef SWAP_ATTRIB_SSE2
SWAP_ATTRIB_SSE2
#endif
void
Z7_FASTCALL
SwapBytes4_128_shift(CSwapUInt32 *items, const CSwapUInt32 *lim)
{
  #define M1 0xff00
  const __m128i mask = _mm_set_epi32(M1, M1, M1, M1);
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    // SWAP4_128_shift(0)  SWAP4_128_shift(1)  items += 2 * 4;
    // SWAP4_128_shift(0)  SWAP4_128_shift(1)  items += 2 * 4;
    SWAP4_128_shift(0); items += 1 * 4;
  }
  while (items != lim);
}
*/


#if defined(USE_SWAP_SSSE3) || defined(USE_SWAP_AVX2)

#define SWAP_SHUF_REV_SEQ_2_VALS(v)                (v)+1, (v)
#define SWAP_SHUF_REV_SEQ_4_VALS(v)  (v)+3, (v)+2, (v)+1, (v)

#define SWAP2_SHUF_MASK_16_BYTES \
    SWAP_SHUF_REV_SEQ_2_VALS (0 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (1 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (2 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (3 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (4 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (5 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (6 * 2), \
    SWAP_SHUF_REV_SEQ_2_VALS (7 * 2)

#define SWAP4_SHUF_MASK_16_BYTES \
    SWAP_SHUF_REV_SEQ_4_VALS (0 * 4), \
    SWAP_SHUF_REV_SEQ_4_VALS (1 * 4), \
    SWAP_SHUF_REV_SEQ_4_VALS (2 * 4), \
    SWAP_SHUF_REV_SEQ_4_VALS (3 * 4)

#if defined(USE_SWAP_AVX2)
/* if we use 256_BIT_INIT_MASK, each static array mask will be larger for 16 bytes */
// #define SWAP_USE_256_BIT_INIT_MASK
#endif

#if defined(SWAP_USE_256_BIT_INIT_MASK) && defined(USE_SWAP_AVX2)
#define SWAP_MASK_INIT_SIZE 32
#else
#define SWAP_MASK_INIT_SIZE 16
#endif

MY_ALIGN(SWAP_MASK_INIT_SIZE)
static const Byte k_ShufMask_Swap2[] =
{
    SWAP2_SHUF_MASK_16_BYTES
  #if SWAP_MASK_INIT_SIZE > 16
  , SWAP2_SHUF_MASK_16_BYTES
  #endif
};

MY_ALIGN(SWAP_MASK_INIT_SIZE)
static const Byte k_ShufMask_Swap4[] =
{
    SWAP4_SHUF_MASK_16_BYTES
  #if SWAP_MASK_INIT_SIZE > 16
  , SWAP4_SHUF_MASK_16_BYTES
  #endif
};


#ifdef USE_SWAP_SSSE3

#include <tmmintrin.h> // ssse3

#define SHUF_128(i)   *(items + (i)) = \
     _mm_shuffle_epi8(*(items + (i)), mask); // SSSE3

// Z7_NO_INLINE
static
#ifdef SWAP_ATTRIB_SSSE3
SWAP_ATTRIB_SSSE3
#endif
Z7_ATTRIB_NO_VECTORIZE
void
Z7_FASTCALL
ShufBytes_128(void *items8, const void *lim8, const void *mask128_ptr)
{
  __m128i *items = (__m128i *)items8;
  const __m128i *lim = (const __m128i *)lim8;
  // const __m128i mask = _mm_set_epi8(SHUF_SWAP2_MASK_16_VALS);
  // const __m128i mask = _mm_set_epi8(SHUF_SWAP4_MASK_16_VALS);
  // const __m128i mask = _mm_load_si128((const __m128i *)(const void *)&(k_ShufMask_Swap4[0]));
  // const __m128i mask = _mm_load_si128((const __m128i *)(const void *)&(k_ShufMask_Swap4[0]));
  // const __m128i mask = *(const __m128i *)(const void *)&(k_ShufMask_Swap4[0]);
  const __m128i mask = *(const __m128i *)mask128_ptr;
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SHUF_128(0)  SHUF_128(1)  items += 2;
    SHUF_128(0)  SHUF_128(1)  items += 2;
  }
  while (items != lim);
}

#endif // USE_SWAP_SSSE3



#ifdef USE_SWAP_AVX2

#include <immintrin.h> // avx, avx2
#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif

#define SHUF_256(i)   *(items + (i)) = \
  _mm256_shuffle_epi8(*(items + (i)), mask); // AVX2

// Z7_NO_INLINE
static
#ifdef SWAP_ATTRIB_AVX2
SWAP_ATTRIB_AVX2
#endif
Z7_ATTRIB_NO_VECTORIZE
void
Z7_FASTCALL
ShufBytes_256(void *items8, const void *lim8, const void *mask128_ptr)
{
  __m256i *items = (__m256i *)items8;
  const __m256i *lim = (const __m256i *)lim8;
  /*
  UNUSED_VAR(mask128_ptr)
  __m256i mask =
  for Swap4: _mm256_setr_epi8(SWAP4_SHUF_MASK_16_BYTES, SWAP4_SHUF_MASK_16_BYTES);
  for Swap2: _mm256_setr_epi8(SWAP2_SHUF_MASK_16_BYTES, SWAP2_SHUF_MASK_16_BYTES);
  */
  const __m256i mask =
 #if SWAP_MASK_INIT_SIZE > 16
      *(const __m256i *)(const void *)mask128_ptr;
 #else
  /* msvc: broadcastsi128() version reserves the stack for no reason
     msvc 19.29-: _mm256_insertf128_si256() / _mm256_set_m128i)) versions use non-avx movdqu   xmm0,XMMWORD PTR [r8]
     msvc 19.30+ (VS2022): replaces _mm256_set_m128i(m,m) to vbroadcastf128(m) as we want
  */
  // _mm256_broadcastsi128_si256(*mask128_ptr);
#if defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION < 80000)
  #define MY_mm256_set_m128i(hi, lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 1)
#else
  #define MY_mm256_set_m128i  _mm256_set_m128i
#endif
      MY_mm256_set_m128i(
        *(const __m128i *)mask128_ptr,
        *(const __m128i *)mask128_ptr);
 #endif
  
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SHUF_256(0)  SHUF_256(1)  items += 2;
    SHUF_256(0)  SHUF_256(1)  items += 2;
  }
  while (items != lim);
}

#endif // USE_SWAP_AVX2
#endif // USE_SWAP_SSSE3 || USE_SWAP_AVX2
#endif // USE_SWAP_128



// compile message "NEON intrinsics not available with the soft-float ABI"
#elif defined(MY_CPU_ARM_OR_ARM64) \
    && defined(MY_CPU_LE) \
    && !defined(Z7_DISABLE_ARM_NEON)

  #if defined(__clang__) && (__clang_major__ >= 8) \
    || defined(__GNUC__) && (__GNUC__ >= 6)
    #if defined(__ARM_FP)
    #if (defined(__ARM_ARCH) && (__ARM_ARCH >= 4)) \
        || defined(MY_CPU_ARM64)
    #if  defined(MY_CPU_ARM64) \
      || !defined(Z7_CLANG_VERSION) \
      || defined(__ARM_NEON)
      #define USE_SWAP_128
    #ifdef MY_CPU_ARM64
      // #define SWAP_ATTRIB_NEON __attribute__((__target__("")))
    #else
#if defined(Z7_CLANG_VERSION)
      // #define SWAP_ATTRIB_NEON __attribute__((__target__("neon")))
#else
      // #pragma message("SWAP_ATTRIB_NEON __attribute__((__target__(fpu=neon))")
      #define SWAP_ATTRIB_NEON __attribute__((__target__("fpu=neon")))
#endif
    #endif // MY_CPU_ARM64
    #endif // __ARM_NEON
    #endif // __ARM_ARCH
    #endif // __ARM_FP

  #elif defined(_MSC_VER)
    #if (_MSC_VER >= 1910)
      #define USE_SWAP_128
    #endif
  #endif

  #ifdef USE_SWAP_128
  #if defined(Z7_MSC_VER_ORIGINAL) && defined(MY_CPU_ARM64)
    #include <arm64_neon.h>
  #else

/*
#if !defined(__ARM_NEON)
#if defined(Z7_GCC_VERSION) && (__GNUC__  <   5) \
 || defined(Z7_GCC_VERSION) && (__GNUC__ ==   5) && (Z7_GCC_VERSION <  90201) \
 || defined(Z7_GCC_VERSION) && (__GNUC__ ==   5) && (Z7_GCC_VERSION < 100100)
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
#pragma message("#define __ARM_NEON 1")
// #define __ARM_NEON 1
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif
*/
    #include <arm_neon.h>
  #endif
  #endif

#ifndef USE_SWAP_128
  #define FORCE_SWAP_MODE
#else
 
#ifdef MY_CPU_ARM64
  // for debug : comment it
  #define FORCE_SWAP_MODE
#else
  #define k_SwapBytes_Mode_NEON 1
#endif
// typedef uint8x16_t v128;
#define SWAP2_128(i)   *(uint8x16_t *)      (void *)(items + (i) * 8) = \
      vrev16q_u8(*(const uint8x16_t *)(const void *)(items + (i) * 8));
#define SWAP4_128(i)   *(uint8x16_t *)      (void *)(items + (i) * 4) = \
      vrev32q_u8(*(const uint8x16_t *)(const void *)(items + (i) * 4));

// Z7_NO_INLINE
static
#ifdef SWAP_ATTRIB_NEON
SWAP_ATTRIB_NEON
#endif
Z7_ATTRIB_NO_VECTORIZE
void
Z7_FASTCALL
SwapBytes2_128(CSwapUInt16 *items, const CSwapUInt16 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP2_128(0)  SWAP2_128(1)  items += 2 * 8;
    SWAP2_128(0)  SWAP2_128(1)  items += 2 * 8;
  }
  while (items != lim);
}

// Z7_NO_INLINE
static
#ifdef SWAP_ATTRIB_NEON
SWAP_ATTRIB_NEON
#endif
Z7_ATTRIB_NO_VECTORIZE
void
Z7_FASTCALL
SwapBytes4_128(CSwapUInt32 *items, const CSwapUInt32 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP4_128(0)  SWAP4_128(1)  items += 2 * 4;
    SWAP4_128(0)  SWAP4_128(1)  items += 2 * 4;
  }
  while (items != lim);
}

#endif // USE_SWAP_128

#else // MY_CPU_ARM_OR_ARM64
#define FORCE_SWAP_MODE
#endif // MY_CPU_ARM_OR_ARM64






#if defined(Z7_MSC_VER_ORIGINAL) && defined(MY_CPU_X86)
  /* _byteswap_ushort() in MSVC x86 32-bit works via slow { mov dh, al; mov dl, ah }
     So we use own versions of byteswap function */
  #if (_MSC_VER < 1400 )  // old MSVC-X86 without _rotr16() support
    #define SWAP2_16(i)  { UInt32 v = items[i];  v += (v << 16);  v >>= 8;  items[i] = (CSwapUInt16)v; }
  #else  // is new MSVC-X86 with fast _rotr16()
    #include <intrin.h>
    #define SWAP2_16(i)  { items[i] = _rotr16(items[i], 8); }
  #endif
#else  // is not MSVC-X86
  #define SWAP2_16(i)  { CSwapUInt16 v = items[i];  items[i] = Z7_BSWAP16(v); }
#endif  // MSVC-X86

#if defined(Z7_CPU_FAST_BSWAP_SUPPORTED)
  #define SWAP4_32(i)  { CSwapUInt32 v = items[i];  items[i] = Z7_BSWAP32(v); }
#else
  #define SWAP4_32(i)  \
    { UInt32 v = items[i]; \
      v = ((v & 0xff00ff) << 8) + ((v >> 8) & 0xff00ff); \
      v = rotlFixed(v, 16); \
      items[i] = v; }
#endif




#if defined(FORCE_SWAP_MODE) && defined(USE_SWAP_128)
  #define DEFAULT_Swap2  SwapBytes2_128
  #if !defined(MY_CPU_X86_OR_AMD64)
    #define DEFAULT_Swap4  SwapBytes4_128
  #endif
#endif

#if !defined(DEFAULT_Swap2) || !defined(DEFAULT_Swap4)

#define SWAP_BASE_FUNCS_PREFIXES \
Z7_FORCE_INLINE  \
static \
Z7_ATTRIB_NO_VECTOR  \
void Z7_FASTCALL


#if defined(MY_CPU_ARM_OR_ARM64)
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#endif
#endif


#ifdef MY_CPU_64BIT

#if defined(MY_CPU_ARM64) \
    && defined(__ARM_ARCH) && (__ARM_ARCH >= 8) \
    && (  (defined(__GNUC__) && (__GNUC__ >= 4)) \
       || (defined(__clang__) && (__clang_major__ >= 4)))

  #define SWAP2_64_VAR(v)  asm ("rev16 %x0,%x0" : "+r" (v));
  #define SWAP4_64_VAR(v)  asm ("rev32 %x0,%x0" : "+r" (v));

#else  // is not ARM64-GNU

#if !defined(MY_CPU_X86_OR_AMD64) || (k_SwapBytes_Mode_MIN == 0) || !defined(USE_SWAP_128)
  #define SWAP2_64_VAR(v) \
    v = ( 0x00ff00ff00ff00ff & (v >> 8))  \
      + ((0x00ff00ff00ff00ff & v) << 8);
      /* plus gives faster code in MSVC */
#endif

#ifdef Z7_CPU_FAST_BSWAP_SUPPORTED
  #define SWAP4_64_VAR(v) \
    v = Z7_BSWAP64(v); \
    v = Z7_ROTL64(v, 32);
#else
  #define SWAP4_64_VAR(v) \
    v = ( 0x000000ff000000ff & (v >> 24))  \
      + ((0x000000ff000000ff & v) << 24 )  \
      + ( 0x0000ff000000ff00 & (v >>  8))  \
      + ((0x0000ff000000ff00 & v) <<  8 )  \
      ;
#endif

#endif  // ARM64-GNU


#ifdef SWAP2_64_VAR

#define SWAP2_64(i) { \
    UInt64 v = *(const UInt64 *)(const void *)(items + (i) * 4); \
    SWAP2_64_VAR(v) \
    *(UInt64 *)(void *)(items + (i) * 4) = v; }

SWAP_BASE_FUNCS_PREFIXES
SwapBytes2_64(CSwapUInt16 *items, const CSwapUInt16 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP2_64(0)  SWAP2_64(1)  items += 2 * 4;
    SWAP2_64(0)  SWAP2_64(1)  items += 2 * 4;
  }
  while (items != lim);
}

  #define DEFAULT_Swap2  SwapBytes2_64
  #if !defined(FORCE_SWAP_MODE)
    #define SWAP2_DEFAULT_MODE 0
  #endif
#else // !defined(SWAP2_64_VAR)
  #define DEFAULT_Swap2  SwapBytes2_128
  #if !defined(FORCE_SWAP_MODE)
    #define SWAP2_DEFAULT_MODE 1
  #endif
#endif // SWAP2_64_VAR


#define SWAP4_64(i) { \
    UInt64 v = *(const UInt64 *)(const void *)(items + (i) * 2); \
    SWAP4_64_VAR(v) \
    *(UInt64 *)(void *)(items + (i) * 2) = v; }

SWAP_BASE_FUNCS_PREFIXES
SwapBytes4_64(CSwapUInt32 *items, const CSwapUInt32 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP4_64(0)  SWAP4_64(1)  items += 2 * 2;
    SWAP4_64(0)  SWAP4_64(1)  items += 2 * 2;
  }
  while (items != lim);
}

#define DEFAULT_Swap4  SwapBytes4_64

#else  // is not 64BIT


#if defined(MY_CPU_ARM_OR_ARM64) \
    && defined(__ARM_ARCH) && (__ARM_ARCH >= 6) \
    && (  (defined(__GNUC__) && (__GNUC__ >= 4)) \
       || (defined(__clang__) && (__clang_major__ >= 4)))

#ifdef MY_CPU_64BIT
  #define SWAP2_32_VAR(v)  asm ("rev16 %w0,%w0" : "+r" (v));
#else
  #define SWAP2_32_VAR(v)  asm ("rev16 %0,%0" : "+r" (v)); // for clang/gcc
    // asm ("rev16 %r0,%r0" : "+r" (a));  // for gcc
#endif

#elif defined(_MSC_VER) && (_MSC_VER < 1300) && defined(MY_CPU_X86) \
    || !defined(Z7_CPU_FAST_BSWAP_SUPPORTED) \
    || !defined(Z7_CPU_FAST_ROTATE_SUPPORTED)
  // old msvc doesn't support _byteswap_ulong()
  #define SWAP2_32_VAR(v) \
    v = ((v & 0xff00ff) << 8) + ((v >> 8) & 0xff00ff);

#else  // is not ARM and is not old-MSVC-X86 and fast BSWAP/ROTATE are supported
  #define SWAP2_32_VAR(v) \
    v = Z7_BSWAP32(v); \
    v = rotlFixed(v, 16);

#endif  // GNU-ARM*

#define SWAP2_32(i) { \
    UInt32 v = *(const UInt32 *)(const void *)(items + (i) * 2); \
    SWAP2_32_VAR(v); \
    *(UInt32 *)(void *)(items + (i) * 2) = v; }


SWAP_BASE_FUNCS_PREFIXES
SwapBytes2_32(CSwapUInt16 *items, const CSwapUInt16 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP2_32(0)  SWAP2_32(1)  items += 2 * 2;
    SWAP2_32(0)  SWAP2_32(1)  items += 2 * 2;
  }
  while (items != lim);
}


SWAP_BASE_FUNCS_PREFIXES
SwapBytes4_32(CSwapUInt32 *items, const CSwapUInt32 *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SWAP4_32(0)  SWAP4_32(1)  items += 2;
    SWAP4_32(0)  SWAP4_32(1)  items += 2;
  }
  while (items != lim);
}

#define DEFAULT_Swap2  SwapBytes2_32
#define DEFAULT_Swap4  SwapBytes4_32
#if !defined(FORCE_SWAP_MODE)
  #define SWAP2_DEFAULT_MODE 0
#endif

#endif // MY_CPU_64BIT
#endif // if !defined(DEFAULT_Swap2) || !defined(DEFAULT_Swap4)



#if !defined(FORCE_SWAP_MODE)
static unsigned g_SwapBytes_Mode;
#endif

/* size of largest unrolled loop iteration: 128 bytes = 4 * 32 bytes (AVX). */
#define SWAP_ITERATION_BLOCK_SIZE_MAX  (1 << 7)

// 32 bytes for (AVX) or 2 * 16-bytes for NEON.
#define SWAP_VECTOR_ALIGN_SIZE  (1 << 5)

Z7_NO_INLINE
void z7_SwapBytes2(CSwapUInt16 *items, size_t numItems)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  for (; numItems != 0 && ((unsigned)(ptrdiff_t)items & (SWAP_VECTOR_ALIGN_SIZE - 1)) != 0; numItems--)
  {
    SWAP2_16(0)
    items++;
  }
  {
    const size_t k_Align_Mask = SWAP_ITERATION_BLOCK_SIZE_MAX / sizeof(CSwapUInt16) - 1;
    size_t numItems2 = numItems;
    CSwapUInt16 *lim;
    numItems &= k_Align_Mask;
    numItems2 &= ~(size_t)k_Align_Mask;
    lim = items + numItems2;
    if (numItems2 != 0)
    {
     #if !defined(FORCE_SWAP_MODE)
      #ifdef MY_CPU_X86_OR_AMD64
        #ifdef USE_SWAP_AVX2
          if (g_SwapBytes_Mode > k_SwapBytes_Mode_SSSE3)
            ShufBytes_256((__m256i *)(void *)items,
                (const __m256i *)(const void *)lim,
                (const __m128i *)(const void *)&(k_ShufMask_Swap2[0]));
          else
        #endif
        #ifdef USE_SWAP_SSSE3
          if (g_SwapBytes_Mode >= k_SwapBytes_Mode_SSSE3)
            ShufBytes_128((__m128i *)(void *)items,
                (const __m128i *)(const void *)lim,
                (const __m128i *)(const void *)&(k_ShufMask_Swap2[0]));
          else
        #endif
      #endif  // MY_CPU_X86_OR_AMD64
      #if SWAP2_DEFAULT_MODE == 0
          if (g_SwapBytes_Mode != 0)
            SwapBytes2_128(items, lim);
          else
      #endif
     #endif // FORCE_SWAP_MODE
            DEFAULT_Swap2(items, lim);
    }
    items = lim;
  }
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  for (; numItems != 0; numItems--)
  {
    SWAP2_16(0)
    items++;
  }
}


Z7_NO_INLINE
void z7_SwapBytes4(CSwapUInt32 *items, size_t numItems)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  for (; numItems != 0 && ((unsigned)(ptrdiff_t)items & (SWAP_VECTOR_ALIGN_SIZE - 1)) != 0; numItems--)
  {
    SWAP4_32(0)
    items++;
  }
  {
    const size_t k_Align_Mask = SWAP_ITERATION_BLOCK_SIZE_MAX / sizeof(CSwapUInt32) - 1;
    size_t numItems2 = numItems;
    CSwapUInt32 *lim;
    numItems &= k_Align_Mask;
    numItems2 &= ~(size_t)k_Align_Mask;
    lim = items + numItems2;
    if (numItems2 != 0)
    {
     #if !defined(FORCE_SWAP_MODE)
      #ifdef MY_CPU_X86_OR_AMD64
        #ifdef USE_SWAP_AVX2
          if (g_SwapBytes_Mode > k_SwapBytes_Mode_SSSE3)
            ShufBytes_256((__m256i *)(void *)items,
                (const __m256i *)(const void *)lim,
                (const __m128i *)(const void *)&(k_ShufMask_Swap4[0]));
          else
        #endif
        #ifdef USE_SWAP_SSSE3
          if (g_SwapBytes_Mode >= k_SwapBytes_Mode_SSSE3)
            ShufBytes_128((__m128i *)(void *)items,
                (const __m128i *)(const void *)lim,
                (const __m128i *)(const void *)&(k_ShufMask_Swap4[0]));
          else
        #endif
      #else  // MY_CPU_X86_OR_AMD64

          if (g_SwapBytes_Mode != 0)
            SwapBytes4_128(items, lim);
          else
      #endif  // MY_CPU_X86_OR_AMD64
     #endif // FORCE_SWAP_MODE
            DEFAULT_Swap4(items, lim);
    }
    items = lim;
  }
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  for (; numItems != 0; numItems--)
  {
    SWAP4_32(0)
    items++;
  }
}


// #define SHOW_HW_STATUS

#ifdef SHOW_HW_STATUS
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

void z7_SwapBytesPrepare(void)
{
#ifndef FORCE_SWAP_MODE
  unsigned mode = 0; // k_SwapBytes_Mode_BASE;

#ifdef MY_CPU_ARM_OR_ARM64
  {
    if (CPU_IsSupported_NEON())
    {
      // #pragma message ("=== SwapBytes NEON")
      PRF(printf("\n=== SwapBytes NEON\n");)
      mode = k_SwapBytes_Mode_NEON;
    }
  }
#else // MY_CPU_ARM_OR_ARM64
  {
    #ifdef USE_SWAP_AVX2
      if (CPU_IsSupported_AVX2())
      {
        // #pragma message ("=== SwapBytes AVX2")
        PRF(printf("\n=== SwapBytes AVX2\n");)
        mode = k_SwapBytes_Mode_AVX2;
      }
      else
    #endif
    #ifdef USE_SWAP_SSSE3
      if (CPU_IsSupported_SSSE3())
      {
        // #pragma message ("=== SwapBytes SSSE3")
        PRF(printf("\n=== SwapBytes SSSE3\n");)
        mode = k_SwapBytes_Mode_SSSE3;
      }
      else
    #endif
    #if !defined(MY_CPU_AMD64)
      if (CPU_IsSupported_SSE2())
    #endif
      {
        // #pragma message ("=== SwapBytes SSE2")
        PRF(printf("\n=== SwapBytes SSE2\n");)
        mode = k_SwapBytes_Mode_SSE2;
      }
  }
#endif // MY_CPU_ARM_OR_ARM64
  g_SwapBytes_Mode = mode;
  // g_SwapBytes_Mode = 0; // for debug
#endif // FORCE_SWAP_MODE
  PRF(printf("\n=== SwapBytesPrepare\n");)
}

#undef PRF

/* ================ unit: C/Threads.c ================ */
/* Threads.c -- multithreading library
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#ifdef _WIN32

#ifndef USE_THREADS_CreateThread
#include <process.h>
#endif

// amalgamation: header emitted in prologue

static WRes GetError(void)
{
  const DWORD res = GetLastError();
  return res ? (WRes)res : 1;
}

static WRes HandleToWRes(HANDLE h) { return (h != NULL) ? 0 : GetError(); }
static WRes BOOLToWRes(BOOL v) { return v ? 0 : GetError(); }

WRes HandlePtr_Close(HANDLE *p)
{
  if (*p != NULL)
  {
    if (!CloseHandle(*p))
      return GetError();
    *p = NULL;
  }
  return 0;
}

WRes Handle_WaitObject(HANDLE h)
{
  DWORD dw = WaitForSingleObject(h, INFINITE);
  /*
    (dw) result:
    WAIT_OBJECT_0  // 0
    WAIT_ABANDONED // 0x00000080 : is not compatible with Win32 Error space
    WAIT_TIMEOUT   // 0x00000102 : is     compatible with Win32 Error space
    WAIT_FAILED    // 0xFFFFFFFF
  */
  if (dw == WAIT_FAILED)
  {
    dw = GetLastError();
    if (dw == 0)
      return WAIT_FAILED;
  }
  return (WRes)dw;
}

#define Thread_Wait(p) Handle_WaitObject(*(p))

WRes Thread_Wait_Close(CThread *p)
{
  WRes res = Thread_Wait(p);
  WRes res2 = Thread_Close(p);
  return (res != 0 ? res : res2);
}

typedef struct MY_PROCESSOR_NUMBER {
    WORD  Group;
    BYTE  Number;
    BYTE  Reserved;
} MY_PROCESSOR_NUMBER, *MY_PPROCESSOR_NUMBER;

typedef struct MY_GROUP_AFFINITY {
#if defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION < 100000)
    // KAFFINITY is not defined in old mingw
    ULONG_PTR
#else
    KAFFINITY
#endif
      Mask;
    WORD   Group;
    WORD   Reserved[3];
} MY_GROUP_AFFINITY, *MY_PGROUP_AFFINITY;

typedef BOOL (WINAPI *Func_SetThreadGroupAffinity)(
    HANDLE hThread,
    CONST MY_GROUP_AFFINITY *GroupAffinity,
    MY_PGROUP_AFFINITY PreviousGroupAffinity);

typedef BOOL (WINAPI *Func_GetThreadGroupAffinity)(
    HANDLE hThread,
    MY_PGROUP_AFFINITY GroupAffinity);

typedef BOOL (WINAPI *Func_GetProcessGroupAffinity)(
    HANDLE hProcess,
    PUSHORT GroupCount,
    PUSHORT GroupArray);

Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION

#if 0
#include <stdio.h>
#define PRF(x) x
/*
--
  before call of SetThreadGroupAffinity()
    GetProcessGroupAffinity return one group.
  after call of SetThreadGroupAffinity():
    GetProcessGroupAffinity return more than group,
    if SetThreadGroupAffinity() was to another group.
--
  GetProcessAffinityMask MS DOCs:
  {
    If the calling process contains threads in multiple groups,
    the function returns zero for both affinity masks.
  }
  but tests in win10 with 2 groups (less than 64 cores total):
    GetProcessAffinityMask() still returns non-zero affinity masks
    even after SetThreadGroupAffinity() calls.
*/
static void PrintProcess_Info()
{
  {
    const
      Func_GetProcessGroupAffinity fn_GetProcessGroupAffinity =
     (Func_GetProcessGroupAffinity) Z7_CAST_FUNC_C GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
          "GetProcessGroupAffinity");
    if (fn_GetProcessGroupAffinity)
    {
      unsigned i;
      USHORT GroupCounts[64];
      USHORT GroupCount = Z7_ARRAY_SIZE(GroupCounts);
      BOOL boolRes = fn_GetProcessGroupAffinity(GetCurrentProcess(),
          &GroupCount, GroupCounts);
      printf("\n====== GetProcessGroupAffinity : "
          "boolRes=%u GroupCounts = %u :",
          boolRes, (unsigned)GroupCount);
      for (i = 0; i < GroupCount; i++)
        printf(" %u", GroupCounts[i]);
      printf("\n");
    }
  }
  {
    DWORD_PTR processAffinityMask, systemAffinityMask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask))
    {
      PRF(printf("\n====== GetProcessAffinityMask : "
        ": processAffinityMask=%x, systemAffinityMask=%x\n",
        (UInt32)processAffinityMask, (UInt32)systemAffinityMask);)
    }
    else
      printf("\n==GetProcessAffinityMask FAIL");
  }
}
#else
#ifndef USE_THREADS_CreateThread
// #define PRF(x)
#endif
#endif

/* if we send (stackSize=0) to CreateThread(), it will
   use default value PE::SizeOfStackReserve from exe file.
   PE::SizeOfStackReserve == 1 MiB in exe file with default linker options.
   Windows aligns specified value to the next 64 KB range. */
static const unsigned k_StackSize_ReserveSize =
  #ifdef UNDER_CE
    1 << 17;
  #else
    1 << 20;
  #endif

WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */

  #ifdef USE_THREADS_CreateThread

  DWORD threadId;
  *p = CreateThread(NULL, k_StackSize_ReserveSize, func, param, STACK_SIZE_PARAM_IS_A_RESERVATION, &threadId);
  
  #else

#define CALL_beginthreadex(func2, param2, flags, threadIdPtr) \
    ((HANDLE)(_beginthreadex(NULL, k_StackSize_ReserveSize, func2, param2, (flags) | STACK_SIZE_PARAM_IS_A_RESERVATION, threadIdPtr)))
  
  unsigned threadId;
  *p = CALL_beginthreadex(func, param, 0, &threadId);

#if 0 // 1 : for debug
  {
      DWORD_PTR prevMask;
      DWORD_PTR affinity = 1 << 0;
      prevMask = SetThreadAffinityMask(*p, (DWORD_PTR)affinity);
      prevMask = prevMask;
  }
#endif
#if 0 // 1 : for debug
  {
      /* win10: new thread will be created in same group that is assigned to parent thread
                but affinity mask will contain all allowed threads of that group,
                even if affinity mask of parent group is not full
         win11: what group it will be created, if we have set
                affinity of parent thread with ThreadGroupAffinity?
      */
      const
         Func_GetThreadGroupAffinity fn =
        (Func_GetThreadGroupAffinity) Z7_CAST_FUNC_C GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
             "GetThreadGroupAffinity");
      if (fn)
      {
        // BOOL wres2;
        MY_GROUP_AFFINITY groupAffinity;
        memset(&groupAffinity, 0, sizeof(groupAffinity));
        /* wres2 = */ fn(*p, &groupAffinity);
        PRF(printf("\n==Thread_Create cur = %6u GetThreadGroupAffinity(): "
            "wres2_BOOL = %u, group=%u mask=%x\n",
            GetCurrentThreadId(),
            wres2,
            groupAffinity.Group,
            (UInt32)groupAffinity.Mask);)
      }
  }
#endif

  #endif

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return HandleToWRes(*p);
}


WRes Thread_Create_With_Affinity(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, CAffinityMask affinity)
{
  #ifdef USE_THREADS_CreateThread

  UNUSED_VAR(affinity)
  return Thread_Create(p, func, param);
  
  #else
  
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  HANDLE h;
  WRes wres;
  unsigned threadId;
  h = CALL_beginthreadex(func, param, CREATE_SUSPENDED, &threadId);
  *p = h;
  wres = HandleToWRes(h);
  if (h)
  {
    {
      // DWORD_PTR prevMask =
      SetThreadAffinityMask(h, (DWORD_PTR)affinity);
      /*
      if (prevMask == 0)
      {
        // affinity change is non-critical error, so we can ignore it
        // wres = GetError();
      }
      */
    }
    {
      const DWORD prevSuspendCount = ResumeThread(h);
      /* ResumeThread() returns:
         0 : was_not_suspended
         1 : was_resumed
        -1 : error
      */
      if (prevSuspendCount == (DWORD)-1)
        wres = GetError();
    }
  }

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return wres;

  #endif
}


WRes Thread_Create_With_Group(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, unsigned group, CAffinityMask affinityMask)
{
#ifdef USE_THREADS_CreateThread

  UNUSED_VAR(group)
  UNUSED_VAR(affinityMask)
  return Thread_Create(p, func, param);
  
#else
  
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  HANDLE h;
  WRes wres;
  unsigned threadId;
  h = CALL_beginthreadex(func, param, CREATE_SUSPENDED, &threadId);
  *p = h;
  wres = HandleToWRes(h);
  if (h)
  {
    // PrintProcess_Info();
    {
      const
         Func_SetThreadGroupAffinity fn =
        (Func_SetThreadGroupAffinity) Z7_CAST_FUNC_C GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
             "SetThreadGroupAffinity");
      if (fn)
      {
        // WRes wres2;
        MY_GROUP_AFFINITY groupAffinity, prev_groupAffinity;
        memset(&groupAffinity, 0, sizeof(groupAffinity));
        // groupAffinity.Mask must use only bits that supported by current group
        // (groupAffinity.Mask = 0) means all allowed bits
        groupAffinity.Mask = affinityMask;
        groupAffinity.Group = (WORD)group;
        // wres2 =
        fn(h, &groupAffinity, &prev_groupAffinity);
        /*
        if (groupAffinity.Group == prev_groupAffinity.Group)
          wres2 = wres2;
        else
          wres2 = wres2;
        if (wres2 == 0)
        {
          wres2 = GetError();
          PRF(printf("\n==SetThreadGroupAffinity error: %u\n", wres2);)
        }
        else
        {
          PRF(printf("\n==Thread_Create_With_Group::SetThreadGroupAffinity()"
            " threadId = %6u"
            " group=%u mask=%x\n",
            threadId,
            prev_groupAffinity.Group,
            (UInt32)prev_groupAffinity.Mask);)
        }
        */
      }
    }
    {
      const DWORD prevSuspendCount = ResumeThread(h);
      /* ResumeThread() returns:
         0 : was_not_suspended
         1 : was_resumed
        -1 : error
      */
      if (prevSuspendCount == (DWORD)-1)
        wres = GetError();
    }
  }

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return wres;

  #endif
}


static WRes Event_Create(CEvent *p, BOOL manualReset, int signaled)
{
  *p = CreateEvent(NULL, manualReset, (signaled ? TRUE : FALSE), NULL);
  return HandleToWRes(*p);
}

WRes Event_Set(CEvent *p) { return BOOLToWRes(SetEvent(*p)); }
WRes Event_Reset(CEvent *p) { return BOOLToWRes(ResetEvent(*p)); }

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled) { return Event_Create(p, TRUE, signaled); }
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled) { return Event_Create(p, FALSE, signaled); }
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p) { return ManualResetEvent_Create(p, 0); }
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) { return AutoResetEvent_Create(p, 0); }


WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
  // negative ((LONG)maxCount) is not supported in WIN32::CreateSemaphore()
  *p = CreateSemaphore(NULL, (LONG)initCount, (LONG)maxCount, NULL);
  return HandleToWRes(*p);
}

WRes Semaphore_OptCreateInit(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
  // if (Semaphore_IsCreated(p))
  {
    WRes wres = Semaphore_Close(p);
    if (wres != 0)
      return wres;
  }
  return Semaphore_Create(p, initCount, maxCount);
}

static WRes Semaphore_Release(CSemaphore *p, LONG releaseCount, LONG *previousCount)
  { return BOOLToWRes(ReleaseSemaphore(*p, releaseCount, previousCount)); }
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num)
  { return Semaphore_Release(p, (LONG)num, NULL); }
WRes Semaphore_Release1(CSemaphore *p) { return Semaphore_ReleaseN(p, 1); }

WRes CriticalSection_Init(CCriticalSection *p)
{
  /* InitializeCriticalSection() can raise exception:
     Windows XP, 2003 : can raise a STATUS_NO_MEMORY exception
     Windows Vista+   : no exceptions */
  #ifdef _MSC_VER
  #ifdef __clang__
    #pragma GCC diagnostic ignored "-Wlanguage-extension-token"
  #endif
  __try
  #endif
  {
    InitializeCriticalSection(p);
    /* InitializeCriticalSectionAndSpinCount(p, 0); */
  }
  #ifdef _MSC_VER
  __except (EXCEPTION_EXECUTE_HANDLER) { return ERROR_NOT_ENOUGH_MEMORY; }
  #endif
  return 0;
}




#else // _WIN32

// ---------- POSIX ----------

#if defined(__linux__) && !defined(__APPLE__) && !defined(_AIX) && !defined(__ANDROID__)
#ifndef Z7_AFFINITY_DISABLE
// _GNU_SOURCE can be required for pthread_setaffinity_np() / CPU_ZERO / CPU_SET
// clang < 3.6       : unknown warning group '-Wreserved-id-macro'
// clang 3.6 - 12.01 : gives warning "macro name is a reserved identifier"
// clang >= 13       : do not give warning
#if !defined(_GNU_SOURCE)
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
// #define _GNU_SOURCE
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif // !defined(_GNU_SOURCE)
#endif // Z7_AFFINITY_DISABLE
#endif // __linux__

// amalgamation: header emitted in prologue

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef Z7_AFFINITY_SUPPORTED
// #include <sched.h>
#endif


// #include <stdio.h>
// #define PRF(p) p
#define PRF(p)
#define Print(s) PRF(printf("\n%s\n", s);)

WRes Thread_Create_With_CpuSet(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, const CCpuSet *cpuSet)
{
  // new thread in Posix probably inherits affinity from parrent thread
  Print("Thread_Create_With_CpuSet")

  pthread_attr_t attr;
  int ret;
  // int ret2;

  p->_created = 0;

  RINOK(pthread_attr_init(&attr))

  ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  if (!ret)
  {
    if (cpuSet)
    {
      // pthread_attr_setaffinity_np() is not supported for MUSL compile.
      // so we check for __GLIBC__ here
#if defined(Z7_AFFINITY_SUPPORTED) && defined( __GLIBC__)
      /*
      printf("\n affinity :");
      unsigned i;
      for (i = 0; i < sizeof(*cpuSet) && i < 8; i++)
      {
        Byte b = *((const Byte *)cpuSet + i);
        char temp[32];
        #define GET_HEX_CHAR(t) ((char)(((t < 10) ? ('0' + t) : ('A' + (t - 10)))))
        temp[0] = GET_HEX_CHAR((b & 0xF));
        temp[1] = GET_HEX_CHAR((b >> 4));
        // temp[0] = GET_HEX_CHAR((b >> 4));  // big-endian
        // temp[1] = GET_HEX_CHAR((b & 0xF));  // big-endian
        temp[2] = 0;
        printf("%s", temp);
      }
      printf("\n");
      */

      // ret2 =
      pthread_attr_setaffinity_np(&attr, sizeof(*cpuSet), cpuSet);
      // if (ret2) ret = ret2;
#endif
    }
    
    ret = pthread_create(&p->_tid, &attr, func, param);
    
    if (!ret)
    {
      p->_created = 1;
      /*
      if (cpuSet)
      {
        // ret2 =
        pthread_setaffinity_np(p->_tid, sizeof(*cpuSet), cpuSet);
        // if (ret2) ret = ret2;
      }
      */
    }
  }
  // ret2 =
  pthread_attr_destroy(&attr);
  // if (ret2 != 0) ret = ret2;
  return ret;
}


WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
  return Thread_Create_With_CpuSet(p, func, param, NULL);
}

/*
WRes Thread_Create_With_Group(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, unsigned group, CAffinityMask affinity)
{
  UNUSED_VAR(group)
  return Thread_Create_With_Affinity(p, func, param, affinity);
}
*/

WRes Thread_Create_With_Affinity(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, CAffinityMask affinity)
{
  Print("Thread_Create_WithAffinity")
  CCpuSet cs;
  unsigned i;
  CpuSet_Zero(&cs);
  for (i = 0; i < sizeof(affinity) * 8; i++)
  {
    if (affinity == 0)
      break;
    if (affinity & 1)
    {
      CpuSet_Set(&cs, i);
    }
    affinity >>= 1;
  }
  return Thread_Create_With_CpuSet(p, func, param, &cs);
}


WRes Thread_Close(CThread *p)
{
  // Print("Thread_Close")
  int ret;
  if (!p->_created)
    return 0;
    
  ret = pthread_detach(p->_tid);
  p->_tid = 0;
  p->_created = 0;
  return ret;
}


WRes Thread_Wait_Close(CThread *p)
{
  // Print("Thread_Wait_Close")
  void *thread_return;
  int ret;
  if (!p->_created)
    return EINVAL;

  ret = pthread_join(p->_tid, &thread_return);
  // probably we can't use that (_tid) after pthread_join(), so we close thread here
  p->_created = 0;
  p->_tid = 0;
  return ret;
}



static WRes Event_Create(CEvent *p, int manualReset, int signaled)
{
  RINOK(pthread_mutex_init(&p->_mutex, NULL))
  RINOK(pthread_cond_init(&p->_cond, NULL))
  p->_manual_reset = manualReset;
  p->_state = (signaled ? True : False);
  p->_created = 1;
  return 0;
}

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled)
  { return Event_Create(p, True, signaled); }
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p)
  { return ManualResetEvent_Create(p, 0); }
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled)
  { return Event_Create(p, False, signaled); }
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p)
  { return AutoResetEvent_Create(p, 0); }


#if defined(Z7_LLVM_CLANG_VERSION) && (__clang_major__ == 13)
// freebsd:
#pragma GCC diagnostic ignored "-Wthread-safety-analysis"
#endif

WRes Event_Set(CEvent *p)
{
  RINOK(pthread_mutex_lock(&p->_mutex))
  p->_state = True;
  {
    const int res1 = pthread_cond_broadcast(&p->_cond);
    const int res2 = pthread_mutex_unlock(&p->_mutex);
    return (res2 ? res2 : res1);
  }
}

WRes Event_Reset(CEvent *p)
{
  RINOK(pthread_mutex_lock(&p->_mutex))
  p->_state = False;
  return pthread_mutex_unlock(&p->_mutex);
}
 
WRes Event_Wait(CEvent *p)
{
  RINOK(pthread_mutex_lock(&p->_mutex))
  while (p->_state == False)
  {
    // ETIMEDOUT
    // ret =
    pthread_cond_wait(&p->_cond, &p->_mutex);
    // if (ret != 0) break;
  }
  if (p->_manual_reset == False)
  {
    p->_state = False;
  }
  return pthread_mutex_unlock(&p->_mutex);
}

WRes Event_Close(CEvent *p)
{
  if (!p->_created)
    return 0;
  p->_created = 0;
  {
    const int res1 = pthread_mutex_destroy(&p->_mutex);
    const int res2 = pthread_cond_destroy(&p->_cond);
    return (res1 ? res1 : res2);
  }
}


WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
  if (initCount > maxCount || maxCount < 1)
    return EINVAL;
  RINOK(pthread_mutex_init(&p->_mutex, NULL))
  RINOK(pthread_cond_init(&p->_cond, NULL))
  p->_count = initCount;
  p->_maxCount = maxCount;
  p->_created = 1;
  return 0;
}


WRes Semaphore_OptCreateInit(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
  if (Semaphore_IsCreated(p))
  {
    /*
    WRes wres = Semaphore_Close(p);
    if (wres != 0)
      return wres;
    */
    if (initCount > maxCount || maxCount < 1)
      return EINVAL;
    // return EINVAL; // for debug
    p->_count = initCount;
    p->_maxCount = maxCount;
    return 0;
  }
  return Semaphore_Create(p, initCount, maxCount);
}


WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 releaseCount)
{
  UInt32 newCount;
  int ret;

  if (releaseCount < 1)
    return EINVAL;

  RINOK(pthread_mutex_lock(&p->_mutex))

  newCount = p->_count + releaseCount;
  if (newCount > p->_maxCount)
    ret = ERROR_TOO_MANY_POSTS; // EINVAL;
  else
  {
    p->_count = newCount;
    ret = pthread_cond_broadcast(&p->_cond);
  }
  RINOK(pthread_mutex_unlock(&p->_mutex))
  return ret;
}

WRes Semaphore_Wait(CSemaphore *p)
{
  RINOK(pthread_mutex_lock(&p->_mutex))
  while (p->_count < 1)
  {
    pthread_cond_wait(&p->_cond, &p->_mutex);
  }
  p->_count--;
  return pthread_mutex_unlock(&p->_mutex);
}

WRes Semaphore_Close(CSemaphore *p)
{
  if (!p->_created)
    return 0;
  p->_created = 0;
  {
    const int res1 = pthread_mutex_destroy(&p->_mutex);
    const int res2 = pthread_cond_destroy(&p->_cond);
    return (res1 ? res1 : res2);
  }
}



WRes CriticalSection_Init(CCriticalSection *p)
{
  // Print("CriticalSection_Init")
  if (!p)
    return EINTR;
  return pthread_mutex_init(&p->_mutex, NULL);
}

void CriticalSection_Enter(CCriticalSection *p)
{
  // Print("CriticalSection_Enter")
  if (p)
  {
    // int ret =
    pthread_mutex_lock(&p->_mutex);
  }
}

void CriticalSection_Leave(CCriticalSection *p)
{
  // Print("CriticalSection_Leave")
  if (p)
  {
    // int ret =
    pthread_mutex_unlock(&p->_mutex);
  }
}

void CriticalSection_Delete(CCriticalSection *p)
{
  // Print("CriticalSection_Delete")
  if (p)
  {
    // int ret =
    pthread_mutex_destroy(&p->_mutex);
  }
}

LONG InterlockedIncrement(LONG volatile *addend)
{
  // Print("InterlockedIncrement")
  #ifdef USE_HACK_UNSAFE_ATOMIC
    LONG val = *addend + 1;
    *addend = val;
    return val;
  #else

  #if defined(__clang__) && (__clang_major__ >= 8)
    #pragma GCC diagnostic ignored "-Watomic-implicit-seq-cst"
  #endif
    return __sync_add_and_fetch(addend, 1);
  #endif
}

LONG InterlockedDecrement(LONG volatile *addend)
{
  // Print("InterlockedDecrement")
  #ifdef USE_HACK_UNSAFE_ATOMIC
    LONG val = *addend - 1;
    *addend = val;
    return val;
  #else
    return __sync_sub_and_fetch(addend, 1);
  #endif
}

#endif // _WIN32

WRes AutoResetEvent_OptCreate_And_Reset(CAutoResetEvent *p)
{
  if (Event_IsCreated(p))
    return Event_Reset(p);
  return AutoResetEvent_CreateNotSignaled(p);
}

void ThreadNextGroup_Init(CThreadNextGroup *p, UInt32 numGroups, UInt32 startGroup)
{
  // printf("\n====== ThreadNextGroup_Init numGroups = %x: startGroup=%x\n", numGroups, startGroup);
  if (numGroups == 0)
      numGroups = 1;
  p->NumGroups = numGroups;
  p->NextGroup = startGroup % numGroups;
}


UInt32 ThreadNextGroup_GetNext(CThreadNextGroup *p)
{
  const UInt32 next = p->NextGroup;
  p->NextGroup = (next + 1) % p->NumGroups;
  return next;
}

#undef PRF
#undef Print

/* ================ unit: C/Xxh64.c ================ */
/* Xxh64.c -- XXH64 hash calculation
original code: Copyright (c) Yann Collet.
modified by Igor Pavlov.
This source code is licensed under BSD 2-Clause License.
*/

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define Z7_XXH_PRIME64_1  UINT64_CONST(0x9E3779B185EBCA87)
#define Z7_XXH_PRIME64_2  UINT64_CONST(0xC2B2AE3D27D4EB4F)
#define Z7_XXH_PRIME64_3  UINT64_CONST(0x165667B19E3779F9)
#define Z7_XXH_PRIME64_4  UINT64_CONST(0x85EBCA77C2B2AE63)
#define Z7_XXH_PRIME64_5  UINT64_CONST(0x27D4EB2F165667C5)

void Xxh64State_Init(CXxh64State *p)
{
  const UInt64 seed = 0;
  p->v[0] = seed + Z7_XXH_PRIME64_1 + Z7_XXH_PRIME64_2;
  p->v[1] = seed + Z7_XXH_PRIME64_2;
  p->v[2] = seed;
  p->v[3] = seed - Z7_XXH_PRIME64_1;
}

#if !defined(MY_CPU_64BIT) && defined(MY_CPU_X86) && defined(_MSC_VER)
  #define Z7_XXH64_USE_ASM
#elif !defined(MY_CPU_LE_UNALIGN_64) // && defined (MY_CPU_LE)
  #define Z7_XXH64_USE_ALIGNED
#endif

#ifdef Z7_XXH64_USE_ALIGNED
  #define Xxh64State_UpdateBlocks_Unaligned_Select  Xxh64State_UpdateBlocks_Unaligned
#else
  #define Xxh64State_UpdateBlocks_Unaligned_Select  Xxh64State_UpdateBlocks
#endif

#if !defined(MY_CPU_64BIT) && defined(MY_CPU_X86) \
    && defined(Z7_MSC_VER_ORIGINAL) && Z7_MSC_VER_ORIGINAL > 1200
/* we try to avoid __allmul calls in MSVC for 64-bit multiply.
   But MSVC6 still uses __allmul for our code.
   So for MSVC6 we use default 64-bit multiply without our optimization.
*/
#define LOW32(b)        ((UInt32)(b & 0xffffffff))
/* MSVC compiler (MSVC > 1200) can use "mul" instruction
   without __allmul for our MY_emulu MACRO.
   MY_emulu is similar to __emulu(a, b) MACRO */
#define MY_emulu(a, b)      ((UInt64)(a) * (b))
#define MY_SET_HIGH32(a)    ((UInt64)(a) << 32)
#define MY_MUL32_SET_HIGH32(a, b) MY_SET_HIGH32((UInt32)(a) * (UInt32)(b))
// /*
#define MY_MUL64(a, b) \
    ( MY_emulu((UInt32)(a), LOW32(b)) + \
      MY_SET_HIGH32( \
        (UInt32)((a) >> 32) * LOW32(b) + \
        (UInt32)(a) * (UInt32)((b) >> 32) \
      ))
// */
/*
#define MY_MUL64(a, b) \
    ( MY_emulu((UInt32)(a), LOW32(b)) \
      + MY_MUL32_SET_HIGH32((a) >> 32, LOW32(b)) + \
      + MY_MUL32_SET_HIGH32(a, (b) >> 32) \
    )
*/

#define MY_MUL_32_64(a32, b) \
    ( MY_emulu((UInt32)(a32), LOW32(b)) \
      + MY_MUL32_SET_HIGH32(a32, (b) >> 32) \
    )

#else
#define MY_MUL64(a, b)        ((a) * (b))
#define MY_MUL_32_64(a32, b)  ((a32) * (UInt64)(b))
#endif


static
Z7_FORCE_INLINE
UInt64 Xxh64_Round(UInt64 acc, UInt64 input)
{
  acc += MY_MUL64(input, Z7_XXH_PRIME64_2);
  acc = Z7_ROTL64(acc, 31);
  return MY_MUL64(acc, Z7_XXH_PRIME64_1);
}

static UInt64 Xxh64_Merge(UInt64 acc, UInt64 val)
{
  acc ^= Xxh64_Round(0, val);
  return MY_MUL64(acc, Z7_XXH_PRIME64_1) + Z7_XXH_PRIME64_4;
}


#ifdef Z7_XXH64_USE_ASM

#define Z7_XXH_PRIME64_1_HIGH  0x9E3779B1
#define Z7_XXH_PRIME64_1_LOW   0x85EBCA87
#define Z7_XXH_PRIME64_2_HIGH  0xC2B2AE3D
#define Z7_XXH_PRIME64_2_LOW   0x27D4EB4F

void
Z7_NO_INLINE
__declspec(naked)
Z7_FASTCALL
Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end)
{
 #if !defined(__clang__)
  UNUSED_VAR(p)
  UNUSED_VAR(data)
  UNUSED_VAR(end)
 #endif
  __asm   push    ebx
  __asm   push    ebp
  __asm   push    esi
  __asm   push    edi

  #define STACK_OFFSET  4 * 8
  __asm   sub     esp, STACK_OFFSET

#define COPY_1(n) \
  __asm   mov     eax, [ecx + n * 4] \
  __asm   mov     [esp + n * 4], eax \

#define COPY_2(n) \
  __asm   mov     eax, [esp + n * 4] \
  __asm   mov     [ecx + n * 4], eax \

  COPY_1(0)
  __asm   mov     edi, [ecx + 1 * 4] \
  COPY_1(2)
  COPY_1(3)
  COPY_1(4)
  COPY_1(5)
  COPY_1(6)
  COPY_1(7)

  __asm   mov     esi, edx \
  __asm   mov     [esp + 0 * 8 + 4], ecx
  __asm   mov     ecx, Z7_XXH_PRIME64_2_LOW \
  __asm   mov     ebp, Z7_XXH_PRIME64_1_LOW \

#define R(n, state1, state1_reg) \
  __asm   mov     eax, [esi + n * 8] \
  __asm   imul    ebx, eax, Z7_XXH_PRIME64_2_HIGH \
  __asm   add     ebx, state1 \
  __asm   mul     ecx \
  __asm   add     edx, ebx \
  __asm   mov     ebx, [esi + n * 8 + 4] \
  __asm   imul    ebx, ecx \
  __asm   add     eax, [esp + n * 8] \
  __asm   adc     edx, ebx \
  __asm   mov     ebx, eax \
  __asm   shld    eax, edx, 31 \
  __asm   shld    edx, ebx, 31 \
  __asm   imul    state1_reg, eax, Z7_XXH_PRIME64_1_HIGH \
  __asm   imul    edx, ebp \
  __asm   add     state1_reg, edx \
  __asm   mul     ebp \
  __asm   add     state1_reg, edx \
  __asm   mov     [esp + n * 8], eax \

#define R2(n) \
  R(n, [esp + n * 8 + 4], ebx) \
  __asm   mov     [esp + n * 8 + 4], ebx \

  __asm   align 16
  __asm   main_loop:
  R(0, edi, edi)
  R2(1)
  R2(2)
  R2(3)
  __asm   add     esi, 32
  __asm   cmp     esi, [esp + STACK_OFFSET + 4 * 4 + 4]
  __asm   jne     main_loop

  __asm   mov     ecx, [esp + 0 * 8 + 4]

  COPY_2(0)
  __asm   mov     [ecx + 1 * 4], edi
  COPY_2(2)
  COPY_2(3)
  COPY_2(4)
  COPY_2(5)
  COPY_2(6)
  COPY_2(7)

  __asm   add     esp, STACK_OFFSET
  __asm   pop     edi
  __asm   pop     esi
  __asm   pop     ebp
  __asm   pop     ebx
  __asm   ret     4
}

#else

#ifdef Z7_XXH64_USE_ALIGNED
static
#endif
void
Z7_NO_INLINE
Z7_FASTCALL
Xxh64State_UpdateBlocks_Unaligned_Select(CXxh64State *p, const void *_data, const void *end)
{
  const Byte *data = (const Byte *)_data;
  UInt64 v0, v1, v2, v3;
  v0 = p->v[0];
  v1 = p->v[1];
  v2 = p->v[2];
  v3 = p->v[3];
  do
  {
    v0 = Xxh64_Round(v0, GetUi64(data));  data += 8;
    v1 = Xxh64_Round(v1, GetUi64(data));  data += 8;
    v2 = Xxh64_Round(v2, GetUi64(data));  data += 8;
    v3 = Xxh64_Round(v3, GetUi64(data));  data += 8;
  }
  while (data != end);
  p->v[0] = v0;
  p->v[1] = v1;
  p->v[2] = v2;
  p->v[3] = v3;
}


#ifdef Z7_XXH64_USE_ALIGNED

static
void
Z7_NO_INLINE
Z7_FASTCALL
Xxh64State_UpdateBlocks_Aligned(CXxh64State *p, const void *_data, const void *end)
{
  const Byte *data = (const Byte *)_data;
  UInt64 v0, v1, v2, v3;
  v0 = p->v[0];
  v1 = p->v[1];
  v2 = p->v[2];
  v3 = p->v[3];
  do
  {
    v0 = Xxh64_Round(v0, GetUi64a(data));  data += 8;
    v1 = Xxh64_Round(v1, GetUi64a(data));  data += 8;
    v2 = Xxh64_Round(v2, GetUi64a(data));  data += 8;
    v3 = Xxh64_Round(v3, GetUi64a(data));  data += 8;
  }
  while (data != end);
  p->v[0] = v0;
  p->v[1] = v1;
  p->v[2] = v2;
  p->v[3] = v3;
}

void
Z7_NO_INLINE
Z7_FASTCALL
Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end)
{
  if (((unsigned)(ptrdiff_t)data & 7) == 0)
    Xxh64State_UpdateBlocks_Aligned(p, data, end);
  else
    Xxh64State_UpdateBlocks_Unaligned(p, data, end);
}

#endif // Z7_XXH64_USE_ALIGNED
#endif // Z7_XXH64_USE_ASM

UInt64 Xxh64State_Digest(const CXxh64State *p, const void *_data, UInt64 count)
{
  UInt64 h = p->v[2];
 
  if (count >= 32)
  {
    h = Z7_ROTL64(p->v[0], 1) +
        Z7_ROTL64(p->v[1], 7) +
        Z7_ROTL64(h, 12) +
        Z7_ROTL64(p->v[3], 18);
    h = Xxh64_Merge(h, p->v[0]);
    h = Xxh64_Merge(h, p->v[1]);
    h = Xxh64_Merge(h, p->v[2]);
    h = Xxh64_Merge(h, p->v[3]);
  }
  else
    h += Z7_XXH_PRIME64_5;
  
  h += count;
  
  // XXH64_finalize():
  {
    unsigned cnt = (unsigned)count & 31;
    const Byte *data = (const Byte *)_data;
    while (cnt >= 8)
    {
      h ^= Xxh64_Round(0, GetUi64(data));
      data += 8;
      h = Z7_ROTL64(h, 27);
      h = MY_MUL64(h, Z7_XXH_PRIME64_1) + Z7_XXH_PRIME64_4;
      cnt -= 8;
    }
    if (cnt >= 4)
    {
      const UInt32 v = GetUi32(data);
      data += 4;
      h ^= MY_MUL_32_64(v, Z7_XXH_PRIME64_1);
      h = Z7_ROTL64(h, 23);
      h = MY_MUL64(h, Z7_XXH_PRIME64_2) + Z7_XXH_PRIME64_3;
      cnt -= 4;
    }
    while (cnt)
    {
      const UInt32 v = *data++;
      h ^= MY_MUL_32_64(v, Z7_XXH_PRIME64_5);
      h = Z7_ROTL64(h, 11);
      h = MY_MUL64(h, Z7_XXH_PRIME64_1);
      cnt--;
    }
    // XXH64_avalanche(h):
    h ^= h >> 33;  h = MY_MUL64(h, Z7_XXH_PRIME64_2);
    h ^= h >> 29;  h = MY_MUL64(h, Z7_XXH_PRIME64_3);
    h ^= h >> 32;
    return h;
  }
}


void Xxh64_Init(CXxh64 *p)
{
  Xxh64State_Init(&p->state);
  p->count = 0;
  p->buf64[0] = 0;
  p->buf64[1] = 0;
  p->buf64[2] = 0;
  p->buf64[3] = 0;
}

void Xxh64_Update(CXxh64 *p, const void *_data, size_t size)
{
  const Byte *data = (const Byte *)_data;
  unsigned cnt;
  if (size == 0)
    return;
  cnt = (unsigned)p->count;
  p->count += size;
  
  if (cnt &= 31)
  {
    unsigned rem = 32 - cnt;
    Byte *dest = (Byte *)p->buf64 + cnt;
    if (rem > size)
      rem = (unsigned)size;
    size -= rem;
    cnt += rem;
    // memcpy((Byte *)p->buf64 + cnt, data, rem);
    do
      *dest++ = *data++;
    while (--rem);
    if (cnt != 32)
      return;
#ifdef Z7_XXH64_USE_ALIGNED
    Xxh64State_UpdateBlocks_Aligned
#else
    Xxh64State_UpdateBlocks_Unaligned_Select
#endif
      (&p->state, p->buf64, &p->buf64[4]);
  }

  if (size &= ~(size_t)31)
  {
#ifdef Z7_XXH64_USE_ALIGNED
    if (((unsigned)(ptrdiff_t)data & 7) == 0)
      Xxh64State_UpdateBlocks_Aligned(&p->state, data, data + size);
    else
#endif
      Xxh64State_UpdateBlocks_Unaligned_Select(&p->state, data, data + size);
    data += size;
  }
  
  cnt = (unsigned)p->count & 31;
  if (cnt)
  {
    // memcpy(p->buf64, data, cnt);
    Byte *dest = (Byte *)p->buf64;
    do
      *dest++ = *data++;
    while (--cnt);
  }
}

/* ================ unit: C/Xz.c ================ */
/* Xz.c - Xz
2024-03-01 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

const Byte XZ_SIG[XZ_SIG_SIZE] = { 0xFD, '7', 'z', 'X', 'Z', 0 };
/* const Byte XZ_FOOTER_SIG[XZ_FOOTER_SIG_SIZE] = { 'Y', 'Z' }; */

unsigned Xz_WriteVarInt(Byte *buf, UInt64 v)
{
  unsigned i = 0;
  do
  {
    buf[i++] = (Byte)((v & 0x7F) | 0x80);
    v >>= 7;
  }
  while (v != 0);
  buf[(size_t)i - 1] &= 0x7F;
  return i;
}

void Xz_Construct(CXzStream *p)
{
  p->numBlocks = 0;
  p->blocks = NULL;
  p->flags = 0;
}

void Xz_Free(CXzStream *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->blocks);
  p->numBlocks = 0;
  p->blocks = NULL;
}

unsigned XzFlags_GetCheckSize(CXzStreamFlags f)
{
  unsigned t = XzFlags_GetCheckType(f);
  return (t == 0) ? 0 : ((unsigned)4 << ((t - 1) / 3));
}

void XzCheck_Init(CXzCheck *p, unsigned mode)
{
  p->mode = mode;
  switch (mode)
  {
    case XZ_CHECK_CRC32: p->crc = CRC_INIT_VAL; break;
    case XZ_CHECK_CRC64: p->crc64 = CRC64_INIT_VAL; break;
    case XZ_CHECK_SHA256: Sha256_Init(&p->sha); break;
    default: break;
  }
}

void XzCheck_Update(CXzCheck *p, const void *data, size_t size)
{
  switch (p->mode)
  {
    case XZ_CHECK_CRC32: p->crc = CrcUpdate(p->crc, data, size); break;
    case XZ_CHECK_CRC64: p->crc64 = Crc64Update(p->crc64, data, size); break;
    case XZ_CHECK_SHA256: Sha256_Update(&p->sha, (const Byte *)data, size); break;
    default: break;
  }
}

int XzCheck_Final(CXzCheck *p, Byte *digest)
{
  switch (p->mode)
  {
    case XZ_CHECK_CRC32:
      SetUi32(digest, CRC_GET_DIGEST(p->crc))
      break;
    case XZ_CHECK_CRC64:
    {
      int i;
      UInt64 v = CRC64_GET_DIGEST(p->crc64);
      for (i = 0; i < 8; i++, v >>= 8)
        digest[i] = (Byte)(v & 0xFF);
      break;
    }
    case XZ_CHECK_SHA256:
      Sha256_Final(&p->sha, digest);
      break;
    default:
      return 0;
  }
  return 1;
}

/* ================ unit: C/XzCrc64.c ================ */
/* XzCrc64.c -- CRC64 calculation
2023-12-08 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define kCrc64Poly UINT64_CONST(0xC96C5795D7870F42)

// for debug only : define Z7_CRC64_DEBUG_BE to test big-endian code in little-endian cpu
// #define Z7_CRC64_DEBUG_BE
#ifdef Z7_CRC64_DEBUG_BE
#undef MY_CPU_LE
#define MY_CPU_BE
#endif

#ifdef Z7_CRC64_NUM_TABLES
  #define Z7_CRC64_NUM_TABLES_USE  Z7_CRC64_NUM_TABLES
#else
  #define Z7_CRC64_NUM_TABLES_USE  12
#endif

#if Z7_CRC64_NUM_TABLES_USE < 1
  #error Stop_Compiling_Bad_Z7_CRC_NUM_TABLES
#endif


#if Z7_CRC64_NUM_TABLES_USE != 1

#ifndef MY_CPU_BE
  #define FUNC_NAME_LE_2(s)   XzCrc64UpdateT ## s
  #define FUNC_NAME_LE_1(s)   FUNC_NAME_LE_2(s)
  #define FUNC_NAME_LE        FUNC_NAME_LE_1(Z7_CRC64_NUM_TABLES_USE)
  UInt64 Z7_FASTCALL FUNC_NAME_LE (UInt64 v, const void *data, size_t size, const UInt64 *table);
#endif
#ifndef MY_CPU_LE
  #define FUNC_NAME_BE_2(s)   XzCrc64UpdateBeT ## s
  #define FUNC_NAME_BE_1(s)   FUNC_NAME_BE_2(s)
  #define FUNC_NAME_BE        FUNC_NAME_BE_1(Z7_CRC64_NUM_TABLES_USE)
  UInt64 Z7_FASTCALL FUNC_NAME_BE (UInt64 v, const void *data, size_t size, const UInt64 *table);
#endif

#if defined(MY_CPU_LE)
  #define FUNC_REF  FUNC_NAME_LE
#elif defined(MY_CPU_BE)
  #define FUNC_REF  FUNC_NAME_BE
#else
  #define FUNC_REF  g_Crc64Update
  static UInt64 (Z7_FASTCALL *FUNC_REF)(UInt64 v, const void *data, size_t size, const UInt64 *table);
#endif

#endif


MY_ALIGN(64)
static UInt64 g_Crc64Table[256 * Z7_CRC64_NUM_TABLES_USE];


UInt64 Z7_FASTCALL Crc64Update(UInt64 v, const void *data, size_t size)
{
#if Z7_CRC64_NUM_TABLES_USE == 1
  #define CRC64_UPDATE_BYTE_2(crc, b)  (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))
  const UInt64 *table = g_Crc64Table;
  const Byte *p = (const Byte *)data;
  const Byte *lim = p + size;
  for (; p != lim; p++)
    v = CRC64_UPDATE_BYTE_2(v, *p);
  return v;
  #undef CRC64_UPDATE_BYTE_2
#else
  return FUNC_REF (v, data, size, g_Crc64Table);
#endif
}


Z7_NO_INLINE
void Z7_FASTCALL Crc64GenerateTable(void)
{
  unsigned i;
  for (i = 0; i < 256; i++)
  {
    UInt64 r = i;
    unsigned j;
    for (j = 0; j < 8; j++)
      r = (r >> 1) ^ (kCrc64Poly & ((UInt64)0 - (r & 1)));
    g_Crc64Table[i] = r;
  }

#if Z7_CRC64_NUM_TABLES_USE != 1
#if 1 || 1 && defined(MY_CPU_X86) // low register count
  for (i = 0; i < 256 * (Z7_CRC64_NUM_TABLES_USE - 1); i++)
  {
    const UInt64 r0 = g_Crc64Table[(size_t)i];
    g_Crc64Table[(size_t)i + 256] = g_Crc64Table[(Byte)r0] ^ (r0 >> 8);
  }
#else
  for (i = 0; i < 256 * (Z7_CRC64_NUM_TABLES_USE - 1); i += 2)
  {
    UInt64 r0 = g_Crc64Table[(size_t)(i)    ];
    UInt64 r1 = g_Crc64Table[(size_t)(i) + 1];
    r0 = g_Crc64Table[(Byte)r0] ^ (r0 >> 8);
    r1 = g_Crc64Table[(Byte)r1] ^ (r1 >> 8);
    g_Crc64Table[(size_t)i + 256    ] = r0;
    g_Crc64Table[(size_t)i + 256 + 1] = r1;
  }
#endif

#ifndef MY_CPU_LE
  {
#ifndef MY_CPU_BE
    UInt32 k = 1;
    if (*(const Byte *)&k == 1)
      FUNC_REF = FUNC_NAME_LE;
    else
#endif
    {
#ifndef MY_CPU_BE
      FUNC_REF = FUNC_NAME_BE;
#endif
      for (i = 0; i < 256 * Z7_CRC64_NUM_TABLES_USE; i++)
      {
        const UInt64 x = g_Crc64Table[i];
        g_Crc64Table[i] = Z7_BSWAP64(x);
      }
    }
  }
#endif // ndef MY_CPU_LE
#endif // Z7_CRC64_NUM_TABLES_USE != 1
}

#undef kCrc64Poly
#undef Z7_CRC64_NUM_TABLES_USE
#undef FUNC_REF
#undef FUNC_NAME_LE_2
#undef FUNC_NAME_LE_1
#undef FUNC_NAME_LE
#undef FUNC_NAME_BE_2
#undef FUNC_NAME_BE_1
#undef FUNC_NAME_BE

/* ================ unit: C/XzCrc64Opt.c ================ */
/* XzCrc64Opt.c -- CRC64 calculation (optimized functions)
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#if !defined(Z7_CRC64_NUM_TABLES) || Z7_CRC64_NUM_TABLES > 1

// for debug only : define Z7_CRC64_DEBUG_BE to test big-endian code in little-endian cpu
// #define Z7_CRC64_DEBUG_BE
#ifdef Z7_CRC64_DEBUG_BE
#undef MY_CPU_LE
#define MY_CPU_BE
#endif

#if defined(MY_CPU_64BIT)
#define Z7_CRC64_USE_64BIT
#endif

// the value Z7_CRC64_NUM_TABLES_USE must be defined to same value as in XzCrc64.c
#ifdef Z7_CRC64_NUM_TABLES
#define Z7_CRC64_NUM_TABLES_USE  Z7_CRC64_NUM_TABLES
#else
#define Z7_CRC64_NUM_TABLES_USE  12
#endif

#if Z7_CRC64_NUM_TABLES_USE % 4 || \
    Z7_CRC64_NUM_TABLES_USE < 4 || \
    Z7_CRC64_NUM_TABLES_USE > 4 * 4
  #error Stop_Compiling_Bad_CRC64_NUM_TABLES
#endif


#ifndef MY_CPU_BE

#define CRC64_UPDATE_BYTE_2(crc, b)  (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

#if defined(Z7_CRC64_USE_64BIT) && (Z7_CRC64_NUM_TABLES_USE % 8 == 0)

#define Q64LE(n, d) \
    ( (table + ((n) * 8 + 7) * 0x100)[((d)         ) & 0xFF] \
    ^ (table + ((n) * 8 + 6) * 0x100)[((d) >> 1 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 5) * 0x100)[((d) >> 2 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 4) * 0x100)[((d) >> 3 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 3) * 0x100)[((d) >> 4 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 2) * 0x100)[((d) >> 5 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 1) * 0x100)[((d) >> 6 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 0) * 0x100)[((d) >> 7 * 8)] )

#define R64(a)  *((const UInt64 *)(const void *)p + (a))

#else

#define Q32LE(n, d) \
    ( (table + ((n) * 4 + 3) * 0x100)[((d)         ) & 0xFF] \
    ^ (table + ((n) * 4 + 2) * 0x100)[((d) >> 1 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 1) * 0x100)[((d) >> 2 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 0) * 0x100)[((d) >> 3 * 8)] )

#define R32(a)  *((const UInt32 *)(const void *)p + (a))

#endif


#define CRC64_FUNC_PRE_LE2(step) \
UInt64 Z7_FASTCALL XzCrc64UpdateT ## step (UInt64 v, const void *data, size_t size, const UInt64 *table)

#define CRC64_FUNC_PRE_LE(step)   \
        CRC64_FUNC_PRE_LE2(step); \
        CRC64_FUNC_PRE_LE2(step)

CRC64_FUNC_PRE_LE(Z7_CRC64_NUM_TABLES_USE)
{
  const Byte *p = (const Byte *)data;
  const Byte *lim;
  for (; size && ((unsigned)(ptrdiff_t)p & (7 - (Z7_CRC64_NUM_TABLES_USE & 4))) != 0; size--, p++)
    v = CRC64_UPDATE_BYTE_2(v, *p);
  lim = p + size;
  if (size >= Z7_CRC64_NUM_TABLES_USE)
  {
    lim -= Z7_CRC64_NUM_TABLES_USE;
    do
    {
#if Z7_CRC64_NUM_TABLES_USE == 4
      const UInt32 d = (UInt32)v ^ R32(0);
      v = (v >> 32) ^ Q32LE(0, d);
#elif Z7_CRC64_NUM_TABLES_USE == 8
#ifdef Z7_CRC64_USE_64BIT
      v ^= R64(0);
      v = Q64LE(0, v);
#else
      UInt32 v0, v1;
      v0 = (UInt32)v         ^ R32(0);
      v1 = (UInt32)(v >> 32) ^ R32(1);
      v = Q32LE(1, v0) ^ Q32LE(0, v1);
#endif
#elif Z7_CRC64_NUM_TABLES_USE == 12
      UInt32 w;
      UInt32 v0, v1;
      v0 = (UInt32)v         ^ R32(0);
      v1 = (UInt32)(v >> 32) ^ R32(1);
      w = R32(2);
      v = Q32LE(0, w);
      v ^= Q32LE(2, v0) ^ Q32LE(1, v1);
#elif Z7_CRC64_NUM_TABLES_USE == 16
#ifdef Z7_CRC64_USE_64BIT
      UInt64 w;
      UInt64 x;
      w  = R64(1);      x = Q64LE(0, w);
      v ^= R64(0);  v = x ^ Q64LE(1, v);
#else
      UInt32 v0, v1;
      UInt32 r0, r1;
      v0 = (UInt32)v         ^ R32(0);
      v1 = (UInt32)(v >> 32) ^ R32(1);
      r0 =                     R32(2);
      r1 =                     R32(3);
      v  = Q32LE(1, r0) ^ Q32LE(0, r1);
      v ^= Q32LE(3, v0) ^ Q32LE(2, v1);
#endif
#else
#error Stop_Compiling_Bad_CRC64_NUM_TABLES
#endif
      p += Z7_CRC64_NUM_TABLES_USE;
    }
    while (p <= lim);
    lim += Z7_CRC64_NUM_TABLES_USE;
  }
  for (; p < lim; p++)
    v = CRC64_UPDATE_BYTE_2(v, *p);
  return v;
}

#undef CRC64_UPDATE_BYTE_2
#undef R32
#undef R64
#undef Q32LE
#undef Q64LE
#undef CRC64_FUNC_PRE_LE
#undef CRC64_FUNC_PRE_LE2

#endif




#ifndef MY_CPU_LE

#define CRC64_UPDATE_BYTE_2_BE(crc, b)  (table[((crc) >> 56) ^ (b)] ^ ((crc) << 8))

#if defined(Z7_CRC64_USE_64BIT) && (Z7_CRC64_NUM_TABLES_USE % 8 == 0)

#define Q64BE(n, d) \
    ( (table + ((n) * 8 + 0) * 0x100)[(Byte)(d)] \
    ^ (table + ((n) * 8 + 1) * 0x100)[((d) >> 1 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 2) * 0x100)[((d) >> 2 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 3) * 0x100)[((d) >> 3 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 4) * 0x100)[((d) >> 4 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 5) * 0x100)[((d) >> 5 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 6) * 0x100)[((d) >> 6 * 8) & 0xFF] \
    ^ (table + ((n) * 8 + 7) * 0x100)[((d) >> 7 * 8)] )

#ifdef Z7_CRC64_DEBUG_BE
  #define R64BE(a)  GetBe64a((const UInt64 *)(const void *)p + (a))
#else
  #define R64BE(a)         *((const UInt64 *)(const void *)p + (a))
#endif

#else

#define Q32BE(n, d) \
    ( (table + ((n) * 4 + 0) * 0x100)[(Byte)(d)] \
    ^ (table + ((n) * 4 + 1) * 0x100)[((d) >> 1 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 2) * 0x100)[((d) >> 2 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 3) * 0x100)[((d) >> 3 * 8)] )

#ifdef Z7_CRC64_DEBUG_BE
  #define R32BE(a)  GetBe32a((const UInt32 *)(const void *)p + (a))
#else
  #define R32BE(a)         *((const UInt32 *)(const void *)p + (a))
#endif

#endif

#define CRC64_FUNC_PRE_BE2(step) \
UInt64 Z7_FASTCALL XzCrc64UpdateBeT ## step (UInt64 v, const void *data, size_t size, const UInt64 *table)

#define CRC64_FUNC_PRE_BE(step)   \
        CRC64_FUNC_PRE_BE2(step); \
        CRC64_FUNC_PRE_BE2(step)

CRC64_FUNC_PRE_BE(Z7_CRC64_NUM_TABLES_USE)
{
  const Byte *p = (const Byte *)data;
  const Byte *lim;
  v = Z7_BSWAP64(v);
  for (; size && ((unsigned)(ptrdiff_t)p & (7 - (Z7_CRC64_NUM_TABLES_USE & 4))) != 0; size--, p++)
    v = CRC64_UPDATE_BYTE_2_BE(v, *p);
  lim = p + size;
  if (size >= Z7_CRC64_NUM_TABLES_USE)
  {
    lim -= Z7_CRC64_NUM_TABLES_USE;
    do
    {
#if   Z7_CRC64_NUM_TABLES_USE == 4
      const UInt32 d = (UInt32)(v >> 32) ^ R32BE(0);
      v = (v << 32) ^ Q32BE(0, d);
#elif Z7_CRC64_NUM_TABLES_USE == 12
      const UInt32 d1 = (UInt32)(v >> 32) ^ R32BE(0);
      const UInt32 d0 = (UInt32)(v      ) ^ R32BE(1);
      const UInt32 w =                      R32BE(2);
      v  = Q32BE(0, w);
      v ^= Q32BE(2, d1) ^ Q32BE(1, d0);

#elif Z7_CRC64_NUM_TABLES_USE == 8
  #ifdef Z7_CRC64_USE_64BIT
      v ^= R64BE(0);
      v  = Q64BE(0, v);
  #else
      const UInt32 d1 = (UInt32)(v >> 32) ^ R32BE(0);
      const UInt32 d0 = (UInt32)(v      ) ^ R32BE(1);
      v = Q32BE(1, d1) ^ Q32BE(0, d0);
  #endif
#elif Z7_CRC64_NUM_TABLES_USE == 16
  #ifdef Z7_CRC64_USE_64BIT
      const UInt64 w = R64BE(1);
      v ^= R64BE(0);
      v  = Q64BE(0, w) ^ Q64BE(1, v);
  #else
      const UInt32 d1 = (UInt32)(v >> 32) ^ R32BE(0);
      const UInt32 d0 = (UInt32)(v      ) ^ R32BE(1);
      const UInt32 w1 =                     R32BE(2);
      const UInt32 w0 =                     R32BE(3);
      v  = Q32BE(1, w1) ^ Q32BE(0, w0);
      v ^= Q32BE(3, d1) ^ Q32BE(2, d0);
  #endif
#else
#error Stop_Compiling_Bad_CRC64_NUM_TABLES
#endif
      p += Z7_CRC64_NUM_TABLES_USE;
    }
    while (p <= lim);
    lim += Z7_CRC64_NUM_TABLES_USE;
  }
  for (; p < lim; p++)
    v = CRC64_UPDATE_BYTE_2_BE(v, *p);
  return Z7_BSWAP64(v);
}

#undef CRC64_UPDATE_BYTE_2_BE
#undef R32BE
#undef R64BE
#undef Q32BE
#undef Q64BE
#undef CRC64_FUNC_PRE_BE
#undef CRC64_FUNC_PRE_BE2

#endif
#undef Z7_CRC64_NUM_TABLES_USE
#endif

/* ================ unit: C/XzDec.c ================ */
/* XzDec.c -- Xz Decode
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// #include <stdio.h>

// #define XZ_DUMP

/* #define XZ_DUMP */

#ifdef XZ_DUMP
#include <stdio.h>
#endif

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#define PRF_STR(s) PRF(printf("\n" s "\n"))
#define PRF_STR_INT(s, d) PRF(printf("\n" s " %d\n", (unsigned)d))

#include <stdlib.h>
#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// #define USE_SUBBLOCK

#ifdef USE_SUBBLOCK
#include "Bcj3Dec.c"
#include "SbDec.h"
#endif

// amalgamation: header emitted in prologue

#define XZ_CHECK_SIZE_MAX 64

#define CODER_BUF_SIZE ((size_t)1 << 17)

unsigned Xz_ReadVarInt(const Byte *p, size_t maxSize, UInt64 *value)
{
  unsigned i, limit;
  *value = 0;
  limit = (maxSize > 9) ? 9 : (unsigned)maxSize;

  for (i = 0; i < limit;)
  {
    const unsigned b = p[i];
    *value |= (UInt64)(b & 0x7F) << (7 * i++);
    if ((b & 0x80) == 0)
      return (b == 0 && i != 1) ? 0 : i;
  }
  return 0;
}


/* ---------- XzBcFilterState ---------- */

#define BRA_BUF_SIZE (1 << 14)

typedef struct
{
  size_t bufPos;
  size_t bufConv;
  size_t bufTotal;
  Byte *buf;  // must be aligned for 4 bytes
  Xz_Func_BcFilterStateBase_Filter filter_func;
  // int encodeMode;
  CXzBcFilterStateBase base;
  // Byte buf[BRA_BUF_SIZE];
} CXzBcFilterState;


static void XzBcFilterState_Free(void *pp, ISzAllocPtr alloc)
{
  if (pp)
  {
    CXzBcFilterState *p = ((CXzBcFilterState *)pp);
    ISzAlloc_Free(alloc, p->buf);
    ISzAlloc_Free(alloc, pp);
  }
}


static SRes XzBcFilterState_SetProps(void *pp, const Byte *props, size_t propSize, ISzAllocPtr alloc)
{
  CXzBcFilterStateBase *p = &((CXzBcFilterState *)pp)->base;
  UNUSED_VAR(alloc)
  p->ip = 0;
  if (p->methodId == XZ_ID_Delta)
  {
    if (propSize != 1)
      return SZ_ERROR_UNSUPPORTED;
    p->delta = (UInt32)props[0] + 1;
  }
  else
  {
    if (propSize == 4)
    {
      const UInt32 v = GetUi32(props);
      switch (p->methodId)
      {
        case XZ_ID_PPC:
        case XZ_ID_ARM:
        case XZ_ID_SPARC:
        case XZ_ID_ARM64:
          if (v & 3)
            return SZ_ERROR_UNSUPPORTED;
          break;
        case XZ_ID_ARMT:
        case XZ_ID_RISCV:
          if (v & 1)
            return SZ_ERROR_UNSUPPORTED;
          break;
        case XZ_ID_IA64:
          if (v & 0xf)
            return SZ_ERROR_UNSUPPORTED;
          break;
        default: break;
      }
      p->ip = v;
    }
    else if (propSize != 0)
      return SZ_ERROR_UNSUPPORTED;
  }
  return SZ_OK;
}


static void XzBcFilterState_Init(void *pp)
{
  CXzBcFilterState *p = ((CXzBcFilterState *)pp);
  p->bufPos = p->bufConv = p->bufTotal = 0;
  p->base.X86_State = Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL;
  if (p->base.methodId == XZ_ID_Delta)
    Delta_Init(p->base.delta_State);
}


static const z7_Func_BranchConv g_Funcs_BranchConv_RISC_Dec[] =
{
  Z7_BRANCH_CONV_DEC_2 (BranchConv_PPC),
  Z7_BRANCH_CONV_DEC_2 (BranchConv_IA64),
  Z7_BRANCH_CONV_DEC_2 (BranchConv_ARM),
  Z7_BRANCH_CONV_DEC_2 (BranchConv_ARMT),
  Z7_BRANCH_CONV_DEC_2 (BranchConv_SPARC),
  Z7_BRANCH_CONV_DEC_2 (BranchConv_ARM64),
  Z7_BRANCH_CONV_DEC_2 (BranchConv_RISCV)
};

static SizeT XzBcFilterStateBase_Filter_Dec(CXzBcFilterStateBase *p, Byte *data, SizeT size)
{
  switch (p->methodId)
  {
    case XZ_ID_Delta:
      Delta_Decode(p->delta_State, p->delta, data, size);
      break;
    case XZ_ID_X86:
      size = (SizeT)(z7_BranchConvSt_X86_Dec(data, size, p->ip, &p->X86_State) - data);
      break;
    default:
      if (p->methodId >= XZ_ID_PPC)
      {
        const UInt32 i = p->methodId - XZ_ID_PPC;
        if (i < Z7_ARRAY_SIZE(g_Funcs_BranchConv_RISC_Dec))
          size = (SizeT)(g_Funcs_BranchConv_RISC_Dec[i](data, size, p->ip) - data);
      }
      break;
  }
  p->ip += (UInt32)size;
  return size;
}


static SizeT XzBcFilterState_Filter(void *pp, Byte *data, SizeT size)
{
  CXzBcFilterState *p = ((CXzBcFilterState *)pp);
  return p->filter_func(&p->base, data, size);
}


static SRes XzBcFilterState_Code2(void *pp,
    Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, int srcWasFinished,
    ECoderFinishMode finishMode,
    // int *wasFinished
    ECoderStatus *status)
{
  CXzBcFilterState *p = ((CXzBcFilterState *)pp);
  SizeT destRem = *destLen;
  SizeT srcRem = *srcLen;
  UNUSED_VAR(finishMode)

  *destLen = 0;
  *srcLen = 0;
  // *wasFinished = False;
  *status = CODER_STATUS_NOT_FINISHED;
  
  while (destRem != 0)
  {
    {
      size_t size = p->bufConv - p->bufPos;
      if (size)
      {
        if (size > destRem)
          size = destRem;
        memcpy(dest, p->buf + p->bufPos, size);
        p->bufPos += size;
        *destLen += size;
        dest += size;
        destRem -= size;
        continue;
      }
    }
    
    p->bufTotal -= p->bufPos;
    memmove(p->buf, p->buf + p->bufPos, p->bufTotal);
    p->bufPos = 0;
    p->bufConv = 0;
    {
      size_t size = BRA_BUF_SIZE - p->bufTotal;
      if (size > srcRem)
        size = srcRem;
      memcpy(p->buf + p->bufTotal, src, size);
      *srcLen += size;
      src += size;
      srcRem -= size;
      p->bufTotal += size;
    }
    if (p->bufTotal == 0)
      break;
    
    p->bufConv = p->filter_func(&p->base, p->buf, p->bufTotal);

    if (p->bufConv == 0)
    {
      if (!srcWasFinished)
        break;
      p->bufConv = p->bufTotal;
    }
  }

  if (p->bufTotal == p->bufPos && srcRem == 0 && srcWasFinished)
  {
    *status = CODER_STATUS_FINISHED_WITH_MARK;
    // *wasFinished = 1;
  }

  return SZ_OK;
}


#define XZ_IS_SUPPORTED_FILTER_ID(id) \
    ((id) >= XZ_ID_Delta && (id) <= XZ_ID_RISCV)
     
SRes Xz_StateCoder_Bc_SetFromMethod_Func(IStateCoder *p, UInt64 id,
    Xz_Func_BcFilterStateBase_Filter func, ISzAllocPtr alloc)
{
  CXzBcFilterState *decoder;
  if (!XZ_IS_SUPPORTED_FILTER_ID(id))
    return SZ_ERROR_UNSUPPORTED;
  decoder = (CXzBcFilterState *)p->p;
  if (!decoder)
  {
    decoder = (CXzBcFilterState *)ISzAlloc_Alloc(alloc, sizeof(CXzBcFilterState));
    if (!decoder)
      return SZ_ERROR_MEM;
    decoder->buf = (Byte *)ISzAlloc_Alloc(alloc, BRA_BUF_SIZE);
    if (!decoder->buf)
    {
      ISzAlloc_Free(alloc, decoder);
      return SZ_ERROR_MEM;
    }
    p->p = decoder;
    p->Free     = XzBcFilterState_Free;
    p->SetProps = XzBcFilterState_SetProps;
    p->Init     = XzBcFilterState_Init;
    p->Code2    = XzBcFilterState_Code2;
    p->Filter   = XzBcFilterState_Filter;
    decoder->filter_func = func;
  }
  decoder->base.methodId = (UInt32)id;
  // decoder->encodeMode = encodeMode;
  return SZ_OK;
}



/* ---------- SbState ---------- */

#ifdef USE_SUBBLOCK

static void SbState_Free(void *pp, ISzAllocPtr alloc)
{
  CSbDec *p = (CSbDec *)pp;
  SbDec_Free(p);
  ISzAlloc_Free(alloc, pp);
}

static SRes SbState_SetProps(void *pp, const Byte *props, size_t propSize, ISzAllocPtr alloc)
{
  UNUSED_VAR(pp)
  UNUSED_VAR(props)
  UNUSED_VAR(alloc)
  return (propSize == 0) ? SZ_OK : SZ_ERROR_UNSUPPORTED;
}

static void SbState_Init(void *pp)
{
  SbDec_Init((CSbDec *)pp);
}

static SRes SbState_Code2(void *pp, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    int srcWasFinished, ECoderFinishMode finishMode,
    // int *wasFinished
    ECoderStatus *status)
{
  CSbDec *p = (CSbDec *)pp;
  SRes res;
  UNUSED_VAR(srcWasFinished)
  p->dest = dest;
  p->destLen = *destLen;
  p->src = src;
  p->srcLen = *srcLen;
  p->finish = finishMode; /* change it */
  res = SbDec_Decode((CSbDec *)pp);
  *destLen -= p->destLen;
  *srcLen -= p->srcLen;
  // *wasFinished = (*destLen == 0 && *srcLen == 0); /* change it */
  *status = (*destLen == 0 && *srcLen == 0) ?
      CODER_STATUS_FINISHED_WITH_MARK :
      CODER_STATUS_NOT_FINISHED;
  return res;
}

static SRes SbState_SetFromMethod(IStateCoder *p, ISzAllocPtr alloc)
{
  CSbDec *decoder = (CSbDec *)p->p;
  if (!decoder)
  {
    decoder = (CSbDec *)ISzAlloc_Alloc(alloc, sizeof(CSbDec));
    if (!decoder)
      return SZ_ERROR_MEM;
    p->p = decoder;
    p->Free = SbState_Free;
    p->SetProps = SbState_SetProps;
    p->Init = SbState_Init;
    p->Code2 = SbState_Code2;
    p->Filter = NULL;
  }
  SbDec_Construct(decoder);
  SbDec_SetAlloc(decoder, alloc);
  return SZ_OK;
}

#endif



/* ---------- Lzma2 ---------- */

typedef struct
{
  CLzma2Dec decoder;
  BoolInt outBufMode;
} CLzma2Dec_Spec;


static void Lzma2State_Free(void *pp, ISzAllocPtr alloc)
{
  CLzma2Dec_Spec *p = (CLzma2Dec_Spec *)pp;
  if (p->outBufMode)
    Lzma2Dec_FreeProbs(&p->decoder, alloc);
  else
    Lzma2Dec_Free(&p->decoder, alloc);
  ISzAlloc_Free(alloc, pp);
}

static SRes Lzma2State_SetProps(void *pp, const Byte *props, size_t propSize, ISzAllocPtr alloc)
{
  if (propSize != 1)
    return SZ_ERROR_UNSUPPORTED;
  {
    CLzma2Dec_Spec *p = (CLzma2Dec_Spec *)pp;
    if (p->outBufMode)
      return Lzma2Dec_AllocateProbs(&p->decoder, props[0], alloc);
    else
      return Lzma2Dec_Allocate(&p->decoder, props[0], alloc);
  }
}

static void Lzma2State_Init(void *pp)
{
  Lzma2Dec_Init(&((CLzma2Dec_Spec *)pp)->decoder);
}


/*
  if (outBufMode), then (dest) is not used. Use NULL.
         Data is unpacked to (spec->decoder.decoder.dic) output buffer.
*/

static SRes Lzma2State_Code2(void *pp, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    int srcWasFinished, ECoderFinishMode finishMode,
    // int *wasFinished,
    ECoderStatus *status)
{
  CLzma2Dec_Spec *spec = (CLzma2Dec_Spec *)pp;
  ELzmaStatus status2;
  /* ELzmaFinishMode fm = (finishMode == LZMA_FINISH_ANY) ? LZMA_FINISH_ANY : LZMA_FINISH_END; */
  SRes res;
  UNUSED_VAR(srcWasFinished)
  if (spec->outBufMode)
  {
    SizeT dicPos = spec->decoder.decoder.dicPos;
    SizeT dicLimit = dicPos + *destLen;
    res = Lzma2Dec_DecodeToDic(&spec->decoder, dicLimit, src, srcLen, (ELzmaFinishMode)finishMode, &status2);
    *destLen = spec->decoder.decoder.dicPos - dicPos;
  }
  else
    res = Lzma2Dec_DecodeToBuf(&spec->decoder, dest, destLen, src, srcLen, (ELzmaFinishMode)finishMode, &status2);
  // *wasFinished = (status2 == LZMA_STATUS_FINISHED_WITH_MARK);
  // ECoderStatus values are identical to ELzmaStatus values of LZMA2 decoder
  *status = (ECoderStatus)status2;
  return res;
}


static SRes Lzma2State_SetFromMethod(IStateCoder *p, Byte *outBuf, size_t outBufSize, ISzAllocPtr alloc)
{
  CLzma2Dec_Spec *spec = (CLzma2Dec_Spec *)p->p;
  if (!spec)
  {
    spec = (CLzma2Dec_Spec *)ISzAlloc_Alloc(alloc, sizeof(CLzma2Dec_Spec));
    if (!spec)
      return SZ_ERROR_MEM;
    p->p = spec;
    p->Free = Lzma2State_Free;
    p->SetProps = Lzma2State_SetProps;
    p->Init = Lzma2State_Init;
    p->Code2 = Lzma2State_Code2;
    p->Filter = NULL;
    Lzma2Dec_CONSTRUCT(&spec->decoder)
  }
  spec->outBufMode = False;
  if (outBuf)
  {
    spec->outBufMode = True;
    spec->decoder.decoder.dic = outBuf;
    spec->decoder.decoder.dicBufSize = outBufSize;
  }
  return SZ_OK;
}


static SRes Lzma2State_ResetOutBuf(IStateCoder *p, Byte *outBuf, size_t outBufSize)
{
  CLzma2Dec_Spec *spec = (CLzma2Dec_Spec *)p->p;
  if ((spec->outBufMode && !outBuf) || (!spec->outBufMode && outBuf))
    return SZ_ERROR_FAIL;
  if (outBuf)
  {
    spec->decoder.decoder.dic = outBuf;
    spec->decoder.decoder.dicBufSize = outBufSize;
  }
  return SZ_OK;
}



static void MixCoder_Construct(CMixCoder *p, ISzAllocPtr alloc)
{
  unsigned i;
  p->alloc = alloc;
  p->buf = NULL;
  p->numCoders = 0;
  
  p->outBufSize = 0;
  p->outBuf = NULL;
  // p->SingleBufMode = False;

  for (i = 0; i < MIXCODER_NUM_FILTERS_MAX; i++)
    p->coders[i].p = NULL;
}


static void MixCoder_Free(CMixCoder *p)
{
  unsigned i;
  p->numCoders = 0;
  for (i = 0; i < MIXCODER_NUM_FILTERS_MAX; i++)
  {
    IStateCoder *sc = &p->coders[i];
    if (sc->p)
    {
      sc->Free(sc->p, p->alloc);
      sc->p = NULL;
    }
  }
  if (p->buf)
  {
    ISzAlloc_Free(p->alloc, p->buf);
    p->buf = NULL; /* 9.31: the BUG was fixed */
  }
}

static void MixCoder_Init(CMixCoder *p)
{
  unsigned i;
  for (i = 0; i < MIXCODER_NUM_FILTERS_MAX - 1; i++)
  {
    p->size[i] = 0;
    p->pos[i] = 0;
    p->finished[i] = 0;
  }
  for (i = 0; i < p->numCoders; i++)
  {
    IStateCoder *coder = &p->coders[i];
    coder->Init(coder->p);
    p->results[i] = SZ_OK;
  }
  p->outWritten = 0;
  p->wasFinished = False;
  p->res = SZ_OK;
  p->status = CODER_STATUS_NOT_SPECIFIED;
}


static SRes MixCoder_SetFromMethod(CMixCoder *p, unsigned coderIndex, UInt64 methodId, Byte *outBuf, size_t outBufSize)
{
  IStateCoder *sc = &p->coders[coderIndex];
  p->ids[coderIndex] = methodId;
  if (methodId == XZ_ID_LZMA2)
    return Lzma2State_SetFromMethod(sc, outBuf, outBufSize, p->alloc);
#ifdef USE_SUBBLOCK
  if (methodId == XZ_ID_Subblock)
    return SbState_SetFromMethod(sc, p->alloc);
#endif
  if (coderIndex == 0)
    return SZ_ERROR_UNSUPPORTED;
  return Xz_StateCoder_Bc_SetFromMethod_Func(sc, methodId,
      XzBcFilterStateBase_Filter_Dec, p->alloc);
}


static SRes MixCoder_ResetFromMethod(CMixCoder *p, unsigned coderIndex, UInt64 methodId, Byte *outBuf, size_t outBufSize)
{
  IStateCoder *sc = &p->coders[coderIndex];
  if (methodId == XZ_ID_LZMA2)
    return Lzma2State_ResetOutBuf(sc, outBuf, outBufSize);
  return SZ_ERROR_UNSUPPORTED;
}



/*
 if (destFinish) - then unpack data block is finished at (*destLen) position,
                   and we can return data that were not processed by filter

output (status) can be :
  CODER_STATUS_NOT_FINISHED
  CODER_STATUS_FINISHED_WITH_MARK
  CODER_STATUS_NEEDS_MORE_INPUT - not implemented still
*/

static SRes MixCoder_Code(CMixCoder *p,
    Byte *dest, SizeT *destLen, int destFinish,
    const Byte *src, SizeT *srcLen, int srcWasFinished,
    ECoderFinishMode finishMode)
{
  SizeT destLenOrig = *destLen;
  SizeT srcLenOrig = *srcLen;

  *destLen = 0;
  *srcLen = 0;

  if (p->wasFinished)
    return p->res;
  
  p->status = CODER_STATUS_NOT_FINISHED;

  // if (p->SingleBufMode)
  if (p->outBuf)
  {
    SRes res;
    SizeT destLen2, srcLen2;
    int wasFinished;
    
    PRF_STR("------- MixCoder Single ----------")
      
    srcLen2 = srcLenOrig;
    destLen2 = destLenOrig;
    
    {
      IStateCoder *coder = &p->coders[0];
      res = coder->Code2(coder->p, NULL, &destLen2, src, &srcLen2, srcWasFinished, finishMode,
          // &wasFinished,
          &p->status);
      wasFinished = (p->status == CODER_STATUS_FINISHED_WITH_MARK);
    }
    
    p->res = res;
    
    /*
    if (wasFinished)
      p->status = CODER_STATUS_FINISHED_WITH_MARK;
    else
    {
      if (res == SZ_OK)
        if (destLen2 != destLenOrig)
          p->status = CODER_STATUS_NEEDS_MORE_INPUT;
    }
    */

    
    *srcLen = srcLen2;
    src += srcLen2;
    p->outWritten += destLen2;
    
    if (res != SZ_OK || srcWasFinished || wasFinished)
      p->wasFinished = True;
    
    if (p->numCoders == 1)
      *destLen = destLen2;
    else if (p->wasFinished)
    {
      unsigned i;
      size_t processed = p->outWritten;
      
      for (i = 1; i < p->numCoders; i++)
      {
        IStateCoder *coder = &p->coders[i];
        processed = coder->Filter(coder->p, p->outBuf, processed);
        if (wasFinished || (destFinish && p->outWritten == destLenOrig))
          processed = p->outWritten;
        PRF_STR_INT("filter", i)
      }
      *destLen = processed;
    }
    return res;
  }

  PRF_STR("standard mix")

  if (p->numCoders != 1)
  {
    if (!p->buf)
    {
      p->buf = (Byte *)ISzAlloc_Alloc(p->alloc, CODER_BUF_SIZE * (MIXCODER_NUM_FILTERS_MAX - 1));
      if (!p->buf)
        return SZ_ERROR_MEM;
    }
    
    finishMode = CODER_FINISH_ANY;
  }

  for (;;)
  {
    BoolInt processed = False;
    BoolInt allFinished = True;
    SRes resMain = SZ_OK;
    unsigned i;

    p->status = CODER_STATUS_NOT_FINISHED;
    /*
    if (p->numCoders == 1 && *destLen == destLenOrig && finishMode == LZMA_FINISH_ANY)
      break;
    */

    for (i = 0; i < p->numCoders; i++)
    {
      SRes res;
      IStateCoder *coder = &p->coders[i];
      Byte *dest2;
      SizeT destLen2, srcLen2; // destLen2_Orig;
      const Byte *src2;
      int srcFinished2;
      int encodingWasFinished;
      ECoderStatus status2;
      
      if (i == 0)
      {
        src2 = src;
        srcLen2 = srcLenOrig - *srcLen;
        srcFinished2 = srcWasFinished;
      }
      else
      {
        size_t k = i - 1;
        src2 = p->buf + (CODER_BUF_SIZE * k) + p->pos[k];
        srcLen2 = p->size[k] - p->pos[k];
        srcFinished2 = p->finished[k];
      }
      
      if (i == p->numCoders - 1)
      {
        dest2 = dest;
        destLen2 = destLenOrig - *destLen;
      }
      else
      {
        if (p->pos[i] != p->size[i])
          continue;
        dest2 = p->buf + (CODER_BUF_SIZE * i);
        destLen2 = CODER_BUF_SIZE;
      }
      
      // destLen2_Orig = destLen2;
      
      if (p->results[i] != SZ_OK)
      {
        if (resMain == SZ_OK)
          resMain = p->results[i];
        continue;
      }

      res = coder->Code2(coder->p,
          dest2, &destLen2,
          src2, &srcLen2, srcFinished2,
          finishMode,
          // &encodingWasFinished,
          &status2);

      if (res != SZ_OK)
      {
        p->results[i] = res;
        if (resMain == SZ_OK)
          resMain = res;
      }

      encodingWasFinished = (status2 == CODER_STATUS_FINISHED_WITH_MARK);
      
      if (!encodingWasFinished)
      {
        allFinished = False;
        if (p->numCoders == 1 && res == SZ_OK)
          p->status = status2;
      }

      if (i == 0)
      {
        *srcLen += srcLen2;
        src += srcLen2;
      }
      else
        p->pos[(size_t)i - 1] += srcLen2;

      if (i == p->numCoders - 1)
      {
        *destLen += destLen2;
        dest += destLen2;
      }
      else
      {
        p->size[i] = destLen2;
        p->pos[i] = 0;
        p->finished[i] = encodingWasFinished;
      }
      
      if (destLen2 != 0 || srcLen2 != 0)
        processed = True;
    }
    
    if (!processed)
    {
      if (allFinished)
        p->status = CODER_STATUS_FINISHED_WITH_MARK;
      return resMain;
    }
  }
}


SRes Xz_ParseHeader(CXzStreamFlags *p, const Byte *buf)
{
  *p = (CXzStreamFlags)GetBe16(buf + XZ_SIG_SIZE);
  if (CrcCalc(buf + XZ_SIG_SIZE, XZ_STREAM_FLAGS_SIZE) !=
      GetUi32(buf + XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE))
    return SZ_ERROR_NO_ARCHIVE;
  return XzFlags_IsSupported(*p) ? SZ_OK : SZ_ERROR_UNSUPPORTED;
}

static BoolInt Xz_CheckFooter(CXzStreamFlags flags, UInt64 indexSize, const Byte *buf)
{
  return indexSize == (((UInt64)GetUi32a(buf + 4) + 1) << 2)
      && GetUi32a(buf) == CrcCalc(buf + 4, 6)
      && flags == GetBe16a(buf + 8)
      && GetUi16a(buf + 10) == (XZ_FOOTER_SIG_0 | (XZ_FOOTER_SIG_1 << 8));
}

#define READ_VARINT_AND_CHECK(buf, pos, size, res) \
  { const unsigned s = Xz_ReadVarInt(buf + pos, size - pos, res); \
  if (s == 0) return SZ_ERROR_ARCHIVE; \
  pos += s; }


static BoolInt XzBlock_AreSupportedFilters(const CXzBlock *p)
{
  const unsigned numFilters = XzBlock_GetNumFilters(p) - 1;
  unsigned i;
  {
    const CXzFilter *f = &p->filters[numFilters];
    if (f->id != XZ_ID_LZMA2 || f->propsSize != 1 || f->props[0] > 40)
      return False;
  }

  for (i = 0; i < numFilters; i++)
  {
    const CXzFilter *f = &p->filters[i];
    if (f->id == XZ_ID_Delta)
    {
      if (f->propsSize != 1)
        return False;
    }
    else if (!XZ_IS_SUPPORTED_FILTER_ID(f->id)
        || (f->propsSize != 0 && f->propsSize != 4))
      return False;
  }
  return True;
}


SRes XzBlock_Parse(CXzBlock *p, const Byte *header)
{
  unsigned pos;
  unsigned numFilters, i;
  unsigned headerSize = (unsigned)header[0] << 2;

  /* (headerSize != 0) : another code checks */

  if (CrcCalc(header, headerSize) != GetUi32(header + headerSize))
    return SZ_ERROR_ARCHIVE;

  pos = 1;
  p->flags = header[pos++];

  p->packSize = (UInt64)(Int64)-1;
  if (XzBlock_HasPackSize(p))
  {
    READ_VARINT_AND_CHECK(header, pos, headerSize, &p->packSize)
    if (p->packSize == 0 || p->packSize + headerSize >= (UInt64)1 << 63)
      return SZ_ERROR_ARCHIVE;
  }

  p->unpackSize = (UInt64)(Int64)-1;
  if (XzBlock_HasUnpackSize(p))
  {
    READ_VARINT_AND_CHECK(header, pos, headerSize, &p->unpackSize)
  }

  numFilters = XzBlock_GetNumFilters(p);
  for (i = 0; i < numFilters; i++)
  {
    CXzFilter *filter = p->filters + i;
    UInt64 size;
    READ_VARINT_AND_CHECK(header, pos, headerSize, &filter->id)
    READ_VARINT_AND_CHECK(header, pos, headerSize, &size)
    if (size > headerSize - pos || size > XZ_FILTER_PROPS_SIZE_MAX)
      return SZ_ERROR_ARCHIVE;
    filter->propsSize = (UInt32)size;
    memcpy(filter->props, header + pos, (size_t)size);
    pos += (unsigned)size;

    #ifdef XZ_DUMP
    printf("\nf[%u] = %2X: ", i, (unsigned)filter->id);
    {
      unsigned i;
      for (i = 0; i < size; i++)
        printf(" %2X", filter->props[i]);
    }
    #endif
  }

  if (XzBlock_HasUnsupportedFlags(p))
    return SZ_ERROR_UNSUPPORTED;

  while (pos < headerSize)
    if (header[pos++] != 0)
      return SZ_ERROR_ARCHIVE;
  return SZ_OK;
}




static SRes XzDecMix_Init(CMixCoder *p, const CXzBlock *block, Byte *outBuf, size_t outBufSize)
{
  unsigned i;
  BoolInt needReInit = True;
  unsigned numFilters = XzBlock_GetNumFilters(block);

  if (numFilters == p->numCoders && ((p->outBuf && outBuf) || (!p->outBuf && !outBuf)))
  {
    needReInit = False;
    for (i = 0; i < numFilters; i++)
      if (p->ids[i] != block->filters[numFilters - 1 - i].id)
      {
        needReInit = True;
        break;
      }
  }

  // p->SingleBufMode = (outBuf != NULL);
  p->outBuf = outBuf;
  p->outBufSize = outBufSize;

  // p->SingleBufMode = False;
  // outBuf = NULL;
  
  if (needReInit)
  {
    MixCoder_Free(p);
    for (i = 0; i < numFilters; i++)
    {
      RINOK(MixCoder_SetFromMethod(p, i, block->filters[numFilters - 1 - i].id, outBuf, outBufSize))
    }
    p->numCoders = numFilters;
  }
  else
  {
    RINOK(MixCoder_ResetFromMethod(p, 0, block->filters[numFilters - 1].id, outBuf, outBufSize))
  }

  for (i = 0; i < numFilters; i++)
  {
    const CXzFilter *f = &block->filters[numFilters - 1 - i];
    IStateCoder *sc = &p->coders[i];
    RINOK(sc->SetProps(sc->p, f->props, f->propsSize, p->alloc))
  }
  
  MixCoder_Init(p);
  return SZ_OK;
}



void XzUnpacker_Init(CXzUnpacker *p)
{
  p->state = XZ_STATE_STREAM_HEADER;
  p->pos = 0;
  p->numStartedStreams = 0;
  p->numFinishedStreams = 0;
  p->numTotalBlocks = 0;
  p->padSize = 0;
  p->decodeOnlyOneBlock = 0;

  p->parseMode = False;
  p->decodeToStreamSignature = False;

  // p->outBuf = NULL;
  // p->outBufSize = 0;
  p->outDataWritten = 0;
}


void XzUnpacker_SetOutBuf(CXzUnpacker *p, Byte *outBuf, size_t outBufSize)
{
  p->outBuf = outBuf;
  p->outBufSize = outBufSize;
}


void XzUnpacker_Construct(CXzUnpacker *p, ISzAllocPtr alloc)
{
  MixCoder_Construct(&p->decoder, alloc);
  p->outBuf = NULL;
  p->outBufSize = 0;
  XzUnpacker_Init(p);
}


void XzUnpacker_Free(CXzUnpacker *p)
{
  MixCoder_Free(&p->decoder);
}


void XzUnpacker_PrepareToRandomBlockDecoding(CXzUnpacker *p)
{
  p->indexSize = 0;
  p->numBlocks = 0;
  Sha256_Init(&p->sha);
  p->state = XZ_STATE_BLOCK_HEADER;
  p->pos = 0;
  p->decodeOnlyOneBlock = 1;
}


static void XzUnpacker_UpdateIndex(CXzUnpacker *p, UInt64 packSize, UInt64 unpackSize)
{
  Byte temp[32];
  unsigned num = Xz_WriteVarInt(temp, packSize);
  num += Xz_WriteVarInt(temp + num, unpackSize);
  Sha256_Update(&p->sha, temp, num);
  p->indexSize += num;
  p->numBlocks++;
}



SRes XzUnpacker_Code(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, int srcFinished,
    ECoderFinishMode finishMode, ECoderStatus *status)
{
  SizeT destLenOrig = *destLen;
  SizeT srcLenOrig = *srcLen;
  *destLen = 0;
  *srcLen = 0;
  *status = CODER_STATUS_NOT_SPECIFIED;

  for (;;)
  {
    SizeT srcRem;

    if (p->state == XZ_STATE_BLOCK)
    {
      SizeT destLen2 = destLenOrig - *destLen;
      SizeT srcLen2 = srcLenOrig - *srcLen;
      SRes res;

      ECoderFinishMode finishMode2 = finishMode;
      BoolInt srcFinished2 = (BoolInt)srcFinished;
      BoolInt destFinish = False;

      if (p->block.packSize != (UInt64)(Int64)-1)
      {
        UInt64 rem = p->block.packSize - p->packSize;
        if (srcLen2 >= rem)
        {
          srcFinished2 = True;
          srcLen2 = (SizeT)rem;
        }
        if (rem == 0 && p->block.unpackSize == p->unpackSize)
          return SZ_ERROR_DATA;
      }

      if (p->block.unpackSize != (UInt64)(Int64)-1)
      {
        UInt64 rem = p->block.unpackSize - p->unpackSize;
        if (destLen2 >= rem)
        {
          destFinish = True;
          finishMode2 = CODER_FINISH_END;
          destLen2 = (SizeT)rem;
        }
      }

      /*
      if (srcLen2 == 0 && destLen2 == 0)
      {
        *status = CODER_STATUS_NOT_FINISHED;
        return SZ_OK;
      }
      */
      
      {
        res = MixCoder_Code(&p->decoder,
            (p->outBuf ? NULL : dest), &destLen2, destFinish,
            src, &srcLen2, srcFinished2,
            finishMode2);

        *status = p->decoder.status;
        XzCheck_Update(&p->check, (p->outBuf ? p->outBuf + p->outDataWritten : dest), destLen2);
        if (!p->outBuf)
          dest += destLen2;
        p->outDataWritten += destLen2;
      }
      
      (*srcLen) += srcLen2;
      src += srcLen2;
      p->packSize += srcLen2;
      (*destLen) += destLen2;
      p->unpackSize += destLen2;

      RINOK(res)

      if (*status != CODER_STATUS_FINISHED_WITH_MARK)
      {
        if (p->block.packSize == p->packSize
            && *status == CODER_STATUS_NEEDS_MORE_INPUT)
        {
          PRF_STR("CODER_STATUS_NEEDS_MORE_INPUT")
          *status = CODER_STATUS_NOT_SPECIFIED;
          return SZ_ERROR_DATA;
        }
        
        return SZ_OK;
      }
      {
        XzUnpacker_UpdateIndex(p, XzUnpacker_GetPackSizeForIndex(p), p->unpackSize);
        p->state = XZ_STATE_BLOCK_FOOTER;
        p->pos = 0;
        p->alignPos = 0;
        *status = CODER_STATUS_NOT_SPECIFIED;

        if ((p->block.packSize != (UInt64)(Int64)-1 && p->block.packSize != p->packSize)
           || (p->block.unpackSize != (UInt64)(Int64)-1 && p->block.unpackSize != p->unpackSize))
        {
          PRF_STR("ERROR: block.size mismatch")
          return SZ_ERROR_DATA;
        }
      }
      // continue;
    }

    srcRem = srcLenOrig - *srcLen;

    // XZ_STATE_BLOCK_FOOTER can transit to XZ_STATE_BLOCK_HEADER without input bytes
    if (srcRem == 0 && p->state != XZ_STATE_BLOCK_FOOTER)
    {
      *status = CODER_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    switch ((int)p->state)
    {
      case XZ_STATE_STREAM_HEADER:
      {
        if (p->pos < XZ_STREAM_HEADER_SIZE)
        {
          if (p->pos < XZ_SIG_SIZE && *src != XZ_SIG[p->pos])
            return SZ_ERROR_NO_ARCHIVE;
          if (p->decodeToStreamSignature)
            return SZ_OK;
          p->buf[p->pos++] = *src++;
          (*srcLen)++;
        }
        else
        {
          RINOK(Xz_ParseHeader(&p->streamFlags, p->buf))
          p->numStartedStreams++;
          p->indexSize = 0;
          p->numBlocks = 0;
          Sha256_Init(&p->sha);
          p->state = XZ_STATE_BLOCK_HEADER;
          p->pos = 0;
        }
        break;
      }

      case XZ_STATE_BLOCK_HEADER:
      {
        if (p->pos == 0)
        {
          p->buf[p->pos++] = *src++;
          (*srcLen)++;
          if (p->buf[0] == 0)
          {
            if (p->decodeOnlyOneBlock)
              return SZ_ERROR_DATA;
            p->indexPreSize = 1 + Xz_WriteVarInt(p->buf + 1, p->numBlocks);
            p->indexPos = p->indexPreSize;
            p->indexSize += p->indexPreSize;
            Sha256_Final(&p->sha, (Byte *)(void *)p->shaDigest32);
            Sha256_Init(&p->sha);
            p->crc = CrcUpdate(CRC_INIT_VAL, p->buf, p->indexPreSize);
            p->state = XZ_STATE_STREAM_INDEX;
            break;
          }
          p->blockHeaderSize = ((unsigned)p->buf[0] << 2) + 4;
          break;
        }
        
        if (p->pos != p->blockHeaderSize)
        {
          unsigned cur = p->blockHeaderSize - p->pos;
          if (cur > srcRem)
            cur = (unsigned)srcRem;
          memcpy(p->buf + p->pos, src, cur);
          p->pos += cur;
          (*srcLen) += cur;
          src += cur;
        }
        else
        {
          RINOK(XzBlock_Parse(&p->block, p->buf))
          if (!XzBlock_AreSupportedFilters(&p->block))
            return SZ_ERROR_UNSUPPORTED;
          p->numTotalBlocks++;
          p->state = XZ_STATE_BLOCK;
          p->packSize = 0;
          p->unpackSize = 0;
          XzCheck_Init(&p->check, XzFlags_GetCheckType(p->streamFlags));
          if (p->parseMode)
          {
            p->headerParsedOk = True;
            return SZ_OK;
          }
          RINOK(XzDecMix_Init(&p->decoder, &p->block, p->outBuf, p->outBufSize))
        }
        break;
      }

      case XZ_STATE_BLOCK_FOOTER:
      {
        if ((((unsigned)p->packSize + p->alignPos) & 3) != 0)
        {
          if (srcRem == 0)
          {
            *status = CODER_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
          (*srcLen)++;
          p->alignPos++;
          if (*src++ != 0)
            return SZ_ERROR_CRC;
        }
        else
        {
          const unsigned checkSize = XzFlags_GetCheckSize(p->streamFlags);
          unsigned cur = checkSize - p->pos;
          if (cur != 0)
          {
            if (srcRem == 0)
            {
              *status = CODER_STATUS_NEEDS_MORE_INPUT;
              return SZ_OK;
            }
            if (cur > srcRem)
              cur = (unsigned)srcRem;
            memcpy(p->buf + p->pos, src, cur);
            p->pos += cur;
            (*srcLen) += cur;
            src += cur;
            if (checkSize != p->pos)
              break;
          }
          {
            UInt32 digest32[XZ_CHECK_SIZE_MAX / 4];
            p->state = XZ_STATE_BLOCK_HEADER;
            p->pos = 0;
            if (XzCheck_Final(&p->check, (Byte *)(void *)digest32) && memcmp(digest32, p->buf, checkSize) != 0)
              return SZ_ERROR_CRC;
            if (p->decodeOnlyOneBlock)
            {
              *status = CODER_STATUS_FINISHED_WITH_MARK;
              return SZ_OK;
            }
          }
        }
        break;
      }

      case XZ_STATE_STREAM_INDEX:
      {
        if (p->pos < p->indexPreSize)
        {
          (*srcLen)++;
          if (*src++ != p->buf[p->pos++])
            return SZ_ERROR_CRC;
        }
        else
        {
          if (p->indexPos < p->indexSize)
          {
            UInt64 cur = p->indexSize - p->indexPos;
            if (srcRem > cur)
              srcRem = (SizeT)cur;
            p->crc = CrcUpdate(p->crc, src, srcRem);
            Sha256_Update(&p->sha, src, srcRem);
            (*srcLen) += srcRem;
            src += srcRem;
            p->indexPos += srcRem;
          }
          else if ((p->indexPos & 3) != 0)
          {
            Byte b = *src++;
            p->crc = CRC_UPDATE_BYTE(p->crc, b);
            (*srcLen)++;
            p->indexPos++;
            p->indexSize++;
            if (b != 0)
              return SZ_ERROR_CRC;
          }
          else
          {
            UInt32 digest32[SHA256_DIGEST_SIZE / 4];
            p->state = XZ_STATE_STREAM_INDEX_CRC;
            p->indexSize += 4;
            p->pos = 0;
            Sha256_Final(&p->sha, (Byte *)(void *)digest32);
            if (memcmp(digest32, p->shaDigest32, SHA256_DIGEST_SIZE) != 0)
              return SZ_ERROR_CRC;
          }
        }
        break;
      }

      case XZ_STATE_STREAM_INDEX_CRC:
      {
        if (p->pos < 4)
        {
          (*srcLen)++;
          p->buf[p->pos++] = *src++;
        }
        else
        {
          const Byte *ptr = p->buf;
          p->state = XZ_STATE_STREAM_FOOTER;
          p->pos = 0;
          if (CRC_GET_DIGEST(p->crc) != GetUi32a(ptr))
            return SZ_ERROR_CRC;
        }
        break;
      }

      case XZ_STATE_STREAM_FOOTER:
      {
        unsigned cur = XZ_STREAM_FOOTER_SIZE - p->pos;
        if (cur > srcRem)
          cur = (unsigned)srcRem;
        memcpy(p->buf + p->pos, src, cur);
        p->pos += cur;
        (*srcLen) += cur;
        src += cur;
        if (p->pos == XZ_STREAM_FOOTER_SIZE)
        {
          p->state = XZ_STATE_STREAM_PADDING;
          p->numFinishedStreams++;
          p->padSize = 0;
          if (!Xz_CheckFooter(p->streamFlags, p->indexSize, p->buf))
            return SZ_ERROR_CRC;
        }
        break;
      }

      case XZ_STATE_STREAM_PADDING:
      {
        if (*src != 0)
        {
          if ((unsigned)p->padSize & 3)
            return SZ_ERROR_NO_ARCHIVE;
          p->pos = 0;
          p->state = XZ_STATE_STREAM_HEADER;
        }
        else
        {
          (*srcLen)++;
          src++;
          p->padSize++;
        }
        break;
      }
      
      case XZ_STATE_BLOCK: break; /* to disable GCC warning */
      
      default: return SZ_ERROR_FAIL;
    }
  }
  /*
  if (p->state == XZ_STATE_FINISHED)
    *status = CODER_STATUS_FINISHED_WITH_MARK;
  return SZ_OK;
  */
}


SRes XzUnpacker_CodeFull(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen,
    ECoderFinishMode finishMode, ECoderStatus *status)
{
  XzUnpacker_Init(p);
  XzUnpacker_SetOutBuf(p, dest, *destLen);

  return XzUnpacker_Code(p,
      NULL, destLen,
      src, srcLen, True,
      finishMode, status);
}


BoolInt XzUnpacker_IsBlockFinished(const CXzUnpacker *p)
{
  return (p->state == XZ_STATE_BLOCK_HEADER) && (p->pos == 0);
}

BoolInt XzUnpacker_IsStreamWasFinished(const CXzUnpacker *p)
{
  return (p->state == XZ_STATE_STREAM_PADDING) && (((UInt32)p->padSize & 3) == 0);
}

UInt64 XzUnpacker_GetExtraSize(const CXzUnpacker *p)
{
  UInt64 num = 0;
  if (p->state == XZ_STATE_STREAM_PADDING)
    num = p->padSize;
  else if (p->state == XZ_STATE_STREAM_HEADER)
    num = p->padSize + p->pos;
  return num;
}





















#ifndef Z7_ST
// amalgamation: header emitted in prologue
#endif


void XzDecMtProps_Init(CXzDecMtProps *p)
{
  p->inBufSize_ST = 1 << 18;
  p->outStep_ST = 1 << 20;
  p->ignoreErrors = False;

  #ifndef Z7_ST
  p->numThreads = 1;
  p->inBufSize_MT = 1 << 18;
  p->memUseMax = sizeof(size_t) << 28;
  #endif
}



#ifndef Z7_ST

/* ---------- CXzDecMtThread ---------- */

typedef struct
{
  Byte *outBuf;
  size_t outBufSize;
  size_t outPreSize;
  size_t inPreSize;
  size_t inPreHeaderSize;
  size_t blockPackSize_for_Index;  // including block header and checksum.
  size_t blockPackTotal;  // including stream header, block header and checksum.
  size_t inCodeSize;
  size_t outCodeSize;
  ECoderStatus status;
  SRes codeRes;
  BoolInt skipMode;
  // BoolInt finishedWithMark;
  EMtDecParseState parseState;
  BoolInt parsing_Truncated;
  BoolInt atBlockHeader;
  CXzStreamFlags streamFlags;
  // UInt64 numFinishedStreams
  UInt64 numStreams;
  UInt64 numTotalBlocks;
  UInt64 numBlocks;

  BoolInt dec_created;
  CXzUnpacker dec;

  Byte mtPad[1 << 7];
} CXzDecMtThread;

#endif


/* ---------- CXzDecMt ---------- */

struct CXzDecMt
{
  CAlignOffsetAlloc alignOffsetAlloc;
  ISzAllocPtr allocMid;

  CXzDecMtProps props;
  size_t unpackBlockMaxSize;
  
  ISeqInStreamPtr inStream;
  ISeqOutStreamPtr outStream;
  ICompressProgressPtr progress;

  BoolInt finishMode;
  BoolInt outSize_Defined;
  UInt64 outSize;

  UInt64 outProcessed;
  UInt64 inProcessed;
  UInt64 readProcessed;
  BoolInt readWasFinished;
  SRes readRes;
  SRes writeRes;

  Byte *outBuf;
  size_t outBufSize;
  Byte *inBuf;
  size_t inBufSize;

  CXzUnpacker dec;

  ECoderStatus status;
  SRes codeRes;

  #ifndef Z7_ST
  BoolInt mainDecoderWasCalled;
  // int statErrorDefined;
  int finishedDecoderIndex;

  // global values that are used in Parse stage
  CXzStreamFlags streamFlags;
  // UInt64 numFinishedStreams
  UInt64 numStreams;
  UInt64 numTotalBlocks;
  UInt64 numBlocks;

  // UInt64 numBadBlocks;
  SRes mainErrorCode;  // it's set to error code, if the size Code() output doesn't patch the size from Parsing stage
                       // it can be = SZ_ERROR_INPUT_EOF
                       // it can be = SZ_ERROR_DATA, in some another cases
  BoolInt isBlockHeaderState_Parse;
  BoolInt isBlockHeaderState_Write;
  UInt64 outProcessed_Parse;
  BoolInt parsing_Truncated;

  BoolInt mtc_WasConstructed;
  CMtDec mtc;
  CXzDecMtThread coders[MTDEC_THREADS_MAX];
  #endif
};



CXzDecMtHandle XzDecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid)
{
  CXzDecMt *p = (CXzDecMt *)ISzAlloc_Alloc(alloc, sizeof(CXzDecMt));
  if (!p)
    return NULL;
  
  AlignOffsetAlloc_CreateVTable(&p->alignOffsetAlloc);
  p->alignOffsetAlloc.baseAlloc = alloc;
  p->alignOffsetAlloc.numAlignBits = 7;
  p->alignOffsetAlloc.offset = 0;

  p->allocMid = allocMid;

  p->outBuf = NULL;
  p->outBufSize = 0;
  p->inBuf = NULL;
  p->inBufSize = 0;

  XzUnpacker_Construct(&p->dec, &p->alignOffsetAlloc.vt);

  p->unpackBlockMaxSize = 0;

  XzDecMtProps_Init(&p->props);

  #ifndef Z7_ST
  p->mtc_WasConstructed = False;
  {
    unsigned i;
    for (i = 0; i < MTDEC_THREADS_MAX; i++)
    {
      CXzDecMtThread *coder = &p->coders[i];
      coder->dec_created = False;
      coder->outBuf = NULL;
      coder->outBufSize = 0;
    }
  }
  #endif

  return (CXzDecMtHandle)p;
}


#ifndef Z7_ST

static void XzDecMt_FreeOutBufs(CXzDecMt *p)
{
  unsigned i;
  for (i = 0; i < MTDEC_THREADS_MAX; i++)
  {
    CXzDecMtThread *coder = &p->coders[i];
    if (coder->outBuf)
    {
      ISzAlloc_Free(p->allocMid, coder->outBuf);
      coder->outBuf = NULL;
      coder->outBufSize = 0;
    }
  }
  p->unpackBlockMaxSize = 0;
}

#endif



static void XzDecMt_FreeSt(CXzDecMt *p)
{
  XzUnpacker_Free(&p->dec);
  
  if (p->outBuf)
  {
    ISzAlloc_Free(p->allocMid, p->outBuf);
    p->outBuf = NULL;
  }
  p->outBufSize = 0;
  
  if (p->inBuf)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBuf = NULL;
  }
  p->inBufSize = 0;
}


// #define GET_CXzDecMt_p  CXzDecMt *p = pp;

void XzDecMt_Destroy(CXzDecMtHandle p)
{
  // GET_CXzDecMt_p

  XzDecMt_FreeSt(p);

  #ifndef Z7_ST

  if (p->mtc_WasConstructed)
  {
    MtDec_Destruct(&p->mtc);
    p->mtc_WasConstructed = False;
  }
  {
    unsigned i;
    for (i = 0; i < MTDEC_THREADS_MAX; i++)
    {
      CXzDecMtThread *t = &p->coders[i];
      if (t->dec_created)
      {
        // we don't need to free dict here
        XzUnpacker_Free(&t->dec);
        t->dec_created = False;
      }
    }
  }
  XzDecMt_FreeOutBufs(p);

  #endif

  ISzAlloc_Free(p->alignOffsetAlloc.baseAlloc, p);
}



#ifndef Z7_ST

static void XzDecMt_Callback_Parse(void *obj, unsigned coderIndex, CMtDecCallbackInfo *cc)
{
  CXzDecMt *me = (CXzDecMt *)obj;
  CXzDecMtThread *coder = &me->coders[coderIndex];
  size_t srcSize = cc->srcSize;

  cc->srcSize = 0;
  cc->outPos = 0;
  cc->state = MTDEC_PARSE_CONTINUE;

  cc->canCreateNewThread = True;

  if (cc->startCall)
  {
    coder->outPreSize = 0;
    coder->inPreSize = 0;
    coder->inPreHeaderSize = 0;
    coder->parseState = MTDEC_PARSE_CONTINUE;
    coder->parsing_Truncated = False;
    coder->skipMode = False;
    coder->codeRes = SZ_OK;
    coder->status = CODER_STATUS_NOT_SPECIFIED;
    coder->inCodeSize = 0;
    coder->outCodeSize = 0;

    coder->numStreams = me->numStreams;
    coder->numTotalBlocks = me->numTotalBlocks;
    coder->numBlocks = me->numBlocks;

    if (!coder->dec_created)
    {
      XzUnpacker_Construct(&coder->dec, &me->alignOffsetAlloc.vt);
      coder->dec_created = True;
    }
    
    XzUnpacker_Init(&coder->dec);

    if (me->isBlockHeaderState_Parse)
    {
      coder->dec.streamFlags = me->streamFlags;
      coder->atBlockHeader = True;
      XzUnpacker_PrepareToRandomBlockDecoding(&coder->dec);
    }
    else
    {
      coder->atBlockHeader = False;
      me->isBlockHeaderState_Parse = True;
    }

    coder->dec.numStartedStreams = me->numStreams;
    coder->dec.numTotalBlocks = me->numTotalBlocks;
    coder->dec.numBlocks = me->numBlocks;
  }

  while (!coder->skipMode)
  {
    ECoderStatus status;
    SRes res;
    size_t srcSize2 = srcSize;
    size_t destSize = (size_t)0 - 1;

    coder->dec.parseMode = True;
    coder->dec.headerParsedOk = False;
    
    PRF_STR_INT("Parse", srcSize2)
    
    res = XzUnpacker_Code(&coder->dec,
        NULL, &destSize,
        cc->src, &srcSize2, cc->srcFinished,
        CODER_FINISH_END, &status);
    
    // PRF(printf(" res = %d, srcSize2 = %d", res, (unsigned)srcSize2));
    
    coder->codeRes = res;
    coder->status = status;
    cc->srcSize += srcSize2;
    srcSize -= srcSize2;
    coder->inPreHeaderSize += srcSize2;
    coder->inPreSize = coder->inPreHeaderSize;
    
    if (res != SZ_OK)
    {
      cc->state =
      coder->parseState = MTDEC_PARSE_END;
      /*
      if (res == SZ_ERROR_MEM)
        return res;
      return SZ_OK;
      */
      return; // res;
    }
    
    if (coder->dec.headerParsedOk)
    {
      const CXzBlock *block = &coder->dec.block;
      if (XzBlock_HasUnpackSize(block)
          // && block->unpackSize <= me->props.outBlockMax
          && XzBlock_HasPackSize(block))
      {
        {
          if (block->unpackSize * 2 * me->mtc.numStartedThreads > me->props.memUseMax)
          {
            cc->state = MTDEC_PARSE_OVERFLOW;
            return; // SZ_OK;
          }
        }
        {
        const UInt64 packSize = block->packSize;
        const UInt64 packSizeAligned = packSize + ((0 - (unsigned)packSize) & 3);
        const unsigned checkSize = XzFlags_GetCheckSize(coder->dec.streamFlags);
        const UInt64 blockPackSum = coder->inPreSize + packSizeAligned + checkSize;
        // if (blockPackSum <= me->props.inBlockMax)
        // unpackBlockMaxSize
        {
          coder->blockPackSize_for_Index = (size_t)(coder->dec.blockHeaderSize + packSize + checkSize);
          coder->blockPackTotal = (size_t)blockPackSum;
          coder->outPreSize = (size_t)block->unpackSize;
          coder->streamFlags = coder->dec.streamFlags;
          me->streamFlags = coder->dec.streamFlags;
          coder->skipMode = True;
          break;
        }
        }
      }
    }
    else
    // if (coder->inPreSize <= me->props.inBlockMax)
    {
      if (!cc->srcFinished)
        return; // SZ_OK;
      cc->state =
      coder->parseState = MTDEC_PARSE_END;
      return; // SZ_OK;
    }
    cc->state = MTDEC_PARSE_OVERFLOW;
    return; // SZ_OK;
  }

  // ---------- skipMode ----------
  {
    UInt64 rem = coder->blockPackTotal - coder->inPreSize;
    size_t cur = srcSize;
    if (cur > rem)
      cur = (size_t)rem;
    cc->srcSize += cur;
    coder->inPreSize += cur;
    srcSize -= cur;

    if (coder->inPreSize == coder->blockPackTotal)
    {
      if (srcSize == 0)
      {
        if (!cc->srcFinished)
          return; // SZ_OK;
        cc->state = MTDEC_PARSE_END;
      }
      else if ((cc->src)[cc->srcSize] == 0) // we check control byte of next block
        cc->state = MTDEC_PARSE_END;
      else
      {
        cc->state = MTDEC_PARSE_NEW;

        {
          size_t blockMax = me->unpackBlockMaxSize;
          if (blockMax < coder->outPreSize)
            blockMax = coder->outPreSize;
          {
            UInt64 required = (UInt64)blockMax * (me->mtc.numStartedThreads + 1) * 2;
            if (me->props.memUseMax < required)
              cc->canCreateNewThread = False;
          }
        }

        if (me->outSize_Defined)
        {
          // next block can be zero size
          const UInt64 rem2 = me->outSize - me->outProcessed_Parse;
          if (rem2 < coder->outPreSize)
          {
            coder->parsing_Truncated = True;
            cc->state = MTDEC_PARSE_END;
          }
          me->outProcessed_Parse += coder->outPreSize;
        }
      }
    }
    else if (cc->srcFinished)
      cc->state = MTDEC_PARSE_END;
    else
      return; // SZ_OK;

    coder->parseState = cc->state;
    cc->outPos = coder->outPreSize;
    
    me->numStreams = coder->dec.numStartedStreams;
    me->numTotalBlocks = coder->dec.numTotalBlocks;
    me->numBlocks = coder->dec.numBlocks + 1;
    return; // SZ_OK;
  }
}


static SRes XzDecMt_Callback_PreCode(void *pp, unsigned coderIndex)
{
  CXzDecMt *me = (CXzDecMt *)pp;
  CXzDecMtThread *coder = &me->coders[coderIndex];
  Byte *dest;

  if (!coder->dec.headerParsedOk)
    return SZ_OK;

  dest = coder->outBuf;

  if (!dest || coder->outBufSize < coder->outPreSize)
  {
    if (dest)
    {
      ISzAlloc_Free(me->allocMid, dest);
      coder->outBuf = NULL;
      coder->outBufSize = 0;
    }
    {
      size_t outPreSize = coder->outPreSize;
      if (outPreSize == 0)
        outPreSize = 1;
      dest = (Byte *)ISzAlloc_Alloc(me->allocMid, outPreSize);
    }
    if (!dest)
      return SZ_ERROR_MEM;
    coder->outBuf = dest;
    coder->outBufSize = coder->outPreSize;

    if (coder->outBufSize > me->unpackBlockMaxSize)
      me->unpackBlockMaxSize = coder->outBufSize;
  }

  // return SZ_ERROR_MEM;

  XzUnpacker_SetOutBuf(&coder->dec, coder->outBuf, coder->outBufSize);

  {
    SRes res = XzDecMix_Init(&coder->dec.decoder, &coder->dec.block, coder->outBuf, coder->outBufSize);
    // res = SZ_ERROR_UNSUPPORTED; // to test
    coder->codeRes = res;
    if (res != SZ_OK)
    {
      // if (res == SZ_ERROR_MEM) return res;
      if (me->props.ignoreErrors && res != SZ_ERROR_MEM)
        return SZ_OK;
      return res;
    }
  }

  return SZ_OK;
}


static SRes XzDecMt_Callback_Code(void *pp, unsigned coderIndex,
    const Byte *src, size_t srcSize, int srcFinished,
    // int finished, int blockFinished,
    UInt64 *inCodePos, UInt64 *outCodePos, int *stop)
{
  CXzDecMt *me = (CXzDecMt *)pp;
  CXzDecMtThread *coder = &me->coders[coderIndex];

  *inCodePos = coder->inCodeSize;
  *outCodePos = coder->outCodeSize;
  *stop = True;

  if (srcSize > coder->inPreSize - coder->inCodeSize)
    return SZ_ERROR_FAIL;
  
  if (coder->inCodeSize < coder->inPreHeaderSize)
  {
    size_t step = coder->inPreHeaderSize - coder->inCodeSize;
    if (step > srcSize)
      step = srcSize;
    src += step;
    srcSize -= step;
    coder->inCodeSize += step;
    *inCodePos = coder->inCodeSize;
    if (coder->inCodeSize < coder->inPreHeaderSize)
    {
      *stop = False;
      return SZ_OK;
    }
  }

  if (!coder->dec.headerParsedOk)
    return SZ_OK;
  if (!coder->outBuf)
    return SZ_OK;

  if (coder->codeRes == SZ_OK)
  {
    ECoderStatus status;
    SRes res;
    size_t srcProcessed = srcSize;
    size_t outSizeCur = coder->outPreSize - coder->dec.outDataWritten;

    // PRF(printf("\nCallback_Code: Code %d %d\n", (unsigned)srcSize, (unsigned)outSizeCur));

    res = XzUnpacker_Code(&coder->dec,
        NULL, &outSizeCur,
        src, &srcProcessed, srcFinished,
        // coder->finishedWithMark ? CODER_FINISH_END : CODER_FINISH_ANY,
        CODER_FINISH_END,
        &status);

    // PRF(printf(" res = %d, srcSize2 = %d, outSizeCur = %d", res, (unsigned)srcProcessed, (unsigned)outSizeCur));

    coder->codeRes = res;
    coder->status = status;
    coder->inCodeSize += srcProcessed;
    coder->outCodeSize = coder->dec.outDataWritten;
    *inCodePos = coder->inCodeSize;
    *outCodePos = coder->outCodeSize;

    if (res == SZ_OK)
    {
      if (srcProcessed == srcSize)
        *stop = False;
      return SZ_OK;
    }
  }

  if (me->props.ignoreErrors && coder->codeRes != SZ_ERROR_MEM)
  {
    *inCodePos = coder->inPreSize;
    *outCodePos = coder->outPreSize;
    return SZ_OK;
  }
  return coder->codeRes;
}


#define XZDECMT_STREAM_WRITE_STEP (1 << 24)

static SRes XzDecMt_Callback_Write(void *pp, unsigned coderIndex,
    BoolInt needWriteToStream,
    const Byte *src, size_t srcSize, BoolInt isCross,
    // int srcFinished,
    BoolInt *needContinue,
    BoolInt *canRecode)
{
  CXzDecMt *me = (CXzDecMt *)pp;
  const CXzDecMtThread *coder = &me->coders[coderIndex];

  // PRF(printf("\nWrite processed = %d srcSize = %d\n", (unsigned)me->mtc.inProcessed, (unsigned)srcSize));
  
  *needContinue = False;
  *canRecode = True;
  
  if (!needWriteToStream)
    return SZ_OK;

  if (!coder->dec.headerParsedOk || !coder->outBuf)
  {
    if (me->finishedDecoderIndex < 0)
      me->finishedDecoderIndex = (int)coderIndex;
    return SZ_OK;
  }

  if (me->finishedDecoderIndex >= 0)
    return SZ_OK;

  me->mtc.inProcessed += coder->inCodeSize;

  *canRecode = False;

  {
    SRes res;
    size_t size = coder->outCodeSize;
    Byte *data = coder->outBuf;
    
    // we use in me->dec: sha, numBlocks, indexSize

    if (!me->isBlockHeaderState_Write)
    {
      XzUnpacker_PrepareToRandomBlockDecoding(&me->dec);
      me->dec.decodeOnlyOneBlock = False;
      me->dec.numStartedStreams = coder->dec.numStartedStreams;
      me->dec.streamFlags = coder->streamFlags;

      me->isBlockHeaderState_Write = True;
    }
    
    me->dec.numTotalBlocks = coder->dec.numTotalBlocks;
    XzUnpacker_UpdateIndex(&me->dec, coder->blockPackSize_for_Index, coder->outPreSize);
    
    if (coder->outPreSize != size)
    {
      if (me->props.ignoreErrors)
      {
        memset(data + size, 0, coder->outPreSize - size);
        size = coder->outPreSize;
      }
      // me->numBadBlocks++;
      if (me->mainErrorCode == SZ_OK)
      {
        if ((int)coder->status == LZMA_STATUS_NEEDS_MORE_INPUT)
          me->mainErrorCode = SZ_ERROR_INPUT_EOF;
        else
          me->mainErrorCode = SZ_ERROR_DATA;
      }
    }
    
    if (me->writeRes != SZ_OK)
      return me->writeRes;

    res = SZ_OK;
    {
      if (me->outSize_Defined)
      {
        const UInt64 rem = me->outSize - me->outProcessed;
        if (size > rem)
          size = (SizeT)rem;
      }

      for (;;)
      {
        size_t cur = size;
        size_t written;
        if (cur > XZDECMT_STREAM_WRITE_STEP)
          cur = XZDECMT_STREAM_WRITE_STEP;

        written = ISeqOutStream_Write(me->outStream, data, cur);

        // PRF(printf("\nWritten ask = %d written = %d\n", (unsigned)cur, (unsigned)written));
        
        me->outProcessed += written;
        if (written != cur)
        {
          me->writeRes = SZ_ERROR_WRITE;
          res = me->writeRes;
          break;
        }
        data += cur;
        size -= cur;
        // PRF_STR_INT("Written size =", size)
        if (size == 0)
          break;
        res = MtProgress_ProgressAdd(&me->mtc.mtProgress, 0, 0);
        if (res != SZ_OK)
          break;
      }
    }

    if (coder->codeRes != SZ_OK)
      if (!me->props.ignoreErrors)
      {
        me->finishedDecoderIndex = (int)coderIndex;
        return res;
      }

    RINOK(res)

    if (coder->inPreSize != coder->inCodeSize
        || coder->blockPackTotal != coder->inCodeSize)
    {
      me->finishedDecoderIndex = (int)coderIndex;
      return SZ_OK;
    }

    if (coder->parseState != MTDEC_PARSE_END)
    {
      *needContinue = True;
      return SZ_OK;
    }
  }

  // (coder->state == MTDEC_PARSE_END) means that there are no other working threads
  // so we can use mtc variables without lock

  PRF_STR_INT("Write MTDEC_PARSE_END", me->mtc.inProcessed)

  me->mtc.mtProgress.totalInSize = me->mtc.inProcessed;
  {
    CXzUnpacker *dec = &me->dec;
    
    PRF_STR_INT("PostSingle", srcSize)
    
    {
      size_t srcProcessed = srcSize;
      ECoderStatus status;
      size_t outSizeCur = 0;
      SRes res;
      
      // dec->decodeOnlyOneBlock = False;
      dec->decodeToStreamSignature = True;

      me->mainDecoderWasCalled = True;

      if (coder->parsing_Truncated)
      {
        me->parsing_Truncated = True;
        return SZ_OK;
      }
      
      /*
      We have processed all xz-blocks of stream,
      And xz unpacker is at XZ_STATE_BLOCK_HEADER state, where
      (src) is a pointer to xz-Index structure.
      We finish reading of current xz-Stream, including Zero padding after xz-Stream.
      We exit, if we reach extra byte (first byte of new-Stream or another data).
      But we don't update input stream pointer for that new extra byte.
      If extra byte is not correct first byte of xz-signature,
      we have SZ_ERROR_NO_ARCHIVE error here.
      */

      res = XzUnpacker_Code(dec,
          NULL, &outSizeCur,
          src, &srcProcessed,
          me->mtc.readWasFinished, // srcFinished
          CODER_FINISH_END, // CODER_FINISH_ANY,
          &status);

      // res = SZ_ERROR_ARCHIVE; // for failure test
      
      me->status = status;
      me->codeRes = res;

      if (isCross)
        me->mtc.crossStart += srcProcessed;

      me->mtc.inProcessed += srcProcessed;
      me->mtc.mtProgress.totalInSize = me->mtc.inProcessed;

      srcSize -= srcProcessed;
      src += srcProcessed;

      if (res != SZ_OK)
      {
        return SZ_OK;
        // return res;
      }
      
      if (dec->state == XZ_STATE_STREAM_HEADER)
      {
        *needContinue = True;
        me->isBlockHeaderState_Parse = False;
        me->isBlockHeaderState_Write = False;

        if (!isCross)
        {
          Byte *crossBuf = MtDec_GetCrossBuff(&me->mtc);
          if (!crossBuf)
            return SZ_ERROR_MEM;
          if (srcSize != 0)
            memcpy(crossBuf, src, srcSize);
          me->mtc.crossStart = 0;
          me->mtc.crossEnd = srcSize;
        }

        PRF_STR_INT("XZ_STATE_STREAM_HEADER crossEnd = ", (unsigned)me->mtc.crossEnd)

        return SZ_OK;
      }
      
      if (status != CODER_STATUS_NEEDS_MORE_INPUT || srcSize != 0)
      {
        return SZ_ERROR_FAIL;
      }
      
      if (me->mtc.readWasFinished)
      {
        return SZ_OK;
      }
    }
    
    {
      size_t inPos;
      size_t inLim;
      // const Byte *inData;
      UInt64 inProgressPrev = me->mtc.inProcessed;
      
      // XzDecMt_Prepare_InBuf_ST(p);
      Byte *crossBuf = MtDec_GetCrossBuff(&me->mtc);
      if (!crossBuf)
        return SZ_ERROR_MEM;
      
      inPos = 0;
      inLim = 0;
      
      // inData = crossBuf;
      
      for (;;)
      {
        SizeT inProcessed;
        SizeT outProcessed;
        ECoderStatus status;
        SRes res;
        
        if (inPos == inLim)
        {
          if (!me->mtc.readWasFinished)
          {
            inPos = 0;
            inLim = me->mtc.inBufSize;
            me->mtc.readRes = ISeqInStream_Read(me->inStream, (void *)crossBuf, &inLim);
            me->mtc.readProcessed += inLim;
            if (inLim == 0 || me->mtc.readRes != SZ_OK)
              me->mtc.readWasFinished = True;
          }
        }
        
        inProcessed = inLim - inPos;
        outProcessed = 0;

        res = XzUnpacker_Code(dec,
            NULL, &outProcessed,
            crossBuf + inPos, &inProcessed,
            (inProcessed == 0), // srcFinished
            CODER_FINISH_END, &status);
        
        me->codeRes = res;
        me->status = status;
        inPos += inProcessed;
        me->mtc.inProcessed += inProcessed;
        me->mtc.mtProgress.totalInSize = me->mtc.inProcessed;

        if (res != SZ_OK)
        {
          return SZ_OK;
          // return res;
        }

        if (dec->state == XZ_STATE_STREAM_HEADER)
        {
          *needContinue = True;
          me->mtc.crossStart = inPos;
          me->mtc.crossEnd = inLim;
          me->isBlockHeaderState_Parse = False;
          me->isBlockHeaderState_Write = False;
          return SZ_OK;
        }
        
        if (status != CODER_STATUS_NEEDS_MORE_INPUT)
          return SZ_ERROR_FAIL;
        
        if (me->mtc.progress)
        {
          UInt64 inDelta = me->mtc.inProcessed - inProgressPrev;
          if (inDelta >= (1 << 22))
          {
            RINOK(MtProgress_Progress_ST(&me->mtc.mtProgress))
            inProgressPrev = me->mtc.inProcessed;
          }
        }
        if (me->mtc.readWasFinished)
          return SZ_OK;
      }
    }
  }
}


#endif



void XzStatInfo_Clear(CXzStatInfo *p)
{
  p->InSize = 0;
  p->OutSize = 0;
  
  p->NumStreams = 0;
  p->NumBlocks = 0;
  
  p->UnpackSize_Defined = False;
  
  p->NumStreams_Defined = False;
  p->NumBlocks_Defined = False;
  
  p->DataAfterEnd = False;
  p->DecodingTruncated = False;
  
  p->DecodeRes = SZ_OK;
  p->ReadRes = SZ_OK;
  p->ProgressRes = SZ_OK;

  p->CombinedRes = SZ_OK;
  p->CombinedRes_Type = SZ_OK;
}



/*
  XzDecMt_Decode_ST() can return SZ_OK or the following errors
     - SZ_ERROR_MEM for memory allocation error
     - error from XzUnpacker_Code() function
     - SZ_ERROR_WRITE for ISeqOutStream::Write(). stat->CombinedRes_Type = SZ_ERROR_WRITE in that case
     - ICompressProgress::Progress() error,  stat->CombinedRes_Type = SZ_ERROR_PROGRESS.
  But XzDecMt_Decode_ST() doesn't return ISeqInStream::Read() errors.
  ISeqInStream::Read() result is set to p->readRes.
  also it can set stat->CombinedRes_Type to SZ_ERROR_WRITE or SZ_ERROR_PROGRESS.
*/

static SRes XzDecMt_Decode_ST(CXzDecMt *p
    #ifndef Z7_ST
    , BoolInt tMode
    #endif
    , CXzStatInfo *stat)
{
  size_t outPos;
  size_t inPos, inLim;
  const Byte *inData;
  UInt64 inPrev, outPrev;

  CXzUnpacker *dec;

  #ifndef Z7_ST
  if (tMode)
  {
    XzDecMt_FreeOutBufs(p);
    tMode = (BoolInt)MtDec_PrepareRead(&p->mtc);
  }
  #endif

  if (!p->outBuf || p->outBufSize != p->props.outStep_ST)
  {
    ISzAlloc_Free(p->allocMid, p->outBuf);
    p->outBufSize = 0;
    p->outBuf = (Byte *)ISzAlloc_Alloc(p->allocMid, p->props.outStep_ST);
    if (!p->outBuf)
      return SZ_ERROR_MEM;
    p->outBufSize = p->props.outStep_ST;
  }

  if (!p->inBuf || p->inBufSize != p->props.inBufSize_ST)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBufSize = 0;
    p->inBuf = (Byte *)ISzAlloc_Alloc(p->allocMid, p->props.inBufSize_ST);
    if (!p->inBuf)
      return SZ_ERROR_MEM;
    p->inBufSize = p->props.inBufSize_ST;
  }

  dec = &p->dec;
  dec->decodeToStreamSignature = False;
  // dec->decodeOnlyOneBlock = False;

  XzUnpacker_SetOutBuf(dec, NULL, 0);

  inPrev = p->inProcessed;
  outPrev = p->outProcessed;

  inPos = 0;
  inLim = 0;
  inData = NULL;
  outPos = 0;

  for (;;)
  {
    SizeT outSize;
    BoolInt finished;
    ECoderFinishMode finishMode;
    SizeT inProcessed;
    ECoderStatus status;
    SRes res;

    SizeT outProcessed;



    if (inPos == inLim)
    {
      #ifndef Z7_ST
      if (tMode)
      {
        inData = MtDec_Read(&p->mtc, &inLim);
        inPos = 0;
        if (inData)
          continue;
        tMode = False;
        inLim = 0;
      }
      #endif
      
      if (!p->readWasFinished)
      {
        inPos = 0;
        inLim = p->inBufSize;
        inData = p->inBuf;
        p->readRes = ISeqInStream_Read(p->inStream, (void *)p->inBuf, &inLim);
        p->readProcessed += inLim;
        if (inLim == 0 || p->readRes != SZ_OK)
          p->readWasFinished = True;
      }
    }

    outSize = p->props.outStep_ST - outPos;

    finishMode = CODER_FINISH_ANY;
    if (p->outSize_Defined)
    {
      const UInt64 rem = p->outSize - p->outProcessed;
      if (outSize >= rem)
      {
        outSize = (SizeT)rem;
        if (p->finishMode)
          finishMode = CODER_FINISH_END;
      }
    }

    inProcessed = inLim - inPos;
    outProcessed = outSize;

    res = XzUnpacker_Code(dec, p->outBuf + outPos, &outProcessed,
        inData + inPos, &inProcessed,
        (inPos == inLim), // srcFinished
        finishMode, &status);

    p->codeRes = res;
    p->status = status;

    inPos += inProcessed;
    outPos += outProcessed;
    p->inProcessed += inProcessed;
    p->outProcessed += outProcessed;

    finished = ((inProcessed == 0 && outProcessed == 0) || res != SZ_OK);

    if (finished || outProcessed >= outSize)
      if (outPos != 0)
      {
        const size_t written = ISeqOutStream_Write(p->outStream, p->outBuf, outPos);
        // p->outProcessed += written; // 21.01: BUG fixed
        if (written != outPos)
        {
          stat->CombinedRes_Type = SZ_ERROR_WRITE;
          return SZ_ERROR_WRITE;
        }
        outPos = 0;
      }

    if (p->progress && res == SZ_OK)
    {
      if (p->inProcessed - inPrev >= (1 << 22) ||
          p->outProcessed - outPrev >= (1 << 22))
      {
        res = ICompressProgress_Progress(p->progress, p->inProcessed, p->outProcessed);
        if (res != SZ_OK)
        {
          stat->CombinedRes_Type = SZ_ERROR_PROGRESS;
          stat->ProgressRes = res;
          return res;
        }
        inPrev = p->inProcessed;
        outPrev = p->outProcessed;
      }
    }

    if (finished)
    {
      // p->codeRes is preliminary error from XzUnpacker_Code.
      // and it can be corrected later as final result
      // so we return SZ_OK here instead of (res);
      return SZ_OK;
      // return res;
    }
  }
}



/*
XzStatInfo_SetStat() transforms
    CXzUnpacker return code and status to combined CXzStatInfo results.
    it can convert SZ_OK to SZ_ERROR_INPUT_EOF
    it can convert SZ_ERROR_NO_ARCHIVE to SZ_OK and (DataAfterEnd = 1)
*/

static void XzStatInfo_SetStat(const CXzUnpacker *dec,
    int finishMode,
    // UInt64 readProcessed,
    UInt64 inProcessed,
    SRes res,                     // it's result from CXzUnpacker unpacker
    ECoderStatus status,
    BoolInt decodingTruncated,
    CXzStatInfo *stat)
{
  UInt64 extraSize;
  
  stat->DecodingTruncated = (Byte)(decodingTruncated ? 1 : 0);
  stat->InSize = inProcessed;
  stat->NumStreams = dec->numStartedStreams;
  stat->NumBlocks = dec->numTotalBlocks;
  
  stat->UnpackSize_Defined = True;
  stat->NumStreams_Defined = True;
  stat->NumBlocks_Defined = True;
  
  extraSize = XzUnpacker_GetExtraSize(dec);
  
  if (res == SZ_OK)
  {
    if (status == CODER_STATUS_NEEDS_MORE_INPUT)
    {
      // CODER_STATUS_NEEDS_MORE_INPUT is expected status for correct xz streams
      // any extra data is part of correct data
      extraSize = 0;
      // if xz stream was not finished, then we need more data
      if (!XzUnpacker_IsStreamWasFinished(dec))
        res = SZ_ERROR_INPUT_EOF;
    }
    else
    {
      // CODER_STATUS_FINISHED_WITH_MARK is not possible for multi stream xz decoding
      // so he we have (status == CODER_STATUS_NOT_FINISHED)
      // if (status != CODER_STATUS_FINISHED_WITH_MARK)
      if (!decodingTruncated || finishMode)
        res = SZ_ERROR_DATA;
    }
  }
  else if (res == SZ_ERROR_NO_ARCHIVE)
  {
    /*
    SZ_ERROR_NO_ARCHIVE is possible for 2 states:
      XZ_STATE_STREAM_HEADER  - if bad signature or bad CRC
      XZ_STATE_STREAM_PADDING - if non-zero padding data
    extraSize and inProcessed don't include "bad" byte
    */
    // if (inProcessed == extraSize), there was no any good xz stream header, and we keep error
    if (inProcessed != extraSize) // if there were good xz streams before error
    {
      // if (extraSize != 0 || readProcessed != inProcessed)
      {
        // he we suppose that all xz streams were finsihed OK, and we have
        // some extra data after all streams
        stat->DataAfterEnd = True;
        res = SZ_OK;
      }
    }
  }
  
  if (stat->DecodeRes == SZ_OK)
    stat->DecodeRes = res;

  stat->InSize -= extraSize;
}



SRes XzDecMt_Decode(CXzDecMtHandle p,
    const CXzDecMtProps *props,
    const UInt64 *outDataSize, int finishMode,
    ISeqOutStreamPtr outStream,
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    // const Byte *inData, size_t inDataSize,
    CXzStatInfo *stat,
    int *isMT,
    ICompressProgressPtr progress)
{
  // GET_CXzDecMt_p
  #ifndef Z7_ST
  BoolInt tMode;
  #endif

  XzStatInfo_Clear(stat);

  p->props = *props;

  p->inStream = inStream;
  p->outStream = outStream;
  p->progress = progress;
  // p->stat = stat;

  p->outSize = 0;
  p->outSize_Defined = False;
  if (outDataSize)
  {
    p->outSize_Defined = True;
    p->outSize = *outDataSize;
  }

  p->finishMode = (BoolInt)finishMode;

  // p->outSize = 457; p->outSize_Defined = True; p->finishMode = False; // for test

  p->writeRes = SZ_OK;
  p->outProcessed = 0;
  p->inProcessed = 0;
  p->readProcessed = 0;
  p->readWasFinished = False;
  p->readRes = SZ_OK;

  p->codeRes = SZ_OK;
  p->status = CODER_STATUS_NOT_SPECIFIED;

  XzUnpacker_Init(&p->dec);

  *isMT = False;

    /*
    p->outBuf = NULL;
    p->outBufSize = 0;
    if (!outStream)
    {
      p->outBuf = outBuf;
      p->outBufSize = *outBufSize;
      *outBufSize = 0;
    }
    */

  
  #ifndef Z7_ST

  p->isBlockHeaderState_Parse = False;
  p->isBlockHeaderState_Write = False;
  // p->numBadBlocks = 0;
  p->mainErrorCode = SZ_OK;
  p->mainDecoderWasCalled = False;

  tMode = False;

  if (p->props.numThreads > 1)
  {
    IMtDecCallback2 vt;
    BoolInt needContinue;
    SRes res;
    // we just free ST buffers here
    // but we still keep state variables, that was set in XzUnpacker_Init()
    XzDecMt_FreeSt(p);

    p->outProcessed_Parse = 0;
    p->parsing_Truncated = False;

    p->numStreams = 0;
    p->numTotalBlocks = 0;
    p->numBlocks = 0;
    p->finishedDecoderIndex = -1;

    if (!p->mtc_WasConstructed)
    {
      p->mtc_WasConstructed = True;
      MtDec_Construct(&p->mtc);
    }
    
    p->mtc.mtCallback = &vt;
    p->mtc.mtCallbackObject = p;

    p->mtc.progress = progress;
    p->mtc.inStream = inStream;
    p->mtc.alloc = &p->alignOffsetAlloc.vt;
    // p->mtc.inData = inData;
    // p->mtc.inDataSize = inDataSize;
    p->mtc.inBufSize = p->props.inBufSize_MT;
    // p->mtc.inBlockMax = p->props.inBlockMax;
    p->mtc.numThreadsMax = p->props.numThreads;

    *isMT = True;

    vt.Parse = XzDecMt_Callback_Parse;
    vt.PreCode = XzDecMt_Callback_PreCode;
    vt.Code = XzDecMt_Callback_Code;
    vt.Write = XzDecMt_Callback_Write;


    res = MtDec_Code(&p->mtc);


    stat->InSize = p->mtc.inProcessed;
    
    p->inProcessed = p->mtc.inProcessed;
    p->readRes = p->mtc.readRes;
    p->readWasFinished = p->mtc.readWasFinished;
    p->readProcessed = p->mtc.readProcessed;
    
    tMode = True;
    needContinue = False;
    
    if (res == SZ_OK)
    {
      if (p->mtc.mtProgress.res != SZ_OK)
      {
        res = p->mtc.mtProgress.res;
        stat->ProgressRes = res;
        stat->CombinedRes_Type = SZ_ERROR_PROGRESS;
      }
      else
        needContinue = p->mtc.needContinue;
    }
    
    if (!needContinue)
    {
      {
        SRes codeRes;
        BoolInt truncated = False;
        ECoderStatus status;
        const CXzUnpacker *dec;

        stat->OutSize = p->outProcessed;
       
        if (p->finishedDecoderIndex >= 0)
        {
          const CXzDecMtThread *coder = &p->coders[(unsigned)p->finishedDecoderIndex];
          codeRes = coder->codeRes;
          dec = &coder->dec;
          status = coder->status;
        }
        else if (p->mainDecoderWasCalled)
        {
          codeRes = p->codeRes;
          dec = &p->dec;
          status = p->status;
          truncated = p->parsing_Truncated;
        }
        else
          return SZ_ERROR_FAIL;

        if (p->mainErrorCode != SZ_OK)
          stat->DecodeRes = p->mainErrorCode;

        XzStatInfo_SetStat(dec, p->finishMode,
            // p->mtc.readProcessed,
            p->mtc.inProcessed,
            codeRes, status,
            truncated,
            stat);
      }

      if (res == SZ_OK)
      {
        stat->ReadRes = p->mtc.readRes;

        if (p->writeRes != SZ_OK)
        {
          res = p->writeRes;
          stat->CombinedRes_Type = SZ_ERROR_WRITE;
        }
        else if (p->mtc.readRes != SZ_OK
            // && p->mtc.inProcessed == p->mtc.readProcessed
            && stat->DecodeRes == SZ_ERROR_INPUT_EOF)
        {
          res = p->mtc.readRes;
          stat->CombinedRes_Type = SZ_ERROR_READ;
        }
        else if (stat->DecodeRes != SZ_OK)
          res = stat->DecodeRes;
      }
      
      stat->CombinedRes = res;
      if (stat->CombinedRes_Type == SZ_OK)
        stat->CombinedRes_Type = res;
      return res;
    }

    PRF_STR("----- decoding ST -----")
  }

  #endif


  *isMT = False;

  {
    SRes res = XzDecMt_Decode_ST(p
        #ifndef Z7_ST
        , tMode
        #endif
        , stat
        );

    #ifndef Z7_ST
    // we must set error code from MT decoding at first
    if (p->mainErrorCode != SZ_OK)
      stat->DecodeRes = p->mainErrorCode;
    #endif

    XzStatInfo_SetStat(&p->dec,
        p->finishMode,
        // p->readProcessed,
        p->inProcessed,
        p->codeRes, p->status,
        False, // truncated
        stat);

    stat->ReadRes = p->readRes;

    if (res == SZ_OK)
    {
      if (p->readRes != SZ_OK
          // && p->inProcessed == p->readProcessed
          && stat->DecodeRes == SZ_ERROR_INPUT_EOF)
      {
        // we set read error as combined error, only if that error was the reason
        // of decoding problem
        res = p->readRes;
        stat->CombinedRes_Type = SZ_ERROR_READ;
      }
      else if (stat->DecodeRes != SZ_OK)
        res = stat->DecodeRes;
    }

    stat->CombinedRes = res;
    if (stat->CombinedRes_Type == SZ_OK)
      stat->CombinedRes_Type = res;
    return res;
  }
}

#undef PRF
#undef PRF_STR
#undef PRF_STR_INT_2

/* ================ unit: C/XzEnc.c ================ */
/* XzEnc.c -- Xz Encode
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <stdlib.h>
#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef USE_SUBBLOCK
#include "Bcj3Enc.c"
#include "SbFind.c"
#include "SbEnc.c"
#endif

// amalgamation: header emitted in prologue

// #define Z7_ST

#ifndef Z7_ST
// amalgamation: header emitted in prologue
#else
#define MTCODER_THREADS_MAX 1
#define MTCODER_BLOCKS_MAX 1
#endif

#define XZ_GET_PAD_SIZE(dataSize) ((4 - ((unsigned)(dataSize) & 3)) & 3)

#define XZ_CHECK_SIZE_MAX 64
/* max pack size for LZMA2 block + pad4 + check_size: */
#define XZ_GET_MAX_BLOCK_PACK_SIZE(unpackSize) ((unpackSize) + ((unpackSize) >> 10) + 16 + XZ_CHECK_SIZE_MAX)

#define XZ_GET_ESTIMATED_BLOCK_TOTAL_PACK_SIZE(unpackSize) (XZ_BLOCK_HEADER_SIZE_MAX + XZ_GET_MAX_BLOCK_PACK_SIZE(unpackSize))


// #define XzBlock_ClearFlags(p)       (p)->flags = 0;
#define XzBlock_ClearFlags_SetNumFilters(p, n) (p)->flags = (Byte)((n) - 1);
#define XzBlock_SetHasPackSize(p)   (p)->flags |= XZ_BF_PACK_SIZE;
#define XzBlock_SetHasUnpackSize(p) (p)->flags |= XZ_BF_UNPACK_SIZE;


static SRes WriteBytes(ISeqOutStreamPtr s, const void *buf, size_t size)
{
  return (ISeqOutStream_Write(s, buf, size) == size) ? SZ_OK : SZ_ERROR_WRITE;
}

static SRes WriteBytes_UpdateCrc(ISeqOutStreamPtr s, const void *buf, size_t size, UInt32 *crc)
{
  *crc = CrcUpdate(*crc, buf, size);
  return WriteBytes(s, buf, size);
}


static SRes Xz_WriteHeader(CXzStreamFlags f, ISeqOutStreamPtr s)
{
  UInt32 crc;
  Byte header[XZ_STREAM_HEADER_SIZE];
  memcpy(header, XZ_SIG, XZ_SIG_SIZE);
  header[XZ_SIG_SIZE] = (Byte)(f >> 8);
  header[XZ_SIG_SIZE + 1] = (Byte)(f & 0xFF);
  crc = CrcCalc(header + XZ_SIG_SIZE, XZ_STREAM_FLAGS_SIZE);
  SetUi32(header + XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE, crc)
  return WriteBytes(s, header, XZ_STREAM_HEADER_SIZE);
}


static SRes XzBlock_WriteHeader(const CXzBlock *p, ISeqOutStreamPtr s)
{
  Byte header[XZ_BLOCK_HEADER_SIZE_MAX];

  unsigned pos = 1;
  unsigned numFilters, i;
  header[pos++] = p->flags;

  if (XzBlock_HasPackSize(p)) pos += Xz_WriteVarInt(header + pos, p->packSize);
  if (XzBlock_HasUnpackSize(p)) pos += Xz_WriteVarInt(header + pos, p->unpackSize);
  numFilters = XzBlock_GetNumFilters(p);
  
  for (i = 0; i < numFilters; i++)
  {
    const CXzFilter *f = &p->filters[i];
    pos += Xz_WriteVarInt(header + pos, f->id);
    pos += Xz_WriteVarInt(header + pos, f->propsSize);
    memcpy(header + pos, f->props, f->propsSize);
    pos += f->propsSize;
  }

  while ((pos & 3) != 0)
    header[pos++] = 0;

  header[0] = (Byte)(pos >> 2);
  SetUi32(header + pos, CrcCalc(header, pos))
  return WriteBytes(s, header, pos + 4);
}




typedef struct
{
  size_t numBlocks;
  size_t size;
  size_t allocated;
  Byte *blocks;
} CXzEncIndex;


static void XzEncIndex_Construct(CXzEncIndex *p)
{
  p->numBlocks = 0;
  p->size = 0;
  p->allocated = 0;
  p->blocks = NULL;
}

static void XzEncIndex_Init(CXzEncIndex *p)
{
  p->numBlocks = 0;
  p->size = 0;
}

static void XzEncIndex_Free(CXzEncIndex *p, ISzAllocPtr alloc)
{
  if (p->blocks)
  {
    ISzAlloc_Free(alloc, p->blocks);
    p->blocks = NULL;
  }
  p->numBlocks = 0;
  p->size = 0;
  p->allocated = 0;
}


static SRes XzEncIndex_ReAlloc(CXzEncIndex *p, size_t newSize, ISzAllocPtr alloc)
{
  Byte *blocks = (Byte *)ISzAlloc_Alloc(alloc, newSize);
  if (!blocks)
    return SZ_ERROR_MEM;
  if (p->size != 0)
    memcpy(blocks, p->blocks, p->size);
  if (p->blocks)
    ISzAlloc_Free(alloc, p->blocks);
  p->blocks = blocks;
  p->allocated = newSize;
  return SZ_OK;
}


static SRes XzEncIndex_PreAlloc(CXzEncIndex *p, UInt64 numBlocks, UInt64 unpackSize, UInt64 totalSize, ISzAllocPtr alloc)
{
  UInt64 pos;
  {
    Byte buf[32];
    unsigned pos2 = Xz_WriteVarInt(buf, totalSize);
    pos2 += Xz_WriteVarInt(buf + pos2, unpackSize);
    pos = numBlocks * pos2;
  }
  
  if (pos <= p->allocated - p->size)
    return SZ_OK;
  {
    UInt64 newSize64 = p->size + pos;
    size_t newSize = (size_t)newSize64;
    if (newSize != newSize64)
      return SZ_ERROR_MEM;
    return XzEncIndex_ReAlloc(p, newSize, alloc);
  }
}


static SRes XzEncIndex_AddIndexRecord(CXzEncIndex *p, UInt64 unpackSize, UInt64 totalSize, ISzAllocPtr alloc)
{
  Byte buf[32];
  unsigned pos = Xz_WriteVarInt(buf, totalSize);
  pos += Xz_WriteVarInt(buf + pos, unpackSize);

  if (pos > p->allocated - p->size)
  {
    size_t newSize = p->allocated * 2 + 16 * 2;
    if (newSize < p->size + pos)
      return SZ_ERROR_MEM;
    RINOK(XzEncIndex_ReAlloc(p, newSize, alloc))
  }
  memcpy(p->blocks + p->size, buf, pos);
  p->size += pos;
  p->numBlocks++;
  return SZ_OK;
}


static SRes XzEncIndex_WriteFooter(const CXzEncIndex *p, CXzStreamFlags flags, ISeqOutStreamPtr s)
{
  Byte buf[32];
  UInt64 globalPos;
  UInt32 crc = CRC_INIT_VAL;
  unsigned pos = 1 + Xz_WriteVarInt(buf + 1, p->numBlocks);
  
  globalPos = pos;
  buf[0] = 0;
  RINOK(WriteBytes_UpdateCrc(s, buf, pos, &crc))
  RINOK(WriteBytes_UpdateCrc(s, p->blocks, p->size, &crc))
  globalPos += p->size;
  
  pos = XZ_GET_PAD_SIZE(globalPos);
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  globalPos += pos;
  
  crc = CrcUpdate(crc, buf + 4 - pos, pos);
  SetUi32(buf + 4, CRC_GET_DIGEST(crc))
  
  SetUi32(buf + 8 + 4, (UInt32)(globalPos >> 2))
  buf[8 + 8] = (Byte)(flags >> 8);
  buf[8 + 9] = (Byte)(flags & 0xFF);
  SetUi32(buf + 8, CrcCalc(buf + 8 + 4, 6))
  buf[8 + 10] = XZ_FOOTER_SIG_0;
  buf[8 + 11] = XZ_FOOTER_SIG_1;
  
  return WriteBytes(s, buf + 4 - pos, pos + 4 + 12);
}



/* ---------- CSeqCheckInStream ---------- */

typedef struct
{
  ISeqInStream vt;
  ISeqInStreamPtr realStream;
  const Byte *data;
  UInt64 limit;
  UInt64 processed;
  int realStreamFinished;
  CXzCheck check;
} CSeqCheckInStream;

static void SeqCheckInStream_Init(CSeqCheckInStream *p, unsigned checkMode)
{
  p->limit = (UInt64)(Int64)-1;
  p->processed = 0;
  p->realStreamFinished = 0;
  XzCheck_Init(&p->check, checkMode);
}

static void SeqCheckInStream_GetDigest(CSeqCheckInStream *p, Byte *digest)
{
  XzCheck_Final(&p->check, digest);
}

static SRes SeqCheckInStream_Read(ISeqInStreamPtr pp, void *data, size_t *size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeqCheckInStream)
  size_t size2 = *size;
  SRes res = SZ_OK;
  
  if (p->limit != (UInt64)(Int64)-1)
  {
    UInt64 rem = p->limit - p->processed;
    if (size2 > rem)
      size2 = (size_t)rem;
  }
  if (size2 != 0)
  {
    if (p->realStream)
    {
      res = ISeqInStream_Read(p->realStream, data, &size2);
      p->realStreamFinished = (size2 == 0) ? 1 : 0;
    }
    else
      memcpy(data, p->data + (size_t)p->processed, size2);
    XzCheck_Update(&p->check, data, size2);
    p->processed += size2;
  }
  *size = size2;
  return res;
}


/* ---------- CSeqSizeOutStream ---------- */

typedef struct
{
  ISeqOutStream vt;
  ISeqOutStreamPtr realStream;
  Byte *outBuf;
  size_t outBufLimit;
  UInt64 processed;
} CSeqSizeOutStream;

static size_t SeqSizeOutStream_Write(ISeqOutStreamPtr pp, const void *data, size_t size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeqSizeOutStream)
  if (p->realStream)
    size = ISeqOutStream_Write(p->realStream, data, size);
  else
  {
    if (size > p->outBufLimit - (size_t)p->processed)
      return 0;
    memcpy(p->outBuf + (size_t)p->processed, data, size);
  }
  p->processed += size;
  return size;
}


/* ---------- CSeqInFilter ---------- */

#define FILTER_BUF_SIZE (1 << 20)

typedef struct
{
  ISeqInStream vt;
  ISeqInStreamPtr realStream;
  IStateCoder StateCoder;
  Byte *buf;
  size_t curPos;
  size_t endPos;
  int srcWasFinished;
} CSeqInFilter;


static const z7_Func_BranchConv g_Funcs_BranchConv_RISC_Enc[] =
{
  Z7_BRANCH_CONV_ENC_2 (BranchConv_PPC),
  Z7_BRANCH_CONV_ENC_2 (BranchConv_IA64),
  Z7_BRANCH_CONV_ENC_2 (BranchConv_ARM),
  Z7_BRANCH_CONV_ENC_2 (BranchConv_ARMT),
  Z7_BRANCH_CONV_ENC_2 (BranchConv_SPARC),
  Z7_BRANCH_CONV_ENC_2 (BranchConv_ARM64),
  Z7_BRANCH_CONV_ENC_2 (BranchConv_RISCV)
};

static SizeT XzBcFilterStateBase_Filter_Enc(CXzBcFilterStateBase *p, Byte *data, SizeT size)
{
  switch (p->methodId)
  {
    case XZ_ID_Delta:
      Delta_Encode(p->delta_State, p->delta, data, size);
      break;
    case XZ_ID_X86:
      size = (SizeT)(z7_BranchConvSt_X86_Enc(data, size, p->ip, &p->X86_State) - data);
      break;
    default:
      if (p->methodId >= XZ_ID_PPC)
      {
        const UInt32 i = p->methodId - XZ_ID_PPC;
        if (i < Z7_ARRAY_SIZE(g_Funcs_BranchConv_RISC_Enc))
          size = (SizeT)(g_Funcs_BranchConv_RISC_Enc[i](data, size, p->ip) - data);
      }
      break;
  }
  p->ip += (UInt32)size;
  return size;
}


static SRes SeqInFilter_Init(CSeqInFilter *p, const CXzFilter *props, ISzAllocPtr alloc)
{
  if (!p->buf)
  {
    p->buf = (Byte *)ISzAlloc_Alloc(alloc, FILTER_BUF_SIZE);
    if (!p->buf)
      return SZ_ERROR_MEM;
  }
  p->curPos = p->endPos = 0;
  p->srcWasFinished = 0;
  RINOK(Xz_StateCoder_Bc_SetFromMethod_Func(&p->StateCoder, props->id, XzBcFilterStateBase_Filter_Enc, alloc))
  RINOK(p->StateCoder.SetProps(p->StateCoder.p, props->props, props->propsSize, alloc))
  p->StateCoder.Init(p->StateCoder.p);
  return SZ_OK;
}


static SRes SeqInFilter_Read(ISeqInStreamPtr pp, void *data, size_t *size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeqInFilter)
  const size_t sizeOriginal = *size;
  if (sizeOriginal == 0)
    return SZ_OK;
  *size = 0;
  
  for (;;)
  {
    if (!p->srcWasFinished && p->curPos == p->endPos)
    {
      p->curPos = 0;
      p->endPos = FILTER_BUF_SIZE;
      RINOK(ISeqInStream_Read(p->realStream, p->buf, &p->endPos))
      if (p->endPos == 0)
        p->srcWasFinished = 1;
    }
    {
      SizeT srcLen = p->endPos - p->curPos;
      ECoderStatus status;
      SRes res;
      *size = sizeOriginal;
      res = p->StateCoder.Code2(p->StateCoder.p,
          (Byte *)data, size,
          p->buf + p->curPos, &srcLen,
          p->srcWasFinished, CODER_FINISH_ANY,
          &status);
      p->curPos += srcLen;
      if (*size != 0 || srcLen == 0 || res != SZ_OK)
        return res;
    }
  }
}

Z7_FORCE_INLINE
static void SeqInFilter_Construct(CSeqInFilter *p)
{
  p->buf = NULL;
  p->StateCoder.p = NULL;
  p->vt.Read = SeqInFilter_Read;
}

Z7_FORCE_INLINE
static void SeqInFilter_Free(CSeqInFilter *p, ISzAllocPtr alloc)
{
  if (p->StateCoder.p)
  {
    p->StateCoder.Free(p->StateCoder.p, alloc);
    p->StateCoder.p = NULL;
  }
  if (p->buf)
  {
    ISzAlloc_Free(alloc, p->buf);
    p->buf = NULL;
  }
}


/* ---------- CSbEncInStream ---------- */

#ifdef USE_SUBBLOCK

typedef struct
{
  ISeqInStream vt;
  ISeqInStreamPtr inStream;
  CSbEnc enc;
} CSbEncInStream;

static SRes SbEncInStream_Read(ISeqInStreamPtr pp, void *data, size_t *size)
{
  CSbEncInStream *p = Z7_CONTAINER_FROM_VTBL(pp, CSbEncInStream, vt);
  size_t sizeOriginal = *size;
  if (sizeOriginal == 0)
    return SZ_OK;
  
  for (;;)
  {
    if (p->enc.needRead && !p->enc.readWasFinished)
    {
      size_t processed = p->enc.needReadSizeMax;
      RINOK(p->inStream->Read(p->inStream, p->enc.buf + p->enc.readPos, &processed))
      p->enc.readPos += processed;
      if (processed == 0)
      {
        p->enc.readWasFinished = True;
        p->enc.isFinalFinished = True;
      }
      p->enc.needRead = False;
    }
  
    *size = sizeOriginal;
    RINOK(SbEnc_Read(&p->enc, data, size))
    if (*size != 0 || !p->enc.needRead)
      return SZ_OK;
  }
}

void SbEncInStream_Construct(CSbEncInStream *p, ISzAllocPtr alloc)
{
  SbEnc_Construct(&p->enc, alloc);
  p->vt.Read = SbEncInStream_Read;
}

SRes SbEncInStream_Init(CSbEncInStream *p)
{
  return SbEnc_Init(&p->enc);
}

void SbEncInStream_Free(CSbEncInStream *p)
{
  SbEnc_Free(&p->enc);
}

#endif



/* ---------- CXzProps ---------- */


void XzFilterProps_Init(CXzFilterProps *p)
{
  p->id = 0;
  p->delta = 0;
  p->ip = 0;
  p->ipDefined = False;
}

void XzProps_Init(CXzProps *p)
{
  p->checkId = XZ_CHECK_CRC32;
  p->numThreadGroups = 0;
  p->blockSize = XZ_PROPS_BLOCK_SIZE_AUTO;
  p->numBlockThreads_Reduced = -1;
  p->numBlockThreads_Max = -1;
  p->numTotalThreads = -1;
  p->reduceSize = (UInt64)(Int64)-1;
  p->forceWriteSizesInHeader = 0;
  // p->forceWriteSizesInHeader = 1;

  XzFilterProps_Init(&p->filterProps);
  Lzma2EncProps_Init(&p->lzma2Props);
}


static void XzEncProps_Normalize_Fixed(CXzProps *p)
{
  UInt64 fileSize;
  int t1, t1n, t2, t2r, t3;
  {
    CLzma2EncProps tp = p->lzma2Props;
    if (tp.numTotalThreads <= 0)
      tp.numTotalThreads = p->numTotalThreads;
    Lzma2EncProps_Normalize(&tp);
    t1n = tp.numTotalThreads;
  }

  t1 = p->lzma2Props.numTotalThreads;
  t2 = p->numBlockThreads_Max;
  t3 = p->numTotalThreads;

  if (t2 > MTCODER_THREADS_MAX)
    t2 = MTCODER_THREADS_MAX;

  if (t3 <= 0)
  {
    if (t2 <= 0)
      t2 = 1;
    t3 = t1n * t2;
  }
  else if (t2 <= 0)
  {
    t2 = t3 / t1n;
    if (t2 == 0)
    {
      t1 = 1;
      t2 = t3;
    }
    if (t2 > MTCODER_THREADS_MAX)
      t2 = MTCODER_THREADS_MAX;
  }
  else if (t1 <= 0)
  {
    t1 = t3 / t2;
    if (t1 == 0)
      t1 = 1;
  }
  else
    t3 = t1n * t2;

  p->lzma2Props.numTotalThreads = t1;

  t2r = t2;

  fileSize = p->reduceSize;

  if ((p->blockSize < fileSize || fileSize == (UInt64)(Int64)-1))
    p->lzma2Props.lzmaProps.reduceSize = p->blockSize;

  Lzma2EncProps_Normalize(&p->lzma2Props);

  t1 = p->lzma2Props.numTotalThreads;

  {
    if (t2 > 1 && fileSize != (UInt64)(Int64)-1)
    {
      UInt64 numBlocks = fileSize / p->blockSize;
      if (numBlocks * p->blockSize != fileSize)
        numBlocks++;
      if (numBlocks < (unsigned)t2)
      {
        t2r = (int)numBlocks;
        if (t2r == 0)
          t2r = 1;
        t3 = t1 * t2r;
      }
    }
  }
  
  p->numBlockThreads_Max = t2;
  p->numBlockThreads_Reduced = t2r;
  p->numTotalThreads = t3;
}


static void XzProps_Normalize(CXzProps *p)
{
  /* we normalize xzProps properties, but we normalize only some of CXzProps::lzma2Props properties.
     Lzma2Enc_SetProps() will normalize lzma2Props later. */
  
  if (p->blockSize == XZ_PROPS_BLOCK_SIZE_SOLID)
  {
    p->lzma2Props.lzmaProps.reduceSize = p->reduceSize;
    p->numBlockThreads_Reduced = 1;
    p->numBlockThreads_Max = 1;
    if (p->lzma2Props.numTotalThreads <= 0)
      p->lzma2Props.numTotalThreads = p->numTotalThreads;
    return;
  }
  else
  {
    CLzma2EncProps *lzma2 = &p->lzma2Props;
    if (p->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO)
    {
      // xz-auto
      p->lzma2Props.lzmaProps.reduceSize = p->reduceSize;

      if (lzma2->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID)
      {
        // if (xz-auto && lzma2-solid) - we use solid for both
        p->blockSize = XZ_PROPS_BLOCK_SIZE_SOLID;
        p->numBlockThreads_Reduced = 1;
        p->numBlockThreads_Max = 1;
        if (p->lzma2Props.numTotalThreads <= 0)
          p->lzma2Props.numTotalThreads = p->numTotalThreads;
      }
      else
      {
        // if (xz-auto && (lzma2-auto || lzma2-fixed_)
        //   we calculate block size for lzma2 and use that block size for xz, lzma2 uses single-chunk per block
        CLzma2EncProps tp = p->lzma2Props;
        if (tp.numTotalThreads <= 0)
          tp.numTotalThreads = p->numTotalThreads;
        
        Lzma2EncProps_Normalize(&tp);
        
        p->blockSize = tp.blockSize; // fixed or solid
        p->numBlockThreads_Reduced = tp.numBlockThreads_Reduced;
        p->numBlockThreads_Max = tp.numBlockThreads_Max;
        if (lzma2->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO)
          lzma2->blockSize = tp.blockSize; // fixed or solid, LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID
        if (lzma2->lzmaProps.reduceSize > tp.blockSize && tp.blockSize != LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID)
          lzma2->lzmaProps.reduceSize = tp.blockSize;
        lzma2->numBlockThreads_Reduced = 1;
        lzma2->numBlockThreads_Max = 1;
        return;
      }
    }
    else
    {
      // xz-fixed
      // we can use xz::reduceSize or xz::blockSize as base for lzmaProps::reduceSize
      
      p->lzma2Props.lzmaProps.reduceSize = p->reduceSize;
      {
        UInt64 r = p->reduceSize;
        if (r > p->blockSize || r == (UInt64)(Int64)-1)
          r = p->blockSize;
        lzma2->lzmaProps.reduceSize = r;
      }
      if (lzma2->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO)
        lzma2->blockSize = LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID;
      else if (lzma2->blockSize > p->blockSize && lzma2->blockSize != LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID)
        lzma2->blockSize = p->blockSize;
      
      XzEncProps_Normalize_Fixed(p);
    }
  }
}


/* ---------- CLzma2WithFilters ---------- */

typedef struct
{
  CLzma2EncHandle lzma2;
  CSeqInFilter filter;

  #ifdef USE_SUBBLOCK
  CSbEncInStream sb;
  #endif
} CLzma2WithFilters;


Z7_FORCE_INLINE
static void Lzma2WithFilters_Construct(CLzma2WithFilters *p)
{
  p->lzma2 = NULL;
  SeqInFilter_Construct(&p->filter);

  #ifdef USE_SUBBLOCK
  SbEncInStream_Construct(&p->sb, alloc);
  #endif
}


static SRes Lzma2WithFilters_Create(CLzma2WithFilters *p, ISzAllocPtr alloc, ISzAllocPtr bigAlloc)
{
  if (!p->lzma2)
  {
    p->lzma2 = Lzma2Enc_Create(alloc, bigAlloc);
    if (!p->lzma2)
      return SZ_ERROR_MEM;
  }
  return SZ_OK;
}


Z7_FORCE_INLINE
static void Lzma2WithFilters_Free(CLzma2WithFilters *p, ISzAllocPtr alloc)
{
  #ifdef USE_SUBBLOCK
  SbEncInStream_Free(&p->sb);
  #endif

  SeqInFilter_Free(&p->filter, alloc);
  if (p->lzma2)
  {
    Lzma2Enc_Destroy(p->lzma2);
    p->lzma2 = NULL;
  }
}


typedef struct
{
  UInt64 unpackSize;
  UInt64 totalSize;
  size_t headerSize;
} CXzEncBlockInfo;


static SRes Xz_CompressBlock(
    CLzma2WithFilters *lzmaf,
    
    ISeqOutStreamPtr outStream,
    Byte *outBufHeader,
    Byte *outBufData, size_t outBufDataLimit,

    ISeqInStreamPtr inStream,
    // UInt64 expectedSize,
    const Byte *inBuf, // used if (!inStream)
    size_t inBufSize,  // used if (!inStream), it's block size, props->blockSize is ignored

    const CXzProps *props,
    ICompressProgressPtr progress,
    int *inStreamFinished,  /* only for inStream version */
    CXzEncBlockInfo *blockSizes,
    ISzAllocPtr alloc,
    ISzAllocPtr allocBig)
{
  CSeqCheckInStream checkInStream;
  CSeqSizeOutStream seqSizeOutStream;
  CXzBlock block;
  unsigned filterIndex = 0;
  CXzFilter *filter = NULL;
  const CXzFilterProps *fp = &props->filterProps;
  if (fp->id == 0)
    fp = NULL;
  
  *inStreamFinished = False;
  
  RINOK(Lzma2WithFilters_Create(lzmaf, alloc, allocBig))
  
  RINOK(Lzma2Enc_SetProps(lzmaf->lzma2, &props->lzma2Props))
  
  // XzBlock_ClearFlags(&block)
  XzBlock_ClearFlags_SetNumFilters(&block, 1 + (fp ? 1 : 0))
  
  if (fp)
  {
    filter = &block.filters[filterIndex++];
    filter->id = fp->id;
    filter->propsSize = 0;
    
    if (fp->id == XZ_ID_Delta)
    {
      filter->props[0] = (Byte)(fp->delta - 1);
      filter->propsSize = 1;
    }
    else if (fp->ipDefined)
    {
      Byte *ptr = filter->props;
      SetUi32(ptr, fp->ip)
      filter->propsSize = 4;
    }
  }
  
  {
    CXzFilter *f = &block.filters[filterIndex++];
    f->id = XZ_ID_LZMA2;
    f->propsSize = 1;
    f->props[0] = Lzma2Enc_WriteProperties(lzmaf->lzma2);
  }
  
  seqSizeOutStream.vt.Write = SeqSizeOutStream_Write;
  seqSizeOutStream.realStream = outStream;
  seqSizeOutStream.outBuf = outBufData;
  seqSizeOutStream.outBufLimit = outBufDataLimit;
  seqSizeOutStream.processed = 0;
    
  /*
  if (expectedSize != (UInt64)(Int64)-1)
  {
    block.unpackSize = expectedSize;
    if (props->blockSize != (UInt64)(Int64)-1)
      if (expectedSize > props->blockSize)
        block.unpackSize = props->blockSize;
    XzBlock_SetHasUnpackSize(&block)
  }
  */

  if (outStream)
  {
    RINOK(XzBlock_WriteHeader(&block, &seqSizeOutStream.vt))
  }
  
  checkInStream.vt.Read = SeqCheckInStream_Read;
  SeqCheckInStream_Init(&checkInStream, props->checkId);
  
  checkInStream.realStream = inStream;
  checkInStream.data = inBuf;
  checkInStream.limit = props->blockSize;
  if (!inStream)
    checkInStream.limit = inBufSize;

  if (fp)
  {
    #ifdef USE_SUBBLOCK
    if (fp->id == XZ_ID_Subblock)
    {
      lzmaf->sb.inStream = &checkInStream.vt;
      RINOK(SbEncInStream_Init(&lzmaf->sb))
    }
    else
    #endif
    {
      lzmaf->filter.realStream = &checkInStream.vt;
      RINOK(SeqInFilter_Init(&lzmaf->filter, filter, alloc))
    }
  }

  {
    SRes res;
    Byte *outBuf = NULL;
    size_t outSize = 0;
    BoolInt useStream = (fp || inStream);
    // useStream = True;
    
    if (!useStream)
    {
      XzCheck_Update(&checkInStream.check, inBuf, inBufSize);
      checkInStream.processed = inBufSize;
    }
    
    if (!outStream)
    {
      outBuf = seqSizeOutStream.outBuf; //  + (size_t)seqSizeOutStream.processed;
      outSize = seqSizeOutStream.outBufLimit; // - (size_t)seqSizeOutStream.processed;
    }
    
    res = Lzma2Enc_Encode2(lzmaf->lzma2,
        outBuf ? NULL : &seqSizeOutStream.vt,
        outBuf,
        outBuf ? &outSize : NULL,
      
        useStream ?
          (fp ?
            (
            #ifdef USE_SUBBLOCK
            (fp->id == XZ_ID_Subblock) ? &lzmaf->sb.vt:
            #endif
            &lzmaf->filter.vt) :
            &checkInStream.vt) : NULL,
      
        useStream ? NULL : inBuf,
        useStream ? 0 : inBufSize,
        
        progress);
    
    if (outBuf)
      seqSizeOutStream.processed += outSize;
    
    RINOK(res)
    blockSizes->unpackSize = checkInStream.processed;
  }
  {
    Byte buf[4 + XZ_CHECK_SIZE_MAX];
    const unsigned padSize = XZ_GET_PAD_SIZE(seqSizeOutStream.processed);
    const UInt64 packSize = seqSizeOutStream.processed;
    
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    
    SeqCheckInStream_GetDigest(&checkInStream, buf + 4);
    RINOK(WriteBytes(&seqSizeOutStream.vt, buf + (4 - padSize),
        padSize + XzFlags_GetCheckSize((CXzStreamFlags)props->checkId)))
    
    blockSizes->totalSize = seqSizeOutStream.processed - padSize;
    
    if (!outStream)
    {
      seqSizeOutStream.outBuf = outBufHeader;
      seqSizeOutStream.outBufLimit = XZ_BLOCK_HEADER_SIZE_MAX;
      seqSizeOutStream.processed = 0;
      
      block.unpackSize = blockSizes->unpackSize;
      XzBlock_SetHasUnpackSize(&block)
      
      block.packSize = packSize;
      XzBlock_SetHasPackSize(&block)
      
      RINOK(XzBlock_WriteHeader(&block, &seqSizeOutStream.vt))
      
      blockSizes->headerSize = (size_t)seqSizeOutStream.processed;
      blockSizes->totalSize += seqSizeOutStream.processed;
    }
  }
  
  if (inStream)
    *inStreamFinished = checkInStream.realStreamFinished;
  else
  {
    *inStreamFinished = False;
    if (checkInStream.processed != inBufSize)
      return SZ_ERROR_FAIL;
  }

  return SZ_OK;
}



typedef struct
{
  ICompressProgress vt;
  ICompressProgressPtr progress;
  UInt64 inOffset;
  UInt64 outOffset;
} CCompressProgress_XzEncOffset;


static SRes CompressProgress_XzEncOffset_Progress(ICompressProgressPtr pp, UInt64 inSize, UInt64 outSize)
{
  const CCompressProgress_XzEncOffset *p = Z7_CONTAINER_FROM_VTBL_CONST(pp, CCompressProgress_XzEncOffset, vt);
  inSize += p->inOffset;
  outSize += p->outOffset;
  return ICompressProgress_Progress(p->progress, inSize, outSize);
}




struct CXzEnc
{
  ISzAllocPtr alloc;
  ISzAllocPtr allocBig;

  CXzProps xzProps;
  UInt64 expectedDataSize;

  CXzEncIndex xzIndex;

  CLzma2WithFilters lzmaf_Items[MTCODER_THREADS_MAX];
  
  size_t outBufSize;       /* size of allocated outBufs[i] */
  Byte *outBufs[MTCODER_BLOCKS_MAX];

  #ifndef Z7_ST
  unsigned checkType;
  ISeqOutStreamPtr outStream;
  BoolInt mtCoder_WasConstructed;
  CMtCoder mtCoder;
  CXzEncBlockInfo EncBlocks[MTCODER_BLOCKS_MAX];
  #endif
};


static void XzEnc_Construct(CXzEnc *p)
{
  unsigned i;

  XzEncIndex_Construct(&p->xzIndex);

  for (i = 0; i < MTCODER_THREADS_MAX; i++)
    Lzma2WithFilters_Construct(&p->lzmaf_Items[i]);

  #ifndef Z7_ST
  p->mtCoder_WasConstructed = False;
  {
    for (i = 0; i < MTCODER_BLOCKS_MAX; i++)
      p->outBufs[i] = NULL;
    p->outBufSize = 0;
  }
  #endif
}


static void XzEnc_FreeOutBufs(CXzEnc *p)
{
  unsigned i;
  for (i = 0; i < MTCODER_BLOCKS_MAX; i++)
    if (p->outBufs[i])
    {
      ISzAlloc_Free(p->alloc, p->outBufs[i]);
      p->outBufs[i] = NULL;
    }
  p->outBufSize = 0;
}


static void XzEnc_Free(CXzEnc *p, ISzAllocPtr alloc)
{
  unsigned i;

  XzEncIndex_Free(&p->xzIndex, alloc);

  for (i = 0; i < MTCODER_THREADS_MAX; i++)
    Lzma2WithFilters_Free(&p->lzmaf_Items[i], alloc);
  
  #ifndef Z7_ST
  if (p->mtCoder_WasConstructed)
  {
    MtCoder_Destruct(&p->mtCoder);
    p->mtCoder_WasConstructed = False;
  }
  XzEnc_FreeOutBufs(p);
  #endif
}


CXzEncHandle XzEnc_Create(ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  CXzEnc *p = (CXzEnc *)ISzAlloc_Alloc(alloc, sizeof(CXzEnc));
  if (!p)
    return NULL;
  XzEnc_Construct(p);
  XzProps_Init(&p->xzProps);
  XzProps_Normalize(&p->xzProps);
  p->expectedDataSize = (UInt64)(Int64)-1;
  p->alloc = alloc;
  p->allocBig = allocBig;
  return (CXzEncHandle)p;
}

// #define GET_CXzEnc_p  CXzEnc *p = (CXzEnc *)(void *)pp;

void XzEnc_Destroy(CXzEncHandle p)
{
  // GET_CXzEnc_p
  XzEnc_Free(p, p->alloc);
  ISzAlloc_Free(p->alloc, p);
}


SRes XzEnc_SetProps(CXzEncHandle p, const CXzProps *props)
{
  // GET_CXzEnc_p
  p->xzProps = *props;
  XzProps_Normalize(&p->xzProps);
  return SZ_OK;
}


void XzEnc_SetDataSize(CXzEncHandle p, UInt64 expectedDataSiize)
{
  // GET_CXzEnc_p
  p->expectedDataSize = expectedDataSiize;
}




#ifndef Z7_ST

static SRes XzEnc_MtCallback_Code(void *pp, unsigned coderIndex, unsigned outBufIndex,
    const Byte *src, size_t srcSize, int finished)
{
  CXzEnc *me = (CXzEnc *)pp;
  SRes res;
  CMtProgressThunk progressThunk;
  Byte *dest;
  UNUSED_VAR(finished)
  {
    CXzEncBlockInfo *bInfo = &me->EncBlocks[outBufIndex];
    bInfo->totalSize = 0;
    bInfo->unpackSize = 0;
    bInfo->headerSize = 0;
    // v23.02: we don't compress empty blocks
    // also we must ignore that empty block in XzEnc_MtCallback_Write()
    if (srcSize == 0)
      return SZ_OK;
  }
  dest = me->outBufs[outBufIndex];
  if (!dest)
  {
    dest = (Byte *)ISzAlloc_Alloc(me->alloc, me->outBufSize);
    if (!dest)
      return SZ_ERROR_MEM;
    me->outBufs[outBufIndex] = dest;
  }
  
  MtProgressThunk_CreateVTable(&progressThunk);
  progressThunk.mtProgress = &me->mtCoder.mtProgress;
  MtProgressThunk_INIT(&progressThunk)

  {
    CXzEncBlockInfo blockSizes;
    int inStreamFinished;

    res = Xz_CompressBlock(
        &me->lzmaf_Items[coderIndex],
        
        NULL,
        dest,
        dest + XZ_BLOCK_HEADER_SIZE_MAX, me->outBufSize - XZ_BLOCK_HEADER_SIZE_MAX,

        NULL,
        // srcSize, // expectedSize
        src, srcSize,

        &me->xzProps,
        &progressThunk.vt,
        &inStreamFinished,
        &blockSizes,
        me->alloc,
        me->allocBig);
    
    if (res == SZ_OK)
      me->EncBlocks[outBufIndex] = blockSizes;

    return res;
  }
}


static SRes XzEnc_MtCallback_Write(void *pp, unsigned outBufIndex)
{
  CXzEnc *me = (CXzEnc *)pp;
  const CXzEncBlockInfo *bInfo = &me->EncBlocks[outBufIndex];
  // v23.02: we don't write empty blocks
  // note: if (bInfo->unpackSize == 0) then there is no compressed data of block
  if (bInfo->unpackSize == 0)
    return SZ_OK;
  {
    const Byte *data = me->outBufs[outBufIndex];
    RINOK(WriteBytes(me->outStream, data, bInfo->headerSize))
    {
      const UInt64 totalPackFull = bInfo->totalSize + XZ_GET_PAD_SIZE(bInfo->totalSize);
      RINOK(WriteBytes(me->outStream, data + XZ_BLOCK_HEADER_SIZE_MAX, (size_t)totalPackFull - bInfo->headerSize))
    }
    return XzEncIndex_AddIndexRecord(&me->xzIndex, bInfo->unpackSize, bInfo->totalSize, me->alloc);
  }
}

#endif



SRes XzEnc_Encode(CXzEncHandle p, ISeqOutStreamPtr outStream, ISeqInStreamPtr inStream, ICompressProgressPtr progress)
{
  // GET_CXzEnc_p

  const CXzProps *props = &p->xzProps;

  XzEncIndex_Init(&p->xzIndex);
  {
    UInt64 numBlocks = 1;
    UInt64 blockSize = props->blockSize;
    
    if (blockSize != XZ_PROPS_BLOCK_SIZE_SOLID
        && props->reduceSize != (UInt64)(Int64)-1)
    {
      numBlocks = props->reduceSize / blockSize;
      if (numBlocks * blockSize != props->reduceSize)
        numBlocks++;
    }
    else
      blockSize = (UInt64)1 << 62;
    
    RINOK(XzEncIndex_PreAlloc(&p->xzIndex, numBlocks, blockSize, XZ_GET_ESTIMATED_BLOCK_TOTAL_PACK_SIZE(blockSize), p->alloc))
  }

  RINOK(Xz_WriteHeader((CXzStreamFlags)props->checkId, outStream))


  #ifndef Z7_ST
  if (props->numBlockThreads_Reduced > 1)
  {
    IMtCoderCallback2 vt;

    if (!p->mtCoder_WasConstructed)
    {
      p->mtCoder_WasConstructed = True;
      MtCoder_Construct(&p->mtCoder);
    }

    vt.Code = XzEnc_MtCallback_Code;
    vt.Write = XzEnc_MtCallback_Write;

    p->checkType = props->checkId;
    p->xzProps = *props;
    
    p->outStream = outStream;

    p->mtCoder.allocBig = p->allocBig;
    p->mtCoder.progress = progress;
    p->mtCoder.inStream = inStream;
    p->mtCoder.inData = NULL;
    p->mtCoder.inDataSize = 0;
    p->mtCoder.mtCallback = &vt;
    p->mtCoder.mtCallbackObject = p;

    if (   props->blockSize == XZ_PROPS_BLOCK_SIZE_SOLID
        || props->blockSize == XZ_PROPS_BLOCK_SIZE_AUTO)
      return SZ_ERROR_FAIL;

    p->mtCoder.blockSize = (size_t)props->blockSize;
    if (p->mtCoder.blockSize != props->blockSize)
      return SZ_ERROR_PARAM; /* SZ_ERROR_MEM */

    {
      size_t destBlockSize = XZ_BLOCK_HEADER_SIZE_MAX + XZ_GET_MAX_BLOCK_PACK_SIZE(p->mtCoder.blockSize);
      if (destBlockSize < p->mtCoder.blockSize)
        return SZ_ERROR_PARAM;
      if (p->outBufSize != destBlockSize)
        XzEnc_FreeOutBufs(p);
      p->outBufSize = destBlockSize;
    }

    p->mtCoder.numThreadsMax = (unsigned)props->numBlockThreads_Max;
    p->mtCoder.numThreadGroups = props->numThreadGroups;
    p->mtCoder.expectedDataSize = p->expectedDataSize;
    
    RINOK(MtCoder_Code(&p->mtCoder))
  }
  else
  #endif
  {
    int writeStartSizes;
    CCompressProgress_XzEncOffset progress2;
    Byte *bufData = NULL;
    size_t bufSize = 0;

    progress2.vt.Progress = CompressProgress_XzEncOffset_Progress;
    progress2.inOffset = 0;
    progress2.outOffset = 0;
    progress2.progress = progress;
    
    writeStartSizes = 0;
    
    if (props->blockSize != XZ_PROPS_BLOCK_SIZE_SOLID)
    {
      writeStartSizes = (props->forceWriteSizesInHeader > 0);
      
      if (writeStartSizes)
      {
        size_t t2;
        size_t t = (size_t)props->blockSize;
        if (t != props->blockSize)
          return SZ_ERROR_PARAM;
        t = XZ_GET_MAX_BLOCK_PACK_SIZE(t);
        if (t < props->blockSize)
          return SZ_ERROR_PARAM;
        t2 = XZ_BLOCK_HEADER_SIZE_MAX + t;
        if (!p->outBufs[0] || t2 != p->outBufSize)
        {
          XzEnc_FreeOutBufs(p);
          p->outBufs[0] = (Byte *)ISzAlloc_Alloc(p->alloc, t2);
          if (!p->outBufs[0])
            return SZ_ERROR_MEM;
          p->outBufSize = t2;
        }
        bufData = p->outBufs[0] + XZ_BLOCK_HEADER_SIZE_MAX;
        bufSize = t;
      }
    }
    
    for (;;)
    {
      CXzEncBlockInfo blockSizes;
      int inStreamFinished;
      
      /*
      UInt64 rem = (UInt64)(Int64)-1;
      if (props->reduceSize != (UInt64)(Int64)-1
          && props->reduceSize >= progress2.inOffset)
        rem = props->reduceSize - progress2.inOffset;
      */

      blockSizes.headerSize = 0; // for GCC
      
      RINOK(Xz_CompressBlock(
          &p->lzmaf_Items[0],
          
          writeStartSizes ? NULL : outStream,
          writeStartSizes ? p->outBufs[0] : NULL,
          bufData, bufSize,
          
          inStream,
          // rem,
          NULL, 0,
          
          props,
          progress ? &progress2.vt : NULL,
          &inStreamFinished,
          &blockSizes,
          p->alloc,
          p->allocBig))

      {
        UInt64 totalPackFull = blockSizes.totalSize + XZ_GET_PAD_SIZE(blockSizes.totalSize);
      
        if (writeStartSizes)
        {
          RINOK(WriteBytes(outStream, p->outBufs[0], blockSizes.headerSize))
          RINOK(WriteBytes(outStream, bufData, (size_t)totalPackFull - blockSizes.headerSize))
        }
        
        RINOK(XzEncIndex_AddIndexRecord(&p->xzIndex, blockSizes.unpackSize, blockSizes.totalSize, p->alloc))
        
        progress2.inOffset += blockSizes.unpackSize;
        progress2.outOffset += totalPackFull;
      }
        
      if (inStreamFinished)
        break;
    }
  }

  return XzEncIndex_WriteFooter(&p->xzIndex, (CXzStreamFlags)props->checkId, outStream);
}


// amalgamation: header emitted in prologue

SRes Xz_Encode(ISeqOutStreamPtr outStream, ISeqInStreamPtr inStream,
    const CXzProps *props, ICompressProgressPtr progress)
{
  SRes res;
  CXzEncHandle xz = XzEnc_Create(&g_Alloc, &g_BigAlloc);
  if (!xz)
    return SZ_ERROR_MEM;
  res = XzEnc_SetProps(xz, props);
  if (res == SZ_OK)
    res = XzEnc_Encode(xz, outStream, inStream, progress);
  XzEnc_Destroy(xz);
  return res;
}


SRes Xz_EncodeEmpty(ISeqOutStreamPtr outStream)
{
  SRes res;
  CXzEncIndex xzIndex;
  XzEncIndex_Construct(&xzIndex);
  res = Xz_WriteHeader((CXzStreamFlags)0, outStream);
  if (res == SZ_OK)
    res = XzEncIndex_WriteFooter(&xzIndex, (CXzStreamFlags)0, outStream);
  XzEncIndex_Free(&xzIndex, NULL); // g_Alloc
  return res;
}

/* ================ unit: C/XzIn.c ================ */
/* XzIn.c - Xz input
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define XZ_FOOTER_12B_ALIGNED16_SIG_CHECK(p) \
    (GetUi16a((const Byte *)(const void *)(p) + 10) == \
      (XZ_FOOTER_SIG_0 | (XZ_FOOTER_SIG_1 << 8)))

SRes Xz_ReadHeader(CXzStreamFlags *p, ISeqInStreamPtr inStream)
{
  UInt32 data32[XZ_STREAM_HEADER_SIZE / 4];
  size_t processedSize = XZ_STREAM_HEADER_SIZE;
  RINOK(SeqInStream_ReadMax(inStream, data32, &processedSize))
  if (processedSize != XZ_STREAM_HEADER_SIZE
      || memcmp(data32, XZ_SIG, XZ_SIG_SIZE) != 0)
    return SZ_ERROR_NO_ARCHIVE;
  return Xz_ParseHeader(p, (const Byte *)(const void *)data32);
}

#define READ_VARINT_AND_CHECK(buf, size, res) \
{ const unsigned s = Xz_ReadVarInt(buf, size, res); \
  if (s == 0) return SZ_ERROR_ARCHIVE; \
  size -= s; \
  buf += s; \
}

SRes XzBlock_ReadHeader(CXzBlock *p, ISeqInStreamPtr inStream, BoolInt *isIndex, UInt32 *headerSizeRes)
{
  MY_ALIGN(4)
  Byte header[XZ_BLOCK_HEADER_SIZE_MAX];
  unsigned headerSize;
  *headerSizeRes = 0;
  RINOK(SeqInStream_ReadByte(inStream, &header[0]))
  headerSize = header[0];
  if (headerSize == 0)
  {
    *headerSizeRes = 1;
    *isIndex = True;
    return SZ_OK;
  }

  *isIndex = False;
  headerSize = (headerSize << 2) + 4;
  *headerSizeRes = (UInt32)headerSize;
  {
    size_t processedSize = headerSize - 1;
    RINOK(SeqInStream_ReadMax(inStream, header + 1, &processedSize))
    if (processedSize != headerSize - 1)
      return SZ_ERROR_INPUT_EOF;
  }
  return XzBlock_Parse(p, header);
}


#define ADD_SIZE_CHECK(size, val) \
{ const UInt64 newSize = size + (val); \
  if (newSize < size) return XZ_SIZE_OVERFLOW; \
  size = newSize; \
}

UInt64 Xz_GetUnpackSize(const CXzStream *p)
{
  UInt64 size = 0;
  size_t i;
  for (i = 0; i < p->numBlocks; i++)
  {
    ADD_SIZE_CHECK(size, p->blocks[i].unpackSize)
  }
  return size;
}

UInt64 Xz_GetPackSize(const CXzStream *p)
{
  UInt64 size = 0;
  size_t i;
  for (i = 0; i < p->numBlocks; i++)
  {
    ADD_SIZE_CHECK(size, (p->blocks[i].totalSize + 3) & ~(UInt64)3)
  }
  return size;
}


// input;
//   CXzStream (p) is empty object.
//   size != 0
//   (size & 3) == 0
//   (buf) is aligned for at least 4 bytes.
// output:
//   p->numBlocks is number of allocated items in p->blocks
//   p->blocks[*] values must be ignored, if function returns error.
static SRes Xz_ParseIndex(CXzStream *p, const Byte *buf, size_t size, ISzAllocPtr alloc)
{
  size_t numBlocks;
  if (size < 5 || buf[0] != 0)
    return SZ_ERROR_ARCHIVE;
  size -= 4;
  {
    const UInt32 crc = CrcCalc(buf, size);
    if (crc != GetUi32a(buf + size))
      return SZ_ERROR_ARCHIVE;
  }
  buf++;
  size--;
  {
    UInt64 numBlocks64;
    READ_VARINT_AND_CHECK(buf, size, &numBlocks64)
    // (numBlocks64) is 63-bit value, so we can calculate (numBlocks64 * 2):
    if (numBlocks64 * 2 > size)
      return SZ_ERROR_ARCHIVE;
    if (numBlocks64 >= ((size_t)1 << (sizeof(size_t) * 8 - 1)) / sizeof(CXzBlockSizes))
      return SZ_ERROR_MEM; // SZ_ERROR_ARCHIVE
    numBlocks = (size_t)numBlocks64;
  }
  // Xz_Free(p, alloc); // it's optional, because (p) is empty already
  if (numBlocks)
  {
    CXzBlockSizes *blocks = (CXzBlockSizes *)ISzAlloc_Alloc(alloc, sizeof(CXzBlockSizes) * numBlocks);
    if (!blocks)
      return SZ_ERROR_MEM;
    p->blocks = blocks;
    p->numBlocks = numBlocks;
    // the caller will call Xz_Free() in case of error
    do
    {
      READ_VARINT_AND_CHECK(buf, size, &blocks->totalSize)
      READ_VARINT_AND_CHECK(buf, size, &blocks->unpackSize)
      if (blocks->totalSize == 0)
        return SZ_ERROR_ARCHIVE;
      blocks++;
    }
    while (--numBlocks);
  }
  if (size >= 4)
    return SZ_ERROR_ARCHIVE;
  while (size)
    if (buf[--size])
      return SZ_ERROR_ARCHIVE;
  return SZ_OK;
}


/*
static SRes Xz_ReadIndex(CXzStream *p, ILookInStreamPtr stream, UInt64 indexSize, ISzAllocPtr alloc)
{
  SRes res;
  size_t size;
  Byte *buf;
  if (indexSize >= ((size_t)1 << (sizeof(size_t) * 8 - 1)))
    return SZ_ERROR_MEM; // SZ_ERROR_ARCHIVE
  size = (size_t)indexSize;
  buf = (Byte *)ISzAlloc_Alloc(alloc, size);
  if (!buf)
    return SZ_ERROR_MEM;
  res = LookInStream_Read2(stream, buf, size, SZ_ERROR_UNSUPPORTED);
  if (res == SZ_OK)
    res = Xz_ParseIndex(p, buf, size, alloc);
  ISzAlloc_Free(alloc, buf);
  return res;
}
*/

static SRes LookInStream_SeekRead_ForArc(ILookInStreamPtr stream, UInt64 offset, void *buf, size_t size)
{
  RINOK(LookInStream_SeekTo(stream, offset))
  return LookInStream_Read(stream, buf, size);
  /* return LookInStream_Read2(stream, buf, size, SZ_ERROR_NO_ARCHIVE); */
}


/*
in:
  (*startOffset) is position in (stream) where xz_stream must be finished.
out:
  if returns SZ_OK, then (*startOffset) is position in stream that shows start of xz_stream.
*/
static SRes Xz_ReadBackward(CXzStream *p, ILookInStreamPtr stream, Int64 *startOffset, ISzAllocPtr alloc)
{
  #define TEMP_BUF_SIZE  (1 << 10)
  UInt32 buf32[TEMP_BUF_SIZE / 4];
  UInt64 pos = (UInt64)*startOffset;

  if ((pos & 3) || pos < XZ_STREAM_FOOTER_SIZE)
    return SZ_ERROR_NO_ARCHIVE;
  pos -= XZ_STREAM_FOOTER_SIZE;
  RINOK(LookInStream_SeekRead_ForArc(stream, pos, buf32, XZ_STREAM_FOOTER_SIZE))
  
  if (!XZ_FOOTER_12B_ALIGNED16_SIG_CHECK(buf32))
  {
    pos += XZ_STREAM_FOOTER_SIZE;
    for (;;)
    {
      // pos != 0
      // (pos & 3) == 0
      size_t i = pos >= TEMP_BUF_SIZE ? TEMP_BUF_SIZE : (size_t)pos;
      pos -= i;
      RINOK(LookInStream_SeekRead_ForArc(stream, pos, buf32, i))
      i /= 4;
      do
        if (buf32[i - 1] != 0)
          break;
      while (--i);

      pos += i * 4;
      #define XZ_STREAM_BACKWARD_READING_PAD_MAX (1 << 16)
      // here we don't support rare case with big padding for xz stream.
      // so we have padding limit for backward reading.
      if ((UInt64)*startOffset - pos > XZ_STREAM_BACKWARD_READING_PAD_MAX)
        return SZ_ERROR_NO_ARCHIVE;
      if (i)
        break;
    }
    // we try to open xz stream after skipping zero padding.
    // ((UInt64)*startOffset == pos) is possible here!
    if (pos < XZ_STREAM_FOOTER_SIZE)
      return SZ_ERROR_NO_ARCHIVE;
    pos -= XZ_STREAM_FOOTER_SIZE;
    RINOK(LookInStream_SeekRead_ForArc(stream, pos, buf32, XZ_STREAM_FOOTER_SIZE))
    if (!XZ_FOOTER_12B_ALIGNED16_SIG_CHECK(buf32))
      return SZ_ERROR_NO_ARCHIVE;
  }
  
  p->flags = (CXzStreamFlags)GetBe16a(buf32 + 2);
  if (!XzFlags_IsSupported(p->flags))
    return SZ_ERROR_UNSUPPORTED;
  {
    /* to eliminate GCC 6.3 warning:
       dereferencing type-punned pointer will break strict-aliasing rules */
    const UInt32 *buf_ptr = buf32;
    if (GetUi32a(buf_ptr) != CrcCalc(buf32 + 1, 6))
      return SZ_ERROR_ARCHIVE;
  }
  {
    const UInt64 indexSize = ((UInt64)GetUi32a(buf32 + 1) + 1) << 2;
    if (pos < indexSize)
      return SZ_ERROR_ARCHIVE;
    pos -= indexSize;
    // v25.00: relaxed indexSize check. We allow big index table.
    // if (indexSize > ((UInt32)1 << 31))
    if (indexSize >= ((size_t)1 << (sizeof(size_t) * 8 - 1)))
      return SZ_ERROR_MEM; // SZ_ERROR_ARCHIVE
    RINOK(LookInStream_SeekTo(stream, pos))
    // RINOK(Xz_ReadIndex(p, stream, indexSize, alloc))
    {
      SRes res;
      const size_t size = (size_t)indexSize;
      // if (size != indexSize) return SZ_ERROR_UNSUPPORTED;
      Byte *buf = (Byte *)ISzAlloc_Alloc(alloc, size);
      if (!buf)
        return SZ_ERROR_MEM;
      res = LookInStream_Read2(stream, buf, size, SZ_ERROR_UNSUPPORTED);
      if (res == SZ_OK)
        res = Xz_ParseIndex(p, buf, size, alloc);
      ISzAlloc_Free(alloc, buf);
      RINOK(res)
    }
  }
  {
    UInt64 total = Xz_GetPackSize(p);
    if (total == XZ_SIZE_OVERFLOW || total >= ((UInt64)1 << 63))
      return SZ_ERROR_ARCHIVE;
    total += XZ_STREAM_HEADER_SIZE;
    if (pos < total)
      return SZ_ERROR_ARCHIVE;
    pos -= total;
    RINOK(LookInStream_SeekTo(stream, pos))
    *startOffset = (Int64)pos;
  }
  {
    CXzStreamFlags headerFlags;
    CSecToRead secToRead;
    SecToRead_CreateVTable(&secToRead);
    secToRead.realStream = stream;
    RINOK(Xz_ReadHeader(&headerFlags, &secToRead.vt))
    return (p->flags == headerFlags) ? SZ_OK : SZ_ERROR_ARCHIVE;
  }
}


/* ---------- Xz Streams ---------- */

void Xzs_Construct(CXzs *p)
{
  Xzs_CONSTRUCT(p)
}

void Xzs_Free(CXzs *p, ISzAllocPtr alloc)
{
  size_t i;
  for (i = 0; i < p->num; i++)
    Xz_Free(&p->streams[i], alloc);
  ISzAlloc_Free(alloc, p->streams);
  p->num = p->numAllocated = 0;
  p->streams = NULL;
}

UInt64 Xzs_GetNumBlocks(const CXzs *p)
{
  UInt64 num = 0;
  size_t i;
  for (i = 0; i < p->num; i++)
    num += p->streams[i].numBlocks;
  return num;
}

UInt64 Xzs_GetUnpackSize(const CXzs *p)
{
  UInt64 size = 0;
  size_t i;
  for (i = 0; i < p->num; i++)
  {
    ADD_SIZE_CHECK(size, Xz_GetUnpackSize(&p->streams[i]))
  }
  return size;
}

/*
UInt64 Xzs_GetPackSize(const CXzs *p)
{
  UInt64 size = 0;
  size_t i;
  for (i = 0; i < p->num; i++)
  {
    ADD_SIZE_CHECK(size, Xz_GetTotalSize(&p->streams[i]))
  }
  return size;
}
*/

SRes Xzs_ReadBackward(CXzs *p, ILookInStreamPtr stream, Int64 *startOffset, ICompressProgressPtr progress, ISzAllocPtr alloc)
{
  Int64 endOffset = 0;
  // it's supposed that CXzs object is empty here.
  // if CXzs object is not empty, it will add new streams to that non-empty object.
  // Xzs_Free(p, alloc); // it's optional call to empty CXzs object.
  RINOK(ILookInStream_Seek(stream, &endOffset, SZ_SEEK_END))
  *startOffset = endOffset;
  for (;;)
  {
    CXzStream st;
    SRes res;
    Xz_CONSTRUCT(&st)
    res = Xz_ReadBackward(&st, stream, startOffset, alloc);
    // if (res == SZ_OK), then (*startOffset) is start offset of new stream if
    // if (res != SZ_OK), then (*startOffset) is unchend or it's expected start offset of stream with error
    st.startOffset = (UInt64)*startOffset;
    // we must store (st) object to array, or we must free (st) local object.
    if (res != SZ_OK)
    {
      Xz_Free(&st, alloc);
      return res;
    }
    if (p->num == p->numAllocated)
    {
      const size_t newNum = p->num + p->num / 4 + 1;
      void *data = ISzAlloc_Alloc(alloc, newNum * sizeof(CXzStream));
      if (!data)
      {
        Xz_Free(&st, alloc);
        return SZ_ERROR_MEM;
      }
      p->numAllocated = newNum;
      if (p->num != 0)
        memcpy(data, p->streams, p->num * sizeof(CXzStream));
      ISzAlloc_Free(alloc, p->streams);
      p->streams = (CXzStream *)data;
    }
    // we use direct copying of raw data from local variable (st) to object in array.
    // so we don't need to call Xz_Free(&st, alloc) after copying and after p->num++
    p->streams[p->num++] = st;
    if (*startOffset == 0)
      return SZ_OK;
    // seek operation is optional:
    // RINOK(LookInStream_SeekTo(stream, (UInt64)*startOffset))
    if (progress && ICompressProgress_Progress(progress, (UInt64)(endOffset - *startOffset), (UInt64)(Int64)-1) != SZ_OK)
      return SZ_ERROR_PROGRESS;
  }
}

/* ================ unit: C/ZstdDec.c ================ */
/* ZstdDec.c -- Zstd Decoder
2024-06-18 : the code was developed by Igor Pavlov, using Zstandard format
             specification and original zstd decoder code as reference code.
original zstd decoder code: Copyright (c) Facebook, Inc. All rights reserved.
This source code is licensed under BSD 3-Clause License.
*/

// amalgamation: header emitted in prologue

#include <string.h>
#include <stdlib.h>
// #include <stdio.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#if defined(MY_CPU_ARM64)
#include <arm_neon.h>
#endif

/* original-zstd still doesn't support window larger than 2 GiB.
   So we also limit our decoder for 2 GiB window: */
#if defined(MY_CPU_64BIT) && 0 == 1
  #define MAX_WINDOW_SIZE_LOG  41
#else
  #define MAX_WINDOW_SIZE_LOG  31
#endif

typedef
  #if MAX_WINDOW_SIZE_LOG < 32
    UInt32
  #else
    size_t
  #endif
    CZstdDecOffset;

// for debug: simpler and smaller code but slow:
// #define Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS

// #define SHOW_STAT
#ifdef SHOW_STAT
#include <stdio.h>
static unsigned g_Num_Blocks_Compressed = 0;
static unsigned g_Num_Blocks_memcpy = 0;
static unsigned g_Num_Wrap_memmove_Num = 0;
static unsigned g_Num_Wrap_memmove_Bytes = 0;
static unsigned g_NumSeqs_total = 0;
// static unsigned g_NumCopy = 0;
static unsigned g_NumOver = 0;
static unsigned g_NumOver2 = 0;
static unsigned g_Num_Match = 0;
static unsigned g_Num_Lits = 0;
static unsigned g_Num_LitsBig = 0;
static unsigned g_Num_Lit0 = 0;
static unsigned g_Num_Rep0 = 0;
static unsigned g_Num_Rep1 = 0;
static unsigned g_Num_Rep2 = 0;
static unsigned g_Num_Rep3 = 0;
static unsigned g_Num_Threshold_0 = 0;
static unsigned g_Num_Threshold_1 = 0;
static unsigned g_Num_Threshold_0sum = 0;
static unsigned g_Num_Threshold_1sum = 0;
#define STAT_UPDATE(v) v
#else
#define STAT_UPDATE(v)
#endif
#define STAT_INC(v)  STAT_UPDATE(v++;)


typedef struct
{
  const Byte *ptr;
  size_t len;
}
CInBufPair;


#if defined(MY_CPU_ARM_OR_ARM64) || defined(MY_CPU_X86_OR_AMD64)
  #if (defined(__clang__) && (__clang_major__ >= 6)) \
   || (defined(__GNUC__) && (__GNUC__ >= 6))
    // disable for debug:
    #define Z7_ZSTD_DEC_USE_BSR
  #elif defined(_MSC_VER) && (_MSC_VER >= 1300)
    // #if defined(MY_CPU_ARM_OR_ARM64)
    #if (_MSC_VER >= 1600)
      #include <intrin.h>
    #endif
    // disable for debug:
    #define Z7_ZSTD_DEC_USE_BSR
  #endif
#endif

#ifdef Z7_ZSTD_DEC_USE_BSR
  #if defined(__clang__) || defined(__GNUC__)
    #define MY_clz(x)  ((unsigned)__builtin_clz((UInt32)x))
  #else  // #if defined(_MSC_VER)
    #ifdef MY_CPU_ARM_OR_ARM64
      #define MY_clz  _CountLeadingZeros
    #endif // MY_CPU_X86_OR_AMD64
  #endif // _MSC_VER
#elif !defined(Z7_ZSTD_DEC_USE_LOG_TABLE)
  #define Z7_ZSTD_DEC_USE_LOG_TABLE
#endif


static
Z7_FORCE_INLINE
unsigned GetHighestSetBit_32_nonzero_big(UInt32 num)
{
  // (num != 0)
  #ifdef MY_clz
    return 31 - MY_clz(num);
  #elif defined(Z7_ZSTD_DEC_USE_BSR)
  {
    unsigned long zz;
    _BitScanReverse(&zz, num);
    return zz;
  }
  #else
  {
    int i = -1;
    for (;;)
    {
      i++;
      num >>= 1;
      if (num == 0)
        return (unsigned)i;
    }
  }
  #endif
}

#ifdef Z7_ZSTD_DEC_USE_LOG_TABLE

#define R1(a)  a, a
#define R2(a)  R1(a), R1(a)
#define R3(a)  R2(a), R2(a)
#define R4(a)  R3(a), R3(a)
#define R5(a)  R4(a), R4(a)
#define R6(a)  R5(a), R5(a)
#define R7(a)  R6(a), R6(a)
#define R8(a)  R7(a), R7(a)
#define R9(a)  R8(a), R8(a)

#define Z7_ZSTD_FSE_MAX_ACCURACY  9
// states[] values in FSE_Generate() can use (Z7_ZSTD_FSE_MAX_ACCURACY + 1) bits.
static const Byte k_zstd_LogTable[2 << Z7_ZSTD_FSE_MAX_ACCURACY] =
{
  R1(0), R1(1), R2(2), R3(3), R4(4), R5(5), R6(6), R7(7), R8(8), R9(9)
};

#define GetHighestSetBit_32_nonzero_small(num)  (k_zstd_LogTable[num])
#else
#define GetHighestSetBit_32_nonzero_small  GetHighestSetBit_32_nonzero_big
#endif


#ifdef MY_clz
  #define UPDATE_BIT_OFFSET_FOR_PADDING(b, bitOffset) \
    bitOffset -= (CBitCtr)(MY_clz(b) - 23);
#elif defined(Z7_ZSTD_DEC_USE_BSR)
  #define UPDATE_BIT_OFFSET_FOR_PADDING(b, bitOffset) \
    { unsigned long zz;  _BitScanReverse(&zz, b);  bitOffset -= 8;  bitOffset += zz; }
#else
  #define UPDATE_BIT_OFFSET_FOR_PADDING(b, bitOffset) \
    for (;;) { bitOffset--;  if (b & 0x80) { break; }  b <<= 1; }
#endif

#define SET_bitOffset_TO_PAD(bitOffset, src, srcLen) \
{ \
  unsigned lastByte = (src)[(size_t)(srcLen) - 1]; \
  if (lastByte == 0) return SZ_ERROR_DATA; \
  bitOffset = (CBitCtr)((srcLen) * 8); \
  UPDATE_BIT_OFFSET_FOR_PADDING(lastByte, bitOffset) \
}

#ifndef Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS

#define SET_bitOffset_TO_PAD_and_SET_BIT_SIZE(bitOffset, src, srcLen_res) \
{ \
  unsigned lastByte = (src)[(size_t)(srcLen_res) - 1]; \
  if (lastByte == 0) return SZ_ERROR_DATA; \
  srcLen_res *= 8; \
  bitOffset = (CBitCtr)srcLen_res; \
  UPDATE_BIT_OFFSET_FOR_PADDING(lastByte, bitOffset) \
}

#endif

/*
typedef Int32 CBitCtr_signed;
typedef Int32 CBitCtr;
*/
// /*
typedef ptrdiff_t CBitCtr_signed;
typedef ptrdiff_t CBitCtr;
// */


#define MATCH_LEN_MIN  3
#define kBlockSizeMax  (1u << 17)

// #define Z7_ZSTD_DEC_PRINT_TABLE

#ifdef Z7_ZSTD_DEC_PRINT_TABLE
#define NUM_OFFSET_SYMBOLS_PREDEF 29
#endif
#define NUM_OFFSET_SYMBOLS_MAX    (MAX_WINDOW_SIZE_LOG + 1)  // 32
#define NUM_LL_SYMBOLS            36
#define NUM_ML_SYMBOLS            53
#define FSE_NUM_SYMBOLS_MAX       53  // NUM_ML_SYMBOLS

// /*
#if !defined(MY_CPU_X86) || defined(__PIC__) || defined(MY_CPU_64BIT)
#define Z7_ZSTD_DEC_USE_BASES_IN_OBJECT
#endif
// */
// for debug:
// #define Z7_ZSTD_DEC_USE_BASES_LOCAL
// #define Z7_ZSTD_DEC_USE_BASES_IN_OBJECT

#define GLOBAL_TABLE(n)  k_ ## n

#if defined(Z7_ZSTD_DEC_USE_BASES_LOCAL)
  #define BASES_TABLE(n)  a_ ## n
#elif defined(Z7_ZSTD_DEC_USE_BASES_IN_OBJECT)
  #define BASES_TABLE(n)  p->m_ ## n
#else
  #define BASES_TABLE(n)  GLOBAL_TABLE(n)
#endif

#define Z7_ZSTD_DEC_USE_ML_PLUS3

#if defined(Z7_ZSTD_DEC_USE_BASES_LOCAL) || \
    defined(Z7_ZSTD_DEC_USE_BASES_IN_OBJECT)
  
#define SEQ_EXTRA_TABLES(n) \
  Byte   n ## SEQ_LL_EXTRA [NUM_LL_SYMBOLS]; \
  Byte   n ## SEQ_ML_EXTRA [NUM_ML_SYMBOLS]; \
  UInt32 n ## SEQ_LL_BASES [NUM_LL_SYMBOLS]; \
  UInt32 n ## SEQ_ML_BASES [NUM_ML_SYMBOLS]; \

#define Z7_ZSTD_DEC_USE_BASES_CALC

#ifdef Z7_ZSTD_DEC_USE_BASES_CALC

  #define FILL_LOC_BASES(n, startSum) \
    { unsigned i; UInt32 sum = startSum; \
      for (i = 0; i != Z7_ARRAY_SIZE(GLOBAL_TABLE(n ## _EXTRA)); i++) \
      { const unsigned a = GLOBAL_TABLE(n ## _EXTRA)[i]; \
        BASES_TABLE(n ## _BASES)[i] = sum; \
        /* if (sum != GLOBAL_TABLE(n ## _BASES)[i]) exit(1); */ \
        sum += (UInt32)1 << a; \
        BASES_TABLE(n ## _EXTRA)[i] = (Byte)a; }}

  #define FILL_LOC_BASES_ALL \
      FILL_LOC_BASES (SEQ_LL, 0) \
      FILL_LOC_BASES (SEQ_ML, MATCH_LEN_MIN) \

#else
  #define COPY_GLOBAL_ARR(n)  \
    memcpy(BASES_TABLE(n), GLOBAL_TABLE(n), sizeof(GLOBAL_TABLE(n)));
  #define FILL_LOC_BASES_ALL \
    COPY_GLOBAL_ARR (SEQ_LL_EXTRA) \
    COPY_GLOBAL_ARR (SEQ_ML_EXTRA) \
    COPY_GLOBAL_ARR (SEQ_LL_BASES) \
    COPY_GLOBAL_ARR (SEQ_ML_BASES) \

#endif

#endif


    
/// The sequence decoding baseline and number of additional bits to read/add
#if !defined(Z7_ZSTD_DEC_USE_BASES_CALC)
static const UInt32 GLOBAL_TABLE(SEQ_LL_BASES) [NUM_LL_SYMBOLS] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 18, 20, 22, 24, 28, 32, 40, 48, 64, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000,
  0x2000, 0x4000, 0x8000, 0x10000
};
#endif

static const Byte GLOBAL_TABLE(SEQ_LL_EXTRA) [NUM_LL_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16
};

#if !defined(Z7_ZSTD_DEC_USE_BASES_CALC)
static const UInt32 GLOBAL_TABLE(SEQ_ML_BASES) [NUM_ML_SYMBOLS] =
{
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 0x83, 0x103, 0x203, 0x403, 0x803,
  0x1003, 0x2003, 0x4003, 0x8003, 0x10003
};
#endif

static const Byte GLOBAL_TABLE(SEQ_ML_EXTRA) [NUM_ML_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16
};


#ifdef Z7_ZSTD_DEC_PRINT_TABLE

static const Int16 SEQ_LL_PREDEF_DIST [NUM_LL_SYMBOLS] =
{
  4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1,
 -1,-1,-1,-1
};
static const Int16 SEQ_OFFSET_PREDEF_DIST [NUM_OFFSET_SYMBOLS_PREDEF] =
{
  1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1
};
static const Int16 SEQ_ML_PREDEF_DIST [NUM_ML_SYMBOLS] =
{
  1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,
 -1,-1,-1,-1,-1
};

#endif

// typedef int FastInt;
// typedef Int32 FastInt32;
typedef unsigned FastInt;
typedef UInt32 FastInt32;
typedef FastInt32 CFseRecord;


#define FSE_REC_LEN_OFFSET    8
#define FSE_REC_STATE_OFFSET  16
#define GET_FSE_REC_SYM(st)   ((Byte)(st))
#define GET_FSE_REC_LEN(st)   ((Byte)((st) >> FSE_REC_LEN_OFFSET))
#define GET_FSE_REC_STATE(st) ((st) >> FSE_REC_STATE_OFFSET)

// #define FSE_REC_SYM_MASK      (0xff)
// #define GET_FSE_REC_SYM(st)   (st & FSE_REC_SYM_MASK)

#define W_BASE(state, len, sym) \
    (((UInt32)state << (4 + FSE_REC_STATE_OFFSET)) + \
    (len << FSE_REC_LEN_OFFSET) + (sym))
#define W(state, len, sym)  W_BASE(state, len, sym)
static const CFseRecord k_PredefRecords_LL[1 << 6] = {
W(0,4, 0),W(1,4, 0),W(2,5, 1),W(0,5, 3),W(0,5, 4),W(0,5, 6),W(0,5, 7),W(0,5, 9),
W(0,5,10),W(0,5,12),W(0,6,14),W(0,5,16),W(0,5,18),W(0,5,19),W(0,5,21),W(0,5,22),
W(0,5,24),W(2,5,25),W(0,5,26),W(0,6,27),W(0,6,29),W(0,6,31),W(2,4, 0),W(0,4, 1),
W(0,5, 2),W(2,5, 4),W(0,5, 5),W(2,5, 7),W(0,5, 8),W(2,5,10),W(0,5,11),W(0,6,13),
W(2,5,16),W(0,5,17),W(2,5,19),W(0,5,20),W(2,5,22),W(0,5,23),W(0,4,25),W(1,4,25),
W(2,5,26),W(0,6,28),W(0,6,30),W(3,4, 0),W(1,4, 1),W(2,5, 2),W(2,5, 3),W(2,5, 5),
W(2,5, 6),W(2,5, 8),W(2,5, 9),W(2,5,11),W(2,5,12),W(0,6,15),W(2,5,17),W(2,5,18),
W(2,5,20),W(2,5,21),W(2,5,23),W(2,5,24),W(0,6,35),W(0,6,34),W(0,6,33),W(0,6,32)
};
static const CFseRecord k_PredefRecords_OF[1 << 5] = {
W(0,5, 0),W(0,4, 6),W(0,5, 9),W(0,5,15),W(0,5,21),W(0,5, 3),W(0,4, 7),W(0,5,12),
W(0,5,18),W(0,5,23),W(0,5, 5),W(0,4, 8),W(0,5,14),W(0,5,20),W(0,5, 2),W(1,4, 7),
W(0,5,11),W(0,5,17),W(0,5,22),W(0,5, 4),W(1,4, 8),W(0,5,13),W(0,5,19),W(0,5, 1),
W(1,4, 6),W(0,5,10),W(0,5,16),W(0,5,28),W(0,5,27),W(0,5,26),W(0,5,25),W(0,5,24)
};
#if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
#undef W
#define W(state, len, sym)  W_BASE(state, len, (sym + MATCH_LEN_MIN))
#endif
static const CFseRecord k_PredefRecords_ML[1 << 6] = {
W(0,6, 0),W(0,4, 1),W(2,5, 2),W(0,5, 3),W(0,5, 5),W(0,5, 6),W(0,5, 8),W(0,6,10),
W(0,6,13),W(0,6,16),W(0,6,19),W(0,6,22),W(0,6,25),W(0,6,28),W(0,6,31),W(0,6,33),
W(0,6,35),W(0,6,37),W(0,6,39),W(0,6,41),W(0,6,43),W(0,6,45),W(1,4, 1),W(0,4, 2),
W(2,5, 3),W(0,5, 4),W(2,5, 6),W(0,5, 7),W(0,6, 9),W(0,6,12),W(0,6,15),W(0,6,18),
W(0,6,21),W(0,6,24),W(0,6,27),W(0,6,30),W(0,6,32),W(0,6,34),W(0,6,36),W(0,6,38),
W(0,6,40),W(0,6,42),W(0,6,44),W(2,4, 1),W(3,4, 1),W(1,4, 2),W(2,5, 4),W(2,5, 5),
W(2,5, 7),W(2,5, 8),W(0,6,11),W(0,6,14),W(0,6,17),W(0,6,20),W(0,6,23),W(0,6,26),
W(0,6,29),W(0,6,52),W(0,6,51),W(0,6,50),W(0,6,49),W(0,6,48),W(0,6,47),W(0,6,46)
};


// sum of freqs[] must be correct
// (numSyms != 0)
// (accuracy >= 5)
static
Z7_NO_INLINE
// Z7_FORCE_INLINE
void FSE_Generate(CFseRecord *table,
    const Int16 *const freqs, const size_t numSyms,
    const unsigned accuracy, UInt32 delta)
{
  size_t size = (size_t)1 << accuracy;
  // max value in states[x] is ((1 << accuracy) * 2)
  UInt16 states[FSE_NUM_SYMBOLS_MAX];
  {
    /* Symbols with "less than 1" probability get a single cell,
       starting from the end of the table.
       These symbols define a full state reset, reading (accuracy) bits. */
    size_t threshold = size;
    {
      size_t s = 0;
      do
        if (freqs[s] == -1)
        {
          table[--threshold] = (CFseRecord)s;
          states[s] = 1;
        }
      while (++s != numSyms);
    }
 
    #ifdef SHOW_STAT
    if (threshold == size)
    {
      STAT_INC(g_Num_Threshold_0)
      STAT_UPDATE(g_Num_Threshold_0sum += (unsigned)size;)
    }
    else
    {
      STAT_INC(g_Num_Threshold_1)
      STAT_UPDATE(g_Num_Threshold_1sum += (unsigned)size;)
    }
    #endif

    // { unsigned uuu; for (uuu = 0; uuu < 400; uuu++)
    {
      // Each (symbol) gets freqs[symbol] cells.
      // Cell allocation is spread, not linear.
      const size_t step = (size >> 1) + (size >> 3) + 3;
      size_t pos = 0;
      // const unsigned mask = size - 1;
      /*
      if (threshold == size)
      {
        size_t s = 0;
        size--;
        do
        {
          int freq = freqs[s];
          if (freq <= 0)
            continue;
          states[s] = (UInt16)freq;
          do
          {
            table[pos] (CFseRecord)s;
            pos = (pos + step) & size; // & mask;
          }
          while (--freq);
        }
        while (++s != numSyms);
      }
      else
      */
      {
        size_t s = 0;
        size--;
        do
        {
          int freq = freqs[s];
          if (freq <= 0)
            continue;
          states[s] = (UInt16)freq;
          do
          {
            table[pos] = (CFseRecord)s;
            // we skip position, if it's already occupied by a "less than 1" probability symbol.
            // (step) is coprime to table size, so the cycle will visit each position exactly once
            do
              pos = (pos + step) & size; // & mask;
            while (pos >= threshold);
          }
          while (--freq);
        }
        while (++s != numSyms);
      }
      size++;
      // (pos != 0) is unexpected case that means that freqs[] are not correct.
      // so it's some failure in code (for example, incorrect predefined freq[] table)
      // if (pos != 0) return SZ_ERROR_FAIL;
    }
    // }
  }
  {
    const CFseRecord * const limit = table + size;
    delta = ((UInt32)size << FSE_REC_STATE_OFFSET) - delta;
    /* State increases by symbol over time, decreasing number of bits.
       Baseline increases until the bit threshold is passed, at which point it resets to 0 */
    do
    {
      #define TABLE_ITER(a) \
      { \
        const FastInt sym = (FastInt)table[a]; \
        const unsigned nextState = states[sym]; \
        unsigned nb; \
        states[sym] = (UInt16)(nextState + 1); \
        nb = accuracy - GetHighestSetBit_32_nonzero_small(nextState); \
        table[a] = (CFseRecord)(sym - delta \
            + ((UInt32)nb << FSE_REC_LEN_OFFSET) \
            + ((UInt32)nextState << FSE_REC_STATE_OFFSET << nb)); \
      }
      TABLE_ITER(0)
      TABLE_ITER(1)
      table += 2;
    }
    while (table != limit);
  }
}


#ifdef Z7_ZSTD_DEC_PRINT_TABLE

static void Print_Predef(unsigned predefAccuracy,
    const unsigned numSymsPredef,
    const Int16 * const predefFreqs,
    const CFseRecord *checkTable)
{
  CFseRecord table[1 << 6];
  unsigned i;
  FSE_Generate(table, predefFreqs, numSymsPredef, predefAccuracy,
        #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
          numSymsPredef == NUM_ML_SYMBOLS ? MATCH_LEN_MIN :
        #endif
          0
    );
  if (memcmp(table, checkTable, sizeof(UInt32) << predefAccuracy) != 0)
    exit(1);
  for (i = 0; i < (1u << predefAccuracy); i++)
  {
    const UInt32 v = table[i];
    const unsigned state = (unsigned)(GET_FSE_REC_STATE(v));
    if (state & 0xf)
      exit(1);
    if (i != 0)
    {
      printf(",");
      if (i % 8 == 0)
        printf("\n");
    }
    printf("W(%d,%d,%2d)",
        (unsigned)(state >> 4),
        (unsigned)((v >> FSE_REC_LEN_OFFSET) & 0xff),
        (unsigned)GET_FSE_REC_SYM(v));
  }
  printf("\n\n");
}

#endif


#define GET16(dest, p)  { const Byte *ptr = p;  dest = GetUi16(ptr); }
#define GET32(dest, p)  { const Byte *ptr = p;  dest = GetUi32(ptr); }

// (1 <= numBits <= 9)
#define FORWARD_READ_BITS(destVal, numBits, mask) \
  { const CBitCtr_signed bos3 = (bitOffset) >> 3; \
    if (bos3 >= 0) return SZ_ERROR_DATA; \
    GET16(destVal, src + bos3) \
    destVal >>= (bitOffset) & 7; \
    bitOffset += (CBitCtr_signed)(numBits); \
    mask = (1u << (numBits)) - 1; \
    destVal &= mask; \
  }

#define FORWARD_READ_1BIT(destVal) \
  { const CBitCtr_signed bos3 = (bitOffset) >> 3; \
    if (bos3 >= 0) return SZ_ERROR_DATA; \
    destVal = *(src + bos3); \
    destVal >>= (bitOffset) & 7; \
    (bitOffset)++; \
    destVal &= 1; \
  }


// in: (accuracyMax <= 9)
// at least 2 bytes will be processed from (in) stream.
// at return: (in->len > 0)
static
Z7_NO_INLINE
SRes FSE_DecodeHeader(CFseRecord *const table,
    CInBufPair *const in,
    const unsigned accuracyMax,
    Byte *const accuracyRes,
    unsigned numSymbolsMax)
{
  unsigned accuracy;
  unsigned remain1;
  unsigned syms;
  Int16 freqs[FSE_NUM_SYMBOLS_MAX + 3]; // +3 for overwrite (repeat)
  const Byte *src = in->ptr;
  CBitCtr_signed bitOffset = (CBitCtr_signed)in->len - 1;
  if (bitOffset <= 0)
    return SZ_ERROR_DATA;
  accuracy = *src & 0xf;
  accuracy += 5;
  if (accuracy > accuracyMax)
    return SZ_ERROR_DATA;
  *accuracyRes = (Byte)accuracy;
  remain1 = (1u << accuracy) + 1; // (it's remain_freqs_sum + 1)
  syms = 0;
  src += bitOffset;  // src points to last byte
  bitOffset = 4 - (bitOffset << 3);
  
  for (;;)
  {
    // (2 <= remain1)
    const unsigned bits = GetHighestSetBit_32_nonzero_small((unsigned)remain1);
    // (1 <= bits <= accuracy)
    unsigned val; // it must be unsigned or int
    unsigned mask;
    FORWARD_READ_BITS(val, bits, mask)
    {
      const unsigned val2 = remain1 + val - mask;
      if (val2 > mask)
      {
        unsigned bit;
        FORWARD_READ_1BIT(bit)
        if (bit)
          val = val2;
      }
    }
    {
      // (remain1 >= 2)
      // (0 <= (int)val <= remain1)
      val = (unsigned)((int)val - 1);
      // val now is "probability" of symbol
      // (probability == -1) means "less than 1" frequency.
      // (-1 <= (int)val <= remain1 - 1)
      freqs[syms++] = (Int16)(int)val;
      if (val != 0)
      {
        remain1 -= (int)val < 0 ? 1u : (unsigned)val;
        // remain1 -= val;
        // val >>= (sizeof(val) * 8 - 2);
        // remain1 -= val & 2;
        // freqs[syms++] = (Int16)(int)val;
        // syms++;
        if (remain1 == 1)
          break;
        if (syms >= FSE_NUM_SYMBOLS_MAX)
          return SZ_ERROR_DATA;
      }
      else // if (val == 0)
      {
        // freqs[syms++] = 0;
        // syms++;
        for (;;)
        {
          unsigned repeat;
          FORWARD_READ_BITS(repeat, 2, mask)
          freqs[syms    ] = 0;
          freqs[syms + 1] = 0;
          freqs[syms + 2] = 0;
          syms += repeat;
          if (syms >= FSE_NUM_SYMBOLS_MAX)
            return SZ_ERROR_DATA;
          if (repeat != 3)
            break;
        }
      }
    }
  }

  if (syms > numSymbolsMax)
    return SZ_ERROR_DATA;
  bitOffset += 7;
  bitOffset >>= 3;
  if (bitOffset > 0)
    return SZ_ERROR_DATA;
  in->ptr = src + bitOffset;
  in->len = (size_t)(1 - bitOffset);
  {
    // unsigned uuu; for (uuu = 0; uuu < 50; uuu++)
    FSE_Generate(table, freqs, syms, accuracy,
        #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
          numSymbolsMax == NUM_ML_SYMBOLS ? MATCH_LEN_MIN :
        #endif
          0
        );
  }
  return SZ_OK;
}


// ---------- HUFFMAN ----------

#define HUF_MAX_BITS    12
#define HUF_MAX_SYMBS   256
#define HUF_DUMMY_SIZE  (128 + 8 * 2)  // it must multiple of 8
// #define HUF_DUMMY_SIZE 0
#define HUF_TABLE_SIZE  ((2 << HUF_MAX_BITS) + HUF_DUMMY_SIZE)
#define HUF_GET_SYMBOLS(table)  ((table) + (1 << HUF_MAX_BITS) + HUF_DUMMY_SIZE)
// #define HUF_GET_LENS(table)  (table)

typedef struct
{
  // Byte table[HUF_TABLE_SIZE];
  UInt64 table64[HUF_TABLE_SIZE / sizeof(UInt64)];
}
CZstdDecHufTable;

/*
Input:
  numSyms != 0
  (bits) array size must be aligned for 2
  if (numSyms & 1), then bits[numSyms] == 0,
  Huffman tree must be correct before Huf_Build() call:
    (sum (1/2^bits[i]) == 1).
    && (bits[i] <= HUF_MAX_BITS)
*/
static
Z7_FORCE_INLINE
void Huf_Build(Byte * const table,
    const Byte *bits, const unsigned numSyms)
{
  unsigned counts0[HUF_MAX_BITS + 1];
  unsigned counts1[HUF_MAX_BITS + 1];
  const Byte * const bitsEnd = bits + numSyms;
  // /*
  {
    unsigned t;
    for (t = 0; t < Z7_ARRAY_SIZE(counts0); t++) counts0[t] = 0;
    for (t = 0; t < Z7_ARRAY_SIZE(counts1); t++) counts1[t] = 0;
  }
  // */
  // memset(counts0, 0, sizeof(counts0));
  // memset(counts1, 0, sizeof(counts1));
  {
    const Byte *bits2 = bits;
    // we access additional bits[symbol] if (numSyms & 1)
    do
    {
      counts0[bits2[0]]++;
      counts1[bits2[1]]++;
    }
    while ((bits2 += 2) < bitsEnd);
  }
  {
    unsigned r = 0;
    unsigned i = HUF_MAX_BITS;
    // Byte *lens = HUF_GET_LENS(symbols);
    do
    {
      const unsigned num = (counts0[i] + counts1[i]) << (HUF_MAX_BITS - i);
      counts0[i] = r;
      if (num)
      {
        Byte *lens = &table[r];
        r += num;
        memset(lens, (int)i, num);
      }
    }
    while (--i);
    counts0[0] = 0; // for speculated loads
    // no need for check:
    // if (r != (UInt32)1 << HUF_MAX_BITS) exit(0);
  }
  {
    #ifdef MY_CPU_64BIT
      UInt64
    #else
      UInt32
    #endif
        v = 0;
    Byte *symbols = HUF_GET_SYMBOLS(table);
    do
    {
      const unsigned nb = *bits++;
      if (nb)
      {
        const unsigned code = counts0[nb];
        const unsigned num = (1u << HUF_MAX_BITS) >> nb;
        counts0[nb] = code + num;
        // memset(&symbols[code], i, num);
        // /*
        {
          Byte *s2 = &symbols[code];
          if (num <= 2)
          {
            s2[0] = (Byte)v;
            s2[(size_t)num - 1] = (Byte)v;
          }
          else if (num <= 8)
          {
            *(UInt32 *)(void *)s2 = (UInt32)v;
            *(UInt32 *)(void *)(s2 + (size_t)num - 4) = (UInt32)v;
          }
          else
          {
            #ifdef MY_CPU_64BIT
              UInt64 *s = (UInt64 *)(void *)s2;
              const UInt64 *lim = (UInt64 *)(void *)(s2 + num);
              do
              {
                s[0] = v;  s[1] = v;  s += 2;
              }
              while (s != lim);
            #else
              UInt32 *s = (UInt32 *)(void *)s2;
              const UInt32 *lim = (const UInt32 *)(const void *)(s2 + num);
              do
              {
                s[0] = v;  s[1] = v;  s += 2;
                s[0] = v;  s[1] = v;  s += 2;
              }
              while (s != lim);
            #endif
          }
        }
        // */
      }
      v +=
        #ifdef MY_CPU_64BIT
          0x0101010101010101;
        #else
          0x01010101;
        #endif
    }
    while (bits != bitsEnd);
  }
}



// how many bytes (src) was moved back from original value.
// we need (HUF_SRC_OFFSET == 3) for optimized 32-bit memory access
#define HUF_SRC_OFFSET  3

// v <<= 8 - (bitOffset & 7) + numBits;
// v >>= 32 - HUF_MAX_BITS;
#define HUF_GET_STATE(v, bitOffset, numBits) \
  GET32(v, src + (HUF_SRC_OFFSET - 3) + ((CBitCtr_signed)bitOffset >> 3)) \
  v >>= 32 - HUF_MAX_BITS - 8 + ((unsigned)bitOffset & 7) - numBits; \
  v &= (1u << HUF_MAX_BITS) - 1; \


#ifndef Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS
#if defined(MY_CPU_AMD64) && defined(_MSC_VER) && _MSC_VER == 1400 \
  || !defined(MY_CPU_X86_OR_AMD64) \
  // || 1 == 1 /* for debug : to force STREAM4_PRELOAD mode */
  // we need big number (>=16) of registers for PRELOAD4
  #define Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD4
  // #define Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD2 // for debug
#endif
#endif

// for debug: simpler and smaller code but slow:
// #define Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE

#if  defined(Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE) || \
    !defined(Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS)
     
#define HUF_DECODE(bitOffset, dest) \
{ \
  UInt32 v; \
  HUF_GET_STATE(v, bitOffset, 0) \
  bitOffset -= table[v]; \
  *(dest) = symbols[v]; \
  if ((CBitCtr_signed)bitOffset < 0) return SZ_ERROR_DATA; \
}

#endif

#if !defined(Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE) || \
     defined(Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD4) || \
     defined(Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD2) \

#define HUF_DECODE_2_INIT(v, bitOffset) \
  HUF_GET_STATE(v, bitOffset, 0)

#define HUF_DECODE_2(v, bitOffset, dest) \
{ \
  unsigned numBits; \
  numBits = table[v]; \
  *(dest) = symbols[v]; \
  HUF_GET_STATE(v, bitOffset, numBits) \
  bitOffset -= (CBitCtr)numBits; \
  if ((CBitCtr_signed)bitOffset < 0) return SZ_ERROR_DATA; \
}

#endif


// src == ptr - HUF_SRC_OFFSET
// we are allowed to access 3 bytes before start of input buffer
static
Z7_NO_INLINE
SRes Huf_Decompress_1stream(const Byte * const table,
    const Byte *src, const size_t srcLen,
    Byte *dest, const size_t destLen)
{
  CBitCtr bitOffset;
  if (srcLen == 0)
    return SZ_ERROR_DATA;
  SET_bitOffset_TO_PAD (bitOffset, src + HUF_SRC_OFFSET, srcLen)
  if (destLen)
  {
    const Byte *symbols = HUF_GET_SYMBOLS(table);
    const Byte *destLim = dest + destLen;
    #ifdef Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE
    {
      do
      {
        HUF_DECODE (bitOffset, dest)
      }
      while (++dest != destLim);
    }
    #else
    {
      UInt32 v;
      HUF_DECODE_2_INIT (v, bitOffset)
      do
      {
        HUF_DECODE_2 (v, bitOffset, dest)
      }
      while (++dest != destLim);
    }
    #endif
  }
  return bitOffset == 0 ? SZ_OK : SZ_ERROR_DATA;
}


// for debug : it reduces register pressure : by array copy can be slow :
// #define Z7_ZSTD_DEC_USE_HUF_LOCAL

// src == ptr + (6 - HUF_SRC_OFFSET)
// srcLen >= 10
// we are allowed to access 3 bytes before start of input buffer
static
Z7_NO_INLINE
SRes Huf_Decompress_4stream(const Byte * const
  #ifdef Z7_ZSTD_DEC_USE_HUF_LOCAL
    table2,
  #else
    table,
  #endif
    const Byte *src, size_t srcLen,
    Byte *dest, size_t destLen)
{
 #ifdef Z7_ZSTD_DEC_USE_HUF_LOCAL
  Byte table[HUF_TABLE_SIZE];
 #endif
  UInt32 sizes[3];
  const size_t delta = (destLen + 3) / 4;
  if ((sizes[0] = GetUi16(src + (0 + HUF_SRC_OFFSET - 6))) == 0) return SZ_ERROR_DATA;
  if ((sizes[1] = GetUi16(src + (2 + HUF_SRC_OFFSET - 6))) == 0) return SZ_ERROR_DATA;
  sizes[1] += sizes[0];
  if ((sizes[2] = GetUi16(src + (4 + HUF_SRC_OFFSET - 6))) == 0) return SZ_ERROR_DATA;
  sizes[2] += sizes[1];
  srcLen -= 6;
  if (srcLen <= sizes[2])
    return SZ_ERROR_DATA;

 #ifdef Z7_ZSTD_DEC_USE_HUF_LOCAL
  {
    // unsigned i = 0; for(; i < 1000; i++)
    memcpy(table, table2, HUF_TABLE_SIZE);
  }
 #endif

  #ifndef Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS
  {
    CBitCtr bitOffset_0,
            bitOffset_1,
            bitOffset_2,
            bitOffset_3;
    {
      SET_bitOffset_TO_PAD_and_SET_BIT_SIZE (bitOffset_0, src + HUF_SRC_OFFSET, sizes[0])
      SET_bitOffset_TO_PAD_and_SET_BIT_SIZE (bitOffset_1, src + HUF_SRC_OFFSET, sizes[1])
      SET_bitOffset_TO_PAD_and_SET_BIT_SIZE (bitOffset_2, src + HUF_SRC_OFFSET, sizes[2])
      SET_bitOffset_TO_PAD                  (bitOffset_3, src + HUF_SRC_OFFSET, srcLen)
    }
    {
      const Byte * const symbols = HUF_GET_SYMBOLS(table);
      Byte *destLim = dest + destLen - delta * 3;

      if (dest != destLim)
    #ifdef Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD4
      {
        UInt32 v_0, v_1, v_2, v_3;
        HUF_DECODE_2_INIT (v_0, bitOffset_0)
        HUF_DECODE_2_INIT (v_1, bitOffset_1)
        HUF_DECODE_2_INIT (v_2, bitOffset_2)
        HUF_DECODE_2_INIT (v_3, bitOffset_3)
        // #define HUF_DELTA (1 << 17) / 4
        do
        {
          HUF_DECODE_2 (v_3, bitOffset_3, dest + delta * 3)
          HUF_DECODE_2 (v_2, bitOffset_2, dest + delta * 2)
          HUF_DECODE_2 (v_1, bitOffset_1, dest + delta)
          HUF_DECODE_2 (v_0, bitOffset_0, dest)
        }
        while (++dest != destLim);
        /*
        {// unsigned y = 0; for (;y < 1; y++)
        {
          const size_t num = destLen - delta * 3;
          Byte *orig = dest - num;
          memmove (orig + delta    , orig + HUF_DELTA,     num);
          memmove (orig + delta * 2, orig + HUF_DELTA * 2, num);
          memmove (orig + delta * 3, orig + HUF_DELTA * 3, num);
        }}
        */
      }
    #elif defined(Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD2)
      {
        UInt32 v_0, v_1, v_2, v_3;
        HUF_DECODE_2_INIT (v_0, bitOffset_0)
        HUF_DECODE_2_INIT (v_1, bitOffset_1)
        do
        {
          HUF_DECODE_2 (v_0, bitOffset_0, dest)
          HUF_DECODE_2 (v_1, bitOffset_1, dest + delta)
        }
        while (++dest != destLim);
        dest = destLim - (destLen - delta * 3);
        dest += delta * 2;
        destLim += delta * 2;
        HUF_DECODE_2_INIT (v_2, bitOffset_2)
        HUF_DECODE_2_INIT (v_3, bitOffset_3)
        do
        {
          HUF_DECODE_2 (v_2, bitOffset_2, dest)
          HUF_DECODE_2 (v_3, bitOffset_3, dest + delta)
        }
        while (++dest != destLim);
        dest -= delta * 2;
        destLim -= delta * 2;
      }
    #else
      {
        do
        {
          HUF_DECODE (bitOffset_3, dest + delta * 3)
          HUF_DECODE (bitOffset_2, dest + delta * 2)
          HUF_DECODE (bitOffset_1, dest + delta)
          HUF_DECODE (bitOffset_0, dest)
        }
        while (++dest != destLim);
      }
    #endif
    
      if (bitOffset_3 != (CBitCtr)sizes[2])
        return SZ_ERROR_DATA;
      if (destLen &= 3)
      {
        destLim = dest + 4 - destLen;
        do
        {
          HUF_DECODE (bitOffset_2, dest + delta * 2)
          HUF_DECODE (bitOffset_1, dest + delta)
          HUF_DECODE (bitOffset_0, dest)
        }
        while (++dest != destLim);
      }
      if (   bitOffset_0 != 0
          || bitOffset_1 != (CBitCtr)sizes[0]
          || bitOffset_2 != (CBitCtr)sizes[1])
        return SZ_ERROR_DATA;
    }
  }
  #else // Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS
  {
    unsigned i;
    for (i = 0; i < 4; i++)
    {
      size_t d = destLen;
      size_t size = srcLen;
      if (i != 3)
      {
        d = delta;
        size = sizes[i];
      }
      if (i != 0)
        size -= sizes[i - 1];
      destLen -= d;
      RINOK(Huf_Decompress_1stream(table, src, size, dest, d))
      dest += d;
      src += size;
    }
  }
  #endif

  return SZ_OK;
}



// (in->len != 0)
// we are allowed to access in->ptr[-3]
// at least 2 bytes in (in->ptr) will be processed
static SRes Huf_DecodeTable(CZstdDecHufTable *const p, CInBufPair *const in)
{
  Byte weights[HUF_MAX_SYMBS + 1];  // +1 for extra write for loop unroll
  unsigned numSyms;
  const unsigned header = *(in->ptr)++;
  in->len--;
  // memset(weights, 0, sizeof(weights));
  if (header >= 128)
  {
    // direct representation: 4 bits field (0-15) per weight
    numSyms = header - 127;
    // numSyms != 0
    {
      const size_t numBytes = (numSyms + 1) / 2;
      const Byte *const ws = in->ptr;
      size_t i = 0;
      if (in->len < numBytes)
        return SZ_ERROR_DATA;
      in->ptr += numBytes;
      in->len -= numBytes;
      do
      {
        const unsigned b = ws[i];
        weights[i * 2    ] = (Byte)(b >> 4);
        weights[i * 2 + 1] = (Byte)(b & 0xf);
      }
      while (++i != numBytes);
      /* 7ZIP: we can restore correct zero value for weights[numSyms],
         if we want to use zero values starting from numSyms in code below. */
      // weights[numSyms] = 0;
    }
  }
  else
  {
    #define MAX_ACCURACY_LOG_FOR_WEIGHTS 6
    CFseRecord table[1 << MAX_ACCURACY_LOG_FOR_WEIGHTS];

    Byte accuracy;
    const Byte *src;
    size_t srcLen;
    if (in->len < header)
      return SZ_ERROR_DATA;
    {
      CInBufPair fse_stream;
      fse_stream.len = header;
      fse_stream.ptr = in->ptr;
      in->ptr += header;
      in->len -= header;
      RINOK(FSE_DecodeHeader(table, &fse_stream,
          MAX_ACCURACY_LOG_FOR_WEIGHTS,
          &accuracy,
          16 // num weight symbols max (max-symbol is 15)
          ))
      // at least 2 bytes were processed in fse_stream.
      // (srcLen > 0) after FSE_DecodeHeader()
      // if (srcLen == 0) return SZ_ERROR_DATA;
      src = fse_stream.ptr;
      srcLen = fse_stream.len;
    }
    // we are allowed to access src[-5]
    {
      // unsigned yyy = 200; do {
      CBitCtr bitOffset;
      FastInt32 state1, state2;
      SET_bitOffset_TO_PAD (bitOffset, src, srcLen)
      state1 = accuracy;
      src -= state1 >> 2;  // src -= 1; // for GET16() optimization
      state1 <<= FSE_REC_LEN_OFFSET;
      state2 = state1;
      numSyms = 0;
      for (;;)
      {
        #define FSE_WEIGHT_DECODE(st) \
        { \
          const unsigned bits = GET_FSE_REC_LEN(st); \
          FastInt r; \
          GET16(r, src + (bitOffset >> 3)) \
          r >>= (unsigned)bitOffset & 7; \
          if ((CBitCtr_signed)(bitOffset -= (CBitCtr)bits) < 0) \
            { if (bitOffset + (CBitCtr)bits != 0) \
                return SZ_ERROR_DATA; \
              break; } \
          r &= 0xff; \
          r >>= 8 - bits; \
          st = table[GET_FSE_REC_STATE(st) + r]; \
          weights[numSyms++] = (Byte)GET_FSE_REC_SYM(st); \
        }
        FSE_WEIGHT_DECODE (state1)
        FSE_WEIGHT_DECODE (state2)
        if (numSyms == HUF_MAX_SYMBS)
          return SZ_ERROR_DATA;
      }
      // src += (unsigned)accuracy >> 2; } while (--yyy);
    }
  }
  
  // Build using weights:
  {
    UInt32 sum = 0;
    {
      // numSyms >= 1
      unsigned i = 0;
      weights[numSyms] = 0;
      do
      {
        sum += ((UInt32)1 << weights[i    ]) & ~(UInt32)1;
        sum += ((UInt32)1 << weights[i + 1]) & ~(UInt32)1;
        i += 2;
      }
      while (i < numSyms);
      if (sum == 0)
        return SZ_ERROR_DATA;
    }
    {
      const unsigned maxBits = GetHighestSetBit_32_nonzero_big(sum) + 1;
      {
        const UInt32 left = ((UInt32)1 << maxBits) - sum;
        // (left != 0)
        // (left) must be power of 2 in correct stream
        if (left & (left - 1))
          return SZ_ERROR_DATA;
        weights[numSyms++] = (Byte)GetHighestSetBit_32_nonzero_big(left);
      }
      // if (numSyms & 1)
        weights[numSyms] = 0; // for loop unroll
      // numSyms >= 2
      {
        unsigned i = 0;
        do
        {
          /*
          #define WEIGHT_ITER(a) \
            { unsigned w = weights[i + (a)]; \
              const unsigned t = maxBits - w; \
              w = w ? t: w; \
              if (w > HUF_MAX_BITS) return SZ_ERROR_DATA; \
              weights[i + (a)] = (Byte)w; }
          */
          // /*
          #define WEIGHT_ITER(a) \
            { unsigned w = weights[i + (a)]; \
              if (w) {  \
                w = maxBits - w; \
                if (w > HUF_MAX_BITS) return SZ_ERROR_DATA; \
                weights[i + (a)] = (Byte)w; }}
          // */
          WEIGHT_ITER(0)
          // WEIGHT_ITER(1)
          // i += 2;
        }
        while (++i != numSyms);
      }
    }
  }
  {
    // unsigned yyy; for (yyy = 0; yyy < 100; yyy++)
    Huf_Build((Byte *)(void *)p->table64, weights, numSyms);
  }
  return SZ_OK;
}


typedef enum
{
  k_SeqMode_Predef = 0,
  k_SeqMode_RLE    = 1,
  k_SeqMode_FSE    = 2,
  k_SeqMode_Repeat = 3
}
z7_zstd_enum_SeqMode;

// predefAccuracy == 5 for OFFSET symbols
// predefAccuracy == 6 for MATCH/LIT LEN symbols
static
SRes
Z7_NO_INLINE
// Z7_FORCE_INLINE
FSE_Decode_SeqTable(CFseRecord * const table,
    CInBufPair * const in,
    unsigned predefAccuracy,
    Byte * const accuracyRes,
    unsigned numSymbolsMax,
    const CFseRecord * const predefs,
    const unsigned seqMode)
{
  // UNUSED_VAR(numSymsPredef)
  // UNUSED_VAR(predefFreqs)
  if (seqMode == k_SeqMode_FSE)
  {
    // unsigned y = 50; CInBufPair in2 = *in; do { *in = in2; RINOK(
    return
    FSE_DecodeHeader(table, in,
        predefAccuracy + 3, // accuracyMax
        accuracyRes,
        numSymbolsMax)
    ;
    // )} while (--y); return SZ_OK;
  }
  // numSymsMax = numSymsPredef + ((predefAccuracy & 1) * (32 - 29))); // numSymsMax
  // numSymsMax == 32 for offsets

  if (seqMode == k_SeqMode_Predef)
  {
    *accuracyRes = (Byte)predefAccuracy;
    memcpy(table, predefs, sizeof(UInt32) << predefAccuracy);
    return SZ_OK;
  }

  // (seqMode == k_SeqMode_RLE)
  if (in->len == 0)
    return SZ_ERROR_DATA;
  in->len--;
  {
    const Byte *ptr = in->ptr;
    const unsigned sym = ptr[0];
    in->ptr = ptr + 1;
    if (sym >= numSymbolsMax)
      return SZ_ERROR_DATA;
    table[0] = (FastInt32)sym
      #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
        + (numSymbolsMax == NUM_ML_SYMBOLS ? MATCH_LEN_MIN : 0)
      #endif
      ;
    *accuracyRes = 0;
  }
  return SZ_OK;
}


typedef struct
{
  CFseRecord of[1 << 8];
  CFseRecord ll[1 << 9];
  CFseRecord ml[1 << 9];
}
CZstdDecFseTables;


typedef struct
{
  Byte *win;
  SizeT cycSize;
  /*
    if (outBuf_fromCaller)  : cycSize = outBufSize_fromCaller
    else {
      if ( isCyclicMode) : cycSize = cyclic_buffer_size = (winSize + extra_space)
      if (!isCyclicMode) : cycSize = ContentSize,
      (isCyclicMode == true) if (ContetSize >= winSize) or ContetSize is unknown
    }
  */
  SizeT winPos;

  CZstdDecOffset reps[3];

  Byte ll_accuracy;
  Byte of_accuracy;
  Byte ml_accuracy;
  // Byte seqTables_wereSet;
  Byte litHuf_wasSet;
  
  Byte *literalsBase;

  size_t winSize;        // from header
  size_t totalOutCheck;  // totalOutCheck <= winSize

  #ifdef Z7_ZSTD_DEC_USE_BASES_IN_OBJECT
  SEQ_EXTRA_TABLES(m_)
  #endif
  // UInt64 _pad_Alignment;  // is not required now
  CZstdDecFseTables fse;
  CZstdDecHufTable huf;
}
CZstdDec1;

#define ZstdDec1_GET_BLOCK_SIZE_LIMIT(p) \
  ((p)->winSize < kBlockSizeMax ? (UInt32)(p)->winSize : kBlockSizeMax)

#define SEQ_TABLES_WERE_NOT_SET_ml_accuracy  1  // accuracy=1 is not used by zstd
#define IS_SEQ_TABLES_WERE_SET(p)  (((p)->ml_accuracy != SEQ_TABLES_WERE_NOT_SET_ml_accuracy))
// #define IS_SEQ_TABLES_WERE_SET(p)  ((p)->seqTables_wereSet)


static void ZstdDec1_Construct(CZstdDec1 *p)
{
  #ifdef Z7_ZSTD_DEC_PRINT_TABLE
  Print_Predef(6, NUM_LL_SYMBOLS, SEQ_LL_PREDEF_DIST, k_PredefRecords_LL);
  Print_Predef(5, NUM_OFFSET_SYMBOLS_PREDEF, SEQ_OFFSET_PREDEF_DIST, k_PredefRecords_OF);
  Print_Predef(6, NUM_ML_SYMBOLS, SEQ_ML_PREDEF_DIST, k_PredefRecords_ML);
  #endif

  p->win = NULL;
  p->cycSize = 0;
  p->literalsBase = NULL;
  #ifdef Z7_ZSTD_DEC_USE_BASES_IN_OBJECT
  FILL_LOC_BASES_ALL
  #endif
}


static void ZstdDec1_Init(CZstdDec1 *p)
{
  p->reps[0] = 1;
  p->reps[1] = 4;
  p->reps[2] = 8;
  // p->seqTables_wereSet = False;
  p->ml_accuracy = SEQ_TABLES_WERE_NOT_SET_ml_accuracy;
  p->litHuf_wasSet = False;
  p->totalOutCheck = 0;
}



#ifdef MY_CPU_LE_UNALIGN
  #define Z7_ZSTD_DEC_USE_UNALIGNED_COPY
#endif

#ifdef Z7_ZSTD_DEC_USE_UNALIGNED_COPY

  #define COPY_CHUNK_SIZE 16

    #define COPY_CHUNK_4_2(dest, src) \
    { \
      ((UInt32 *)(void *)dest)[0] = ((const UInt32 *)(const void *)src)[0]; \
      ((UInt32 *)(void *)dest)[1] = ((const UInt32 *)(const void *)src)[1]; \
      src += 4 * 2; \
      dest += 4 * 2; \
    }

  /* sse2 doesn't help here in GCC and CLANG.
     so we disabled sse2 here */
  /*
  #if defined(MY_CPU_AMD64)
    #define Z7_ZSTD_DEC_USE_SSE2
  #elif defined(MY_CPU_X86)
    #if defined(_MSC_VER) && _MSC_VER >= 1300 && defined(_M_IX86_FP) && (_M_IX86_FP >= 2) \
      || defined(__SSE2__) \
      // || 1 == 1  // for debug only
      #define Z7_ZSTD_DEC_USE_SSE2
    #endif
  #endif
  */

  #if defined(MY_CPU_ARM64)
    #define COPY_OFFSET_MIN  16
    #define COPY_CHUNK1(dest, src) \
    { \
      vst1q_u8((uint8_t *)(void *)dest, \
      vld1q_u8((const uint8_t *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }
    
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if ((len -= COPY_CHUNK_SIZE) == 0) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(Z7_ZSTD_DEC_USE_SSE2)
    #include <emmintrin.h> // sse2
    #define COPY_OFFSET_MIN  16

    #define COPY_CHUNK1(dest, src) \
    { \
      _mm_storeu_si128((__m128i *)(void *)dest, \
      _mm_loadu_si128((const __m128i *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if ((len -= COPY_CHUNK_SIZE) == 0) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(MY_CPU_64BIT)
    #define COPY_OFFSET_MIN  8

    #define COPY_CHUNK(dest, src) \
    { \
      ((UInt64 *)(void *)dest)[0] = ((const UInt64 *)(const void *)src)[0]; \
      ((UInt64 *)(void *)dest)[1] = ((const UInt64 *)(const void *)src)[1]; \
      src += 8 * 2; \
      dest += 8 * 2; \
    }

  #else
    #define COPY_OFFSET_MIN  4

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_4_2(dest, src); \
      COPY_CHUNK_4_2(dest, src); \
    }

  #endif
#endif


#ifndef COPY_CHUNK_SIZE
    #define COPY_OFFSET_MIN  4
    #define COPY_CHUNK_SIZE  8
    #define COPY_CHUNK_2(dest, src) \
    { \
      const Byte a0 = src[0]; \
      const Byte a1 = src[1]; \
      dest[0] = a0; \
      dest[1] = a1; \
      src += 2; \
      dest += 2; \
    }
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
    }
#endif


#define COPY_PREPARE \
  len += (COPY_CHUNK_SIZE - 1); \
  len &= ~(size_t)(COPY_CHUNK_SIZE - 1); \
  { if (len > rem) \
  { len = rem; \
    rem &= (COPY_CHUNK_SIZE - 1); \
    if (rem) {  \
      len -= rem; \
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
      do *dest++ = *src++; while (--rem); \
      if (len == 0) return; }}}

#define COPY_CHUNKS \
{ \
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
  do { COPY_CHUNK(dest, src) } \
  while (len -= COPY_CHUNK_SIZE); \
}

// (len != 0)
// (len <= rem)
static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyLiterals(Byte *dest, Byte const *src, size_t len, size_t rem)
{
  COPY_PREPARE
  COPY_CHUNKS
}


/* we can define Z7_STD_DEC_USE_AFTER_CYC_BUF, if we want to use additional
   space after cycSize that can be used to reduce the code in CopyMatch(): */
// for debug:
// #define Z7_STD_DEC_USE_AFTER_CYC_BUF

/*
CopyMatch()
if wrap (offset > winPos)
{
  then we have at least (COPY_CHUNK_SIZE) avail in (dest) before we will overwrite (src):
  (cycSize >= offset + COPY_CHUNK_SIZE)
  if defined(Z7_STD_DEC_USE_AFTER_CYC_BUF)
    we are allowed to read win[cycSize + COPY_CHUNK_SIZE - 1],
}
(len != 0)
*/
static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyMatch(size_t offset, size_t len,
    Byte *win, size_t winPos, size_t rem, const size_t cycSize)
{
  Byte *dest = win + winPos;
  const Byte *src;
  // STAT_INC(g_NumCopy)

  if (offset > winPos)
  {
    size_t back = offset - winPos;
    // src = win + cycSize - back;
    // cycSize -= offset;
    STAT_INC(g_NumOver)
    src = dest + (cycSize - offset);
    // (src >= dest) here
   #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
    if (back < len)
    {
   #else
    if (back < len + (COPY_CHUNK_SIZE - 1))
    {
      if (back >= len)
      {
        Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
        do
          *dest++ = *src++;
        while (--len);
        return;
      }
   #endif
      // back < len
      STAT_INC(g_NumOver2)
      len -= back;
      rem -= back;
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
      do
        *dest++ = *src++;
      while (--back);
      src = dest - offset;
      // src = win;
      // we go to MAIN-COPY
    }
  }
  else
    src = dest - offset;

  // len != 0
  // do *dest++ = *src++; while (--len); return;

  // --- MAIN COPY ---
  // if (src >= dest), then ((size_t)(src - dest) >= COPY_CHUNK_SIZE)
  //   so we have at least COPY_CHUNK_SIZE space before overlap for writing.
  COPY_PREPARE

  /* now (len == COPY_CHUNK_SIZE * x)
     so we can unroll for aligned copy */
  {
    // const unsigned b0 = src[0];
    // (COPY_OFFSET_MIN >= 4)
 
    if (offset >= COPY_OFFSET_MIN)
    {
      COPY_CHUNKS
      // return;
    }
    else
  #if (COPY_OFFSET_MIN > 4)
    #if COPY_CHUNK_SIZE < 8
      #error Stop_Compiling_Bad_COPY_CHUNK_SIZE
    #endif
    if (offset >= 4)
    {
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
      do
      {
        COPY_CHUNK_4_2(dest, src)
        #if COPY_CHUNK_SIZE != 16
          if (len == 8) break;
        #endif
        COPY_CHUNK_4_2(dest, src)
      }
      while (len -= 16);
      // return;
    }
    else
  #endif
    {
      // (offset < 4)
      const unsigned b0 = src[0];
      if (offset < 2)
      {
      #if defined(Z7_ZSTD_DEC_USE_UNALIGNED_COPY) && (COPY_CHUNK_SIZE == 16)
        #if defined(MY_CPU_64BIT)
        {
          const UInt64 v64 = (UInt64)b0 * 0x0101010101010101;
          Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
          do
          {
            ((UInt64 *)(void *)dest)[0] = v64;
            ((UInt64 *)(void *)dest)[1] = v64;
            dest += 16;
          }
          while (len -= 16);
        }
        #else
        {
          UInt32 v = b0;
          v |= v << 8;
          v |= v << 16;
          do
          {
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
          }
          while (len -= 16);
        }
        #endif
      #else
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
        }
        while (len -= 4);
      #endif
      }
      else if (offset == 2)
      {
        const Byte b1 = src[1];
        {
          do
          {
            dest[0] = (Byte)b0;
            dest[1] = b1;
            dest += 2;
          }
          while (len -= 2);
        }
      }
      else // (offset == 3)
      {
        const Byte *lim = dest + len - 2;
        const Byte b1 = src[1];
        const Byte b2 = src[2];
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = b1;
          dest[2] = b2;
          dest += 3;
        }
        while (dest < lim);
        lim++; // points to last byte that must be written
        if (dest <= lim)
        {
          *dest = (Byte)b0;
          if (dest != lim)
            dest[1] = b1;
        }
      }
    }
  }
}



#define UPDATE_TOTAL_OUT(p, size) \
{ \
  size_t _toc = (p)->totalOutCheck + (size); \
  const size_t _ws = (p)->winSize; \
  if (_toc >= _ws) _toc = _ws; \
  (p)->totalOutCheck = _toc; \
}


#if defined(MY_CPU_64BIT) && defined(MY_CPU_LE_UNALIGN)
// we can disable it for debug:
#define Z7_ZSTD_DEC_USE_64BIT_LOADS
#endif
// #define Z7_ZSTD_DEC_USE_64BIT_LOADS // for debug : slow in 32-bit

// SEQ_SRC_OFFSET: how many bytes (src) (seqSrc) was moved back from original value.
// we need (SEQ_SRC_OFFSET != 0) for optimized memory access
#ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
  #define SEQ_SRC_OFFSET 7
#else
  #define SEQ_SRC_OFFSET 3
#endif
#define SRC_PLUS_FOR_4BYTES(bitOffset)  (SEQ_SRC_OFFSET - 3) + ((CBitCtr_signed)(bitOffset) >> 3)
#define BIT_OFFSET_7BITS(bitOffset)  ((unsigned)(bitOffset) & 7)
/*
  if (BIT_OFFSET_DELTA_BITS == 0) : bitOffset == number_of_unprocessed_bits
  if (BIT_OFFSET_DELTA_BITS == 1) : bitOffset == number_of_unprocessed_bits - 1
      and we can read 1 bit more in that mode : (8 * n + 1).
*/
// #define BIT_OFFSET_DELTA_BITS 0
#define BIT_OFFSET_DELTA_BITS 1
#if BIT_OFFSET_DELTA_BITS == 1
  #define GET_SHIFT_FROM_BOFFS7(boff7)  (7 ^ (boff7))
#else
  #define GET_SHIFT_FROM_BOFFS7(boff7)  (8 - BIT_OFFSET_DELTA_BITS - (boff7))
#endif

#define UPDATE_BIT_OFFSET(bitOffset, numBits) \
    (bitOffset) -= (CBitCtr)(numBits);

#define GET_SHIFT(bitOffset)  GET_SHIFT_FROM_BOFFS7(BIT_OFFSET_7BITS(bitOffset))


#if defined(Z7_ZSTD_DEC_USE_64BIT_LOADS)
  #if (NUM_OFFSET_SYMBOLS_MAX - BIT_OFFSET_DELTA_BITS < 32)
    /* if (NUM_OFFSET_SYMBOLS_MAX == 32 && BIT_OFFSET_DELTA_BITS == 1),
       we have depth 31 + 9 + 9 + 8 = 57 bits that can b read with single read. */
    #define Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF
  #endif
  #ifndef Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF
    #if (BIT_OFFSET_DELTA_BITS == 1)
    /* if (winLimit - winPos <= (kBlockSizeMax = (1 << 17)))
       {
         the case (16 bits literal extra + 16 match extra) is not possible
         in correct stream. So error will be detected for (16 + 16) case.
         And longest correct sequence after offset reading is (31 + 9 + 9 + 8 = 57 bits).
         So we can use just one 64-bit load here in that case.
       }
    */
    #define Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML
    #endif
  #endif
#endif


#if !defined(Z7_ZSTD_DEC_USE_64BIT_LOADS) || \
    (!defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF) && \
     !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML))
// in : (0 < bits <= (24 or 25)):
#define STREAM_READ_BITS(dest, bits) \
{ \
  GET32(dest, src + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  dest <<= GET_SHIFT(bitOffset); \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  dest >>= 32 - bits; \
}
#endif


#define FSE_Peek_1(table, state)  table[state]

#define STATE_VAR(name)  state_ ## name

// in : (0 <= accuracy <= (24 or 25))
#define FSE_INIT_STATE(name, cond) \
{ \
  UInt32 r; \
  const unsigned bits = p->name ## _accuracy; \
  GET32(r, src + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  r <<= GET_SHIFT(bitOffset); \
  r >>= 1; \
  r >>= 31 ^ bits; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  cond \
  STATE_VAR(name) = FSE_Peek_1(FSE_TABLE(name), r); \
  /* STATE_VAR(name) = dest << 16; */ \
}


#define FSE_Peek_Plus(name, r)  \
  STATE_VAR(name) = FSE_Peek_1(FSE_TABLE(name), \
    GET_FSE_REC_STATE(STATE_VAR(name)) + r);

#define LZ_LOOP_ERROR_EXIT  { return SZ_ERROR_DATA; }

#define BO_OVERFLOW_CHECK \
  { if ((CBitCtr_signed)bitOffset < 0) LZ_LOOP_ERROR_EXIT }


#ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS

#define GET64(dest, p)  { const Byte *ptr = p;  dest = GetUi64(ptr); }

#define FSE_PRELOAD \
{ \
  GET64(v, src - 4 + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  v <<= GET_SHIFT(bitOffset); \
}

#define FSE_UPDATE_STATE_2(name, cond) \
{ \
  const unsigned bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  UInt64 r = v; \
  v <<= bits; \
  r >>= 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  cond \
  r >>= 63 ^ bits; \
  FSE_Peek_Plus(name, r); \
}

#define FSE_UPDATE_STATES \
  FSE_UPDATE_STATE_2 (ll, {} ) \
  FSE_UPDATE_STATE_2 (ml, {} ) \
  FSE_UPDATE_STATE_2 (of, BO_OVERFLOW_CHECK) \

#else // Z7_ZSTD_DEC_USE_64BIT_LOADS

// it supports 8 bits accuracy for any code
// it supports 9 bits accuracy, if (BIT_OFFSET_DELTA_BITS == 1)
#define FSE_UPDATE_STATE_0(name, cond) \
{ \
  UInt32 r; \
  const unsigned bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  GET16(r, src + 2 + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  r >>= (bitOffset & 7); \
  r &= (1 << (8 + BIT_OFFSET_DELTA_BITS)) - 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  cond \
  r >>= (8 + BIT_OFFSET_DELTA_BITS) - bits; \
  FSE_Peek_Plus(name, r); \
}

// for debug (slow):
// #define Z7_ZSTD_DEC_USE_FSE_FUSION_FORCE
#if BIT_OFFSET_DELTA_BITS == 0 || defined(Z7_ZSTD_DEC_USE_FSE_FUSION_FORCE)
  #define Z7_ZSTD_DEC_USE_FSE_FUSION
#endif

#ifdef Z7_ZSTD_DEC_USE_FSE_FUSION
#define FSE_UPDATE_STATE_1(name) \
{ UInt32 rest2; \
{ \
  UInt32 r; \
  unsigned bits; \
  GET32(r, src + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  r <<= GET_SHIFT(bitOffset); \
  rest2 = r << bits; \
  r >>= 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  r >>= 31 ^ bits; \
  FSE_Peek_Plus(name, r); \
}

#define FSE_UPDATE_STATE_3(name) \
{ \
  const unsigned bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  rest2 >>= 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  rest2 >>= 31 ^ bits; \
  FSE_Peek_Plus(name, rest2); \
}}

#define FSE_UPDATE_STATES \
  FSE_UPDATE_STATE_1 (ll) \
  FSE_UPDATE_STATE_3 (ml) \
  FSE_UPDATE_STATE_0 (of, BO_OVERFLOW_CHECK) \

#else // Z7_ZSTD_DEC_USE_64BIT_LOADS

#define FSE_UPDATE_STATES \
  FSE_UPDATE_STATE_0 (ll, {} ) \
  FSE_UPDATE_STATE_0 (ml, {} ) \
  FSE_UPDATE_STATE_0 (of, BO_OVERFLOW_CHECK) \

#endif // Z7_ZSTD_DEC_USE_FSE_FUSION
#endif // Z7_ZSTD_DEC_USE_64BIT_LOADS



typedef struct
{
  UInt32 numSeqs;
  UInt32 literalsLen;
  const Byte *literals;
}
CZstdDec1_Vars;


// if (BIT_OFFSET_DELTA_BITS != 0), we need (BIT_OFFSET_DELTA_BYTES > 0)
#define BIT_OFFSET_DELTA_BYTES   BIT_OFFSET_DELTA_BITS

/* if (NUM_OFFSET_SYMBOLS_MAX == 32)
     max_seq_bit_length = (31) + 16 + 16 + 9 + 8 + 9 = 89 bits
   if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF) we have longest backward
     lookahead offset, and we read UInt64 after literal_len reading.
   if (BIT_OFFSET_DELTA_BITS == 1 && NUM_OFFSET_SYMBOLS_MAX == 32)
     MAX_BACKWARD_DEPTH = 16 bytes
*/
#define MAX_BACKWARD_DEPTH  \
    ((NUM_OFFSET_SYMBOLS_MAX - 1 + 16 + 16 + 7) / 8 + 7 + BIT_OFFSET_DELTA_BYTES)

/* srcLen != 0
   src == real_data_ptr - SEQ_SRC_OFFSET - BIT_OFFSET_DELTA_BYTES
   if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML) then
     (winLimit - p->winPos <= (1 << 17)) is required
*/
static
Z7_NO_INLINE
// Z7_ATTRIB_NO_VECTOR
SRes Decompress_Sequences(CZstdDec1 * const p,
    const Byte *src, const size_t srcLen,
    const size_t winLimit,
    const CZstdDec1_Vars * const vars)
{
#ifdef Z7_ZSTD_DEC_USE_BASES_LOCAL
  SEQ_EXTRA_TABLES(a_)
#endif

  // for debug:
  // #define Z7_ZSTD_DEC_USE_LOCAL_FSE_TABLES
#ifdef Z7_ZSTD_DEC_USE_LOCAL_FSE_TABLES
  #define FSE_TABLE(n)  fse. n
  const CZstdDecFseTables fse = p->fse;
  /*
  CZstdDecFseTables fse;
  #define COPY_FSE_TABLE(n) \
    memcpy(fse. n, p->fse. n, (size_t)4 << p-> n ## _accuracy);
  COPY_FSE_TABLE(of)
  COPY_FSE_TABLE(ll)
  COPY_FSE_TABLE(ml)
  */
#else
  #define FSE_TABLE(n)  (p->fse.  n)
#endif

#ifdef Z7_ZSTD_DEC_USE_BASES_LOCAL
  FILL_LOC_BASES_ALL
#endif

  {
    unsigned numSeqs = vars->numSeqs;
    const Byte *literals = vars->literals;
    ptrdiff_t literalsLen = (ptrdiff_t)vars->literalsLen;
    Byte * const win = p->win;
    size_t winPos = p->winPos;
    const size_t cycSize = p->cycSize;
    size_t totalOutCheck = p->totalOutCheck;
    const size_t winSize = p->winSize;
    size_t reps_0 = p->reps[0];
    size_t reps_1 = p->reps[1];
    size_t reps_2 = p->reps[2];
    UInt32 STATE_VAR(ll), STATE_VAR(of), STATE_VAR(ml);
    CBitCtr bitOffset;

    SET_bitOffset_TO_PAD (bitOffset, src + SEQ_SRC_OFFSET, srcLen + BIT_OFFSET_DELTA_BYTES)

    bitOffset -= BIT_OFFSET_DELTA_BITS;

    FSE_INIT_STATE(ll, {} )
    FSE_INIT_STATE(of, {} )
    FSE_INIT_STATE(ml, BO_OVERFLOW_CHECK)

    for (;;)
    {
      size_t matchLen;
    #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
      UInt64 v;
    #endif

      #ifdef Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF
        FSE_PRELOAD
      #endif

      // if (of_code == 0)
      if ((Byte)STATE_VAR(of) == 0)
      {
        if (GET_FSE_REC_SYM(STATE_VAR(ll)) == 0)
        {
          const size_t offset = reps_1;
          reps_1 = reps_0;
          reps_0 = offset;
          STAT_INC(g_Num_Rep1)
        }
        STAT_UPDATE(else g_Num_Rep0++;)
      }
      else
      {
        const unsigned of_code = (Byte)STATE_VAR(of);

      #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
        #if !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF)
          FSE_PRELOAD
        #endif
      #else
        UInt32 v;
        {
          const Byte *src4 = src + SRC_PLUS_FOR_4BYTES(bitOffset);
          const unsigned skip = GET_SHIFT(bitOffset);
          GET32(v, src4)
          v <<= skip;
          v |= (UInt32)src4[-1] >> (8 - skip);
        }
      #endif

        UPDATE_BIT_OFFSET(bitOffset, of_code)

        if (of_code == 1)
        {
          // read 1 bit
          #if defined(Z7_MSC_VER_ORIGINAL) || defined(MY_CPU_X86_OR_AMD64)
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              #define CHECK_HIGH_BIT_64(a)  ((Int64)(UInt64)(a) < 0)
            #else
              #define CHECK_HIGH_BIT_32(a)  ((Int32)(UInt32)(a) < 0)
            #endif
          #else
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              #define CHECK_HIGH_BIT_64(a)  ((UInt64)(a) & ((UInt64)1 << 63))
            #else
              #define CHECK_HIGH_BIT_32(a)  ((UInt32)(a) & ((UInt32)1 << 31))
            #endif
          #endif

          if
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              CHECK_HIGH_BIT_64 (((UInt64)GET_FSE_REC_SYM(STATE_VAR(ll)) - 1) ^ v)
            #else
              CHECK_HIGH_BIT_32 (((UInt32)GET_FSE_REC_SYM(STATE_VAR(ll)) - 1) ^ v)
            #endif
          {
            v <<= 1;
            {
              const size_t offset = reps_2;
              reps_2 = reps_1;
              reps_1 = reps_0;
              reps_0 = offset;
              STAT_INC(g_Num_Rep2)
            }
          }
          else
          {
            if (GET_FSE_REC_SYM(STATE_VAR(ll)) == 0)
            {
              // litLen == 0 && bit == 1
              STAT_INC(g_Num_Rep3)
              v <<= 1;
              reps_2 = reps_1;
              reps_1 = reps_0;
              if (--reps_0 == 0)
              {
                // LZ_LOOP_ERROR_EXIT
                // original-zstd decoder : input is corrupted; force offset to 1
                // reps_0 = 1;
                reps_0++;
              }
            }
            else
            {
              // litLen != 0 && bit == 0
              v <<= 1;
              {
                const size_t offset = reps_1;
                reps_1 = reps_0;
                reps_0 = offset;
                STAT_INC(g_Num_Rep1)
              }
            }
          }
        }
        else
        {
          // (2 <= of_code)
          // if (of_code >= 32) LZ_LOOP_ERROR_EXIT // optional check
          // we don't allow (of_code >= 32) cases in another code
          reps_2 = reps_1;
          reps_1 = reps_0;
          reps_0 = ((size_t)1 << of_code) - 3 + (size_t)
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              (v >> (64 - of_code));
              v <<= of_code;
            #else
              (v >> (32 - of_code));
            #endif
        }
      }

      #ifdef Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML
        FSE_PRELOAD
      #endif

      matchLen = (size_t)GET_FSE_REC_SYM(STATE_VAR(ml))
          #ifndef Z7_ZSTD_DEC_USE_ML_PLUS3
            + MATCH_LEN_MIN
          #endif
          ;
      {
        {
          if (matchLen >= 32 + MATCH_LEN_MIN) // if (state_ml & 0x20)
          {
            const unsigned extra = BASES_TABLE(SEQ_ML_EXTRA) [(size_t)matchLen - MATCH_LEN_MIN];
            matchLen = BASES_TABLE(SEQ_ML_BASES) [(size_t)matchLen - MATCH_LEN_MIN];
            #if defined(Z7_ZSTD_DEC_USE_64BIT_LOADS) && \
               (defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML) || \
                defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF))
            {
              UPDATE_BIT_OFFSET(bitOffset, extra)
              matchLen += (size_t)(v >> (64 - extra));
              #if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF)
                FSE_PRELOAD
              #else
                v <<= extra;
              #endif
            }
            #else
            {
              UInt32 v32;
              STREAM_READ_BITS(v32, extra)
              matchLen += v32;
            }
            #endif
            STAT_INC(g_Num_Match)
          }
        }
      }

      #if  defined(Z7_ZSTD_DEC_USE_64BIT_LOADS) && \
          !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF) && \
          !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML)
        FSE_PRELOAD
      #endif

      {
        size_t litLen = GET_FSE_REC_SYM(STATE_VAR(ll));
        if (litLen)
        {
          // if (STATE_VAR(ll) & 0x70)
          if (litLen >= 16)
          {
            const unsigned extra = BASES_TABLE(SEQ_LL_EXTRA) [litLen];
            litLen = BASES_TABLE(SEQ_LL_BASES) [litLen];
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
            {
              UPDATE_BIT_OFFSET(bitOffset, extra)
              litLen += (size_t)(v >> (64 - extra));
              #if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF)
                FSE_PRELOAD
              #else
                v <<= extra;
              #endif
            }
            #else
            {
              UInt32 v32;
              STREAM_READ_BITS(v32, extra)
              litLen += v32;
            }
            #endif
            STAT_INC(g_Num_LitsBig)
          }

          if ((literalsLen -= (ptrdiff_t)litLen) < 0)
            LZ_LOOP_ERROR_EXIT
          totalOutCheck += litLen;
          {
            const size_t rem = winLimit - winPos;
            if (litLen > rem)
              LZ_LOOP_ERROR_EXIT
            {
              const Byte *literals_temp = literals;
              Byte *d = win + winPos;
              literals += litLen;
              winPos += litLen;
              CopyLiterals(d, literals_temp, litLen, rem);
            }
          }
        }
        STAT_UPDATE(else g_Num_Lit0++;)
      }

      #define COPY_MATCH \
        { if (reps_0 > winSize || reps_0 > totalOutCheck) LZ_LOOP_ERROR_EXIT \
        totalOutCheck += matchLen; \
        { const size_t rem = winLimit - winPos; \
        if (matchLen > rem) LZ_LOOP_ERROR_EXIT \
        { const size_t winPos_temp = winPos; \
        winPos += matchLen; \
        CopyMatch(reps_0, matchLen, win, winPos_temp, rem, cycSize); }}}

      if (--numSeqs == 0)
      {
        COPY_MATCH
        break;
      }
      FSE_UPDATE_STATES
      COPY_MATCH
    } // for

    if ((CBitCtr_signed)bitOffset != BIT_OFFSET_DELTA_BYTES * 8 - BIT_OFFSET_DELTA_BITS)
      return SZ_ERROR_DATA;
    
    if (literalsLen)
    {
      const size_t rem = winLimit - winPos;
      if ((size_t)literalsLen > rem)
        return SZ_ERROR_DATA;
      {
        Byte *d = win + winPos;
        winPos += (size_t)literalsLen;
        totalOutCheck += (size_t)literalsLen;
        CopyLiterals
        // memcpy
          (d, literals, (size_t)literalsLen, rem);
      }
    }
    if (totalOutCheck >= winSize)
      totalOutCheck = winSize;
    p->totalOutCheck = totalOutCheck;
    p->winPos = winPos;
    p->reps[0] = (CZstdDecOffset)reps_0;
    p->reps[1] = (CZstdDecOffset)reps_1;
    p->reps[2] = (CZstdDecOffset)reps_2;
  }
  return SZ_OK;
}


// for debug: define to check that ZstdDec1_NeedTempBufferForInput() works correctly:
// #define Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP // define it for debug only
#ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
static unsigned g_numSeqs;
#endif


#define k_LitBlockType_Flag_RLE_or_Treeless  1
#define k_LitBlockType_Flag_Compressed       2

// outLimit : is strong limit
// outLimit <= ZstdDec1_GET_BLOCK_SIZE_LIMIT(p)
// inSize != 0
static
Z7_NO_INLINE
SRes ZstdDec1_DecodeBlock(CZstdDec1 *p,
    const Byte *src, SizeT inSize, SizeT afterAvail,
    const size_t outLimit)
{
  CZstdDec1_Vars vars;
  vars.literals = p->literalsBase;
  {
    const unsigned b0 = *src++;
    UInt32 numLits, compressedSize;
    const Byte *litStream;
    Byte *literalsDest;
    inSize--;
    
    if ((b0 & k_LitBlockType_Flag_Compressed) == 0)
    {
      // we need at least one additional byte for (numSeqs).
      // so we check for that additional byte in conditions.
      numLits = b0 >> 3;
      if (b0 & 4)
      {
        UInt32 v;
        if (inSize < 1 + 1) // we need at least 1 byte here and 1 byte for (numSeqs).
          return SZ_ERROR_DATA;
        numLits >>= 1;
        v = GetUi16(src);
        src += 2;
        inSize -= 2;
        if ((b0 & 8) == 0)
        {
          src--;
          inSize++;
          v = (Byte)v;
        }
        numLits += v << 4;
      }
      compressedSize = 1;
      if ((b0 & k_LitBlockType_Flag_RLE_or_Treeless) == 0)
        compressedSize = numLits;
    }
    else if (inSize < 4)
      return SZ_ERROR_DATA;
    else
    {
      const unsigned mode4Streams = b0 & 0xc;
      const unsigned numBytes = (3 * mode4Streams + 32) >> 4;
      const unsigned numBits = 4 * numBytes - 2;
      const UInt32 mask = ((UInt32)16 << numBits) - 1;
      compressedSize = GetUi32(src);
      numLits = ((
          #ifdef MY_CPU_LE_UNALIGN
            GetUi32(src - 1)
          #else
            ((compressedSize << 8) + b0)
          #endif
          ) >> 4) & mask;
      src += numBytes;
      inSize -= numBytes;
      compressedSize >>= numBits;
      compressedSize &= mask;
      /*
      if (numLits != 0) printf("inSize = %7u num_lits=%7u compressed=%7u ratio = %u  ratio2 = %u\n",
          i1, numLits, (unsigned)compressedSize * 1, (unsigned)compressedSize * 100 / numLits,
          (unsigned)numLits * 100 / (unsigned)inSize);
      }
      */
      if (compressedSize == 0)
        return SZ_ERROR_DATA; // (compressedSize == 0) is not allowed
    }

    STAT_UPDATE(g_Num_Lits += numLits;)

    vars.literalsLen = numLits;

    if (compressedSize >= inSize)
      return SZ_ERROR_DATA;
    litStream = src;
    src += compressedSize;
    inSize -= compressedSize;
    // inSize != 0
    {
      UInt32 numSeqs = *src++;
      inSize--;
      if (numSeqs > 127)
      {
        UInt32 b1;
        if (inSize == 0)
          return SZ_ERROR_DATA;
        numSeqs -= 128;
        b1 = *src++;
        inSize--;
        if (numSeqs == 127)
        {
          if (inSize == 0)
            return SZ_ERROR_DATA;
          numSeqs = (UInt32)(*src++) + 127;
          inSize--;
        }
        numSeqs = (numSeqs << 8) + b1;
      }
      if (numSeqs * MATCH_LEN_MIN + numLits > outLimit)
        return SZ_ERROR_DATA;
      vars.numSeqs = numSeqs;

      STAT_UPDATE(g_NumSeqs_total += numSeqs;)
      /*
        #ifdef SHOW_STAT
        printf("\n %5u : %8u, %8u : %5u", (int)g_Num_Blocks_Compressed, (int)numSeqs, (int)g_NumSeqs_total,
          (int)g_NumSeqs_total / g_Num_Blocks_Compressed);
        #endif
        // printf("\nnumSeqs2 = %d", numSeqs);
      */
    #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
      if (numSeqs != g_numSeqs) return SZ_ERROR_DATA; // for debug
    #endif
      if (numSeqs == 0)
      {
        if (inSize != 0)
          return SZ_ERROR_DATA;
        literalsDest = p->win + p->winPos;
      }
      else
        literalsDest = p->literalsBase;
    }
    
    if ((b0 & k_LitBlockType_Flag_Compressed) == 0)
    {
      if (b0 & k_LitBlockType_Flag_RLE_or_Treeless)
      {
        memset(literalsDest, litStream[0], numLits);
        if (vars.numSeqs)
        {
          // literalsDest == p->literalsBase == vars.literals
          #if COPY_CHUNK_SIZE > 1
            memset(p->literalsBase + numLits, 0, COPY_CHUNK_SIZE);
          #endif
        }
      }
      else
      {
        // unsigned y;
        // for (y = 0; y < 10000; y++)
        memcpy(literalsDest, litStream, numLits);
        if (vars.numSeqs)
        {
          /* we need up to (15 == COPY_CHUNK_SIZE - 1) space for optimized CopyLiterals().
             If we have additional space in input stream after literals stream,
             we use direct copy of rar literals in input stream */
          if ((size_t)(src + inSize - litStream) - numLits + afterAvail >= (COPY_CHUNK_SIZE - 1))
            vars.literals = litStream;
          else
          {
            // literalsDest == p->literalsBase == vars.literals
            #if COPY_CHUNK_SIZE > 1
            /* CopyLiterals():
                1) we don't want reading non-initialized data
                2) we will copy only zero byte after literals buffer */
              memset(p->literalsBase + numLits, 0, COPY_CHUNK_SIZE);
            #endif
          }
        }
      }
    }
    else
    {
      CInBufPair hufStream;
      hufStream.ptr = litStream;
      hufStream.len = compressedSize;
      
      if ((b0 & k_LitBlockType_Flag_RLE_or_Treeless) == 0)
      {
        // unsigned y = 100; CInBufPair hs2 = hufStream; do { hufStream = hs2;
        RINOK(Huf_DecodeTable(&p->huf, &hufStream))
        p->litHuf_wasSet = True;
        // } while (--y);
      }
      else if (!p->litHuf_wasSet)
        return SZ_ERROR_DATA;
      
      {
        // int yyy; for (yyy = 0; yyy < 34; yyy++) {
        SRes sres;
        if ((b0 & 0xc) == 0) // mode4Streams
          sres = Huf_Decompress_1stream((const Byte *)(const void *)p->huf.table64,
              hufStream.ptr - HUF_SRC_OFFSET, hufStream.len, literalsDest, numLits);
        else
        {
          // 6 bytes for the jump table + 4x1 bytes of end-padding Bytes)
          if (hufStream.len < 6 + 4)
            return SZ_ERROR_DATA;
          // the condition from original-zstd decoder:
          #define Z7_ZSTD_MIN_LITERALS_FOR_4_STREAMS 6
          if (numLits < Z7_ZSTD_MIN_LITERALS_FOR_4_STREAMS)
            return SZ_ERROR_DATA;
          sres = Huf_Decompress_4stream((const Byte *)(const void *)p->huf.table64,
              hufStream.ptr + (6 - HUF_SRC_OFFSET), hufStream.len, literalsDest, numLits);
        }
        RINOK(sres)
        // }
      }
    }

    if (vars.numSeqs == 0)
    {
      p->winPos += numLits;
      UPDATE_TOTAL_OUT(p, numLits)
      return SZ_OK;
    }
  }
  {
    CInBufPair in;
    unsigned mode;
    unsigned seqMode;
      
    in.ptr = src;
    in.len = inSize;
    if (in.len == 0)
      return SZ_ERROR_DATA;
    in.len--;
    mode = *in.ptr++;
    if (mode & 3) // Reserved bits
      return SZ_ERROR_DATA;
    
    seqMode = (mode >> 6);
    if (seqMode == k_SeqMode_Repeat)
      { if (!IS_SEQ_TABLES_WERE_SET(p)) return SZ_ERROR_DATA; }
    else RINOK(FSE_Decode_SeqTable(
        p->fse.ll,
        &in,
        6, // predefAccuracy
        &p->ll_accuracy,
        NUM_LL_SYMBOLS,
        k_PredefRecords_LL,
        seqMode))
      
    seqMode = (mode >> 4) & 3;
    if (seqMode == k_SeqMode_Repeat)
      { if (!IS_SEQ_TABLES_WERE_SET(p)) return SZ_ERROR_DATA; }
    else RINOK(FSE_Decode_SeqTable(
        p->fse.of,
        &in,
        5, // predefAccuracy
        &p->of_accuracy,
        NUM_OFFSET_SYMBOLS_MAX,
        k_PredefRecords_OF,
        seqMode))
       
    seqMode = (mode >> 2) & 3;
    if (seqMode == k_SeqMode_Repeat)
      { if (!IS_SEQ_TABLES_WERE_SET(p)) return SZ_ERROR_DATA; }
    else
    {
      RINOK(FSE_Decode_SeqTable(
        p->fse.ml,
        &in,
        6, // predefAccuracy
        &p->ml_accuracy,
        NUM_ML_SYMBOLS,
        k_PredefRecords_ML,
        seqMode))
      /*
      #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
        // { unsigned y = 1 << 10; do
      {
        const unsigned accuracy = p->ml_accuracy;
        if (accuracy == 0)
          p->fse.ml[0] += 3;
        else
        #ifdef MY_CPU_64BIT
        {
          // alignemt (UInt64 _pad_Alignment) in fse.ml is required for that code
          UInt64 *table = (UInt64 *)(void *)p->fse.ml;
          const UInt64 *end = (const UInt64 *)(const void *)
            ((const Byte *)(const void *)table + ((size_t)sizeof(CFseRecord) << accuracy));
          do
          {
            table[0] += ((UInt64)MATCH_LEN_MIN << 32) + MATCH_LEN_MIN;
            table[1] += ((UInt64)MATCH_LEN_MIN << 32) + MATCH_LEN_MIN;
            table += 2;
          }
          while (table != end);
        }
        #else
        {
          UInt32 *table = p->fse.ml;
          const UInt32 *end = (const UInt32 *)(const void *)
            ((const Byte *)(const void *)table + ((size_t)sizeof(CFseRecord) << accuracy));
          do
          {
            table[0] += MATCH_LEN_MIN;
            table[1] += MATCH_LEN_MIN;
            table += 2;
            table[0] += MATCH_LEN_MIN;
            table[1] += MATCH_LEN_MIN;
            table += 2;
          }
          while (table != end);
        }
        #endif
      }
      // while (--y); }
      #endif
      */
    }
    
    // p->seqTables_wereSet = True;
    if (in.len == 0)
      return SZ_ERROR_DATA;
    return Decompress_Sequences(p,
        in.ptr - SEQ_SRC_OFFSET - BIT_OFFSET_DELTA_BYTES, in.len,
        p->winPos + outLimit, &vars);
  }
}




// inSize != 0
// it must do similar to ZstdDec1_DecodeBlock()
static size_t ZstdDec1_NeedTempBufferForInput(
    const SizeT beforeSize, const Byte * const src, const SizeT inSize)
{
  unsigned b0;
  UInt32 pos;

  #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
    g_numSeqs = 1 << 24;
  #else
  // we have at least 3 bytes before seq data: litBlockType, numSeqs, seqMode
  #define MIN_BLOCK_LZ_HEADERS_SIZE 3
  if (beforeSize >= MAX_BACKWARD_DEPTH - MIN_BLOCK_LZ_HEADERS_SIZE)
    return 0;
  #endif

  b0 = src[0];
  
  if ((b0 & k_LitBlockType_Flag_Compressed) == 0)
  {
    UInt32 numLits = b0 >> 3;
    pos = 1;
    if (b0 & 4)
    {
      UInt32 v;
      if (inSize < 3)
        return 0;
      numLits >>= 1;
      v = GetUi16(src + 1);
      pos = 3;
      if ((b0 & 8) == 0)
      {
        pos = 2;
        v = (Byte)v;
      }
      numLits += v << 4;
    }
    if (b0 & k_LitBlockType_Flag_RLE_or_Treeless)
      numLits = 1;
    pos += numLits;
  }
  else if (inSize < 5)
    return 0;
  else
  {
    const unsigned mode4Streams = b0 & 0xc;
    const unsigned numBytes = (3 * mode4Streams + 48) >> 4;
    const unsigned numBits = 4 * numBytes - 6;
    UInt32 cs = GetUi32(src + 1);
    cs >>= numBits;
    cs &= ((UInt32)16 << numBits) - 1;
    if (cs == 0)
      return 0;
    pos = numBytes + cs;
  }
  
  if (pos >= inSize)
    return 0;
  {
    UInt32 numSeqs = src[pos++];
    if (numSeqs > 127)
    {
      UInt32 b1;
      if (pos >= inSize)
        return 0;
      numSeqs -= 128;
      b1 = src[pos++];
      if (numSeqs == 127)
      {
        if (pos >= inSize)
          return 0;
        numSeqs = (UInt32)(src[pos++]) + 127;
      }
      numSeqs = (numSeqs << 8) + b1;
    }
    #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
      g_numSeqs = numSeqs; // for debug
    #endif
    if (numSeqs == 0)
      return 0;
  }
  /*
  if (pos >= inSize)
    return 0;
  pos++;
  */
  // we will have one additional byte for seqMode:
  if (beforeSize + pos >= MAX_BACKWARD_DEPTH - 1)
    return 0;
  return 1;
}



// ---------- ZSTD FRAME ----------

#define kBlockType_Raw          0
#define kBlockType_RLE          1
#define kBlockType_Compressed   2
#define kBlockType_Reserved     3

typedef enum
{
  // begin: states that require 4 bytes:
  ZSTD2_STATE_SIGNATURE,
  ZSTD2_STATE_HASH,
  ZSTD2_STATE_SKIP_HEADER,
  // end of states that require 4 bytes

  ZSTD2_STATE_SKIP_DATA,
  ZSTD2_STATE_FRAME_HEADER,
  ZSTD2_STATE_AFTER_HEADER,
  ZSTD2_STATE_BLOCK,
  ZSTD2_STATE_DATA,
  ZSTD2_STATE_FINISHED
} EZstd2State;


struct CZstdDec
{
  EZstd2State frameState;
  unsigned tempSize;
  
  Byte temp[14]; // 14 is required

  Byte descriptor;
  Byte windowDescriptor;
  Byte isLastBlock;
  Byte blockType;
  Byte isErrorState;
  Byte hashError;
  Byte disableHash;
  Byte isCyclicMode;
  
  UInt32 blockSize;
  UInt32 dictionaryId;
  UInt32 curBlockUnpackRem; // for compressed blocks only
  UInt32 inTempPos;

  UInt64 contentSize;
  UInt64 contentProcessed;
  CXxh64State xxh64;

  Byte *inTemp;
  SizeT winBufSize_Allocated;
  Byte *win_Base;

  ISzAllocPtr alloc_Small;
  ISzAllocPtr alloc_Big;

  CZstdDec1 decoder;
};

#define ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p) \
  ((unsigned)(p)->contentProcessed & (Z7_XXH64_BLOCK_SIZE - 1))

#define ZSTD_DEC_IS_LAST_BLOCK(p) ((p)->isLastBlock)


static void ZstdDec_FreeWindow(CZstdDec * const p)
{
  if (p->win_Base)
  {
    ISzAlloc_Free(p->alloc_Big, p->win_Base);
    p->win_Base = NULL;
    // p->decoder.win = NULL;
    p->winBufSize_Allocated = 0;
  }
}


CZstdDecHandle ZstdDec_Create(ISzAllocPtr alloc_Small, ISzAllocPtr alloc_Big)
{
  CZstdDec *p = (CZstdDec *)ISzAlloc_Alloc(alloc_Small, sizeof(CZstdDec));
  if (!p)
    return NULL;
  p->alloc_Small = alloc_Small;
  p->alloc_Big = alloc_Big;
  // ZstdDec_CONSTRUCT(p)
  p->inTemp = NULL;
  p->win_Base = NULL;
  p->winBufSize_Allocated = 0;
  p->disableHash = False;
  ZstdDec1_Construct(&p->decoder);
  return p;
}

void ZstdDec_Destroy(CZstdDecHandle p)
{
  #ifdef SHOW_STAT
    #define PRINT_STAT1(name, v) \
      printf("\n%25s = %9u", name, v);
  PRINT_STAT1("g_Num_Blocks_Compressed", g_Num_Blocks_Compressed)
  PRINT_STAT1("g_Num_Blocks_memcpy", g_Num_Blocks_memcpy)
  PRINT_STAT1("g_Num_Wrap_memmove_Num", g_Num_Wrap_memmove_Num)
  PRINT_STAT1("g_Num_Wrap_memmove_Bytes", g_Num_Wrap_memmove_Bytes)
  if (g_Num_Blocks_Compressed)
  {
    #define PRINT_STAT(name, v) \
      printf("\n%17s = %9u, per_block = %8u", name, v, v / g_Num_Blocks_Compressed);
    PRINT_STAT("g_NumSeqs", g_NumSeqs_total)
    // PRINT_STAT("g_NumCopy", g_NumCopy)
    PRINT_STAT("g_NumOver", g_NumOver)
    PRINT_STAT("g_NumOver2", g_NumOver2)
    PRINT_STAT("g_Num_Match", g_Num_Match)
    PRINT_STAT("g_Num_Lits", g_Num_Lits)
    PRINT_STAT("g_Num_LitsBig", g_Num_LitsBig)
    PRINT_STAT("g_Num_Lit0", g_Num_Lit0)
    PRINT_STAT("g_Num_Rep_0", g_Num_Rep0)
    PRINT_STAT("g_Num_Rep_1", g_Num_Rep1)
    PRINT_STAT("g_Num_Rep_2", g_Num_Rep2)
    PRINT_STAT("g_Num_Rep_3", g_Num_Rep3)
    PRINT_STAT("g_Num_Threshold_0", g_Num_Threshold_0)
    PRINT_STAT("g_Num_Threshold_1", g_Num_Threshold_1)
    PRINT_STAT("g_Num_Threshold_0sum", g_Num_Threshold_0sum)
    PRINT_STAT("g_Num_Threshold_1sum", g_Num_Threshold_1sum)
  }
  printf("\n");
  #endif

  ISzAlloc_Free(p->alloc_Small, p->decoder.literalsBase);
  // p->->decoder.literalsBase = NULL;
  ISzAlloc_Free(p->alloc_Small, p->inTemp);
  // p->inTemp = NULL;
  ZstdDec_FreeWindow(p);
  ISzAlloc_Free(p->alloc_Small, p);
}



#define kTempBuffer_PreSize  (1u << 6)
#if kTempBuffer_PreSize < MAX_BACKWARD_DEPTH
  #error Stop_Compiling_Bad_kTempBuffer_PreSize
#endif

static SRes ZstdDec_AllocateMisc(CZstdDec *p)
{
  #define k_Lit_AfterAvail  (1u << 6)
  #if k_Lit_AfterAvail < (COPY_CHUNK_SIZE - 1)
    #error Stop_Compiling_Bad_k_Lit_AfterAvail
  #endif
  // return ZstdDec1_Allocate(&p->decoder, p->alloc_Small);
  if (!p->decoder.literalsBase)
  {
    p->decoder.literalsBase = (Byte *)ISzAlloc_Alloc(p->alloc_Small,
        kBlockSizeMax + k_Lit_AfterAvail);
    if (!p->decoder.literalsBase)
      return SZ_ERROR_MEM;
  }
  if (!p->inTemp)
  {
    // we need k_Lit_AfterAvail here for owerread from raw literals stream
    p->inTemp = (Byte *)ISzAlloc_Alloc(p->alloc_Small,
        kBlockSizeMax + kTempBuffer_PreSize + k_Lit_AfterAvail);
    if (!p->inTemp)
      return SZ_ERROR_MEM;
  }
  return SZ_OK;
}


static void ZstdDec_Init_ForNewFrame(CZstdDec *p)
{
  p->frameState = ZSTD2_STATE_SIGNATURE;
  p->tempSize = 0;

  p->isErrorState = False;
  p->hashError = False;
  p->isCyclicMode = False;
  p->contentProcessed = 0;
  Xxh64State_Init(&p->xxh64);
  ZstdDec1_Init(&p->decoder);
}


void ZstdDec_Init(CZstdDec *p)
{
  ZstdDec_Init_ForNewFrame(p);
  p->decoder.winPos = 0;
  memset(p->temp, 0, sizeof(p->temp));
}


#define DESCRIPTOR_Get_DictionaryId_Flag(d)   ((d) & 3)
#define DESCRIPTOR_FLAG_CHECKSUM              (1 << 2)
#define DESCRIPTOR_FLAG_RESERVED              (1 << 3)
// #define DESCRIPTOR_FLAG_UNUSED                (1 << 4)
#define DESCRIPTOR_FLAG_SINGLE                (1 << 5)
#define DESCRIPTOR_Get_ContentSize_Flag3(d)   ((d) >> 5)
#define DESCRIPTOR_Is_ContentSize_Defined(d)  (((d) & 0xe0) != 0)


static EZstd2State ZstdDec_UpdateState(CZstdDec * const p, const Byte b, CZstdDecInfo * const info)
{
  unsigned tempSize = p->tempSize;
  p->temp[tempSize++] = b;
  p->tempSize = tempSize;

  if (p->frameState == ZSTD2_STATE_BLOCK)
  {
    if (tempSize < 3)
      return ZSTD2_STATE_BLOCK;
    {
      UInt32 b0 = GetUi32(p->temp);
      const unsigned type = ((unsigned)b0 >> 1) & 3;
      if (type == kBlockType_RLE && tempSize == 3)
        return ZSTD2_STATE_BLOCK;
      // info->num_Blocks_forType[type]++;
      info->num_Blocks++;
      if (type == kBlockType_Reserved)
      {
        p->isErrorState = True; // SZ_ERROR_UNSUPPORTED
        return ZSTD2_STATE_BLOCK;
      }
      p->blockType = (Byte)type;
      p->isLastBlock = (Byte)(b0 & 1);
      p->inTempPos = 0;
      p->tempSize = 0;
      b0 >>= 3;
      b0 &= 0x1fffff;
      // info->num_BlockBytes_forType[type] += b0;
      if (b0 == 0)
      {
        // empty RAW/RLE blocks are allowed in original-zstd decoder
        if (type == kBlockType_Compressed)
        {
          p->isErrorState = True;
          return ZSTD2_STATE_BLOCK;
        }
        if (!ZSTD_DEC_IS_LAST_BLOCK(p))
          return ZSTD2_STATE_BLOCK;
        if (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM)
          return ZSTD2_STATE_HASH;
        return ZSTD2_STATE_FINISHED;
      }
      p->blockSize = b0;
      {
        UInt32 blockLim = ZstdDec1_GET_BLOCK_SIZE_LIMIT(&p->decoder);
        // compressed and uncompressed block sizes cannot be larger than min(kBlockSizeMax, window_size)
        if (b0 > blockLim)
        {
          p->isErrorState = True; // SZ_ERROR_UNSUPPORTED;
          return ZSTD2_STATE_BLOCK;
        }
        if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor))
        {
          const UInt64 rem = p->contentSize - p->contentProcessed;
          if (blockLim > rem)
              blockLim = (UInt32)rem;
        }
        p->curBlockUnpackRem = blockLim;
        // uncompressed block size cannot be larger than remain data size:
        if (type != kBlockType_Compressed)
        {
          if (b0 > blockLim)
          {
            p->isErrorState = True; // SZ_ERROR_UNSUPPORTED;
            return ZSTD2_STATE_BLOCK;
          }
        }
      }
    }
    return ZSTD2_STATE_DATA;
  }
  
  if ((unsigned)p->frameState < ZSTD2_STATE_SKIP_DATA)
  {
    UInt32 v;
    if (tempSize != 4)
      return p->frameState;
    v = GetUi32(p->temp);
    if ((unsigned)p->frameState < ZSTD2_STATE_HASH) // == ZSTD2_STATE_SIGNATURE
    {
      if (v == 0xfd2fb528)
      {
        p->tempSize = 0;
        info->num_DataFrames++;
        return ZSTD2_STATE_FRAME_HEADER;
      }
      if ((v & 0xfffffff0) == 0x184d2a50)
      {
        p->tempSize = 0;
        info->num_SkipFrames++;
        return ZSTD2_STATE_SKIP_HEADER;
      }
      p->isErrorState = True;
      return ZSTD2_STATE_SIGNATURE;
      // return ZSTD2_STATE_ERROR; // is not ZSTD stream
    }
    if (p->frameState == ZSTD2_STATE_HASH)
    {
      info->checksum_Defined = True;
      info->checksum = v;
      // #ifndef DISABLE_XXH_CHECK
      if (!p->disableHash)
      {
        if (p->decoder.winPos < ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p))
        {
          // unexpected code failure
          p->isErrorState = True;
          // SZ_ERROR_FAIL;
        }
        else
        if ((UInt32)Xxh64State_Digest(&p->xxh64,
            p->decoder.win + (p->decoder.winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p)),
            p->contentProcessed) != v)
        {
          p->hashError = True;
          // return ZSTD2_STATE_ERROR; // hash error
        }
      }
      // #endif
      return ZSTD2_STATE_FINISHED;
    }
    // (p->frameState == ZSTD2_STATE_SKIP_HEADER)
    {
      p->blockSize = v;
      info->skipFrames_Size += v;
      p->tempSize = 0;
      /* we want the caller could know that there was finished frame
         finished frame. So we allow the case where
         we have ZSTD2_STATE_SKIP_DATA state with (blockSize == 0).
      */
      // if (v == 0) return ZSTD2_STATE_SIGNATURE;
      return ZSTD2_STATE_SKIP_DATA;
    }
  }

  // if (p->frameState == ZSTD2_STATE_FRAME_HEADER)
  {
    unsigned descriptor;
    const Byte *h;
    descriptor = p->temp[0];
    p->descriptor = (Byte)descriptor;
    if (descriptor & DESCRIPTOR_FLAG_RESERVED) // reserved bit
    {
      p->isErrorState = True;
      return ZSTD2_STATE_FRAME_HEADER;
      // return ZSTD2_STATE_ERROR;
    }
    {
      const unsigned n = DESCRIPTOR_Get_ContentSize_Flag3(descriptor);
      // tempSize -= 1 + ((1u << (n >> 1)) | ((n + 1) & 1));
      tempSize -= (0x9a563422u >> (n * 4)) & 0xf;
    }
    if (tempSize != (4u >> (3 - DESCRIPTOR_Get_DictionaryId_Flag(descriptor))))
      return ZSTD2_STATE_FRAME_HEADER;
    
    info->descriptor_OR     = (Byte)(info->descriptor_OR     |  descriptor);
    info->descriptor_NOT_OR = (Byte)(info->descriptor_NOT_OR | ~descriptor);

    h = &p->temp[1];
    {
      Byte w = 0;
      if ((descriptor & DESCRIPTOR_FLAG_SINGLE) == 0)
      {
        w = *h++;
        if (info->windowDescriptor_MAX < w)
            info->windowDescriptor_MAX = w;
        // info->are_WindowDescriptors = True;
        // info->num_WindowDescriptors++;
      }
      else
      {
        // info->are_SingleSegments = True;
        // info->num_SingleSegments++;
      }
      p->windowDescriptor = w;
    }
    {
      unsigned n = DESCRIPTOR_Get_DictionaryId_Flag(descriptor);
      UInt32 d = 0;
      if (n)
      {
        n = 1u << (n - 1);
        d = GetUi32(h) & ((UInt32)(Int32)-1 >> (32 - 8u * n));
        h += n;
      }
      p->dictionaryId = d;
      // info->dictionaryId_Cur = d;
      if (d != 0)
      {
        if (info->dictionaryId == 0)
          info->dictionaryId = d;
        else if (info->dictionaryId != d)
          info->are_DictionaryId_Different = True;
      }
    }
    {
      unsigned n = DESCRIPTOR_Get_ContentSize_Flag3(descriptor);
      UInt64 v = 0;
      if (n)
      {
        n >>= 1;
        if (n == 1)
          v = 256;
        v += GetUi64(h) & ((UInt64)(Int64)-1 >> (64 - (8u << n)));
        // info->are_ContentSize_Known = True;
        // info->num_Frames_with_ContentSize++;
        if (info->contentSize_MAX < v)
            info->contentSize_MAX = v;
        info->contentSize_Total += v;
      }
      else
      {
        info->are_ContentSize_Unknown = True;
        // info->num_Frames_without_ContentSize++;
      }
      p->contentSize = v;
    }
    // if ((size_t)(h - p->temp) != headerSize) return ZSTD2_STATE_ERROR; // it's unexpected internal code failure
    p->tempSize = 0;

    info->checksum_Defined = False;
    /*
    if (descriptor & DESCRIPTOR_FLAG_CHECKSUM)
      info->are_Checksums = True;
    else
      info->are_Non_Checksums = True;
    */

    return ZSTD2_STATE_AFTER_HEADER; // ZSTD2_STATE_BLOCK;
  }
}


static void ZstdDec_Update_XXH(CZstdDec * const p, size_t xxh64_winPos)
{
 /*
 #ifdef DISABLE_XXH_CHECK
  UNUSED_VAR(data)
 #else
 */
  if (!p->disableHash && (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM))
  {
    // const size_t pos = p->xxh64_winPos;
    const size_t size = (p->decoder.winPos - xxh64_winPos) & ~(size_t)31;
    if (size)
    {
      // p->xxh64_winPos = pos + size;
      Xxh64State_UpdateBlocks(&p->xxh64,
          p->decoder.win + xxh64_winPos,
          p->decoder.win + xxh64_winPos + size);
    }
  }
}


/*
in:
  (winLimit) : is relaxed limit, where this function is allowed to stop writing of decoded data (if possible).
    - this function uses (winLimit) for RAW/RLE blocks only,
        because this function can decode single RAW/RLE block in several different calls.
    - this function DOESN'T use (winLimit) for Compressed blocks,
        because this function decodes full compressed block in single call.
  (CZstdDec1::winPos <= winLimit)
  (winLimit <= CZstdDec1::cycSize).
  Note: if (ds->outBuf_fromCaller) mode is used, then
  {
    (strong_limit) is stored in CZstdDec1::cycSize.
    So (winLimit) is more strong than (strong_limit).
  }

exit:
  Note: (CZstdDecState::winPos) will be set by caller after exit of this function.

  This function can exit for any of these conditions:
    - (frameState == ZSTD2_STATE_AFTER_HEADER)
    - (frameState == ZSTD2_STATE_FINISHED) : frame was finished : (status == ZSTD_STATUS_FINISHED_FRAME) is set
    - finished non-empty non-last block. So (CZstdDec1::winPos_atExit != winPos_atFuncStart).
    - ZSTD_STATUS_NEEDS_MORE_INPUT in src
    - (CZstdDec1::winPos) have reached (winLimit) in non-finished RAW/RLE block

  This function decodes no more than one non-empty block.
  So it fulfills the condition at exit:
    (CZstdDec1::winPos_atExit - winPos_atFuncStart <= block_size_max)
  Note: (winPos_atExit > winLimit) is possible in some cases after compressed block decoding.
      
  if (ds->outBuf_fromCaller) mode (useAdditionalWinLimit medo)
  {
    then this function uses additional strong limit from (CZstdDec1::cycSize).
    So this function will not write any data after (CZstdDec1::cycSize)
    And it fulfills the condition at exit:
      (CZstdDec1::winPos_atExit <= CZstdDec1::cycSize)
  }
*/
static SRes ZstdDec_DecodeBlock(CZstdDec * const p, CZstdDecState * const ds,
    SizeT winLimitAdd)
{
  const Byte *src = ds->inBuf;
  SizeT * const srcLen = &ds->inPos;
  const SizeT inSize = ds->inLim;
  // const int useAdditionalWinLimit = ds->outBuf_fromCaller ? 1 : 0;
  enum_ZstdStatus * const status = &ds->status;
  CZstdDecInfo * const info = &ds->info;
  SizeT winLimit;

  const SizeT winPos_atFuncStart = p->decoder.winPos;
  src += *srcLen;
  *status = ZSTD_STATUS_NOT_SPECIFIED;

  // finishMode = ZSTD_FINISH_ANY;
  if (ds->outSize_Defined)
  {
    if (ds->outSize < ds->outProcessed)
    {
      // p->isAfterSizeMode = 2; // we have extra bytes already
      *status = ZSTD_STATUS_OUT_REACHED;
      return SZ_OK;
      // size = 0;
    }
    else
    {
      // p->outSize >= p->outProcessed
      const UInt64 rem = ds->outSize - ds->outProcessed;
      /*
      if (rem == 0)
      p->isAfterSizeMode = 1; // we have reached exact required size
      */
      if (winLimitAdd >= rem)
      {
        winLimitAdd = (SizeT)rem;
        // if (p->finishMode) finishMode = ZSTD_FINISH_END;
      }
    }
  }

  winLimit = p->decoder.winPos + winLimitAdd;
  // (p->decoder.winPos <= winLimit)

  // while (p->frameState != ZSTD2_STATE_ERROR)
  while (!p->isErrorState)
  {
    SizeT inCur = inSize - *srcLen;

    if (p->frameState == ZSTD2_STATE_DATA)
    {
      /* (p->decoder.winPos == winPos_atFuncStart) is expected,
         because this function doesn't start new block.
         if it have finished some non-empty block in this call. */
      if (p->decoder.winPos != winPos_atFuncStart)
        return SZ_ERROR_FAIL; // it's unexpected

      /*
      if (p->decoder.winPos > winLimit)
      {
        // we can be here, if in this function call
        //      - we have extracted non-empty compressed block, and (winPos > winLimit) after that.
        //      - we have started new block decoding after that.
        // It's unexpected case, because we exit after non-empty non-last block.
        *status = (inSize == *srcLen) ?
            ZSTD_STATUS_NEEDS_MORE_INPUT :
            ZSTD_STATUS_NOT_FINISHED;
        return SZ_OK;
      }
      */
      // p->decoder.winPos <= winLimit
      
      if (p->blockType != kBlockType_Compressed)
      {
        // it's RLE or RAW block.
        // p->BlockSize != 0_
        // winLimit <= p->decoder.cycSize
        /* So here we use more strong (winLimit), even for
           (ds->outBuf_fromCaller) mode. */
        SizeT outCur = winLimit - p->decoder.winPos;
        {
          const UInt32 rem = p->blockSize;
          if (outCur > rem)
              outCur = rem;
        }
        if (p->blockType == kBlockType_Raw)
        {
          if (outCur > inCur)
              outCur = inCur;
          /* output buffer is better aligned for XXH code.
             So we use hash for output buffer data */
          // ZstdDec_Update_XXH(p, src, outCur); // for debug:
          memcpy(p->decoder.win + p->decoder.winPos, src, outCur);
          src += outCur;
          *srcLen += outCur;
        }
        else // kBlockType_RLE
        {
          #define RLE_BYTE_INDEX_IN_temp  3
          memset(p->decoder.win + p->decoder.winPos,
              p->temp[RLE_BYTE_INDEX_IN_temp], outCur);
        }
        {
          const SizeT xxh64_winPos = p->decoder.winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p);
          p->decoder.winPos += outCur;
          UPDATE_TOTAL_OUT(&p->decoder, outCur)
          p->contentProcessed += outCur;
          ZstdDec_Update_XXH(p, xxh64_winPos);
        }
        // ds->winPos = p->decoder.winPos;  // the caller does it instead. for debug:
        ds->outProcessed += outCur;
        if (p->blockSize -= (UInt32)outCur)
        {
          /*
          if (ds->outSize_Defined)
          {
            if (ds->outSize <= ds->outProcessed) ds->isAfterSizeMode = (enum_ZstdStatus)
               (ds->outSize == ds->outProcessed ? 1u: 2u);
          }
          */
          *status = (enum_ZstdStatus)
              (ds->outSize_Defined && ds->outSize <= ds->outProcessed ?
                ZSTD_STATUS_OUT_REACHED : (p->blockType == kBlockType_Raw && inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED);
          return SZ_OK;
        }
      }
      else // kBlockType_Compressed
      {
        // p->blockSize != 0
        // (uncompressed_size_of_block == 0) is allowed
        // (p->curBlockUnpackRem == 0) is allowed
        /*
        if (p->decoder.winPos >= winLimit)
        {
          if (p->decoder.winPos != winPos_atFuncStart)
          {
            // it's unexpected case
            // We already have some data in finished blocks in this function call.
            //   So we don't decompress new block after (>=winLimit),
            //   even if it's empty block.
            *status = (inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED;
            return SZ_OK;
          }
          // (p->decoder.winPos == winLimit == winPos_atFuncStart)
          // we will decode current block, because that current
          //   block can be empty block and we want to make some visible
          //   change of (src) stream after function start.
        }
        */
        /*
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          // we don't want to start new block, if we have more extra decoded bytes already
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK;
        }
        */
        {
          const Byte *comprStream;
          size_t afterAvail;
          UInt32 inTempPos = p->inTempPos;
          const UInt32 rem = p->blockSize - inTempPos;
          // rem != 0
          if (inTempPos != 0  // (inTemp) buffer already contains some input data
              || inCur < rem  // available input data size is smaller than compressed block size
              || ZstdDec1_NeedTempBufferForInput(*srcLen, src, rem))
          {
            if (inCur > rem)
                inCur = rem;
            if (inCur)
            {
              STAT_INC(g_Num_Blocks_memcpy)
              // we clear data for backward lookahead reading
              if (inTempPos == 0)
                memset(p->inTemp + kTempBuffer_PreSize - MAX_BACKWARD_DEPTH, 0, MAX_BACKWARD_DEPTH);
              // { unsigned y = 0; for(;y < 1000; y++)
              memcpy(p->inTemp + inTempPos + kTempBuffer_PreSize, src, inCur);
              // }
              src += inCur;
              *srcLen += inCur;
              inTempPos += (UInt32)inCur;
              p->inTempPos = inTempPos;
            }
            if (inTempPos != p->blockSize)
            {
              *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
              return SZ_OK;
            }
            #if COPY_CHUNK_SIZE > 1
              memset(p->inTemp + kTempBuffer_PreSize + inTempPos, 0, COPY_CHUNK_SIZE);
            #endif
            comprStream = p->inTemp + kTempBuffer_PreSize;
            afterAvail = k_Lit_AfterAvail;
            // we don't want to read non-initialized data or junk in CopyMatch():
          }
          else
          {
            // inCur >= rem
            // we use direct decoding from (src) buffer:
            afterAvail = inCur - rem;
            comprStream = src;
            src += rem;
            *srcLen += rem;
          }

          #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
            ZstdDec1_NeedTempBufferForInput(*srcLen, comprStream, p->blockSize);
          #endif
          // printf("\nblockSize=%u", p->blockSize);
          // printf("%x\n", (unsigned)p->contentProcessed);
          STAT_INC(g_Num_Blocks_Compressed)
          {
            SRes sres;
            const size_t winPos = p->decoder.winPos;
            /*
               if ( useAdditionalWinLimit), we use strong unpack limit: smallest from
                  - limit from stream : (curBlockUnpackRem)
                  - limit from caller : (cycSize - winPos)
               if (!useAdditionalWinLimit), we use only relaxed limit:
                  - limit from stream : (curBlockUnpackRem)
            */
            SizeT outLimit = p->curBlockUnpackRem;
            if (ds->outBuf_fromCaller)
            // if (useAdditionalWinLimit)
            {
              const size_t limit = p->decoder.cycSize - winPos;
              if (outLimit > limit)
                  outLimit = limit;
            }
            sres = ZstdDec1_DecodeBlock(&p->decoder,
                comprStream, p->blockSize, afterAvail, outLimit);
            // ds->winPos = p->decoder.winPos;  // the caller does it instead. for debug:
            if (sres)
            {
              p->isErrorState = True;
              return sres;
            }
            {
              const SizeT xxh64_winPos = winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p);
              const size_t num = p->decoder.winPos - winPos;
              ds->outProcessed += num;
              p->contentProcessed += num;
              ZstdDec_Update_XXH(p, xxh64_winPos);
            }
          }
          // printf("\nwinPos=%x", (int)(unsigned)p->decoder.winPos);
        }
      }

      /*
      if (ds->outSize_Defined)
      {
        if (ds->outSize <= ds->outProcessed) ds->isAfterSizeMode = (enum_ZstdStatus)
           (ds->outSize == ds->outProcessed ? 1u: 2u);
      }
      */
      
      if (!ZSTD_DEC_IS_LAST_BLOCK(p))
      {
        p->frameState = ZSTD2_STATE_BLOCK;
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK;
        }
        // we exit only if (winPos) was changed in this function call:
        if (p->decoder.winPos != winPos_atFuncStart)
        {
          // decoded block was not empty. So we exit:
          *status = (enum_ZstdStatus)(
              (inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED);
          return SZ_OK;
        }
        // (p->decoder.winPos == winPos_atFuncStart)
        // so current decoded block was empty.
        // we will try to decode more blocks in this function.
        continue;
      }
      
      // decoded block was last in frame
      if (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM)
      {
        p->frameState = ZSTD2_STATE_HASH;
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK; // disable if want to
          /* We want to get same return codes for any input buffer sizes.
             We want to get faster ZSTD_STATUS_OUT_REACHED status.
             So we exit with ZSTD_STATUS_OUT_REACHED here,
             instead of ZSTD2_STATE_HASH and ZSTD2_STATE_FINISHED processing.
             that depends from input buffer size and that can set
             ZSTD_STATUS_NEEDS_MORE_INPUT or return SZ_ERROR_DATA or SZ_ERROR_CRC.
          */
        }
      }
      else
      {
        /* ZSTD2_STATE_FINISHED proccesing doesn't depend from input buffer */
        p->frameState = ZSTD2_STATE_FINISHED;
      }
      /*
      p->frameState = (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM) ?
          ZSTD2_STATE_HASH :
          ZSTD2_STATE_FINISHED;
      */
      /* it's required to process ZSTD2_STATE_FINISHED state in this function call,
         because we must check contentSize and hashError in ZSTD2_STATE_FINISHED code,
         while the caller can reinit full state for ZSTD2_STATE_FINISHED
         So we can't exit from function here. */
      continue;
    }

    if (p->frameState == ZSTD2_STATE_FINISHED)
    {
      *status = ZSTD_STATUS_FINISHED_FRAME;
      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor)
          && p->contentSize != p->contentProcessed)
        return SZ_ERROR_DATA;
      if (p->hashError) // for debug
        return SZ_ERROR_CRC;
      return SZ_OK;
      // p->frameState = ZSTD2_STATE_SIGNATURE;
      // continue;
    }
    
    if (p->frameState == ZSTD2_STATE_AFTER_HEADER)
      return SZ_OK; // we need memory allocation for that state

    if (p->frameState == ZSTD2_STATE_SKIP_DATA)
    {
      UInt32 blockSize = p->blockSize;
      // (blockSize == 0) is possible
      if (inCur > blockSize)
          inCur = blockSize;
      src += inCur;
      *srcLen += inCur;
      blockSize -= (UInt32)inCur;
      p->blockSize = blockSize;
      if (blockSize == 0)
      {
        p->frameState = ZSTD2_STATE_SIGNATURE;
        // continue; // for debug: we can continue without return to caller.
        // we notify the caller that skip frame was finished:
        *status = ZSTD_STATUS_FINISHED_FRAME;
        return SZ_OK;
      }
      // blockSize != 0
      // (inCur) was smaller than previous value of p->blockSize.
      // (inSize == *srcLen) now
      *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    if (inCur == 0)
    {
      *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    {
      (*srcLen)++;
      p->frameState = ZstdDec_UpdateState(p, *src++, info);
    }
  }
  
  *status = ZSTD_STATUS_NOT_SPECIFIED;
  p->isErrorState = True;
  // p->frameState = ZSTD2_STATE_ERROR;
  // if (p->frameState = ZSTD2_STATE_SIGNATURE)  return SZ_ERROR_NO_ARCHIVE
  return SZ_ERROR_DATA;
}




SRes ZstdDec_Decode(CZstdDecHandle dec, CZstdDecState *p)
{
  p->needWrite_Size = 0;
  p->status = ZSTD_STATUS_NOT_SPECIFIED;
  dec->disableHash = p->disableHash;

  if (p->outBuf_fromCaller)
  {
    dec->decoder.win = p->outBuf_fromCaller;
    dec->decoder.cycSize = p->outBufSize_fromCaller;
  }

  // p->winPos = dec->decoder.winPos;

  for (;;)
  {
    SizeT winPos, size;
    // SizeT outProcessed;
    SRes res;

    if (p->wrPos > dec->decoder.winPos)
      return SZ_ERROR_FAIL;

    if (dec->frameState == ZSTD2_STATE_FINISHED)
    {
      if (!p->outBuf_fromCaller)
      {
        // we need to set positions to zero for new frame.
        if (p->wrPos != dec->decoder.winPos)
        {
          /* We have already asked the caller to flush all data
             with (p->needWrite_Size) and (ZSTD_STATUS_FINISHED_FRAME) status.
             So it's unexpected case */
          // p->winPos = dec->decoder.winPos;
          // p->needWrite_Size = dec->decoder.winPos - p->wrPos; // flush size asking
          // return SZ_OK; // ask to flush again
          return SZ_ERROR_FAIL;
        }
        // (p->wrPos == dec->decoder.winPos), and we wrap to zero:
        dec->decoder.winPos = 0;
        p->winPos = 0;
        p->wrPos = 0;
      }
      ZstdDec_Init_ForNewFrame(dec);
      // continue;
    }

    winPos = dec->decoder.winPos;
    {
      SizeT next = dec->decoder.cycSize;
      /* cycSize == 0, if no buffer was allocated still,
         or, if (outBuf_fromCaller) mode and (outBufSize_fromCaller == 0) */
      if (!p->outBuf_fromCaller
          && next
          && next <= winPos
          && dec->isCyclicMode)
      {
        // (0 < decoder.cycSize <= winPos) in isCyclicMode.
        // so we need to wrap (winPos) and (wrPos) over (cycSize).
        const size_t delta = next;
        // (delta) is how many bytes we remove from buffer.
        /*
        // we don't need data older than last (cycSize) bytes.
        size_t delta = winPos - next; // num bytes after (cycSize)
        if (delta <= next) // it's expected case
          delta = next;
        // delta == Max(cycSize, winPos - cycSize)
        */
        if (p->wrPos < delta)
        {
          // (wrPos < decoder.cycSize)
          // We have asked already the caller to flush required data
          // p->status = ZSTD_STATUS_NOT_SPECIFIED;
          // p->winPos = winPos;
          // p->needWrite_Size = delta - p->wrPos; // flush size asking
          // return SZ_OK; // ask to flush again
          return SZ_ERROR_FAIL;
        }
        // p->wrPos >= decoder.cycSize
        // we move extra data after (decoder.cycSize) to start of cyclic buffer:
        winPos -= delta;
        if (winPos)
        {
          if (winPos >= delta)
            return SZ_ERROR_FAIL;
          memmove(dec->decoder.win, dec->decoder.win + delta, winPos);
          // printf("\nmemmove processed=%8x winPos=%8x\n", (unsigned)p->outProcessed, (unsigned)dec->decoder.winPos);
          STAT_INC(g_Num_Wrap_memmove_Num)
          STAT_UPDATE(g_Num_Wrap_memmove_Bytes += (unsigned)winPos;)
        }
        dec->decoder.winPos = winPos;
        p->winPos = winPos;
        p->wrPos -= delta;
        // dec->xxh64_winPos -= delta;

        // (winPos < delta)
        #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
          /* we set the data after cycSize, because
             we don't want to read non-initialized data or junk in CopyMatch(). */
          memset(dec->decoder.win + next, 0, COPY_CHUNK_SIZE);
        #endif

        /*
        if (winPos == next)
        {
          if (winPos != p->wrPos)
          {
            // we already requested before to flush full data for that case.
            //   but we give the caller a second chance to flush data:
            p->needWrite_Size = winPos - p->wrPos;
            return SZ_OK;
          }
          // (decoder.cycSize == winPos == p->wrPos)
          // so we do second wrapping to zero:
          winPos = 0;
          dec->decoder.winPos = 0;
          p->winPos = 0;
          p->wrPos = 0;
        }
        */
        // (winPos < next)
      }

      if (winPos > next)
        return SZ_ERROR_FAIL; // it's unexpected case
      /*
        if (!outBuf_fromCaller && isCyclicMode && cycSize != 0)
          then (winPos <  cycSize)
          else (winPos <= cycSize)
      */
      if (!p->outBuf_fromCaller)
      {
        // that code is optional. We try to optimize write chunk sizes.
        /* (next2) is expected next write position in the caller,
           if the caller writes by kBlockSizeMax chunks.
        */
        /*
        const size_t next2 = (winPos + kBlockSizeMax) & (kBlockSizeMax - 1);
        if (winPos < next2 && next2 < next)
          next = next2;
        */
      }
      size = next - winPos;
    }

    // note: ZstdDec_DecodeBlock() uses (winLimit = winPos + size) only for RLE and RAW blocks
    res = ZstdDec_DecodeBlock(dec, p, size);
    /*
      after one block decoding:
      if (!outBuf_fromCaller && isCyclicMode && cycSize != 0)
        then (winPos <  cycSize + max_block_size)
        else (winPos <= cycSize)
    */

    if (!p->outBuf_fromCaller)
      p->win = dec->decoder.win;
    p->winPos = dec->decoder.winPos;

    // outProcessed = dec->decoder.winPos - winPos;
    // p->outProcessed += outProcessed;

    if (res != SZ_OK)
      return res;

    if (dec->frameState != ZSTD2_STATE_AFTER_HEADER)
    {
      if (p->outBuf_fromCaller)
        return SZ_OK;
      {
        // !p->outBuf_fromCaller
        /*
          if (ZSTD_STATUS_FINISHED_FRAME), we request full flushing here because
            1) it's simpler to work with allocation and extracting of next frame,
            2) it's better to start writing to next new frame with aligned memory
               for faster xxh 64-bit reads.
        */
        size_t end = dec->decoder.winPos;  // end pos for all data flushing
        if (p->status != ZSTD_STATUS_FINISHED_FRAME)
        {
          // we will request flush here only for cases when wrap in cyclic buffer can be required in next call.
          if (!dec->isCyclicMode)
            return SZ_OK;
          // isCyclicMode
          {
            const size_t delta = dec->decoder.cycSize;
            if (end < delta)
              return SZ_OK; // (winPos < cycSize). no need for flush
            // cycSize <= winPos
            // So we ask the caller to flush of (cycSize - wrPos) bytes,
            // and then we will wrap cylicBuffer in next call
            end = delta;
          }
        }
        p->needWrite_Size = end - p->wrPos;
      }
      return SZ_OK;
    }

    // ZSTD2_STATE_AFTER_HEADER
    {
      BoolInt useCyclic = False;
      size_t cycSize;

      // p->status = ZSTD_STATUS_NOT_FINISHED;
      if (dec->dictionaryId != 0)
      {
        /* actually we can try to decode some data,
           because it's possible that some data doesn't use dictionary */
        // p->status = ZSTD_STATUS_NOT_SPECIFIED;
        return SZ_ERROR_UNSUPPORTED;
      }

      {
        UInt64 winSize = dec->contentSize;
        UInt64 winSize_Allocate = winSize;
        const unsigned descriptor = dec->descriptor;
        
        if ((descriptor & DESCRIPTOR_FLAG_SINGLE) == 0)
        {
          const Byte wd = dec->windowDescriptor;
          winSize = (UInt64)(8 + (wd & 7)) << ((wd >> 3) + 10 - 3);
          if (!DESCRIPTOR_Is_ContentSize_Defined(descriptor)
              || winSize_Allocate > winSize)
          {
            winSize_Allocate = winSize;
            useCyclic = True;
          }
        }
        /*
        else
        {
          if (p->info.singleSegment_ContentSize_MAX < winSize)
              p->info.singleSegment_ContentSize_MAX = winSize;
          // p->info.num_SingleSegments++;
        }
        */
        if (p->info.windowSize_MAX < winSize)
            p->info.windowSize_MAX = winSize;
        if (p->info.windowSize_Allocate_MAX < winSize_Allocate)
            p->info.windowSize_Allocate_MAX = winSize_Allocate;
        /*
           winSize_Allocate is MIN(content_size, window_size_from_descriptor).
           Wven if (content_size < (window_size_from_descriptor))
             original-zstd still uses (window_size_from_descriptor) to check that decoding is allowed.
           We try to follow original-zstd, and here we check (winSize) instead of (winSize_Allocate))
        */
        if (
              // winSize_Allocate   // it's relaxed check
              winSize               // it's more strict check to be compatible with original-zstd
            > ((UInt64)1 << MAX_WINDOW_SIZE_LOG))
          return SZ_ERROR_UNSUPPORTED; // SZ_ERROR_MEM
        cycSize = (size_t)winSize_Allocate;
        if (cycSize != winSize_Allocate)
          return SZ_ERROR_MEM;
        // cycSize <= winSize
        /* later we will use (CZstdDec1::winSize) to check match offsets and check block sizes.
           if (there is window descriptor)
           {
             We will check block size with (window_size_from_descriptor) instead of (winSize_Allocate).
             Does original-zstd do it that way also?
           }
           Here we must reduce full real 64-bit (winSize) to size_t for (CZstdDec1::winSize).
           Also we don't want too big values for (CZstdDec1::winSize).
           our (CZstdDec1::winSize) will meet the condition:
             (CZstdDec1::winSize < kBlockSizeMax || CZstdDec1::winSize <= cycSize).
        */
        dec->decoder.winSize = (winSize < kBlockSizeMax) ? (size_t)winSize: cycSize;
        // note: (CZstdDec1::winSize > cycSize) is possible, if (!useCyclic)
      }

      RINOK(ZstdDec_AllocateMisc(dec))

      if (p->outBuf_fromCaller)
        dec->isCyclicMode = False;
      else
      {
        size_t d = cycSize;

        if (dec->decoder.winPos != p->wrPos)
          return SZ_ERROR_FAIL;

        dec->decoder.winPos = 0;
        p->wrPos = 0;
        p->winPos = dec->decoder.winPos;

        /*
        const size_t needWrite = dec->decoder.winPos - p->wrPos;
        if (!needWrite)
        {
          dec->decoder.winPos = 0;
          p->wrPos = 0;
          p->winPos = dec->decoder.winPos;
        }
        */
        /* if (!useCyclic) we allocate only cycSize = ContentSize.
           But if we want to support the case where new frame starts with winPos != 0,
           then we will wrap over zero, and we still need
           to set (useCyclic) and allocate additional buffer spaces.
           Now we don't allow new frame starting with (winPos != 0).
           so (dec->decoder->winPos == 0)
           can use (!useCyclic) with reduced buffer sizes.
        */
        /*
        if (dec->decoder->winPos != 0)
          useCyclic = True;
        */

        if (useCyclic)
        {
          /* cyclyc buffer size must be at least (COPY_CHUNK_SIZE - 1) bytes
             larger than window size, because CopyMatch() can write additional
             (COPY_CHUNK_SIZE - 1) bytes and overwrite oldests data in cyclyc buffer.
             But for performance reasons we align (cycSize) for (kBlockSizeMax).
             also we must provide (cycSize >= max_decoded_data_after_cycSize),
             because after data move wrapping over zero we must provide (winPos < cycSize).
          */
          const size_t alignSize = kBlockSizeMax;
          /* here we add (1 << 7) instead of (COPY_CHUNK_SIZE - 1), because
             we want to get same (cycSize) for different COPY_CHUNK_SIZE values. */
          // cycSize += (COPY_CHUNK_SIZE - 1) + (alignSize - 1); // for debug : we can get smallest (cycSize)
          cycSize += (1 << 7) + alignSize;
          cycSize &= ~(size_t)(alignSize - 1);
          // cycSize must be aligned for 32, because xxh requires 32-bytes blocks.
          // cycSize += 12345; // for debug
          // cycSize += 1 << 10; // for debug
          // cycSize += 32; // for debug
          // cycSize += kBlockSizeMax; // for debug
          if (cycSize < d)
            return SZ_ERROR_MEM;
          /*
             in cyclic buffer mode we allow to decode one additional block
             that exceeds (cycSize).
             So we must allocate additional (kBlockSizeMax) bytes after (cycSize).
             if defined(Z7_STD_DEC_USE_AFTER_CYC_BUF)
             {
               we can read (COPY_CHUNK_SIZE - 1) bytes after (cycSize)
               but we aready allocate additional kBlockSizeMax that
               is larger than COPY_CHUNK_SIZE.
               So we don't need additional space of COPY_CHUNK_SIZE after (cycSize).
             }
          */
          /*
          #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
          d = cycSize + (1 << 7); // we must add at least (COPY_CHUNK_SIZE - 1)
          #endif
          */
          d = cycSize + kBlockSizeMax;
          if (d < cycSize)
            return SZ_ERROR_MEM;
        }

        {
          const size_t kMinWinAllocSize = 1 << 12;
          if (d < kMinWinAllocSize)
              d = kMinWinAllocSize;
        }

        if (d > dec->winBufSize_Allocated)
        {
          /*
          if (needWrite)
          {
            p->needWrite_Size = needWrite;
            return SZ_OK;
            // return SZ_ERROR_FAIL;
          }
          */

          if (dec->winBufSize_Allocated != 0)
          {
            const size_t k_extra = (useCyclic || d >= (1u << 20)) ?
                2 * kBlockSizeMax : 0;
            unsigned i = useCyclic ? 17 : 12;
            for (; i < sizeof(size_t) * 8; i++)
            {
              const size_t d2 = ((size_t)1 << i) + k_extra;
              if (d2 >= d)
              {
                d = d2;
                break;
              }
            }
          }
          // RINOK(ZstdDec_AllocateWindow(dec, d))
          ZstdDec_FreeWindow(dec);
          dec->win_Base = (Byte *)ISzAlloc_Alloc(dec->alloc_Big, d);
          if (!dec->win_Base)
            return SZ_ERROR_MEM;
          dec->decoder.win = dec->win_Base;
          dec->winBufSize_Allocated = d;
        }
        /*
        else
        {
          // for non-cyclycMode we want flush data, and set winPos = 0
          if (needWrite)
          {
            if (!useCyclic || dec->decoder.winPos >= cycSize)
            {
              p->needWrite_Size = needWrite;
              return SZ_OK;
              // return SZ_ERROR_FAIL;
            }
          }
        }
        */

        dec->decoder.cycSize = cycSize;
        p->win = dec->decoder.win;
        // p->cycSize = dec->decoder.cycSize;
        dec->isCyclicMode = (Byte)useCyclic;
      } // (!p->outBuf_fromCaller) end
      
      // p->winPos = dec->decoder.winPos;
      dec->frameState = ZSTD2_STATE_BLOCK;
      // continue;
    } // ZSTD2_STATE_AFTER_HEADER end
  }
}


void ZstdDec_GetResInfo(const CZstdDec *dec,
    const CZstdDecState *p,
    SRes res,
    CZstdDecResInfo *stat)
{
  // ZstdDecInfo_CLEAR(stat);
  stat->extraSize = 0;
  stat->is_NonFinishedFrame = False;
  if (dec->frameState != ZSTD2_STATE_FINISHED)
  {
    if (dec->frameState == ZSTD2_STATE_SIGNATURE)
    {
      stat->extraSize = (Byte)dec->tempSize;
      if (ZstdDecInfo_GET_NUM_FRAMES(&p->info) == 0)
        res = SZ_ERROR_NO_ARCHIVE;
    }
    else
    {
      stat->is_NonFinishedFrame = True;
      if (res == SZ_OK && p->status == ZSTD_STATUS_NEEDS_MORE_INPUT)
        res = SZ_ERROR_INPUT_EOF;
    }
  }
  stat->decode_SRes = res;
}


size_t ZstdDec_ReadUnusedFromInBuf(
    CZstdDecHandle dec,
    size_t afterDecoding_tempPos,
    void *data, size_t size)
{
  size_t processed = 0;
  if (dec->frameState == ZSTD2_STATE_SIGNATURE)
  {
    Byte *dest = (Byte *)data;
    const size_t tempSize = dec->tempSize;
    while (afterDecoding_tempPos < tempSize)
    {
      if (size == 0)
        break;
      size--;
      *dest++ = dec->temp[afterDecoding_tempPos++];
      processed++;
    }
  }
  return processed;
}


void ZstdDecState_Clear(CZstdDecState *p)
{
  memset(p, 0 , sizeof(*p));
}
