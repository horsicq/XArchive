/* XArchive amalgamation of 7-Zip 26.01 -- externally-owned decoder Ppmd8.
 *
 * 1 upstream translation units folded into one. Code is verbatim;
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

/* ---- C/Ppmd.h ---- */
/* Ppmd.h -- PPMD codec common code
2023-03-05 : Igor Pavlov : Public domain
This code is based on PPMd var.H (2001): Dmitry Shkarin : Public domain */

#ifndef ZIP7_INC_PPMD_H
#define ZIP7_INC_PPMD_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#if defined(MY_CPU_SIZEOF_POINTER) && (MY_CPU_SIZEOF_POINTER == 4)
/*
   PPMD code always uses 32-bit internal fields in PPMD structures to store internal references in main block.
   if (PPMD_32BIT is     defined), the PPMD code stores internal pointers to 32-bit reference fields.
   if (PPMD_32BIT is NOT defined), the PPMD code stores internal UInt32 offsets to reference fields.
   if (pointer size is 64-bit), then (PPMD_32BIT) mode is not allowed,
   if (pointer size is 32-bit), then (PPMD_32BIT) mode is optional,
     and it's allowed to disable PPMD_32BIT mode even if pointer is 32-bit.
   PPMD code works slightly faster in (PPMD_32BIT) mode.
*/
  #define PPMD_32BIT
#endif

#define PPMD_INT_BITS 7
#define PPMD_PERIOD_BITS 7
#define PPMD_BIN_SCALE (1 << (PPMD_INT_BITS + PPMD_PERIOD_BITS))

#define PPMD_GET_MEAN_SPEC(summ, shift, round) (((summ) + (1 << ((shift) - (round)))) >> (shift))
#define PPMD_GET_MEAN(summ) PPMD_GET_MEAN_SPEC((summ), PPMD_PERIOD_BITS, 2)
#define PPMD_UPDATE_PROB_0(prob) ((prob) + (1 << PPMD_INT_BITS) - PPMD_GET_MEAN(prob))
#define PPMD_UPDATE_PROB_1(prob) ((prob) - PPMD_GET_MEAN(prob))

#define PPMD_N1 4
#define PPMD_N2 4
#define PPMD_N3 4
#define PPMD_N4 ((128 + 3 - 1 * PPMD_N1 - 2 * PPMD_N2 - 3 * PPMD_N3) / 4)
#define PPMD_NUM_INDEXES (PPMD_N1 + PPMD_N2 + PPMD_N3 + PPMD_N4)

MY_CPU_pragma_pack_push_1
/* Most compilers works OK here even without #pragma pack(push, 1), but some GCC compilers need it. */

/* SEE-contexts for PPM-contexts with masked symbols */
typedef struct
{
  UInt16 Summ; /* Freq */
  Byte Shift;  /* Speed of Freq change; low Shift is for fast change */
  Byte Count;  /* Count to next change of Shift */
} CPpmd_See;

#define Ppmd_See_UPDATE(p) \
  { if ((p)->Shift < PPMD_PERIOD_BITS && --(p)->Count == 0) \
    { (p)->Summ = (UInt16)((p)->Summ << 1); \
      (p)->Count = (Byte)(3 << (p)->Shift++); }}


typedef struct
{
  Byte Symbol;
  Byte Freq;
  UInt16 Successor_0;
  UInt16 Successor_1;
} CPpmd_State;

typedef struct CPpmd_State2_
{
  Byte Symbol;
  Byte Freq;
} CPpmd_State2;

typedef struct CPpmd_State4_
{
  UInt16 Successor_0;
  UInt16 Successor_1;
} CPpmd_State4;

MY_CPU_pragma_pop

/*
   PPMD code can write full CPpmd_State structure data to CPpmd*_Context
      at (byte offset = 2) instead of some fields of original CPpmd*_Context structure.
   
   If we use pointers to different types, but that point to shared
   memory space, we can have aliasing problem (strict aliasing).
   
   XLC compiler in -O2 mode can change the order of memory write instructions
   in relation to read instructions, if we have use pointers to different types.
   
   To solve that aliasing problem we use combined CPpmd*_Context structure
   with unions that contain the fields from both structures:
   the original CPpmd*_Context and CPpmd_State.
   So we can access the fields from both structures via one pointer,
   and the compiler doesn't change the order of write instructions
   in relation to read instructions.

   If we don't use memory write instructions to shared memory in
   some local code, and we use only reading instructions (read only),
   then probably it's safe to use pointers to different types for reading.
*/
  


#ifdef PPMD_32BIT

  #define Ppmd_Ref_Type(type)   type *
  #define Ppmd_GetRef(p, ptr)   (ptr)
  #define Ppmd_GetPtr(p, ptr)   (ptr)
  #define Ppmd_GetPtr_Type(p, ptr, note_type) (ptr)

#else

  #define Ppmd_Ref_Type(type)   UInt32
  #define Ppmd_GetRef(p, ptr)   ((UInt32)((Byte *)(ptr) - (p)->Base))
  #define Ppmd_GetPtr(p, offs)  ((void *)((p)->Base + (offs)))
  #define Ppmd_GetPtr_Type(p, offs, type) ((type *)Ppmd_GetPtr(p, offs))

#endif // PPMD_32BIT


typedef Ppmd_Ref_Type(CPpmd_State) CPpmd_State_Ref;
typedef Ppmd_Ref_Type(void)        CPpmd_Void_Ref;
typedef Ppmd_Ref_Type(Byte)        CPpmd_Byte_Ref;


/*
#ifdef MY_CPU_LE_UNALIGN
// the unaligned 32-bit access latency can be too large, if the data is not in L1 cache.
#define Ppmd_GET_SUCCESSOR(p) ((CPpmd_Void_Ref)*(const UInt32 *)(const void *)&(p)->Successor_0)
#define Ppmd_SET_SUCCESSOR(p, v) *(UInt32 *)(void *)(void *)&(p)->Successor_0 = (UInt32)(v)

#else
*/

/*
   We can write 16-bit halves to 32-bit (Successor) field in any selected order.
   But the native order is more consistent way.
   So we use the native order, if LE/BE order can be detected here at compile time.
*/

#ifdef MY_CPU_BE

  #define Ppmd_GET_SUCCESSOR(p) \
    ( (CPpmd_Void_Ref) (((UInt32)(p)->Successor_0 << 16) | (p)->Successor_1) )

  #define Ppmd_SET_SUCCESSOR(p, v) { \
    (p)->Successor_0 = (UInt16)(((UInt32)(v) >> 16) /* & 0xFFFF */); \
    (p)->Successor_1 = (UInt16)((UInt32)(v) /* & 0xFFFF */); }

#else

  #define Ppmd_GET_SUCCESSOR(p) \
    ( (CPpmd_Void_Ref) ((p)->Successor_0 | ((UInt32)(p)->Successor_1 << 16)) )

  #define Ppmd_SET_SUCCESSOR(p, v) { \
    (p)->Successor_0 = (UInt16)((UInt32)(v) /* & 0xFFFF */); \
    (p)->Successor_1 = (UInt16)(((UInt32)(v) >> 16) /* & 0xFFFF */); }

#endif

// #endif


#define PPMD_SetAllBitsIn256Bytes(p) \
  { size_t z; for (z = 0; z < 256 / sizeof(p[0]); z += 8) { \
  p[z+7] = p[z+6] = p[z+5] = p[z+4] = p[z+3] = p[z+2] = p[z+1] = p[z+0] = ~(size_t)0; }}

EXTERN_C_END
 
#endif

