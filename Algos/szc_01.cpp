/* XArchive amalgamation of 7-Zip 26.01 -- 7-Zip C codecs.
 *
 * 16 upstream translation units folded into one. Code is verbatim;
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

/* ---- C/7zBuf.h ---- */
/* 7zBuf.h -- Byte Buffer
2023-03-04 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_7Z_BUF_H
#define ZIP7_INC_7Z_BUF_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

typedef struct
{
  Byte *data;
  size_t size;
} CBuf;

void Buf_Init(CBuf *p);
int Buf_Create(CBuf *p, size_t size, ISzAllocPtr alloc);
void Buf_Free(CBuf *p, ISzAllocPtr alloc);

typedef struct
{
  Byte *data;
  size_t size;
  size_t pos;
} CDynBuf;

void DynBuf_Construct(CDynBuf *p);
void DynBuf_SeekToBeg(CDynBuf *p);
int DynBuf_Write(CDynBuf *p, const Byte *buf, size_t size, ISzAllocPtr alloc);
void DynBuf_Free(CDynBuf *p, ISzAllocPtr alloc);

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

/* ---- C/Aes.h ---- */
/* Aes.h -- AES encryption / decryption
2023-04-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_AES_H
#define ZIP7_INC_AES_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define AES_BLOCK_SIZE 16

/* Call AesGenTables one time before other AES functions */
void AesGenTables(void);

/* UInt32 pointers must be 16-byte aligned */

/* 16-byte (4 * 32-bit words) blocks: 1 (IV) + 1 (keyMode) + 15 (AES-256 roundKeys) */
#define AES_NUM_IVMRK_WORDS ((1 + 1 + 15) * 4)

/* aes - 16-byte aligned pointer to keyMode+roundKeys sequence */
/* keySize = 16 or 24 or 32 (bytes) */
typedef void (Z7_FASTCALL *AES_SET_KEY_FUNC)(UInt32 *aes, const Byte *key, unsigned keySize);
void Z7_FASTCALL Aes_SetKey_Enc(UInt32 *aes, const Byte *key, unsigned keySize);
void Z7_FASTCALL Aes_SetKey_Dec(UInt32 *aes, const Byte *key, unsigned keySize);

/* ivAes - 16-byte aligned pointer to iv+keyMode+roundKeys sequence: UInt32[AES_NUM_IVMRK_WORDS] */
void AesCbc_Init(UInt32 *ivAes, const Byte *iv); /* iv size is AES_BLOCK_SIZE */

/* data - 16-byte aligned pointer to data */
/* numBlocks - the number of 16-byte blocks in data array */
typedef void (Z7_FASTCALL *AES_CODE_FUNC)(UInt32 *ivAes, Byte *data, size_t numBlocks);

extern AES_CODE_FUNC g_AesCbc_Decode;
#ifndef Z7_SFX
extern AES_CODE_FUNC g_AesCbc_Encode;
extern AES_CODE_FUNC g_AesCtr_Code;
#define k_Aes_SupportedFunctions_HW     (1 << 2)
#define k_Aes_SupportedFunctions_HW_256 (1 << 3)
extern UInt32 g_Aes_SupportedFunctions_Flags;
#endif


#define Z7_DECLARE_AES_CODE_FUNC(funcName) \
    void Z7_FASTCALL funcName(UInt32 *ivAes, Byte *data, size_t numBlocks);

Z7_DECLARE_AES_CODE_FUNC (AesCbc_Encode)
Z7_DECLARE_AES_CODE_FUNC (AesCbc_Decode)
Z7_DECLARE_AES_CODE_FUNC (AesCtr_Code)

Z7_DECLARE_AES_CODE_FUNC (AesCbc_Encode_HW)
Z7_DECLARE_AES_CODE_FUNC (AesCbc_Decode_HW)
Z7_DECLARE_AES_CODE_FUNC (AesCtr_Code_HW)

Z7_DECLARE_AES_CODE_FUNC (AesCbc_Decode_HW_256)
Z7_DECLARE_AES_CODE_FUNC (AesCtr_Code_HW_256)

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

/* ---- C/Bcj2.h ---- */
/* Bcj2.h -- BCJ2 converter for x86 code (Branch CALL/JUMP variant2)
2023-03-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BCJ2_H
#define ZIP7_INC_BCJ2_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define BCJ2_NUM_STREAMS 4

enum
{
  BCJ2_STREAM_MAIN,
  BCJ2_STREAM_CALL,
  BCJ2_STREAM_JUMP,
  BCJ2_STREAM_RC
};

enum
{
  BCJ2_DEC_STATE_ORIG_0 = BCJ2_NUM_STREAMS,
  BCJ2_DEC_STATE_ORIG_1,
  BCJ2_DEC_STATE_ORIG_2,
  BCJ2_DEC_STATE_ORIG_3,
  
  BCJ2_DEC_STATE_ORIG,
  BCJ2_DEC_STATE_ERROR     /* after detected data error */
};

enum
{
  BCJ2_ENC_STATE_ORIG = BCJ2_NUM_STREAMS,
  BCJ2_ENC_STATE_FINISHED  /* it's state after fully encoded stream */
};


/* #define BCJ2_IS_32BIT_STREAM(s) ((s) == BCJ2_STREAM_CALL || (s) == BCJ2_STREAM_JUMP) */
#define BCJ2_IS_32BIT_STREAM(s) ((unsigned)((unsigned)(s) - (unsigned)BCJ2_STREAM_CALL) < 2)

/*
CBcj2Dec / CBcj2Enc
bufs sizes:
  BUF_SIZE(n) = lims[n] - bufs[n]
bufs sizes for BCJ2_STREAM_CALL and BCJ2_STREAM_JUMP must be multiply of 4:
    (BUF_SIZE(BCJ2_STREAM_CALL) & 3) == 0
    (BUF_SIZE(BCJ2_STREAM_JUMP) & 3) == 0
*/

// typedef UInt32 CBcj2Prob;
typedef UInt16 CBcj2Prob;

/*
BCJ2 encoder / decoder internal requirements:
  - If last bytes of stream contain marker (e8/e8/0f8x), then
    there is also encoded symbol (0 : no conversion) in RC stream.
  - One case of overlapped instructions is supported,
    if last byte of converted instruction is (0f) and next byte is (8x):
      marker [xx xx xx 0f] 8x
    then the pair (0f 8x) is treated as marker.
*/

/* ---------- BCJ2 Decoder ---------- */

/*
CBcj2Dec:
(dest) is allowed to overlap with bufs[BCJ2_STREAM_MAIN], with the following conditions:
  bufs[BCJ2_STREAM_MAIN] >= dest &&
  bufs[BCJ2_STREAM_MAIN] - dest >=
        BUF_SIZE(BCJ2_STREAM_CALL) +
        BUF_SIZE(BCJ2_STREAM_JUMP)
  reserve = bufs[BCJ2_STREAM_MAIN] - dest -
      ( BUF_SIZE(BCJ2_STREAM_CALL) +
        BUF_SIZE(BCJ2_STREAM_JUMP) )
  and additional conditions:
  if (it's first call of Bcj2Dec_Decode() after Bcj2Dec_Init())
  {
    (reserve != 1) : if (ver <  v23.00)
  }
  else // if there are more than one calls of Bcj2Dec_Decode() after Bcj2Dec_Init())
  {
    (reserve >= 6) : if (ver <  v23.00)
    (reserve >= 4) : if (ver >= v23.00)
    We need that (reserve) because after first call of Bcj2Dec_Decode(),
    CBcj2Dec::temp can contain up to 4 bytes for writing to (dest).
  }
  (reserve == 0) is allowed, if we decode full stream via single call of Bcj2Dec_Decode().
  (reserve == 0) also is allowed in case of multi-call, if we use fixed buffers,
     and (reserve) is calculated from full (final) sizes of all streams before first call.
*/

typedef struct
{
  const Byte *bufs[BCJ2_NUM_STREAMS];
  const Byte *lims[BCJ2_NUM_STREAMS];
  Byte *dest;
  const Byte *destLim;

  unsigned state; /* BCJ2_STREAM_MAIN has more priority than BCJ2_STATE_ORIG */

  UInt32 ip;      /* property of starting base for decoding */
  UInt32 temp;    /* Byte temp[4]; */
  UInt32 range;
  UInt32 code;
  CBcj2Prob probs[2 + 256];
} CBcj2Dec;


/* Note:
   Bcj2Dec_Init() sets (CBcj2Dec::ip = 0)
   if (ip != 0) property is required, the caller must set CBcj2Dec::ip after Bcj2Dec_Init()
*/
void Bcj2Dec_Init(CBcj2Dec *p);


/* Bcj2Dec_Decode():
   returns:
     SZ_OK
     SZ_ERROR_DATA : if data in 5 starting bytes of BCJ2_STREAM_RC stream are not correct
*/
SRes Bcj2Dec_Decode(CBcj2Dec *p);

/* To check that decoding was finished you can compare
   sizes of processed streams with sizes known from another sources.
   You must do at least one mandatory check from the two following options:
      - the check for size of processed output (ORIG) stream.
      - the check for size of processed input  (MAIN) stream.
   additional optional checks:
      - the checks for processed sizes of all input streams (MAIN, CALL, JUMP, RC)
      - the checks Bcj2Dec_IsMaybeFinished*()
   also before actual decoding you can check that the
   following condition is met for stream sizes:
     ( size(ORIG) == size(MAIN) + size(CALL) + size(JUMP) )
*/

/* (state == BCJ2_STREAM_MAIN) means that decoder is ready for
      additional input data in BCJ2_STREAM_MAIN stream.
   Note that (state == BCJ2_STREAM_MAIN) is allowed for non-finished decoding.
*/
#define Bcj2Dec_IsMaybeFinished_state_MAIN(_p_) ((_p_)->state == BCJ2_STREAM_MAIN)

/* if the stream decoding was finished correctly, then range decoder
   part of CBcj2Dec also was finished, and then (CBcj2Dec::code == 0).
   Note that (CBcj2Dec::code == 0) is allowed for non-finished decoding.
*/
#define Bcj2Dec_IsMaybeFinished_code(_p_) ((_p_)->code == 0)

/* use Bcj2Dec_IsMaybeFinished() only as additional check
    after at least one mandatory check from the two following options:
      - the check for size of processed output (ORIG) stream.
      - the check for size of processed input  (MAIN) stream.
*/
#define Bcj2Dec_IsMaybeFinished(_p_) ( \
        Bcj2Dec_IsMaybeFinished_state_MAIN(_p_) && \
        Bcj2Dec_IsMaybeFinished_code(_p_))



/* ---------- BCJ2 Encoder ---------- */

typedef enum
{
  BCJ2_ENC_FINISH_MODE_CONTINUE,
  BCJ2_ENC_FINISH_MODE_END_BLOCK,
  BCJ2_ENC_FINISH_MODE_END_STREAM
} EBcj2Enc_FinishMode;

/*
  BCJ2_ENC_FINISH_MODE_CONTINUE:
     process non finished encoding.
     It notifies the encoder that additional further calls
     can provide more input data (src) than provided by current call.
     In  that case the CBcj2Enc encoder still can move (src) pointer
     up to (srcLim), but CBcj2Enc encoder can store some of the last
     processed bytes (up to 4 bytes) from src to internal CBcj2Enc::temp[] buffer.
   at return:
       (CBcj2Enc::src will point to position that includes
       processed data and data copied to (temp[]) buffer)
       That data from (temp[]) buffer will be used in further calls.
  
  BCJ2_ENC_FINISH_MODE_END_BLOCK:
     finish encoding of current block (ended at srcLim) without RC flushing.
   at return: if (CBcj2Enc::state == BCJ2_ENC_STATE_ORIG) &&
                  CBcj2Enc::src == CBcj2Enc::srcLim)
        :  it shows that block encoding was finished. And the encoder is
           ready for new (src) data or for stream finish operation.
     finished block means
     {
       CBcj2Enc has completed block encoding up to (srcLim).
       (1 + 4 bytes) or (2 + 4 bytes) CALL/JUMP cortages will
       not cross block boundary at (srcLim).
       temporary CBcj2Enc buffer for (ORIG) src data is empty.
       3 output uncompressed streams (MAIN, CALL, JUMP) were flushed.
       RC stream was not flushed. And RC stream will cross block boundary.
     }
     Note: some possible implementation of BCJ2 encoder could
     write branch marker (e8/e8/0f8x) in one call of Bcj2Enc_Encode(),
     and it could calculate symbol for RC in another call of Bcj2Enc_Encode().
     BCJ2 encoder uses ip/fileIp/fileSize/relatLimit values to calculate RC symbol.
     And these CBcj2Enc variables can have different values in different Bcj2Enc_Encode() calls.
     So caller must finish each block with BCJ2_ENC_FINISH_MODE_END_BLOCK
     to ensure that RC symbol is calculated and written in proper block.
    
  BCJ2_ENC_FINISH_MODE_END_STREAM
     finish encoding of stream (ended at srcLim) fully including RC flushing.
   at return: if (CBcj2Enc::state == BCJ2_ENC_STATE_FINISHED)
        : it shows that stream encoding was finished fully,
          and all output streams were flushed fully.
     also Bcj2Enc_IsFinished() can be called.
*/


/*
  32-bit relative offset in JUMP/CALL commands is
    - (mod 4 GiB)  for 32-bit x86 code
    - signed Int32 for 64-bit x86-64 code
  BCJ2 encoder also does internal relative to absolute address conversions.
  And there are 2 possible ways to do it:
    before v23: we used 32-bit variables and (mod 4 GiB) conversion
    since  v23: we use  64-bit variables and (signed Int32 offset) conversion.
  The absolute address condition for conversion in v23:
    ((UInt64)((Int64)ip64 - (Int64)fileIp64 + 5 + (Int32)offset) < (UInt64)fileSize64)
  note that if (fileSize64 > 2 GiB). there is difference between
  old (mod 4 GiB) way (v22) and new (signed Int32 offset) way (v23).
  And new (v23) way is more suitable to encode 64-bit x86-64 code for (fileSize64 > 2 GiB) cases.
*/

/*
// for old (v22) way for conversion:
typedef UInt32 CBcj2Enc_ip_unsigned;
typedef  Int32 CBcj2Enc_ip_signed;
#define BCJ2_ENC_FileSize_MAX ((UInt32)1 << 31)
*/
typedef UInt64 CBcj2Enc_ip_unsigned;
typedef  Int64 CBcj2Enc_ip_signed;

/* maximum size of file that can be used for conversion condition */
#define BCJ2_ENC_FileSize_MAX             ((CBcj2Enc_ip_unsigned)0 - 2)

/* default value of fileSize64_minus1 variable that means
   that absolute address limitation will not be used */
#define BCJ2_ENC_FileSizeField_UNLIMITED  ((CBcj2Enc_ip_unsigned)0 - 1)

/* calculate value that later can be set to CBcj2Enc::fileSize64_minus1 */
#define BCJ2_ENC_GET_FileSizeField_VAL_FROM_FileSize(fileSize) \
    ((CBcj2Enc_ip_unsigned)(fileSize) - 1)

/* set CBcj2Enc::fileSize64_minus1 variable from size of file */
#define Bcj2Enc_SET_FileSize(p, fileSize) \
    (p)->fileSize64_minus1 = BCJ2_ENC_GET_FileSizeField_VAL_FROM_FileSize(fileSize);


typedef struct
{
  Byte *bufs[BCJ2_NUM_STREAMS];
  const Byte *lims[BCJ2_NUM_STREAMS];
  const Byte *src;
  const Byte *srcLim;

  unsigned state;
  EBcj2Enc_FinishMode finishMode;

  Byte context;
  Byte flushRem;
  Byte isFlushState;

  Byte cache;
  UInt32 range;
  UInt64 low;
  UInt64 cacheSize;
  
  // UInt32 context;  // for marker version, it can include marker flag.

  /* (ip64) and (fileIp64) correspond to virtual source stream position
     that doesn't include data in temp[] */
  CBcj2Enc_ip_unsigned ip64;         /* current (ip) position */
  CBcj2Enc_ip_unsigned fileIp64;     /* start (ip) position of current file */
  CBcj2Enc_ip_unsigned fileSize64_minus1;   /* size of current file (for conversion limitation) */
  UInt32 relatLimit;  /* (relatLimit <= ((UInt32)1 << 31)) : 0 means disable_conversion */
  // UInt32 relatExcludeBits;

  UInt32 tempTarget;
  unsigned tempPos; /* the number of bytes that were copied to temp[] buffer
                       (tempPos <= 4) outside of Bcj2Enc_Encode() */
  // Byte temp[4]; // for marker version
  Byte temp[8];
  CBcj2Prob probs[2 + 256];
} CBcj2Enc;

void Bcj2Enc_Init(CBcj2Enc *p);


/*
Bcj2Enc_Encode(): at exit:
  p->State <  BCJ2_NUM_STREAMS    : we need more buffer space for output stream
                                    (bufs[p->State] == lims[p->State])
  p->State == BCJ2_ENC_STATE_ORIG : we need more data in input src stream
                                    (src == srcLim)
  p->State == BCJ2_ENC_STATE_FINISHED : after fully encoded stream
*/
void Bcj2Enc_Encode(CBcj2Enc *p);

/* Bcj2Enc encoder can look ahead for up 4 bytes of source stream.
   CBcj2Enc::tempPos : is the number of bytes that were copied from input stream to temp[] buffer.
   (CBcj2Enc::src) after Bcj2Enc_Encode() is starting position after
   fully processed data and after data copied to temp buffer.
   So if the caller needs to get real number of fully processed input
   bytes (without look ahead data in temp buffer),
   the caller must subtruct (CBcj2Enc::tempPos) value from processed size
   value that is calculated based on current (CBcj2Enc::src):
     cur_processed_pos = Calc_Big_Processed_Pos(enc.src)) -
        Bcj2Enc_Get_AvailInputSize_in_Temp(&enc);
*/
/* get the size of input data that was stored in temp[] buffer: */
#define Bcj2Enc_Get_AvailInputSize_in_Temp(p) ((p)->tempPos)

#define Bcj2Enc_IsFinished(p) ((p)->flushRem == 0)

/* Note : the decoder supports overlapping of marker (0f 80).
   But we can eliminate such overlapping cases by setting
   the limit for relative offset conversion as
     CBcj2Enc::relatLimit <= (0x0f << 24) == (240 MiB)
*/
/* default value for CBcj2Enc::relatLimit */
#define BCJ2_ENC_RELAT_LIMIT_DEFAULT  ((UInt32)0x0f << 24)
#define BCJ2_ENC_RELAT_LIMIT_MAX      ((UInt32)1 << 31)
// #define BCJ2_RELAT_EXCLUDE_NUM_BITS 5

EXTERN_C_END

#endif

/* ---- C/Blake2.h ---- */
/* Blake2.h -- BLAKE2sp Hash
2024-01-17 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BLAKE2_H
#define ZIP7_INC_BLAKE2_H

// amalgamation: header emitted in prologue

#if 0
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#if defined(MY_CPU_X86_OR_AMD64)
#if defined(__SSE2__) \
    || defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
#include <emmintrin.h> // SSE2
#endif

#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
#include <immintrin.h>
#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif
#endif // avx2
#endif // MY_CPU_X86_OR_AMD64
#endif // 0

EXTERN_C_BEGIN

#define Z7_BLAKE2S_BLOCK_SIZE         64
#define Z7_BLAKE2S_DIGEST_SIZE        32
#define Z7_BLAKE2SP_PARALLEL_DEGREE   8
#define Z7_BLAKE2SP_NUM_STRUCT_WORDS  16

#if 1 || defined(Z7_BLAKE2SP_USE_FUNCTIONS)
typedef void (Z7_FASTCALL *Z7_BLAKE2SP_FUNC_COMPRESS)(UInt32 *states, const Byte *data, const Byte *end);
typedef void (Z7_FASTCALL *Z7_BLAKE2SP_FUNC_INIT)(UInt32 *states);
#endif

// it's required that CBlake2sp is aligned for 32-bytes,
// because the code can use unaligned access with sse and avx256.
// but 64-bytes alignment can be better.
MY_ALIGN(64)
typedef struct
{
  union
  {
#if 0
#if defined(MY_CPU_X86_OR_AMD64)
#if defined(__SSE2__) \
    || defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
    __m128i _pad_align_128bit[4];
#endif // sse2
#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
    __m256i _pad_align_256bit[2];
#endif // avx2
#endif // x86
#endif // 0

    void * _pad_align_ptr[8];
    UInt32 _pad_align_32bit[16];
    struct
    {
      unsigned cycPos;
      unsigned _pad_unused;
#if 1 || defined(Z7_BLAKE2SP_USE_FUNCTIONS)
      Z7_BLAKE2SP_FUNC_COMPRESS func_Compress_Fast;
      Z7_BLAKE2SP_FUNC_COMPRESS func_Compress_Single;
      Z7_BLAKE2SP_FUNC_INIT func_Init;
      Z7_BLAKE2SP_FUNC_INIT func_Final;
#endif
    } header;
  } u;
  // MY_ALIGN(64)
  UInt32 states[Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2SP_NUM_STRUCT_WORDS];
  // MY_ALIGN(64)
  UInt32  buf32[Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2SP_NUM_STRUCT_WORDS * 2];
} CBlake2sp;

BoolInt Blake2sp_SetFunction(CBlake2sp *p, unsigned algo);
void Blake2sp_Init(CBlake2sp *p);
void Blake2sp_InitState(CBlake2sp *p);
void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size);
void Blake2sp_Final(CBlake2sp *p, Byte *digest);
void z7_Black2sp_Prepare(void);

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

/* ---- C/BwtSort.h ---- */
/* BwtSort.h -- BWT block sorting
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BWT_SORT_H
#define ZIP7_INC_BWT_SORT_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/* use BLOCK_SORT_EXTERNAL_FLAGS if blockSize can be > 1M */
/* #define BLOCK_SORT_EXTERNAL_FLAGS */
// #define BLOCK_SORT_EXTERNAL_FLAGS

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
#define BLOCK_SORT_EXTERNAL_SIZE(blockSize) (((blockSize) + 31) >> 5)
#else
#define BLOCK_SORT_EXTERNAL_SIZE(blockSize) 0
#endif

#define BLOCK_SORT_BUF_SIZE(blockSize) ((blockSize) * 2 + BLOCK_SORT_EXTERNAL_SIZE(blockSize) + (1 << 16))

UInt32 BlockSort(UInt32 *indices, const Byte *data, size_t blockSize);

EXTERN_C_END

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

/* ================ unit bodies ================ */

/* ================ unit: C/7zBuf2.c ================ */
/* 7zBuf2.c -- Byte Buffer
2017-04-03 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue

void DynBuf_Construct(CDynBuf *p)
{
  p->data = 0;
  p->size = 0;
  p->pos = 0;
}

void DynBuf_SeekToBeg(CDynBuf *p)
{
  p->pos = 0;
}

int DynBuf_Write(CDynBuf *p, const Byte *buf, size_t size, ISzAllocPtr alloc)
{
  if (size > p->size - p->pos)
  {
    size_t newSize = p->pos + size;
    Byte *data;
    newSize += newSize / 4;
    data = (Byte *)ISzAlloc_Alloc(alloc, newSize);
    if (!data)
      return 0;
    p->size = newSize;
    if (p->pos != 0)
      memcpy(data, p->data, p->pos);
    ISzAlloc_Free(alloc, p->data);
    p->data = data;
  }
  if (size != 0)
  {
    memcpy(p->data + p->pos, buf, size);
    p->pos += size;
  }
  return 1;
}

void DynBuf_Free(CDynBuf *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->data);
  p->data = 0;
  p->size = 0;
  p->pos = 0;
}

/* ================ unit: C/7zCrc.c ================ */
/* 7zCrc.c -- CRC32 calculation and init
2024-03-01 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// for debug:
// #define __ARM_FEATURE_CRC32 1

#ifdef __ARM_FEATURE_CRC32
// #pragma message("__ARM_FEATURE_CRC32")
#define Z7_CRC_HW_FORCE
#endif

// #define Z7_CRC_DEBUG_BE
#ifdef Z7_CRC_DEBUG_BE
#undef MY_CPU_LE
#define MY_CPU_BE
#endif

#ifdef Z7_CRC_HW_FORCE
  #define Z7_CRC_NUM_TABLES_USE  1
#else
#ifdef Z7_CRC_NUM_TABLES
  #define Z7_CRC_NUM_TABLES_USE  Z7_CRC_NUM_TABLES
#else
  #define Z7_CRC_NUM_TABLES_USE  12
#endif
#endif

#if Z7_CRC_NUM_TABLES_USE < 1
  #error Stop_Compiling_Bad_Z7_CRC_NUM_TABLES
#endif

#if defined(MY_CPU_LE) || (Z7_CRC_NUM_TABLES_USE == 1)
  #define Z7_CRC_NUM_TABLES_TOTAL  Z7_CRC_NUM_TABLES_USE
#else
  #define Z7_CRC_NUM_TABLES_TOTAL  (Z7_CRC_NUM_TABLES_USE + 1)
#endif

#ifndef Z7_CRC_HW_FORCE

#if Z7_CRC_NUM_TABLES_USE == 1 \
   || (!defined(MY_CPU_LE) && !defined(MY_CPU_BE))
#define CRC_UPDATE_BYTE_2(crc, b)   (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))
#define Z7_CRC_UPDATE_T1_FUNC_NAME  CrcUpdateGT1
static UInt32 Z7_FASTCALL Z7_CRC_UPDATE_T1_FUNC_NAME(UInt32 v, const void *data, size_t size)
{
  const UInt32 *table = g_CrcTable;
  const Byte *p = (const Byte *)data;
  const Byte *lim = p + size;
  for (; p != lim; p++)
    v = CRC_UPDATE_BYTE_2(v, *p);
  return v;
}
#endif


#if Z7_CRC_NUM_TABLES_USE != 1
#ifndef MY_CPU_BE
  #define FUNC_NAME_LE_2(s)   CrcUpdateT ## s
  #define FUNC_NAME_LE_1(s)   FUNC_NAME_LE_2(s)
  #define FUNC_NAME_LE        FUNC_NAME_LE_1(Z7_CRC_NUM_TABLES_USE)
  UInt32 Z7_FASTCALL FUNC_NAME_LE (UInt32 v, const void *data, size_t size, const UInt32 *table);
#endif
#ifndef MY_CPU_LE
  #define FUNC_NAME_BE_2(s)   CrcUpdateT1_BeT ## s
  #define FUNC_NAME_BE_1(s)   FUNC_NAME_BE_2(s)
  #define FUNC_NAME_BE        FUNC_NAME_BE_1(Z7_CRC_NUM_TABLES_USE)
  UInt32 Z7_FASTCALL FUNC_NAME_BE (UInt32 v, const void *data, size_t size, const UInt32 *table);
#endif
#endif

#endif // Z7_CRC_HW_FORCE

/* ---------- hardware CRC ---------- */

#ifdef MY_CPU_LE

#if defined(MY_CPU_ARM_OR_ARM64)
// #pragma message("ARM*")

  #if (defined(__clang__) && (__clang_major__ >= 3)) \
     || defined(__GNUC__) && (__GNUC__ >= 6) && defined(MY_CPU_ARM64) \
     || defined(__GNUC__) && (__GNUC__ >= 8)
      #if !defined(__ARM_FEATURE_CRC32)
//        #pragma message("!defined(__ARM_FEATURE_CRC32)")
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
        #define __ARM_FEATURE_CRC32 1
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
        #define Z7_ARM_FEATURE_CRC32_WAS_SET
        #if defined(__clang__)
          #if defined(MY_CPU_ARM64)
            #define ATTRIB_CRC __attribute__((__target__("crc")))
          #else
            #define ATTRIB_CRC __attribute__((__target__("armv8-a,crc")))
          #endif
        #else
          #if defined(MY_CPU_ARM64)
#if !defined(Z7_GCC_VERSION) || (Z7_GCC_VERSION >= 60000)
            #define ATTRIB_CRC __attribute__((__target__("+crc")))
#endif
          #else
#if !defined(Z7_GCC_VERSION) || (__GNUC__  >= 8)
#if defined(__ARM_FP) && __GNUC__ >= 8
// for -mfloat-abi=hard: similar to <arm_acle.h>
            #define ATTRIB_CRC __attribute__((__target__("arch=armv8-a+crc+simd")))
#else
            #define ATTRIB_CRC __attribute__((__target__("arch=armv8-a+crc")))
#endif
#endif
          #endif
        #endif
      #endif
      #if defined(__ARM_FEATURE_CRC32)
      // #pragma message("<arm_acle.h>")
/*
arm_acle.h (GGC):
    before Nov 17, 2017:
#ifdef __ARM_FEATURE_CRC32

    Nov 17, 2017: gcc10.0  (gcc 9.2.0) checked"
#if __ARM_ARCH >= 8
#pragma GCC target ("arch=armv8-a+crc")

    Aug 22, 2019: GCC 8.4?, 9.2.1, 10.1:
#ifdef __ARM_FEATURE_CRC32
#ifdef __ARM_FP
#pragma GCC target ("arch=armv8-a+crc+simd")
#else
#pragma GCC target ("arch=armv8-a+crc")
#endif
*/
#if defined(__ARM_ARCH) && __ARM_ARCH < 8
#if defined(Z7_GCC_VERSION) && (__GNUC__ ==   8) && (Z7_GCC_VERSION <  80400) \
 || defined(Z7_GCC_VERSION) && (__GNUC__ ==   9) && (Z7_GCC_VERSION <  90201) \
 || defined(Z7_GCC_VERSION) && (__GNUC__ ==  10) && (Z7_GCC_VERSION < 100100)
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
// #pragma message("#define __ARM_ARCH 8")
#undef  __ARM_ARCH
#define __ARM_ARCH 8
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif
        #define Z7_CRC_HW_USE
        #include <arm_acle.h>
      #endif
  #elif defined(_MSC_VER)
    #if defined(MY_CPU_ARM64)
    #if (_MSC_VER >= 1910)
    #ifdef __clang__
       // #define Z7_CRC_HW_USE
       // #include <arm_acle.h>
    #else
       #define Z7_CRC_HW_USE
       #include <intrin.h>
    #endif
    #endif
    #endif
  #endif

#else // non-ARM*

// #define Z7_CRC_HW_USE // for debug : we can test HW-branch of code
#ifdef Z7_CRC_HW_USE
#include "7zCrcEmu.h"
#endif

#endif // non-ARM*



#if defined(Z7_CRC_HW_USE)

// #pragma message("USE ARM HW CRC")

#ifdef MY_CPU_64BIT
  #define CRC_HW_WORD_TYPE  UInt64
  #define CRC_HW_WORD_FUNC  __crc32d
#else
  #define CRC_HW_WORD_TYPE  UInt32
  #define CRC_HW_WORD_FUNC  __crc32w
#endif

#define CRC_HW_UNROLL_BYTES (sizeof(CRC_HW_WORD_TYPE) * 4)

#ifdef ATTRIB_CRC
  ATTRIB_CRC
#endif
Z7_NO_INLINE
#ifdef Z7_CRC_HW_FORCE
         UInt32 Z7_FASTCALL CrcUpdate
#else
  static UInt32 Z7_FASTCALL CrcUpdate_HW
#endif
    (UInt32 v, const void *data, size_t size)
{
  const Byte *p = (const Byte *)data;
  for (; size != 0 && ((unsigned)(ptrdiff_t)p & (CRC_HW_UNROLL_BYTES - 1)) != 0; size--)
    v = __crc32b(v, *p++);
  if (size >= CRC_HW_UNROLL_BYTES)
  {
    const Byte *lim = p + size;
    size &= CRC_HW_UNROLL_BYTES - 1;
    lim -= size;
    do
    {
      v = CRC_HW_WORD_FUNC(v, *(const CRC_HW_WORD_TYPE *)(const void *)(p));
      v = CRC_HW_WORD_FUNC(v, *(const CRC_HW_WORD_TYPE *)(const void *)(p + sizeof(CRC_HW_WORD_TYPE)));
      p += 2 * sizeof(CRC_HW_WORD_TYPE);
      v = CRC_HW_WORD_FUNC(v, *(const CRC_HW_WORD_TYPE *)(const void *)(p));
      v = CRC_HW_WORD_FUNC(v, *(const CRC_HW_WORD_TYPE *)(const void *)(p + sizeof(CRC_HW_WORD_TYPE)));
      p += 2 * sizeof(CRC_HW_WORD_TYPE);
    }
    while (p != lim);
  }
  
  for (; size != 0; size--)
    v = __crc32b(v, *p++);

  return v;
}

#ifdef Z7_ARM_FEATURE_CRC32_WAS_SET
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
#undef __ARM_FEATURE_CRC32
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#undef Z7_ARM_FEATURE_CRC32_WAS_SET
#endif

#endif // defined(Z7_CRC_HW_USE)
#endif // MY_CPU_LE



#ifndef Z7_CRC_HW_FORCE

#if defined(Z7_CRC_HW_USE) || defined(Z7_CRC_UPDATE_T1_FUNC_NAME)
/*
typedef UInt32 (Z7_FASTCALL *Z7_CRC_UPDATE_WITH_TABLE_FUNC)
    (UInt32 v, const void *data, size_t size, const UInt32 *table);
Z7_CRC_UPDATE_WITH_TABLE_FUNC g_CrcUpdate;
*/
static unsigned g_Crc_Algo;
#if (!defined(MY_CPU_LE) && !defined(MY_CPU_BE))
static unsigned g_Crc_Be;
#endif
#endif // defined(Z7_CRC_HW_USE) || defined(Z7_CRC_UPDATE_T1_FUNC_NAME)



Z7_NO_INLINE
#ifdef Z7_CRC_HW_USE
  static UInt32 Z7_FASTCALL CrcUpdate_Base
#else
         UInt32 Z7_FASTCALL CrcUpdate
#endif
    (UInt32 crc, const void *data, size_t size)
{
#if Z7_CRC_NUM_TABLES_USE == 1
    return Z7_CRC_UPDATE_T1_FUNC_NAME(crc, data, size);
#else // Z7_CRC_NUM_TABLES_USE != 1
#ifdef Z7_CRC_UPDATE_T1_FUNC_NAME
  if (g_Crc_Algo == 1)
    return Z7_CRC_UPDATE_T1_FUNC_NAME(crc, data, size);
#endif

#ifdef MY_CPU_LE
    return FUNC_NAME_LE(crc, data, size, g_CrcTable);
#elif defined(MY_CPU_BE)
    return FUNC_NAME_BE(crc, data, size, g_CrcTable);
#else
  if (g_Crc_Be)
    return FUNC_NAME_BE(crc, data, size, g_CrcTable);
  else
    return FUNC_NAME_LE(crc, data, size, g_CrcTable);
#endif
#endif // Z7_CRC_NUM_TABLES_USE != 1
}


#ifdef Z7_CRC_HW_USE
Z7_NO_INLINE
UInt32 Z7_FASTCALL CrcUpdate(UInt32 crc, const void *data, size_t size)
{
  if (g_Crc_Algo == 0)
    return CrcUpdate_HW(crc, data, size);
  return CrcUpdate_Base(crc, data, size);
}
#endif

#endif // !defined(Z7_CRC_HW_FORCE)



UInt32 Z7_FASTCALL CrcCalc(const void *data, size_t size)
{
  return CrcUpdate(CRC_INIT_VAL, data, size) ^ CRC_INIT_VAL;
}


MY_ALIGN(64)
UInt32 g_CrcTable[256 * Z7_CRC_NUM_TABLES_TOTAL];


void Z7_FASTCALL CrcGenerateTable(void)
{
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
#if defined(Z7_CRC_HW_FORCE)
    g_CrcTable[i] = __crc32b(i, 0);
#else
    #define kCrcPoly 0xEDB88320
    UInt32 r = i;
    unsigned j;
    for (j = 0; j < 8; j++)
      r = (r >> 1) ^ (kCrcPoly & ((UInt32)0 - (r & 1)));
    g_CrcTable[i] = r;
#endif
  }
  for (i = 256; i < 256 * Z7_CRC_NUM_TABLES_USE; i++)
  {
    const UInt32 r = g_CrcTable[(size_t)i - 256];
    g_CrcTable[i] = g_CrcTable[r & 0xFF] ^ (r >> 8);
  }

#if !defined(Z7_CRC_HW_FORCE) && \
    (defined(Z7_CRC_HW_USE) || defined(Z7_CRC_UPDATE_T1_FUNC_NAME) || defined(MY_CPU_BE))

#if Z7_CRC_NUM_TABLES_USE <= 1
    g_Crc_Algo = 1;
#else // Z7_CRC_NUM_TABLES_USE <= 1

#if defined(MY_CPU_LE)
    g_Crc_Algo = Z7_CRC_NUM_TABLES_USE;
#else // !defined(MY_CPU_LE)
  {
#ifndef MY_CPU_BE
    UInt32 k = 0x01020304;
    const Byte *p = (const Byte *)&k;
    if (p[0] == 4 && p[1] == 3)
      g_Crc_Algo = Z7_CRC_NUM_TABLES_USE;
    else if (p[0] != 1 || p[1] != 2)
      g_Crc_Algo = 1;
    else
#endif // MY_CPU_BE
    {
      for (i = 256 * Z7_CRC_NUM_TABLES_TOTAL - 1; i >= 256; i--)
      {
        const UInt32 x = g_CrcTable[(size_t)i - 256];
        g_CrcTable[i] = Z7_BSWAP32(x);
      }
#if defined(Z7_CRC_UPDATE_T1_FUNC_NAME)
      g_Crc_Algo = Z7_CRC_NUM_TABLES_USE;
#endif
#if (!defined(MY_CPU_LE) && !defined(MY_CPU_BE))
      g_Crc_Be = 1;
#endif
    }
  }
#endif  // !defined(MY_CPU_LE)

#ifdef MY_CPU_LE
#ifdef Z7_CRC_HW_USE
  if (CPU_IsSupported_CRC32())
    g_Crc_Algo = 0;
#endif // Z7_CRC_HW_USE
#endif // MY_CPU_LE

#endif // Z7_CRC_NUM_TABLES_USE <= 1
#endif // g_Crc_Algo was declared
}

Z7_CRC_UPDATE_FUNC z7_GetFunc_CrcUpdate(unsigned algo)
{
  if (algo == 0)
    return &CrcUpdate;

#if defined(Z7_CRC_HW_USE)
  if (algo == sizeof(CRC_HW_WORD_TYPE) * 8)
  {
#ifdef Z7_CRC_HW_FORCE
    return &CrcUpdate;
#else
    if (g_Crc_Algo == 0)
      return &CrcUpdate_HW;
#endif
  }
#endif

#ifndef Z7_CRC_HW_FORCE
  if (algo == Z7_CRC_NUM_TABLES_USE)
    return
  #ifdef Z7_CRC_HW_USE
      &CrcUpdate_Base;
  #else
      &CrcUpdate;
  #endif
#endif

  return NULL;
}

#undef kCrcPoly
#undef Z7_CRC_NUM_TABLES_USE
#undef Z7_CRC_NUM_TABLES_TOTAL
#undef CRC_UPDATE_BYTE_2
#undef FUNC_NAME_LE_2
#undef FUNC_NAME_LE_1
#undef FUNC_NAME_LE
#undef FUNC_NAME_BE_2
#undef FUNC_NAME_BE_1
#undef FUNC_NAME_BE

#undef CRC_HW_UNROLL_BYTES
#undef CRC_HW_WORD_FUNC
#undef CRC_HW_WORD_TYPE

/* ================ unit: C/7zCrcOpt.c ================ */
/* 7zCrcOpt.c -- CRC32 calculation (optimized functions)
2023-12-07 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#if !defined(Z7_CRC_NUM_TABLES) || Z7_CRC_NUM_TABLES > 1

// for debug only : define Z7_CRC_DEBUG_BE to test big-endian code in little-endian cpu
// #define Z7_CRC_DEBUG_BE
#ifdef Z7_CRC_DEBUG_BE
#undef MY_CPU_LE
#define MY_CPU_BE
#endif

// the value Z7_CRC_NUM_TABLES_USE must be defined to same value as in 7zCrc.c
#ifdef Z7_CRC_NUM_TABLES
#define Z7_CRC_NUM_TABLES_USE  Z7_CRC_NUM_TABLES
#else
#define Z7_CRC_NUM_TABLES_USE  12
#endif

#if Z7_CRC_NUM_TABLES_USE % 4     || \
    Z7_CRC_NUM_TABLES_USE < 4 * 1 || \
    Z7_CRC_NUM_TABLES_USE > 4 * 6
  #error Stop_Compiling_Bad_Z7_CRC_NUM_TABLES
#endif


#ifndef MY_CPU_BE

#define CRC_UPDATE_BYTE_2(crc, b)  (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

#define Q(n, d) \
    ( (table + ((n) * 4 + 3) * 0x100)[(Byte)(d)] \
    ^ (table + ((n) * 4 + 2) * 0x100)[((d) >> 1 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 1) * 0x100)[((d) >> 2 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 0) * 0x100)[((d) >> 3 * 8)] )

#define R(a)  *((const UInt32 *)(const void *)p + (a))

#define CRC_FUNC_PRE_LE2(step) \
UInt32 Z7_FASTCALL CrcUpdateT ## step (UInt32 v, const void *data, size_t size, const UInt32 *table)

#define CRC_FUNC_PRE_LE(step)   \
        CRC_FUNC_PRE_LE2(step); \
        CRC_FUNC_PRE_LE2(step)

CRC_FUNC_PRE_LE(Z7_CRC_NUM_TABLES_USE)
{
  const Byte *p = (const Byte *)data;
  const Byte *lim;
  for (; size && ((unsigned)(ptrdiff_t)p & (7 - (Z7_CRC_NUM_TABLES_USE & 4))) != 0; size--, p++)
    v = CRC_UPDATE_BYTE_2(v, *p);
  lim = p + size;
  if (size >= Z7_CRC_NUM_TABLES_USE)
  {
    lim -= Z7_CRC_NUM_TABLES_USE;
    do
    {
      v ^= R(0);
      {
#if Z7_CRC_NUM_TABLES_USE == 1 * 4
        v = Q(0, v);
#else
#define U2(r, op) \
        { d = R(r);  x op Q(Z7_CRC_NUM_TABLES_USE / 4 - 1 - (r), d); }
        UInt32 d, x;
        U2(1, =)
#if Z7_CRC_NUM_TABLES_USE >= 3 * 4
#define U(r)  U2(r, ^=)
        U(2)
#if Z7_CRC_NUM_TABLES_USE >= 4 * 4
        U(3)
#if Z7_CRC_NUM_TABLES_USE >= 5 * 4
        U(4)
#if Z7_CRC_NUM_TABLES_USE >= 6 * 4
        U(5)
#if Z7_CRC_NUM_TABLES_USE >= 7 * 4
#error Stop_Compiling_Bad_Z7_CRC_NUM_TABLES
#endif
#endif
#endif
#endif
#endif
#undef U
#undef U2
        v = x ^ Q(Z7_CRC_NUM_TABLES_USE / 4 - 1, v);
#endif
      }
      p += Z7_CRC_NUM_TABLES_USE;
    }
    while (p <= lim);
    lim += Z7_CRC_NUM_TABLES_USE;
  }
  for (; p < lim; p++)
    v = CRC_UPDATE_BYTE_2(v, *p);
  return v;
}

#undef CRC_UPDATE_BYTE_2
#undef R
#undef Q
#undef CRC_FUNC_PRE_LE
#undef CRC_FUNC_PRE_LE2

#endif




#ifndef MY_CPU_LE

#define CRC_UPDATE_BYTE_2_BE(crc, b)  (table[((crc) >> 24) ^ (b)] ^ ((crc) << 8))

#define Q(n, d) \
    ( (table + ((n) * 4 + 0) * 0x100)[((d)) & 0xFF] \
    ^ (table + ((n) * 4 + 1) * 0x100)[((d) >> 1 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 2) * 0x100)[((d) >> 2 * 8) & 0xFF] \
    ^ (table + ((n) * 4 + 3) * 0x100)[((d) >> 3 * 8)] )

#ifdef Z7_CRC_DEBUG_BE
  #define R(a)  GetBe32a((const UInt32 *)(const void *)p + (a))
#else
  #define R(a)         *((const UInt32 *)(const void *)p + (a))
#endif


#define CRC_FUNC_PRE_BE2(step) \
UInt32 Z7_FASTCALL CrcUpdateT1_BeT ## step (UInt32 v, const void *data, size_t size, const UInt32 *table)

#define CRC_FUNC_PRE_BE(step)   \
        CRC_FUNC_PRE_BE2(step); \
        CRC_FUNC_PRE_BE2(step)

CRC_FUNC_PRE_BE(Z7_CRC_NUM_TABLES_USE)
{
  const Byte *p = (const Byte *)data;
  const Byte *lim;
  table += 0x100;
  v = Z7_BSWAP32(v);
  for (; size && ((unsigned)(ptrdiff_t)p & (7 - (Z7_CRC_NUM_TABLES_USE & 4))) != 0; size--, p++)
    v = CRC_UPDATE_BYTE_2_BE(v, *p);
  lim = p + size;
  if (size >= Z7_CRC_NUM_TABLES_USE)
  {
    lim -= Z7_CRC_NUM_TABLES_USE;
    do
    {
      v ^= R(0);
      {
#if Z7_CRC_NUM_TABLES_USE == 1 * 4
        v = Q(0, v);
#else
#define U2(r, op) \
        { d = R(r);  x op Q(Z7_CRC_NUM_TABLES_USE / 4 - 1 - (r), d); }
        UInt32 d, x;
        U2(1, =)
#if Z7_CRC_NUM_TABLES_USE >= 3 * 4
#define U(r)  U2(r, ^=)
        U(2)
#if Z7_CRC_NUM_TABLES_USE >= 4 * 4
        U(3)
#if Z7_CRC_NUM_TABLES_USE >= 5 * 4
        U(4)
#if Z7_CRC_NUM_TABLES_USE >= 6 * 4
        U(5)
#if Z7_CRC_NUM_TABLES_USE >= 7 * 4
#error Stop_Compiling_Bad_Z7_CRC_NUM_TABLES
#endif
#endif
#endif
#endif
#endif
#undef U
#undef U2
        v = x ^ Q(Z7_CRC_NUM_TABLES_USE / 4 - 1, v);
#endif
      }
      p += Z7_CRC_NUM_TABLES_USE;
    }
    while (p <= lim);
    lim += Z7_CRC_NUM_TABLES_USE;
  }
  for (; p < lim; p++)
    v = CRC_UPDATE_BYTE_2_BE(v, *p);
  return Z7_BSWAP32(v);
}

#undef CRC_UPDATE_BYTE_2_BE
#undef R
#undef Q
#undef CRC_FUNC_PRE_BE
#undef CRC_FUNC_PRE_BE2

#endif
#undef Z7_CRC_NUM_TABLES_USE
#endif

/* ================ unit: C/7zStream.c ================ */
/* 7zStream.c -- 7z Stream functions
2023-04-02 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue


SRes SeqInStream_ReadMax(ISeqInStreamPtr stream, void *buf, size_t *processedSize)
{
  size_t size = *processedSize;
  *processedSize = 0;
  while (size != 0)
  {
    size_t cur = size;
    const SRes res = ISeqInStream_Read(stream, buf, &cur);
    *processedSize += cur;
    buf = (void *)((Byte *)buf + cur);
    size -= cur;
    if (res != SZ_OK)
      return res;
    if (cur == 0)
      return SZ_OK;
  }
  return SZ_OK;
}

/*
SRes SeqInStream_Read2(ISeqInStreamPtr stream, void *buf, size_t size, SRes errorType)
{
  while (size != 0)
  {
    size_t processed = size;
    RINOK(ISeqInStream_Read(stream, buf, &processed))
    if (processed == 0)
      return errorType;
    buf = (void *)((Byte *)buf + processed);
    size -= processed;
  }
  return SZ_OK;
}

SRes SeqInStream_Read(ISeqInStreamPtr stream, void *buf, size_t size)
{
  return SeqInStream_Read2(stream, buf, size, SZ_ERROR_INPUT_EOF);
}
*/


SRes SeqInStream_ReadByte(ISeqInStreamPtr stream, Byte *buf)
{
  size_t processed = 1;
  RINOK(ISeqInStream_Read(stream, buf, &processed))
  return (processed == 1) ? SZ_OK : SZ_ERROR_INPUT_EOF;
}



SRes LookInStream_SeekTo(ILookInStreamPtr stream, UInt64 offset)
{
  Int64 t = (Int64)offset;
  return ILookInStream_Seek(stream, &t, SZ_SEEK_SET);
}

SRes LookInStream_LookRead(ILookInStreamPtr stream, void *buf, size_t *size)
{
  const void *lookBuf;
  if (*size == 0)
    return SZ_OK;
  RINOK(ILookInStream_Look(stream, &lookBuf, size))
  memcpy(buf, lookBuf, *size);
  return ILookInStream_Skip(stream, *size);
}

SRes LookInStream_Read2(ILookInStreamPtr stream, void *buf, size_t size, SRes errorType)
{
  while (size != 0)
  {
    size_t processed = size;
    RINOK(ILookInStream_Read(stream, buf, &processed))
    if (processed == 0)
      return errorType;
    buf = (void *)((Byte *)buf + processed);
    size -= processed;
  }
  return SZ_OK;
}

SRes LookInStream_Read(ILookInStreamPtr stream, void *buf, size_t size)
{
  return LookInStream_Read2(stream, buf, size, SZ_ERROR_INPUT_EOF);
}



#define GET_LookToRead2  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CLookToRead2)

static SRes LookToRead2_Look_Lookahead(ILookInStreamPtr pp, const void **buf, size_t *size)
{
  SRes res = SZ_OK;
  GET_LookToRead2
  size_t size2 = p->size - p->pos;
  if (size2 == 0 && *size != 0)
  {
    p->pos = 0;
    p->size = 0;
    size2 = p->bufSize;
    res = ISeekInStream_Read(p->realStream, p->buf, &size2);
    p->size = size2;
  }
  if (*size > size2)
    *size = size2;
  *buf = p->buf + p->pos;
  return res;
}

static SRes LookToRead2_Look_Exact(ILookInStreamPtr pp, const void **buf, size_t *size)
{
  SRes res = SZ_OK;
  GET_LookToRead2
  size_t size2 = p->size - p->pos;
  if (size2 == 0 && *size != 0)
  {
    p->pos = 0;
    p->size = 0;
    if (*size > p->bufSize)
      *size = p->bufSize;
    res = ISeekInStream_Read(p->realStream, p->buf, size);
    size2 = p->size = *size;
  }
  if (*size > size2)
    *size = size2;
  *buf = p->buf + p->pos;
  return res;
}

static SRes LookToRead2_Skip(ILookInStreamPtr pp, size_t offset)
{
  GET_LookToRead2
  p->pos += offset;
  return SZ_OK;
}

static SRes LookToRead2_Read(ILookInStreamPtr pp, void *buf, size_t *size)
{
  GET_LookToRead2
  size_t rem = p->size - p->pos;
  if (rem == 0)
    return ISeekInStream_Read(p->realStream, buf, size);
  if (rem > *size)
    rem = *size;
  memcpy(buf, p->buf + p->pos, rem);
  p->pos += rem;
  *size = rem;
  return SZ_OK;
}

static SRes LookToRead2_Seek(ILookInStreamPtr pp, Int64 *pos, ESzSeek origin)
{
  GET_LookToRead2
  p->pos = p->size = 0;
  return ISeekInStream_Seek(p->realStream, pos, origin);
}

void LookToRead2_CreateVTable(CLookToRead2 *p, int lookahead)
{
  p->vt.Look = lookahead ?
      LookToRead2_Look_Lookahead :
      LookToRead2_Look_Exact;
  p->vt.Skip = LookToRead2_Skip;
  p->vt.Read = LookToRead2_Read;
  p->vt.Seek = LookToRead2_Seek;
}



static SRes SecToLook_Read(ISeqInStreamPtr pp, void *buf, size_t *size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSecToLook)
  return LookInStream_LookRead(p->realStream, buf, size);
}

void SecToLook_CreateVTable(CSecToLook *p)
{
  p->vt.Read = SecToLook_Read;
}

static SRes SecToRead_Read(ISeqInStreamPtr pp, void *buf, size_t *size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSecToRead)
  return ILookInStream_Read(p->realStream, buf, size);
}

void SecToRead_CreateVTable(CSecToRead *p)
{
  p->vt.Read = SecToRead_Read;
}

/* ================ unit: C/Aes.c ================ */
/* Aes.c -- AES encryption / decryption
2024-03-01 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

AES_CODE_FUNC g_AesCbc_Decode;
#ifndef Z7_SFX
AES_CODE_FUNC g_AesCbc_Encode;
AES_CODE_FUNC g_AesCtr_Code;
UInt32 g_Aes_SupportedFunctions_Flags;
#endif

MY_ALIGN(64)
static UInt32 T[256 * 4];
MY_ALIGN(64)
static const Byte Sbox[256] = {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};


MY_ALIGN(64)
static UInt32 D[256 * 4];
MY_ALIGN(64)
static Byte InvS[256];

#define xtime(x) ((((x) << 1) ^ (((x) & 0x80) != 0 ? 0x1B : 0)) & 0xFF)

#define Ui32(a0, a1, a2, a3) ((UInt32)(a0) | ((UInt32)(a1) << 8) | ((UInt32)(a2) << 16) | ((UInt32)(a3) << 24))

#define gb0(x) ( (x)          & 0xFF)
#define gb1(x) (((x) >> ( 8)) & 0xFF)
#define gb2(x) (((x) >> (16)) & 0xFF)
#define gb3(x) (((x) >> (24)))

#define gb(n, x) gb ## n(x)

#define TT(x) (T + (x << 8))
#define DD(x) (D + (x << 8))


// #define Z7_SHOW_AES_STATUS

#ifdef MY_CPU_X86_OR_AMD64

  #if defined(__INTEL_COMPILER)
    #if (__INTEL_COMPILER >= 1110)
      #define USE_HW_AES
      #if (__INTEL_COMPILER >= 1900)
        #define USE_HW_VAES
      #endif
    #endif
  #elif defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30800) \
     || defined(Z7_GCC_VERSION)   && (Z7_GCC_VERSION   >= 40400)
    #define USE_HW_AES
      #if defined(__clang__) && (__clang_major__ >= 8) \
          || defined(__GNUC__) && (__GNUC__ >= 8)
        #define USE_HW_VAES
      #endif
  #elif defined(_MSC_VER)
    #define USE_HW_AES
    #define USE_HW_VAES
  #endif

#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE)
  
  #if   defined(__ARM_FEATURE_AES) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define USE_HW_AES
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
      #define USE_HW_AES
    #endif
    #endif
    #endif
  #endif
#endif

#ifdef USE_HW_AES
// #pragma message("=== Aes.c USE_HW_AES === ")
#ifdef Z7_SHOW_AES_STATUS
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif
#endif


void AesGenTables(void)
{
  unsigned i;
  for (i = 0; i < 256; i++)
    InvS[Sbox[i]] = (Byte)i;
  
  for (i = 0; i < 256; i++)
  {
    {
      const UInt32 a1 = Sbox[i];
      const UInt32 a2 = xtime(a1);
      const UInt32 a3 = a2 ^ a1;
      TT(0)[i] = Ui32(a2, a1, a1, a3);
      TT(1)[i] = Ui32(a3, a2, a1, a1);
      TT(2)[i] = Ui32(a1, a3, a2, a1);
      TT(3)[i] = Ui32(a1, a1, a3, a2);
    }
    {
      const UInt32 a1 = InvS[i];
      const UInt32 a2 = xtime(a1);
      const UInt32 a4 = xtime(a2);
      const UInt32 a8 = xtime(a4);
      const UInt32 a9 = a8 ^ a1;
      const UInt32 aB = a8 ^ a2 ^ a1;
      const UInt32 aD = a8 ^ a4 ^ a1;
      const UInt32 aE = a8 ^ a4 ^ a2;
      DD(0)[i] = Ui32(aE, a9, aD, aB);
      DD(1)[i] = Ui32(aB, aE, a9, aD);
      DD(2)[i] = Ui32(aD, aB, aE, a9);
      DD(3)[i] = Ui32(a9, aD, aB, aE);
    }
  }
  
  {
  AES_CODE_FUNC d = AesCbc_Decode;
  #ifndef Z7_SFX
  AES_CODE_FUNC e = AesCbc_Encode;
  AES_CODE_FUNC c = AesCtr_Code;
  UInt32 flags = 0;
  #endif
  
  #ifdef USE_HW_AES
  if (CPU_IsSupported_AES())
  {
    // #pragma message ("AES HW")
    PRF(printf("\n===AES HW\n"));
    d = AesCbc_Decode_HW;

    #ifndef Z7_SFX
    e = AesCbc_Encode_HW;
    c = AesCtr_Code_HW;
    flags = k_Aes_SupportedFunctions_HW;
    #endif

    #ifdef MY_CPU_X86_OR_AMD64
    #ifdef USE_HW_VAES
    if (CPU_IsSupported_VAES_AVX2())
    {
      PRF(printf("\n===vaes avx2\n"));
      d = AesCbc_Decode_HW_256;
      #ifndef Z7_SFX
      c = AesCtr_Code_HW_256;
      flags |= k_Aes_SupportedFunctions_HW_256;
      #endif
    }
    #endif
    #endif
  }
  #endif

  g_AesCbc_Decode = d;
  #ifndef Z7_SFX
  g_AesCbc_Encode = e;
  g_AesCtr_Code = c;
  g_Aes_SupportedFunctions_Flags = flags;
  #endif
  }
}


#define HT(i, x, s) TT(x)[gb(x, s[(i + x) & 3])]

#define HT4(m, i, s, p) m[i] = \
    HT(i, 0, s) ^ \
    HT(i, 1, s) ^ \
    HT(i, 2, s) ^ \
    HT(i, 3, s) ^ w[p + i]

#define HT16(m, s, p) \
    HT4(m, 0, s, p); \
    HT4(m, 1, s, p); \
    HT4(m, 2, s, p); \
    HT4(m, 3, s, p); \

#define FT(i, x) Sbox[gb(x, m[(i + x) & 3])]
#define FT4(i) dest[i] = Ui32(FT(i, 0), FT(i, 1), FT(i, 2), FT(i, 3)) ^ w[i];


#define HD(i, x, s) DD(x)[gb(x, s[(i - x) & 3])]

#define HD4(m, i, s, p) m[i] = \
    HD(i, 0, s) ^ \
    HD(i, 1, s) ^ \
    HD(i, 2, s) ^ \
    HD(i, 3, s) ^ w[p + i];

#define HD16(m, s, p) \
    HD4(m, 0, s, p); \
    HD4(m, 1, s, p); \
    HD4(m, 2, s, p); \
    HD4(m, 3, s, p); \

#define FD(i, x) InvS[gb(x, m[(i - x) & 3])]
#define FD4(i) dest[i] = Ui32(FD(i, 0), FD(i, 1), FD(i, 2), FD(i, 3)) ^ w[i];

void Z7_FASTCALL Aes_SetKey_Enc(UInt32 *w, const Byte *key, unsigned keySize)
{
  unsigned i, m;
  const UInt32 *wLim;
  UInt32 t;
  UInt32 rcon = 1;
  
  keySize /= 4;
  w[0] = ((UInt32)keySize / 2) + 3;
  w += 4;

  for (i = 0; i < keySize; i++, key += 4)
    w[i] = GetUi32(key);

  t = w[(size_t)keySize - 1];
  wLim = w + (size_t)keySize * 3 + 28;
  m = 0;
  do
  {
    if (m == 0)
    {
      t = Ui32(Sbox[gb1(t)] ^ rcon, Sbox[gb2(t)], Sbox[gb3(t)], Sbox[gb0(t)]);
      rcon <<= 1;
      if (rcon & 0x100)
        rcon = 0x1b;
      m = keySize;
    }
    else if (m == 4 && keySize > 6)
      t = Ui32(Sbox[gb0(t)], Sbox[gb1(t)], Sbox[gb2(t)], Sbox[gb3(t)]);
    m--;
    t ^= w[0];
    w[keySize] = t;
  }
  while (++w != wLim);
}

void Z7_FASTCALL Aes_SetKey_Dec(UInt32 *w, const Byte *key, unsigned keySize)
{
  unsigned i, num;
  Aes_SetKey_Enc(w, key, keySize);
  num = keySize + 20;
  w += 8;
  for (i = 0; i < num; i++)
  {
    UInt32 r = w[i];
    w[i] =
      DD(0)[Sbox[gb0(r)]] ^
      DD(1)[Sbox[gb1(r)]] ^
      DD(2)[Sbox[gb2(r)]] ^
      DD(3)[Sbox[gb3(r)]];
  }
}

/* Aes_Encode and Aes_Decode functions work with little-endian words.
  src and dest are pointers to 4 UInt32 words.
  src and dest can point to same block */

// Z7_FORCE_INLINE
static void Aes_Encode(const UInt32 *w, UInt32 *dest, const UInt32 *src)
{
  UInt32 s[4];
  UInt32 m[4];
  UInt32 numRounds2 = w[0];
  w += 4;
  s[0] = src[0] ^ w[0];
  s[1] = src[1] ^ w[1];
  s[2] = src[2] ^ w[2];
  s[3] = src[3] ^ w[3];
  w += 4;
  for (;;)
  {
    HT16(m, s, 0)
    if (--numRounds2 == 0)
      break;
    HT16(s, m, 4)
    w += 8;
  }
  w += 4;
  FT4(0)
  FT4(1)
  FT4(2)
  FT4(3)
}

Z7_FORCE_INLINE
static void Aes_Decode(const UInt32 *w, UInt32 *dest, const UInt32 *src)
{
  UInt32 s[4];
  UInt32 m[4];
  UInt32 numRounds2 = w[0];
  w += 4 + numRounds2 * 8;
  s[0] = src[0] ^ w[0];
  s[1] = src[1] ^ w[1];
  s[2] = src[2] ^ w[2];
  s[3] = src[3] ^ w[3];
  for (;;)
  {
    w -= 8;
    HD16(m, s, 4)
    if (--numRounds2 == 0)
      break;
    HD16(s, m, 0)
  }
  FD4(0)
  FD4(1)
  FD4(2)
  FD4(3)
}

void AesCbc_Init(UInt32 *p, const Byte *iv)
{
  unsigned i;
  for (i = 0; i < 4; i++)
    p[i] = GetUi32(iv + i * 4);
}

void Z7_FASTCALL AesCbc_Encode(UInt32 *p, Byte *data, size_t numBlocks)
{
  for (; numBlocks != 0; numBlocks--, data += AES_BLOCK_SIZE)
  {
    p[0] ^= GetUi32(data);
    p[1] ^= GetUi32(data + 4);
    p[2] ^= GetUi32(data + 8);
    p[3] ^= GetUi32(data + 12);
    
    Aes_Encode(p + 4, p, p);
    
    SetUi32(data,      p[0])
    SetUi32(data + 4,  p[1])
    SetUi32(data + 8,  p[2])
    SetUi32(data + 12, p[3])
  }
}

void Z7_FASTCALL AesCbc_Decode(UInt32 *p, Byte *data, size_t numBlocks)
{
  UInt32 in[4], out[4];
  for (; numBlocks != 0; numBlocks--, data += AES_BLOCK_SIZE)
  {
    in[0] = GetUi32(data);
    in[1] = GetUi32(data + 4);
    in[2] = GetUi32(data + 8);
    in[3] = GetUi32(data + 12);

    Aes_Decode(p + 4, out, in);

    SetUi32(data,      p[0] ^ out[0])
    SetUi32(data + 4,  p[1] ^ out[1])
    SetUi32(data + 8,  p[2] ^ out[2])
    SetUi32(data + 12, p[3] ^ out[3])
    
    p[0] = in[0];
    p[1] = in[1];
    p[2] = in[2];
    p[3] = in[3];
  }
}

void Z7_FASTCALL AesCtr_Code(UInt32 *p, Byte *data, size_t numBlocks)
{
  for (; numBlocks != 0; numBlocks--)
  {
    UInt32 temp[4];
    unsigned i;

    if (++p[0] == 0)
      p[1]++;
    
    Aes_Encode(p + 4, temp, p);
    
    for (i = 0; i < 4; i++, data += 4)
    {
      const UInt32 t = temp[i];

      #ifdef MY_CPU_LE_UNALIGN
        *((UInt32 *)(void *)data) ^= t;
      #else
        data[0] = (Byte)(data[0] ^ (t & 0xFF));
        data[1] = (Byte)(data[1] ^ ((t >> 8) & 0xFF));
        data[2] = (Byte)(data[2] ^ ((t >> 16) & 0xFF));
        data[3] = (Byte)(data[3] ^ ((t >> 24)));
      #endif
    }
  }
}

#undef xtime
#undef Ui32
#undef gb0
#undef gb1
#undef gb2
#undef gb3
#undef gb
#undef TT
#undef DD
#undef USE_HW_AES
#undef PRF

/* ================ unit: C/AesOpt.c ================ */
/* AesOpt.c -- AES optimized code for x86 AES hardware instructions
Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef MY_CPU_X86_OR_AMD64

  #if defined(__INTEL_COMPILER)
    #if (__INTEL_COMPILER >= 1110)
      #define USE_INTEL_AES
      #if (__INTEL_COMPILER >= 1900)
        #define USE_INTEL_VAES
      #endif
    #endif
  #elif defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30800) \
     || defined(Z7_GCC_VERSION)   && (Z7_GCC_VERSION   >= 40400)
        #define USE_INTEL_AES
        #if !defined(__AES__)
          #define ATTRIB_AES __attribute__((__target__("aes")))
        #endif
      #if defined(__clang__) && (__clang_major__ >= 8) \
          || defined(__GNUC__) && (__GNUC__ >= 8)
        #define USE_INTEL_VAES
        #if !defined(__AES__) || !defined(__VAES__) || !defined(__AVX__) || !defined(__AVX2__)
          #define ATTRIB_VAES __attribute__((__target__("aes,vaes,avx,avx2")))
        #endif
      #endif
  #elif defined(_MSC_VER)
    #if (_MSC_VER > 1500) || (_MSC_FULL_VER >= 150030729)
      #define USE_INTEL_AES
      #if (_MSC_VER >= 1910)
        #define USE_INTEL_VAES
      #endif
    #endif
    #ifndef USE_INTEL_AES
      #define Z7_USE_AES_HW_STUB
    #endif
    #ifndef USE_INTEL_VAES
      #define Z7_USE_VAES_HW_STUB
    #endif
  #endif

    #ifndef USE_INTEL_AES
      // #define Z7_USE_AES_HW_STUB // for debug
    #endif
    #ifndef USE_INTEL_VAES
      // #define Z7_USE_VAES_HW_STUB // for debug
    #endif


#ifdef USE_INTEL_AES

#include <wmmintrin.h>

#if !defined(USE_INTEL_VAES) && defined(Z7_USE_VAES_HW_STUB)
#define AES_TYPE_keys UInt32
#define AES_TYPE_data Byte
// #define AES_TYPE_keys __m128i
// #define AES_TYPE_data __m128i
#endif

#ifndef ATTRIB_AES
  #define ATTRIB_AES
#endif

#define AES_FUNC_START(name) \
    void Z7_FASTCALL name(UInt32 *ivAes, Byte *data8, size_t numBlocks)
    // void Z7_FASTCALL name(__m128i *p, __m128i *data, size_t numBlocks)

#define AES_FUNC_START2(name) \
AES_FUNC_START (name); \
ATTRIB_AES \
AES_FUNC_START (name)

#define MM_OP(op, dest, src)  dest = op(dest, src);
#define MM_OP_m(op, src)      MM_OP(op, m, src)

#define MM_XOR( dest, src)    MM_OP(_mm_xor_si128,    dest, src)

#if 1
// use aligned SSE load/store for data.
// It is required for our Aes functions, that data is aligned for 16-bytes.
// So we can use this branch of code.
// and compiler can use fused load-op SSE instructions:
//   xorps xmm0, XMMWORD PTR [rdx]
#define LOAD_128(pp)        (*(__m128i *)(void *)(pp))
#define STORE_128(pp, _v)    *(__m128i *)(void *)(pp) = _v
// use aligned SSE load/store for data. Alternative code with direct access
// #define LOAD_128(pp)        _mm_load_si128(pp)
// #define STORE_128(pp, _v)   _mm_store_si128(pp, _v)
#else
// use unaligned load/store for data: movdqu XMMWORD PTR [rdx]
#define LOAD_128(pp)        _mm_loadu_si128(pp)
#define STORE_128(pp, _v)   _mm_storeu_si128(pp, _v)
#endif

AES_FUNC_START2 (AesCbc_Encode_HW)
{
  if (numBlocks == 0)
    return;
  {
  __m128i *p = (__m128i *)(void *)ivAes;
  __m128i *data = (__m128i *)(void *)data8;
  __m128i m = *p;
  const __m128i k0 = p[2];
  const __m128i k1 = p[3];
  const UInt32 numRounds2 = *(const UInt32 *)(p + 1) - 1;
  do
  {
    UInt32 r = numRounds2;
    const __m128i *w = p + 4;
    __m128i temp = LOAD_128(data);
    MM_XOR (temp, k0)
    MM_XOR (m, temp)
    MM_OP_m (_mm_aesenc_si128, k1)
    do
    {
      MM_OP_m (_mm_aesenc_si128, w[0])
      MM_OP_m (_mm_aesenc_si128, w[1])
      w += 2;
    }
    while (--r);
    MM_OP_m (_mm_aesenclast_si128, w[0])
    STORE_128(data, m);
    data++;
  }
  while (--numBlocks);
  *p = m;
  }
}


#define WOP_1(op)
#define WOP_2(op)   WOP_1 (op)  op (m1, 1)
#define WOP_3(op)   WOP_2 (op)  op (m2, 2)
#define WOP_4(op)   WOP_3 (op)  op (m3, 3)
#ifdef MY_CPU_AMD64
#define WOP_5(op)   WOP_4 (op)  op (m4, 4)
#define WOP_6(op)   WOP_5 (op)  op (m5, 5)
#define WOP_7(op)   WOP_6 (op)  op (m6, 6)
#define WOP_8(op)   WOP_7 (op)  op (m7, 7)
#endif
/*
#define WOP_9(op)   WOP_8 (op)  op (m8, 8);
#define WOP_10(op)  WOP_9 (op)  op (m9, 9);
#define WOP_11(op)  WOP_10(op)  op (m10, 10);
#define WOP_12(op)  WOP_11(op)  op (m11, 11);
#define WOP_13(op)  WOP_12(op)  op (m12, 12);
#define WOP_14(op)  WOP_13(op)  op (m13, 13);
*/

#ifdef MY_CPU_AMD64
  #define NUM_WAYS      8
  #define WOP_M1    WOP_8
#else
  #define NUM_WAYS      4
  #define WOP_M1    WOP_4
#endif

#define WOP(op)  op (m0, 0)  WOP_M1(op)

#define DECLARE_VAR(reg, ii)  __m128i reg;
#define LOAD_data_ii(ii)      LOAD_128(data + (ii))
#define LOAD_data(  reg, ii)  reg = LOAD_data_ii(ii);
#define STORE_data( reg, ii)  STORE_128(data + (ii), reg);
#if (NUM_WAYS > 1)
#define XOR_data_M1(reg, ii)  MM_XOR (reg, LOAD_128(data + (ii- 1)))
#endif

#define MM_OP_key(op, reg)  MM_OP(op, reg, key);

#define AES_DEC(      reg, ii)   MM_OP_key (_mm_aesdec_si128,     reg)
#define AES_DEC_LAST( reg, ii)   MM_OP_key (_mm_aesdeclast_si128, reg)
#define AES_ENC(      reg, ii)   MM_OP_key (_mm_aesenc_si128,     reg)
#define AES_ENC_LAST( reg, ii)   MM_OP_key (_mm_aesenclast_si128, reg)
#define AES_XOR(      reg, ii)   MM_OP_key (_mm_xor_si128,        reg)

#define CTR_START(reg, ii)  MM_OP (_mm_add_epi64, ctr, one)  reg = ctr;
#define CTR_END(  reg, ii)  STORE_128(data + (ii), _mm_xor_si128(reg, \
                            LOAD_128 (data + (ii))));
#define WOP_KEY(op, n) { \
    const __m128i key = w[n]; \
    WOP(op) }

#define WIDE_LOOP_START  \
    dataEnd = data + numBlocks;  \
    if (numBlocks >= NUM_WAYS)  \
    { dataEnd -= NUM_WAYS; do {  \

#define WIDE_LOOP_END  \
    data += NUM_WAYS;  \
    } while (data <= dataEnd);  \
    dataEnd += NUM_WAYS; }  \

#define SINGLE_LOOP  \
    for (; data < dataEnd; data++)



#ifdef USE_INTEL_VAES

#define AVX_XOR(dest, src)    MM_OP(_mm256_xor_si256, dest, src)
#define AVX_DECLARE_VAR(reg, ii)  __m256i reg;

#if 1
// use unaligned AVX load/store for data.
// It is required for our Aes functions, that data is aligned for 16-bytes.
// But we need 32-bytes reading.
// So we use intrinsics for unaligned AVX load/store.
// notes for _mm256_storeu_si256:
// msvc2022: uses vmovdqu and keeps the order of instruction sequence.
// new gcc11 uses vmovdqu
// old gcc9 could use pair of instructions:
//   vmovups        %xmm7, -224(%rax)
//   vextracti128   $0x1, %ymm7, -208(%rax)
#define AVX_LOAD(p)         _mm256_loadu_si256((const __m256i *)(const void *)(p))
#define AVX_STORE(p, _v)    _mm256_storeu_si256((__m256i *)(void *)(p), _v);
#else
// use aligned AVX load/store for data.
// for debug: we can use this branch, if we are sure that data is aligned for 32-bytes.
// msvc2022 uses vmovdqu still
// gcc      uses vmovdqa (that requires 32-bytes alignment)
#define AVX_LOAD(p)         (*(const __m256i *)(const void *)(p))
#define AVX_STORE(p, _v)    (*(__m256i *)(void *)(p)) = _v;
#endif

#define AVX_LOAD_data(  reg, ii)  reg = AVX_LOAD((const __m256i *)(const void *)data + (ii));
#define AVX_STORE_data( reg, ii)  AVX_STORE((__m256i *)(void *)data + (ii), reg)
/*
AVX_XOR_data_M1() needs unaligned memory load, even if (data)
is aligned for 256-bits, because we read 32-bytes chunk that
crosses (data) position: from (data - 16bytes) to (data + 16bytes).
*/
#define AVX_XOR_data_M1(reg, ii)  AVX_XOR (reg, _mm256_loadu_si256((const __m256i *)(const void *)(data - 1) + (ii)))

#define AVX_AES_DEC(      reg, ii)   MM_OP_key (_mm256_aesdec_epi128,     reg)
#define AVX_AES_DEC_LAST( reg, ii)   MM_OP_key (_mm256_aesdeclast_epi128, reg)
#define AVX_AES_ENC(      reg, ii)   MM_OP_key (_mm256_aesenc_epi128,     reg)
#define AVX_AES_ENC_LAST( reg, ii)   MM_OP_key (_mm256_aesenclast_epi128, reg)
#define AVX_AES_XOR(      reg, ii)   MM_OP_key (_mm256_xor_si256,         reg)
#define AVX_CTR_START(reg, ii)  \
    MM_OP (_mm256_add_epi64, ctr2, two) \
    reg = _mm256_xor_si256(ctr2, key);

#define AVX_CTR_END(reg, ii)  \
    AVX_STORE((__m256i *)(void *)data + (ii), _mm256_xor_si256(reg, \
    AVX_LOAD ((__m256i *)(void *)data + (ii))));

#define AVX_WOP_KEY(op, n) { \
    const __m256i key = w[n]; \
    WOP(op) }

#define NUM_AES_KEYS_MAX 15

#define WIDE_LOOP_START_AVX(OP)  \
    dataEnd = data + numBlocks;  \
    if (numBlocks >= NUM_WAYS * 2)  \
    { __m256i keys[NUM_AES_KEYS_MAX];  \
      OP  \
      { UInt32 ii; for (ii = 0; ii < numRounds; ii++)  \
        keys[ii] = _mm256_broadcastsi128_si256(p[ii]); }  \
      dataEnd -= NUM_WAYS * 2; \
      do {  \

#define WIDE_LOOP_END_AVX(OP)  \
        data += NUM_WAYS * 2;  \
      } while (data <= dataEnd);  \
      dataEnd += NUM_WAYS * 2;  \
      OP  \
      _mm256_zeroupper();  \
    }  \

/* MSVC for x86: If we don't call _mm256_zeroupper(), and -arch:IA32 is not specified,
   MSVC still can insert vzeroupper instruction. */

#endif



AES_FUNC_START2 (AesCbc_Decode_HW)
{
  __m128i *p = (__m128i *)(void *)ivAes;
  __m128i *data = (__m128i *)(void *)data8;
  __m128i iv = *p;
  const __m128i * const wStart = p + (size_t)*(const UInt32 *)(p + 1) * 2 + 2 - 1;
  const __m128i *dataEnd;
  p += 2;
  
  WIDE_LOOP_START
  {
    const __m128i *w = wStart;
    WOP (DECLARE_VAR)
    WOP (LOAD_data)
    WOP_KEY (AES_XOR, 1)
    do
    {
      WOP_KEY (AES_DEC, 0)

      w--;
    }
    while (w != p);
    WOP_KEY (AES_DEC_LAST, 0)

    MM_XOR (m0, iv)
    WOP_M1 (XOR_data_M1)
    LOAD_data(iv, NUM_WAYS - 1)
    WOP (STORE_data)
  }
  WIDE_LOOP_END

  SINGLE_LOOP
  {
    const __m128i *w = wStart - 1;
    __m128i m = _mm_xor_si128 (w[2], LOAD_data_ii(0));
    
    do
    {
      MM_OP_m (_mm_aesdec_si128, w[1])
      MM_OP_m (_mm_aesdec_si128, w[0])
      w -= 2;
    }
    while (w != p);
    MM_OP_m (_mm_aesdec_si128,     w[1])
    MM_OP_m (_mm_aesdeclast_si128, w[0])
    MM_XOR (m, iv)
    LOAD_data(iv, 0)
    STORE_data(m, 0)
  }
  
  p[-2] = iv;
}


AES_FUNC_START2 (AesCtr_Code_HW)
{
  __m128i *p = (__m128i *)(void *)ivAes;
  __m128i *data = (__m128i *)(void *)data8;
  __m128i ctr = *p;
  const UInt32 numRoundsMinus2 = *(const UInt32 *)(p + 1) * 2 - 1;
  const __m128i *dataEnd;
  const __m128i one = _mm_cvtsi32_si128(1);

  p += 2;
  
  WIDE_LOOP_START
  {
    const __m128i *w = p;
    UInt32 r = numRoundsMinus2;
    WOP (DECLARE_VAR)
    WOP (CTR_START)
    WOP_KEY (AES_XOR, 0)
    w += 1;
    do
    {
      WOP_KEY (AES_ENC, 0)
      w += 1;
    }
    while (--r);
    WOP_KEY (AES_ENC_LAST, 0)
    WOP (CTR_END)
  }
  WIDE_LOOP_END

  SINGLE_LOOP
  {
    UInt32 numRounds2 = *(const UInt32 *)(p - 2 + 1) - 1;
    const __m128i *w = p;
    __m128i m;
    MM_OP (_mm_add_epi64, ctr, one)
    m = _mm_xor_si128 (ctr, p[0]);
    w += 1;
    do
    {
      MM_OP_m (_mm_aesenc_si128, w[0])
      MM_OP_m (_mm_aesenc_si128, w[1])
      w += 2;
    }
    while (--numRounds2);
    MM_OP_m (_mm_aesenc_si128,     w[0])
    MM_OP_m (_mm_aesenclast_si128, w[1])
    CTR_END (m, 0)
  }
  
  p[-2] = ctr;
}



#ifdef USE_INTEL_VAES

/*
GCC before 2013-Jun:
  <immintrin.h>:
    #ifdef __AVX__
     #include <avxintrin.h>
    #endif
GCC after 2013-Jun:
  <immintrin.h>:
    #include <avxintrin.h>
CLANG 3.8+:
{
  <immintrin.h>:
    #if !defined(_MSC_VER) || defined(__AVX__)
      #include <avxintrin.h>
    #endif

  if (the compiler is clang for Windows and if global arch is not set for __AVX__)
    [ if (defined(_MSC_VER) && !defined(__AVX__)) ]
  {
    <immintrin.h> doesn't include <avxintrin.h>
    and we have 2 ways to fix it:
      1) we can define required __AVX__ before <immintrin.h>
      or
      2) we can include <avxintrin.h> after <immintrin.h>
  }
}

If we include <avxintrin.h> manually for GCC/CLANG, it's
required that <immintrin.h> must be included before <avxintrin.h>.
*/

/*
#if defined(__clang__) && defined(_MSC_VER)
#define __AVX__
#define __AVX2__
#define __VAES__
#endif
*/

#include <immintrin.h>
#if defined(__clang__) && defined(_MSC_VER)
  #if !defined(__AVX__)
    #include <avxintrin.h>
  #endif
  #if !defined(__AVX2__)
    #include <avx2intrin.h>
  #endif
  #if !defined(__VAES__)
    #include <vaesintrin.h>
  #endif
#endif  // __clang__ && _MSC_VER

#ifndef ATTRIB_VAES
  #define ATTRIB_VAES
#endif

#define VAES_FUNC_START2(name) \
AES_FUNC_START (name); \
ATTRIB_VAES \
AES_FUNC_START (name)

VAES_FUNC_START2 (AesCbc_Decode_HW_256)
{
  __m128i *p = (__m128i *)(void *)ivAes;
  __m128i *data = (__m128i *)(void *)data8;
  __m128i iv = *p;
  const __m128i *dataEnd;
  const UInt32 numRounds = *(const UInt32 *)(p + 1) * 2 + 1;
  p += 2;
  
  WIDE_LOOP_START_AVX(;)
  {
    const __m256i *w = keys + numRounds - 2;
    
    WOP (AVX_DECLARE_VAR)
    WOP (AVX_LOAD_data)
    AVX_WOP_KEY (AVX_AES_XOR, 1)

    do
    {
      AVX_WOP_KEY (AVX_AES_DEC, 0)
      w--;
    }
    while (w != keys);
    AVX_WOP_KEY (AVX_AES_DEC_LAST, 0)

    AVX_XOR (m0, _mm256_setr_m128i(iv, LOAD_data_ii(0)))
    WOP_M1 (AVX_XOR_data_M1)
    LOAD_data (iv, NUM_WAYS * 2 - 1)
    WOP (AVX_STORE_data)
  }
  WIDE_LOOP_END_AVX(;)

  SINGLE_LOOP
  {
    const __m128i *w = p - 2 + (size_t)*(const UInt32 *)(p + 1 - 2) * 2;
    __m128i m = _mm_xor_si128 (w[2], LOAD_data_ii(0));
    do
    {
      MM_OP_m (_mm_aesdec_si128, w[1])
      MM_OP_m (_mm_aesdec_si128, w[0])
      w -= 2;
    }
    while (w != p);
    MM_OP_m (_mm_aesdec_si128,     w[1])
    MM_OP_m (_mm_aesdeclast_si128, w[0])

    MM_XOR (m, iv)
    LOAD_data(iv, 0)
    STORE_data(m, 0)
  }
  
  p[-2] = iv;
}


/*
SSE2: _mm_cvtsi32_si128 : movd
AVX:  _mm256_setr_m128i            : vinsertf128
AVX2: _mm256_add_epi64             : vpaddq ymm, ymm, ymm
      _mm256_extracti128_si256     : vextracti128
      _mm256_broadcastsi128_si256  : vbroadcasti128
*/

#define AVX_CTR_LOOP_START  \
    ctr2 = _mm256_setr_m128i(_mm_sub_epi64(ctr, one), ctr); \
    two = _mm256_setr_m128i(one, one); \
    two = _mm256_add_epi64(two, two); \

// two = _mm256_setr_epi64x(2, 0, 2, 0);
  
#define AVX_CTR_LOOP_ENC  \
    ctr = _mm256_extracti128_si256 (ctr2, 1); \
 
VAES_FUNC_START2 (AesCtr_Code_HW_256)
{
  __m128i *p = (__m128i *)(void *)ivAes;
  __m128i *data = (__m128i *)(void *)data8;
  __m128i ctr = *p;
  const UInt32 numRounds = *(const UInt32 *)(p + 1) * 2 + 1;
  const __m128i *dataEnd;
  const __m128i one = _mm_cvtsi32_si128(1);
  __m256i ctr2, two;
  p += 2;
  
  WIDE_LOOP_START_AVX (AVX_CTR_LOOP_START)
  {
    const __m256i *w = keys;
    UInt32 r = numRounds - 2;
    WOP (AVX_DECLARE_VAR)
    AVX_WOP_KEY (AVX_CTR_START, 0)

    w += 1;
    do
    {
      AVX_WOP_KEY (AVX_AES_ENC, 0)
      w += 1;
    }
    while (--r);
    AVX_WOP_KEY (AVX_AES_ENC_LAST, 0)
   
    WOP (AVX_CTR_END)
  }
  WIDE_LOOP_END_AVX (AVX_CTR_LOOP_ENC)
  
  SINGLE_LOOP
  {
    UInt32 numRounds2 = *(const UInt32 *)(p - 2 + 1) - 1;
    const __m128i *w = p;
    __m128i m;
    MM_OP (_mm_add_epi64, ctr, one)
    m = _mm_xor_si128 (ctr, p[0]);
    w += 1;
    do
    {
      MM_OP_m (_mm_aesenc_si128, w[0])
      MM_OP_m (_mm_aesenc_si128, w[1])
      w += 2;
    }
    while (--numRounds2);
    MM_OP_m (_mm_aesenc_si128,     w[0])
    MM_OP_m (_mm_aesenclast_si128, w[1])
    CTR_END (m, 0)
  }

  p[-2] = ctr;
}

#endif // USE_INTEL_VAES

#else // USE_INTEL_AES

/* no USE_INTEL_AES */

#if defined(Z7_USE_AES_HW_STUB)
// We can compile this file with another C compiler,
// or we can compile asm version.
// So we can generate real code instead of this stub function.
// #if defined(_MSC_VER)
#pragma message("AES  HW_SW stub was used")
// #endif

#if !defined(USE_INTEL_VAES) && defined(Z7_USE_VAES_HW_STUB)
#define AES_TYPE_keys UInt32
#define AES_TYPE_data Byte
#endif

#define AES_FUNC_START(name) \
    void Z7_FASTCALL name(UInt32 *p, Byte *data, size_t numBlocks) \

#define AES_COMPAT_STUB(name) \
    AES_FUNC_START(name); \
    AES_FUNC_START(name ## _HW) \
    { name(p, data, numBlocks); }

AES_COMPAT_STUB (AesCbc_Encode)
AES_COMPAT_STUB (AesCbc_Decode)
AES_COMPAT_STUB (AesCtr_Code)
#endif // Z7_USE_AES_HW_STUB

#endif // USE_INTEL_AES


#ifndef USE_INTEL_VAES
#if defined(Z7_USE_VAES_HW_STUB)
// #if defined(_MSC_VER)
#pragma message("VAES HW_SW stub was used")
// #endif

#define VAES_COMPAT_STUB(name) \
    void Z7_FASTCALL name ## _256(UInt32 *p, Byte *data, size_t numBlocks); \
    void Z7_FASTCALL name ## _256(UInt32 *p, Byte *data, size_t numBlocks) \
    { name((AES_TYPE_keys *)(void *)p, (AES_TYPE_data *)(void *)data, numBlocks); }

VAES_COMPAT_STUB (AesCbc_Decode_HW)
VAES_COMPAT_STUB (AesCtr_Code_HW)
#endif
#endif // ! USE_INTEL_VAES




#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE)

  #if   defined(__ARM_FEATURE_AES) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define USE_HW_AES
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
      #define USE_HW_AES
    #endif
    #endif
    #endif
  #endif

#ifdef USE_HW_AES

// #pragma message("=== AES HW === ")
// __ARM_FEATURE_CRYPTO macro is deprecated in favor of the finer grained feature macro __ARM_FEATURE_AES

#if defined(__clang__) || defined(__GNUC__)
#if !defined(__ARM_FEATURE_AES) && \
    !defined(__ARM_FEATURE_CRYPTO)
  #ifdef MY_CPU_ARM64
#if defined(__clang__)
    #define ATTRIB_AES __attribute__((__target__("crypto")))
#else
    #define ATTRIB_AES __attribute__((__target__("+crypto")))
#endif
  #else
#if defined(__clang__)
    #define ATTRIB_AES __attribute__((__target__("armv8-a,aes")))
#else
    #define ATTRIB_AES __attribute__((__target__("fpu=crypto-neon-fp-armv8")))
#endif
  #endif
#endif
#else
  // _MSC_VER
  // for arm32
  #define _ARM_USE_NEW_NEON_INTRINSICS
#endif

#ifndef ATTRIB_AES
  #define ATTRIB_AES
#endif

#if defined(Z7_MSC_VER_ORIGINAL) && defined(MY_CPU_ARM64)
#include <arm64_neon.h>
#else
/*
  clang-17.0.1: error : Cannot select: intrinsic %llvm.arm.neon.aese
  clang
   3.8.1 : __ARM_NEON             :                    defined(__ARM_FEATURE_CRYPTO)
   7.0.1 : __ARM_NEON             : __ARM_ARCH >= 8 && defined(__ARM_FEATURE_CRYPTO)
  11.?.0 : __ARM_NEON && __ARM_FP : __ARM_ARCH >= 8 && defined(__ARM_FEATURE_CRYPTO)
  13.0.1 : __ARM_NEON && __ARM_FP : __ARM_ARCH >= 8 && defined(__ARM_FEATURE_AES)
  16     : __ARM_NEON && __ARM_FP : __ARM_ARCH >= 8
*/
#if defined(__clang__) && __clang_major__ < 16
#if !defined(__ARM_FEATURE_AES) && \
    !defined(__ARM_FEATURE_CRYPTO)
//     #pragma message("=== we set __ARM_FEATURE_CRYPTO 1 === ")
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #define Z7_ARM_FEATURE_CRYPTO_WAS_SET 1
// #if defined(__clang__) && __clang_major__ < 13
    #define __ARM_FEATURE_CRYPTO 1
// #else
    #define __ARM_FEATURE_AES 1
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
    defined(__ARM_FEATURE_AES)
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #undef __ARM_FEATURE_CRYPTO
    #undef __ARM_FEATURE_AES
    #undef Z7_ARM_FEATURE_CRYPTO_WAS_SET
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
//    #pragma message("=== we undefine __ARM_FEATURE_CRYPTO === ")
#endif

#endif // Z7_MSC_VER_ORIGINAL

typedef uint8x16_t v128;

#define AES_FUNC_START(name) \
    void Z7_FASTCALL name(UInt32 *ivAes, Byte *data8, size_t numBlocks)
    // void Z7_FASTCALL name(v128 *p, v128 *data, size_t numBlocks)

#define AES_FUNC_START2(name) \
AES_FUNC_START (name); \
ATTRIB_AES \
AES_FUNC_START (name)

#define MM_OP(op, dest, src)  dest = op(dest, src);
#define MM_OP_m(op, src)      MM_OP(op, m, src)
#define MM_OP1_m(op)          m = op(m);

#define MM_XOR( dest, src)    MM_OP(veorq_u8, dest, src)
#define MM_XOR_m( src)        MM_XOR(m, src)

#define AES_E_m(k)     MM_OP_m (vaeseq_u8, k)
#define AES_E_MC_m(k)  AES_E_m (k)  MM_OP1_m(vaesmcq_u8)


AES_FUNC_START2 (AesCbc_Encode_HW)
{
  if (numBlocks == 0)
    return;
  {
  v128 * const p = (v128 *)(void *)ivAes;
  v128 *data = (v128 *)(void *)data8;
  v128 m = *p;
  const UInt32 numRounds2 = *(const UInt32 *)(p + 1);
  const v128 *w = p + (size_t)numRounds2 * 2;
  const v128 k0 = p[2];
  const v128 k1 = p[3];
  const v128 k2 = p[4];
  const v128 k3 = p[5];
  const v128 k4 = p[6];
  const v128 k5 = p[7];
  const v128 k6 = p[8];
  const v128 k7 = p[9];
  const v128 k8 = p[10];
  const v128 k9 = p[11];
  const v128 k_z4 = w[-2];
  const v128 k_z3 = w[-1];
  const v128 k_z2 = w[0];
  const v128 k_z1 = w[1];
  const v128 k_z0 = w[2];
  // we don't use optimization veorq_u8(*data, k_z0) that can reduce one cycle,
  // because gcc/clang compilers are not good for that optimization.
  do
  {
    MM_XOR_m (*data)
    AES_E_MC_m (k0)
    AES_E_MC_m (k1)
    AES_E_MC_m (k2)
    AES_E_MC_m (k3)
    AES_E_MC_m (k4)
    AES_E_MC_m (k5)
    if (numRounds2 >= 6)
    {
      AES_E_MC_m (k6)
      AES_E_MC_m (k7)
      if (numRounds2 != 6)
      {
        AES_E_MC_m (k8)
        AES_E_MC_m (k9)
      }
    }
    AES_E_MC_m (k_z4)
    AES_E_MC_m (k_z3)
    AES_E_MC_m (k_z2)
    AES_E_m    (k_z1)
    MM_XOR_m   (k_z0)
    *data++ = m;
  }
  while (--numBlocks);
  *p = m;
  }
}


#define WOP_1(op)
#define WOP_2(op)   WOP_1 (op)  op (m1, 1)
#define WOP_3(op)   WOP_2 (op)  op (m2, 2)
#define WOP_4(op)   WOP_3 (op)  op (m3, 3)
#define WOP_5(op)   WOP_4 (op)  op (m4, 4)
#define WOP_6(op)   WOP_5 (op)  op (m5, 5)
#define WOP_7(op)   WOP_6 (op)  op (m6, 6)
#define WOP_8(op)   WOP_7 (op)  op (m7, 7)

  #define NUM_WAYS      8
  #define WOP_M1    WOP_8

#define WOP(op)  op (m0, 0)   WOP_M1(op)

#define DECLARE_VAR(reg, ii)  v128 reg;
#define LOAD_data(  reg, ii)  reg = data[ii];
#define STORE_data( reg, ii)  data[ii] = reg;
#if (NUM_WAYS > 1)
#define XOR_data_M1(reg, ii)  MM_XOR (reg, data[ii- 1])
#endif

#define MM_OP_key(op, reg)  MM_OP (op, reg, key)

#define AES_D_m(k)      MM_OP_m (vaesdq_u8, k)
#define AES_D_IMC_m(k)  AES_D_m (k)  MM_OP1_m (vaesimcq_u8)

#define AES_XOR(   reg, ii)  MM_OP_key (veorq_u8,  reg)
#define AES_D(     reg, ii)  MM_OP_key (vaesdq_u8, reg)
#define AES_E(     reg, ii)  MM_OP_key (vaeseq_u8, reg)

#define AES_D_IMC( reg, ii)  AES_D (reg, ii)  reg = vaesimcq_u8(reg);
#define AES_E_MC(  reg, ii)  AES_E (reg, ii)  reg = vaesmcq_u8(reg);

#define CTR_START(reg, ii)  MM_OP (vaddq_u64, ctr, one)  reg = vreinterpretq_u8_u64(ctr);
#define CTR_END(  reg, ii)  MM_XOR (data[ii], reg)

#define WOP_KEY(op, n) { \
    const v128 key = w[n]; \
    WOP(op) }

#define WIDE_LOOP_START  \
    dataEnd = data + numBlocks;  \
    if (numBlocks >= NUM_WAYS)  \
    { dataEnd -= NUM_WAYS; do {  \

#define WIDE_LOOP_END  \
    data += NUM_WAYS;  \
    } while (data <= dataEnd);  \
    dataEnd += NUM_WAYS; }  \

#define SINGLE_LOOP  \
    for (; data < dataEnd; data++)


AES_FUNC_START2 (AesCbc_Decode_HW)
{
  v128 *p = (v128 *)(void *)ivAes;
  v128 *data = (v128 *)(void *)data8;
  v128 iv = *p;
  const v128 * const wStart = p + (size_t)*(const UInt32 *)(p + 1) * 2;
  const v128 *dataEnd;
  p += 2;
  
  WIDE_LOOP_START
  {
    const v128 *w = wStart;
    WOP (DECLARE_VAR)
    WOP (LOAD_data)
    WOP_KEY (AES_D_IMC, 2)
    do
    {
      WOP_KEY (AES_D_IMC, 1)
      WOP_KEY (AES_D_IMC, 0)
      w -= 2;
    }
    while (w != p);
    WOP_KEY (AES_D,   1)
    WOP_KEY (AES_XOR, 0)
    MM_XOR (m0, iv)
    WOP_M1 (XOR_data_M1)
    LOAD_data(iv, NUM_WAYS - 1)
    WOP (STORE_data)
  }
  WIDE_LOOP_END

  SINGLE_LOOP
  {
    const v128 *w = wStart;
    v128 m;  LOAD_data(m, 0)
    AES_D_IMC_m (w[2])
    do
    {
      AES_D_IMC_m (w[1])
      AES_D_IMC_m (w[0])
      w -= 2;
    }
    while (w != p);
    AES_D_m  (w[1])
    MM_XOR_m (w[0])
    MM_XOR_m (iv)
    LOAD_data(iv, 0)
    STORE_data(m, 0)
  }
  
  p[-2] = iv;
}


AES_FUNC_START2 (AesCtr_Code_HW)
{
  v128 *p = (v128 *)(void *)ivAes;
  v128 *data = (v128 *)(void *)data8;
  uint64x2_t ctr = vreinterpretq_u64_u8(*p);
  const v128 * const wEnd = p + (size_t)*(const UInt32 *)(p + 1) * 2;
  const v128 *dataEnd;
// the bug in clang:
// __builtin_neon_vsetq_lane_i64(__s0, (int8x16_t)__s1, __p2);
#if defined(__clang__) && (__clang_major__ <= 9)
#pragma GCC diagnostic ignored "-Wvector-conversion"
#endif
  const uint64x2_t one = vsetq_lane_u64(1, vdupq_n_u64(0), 0);
  p += 2;
  
  WIDE_LOOP_START
  {
    const v128 *w = p;
    WOP (DECLARE_VAR)
    WOP (CTR_START)
    do
    {
      WOP_KEY (AES_E_MC, 0)
      WOP_KEY (AES_E_MC, 1)
      w += 2;
    }
    while (w != wEnd);
    WOP_KEY (AES_E_MC, 0)
    WOP_KEY (AES_E,    1)
    WOP_KEY (AES_XOR,  2)
    WOP (CTR_END)
  }
  WIDE_LOOP_END

  SINGLE_LOOP
  {
    const v128 *w = p;
    v128 m;
    CTR_START (m, 0)
    do
    {
      AES_E_MC_m (w[0])
      AES_E_MC_m (w[1])
      w += 2;
    }
    while (w != wEnd);
    AES_E_MC_m (w[0])
    AES_E_m    (w[1])
    MM_XOR_m   (w[2])
    CTR_END (m, 0)
  }
  
  p[-2] = vreinterpretq_u8_u64(ctr);
}

#endif // USE_HW_AES

#endif // MY_CPU_ARM_OR_ARM64

#undef NUM_WAYS
#undef WOP_M1
#undef WOP
#undef DECLARE_VAR
#undef LOAD_data
#undef STORE_data
#undef USE_INTEL_AES
#undef USE_HW_AES

/* ================ unit: C/Alloc.c ================ */
/* Alloc.c -- Memory allocation functions
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#ifdef _WIN32
// amalgamation: header emitted in prologue
#endif
#include <stdlib.h>

// amalgamation: header emitted in prologue

#if defined(Z7_LARGE_PAGES) && defined(_WIN32) && \
    (!defined(Z7_WIN32_WINNT_MIN) || Z7_WIN32_WINNT_MIN < 0x0502)  // < Win2003 (xp-64)
  #define Z7_USE_DYN_GetLargePageMinimum
#endif

// for debug:
#if 0
#if defined(__CHERI__) && defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 16)
// #pragma message("=== Z7_ALLOC_NO_OFFSET_ALLOCATOR === ")
#define Z7_ALLOC_NO_OFFSET_ALLOCATOR
#endif
#endif

// #define SZ_ALLOC_DEBUG
/* use SZ_ALLOC_DEBUG to debug alloc/free operations */
#ifdef SZ_ALLOC_DEBUG

#include <string.h>
#include <stdio.h>
static int g_allocCount = 0;
#ifdef _WIN32
static int g_allocCountMid = 0;
#ifdef Z7_LARGE_PAGES
static int g_allocCountBig = 0;
#endif
#endif

#define CONVERT_INT_TO_STR(charType, tempSize) \
  char temp[tempSize]; unsigned i = 0; \
  while (val >= 10) { temp[i++] = (char)('0' + (unsigned)(val % 10)); val /= 10; } \
  *s++ = (charType)('0' + (unsigned)val); \
  while (i != 0) { i--; *s++ = temp[i]; } \
  *s = 0;

static void ConvertUInt64ToString(UInt64 val, char *s)
{
  CONVERT_INT_TO_STR(char, 24)
}

#define GET_HEX_CHAR(t) ((char)(((t < 10) ? ('0' + t) : ('A' + (t - 10)))))

static void ConvertUInt64ToHex(UInt64 val, char *s)
{
  UInt64 v = val;
  unsigned i;
  for (i = 1;; i++)
  {
    v >>= 4;
    if (v == 0)
      break;
  }
  s[i] = 0;
  do
  {
    unsigned t = (unsigned)(val & 0xF);
    val >>= 4;
    s[--i] = GET_HEX_CHAR(t);
  }
  while (i);
}

#define DEBUG_OUT_STREAM stderr

static void Print(const char *s)
{
  fputs(s, DEBUG_OUT_STREAM);
}

static void PrintAligned(const char *s, size_t align)
{
  size_t len = strlen(s);
  for(;;)
  {
    fputc(' ', DEBUG_OUT_STREAM);
    if (len >= align)
      break;
    ++len;
  }
  Print(s);
}

static void PrintLn(void)
{
  Print("\n");
}

static void PrintHex(UInt64 v, size_t align)
{
  char s[32];
  ConvertUInt64ToHex(v, s);
  PrintAligned(s, align);
}

static void PrintDec(int v, size_t align)
{
  char s[32];
  ConvertUInt64ToString((unsigned)v, s);
  PrintAligned(s, align);
}

static void PrintAddr(void *p)
{
  PrintHex((UInt64)(size_t)(ptrdiff_t)p, 12);
}


#define PRINT_REALLOC(name, cnt, size, ptr) { \
    Print(name " "); \
    if (!ptr) PrintDec(cnt++, 10); \
    PrintHex(size, 10); \
    PrintAddr(ptr); \
    PrintLn(); }

#define PRINT_ALLOC(name, cnt, size, ptr) { \
    Print(name " "); \
    PrintDec(cnt++, 10); \
    PrintHex(size, 10); \
    PrintAddr(ptr); \
    PrintLn(); }
 
#define PRINT_FREE(name, cnt, ptr) if (ptr) { \
    Print(name " "); \
    PrintDec(--cnt, 10); \
    PrintAddr(ptr); \
    PrintLn(); }
 
#else

#ifdef _WIN32
#ifdef Z7_LARGE_PAGES
#define PRINT_ALLOC(name, cnt, size, ptr)
#endif
#endif
#define PRINT_FREE(name, cnt, ptr)
#define Print(s)
#define PrintLn()
#ifndef Z7_ALLOC_NO_OFFSET_ALLOCATOR
#define PrintHex(v, align)
#endif
#define PrintAddr(p)

#endif


/*
by specification:
  malloc(non_NULL, 0)   : returns NULL or a unique pointer value that can later be successfully passed to free()
  realloc(NULL, size)   : the call is equivalent to malloc(size)
  realloc(non_NULL, 0)  : the call is equivalent to free(ptr)

in main compilers:
  malloc(0)             : returns non_NULL
  realloc(NULL,     0)  : returns non_NULL
  realloc(non_NULL, 0)  : returns NULL
*/


void *MyAlloc(size_t size)
{
  if (size == 0)
    return NULL;
  // PRINT_ALLOC("Alloc    ", g_allocCount, size, NULL)
  #ifdef SZ_ALLOC_DEBUG
  {
    void *p = malloc(size);
    if (p)
    {
      PRINT_ALLOC("Alloc    ", g_allocCount, size, p)
    }
    return p;
  }
  #else
  return malloc(size);
  #endif
}

void MyFree(void *address)
{
  PRINT_FREE("Free    ", g_allocCount, address)
  
  free(address);
}

void *MyRealloc(void *address, size_t size)
{
  if (size == 0)
  {
    MyFree(address);
    return NULL;
  }
  // PRINT_REALLOC("Realloc  ", g_allocCount, size, address)
  #ifdef SZ_ALLOC_DEBUG
  {
    void *p = realloc(address, size);
    if (p)
    {
      PRINT_REALLOC("Realloc    ", g_allocCount, size, address)
    }
    return p;
  }
  #else
  return realloc(address, size);
  #endif
}


#ifdef _WIN32

void *MidAlloc(size_t size)
{
  if (size == 0)
    return NULL;
  #ifdef SZ_ALLOC_DEBUG
  {
    void *p = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (p)
    {
      PRINT_ALLOC("Alloc-Mid", g_allocCountMid, size, p)
    }
    return p;
  }
  #else
  return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
  #endif
}

void MidFree(void *address)
{
  PRINT_FREE("Free-Mid", g_allocCountMid, address)

  if (!address)
    return;
  VirtualFree(address, 0, MEM_RELEASE);
}

#ifdef Z7_LARGE_PAGES
// #pragma message("Z7_LARGE_PAGES")

#ifdef MEM_LARGE_PAGES
  #define MY_MEM_LARGE_PAGES  MEM_LARGE_PAGES
#else
  #define MY_MEM_LARGE_PAGES  0x20000000
#endif

extern
size_t g_LargePageSize;
size_t g_LargePageSize = 0;
extern
size_t g_LargePageThresholdMin;
size_t g_LargePageThresholdMin = 0;
extern
UInt32 g_LargePageFlags;
UInt32 g_LargePageFlags = 0;

void *BigAlloc(size_t size)
{
  if (size == 0)
    return NULL;

  PRINT_ALLOC("Alloc-Big", g_allocCountBig, size, NULL)

  #ifdef Z7_LARGE_PAGES
  {
    const size_t ps = g_LargePageSize - 1;
    if (ps < (1u << 30) && size > g_LargePageThresholdMin)
    {
      const size_t size2 = (size + ps) & ~ps;
      if (size2 >= size)
      {
        void *p = VirtualAlloc(NULL, size2, MEM_COMMIT | MY_MEM_LARGE_PAGES, PAGE_READWRITE);
        if (p)
        {
          PRINT_ALLOC("Alloc-BM ", g_allocCountMid, size2, p)
          return p;
        }
        if (g_LargePageFlags & Z7_LARGE_PAGES_FLAG_FAIL_STOP)
          return p;
      }
    }
  }
  #endif

  return MidAlloc(size);
}

void BigFree(void *address)
{
  PRINT_FREE("Free-Big", g_allocCountBig, address)
  MidFree(address);
}

#endif // Z7_LARGE_PAGES
#endif // _WIN32


static void *SzAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p)  return MyAlloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p)  MyFree(address); }
const ISzAlloc g_Alloc = { SzAlloc, SzFree };

#ifdef _WIN32
static void *SzMidAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p)  return MidAlloc(size); }
static void SzMidFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p)  MidFree(address); }
const ISzAlloc g_MidAlloc = { SzMidAlloc, SzMidFree };
#endif

#if defined(Z7_LARGE_PAGES)
static void *SzBigAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p)  return BigAlloc(size); }
static void SzBigFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p)  BigFree(address); }
const ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };
#endif

#ifndef Z7_ALLOC_NO_OFFSET_ALLOCATOR

#define ADJUST_ALLOC_SIZE 0
/*
#define ADJUST_ALLOC_SIZE (sizeof(void *) - 1)
*/
/*
  Use (ADJUST_ALLOC_SIZE = (sizeof(void *) - 1)), if
     MyAlloc() can return address that is NOT multiple of sizeof(void *).
*/

/*
  uintptr_t : <stdint.h> C99 (optional)
            : unsupported in VS6
*/
typedef
  #ifdef _WIN32
    UINT_PTR
  #elif 1
    uintptr_t
  #else
    ptrdiff_t
  #endif
    MY_uintptr_t;

#if 0 \
    || (defined(__CHERI__) \
    || defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ > 8))
// for 128-bit pointers (cheri):
#define MY_ALIGN_PTR_DOWN(p, align)  \
    ((void *)((char *)(p) - ((size_t)(MY_uintptr_t)(p) & ((align) - 1))))
#else
#define MY_ALIGN_PTR_DOWN(p, align) \
    ((void *)((((MY_uintptr_t)(p)) & ~((MY_uintptr_t)(align) - 1))))
#endif

#endif

#ifndef _WIN32
#include <unistd.h> // for _POSIX_ADVISORY_INFO : for some linux
#if (defined(Z7_ALLOC_NO_OFFSET_ALLOCATOR) \
        || defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L) \
        || defined(_POSIX_ADVISORY_INFO) && (_POSIX_ADVISORY_INFO >= 200112L) \
        || defined(__APPLE__) \
        /* || defined(__linux__) */)
  #define USE_posix_memalign
  // #pragma message("USE_posix_memalign")
#endif
#endif

#ifndef USE_posix_memalign
#define MY_ALIGN_PTR_UP_PLUS(p, align) MY_ALIGN_PTR_DOWN(((char *)(p) + (align) + ADJUST_ALLOC_SIZE), align)
#endif

/*
  This posix_memalign() is for test purposes only.
  We also need special Free() function instead of free(),
  if this posix_memalign() is used.
*/

/*
static int posix_memalign(void **ptr, size_t align, size_t size)
{
  size_t newSize = size + align;
  void *p;
  void *pAligned;
  *ptr = NULL;
  if (newSize < size)
    return 12; // ENOMEM
  p = MyAlloc(newSize);
  if (!p)
    return 12; // ENOMEM
  pAligned = MY_ALIGN_PTR_UP_PLUS(p, align);
  ((void **)pAligned)[-1] = p;
  *ptr = pAligned;
  return 0;
}
*/

/*
  ALLOC_ALIGN_SIZE >= sizeof(void *)
  ALLOC_ALIGN_SIZE >= cache_line_size
*/

#define ALLOC_ALIGN_SIZE ((size_t)1 << 7)

void *z7_AlignedAlloc(size_t size)
{
#ifndef USE_posix_memalign
  
  void *p;
  void *pAligned;
  size_t newSize;

  /* also we can allocate additional dummy ALLOC_ALIGN_SIZE bytes after aligned
     block to prevent cache line sharing with another allocated blocks */

  newSize = size + ALLOC_ALIGN_SIZE * 1 + ADJUST_ALLOC_SIZE;
  if (newSize < size)
    return NULL;

  p = MyAlloc(newSize);
  
  if (!p)
    return NULL;
  pAligned = MY_ALIGN_PTR_UP_PLUS(p, ALLOC_ALIGN_SIZE);

  Print(" size="); PrintHex(size, 8);
  Print(" a_size="); PrintHex(newSize, 8);
  Print(" ptr="); PrintAddr(p);
  Print(" a_ptr="); PrintAddr(pAligned);
  PrintLn();

  ((void **)pAligned)[-1] = p;

  return pAligned;

#else

  void *p;
  if (posix_memalign(&p, ALLOC_ALIGN_SIZE, size))
    return NULL;

  Print(" posix_memalign="); PrintAddr(p);
  PrintLn();

  return p;

#endif
}


void z7_AlignedFree(void *address)
{
#ifndef USE_posix_memalign
  if (address)
    MyFree(((void **)address)[-1]);
#else
  free(address);
#endif
}


static void *SzAlignedAlloc(ISzAllocPtr pp, size_t size)
{
  UNUSED_VAR(pp)
  return z7_AlignedAlloc(size);
}


static void SzAlignedFree(ISzAllocPtr pp, void *address)
{
  UNUSED_VAR(pp)
#ifndef USE_posix_memalign
  if (address)
    MyFree(((void **)address)[-1]);
#else
  free(address);
#endif
}

#ifndef _WIN32

#ifdef Z7_LARGE_PAGES

#if 0 // 1 for debug
  #include <stdio.h>
  #include <string.h>  // for strerror()
  #define PRF(x) x
#else
  #define PRF(x)
#endif

#ifdef USE_posix_memalign
  /* madvise():
     glibc <= 2.19 : _BSD_SOURCE
     glibc  > 2.19 : _DEFAULT_SOURCE
  */
  /* && (defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE)) */
#if 1 && !defined(Z7_NO_MADVISE) && \
  (defined(__linux__) || defined(__unix__) || defined(__APPLE__))
#include <sys/mman.h> // for madvise
// #pragma message("sys/mman.h")
#if (defined(MADV_HUGEPAGE) && defined(MADV_NOHUGEPAGE))
  #define Z7_USE_BIG_ALLOC_MADVISE
  // #pragma message("Z7_USE_BIG_ALLOC_MADVISE")
#endif
#endif
#endif // USE_posix_memalign

#ifdef Z7_USE_BIG_ALLOC_MADVISE
#define LARGE_PAGE_SIZE_DEFAULT (1 << 21)
#else
#define LARGE_PAGE_SIZE_DEFAULT 0
#endif

extern
size_t g_LargePageSize;
size_t g_LargePageSize = LARGE_PAGE_SIZE_DEFAULT;
extern
size_t g_LargePageThresholdMin;
size_t g_LargePageThresholdMin = LARGE_PAGE_SIZE_DEFAULT / 2;
extern
UInt32 g_LargePageFlags;
UInt32 g_LargePageFlags = 0;

void *BigAlloc(size_t size)
{
  if (size == 0)
    return NULL;
#ifdef USE_posix_memalign
  {
    const size_t pageSize = g_LargePageSize;
    void *buf = NULL; // on Linux (and other systems), posix_memalign() does not modify memptr on failure (POSIX.1-2008 TC2).
    PRF(printf("\nBigAlloc 0x%08x=%5uMB", (unsigned)(size), (unsigned)(size >> 20));)
    if (pageSize && size > g_LargePageThresholdMin)
    {
      int res;
      const size_t mask = pageSize - 1;
      /* we can allocate aligned size, so data at the end of buffer also will use huge page
         if (size2 for madvise() is not aligned for huge page size)
           { Last data block will use small pages. It reduces memory allocation,
             but last data block with small pages can work slower.
             It's useful, if we have very large HUGE_PAGE: 32MB or 512MB. }
      */
      size_t size2 = (size + mask) & ~mask;
      if (size2 < size || (size & mask) <= g_LargePageThresholdMin)
        size2 = size;
      res = posix_memalign(&buf, pageSize, size2);
      PRF(printf(" posix_memalign size=0x%08x=%5uMB align=%u",
          (unsigned)(size2), (unsigned)(size2 >> 20), (unsigned)pageSize);)
      PRF(printf(" buf=%p", (void *)buf);)
      if (res == 0)
      {
#ifdef Z7_USE_BIG_ALLOC_MADVISE
        if ((g_LargePageFlags & Z7_LARGE_PAGES_FLAG_NO_MADVISE) == 0)
        {
          // Advise the kernel to use huge pages for this memory range
          // MADV_HUGEPAGE / MADV_NOHUGEPAGE : since Linux 2.6.38
          // madvise() only operates on whole pages, therefore addr must be page-aligned (4KB/8KB/16KB/64KB).
          // The value of size is rounded up to a multiple of page size.
          PRF(printf(" madvise g_LargePageFlags=%x", (unsigned)g_LargePageFlags);)
          res = madvise(buf, size2, (g_LargePageFlags & Z7_LARGE_PAGES_FLAG_NO_HUGEPAGE) ? MADV_NOHUGEPAGE : MADV_HUGEPAGE);
          if (res)
          {
            PRF(printf("\nERROR res=%d, errno=%d=%s\n", res, (int)errno, strerror(errno));)
            if (g_LargePageFlags & Z7_LARGE_PAGES_FLAG_FAIL_STOP)
            {
              free(buf);
              return NULL;
            }
          }
        }
#endif // Z7_USE_BIG_ALLOC_MADVISE
        PRF(printf("\n");)
        return buf;
      }
      PRF(printf("\nERROR res=%d=%s\n", res, strerror(res));)
      if (g_LargePageFlags & Z7_LARGE_PAGES_FLAG_FAIL_STOP)
        return NULL;
      // (res == ENOMEM) "Out of memory" is possible, if pageSize is too big.
      // so we do second attempt with smaller alignment
    }
  }
#endif // !USE_posix_memalign
  PRF(printf(" z7_AlignedAlloc size=0x%08x=%5uMB\n", (unsigned)(size), (unsigned)(size >> 20));)
  return z7_AlignedAlloc(size);
}


void BigFree(void *address)
{
  z7_AlignedFree(address);
}
#endif // Z7_LARGE_PAGES
#endif // !_WIN32


#ifdef Z7_LARGE_PAGES
void z7_LargePage_Set(UInt32 flags, size_t pageSize, size_t threshold)
{
  g_LargePageFlags = flags;

#ifdef _WIN32
  if ((flags & Z7_LARGE_PAGES_FLAG_USE_HUGEPAGE) == 0)
  {
    g_LargePageSize = 0;
    g_LargePageThresholdMin = 0;
  }
  else
  {
    if ((flags & Z7_LARGE_PAGES_FLAG_DIRECT_PAGE_SIZE) == 0)
    {
#ifdef Z7_USE_DYN_GetLargePageMinimum
      Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION
typedef SIZE_T (WINAPI *Func_GetLargePageMinimum)(VOID);
      const
        Func_GetLargePageMinimum fn =
       (Func_GetLargePageMinimum) Z7_CAST_FUNC_C GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
            "GetLargePageMinimum");
      if (fn)
        pageSize = fn();
      else
        pageSize = 0;
#else
      pageSize = GetLargePageMinimum();
#endif
      if (pageSize & (pageSize - 1))
        pageSize = 0;
    }
    g_LargePageSize = pageSize;
    if ((flags & Z7_LARGE_PAGES_FLAG_DIRECT_THRESHOLD) == 0)
      threshold = pageSize / 2;
    g_LargePageThresholdMin = threshold;
  }

#else // !_WIN32

  if (flags & Z7_LARGE_PAGES_FLAG_NO_PAGECODE)
  {
    g_LargePageSize = 0;
    g_LargePageThresholdMin = 0;
  }
  else
  {
    if ((flags & Z7_LARGE_PAGES_FLAG_DIRECT_PAGE_SIZE) == 0)
      pageSize = LARGE_PAGE_SIZE_DEFAULT;
    g_LargePageSize = pageSize;
    if ((flags & Z7_LARGE_PAGES_FLAG_DIRECT_THRESHOLD) == 0)
      threshold = pageSize / 2;
    g_LargePageThresholdMin = threshold;
  }
  // PRF(printf("\ng_LargePageSize=%x g_LargePageThresholdMin = %x g_LargePageFlags = %x", (unsigned)g_LargePageSize, (unsigned)g_LargePageThresholdMin, (unsigned)g_LargePageFlags);)
#endif // !_WIN32
}
#endif // Z7_LARGE_PAGES

const ISzAlloc g_AlignedAlloc = { SzAlignedAlloc, SzAlignedFree };



/* we align ptr to support cases where CAlignOffsetAlloc::offset is not multiply of sizeof(void *) */
#ifndef Z7_ALLOC_NO_OFFSET_ALLOCATOR
#if 1
  #define MY_ALIGN_PTR_DOWN_1(p)  MY_ALIGN_PTR_DOWN(p, sizeof(void *))
  #define REAL_BLOCK_PTR_VAR(p)  ((void **)MY_ALIGN_PTR_DOWN_1(p))[-1]
#else
  // we can use this simplified code,
  // if (CAlignOffsetAlloc::offset == (k * sizeof(void *))
  #define REAL_BLOCK_PTR_VAR(p)  (((void **)(p))[-1])
#endif
#endif


#if 0
#ifndef Z7_ALLOC_NO_OFFSET_ALLOCATOR
#include <stdio.h>
static void PrintPtr(const char *s, const void *p)
{
  const Byte *p2 = (const Byte *)&p;
  unsigned i;
  printf("%s %p ", s, p);
  for (i = sizeof(p); i != 0;)
  {
    i--;
    printf("%02x", p2[i]);
  }
  printf("\n");
}
#endif
#endif


static void *AlignOffsetAlloc_Alloc(ISzAllocPtr pp, size_t size)
{
#if defined(Z7_ALLOC_NO_OFFSET_ALLOCATOR)
  UNUSED_VAR(pp)
  return z7_AlignedAlloc(size);
#else
  const CAlignOffsetAlloc *p = Z7_CONTAINER_FROM_VTBL_CONST(pp, CAlignOffsetAlloc, vt);
  void *adr;
  void *pAligned;
  size_t newSize;
  size_t extra;
  size_t alignSize = (size_t)1 << p->numAlignBits;

  if (alignSize < sizeof(void *))
    alignSize = sizeof(void *);
  
  if (p->offset >= alignSize)
    return NULL;

  /* also we can allocate additional dummy ALLOC_ALIGN_SIZE bytes after aligned
     block to prevent cache line sharing with another allocated blocks */
  extra = p->offset & (sizeof(void *) - 1);
  newSize = size + alignSize + extra + ADJUST_ALLOC_SIZE;
  if (newSize < size)
    return NULL;

  adr = ISzAlloc_Alloc(p->baseAlloc, newSize);
  
  if (!adr)
    return NULL;

  pAligned = (char *)MY_ALIGN_PTR_DOWN((char *)adr +
      alignSize - p->offset + extra + ADJUST_ALLOC_SIZE, alignSize) + p->offset;

#if 0
  printf("\nalignSize = %6x, offset=%6x, size=%8x \n", (unsigned)alignSize, (unsigned)p->offset, (unsigned)size);
  PrintPtr("base", adr);
  PrintPtr("alig", pAligned);
#endif

  PrintLn();
  Print("- Aligned: ");
  Print(" size="); PrintHex(size, 8);
  Print(" a_size="); PrintHex(newSize, 8);
  Print(" ptr="); PrintAddr(adr);
  Print(" a_ptr="); PrintAddr(pAligned);
  PrintLn();

  REAL_BLOCK_PTR_VAR(pAligned) = adr;

  return pAligned;
#endif
}


static void AlignOffsetAlloc_Free(ISzAllocPtr pp, void *address)
{
#if defined(Z7_ALLOC_NO_OFFSET_ALLOCATOR)
  UNUSED_VAR(pp)
  z7_AlignedFree(address);
#else
  if (address)
  {
    const CAlignOffsetAlloc *p = Z7_CONTAINER_FROM_VTBL_CONST(pp, CAlignOffsetAlloc, vt);
    PrintLn();
    Print("- Aligned Free: ");
    PrintLn();
    ISzAlloc_Free(p->baseAlloc, REAL_BLOCK_PTR_VAR(address));
  }
#endif
}


void AlignOffsetAlloc_CreateVTable(CAlignOffsetAlloc *p)
{
  p->vt.Alloc = AlignOffsetAlloc_Alloc;
  p->vt.Free = AlignOffsetAlloc_Free;
}

/* ================ unit: C/Bcj2.c ================ */
/* Bcj2.c -- BCJ2 Decoder (Converter for x86 code)
2023-03-01 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define kTopValue ((UInt32)1 << 24)
#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

// UInt32 bcj2_stats[256 + 2][2];

void Bcj2Dec_Init(CBcj2Dec *p)
{
  unsigned i;
  p->state = BCJ2_STREAM_RC; // BCJ2_DEC_STATE_OK;
  p->ip = 0;
  p->temp = 0;
  p->range = 0;
  p->code = 0;
  for (i = 0; i < sizeof(p->probs) / sizeof(p->probs[0]); i++)
    p->probs[i] = kBitModelTotal >> 1;
}

SRes Bcj2Dec_Decode(CBcj2Dec *p)
{
  UInt32 v = p->temp;
  // const Byte *src;
  if (p->range <= 5)
  {
    UInt32 code = p->code;
    p->state = BCJ2_DEC_STATE_ERROR; /* for case if we return SZ_ERROR_DATA; */
    for (; p->range != 5; p->range++)
    {
      if (p->range == 1 && code != 0)
        return SZ_ERROR_DATA;
      if (p->bufs[BCJ2_STREAM_RC] == p->lims[BCJ2_STREAM_RC])
      {
        p->state = BCJ2_STREAM_RC;
        return SZ_OK;
      }
      code = (code << 8) | *(p->bufs[BCJ2_STREAM_RC])++;
      p->code = code;
    }
    if (code == 0xffffffff)
      return SZ_ERROR_DATA;
    p->range = 0xffffffff;
  }
  // else
  {
    unsigned state = p->state;
    // we check BCJ2_IS_32BIT_STREAM() here instead of check in the main loop
    if (BCJ2_IS_32BIT_STREAM(state))
    {
      const Byte *cur = p->bufs[state];
      if (cur == p->lims[state])
        return SZ_OK;
      p->bufs[state] = cur + 4;
      {
        const UInt32 ip = p->ip + 4;
        v = GetBe32a(cur) - ip;
        p->ip = ip;
      }
      state = BCJ2_DEC_STATE_ORIG_0;
    }
    if ((unsigned)(state - BCJ2_DEC_STATE_ORIG_0) < 4)
    {
      Byte *dest = p->dest;
      for (;;)
      {
        if (dest == p->destLim)
        {
          p->state = state;
          p->temp = v;
          return SZ_OK;
        }
        *dest++ = (Byte)v;
        p->dest = dest;
        if (++state == BCJ2_DEC_STATE_ORIG_3 + 1)
          break;
        v >>= 8;
      }
    }
  }

  // src = p->bufs[BCJ2_STREAM_MAIN];
  for (;;)
  {
    /*
    if (BCJ2_IS_32BIT_STREAM(p->state))
      p->state = BCJ2_DEC_STATE_OK;
    else
    */
    {
      if (p->range < kTopValue)
      {
        if (p->bufs[BCJ2_STREAM_RC] == p->lims[BCJ2_STREAM_RC])
        {
          p->state = BCJ2_STREAM_RC;
          p->temp = v;
          return SZ_OK;
        }
        p->range <<= 8;
        p->code = (p->code << 8) | *(p->bufs[BCJ2_STREAM_RC])++;
      }
      {
        const Byte *src = p->bufs[BCJ2_STREAM_MAIN];
        const Byte *srcLim;
        Byte *dest = p->dest;
        {
          const SizeT rem = (SizeT)(p->lims[BCJ2_STREAM_MAIN] - src);
          SizeT num = (SizeT)(p->destLim - dest);
          if (num >= rem)
            num = rem;
        #define NUM_ITERS 4
        #if (NUM_ITERS & (NUM_ITERS - 1)) == 0
          num &= ~((SizeT)NUM_ITERS - 1);   // if (NUM_ITERS == (1 << x))
        #else
          num -= num % NUM_ITERS; // if (NUM_ITERS != (1 << x))
        #endif
          srcLim = src + num;
        }

        #define NUM_SHIFT_BITS  24
        #define ONE_ITER(indx) { \
          const unsigned b = src[indx]; \
          *dest++ = (Byte)b; \
          v = (v << NUM_SHIFT_BITS) | b; \
          if (((b + (0x100 - 0xe8)) & 0xfe) == 0) break; \
          if (((v - (((UInt32)0x0f << (NUM_SHIFT_BITS)) + 0x80)) & \
              ((((UInt32)1 << (4 + NUM_SHIFT_BITS)) - 0x1) << 4)) == 0) break; \
            /* ++dest */; /* v = b; */ }
          
        if (src != srcLim)
        for (;;)
        {
            /* The dependency chain of 2-cycle for (v) calculation is not big problem here.
               But we can remove dependency chain with v = b in the end of loop. */
          ONE_ITER(0)
          #if (NUM_ITERS > 1)
            ONE_ITER(1)
          #if (NUM_ITERS > 2)
            ONE_ITER(2)
          #if (NUM_ITERS > 3)
            ONE_ITER(3)
          #if (NUM_ITERS > 4)
            ONE_ITER(4)
          #if (NUM_ITERS > 5)
            ONE_ITER(5)
          #if (NUM_ITERS > 6)
            ONE_ITER(6)
          #if (NUM_ITERS > 7)
            ONE_ITER(7)
          #endif
          #endif
          #endif
          #endif
          #endif
          #endif
          #endif
          
          src += NUM_ITERS;
          if (src == srcLim)
            break;
        }

        if (src == srcLim)
      #if (NUM_ITERS > 1)
        for (;;)
      #endif
        {
        #if (NUM_ITERS > 1)
          if (src == p->lims[BCJ2_STREAM_MAIN] || dest == p->destLim)
        #endif
          {
            const SizeT num = (SizeT)(src - p->bufs[BCJ2_STREAM_MAIN]);
            p->bufs[BCJ2_STREAM_MAIN] = src;
            p->dest = dest;
            p->ip += (UInt32)num;
            /* state BCJ2_STREAM_MAIN has more priority than BCJ2_STATE_ORIG */
            p->state =
              src == p->lims[BCJ2_STREAM_MAIN] ?
                (unsigned)BCJ2_STREAM_MAIN :
                (unsigned)BCJ2_DEC_STATE_ORIG;
            p->temp = v;
            return SZ_OK;
          }
        #if (NUM_ITERS > 1)
          ONE_ITER(0)
          src++;
        #endif
        }

        {
          const SizeT num = (SizeT)(dest - p->dest);
          p->dest = dest; // p->dest += num;
          p->bufs[BCJ2_STREAM_MAIN] += num; // = src;
          p->ip += (UInt32)num;
        }
        {
          UInt32 bound, ttt;
          CBcj2Prob *prob; // unsigned index;
          /*
          prob = p->probs + (unsigned)((Byte)v == 0xe8 ?
              2 + (Byte)(v >> 8) :
              ((v >> 5) & 1));  // ((Byte)v < 0xe8 ? 0 : 1));
          */
          {
            const unsigned c = ((v + 0x17) >> 6) & 1;
            prob = p->probs + (unsigned)
                (((0 - c) & (Byte)(v >> NUM_SHIFT_BITS)) + c + ((v >> 5) & 1));
                // (Byte)
                // 8x->0     : e9->1     : xxe8->xx+2
                // 8x->0x100 : e9->0x101 : xxe8->xx
                // (((0x100 - (e & ~v)) & (0x100 | (v >> 8))) + (e & v));
                // (((0x101 + (~e | v)) & (0x100 | (v >> 8))) + (e & v));
          }
          ttt = *prob;
          bound = (p->range >> kNumBitModelTotalBits) * ttt;
          if (p->code < bound)
          {
            // bcj2_stats[prob - p->probs][0]++;
            p->range = bound;
            *prob = (CBcj2Prob)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
            continue;
          }
          {
            // bcj2_stats[prob - p->probs][1]++;
            p->range -= bound;
            p->code -= bound;
            *prob = (CBcj2Prob)(ttt - (ttt >> kNumMoveBits));
          }
        }
      }
    }
    {
      /* (v == 0xe8 ? 0 : 1) uses setcc instruction with additional zero register usage in x64 MSVC. */
      // const unsigned cj = ((Byte)v == 0xe8) ? BCJ2_STREAM_CALL : BCJ2_STREAM_JUMP;
      const unsigned cj = (((v + 0x57) >> 6) & 1) + BCJ2_STREAM_CALL;
      const Byte *cur = p->bufs[cj];
      Byte *dest;
      SizeT rem;
      if (cur == p->lims[cj])
      {
        p->state = cj;
        break;
      }
      v = GetBe32a(cur);
      p->bufs[cj] = cur + 4;
      {
        const UInt32 ip = p->ip + 4;
        v -= ip;
        p->ip = ip;
      }
      dest = p->dest;
      rem = (SizeT)(p->destLim - dest);
      if (rem < 4)
      {
        if ((unsigned)rem > 0) { dest[0] = (Byte)v;  v >>= 8;
        if ((unsigned)rem > 1) { dest[1] = (Byte)v;  v >>= 8;
        if ((unsigned)rem > 2) { dest[2] = (Byte)v;  v >>= 8; }}}
        p->temp = v;
        p->dest = dest + rem;
        p->state = BCJ2_DEC_STATE_ORIG_0 + (unsigned)rem;
        break;
      }
      SetUi32(dest, v)
      v >>= 24;
      p->dest = dest + 4;
    }
  }

  if (p->range < kTopValue && p->bufs[BCJ2_STREAM_RC] != p->lims[BCJ2_STREAM_RC])
  {
    p->range <<= 8;
    p->code = (p->code << 8) | *(p->bufs[BCJ2_STREAM_RC])++;
  }
  return SZ_OK;
}

#undef NUM_ITERS
#undef ONE_ITER
#undef NUM_SHIFT_BITS
#undef kTopValue
#undef kNumBitModelTotalBits
#undef kBitModelTotal
#undef kNumMoveBits

/* ================ unit: C/Bcj2Enc.c ================ */
/* Bcj2Enc.c -- BCJ2 Encoder converter for x86 code (Branch CALL/JUMP variant2)
2023-04-02 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

/* #define SHOW_STAT */
#ifdef SHOW_STAT
#include <stdio.h>
#define PRF2(s) printf("%s ip=%8x  tempPos=%d  src= %8x\n", s, (unsigned)p->ip64, p->tempPos, (unsigned)(p->srcLim - p->src));
#else
#define PRF2(s)
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define kTopValue ((UInt32)1 << 24)
#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

void Bcj2Enc_Init(CBcj2Enc *p)
{
  unsigned i;
  p->state = BCJ2_ENC_STATE_ORIG;
  p->finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
  p->context = 0;
  p->flushRem = 5;
  p->isFlushState = 0;
  p->cache = 0;
  p->range = 0xffffffff;
  p->low = 0;
  p->cacheSize = 1;
  p->ip64 = 0;
  p->fileIp64 = 0;
  p->fileSize64_minus1 = BCJ2_ENC_FileSizeField_UNLIMITED;
  p->relatLimit = BCJ2_ENC_RELAT_LIMIT_DEFAULT;
  // p->relatExcludeBits = 0;
  p->tempPos = 0;
  for (i = 0; i < sizeof(p->probs) / sizeof(p->probs[0]); i++)
    p->probs[i] = kBitModelTotal >> 1;
}

// Z7_NO_INLINE
Z7_FORCE_INLINE
static BoolInt Bcj2_RangeEnc_ShiftLow(CBcj2Enc *p)
{
  const UInt32 low = (UInt32)p->low;
  const unsigned high = (unsigned)
    #if defined(Z7_MSC_VER_ORIGINAL) \
        && defined(MY_CPU_X86) \
        && defined(MY_CPU_LE) \
        && !defined(MY_CPU_64BIT)
      // we try to rid of __aullshr() call in MSVS-x86
      (((const UInt32 *)&p->low)[1]); // [1] : for little-endian only
    #else
      (p->low >> 32);
    #endif
  if (low < (UInt32)0xff000000 || high != 0)
  {
    Byte *buf = p->bufs[BCJ2_STREAM_RC];
    do
    {
      if (buf == p->lims[BCJ2_STREAM_RC])
      {
        p->state = BCJ2_STREAM_RC;
        p->bufs[BCJ2_STREAM_RC] = buf;
        return True;
      }
      *buf++ = (Byte)(p->cache + high);
      p->cache = 0xff;
    }
    while (--p->cacheSize);
    p->bufs[BCJ2_STREAM_RC] = buf;
    p->cache = (Byte)(low >> 24);
  }
  p->cacheSize++;
  p->low = low << 8;
  return False;
}


/*
We can use 2 alternative versions of code:
1) non-marker version:
  Byte CBcj2Enc::context
  Byte temp[8];
  Last byte of marker (e8/e9/[0f]8x) can be written to temp[] buffer.
  Encoder writes last byte of marker (e8/e9/[0f]8x) to dest, only in conjunction
  with writing branch symbol to range coder in same Bcj2Enc_Encode_2() call.

2) marker version:
  UInt32 CBcj2Enc::context
  Byte CBcj2Enc::temp[4];
  MARKER_FLAG in CBcj2Enc::context shows that CBcj2Enc::context contains finded marker.
  it's allowed that
    one call of Bcj2Enc_Encode_2() writes last byte of marker (e8/e9/[0f]8x) to dest,
    and another call of Bcj2Enc_Encode_2() does offset conversion.
    So different values of (fileIp) and (fileSize) are possible
    in these different Bcj2Enc_Encode_2() calls.

Also marker version requires additional if((v & MARKER_FLAG) == 0) check in main loop.
So we use non-marker version.
*/

/*
  Corner cases with overlap in multi-block.
  before v23: there was one corner case, where converted instruction
    could start in one sub-stream and finish in next sub-stream.
  If multi-block (solid) encoding is used,
    and BCJ2_ENC_FINISH_MODE_END_BLOCK is used for each sub-stream.
    and (0f) is last byte of previous sub-stream
    and (8x) is first byte of current sub-stream
  then (0f 8x) pair is treated as marker by BCJ2 encoder and decoder.
  BCJ2 encoder can converts 32-bit offset for that (0f 8x) cortage,
  if that offset meets limit requirements.
  If encoder allows 32-bit offset conversion for such overlap case,
  then the data in 3 uncompressed BCJ2 streams for some sub-stream
  can depend from data of previous sub-stream.
  That corner case is not big problem, and it's rare case.
  Since v23.00 we do additional check to prevent conversions in such overlap cases.
*/

/*
  Bcj2Enc_Encode_2() output variables at exit:
  {
    if (Bcj2Enc_Encode_2() exits with (p->state == BCJ2_ENC_STATE_ORIG))
    {
      it means that encoder needs more input data.
      if (p->srcLim == p->src) at exit, then
      {
        (p->finishMode != BCJ2_ENC_FINISH_MODE_END_STREAM)
        all input data were read and processed, and we are ready for
        new input data.
      }
      else
      {
        (p->srcLim != p->src)
        (p->finishMode == BCJ2_ENC_FINISH_MODE_CONTINUE)
          The encoder have found e8/e9/0f_8x marker,
          and p->src points to last byte of that marker,
          Bcj2Enc_Encode_2() needs more input data to get totally
          5 bytes (last byte of marker and 32-bit branch offset)
          as continuous array starting from p->src.
        (p->srcLim - p->src < 5) requirement is met after exit.
          So non-processed resedue from p->src to p->srcLim is always less than 5 bytes.
      }
    }
  }
*/

Z7_NO_INLINE
static void Bcj2Enc_Encode_2(CBcj2Enc *p)
{
  if (!p->isFlushState)
  {
    const Byte *src;
    UInt32 v;
    {
      const unsigned state = p->state;
      if (BCJ2_IS_32BIT_STREAM(state))
      {
        Byte *cur = p->bufs[state];
        if (cur == p->lims[state])
          return;
        SetBe32a(cur, p->tempTarget)
        p->bufs[state] = cur + 4;
      }
    }
    p->state = BCJ2_ENC_STATE_ORIG; // for main reason of exit
    src = p->src;
    v = p->context;
    
    // #define WRITE_CONTEXT  p->context = v; // for marker version
    #define WRITE_CONTEXT           p->context = (Byte)v;
    #define WRITE_CONTEXT_AND_SRC   p->src = src;  WRITE_CONTEXT

    for (;;)
    {
      // const Byte *src;
      // UInt32 v;
      CBcj2Enc_ip_unsigned ip;
      if (p->range < kTopValue)
      {
        // to reduce register pressure and code size: we save and restore local variables.
        WRITE_CONTEXT_AND_SRC
        if (Bcj2_RangeEnc_ShiftLow(p))
          return;
        p->range <<= 8;
        src = p->src;
        v = p->context;
      }
      // src = p->src;
      // #define MARKER_FLAG  ((UInt32)1 << 17)
      // if ((v & MARKER_FLAG) == 0) // for marker version
      {
        const Byte *srcLim;
        Byte *dest = p->bufs[BCJ2_STREAM_MAIN];
        {
          const SizeT remSrc = (SizeT)(p->srcLim - src);
          SizeT rem = (SizeT)(p->lims[BCJ2_STREAM_MAIN] - dest);
          if (rem >= remSrc)
            rem = remSrc;
          srcLim = src + rem;
        }
        /* p->context contains context of previous byte:
           bits [0 : 7]  : src[-1], if (src) was changed in this call
           bits [8 : 31] : are undefined for non-marker version
        */
        // v = p->context;
        #define NUM_SHIFT_BITS  24
        #define CONV_FLAG  ((UInt32)1 << 16)
        #define ONE_ITER { \
          b = src[0]; \
          *dest++ = (Byte)b; \
          v = (v << NUM_SHIFT_BITS) | b; \
          if (((b + (0x100 - 0xe8)) & 0xfe) == 0) break; \
          if (((v - (((UInt32)0x0f << (NUM_SHIFT_BITS)) + 0x80)) & \
              ((((UInt32)1 << (4 + NUM_SHIFT_BITS)) - 0x1) << 4)) == 0) break; \
          src++; if (src == srcLim) { break; } }

        if (src != srcLim)
        for (;;)
        {
          /* clang can generate ineffective code with setne instead of two jcc instructions.
             we can use 2 iterations and external (unsigned b) to avoid that ineffective code genaration. */
          unsigned b;
          ONE_ITER
          ONE_ITER
        }
        
        ip = p->ip64 + (CBcj2Enc_ip_unsigned)(SizeT)(dest - p->bufs[BCJ2_STREAM_MAIN]);
        p->bufs[BCJ2_STREAM_MAIN] = dest;
        p->ip64 = ip;

        if (src == srcLim)
        {
          WRITE_CONTEXT_AND_SRC
          if (src != p->srcLim)
          {
            p->state = BCJ2_STREAM_MAIN;
            return;
          }
          /* (p->src == p->srcLim)
          (p->state == BCJ2_ENC_STATE_ORIG) */
          if (p->finishMode != BCJ2_ENC_FINISH_MODE_END_STREAM)
            return;
          /* (p->finishMode == BCJ2_ENC_FINISH_MODE_END_STREAM */
          // (p->flushRem == 5);
          p->isFlushState = 1;
          break;
        }
        src++;
        // p->src = src;
      }
      // ip = p->ip; // for marker version
      /* marker was found */
      /* (v) contains marker that was found:
           bits [NUM_SHIFT_BITS : NUM_SHIFT_BITS + 7]
                         : value of src[-2] : xx/xx/0f
           bits [0 : 7]  : value of src[-1] : e8/e9/8x
      */
      {
        {
        #if NUM_SHIFT_BITS != 24
          v &= ~(UInt32)CONV_FLAG;
        #endif
          // UInt32 relat = 0;
          if ((SizeT)(p->srcLim - src) >= 4)
          {
            /*
            if (relat != 0 || (Byte)v != 0xe8)
            BoolInt isBigOffset = True;
            */
            const UInt32 relat = GetUi32(src);
            /*
            #define EXCLUDE_FLAG  ((UInt32)1 << 4)
            #define NEED_CONVERT(rel) ((((rel) + EXCLUDE_FLAG) & (0 - EXCLUDE_FLAG * 2)) != 0)
            if (p->relatExcludeBits != 0)
            {
              const UInt32 flag = (UInt32)1 << (p->relatExcludeBits - 1);
              isBigOffset = (((relat + flag) & (0 - flag * 2)) != 0);
            }
            // isBigOffset = False; // for debug
            */
            ip -= p->fileIp64;
            // Use the following if check, if (ip) is 64-bit:
            if (ip > (((v + 0x20) >> 5) & 1))  // 23.00 : we eliminate milti-block overlap for (Of 80) and (e8/e9)
            if ((CBcj2Enc_ip_unsigned)((CBcj2Enc_ip_signed)ip + 4 + (Int32)relat) <= p->fileSize64_minus1)
            if (((UInt32)(relat + p->relatLimit) >> 1) < p->relatLimit)
              v |= CONV_FLAG;
          }
          else if (p->finishMode == BCJ2_ENC_FINISH_MODE_CONTINUE)
          {
            // (p->srcLim - src < 4)
            // /*
            // for non-marker version
            p->ip64--; // p->ip = ip - 1;
            p->bufs[BCJ2_STREAM_MAIN]--;
            src--;
            v >>= NUM_SHIFT_BITS;
            // (0 < p->srcLim - p->src <= 4)
            // */
            // v |= MARKER_FLAG; // for marker version
            /* (p->state == BCJ2_ENC_STATE_ORIG) */
            WRITE_CONTEXT_AND_SRC
            return;
          }
          {
            const unsigned c = ((v + 0x17) >> 6) & 1;
            CBcj2Prob *prob = p->probs + (unsigned)
                (((0 - c) & (Byte)(v >> NUM_SHIFT_BITS)) + c + ((v >> 5) & 1));
            /*
                ((Byte)v == 0xe8 ? 2 + ((Byte)(v >> 8)) :
                ((Byte)v < 0xe8 ? 0 : 1));  // ((v >> 5) & 1));
            */
            const unsigned ttt = *prob;
            const UInt32 bound = (p->range >> kNumBitModelTotalBits) * ttt;
            if ((v & CONV_FLAG) == 0)
            {
              // static int yyy = 0; yyy++; printf("\n!needConvert = %d\n", yyy);
              // v = (Byte)v; // for marker version
              p->range = bound;
              *prob = (CBcj2Prob)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
              // WRITE_CONTEXT_AND_SRC
              continue;
            }
            p->low += bound;
            p->range -= bound;
            *prob = (CBcj2Prob)(ttt - (ttt >> kNumMoveBits));
          }
          // p->context = src[3];
          {
            // const unsigned cj = ((Byte)v == 0xe8 ? BCJ2_STREAM_CALL : BCJ2_STREAM_JUMP);
            const unsigned cj = (((v + 0x57) >> 6) & 1) + BCJ2_STREAM_CALL;
            ip = p->ip64;
            v = GetUi32(src); // relat
            ip += 4;
            p->ip64 = ip;
            src += 4;
            // p->src = src;
            {
              const UInt32 absol = (UInt32)ip + v;
              Byte *cur = p->bufs[cj];
              v >>= 24;
              // WRITE_CONTEXT
              if (cur == p->lims[cj])
              {
                p->state = cj;
                p->tempTarget = absol;
                WRITE_CONTEXT_AND_SRC
                return;
              }
              SetBe32a(cur, absol)
              p->bufs[cj] = cur + 4;
            }
          }
        }
      }
    } // end of loop
  }

  for (; p->flushRem != 0; p->flushRem--)
    if (Bcj2_RangeEnc_ShiftLow(p))
      return;
  p->state = BCJ2_ENC_STATE_FINISHED;
}


/*
BCJ2 encoder needs look ahead for up to 4 bytes in (src) buffer.
So base function Bcj2Enc_Encode_2()
  in BCJ2_ENC_FINISH_MODE_CONTINUE mode can return with
  (p->state == BCJ2_ENC_STATE_ORIG && p->src < p->srcLim)
Bcj2Enc_Encode() solves that look ahead problem by using p->temp[] buffer.
  so if (p->state == BCJ2_ENC_STATE_ORIG) after Bcj2Enc_Encode(),
    then (p->src == p->srcLim).
  And the caller's code is simpler with Bcj2Enc_Encode().
*/

Z7_NO_INLINE
void Bcj2Enc_Encode(CBcj2Enc *p)
{
  PRF2("\n----")
  if (p->tempPos != 0)
  {
    /* extra: number of bytes that were copied from (src) to (temp) buffer in this call */
    unsigned extra = 0;
    /* We will touch only minimal required number of bytes in input (src) stream.
       So we will add input bytes from (src) stream to temp[] with step of 1 byte.
       We don't add new bytes to temp[] before Bcj2Enc_Encode_2() call
         in first loop iteration because
         - previous call of Bcj2Enc_Encode() could use another (finishMode),
         - previous call could finish with (p->state != BCJ2_ENC_STATE_ORIG).
       the case with full temp[] buffer (p->tempPos == 4) is possible here.
    */
    for (;;)
    {
      // (0 < p->tempPos <= 5) // in non-marker version
      /* p->src : the current src data position including extra bytes
                  that were copied to temp[] buffer in this call */
      const Byte *src = p->src;
      const Byte *srcLim = p->srcLim;
      const EBcj2Enc_FinishMode finishMode = p->finishMode;
      if (src != srcLim)
      {
        /* if there are some src data after the data copied to temp[],
           then we use MODE_CONTINUE for temp data */
        p->finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
      }
      p->src = p->temp;
      p->srcLim = p->temp + p->tempPos;
      PRF2("    ")
      Bcj2Enc_Encode_2(p);
      {
        const unsigned num = (unsigned)(p->src - p->temp);
        const unsigned tempPos = p->tempPos - num;
        unsigned i;
        p->tempPos = tempPos;
        for (i = 0; i < tempPos; i++)
          p->temp[i] = p->temp[(SizeT)i + num];
        // tempPos : number of bytes in temp buffer
        p->src = src;
        p->srcLim = srcLim;
        p->finishMode = finishMode;
        if (p->state != BCJ2_ENC_STATE_ORIG)
        {
          // (p->tempPos <= 4) // in non-marker version
          /* if (the reason of exit from Bcj2Enc_Encode_2()
                 is not BCJ2_ENC_STATE_ORIG),
             then we exit from Bcj2Enc_Encode() with same reason */
          // optional code begin : we rollback (src) and tempPos, if it's possible:
          if (extra >= tempPos)
            extra = tempPos;
          p->src = src - extra;
          p->tempPos = tempPos - extra;
          // optional code end : rollback of (src) and tempPos
          return;
        }
        /* (p->tempPos <= 4)
           (p->state == BCJ2_ENC_STATE_ORIG)
             so encoder needs more data than in temp[] */
        if (src == srcLim)
          return; // src buffer has no more input data.
        /* (src != srcLim)
           so we can provide more input data from src for Bcj2Enc_Encode_2() */
        if (extra >= tempPos)
        {
          /* (extra >= tempPos) means that temp buffer contains
             only data from src buffer of this call.
             So now we can encode without temp buffer */
          p->src = src - tempPos; // rollback (src)
          p->tempPos = 0;
          break;
        }
        // we append one additional extra byte from (src) to temp[] buffer:
        p->temp[tempPos] = *src;
        p->tempPos = tempPos + 1;
        // (0 < p->tempPos <= 5) // in non-marker version
        p->src = src + 1;
        extra++;
      }
    }
  }

  PRF2("++++")
  // (p->tempPos == 0)
  Bcj2Enc_Encode_2(p);
  PRF2("====")
  
  if (p->state == BCJ2_ENC_STATE_ORIG)
  {
    const Byte *src = p->src;
    const Byte *srcLim = p->srcLim;
    const unsigned rem = (unsigned)(srcLim - src);
    /* (rem <= 4) here.
       if (p->src != p->srcLim), then
         - we copy non-processed bytes from (p->src) to temp[] buffer,
         - we set p->src equal to p->srcLim.
    */
    if (rem)
    {
      unsigned i = 0;
      p->src = srcLim;
      p->tempPos = rem;
      // (0 < p->tempPos <= 4)
      do
        p->temp[i] = src[i];
      while (++i != rem);
    }
    // (p->tempPos <= 4)
    // (p->src == p->srcLim)
  }
}

#undef PRF2
#undef CONV_FLAG
#undef MARKER_FLAG
#undef WRITE_CONTEXT
#undef WRITE_CONTEXT_AND_SRC
#undef ONE_ITER
#undef NUM_SHIFT_BITS
#undef kTopValue
#undef kNumBitModelTotalBits
#undef kBitModelTotal
#undef kNumMoveBits

/* ================ unit: C/Blake2s.c ================ */
/* Blake2s.c -- BLAKE2sp Hash
2024-05-18 : Igor Pavlov : Public domain
2015-2019 : Samuel Neves : original code : CC0 1.0 Universal (CC0 1.0). */

// amalgamation: header emitted in prologue

// #include <stdio.h>
#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

/*
  if defined(__AVX512F__) && defined(__AVX512VL__)
  {
    we define Z7_BLAKE2S_USE_AVX512_ALWAYS,
    but the compiler can use avx512 for any code.
  }
  else if defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
    { we use avx512 only for sse* and avx* branches of code. }
*/
// #define Z7_BLAKE2S_USE_AVX512_ALWAYS // for debug

#if defined(__SSE2__)
    #define Z7_BLAKE2S_USE_VECTORS
#elif defined(MY_CPU_X86_OR_AMD64)
  #if  defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
    #define Z7_BLAKE2S_USE_VECTORS
  #endif
#endif

#ifdef Z7_BLAKE2S_USE_VECTORS

#define Z7_BLAKE2SP_USE_FUNCTIONS

//  define Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED, if CBlake2sp can be non aligned for 32-bytes.
// #define Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED

// SSSE3 : for _mm_shuffle_epi8 (pshufb) that improves the performance for 5-15%.
#if defined(__SSSE3__)
  #define Z7_BLAKE2S_USE_SSSE3
#elif  defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1500) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40300) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40000) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 20300) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1000)
  #define Z7_BLAKE2S_USE_SSSE3
#endif

#ifdef Z7_BLAKE2S_USE_SSSE3
/* SSE41 : for _mm_insert_epi32 (pinsrd)
  it can slightly reduce code size and improves the performance in some cases.
    it's used only for last 512-1024 bytes, if FAST versions (2 or 3) of vector algos are used.
    it can be used for all blocks in another algos (4+).
*/
#if defined(__SSE4_1__)
  #define Z7_BLAKE2S_USE_SSE41
#elif  defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1500) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40300) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40000) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 20300) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1000)
  #define Z7_BLAKE2S_USE_SSE41
#endif
#endif // SSSE3

#if defined(__GNUC__) || defined(__clang__)
#if defined(Z7_BLAKE2S_USE_AVX512_ALWAYS) && !(defined(__AVX512F__) && defined(__AVX512VL__))
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("avx512vl,avx512f")))
#else
  #if defined(Z7_BLAKE2S_USE_SSE41)
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("sse4.1")))
  #elif defined(Z7_BLAKE2S_USE_SSSE3)
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("ssse3")))
  #else
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("sse2")))
  #endif
#endif
#endif


#if defined(__AVX2__)
  #define Z7_BLAKE2S_USE_AVX2
#else
  #if    defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
      || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
      || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100)
    #define Z7_BLAKE2S_USE_AVX2
    #ifdef Z7_BLAKE2S_USE_AVX2
#if defined(Z7_BLAKE2S_USE_AVX512_ALWAYS) && !(defined(__AVX512F__) && defined(__AVX512VL__))
      #define BLAKE2S_ATTRIB_AVX2  __attribute__((__target__("avx512vl,avx512f")))
#else
      #define BLAKE2S_ATTRIB_AVX2  __attribute__((__target__("avx2")))
#endif
    #endif
  #elif  defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
      || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
    #if (Z7_MSC_VER_ORIGINAL == 1900)
      #pragma warning(disable : 4752) // found Intel(R) Advanced Vector Extensions; consider using /arch:AVX
    #endif
    #define Z7_BLAKE2S_USE_AVX2
  #endif
#endif

#ifdef Z7_BLAKE2S_USE_SSE41
#include <smmintrin.h> // SSE4.1
#elif defined(Z7_BLAKE2S_USE_SSSE3)
#include <tmmintrin.h> // SSSE3
#else
#include <emmintrin.h> // SSE2
#endif

#ifdef Z7_BLAKE2S_USE_AVX2
#include <immintrin.h>
#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif
#endif // avx2


#if defined(__AVX512F__) && defined(__AVX512VL__)
   // && defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL > 1930)
  #ifndef Z7_BLAKE2S_USE_AVX512_ALWAYS
  #define Z7_BLAKE2S_USE_AVX512_ALWAYS
  #endif
  // #pragma message ("=== Blake2s AVX512")
#endif


#define Z7_BLAKE2S_USE_V128_FAST
// for speed optimization for small messages:
// #define Z7_BLAKE2S_USE_V128_WAY2

#ifdef Z7_BLAKE2S_USE_AVX2

// for debug:
// gather is slow
// #define Z7_BLAKE2S_USE_GATHER

  #define   Z7_BLAKE2S_USE_AVX2_FAST
// for speed optimization for small messages:
//   #define   Z7_BLAKE2S_USE_AVX2_WAY2
//   #define   Z7_BLAKE2S_USE_AVX2_WAY4
#if defined(Z7_BLAKE2S_USE_AVX2_WAY2) || \
    defined(Z7_BLAKE2S_USE_AVX2_WAY4)
  #define   Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#endif
#endif

  #define Z7_BLAKE2SP_ALGO_DEFAULT    0
  #define Z7_BLAKE2SP_ALGO_SCALAR     1
#ifdef Z7_BLAKE2S_USE_V128_FAST
  #define Z7_BLAKE2SP_ALGO_V128_FAST  2
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_FAST
  #define Z7_BLAKE2SP_ALGO_V256_FAST  3
#endif
  #define Z7_BLAKE2SP_ALGO_V128_WAY1  4
#ifdef Z7_BLAKE2S_USE_V128_WAY2
  #define Z7_BLAKE2SP_ALGO_V128_WAY2  5
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
  #define Z7_BLAKE2SP_ALGO_V256_WAY2  6
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY4
  #define Z7_BLAKE2SP_ALGO_V256_WAY4  7
#endif

#endif // Z7_BLAKE2S_USE_VECTORS




#define BLAKE2S_FINAL_FLAG  (~(UInt32)0)
#define NSW                 Z7_BLAKE2SP_NUM_STRUCT_WORDS
#define SUPER_BLOCK_SIZE    (Z7_BLAKE2S_BLOCK_SIZE * Z7_BLAKE2SP_PARALLEL_DEGREE)
#define SUPER_BLOCK_MASK    (SUPER_BLOCK_SIZE - 1)

#define V_INDEX_0_0   0
#define V_INDEX_1_0   1
#define V_INDEX_2_0   2
#define V_INDEX_3_0   3
#define V_INDEX_0_1   4
#define V_INDEX_1_1   5
#define V_INDEX_2_1   6
#define V_INDEX_3_1   7
#define V_INDEX_0_2   8
#define V_INDEX_1_2   9
#define V_INDEX_2_2  10
#define V_INDEX_3_2  11
#define V_INDEX_0_3  12
#define V_INDEX_1_3  13
#define V_INDEX_2_3  14
#define V_INDEX_3_3  15
#define V_INDEX_4_0   0
#define V_INDEX_5_0   1
#define V_INDEX_6_0   2
#define V_INDEX_7_0   3
#define V_INDEX_7_1   4
#define V_INDEX_4_1   5
#define V_INDEX_5_1   6
#define V_INDEX_6_1   7
#define V_INDEX_6_2   8
#define V_INDEX_7_2   9
#define V_INDEX_4_2  10
#define V_INDEX_5_2  11
#define V_INDEX_5_3  12
#define V_INDEX_6_3  13
#define V_INDEX_7_3  14
#define V_INDEX_4_3  15

#define V(row, col)  v[V_INDEX_ ## row ## _ ## col]

#define k_Blake2s_IV_0  0x6A09E667UL
#define k_Blake2s_IV_1  0xBB67AE85UL
#define k_Blake2s_IV_2  0x3C6EF372UL
#define k_Blake2s_IV_3  0xA54FF53AUL
#define k_Blake2s_IV_4  0x510E527FUL
#define k_Blake2s_IV_5  0x9B05688CUL
#define k_Blake2s_IV_6  0x1F83D9ABUL
#define k_Blake2s_IV_7  0x5BE0CD19UL

#define KIV(n)  (k_Blake2s_IV_## n)

#ifdef Z7_BLAKE2S_USE_VECTORS
MY_ALIGN(16)
static const UInt32 k_Blake2s_IV[8] =
{
  KIV(0), KIV(1), KIV(2), KIV(3), KIV(4), KIV(5), KIV(6), KIV(7)
};
#endif

#define STATE_T(s)        ((s) + 8)
#define STATE_F(s)        ((s) + 10)

#ifdef Z7_BLAKE2S_USE_VECTORS

#define LOAD_128(p)    _mm_load_si128 ((const __m128i *)(const void *)(p))
#define LOADU_128(p)   _mm_loadu_si128((const __m128i *)(const void *)(p))
#ifdef Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED
  // here we use unaligned load and stores
  // use this branch if CBlake2sp can be unaligned for 16 bytes
  #define STOREU_128(p, r)  _mm_storeu_si128((__m128i *)(void *)(p), r)
  #define LOAD_128_FROM_STRUCT(p)     LOADU_128(p)
  #define STORE_128_TO_STRUCT(p, r)   STOREU_128(p, r)
#else
  // here we use aligned load and stores
  // use this branch if CBlake2sp is aligned for 16 bytes
  #define STORE_128(p, r)  _mm_store_si128((__m128i *)(void *)(p), r)
  #define LOAD_128_FROM_STRUCT(p)     LOAD_128(p)
  #define STORE_128_TO_STRUCT(p, r)   STORE_128(p, r)
#endif

#endif // Z7_BLAKE2S_USE_VECTORS


#if 0
static void PrintState(const UInt32 *s, unsigned num)
{
  unsigned i;
  printf("\n");
  for (i = 0; i < num; i++)
    printf(" %08x", (unsigned)s[i]);
}
static void PrintStates2(const UInt32 *s, unsigned x, unsigned y)
{
  unsigned i;
  for (i = 0; i < y; i++)
    PrintState(s + i * x, x);
  printf("\n");
}
#endif


#define REP8_MACRO(m)  { m(0) m(1) m(2) m(3) m(4) m(5) m(6) m(7) }

#define BLAKE2S_NUM_ROUNDS  10

#if defined(Z7_BLAKE2S_USE_VECTORS)
#define ROUNDS_LOOP(mac) \
  { unsigned r; for (r = 0; r < BLAKE2S_NUM_ROUNDS; r++) mac(r) }
#endif
/*
#define ROUNDS_LOOP_2(mac) \
  { unsigned r; for (r = 0; r < BLAKE2S_NUM_ROUNDS; r += 2) { mac(r) mac(r + 1) } }
*/
#if 0 || 1 && !defined(Z7_BLAKE2S_USE_VECTORS)
#define ROUNDS_LOOP_UNROLLED(m) \
  { m(0) m(1) m(2) m(3) m(4) m(5) m(6) m(7) m(8) m(9) }
#endif

#define SIGMA_TABLE(M) \
  M(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 ), \
  M( 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 ), \
  M( 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 ), \
  M(  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 ), \
  M(  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 ), \
  M(  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 ), \
  M( 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 ), \
  M( 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 ), \
  M(  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 ), \
  M( 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 )

#define SIGMA_TABLE_MULT(m, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
  { a0*m,a1*m,a2*m,a3*m,a4*m,a5*m,a6*m,a7*m,a8*m,a9*m,a10*m,a11*m,a12*m,a13*m,a14*m,a15*m }
#define SIGMA_TABLE_MULT_4( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        SIGMA_TABLE_MULT(4, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)

// MY_ALIGN(32)
MY_ALIGN(16)
static const Byte k_Blake2s_Sigma_4[BLAKE2S_NUM_ROUNDS][16] =
  { SIGMA_TABLE(SIGMA_TABLE_MULT_4) };

#define GET_SIGMA_PTR(p, index) \
    ((const void *)((const Byte *)(const void *)(p) + (index)))

#define GET_STATE_TABLE_PTR_FROM_BYTE_POS(s, pos) \
    ((UInt32 *)(void *)((Byte *)(void *)(s) + (pos)))


#ifdef Z7_BLAKE2S_USE_VECTORS


#if 0
  // use loading constants from memory
  // is faster for some compilers.
  #define KK4(n)  KIV(n), KIV(n), KIV(n), KIV(n)
MY_ALIGN(64)
static const UInt32 k_Blake2s_IV_WAY4[]=
{
  KK4(0), KK4(1), KK4(2), KK4(3), KK4(4), KK4(5), KK4(6), KK4(7)
};
  #define GET_128_IV_WAY4(i)  LOAD_128(k_Blake2s_IV_WAY4 + 4 * (i))
#else
  // use constant generation:
  #define GET_128_IV_WAY4(i)  _mm_set1_epi32((Int32)KIV(i))
#endif


#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#define GET_CONST_128_FROM_ARRAY32(k) \
    _mm_set_epi32((Int32)(k)[3], (Int32)(k)[2], (Int32)(k)[1], (Int32)(k)[0])
#endif


#if 0
#define k_r8    _mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1)
#define k_r16   _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2)
#define k_inc   _mm_set_epi32(0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE)
#define k_iv0_128  GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 0)
#define k_iv4_128  GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 4)
#else
#if  defined(Z7_BLAKE2S_USE_SSSE3) && \
    !defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
MY_ALIGN(16) static const Byte k_r8_arr [16] = { 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8 ,13, 14, 15, 12 };
MY_ALIGN(16) static const Byte k_r16_arr[16] = { 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13 };
#define k_r8    LOAD_128(k_r8_arr)
#define k_r16   LOAD_128(k_r16_arr)
#endif
MY_ALIGN(16) static const UInt32 k_inc_arr[4] = { Z7_BLAKE2S_BLOCK_SIZE, 0, 0, 0 };
#define k_inc   LOAD_128(k_inc_arr)
#define k_iv0_128  LOAD_128(k_Blake2s_IV + 0)
#define k_iv4_128  LOAD_128(k_Blake2s_IV + 4)
#endif


#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW

#ifdef Z7_BLAKE2S_USE_AVX2
#if defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION < 80000)
  #define MY_mm256_set_m128i(hi, lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 1)
#else
  #define MY_mm256_set_m128i  _mm256_set_m128i
#endif

#define SET_FROM_128(a)  MY_mm256_set_m128i(a, a)

#ifndef Z7_BLAKE2S_USE_AVX512_ALWAYS
MY_ALIGN(32) static const Byte k_r8_arr_256 [32] =
{
  1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8 ,13, 14, 15, 12,
  1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8 ,13, 14, 15, 12
};
MY_ALIGN(32) static const Byte k_r16_arr_256[32] =
{
  2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13,
  2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13
};
#define k_r8_256    LOAD_256(k_r8_arr_256)
#define k_r16_256   LOAD_256(k_r16_arr_256)
#endif

// #define k_r8_256    SET_FROM_128(_mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1))
// #define k_r16_256   SET_FROM_128(_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2))
// #define k_inc_256   SET_FROM_128(_mm_set_epi32(0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE))
// #define k_iv0_256   SET_FROM_128(GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 0))
#define k_iv4_256   SET_FROM_128(GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 4))
#endif // Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#endif


/*
IPC(TP) ports:
1 p__5  : skl-      : SSE   : shufps  : _mm_shuffle_ps
2 p_15  : icl+
1 p__5  : nhm-bdw   : SSE   : xorps   : _mm_xor_ps
3 p015  : skl+

3 p015              : SSE2  : pxor    : _mm_xor_si128
2 p_15:   snb-bdw   : SSE2  : padd    : _mm_add_epi32
2 p0_5:   mrm-wsm   :
3 p015  : skl+

2 p_15  : ivb-,icl+ : SSE2  : punpcklqdq, punpckhqdq, punpckldq, punpckhdq
2 p_15  :           : SSE2  : pshufd  : _mm_shuffle_epi32
2 p_15  :           : SSE2  : pshuflw : _mm_shufflelo_epi16
2 p_15  :           : SSE2  : psrldq  :
2 p_15  :           : SSE3  : pshufb  : _mm_shuffle_epi8
2 p_15  :           : SSE4  : pblendw : _mm_blend_epi16
1 p__5  : hsw-skl   : *

1 p0                : SSE2  : pslld (i8) : _mm_slli_si128
2 p01   : skl+      :

2 p_15  : ivb-      : SSE3  : palignr
1 p__5  : hsw+

2 p_15 + p23 : ivb-, icl+ : SSE4   : pinsrd  : _mm_insert_epi32(xmm, m32, i8)
1 p__5 + p23 : hsw-skl
1 p_15 + p5  : ivb-, ice+ : SSE4   : pinsrd  : _mm_insert_epi32(xmm, r32, i8)
0.5    2*p5  : hsw-skl

2 p23               : SSE2   : movd (m32)
3 p23A  : adl       :
1 p5:               : SSE2   : movd (r32)
*/

#if 0 && defined(__XOP__)
// we must debug and test __XOP__ instruction
#include <x86intrin.h>
#include <ammintrin.h>
    #define LOAD_ROTATE_CONSTS
    #define MM_ROR_EPI32(r, c)  _mm_roti_epi32(r, -(c))
    #define Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED
#elif 1 && defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
    #define LOAD_ROTATE_CONSTS
    #define MM_ROR_EPI32(r, c)  _mm_ror_epi32(r, c)
    #define Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED
#else

// MSVC_1937+ uses "orps" instruction for _mm_or_si128().
// But "orps" has low throughput: TP=1 for bdw-nhm.
// So it can be better to use _mm_add_epi32()/"paddd" (TP=2 for bdw-nhm) instead of "xorps".
// But "orps" is fast for modern cpus (skl+).
// So we are default with "or" version:
#if 0 || 0 && defined(Z7_MSC_VER_ORIGINAL) && Z7_MSC_VER_ORIGINAL > 1937
  // minor optimization for some old cpus, if "xorps" is slow.
  #define MM128_EPI32_OR_or_ADD  _mm_add_epi32
#else
  #define MM128_EPI32_OR_or_ADD  _mm_or_si128
#endif

  #define MM_ROR_EPI32_VIA_SHIFT(r, c)( \
    MM128_EPI32_OR_or_ADD( \
      _mm_srli_epi32((r), (c)), \
      _mm_slli_epi32((r), 32-(c))))
  #if defined(Z7_BLAKE2S_USE_SSSE3) || defined(Z7_BLAKE2S_USE_SSE41)
    #define LOAD_ROTATE_CONSTS \
      const __m128i  r8 = k_r8; \
      const __m128i r16 = k_r16;
    #define MM_ROR_EPI32(r, c) ( \
      ( 8==(c)) ? _mm_shuffle_epi8(r,r8) \
    : (16==(c)) ? _mm_shuffle_epi8(r,r16) \
    : MM_ROR_EPI32_VIA_SHIFT(r, c))
  #else
    #define LOAD_ROTATE_CONSTS
    #define  MM_ROR_EPI32(r, c) ( \
      (16==(c)) ? _mm_shufflehi_epi16(_mm_shufflelo_epi16(r, 0xb1), 0xb1) \
    : MM_ROR_EPI32_VIA_SHIFT(r, c))
  #endif
#endif

/*
we have 3 main ways to load 4 32-bit integers to __m128i:
  1) SSE2:  _mm_set_epi32()
  2) SSE2:  _mm_unpacklo_epi64() / _mm_unpacklo_epi32 / _mm_cvtsi32_si128()
  3) SSE41: _mm_insert_epi32() and _mm_cvtsi32_si128()
good compiler for _mm_set_epi32() generates these instructions:
{
  movd xmm, [m32]; vpunpckldq; vpunpckldq; vpunpcklqdq;
}
good new compiler generates one instruction
{
  for _mm_insert_epi32()  : { pinsrd xmm, [m32], i }
  for _mm_cvtsi32_si128() : { movd xmm, [m32] }
}
but vc2010 generates slow pair of instructions:
{
  for _mm_insert_epi32()  : { mov r32, [m32];  pinsrd xmm, r32, i  }
  for _mm_cvtsi32_si128() : { mov r32, [m32];  movd  xmm, r32 }
}
_mm_insert_epi32() (pinsrd) code reduces xmm register pressure
in comparison with _mm_set_epi32() (movd + vpunpckld) code.
Note that variant with "movd xmm, r32" can be more slow,
but register pressure can be more important.
So we can force to "pinsrd" always.
*/
// #if !defined(Z7_MSC_VER_ORIGINAL) || Z7_MSC_VER_ORIGINAL > 1600 || defined(MY_CPU_X86)
#ifdef Z7_BLAKE2S_USE_SSE41
  /* _mm_set_epi32() can be more effective for GCC and CLANG
     _mm_insert_epi32()  is more effective for MSVC */
  #if 0 || 1 && defined(Z7_MSC_VER_ORIGINAL)
    #define Z7_BLAKE2S_USE_INSERT_INSTRUCTION
  #endif
#endif // USE_SSE41
// #endif

#ifdef Z7_BLAKE2S_USE_INSERT_INSTRUCTION
  // for SSE4.1
#define MM_LOAD_EPI32_FROM_4_POINTERS(p0, p1, p2, p3)  \
    _mm_insert_epi32( \
    _mm_insert_epi32( \
    _mm_insert_epi32( \
    _mm_cvtsi32_si128( \
        *(const Int32 *)p0), \
        *(const Int32 *)p1, 1), \
        *(const Int32 *)p2, 2), \
        *(const Int32 *)p3, 3)
#elif 0 || 1 && defined(Z7_MSC_VER_ORIGINAL)
/* MSVC 1400 implements _mm_set_epi32() via slow memory write/read.
   Also _mm_unpacklo_epi32 is more effective for another MSVC compilers.
   But _mm_set_epi32() is more effective for GCC and CLANG.
   So we use _mm_unpacklo_epi32 for MSVC only */
#define MM_LOAD_EPI32_FROM_4_POINTERS(p0, p1, p2, p3)  \
    _mm_unpacklo_epi64(  \
        _mm_unpacklo_epi32( _mm_cvtsi32_si128(*(const Int32 *)p0),  \
                            _mm_cvtsi32_si128(*(const Int32 *)p1)), \
        _mm_unpacklo_epi32( _mm_cvtsi32_si128(*(const Int32 *)p2),  \
                            _mm_cvtsi32_si128(*(const Int32 *)p3)))
#else
#define MM_LOAD_EPI32_FROM_4_POINTERS(p0, p1, p2, p3)  \
    _mm_set_epi32( \
        *(const Int32 *)p3, \
        *(const Int32 *)p2, \
        *(const Int32 *)p1, \
        *(const Int32 *)p0)
#endif

#define SET_ROW_FROM_SIGMA_BASE(input, i0, i1, i2, i3)  \
      MM_LOAD_EPI32_FROM_4_POINTERS( \
        GET_SIGMA_PTR(input, i0), \
        GET_SIGMA_PTR(input, i1), \
        GET_SIGMA_PTR(input, i2), \
        GET_SIGMA_PTR(input, i3))

#define SET_ROW_FROM_SIGMA(input, sigma_index)  \
        SET_ROW_FROM_SIGMA_BASE(input, \
            sigma[(sigma_index)        ], \
            sigma[(sigma_index) + 2 * 1], \
            sigma[(sigma_index) + 2 * 2], \
            sigma[(sigma_index) + 2 * 3]) \


#define ADD_128(a, b)   _mm_add_epi32(a, b)
#define XOR_128(a, b)   _mm_xor_si128(a, b)

#define D_ADD_128(dest, src)        dest = ADD_128(dest, src)
#define D_XOR_128(dest, src)        dest = XOR_128(dest, src)
#define D_ROR_128(dest, shift)      dest = MM_ROR_EPI32(dest, shift)
#define D_ADD_EPI64_128(dest, src)  dest = _mm_add_epi64(dest, src)


#define AXR(a, b, d, shift) \
    D_ADD_128(a, b); \
    D_XOR_128(d, a); \
    D_ROR_128(d, shift);

#define AXR2(a, b, c, d, input, sigma_index, shift1, shift2) \
    a = _mm_add_epi32 (a, SET_ROW_FROM_SIGMA(input, sigma_index)); \
    AXR(a, b, d, shift1) \
    AXR(c, d, b, shift2)

#define ROTATE_WORDS_TO_RIGHT(a, n) \
    a = _mm_shuffle_epi32(a, _MM_SHUFFLE((3+n)&3, (2+n)&3, (1+n)&3, (0+n)&3));

#define AXR4(a, b, c, d, input, sigma_index)  \
    AXR2(a, b, c, d, input, sigma_index,     16, 12) \
    AXR2(a, b, c, d, input, sigma_index + 1,  8,  7) \

#define RR2(a, b, c, d, input) \
  { \
    AXR4(a, b, c, d, input, 0) \
      ROTATE_WORDS_TO_RIGHT(b, 1) \
      ROTATE_WORDS_TO_RIGHT(c, 2) \
      ROTATE_WORDS_TO_RIGHT(d, 3) \
    AXR4(a, b, c, d, input, 8) \
      ROTATE_WORDS_TO_RIGHT(b, 3) \
      ROTATE_WORDS_TO_RIGHT(c, 2) \
      ROTATE_WORDS_TO_RIGHT(d, 1) \
  }


/*
Way1:
per 64 bytes block:
10 rounds * 4 iters * (7 + 2) = 360 cycles = if pslld TP=1
                    * (7 + 1) = 320 cycles = if pslld TP=2 (skl+)
additional operations per 7_op_iter :
4 movzx   byte mem
1 movd    mem
3 pinsrd  mem
1.5 pshufd
*/

static
#if 0 || 0 && (defined(Z7_BLAKE2S_USE_V128_WAY2) || \
               defined(Z7_BLAKE2S_USE_V256_WAY2))
  Z7_NO_INLINE
#else
  Z7_FORCE_INLINE
#endif
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2s_Compress_V128_Way1(UInt32 * const s, const Byte * const input)
{
  __m128i a, b, c, d;
  __m128i f0, f1;

  LOAD_ROTATE_CONSTS
  d = LOAD_128_FROM_STRUCT(STATE_T(s));
  c = k_iv0_128;
  a = f0 = LOAD_128_FROM_STRUCT(s);
  b = f1 = LOAD_128_FROM_STRUCT(s + 4);
  D_ADD_EPI64_128(d, k_inc);
  STORE_128_TO_STRUCT (STATE_T(s), d);
  D_XOR_128(d, k_iv4_128);

#define RR(r) { const Byte * const sigma = k_Blake2s_Sigma_4[r]; \
      RR2(a, b, c, d, input) }

  ROUNDS_LOOP(RR)
#undef RR

  STORE_128_TO_STRUCT(s    , XOR_128(f0, XOR_128(a, c)));
  STORE_128_TO_STRUCT(s + 4, XOR_128(f1, XOR_128(b, d)));
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_V128_Way1(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  do
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Blake2s_Compress_V128_Way1(s, data);
    data += Z7_BLAKE2S_BLOCK_SIZE;
    pos  += Z7_BLAKE2S_BLOCK_SIZE;
    pos &= SUPER_BLOCK_MASK;
  }
  while (data != end);
}


#if defined(Z7_BLAKE2S_USE_V128_WAY2) || \
    defined(Z7_BLAKE2S_USE_AVX2_WAY2)
#if 1
  #define Z7_BLAKE2S_CompressSingleBlock(s, data) \
    Blake2sp_Compress2_V128_Way1(s, data, \
        (const Byte *)(const void *)(data) + Z7_BLAKE2S_BLOCK_SIZE)
#else
  #define Z7_BLAKE2S_CompressSingleBlock  Blake2s_Compress_V128_Way1
#endif
#endif


#if (defined(Z7_BLAKE2S_USE_AVX2_WAY_SLOW) || \
     defined(Z7_BLAKE2S_USE_V128_WAY2)) && \
    !defined(Z7_BLAKE2S_USE_GATHER)
#define AXR2_LOAD_INDEXES(sigma_index) \
      const unsigned i0 = sigma[(sigma_index)]; \
      const unsigned i1 = sigma[(sigma_index) + 2 * 1]; \
      const unsigned i2 = sigma[(sigma_index) + 2 * 2]; \
      const unsigned i3 = sigma[(sigma_index) + 2 * 3]; \

#define SET_ROW_FROM_SIGMA_W(input) \
    SET_ROW_FROM_SIGMA_BASE(input, i0, i1, i2, i3)
#endif


#ifdef Z7_BLAKE2S_USE_V128_WAY2

#if 1 || !defined(Z7_BLAKE2S_USE_SSE41)
/* we use SET_ROW_FROM_SIGMA_BASE, that uses
   (SSE4) _mm_insert_epi32(), if Z7_BLAKE2S_USE_INSERT_INSTRUCTION is defined
   (SSE2) _mm_set_epi32()
   MSVC can be faster for this branch:
*/
#define AXR2_W(sigma_index, shift1, shift2) \
  { \
    AXR2_LOAD_INDEXES(sigma_index) \
    a0 = _mm_add_epi32(a0, SET_ROW_FROM_SIGMA_W(data)); \
    a1 = _mm_add_epi32(a1, SET_ROW_FROM_SIGMA_W(data + Z7_BLAKE2S_BLOCK_SIZE)); \
    AXR(a0, b0, d0, shift1) \
    AXR(a1, b1, d1, shift1) \
    AXR(c0, d0, b0, shift2) \
    AXR(c1, d1, b1, shift2) \
  }
#else
/* we use interleaved _mm_insert_epi32():
   GCC can be faster for this branch:
*/
#define AXR2_W_PRE_INSERT(sigma_index, i) \
  { const unsigned ii = sigma[(sigma_index) + i * 2]; \
    t0 = _mm_insert_epi32(t0, *(const Int32 *)GET_SIGMA_PTR(data, ii),                      i); \
    t1 = _mm_insert_epi32(t1, *(const Int32 *)GET_SIGMA_PTR(data, Z7_BLAKE2S_BLOCK_SIZE + ii), i); \
  }
#define AXR2_W(sigma_index, shift1, shift2) \
  { __m128i t0, t1; \
    { const unsigned ii = sigma[sigma_index]; \
      t0 = _mm_cvtsi32_si128(*(const Int32 *)GET_SIGMA_PTR(data, ii)); \
      t1 = _mm_cvtsi32_si128(*(const Int32 *)GET_SIGMA_PTR(data, Z7_BLAKE2S_BLOCK_SIZE + ii)); \
    } \
    AXR2_W_PRE_INSERT(sigma_index, 1) \
    AXR2_W_PRE_INSERT(sigma_index, 2) \
    AXR2_W_PRE_INSERT(sigma_index, 3) \
    a0 = _mm_add_epi32(a0, t0); \
    a1 = _mm_add_epi32(a1, t1); \
    AXR(a0, b0, d0, shift1) \
    AXR(a1, b1, d1, shift1) \
    AXR(c0, d0, b0, shift2) \
    AXR(c1, d1, b1, shift2) \
  }
#endif


#define AXR4_W(sigma_index) \
    AXR2_W(sigma_index,     16, 12) \
    AXR2_W(sigma_index + 1,  8,  7) \

#define WW(r) \
  { const Byte * const sigma = k_Blake2s_Sigma_4[r]; \
    AXR4_W(0) \
      ROTATE_WORDS_TO_RIGHT(b0, 1) \
      ROTATE_WORDS_TO_RIGHT(b1, 1) \
      ROTATE_WORDS_TO_RIGHT(c0, 2) \
      ROTATE_WORDS_TO_RIGHT(c1, 2) \
      ROTATE_WORDS_TO_RIGHT(d0, 3) \
      ROTATE_WORDS_TO_RIGHT(d1, 3) \
    AXR4_W(8) \
      ROTATE_WORDS_TO_RIGHT(b0, 3) \
      ROTATE_WORDS_TO_RIGHT(b1, 3) \
      ROTATE_WORDS_TO_RIGHT(c0, 2) \
      ROTATE_WORDS_TO_RIGHT(c1, 2) \
      ROTATE_WORDS_TO_RIGHT(d0, 1) \
      ROTATE_WORDS_TO_RIGHT(d1, 1) \
  }


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_V128_Way2(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  end -= Z7_BLAKE2S_BLOCK_SIZE;

  if (data != end)
  {
    LOAD_ROTATE_CONSTS
    do
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
      __m128i a0, b0, c0, d0;
      __m128i a1, b1, c1, d1;
      {
        const __m128i inc = k_inc;
        const __m128i temp = k_iv4_128;
        d0 = LOAD_128_FROM_STRUCT (STATE_T(s));
        d1 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW));
        D_ADD_EPI64_128(d0, inc);
        D_ADD_EPI64_128(d1, inc);
        STORE_128_TO_STRUCT (STATE_T(s      ), d0);
        STORE_128_TO_STRUCT (STATE_T(s + NSW), d1);
        D_XOR_128(d0, temp);
        D_XOR_128(d1, temp);
      }
      c1 = c0 = k_iv0_128;
      a0 = LOAD_128_FROM_STRUCT(s);
      b0 = LOAD_128_FROM_STRUCT(s + 4);
      a1 = LOAD_128_FROM_STRUCT(s + NSW);
      b1 = LOAD_128_FROM_STRUCT(s + NSW + 4);
      
      ROUNDS_LOOP (WW)

#undef WW
      
      D_XOR_128(a0, c0);
      D_XOR_128(b0, d0);
      D_XOR_128(a1, c1);
      D_XOR_128(b1, d1);
      
      D_XOR_128(a0, LOAD_128_FROM_STRUCT(s));
      D_XOR_128(b0, LOAD_128_FROM_STRUCT(s + 4));
      D_XOR_128(a1, LOAD_128_FROM_STRUCT(s + NSW));
      D_XOR_128(b1, LOAD_128_FROM_STRUCT(s + NSW + 4));
      
      STORE_128_TO_STRUCT(s,           a0);
      STORE_128_TO_STRUCT(s + 4,       b0);
      STORE_128_TO_STRUCT(s + NSW,     a1);
      STORE_128_TO_STRUCT(s + NSW + 4, b1);
      
      data += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos  += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos &= SUPER_BLOCK_MASK;
    }
    while (data < end);
    if (data != end)
      return;
  }
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Z7_BLAKE2S_CompressSingleBlock(s, data);
  }
}
#endif // Z7_BLAKE2S_USE_V128_WAY2


#ifdef Z7_BLAKE2S_USE_V128_WAY2
  #define Z7_BLAKE2S_Compress2_V128  Blake2sp_Compress2_V128_Way2
#else
  #define Z7_BLAKE2S_Compress2_V128  Blake2sp_Compress2_V128_Way1
#endif



#ifdef Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED
  #define ROT_128_8(x)    MM_ROR_EPI32(x, 8)
  #define ROT_128_16(x)   MM_ROR_EPI32(x, 16)
  #define ROT_128_7(x)    MM_ROR_EPI32(x, 7)
  #define ROT_128_12(x)   MM_ROR_EPI32(x, 12)
#else
#if defined(Z7_BLAKE2S_USE_SSSE3) || defined(Z7_BLAKE2S_USE_SSE41)
  #define ROT_128_8(x)    _mm_shuffle_epi8(x, r8)   // k_r8
  #define ROT_128_16(x)   _mm_shuffle_epi8(x, r16)  // k_r16
#else
  #define ROT_128_8(x)    MM_ROR_EPI32_VIA_SHIFT(x, 8)
  #define ROT_128_16(x)   MM_ROR_EPI32_VIA_SHIFT(x, 16)
#endif
  #define ROT_128_7(x)    MM_ROR_EPI32_VIA_SHIFT(x, 7)
  #define ROT_128_12(x)   MM_ROR_EPI32_VIA_SHIFT(x, 12)
#endif


#if 1
// this branch can provide similar speed on x86* in most cases,
// because [base + index*4] provides same speed as [base + index].
// but some compilers can generate different code with this branch, that can be faster sometimes.
// this branch uses additional table of 10*16=160 bytes.
#define SIGMA_TABLE_MULT_16( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        SIGMA_TABLE_MULT(16, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)
MY_ALIGN(16)
static const Byte k_Blake2s_Sigma_16[BLAKE2S_NUM_ROUNDS][16] =
  { SIGMA_TABLE(SIGMA_TABLE_MULT_16) };
#define GET_SIGMA_PTR_128(r)  const Byte * const sigma = k_Blake2s_Sigma_16[r];
#define GET_SIGMA_VAL_128(n)  (sigma[n])
#else
#define GET_SIGMA_PTR_128(r)  const Byte * const sigma = k_Blake2s_Sigma_4[r];
#define GET_SIGMA_VAL_128(n)  (4 * (size_t)sigma[n])
#endif


#ifdef Z7_BLAKE2S_USE_AVX2_FAST
#if 1
#define SIGMA_TABLE_MULT_32( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        SIGMA_TABLE_MULT(32, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)
MY_ALIGN(64)
static const UInt16 k_Blake2s_Sigma_32[BLAKE2S_NUM_ROUNDS][16] =
  { SIGMA_TABLE(SIGMA_TABLE_MULT_32) };
#define GET_SIGMA_PTR_256(r)  const UInt16 * const sigma = k_Blake2s_Sigma_32[r];
#define GET_SIGMA_VAL_256(n)  (sigma[n])
#else
#define GET_SIGMA_PTR_256(r)  const Byte * const sigma = k_Blake2s_Sigma_4[r];
#define GET_SIGMA_VAL_256(n)  (8 * (size_t)sigma[n])
#endif
#endif // Z7_BLAKE2S_USE_AVX2_FAST


#define D_ROT_128_7(dest)     dest = ROT_128_7(dest)
#define D_ROT_128_8(dest)     dest = ROT_128_8(dest)
#define D_ROT_128_12(dest)    dest = ROT_128_12(dest)
#define D_ROT_128_16(dest)    dest = ROT_128_16(dest)

#define OP_L(a, i)   D_ADD_128 (V(a, 0), \
    LOAD_128((const Byte *)(w) + GET_SIGMA_VAL_128(2*(a)+(i))));

#define OP_0(a)   OP_L(a, 0)
#define OP_7(a)   OP_L(a, 1)

#define OP_1(a)   D_ADD_128 (V(a, 0), V(a, 1));
#define OP_2(a)   D_XOR_128 (V(a, 3), V(a, 0));
#define OP_4(a)   D_ADD_128 (V(a, 2), V(a, 3));
#define OP_5(a)   D_XOR_128 (V(a, 1), V(a, 2));

#define OP_3(a)   D_ROT_128_16 (V(a, 3));
#define OP_6(a)   D_ROT_128_12 (V(a, 1));
#define OP_8(a)   D_ROT_128_8  (V(a, 3));
#define OP_9(a)   D_ROT_128_7  (V(a, 1));


// for 32-bit x86 : interleave mode works slower, because of register pressure.

#if 0 || 1 && (defined(MY_CPU_X86) \
  || defined(__GNUC__) && !defined(__clang__))
// non-inteleaved version:
// is fast for x86 32-bit.
// is fast for GCC x86-64.

#define V4G(a) \
  OP_0 (a) \
  OP_1 (a) \
  OP_2 (a) \
  OP_3 (a) \
  OP_4 (a) \
  OP_5 (a) \
  OP_6 (a) \
  OP_7 (a) \
  OP_1 (a) \
  OP_2 (a) \
  OP_8 (a) \
  OP_4 (a) \
  OP_5 (a) \
  OP_9 (a) \

#define V4R \
{ \
  V4G (0) \
  V4G (1) \
  V4G (2) \
  V4G (3) \
  V4G (4) \
  V4G (5) \
  V4G (6) \
  V4G (7) \
}

#elif 0 || 1 && defined(MY_CPU_X86)

#define OP_INTER_2(op, a,b) \
  op (a) \
  op (b) \

#define V4G(a,b) \
  OP_INTER_2 (OP_0, a,b) \
  OP_INTER_2 (OP_1, a,b) \
  OP_INTER_2 (OP_2, a,b) \
  OP_INTER_2 (OP_3, a,b) \
  OP_INTER_2 (OP_4, a,b) \
  OP_INTER_2 (OP_5, a,b) \
  OP_INTER_2 (OP_6, a,b) \
  OP_INTER_2 (OP_7, a,b) \
  OP_INTER_2 (OP_1, a,b) \
  OP_INTER_2 (OP_2, a,b) \
  OP_INTER_2 (OP_8, a,b) \
  OP_INTER_2 (OP_4, a,b) \
  OP_INTER_2 (OP_5, a,b) \
  OP_INTER_2 (OP_9, a,b) \

#define V4R \
{ \
  V4G (0, 1) \
  V4G (2, 3) \
  V4G (4, 5) \
  V4G (6, 7) \
}

#else
// iterleave-4 version is fast for x64 (MSVC/CLANG)

#define OP_INTER_4(op, a,b,c,d) \
  op (a) \
  op (b) \
  op (c) \
  op (d) \

#define V4G(a,b,c,d) \
  OP_INTER_4 (OP_0, a,b,c,d) \
  OP_INTER_4 (OP_1, a,b,c,d) \
  OP_INTER_4 (OP_2, a,b,c,d) \
  OP_INTER_4 (OP_3, a,b,c,d) \
  OP_INTER_4 (OP_4, a,b,c,d) \
  OP_INTER_4 (OP_5, a,b,c,d) \
  OP_INTER_4 (OP_6, a,b,c,d) \
  OP_INTER_4 (OP_7, a,b,c,d) \
  OP_INTER_4 (OP_1, a,b,c,d) \
  OP_INTER_4 (OP_2, a,b,c,d) \
  OP_INTER_4 (OP_8, a,b,c,d) \
  OP_INTER_4 (OP_4, a,b,c,d) \
  OP_INTER_4 (OP_5, a,b,c,d) \
  OP_INTER_4 (OP_9, a,b,c,d) \

#define V4R \
{ \
  V4G (0, 1, 2, 3) \
  V4G (4, 5, 6, 7) \
}

#endif

#define V4_ROUND(r)  { GET_SIGMA_PTR_128(r); V4R }


#define V4_LOAD_MSG_1(w, m, i) \
{ \
  __m128i m0, m1, m2, m3; \
  __m128i t0, t1, t2, t3; \
  m0 = LOADU_128((m) + ((i) + 0 * 4) * 16); \
  m1 = LOADU_128((m) + ((i) + 1 * 4) * 16); \
  m2 = LOADU_128((m) + ((i) + 2 * 4) * 16); \
  m3 = LOADU_128((m) + ((i) + 3 * 4) * 16); \
  t0 = _mm_unpacklo_epi32(m0, m1); \
  t1 = _mm_unpackhi_epi32(m0, m1); \
  t2 = _mm_unpacklo_epi32(m2, m3); \
  t3 = _mm_unpackhi_epi32(m2, m3); \
  w[(i) * 4 + 0] = _mm_unpacklo_epi64(t0, t2); \
  w[(i) * 4 + 1] = _mm_unpackhi_epi64(t0, t2); \
  w[(i) * 4 + 2] = _mm_unpacklo_epi64(t1, t3); \
  w[(i) * 4 + 3] = _mm_unpackhi_epi64(t1, t3); \
}

#define V4_LOAD_MSG(w, m) \
{ \
  V4_LOAD_MSG_1 (w, m, 0) \
  V4_LOAD_MSG_1 (w, m, 1) \
  V4_LOAD_MSG_1 (w, m, 2) \
  V4_LOAD_MSG_1 (w, m, 3) \
}

#define V4_LOAD_UNPACK_PAIR_128(src32, i, d0, d1) \
{ \
  const __m128i v0 = LOAD_128_FROM_STRUCT((src32) + (i    ) * 4);  \
  const __m128i v1 = LOAD_128_FROM_STRUCT((src32) + (i + 1) * 4);  \
  d0 = _mm_unpacklo_epi32(v0, v1);  \
  d1 = _mm_unpackhi_epi32(v0, v1);  \
}

#define V4_UNPACK_PAIR_128(dest32, i, s0, s1) \
{ \
  STORE_128_TO_STRUCT((dest32) + i * 4     , _mm_unpacklo_epi64(s0, s1));  \
  STORE_128_TO_STRUCT((dest32) + i * 4 + 16, _mm_unpackhi_epi64(s0, s1));  \
}

#define V4_UNPACK_STATE(dest32, src32) \
{ \
  __m128i t0, t1, t2, t3, t4, t5, t6, t7; \
  V4_LOAD_UNPACK_PAIR_128(src32, 0, t0, t1)  \
  V4_LOAD_UNPACK_PAIR_128(src32, 2, t2, t3)  \
  V4_LOAD_UNPACK_PAIR_128(src32, 4, t4, t5)  \
  V4_LOAD_UNPACK_PAIR_128(src32, 6, t6, t7)  \
  V4_UNPACK_PAIR_128(dest32, 0, t0, t2)  \
  V4_UNPACK_PAIR_128(dest32, 8, t1, t3)  \
  V4_UNPACK_PAIR_128(dest32, 1, t4, t6)  \
  V4_UNPACK_PAIR_128(dest32, 9, t5, t7)  \
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_V128_Fast(UInt32 *s_items, const Byte *data, const Byte *end)
{
  // PrintStates2(s_items, 8, 16);
  size_t pos = 0;
  pos /= 2;
  do
  {
#if defined(Z7_BLAKE2S_USE_SSSE3) && \
   !defined(Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED)
    const __m128i  r8 = k_r8;
    const __m128i r16 = k_r16;
#endif
    __m128i w[16];
    __m128i v[16];
    UInt32 *s;
    V4_LOAD_MSG(w, data)
    s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    {
      __m128i ctr = LOAD_128_FROM_STRUCT(s + 64);
      D_ADD_EPI64_128 (ctr, k_inc);
      STORE_128_TO_STRUCT(s + 64, ctr);
      v[12] = XOR_128 (GET_128_IV_WAY4(4), _mm_shuffle_epi32(ctr, _MM_SHUFFLE(0, 0, 0, 0)));
      v[13] = XOR_128 (GET_128_IV_WAY4(5), _mm_shuffle_epi32(ctr, _MM_SHUFFLE(1, 1, 1, 1)));
    }
    v[ 8] = GET_128_IV_WAY4(0);
    v[ 9] = GET_128_IV_WAY4(1);
    v[10] = GET_128_IV_WAY4(2);
    v[11] = GET_128_IV_WAY4(3);
    v[14] = GET_128_IV_WAY4(6);
    v[15] = GET_128_IV_WAY4(7);
    
#define LOAD_STATE_128_FROM_STRUCT(i) \
      v[i] = LOAD_128_FROM_STRUCT(s + (i) * 4);

#define UPDATE_STATE_128_IN_STRUCT(i) \
      STORE_128_TO_STRUCT(s + (i) * 4, XOR_128( \
      XOR_128(v[i], v[(i) + 8]), \
      LOAD_128_FROM_STRUCT(s + (i) * 4)));
    
    REP8_MACRO (LOAD_STATE_128_FROM_STRUCT)
    ROUNDS_LOOP (V4_ROUND)
    REP8_MACRO (UPDATE_STATE_128_IN_STRUCT)

    data += Z7_BLAKE2S_BLOCK_SIZE * 4;
    pos  += Z7_BLAKE2S_BLOCK_SIZE * 4 / 2;
    pos &= SUPER_BLOCK_SIZE / 2 - 1;
  }
  while (data != end);
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Final_V128_Fast(UInt32 *states)
{
  const __m128i ctr = LOAD_128_FROM_STRUCT(states + 64);
  // printf("\nBlake2sp_Compress2_V128_Fast_Final4\n");
  // PrintStates2(states, 8, 16);
  {
    ptrdiff_t pos = 8 * 4;
    do
    {
      UInt32 *src32  = states + (size_t)(pos * 1);
      UInt32 *dest32 = states + (size_t)(pos * 2);
      V4_UNPACK_STATE(dest32, src32)
      pos -= 8 * 4;
    }
    while (pos >= 0);
  }
  {
    unsigned k;
    for (k = 0; k < 8; k++)
    {
      UInt32 *s = states + (size_t)k * 16;
      STORE_128_TO_STRUCT (STATE_T(s), ctr);
    }
  }
  // PrintStates2(states, 8, 16);
}



#ifdef Z7_BLAKE2S_USE_AVX2

#define ADD_256(a, b)  _mm256_add_epi32(a, b)
#define XOR_256(a, b)  _mm256_xor_si256(a, b)

#if 1 && defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
  #define MM256_ROR_EPI32  _mm256_ror_epi32
  #define Z7_MM256_ROR_EPI32_IS_SUPPORTED
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
  #define LOAD_ROTATE_CONSTS_256
#endif
#else
#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
  #define LOAD_ROTATE_CONSTS_256 \
      const __m256i  r8 = k_r8_256; \
      const __m256i r16 = k_r16_256;
#endif // AVX2_WAY2

  #define MM256_ROR_EPI32(r, c) ( \
      ( 8==(c)) ? _mm256_shuffle_epi8(r,r8) \
    : (16==(c)) ? _mm256_shuffle_epi8(r,r16) \
    : _mm256_or_si256( \
      _mm256_srli_epi32((r), (c)), \
      _mm256_slli_epi32((r), 32-(c))))
#endif // WAY_SLOW
#endif


#define D_ADD_256(dest, src)  dest = ADD_256(dest, src)
#define D_XOR_256(dest, src)  dest = XOR_256(dest, src)

#define LOADU_256(p)     _mm256_loadu_si256((const __m256i *)(const void *)(p))

#ifdef Z7_BLAKE2S_USE_AVX2_FAST

#ifdef Z7_MM256_ROR_EPI32_IS_SUPPORTED
#define ROT_256_16(x) MM256_ROR_EPI32((x), 16)
#define ROT_256_12(x) MM256_ROR_EPI32((x), 12)
#define ROT_256_8(x)  MM256_ROR_EPI32((x),  8)
#define ROT_256_7(x)  MM256_ROR_EPI32((x),  7)
#else
#define ROTATE8  _mm256_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1, \
                                 12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1)
#define ROTATE16 _mm256_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2, \
                                 13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2)
#define ROT_256_16(x) _mm256_shuffle_epi8((x), ROTATE16)
#define ROT_256_12(x) _mm256_or_si256(_mm256_srli_epi32((x), 12), _mm256_slli_epi32((x), 20))
#define ROT_256_8(x)  _mm256_shuffle_epi8((x), ROTATE8)
#define ROT_256_7(x)  _mm256_or_si256(_mm256_srli_epi32((x),  7), _mm256_slli_epi32((x), 25))
#endif

#define D_ROT_256_7(dest)     dest = ROT_256_7(dest)
#define D_ROT_256_8(dest)     dest = ROT_256_8(dest)
#define D_ROT_256_12(dest)    dest = ROT_256_12(dest)
#define D_ROT_256_16(dest)    dest = ROT_256_16(dest)

#define LOAD_256(p)      _mm256_load_si256((const __m256i *)(const void *)(p))
#ifdef Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED
  #define STOREU_256(p, r) _mm256_storeu_si256((__m256i *)(void *)(p), r)
  #define LOAD_256_FROM_STRUCT(p)     LOADU_256(p)
  #define STORE_256_TO_STRUCT(p, r)   STOREU_256(p, r)
#else
  // if struct is aligned for 32-bytes
  #define STORE_256(p, r)  _mm256_store_si256((__m256i *)(void *)(p), r)
  #define LOAD_256_FROM_STRUCT(p)     LOAD_256(p)
  #define STORE_256_TO_STRUCT(p, r)   STORE_256(p, r)
#endif

#endif // Z7_BLAKE2S_USE_AVX2_FAST



#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW

#if 0
    #define DIAG_PERM2(s) \
    { \
      const __m256i a = LOAD_256_FROM_STRUCT((s)      ); \
      const __m256i b = LOAD_256_FROM_STRUCT((s) + NSW); \
      STORE_256_TO_STRUCT((s      ), _mm256_permute2x128_si256(a, b, 0x20)); \
      STORE_256_TO_STRUCT((s + NSW), _mm256_permute2x128_si256(a, b, 0x31)); \
    }
#else
    #define DIAG_PERM2(s) \
    { \
      const __m128i a = LOAD_128_FROM_STRUCT((s) + 4); \
      const __m128i b = LOAD_128_FROM_STRUCT((s) + NSW); \
      STORE_128_TO_STRUCT((s) + NSW, a); \
      STORE_128_TO_STRUCT((s) + 4  , b); \
    }
#endif
    #define DIAG_PERM8(s_items) \
    { \
      DIAG_PERM2(s_items) \
      DIAG_PERM2(s_items + NSW * 2) \
      DIAG_PERM2(s_items + NSW * 4) \
      DIAG_PERM2(s_items + NSW * 6) \
    }


#define AXR256(a, b, d, shift) \
    D_ADD_256(a, b); \
    D_XOR_256(d, a); \
    d = MM256_ROR_EPI32(d, shift); \



#ifdef Z7_BLAKE2S_USE_GATHER

  #define TABLE_GATHER_256_4(a0,a1,a2,a3) \
    a0,a1,a2,a3, a0+16,a1+16,a2+16,a3+16
  #define TABLE_GATHER_256( \
    a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
  { TABLE_GATHER_256_4(a0,a2,a4,a6), \
    TABLE_GATHER_256_4(a1,a3,a5,a7), \
    TABLE_GATHER_256_4(a8,a10,a12,a14), \
    TABLE_GATHER_256_4(a9,a11,a13,a15) }
MY_ALIGN(64)
static const UInt32 k_Blake2s_Sigma_gather256[BLAKE2S_NUM_ROUNDS][16 * 2] =
  { SIGMA_TABLE(TABLE_GATHER_256) };
  #define GET_SIGMA(r) \
    const UInt32 * const sigma = k_Blake2s_Sigma_gather256[r];
  #define AXR2_LOAD_INDEXES_AVX(sigma_index) \
    const __m256i i01234567 = LOAD_256(sigma + (sigma_index));
  #define SET_ROW_FROM_SIGMA_AVX(in) \
    _mm256_i32gather_epi32((const void *)(in), i01234567, 4)
  #define SIGMA_INTERLEAVE    8
  #define SIGMA_HALF_ROW_SIZE 16

#else // !Z7_BLAKE2S_USE_GATHER

  #define GET_SIGMA(r) \
    const Byte * const sigma = k_Blake2s_Sigma_4[r];
  #define AXR2_LOAD_INDEXES_AVX(sigma_index) \
    AXR2_LOAD_INDEXES(sigma_index)
  #define SET_ROW_FROM_SIGMA_AVX(in) \
    MY_mm256_set_m128i( \
        SET_ROW_FROM_SIGMA_W((in) + Z7_BLAKE2S_BLOCK_SIZE), \
        SET_ROW_FROM_SIGMA_W(in))
  #define SIGMA_INTERLEAVE    1
  #define SIGMA_HALF_ROW_SIZE 8
#endif // !Z7_BLAKE2S_USE_GATHER


#define ROTATE_WORDS_TO_RIGHT_256(a, n) \
    a = _mm256_shuffle_epi32(a, _MM_SHUFFLE((3+n)&3, (2+n)&3, (1+n)&3, (0+n)&3));



#ifdef Z7_BLAKE2S_USE_AVX2_WAY2

#define AXR2_A(sigma_index, shift1, shift2) \
    AXR2_LOAD_INDEXES_AVX(sigma_index) \
    D_ADD_256( a0, SET_ROW_FROM_SIGMA_AVX(data)); \
    AXR256(a0, b0, d0, shift1) \
    AXR256(c0, d0, b0, shift2) \

#define AXR4_A(sigma_index) \
    { AXR2_A(sigma_index, 16, 12) } \
    { AXR2_A(sigma_index + SIGMA_INTERLEAVE, 8, 7) }

#define EE1(r) \
    { GET_SIGMA(r) \
      AXR4_A(0) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 1) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 3) \
      AXR4_A(SIGMA_HALF_ROW_SIZE) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 3) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 1) \
    }

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_AVX2_Way2(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  end -= Z7_BLAKE2S_BLOCK_SIZE;

  if (data != end)
  {
    LOAD_ROTATE_CONSTS_256
    DIAG_PERM8(s_items)
    do
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
      __m256i a0, b0, c0, d0;
      {
        const __m128i inc = k_inc;
        __m128i d0_128 = LOAD_128_FROM_STRUCT (STATE_T(s));
        __m128i d1_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW));
        D_ADD_EPI64_128(d0_128, inc);
        D_ADD_EPI64_128(d1_128, inc);
        STORE_128_TO_STRUCT (STATE_T(s      ), d0_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW), d1_128);
        d0 = MY_mm256_set_m128i(d1_128, d0_128);
        D_XOR_256(d0, k_iv4_256);
      }
      c0 = SET_FROM_128(k_iv0_128);
      a0 = LOAD_256_FROM_STRUCT(s + NSW * 0);
      b0 = LOAD_256_FROM_STRUCT(s + NSW * 1);
      
      ROUNDS_LOOP (EE1)
      
      D_XOR_256(a0, c0);
      D_XOR_256(b0, d0);
      
      D_XOR_256(a0, LOAD_256_FROM_STRUCT(s + NSW * 0));
      D_XOR_256(b0, LOAD_256_FROM_STRUCT(s + NSW * 1));
      
      STORE_256_TO_STRUCT(s + NSW * 0, a0);
      STORE_256_TO_STRUCT(s + NSW * 1, b0);
      
      data += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos  += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos &= SUPER_BLOCK_MASK;
    }
    while (data < end);
    DIAG_PERM8(s_items)
    if (data != end)
      return;
  }
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Z7_BLAKE2S_CompressSingleBlock(s, data);
  }
}

#endif // Z7_BLAKE2S_USE_AVX2_WAY2



#ifdef Z7_BLAKE2S_USE_AVX2_WAY4

#define AXR2_X(sigma_index, shift1, shift2) \
    AXR2_LOAD_INDEXES_AVX(sigma_index) \
    D_ADD_256( a0, SET_ROW_FROM_SIGMA_AVX(data)); \
    D_ADD_256( a1, SET_ROW_FROM_SIGMA_AVX((data) + Z7_BLAKE2S_BLOCK_SIZE * 2)); \
    AXR256(a0, b0, d0, shift1) \
    AXR256(a1, b1, d1, shift1) \
    AXR256(c0, d0, b0, shift2) \
    AXR256(c1, d1, b1, shift2) \

#define AXR4_X(sigma_index) \
    { AXR2_X(sigma_index, 16, 12) } \
    { AXR2_X(sigma_index + SIGMA_INTERLEAVE, 8, 7) }

#define EE2(r) \
    { GET_SIGMA(r) \
      AXR4_X(0) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 1) \
        ROTATE_WORDS_TO_RIGHT_256(b1, 1) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(c1, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 3) \
        ROTATE_WORDS_TO_RIGHT_256(d1, 3) \
      AXR4_X(SIGMA_HALF_ROW_SIZE) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 3) \
        ROTATE_WORDS_TO_RIGHT_256(b1, 3) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(c1, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 1) \
        ROTATE_WORDS_TO_RIGHT_256(d1, 1) \
    }

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_AVX2_Way4(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;

  if ((size_t)(end - data) >= Z7_BLAKE2S_BLOCK_SIZE * 4)
  {
#ifndef Z7_MM256_ROR_EPI32_IS_SUPPORTED
    const __m256i  r8 = k_r8_256;
    const __m256i r16 = k_r16_256;
#endif
    end -= Z7_BLAKE2S_BLOCK_SIZE * 3;
    DIAG_PERM8(s_items)
    do
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
      __m256i a0, b0, c0, d0;
      __m256i a1, b1, c1, d1;
      {
        const __m128i inc = k_inc;
        __m128i d0_128 = LOAD_128_FROM_STRUCT (STATE_T(s));
        __m128i d1_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW));
        __m128i d2_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW * 2));
        __m128i d3_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW * 3));
        D_ADD_EPI64_128(d0_128, inc);
        D_ADD_EPI64_128(d1_128, inc);
        D_ADD_EPI64_128(d2_128, inc);
        D_ADD_EPI64_128(d3_128, inc);
        STORE_128_TO_STRUCT (STATE_T(s          ), d0_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW * 1), d1_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW * 2), d2_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW * 3), d3_128);
        d0 = MY_mm256_set_m128i(d1_128, d0_128);
        d1 = MY_mm256_set_m128i(d3_128, d2_128);
        D_XOR_256(d0, k_iv4_256);
        D_XOR_256(d1, k_iv4_256);
      }
      c1 = c0 = SET_FROM_128(k_iv0_128);
      a0 = LOAD_256_FROM_STRUCT(s + NSW * 0);
      b0 = LOAD_256_FROM_STRUCT(s + NSW * 1);
      a1 = LOAD_256_FROM_STRUCT(s + NSW * 2);
      b1 = LOAD_256_FROM_STRUCT(s + NSW * 3);
      
      ROUNDS_LOOP (EE2)

      D_XOR_256(a0, c0);
      D_XOR_256(b0, d0);
      D_XOR_256(a1, c1);
      D_XOR_256(b1, d1);
      
      D_XOR_256(a0, LOAD_256_FROM_STRUCT(s + NSW * 0));
      D_XOR_256(b0, LOAD_256_FROM_STRUCT(s + NSW * 1));
      D_XOR_256(a1, LOAD_256_FROM_STRUCT(s + NSW * 2));
      D_XOR_256(b1, LOAD_256_FROM_STRUCT(s + NSW * 3));
      
      STORE_256_TO_STRUCT(s + NSW * 0, a0);
      STORE_256_TO_STRUCT(s + NSW * 1, b0);
      STORE_256_TO_STRUCT(s + NSW * 2, a1);
      STORE_256_TO_STRUCT(s + NSW * 3, b1);
      
      data += Z7_BLAKE2S_BLOCK_SIZE * 4;
      pos  += Z7_BLAKE2S_BLOCK_SIZE * 4;
      pos &= SUPER_BLOCK_MASK;
    }
    while (data < end);
    DIAG_PERM8(s_items)
    end += Z7_BLAKE2S_BLOCK_SIZE * 3;
  }
  if (data == end)
    return;
  // Z7_BLAKE2S_Compress2_V128(s_items, data, end, pos);
  do
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Z7_BLAKE2S_CompressSingleBlock(s, data);
    data += Z7_BLAKE2S_BLOCK_SIZE;
    pos  += Z7_BLAKE2S_BLOCK_SIZE;
    pos &= SUPER_BLOCK_MASK;
  }
  while (data != end);
}

#endif // Z7_BLAKE2S_USE_AVX2_WAY4
#endif // Z7_BLAKE2S_USE_AVX2_WAY_SLOW


// ---------------------------------------------------------

#ifdef Z7_BLAKE2S_USE_AVX2_FAST

#define OP256_L(a, i)   D_ADD_256 (V(a, 0), \
    LOAD_256((const Byte *)(w) + GET_SIGMA_VAL_256(2*(a)+(i))));

#define OP256_0(a)   OP256_L(a, 0)
#define OP256_7(a)   OP256_L(a, 1)

#define OP256_1(a)   D_ADD_256 (V(a, 0), V(a, 1));
#define OP256_2(a)   D_XOR_256 (V(a, 3), V(a, 0));
#define OP256_4(a)   D_ADD_256 (V(a, 2), V(a, 3));
#define OP256_5(a)   D_XOR_256 (V(a, 1), V(a, 2));

#define OP256_3(a)   D_ROT_256_16 (V(a, 3));
#define OP256_6(a)   D_ROT_256_12 (V(a, 1));
#define OP256_8(a)   D_ROT_256_8  (V(a, 3));
#define OP256_9(a)   D_ROT_256_7  (V(a, 1));


#if 0 || 1 && defined(MY_CPU_X86)

#define V8_G(a) \
  OP256_0 (a) \
  OP256_1 (a) \
  OP256_2 (a) \
  OP256_3 (a) \
  OP256_4 (a) \
  OP256_5 (a) \
  OP256_6 (a) \
  OP256_7 (a) \
  OP256_1 (a) \
  OP256_2 (a) \
  OP256_8 (a) \
  OP256_4 (a) \
  OP256_5 (a) \
  OP256_9 (a) \

#define V8R { \
  V8_G (0); \
  V8_G (1); \
  V8_G (2); \
  V8_G (3); \
  V8_G (4); \
  V8_G (5); \
  V8_G (6); \
  V8_G (7); \
}

#else

#define OP256_INTER_4(op, a,b,c,d) \
  op (a) \
  op (b) \
  op (c) \
  op (d) \

#define V8_G(a,b,c,d) \
  OP256_INTER_4 (OP256_0, a,b,c,d) \
  OP256_INTER_4 (OP256_1, a,b,c,d) \
  OP256_INTER_4 (OP256_2, a,b,c,d) \
  OP256_INTER_4 (OP256_3, a,b,c,d) \
  OP256_INTER_4 (OP256_4, a,b,c,d) \
  OP256_INTER_4 (OP256_5, a,b,c,d) \
  OP256_INTER_4 (OP256_6, a,b,c,d) \
  OP256_INTER_4 (OP256_7, a,b,c,d) \
  OP256_INTER_4 (OP256_1, a,b,c,d) \
  OP256_INTER_4 (OP256_2, a,b,c,d) \
  OP256_INTER_4 (OP256_8, a,b,c,d) \
  OP256_INTER_4 (OP256_4, a,b,c,d) \
  OP256_INTER_4 (OP256_5, a,b,c,d) \
  OP256_INTER_4 (OP256_9, a,b,c,d) \

#define V8R { \
  V8_G (0, 1, 2, 3) \
  V8_G (4, 5, 6, 7) \
}
#endif

#define V8_ROUND(r)  { GET_SIGMA_PTR_256(r); V8R }


// for debug:
// #define Z7_BLAKE2S_PERMUTE_WITH_GATHER
#if defined(Z7_BLAKE2S_PERMUTE_WITH_GATHER)
// gather instruction is slow.
#define V8_LOAD_MSG(w, m) \
{ \
  unsigned i; \
  for (i = 0; i < 16; ++i) { \
    w[i] = _mm256_i32gather_epi32( \
      (const void *)((m) + i * sizeof(UInt32)),\
      _mm256_set_epi32(0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00), \
      sizeof(UInt32)); \
  } \
}
#else // !Z7_BLAKE2S_PERMUTE_WITH_GATHER

#define V8_LOAD_MSG_2(w, a0, a1) \
{ \
  (w)[0] = _mm256_permute2x128_si256(a0, a1, 0x20);  \
  (w)[4] = _mm256_permute2x128_si256(a0, a1, 0x31);  \
}

#define V8_LOAD_MSG_4(w, z0, z1, z2, z3) \
{ \
  __m256i s0, s1, s2, s3;  \
  s0 = _mm256_unpacklo_epi64(z0, z1);  \
  s1 = _mm256_unpackhi_epi64(z0, z1);  \
  s2 = _mm256_unpacklo_epi64(z2, z3);  \
  s3 = _mm256_unpackhi_epi64(z2, z3);  \
  V8_LOAD_MSG_2((w) + 0, s0, s2)   \
  V8_LOAD_MSG_2((w) + 1, s1, s3)   \
}

#define V8_LOAD_MSG_0(t0, t1, m) \
{ \
  __m256i m0, m1;  \
  m0 = LOADU_256(m);  \
  m1 = LOADU_256((m) + 2 * 32);  \
  t0 = _mm256_unpacklo_epi32(m0, m1);  \
  t1 = _mm256_unpackhi_epi32(m0, m1);  \
}

#define V8_LOAD_MSG_8(w, m) \
{ \
  __m256i t0, t1, t2, t3, t4, t5, t6, t7;  \
  V8_LOAD_MSG_0(t0, t4, (m) + 0 * 4 * 32)  \
  V8_LOAD_MSG_0(t1, t5, (m) + 1 * 4 * 32)  \
  V8_LOAD_MSG_0(t2, t6, (m) + 2 * 4 * 32)  \
  V8_LOAD_MSG_0(t3, t7, (m) + 3 * 4 * 32)  \
  V8_LOAD_MSG_4((w)    , t0, t1, t2, t3)   \
  V8_LOAD_MSG_4((w) + 2, t4, t5, t6, t7)   \
}

#define V8_LOAD_MSG(w, m) \
{ \
  V8_LOAD_MSG_8(w, m)  \
  V8_LOAD_MSG_8((w) + 8, (m) + 32)  \
}

#endif // !Z7_BLAKE2S_PERMUTE_WITH_GATHER


#define V8_PERM_PAIR_STORE(u, a0, a2) \
{ \
  STORE_256_TO_STRUCT((u),     _mm256_permute2x128_si256(a0, a2, 0x20));  \
  STORE_256_TO_STRUCT((u) + 8, _mm256_permute2x128_si256(a0, a2, 0x31));  \
}

#define V8_UNPACK_STORE_4(u, z0, z1, z2, z3) \
{ \
  __m256i s0, s1, s2, s3;  \
  s0 = _mm256_unpacklo_epi64(z0, z1);  \
  s1 = _mm256_unpackhi_epi64(z0, z1);  \
  s2 = _mm256_unpacklo_epi64(z2, z3);  \
  s3 = _mm256_unpackhi_epi64(z2, z3);  \
  V8_PERM_PAIR_STORE(u + 0, s0, s2)  \
  V8_PERM_PAIR_STORE(u + 2, s1, s3)  \
}

#define V8_UNPACK_STORE_0(src32, d0, d1) \
{ \
  const __m256i v0 = LOAD_256_FROM_STRUCT ((src32)    );  \
  const __m256i v1 = LOAD_256_FROM_STRUCT ((src32) + 8);  \
  d0 = _mm256_unpacklo_epi32(v0, v1);  \
  d1 = _mm256_unpackhi_epi32(v0, v1);  \
}

#define V8_UNPACK_STATE(dest32, src32) \
{ \
  __m256i t0, t1, t2, t3, t4, t5, t6, t7;  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 0, t0, t4)  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 1, t1, t5)  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 2, t2, t6)  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 3, t3, t7)  \
  V8_UNPACK_STORE_4 ((__m256i *)(void *)(dest32)    , t0, t1, t2, t3)  \
  V8_UNPACK_STORE_4 ((__m256i *)(void *)(dest32) + 4, t4, t5, t6, t7)  \
}



#define V8_LOAD_STATE_256_FROM_STRUCT(i) \
      v[i] = LOAD_256_FROM_STRUCT(s_items + (i) * 8);

#if 0 || 0 && defined(MY_CPU_X86)
#define Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
#endif

#ifdef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
// this branch doesn't use (iv) array
// so register pressure can be lower.
// it can be faster sometimes
#define V8_LOAD_STATE_256(i)  V8_LOAD_STATE_256_FROM_STRUCT(i)
#define V8_UPDATE_STATE_256(i) \
{ \
    STORE_256_TO_STRUCT(s_items + (i) * 8, XOR_256( \
    XOR_256(v[i], v[(i) + 8]), \
    LOAD_256_FROM_STRUCT(s_items + (i) * 8))); \
}
#else
// it uses more variables (iv) registers
// it's better for gcc
// maybe that branch is better, if register pressure will be lower (avx512)
#define V8_LOAD_STATE_256(i)   { iv[i] = v[i]; }
#define V8_UPDATE_STATE_256(i) { v[i] = XOR_256(XOR_256(v[i], v[i + 8]), iv[i]); }
#define V8_STORE_STATE_256(i)  { STORE_256_TO_STRUCT(s_items + (i) * 8, v[i]); }
#endif


#if 0
  // use loading constants from memory
  #define KK8(n)  KIV(n), KIV(n), KIV(n), KIV(n), KIV(n), KIV(n), KIV(n), KIV(n)
MY_ALIGN(64)
static const UInt32 k_Blake2s_IV_WAY8[]=
{
  KK8(0), KK8(1), KK8(2), KK8(3), KK8(4), KK8(5), KK8(6), KK8(7)
};
  #define GET_256_IV_WAY8(i)  LOAD_256(k_Blake2s_IV_WAY8 + 8 * (i))
#else
  // use constant generation:
  #define GET_256_IV_WAY8(i)  _mm256_set1_epi32((Int32)KIV(i))
#endif


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_AVX2_Fast(UInt32 *s_items, const Byte *data, const Byte *end)
{
#ifndef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
  __m256i v[16];
#endif

  // PrintStates2(s_items, 8, 16);

#ifndef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
  REP8_MACRO (V8_LOAD_STATE_256_FROM_STRUCT)
#endif

  do
  {
    __m256i w[16];
#ifdef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
    __m256i v[16];
#else
    __m256i iv[8];
#endif
    V8_LOAD_MSG(w, data)
    {
      // we use load/store ctr inside loop to reduce register pressure:
#if 1 || 1 && defined(MY_CPU_X86)
      const __m256i ctr = _mm256_add_epi64(
          LOAD_256_FROM_STRUCT(s_items + 64),
          _mm256_set_epi32(
              0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE,
              0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE));
      STORE_256_TO_STRUCT(s_items + 64, ctr);
#else
      const UInt64 ctr64 = *(const UInt64 *)(const void *)(s_items + 64)
          + Z7_BLAKE2S_BLOCK_SIZE;
      const __m256i ctr = _mm256_set_epi64x(0, (Int64)ctr64, 0, (Int64)ctr64);
      *(UInt64 *)(void *)(s_items + 64) = ctr64;
#endif
      v[12] = XOR_256 (GET_256_IV_WAY8(4), _mm256_shuffle_epi32(ctr, _MM_SHUFFLE(0, 0, 0, 0)));
      v[13] = XOR_256 (GET_256_IV_WAY8(5), _mm256_shuffle_epi32(ctr, _MM_SHUFFLE(1, 1, 1, 1)));
    }
    v[ 8] = GET_256_IV_WAY8(0);
    v[ 9] = GET_256_IV_WAY8(1);
    v[10] = GET_256_IV_WAY8(2);
    v[11] = GET_256_IV_WAY8(3);
    v[14] = GET_256_IV_WAY8(6);
    v[15] = GET_256_IV_WAY8(7);

    REP8_MACRO (V8_LOAD_STATE_256)
    ROUNDS_LOOP (V8_ROUND)
    REP8_MACRO (V8_UPDATE_STATE_256)
    data += SUPER_BLOCK_SIZE;
  }
  while (data != end);

#ifndef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
  REP8_MACRO (V8_STORE_STATE_256)
#endif
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Final_AVX2_Fast(UInt32 *states)
{
  const __m128i ctr = LOAD_128_FROM_STRUCT(states + 64);
  // PrintStates2(states, 8, 16);
  V8_UNPACK_STATE(states, states)
  // PrintStates2(states, 8, 16);
  {
    unsigned k;
    for (k = 0; k < 8; k++)
    {
      UInt32 *s = states + (size_t)k * 16;
      STORE_128_TO_STRUCT (STATE_T(s), ctr);
    }
  }
  // PrintStates2(states, 8, 16);
  // printf("\nafter V8_UNPACK_STATE \n");
}

#endif // Z7_BLAKE2S_USE_AVX2_FAST
#endif // avx2
#endif // vector


/*
#define Blake2s_Increment_Counter(s, inc) \
  { STATE_T(s)[0] += (inc);  STATE_T(s)[1] += (STATE_T(s)[0] < (inc)); }
#define Blake2s_Increment_Counter_Small(s, inc) \
  { STATE_T(s)[0] += (inc); }
*/

#define Blake2s_Set_LastBlock(s) \
  { STATE_F(s)[0] = BLAKE2S_FINAL_FLAG; /* STATE_F(s)[1] = p->u.header.lastNode_f1; */ }


#if 0 || 1 && defined(Z7_MSC_VER_ORIGINAL) && Z7_MSC_VER_ORIGINAL >= 1600
  // good for vs2022
  #define LOOP_8(mac) { unsigned kkk; for (kkk = 0; kkk < 8; kkk++) mac(kkk) }
#else
   // good for Z7_BLAKE2S_UNROLL for GCC9 (arm*/x86*) and MSC_VER_1400-x64.
  #define LOOP_8(mac) { REP8_MACRO(mac) }
#endif


static
Z7_FORCE_INLINE
// Z7_NO_INLINE
void
Z7_FASTCALL
Blake2s_Compress(UInt32 *s, const Byte *input)
{
  UInt32 m[16];
  UInt32 v[16];
  {
    unsigned i;
    for (i = 0; i < 16; i++)
      m[i] = GetUi32(input + i * 4);
  }

#define INIT_v_FROM_s(i)  v[i] = s[i];
  
  LOOP_8(INIT_v_FROM_s)
 
  // Blake2s_Increment_Counter(s, Z7_BLAKE2S_BLOCK_SIZE)
  {
    const UInt32 t0 = STATE_T(s)[0] + Z7_BLAKE2S_BLOCK_SIZE;
    const UInt32 t1 = STATE_T(s)[1] + (t0 < Z7_BLAKE2S_BLOCK_SIZE);
    STATE_T(s)[0] = t0;
    STATE_T(s)[1] = t1;
    v[12] = t0 ^ KIV(4);
    v[13] = t1 ^ KIV(5);
  }
  // v[12] = STATE_T(s)[0] ^ KIV(4);
  // v[13] = STATE_T(s)[1] ^ KIV(5);
  v[14] = STATE_F(s)[0] ^ KIV(6);
  v[15] = STATE_F(s)[1] ^ KIV(7);

  v[ 8] = KIV(0);
  v[ 9] = KIV(1);
  v[10] = KIV(2);
  v[11] = KIV(3);
  // PrintStates2((const UInt32 *)v, 1, 16);

  #define ADD_SIGMA(a, index)  V(a, 0) += *(const UInt32 *)GET_SIGMA_PTR(m, sigma[index]);
  #define ADD32M(dest, src, a)    V(a, dest) += V(a, src);
  #define XOR32M(dest, src, a)    V(a, dest) ^= V(a, src);
  #define RTR32M(dest, shift, a)  V(a, dest) = rotrFixed(V(a, dest), shift);

// big interleaving can provides big performance gain, if scheduler queues are small.
#if 0 || 1 && defined(MY_CPU_X86)
  // interleave-1: for small register number (x86-32bit)
  #define G2(index, a, x, y) \
    ADD_SIGMA (a, (index) + 2 * 0) \
    ADD32M (0, 1, a) \
    XOR32M (3, 0, a) \
    RTR32M (3, x, a) \
    ADD32M (2, 3, a) \
    XOR32M (1, 2, a) \
    RTR32M (1, y, a) \

  #define G(a) \
    G2(a * 2    , a, 16, 12) \
    G2(a * 2 + 1, a,  8,  7) \

  #define R2 \
    G(0) \
    G(1) \
    G(2) \
    G(3) \
    G(4) \
    G(5) \
    G(6) \
    G(7) \

#elif 0 || 1 && defined(MY_CPU_X86_OR_AMD64)
  // interleave-2: is good if the number of registers is not big (x86-64).

  #define REP2(mac, dest, src, a, b) \
      mac(dest, src, a) \
      mac(dest, src, b)

  #define G2(index, a, b, x, y) \
    ADD_SIGMA (a, (index) + 2 * 0) \
    ADD_SIGMA (b, (index) + 2 * 1) \
    REP2 (ADD32M, 0, 1, a, b) \
    REP2 (XOR32M, 3, 0, a, b) \
    REP2 (RTR32M, 3, x, a, b) \
    REP2 (ADD32M, 2, 3, a, b) \
    REP2 (XOR32M, 1, 2, a, b) \
    REP2 (RTR32M, 1, y, a, b) \

  #define G(a, b) \
    G2(a * 2    , a, b, 16, 12) \
    G2(a * 2 + 1, a, b,  8,  7) \

  #define R2 \
    G(0, 1) \
    G(2, 3) \
    G(4, 5) \
    G(6, 7) \

#else
  // interleave-4:
  // it has big register pressure for x86/x64.
  // and MSVC compilers for x86/x64 are slow for this branch.
  // but if we have big number of registers, this branch can be faster.

  #define REP4(mac, dest, src, a, b, c, d) \
      mac(dest, src, a) \
      mac(dest, src, b) \
      mac(dest, src, c) \
      mac(dest, src, d)

  #define G2(index, a, b, c, d, x, y) \
    ADD_SIGMA (a, (index) + 2 * 0) \
    ADD_SIGMA (b, (index) + 2 * 1) \
    ADD_SIGMA (c, (index) + 2 * 2) \
    ADD_SIGMA (d, (index) + 2 * 3) \
    REP4 (ADD32M, 0, 1, a, b, c, d) \
    REP4 (XOR32M, 3, 0, a, b, c, d) \
    REP4 (RTR32M, 3, x, a, b, c, d) \
    REP4 (ADD32M, 2, 3, a, b, c, d) \
    REP4 (XOR32M, 1, 2, a, b, c, d) \
    REP4 (RTR32M, 1, y, a, b, c, d) \

  #define G(a, b, c, d) \
    G2(a * 2    , a, b, c, d, 16, 12) \
    G2(a * 2 + 1, a, b, c, d,  8,  7) \

  #define R2 \
    G(0, 1, 2, 3) \
    G(4, 5, 6, 7) \

#endif

  #define R(r)  { const Byte *sigma = k_Blake2s_Sigma_4[r];  R2 }

  // Z7_BLAKE2S_UNROLL gives 5-6 KB larger code, but faster:
  //   20-40% faster for (x86/x64) VC2010+/GCC/CLANG.
  //   30-60% faster for (arm64-arm32) GCC.
  //    5-11% faster for (arm64) CLANG-MAC.
  // so Z7_BLAKE2S_UNROLL is good optimization, if there is no vector branch.
  // But if there is vectors branch (for x86*), this scalar code will be unused mostly.
  // So we want smaller code (without unrolling) in that case (x86*).
#if 0 || 1 && !defined(Z7_BLAKE2S_USE_VECTORS)
  #define Z7_BLAKE2S_UNROLL
#endif

#ifdef Z7_BLAKE2S_UNROLL
    ROUNDS_LOOP_UNROLLED (R)
#else
    ROUNDS_LOOP (R)
#endif
  
  #undef G
  #undef G2
  #undef R
  #undef R2

  // printf("\n v after: \n");
  // PrintStates2((const UInt32 *)v, 1, 16);
#define XOR_s_PAIR_v(i)  s[i] ^= v[i] ^ v[i + 8];

  LOOP_8(XOR_s_PAIR_v)
  // printf("\n s after:\n");
  // PrintStates2((const UInt32 *)s, 1, 16);
}


static
Z7_NO_INLINE
void
Z7_FASTCALL
Blake2sp_Compress2(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  // PrintStates2(s_items, 8, 16);
  do
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Blake2s_Compress(s, data);
    data += Z7_BLAKE2S_BLOCK_SIZE;
    pos  += Z7_BLAKE2S_BLOCK_SIZE;
    pos &= SUPER_BLOCK_MASK;
  }
  while (data != end);
}


#ifdef Z7_BLAKE2S_USE_VECTORS

static Z7_BLAKE2SP_FUNC_COMPRESS g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast   = Blake2sp_Compress2;
static Z7_BLAKE2SP_FUNC_COMPRESS g_Z7_BLAKE2SP_FUNC_COMPRESS_Single = Blake2sp_Compress2;
static Z7_BLAKE2SP_FUNC_INIT     g_Z7_BLAKE2SP_FUNC_INIT_Init;
static Z7_BLAKE2SP_FUNC_INIT     g_Z7_BLAKE2SP_FUNC_INIT_Final;
static unsigned g_z7_Blake2sp_SupportedFlags;

  #define Z7_BLAKE2SP_Compress_Fast(p)   (p)->u.header.func_Compress_Fast
  #define Z7_BLAKE2SP_Compress_Single(p) (p)->u.header.func_Compress_Single
#else
  #define Z7_BLAKE2SP_Compress_Fast(p)   Blake2sp_Compress2
  #define Z7_BLAKE2SP_Compress_Single(p) Blake2sp_Compress2
#endif // Z7_BLAKE2S_USE_VECTORS


#if 1 && defined(MY_CPU_LE)
    #define GET_DIGEST(_s, _digest) \
      { memcpy(_digest, _s, Z7_BLAKE2S_DIGEST_SIZE); }
#else
    #define GET_DIGEST(_s, _digest) \
    { unsigned _i; for (_i = 0; _i < 8; _i++) \
        { SetUi32((_digest) + 4 * _i, (_s)[_i]) } \
    }
#endif


/* ---------- BLAKE2s ---------- */
/*
// we need to xor CBlake2s::h[i] with input parameter block after Blake2s_Init0()
typedef struct
{
  Byte  digest_length;
  Byte  key_length;
  Byte  fanout;               // = 1 : in sequential mode
  Byte  depth;                // = 1 : in sequential mode
  UInt32 leaf_length;
  Byte  node_offset[6];       // 0 for the first, leftmost, leaf, or in sequential mode
  Byte  node_depth;           // 0 for the leaves, or in sequential mode
  Byte  inner_length;         // [0, 32], 0 in sequential mode
  Byte  salt[BLAKE2S_SALTBYTES];
  Byte  personal[BLAKE2S_PERSONALBYTES];
} CBlake2sParam;
*/

#define k_Blake2sp_IV_0  \
    (KIV(0) ^ (Z7_BLAKE2S_DIGEST_SIZE | ((UInt32)Z7_BLAKE2SP_PARALLEL_DEGREE << 16) | ((UInt32)2 << 24)))
#define k_Blake2sp_IV_3_FROM_NODE_DEPTH(node_depth)  \
    (KIV(3) ^ ((UInt32)(node_depth) << 16) ^ ((UInt32)Z7_BLAKE2S_DIGEST_SIZE << 24))

Z7_FORCE_INLINE
static void Blake2sp_Init_Spec(UInt32 *s, unsigned node_offset, unsigned node_depth)
{
  s[0] = k_Blake2sp_IV_0;
  s[1] = KIV(1);
  s[2] = KIV(2) ^ (UInt32)node_offset;
  s[3] = k_Blake2sp_IV_3_FROM_NODE_DEPTH(node_depth);
  s[4] = KIV(4);
  s[5] = KIV(5);
  s[6] = KIV(6);
  s[7] = KIV(7);

  STATE_T(s)[0] = 0;
  STATE_T(s)[1] = 0;
  STATE_F(s)[0] = 0;
  STATE_F(s)[1] = 0;
}


#ifdef Z7_BLAKE2S_USE_V128_FAST

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_InitState_V128_Fast(UInt32 *states)
{
#define STORE_128_PAIR_INIT_STATES_2(i, t0, t1) \
  { STORE_128_TO_STRUCT(states +  0 + 4 * (i), (t0)); \
    STORE_128_TO_STRUCT(states + 32 + 4 * (i), (t1)); \
  }
#define STORE_128_PAIR_INIT_STATES_1(i, mac) \
  { const __m128i t = mac; \
    STORE_128_PAIR_INIT_STATES_2(i, t, t) \
  }
#define STORE_128_PAIR_INIT_STATES_IV(i) \
    STORE_128_PAIR_INIT_STATES_1(i, GET_128_IV_WAY4(i))

  STORE_128_PAIR_INIT_STATES_1  (0, _mm_set1_epi32((Int32)k_Blake2sp_IV_0))
  STORE_128_PAIR_INIT_STATES_IV (1)
  {
    const __m128i t = GET_128_IV_WAY4(2);
    STORE_128_PAIR_INIT_STATES_2 (2,
        XOR_128(t, _mm_set_epi32(3, 2, 1, 0)),
        XOR_128(t, _mm_set_epi32(7, 6, 5, 4)))
  }
  STORE_128_PAIR_INIT_STATES_1  (3, _mm_set1_epi32((Int32)k_Blake2sp_IV_3_FROM_NODE_DEPTH(0)))
  STORE_128_PAIR_INIT_STATES_IV (4)
  STORE_128_PAIR_INIT_STATES_IV (5)
  STORE_128_PAIR_INIT_STATES_IV (6)
  STORE_128_PAIR_INIT_STATES_IV (7)
  STORE_128_PAIR_INIT_STATES_1  (16, _mm_set_epi32(0, 0, 0, 0))
  // printf("\n== exit Blake2sp_InitState_V128_Fast ctr=%d\n", states[64]);
}

#endif // Z7_BLAKE2S_USE_V128_FAST


#ifdef Z7_BLAKE2S_USE_AVX2_FAST

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_InitState_AVX2_Fast(UInt32 *states)
{
#define STORE_256_INIT_STATES(i, t) \
    STORE_256_TO_STRUCT(states + 8 * (i), t);
#define STORE_256_INIT_STATES_IV(i) \
    STORE_256_INIT_STATES(i, GET_256_IV_WAY8(i))

  STORE_256_INIT_STATES    (0,  _mm256_set1_epi32((Int32)k_Blake2sp_IV_0))
  STORE_256_INIT_STATES_IV (1)
  STORE_256_INIT_STATES    (2, XOR_256( GET_256_IV_WAY8(2),
                                _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0)))
  STORE_256_INIT_STATES    (3,  _mm256_set1_epi32((Int32)k_Blake2sp_IV_3_FROM_NODE_DEPTH(0)))
  STORE_256_INIT_STATES_IV (4)
  STORE_256_INIT_STATES_IV (5)
  STORE_256_INIT_STATES_IV (6)
  STORE_256_INIT_STATES_IV (7)
  STORE_256_INIT_STATES    (8, _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0))
  // printf("\n== exit Blake2sp_InitState_AVX2_Fast\n");
}

#endif // Z7_BLAKE2S_USE_AVX2_FAST



Z7_NO_INLINE
void Blake2sp_InitState(CBlake2sp *p)
{
  size_t i;
  // memset(p->states, 0, sizeof(p->states)); // for debug
  p->u.header.cycPos = 0;
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  if (p->u.header.func_Init)
  {
    p->u.header.func_Init(p->states);
    return;
  }
#endif
  for (i = 0; i < Z7_BLAKE2SP_PARALLEL_DEGREE; i++)
    Blake2sp_Init_Spec(p->states + i * NSW, (unsigned)i, 0);
}

void Blake2sp_Init(CBlake2sp *p)
{
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  p->u.header.func_Compress_Fast =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast;
#else
    NULL;
#endif

  p->u.header.func_Compress_Single =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_COMPRESS_Single;
#else
    NULL;
#endif

  p->u.header.func_Init =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_INIT_Init;
#else
    NULL;
#endif

  p->u.header.func_Final =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_INIT_Final;
#else
    NULL;
#endif
#endif

  Blake2sp_InitState(p);
}


void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size)
{
  size_t pos;
  // printf("\nsize = 0x%6x, cycPos = %5u data = %p\n", (unsigned)size, (unsigned)p->u.header.cycPos, data);
  if (size == 0)
    return;
  pos = p->u.header.cycPos;
  // pos <  SUPER_BLOCK_SIZE * 2  : is expected
  // pos == SUPER_BLOCK_SIZE * 2  : is not expected, but is supported also
  {
    const size_t pos2 = pos & SUPER_BLOCK_MASK;
    if (pos2)
    {
      const size_t rem = SUPER_BLOCK_SIZE - pos2;
      if (rem > size)
      {
        p->u.header.cycPos = (unsigned)(pos + size);
        // cycPos < SUPER_BLOCK_SIZE * 2
        memcpy((Byte *)(void *)p->buf32 + pos, data, size);
        /* to simpilify the code here we don't try to process first superblock,
           if (cycPos > SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE) */
        return;
      }
      // (rem <= size)
      memcpy((Byte *)(void *)p->buf32 + pos, data, rem);
      pos += rem;
      data += rem;
      size -= rem;
    }
  }

  // pos <= SUPER_BLOCK_SIZE * 2
  // pos  % SUPER_BLOCK_SIZE == 0
  if (pos)
  {
    /* pos == SUPER_BLOCK_SIZE ||
       pos == SUPER_BLOCK_SIZE * 2 */
    size_t end = pos;
    if (size > SUPER_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE
        || (end -= SUPER_BLOCK_SIZE))
    {
      Z7_BLAKE2SP_Compress_Fast(p)(p->states,
          (const Byte *)(const void *)p->buf32,
          (const Byte *)(const void *)p->buf32 + end);
      if (pos -= end)
        memcpy(p->buf32, (const Byte *)(const void *)p->buf32
            + SUPER_BLOCK_SIZE, SUPER_BLOCK_SIZE);
    }
  }

  // pos == 0 || (pos == SUPER_BLOCK_SIZE && size <= SUPER_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE)
  if (size > SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE)
  {
    // pos == 0
    const Byte *end;
    const size_t size2 = (size - (SUPER_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE + 1))
        & ~(size_t)SUPER_BLOCK_MASK;
    size -= size2;
    // size < SUPER_BLOCK_SIZE * 2
    end = data + size2;
    Z7_BLAKE2SP_Compress_Fast(p)(p->states, data, end);
    data = end;
  }
  
  if (size != 0)
  {
    memcpy((Byte *)(void *)p->buf32 + pos, data, size);
    pos += size;
  }
  p->u.header.cycPos = (unsigned)pos;
  // cycPos < SUPER_BLOCK_SIZE * 2
}


void Blake2sp_Final(CBlake2sp *p, Byte *digest)
{
  // UInt32 * const R_states = p->states;
  // printf("\nBlake2sp_Final \n");
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  if (p->u.header.func_Final)
      p->u.header.func_Final(p->states);
#endif
  // printf("\n=====\nBlake2sp_Final \n");
  // PrintStates(p->states, 32);

  // (p->u.header.cycPos == SUPER_BLOCK_SIZE) can be processed in any branch:
  if (p->u.header.cycPos <= SUPER_BLOCK_SIZE)
  {
    unsigned pos;
    memset((Byte *)(void *)p->buf32 + p->u.header.cycPos,
        0, SUPER_BLOCK_SIZE - p->u.header.cycPos);
    STATE_F(&p->states[(Z7_BLAKE2SP_PARALLEL_DEGREE - 1) * NSW])[1] = BLAKE2S_FINAL_FLAG;
    for (pos = 0; pos < SUPER_BLOCK_SIZE; pos += Z7_BLAKE2S_BLOCK_SIZE)
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, pos);
      Blake2s_Set_LastBlock(s)
      if (pos + Z7_BLAKE2S_BLOCK_SIZE > p->u.header.cycPos)
      {
        UInt32 delta = Z7_BLAKE2S_BLOCK_SIZE;
        if (pos < p->u.header.cycPos)
          delta -= p->u.header.cycPos & (Z7_BLAKE2S_BLOCK_SIZE - 1);
        // 0 < delta <= Z7_BLAKE2S_BLOCK_SIZE
        {
          const UInt32 v = STATE_T(s)[0];
          STATE_T(s)[1] -= v < delta; //  (v < delta) is same condition here as (v == 0)
          STATE_T(s)[0]  = v - delta;
        }
      }
    }
    // PrintStates(p->states, 16);
    Z7_BLAKE2SP_Compress_Single(p)(p->states,
        (Byte *)(void *)p->buf32,
        (Byte *)(void *)p->buf32 + SUPER_BLOCK_SIZE);
    // PrintStates(p->states, 16);
  }
  else
  {
    // (p->u.header.cycPos > SUPER_BLOCK_SIZE)
    unsigned pos;
    for (pos = 0; pos < SUPER_BLOCK_SIZE; pos += Z7_BLAKE2S_BLOCK_SIZE)
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, pos);
      if (pos + SUPER_BLOCK_SIZE >= p->u.header.cycPos)
        Blake2s_Set_LastBlock(s)
    }
    if (p->u.header.cycPos <= SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE)
      STATE_F(&p->states[(Z7_BLAKE2SP_PARALLEL_DEGREE - 1) * NSW])[1] = BLAKE2S_FINAL_FLAG;

    Z7_BLAKE2SP_Compress_Single(p)(p->states,
        (Byte *)(void *)p->buf32,
        (Byte *)(void *)p->buf32 + SUPER_BLOCK_SIZE);

    // if (p->u.header.cycPos > SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE;
      STATE_F(&p->states[(Z7_BLAKE2SP_PARALLEL_DEGREE - 1) * NSW])[1] = BLAKE2S_FINAL_FLAG;

    // if (p->u.header.cycPos != SUPER_BLOCK_SIZE)
    {
      pos = SUPER_BLOCK_SIZE;
      for (;;)
      {
        UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, pos & SUPER_BLOCK_MASK);
        Blake2s_Set_LastBlock(s)
        pos += Z7_BLAKE2S_BLOCK_SIZE;
        if (pos >= p->u.header.cycPos)
        {
          if (pos != p->u.header.cycPos)
          {
            const UInt32 delta = pos - p->u.header.cycPos;
            const UInt32 v = STATE_T(s)[0];
            STATE_T(s)[1] -= v < delta;
            STATE_T(s)[0]  = v - delta;
            memset((Byte *)(void *)p->buf32 + p->u.header.cycPos, 0, delta);
          }
          break;
        }
      }
      Z7_BLAKE2SP_Compress_Single(p)(p->states,
          (Byte *)(void *)p->buf32 + SUPER_BLOCK_SIZE,
          (Byte *)(void *)p->buf32 + pos);
    }
  }

  {
    size_t pos;
    for (pos = 0; pos < SUPER_BLOCK_SIZE / 2; pos += Z7_BLAKE2S_BLOCK_SIZE / 2)
    {
      const UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, (pos * 2));
      Byte *dest = (Byte *)(void *)p->buf32 + pos;
      GET_DIGEST(s, dest)
    }
  }
  Blake2sp_Init_Spec(p->states, 0, 1);
  {
    size_t pos;
    for (pos = 0; pos < (Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2S_DIGEST_SIZE)
        - Z7_BLAKE2S_BLOCK_SIZE; pos += Z7_BLAKE2S_BLOCK_SIZE)
    {
      Z7_BLAKE2SP_Compress_Single(p)(p->states,
          (const Byte *)(const void *)p->buf32 + pos,
          (const Byte *)(const void *)p->buf32 + pos + Z7_BLAKE2S_BLOCK_SIZE);
    }
  }
  // Blake2s_Final(p->states, 0, digest, p, (Byte *)(void *)p->buf32 + i);
  Blake2s_Set_LastBlock(p->states)
  STATE_F(p->states)[1] = BLAKE2S_FINAL_FLAG;
  {
    Z7_BLAKE2SP_Compress_Single(p)(p->states,
        (const Byte *)(const void *)p->buf32 + Z7_BLAKE2SP_PARALLEL_DEGREE / 2 * Z7_BLAKE2S_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE,
        (const Byte *)(const void *)p->buf32 + Z7_BLAKE2SP_PARALLEL_DEGREE / 2 * Z7_BLAKE2S_BLOCK_SIZE);
  }
  GET_DIGEST(p->states, digest)
  // printf("\n Blake2sp_Final 555 numDataInBufs = %5u\n", (unsigned)p->u.header.numDataInBufs);
}


BoolInt Blake2sp_SetFunction(CBlake2sp *p, unsigned algo)
{
  // printf("\n========== setfunction = %d ======== \n",  algo);
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  Z7_BLAKE2SP_FUNC_COMPRESS func = NULL;
  Z7_BLAKE2SP_FUNC_COMPRESS func_Single = NULL;
  Z7_BLAKE2SP_FUNC_INIT     func_Final = NULL;
  Z7_BLAKE2SP_FUNC_INIT     func_Init = NULL;
#else
  UNUSED_VAR(p)
#endif
  
#ifdef Z7_BLAKE2S_USE_VECTORS

  func = func_Single = Blake2sp_Compress2;

  if (algo != Z7_BLAKE2SP_ALGO_SCALAR)
  {
    // printf("\n========== setfunction NON-SCALER ======== \n");
    if (algo == Z7_BLAKE2SP_ALGO_DEFAULT)
    {
      func        = g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast;
      func_Single = g_Z7_BLAKE2SP_FUNC_COMPRESS_Single;
      func_Init   = g_Z7_BLAKE2SP_FUNC_INIT_Init;
      func_Final  = g_Z7_BLAKE2SP_FUNC_INIT_Final;
    }
    else
    {
      if ((g_z7_Blake2sp_SupportedFlags & (1u << algo)) == 0)
        return False;

#ifdef Z7_BLAKE2S_USE_AVX2

      func_Single =
#if defined(Z7_BLAKE2S_USE_AVX2_WAY2)
        Blake2sp_Compress2_AVX2_Way2;
#else
        Z7_BLAKE2S_Compress2_V128;
#endif

#ifdef Z7_BLAKE2S_USE_AVX2_FAST
      if (algo == Z7_BLAKE2SP_ALGO_V256_FAST)
      {
        func = Blake2sp_Compress2_AVX2_Fast;
        func_Final = Blake2sp_Final_AVX2_Fast;
        func_Init  = Blake2sp_InitState_AVX2_Fast;
      }
      else
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
      if (algo == Z7_BLAKE2SP_ALGO_V256_WAY2)
        func = Blake2sp_Compress2_AVX2_Way2;
      else
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY4
      if (algo == Z7_BLAKE2SP_ALGO_V256_WAY4)
      {
        func_Single = func = Blake2sp_Compress2_AVX2_Way4;
      }
      else
#endif
#endif // avx2
      {
        if (algo == Z7_BLAKE2SP_ALGO_V128_FAST)
        {
          func       = Blake2sp_Compress2_V128_Fast;
          func_Final = Blake2sp_Final_V128_Fast;
          func_Init  = Blake2sp_InitState_V128_Fast;
          func_Single = Z7_BLAKE2S_Compress2_V128;
        }
        else
#ifdef Z7_BLAKE2S_USE_V128_WAY2
        if (algo == Z7_BLAKE2SP_ALGO_V128_WAY2)
          func = func_Single = Blake2sp_Compress2_V128_Way2;
        else
#endif
        {
          if (algo != Z7_BLAKE2SP_ALGO_V128_WAY1)
            return False;
          func = func_Single = Blake2sp_Compress2_V128_Way1;
        }
      }
    }
  }
#else // !VECTORS
  if (algo > 1) // Z7_BLAKE2SP_ALGO_SCALAR
    return False;
#endif // !VECTORS

#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  p->u.header.func_Compress_Fast = func;
  p->u.header.func_Compress_Single = func_Single;
  p->u.header.func_Final = func_Final;
  p->u.header.func_Init = func_Init;
#endif
  // printf("\n p->u.header.func_Compress = %p", p->u.header.func_Compress);
  return True;
}


void z7_Black2sp_Prepare(void)
{
#ifdef Z7_BLAKE2S_USE_VECTORS
  unsigned flags = 0; // (1u << Z7_BLAKE2SP_ALGO_V128_SCALAR);

  Z7_BLAKE2SP_FUNC_COMPRESS func_Fast = Blake2sp_Compress2;
  Z7_BLAKE2SP_FUNC_COMPRESS func_Single = Blake2sp_Compress2;
  Z7_BLAKE2SP_FUNC_INIT func_Init = NULL;
  Z7_BLAKE2SP_FUNC_INIT func_Final = NULL;

#if defined(MY_CPU_X86_OR_AMD64)
    #if defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
      // optional check
      #if 0 || !(defined(__AVX512F__) && defined(__AVX512VL__))
      if (CPU_IsSupported_AVX512F_AVX512VL())
      #endif
    #elif defined(Z7_BLAKE2S_USE_SSE41)
      if (CPU_IsSupported_SSE41())
    #elif defined(Z7_BLAKE2S_USE_SSSE3)
      if (CPU_IsSupported_SSSE3())
    #elif !defined(MY_CPU_AMD64)
      if (CPU_IsSupported_SSE2())
    #endif
#endif
  {
    #if defined(Z7_BLAKE2S_USE_SSE41)
      // printf("\n========== Blake2s SSE41 128-bit\n");
    #elif defined(Z7_BLAKE2S_USE_SSSE3)
      // printf("\n========== Blake2s SSSE3 128-bit\n");
    #else
      // printf("\n========== Blake2s SSE2 128-bit\n");
    #endif
    // func_Fast = f_vector = Blake2sp_Compress2_V128_Way2;
    // printf("\n========== Blake2sp_Compress2_V128_Way2\n");
    func_Fast   =
    func_Single = Z7_BLAKE2S_Compress2_V128;
    flags |= (1u << Z7_BLAKE2SP_ALGO_V128_WAY1);
#ifdef Z7_BLAKE2S_USE_V128_WAY2
    flags |= (1u << Z7_BLAKE2SP_ALGO_V128_WAY2);
#endif
#ifdef Z7_BLAKE2S_USE_V128_FAST
    flags |= (1u << Z7_BLAKE2SP_ALGO_V128_FAST);
    func_Fast  = Blake2sp_Compress2_V128_Fast;
    func_Init  = Blake2sp_InitState_V128_Fast;
    func_Final = Blake2sp_Final_V128_Fast;
#endif

#ifdef Z7_BLAKE2S_USE_AVX2
#if defined(MY_CPU_X86_OR_AMD64)
    
    #if defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
      #if 0
        if (CPU_IsSupported_AVX512F_AVX512VL())
      #endif
    #else
        if (CPU_IsSupported_AVX2())
    #endif
#endif
    {
    // #pragma message ("=== Blake2s AVX2")
    // printf("\n========== Blake2s AVX2\n");
    
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
      func_Single = Blake2sp_Compress2_AVX2_Way2;
      flags |= (1u << Z7_BLAKE2SP_ALGO_V256_WAY2);
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY4
      flags |= (1u << Z7_BLAKE2SP_ALGO_V256_WAY4);
#endif

#ifdef Z7_BLAKE2S_USE_AVX2_FAST
      flags |= (1u << Z7_BLAKE2SP_ALGO_V256_FAST);
      func_Fast  = Blake2sp_Compress2_AVX2_Fast;
      func_Init  = Blake2sp_InitState_AVX2_Fast;
      func_Final = Blake2sp_Final_AVX2_Fast;
#elif defined(Z7_BLAKE2S_USE_AVX2_WAY4)
      func_Fast  = Blake2sp_Compress2_AVX2_Way4;
#elif defined(Z7_BLAKE2S_USE_AVX2_WAY2)
      func_Fast  = Blake2sp_Compress2_AVX2_Way2;
#endif
    } // avx2
#endif // avx2
  } // sse*
  g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast   = func_Fast;
  g_Z7_BLAKE2SP_FUNC_COMPRESS_Single = func_Single;
  g_Z7_BLAKE2SP_FUNC_INIT_Init       = func_Init;
  g_Z7_BLAKE2SP_FUNC_INIT_Final      = func_Final;
  g_z7_Blake2sp_SupportedFlags = flags;
  // printf("\nflags=%x\n", flags);
#endif // vectors
}

/*
#ifdef Z7_BLAKE2S_USE_VECTORS
void align_test2(CBlake2sp *sp);
void align_test2(CBlake2sp *sp)
{
  __m128i a = LOAD_128(sp->states);
  D_XOR_128(a, LOAD_128(sp->states + 4));
  STORE_128(sp->states, a);
}
void align_test2(void);
void align_test2(void)
{
  CBlake2sp sp;
  Blake2sp_Init(&sp);
  Blake2sp_Update(&sp, NULL, 0);
}
#endif
*/

/* ================ unit: C/Bra.c ================ */
/* Bra.c -- Branch converters for RISC code
2024-01-20 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#if defined(MY_CPU_SIZEOF_POINTER) \
    && ( MY_CPU_SIZEOF_POINTER == 4 \
      || MY_CPU_SIZEOF_POINTER == 8)
  #define BR_CONV_USE_OPT_PC_PTR
#endif

#ifdef BR_CONV_USE_OPT_PC_PTR
#define BR_PC_INIT  pc -= (UInt32)(SizeT)p;
#define BR_PC_GET   (pc + (UInt32)(SizeT)p)
#else
#define BR_PC_INIT  pc += (UInt32)size;
#define BR_PC_GET   (pc - (UInt32)(SizeT)(lim - p))
// #define BR_PC_INIT
// #define BR_PC_GET   (pc + (UInt32)(SizeT)(p - data))
#endif

#define BR_CONVERT_VAL(v, c) if (encoding) v += c; else v -= c;
// #define BR_CONVERT_VAL(v, c) if (!encoding) c = (UInt32)0 - c; v += c;

#define Z7_BRANCH_CONV(name) z7_ ## name

#define Z7_BRANCH_FUNC_MAIN(name) \
static \
Z7_FORCE_INLINE \
Z7_ATTRIB_NO_VECTOR \
Byte *Z7_BRANCH_CONV(name)(Byte *p, SizeT size, UInt32 pc, int encoding)

#define Z7_BRANCH_FUNC_IMP(name, m, encoding) \
Z7_NO_INLINE \
Z7_ATTRIB_NO_VECTOR \
Byte *m(name)(Byte *data, SizeT size, UInt32 pc) \
  { return Z7_BRANCH_CONV(name)(data, size, pc, encoding); } \

#ifdef Z7_EXTRACT_ONLY
#define Z7_BRANCH_FUNCS_IMP(name) \
  Z7_BRANCH_FUNC_IMP(name, Z7_BRANCH_CONV_DEC_2, 0)
#else
#define Z7_BRANCH_FUNCS_IMP(name) \
  Z7_BRANCH_FUNC_IMP(name, Z7_BRANCH_CONV_DEC_2, 0) \
  Z7_BRANCH_FUNC_IMP(name, Z7_BRANCH_CONV_ENC_2, 1)
#endif

#if defined(__clang__)
#define BR_EXTERNAL_FOR
#define BR_NEXT_ITERATION  continue;
#else
#define BR_EXTERNAL_FOR    for (;;)
#define BR_NEXT_ITERATION  break;
#endif

#if defined(__clang__) && (__clang_major__ >= 8) \
  || defined(__GNUC__) && (__GNUC__ >= 1000) \
  // GCC is not good for __builtin_expect() here
  /* || defined(_MSC_VER) && (_MSC_VER >= 1920) */
  // #define Z7_unlikely [[unlikely]]
  // #define Z7_LIKELY(x)   (__builtin_expect((x), 1))
  #define Z7_UNLIKELY(x) (__builtin_expect((x), 0))
  // #define Z7_likely [[likely]]
#else
  // #define Z7_LIKELY(x)   (x)
  #define Z7_UNLIKELY(x) (x)
  // #define Z7_likely
#endif


Z7_BRANCH_FUNC_MAIN(BranchConv_ARM64)
{
  // Byte *p = data;
  const Byte *lim;
  const UInt32 flag = (UInt32)1 << (24 - 4);
  const UInt32 mask = ((UInt32)1 << 24) - (flag << 1);
  size &= ~(SizeT)3;
  // if (size == 0) return p;
  lim = p + size;
  BR_PC_INIT
  pc -= 4;  // because (p) will point to next instruction
  
  BR_EXTERNAL_FOR
  {
    // Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
    for (;;)
    {
      UInt32 v;
      if Z7_UNLIKELY(p == lim)
        return p;
      v = GetUi32a(p);
      p += 4;
      if Z7_UNLIKELY(((v - 0x94000000) & 0xfc000000) == 0)
      {
        UInt32 c = BR_PC_GET >> 2;
        BR_CONVERT_VAL(v, c)
        v &= 0x03ffffff;
        v |= 0x94000000;
        SetUi32a(p - 4, v)
        BR_NEXT_ITERATION
      }
      // v = rotlFixed(v, 8);  v += (flag << 8) - 0x90;  if Z7_UNLIKELY((v & ((mask << 8) + 0x9f)) == 0)
      v -= 0x90000000;  if Z7_UNLIKELY((v & 0x9f000000) == 0)
      {
        UInt32 z, c;
        // v = rotrFixed(v, 8);
        v += flag; if Z7_UNLIKELY(v & mask) continue;
        z = (v & 0xffffffe0) | (v >> 26);
        c = (BR_PC_GET >> (12 - 3)) & ~(UInt32)7;
        BR_CONVERT_VAL(z, c)
        v &= 0x1f;
        v |= 0x90000000;
        v |= z << 26;
        v |= 0x00ffffe0 & ((z & (((flag << 1) - 1))) - flag);
        SetUi32a(p - 4, v)
      }
    }
  }
}
Z7_BRANCH_FUNCS_IMP(BranchConv_ARM64)


Z7_BRANCH_FUNC_MAIN(BranchConv_ARM)
{
  // Byte *p = data;
  const Byte *lim;
  size &= ~(SizeT)3;
  lim = p + size;
  BR_PC_INIT
  /* in ARM: branch offset is relative to the +2 instructions from current instruction.
     (p) will point to next instruction */
  pc += 8 - 4;
  
  for (;;)
  {
    for (;;)
    {
      if Z7_UNLIKELY(p >= lim) { return p; }  p += 4;  if Z7_UNLIKELY(p[-1] == 0xeb) break;
      if Z7_UNLIKELY(p >= lim) { return p; }  p += 4;  if Z7_UNLIKELY(p[-1] == 0xeb) break;
    }
    {
      UInt32 v = GetUi32a(p - 4);
      UInt32 c = BR_PC_GET >> 2;
      BR_CONVERT_VAL(v, c)
      v &= 0x00ffffff;
      v |= 0xeb000000;
      SetUi32a(p - 4, v)
    }
  }
}
Z7_BRANCH_FUNCS_IMP(BranchConv_ARM)


Z7_BRANCH_FUNC_MAIN(BranchConv_PPC)
{
  // Byte *p = data;
  const Byte *lim;
  size &= ~(SizeT)3;
  lim = p + size;
  BR_PC_INIT
  pc -= 4;  // because (p) will point to next instruction
  
  for (;;)
  {
    UInt32 v;
    for (;;)
    {
      if Z7_UNLIKELY(p == lim)
        return p;
      // v = GetBe32a(p);
      v = *(UInt32 *)(void *)p;
      p += 4;
      // if ((v & 0xfc000003) == 0x48000001) break;
      // if ((p[-4] & 0xFC) == 0x48 && (p[-1] & 3) == 1) break;
      if Z7_UNLIKELY(
          ((v - Z7_CONV_BE_TO_NATIVE_CONST32(0x48000001))
              & Z7_CONV_BE_TO_NATIVE_CONST32(0xfc000003)) == 0) break;
    }
    {
      v = Z7_CONV_NATIVE_TO_BE_32(v);
      {
        UInt32 c = BR_PC_GET;
        BR_CONVERT_VAL(v, c)
      }
      v &= 0x03ffffff;
      v |= 0x48000000;
      SetBe32a(p - 4, v)
    }
  }
}
Z7_BRANCH_FUNCS_IMP(BranchConv_PPC)


#ifdef Z7_CPU_FAST_ROTATE_SUPPORTED
#define BR_SPARC_USE_ROTATE
#endif

Z7_BRANCH_FUNC_MAIN(BranchConv_SPARC)
{
  // Byte *p = data;
  const Byte *lim;
  const UInt32 flag = (UInt32)1 << 22;
  size &= ~(SizeT)3;
  lim = p + size;
  BR_PC_INIT
  pc -= 4;  // because (p) will point to next instruction
  for (;;)
  {
    UInt32 v;
    for (;;)
    {
      if Z7_UNLIKELY(p == lim)
        return p;
      /* // the code without GetBe32a():
      { const UInt32 v = GetUi16a(p) & 0xc0ff; p += 4; if (v == 0x40 || v == 0xc07f) break; }
      */
      v = GetBe32a(p);
      p += 4;
    #ifdef BR_SPARC_USE_ROTATE
      v = rotlFixed(v, 2);
      v += (flag << 2) - 1;
      if Z7_UNLIKELY((v & (3 - (flag << 3))) == 0)
    #else
      v += (UInt32)5 << 29;
      v ^= (UInt32)7 << 29;
      v += flag;
      if Z7_UNLIKELY((v & (0 - (flag << 1))) == 0)
    #endif
        break;
    }
    {
      // UInt32 v = GetBe32a(p - 4);
    #ifndef BR_SPARC_USE_ROTATE
      v <<= 2;
    #endif
      {
        UInt32 c = BR_PC_GET;
        BR_CONVERT_VAL(v, c)
      }
      v &= (flag << 3) - 1;
    #ifdef BR_SPARC_USE_ROTATE
      v -= (flag << 2) - 1;
      v = rotrFixed(v, 2);
    #else
      v -= (flag << 2);
      v >>= 2;
      v |= (UInt32)1 << 30;
    #endif
      SetBe32a(p - 4, v)
    }
  }
}
Z7_BRANCH_FUNCS_IMP(BranchConv_SPARC)


Z7_BRANCH_FUNC_MAIN(BranchConv_ARMT)
{
  // Byte *p = data;
  Byte *lim;
  size &= ~(SizeT)1;
  // if (size == 0) return p;
  if (size <= 2) return p;
  size -= 2;
  lim = p + size;
  BR_PC_INIT
  /* in ARM: branch offset is relative to the +2 instructions from current instruction.
     (p) will point to the +2 instructions from current instruction */
  // pc += 4 - 4;
  // if (encoding) pc -= 0xf800 << 1; else pc += 0xf800 << 1;
  // #define ARMT_TAIL_PROC { goto armt_tail; }
  #define ARMT_TAIL_PROC { return p; }
  
  do
  {
    /* in MSVC 32-bit x86 compilers:
       UInt32 version : it loads value from memory with movzx
       Byte   version : it loads value to 8-bit register (AL/CL)
       movzx version is slightly faster in some cpus
    */
    unsigned b1;
    // Byte / unsigned
    b1 = p[1];
    // optimized version to reduce one (p >= lim) check:
    // unsigned a1 = p[1];  b1 = p[3];  p += 2;  if Z7_LIKELY((b1 & (a1 ^ 8)) < 0xf8)
    for (;;)
    {
      unsigned b3; // Byte / UInt32
      /* (Byte)(b3) normalization can use low byte computations in MSVC.
         It gives smaller code, and no loss of speed in some compilers/cpus.
         But new MSVC 32-bit x86 compilers use more slow load
         from memory to low byte register in that case.
         So we try to use full 32-bit computations for faster code.
      */
      // if (p >= lim) { ARMT_TAIL_PROC }  b3 = b1 + 8;  b1 = p[3];  p += 2;  if ((b3 & b1) >= 0xf8) break;
      if Z7_UNLIKELY(p >= lim) { ARMT_TAIL_PROC }  b3 = p[3];  p += 2;  if Z7_UNLIKELY((b3 & (b1 ^ 8)) >= 0xf8) break;
      if Z7_UNLIKELY(p >= lim) { ARMT_TAIL_PROC }  b1 = p[3];  p += 2;  if Z7_UNLIKELY((b1 & (b3 ^ 8)) >= 0xf8) break;
    }
    {
      /* we can adjust pc for (0xf800) to rid of (& 0x7FF) operation.
         But gcc/clang for arm64 can use bfi instruction for full code here */
      UInt32 v =
          ((UInt32)GetUi16a(p - 2) << 11) |
          ((UInt32)GetUi16a(p) & 0x7FF);
      /*
      UInt32 v =
            ((UInt32)p[1 - 2] << 19)
          + (((UInt32)p[1] & 0x7) << 8)
          + (((UInt32)p[-2] << 11))
          + (p[0]);
      */
      p += 2;
      {
        UInt32 c = BR_PC_GET >> 1;
        BR_CONVERT_VAL(v, c)
      }
      SetUi16a(p - 4, (UInt16)(((v >> 11) & 0x7ff) | 0xf000))
      SetUi16a(p - 2, (UInt16)(v | 0xf800))
      /*
      p[-4] = (Byte)(v >> 11);
      p[-3] = (Byte)(0xf0 | ((v >> 19) & 0x7));
      p[-2] = (Byte)v;
      p[-1] = (Byte)(0xf8 | (v >> 8));
      */
    }
  }
  while (p < lim);
  return p;
  // armt_tail:
  // if ((Byte)((lim[1] & 0xf8)) != 0xf0) { lim += 2; }  return lim;
  // return (Byte *)(lim + ((Byte)((lim[1] ^ 0xf0) & 0xf8) == 0 ? 0 : 2));
  // return (Byte *)(lim + (((lim[1] ^ ~0xfu) & ~7u) == 0 ? 0 : 2));
  // return (Byte *)(lim + 2 - (((((unsigned)lim[1] ^ 8) + 8) >> 7) & 2));
}
Z7_BRANCH_FUNCS_IMP(BranchConv_ARMT)


// #define BR_IA64_NO_INLINE

Z7_BRANCH_FUNC_MAIN(BranchConv_IA64)
{
  // Byte *p = data;
  const Byte *lim;
  size &= ~(SizeT)15;
  lim = p + size;
  pc -= 1 << 4;
  pc >>= 4 - 1;
  // pc -= 1 << 1;
  
  for (;;)
  {
    unsigned m;
    for (;;)
    {
      if Z7_UNLIKELY(p == lim)
        return p;
      m = (unsigned)((UInt32)0x334b0000 >> (*p & 0x1e));
      p += 16;
      pc += 1 << 1;
      if (m &= 3)
        break;
    }
    {
      p += (ptrdiff_t)m * 5 - 20; // negative value is expected here.
      do
      {
        const UInt32 t =
          #if defined(MY_CPU_X86_OR_AMD64)
            // we use 32-bit load here to reduce code size on x86:
            GetUi32(p);
          #else
            GetUi16(p);
          #endif
        UInt32 z = GetUi32(p + 1) >> m;
        p += 5;
        if (((t >> m) & (0x70 << 1)) == 0
            && ((z - (0x5000000 << 1)) & (0xf000000 << 1)) == 0)
        {
          UInt32 v = (UInt32)((0x8fffff << 1) | 1) & z;
          z ^= v;
        #ifdef BR_IA64_NO_INLINE
          v |= (v & ((UInt32)1 << (23 + 1))) >> 3;
          {
            UInt32 c = pc;
            BR_CONVERT_VAL(v, c)
          }
          v &= (0x1fffff << 1) | 1;
        #else
          {
            if (encoding)
            {
              // pc &= ~(0xc00000 << 1); // we just need to clear at least 2 bits
              pc &= (0x1fffff << 1) | 1;
              v += pc;
            }
            else
            {
              // pc |= 0xc00000 << 1; // we need to set at least 2 bits
              pc |= ~(UInt32)((0x1fffff << 1) | 1);
              v -= pc;
            }
          }
          v &= ~(UInt32)(0x600000 << 1);
        #endif
          v += (0x700000 << 1);
          v &= (0x8fffff << 1) | 1;
          z |= v;
          z <<= m;
          SetUi32(p + 1 - 5, z)
        }
        m++;
      }
      while (m &= 3); // while (m < 4);
    }
  }
}
Z7_BRANCH_FUNCS_IMP(BranchConv_IA64)


#define BR_CONVERT_VAL_ENC(v)  v += BR_PC_GET;
#define BR_CONVERT_VAL_DEC(v)  v -= BR_PC_GET;

#if 1 && defined(MY_CPU_LE_UNALIGN)
  #define RISCV_USE_UNALIGNED_LOAD
#endif

#ifdef RISCV_USE_UNALIGNED_LOAD
  #define RISCV_GET_UI32(p)      GetUi32(p)
  #define RISCV_SET_UI32(p, v)   { SetUi32(p, v) }
#else
  #define RISCV_GET_UI32(p) \
    ((UInt32)GetUi16a(p) + \
    ((UInt32)GetUi16a((p) + 2) << 16))
  #define RISCV_SET_UI32(p, v) { \
    SetUi16a(p, (UInt16)(v)) \
    SetUi16a((p) + 2, (UInt16)(v >> 16)) }
#endif

#if 1 && defined(MY_CPU_LE)
  #define RISCV_USE_16BIT_LOAD
#endif

#ifdef RISCV_USE_16BIT_LOAD
  #define RISCV_LOAD_VAL(p)  GetUi16a(p)
#else
  #define RISCV_LOAD_VAL(p)  (*(p))
#endif

#define RISCV_INSTR_SIZE  2
#define RISCV_STEP_1      (4 + RISCV_INSTR_SIZE)
#define RISCV_STEP_2      4
#define RISCV_REG_VAL     (2 << 7)
#define RISCV_CMD_VAL     3
#if 1
  // for code size optimization:
  #define RISCV_DELTA_7F  0x7f
#else
  #define RISCV_DELTA_7F  0
#endif

#define RISCV_CHECK_1(v, b) \
    (((((b) - RISCV_CMD_VAL) ^ ((v) << 8)) & (0xf8000 + RISCV_CMD_VAL)) == 0)

#if 1
  #define RISCV_CHECK_2(v, r) \
    ((((v) - ((RISCV_CMD_VAL << 12) | RISCV_REG_VAL | 8)) \
           << 18) \
     < ((r) & 0x1d))
#else
  // this branch gives larger code, because
  // compilers generate larger code for big constants.
  #define RISCV_CHECK_2(v, r) \
    ((((v) - ((RISCV_CMD_VAL << 12) | RISCV_REG_VAL)) \
           & ((RISCV_CMD_VAL << 12) | RISCV_REG_VAL)) \
     < ((r) & 0x1d))
#endif


#define RISCV_SCAN_LOOP \
  Byte *lim; \
  size &= ~(SizeT)(RISCV_INSTR_SIZE - 1); \
  if (size <= 6) return p; \
  size -= 6; \
  lim = p + size; \
  BR_PC_INIT \
  for (;;) \
  { \
    UInt32 a, v; \
    /* Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE */ \
    for (;;) \
    { \
      if Z7_UNLIKELY(p >= lim) { return p; } \
      a = (RISCV_LOAD_VAL(p) ^ 0x10u) + 1; \
      if ((a & 0x77) == 0) break; \
      a = (RISCV_LOAD_VAL(p + RISCV_INSTR_SIZE) ^ 0x10u) + 1; \
      p += RISCV_INSTR_SIZE * 2; \
      if ((a & 0x77) == 0) \
      { \
        p -= RISCV_INSTR_SIZE; \
        if Z7_UNLIKELY(p >= lim) { return p; } \
        break; \
      } \
    }
// (xx6f ^ 10) + 1 = xx7f + 1 = xx80       : JAL
// (xxef ^ 10) + 1 = xxff + 1 = xx00 + 100 : JAL
// (xx17 ^ 10) + 1 = xx07 + 1 = xx08       : AUIPC
// (xx97 ^ 10) + 1 = xx87 + 1 = xx88       : AUIPC

Byte * Z7_BRANCH_CONV_ENC(RISCV)(Byte *p, SizeT size, UInt32 pc)
{
  RISCV_SCAN_LOOP
    v = a;
    a = RISCV_GET_UI32(p);
#ifndef RISCV_USE_16BIT_LOAD
    v += (UInt32)p[1] << 8;
#endif

    if ((v & 8) == 0) // JAL
    {
      if ((v - (0x100 /* - RISCV_DELTA_7F */)) & 0xd80)
      {
        p += RISCV_INSTR_SIZE;
        continue;
      }
      {
        v = ((a &    1u << 31) >> 11)
          | ((a & 0x3ff << 21) >> 20)
          | ((a &     1 << 20) >> 9)
          |  (a &  0xff << 12);
        BR_CONVERT_VAL_ENC(v)
        // ((v & 1) == 0)
        // v: bits [1 : 20] contain offset bits
#if 0 && defined(RISCV_USE_UNALIGNED_LOAD)
        a &= 0xfff;
        a |= ((UInt32)(v << 23))
          |  ((UInt32)(v <<  7) & ((UInt32)0xff << 16))
          |  ((UInt32)(v >>  5) & ((UInt32)0xf0 << 8));
        RISCV_SET_UI32(p, a)
#else // aligned
#if 0
        SetUi16a(p, (UInt16)(((v >> 5) & 0xf000) | (a & 0xfff)))
#else
        p[1] = (Byte)(((v >> 13) & 0xf0) | ((a >> 8) & 0xf));
#endif

#if 1 && defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_LE)
        v <<= 15;
        v = Z7_BSWAP32(v);
        SetUi16a(p + 2, (UInt16)v)
#else
        p[2] = (Byte)(v >> 9);
        p[3] = (Byte)(v >> 1);
#endif
#endif // aligned
      }
      p += 4;
      continue;
    } // JAL

    {
      // AUIPC
      if (v & 0xe80)  // (not x0) and (not x2)
      {
        const UInt32 b = RISCV_GET_UI32(p + 4);
        if (RISCV_CHECK_1(v, b))
        {
          {
            const UInt32 temp = (b << 12) | (0x17 + RISCV_REG_VAL);
            RISCV_SET_UI32(p, temp)
          }
          a &= 0xfffff000;
          {
#if 1
          const int t = -1 >> 1;
          if (t != -1)
            a += (b >> 20) - ((b >> 19) & 0x1000); // arithmetic right shift emulation
          else
#endif
            a += (UInt32)((Int32)b >> 20); // arithmetic right shift (sign-extension).
          }
          BR_CONVERT_VAL_ENC(a)
#if 1 && defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_LE)
          a = Z7_BSWAP32(a);
          RISCV_SET_UI32(p + 4, a)
#else
          SetBe32(p + 4, a)
#endif
          p += 8;
        }
        else
          p += RISCV_STEP_1;
      }
      else
      {
        UInt32 r = a >> 27;
        if (RISCV_CHECK_2(v, r))
        {
          v = RISCV_GET_UI32(p + 4);
          r = (r << 7) + 0x17 + (v & 0xfffff000);
          a = (a >> 12) | (v << 20);
          RISCV_SET_UI32(p, r)
          RISCV_SET_UI32(p + 4, a)
          p += 8;
        }
        else
          p += RISCV_STEP_2;
      }
    }
  } // for
}


Byte * Z7_BRANCH_CONV_DEC(RISCV)(Byte *p, SizeT size, UInt32 pc)
{
  RISCV_SCAN_LOOP
#ifdef RISCV_USE_16BIT_LOAD
    if ((a & 8) == 0)
    {
#else
    v = a;
    a += (UInt32)p[1] << 8;
    if ((v & 8) == 0)
    {
#endif
      // JAL
      a -= 0x100 - RISCV_DELTA_7F;
      if (a & 0xd80)
      {
        p += RISCV_INSTR_SIZE;
        continue;
      }
      {
        const UInt32 a_old = (a + (0xef - RISCV_DELTA_7F)) & 0xfff;
#if 0 // unaligned
        a = GetUi32(p);
        v = (UInt32)(a >> 23) & ((UInt32)0xff << 1)
          | (UInt32)(a >>  7) & ((UInt32)0xff << 9)
#elif 1 && defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_LE)
        v = GetUi16a(p + 2);
        v = Z7_BSWAP32(v) >> 15
#else
        v = (UInt32)p[3] << 1
          | (UInt32)p[2] << 9
#endif
          | (UInt32)((a & 0xf000) << 5);
        BR_CONVERT_VAL_DEC(v)
        a = a_old
          | (v << 11 &    1u << 31)
          | (v << 20 & 0x3ff << 21)
          | (v <<  9 &     1 << 20)
          | (v       &  0xff << 12);
        RISCV_SET_UI32(p, a)
      }
      p += 4;
      continue;
    } // JAL

    {
      // AUIPC
      v = a;
#if 1 && defined(RISCV_USE_UNALIGNED_LOAD)
      a = GetUi32(p);
#else
      a |= (UInt32)GetUi16a(p + 2) << 16;
#endif
      if ((v & 0xe80) == 0)  // x0/x2
      {
        const UInt32 r = a >> 27;
        if (RISCV_CHECK_2(v, r))
        {
          UInt32 b;
#if 1 && defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_LE)
          b = RISCV_GET_UI32(p + 4);
          b = Z7_BSWAP32(b);
#else
          b = GetBe32(p + 4);
#endif
          v = a >> 12;
          BR_CONVERT_VAL_DEC(b)
          a = (r << 7) + 0x17;
          a += (b + 0x800) & 0xfffff000;
          v |= b << 20;
          RISCV_SET_UI32(p, a)
          RISCV_SET_UI32(p + 4, v)
          p += 8;
        }
        else
          p += RISCV_STEP_2;
      }
      else
      {
        const UInt32 b = RISCV_GET_UI32(p + 4);
        if (!RISCV_CHECK_1(v, b))
          p += RISCV_STEP_1;
        else
        {
          v = (a & 0xfffff000) | (b >> 20);
          a = (b << 12) | (0x17 + RISCV_REG_VAL);
          RISCV_SET_UI32(p, a)
          RISCV_SET_UI32(p + 4, v)
          p += 8;
        }
      }
    }
  } // for
}

/* ================ unit: C/Bra86.c ================ */
/* Bra86.c -- Branch converter for X86 code (BCJ)
2023-04-02 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue


#if defined(MY_CPU_SIZEOF_POINTER) \
    && ( MY_CPU_SIZEOF_POINTER == 4 \
      || MY_CPU_SIZEOF_POINTER == 8)
  #define BR_CONV_USE_OPT_PC_PTR
#endif

#ifdef BR_CONV_USE_OPT_PC_PTR
#define BR_PC_INIT  pc -= (UInt32)(SizeT)p; // (MY_uintptr_t)
#define BR_PC_GET   (pc + (UInt32)(SizeT)p)
#else
#define BR_PC_INIT  pc += (UInt32)size;
#define BR_PC_GET   (pc - (UInt32)(SizeT)(lim - p))
// #define BR_PC_INIT
// #define BR_PC_GET   (pc + (UInt32)(SizeT)(p - data))
#endif

#define BR_CONVERT_VAL(v, c) if (encoding) v += c; else v -= c;
// #define BR_CONVERT_VAL(v, c) if (!encoding) c = (UInt32)0 - c; v += c;

#define Z7_BRANCH_CONV_ST(name) z7_BranchConvSt_ ## name

#define BR86_NEED_CONV_FOR_MS_BYTE(b) ((((b) + 1) & 0xfe) == 0)

#ifdef MY_CPU_LE_UNALIGN
  #define BR86_PREPARE_BCJ_SCAN  const UInt32 v = GetUi32(p) ^ 0xe8e8e8e8;
  #define BR86_IS_BCJ_BYTE(n)    ((v & ((UInt32)0xfe << (n) * 8)) == 0)
#else
  #define BR86_PREPARE_BCJ_SCAN
  // bad for MSVC X86 (partial write to byte reg):
  #define BR86_IS_BCJ_BYTE(n)    ((p[n - 4] & 0xfe) == 0xe8)
  // bad for old MSVC (partial write to byte reg):
  // #define BR86_IS_BCJ_BYTE(n)    (((*p ^ 0xe8) & 0xfe) == 0)
#endif
 
static
Z7_FORCE_INLINE
Z7_ATTRIB_NO_VECTOR
Byte *Z7_BRANCH_CONV_ST(X86)(Byte *p, SizeT size, UInt32 pc, UInt32 *state, int encoding)
{
  if (size < 5)
    return p;
 {
  // Byte *p = data;
  const Byte *lim = p + size - 4;
  unsigned mask = (unsigned)*state;  // & 7;
#ifdef BR_CONV_USE_OPT_PC_PTR
  /* if BR_CONV_USE_OPT_PC_PTR is defined: we need to adjust (pc) for (+4),
        because call/jump offset is relative to the next instruction.
     if BR_CONV_USE_OPT_PC_PTR is not defined : we don't need to adjust (pc) for (+4),
         because  BR_PC_GET uses (pc - (lim - p)), and lim was adjusted for (-4) before.
  */
  pc += 4;
#endif
  BR_PC_INIT
  goto start;

  for (;; mask |= 4)
  {
    // cont: mask |= 4;
  start:
    if (p >= lim)
      goto fin;
    {
      BR86_PREPARE_BCJ_SCAN
      p += 4;
      if (BR86_IS_BCJ_BYTE(0))  { goto m0; }  mask >>= 1;
      if (BR86_IS_BCJ_BYTE(1))  { goto m1; }  mask >>= 1;
      if (BR86_IS_BCJ_BYTE(2))  { goto m2; }  mask = 0;
      if (BR86_IS_BCJ_BYTE(3))  { goto a3; }
    }
    goto main_loop;

  m0: p--;
  m1: p--;
  m2: p--;
    if (mask == 0)
      goto a3;
    if (p > lim)
      goto fin_p;
   
    // if (((0x17u >> mask) & 1) == 0)
    if (mask > 4 || mask == 3)
    {
      mask >>= 1;
      continue; // goto cont;
    }
    mask >>= 1;
    if (BR86_NEED_CONV_FOR_MS_BYTE(p[mask]))
      continue; // goto cont;
    // if (!BR86_NEED_CONV_FOR_MS_BYTE(p[3])) continue; // goto cont;
    {
      UInt32 v = GetUi32(p);
      UInt32 c;
      v += (1 << 24);  if (v & 0xfe000000) continue; // goto cont;
      c = BR_PC_GET;
      BR_CONVERT_VAL(v, c)
      {
        mask <<= 3;
        if (BR86_NEED_CONV_FOR_MS_BYTE(v >> mask))
        {
          v ^= (((UInt32)0x100 << mask) - 1);
          #ifdef MY_CPU_X86
          // for X86 : we can recalculate (c) to reduce register pressure
            c = BR_PC_GET;
          #endif
          BR_CONVERT_VAL(v, c)
        }
        mask = 0;
      }
      // v = (v & ((1 << 24) - 1)) - (v & (1 << 24));
      v &= (1 << 25) - 1;  v -= (1 << 24);
      SetUi32(p, v)
      p += 4;
      goto main_loop;
    }

  main_loop:
    if (p >= lim)
      goto fin;
    for (;;)
    {
      BR86_PREPARE_BCJ_SCAN
      p += 4;
      if (BR86_IS_BCJ_BYTE(0))  { goto a0; }
      if (BR86_IS_BCJ_BYTE(1))  { goto a1; }
      if (BR86_IS_BCJ_BYTE(2))  { goto a2; }
      if (BR86_IS_BCJ_BYTE(3))  { goto a3; }
      if (p >= lim)
        goto fin;
    }
  
  a0: p--;
  a1: p--;
  a2: p--;
  a3:
    if (p > lim)
      goto fin_p;
    // if (!BR86_NEED_CONV_FOR_MS_BYTE(p[3])) continue; // goto cont;
    {
      UInt32 v = GetUi32(p);
      UInt32 c;
      v += (1 << 24);  if (v & 0xfe000000) continue; // goto cont;
      c = BR_PC_GET;
      BR_CONVERT_VAL(v, c)
      // v = (v & ((1 << 24) - 1)) - (v & (1 << 24));
      v &= (1 << 25) - 1;  v -= (1 << 24);
      SetUi32(p, v)
      p += 4;
      goto main_loop;
    }
  }

fin_p:
  p--;
fin:
  // the following processing for tail is optional and can be commented
  /*
  lim += 4;
  for (; p < lim; p++, mask >>= 1)
    if ((*p & 0xfe) == 0xe8)
      break;
  */
  *state = (UInt32)mask;
  return p;
 }
}


#define Z7_BRANCH_CONV_ST_FUNC_IMP(name, m, encoding) \
Z7_NO_INLINE \
Z7_ATTRIB_NO_VECTOR \
Byte *m(name)(Byte *data, SizeT size, UInt32 pc, UInt32 *state) \
  { return Z7_BRANCH_CONV_ST(name)(data, size, pc, state, encoding); }

Z7_BRANCH_CONV_ST_FUNC_IMP(X86, Z7_BRANCH_CONV_ST_DEC, 0)
#ifndef Z7_EXTRACT_ONLY
Z7_BRANCH_CONV_ST_FUNC_IMP(X86, Z7_BRANCH_CONV_ST_ENC, 1)
#endif

/* ================ unit: C/BraIA64.c ================ */
/* BraIA64.c -- Converter for IA-64 code
2023-02-20 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// the code was moved to Bra.c

#ifdef _MSC_VER
#pragma warning(disable : 4206) // nonstandard extension used : translation unit is empty
#endif

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wempty-translation-unit"
#endif

/* ================ unit: C/BwtSort.c ================ */
/* BwtSort.c -- BWT block sorting
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

/* #define BLOCK_SORT_USE_HEAP_SORT */
// #define BLOCK_SORT_USE_HEAP_SORT

#ifdef BLOCK_SORT_USE_HEAP_SORT

#define HeapSortRefDown(p, vals, n, size, temp) \
  { size_t k = n; UInt32 val = vals[temp]; for (;;) { \
    size_t s = k << 1; \
    if (s > size) break; \
    if (s < size && vals[p[s + 1]] > vals[p[s]]) s++; \
    if (val >= vals[p[s]]) break; \
    p[k] = p[s]; k = s; \
  } p[k] = temp; }

void HeapSortRef(UInt32 *p, UInt32 *vals, size_t size)
{
  if (size <= 1)
    return;
  p--;
  {
    size_t i = size / 2;
    do
    {
      UInt32 temp = p[i];
      HeapSortRefDown(p, vals, i, size, temp);
    }
    while (--i != 0);
  }
  do
  {
    UInt32 temp = p[size];
    p[size--] = p[1];
    HeapSortRefDown(p, vals, 1, size, temp);
  }
  while (size > 1);
}

#endif // BLOCK_SORT_USE_HEAP_SORT


/* Don't change it !!! */
#define kNumHashBytes 2
#define kNumHashValues (1 << (kNumHashBytes * 8))

/* kNumRefBitsMax must be < (kNumHashBytes * 8) = 16 */
#define kNumRefBitsMax 12

#define BS_TEMP_SIZE kNumHashValues

#ifdef BLOCK_SORT_EXTERNAL_FLAGS

/* 32 Flags in UInt32 word */
#define kNumFlagsBits 5
#define kNumFlagsInWord (1 << kNumFlagsBits)
#define kFlagsMask (kNumFlagsInWord - 1)
#define kAllFlags 0xFFFFFFFF

#else

#define kNumBitsMax     20
#define kIndexMask      (((UInt32)1 << kNumBitsMax) - 1)
#define kNumExtraBits   (32 - kNumBitsMax)
#define kNumExtra0Bits  (kNumExtraBits - 2)
#define kNumExtra0Mask  ((1 << kNumExtra0Bits) - 1)

#define SetFinishedGroupSize(p, size) \
  {  *(p) |= ((((UInt32)(size) - 1) & kNumExtra0Mask) << kNumBitsMax); \
    if ((size) > (1 << kNumExtra0Bits)) { \
      *(p) |= 0x40000000; \
      *((p) + 1) |= (((UInt32)(size) - 1) >> kNumExtra0Bits) << kNumBitsMax; } } \

static void SetGroupSize(UInt32 *p, size_t size)
{
  if (--size == 0)
    return;
  *p |= 0x80000000 | (((UInt32)size & kNumExtra0Mask) << kNumBitsMax);
  if (size >= (1 << kNumExtra0Bits))
  {
    *p |= 0x40000000;
    p[1] |= (((UInt32)size >> kNumExtra0Bits) << kNumBitsMax);
  }
}

#endif

/*
SortGroup - is recursive Range-Sort function with HeapSort optimization for small blocks
  "range" is not real range. It's only for optimization.
returns: 1 - if there are groups, 0 - no more groups
*/

static
unsigned
Z7_FASTCALL
SortGroup(size_t BlockSize, size_t NumSortedBytes,
    size_t groupOffset, size_t groupSize,
    unsigned NumRefBits, UInt32 *Indices
#ifndef BLOCK_SORT_USE_HEAP_SORT
    , size_t left, size_t range
#endif
  )
{
  UInt32 *ind2 = Indices + groupOffset;
  UInt32 *Groups;
  if (groupSize <= 1)
  {
    /*
    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetFinishedGroupSize(ind2, 1)
    #endif
    */
    return 0;
  }
  Groups = Indices + BlockSize + BS_TEMP_SIZE;
  if (groupSize <= ((size_t)1 << NumRefBits)
#ifndef BLOCK_SORT_USE_HEAP_SORT
      && groupSize <= range
#endif
      )
  {
    UInt32 *temp = Indices + BlockSize;
    size_t j, group;
    UInt32 mask, cg;
    unsigned thereAreGroups;
    {
      UInt32 gPrev;
      UInt32 gRes = 0;
      {
        size_t sp = ind2[0] + NumSortedBytes;
        if (sp >= BlockSize)
            sp -= BlockSize;
        gPrev = Groups[sp];
        temp[0] = gPrev << NumRefBits;
      }
      
      for (j = 1; j < groupSize; j++)
      {
        size_t sp = ind2[j] + NumSortedBytes;
        UInt32 g;
        if (sp >= BlockSize)
            sp -= BlockSize;
        g = Groups[sp];
        temp[j] = (g << NumRefBits) | (UInt32)j;
        gRes |= (gPrev ^ g);
      }
      if (gRes == 0)
      {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
        SetGroupSize(ind2, groupSize);
#endif
        return 1;
      }
    }
    
    HeapSort(temp, groupSize);
    mask = ((UInt32)1 << NumRefBits) - 1;
    thereAreGroups = 0;
    
    group = groupOffset;
    cg = temp[0] >> NumRefBits;
    temp[0] = ind2[temp[0] & mask];

    {
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 *Flags = Groups + BlockSize;
#else
    size_t prevGroupStart = 0;
#endif
    
    for (j = 1; j < groupSize; j++)
    {
      const UInt32 val = temp[j];
      const UInt32 cgCur = val >> NumRefBits;
      
      if (cgCur != cg)
      {
        cg = cgCur;
        group = groupOffset + j;

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
        {
          const size_t t = group - 1;
          Flags[t >> kNumFlagsBits] &= ~((UInt32)1 << (t & kFlagsMask));
        }
#else
        SetGroupSize(temp + prevGroupStart, j - prevGroupStart);
        prevGroupStart = j;
#endif
      }
      else
        thereAreGroups = 1;
      {
        const UInt32 ind = ind2[val & mask];
        temp[j] = ind;
        Groups[ind] = (UInt32)group;
      }
    }

#ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetGroupSize(temp + prevGroupStart, j - prevGroupStart);
#endif
    }

    for (j = 0; j < groupSize; j++)
      ind2[j] = temp[j];
    return thereAreGroups;
  }

  /* Check that all strings are in one group (cannot sort) */
  {
    UInt32 group;
    size_t j;
    size_t sp = ind2[0] + NumSortedBytes;
    if (sp >= BlockSize)
        sp -= BlockSize;
    group = Groups[sp];
    for (j = 1; j < groupSize; j++)
    {
      sp = ind2[j] + NumSortedBytes;
      if (sp >= BlockSize)
          sp -= BlockSize;
      if (Groups[sp] != group)
        break;
    }
    if (j == groupSize)
    {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
      SetGroupSize(ind2, groupSize);
#endif
      return 1;
    }
  }

#ifndef BLOCK_SORT_USE_HEAP_SORT
  {
  /* ---------- Range Sort ---------- */
  size_t i;
  size_t mid;
  for (;;)
  {
    size_t j;
    if (range <= 1)
    {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
      SetGroupSize(ind2, groupSize);
#endif
      return 1;
    }
    mid = left + ((range + 1) >> 1);
    j = groupSize;
    i = 0;
    do
    {
      size_t sp = ind2[i] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
      if (Groups[sp] >= mid)
      {
        for (j--; j > i; j--)
        {
          sp = ind2[j] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
          if (Groups[sp] < mid)
          {
            UInt32 temp = ind2[i]; ind2[i] = ind2[j]; ind2[j] = temp;
            break;
          }
        }
        if (i >= j)
          break;
      }
    }
    while (++i < j);
    if (i == 0)
    {
      range = range - (mid - left);
      left = mid;
    }
    else if (i == groupSize)
      range = (mid - left);
    else
      break;
  }

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
  {
    const size_t t = groupOffset + i - 1;
    UInt32 *Flags = Groups + BlockSize;
    Flags[t >> kNumFlagsBits] &= ~((UInt32)1 << (t & kFlagsMask));
  }
#endif

  {
    size_t j;
    for (j = i; j < groupSize; j++)
      Groups[ind2[j]] = (UInt32)(groupOffset + i);
  }

  {
    unsigned res = SortGroup(BlockSize, NumSortedBytes, groupOffset, i, NumRefBits, Indices, left, mid - left);
    return   res | SortGroup(BlockSize, NumSortedBytes, groupOffset + i, groupSize - i, NumRefBits, Indices, mid, range - (mid - left));
  }

  }

#else // BLOCK_SORT_USE_HEAP_SORT

  /* ---------- Heap Sort ---------- */

  {
    size_t j;
    for (j = 0; j < groupSize; j++)
    {
      size_t sp = ind2[j] + NumSortedBytes;
      if (sp >= BlockSize)
          sp -= BlockSize;
      ind2[j] = (UInt32)sp;
    }

    HeapSortRef(ind2, Groups, groupSize);

    /* Write Flags */
    {
    size_t sp = ind2[0];
    UInt32 group = Groups[sp];

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 *Flags = Groups + BlockSize;
#else
    size_t prevGroupStart = 0;
#endif

    for (j = 1; j < groupSize; j++)
    {
      sp = ind2[j];
      if (Groups[sp] != group)
      {
        group = Groups[sp];
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
        {
        const size_t t = groupOffset + j - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
        }
#else
        SetGroupSize(ind2 + prevGroupStart, j - prevGroupStart);
        prevGroupStart = j;
#endif
      }
    }

#ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetGroupSize(ind2 + prevGroupStart, j - prevGroupStart);
#endif
    }
    {
    /* Write new Groups values and Check that there are groups */
    unsigned thereAreGroups = 0;
    for (j = 0; j < groupSize; j++)
    {
      size_t group = groupOffset + j;
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
      UInt32 subGroupSize = ((ind2[j] & ~0xC0000000) >> kNumBitsMax);
      if (ind2[j] & 0x40000000)
        subGroupSize += ((ind2[(size_t)j + 1] >> kNumBitsMax) << kNumExtra0Bits);
      subGroupSize++;
      for (;;)
      {
        const UInt32 original = ind2[j];
        size_t sp = original & kIndexMask;
        if (sp < NumSortedBytes)
          sp += BlockSize;
        sp -= NumSortedBytes;
        ind2[j] = (UInt32)sp | (original & ~kIndexMask);
        Groups[sp] = (UInt32)group;
        if (--subGroupSize == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
#else
      UInt32 *Flags = Groups + BlockSize;
      for (;;)
      {
        size_t sp = ind2[j];
        if (sp < NumSortedBytes)
          sp += BlockSize;
        sp -= NumSortedBytes;
        ind2[j] = (UInt32)sp;
        Groups[sp] = (UInt32)group;
        if ((Flags[(groupOffset + j) >> kNumFlagsBits] & (1 << ((groupOffset + j) & kFlagsMask))) == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
#endif
    }
    return thereAreGroups;
    }
  }
#endif // BLOCK_SORT_USE_HEAP_SORT
}


/* conditions: blockSize > 0 */
UInt32 BlockSort(UInt32 *Indices, const Byte *data, size_t blockSize)
{
  UInt32 *counters = Indices + blockSize;
  size_t i;
  UInt32 *Groups;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
  UInt32 *Flags;
#endif

/* Radix-Sort for 2 bytes */
// { UInt32 yyy; for (yyy = 0; yyy < 100; yyy++) {
  for (i = 0; i < kNumHashValues; i++)
    counters[i] = 0;
  {
    const Byte *data2 = data;
    size_t a = data[(size_t)blockSize - 1];
    const Byte *data_lim = data + blockSize;
    if (blockSize >= 4)
    {
      data_lim -= 3;
      do
      {
        size_t b;
        b = data2[0]; counters[(a << 8) | b]++;
        a = data2[1]; counters[(b << 8) | a]++;
        b = data2[2]; counters[(a << 8) | b]++;
        a = data2[3]; counters[(b << 8) | a]++;
        data2 += 4;
      }
      while (data2 < data_lim);
      data_lim += 3;
    }
    while (data2 != data_lim)
    {
      size_t b = *data2++;
      counters[(a << 8) | b]++;
      a = b;
    }
  }
// }}

  Groups = counters + BS_TEMP_SIZE;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
  Flags = Groups + blockSize;
  {
    const size_t numWords = (blockSize + kFlagsMask) >> kNumFlagsBits;
    for (i = 0; i < numWords; i++)
      Flags[i] = kAllFlags;
  }
#endif

  {
    UInt32 sum = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      const UInt32 groupSize = counters[i];
      counters[i] = sum;
      sum += groupSize;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
      if (groupSize)
      {
        const UInt32 t = sum - 1;
        Flags[t >> kNumFlagsBits] &= ~((UInt32)1 << (t & kFlagsMask));
      }
#endif
    }
  }

  for (i = 0; i < blockSize - 1; i++)
    Groups[i] = counters[((unsigned)data[i] << 8) | data[(size_t)i + 1]];
  Groups[i] = counters[((unsigned)data[i] << 8) | data[0]];
  
  {
#define SET_Indices(a, b, i)  \
    { UInt32 c; \
      a = (a << 8) | (b); \
      c = counters[a]; \
      Indices[c] = (UInt32)i++; \
      counters[a] = c + 1; \
    }

    size_t a = data[0];
    const Byte *data_ptr = data + 1;
    i = 0;
    if (blockSize >= 3)
    {
      blockSize -= 2;
      do
      {
        size_t b;
        b = data_ptr[0];  SET_Indices(a, b, i)
        a = data_ptr[1];  SET_Indices(b, a, i)
        data_ptr += 2;
      }
      while (i < blockSize);
      blockSize += 2;
    }
    if (i < blockSize - 1)
    {
      SET_Indices(a, data[(size_t)i + 1], i)
      a = (Byte)a;
    }
    SET_Indices(a, data[0], i)
  }
  
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
  {
    UInt32 prev = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      const UInt32 prevGroupSize = counters[i] - prev;
      if (prevGroupSize == 0)
        continue;
      SetGroupSize(Indices + prev, prevGroupSize);
      prev = counters[i];
    }
  }
#endif

  {
  unsigned NumRefBits;
  size_t NumSortedBytes;
  for (NumRefBits = 0; ((blockSize - 1) >> NumRefBits) != 0; NumRefBits++)
  {}
  NumRefBits = 32 - NumRefBits;
  if (NumRefBits > kNumRefBitsMax)
      NumRefBits = kNumRefBitsMax;

  for (NumSortedBytes = kNumHashBytes; ; NumSortedBytes <<= 1)
  {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
    size_t finishedGroupSize = 0;
#endif
    size_t newLimit = 0;
    for (i = 0; i < blockSize;)
    {
      size_t groupSize;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS

      if ((Flags[i >> kNumFlagsBits] & (1 << (i & kFlagsMask))) == 0)
      {
        i++;
        continue;
      }
      for (groupSize = 1;
        (Flags[(i + groupSize) >> kNumFlagsBits] & (1 << ((i + groupSize) & kFlagsMask))) != 0;
        groupSize++)
        {}
      groupSize++;

#else

      groupSize = (Indices[i] & ~0xC0000000) >> kNumBitsMax;
      {
        const BoolInt finishedGroup = ((Indices[i] & 0x80000000) == 0);
        if (Indices[i] & 0x40000000)
        {
          groupSize += ((Indices[(size_t)i + 1] >> kNumBitsMax) << kNumExtra0Bits);
          Indices[(size_t)i + 1] &= kIndexMask;
        }
        Indices[i] &= kIndexMask;
        groupSize++;
        if (finishedGroup || groupSize == 1)
        {
          Indices[i - finishedGroupSize] &= kIndexMask;
          if (finishedGroupSize > 1)
            Indices[(size_t)(i - finishedGroupSize) + 1] &= kIndexMask;
          {
            const size_t newGroupSize = groupSize + finishedGroupSize;
            SetFinishedGroupSize(Indices + i - finishedGroupSize, newGroupSize)
            finishedGroupSize = newGroupSize;
          }
          i += groupSize;
          continue;
        }
        finishedGroupSize = 0;
      }

#endif
      
      if (NumSortedBytes >= blockSize)
      {
        size_t j;
        for (j = 0; j < groupSize; j++)
        {
          size_t t = i + j;
          /* Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask)); */
          Groups[Indices[t]] = (UInt32)t;
        }
      }
      else
        if (SortGroup(blockSize, NumSortedBytes, i, groupSize, NumRefBits, Indices
            #ifndef BLOCK_SORT_USE_HEAP_SORT
              , 0, blockSize
            #endif
            ))
          newLimit = i + groupSize;
      i += groupSize;
    }
    if (newLimit == 0)
      break;
  }
  }
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
  for (i = 0; i < blockSize;)
  {
    size_t groupSize = (Indices[i] & ~0xC0000000) >> kNumBitsMax;
    if (Indices[i] & 0x40000000)
    {
      groupSize += (Indices[(size_t)i + 1] >> kNumBitsMax) << kNumExtra0Bits;
      Indices[(size_t)i + 1] &= kIndexMask;
    }
    Indices[i] &= kIndexMask;
    groupSize++;
    i += groupSize;
  }
#endif
  return Groups[0];
}

/* ================ unit: C/CpuArch.c ================ */
/* CpuArch.c -- CPU specific code
Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

#ifdef MY_CPU_X86_OR_AMD64

#undef NEED_CHECK_FOR_CPUID
#if !defined(MY_CPU_AMD64)
#define NEED_CHECK_FOR_CPUID
#endif

/*
  cpuid instruction supports (subFunction) parameter in ECX,
  that is used only with some specific (function) parameter values.
  most functions use only (subFunction==0).
*/
/*
  __cpuid(): MSVC and GCC/CLANG use same function/macro name
             but parameters are different.
   We use MSVC __cpuid() parameters style for our z7_x86_cpuid() function.
*/

#if defined(__GNUC__) /* && (__GNUC__ >= 10) */ \
    || defined(__clang__) /* && (__clang_major__ >= 10) */

/* there was some CLANG/GCC compilers that have issues with
   rbx(ebx) handling in asm blocks in -fPIC mode (__PIC__ is defined).
   compiler's <cpuid.h> contains the macro __cpuid() that is similar to our code.
   The history of __cpuid() changes in CLANG/GCC:
   GCC:
     2007: it preserved ebx for (__PIC__ && __i386__)
     2013: it preserved rbx and ebx for __PIC__
     2014: it doesn't preserves rbx and ebx anymore
     we suppose that (__GNUC__ >= 5) fixed that __PIC__ ebx/rbx problem.
   CLANG:
     2014+: it preserves rbx, but only for 64-bit code. No __PIC__ check.
   Why CLANG cares about 64-bit mode only, and doesn't care about ebx (in 32-bit)?
   Do we need __PIC__ test for CLANG or we must care about rbx even if
   __PIC__ is not defined?
*/

#define ASM_LN "\n"
   
#if defined(MY_CPU_AMD64) && defined(__PIC__) \
    && ((defined (__GNUC__) && (__GNUC__ < 5)) || defined(__clang__))

  /* "=&r" selects free register. It can select even rbx, if that register is free.
     "=&D" for (RDI) also works, but the code can be larger with "=&D"
     "2"(subFun) : 2 is (zero-based) index in the output constraint list "=c" (ECX). */

#define x86_cpuid_MACRO_2(p, func, subFunc) { \
  __asm__ __volatile__ ( \
    ASM_LN   "mov     %%rbx, %q1"  \
    ASM_LN   "cpuid"               \
    ASM_LN   "xchg    %%rbx, %q1"  \
    : "=a" ((p)[0]), "=&r" ((p)[1]), "=c" ((p)[2]), "=d" ((p)[3]) : "0" (func), "2"(subFunc)); }

#elif defined(MY_CPU_X86) && defined(__PIC__) \
    && ((defined (__GNUC__) && (__GNUC__ < 5)) || defined(__clang__))

#define x86_cpuid_MACRO_2(p, func, subFunc) { \
  __asm__ __volatile__ ( \
    ASM_LN   "mov     %%ebx, %k1"  \
    ASM_LN   "cpuid"               \
    ASM_LN   "xchg    %%ebx, %k1"  \
    : "=a" ((p)[0]), "=&r" ((p)[1]), "=c" ((p)[2]), "=d" ((p)[3]) : "0" (func), "2"(subFunc)); }

#else

#define x86_cpuid_MACRO_2(p, func, subFunc) { \
  __asm__ __volatile__ ( \
    ASM_LN   "cpuid"               \
    : "=a" ((p)[0]), "=b" ((p)[1]), "=c" ((p)[2]), "=d" ((p)[3]) : "0" (func), "2"(subFunc)); }

#endif

#define x86_cpuid_MACRO(p, func)  x86_cpuid_MACRO_2(p, func, 0)

void Z7_FASTCALL z7_x86_cpuid(UInt32 p[4], UInt32 func)
{
  x86_cpuid_MACRO(p, func)
}

static
void Z7_FASTCALL z7_x86_cpuid_subFunc(UInt32 p[4], UInt32 func, UInt32 subFunc)
{
  x86_cpuid_MACRO_2(p, func, subFunc)
}


Z7_NO_INLINE
UInt32 Z7_FASTCALL z7_x86_cpuid_GetMaxFunc(void)
{
 #if defined(NEED_CHECK_FOR_CPUID)
  #define EFALGS_CPUID_BIT 21
  UInt32 a;
  __asm__ __volatile__ (
    ASM_LN   "pushf"
    ASM_LN   "pushf"
    ASM_LN   "pop     %0"
    // ASM_LN   "movl    %0, %1"
    // ASM_LN   "xorl    $0x200000, %0"
    ASM_LN   "btc     %1, %0"
    ASM_LN   "push    %0"
    ASM_LN   "popf"
    ASM_LN   "pushf"
    ASM_LN   "pop     %0"
    ASM_LN   "xorl    (%%esp), %0"

    ASM_LN   "popf"
    ASM_LN
    : "=&r" (a) // "=a"
    : "i" (EFALGS_CPUID_BIT)
    );
  if ((a & (1 << EFALGS_CPUID_BIT)) == 0)
    return 0;
 #endif
  {
    UInt32 p[4];
    x86_cpuid_MACRO(p, 0)
    return p[0];
  }
}

#undef ASM_LN

#elif !defined(_MSC_VER)

/*
// for gcc/clang and other: we can try to use __cpuid macro:
#include <cpuid.h>
void Z7_FASTCALL z7_x86_cpuid(UInt32 p[4], UInt32 func)
{
  __cpuid(func, p[0], p[1], p[2], p[3]);
}
UInt32 Z7_FASTCALL z7_x86_cpuid_GetMaxFunc(void)
{
  return (UInt32)__get_cpuid_max(0, NULL);
}
*/
// for unsupported cpuid:
void Z7_FASTCALL z7_x86_cpuid(UInt32 p[4], UInt32 func)
{
  UNUSED_VAR(func)
  p[0] = p[1] = p[2] = p[3] = 0;
}
UInt32 Z7_FASTCALL z7_x86_cpuid_GetMaxFunc(void)
{
  return 0;
}

#else // _MSC_VER

#if !defined(MY_CPU_AMD64)

UInt32 __declspec(naked) Z7_FASTCALL z7_x86_cpuid_GetMaxFunc(void)
{
  #if defined(NEED_CHECK_FOR_CPUID)
  #define EFALGS_CPUID_BIT 21
  __asm   pushfd
  __asm   pushfd
  /*
  __asm   pop     eax
  // __asm   mov     edx, eax
  __asm   btc     eax, EFALGS_CPUID_BIT
  __asm   push    eax
  */
  __asm   btc     dword ptr [esp], EFALGS_CPUID_BIT
  __asm   popfd
  __asm   pushfd
  __asm   pop     eax
  // __asm   xor     eax, edx
  __asm   xor     eax, [esp]
  // __asm   push    edx
  __asm   popfd
  __asm   and     eax, (1 shl EFALGS_CPUID_BIT)
  __asm   jz end_func
  #endif
  __asm   push    ebx
  __asm   xor     eax, eax    // func
  __asm   xor     ecx, ecx    // subFunction (optional) for (func == 0)
  __asm   cpuid
  __asm   pop     ebx
  #if defined(NEED_CHECK_FOR_CPUID)
  end_func:
  #endif
  __asm   ret 0
}

void __declspec(naked) Z7_FASTCALL z7_x86_cpuid(UInt32 p[4], UInt32 func)
{
  UNUSED_VAR(p)
  UNUSED_VAR(func)
  __asm   push    ebx
  __asm   push    edi
  __asm   mov     edi, ecx    // p
  __asm   mov     eax, edx    // func
  __asm   xor     ecx, ecx    // subfunction (optional) for (func == 0)
  __asm   cpuid
  __asm   mov     [edi     ], eax
  __asm   mov     [edi +  4], ebx
  __asm   mov     [edi +  8], ecx
  __asm   mov     [edi + 12], edx
  __asm   pop     edi
  __asm   pop     ebx
  __asm   ret     0
}

static
void __declspec(naked) Z7_FASTCALL z7_x86_cpuid_subFunc(UInt32 p[4], UInt32 func, UInt32 subFunc)
{
  UNUSED_VAR(p)
  UNUSED_VAR(func)
  UNUSED_VAR(subFunc)
  __asm   push    ebx
  __asm   push    edi
  __asm   mov     edi, ecx    // p
  __asm   mov     eax, edx    // func
  __asm   mov     ecx, [esp + 12]  // subFunc
  __asm   cpuid
  __asm   mov     [edi     ], eax
  __asm   mov     [edi +  4], ebx
  __asm   mov     [edi +  8], ecx
  __asm   mov     [edi + 12], edx
  __asm   pop     edi
  __asm   pop     ebx
  __asm   ret     4
}

#else // MY_CPU_AMD64

    #if _MSC_VER >= 1600
      #include <intrin.h>
      #define MY_cpuidex  __cpuidex

static
void Z7_FASTCALL z7_x86_cpuid_subFunc(UInt32 p[4], UInt32 func, UInt32 subFunc)
{
  __cpuidex((int *)p, func, subFunc);
}

    #else
/*
 __cpuid (func == (0 or 7)) requires subfunction number in ECX.
  MSDN: The __cpuid intrinsic clears the ECX register before calling the cpuid instruction.
   __cpuid() in new MSVC clears ECX.
   __cpuid() in old MSVC (14.00) x64 doesn't clear ECX
 We still can use __cpuid for low (func) values that don't require ECX,
 but __cpuid() in old MSVC will be incorrect for some func values: (func == 7).
 So here we use the hack for old MSVC to send (subFunction) in ECX register to cpuid instruction,
 where ECX value is first parameter for FASTCALL / NO_INLINE func.
 So the caller of MY_cpuidex_HACK() sets ECX as subFunction, and
 old MSVC for __cpuid() doesn't change ECX and cpuid instruction gets (subFunction) value.
 
DON'T remove Z7_NO_INLINE and Z7_FASTCALL for MY_cpuidex_HACK(): !!!
*/
static
Z7_NO_INLINE void Z7_FASTCALL MY_cpuidex_HACK(Int32 subFunction, Int32 func, Int32 *CPUInfo)
{
  UNUSED_VAR(subFunction)
  __cpuid(CPUInfo, func);
}
      #define MY_cpuidex(info, func, func2)  MY_cpuidex_HACK(func2, func, info)
      #pragma message("======== MY_cpuidex_HACK WAS USED ========")
static
void Z7_FASTCALL z7_x86_cpuid_subFunc(UInt32 p[4], UInt32 func, UInt32 subFunc)
{
  MY_cpuidex_HACK(subFunc, func, (Int32 *)p);
}
    #endif // _MSC_VER >= 1600

#if !defined(MY_CPU_AMD64)
/* inlining for __cpuid() in MSVC x86 (32-bit) produces big ineffective code,
   so we disable inlining here */
Z7_NO_INLINE
#endif
void Z7_FASTCALL z7_x86_cpuid(UInt32 p[4], UInt32 func)
{
  MY_cpuidex((Int32 *)p, (Int32)func, 0);
}

Z7_NO_INLINE
UInt32 Z7_FASTCALL z7_x86_cpuid_GetMaxFunc(void)
{
  Int32 a[4];
  MY_cpuidex(a, 0, 0);
  return a[0];
}

#endif // MY_CPU_AMD64
#endif // _MSC_VER

#if defined(NEED_CHECK_FOR_CPUID)
#define CHECK_CPUID_IS_SUPPORTED { if (z7_x86_cpuid_GetMaxFunc() == 0) return 0; }
#else
#define CHECK_CPUID_IS_SUPPORTED
#endif
#undef NEED_CHECK_FOR_CPUID


static
BoolInt x86cpuid_Func_1(UInt32 *p)
{
  CHECK_CPUID_IS_SUPPORTED
  z7_x86_cpuid(p, 1);
  return True;
}

/*
static const UInt32 kVendors[][1] =
{
  { 0x756E6547 }, // , 0x49656E69, 0x6C65746E },
  { 0x68747541 }, // , 0x69746E65, 0x444D4163 },
  { 0x746E6543 }  // , 0x48727561, 0x736C7561 }
};
*/

/*
typedef struct
{
  UInt32 maxFunc;
  UInt32 vendor[3];
  UInt32 ver;
  UInt32 b;
  UInt32 c;
  UInt32 d;
} Cx86cpuid;

enum
{
  CPU_FIRM_INTEL,
  CPU_FIRM_AMD,
  CPU_FIRM_VIA
};
int x86cpuid_GetFirm(const Cx86cpuid *p);
#define x86cpuid_ver_GetFamily(ver) (((ver >> 16) & 0xff0) | ((ver >> 8) & 0xf))
#define x86cpuid_ver_GetModel(ver)  (((ver >> 12) &  0xf0) | ((ver >> 4) & 0xf))
#define x86cpuid_ver_GetStepping(ver) (ver & 0xf)

int x86cpuid_GetFirm(const Cx86cpuid *p)
{
  unsigned i;
  for (i = 0; i < sizeof(kVendors) / sizeof(kVendors[0]); i++)
  {
    const UInt32 *v = kVendors[i];
    if (v[0] == p->vendor[0]
        // && v[1] == p->vendor[1]
        // && v[2] == p->vendor[2]
        )
      return (int)i;
  }
  return -1;
}

BoolInt CPU_Is_InOrder()
{
  Cx86cpuid p;
  UInt32 family, model;
  if (!x86cpuid_CheckAndRead(&p))
    return True;

  family = x86cpuid_ver_GetFamily(p.ver);
  model = x86cpuid_ver_GetModel(p.ver);

  switch (x86cpuid_GetFirm(&p))
  {
    case CPU_FIRM_INTEL: return (family < 6 || (family == 6 && (
        // In-Order Atom CPU
           model == 0x1C  // 45 nm, N4xx, D4xx, N5xx, D5xx, 230, 330
        || model == 0x26  // 45 nm, Z6xx
        || model == 0x27  // 32 nm, Z2460
        || model == 0x35  // 32 nm, Z2760
        || model == 0x36  // 32 nm, N2xxx, D2xxx
        )));
    case CPU_FIRM_AMD: return (family < 5 || (family == 5 && (model < 6 || model == 0xA)));
    case CPU_FIRM_VIA: return (family < 6 || (family == 6 && model < 0xF));
  }
  return False; // v23 : unknown processors are not In-Order
}
*/

#ifdef _WIN32
// amalgamation: header emitted in prologue
#endif

#if !defined(MY_CPU_AMD64) && defined(_WIN32)

/* for legacy SSE ia32: there is no user-space cpu instruction to check
   that OS supports SSE register storing/restoring on context switches.
   So we need some OS-specific function to check that it's safe to use SSE registers.
*/

Z7_FORCE_INLINE
static BoolInt CPU_Sys_Is_SSE_Supported(void)
{
#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4996) // `GetVersion': was declared deprecated
#endif
  /* low byte is major version of Windows
     We suppose that any Windows version since
     Windows2000 (major == 5) supports SSE registers */
  return (Byte)GetVersion() >= 5;
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif
}
#define CHECK_SYS_SSE_SUPPORT if (!CPU_Sys_Is_SSE_Supported()) return False;
#else
#define CHECK_SYS_SSE_SUPPORT
#endif


#if !defined(MY_CPU_AMD64)

BoolInt CPU_IsSupported_CMOV(void)
{
  UInt32 a[4];
  if (!x86cpuid_Func_1(&a[0]))
    return 0;
  return (BoolInt)(a[3] >> 15) & 1;
}

BoolInt CPU_IsSupported_SSE(void)
{
  UInt32 a[4];
  CHECK_SYS_SSE_SUPPORT
  if (!x86cpuid_Func_1(&a[0]))
    return 0;
  return (BoolInt)(a[3] >> 25) & 1;
}

BoolInt CPU_IsSupported_SSE2(void)
{
  UInt32 a[4];
  CHECK_SYS_SSE_SUPPORT
  if (!x86cpuid_Func_1(&a[0]))
    return 0;
  return (BoolInt)(a[3] >> 26) & 1;
}

#endif


static UInt32 x86cpuid_Func_1_ECX(void)
{
  UInt32 a[4];
  CHECK_SYS_SSE_SUPPORT
  if (!x86cpuid_Func_1(&a[0]))
    return 0;
  return a[2];
}

BoolInt CPU_IsSupported_AES(void)
{
  return (BoolInt)(x86cpuid_Func_1_ECX() >> 25) & 1;
}

BoolInt CPU_IsSupported_SSSE3(void)
{
  return (BoolInt)(x86cpuid_Func_1_ECX() >> 9) & 1;
}

BoolInt CPU_IsSupported_SSE41(void)
{
  return (BoolInt)(x86cpuid_Func_1_ECX() >> 19) & 1;
}

BoolInt CPU_IsSupported_SHA(void)
{
  CHECK_SYS_SSE_SUPPORT

  if (z7_x86_cpuid_GetMaxFunc() < 7)
    return False;
  {
    UInt32 d[4];
    z7_x86_cpuid(d, 7);
    return (BoolInt)(d[1] >> 29) & 1;
  }
}


BoolInt CPU_IsSupported_SHA512(void)
{
  if (!CPU_IsSupported_AVX2()) return False; // maybe CPU_IsSupported_AVX() is enough here

  if (z7_x86_cpuid_GetMaxFunc() < 7)
    return False;
  {
    UInt32 d[4];
    z7_x86_cpuid_subFunc(d, 7, 0);
    if (d[0] < 1) // d[0] - is max supported subleaf value
      return False;
    z7_x86_cpuid_subFunc(d, 7, 1);
    return (BoolInt)(d[0]) & 1;
  }
}

/*
MSVC: _xgetbv() intrinsic is available since VS2010SP1.
   MSVC also defines (_XCR_XFEATURE_ENABLED_MASK) macro in
   <immintrin.h> that we can use or check.
   For any 32-bit x86 we can use asm code in MSVC,
   but MSVC asm code is huge after compilation.
   So _xgetbv() is better

ICC: _xgetbv() intrinsic is available (in what version of ICC?)
   ICC defines (__GNUC___) and it supports gnu assembler
   also ICC supports MASM style code with -use-msasm switch.
   but ICC doesn't support __attribute__((__target__))

GCC/CLANG 9:
  _xgetbv() is macro that works via __builtin_ia32_xgetbv()
  and we need __attribute__((__target__("xsave")).
  But with __target__("xsave") the function will be not
  inlined to function that has no __target__("xsave") attribute.
  If we want _xgetbv() call inlining, then we should use asm version
  instead of calling _xgetbv().
  Note:intrinsic is broke before GCC 8.2:
    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85684
*/

#if    defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1100) \
    || defined(_MSC_VER) && (_MSC_VER >= 1600) && (_MSC_FULL_VER >= 160040219)  \
    || defined(__GNUC__) && (__GNUC__ >= 9) \
    || defined(__clang__) && (__clang_major__ >= 9)
// we define ATTRIB_XGETBV, if we want to use predefined _xgetbv() from compiler
#if defined(__INTEL_COMPILER)
#define ATTRIB_XGETBV
#elif defined(__GNUC__) || defined(__clang__)
// we don't define ATTRIB_XGETBV here, because asm version is better for inlining.
// #define ATTRIB_XGETBV __attribute__((__target__("xsave")))
#else
#define ATTRIB_XGETBV
#endif
#endif

#if defined(ATTRIB_XGETBV)
#include <immintrin.h>
#endif


// XFEATURE_ENABLED_MASK/XCR0
#define MY_XCR_XFEATURE_ENABLED_MASK 0

#if defined(ATTRIB_XGETBV)
ATTRIB_XGETBV
#endif
static UInt64 x86_xgetbv_0(UInt32 num)
{
#if defined(ATTRIB_XGETBV)
  {
    return
      #if (defined(_MSC_VER))
        _xgetbv(num);
      #else
        __builtin_ia32_xgetbv(
          #if !defined(__clang__)
            (int)
          #endif
            num);
      #endif
  }

#elif defined(__GNUC__) || defined(__clang__) || defined(__SUNPRO_CC)

  UInt32 a, d;
 #if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
  __asm__
  (
    "xgetbv"
    : "=a"(a), "=d"(d) : "c"(num) : "cc"
  );
 #else // is old gcc
  __asm__
  (
    ".byte 0x0f, 0x01, 0xd0" "\n\t"
    : "=a"(a), "=d"(d) : "c"(num) : "cc"
  );
 #endif
  return ((UInt64)d << 32) | a;
  // return a;

#elif defined(_MSC_VER) && !defined(MY_CPU_AMD64)
  
  UInt32 a, d;
  __asm {
    push eax
    push edx
    push ecx
    mov ecx, num;
    // xor ecx, ecx // = MY_XCR_XFEATURE_ENABLED_MASK
    _emit 0x0f
    _emit 0x01
    _emit 0xd0
    mov a, eax
    mov d, edx
    pop ecx
    pop edx
    pop eax
  }
  return ((UInt64)d << 32) | a;
  // return a;

#else // it's unknown compiler
  // #error "Need xgetbv function"
  UNUSED_VAR(num)
  // for MSVC-X64 we could call external function from external file.
  /* Actually we had checked OSXSAVE/AVX in cpuid before.
     So it's expected that OS supports at least AVX and below. */
  // if (num != MY_XCR_XFEATURE_ENABLED_MASK) return 0; // if not XCR0
  return
      // (1 << 0) |  // x87
        (1 << 1)   // SSE
      | (1 << 2);  // AVX
  
#endif
}

#ifdef _WIN32
/*
  Windows versions do not know about new ISA extensions that
  can be introduced. But we still can use new extensions,
  even if Windows doesn't report about supporting them,
  But we can use new extensions, only if Windows knows about new ISA extension
  that changes the number or size of registers: SSE, AVX/XSAVE, AVX512
  So it's enough to check
    MY_PF_AVX_INSTRUCTIONS_AVAILABLE
      instead of
    MY_PF_AVX2_INSTRUCTIONS_AVAILABLE
*/
#define MY_PF_XSAVE_ENABLED                            17
// #define MY_PF_SSSE3_INSTRUCTIONS_AVAILABLE             36
// #define MY_PF_SSE4_1_INSTRUCTIONS_AVAILABLE            37
// #define MY_PF_SSE4_2_INSTRUCTIONS_AVAILABLE            38
// #define MY_PF_AVX_INSTRUCTIONS_AVAILABLE               39
// #define MY_PF_AVX2_INSTRUCTIONS_AVAILABLE              40
// #define MY_PF_AVX512F_INSTRUCTIONS_AVAILABLE           41
#endif

BoolInt CPU_IsSupported_AVX(void)
{
  #ifdef _WIN32
  if (!IsProcessorFeaturePresent(MY_PF_XSAVE_ENABLED))
    return False;
  /* PF_AVX_INSTRUCTIONS_AVAILABLE probably is supported starting from
     some latest Win10 revisions. But we need AVX in older Windows also.
     So we don't use the following check: */
  /*
  if (!IsProcessorFeaturePresent(MY_PF_AVX_INSTRUCTIONS_AVAILABLE))
    return False;
  */
  #endif

  /*
    OS must use new special XSAVE/XRSTOR instructions to save
    AVX registers when it required for context switching.
    At OS statring:
      OS sets CR4.OSXSAVE flag to signal the processor that OS supports the XSAVE extensions.
      Also OS sets bitmask in XCR0 register that defines what
      registers will be processed by XSAVE instruction:
        XCR0.SSE[bit 0] - x87 registers and state
        XCR0.SSE[bit 1] - SSE registers and state
        XCR0.AVX[bit 2] - AVX registers and state
    CR4.OSXSAVE is reflected to CPUID.1:ECX.OSXSAVE[bit 27].
       So we can read that bit in user-space.
    XCR0 is available for reading in user-space by new XGETBV instruction.
  */
  {
    const UInt32 c = x86cpuid_Func_1_ECX();
    if (0 == (1
        & (c >> 28)   // AVX instructions are supported by hardware
        & (c >> 27))) // OSXSAVE bit: XSAVE and related instructions are enabled by OS.
      return False;
  }

  /* also we can check
     CPUID.1:ECX.XSAVE [bit 26] : that shows that
        XSAVE, XRESTOR, XSETBV, XGETBV instructions are supported by hardware.
     But that check is redundant, because if OSXSAVE bit is set, then XSAVE is also set */

  /* If OS have enabled XSAVE extension instructions (OSXSAVE == 1),
     in most cases we expect that OS also will support storing/restoring
     for AVX and SSE states at least.
     But to be ensure for that we call user-space instruction
     XGETBV(0) to get XCR0 value that contains bitmask that defines
     what exact states(registers) OS have enabled for storing/restoring.
  */

  {
    const UInt32 bm = (UInt32)x86_xgetbv_0(MY_XCR_XFEATURE_ENABLED_MASK);
    // printf("\n=== XGetBV=0x%x\n", bm);
    return 1
        & (BoolInt)(bm >> 1)  // SSE state is supported (set by OS) for storing/restoring
        & (BoolInt)(bm >> 2); // AVX state is supported (set by OS) for storing/restoring
  }
  // since Win7SP1: we can use GetEnabledXStateFeatures();
}


BoolInt CPU_IsSupported_AVX2(void)
{
  if (!CPU_IsSupported_AVX())
    return False;
  if (z7_x86_cpuid_GetMaxFunc() < 7)
    return False;
  {
    UInt32 d[4];
    z7_x86_cpuid(d, 7);
    // printf("\ncpuid(7): ebx=%8x ecx=%8x\n", d[1], d[2]);
    return 1
      & (BoolInt)(d[1] >> 5); // avx2
  }
}

#if 0
BoolInt CPU_IsSupported_AVX512F_AVX512VL(void)
{
  if (!CPU_IsSupported_AVX())
    return False;
  if (z7_x86_cpuid_GetMaxFunc() < 7)
    return False;
  {
    UInt32 d[4];
    BoolInt v;
    z7_x86_cpuid(d, 7);
    // printf("\ncpuid(7): ebx=%8x ecx=%8x\n", d[1], d[2]);
    v = 1
      & (BoolInt)(d[1] >> 16)  // avx512f
      & (BoolInt)(d[1] >> 31); // avx512vl
    if (!v)
      return False;
  }
  {
    const UInt32 bm = (UInt32)x86_xgetbv_0(MY_XCR_XFEATURE_ENABLED_MASK);
    // printf("\n=== XGetBV=0x%x\n", bm);
    return 1
        & (BoolInt)(bm >> 5)  // OPMASK
        & (BoolInt)(bm >> 6)  // ZMM upper 256-bit
        & (BoolInt)(bm >> 7); // ZMM16 ... ZMM31
  }
}
#endif

BoolInt CPU_IsSupported_VAES_AVX2(void)
{
  if (!CPU_IsSupported_AVX())
    return False;
  if (z7_x86_cpuid_GetMaxFunc() < 7)
    return False;
  {
    UInt32 d[4];
    z7_x86_cpuid(d, 7);
    // printf("\ncpuid(7): ebx=%8x ecx=%8x\n", d[1], d[2]);
    return 1
      & (BoolInt)(d[1] >> 5) // avx2
      // & (d[1] >> 31) // avx512vl
      & (BoolInt)(d[2] >> 9); // vaes // VEX-256/EVEX
  }
}

BoolInt CPU_IsSupported_PageGB(void)
{
  CHECK_CPUID_IS_SUPPORTED
  {
    UInt32 d[4];
    z7_x86_cpuid(d, 0x80000000);
    if (d[0] < 0x80000001)
      return False;
    z7_x86_cpuid(d, 0x80000001);
    return (BoolInt)(d[3] >> 26) & 1;
  }
}


#elif defined(MY_CPU_ARM_OR_ARM64)

#ifdef _WIN32

// amalgamation: header emitted in prologue

BoolInt CPU_IsSupported_CRC32(void)  { return IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) ? 1 : 0; }
BoolInt CPU_IsSupported_CRYPTO(void) { return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) ? 1 : 0; }
BoolInt CPU_IsSupported_NEON(void)   { return IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) ? 1 : 0; }

#else

#if defined(__APPLE__)

/*
#include <stdio.h>
#include <string.h>
static void Print_sysctlbyname(const char *name)
{
  size_t bufSize = 256;
  char buf[256];
  int res = sysctlbyname(name, &buf, &bufSize, NULL, 0);
  {
    int i;
    printf("\nres = %d : %s : '%s' : bufSize = %d, numeric", res, name, buf, (unsigned)bufSize);
    for (i = 0; i < 20; i++)
      printf(" %2x", (unsigned)(Byte)buf[i]);

  }
}
*/
/*
  Print_sysctlbyname("hw.pagesize");
  Print_sysctlbyname("machdep.cpu.brand_string");
*/

static BoolInt z7_sysctlbyname_Get_BoolInt(const char *name)
{
  UInt32 val = 0;
  if (z7_sysctlbyname_Get_UInt32(name, &val) == 0 && val == 1)
    return 1;
  return 0;
}

BoolInt CPU_IsSupported_CRC32(void)
{
  return z7_sysctlbyname_Get_BoolInt("hw.optional.armv8_crc32");
}

BoolInt CPU_IsSupported_NEON(void)
{
  return z7_sysctlbyname_Get_BoolInt("hw.optional.neon");
}

BoolInt CPU_IsSupported_SHA512(void)
{
  return z7_sysctlbyname_Get_BoolInt("hw.optional.armv8_2_sha512");
}

/*
BoolInt CPU_IsSupported_SHA3(void)
{
  return z7_sysctlbyname_Get_BoolInt("hw.optional.armv8_2_sha3");
}
*/

#ifdef MY_CPU_ARM64
#define APPLE_CRYPTO_SUPPORT_VAL 1
#else
#define APPLE_CRYPTO_SUPPORT_VAL 0
#endif

BoolInt CPU_IsSupported_SHA1(void) { return APPLE_CRYPTO_SUPPORT_VAL; }
BoolInt CPU_IsSupported_SHA2(void) { return APPLE_CRYPTO_SUPPORT_VAL; }
BoolInt CPU_IsSupported_AES (void) { return APPLE_CRYPTO_SUPPORT_VAL; }


#else // __APPLE__

#if defined(__GLIBC__) && (__GLIBC__ * 100 + __GLIBC_MINOR__ >= 216)
  #define Z7_GETAUXV_AVAILABLE
#elif !defined(__QNXNTO__)
// #pragma message("=== is not NEW GLIBC === ")
  #if defined __has_include
  #if __has_include (<sys/auxv.h>)
// #pragma message("=== sys/auxv.h is avail=== ")
    #define Z7_GETAUXV_AVAILABLE
  #endif
  #endif
#endif

#ifdef Z7_GETAUXV_AVAILABLE
// #pragma message("=== Z7_GETAUXV_AVAILABLE === ")
#include <sys/auxv.h>
#define USE_HWCAP
#endif

#ifdef USE_HWCAP

#if defined(__FreeBSD__) || defined(__OpenBSD__)
static unsigned long MY_getauxval(int aux)
{
  unsigned long val;
  if (elf_aux_info(aux, &val, sizeof(val)))
    return 0;
  return val;
}
#else
#define MY_getauxval  getauxval
  #if defined __has_include
  #if __has_include (<asm/hwcap.h>)
#include <asm/hwcap.h>
  #endif
  #endif
#endif

  #define MY_HWCAP_CHECK_FUNC_2(name1, name2) \
  BoolInt CPU_IsSupported_ ## name1(void) { return (MY_getauxval(AT_HWCAP)  & (HWCAP_  ## name2)); }

#ifdef MY_CPU_ARM64
  #define MY_HWCAP_CHECK_FUNC(name) \
  MY_HWCAP_CHECK_FUNC_2(name, name)
#if 1 || defined(__ARM_NEON)
  BoolInt CPU_IsSupported_NEON(void) { return True; }
#else
  MY_HWCAP_CHECK_FUNC_2(NEON, ASIMD)
#endif
// MY_HWCAP_CHECK_FUNC (ASIMD)
#elif defined(MY_CPU_ARM)
  #define MY_HWCAP_CHECK_FUNC(name) \
  BoolInt CPU_IsSupported_ ## name(void) { return (MY_getauxval(AT_HWCAP2) & (HWCAP2_ ## name)); }
  MY_HWCAP_CHECK_FUNC_2(NEON, NEON)
#endif

#else // USE_HWCAP

  #define MY_HWCAP_CHECK_FUNC(name) \
  BoolInt CPU_IsSupported_ ## name(void) { return 0; }
#if defined(__ARM_NEON)
  BoolInt CPU_IsSupported_NEON(void) { return True; }
#else
  MY_HWCAP_CHECK_FUNC(NEON)
#endif

#endif // USE_HWCAP

MY_HWCAP_CHECK_FUNC (CRC32)
MY_HWCAP_CHECK_FUNC (SHA1)
MY_HWCAP_CHECK_FUNC (SHA2)
MY_HWCAP_CHECK_FUNC (AES)
#ifdef MY_CPU_ARM64
// <hwcap.h> supports HWCAP_SHA512 and HWCAP_SHA3 since 2017.
// we define them here, if they are not defined
#ifndef HWCAP_SHA3
// #define HWCAP_SHA3    (1 << 17)
#endif
#ifndef HWCAP_SHA512
// #pragma message("=== HWCAP_SHA512 define === ")
#define HWCAP_SHA512  (1 << 21)
#endif
MY_HWCAP_CHECK_FUNC (SHA512)
// MY_HWCAP_CHECK_FUNC (SHA3)
#endif

#endif // __APPLE__
#endif // _WIN32

#endif // MY_CPU_ARM_OR_ARM64



#ifdef __APPLE__

#include <sys/sysctl.h>

int z7_sysctlbyname_Get(const char *name, void *buf, size_t *bufSize)
{
  return sysctlbyname(name, buf, bufSize, NULL, 0);
}

int z7_sysctlbyname_Get_UInt32(const char *name, UInt32 *val)
{
  size_t bufSize = sizeof(*val);
  const int res = z7_sysctlbyname_Get(name, val, &bufSize);
  if (res == 0 && bufSize != sizeof(*val))
    return EFAULT;
  return res;
}

#endif

/* ================ unit: C/Delta.c ================ */
/* Delta.c -- Delta converter
2021-02-09 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

void Delta_Init(Byte *state)
{
  unsigned i;
  for (i = 0; i < DELTA_STATE_SIZE; i++)
    state[i] = 0;
}


void Delta_Encode(Byte *state, unsigned delta, Byte *data, SizeT size)
{
  Byte temp[DELTA_STATE_SIZE];

  if (size == 0)
    return;

  {
    unsigned i = 0;
    do
      temp[i] = state[i];
    while (++i != delta);
  }

  if (size <= delta)
  {
    unsigned i = 0, k;
    do
    {
      Byte b = *data;
      *data++ = (Byte)(b - temp[i]);
      temp[i] = b;
    }
    while (++i != size);
    
    k = 0;
    
    do
    {
      if (i == delta)
        i = 0;
      state[k] = temp[i++];
    }
    while (++k != delta);
    
    return;
  }
    
  {
    Byte *p = data + size - delta;
    {
      unsigned i = 0;
      do
        state[i] = *p++;
      while (++i != delta);
    }
    {
      const Byte *lim = data + delta;
      ptrdiff_t dif = -(ptrdiff_t)delta;
      
      if (((ptrdiff_t)size + dif) & 1)
      {
        --p;  *p = (Byte)(*p - p[dif]);
      }

      while (p != lim)
      {
        --p;  *p = (Byte)(*p - p[dif]);
        --p;  *p = (Byte)(*p - p[dif]);
      }
      
      dif = -dif;
      
      do
      {
        --p;  *p = (Byte)(*p - temp[--dif]);
      }
      while (dif != 0);
    }
  }
}


void Delta_Decode(Byte *state, unsigned delta, Byte *data, SizeT size)
{
  unsigned i;
  const Byte *lim;

  if (size == 0)
    return;
  
  i = 0;
  lim = data + size;
  
  if (size <= delta)
  {
    do
      *data = (Byte)(*data + state[i++]);
    while (++data != lim);

    for (; delta != i; state++, delta--)
      *state = state[i];
    data -= i;
  }
  else
  {
    /*
    #define B(n) b ## n
    #define I(n) Byte B(n) = state[n];
    #define U(n) { B(n) = (Byte)((B(n)) + *data++); data[-1] = (B(n)); }
    #define F(n) if (data != lim) { U(n) }

    if (delta == 1)
    {
      I(0)
      if ((lim - data) & 1) { U(0) }
      while (data != lim) { U(0) U(0) }
      data -= 1;
    }
    else if (delta == 2)
    {
      I(0) I(1)
      lim -= 1; while (data < lim) { U(0) U(1) }
      lim += 1; F(0)
      data -= 2;
    }
    else if (delta == 3)
    {
      I(0) I(1) I(2)
      lim -= 2; while (data < lim) { U(0) U(1) U(2) }
      lim += 2; F(0) F(1)
      data -= 3;
    }
    else if (delta == 4)
    {
      I(0) I(1) I(2) I(3)
      lim -= 3; while (data < lim) { U(0) U(1) U(2) U(3) }
      lim += 3; F(0) F(1) F(2)
      data -= 4;
    }
    else
    */
    {
      do
      {
        *data = (Byte)(*data + state[i++]);
        data++;
      }
      while (i != delta);
  
      {
        ptrdiff_t dif = -(ptrdiff_t)delta;
        do
          *data = (Byte)(*data + data[dif]);
        while (++data != lim);
        data += dif;
      }
    }
  }

  do
    *state++ = *data;
  while (++data != lim);
}