/* ---- C/Ppmd8.h ---- */
/* Ppmd8.h -- Ppmd8 (PPMdI) compression codec
2023-04-02 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.I (2002): Dmitry Shkarin : Public domain
  Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

#ifndef ZIP7_INC_PPMD8_H
#define ZIP7_INC_PPMD8_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define PPMD8_MIN_ORDER 2
#define PPMD8_MAX_ORDER 16




struct CPpmd8_Context_;

typedef Ppmd_Ref_Type(struct CPpmd8_Context_) CPpmd8_Context_Ref;

// MY_CPU_pragma_pack_push_1

typedef struct CPpmd8_Context_
{
  Byte NumStats;
  Byte Flags;
  
  union
  {
    UInt16 SummFreq;
    CPpmd_State2 State2;
  } Union2;
  
  union
  {
    CPpmd_State_Ref Stats;
    CPpmd_State4 State4;
  } Union4;

  CPpmd8_Context_Ref Suffix;
} CPpmd8_Context;

// MY_CPU_pragma_pop

#define Ppmd8Context_OneState(p) ((CPpmd_State *)&(p)->Union2)

/* PPMdI code rev.2 contains the fix over PPMdI code rev.1.
   But the code PPMdI.2 is not compatible with PPMdI.1 for some files compressed
   in FREEZE mode. So we disable FREEZE mode support. */

// #define PPMD8_FREEZE_SUPPORT

enum
{
  PPMD8_RESTORE_METHOD_RESTART,
  PPMD8_RESTORE_METHOD_CUT_OFF
  #ifdef PPMD8_FREEZE_SUPPORT
  , PPMD8_RESTORE_METHOD_FREEZE
  #endif
  , PPMD8_RESTORE_METHOD_UNSUPPPORTED
};








typedef struct
{
  CPpmd8_Context *MinContext, *MaxContext;
  CPpmd_State *FoundState;
  unsigned OrderFall, InitEsc, PrevSuccess, MaxOrder, RestoreMethod;
  Int32 RunLength, InitRL; /* must be 32-bit at least */

  UInt32 Size;
  UInt32 GlueCount;
  UInt32 AlignOffset;
  Byte *Base, *LoUnit, *HiUnit, *Text, *UnitsStart;

  UInt32 Range;
  UInt32 Code;
  UInt32 Low;
  union
  {
    IByteInPtr In;
    IByteOutPtr Out;
  } Stream;

  Byte Indx2Units[PPMD_NUM_INDEXES + 2]; // +2 for alignment
  Byte Units2Indx[128];
  CPpmd_Void_Ref FreeList[PPMD_NUM_INDEXES];
  UInt32 Stamps[PPMD_NUM_INDEXES];
  Byte NS2BSIndx[256], NS2Indx[260];
  Byte ExpEscape[16];
  CPpmd_See DummySee, See[24][32];
  UInt16 BinSumm[25][64];

} CPpmd8;


void Ppmd8_Construct(CPpmd8 *p);
BoolInt Ppmd8_Alloc(CPpmd8 *p, UInt32 size, ISzAllocPtr alloc);
void Ppmd8_Free(CPpmd8 *p, ISzAllocPtr alloc);
void Ppmd8_Init(CPpmd8 *p, unsigned maxOrder, unsigned restoreMethod);
#define Ppmd8_WasAllocated(p) ((p)->Base != NULL)


/* ---------- Internal Functions ---------- */

#define Ppmd8_GetPtr(p, ptr)     Ppmd_GetPtr(p, ptr)
#define Ppmd8_GetContext(p, ptr) Ppmd_GetPtr_Type(p, ptr, CPpmd8_Context)
#define Ppmd8_GetStats(p, ctx)   Ppmd_GetPtr_Type(p, (ctx)->Union4.Stats, CPpmd_State)

void Ppmd8_Update1(CPpmd8 *p);
void Ppmd8_Update1_0(CPpmd8 *p);
void Ppmd8_Update2(CPpmd8 *p);






#define Ppmd8_GetBinSumm(p) \
    &p->BinSumm[p->NS2Indx[(size_t)Ppmd8Context_OneState(p->MinContext)->Freq - 1]] \
    [ p->PrevSuccess + ((p->RunLength >> 26) & 0x20) \
    + p->NS2BSIndx[Ppmd8_GetContext(p, p->MinContext->Suffix)->NumStats] + \
    + p->MinContext->Flags ]


CPpmd_See *Ppmd8_MakeEscFreq(CPpmd8 *p, unsigned numMasked, UInt32 *scale);


/* 20.01: the original PPMdI encoder and decoder probably could work incorrectly in some rare cases,
   where the original PPMdI code can give "Divide by Zero" operation.
   We use the following fix to allow correct working of encoder and decoder in any cases.
   We correct (Escape_Freq) and (_sum_), if (_sum_) is larger than p->Range) */
#define PPMD8_CORRECT_SUM_RANGE(p, _sum_) if (_sum_ > p->Range /* /1 */) _sum_ = p->Range;


/* ---------- Decode ---------- */

#define PPMD8_SYM_END    (-1)
#define PPMD8_SYM_ERROR  (-2)

/*
You must set (CPpmd8::Stream.In) before Ppmd8_RangeDec_Init()

Ppmd8_DecodeSymbol()
out:
  >= 0 : decoded byte
    -1 : PPMD8_SYM_END   : End of payload marker
    -2 : PPMD8_SYM_ERROR : Data error
*/


BoolInt Ppmd8_Init_RangeDec(CPpmd8 *p);
#define Ppmd8_RangeDec_IsFinishedOK(p) ((p)->Code == 0)
int Ppmd8_DecodeSymbol(CPpmd8 *p);








/* ---------- Encode ---------- */

#define Ppmd8_Init_RangeEnc(p) { (p)->Low = 0; (p)->Range = 0xFFFFFFFF; }
void Ppmd8_Flush_RangeEnc(CPpmd8 *p);
void Ppmd8_EncodeSymbol(CPpmd8 *p, int symbol);


EXTERN_C_END
 
#endif

/* ================ unit bodies ================ */

/* ================ unit: C/Ppmd8.c ================ */
/* Ppmd8.c -- PPMdI codec
: Igor Pavlov : Public domain
This code is based on PPMd var.I (2002): Dmitry Shkarin : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue




MY_ALIGN(16)
static const Byte PPMD8_kExpEscape[16] = { 25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
MY_ALIGN(16)
static const UInt16 PPMD8_kInitBinEsc[] = { 0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

#define MAX_FREQ 124
#define UNIT_SIZE 12

#define U2B(nu) ((UInt32)(nu) * UNIT_SIZE)
#define U2I(nu) (p->Units2Indx[(size_t)(nu) - 1])
#define I2U(indx) ((unsigned)p->Indx2Units[indx])


#define REF(ptr) Ppmd_GetRef(p, ptr)

#define STATS_REF(ptr) ((CPpmd_State_Ref)REF(ptr))

#define CTX(ref) ((CPpmd8_Context *)Ppmd8_GetContext(p, ref))
#define STATS(ctx) Ppmd8_GetStats(p, ctx)
#define ONE_STATE(ctx) Ppmd8Context_OneState(ctx)
#define SUFFIX(ctx) CTX((ctx)->Suffix)

typedef CPpmd8_Context * PPMD8_CTX_PTR;

struct CPpmd8_Node_;

typedef Ppmd_Ref_Type(struct CPpmd8_Node_) CPpmd8_Node_Ref;

typedef struct CPpmd8_Node_
{
  UInt32 Stamp;
  
  CPpmd8_Node_Ref Next;
  UInt32 NU;
} CPpmd8_Node;

#define NODE(r)  Ppmd_GetPtr_Type(p, r, CPpmd8_Node)

void Ppmd8_Construct(CPpmd8 *p)
{
  unsigned i, k, m;

  p->Base = NULL;

  for (i = 0, k = 0; i < PPMD_NUM_INDEXES; i++)
  {
    unsigned step = (i >= 12 ? 4 : (i >> 2) + 1);
    do { p->Units2Indx[k++] = (Byte)i; } while (--step);
    p->Indx2Units[i] = (Byte)k;
  }

  p->NS2BSIndx[0] = (0 << 1);
  p->NS2BSIndx[1] = (1 << 1);
  memset(p->NS2BSIndx + 2, (2 << 1), 9);
  memset(p->NS2BSIndx + 11, (3 << 1), 256 - 11);

  for (i = 0; i < 5; i++)
    p->NS2Indx[i] = (Byte)i;
  
  for (m = i, k = 1; i < 260; i++)
  {
    p->NS2Indx[i] = (Byte)m;
    if (--k == 0)
      k = (++m) - 4;
  }

  memcpy(p->ExpEscape, PPMD8_kExpEscape, 16);
}


void Ppmd8_Free(CPpmd8 *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->Base);
  p->Size = 0;
  p->Base = NULL;
}


BoolInt Ppmd8_Alloc(CPpmd8 *p, UInt32 size, ISzAllocPtr alloc)
{
  if (!p->Base || p->Size != size)
  {
    Ppmd8_Free(p, alloc);
    p->AlignOffset = (4 - size) & 3;
    if ((p->Base = (Byte *)ISzAlloc_Alloc(alloc, p->AlignOffset + size)) == NULL)
      return False;
    p->Size = size;
  }
  return True;
}



// ---------- Internal Memory Allocator ----------






#define EMPTY_NODE 0xFFFFFFFF


static void Ppmd8_InsertNode(CPpmd8 *p, void *node, unsigned indx)
{
  ((CPpmd8_Node *)node)->Stamp = EMPTY_NODE;
  ((CPpmd8_Node *)node)->Next = (CPpmd8_Node_Ref)p->FreeList[indx];
  ((CPpmd8_Node *)node)->NU = I2U(indx);
  p->FreeList[indx] = REF(node);
  p->Stamps[indx]++;
}


static void *Ppmd8_RemoveNode(CPpmd8 *p, unsigned indx)
{
  CPpmd8_Node *node = NODE((CPpmd8_Node_Ref)p->FreeList[indx]);
  p->FreeList[indx] = node->Next;
  p->Stamps[indx]--;

  return node;
}


static void Ppmd8_SplitBlock(CPpmd8 *p, void *ptr, unsigned oldIndx, unsigned newIndx)
{
  unsigned i, nu = I2U(oldIndx) - I2U(newIndx);
  ptr = (Byte *)ptr + U2B(I2U(newIndx));
  if (I2U(i = U2I(nu)) != nu)
  {
    unsigned k = I2U(--i);
    Ppmd8_InsertNode(p, ((Byte *)ptr) + U2B(k), nu - k - 1);
  }
  Ppmd8_InsertNode(p, ptr, i);
}














static void Ppmd8_GlueFreeBlocks(CPpmd8 *p)
{
  /*
  we use first UInt32 field of 12-bytes UNITs as record type stamp
    CPpmd_State    { Byte Symbol; Byte Freq; : Freq != 0xFF
    CPpmd8_Context { Byte NumStats; Byte Flags; UInt16 SummFreq;  : Flags != 0xFF ???
    CPpmd8_Node    { UInt32 Stamp            : Stamp == 0xFFFFFFFF for free record
                                             : Stamp == 0 for guard
    Last 12-bytes UNIT in array is always contains 12-bytes order-0 CPpmd8_Context record
  */
  CPpmd8_Node_Ref n;

  p->GlueCount = 1 << 13;
  memset(p->Stamps, 0, sizeof(p->Stamps));
  
  /* we set guard NODE at LoUnit */
  if (p->LoUnit != p->HiUnit)
    ((CPpmd8_Node *)(void *)p->LoUnit)->Stamp = 0;

  {
    /* Glue free blocks */
    CPpmd8_Node_Ref *prev = &n;
    unsigned i;
    for (i = 0; i < PPMD_NUM_INDEXES; i++)
    {

      CPpmd8_Node_Ref next = (CPpmd8_Node_Ref)p->FreeList[i];
      p->FreeList[i] = 0;
      while (next != 0)
      {
        CPpmd8_Node *node = NODE(next);
        UInt32 nu = node->NU;
        *prev = next;
        next = node->Next;
        if (nu != 0)
        {
          CPpmd8_Node *node2;
          prev = &(node->Next);
          while ((node2 = node + nu)->Stamp == EMPTY_NODE)
          {
            nu += node2->NU;
            node2->NU = 0;
            node->NU = nu;
          }
        }
      }
    }

    *prev = 0;
  }



  






  

  
  
  
  
  
  
  
  
  /* Fill lists of free blocks */
  while (n != 0)
  {
    CPpmd8_Node *node = NODE(n);
    UInt32 nu = node->NU;
    unsigned i;
    n = node->Next;
    if (nu == 0)
      continue;
    for (; nu > 128; nu -= 128, node += 128)
      Ppmd8_InsertNode(p, node, PPMD_NUM_INDEXES - 1);
    if (I2U(i = U2I(nu)) != nu)
    {
      unsigned k = I2U(--i);
      Ppmd8_InsertNode(p, node + k, (unsigned)nu - k - 1);
    }
    Ppmd8_InsertNode(p, node, i);
  }
}


Z7_NO_INLINE
static void *Ppmd8_AllocUnitsRare(CPpmd8 *p, unsigned indx)
{
  unsigned i;
  
  if (p->GlueCount == 0)
  {
    Ppmd8_GlueFreeBlocks(p);
    if (p->FreeList[indx] != 0)
      return Ppmd8_RemoveNode(p, indx);
  }
  
  i = indx;
  
  do
  {
    if (++i == PPMD_NUM_INDEXES)
    {
      UInt32 numBytes = U2B(I2U(indx));
      Byte *us = p->UnitsStart;
      p->GlueCount--;
      return ((UInt32)(us - p->Text) > numBytes) ? (p->UnitsStart = us - numBytes) : (NULL);
    }
  }
  while (p->FreeList[i] == 0);
  
  {
    void *block = Ppmd8_RemoveNode(p, i);
    Ppmd8_SplitBlock(p, block, i, indx);
    return block;
  }
}


static void *Ppmd8_AllocUnits(CPpmd8 *p, unsigned indx)
{
  if (p->FreeList[indx] != 0)
    return Ppmd8_RemoveNode(p, indx);
  {
    UInt32 numBytes = U2B(I2U(indx));
    Byte *lo = p->LoUnit;
    if ((UInt32)(p->HiUnit - lo) >= numBytes)
    {
      p->LoUnit = lo + numBytes;
      return lo;
    }
  }
  return Ppmd8_AllocUnitsRare(p, indx);
}


#define MEM_12_CPY(dest, src, num) \
  { UInt32 *d = (UInt32 *)(dest); \
    const UInt32 *z = (const UInt32 *)(src); \
    unsigned n = (num); \
    do { \
      d[0] = z[0]; \
      d[1] = z[1]; \
      d[2] = z[2]; \
      z += 3; \
      d += 3; \
    } while (--n); \
  }



static void *ShrinkUnits(CPpmd8 *p, void *oldPtr, unsigned oldNU, unsigned newNU)
{
  unsigned i0 = U2I(oldNU);
  unsigned i1 = U2I(newNU);
  if (i0 == i1)
    return oldPtr;
  if (p->FreeList[i1] != 0)
  {
    void *ptr = Ppmd8_RemoveNode(p, i1);
    MEM_12_CPY(ptr, oldPtr, newNU)
    Ppmd8_InsertNode(p, oldPtr, i0);
    return ptr;
  }
  Ppmd8_SplitBlock(p, oldPtr, i0, i1);
  return oldPtr;
}


static void FreeUnits(CPpmd8 *p, void *ptr, unsigned nu)
{
  Ppmd8_InsertNode(p, ptr, U2I(nu));
}


static void SpecialFreeUnit(CPpmd8 *p, void *ptr)
{
  if ((Byte *)ptr != p->UnitsStart)
    Ppmd8_InsertNode(p, ptr, 0);
  else
  {
    #ifdef PPMD8_FREEZE_SUPPORT
    *(UInt32 *)ptr = EMPTY_NODE; /* it's used for (Flags == 0xFF) check in RemoveBinContexts() */
    #endif
    p->UnitsStart += UNIT_SIZE;
  }
}


/*
static void *MoveUnitsUp(CPpmd8 *p, void *oldPtr, unsigned nu)
{
  unsigned indx = U2I(nu);
  void *ptr;
  if ((Byte *)oldPtr > p->UnitsStart + (1 << 14) || REF(oldPtr) > p->FreeList[indx])
    return oldPtr;
  ptr = Ppmd8_RemoveNode(p, indx);
  MEM_12_CPY(ptr, oldPtr, nu)
  if ((Byte *)oldPtr != p->UnitsStart)
    Ppmd8_InsertNode(p, oldPtr, indx);
  else
    p->UnitsStart += U2B(I2U(indx));
  return ptr;
}
*/

static void ExpandTextArea(CPpmd8 *p)
{
  UInt32 count[PPMD_NUM_INDEXES];
  unsigned i;
 
  memset(count, 0, sizeof(count));
  if (p->LoUnit != p->HiUnit)
    ((CPpmd8_Node *)(void *)p->LoUnit)->Stamp = 0;
  
  {
    CPpmd8_Node *node = (CPpmd8_Node *)(void *)p->UnitsStart;
    while (node->Stamp == EMPTY_NODE)
    {
      UInt32 nu = node->NU;
      node->Stamp = 0;
      count[U2I(nu)]++;
      node += nu;
    }
    p->UnitsStart = (Byte *)node;
  }
  
  for (i = 0; i < PPMD_NUM_INDEXES; i++)
  {
    UInt32 cnt = count[i];
    if (cnt == 0)
      continue;
    {
      CPpmd8_Node_Ref *prev = (CPpmd8_Node_Ref *)&p->FreeList[i];
      CPpmd8_Node_Ref n = *prev;
      p->Stamps[i] -= cnt;
      for (;;)
      {
        CPpmd8_Node *node = NODE(n);
        n = node->Next;
        if (node->Stamp != 0)
        {
          prev = &node->Next;
          continue;
        }
        *prev = n;
        if (--cnt == 0)
          break;
      }
    }
  }
}


#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
static void Ppmd8State_SetSuccessor(CPpmd_State *p, CPpmd_Void_Ref v)
{
  Ppmd_SET_SUCCESSOR(p, v)
}

#define RESET_TEXT(offs) { p->Text = p->Base + p->AlignOffset + (offs); }

Z7_NO_INLINE
static
void Ppmd8_RestartModel(CPpmd8 *p)
{
  unsigned i, k, m;

  memset(p->FreeList, 0, sizeof(p->FreeList));
  memset(p->Stamps, 0, sizeof(p->Stamps));
  RESET_TEXT(0)
  p->HiUnit = p->Text + p->Size;
  p->LoUnit = p->UnitsStart = p->HiUnit - p->Size / 8 / UNIT_SIZE * 7 * UNIT_SIZE;
  p->GlueCount = 0;

  p->OrderFall = p->MaxOrder;
  p->RunLength = p->InitRL = -(Int32)((p->MaxOrder < 12) ? p->MaxOrder : 12) - 1;
  p->PrevSuccess = 0;

  {
    CPpmd8_Context *mc = (PPMD8_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE); /* AllocContext(p); */
    CPpmd_State *s = (CPpmd_State *)p->LoUnit; /* Ppmd8_AllocUnits(p, PPMD_NUM_INDEXES - 1); */
    
    p->LoUnit += U2B(256 / 2);
    p->MaxContext = p->MinContext = mc;
    p->FoundState = s;
    mc->Flags = 0;
    mc->NumStats = 256 - 1;
    mc->Union2.SummFreq = 256 + 1;
    mc->Union4.Stats = REF(s);
    mc->Suffix = 0;

    for (i = 0; i < 256; i++, s++)
    {
      s->Symbol = (Byte)i;
      s->Freq = 1;
      Ppmd8State_SetSuccessor(s, 0);
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  for (i = m = 0; m < 25; m++)
  {
    while (p->NS2Indx[i] == m)
      i++;
    for (k = 0; k < 8; k++)
    {
      unsigned r;
      UInt16 *dest = p->BinSumm[m] + k;
      const UInt16 val = (UInt16)(PPMD_BIN_SCALE - PPMD8_kInitBinEsc[k] / (i + 1));
      for (r = 0; r < 64; r += 8)
        dest[r] = val;
    }
  }

  for (i = m = 0; m < 24; m++)
  {
    unsigned summ;
    CPpmd_See *s;
    while (p->NS2Indx[(size_t)i + 3] == m + 3)
      i++;
    s = p->See[m];
    summ = ((2 * i + 5) << (PPMD_PERIOD_BITS - 4));
    for (k = 0; k < 32; k++, s++)
    {
      s->Summ = (UInt16)summ;
      s->Shift = (PPMD_PERIOD_BITS - 4);
      s->Count = 7;
    }
  }

  p->DummySee.Summ = 0; /* unused */
  p->DummySee.Shift = PPMD_PERIOD_BITS;
  p->DummySee.Count = 64; /* unused */
}


void Ppmd8_Init(CPpmd8 *p, unsigned maxOrder, unsigned restoreMethod)
{
  p->MaxOrder = maxOrder;
  p->RestoreMethod = restoreMethod;
  Ppmd8_RestartModel(p);
}


#define FLAG_RESCALED  (1 << 2)
// #define FLAG_SYM_HIGH  (1 << 3)
#define FLAG_PREV_HIGH (1 << 4)

#define HiBits_Prepare(sym) ((unsigned)(sym) + 0xC0)

#define HiBits_Convert_3(flags) (((flags) >> (8 - 3)) & (1 << 3))
#define HiBits_Convert_4(flags) (((flags) >> (8 - 4)) & (1 << 4))

#define PPMD8_HiBitsFlag_3(sym) HiBits_Convert_3(HiBits_Prepare(sym))
#define PPMD8_HiBitsFlag_4(sym) HiBits_Convert_4(HiBits_Prepare(sym))

// #define PPMD8_HiBitsFlag_3(sym) (0x08 * ((sym) >= 0x40))
// #define PPMD8_HiBitsFlag_4(sym) (0x10 * ((sym) >= 0x40))

/*
Refresh() is called when we remove some symbols (successors) in context.
It increases Escape_Freq for sum of all removed symbols.
*/

static void Refresh(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned oldNU, unsigned scale)
{
  unsigned i = ctx->NumStats, escFreq, sumFreq, flags;
  CPpmd_State *s = (CPpmd_State *)ShrinkUnits(p, STATS(ctx), oldNU, (i + 2) >> 1);
  ctx->Union4.Stats = REF(s);

  // #ifdef PPMD8_FREEZE_SUPPORT
  /*
    (ctx->Union2.SummFreq >= ((UInt32)1 << 15)) can be in FREEZE mode for some files.
    It's not good for range coder. So new versions of support fix:
       -   original PPMdI code rev.1
       +   original PPMdI code rev.2
       -   7-Zip default ((PPMD8_FREEZE_SUPPORT is not defined)
       +   7-Zip (p->RestoreMethod >= PPMD8_RESTORE_METHOD_FREEZE)
    if we       use that fixed line, we can lose compatibility with some files created before fix
    if we don't use that fixed line, the program can work incorrectly in FREEZE mode in rare case.
  */
  // if (p->RestoreMethod >= PPMD8_RESTORE_METHOD_FREEZE)
  {
    scale |= (ctx->Union2.SummFreq >= ((UInt32)1 << 15));
  }
  // #endif



  flags = HiBits_Prepare(s->Symbol);
  {
    unsigned freq = s->Freq;
    escFreq = ctx->Union2.SummFreq - freq;
    freq = (freq + scale) >> scale;
    sumFreq = freq;
    s->Freq = (Byte)freq;
  }
 
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + scale) >> scale;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    flags |= HiBits_Prepare(s->Symbol);
  }
  while (--i);
  
  ctx->Union2.SummFreq = (UInt16)(sumFreq + ((escFreq + scale) >> scale));
  ctx->Flags = (Byte)((ctx->Flags & (FLAG_PREV_HIGH + FLAG_RESCALED * scale)) + HiBits_Convert_3(flags));
}


static void SWAP_STATES(CPpmd_State *t1, CPpmd_State *t2)
{
  CPpmd_State tmp = *t1;
  *t1 = *t2;
  *t2 = tmp;
}


/*
CutOff() reduces contexts:
  It conversts Successors at MaxOrder to another Contexts to NULL-Successors
  It removes RAW-Successors and NULL-Successors that are not Order-0
      and it removes contexts when it has no Successors.
  if the (Union4.Stats) is close to (UnitsStart), it moves it up.
*/

static CPpmd_Void_Ref CutOff(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned order)
{
  int ns = ctx->NumStats;
  unsigned nu;
  CPpmd_State *stats;
  
  if (ns == 0)
  {
    CPpmd_State *s = ONE_STATE(ctx);
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart)
    {
      if (order < p->MaxOrder)
        successor = CutOff(p, CTX(successor), order + 1);
      else
        successor = 0;
      Ppmd8State_SetSuccessor(s, successor);
      if (successor || order <= 9) /* O_BOUND */
        return REF(ctx);
    }
    SpecialFreeUnit(p, ctx);
    return 0;
  }

  nu = ((unsigned)ns + 2) >> 1;
  // ctx->Union4.Stats = STATS_REF(MoveUnitsUp(p, STATS(ctx), nu));
  {
    unsigned indx = U2I(nu);
    stats = STATS(ctx);

    if ((UInt32)((Byte *)stats - p->UnitsStart) <= (1 << 14)
        && (CPpmd_Void_Ref)ctx->Union4.Stats <= p->FreeList[indx])
    {
      void *ptr = Ppmd8_RemoveNode(p, indx);
      ctx->Union4.Stats = STATS_REF(ptr);
      MEM_12_CPY(ptr, (const void *)stats, nu)
      if ((Byte *)stats != p->UnitsStart)
        Ppmd8_InsertNode(p, stats, indx);
      else
        p->UnitsStart += U2B(I2U(indx));
      stats = (CPpmd_State *)ptr;
    }
  }

  {
    CPpmd_State *s = stats + (unsigned)ns;
    do
    {
      CPpmd_Void_Ref successor = SUCCESSOR(s);
      if ((Byte *)Ppmd8_GetPtr(p, successor) < p->UnitsStart)
      {
        CPpmd_State *s2 = stats + (unsigned)(ns--);
        if (order)
        {
          if (s != s2)
            *s = *s2;
        }
        else
        {
          SWAP_STATES(s, s2);
          Ppmd8State_SetSuccessor(s2, 0);
        }
      }
      else
      {
        if (order < p->MaxOrder)
          Ppmd8State_SetSuccessor(s, CutOff(p, CTX(successor), order + 1));
        else
          Ppmd8State_SetSuccessor(s, 0);
      }
    }
    while (--s >= stats);
  }
  
  if (ns != ctx->NumStats && order)
  {
    if (ns < 0)
    {
      FreeUnits(p, stats, nu);
      SpecialFreeUnit(p, ctx);
      return 0;
    }
    ctx->NumStats = (Byte)ns;
    if (ns == 0)
    {
      const Byte sym = stats->Symbol;
      ctx->Flags = (Byte)((ctx->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(sym));
      // *ONE_STATE(ctx) = *stats;
      ctx->Union2.State2.Symbol = sym;
      ctx->Union2.State2.Freq = (Byte)(((unsigned)stats->Freq + 11) >> 3);
      ctx->Union4.State4.Successor_0 = stats->Successor_0;
      ctx->Union4.State4.Successor_1 = stats->Successor_1;
      FreeUnits(p, stats, nu);
    }
    else
    {
      Refresh(p, ctx, nu, ctx->Union2.SummFreq > 16 * (unsigned)ns);
    }
  }
  
  return REF(ctx);
}



#ifdef PPMD8_FREEZE_SUPPORT

/*
RemoveBinContexts()
  It conversts Successors at MaxOrder to another Contexts to NULL-Successors
  It changes RAW-Successors to NULL-Successors
  removes Bin Context without Successor, if suffix of that context is also binary.
*/

static CPpmd_Void_Ref RemoveBinContexts(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned order)
{
  if (!ctx->NumStats)
  {
    CPpmd_State *s = ONE_STATE(ctx);
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart && order < p->MaxOrder)
      successor = RemoveBinContexts(p, CTX(successor), order + 1);
    else
      successor = 0;
    Ppmd8State_SetSuccessor(s, successor);
    /* Suffix context can be removed already, since different (high-order)
       Successors may refer to same context. So we check Flags == 0xFF (Stamp == EMPTY_NODE) */
    if (!successor && (!SUFFIX(ctx)->NumStats || SUFFIX(ctx)->Flags == 0xFF))
    {
      FreeUnits(p, ctx, 1);
      return 0;
    }
  }
  else
  {
    CPpmd_State *s = STATS(ctx) + ctx->NumStats;
    do
    {
      CPpmd_Void_Ref successor = SUCCESSOR(s);
      if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart && order < p->MaxOrder)
        Ppmd8State_SetSuccessor(s, RemoveBinContexts(p, CTX(successor), order + 1));
      else
        Ppmd8State_SetSuccessor(s, 0);
    }
    while (--s >= STATS(ctx));
  }
  
  return REF(ctx);
}

#endif



static UInt32 GetUsedMemory(const CPpmd8 *p)
{
  UInt32 v = 0;
  unsigned i;
  for (i = 0; i < PPMD_NUM_INDEXES; i++)
    v += p->Stamps[i] * I2U(i);
  return p->Size - (UInt32)(p->HiUnit - p->LoUnit) - (UInt32)(p->UnitsStart - p->Text) - U2B(v);
}

#ifdef PPMD8_FREEZE_SUPPORT
  #define RESTORE_MODEL(c1, fSuccessor) RestoreModel(p, c1, fSuccessor)
#else
  #define RESTORE_MODEL(c1, fSuccessor) RestoreModel(p, c1)
#endif


static void RestoreModel(CPpmd8 *p, PPMD8_CTX_PTR ctxError
    #ifdef PPMD8_FREEZE_SUPPORT
    , PPMD8_CTX_PTR fSuccessor
    #endif
    )
{
  PPMD8_CTX_PTR c;
  CPpmd_State *s;
  RESET_TEXT(0)

  // we go here in cases of error of allocation for context (c1)
  // Order(MinContext) < Order(ctxError) <= Order(MaxContext)
  
  // We remove last symbol from each of contexts [p->MaxContext ... ctxError) contexts
  // So we rollback all created (symbols) before error.
  for (c = p->MaxContext; c != ctxError; c = SUFFIX(c))
    if (--(c->NumStats) == 0)
    {
      s = STATS(c);
      c->Flags = (Byte)((c->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(s->Symbol));
      // *ONE_STATE(c) = *s;
      c->Union2.State2.Symbol = s->Symbol;
      c->Union2.State2.Freq = (Byte)(((unsigned)s->Freq + 11) >> 3);
      c->Union4.State4.Successor_0 = s->Successor_0;
      c->Union4.State4.Successor_1 = s->Successor_1;

      SpecialFreeUnit(p, s);
    }
    else
    {
      /* Refresh() can increase Escape_Freq on value of Freq of last symbol, that was added before error.
         so the largest possible increase for Escape_Freq is (8) from value before ModelUpoadet() */
      Refresh(p, c, ((unsigned)c->NumStats + 3) >> 1, 0);
    }
 
  // increase Escape Freq for context [ctxError ... p->MinContext)
  for (; c != p->MinContext; c = SUFFIX(c))
    if (c->NumStats == 0)
    {
      // ONE_STATE(c)
      c->Union2.State2.Freq = (Byte)(((unsigned)c->Union2.State2.Freq + 1) >> 1);
    }
    else if ((c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 4)) > 128 + 4 * c->NumStats)
      Refresh(p, c, ((unsigned)c->NumStats + 2) >> 1, 1);

  #ifdef PPMD8_FREEZE_SUPPORT
  if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    p->MaxContext = fSuccessor;
    p->GlueCount += !(p->Stamps[1] & 1); // why?
  }
  else if (p->RestoreMethod == PPMD8_RESTORE_METHOD_FREEZE)
  {
    while (p->MaxContext->Suffix)
      p->MaxContext = SUFFIX(p->MaxContext);
    RemoveBinContexts(p, p->MaxContext, 0);
    // we change the current mode to (PPMD8_RESTORE_METHOD_FREEZE + 1)
    p->RestoreMethod = PPMD8_RESTORE_METHOD_FREEZE + 1;
    p->GlueCount = 0;
    p->OrderFall = p->MaxOrder;
  }
  else
  #endif
  if (p->RestoreMethod == PPMD8_RESTORE_METHOD_RESTART || GetUsedMemory(p) < (p->Size >> 1))
    Ppmd8_RestartModel(p);
  else
  {
    while (p->MaxContext->Suffix)
      p->MaxContext = SUFFIX(p->MaxContext);
    do
    {
      CutOff(p, p->MaxContext, 0);
      ExpandTextArea(p);
    }
    while (GetUsedMemory(p) > 3 * (p->Size >> 2));
    p->GlueCount = 0;
    p->OrderFall = p->MaxOrder;
  }
  p->MinContext = p->MaxContext;
}



Z7_NO_INLINE
static PPMD8_CTX_PTR Ppmd8_CreateSuccessors(CPpmd8 *p, BoolInt skip, CPpmd_State *s1, PPMD8_CTX_PTR c)
{

  CPpmd_Byte_Ref upBranch = (CPpmd_Byte_Ref)SUCCESSOR(p->FoundState);
  Byte newSym, newFreq, flags;
  unsigned numPs = 0;
  CPpmd_State *ps[PPMD8_MAX_ORDER + 1]; /* fixed over Shkarin's code. Maybe it could work without + 1 too. */
  
  if (!skip)
    ps[numPs++] = p->FoundState;
  
  while (c->Suffix)
  {
    CPpmd_Void_Ref successor;
    CPpmd_State *s;
    c = SUFFIX(c);
    
    if (s1) { s = s1; s1 = NULL; }
    else if (c->NumStats != 0)
    {
      Byte sym = p->FoundState->Symbol;
      for (s = STATS(c); s->Symbol != sym; s++);
      if (s->Freq < MAX_FREQ - 9) { s->Freq++; c->Union2.SummFreq++; }
    }
    else
    {
      s = ONE_STATE(c);
      s->Freq = (Byte)(s->Freq + (!SUFFIX(c)->NumStats & (s->Freq < 24)));
    }
    successor = SUCCESSOR(s);
    if (successor != upBranch)
    {

      c = CTX(successor);
      if (numPs == 0)
      {
        
       
        return c;
      }
      break;
    }
    ps[numPs++] = s;
  }
  
  
  
  
  
  newSym = *(const Byte *)Ppmd8_GetPtr(p, upBranch);
  upBranch++;
  flags = (Byte)(PPMD8_HiBitsFlag_4(p->FoundState->Symbol) + PPMD8_HiBitsFlag_3(newSym));
  
  if (c->NumStats == 0)
    newFreq = c->Union2.State2.Freq;
  else
  {
    UInt32 cf, s0;
    CPpmd_State *s;
    for (s = STATS(c); s->Symbol != newSym; s++);
    cf = (UInt32)s->Freq - 1;
    s0 = (UInt32)c->Union2.SummFreq - c->NumStats - cf;
    /*
    

      max(newFreq)= (s->Freq - 1), when (s0 == 1)


    */
    newFreq = (Byte)(1 + ((2 * cf <= s0) ? (5 * cf > s0) : ((cf + 2 * s0 - 3) / s0)));
  }



  do
  {
    PPMD8_CTX_PTR c1;
    /* = AllocContext(p); */
    if (p->HiUnit != p->LoUnit)
      c1 = (PPMD8_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE);
    else if (p->FreeList[0] != 0)
      c1 = (PPMD8_CTX_PTR)Ppmd8_RemoveNode(p, 0);
    else
    {
      c1 = (PPMD8_CTX_PTR)Ppmd8_AllocUnitsRare(p, 0);
      if (!c1)
        return NULL;
    }
    c1->Flags = flags;
    c1->NumStats = 0;
    c1->Union2.State2.Symbol = newSym;
    c1->Union2.State2.Freq = newFreq;
    Ppmd8State_SetSuccessor(ONE_STATE(c1), upBranch);
    c1->Suffix = REF(c);
    Ppmd8State_SetSuccessor(ps[--numPs], REF(c1));
    c = c1;
  }
  while (numPs != 0);
  
  return c;
}


static PPMD8_CTX_PTR ReduceOrder(CPpmd8 *p, CPpmd_State *s1, PPMD8_CTX_PTR c)
{
  CPpmd_State *s = NULL;
  PPMD8_CTX_PTR c1 = c;
  CPpmd_Void_Ref upBranch = REF(p->Text);
  
  #ifdef PPMD8_FREEZE_SUPPORT
  /* The BUG in Shkarin's code was fixed: ps could overflow in CUT_OFF mode. */
  CPpmd_State *ps[PPMD8_MAX_ORDER + 1];
  unsigned numPs = 0;
  ps[numPs++] = p->FoundState;
  #endif

  Ppmd8State_SetSuccessor(p->FoundState, upBranch);
  p->OrderFall++;

  for (;;)
  {
    if (s1)
    {
      c = SUFFIX(c);
      s = s1;
      s1 = NULL;
    }
    else
    {
      if (!c->Suffix)
      {
        #ifdef PPMD8_FREEZE_SUPPORT
        if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
        {
          do { Ppmd8State_SetSuccessor(ps[--numPs], REF(c)); } while (numPs);
          RESET_TEXT(1)
          p->OrderFall = 1;
        }
        #endif
        return c;
      }
      c = SUFFIX(c);
      if (c->NumStats)
      {
        if ((s = STATS(c))->Symbol != p->FoundState->Symbol)
          do { s++; } while (s->Symbol != p->FoundState->Symbol);
        if (s->Freq < MAX_FREQ - 9)
        {
          s->Freq = (Byte)(s->Freq + 2);
          c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
        }
      }
      else
      {
        s = ONE_STATE(c);
        s->Freq = (Byte)(s->Freq + (s->Freq < 32));
      }
    }
    if (SUCCESSOR(s))
      break;
    #ifdef PPMD8_FREEZE_SUPPORT
    ps[numPs++] = s;
    #endif
    Ppmd8State_SetSuccessor(s, upBranch);
    p->OrderFall++;
  }
  
  #ifdef PPMD8_FREEZE_SUPPORT
  if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    c = CTX(SUCCESSOR(s));
    do { Ppmd8State_SetSuccessor(ps[--numPs], REF(c)); } while (numPs);
    RESET_TEXT(1)
    p->OrderFall = 1;
    return c;
  }
  else
  #endif
  if (SUCCESSOR(s) <= upBranch)
  {
    PPMD8_CTX_PTR successor;
    CPpmd_State *s2 = p->FoundState;
    p->FoundState = s;

    successor = Ppmd8_CreateSuccessors(p, False, NULL, c);
    if (!successor)
      Ppmd8State_SetSuccessor(s, 0);
    else
      Ppmd8State_SetSuccessor(s, REF(successor));
    p->FoundState = s2;
  }
  
  {
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if (p->OrderFall == 1 && c1 == p->MaxContext)
    {
      Ppmd8State_SetSuccessor(p->FoundState, successor);
      p->Text--;
    }
    if (successor == 0)
      return NULL;
    return CTX(successor);
  }
}



void Ppmd8_UpdateModel(CPpmd8 *p);
Z7_NO_INLINE
void Ppmd8_UpdateModel(CPpmd8 *p)
{
  CPpmd_Void_Ref maxSuccessor, minSuccessor = SUCCESSOR(p->FoundState);
  PPMD8_CTX_PTR c;
  unsigned s0, ns, fFreq = p->FoundState->Freq;
  Byte flag, fSymbol = p->FoundState->Symbol;
  {
  CPpmd_State *s = NULL;
  if (p->FoundState->Freq < MAX_FREQ / 4 && p->MinContext->Suffix != 0)
  {
    /* Update Freqs in Suffix Context */

    c = SUFFIX(p->MinContext);
    
    if (c->NumStats == 0)
    {
      s = ONE_STATE(c);
      if (s->Freq < 32)
        s->Freq++;
    }
    else
    {
      Byte sym = p->FoundState->Symbol;
      s = STATS(c);

      if (s->Symbol != sym)
      {
        do
        {
        
          s++;
        }
        while (s->Symbol != sym);
        
        if (s[0].Freq >= s[-1].Freq)
        {
          SWAP_STATES(&s[0], &s[-1]);
          s--;
        }
      }
      
      if (s->Freq < MAX_FREQ - 9)
      {
        s->Freq = (Byte)(s->Freq + 2);
        c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
      }
    }
  }
  
  c = p->MaxContext;
  if (p->OrderFall == 0 && minSuccessor)
  {
    PPMD8_CTX_PTR cs = Ppmd8_CreateSuccessors(p, True, s, p->MinContext);
    if (!cs)
    {
      Ppmd8State_SetSuccessor(p->FoundState, 0);
      RESTORE_MODEL(c, CTX(minSuccessor));
      return;
    }
    Ppmd8State_SetSuccessor(p->FoundState, REF(cs));
    p->MinContext = p->MaxContext = cs;
    return;
  }
  



  {
    Byte *text = p->Text;
    *text++ = p->FoundState->Symbol;
    p->Text = text;
    if (text >= p->UnitsStart)
    {
      RESTORE_MODEL(c, CTX(minSuccessor)); /* check it */
      return;
    }
    maxSuccessor = REF(text);
  }

  if (!minSuccessor)
  {
    PPMD8_CTX_PTR cs = ReduceOrder(p, s, p->MinContext);
    if (!cs)
    {
      RESTORE_MODEL(c, NULL);
      return;
    }
    minSuccessor = REF(cs);
  }
  else if ((Byte *)Ppmd8_GetPtr(p, minSuccessor) < p->UnitsStart)
  {
    PPMD8_CTX_PTR cs = Ppmd8_CreateSuccessors(p, False, s, p->MinContext);
    if (!cs)
    {
      RESTORE_MODEL(c, NULL);
      return;
    }
    minSuccessor = REF(cs);
  }
  
  if (--p->OrderFall == 0)
  {
    maxSuccessor = minSuccessor;
    p->Text -= (p->MaxContext != p->MinContext);
  }
  #ifdef PPMD8_FREEZE_SUPPORT
  else if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    maxSuccessor = minSuccessor;
    RESET_TEXT(0)
    p->OrderFall = 0;
  }
  #endif
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  flag = (Byte)(PPMD8_HiBitsFlag_3(fSymbol));
  s0 = p->MinContext->Union2.SummFreq - (ns = p->MinContext->NumStats) - fFreq;
  
  for (; c != p->MinContext; c = SUFFIX(c))
  {
    unsigned ns1;
    UInt32 sum;
    
    if ((ns1 = c->NumStats) != 0)
    {
      if ((ns1 & 1) != 0)
      {
        /* Expand for one UNIT */
        const unsigned oldNU = (ns1 + 1) >> 1;
        const unsigned i = U2I(oldNU);
        if (i != U2I((size_t)oldNU + 1))
        {
          void *ptr = Ppmd8_AllocUnits(p, i + 1);
          void *oldPtr;
          if (!ptr)
          {
            RESTORE_MODEL(c, CTX(minSuccessor));
            return;
          }
          oldPtr = STATS(c);
          MEM_12_CPY(ptr, oldPtr, oldNU)
          Ppmd8_InsertNode(p, oldPtr, i);
          c->Union4.Stats = STATS_REF(ptr);
        }
      }
      sum = c->Union2.SummFreq;
      /* max increase of Escape_Freq is 1 here.
         an average increase is 1/3 per symbol */
      sum += (UInt32)(unsigned)(3 * ns1 + 1 < ns);
      /* original PPMdH uses 16-bit variable for (sum) here.
         But (sum < ???). Do we need to truncate (sum) to 16-bit */
      // sum = (UInt16)sum;
    }
    else
    {
      
      CPpmd_State *s = (CPpmd_State*)Ppmd8_AllocUnits(p, 0);
      if (!s)
      {
        RESTORE_MODEL(c, CTX(minSuccessor));
        return;
      }
      {
        unsigned freq = c->Union2.State2.Freq;
        // s = *ONE_STATE(c);
        s->Symbol = c->Union2.State2.Symbol;
        s->Successor_0 = c->Union4.State4.Successor_0;
        s->Successor_1 = c->Union4.State4.Successor_1;
        // Ppmd8State_SetSuccessor(s, c->Union4.Stats);  // call it only for debug purposes to check the order of
                                              // (Successor_0 and Successor_1) in LE/BE.
        c->Union4.Stats = REF(s);
        if (freq < MAX_FREQ / 4 - 1)
          freq <<= 1;
        else
          freq = MAX_FREQ - 4;

        s->Freq = (Byte)freq;

        sum = (UInt32)(freq + p->InitEsc + (ns > 2));   // Ppmd8 (> 2)
      }
    }

    {
      CPpmd_State *s = STATS(c) + ns1 + 1;
      UInt32 cf = 2 * (sum + 6) * (UInt32)fFreq;
      UInt32 sf = (UInt32)s0 + sum;
      s->Symbol = fSymbol;
      c->NumStats = (Byte)(ns1 + 1);
      Ppmd8State_SetSuccessor(s, maxSuccessor);
      c->Flags |= flag;
      if (cf < 6 * sf)
      {
        cf = (unsigned)1 + (cf > sf) + (cf >= 4 * sf);
        sum += 4;
        /* It can add (1, 2, 3) to Escape_Freq */
      }
      else
      {
        cf = (unsigned)4 + (cf > 9 * sf) + (cf > 12 * sf) + (cf > 15 * sf);
        sum += cf;
      }

      c->Union2.SummFreq = (UInt16)sum;
      s->Freq = (Byte)cf;
    }

  }
  p->MaxContext = p->MinContext = CTX(minSuccessor);
}
  


Z7_NO_INLINE
static void Ppmd8_Rescale(CPpmd8 *p)
{
  unsigned i, adder, sumFreq, escFreq;
  CPpmd_State *stats = STATS(p->MinContext);
  CPpmd_State *s = p->FoundState;

  /* Sort the list by Freq */
  if (s != stats)
  {
    CPpmd_State tmp = *s;
    do
      s[0] = s[-1];
    while (--s != stats);
    *s = tmp;
  }

  sumFreq = s->Freq;
  escFreq = p->MinContext->Union2.SummFreq - sumFreq;


  
  
  
  
  adder = (p->OrderFall != 0);
  
  #ifdef PPMD8_FREEZE_SUPPORT
  adder |= (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE);
  #endif
  
  sumFreq = (sumFreq + 4 + adder) >> 1;
  i = p->MinContext->NumStats;
  s->Freq = (Byte)sumFreq;
  
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + adder) >> 1;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    if (freq > s[-1].Freq)
    {
      CPpmd_State tmp = *s;
      CPpmd_State *s1 = s;
      do
      {
        s1[0] = s1[-1];
      }
      while (--s1 != stats && freq > s1[-1].Freq);
      *s1 = tmp;
    }
  }
  while (--i);
  
  if (s->Freq == 0)
  {
    /* Remove all items with Freq == 0 */
    CPpmd8_Context *mc;
    unsigned numStats, numStatsNew, n0, n1;
    
    i = 0; do { i++; } while ((--s)->Freq == 0);
    
    

    
    escFreq += i;
    mc = p->MinContext;
    numStats = mc->NumStats;
    numStatsNew = numStats - i;
    mc->NumStats = (Byte)(numStatsNew);
    n0 = (numStats + 2) >> 1;
    
    if (numStatsNew == 0)
    {
    
      unsigned freq = (2 * (unsigned)stats->Freq + escFreq - 1) / escFreq;
      if (freq > MAX_FREQ / 3)
        freq = MAX_FREQ / 3;
      mc->Flags = (Byte)((mc->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(stats->Symbol));
      
      
      
      
      
      s = ONE_STATE(mc);
      *s = *stats;
      s->Freq = (Byte)freq;
      p->FoundState = s;
      Ppmd8_InsertNode(p, stats, U2I(n0));
      return;
    }

    n1 = (numStatsNew + 2) >> 1;
    if (n0 != n1)
      mc->Union4.Stats = STATS_REF(ShrinkUnits(p, stats, n0, n1));
    {
      // here we are for max order only. So Ppmd8_MakeEscFreq() doesn't use mc->Flags
      // but we still need current (Flags & FLAG_PREV_HIGH), if we will convert context to 1-symbol context later.
      /*
      unsigned flags = HiBits_Prepare((s = STATS(mc))->Symbol);
      i = mc->NumStats;
      do { flags |= HiBits_Prepare((++s)->Symbol); } while (--i);
      mc->Flags = (Byte)((mc->Flags & ~FLAG_SYM_HIGH) + HiBits_Convert_3(flags));
      */
    }
  }



  


  {
    CPpmd8_Context *mc = p->MinContext;
    mc->Union2.SummFreq = (UInt16)(sumFreq + escFreq - (escFreq >> 1));
    mc->Flags |= FLAG_RESCALED;
    p->FoundState = STATS(mc);
  }
}


CPpmd_See *Ppmd8_MakeEscFreq(CPpmd8 *p, unsigned numMasked1, UInt32 *escFreq)
{
  CPpmd_See *see;
  const CPpmd8_Context *mc = p->MinContext;
  unsigned numStats = mc->NumStats;
  if (numStats != 0xFF)
  {
    // (3 <= numStats + 2 <= 256)   (3 <= NS2Indx[3] and NS2Indx[256] === 26)
    see = p->See[(size_t)(unsigned)p->NS2Indx[(size_t)numStats + 2] - 3]
        + (mc->Union2.SummFreq > 11 * (numStats + 1))
        + 2 * (unsigned)(2 * numStats < ((unsigned)SUFFIX(mc)->NumStats + numMasked1))
        + mc->Flags;

    {
      // if (see->Summ) field is larger than 16-bit, we need only low 16 bits of Summ
      const unsigned summ = (UInt16)see->Summ; // & 0xFFFF
      const unsigned r = (summ >> see->Shift);
      see->Summ = (UInt16)(summ - r);
      *escFreq = (UInt32)(r + (r == 0));
    }
  }
  else
  {
    see = &p->DummySee;
    *escFreq = 1;
  }
  return see;
}

 
static void Ppmd8_NextContext(CPpmd8 *p)
{
  PPMD8_CTX_PTR c = CTX(SUCCESSOR(p->FoundState));
  if (p->OrderFall == 0 && (const Byte *)c >= p->UnitsStart)
    p->MaxContext = p->MinContext = c;
  else
    Ppmd8_UpdateModel(p);
}
 

void Ppmd8_Update1(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > s[-1].Freq)
  {
    SWAP_STATES(s, &s[-1]);
    p->FoundState = --s;
    if (freq > MAX_FREQ)
      Ppmd8_Rescale(p);
  }
  Ppmd8_NextContext(p);
}


void Ppmd8_Update1_0(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  CPpmd8_Context *mc = p->MinContext;
  unsigned freq = s->Freq;
  const unsigned summFreq = mc->Union2.SummFreq;
  p->PrevSuccess = (2 * freq >= summFreq); // Ppmd8 (>=)
  p->RunLength += (Int32)p->PrevSuccess;
  mc->Union2.SummFreq = (UInt16)(summFreq + 4);
  freq += 4;
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd8_Rescale(p);
  Ppmd8_NextContext(p);
}


/*
void Ppmd8_UpdateBin(CPpmd8 *p)
{
  unsigned freq = p->FoundState->Freq;
  p->FoundState->Freq = (Byte)(freq + (freq < 196)); // Ppmd8 (196)
  p->PrevSuccess = 1;
  p->RunLength++;
  Ppmd8_NextContext(p);
}
*/

void Ppmd8_Update2(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->RunLength = p->InitRL;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd8_Rescale(p);
  Ppmd8_UpdateModel(p);
}

/* H->I changes:
  NS2Indx
  GlueCount, and Glue method
  BinSum
  See / EscFreq
  Ppmd8_CreateSuccessors updates more suffix contexts
  Ppmd8_UpdateModel consts.
  PrevSuccess Update

Flags:
  (1 << 2) - the Context was Rescaled
  (1 << 3) - there is symbol in Stats with (sym >= 0x40) in
  (1 << 4) - main symbol of context is (sym >= 0x40)
*/

#undef RESET_TEXT
#undef FLAG_RESCALED
#undef FLAG_PREV_HIGH
#undef HiBits_Prepare
#undef HiBits_Convert_3
#undef HiBits_Convert_4
#undef PPMD8_HiBitsFlag_3
#undef PPMD8_HiBitsFlag_4
#undef RESTORE_MODEL

#undef MAX_FREQ
#undef UNIT_SIZE
#undef U2B
#undef U2I
#undef I2U

#undef REF
#undef STATS_REF
#undef CTX
#undef STATS
#undef ONE_STATE
#undef SUFFIX
#undef NODE
#undef EMPTY_NODE
#undef MEM_12_CPY
#undef SUCCESSOR
#undef SWAP_STATES
