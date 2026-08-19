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

/* ---- C/HuffEnc.h ---- */
/* HuffEnc.h -- Huffman encoding
Igor Pavlov : Public domain */

#ifndef ZIP7_INC_HUFF_ENC_H
#define ZIP7_INC_HUFF_ENC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define Z7_HUFFMAN_LEN_MAX 16
#define Z7_HUFFMAN_FREQS_SUM_MAX ((1 << 22) - 1)
/*
Conditions:
  2 <= num <= 1024 = 2 ^ NUM_BITS
  Sum(freqs) <= Z7_HUFFMAN_FREQS_SUM_MAX = 4M - 1 = 2 ^ (32 - NUM_BITS) - 1
  1 <= maxLen <= 16 = Z7_HUFFMAN_LEN_MAX
*/
void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, unsigned num, unsigned maxLen);

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

/* ---- C/LzFind.h ---- */
/* LzFind.h -- Match finder for LZ algorithms
2024-01-22 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZ_FIND_H
#define ZIP7_INC_LZ_FIND_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

typedef UInt32 CLzRef;

typedef struct
{
  const Byte *buffer;
  UInt32 pos;
  UInt32 posLimit;
  UInt32 streamPos;  /* wrap over Zero is allowed (streamPos < pos). Use (UInt32)(streamPos - pos) */
  UInt32 lenLimit;

  UInt32 cyclicBufferPos;
  UInt32 cyclicBufferSize; /* it must be = (historySize + 1) */

  Byte streamEndWasReached;
  Byte btMode;
  Byte bigHash;
  Byte directInput;

  UInt32 matchMaxLen;
  CLzRef *hash;
  CLzRef *son;
  UInt32 hashMask;
  UInt32 cutValue;

  Byte *bufBase;
  ISeqInStreamPtr stream;
  
  UInt32 blockSize;
  UInt32 keepSizeBefore;
  UInt32 keepSizeAfter;

  UInt32 numHashBytes;
  size_t directInputRem;
  UInt32 historySize;
  UInt32 fixedHashSize;
  Byte numHashBytes_Min;
  Byte numHashOutBits;
  Byte _pad2_[2];
  SRes result;
  UInt32 crc[256];
  size_t numRefs;

  UInt64 expectedDataSize;
} CMatchFinder;

#define Inline_MatchFinder_GetPointerToCurrentPos(p) ((const Byte *)(p)->buffer)

#define Inline_MatchFinder_GetNumAvailableBytes(p) ((UInt32)((p)->streamPos - (p)->pos))

/*
#define Inline_MatchFinder_IsFinishedOK(p) \
    ((p)->streamEndWasReached \
        && (p)->streamPos == (p)->pos \
        && (!(p)->directInput || (p)->directInputRem == 0))
*/
      
int MatchFinder_NeedMove(CMatchFinder *p);
/* Byte *MatchFinder_GetPointerToCurrentPos(CMatchFinder *p); */
void MatchFinder_MoveBlock(CMatchFinder *p);
void MatchFinder_ReadIfRequired(CMatchFinder *p);

void MatchFinder_Construct(CMatchFinder *p);

/* (directInput = 0) is default value.
   It's required to provide correct (directInput) value
   before calling MatchFinder_Create().
   You can set (directInput) by any of the following calls:
     - MatchFinder_SET_DIRECT_INPUT_BUF()
     - MatchFinder_SET_STREAM()
     - MatchFinder_SET_STREAM_MODE()
*/

#define MatchFinder_SET_DIRECT_INPUT_BUF(p, _src_, _srcLen_) { \
  (p)->stream = NULL; \
  (p)->directInput = 1; \
  (p)->buffer = (_src_); \
  (p)->directInputRem = (_srcLen_); }

/*
#define MatchFinder_SET_STREAM_MODE(p) { \
  (p)->directInput = 0; }
*/

#define MatchFinder_SET_STREAM(p, _stream_) { \
  (p)->stream = _stream_; \
  (p)->directInput = 0; }
  

int MatchFinder_Create(CMatchFinder *p, UInt32 historySize,
    UInt32 keepAddBufferBefore, UInt32 matchMaxLen, UInt32 keepAddBufferAfter,
    ISzAllocPtr alloc);
void MatchFinder_Free(CMatchFinder *p, ISzAllocPtr alloc);
void MatchFinder_Normalize3(UInt32 subValue, CLzRef *items, size_t numItems);

/*
#define MatchFinder_INIT_POS(p, val) \
    (p)->pos = (val); \
    (p)->streamPos = (val);
*/

// void MatchFinder_ReduceOffsets(CMatchFinder *p, UInt32 subValue);
#define MatchFinder_REDUCE_OFFSETS(p, subValue) \
    (p)->pos -= (subValue); \
    (p)->streamPos -= (subValue);


UInt32 * GetMatchesSpec1(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *buffer, CLzRef *son,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 _cutValue,
    UInt32 *distances, UInt32 maxLen);

/*
Conditions:
  Mf_GetNumAvailableBytes_Func must be called before each Mf_GetMatchLen_Func.
  Mf_GetPointerToCurrentPos_Func's result must be used only before any other function
*/

typedef void (*Mf_Init_Func)(void *object);
typedef UInt32 (*Mf_GetNumAvailableBytes_Func)(void *object);
typedef const Byte * (*Mf_GetPointerToCurrentPos_Func)(void *object);
typedef UInt32 * (*Mf_GetMatches_Func)(void *object, UInt32 *distances);
typedef void (*Mf_Skip_Func)(void *object, UInt32);

typedef struct
{
  Mf_Init_Func Init;
  Mf_GetNumAvailableBytes_Func GetNumAvailableBytes;
  Mf_GetPointerToCurrentPos_Func GetPointerToCurrentPos;
  Mf_GetMatches_Func GetMatches;
  Mf_Skip_Func Skip;
} IMatchFinder2;

void MatchFinder_CreateVTable(CMatchFinder *p, IMatchFinder2 *vTable);

void MatchFinder_Init_LowHash(CMatchFinder *p);
void MatchFinder_Init_HighHash(CMatchFinder *p);
void MatchFinder_Init_4(CMatchFinder *p);
// void MatchFinder_Init(CMatchFinder *p);
void MatchFinder_Init(void *p);

UInt32* Bt3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances);
UInt32* Hc3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances);

void Bt3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num);
void Hc3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num);

void LzFindPrepare(void);

EXTERN_C_END

#endif

/* ---- C/LzHash.h ---- */
/* LzHash.h -- HASH constants for LZ algorithms
2023-03-05 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZ_HASH_H
#define ZIP7_INC_LZ_HASH_H

/*
  (kHash2Size >= (1 <<  8)) : Required
  (kHash3Size >= (1 << 16)) : Required
*/

#define kHash2Size (1 << 10)
#define kHash3Size (1 << 16)
// #define kHash4Size (1 << 20)

#define kFix3HashSize (kHash2Size)
#define kFix4HashSize (kHash2Size + kHash3Size)
// #define kFix5HashSize (kHash2Size + kHash3Size + kHash4Size)

/*
  We use up to 3 crc values for hash:
    crc0
    crc1 << Shift_1
    crc2 << Shift_2
  (Shift_1 = 5) and (Shift_2 = 10) is good tradeoff.
  Small values for Shift are not good for collision rate.
  Big value for Shift_2 increases the minimum size
  of hash table, that will be slow for small files.
*/

#define kLzHash_CrcShift_1 5
#define kLzHash_CrcShift_2 10

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

/* ---- C/LzFindMt.h ---- */
/* LzFindMt.h -- multithreaded Match finder for LZ algorithms
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZ_FIND_MT_H
#define ZIP7_INC_LZ_FIND_MT_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

typedef struct
{
  UInt32 numProcessedBlocks;
  Int32 affinityGroup;
  UInt64 affinityInGroup;
  UInt64 affinity;
  CThread thread;

  BoolInt wasCreated;
  BoolInt needStart;
  BoolInt csWasInitialized;
  BoolInt csWasEntered;

  BoolInt exit;
  BoolInt stopWriting;

  CAutoResetEvent canStart;
  CAutoResetEvent wasStopped;
  CSemaphore freeSemaphore;
  CSemaphore filledSemaphore;
  CCriticalSection cs;
  // UInt32 numBlocks_Sent;
} CMtSync;


struct CMatchFinderMt_;

typedef UInt32 * (*Mf_Mix_Matches)(struct CMatchFinderMt_ *p, UInt32 matchMinPos, UInt32 *distances);

/* kMtCacheLineDummy must be >= size_of_CPU_cache_line */
#define kMtCacheLineDummy 128

typedef void (*Mf_GetHeads)(const Byte *buffer, UInt32 pos,
  UInt32 *hash, UInt32 hashMask, UInt32 *heads, UInt32 numHeads, const UInt32 *crc);

typedef struct CMatchFinderMt_
{
  /* LZ */
  const Byte *pointerToCurPos;
  UInt32 *btBuf;
  const UInt32 *btBufPos;
  const UInt32 *btBufPosLimit;
  UInt32 lzPos;
  UInt32 btNumAvailBytes;

  UInt32 *hash;
  UInt32 fixedHashSize;
  // UInt32 hash4Mask;
  UInt32 historySize;
  const UInt32 *crc;

  Mf_Mix_Matches MixMatchesFunc;
  UInt32 failure_LZ_BT; // failure in BT transfered to LZ
  // UInt32 failure_LZ_LZ; // failure in LZ tables
  UInt32 failureBuf[1];
  // UInt32 crc[256];

  /* LZ + BT */
  CMtSync btSync;
  Byte btDummy[kMtCacheLineDummy];

  /* BT */
  UInt32 *hashBuf;
  UInt32 hashBufPos;
  UInt32 hashBufPosLimit;
  UInt32 hashNumAvail;
  UInt32 failure_BT;


  CLzRef *son;
  UInt32 matchMaxLen;
  UInt32 numHashBytes;
  UInt32 pos;
  const Byte *buffer;
  UInt32 cyclicBufferPos;
  UInt32 cyclicBufferSize; /* it must be = (historySize + 1) */
  UInt32 cutValue;

  /* BT + Hash */
  CMtSync hashSync;
  /* Byte hashDummy[kMtCacheLineDummy]; */
  
  /* Hash */
  Mf_GetHeads GetHeadsFunc;
  CMatchFinder *MatchFinder;
  // CMatchFinder MatchFinder;
} CMatchFinderMt;

// only for Mt part
void MatchFinderMt_Construct(CMatchFinderMt *p);
void MatchFinderMt_Destruct(CMatchFinderMt *p, ISzAllocPtr alloc);

SRes MatchFinderMt_Create(CMatchFinderMt *p, UInt32 historySize, UInt32 keepAddBufferBefore,
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter, ISzAllocPtr alloc);
void MatchFinderMt_CreateVTable(CMatchFinderMt *p, IMatchFinder2 *vTable);

/* call MatchFinderMt_InitMt() before IMatchFinder::Init() */
SRes MatchFinderMt_InitMt(CMatchFinderMt *p);
void MatchFinderMt_ReleaseStream(CMatchFinderMt *p);

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

/* ---- C/Lzma2DecMt.h ---- */
/* Lzma2DecMt.h -- LZMA2 Decoder Multi-thread
2023-04-13 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZMA2_DEC_MT_H
#define ZIP7_INC_LZMA2_DEC_MT_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

typedef struct
{
  size_t inBufSize_ST;
  size_t outStep_ST;
  
  #ifndef Z7_ST
  unsigned numThreads;
  size_t inBufSize_MT;
  size_t outBlockMax;
  size_t inBlockMax;
  #endif
} CLzma2DecMtProps;

/* init to single-thread mode */
void Lzma2DecMtProps_Init(CLzma2DecMtProps *p);


/* ---------- CLzma2DecMtHandle Interface ---------- */

/* Lzma2DecMt_ * functions can return the following exit codes:
SRes:
  SZ_OK           - OK
  SZ_ERROR_MEM    - Memory allocation error
  SZ_ERROR_PARAM  - Incorrect paramater in props
  SZ_ERROR_WRITE  - ISeqOutStream write callback error
  // SZ_ERROR_OUTPUT_EOF - output buffer overflow - version with (Byte *) output
  SZ_ERROR_PROGRESS - some break from progress callback
  SZ_ERROR_THREAD - error in multithreading functions (only for Mt version)
*/

typedef struct CLzma2DecMt CLzma2DecMt;
typedef CLzma2DecMt * CLzma2DecMtHandle;
// Z7_DECLARE_HANDLE(CLzma2DecMtHandle)

CLzma2DecMtHandle Lzma2DecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid);
void Lzma2DecMt_Destroy(CLzma2DecMtHandle p);

SRes Lzma2DecMt_Decode(CLzma2DecMtHandle p,
    Byte prop,
    const CLzma2DecMtProps *props,
    ISeqOutStreamPtr outStream,
    const UInt64 *outDataSize, // NULL means undefined
    int finishMode,            // 0 - partial unpacking is allowed, 1 - if lzma2 stream must be finished
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    // const Byte *inData, size_t inDataSize,
    
    // out variables:
    UInt64 *inProcessed,
    int *isMT,  /* out: (*isMT == 0), if single thread decoding was used */

    // UInt64 *outProcessed,
    ICompressProgressPtr progress);


/* ---------- Read from CLzma2DecMtHandle Interface ---------- */

SRes Lzma2DecMt_Init(CLzma2DecMtHandle pp,
    Byte prop,
    const CLzma2DecMtProps *props,
    const UInt64 *outDataSize, int finishMode,
    ISeqInStreamPtr inStream);

SRes Lzma2DecMt_Read(CLzma2DecMtHandle pp,
    Byte *data, size_t *outSize,
    UInt64 *inStreamProcessed);


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

/* ---- C/Md5.h ---- */
/* Md5.h -- MD5 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_MD5_H
#define ZIP7_INC_MD5_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define MD5_NUM_BLOCK_WORDS  16
#define MD5_NUM_DIGEST_WORDS  4

#define MD5_BLOCK_SIZE   (MD5_NUM_BLOCK_WORDS * 4)
#define MD5_DIGEST_SIZE  (MD5_NUM_DIGEST_WORDS * 4)

typedef struct
{
  UInt64 count;
  UInt64 _pad_1;
  // we want 16-bytes alignment here
  UInt32 state[MD5_NUM_DIGEST_WORDS];
  UInt64 _pad_2[4];
  // we want 64-bytes alignment here
  Byte buffer[MD5_BLOCK_SIZE];
} CMd5;

void Md5_Init(CMd5 *p);
void Md5_Update(CMd5 *p, const Byte *data, size_t size);
void Md5_Final(CMd5 *p, Byte *digest);

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

/* ---- C/Ppmd7.h ---- */
/* Ppmd7.h -- Ppmd7 (PPMdH) compression codec
2023-04-02 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.H (2001): Dmitry Shkarin : Public domain */
 

#ifndef ZIP7_INC_PPMD7_H
#define ZIP7_INC_PPMD7_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define PPMD7_MIN_ORDER 2
#define PPMD7_MAX_ORDER 64

#define PPMD7_MIN_MEM_SIZE (1 << 11)
#define PPMD7_MAX_MEM_SIZE (0xFFFFFFFF - 12 * 3)

struct CPpmd7_Context_;

typedef Ppmd_Ref_Type(struct CPpmd7_Context_) CPpmd7_Context_Ref;

// MY_CPU_pragma_pack_push_1

typedef struct CPpmd7_Context_
{
  UInt16 NumStats;


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

  CPpmd7_Context_Ref Suffix;
} CPpmd7_Context;

// MY_CPU_pragma_pop

#define Ppmd7Context_OneState(p) ((CPpmd_State *)&(p)->Union2)




typedef struct
{
  UInt32 Range;
  UInt32 Code;
  UInt32 Low;
  IByteInPtr Stream;
} CPpmd7_RangeDec;


typedef struct
{
  UInt32 Range;
  Byte Cache;
  // Byte _dummy_[3];
  UInt64 Low;
  UInt64 CacheSize;
  IByteOutPtr Stream;
} CPpmd7z_RangeEnc;


typedef struct
{
  CPpmd7_Context *MinContext, *MaxContext;
  CPpmd_State *FoundState;
  unsigned OrderFall, InitEsc, PrevSuccess, MaxOrder, HiBitsFlag;
  Int32 RunLength, InitRL; /* must be 32-bit at least */

  UInt32 Size;
  UInt32 GlueCount;
  UInt32 AlignOffset;
  Byte *Base, *LoUnit, *HiUnit, *Text, *UnitsStart;


  
  
  union
  {
    CPpmd7_RangeDec dec;
    CPpmd7z_RangeEnc enc;
  } rc;
  
  Byte Indx2Units[PPMD_NUM_INDEXES + 2]; // +2 for alignment
  Byte Units2Indx[128];
  CPpmd_Void_Ref FreeList[PPMD_NUM_INDEXES];

  Byte NS2BSIndx[256], NS2Indx[256];
  Byte ExpEscape[16];
  CPpmd_See DummySee, See[25][16];
  UInt16 BinSumm[128][64];
  // int LastSymbol;
} CPpmd7;


void Ppmd7_Construct(CPpmd7 *p);
BoolInt Ppmd7_Alloc(CPpmd7 *p, UInt32 size, ISzAllocPtr alloc);
void Ppmd7_Free(CPpmd7 *p, ISzAllocPtr alloc);
void Ppmd7_Init(CPpmd7 *p, unsigned maxOrder);
#define Ppmd7_WasAllocated(p) ((p)->Base != NULL)


/* ---------- Internal Functions ---------- */

#define Ppmd7_GetPtr(p, ptr)     Ppmd_GetPtr(p, ptr)
#define Ppmd7_GetContext(p, ptr) Ppmd_GetPtr_Type(p, ptr, CPpmd7_Context)
#define Ppmd7_GetStats(p, ctx)   Ppmd_GetPtr_Type(p, (ctx)->Union4.Stats, CPpmd_State)

void Ppmd7_Update1(CPpmd7 *p);
void Ppmd7_Update1_0(CPpmd7 *p);
void Ppmd7_Update2(CPpmd7 *p);

#define PPMD7_HiBitsFlag_3(sym) ((((unsigned)sym + 0xC0) >> (8 - 3)) & (1 << 3))
#define PPMD7_HiBitsFlag_4(sym) ((((unsigned)sym + 0xC0) >> (8 - 4)) & (1 << 4))
// #define PPMD7_HiBitsFlag_3(sym) ((sym) < 0x40 ? 0 : (1 << 3))
// #define PPMD7_HiBitsFlag_4(sym) ((sym) < 0x40 ? 0 : (1 << 4))

#define Ppmd7_GetBinSumm(p) \
    &p->BinSumm[(size_t)(unsigned)Ppmd7Context_OneState(p->MinContext)->Freq - 1] \
    [ p->PrevSuccess + ((p->RunLength >> 26) & 0x20) \
    + p->NS2BSIndx[(size_t)Ppmd7_GetContext(p, p->MinContext->Suffix)->NumStats - 1] \
    + PPMD7_HiBitsFlag_4(Ppmd7Context_OneState(p->MinContext)->Symbol) \
    + (p->HiBitsFlag = PPMD7_HiBitsFlag_3(p->FoundState->Symbol)) ]

CPpmd_See *Ppmd7_MakeEscFreq(CPpmd7 *p, unsigned numMasked, UInt32 *scale);


/*
We support two versions of Ppmd7 (PPMdH) methods that use same CPpmd7 structure:
  1) Ppmd7a_*: original PPMdH
  2) Ppmd7z_*: modified PPMdH with 7z Range Coder
Ppmd7_*: the structures and functions that are common for both versions of PPMd7 (PPMdH)
*/

/* ---------- Decode ---------- */

#define PPMD7_SYM_END    (-1)
#define PPMD7_SYM_ERROR  (-2)

/*
You must set (CPpmd7::rc.dec.Stream) before Ppmd7*_RangeDec_Init()

Ppmd7*_DecodeSymbol()
out:
  >= 0 : decoded byte
    -1 : PPMD7_SYM_END   : End of payload marker
    -2 : PPMD7_SYM_ERROR : Data error
*/

/* Ppmd7a_* : original PPMdH */
BoolInt Ppmd7a_RangeDec_Init(CPpmd7_RangeDec *p);
#define Ppmd7a_RangeDec_IsFinishedOK(p) ((p)->Code == 0)
int Ppmd7a_DecodeSymbol(CPpmd7 *p);

/* Ppmd7z_* : modified PPMdH with 7z Range Coder */
BoolInt Ppmd7z_RangeDec_Init(CPpmd7_RangeDec *p);
#define Ppmd7z_RangeDec_IsFinishedOK(p) ((p)->Code == 0)
int Ppmd7z_DecodeSymbol(CPpmd7 *p);
// Byte *Ppmd7z_DecodeSymbols(CPpmd7 *p, Byte *buf, const Byte *lim);


/* ---------- Encode ---------- */

void Ppmd7z_Init_RangeEnc(CPpmd7 *p);
void Ppmd7z_Flush_RangeEnc(CPpmd7 *p);
// void Ppmd7z_EncodeSymbol(CPpmd7 *p, int symbol);
void Ppmd7z_EncodeSymbols(CPpmd7 *p, const Byte *buf, const Byte *lim);

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

/* ---- C/Sha1.h ---- */
/* Sha1.h -- SHA-1 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SHA1_H
#define ZIP7_INC_SHA1_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define SHA1_NUM_BLOCK_WORDS  16
#define SHA1_NUM_DIGEST_WORDS  5

#define SHA1_BLOCK_SIZE   (SHA1_NUM_BLOCK_WORDS * 4)
#define SHA1_DIGEST_SIZE  (SHA1_NUM_DIGEST_WORDS * 4)




typedef void (Z7_FASTCALL *SHA1_FUNC_UPDATE_BLOCKS)(UInt32 state[5], const Byte *data, size_t numBlocks);

/*
  if (the system supports different SHA1 code implementations)
  {
    (CSha1::func_UpdateBlocks) will be used
    (CSha1::func_UpdateBlocks) can be set by
       Sha1_Init()        - to default (fastest)
       Sha1_SetFunction() - to any algo
  }
  else
  {
    (CSha1::func_UpdateBlocks) is ignored.
  }
*/

typedef struct
{
  union
  {
    struct
    {
      SHA1_FUNC_UPDATE_BLOCKS func_UpdateBlocks;
      UInt64 count;
    } vars;
    UInt64 _pad_64bit[4];
    void *_pad_align_ptr[2];
  } v;
  UInt32 state[SHA1_NUM_DIGEST_WORDS];
  UInt32 _pad_3[3];
  Byte buffer[SHA1_BLOCK_SIZE];
} CSha1;


#define SHA1_ALGO_DEFAULT 0
#define SHA1_ALGO_SW      1
#define SHA1_ALGO_HW      2

/*
Sha1_SetFunction()
return:
  0 - (algo) value is not supported, and func_UpdateBlocks was not changed
  1 - func_UpdateBlocks was set according (algo) value.
*/

BoolInt Sha1_SetFunction(CSha1 *p, unsigned algo);

void Sha1_InitState(CSha1 *p);
void Sha1_Init(CSha1 *p);
void Sha1_Update(CSha1 *p, const Byte *data, size_t size);
void Sha1_Final(CSha1 *p, Byte *digest);

void Sha1_PrepareBlock(const CSha1 *p, Byte *block, unsigned size);
void Sha1_GetBlockDigest(const CSha1 *p, const Byte *data, Byte *destDigest);

// void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks);

/*
call Sha1Prepare() once at program start.
It prepares all supported implementations, and detects the fastest implementation.
*/

void Sha1Prepare(void);

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

/* ================ unit bodies ================ */

/* ================ unit: C/HuffEnc.c ================ */
/* HuffEnc.c -- functions for Huffman encoding
Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define kMaxLen       Z7_HUFFMAN_LEN_MAX
#define NUM_BITS      10
#define MASK          ((1u << NUM_BITS) - 1)
#define FREQ_MASK     (~(UInt32)MASK)
#define NUM_COUNTERS  (104 * 2) // (80 * 2) or (128 * 2) : ((prime_number + 1) * 2) for smaller code.

#if 1 && (defined(MY_CPU_LE) || defined(MY_CPU_BE))
#if defined(MY_CPU_LE)
  #define HI_HALF_OFFSET 1
#else
  #define HI_HALF_OFFSET 0
#endif
#define LOAD_PARENT(p)                  ((unsigned)*((const UInt16 *)(p) + HI_HALF_OFFSET))
#define STORE_PARENT(p, fb, val)         *((UInt16 *)(p) + HI_HALF_OFFSET) = (UInt16)(val);
#define STORE_PARENT_DIRECT(p, fb, hi)  STORE_PARENT(p, fb, hi)
#define UPDATE_E(eHi)                   eHi++;
#else
#define LOAD_PARENT(p)                  ((unsigned)(*(p) >> NUM_BITS))
#define STORE_PARENT_DIRECT(p, fb, hi)  *(p) = ((fb) & MASK) | (hi); // set parent field
#define STORE_PARENT(p, fb, val)        STORE_PARENT_DIRECT(p, fb, ((UInt32)(val) << NUM_BITS))
#define UPDATE_E(eHi)                   eHi += 1 << NUM_BITS;
#endif

void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, unsigned numSymbols, unsigned maxLen)
{
#if NUM_COUNTERS > 2
  unsigned counters[NUM_COUNTERS];
#endif
#if 1 && NUM_COUNTERS > (kMaxLen + 4) * 2
  #define lenCounters   (counters)
  #define codes         (counters + kMaxLen + 4)
#else
  unsigned lenCounters[kMaxLen + 1];
  UInt32 codes[kMaxLen + 1];
#endif

  unsigned num;
  {
    unsigned i;
    // UInt32 sum = 0;

#if NUM_COUNTERS > 2
    
#define CTR_ITEM_FOR_FREQ(freq) \
    counters[(freq) >= NUM_COUNTERS - 1 ? NUM_COUNTERS - 1 : (unsigned)(freq)]

    for (i = 0; i < NUM_COUNTERS; i++)
      counters[i] = 0;
    memset(lens, 0, numSymbols);
    {
      const UInt32 *fp = freqs + numSymbols;
#define NUM_UNROLLS 1
#if NUM_UNROLLS > 1 // use 1 if odd (numSymbols) is possisble
      if (numSymbols & 1)
      {
        UInt32 f;
        f = *--fp;  CTR_ITEM_FOR_FREQ(f)++;
        // sum += f;
      }
#endif
      do
      {
        UInt32 f;
        fp -= NUM_UNROLLS;
        f = fp[0];  CTR_ITEM_FOR_FREQ(f)++;
        // sum += f;
#if NUM_UNROLLS > 1
        f = fp[1];  CTR_ITEM_FOR_FREQ(f)++;
        // sum += f;
#endif
      }
      while (fp != freqs);
    }
#if 0
    printf("\nsum=%8u numSymbols =%3u ctrs:", sum, numSymbols);
    {
      unsigned k = 0;
      for (k = 0; k < NUM_COUNTERS; k++)
        printf(" %u", counters[k]);
    }
#endif
        
    num = counters[1];
    counters[1] = 0;
    for (i = 2; i != NUM_COUNTERS; i += 2)
    {
      const unsigned c0 = (counters    )[i];
      const unsigned c1 = (counters + 1)[i];
      (counters    )[i] = num;  num += c0;
      (counters + 1)[i] = num;  num += c1;
    }
    counters[0] = num; // we want to write (freq==0) symbols to the end of (p) array
    {
      i = 0;
      do
      {
        const UInt32 f = freqs[i];
#if 0
        if (f == 0) lens[i] = 0; else
#endif
          p[CTR_ITEM_FOR_FREQ(f)++] = i | (f << NUM_BITS);
      }
      while (++i != numSymbols);
    }
    HeapSort(p + counters[NUM_COUNTERS - 2], counters[NUM_COUNTERS - 1] - counters[NUM_COUNTERS - 2]);
    
#else // NUM_COUNTERS <= 2
    
    num = 0;
    for (i = 0; i < numSymbols; i++)
    {
      const UInt32 freq = freqs[i];
      if (freq == 0)
        lens[i] = 0;
      else
        p[num++] = i | (freq << NUM_BITS);
    }
    HeapSort(p, num);

#endif
  }

  if (num <= 2)
  {
    unsigned minCode = 0;
    unsigned maxCode = 1;
    if (num)
    {
      maxCode = (unsigned)p[(size_t)num - 1] & MASK;
      if (num == 2)
      {
        minCode = (unsigned)p[0] & MASK;
        if (minCode > maxCode)
        {
          const unsigned temp = minCode;
          minCode = maxCode;
          maxCode = temp;
        }
      }
      else if (maxCode == 0)
        maxCode++;
    }
    p[minCode] = 0;
    p[maxCode] = 1;
    lens[minCode] = lens[maxCode] = 1;
    return;
  }
  {
    unsigned i;
    for (i = 0; i <= kMaxLen; i++)
      lenCounters[i] = 0;
    lenCounters[1] = 2;  // by default root node has 2 child leaves at level 1.
  }
  // if (num != 2)
  {
    // num > 2
    // the binary tree will contain (num - 1) internal nodes.
    // p[num - 2] will be root node of binary tree.
    UInt32 *b;
    UInt32 *n;
    // first node will have two leaf childs: p[0] and p[1]:
    // p[0] += p[1] & FREQ_MASK; // set frequency sum of child leafs
    // if (pi == n) exit(0);
    // if (pi != n)
    {
      UInt32 fb = (p[1] & FREQ_MASK) + p[0];
      UInt32 f = p[2] & FREQ_MASK;
      const UInt32 *pi = p + 2;
      UInt32 *e = p;
      UInt32 eHi = 0;
      n = p + num;
      b = p;
      // p[0] = fb;
      for (;;)
      {
        // (b <= e)
        UInt32 sum;
        e++;
        UPDATE_E(eHi)

        // (b < e)

        // p range : high bits
        // [0, b)  : parent :     processed nodes that have parent and childs
        // [b, e)  : FREQ   : non-processed nodes that have no parent but have childs
        // [e, pi) : FREQ   :     processed leaves for which parent node was     created
        // [pi, n) : FREQ   : non-processed leaves for which parent node was not created

        // first child
        // note : (*b < f) is same result as ((*b & FREQ_MASK) < f)
        if (fb < f)
        {
          // node freq is smaller
          sum = fb & FREQ_MASK;
          STORE_PARENT_DIRECT (b, fb, eHi)
          b++;
          fb = *b;
          if (b == e)
          {
            if (++pi == n)
              break;
            sum += f;
            fb &= MASK;
            fb |= sum;
            *e = fb;
            f = *pi & FREQ_MASK;
            continue;
          }
        }
        else if (++pi == n)
        {
          STORE_PARENT_DIRECT (b, fb, eHi)
          b++;
          break;
        }
        else
        {
          sum = f;
          f = *pi & FREQ_MASK;
        }

        // (b < e)

        // second child
        if (fb < f)
        {
          sum += fb;
          sum &= FREQ_MASK;
          STORE_PARENT_DIRECT (b, fb, eHi)
          b++;
          *e = (*e & MASK) | sum; // set frequency sum
          // (b <= e) is possible here
          fb = *b;
        }
        else if (++pi == n)
          break;
        else
        {
          sum += f;
          f = *pi & FREQ_MASK;
          *e = (*e & MASK) | sum; // set frequency sum
        }
      }
    }
    
    // printf("\nnum-e=%3u, numSymbols=%3u, num=%3u, b=%3u", n - e, numSymbols, n - p, b - p);
    {
      n -= 2;
      *n &= MASK; // root node : we clear high bits (zero bits mean level == 0)
      if (n != b)
      {
        // We go here, if we have some number of non-created nodes up to root.
        // We process them in simplified code:
        // position of parent for each pair of nodes is known.
        // n[-2], n[-1] : current pair of child nodes
        // (p1) : parent node for current pair.
        UInt32 *p1 = n;
        do
        {
          const unsigned len = LOAD_PARENT(p1) + 1;
          p1--;
          (lenCounters    )[len] -= 2;                  // we remove 2 leaves from level (len)
          (lenCounters + 1)[len] += 2 * 2;  // we add 4 leaves at level (len + 1)
          n -= 2;
          STORE_PARENT (n    , n[0], len)
          STORE_PARENT (n + 1, n[1], len)
        }
        while (n != b);
      }
    }
    
    if (b != p)
    {
      // we detect level of each node (realtive to root),
      // and update lenCounters[].
      // We process only intermediate nodes and we don't process leaves.
      do
      {
        // if (ii <  b) : parent_bits_of (p[ii]) == index of parent node : ii < (p[ii])
        // if (ii >= b) : parent_bits_of (p[ii]) == level of this (ii) node in tree
        unsigned len;
        b--;
        len = (unsigned)LOAD_PARENT(p + LOAD_PARENT(b)) + 1;
        STORE_PARENT (b, *b, len)
        if (len >= maxLen)
        {
          // We are not allowed to create node at level (maxLen) and higher,
          // because all leaves must be placed to level (maxLen) or lower.
          // We find nearest allowed leaf and place current node to level of that leaf:
          for (len = maxLen - 1; lenCounters[len] == 0; len--) {}
        }
        lenCounters[len]--;                 // we remove 1 leaf from level (len)
        (lenCounters + 1)[len] += 2;  // we add 2 leaves at level (len + 1)
      }
      while (b != p);
    }
  }
  {
    {
      unsigned len = maxLen;
      const UInt32 *p2 = p;
      do
      {
        unsigned k = lenCounters[len];
        if (k)
          do
            lens[(unsigned)*p2++ & MASK] = (Byte)len;
          while (--k);
      }
      while (--len);
    }
    codes[0] = 0; // we don't want garbage values to be written to p[] array.
    // codes[1] = 0;
    {
      UInt32 code = 0;
      unsigned len;
      for (len = 0; len < kMaxLen; len++)
        (codes + 1)[len] = code = (code + lenCounters[len]) << 1;
    }
    /* if (code + lenCounters[kMaxLen] - 1 != (1 << kMaxLen) - 1) throw 1; */
    {
      const Byte * const limit = lens + numSymbols;
      do
      {
        unsigned len;
        UInt32 c;
        len = lens[0];  c = codes[len];  p[0] = c;  codes[len] = c + 1;
        // len = lens[1];  c = codes[len];  p[1] = c;  codes[len] = c + 1;
        p += 1;
        lens += 1;
      }
      while (lens != limit);
    }
  }
}

#undef kMaxLen
#undef NUM_BITS
#undef MASK
#undef FREQ_MASK
#undef NUM_COUNTERS
#undef CTR_ITEM_FOR_FREQ
#undef LOAD_PARENT
#undef STORE_PARENT
#undef STORE_PARENT_DIRECT
#undef UPDATE_E
#undef HI_HALF_OFFSET
#undef NUM_UNROLLS
#undef lenCounters
#undef codes

/* ================ unit: C/LzFind.c ================ */
/* LzFind.c -- Match finder for LZ algorithms
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>
// #include <stdio.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define kBlockMoveAlign       (1 << 7)    // alignment for memmove()
#define kBlockSizeAlign       (1 << 16)   // alignment for block allocation
#define kBlockSizeReserveMin  (1 << 24)   // it's 1/256 from 4 GB dictinary

#define kEmptyHashValue 0

#define kMaxValForNormalize ((UInt32)0)
// #define kMaxValForNormalize ((UInt32)(1 << 20) + 0xfff) // for debug

// #define kNormalizeAlign (1 << 7) // alignment for speculated accesses

#define GET_AVAIL_BYTES(p) \
  Inline_MatchFinder_GetNumAvailableBytes(p)


// #define kFix5HashSize (kHash2Size + kHash3Size + kHash4Size)
#define kFix5HashSize kFix4HashSize

/*
 HASH2_CALC:
   if (hv) match, then cur[0] and cur[1] also match
*/
#define HASH2_CALC hv = GetUi16(cur);

// (crc[0 ... 255] & 0xFF) provides one-to-one correspondence to [0 ... 255]

/*
 HASH3_CALC:
   if (cur[0]) and (h2) match, then cur[1]            also match
   if (cur[0]) and (hv) match, then cur[1] and cur[2] also match
*/
#define HASH3_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  h2 = temp & (kHash2Size - 1); \
  hv = (temp ^ ((UInt32)cur[2] << 8)) & p->hashMask; }

#define HASH4_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  h2 = temp & (kHash2Size - 1); \
  temp ^= ((UInt32)cur[2] << 8); \
  h3 = temp & (kHash3Size - 1); \
  hv = (temp ^ (p->crc[cur[3]] << kLzHash_CrcShift_1)) & p->hashMask; }

#define HASH5_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  h2 = temp & (kHash2Size - 1); \
  temp ^= ((UInt32)cur[2] << 8); \
  h3 = temp & (kHash3Size - 1); \
  temp ^= (p->crc[cur[3]] << kLzHash_CrcShift_1); \
  /* h4 = temp & p->hash4Mask; */ /* (kHash4Size - 1); */ \
  hv = (temp ^ (p->crc[cur[4]] << kLzHash_CrcShift_2)) & p->hashMask; }

#define HASH_ZIP_CALC hv = ((cur[2] | ((UInt32)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;


static void LzInWindow_Free(CMatchFinder *p, ISzAllocPtr alloc)
{
  // if (!p->directInput)
  {
    ISzAlloc_Free(alloc, p->bufBase);
    p->bufBase = NULL;
  }
}


static int LzInWindow_Create2(CMatchFinder *p, UInt32 blockSize, ISzAllocPtr alloc)
{
  if (blockSize == 0)
    return 0;
  if (!p->bufBase || p->blockSize != blockSize)
  {
    // size_t blockSizeT;
    LzInWindow_Free(p, alloc);
    p->blockSize = blockSize;
    // blockSizeT = blockSize;
    
    // printf("\nblockSize = 0x%x\n", blockSize);
    /*
    #if defined _WIN64
    // we can allocate 4GiB, but still use UInt32 for (p->blockSize)
    // we use UInt32 type for (p->blockSize), because
    // we don't want to wrap over 4 GiB,
    // when we use (p->streamPos - p->pos) that is UInt32.
    if (blockSize >= (UInt32)0 - (UInt32)kBlockSizeAlign)
    {
      blockSizeT = ((size_t)1 << 32);
      printf("\nchanged to blockSizeT = 4GiB\n");
    }
    #endif
    */
    
    p->bufBase = (Byte *)ISzAlloc_Alloc(alloc, blockSize);
    // printf("\nbufferBase = %p\n", p->bufBase);
    // return 0; // for debug
  }
  return (p->bufBase != NULL);
}

static const Byte *MatchFinder_GetPointerToCurrentPos(void *p)
{
  return ((CMatchFinder *)p)->buffer;
}

static UInt32 MatchFinder_GetNumAvailableBytes(void *p)
{
  return GET_AVAIL_BYTES((CMatchFinder *)p);
}


Z7_NO_INLINE
static void MatchFinder_ReadBlock(CMatchFinder *p)
{
  if (p->streamEndWasReached || p->result != SZ_OK)
    return;

  /* We use (p->streamPos - p->pos) value.
     (p->streamPos < p->pos) is allowed. */

  if (p->directInput)
  {
    UInt32 curSize = 0xFFFFFFFF - GET_AVAIL_BYTES(p);
    if (curSize > p->directInputRem)
      curSize = (UInt32)p->directInputRem;
    p->streamPos += curSize;
    p->directInputRem -= curSize;
    if (p->directInputRem == 0)
      p->streamEndWasReached = 1;
    return;
  }
  
  for (;;)
  {
    const Byte *dest = p->buffer + GET_AVAIL_BYTES(p);
    size_t size = (size_t)(p->bufBase + p->blockSize - dest);
    if (size == 0)
    {
      /* we call ReadBlock() after NeedMove() and MoveBlock().
         NeedMove() and MoveBlock() povide more than (keepSizeAfter)
         to the end of (blockSize).
         So we don't execute this branch in normal code flow.
         We can go here, if we will call ReadBlock() before NeedMove(), MoveBlock().
      */
      // p->result = SZ_ERROR_FAIL; // we can show error here
      return;
    }

    // #define kRead 3
    // if (size > kRead) size = kRead; // for debug

    /*
    // we need cast (Byte *)dest.
    #ifdef __clang__
      #pragma GCC diagnostic ignored "-Wcast-qual"
    #endif
    */
    p->result = ISeqInStream_Read(p->stream,
        p->bufBase + (dest - p->bufBase), &size);
    if (p->result != SZ_OK)
      return;
    if (size == 0)
    {
      p->streamEndWasReached = 1;
      return;
    }
    p->streamPos += (UInt32)size;
    if (GET_AVAIL_BYTES(p) > p->keepSizeAfter)
      return;
    /* here and in another (p->keepSizeAfter) checks we keep on 1 byte more than was requested by Create() function
         (GET_AVAIL_BYTES(p) >= p->keepSizeAfter) - minimal required size */
  }

  // on exit: (p->result != SZ_OK || p->streamEndWasReached || GET_AVAIL_BYTES(p) > p->keepSizeAfter)
}



Z7_NO_INLINE
void MatchFinder_MoveBlock(CMatchFinder *p)
{
  const size_t offset = (size_t)(p->buffer - p->bufBase) - p->keepSizeBefore;
  const size_t keepBefore = (offset & (kBlockMoveAlign - 1)) + p->keepSizeBefore;
  p->buffer = p->bufBase + keepBefore;
  memmove(p->bufBase,
      p->bufBase + (offset & ~((size_t)kBlockMoveAlign - 1)),
      keepBefore + (size_t)GET_AVAIL_BYTES(p));
}

/* We call MoveBlock() before ReadBlock().
   So MoveBlock() can be wasteful operation, if the whole input data
   can fit in current block even without calling MoveBlock().
   in important case where (dataSize <= historySize)
     condition (p->blockSize > dataSize + p->keepSizeAfter) is met
     So there is no MoveBlock() in that case case.
*/

int MatchFinder_NeedMove(CMatchFinder *p)
{
  if (p->directInput)
    return 0;
  if (p->streamEndWasReached || p->result != SZ_OK)
    return 0;
  return ((size_t)(p->bufBase + p->blockSize - p->buffer) <= p->keepSizeAfter);
}

void MatchFinder_ReadIfRequired(CMatchFinder *p)
{
  if (p->keepSizeAfter >= GET_AVAIL_BYTES(p))
    MatchFinder_ReadBlock(p);
}



static void MatchFinder_SetDefaultSettings(CMatchFinder *p)
{
  p->cutValue = 32;
  p->btMode = 1;
  p->numHashBytes = 4;
  p->numHashBytes_Min = 2;
  p->numHashOutBits = 0;
  p->bigHash = 0;
}

#define kCrcPoly 0xEDB88320

void MatchFinder_Construct(CMatchFinder *p)
{
  unsigned i;
  p->buffer = NULL;
  p->bufBase = NULL;
  p->directInput = 0;
  p->stream = NULL;
  p->hash = NULL;
  p->expectedDataSize = (UInt64)(Int64)-1;
  MatchFinder_SetDefaultSettings(p);

  for (i = 0; i < 256; i++)
  {
    UInt32 r = (UInt32)i;
    unsigned j;
    for (j = 0; j < 8; j++)
      r = (r >> 1) ^ (kCrcPoly & ((UInt32)0 - (r & 1)));
    p->crc[i] = r;
  }
}

#undef kCrcPoly

static void MatchFinder_FreeThisClassMemory(CMatchFinder *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->hash);
  p->hash = NULL;
}

void MatchFinder_Free(CMatchFinder *p, ISzAllocPtr alloc)
{
  MatchFinder_FreeThisClassMemory(p, alloc);
  LzInWindow_Free(p, alloc);
}

static CLzRef* AllocRefs(size_t num, ISzAllocPtr alloc)
{
  const size_t sizeInBytes = (size_t)num * sizeof(CLzRef);
  if (sizeInBytes / sizeof(CLzRef) != num)
    return NULL;
  return (CLzRef *)ISzAlloc_Alloc(alloc, sizeInBytes);
}

#if (kBlockSizeReserveMin < kBlockSizeAlign * 2)
  #error Stop_Compiling_Bad_Reserve
#endif



static UInt32 GetBlockSize(CMatchFinder *p, UInt32 historySize)
{
  UInt32 blockSize = (p->keepSizeBefore + p->keepSizeAfter);
  /*
  if (historySize > kMaxHistorySize)
    return 0;
  */
  // printf("\nhistorySize == 0x%x\n", historySize);
  
  if (p->keepSizeBefore < historySize || blockSize < p->keepSizeBefore)  // if 32-bit overflow
    return 0;
  
  {
    const UInt32 kBlockSizeMax = (UInt32)0 - (UInt32)kBlockSizeAlign;
    const UInt32 rem = kBlockSizeMax - blockSize;
    const UInt32 reserve = (blockSize >> (blockSize < ((UInt32)1 << 30) ? 1 : 2))
        + (1 << 12) + kBlockMoveAlign + kBlockSizeAlign; // do not overflow 32-bit here
    if (blockSize >= kBlockSizeMax
        || rem < kBlockSizeReserveMin) // we reject settings that will be slow
      return 0;
    if (reserve >= rem)
      blockSize = kBlockSizeMax;
    else
    {
      blockSize += reserve;
      blockSize &= ~(UInt32)(kBlockSizeAlign - 1);
    }
  }
  // printf("\n LzFind_blockSize = %x\n", blockSize);
  // printf("\n LzFind_blockSize = %d\n", blockSize >> 20);
  return blockSize;
}


// input is historySize
static UInt32 MatchFinder_GetHashMask2(CMatchFinder *p, UInt32 hs)
{
  if (p->numHashBytes == 2)
    return (1 << 16) - 1;
  if (hs != 0)
    hs--;
  hs |= (hs >> 1);
  hs |= (hs >> 2);
  hs |= (hs >> 4);
  hs |= (hs >> 8);
  // we propagated 16 bits in (hs). Low 16 bits must be set later
  if (hs >= (1 << 24))
  {
    if (p->numHashBytes == 3)
      hs = (1 << 24) - 1;
    /* if (bigHash) mode, GetHeads4b() in LzFindMt.c needs (hs >= ((1 << 24) - 1))) */
  }
  // (hash_size >= (1 << 16)) : Required for (numHashBytes > 2)
  hs |= (1 << 16) - 1; /* don't change it! */
  // bt5: we adjust the size with recommended minimum size
  if (p->numHashBytes >= 5)
    hs |= (256 << kLzHash_CrcShift_2) - 1;
  return hs;
}

// input is historySize
static UInt32 MatchFinder_GetHashMask(CMatchFinder *p, UInt32 hs)
{
  if (p->numHashBytes == 2)
    return (1 << 16) - 1;
  if (hs != 0)
    hs--;
  hs |= (hs >> 1);
  hs |= (hs >> 2);
  hs |= (hs >> 4);
  hs |= (hs >> 8);
  // we propagated 16 bits in (hs). Low 16 bits must be set later
  hs >>= 1;
  if (hs >= (1 << 24))
  {
    if (p->numHashBytes == 3)
      hs = (1 << 24) - 1;
    else
      hs >>= 1;
    /* if (bigHash) mode, GetHeads4b() in LzFindMt.c needs (hs >= ((1 << 24) - 1))) */
  }
  // (hash_size >= (1 << 16)) : Required for (numHashBytes > 2)
  hs |= (1 << 16) - 1; /* don't change it! */
  // bt5: we adjust the size with recommended minimum size
  if (p->numHashBytes >= 5)
    hs |= (256 << kLzHash_CrcShift_2) - 1;
  return hs;
}


int MatchFinder_Create(CMatchFinder *p, UInt32 historySize,
    UInt32 keepAddBufferBefore, UInt32 matchMaxLen, UInt32 keepAddBufferAfter,
    ISzAllocPtr alloc)
{
  /* we need one additional byte in (p->keepSizeBefore),
     since we use MoveBlock() after (p->pos++) and before dictionary using */
  // keepAddBufferBefore = (UInt32)0xFFFFFFFF - (1 << 22); // for debug
  p->keepSizeBefore = historySize + keepAddBufferBefore + 1;

  keepAddBufferAfter += matchMaxLen;
  /* we need (p->keepSizeAfter >= p->numHashBytes) */
  if (keepAddBufferAfter < p->numHashBytes)
    keepAddBufferAfter = p->numHashBytes;
  // keepAddBufferAfter -= 2; // for debug
  p->keepSizeAfter = keepAddBufferAfter;

  if (p->directInput)
    p->blockSize = 0;
  if (p->directInput || LzInWindow_Create2(p, GetBlockSize(p, historySize), alloc))
  {
    size_t hashSizeSum;
    {
      UInt32 hs;
      UInt32 hsCur;
      
      if (p->numHashOutBits != 0)
      {
        unsigned numBits = p->numHashOutBits;
        const unsigned nbMax =
            (p->numHashBytes == 2 ? 16 :
            (p->numHashBytes == 3 ? 24 : 32));
        if (numBits >= nbMax)
          numBits = nbMax;
        if (numBits >= 32)
          hs = (UInt32)0 - 1;
        else
          hs = ((UInt32)1 << numBits) - 1;
        // (hash_size >= (1 << 16)) : Required for (numHashBytes > 2)
        hs |= (1 << 16) - 1; /* don't change it! */
        if (p->numHashBytes >= 5)
          hs |= (256 << kLzHash_CrcShift_2) - 1;
        {
          const UInt32 hs2 = MatchFinder_GetHashMask2(p, historySize);
          if (hs >= hs2)
            hs = hs2;
        }
        hsCur = hs;
        if (p->expectedDataSize < historySize)
        {
          const UInt32 hs2 = MatchFinder_GetHashMask2(p, (UInt32)p->expectedDataSize);
          if (hsCur >= hs2)
            hsCur = hs2;
        }
      }
      else
      {
        hs = MatchFinder_GetHashMask(p, historySize);
        hsCur = hs;
        if (p->expectedDataSize < historySize)
        {
          hsCur = MatchFinder_GetHashMask(p, (UInt32)p->expectedDataSize);
          if (hsCur >= hs) // is it possible?
            hsCur = hs;
        }
      }

      p->hashMask = hsCur;

      hashSizeSum = hs;
      hashSizeSum++;
      if (hashSizeSum < hs)
        return 0;
      {
        UInt32 fixedHashSize = 0;
        if (p->numHashBytes > 2 && p->numHashBytes_Min <= 2) fixedHashSize += kHash2Size;
        if (p->numHashBytes > 3 && p->numHashBytes_Min <= 3) fixedHashSize += kHash3Size;
        // if (p->numHashBytes > 4) p->fixedHashSize += hs4; // kHash4Size;
        hashSizeSum += fixedHashSize;
        p->fixedHashSize = fixedHashSize;
      }
    }

    p->matchMaxLen = matchMaxLen;

    {
      size_t newSize;
      size_t numSons;
      const UInt32 newCyclicBufferSize = historySize + 1; // do not change it
      p->historySize = historySize;
      p->cyclicBufferSize = newCyclicBufferSize; // it must be = (historySize + 1)
      
      numSons = newCyclicBufferSize;
      if (p->btMode)
        numSons <<= 1;
      newSize = hashSizeSum + numSons;

      if (numSons < newCyclicBufferSize || newSize < numSons)
        return 0;

      // aligned size is not required here, but it can be better for some loops
      #define NUM_REFS_ALIGN_MASK 0xF
      newSize = (newSize + NUM_REFS_ALIGN_MASK) & ~(size_t)NUM_REFS_ALIGN_MASK;

      // 22.02: we don't reallocate buffer, if old size is enough
      if (p->hash && p->numRefs >= newSize)
        return 1;
      
      MatchFinder_FreeThisClassMemory(p, alloc);
      p->numRefs = newSize;
      p->hash = AllocRefs(newSize, alloc);
      
      if (p->hash)
      {
        p->son = p->hash + hashSizeSum;
        return 1;
      }
    }
  }

  MatchFinder_Free(p, alloc);
  return 0;
}


static void MatchFinder_SetLimits(CMatchFinder *p)
{
  UInt32 k;
  UInt32 n = kMaxValForNormalize - p->pos;
  if (n == 0)
    n = (UInt32)(Int32)-1;  // we allow (pos == 0) at start even with (kMaxValForNormalize == 0)
  
  k = p->cyclicBufferSize - p->cyclicBufferPos;
  if (k < n)
    n = k;

  k = GET_AVAIL_BYTES(p);
  {
    const UInt32 ksa = p->keepSizeAfter;
    UInt32 mm = p->matchMaxLen;
    if (k > ksa)
      k -= ksa; // we must limit exactly to keepSizeAfter for ReadBlock
    else if (k >= mm)
    {
      // the limitation for (p->lenLimit) update
      k -= mm;   // optimization : to reduce the number of checks
      k++;
      // k = 1; // non-optimized version : for debug
    }
    else
    {
      mm = k;
      if (k != 0)
        k = 1;
    }
    p->lenLimit = mm;
  }
  if (k < n)
    n = k;
  
  p->posLimit = p->pos + n;
}


void MatchFinder_Init_LowHash(CMatchFinder *p)
{
  size_t i;
  CLzRef *items = p->hash;
  const size_t numItems = p->fixedHashSize;
  for (i = 0; i < numItems; i++)
    items[i] = kEmptyHashValue;
}


void MatchFinder_Init_HighHash(CMatchFinder *p)
{
  size_t i;
  CLzRef *items = p->hash + p->fixedHashSize;
  const size_t numItems = (size_t)p->hashMask + 1;
  for (i = 0; i < numItems; i++)
    items[i] = kEmptyHashValue;
}


void MatchFinder_Init_4(CMatchFinder *p)
{
  if (!p->directInput)
    p->buffer = p->bufBase;
  {
    /* kEmptyHashValue = 0 (Zero) is used in hash tables as NO-VALUE marker.
       the code in CMatchFinderMt expects (pos = 1) */
    p->pos =
    p->streamPos =
        1; // it's smallest optimal value. do not change it
        // 0; // for debug
  }
  p->result = SZ_OK;
  p->streamEndWasReached = 0;
}


// (CYC_TO_POS_OFFSET == 0) is expected by some optimized code
#define CYC_TO_POS_OFFSET 0
// #define CYC_TO_POS_OFFSET 1 // for debug

void MatchFinder_Init(void *_p)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  MatchFinder_Init_HighHash(p);
  MatchFinder_Init_LowHash(p);
  MatchFinder_Init_4(p);
  // if (readData)
  MatchFinder_ReadBlock(p);

  /* if we init (cyclicBufferPos = pos), then we can use one variable
     instead of both (cyclicBufferPos) and (pos) : only before (cyclicBufferPos) wrapping */
  p->cyclicBufferPos = (p->pos - CYC_TO_POS_OFFSET); // init with relation to (pos)
  // p->cyclicBufferPos = 0; // smallest value
  // p->son[0] = p->son[1] = 0; // unused: we can init skipped record for speculated accesses.
  MatchFinder_SetLimits(p);
}



#ifdef MY_CPU_X86_OR_AMD64
  #if defined(__clang__) && (__clang_major__ >= 4) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900)
    // || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1900)

      #define USE_LZFIND_SATUR_SUB_128
      #define USE_LZFIND_SATUR_SUB_256
      #define LZFIND_ATTRIB_SSE41 __attribute__((__target__("sse4.1")))
      #define LZFIND_ATTRIB_AVX2  __attribute__((__target__("avx2")))
  #elif defined(_MSC_VER)
    #if (_MSC_VER >= 1600)
      #define USE_LZFIND_SATUR_SUB_128
    #endif
    #if (_MSC_VER >= 1900)
      #define USE_LZFIND_SATUR_SUB_256
    #endif
  #endif

#elif defined(MY_CPU_ARM64) \
  /* || (defined(__ARM_ARCH) && (__ARM_ARCH >= 7)) */

  #if  defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30800) \
    || defined(__GNUC__) && (__GNUC__ >= 6)
      #define USE_LZFIND_SATUR_SUB_128
    #ifdef MY_CPU_ARM64
      // #define LZFIND_ATTRIB_SSE41 __attribute__((__target__("")))
    #else
      #define LZFIND_ATTRIB_SSE41 __attribute__((__target__("fpu=neon")))
    #endif

  #elif defined(_MSC_VER)
    #if (_MSC_VER >= 1910)
      #define USE_LZFIND_SATUR_SUB_128
    #endif
  #endif

  #if defined(Z7_MSC_VER_ORIGINAL) && defined(MY_CPU_ARM64)
    #include <arm64_neon.h>
  #else
    #include <arm_neon.h>
  #endif

#endif


#ifdef USE_LZFIND_SATUR_SUB_128

// #define Z7_SHOW_HW_STATUS

#ifdef Z7_SHOW_HW_STATUS
#include <stdio.h>
#define PRF(x) x
PRF(;)
#else
#define PRF(x)
#endif


#ifdef MY_CPU_ARM_OR_ARM64

#ifdef MY_CPU_ARM64
// #define FORCE_LZFIND_SATUR_SUB_128
#endif
typedef uint32x4_t LzFind_v128;
#define SASUB_128_V(v, s) \
  vsubq_u32(vmaxq_u32(v, s), s)

#else // MY_CPU_ARM_OR_ARM64

#include <smmintrin.h> // sse4.1

typedef __m128i LzFind_v128;
// SSE 4.1
#define SASUB_128_V(v, s)   \
  _mm_sub_epi32(_mm_max_epu32(v, s), s)

#endif // MY_CPU_ARM_OR_ARM64


#define SASUB_128(i) \
  *(      LzFind_v128 *)(      void *)(items + (i) * 4) = SASUB_128_V( \
  *(const LzFind_v128 *)(const void *)(items + (i) * 4), sub2);


Z7_NO_INLINE
static
#ifdef LZFIND_ATTRIB_SSE41
LZFIND_ATTRIB_SSE41
#endif
void
Z7_FASTCALL
LzFind_SaturSub_128(UInt32 subValue, CLzRef *items, const CLzRef *lim)
{
  const LzFind_v128 sub2 =
    #ifdef MY_CPU_ARM_OR_ARM64
      vdupq_n_u32(subValue);
    #else
      _mm_set_epi32((Int32)subValue, (Int32)subValue, (Int32)subValue, (Int32)subValue);
    #endif
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SASUB_128(0)  SASUB_128(1)  items += 2 * 4;
    SASUB_128(0)  SASUB_128(1)  items += 2 * 4;
  }
  while (items != lim);
}



#ifdef USE_LZFIND_SATUR_SUB_256

#include <immintrin.h> // avx
/*
clang :immintrin.h uses
#if !(defined(_MSC_VER) || defined(__SCE__)) || __has_feature(modules) ||      \
    defined(__AVX2__)
#include <avx2intrin.h>
#endif
so we need <avxintrin.h> for clang-cl */

#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif

// AVX2:
#define SASUB_256(i) \
    *(      __m256i *)(      void *)(items + (i) * 8) = \
   _mm256_sub_epi32(_mm256_max_epu32( \
    *(const __m256i *)(const void *)(items + (i) * 8), sub2), sub2);

Z7_NO_INLINE
static
#ifdef LZFIND_ATTRIB_AVX2
LZFIND_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
LzFind_SaturSub_256(UInt32 subValue, CLzRef *items, const CLzRef *lim)
{
  const __m256i sub2 = _mm256_set_epi32(
      (Int32)subValue, (Int32)subValue, (Int32)subValue, (Int32)subValue,
      (Int32)subValue, (Int32)subValue, (Int32)subValue, (Int32)subValue);
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SASUB_256(0)  SASUB_256(1)  items += 2 * 8;
    SASUB_256(0)  SASUB_256(1)  items += 2 * 8;
  }
  while (items != lim);
}
#endif // USE_LZFIND_SATUR_SUB_256

#ifndef FORCE_LZFIND_SATUR_SUB_128
typedef void (Z7_FASTCALL *LZFIND_SATUR_SUB_CODE_FUNC)(
    UInt32 subValue, CLzRef *items, const CLzRef *lim);
static LZFIND_SATUR_SUB_CODE_FUNC g_LzFind_SaturSub;
#endif // FORCE_LZFIND_SATUR_SUB_128

#endif // USE_LZFIND_SATUR_SUB_128


// kEmptyHashValue must be zero
// #define SASUB_32(i)  { UInt32 v = items[i];  UInt32 m = v - subValue;  if (v < subValue) m = kEmptyHashValue;  items[i] = m; }
#define SASUB_32(i)  { UInt32 v = items[i];  if (v < subValue) v = subValue; items[i] = v - subValue; }

#ifdef FORCE_LZFIND_SATUR_SUB_128

#define DEFAULT_SaturSub LzFind_SaturSub_128

#else

#define DEFAULT_SaturSub LzFind_SaturSub_32

Z7_NO_INLINE
static
void
Z7_FASTCALL
LzFind_SaturSub_32(UInt32 subValue, CLzRef *items, const CLzRef *lim)
{
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  do
  {
    SASUB_32(0)  SASUB_32(1)  items += 2;
    SASUB_32(0)  SASUB_32(1)  items += 2;
    SASUB_32(0)  SASUB_32(1)  items += 2;
    SASUB_32(0)  SASUB_32(1)  items += 2;
  }
  while (items != lim);
}

#endif


Z7_NO_INLINE
void MatchFinder_Normalize3(UInt32 subValue, CLzRef *items, size_t numItems)
{
  #define LZFIND_NORM_ALIGN_BLOCK_SIZE (1 << 7)
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  for (; numItems != 0 && ((unsigned)(ptrdiff_t)items & (LZFIND_NORM_ALIGN_BLOCK_SIZE - 1)) != 0; numItems--)
  {
    SASUB_32(0)
    items++;
  }
  {
    const size_t k_Align_Mask = (LZFIND_NORM_ALIGN_BLOCK_SIZE / 4 - 1);
    CLzRef *lim = items + (numItems & ~(size_t)k_Align_Mask);
    numItems &= k_Align_Mask;
    if (items != lim)
    {
      #if defined(USE_LZFIND_SATUR_SUB_128) && !defined(FORCE_LZFIND_SATUR_SUB_128)
        if (g_LzFind_SaturSub)
          g_LzFind_SaturSub(subValue, items, lim);
        else
      #endif
          DEFAULT_SaturSub(subValue, items, lim);
    }
    items = lim;
  }
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
  for (; numItems != 0; numItems--)
  {
    SASUB_32(0)
    items++;
  }
}



// call MatchFinder_CheckLimits() only after (p->pos++) update

Z7_NO_INLINE
static void MatchFinder_CheckLimits(CMatchFinder *p)
{
  if (// !p->streamEndWasReached && p->result == SZ_OK &&
      p->keepSizeAfter == GET_AVAIL_BYTES(p))
  {
    // we try to read only in exact state (p->keepSizeAfter == GET_AVAIL_BYTES(p))
    if (MatchFinder_NeedMove(p))
      MatchFinder_MoveBlock(p);
    MatchFinder_ReadBlock(p);
  }

  if (p->pos == kMaxValForNormalize)
  if (GET_AVAIL_BYTES(p) >= p->numHashBytes) // optional optimization for last bytes of data.
    /*
       if we disable normalization for last bytes of data, and
       if (data_size == 4 GiB), we don't call wastfull normalization,
       but (pos) will be wrapped over Zero (0) in that case.
       And we cannot resume later to normal operation
    */
  {
    // MatchFinder_Normalize(p);
    /* after normalization we need (p->pos >= p->historySize + 1); */
    /* we can reduce subValue to aligned value, if want to keep alignment
       of (p->pos) and (p->buffer) for speculated accesses. */
    const UInt32 subValue = (p->pos - p->historySize - 1) /* & ~(UInt32)(kNormalizeAlign - 1) */;
    // const UInt32 subValue = (1 << 15); // for debug
    // printf("\nMatchFinder_Normalize() subValue == 0x%x\n", subValue);
    MatchFinder_REDUCE_OFFSETS(p, subValue)
    MatchFinder_Normalize3(subValue, p->hash, (size_t)p->hashMask + 1 + p->fixedHashSize);
    {
      size_t numSonRefs = p->cyclicBufferSize;
      if (p->btMode)
        numSonRefs <<= 1;
      MatchFinder_Normalize3(subValue, p->son, numSonRefs);
    }
  }

  if (p->cyclicBufferPos == p->cyclicBufferSize)
    p->cyclicBufferPos = 0;
  
  MatchFinder_SetLimits(p);
}


/*
  (lenLimit > maxLen)
*/
Z7_FORCE_INLINE
static UInt32 * Hc_GetMatchesSpec(size_t lenLimit, UInt32 curMatch, UInt32 pos, const Byte *cur, CLzRef *son,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 cutValue,
    UInt32 *d, unsigned maxLen)
{
  /*
  son[_cyclicBufferPos] = curMatch;
  for (;;)
  {
    UInt32 delta = pos - curMatch;
    if (cutValue-- == 0 || delta >= _cyclicBufferSize)
      return d;
    {
      const Byte *pb = cur - delta;
      curMatch = son[_cyclicBufferPos - delta + (_cyclicBufferPos < delta ? _cyclicBufferSize : 0)];
      if (pb[maxLen] == cur[maxLen] && *pb == *cur)
      {
        UInt32 len = 0;
        while (++len != lenLimit)
          if (pb[len] != cur[len])
            break;
        if (maxLen < len)
        {
          maxLen = len;
          *d++ = len;
          *d++ = delta - 1;
          if (len == lenLimit)
            return d;
        }
      }
    }
  }
  */

  const Byte *lim = cur + lenLimit;
  son[_cyclicBufferPos] = curMatch;

  do
  {
    UInt32 delta;

    if (curMatch == 0)
      break;
    // if (curMatch2 >= curMatch) return NULL;
    delta = pos - curMatch;
    if (delta >= _cyclicBufferSize)
      break;
    {
      ptrdiff_t diff;
      curMatch = son[_cyclicBufferPos - delta + (_cyclicBufferPos < delta ? _cyclicBufferSize : 0)];
      diff = (ptrdiff_t)0 - (ptrdiff_t)delta;
      if (cur[maxLen] == cur[(ptrdiff_t)maxLen + diff])
      {
        const Byte *c = cur;
        while (*c == c[diff])
        {
          if (++c == lim)
          {
            d[0] = (UInt32)(lim - cur);
            d[1] = delta - 1;
            return d + 2;
          }
        }
        {
          const unsigned len = (unsigned)(c - cur);
          if (maxLen < len)
          {
            maxLen = len;
            d[0] = (UInt32)len;
            d[1] = delta - 1;
            d += 2;
          }
        }
      }
    }
  }
  while (--cutValue);
  
  return d;
}


Z7_FORCE_INLINE
UInt32 * GetMatchesSpec1(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *cur, CLzRef *son,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 cutValue,
    UInt32 *d, UInt32 maxLen)
{
  CLzRef *ptr0 = son + ((size_t)_cyclicBufferPos << 1) + 1;
  CLzRef *ptr1 = son + ((size_t)_cyclicBufferPos << 1);
  unsigned len0 = 0, len1 = 0;

  UInt32 cmCheck;

  // if (curMatch >= pos) { *ptr0 = *ptr1 = kEmptyHashValue; return NULL; }

  cmCheck = (UInt32)(pos - _cyclicBufferSize);
  if ((UInt32)pos < _cyclicBufferSize)
    cmCheck = 0;

  if (cmCheck < curMatch)
  do
  {
    const UInt32 delta = pos - curMatch;
    {
      CLzRef *pair = son + ((size_t)(_cyclicBufferPos - delta + (_cyclicBufferPos < delta ? _cyclicBufferSize : 0)) << 1);
      const Byte *pb = cur - delta;
      unsigned len = (len0 < len1 ? len0 : len1);
      const UInt32 pair0 = pair[0];
      if (pb[len] == cur[len])
      {
        if (++len != lenLimit && pb[len] == cur[len])
          while (++len != lenLimit)
            if (pb[len] != cur[len])
              break;
        if (maxLen < len)
        {
          maxLen = (UInt32)len;
          *d++ = (UInt32)len;
          *d++ = delta - 1;
          if (len == lenLimit)
          {
            *ptr1 = pair0;
            *ptr0 = pair[1];
            return d;
          }
        }
      }
      if (pb[len] < cur[len])
      {
        *ptr1 = curMatch;
        // const UInt32 curMatch2 = pair[1];
        // if (curMatch2 >= curMatch) { *ptr0 = *ptr1 = kEmptyHashValue;  return NULL; }
        // curMatch = curMatch2;
        curMatch = pair[1];
        ptr1 = pair + 1;
        len1 = len;
      }
      else
      {
        *ptr0 = curMatch;
        curMatch = pair[0];
        ptr0 = pair;
        len0 = len;
      }
    }
  }
  while(--cutValue && cmCheck < curMatch);

  *ptr0 = *ptr1 = kEmptyHashValue;
  return d;
}


static void SkipMatchesSpec(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *cur, CLzRef *son,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 cutValue)
{
  CLzRef *ptr0 = son + ((size_t)_cyclicBufferPos << 1) + 1;
  CLzRef *ptr1 = son + ((size_t)_cyclicBufferPos << 1);
  unsigned len0 = 0, len1 = 0;

  UInt32 cmCheck;

  cmCheck = (UInt32)(pos - _cyclicBufferSize);
  if ((UInt32)pos < _cyclicBufferSize)
    cmCheck = 0;

  if (// curMatch >= pos ||  // failure
      cmCheck < curMatch)
  do
  {
    const UInt32 delta = pos - curMatch;
    {
      CLzRef *pair = son + ((size_t)(_cyclicBufferPos - delta + (_cyclicBufferPos < delta ? _cyclicBufferSize : 0)) << 1);
      const Byte *pb = cur - delta;
      unsigned len = (len0 < len1 ? len0 : len1);
      if (pb[len] == cur[len])
      {
        while (++len != lenLimit)
          if (pb[len] != cur[len])
            break;
        {
          if (len == lenLimit)
          {
            *ptr1 = pair[0];
            *ptr0 = pair[1];
            return;
          }
        }
      }
      if (pb[len] < cur[len])
      {
        *ptr1 = curMatch;
        curMatch = pair[1];
        ptr1 = pair + 1;
        len1 = len;
      }
      else
      {
        *ptr0 = curMatch;
        curMatch = pair[0];
        ptr0 = pair;
        len0 = len;
      }
    }
  }
  while(--cutValue && cmCheck < curMatch);
  
  *ptr0 = *ptr1 = kEmptyHashValue;
  return;
}


#define MOVE_POS \
  p->cyclicBufferPos++; \
  p->buffer++; \
  { const UInt32 pos1 = p->pos + 1; \
    p->pos = pos1; \
    if (pos1 == p->posLimit) MatchFinder_CheckLimits(p); }

#define MOVE_POS_RET MOVE_POS return distances;

Z7_NO_INLINE
static void MatchFinder_MovePos(CMatchFinder *p)
{
  /* we go here at the end of stream data, when (avail < num_hash_bytes)
     We don't update sons[cyclicBufferPos << btMode].
     So (sons) record will contain junk. And we cannot resume match searching
     to normal operation, even if we will provide more input data in buffer.
     p->sons[p->cyclicBufferPos << p->btMode] = 0;  // kEmptyHashValue
     if (p->btMode)
        p->sons[(p->cyclicBufferPos << p->btMode) + 1] = 0;  // kEmptyHashValue
  */
  MOVE_POS
}

#define GET_MATCHES_HEADER2(minLen, ret_op) \
  UInt32 hv; const Byte *cur; UInt32 curMatch; \
  UInt32 lenLimit = p->lenLimit; \
  if (lenLimit < minLen) { MatchFinder_MovePos(p);  ret_op; } \
  cur = p->buffer;

#define GET_MATCHES_HEADER(minLen) GET_MATCHES_HEADER2(minLen, return distances)
#define SKIP_HEADER(minLen)  \
  do { GET_MATCHES_HEADER2(minLen, continue)

#define MF_PARAMS(p)  lenLimit, curMatch, p->pos, p->buffer, p->son, \
    p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue

#define SKIP_FOOTER  \
    SkipMatchesSpec(MF_PARAMS(p)); \
    MOVE_POS \
  } while (--num);

#define GET_MATCHES_FOOTER_BASE(_maxLen_, func) \
  distances = func(MF_PARAMS(p), distances, (UInt32)_maxLen_); \
  MOVE_POS_RET

#define GET_MATCHES_FOOTER_BT(_maxLen_) \
  GET_MATCHES_FOOTER_BASE(_maxLen_, GetMatchesSpec1)

#define GET_MATCHES_FOOTER_HC(_maxLen_) \
  GET_MATCHES_FOOTER_BASE(_maxLen_, Hc_GetMatchesSpec)



#define UPDATE_maxLen { \
    const ptrdiff_t diff = (ptrdiff_t)0 - (ptrdiff_t)d2; \
    const Byte *c = cur + maxLen; \
    const Byte *lim = cur + lenLimit; \
    for (; c != lim; c++) if (*(c + diff) != *c) break; \
    maxLen = (unsigned)(c - cur); }

static UInt32* Bt2_MatchFinder_GetMatches(void *_p, UInt32 *distances)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  GET_MATCHES_HEADER(2)
  HASH2_CALC
  curMatch = p->hash[hv];
  p->hash[hv] = p->pos;
  GET_MATCHES_FOOTER_BT(1)
}

UInt32* Bt3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  GET_MATCHES_HEADER(3)
  HASH_ZIP_CALC
  curMatch = p->hash[hv];
  p->hash[hv] = p->pos;
  GET_MATCHES_FOOTER_BT(2)
}


#define SET_mmm  \
  mmm = p->cyclicBufferSize; \
  if (pos < mmm) \
    mmm = pos;


static UInt32* Bt3_MatchFinder_GetMatches(void *_p, UInt32 *distances)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  UInt32 mmm;
  UInt32 h2, d2, pos;
  unsigned maxLen;
  UInt32 *hash;
  GET_MATCHES_HEADER(3)

  HASH3_CALC

  hash = p->hash;
  pos = p->pos;

  d2 = pos - hash[h2];

  curMatch = (hash + kFix3HashSize)[hv];
  
  hash[h2] = pos;
  (hash + kFix3HashSize)[hv] = pos;

  SET_mmm

  maxLen = 2;

  if (d2 < mmm && *(cur - d2) == *cur)
  {
    UPDATE_maxLen
    distances[0] = (UInt32)maxLen;
    distances[1] = d2 - 1;
    distances += 2;
    if (maxLen == lenLimit)
    {
      SkipMatchesSpec(MF_PARAMS(p));
      MOVE_POS_RET
    }
  }
  
  GET_MATCHES_FOOTER_BT(maxLen)
}


static UInt32* Bt4_MatchFinder_GetMatches(void *_p, UInt32 *distances)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  UInt32 mmm;
  UInt32 h2, h3, d2, d3, pos;
  unsigned maxLen;
  UInt32 *hash;
  GET_MATCHES_HEADER(4)

  HASH4_CALC

  hash = p->hash;
  pos = p->pos;

  d2 = pos - hash                  [h2];
  d3 = pos - (hash + kFix3HashSize)[h3];
  curMatch = (hash + kFix4HashSize)[hv];

  hash                  [h2] = pos;
  (hash + kFix3HashSize)[h3] = pos;
  (hash + kFix4HashSize)[hv] = pos;

  SET_mmm

  maxLen = 3;
  
  for (;;)
  {
    if (d2 < mmm && *(cur - d2) == *cur)
    {
      distances[0] = 2;
      distances[1] = d2 - 1;
      distances += 2;
      if (*(cur - d2 + 2) == cur[2])
      {
        // distances[-2] = 3;
      }
      else if (d3 < mmm && *(cur - d3) == *cur)
      {
        d2 = d3;
        distances[1] = d3 - 1;
        distances += 2;
      }
      else
        break;
    }
    else if (d3 < mmm && *(cur - d3) == *cur)
    {
      d2 = d3;
      distances[1] = d3 - 1;
      distances += 2;
    }
    else
      break;
  
    UPDATE_maxLen
    distances[-2] = (UInt32)maxLen;
    if (maxLen == lenLimit)
    {
      SkipMatchesSpec(MF_PARAMS(p));
      MOVE_POS_RET
    }
    break;
  }
  
  GET_MATCHES_FOOTER_BT(maxLen)
}


static UInt32* Bt5_MatchFinder_GetMatches(void *_p, UInt32 *distances)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  UInt32 mmm;
  UInt32 h2, h3, d2, d3, pos;
  unsigned maxLen;
  UInt32 *hash;
  GET_MATCHES_HEADER(5)

  HASH5_CALC

  hash = p->hash;
  pos = p->pos;

  d2 = pos - hash                  [h2];
  d3 = pos - (hash + kFix3HashSize)[h3];
  // d4 = pos - (hash + kFix4HashSize)[h4];

  curMatch = (hash + kFix5HashSize)[hv];

  hash                  [h2] = pos;
  (hash + kFix3HashSize)[h3] = pos;
  // (hash + kFix4HashSize)[h4] = pos;
  (hash + kFix5HashSize)[hv] = pos;

  SET_mmm

  maxLen = 4;

  for (;;)
  {
    if (d2 < mmm && *(cur - d2) == *cur)
    {
      distances[0] = 2;
      distances[1] = d2 - 1;
      distances += 2;
      if (*(cur - d2 + 2) == cur[2])
      {
      }
      else if (d3 < mmm && *(cur - d3) == *cur)
      {
        distances[1] = d3 - 1;
        distances += 2;
        d2 = d3;
      }
      else
        break;
    }
    else if (d3 < mmm && *(cur - d3) == *cur)
    {
      distances[1] = d3 - 1;
      distances += 2;
      d2 = d3;
    }
    else
      break;

    distances[-2] = 3;
    if (*(cur - d2 + 3) != cur[3])
      break;
    UPDATE_maxLen
    distances[-2] = (UInt32)maxLen;
    if (maxLen == lenLimit)
    {
      SkipMatchesSpec(MF_PARAMS(p));
      MOVE_POS_RET
    }
    break;
  }
  
  GET_MATCHES_FOOTER_BT(maxLen)
}


static UInt32* Hc4_MatchFinder_GetMatches(void *_p, UInt32 *distances)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  UInt32 mmm;
  UInt32 h2, h3, d2, d3, pos;
  unsigned maxLen;
  UInt32 *hash;
  GET_MATCHES_HEADER(4)

  HASH4_CALC

  hash = p->hash;
  pos = p->pos;
  
  d2 = pos - hash                  [h2];
  d3 = pos - (hash + kFix3HashSize)[h3];
  curMatch = (hash + kFix4HashSize)[hv];

  hash                  [h2] = pos;
  (hash + kFix3HashSize)[h3] = pos;
  (hash + kFix4HashSize)[hv] = pos;

  SET_mmm

  maxLen = 3;

  for (;;)
  {
    if (d2 < mmm && *(cur - d2) == *cur)
    {
      distances[0] = 2;
      distances[1] = d2 - 1;
      distances += 2;
      if (*(cur - d2 + 2) == cur[2])
      {
        // distances[-2] = 3;
      }
      else if (d3 < mmm && *(cur - d3) == *cur)
      {
        d2 = d3;
        distances[1] = d3 - 1;
        distances += 2;
      }
      else
        break;
    }
    else if (d3 < mmm && *(cur - d3) == *cur)
    {
      d2 = d3;
      distances[1] = d3 - 1;
      distances += 2;
    }
    else
      break;

    UPDATE_maxLen
    distances[-2] = (UInt32)maxLen;
    if (maxLen == lenLimit)
    {
      p->son[p->cyclicBufferPos] = curMatch;
      MOVE_POS_RET
    }
    break;
  }
  
  GET_MATCHES_FOOTER_HC(maxLen)
}


static UInt32 * Hc5_MatchFinder_GetMatches(void *_p, UInt32 *distances)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  UInt32 mmm;
  UInt32 h2, h3, d2, d3, pos;
  unsigned maxLen;
  UInt32 *hash;
  GET_MATCHES_HEADER(5)

  HASH5_CALC

  hash = p->hash;
  pos = p->pos;

  d2 = pos - hash                  [h2];
  d3 = pos - (hash + kFix3HashSize)[h3];
  // d4 = pos - (hash + kFix4HashSize)[h4];

  curMatch = (hash + kFix5HashSize)[hv];

  hash                  [h2] = pos;
  (hash + kFix3HashSize)[h3] = pos;
  // (hash + kFix4HashSize)[h4] = pos;
  (hash + kFix5HashSize)[hv] = pos;

  SET_mmm
  
  maxLen = 4;

  for (;;)
  {
    if (d2 < mmm && *(cur - d2) == *cur)
    {
      distances[0] = 2;
      distances[1] = d2 - 1;
      distances += 2;
      if (*(cur - d2 + 2) == cur[2])
      {
      }
      else if (d3 < mmm && *(cur - d3) == *cur)
      {
        distances[1] = d3 - 1;
        distances += 2;
        d2 = d3;
      }
      else
        break;
    }
    else if (d3 < mmm && *(cur - d3) == *cur)
    {
      distances[1] = d3 - 1;
      distances += 2;
      d2 = d3;
    }
    else
      break;

    distances[-2] = 3;
    if (*(cur - d2 + 3) != cur[3])
      break;
    UPDATE_maxLen
    distances[-2] = (UInt32)maxLen;
    if (maxLen == lenLimit)
    {
      p->son[p->cyclicBufferPos] = curMatch;
      MOVE_POS_RET
    }
    break;
  }
  
  GET_MATCHES_FOOTER_HC(maxLen)
}


UInt32* Hc3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  GET_MATCHES_HEADER(3)
  HASH_ZIP_CALC
  curMatch = p->hash[hv];
  p->hash[hv] = p->pos;
  GET_MATCHES_FOOTER_HC(2)
}


static void Bt2_MatchFinder_Skip(void *_p, UInt32 num)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  SKIP_HEADER(2)
  {
    HASH2_CALC
    curMatch = p->hash[hv];
    p->hash[hv] = p->pos;
  }
  SKIP_FOOTER
}

void Bt3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  SKIP_HEADER(3)
  {
    HASH_ZIP_CALC
    curMatch = p->hash[hv];
    p->hash[hv] = p->pos;
  }
  SKIP_FOOTER
}

static void Bt3_MatchFinder_Skip(void *_p, UInt32 num)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  SKIP_HEADER(3)
  {
    UInt32 h2;
    UInt32 *hash;
    HASH3_CALC
    hash = p->hash;
    curMatch = (hash + kFix3HashSize)[hv];
    hash[h2] =
    (hash + kFix3HashSize)[hv] = p->pos;
  }
  SKIP_FOOTER
}

static void Bt4_MatchFinder_Skip(void *_p, UInt32 num)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  SKIP_HEADER(4)
  {
    UInt32 h2, h3;
    UInt32 *hash;
    HASH4_CALC
    hash = p->hash;
    curMatch = (hash + kFix4HashSize)[hv];
    hash                  [h2] =
    (hash + kFix3HashSize)[h3] =
    (hash + kFix4HashSize)[hv] = p->pos;
  }
  SKIP_FOOTER
}

static void Bt5_MatchFinder_Skip(void *_p, UInt32 num)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  SKIP_HEADER(5)
  {
    UInt32 h2, h3;
    UInt32 *hash;
    HASH5_CALC
    hash = p->hash;
    curMatch = (hash + kFix5HashSize)[hv];
    hash                  [h2] =
    (hash + kFix3HashSize)[h3] =
    // (hash + kFix4HashSize)[h4] =
    (hash + kFix5HashSize)[hv] = p->pos;
  }
  SKIP_FOOTER
}


#define HC_SKIP_HEADER(minLen) \
    do { if (p->lenLimit < minLen) { MatchFinder_MovePos(p); num--; continue; } { \
    const Byte *cur; \
    UInt32 *hash; \
    UInt32 *son; \
    UInt32 pos = p->pos; \
    UInt32 num2 = num; \
    /* (p->pos == p->posLimit) is not allowed here !!! */ \
    { const UInt32 rem = p->posLimit - pos; if (num2 >= rem) num2 = rem; } \
    num -= num2; \
    { const UInt32 cycPos = p->cyclicBufferPos; \
      son = p->son + cycPos; \
      p->cyclicBufferPos = cycPos + num2; } \
    cur = p->buffer; \
    hash = p->hash; \
    do { \
    UInt32 curMatch; \
    UInt32 hv;


#define HC_SKIP_FOOTER \
    cur++;  pos++;  *son++ = curMatch; \
    } while (--num2); \
    p->buffer = cur; \
    p->pos = pos; \
    if (pos == p->posLimit) MatchFinder_CheckLimits(p); \
    }} while(num); \


static void Hc4_MatchFinder_Skip(void *_p, UInt32 num)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  HC_SKIP_HEADER(4)

    UInt32 h2, h3;
    HASH4_CALC
    curMatch = (hash + kFix4HashSize)[hv];
    hash                  [h2] =
    (hash + kFix3HashSize)[h3] =
    (hash + kFix4HashSize)[hv] = pos;
  
  HC_SKIP_FOOTER
}


static void Hc5_MatchFinder_Skip(void *_p, UInt32 num)
{
  CMatchFinder *p = (CMatchFinder *)_p;
  HC_SKIP_HEADER(5)
  
    UInt32 h2, h3;
    HASH5_CALC
    curMatch = (hash + kFix5HashSize)[hv];
    hash                  [h2] =
    (hash + kFix3HashSize)[h3] =
    // (hash + kFix4HashSize)[h4] =
    (hash + kFix5HashSize)[hv] = pos;
  
  HC_SKIP_FOOTER
}


void Hc3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  HC_SKIP_HEADER(3)

    HASH_ZIP_CALC
    curMatch = hash[hv];
    hash[hv] = pos;

  HC_SKIP_FOOTER
}


void MatchFinder_CreateVTable(CMatchFinder *p, IMatchFinder2 *vTable)
{
  vTable->Init = MatchFinder_Init;
  vTable->GetNumAvailableBytes = MatchFinder_GetNumAvailableBytes;
  vTable->GetPointerToCurrentPos = MatchFinder_GetPointerToCurrentPos;
  if (!p->btMode)
  {
    if (p->numHashBytes <= 4)
    {
      vTable->GetMatches = Hc4_MatchFinder_GetMatches;
      vTable->Skip = Hc4_MatchFinder_Skip;
    }
    else
    {
      vTable->GetMatches = Hc5_MatchFinder_GetMatches;
      vTable->Skip = Hc5_MatchFinder_Skip;
    }
  }
  else if (p->numHashBytes == 2)
  {
    vTable->GetMatches = Bt2_MatchFinder_GetMatches;
    vTable->Skip = Bt2_MatchFinder_Skip;
  }
  else if (p->numHashBytes == 3)
  {
    vTable->GetMatches = Bt3_MatchFinder_GetMatches;
    vTable->Skip = Bt3_MatchFinder_Skip;
  }
  else if (p->numHashBytes == 4)
  {
    vTable->GetMatches = Bt4_MatchFinder_GetMatches;
    vTable->Skip = Bt4_MatchFinder_Skip;
  }
  else
  {
    vTable->GetMatches = Bt5_MatchFinder_GetMatches;
    vTable->Skip = Bt5_MatchFinder_Skip;
  }
}



void LzFindPrepare(void)
{
  #ifndef FORCE_LZFIND_SATUR_SUB_128
  #ifdef USE_LZFIND_SATUR_SUB_128
  LZFIND_SATUR_SUB_CODE_FUNC f = NULL;
  #ifdef MY_CPU_ARM_OR_ARM64
  {
    if (CPU_IsSupported_NEON())
    {
      // #pragma message ("=== LzFind NEON")
      PRF(printf("\n=== LzFind NEON\n"));
      f = LzFind_SaturSub_128;
    }
    // f = 0; // for debug
  }
  #else // MY_CPU_ARM_OR_ARM64
  if (CPU_IsSupported_SSE41())
  {
    // #pragma message ("=== LzFind SSE41")
    PRF(printf("\n=== LzFind SSE41\n"));
    f = LzFind_SaturSub_128;

    #ifdef USE_LZFIND_SATUR_SUB_256
    if (CPU_IsSupported_AVX2())
    {
      // #pragma message ("=== LzFind AVX2")
      PRF(printf("\n=== LzFind AVX2\n"));
      f = LzFind_SaturSub_256;
    }
    #endif
  }
  #endif // MY_CPU_ARM_OR_ARM64
  g_LzFind_SaturSub = f;
  #endif // USE_LZFIND_SATUR_SUB_128
  #endif // FORCE_LZFIND_SATUR_SUB_128
}


#undef MOVE_POS
#undef MOVE_POS_RET
#undef PRF

/* ================ unit: C/LzFindMt.c ================ */
/* LzFindMt.c -- multithreaded Match finder for LZ algorithms
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// #define LOG_ITERS

// #define LOG_THREAD

#ifdef LOG_THREAD
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

#ifdef LOG_ITERS
#include <stdio.h>
extern UInt64 g_NumIters_Tree;
extern UInt64 g_NumIters_Loop;
extern UInt64 g_NumIters_Bytes;
#define LOG_ITER(x) x
#else
#define LOG_ITER(x)
#endif

#define kMtHashBlockSize ((UInt32)1 << 17)
#define kMtHashNumBlocks (1 << 1)

#define GET_HASH_BLOCK_OFFSET(i)  (((i) & (kMtHashNumBlocks - 1)) * kMtHashBlockSize)

#define kMtBtBlockSize ((UInt32)1 << 16)
#define kMtBtNumBlocks (1 << 4)

#define GET_BT_BLOCK_OFFSET(i)  (((i) & (kMtBtNumBlocks - 1)) * (size_t)kMtBtBlockSize)

/*
  HASH functions:
  We use raw 8/16 bits from a[1] and a[2],
  xored with crc(a[0]) and crc(a[3]).
  We check a[0], a[3] only. We don't need to compare a[1] and a[2] in matches.
  our crc() function provides one-to-one correspondence for low 8-bit values:
    (crc[0...0xFF] & 0xFF) <-> [0...0xFF]
*/

#define MF(mt) ((mt)->MatchFinder)
#define MF_CRC (p->crc)

// #define MF(mt) (&(mt)->MatchFinder)
// #define MF_CRC (p->MatchFinder.crc)

#define MT_HASH2_CALC \
  h2 = (MF_CRC[cur[0]] ^ cur[1]) & (kHash2Size - 1);

#define MT_HASH3_CALC { \
  UInt32 temp = MF_CRC[cur[0]] ^ cur[1]; \
  h2 = temp & (kHash2Size - 1); \
  h3 = (temp ^ ((UInt32)cur[2] << 8)) & (kHash3Size - 1); }

/*
#define MT_HASH3_CALC__NO_2 { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  h3 = (temp ^ ((UInt32)cur[2] << 8)) & (kHash3Size - 1); }

#define MT_HASH4_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  h2 = temp & (kHash2Size - 1); \
  temp ^= ((UInt32)cur[2] << 8); \
  h3 = temp & (kHash3Size - 1); \
  h4 = (temp ^ (p->crc[cur[3]] << kLzHash_CrcShift_1)) & p->hash4Mask; }
  // (kHash4Size - 1);
*/


Z7_NO_INLINE
static void MtSync_Construct(CMtSync *p)
{
  p->affinityGroup = -1;
  p->affinityInGroup = 0;
  p->affinity = 0;
  p->wasCreated = False;
  p->csWasInitialized = False;
  p->csWasEntered = False;
  Thread_CONSTRUCT(&p->thread)
  Event_Construct(&p->canStart);
  Event_Construct(&p->wasStopped);
  Semaphore_Construct(&p->freeSemaphore);
  Semaphore_Construct(&p->filledSemaphore);
}


// #define DEBUG_BUFFER_LOCK   // define it to debug lock state

#ifdef DEBUG_BUFFER_LOCK
#include <stdlib.h>
#define BUFFER_MUST_BE_LOCKED(p)    if (!(p)->csWasEntered) exit(1);
#define BUFFER_MUST_BE_UNLOCKED(p)  if ( (p)->csWasEntered) exit(1);
#else
#define BUFFER_MUST_BE_LOCKED(p)
#define BUFFER_MUST_BE_UNLOCKED(p)
#endif

#define LOCK_BUFFER(p) { \
    BUFFER_MUST_BE_UNLOCKED(p); \
    CriticalSection_Enter(&(p)->cs); \
    (p)->csWasEntered = True; }

#define UNLOCK_BUFFER(p) { \
    BUFFER_MUST_BE_LOCKED(p); \
    CriticalSection_Leave(&(p)->cs); \
    (p)->csWasEntered = False; }


Z7_NO_INLINE
static UInt32 MtSync_GetNextBlock(CMtSync *p)
{
  UInt32 numBlocks = 0;
  if (p->needStart)
  {
    BUFFER_MUST_BE_UNLOCKED(p)
    p->numProcessedBlocks = 1;
    p->needStart = False;
    p->stopWriting = False;
    p->exit = False;
    Event_Reset(&p->wasStopped);
    Event_Set(&p->canStart);
  }
  else
  {
    UNLOCK_BUFFER(p)
    // we free current block
    numBlocks = p->numProcessedBlocks++;
    Semaphore_Release1(&p->freeSemaphore);
  }

  // buffer is UNLOCKED here
  Semaphore_Wait(&p->filledSemaphore);
  LOCK_BUFFER(p)
  return numBlocks;
}


/* if Writing (Processing) thread was started, we must call MtSync_StopWriting() */

Z7_NO_INLINE
static void MtSync_StopWriting(CMtSync *p)
{
  if (!Thread_WasCreated(&p->thread) || p->needStart)
    return;

    PRF(printf("\nMtSync_StopWriting %p\n", p));

  if (p->csWasEntered)
  {
    /* we don't use buffer in this thread after StopWriting().
       So we UNLOCK buffer.
       And we restore default UNLOCKED state for stopped thread */
    UNLOCK_BUFFER(p)
  }

  /* We send (p->stopWriting) message and release freeSemaphore
     to free current block.
     So the thread will see (p->stopWriting) at some
     iteration after Wait(freeSemaphore).
     The thread doesn't need to fill all avail free blocks,
     so we can get fast thread stop.
  */

  p->stopWriting = True;
  Semaphore_Release1(&p->freeSemaphore); // check semaphore count !!!

    PRF(printf("\nMtSync_StopWriting %p : Event_Wait(&p->wasStopped)\n", p));
  Event_Wait(&p->wasStopped);
    PRF(printf("\nMtSync_StopWriting %p : Event_Wait() finsihed\n", p));

  /* 21.03 : we don't restore samaphore counters here.
     We will recreate and reinit samaphores in next start */

  p->needStart = True;
}


Z7_NO_INLINE
static void MtSync_Destruct(CMtSync *p)
{
    PRF(printf("\nMtSync_Destruct %p\n", p));
  
  if (Thread_WasCreated(&p->thread))
  {
    /* we want thread to be in Stopped state before sending EXIT command.
       note: stop(btSync) will stop (htSync) also */
    MtSync_StopWriting(p);
    /* thread in Stopped state here : (p->needStart == true) */
    p->exit = True;
    // if (p->needStart)  // it's (true)
    Event_Set(&p->canStart);  // we send EXIT command to thread
    Thread_Wait_Close(&p->thread);  // we wait thread finishing
  }

  if (p->csWasInitialized)
  {
    CriticalSection_Delete(&p->cs);
    p->csWasInitialized = False;
  }
  p->csWasEntered = False;

  Event_Close(&p->canStart);
  Event_Close(&p->wasStopped);
  Semaphore_Close(&p->freeSemaphore);
  Semaphore_Close(&p->filledSemaphore);

  p->wasCreated = False;
}


// #define RINOK_THREAD(x) { if ((x) != 0) return SZ_ERROR_THREAD; }
// we want to get real system error codes here instead of SZ_ERROR_THREAD
#define RINOK_THREAD(x)  RINOK_WRes(x)


// call it before each new file (when new starting is required):
Z7_NO_INLINE
static SRes MtSync_Init(CMtSync *p, UInt32 numBlocks)
{
  WRes wres;
  // BUFFER_MUST_BE_UNLOCKED(p)
  if (!p->needStart || p->csWasEntered)
    return SZ_ERROR_FAIL;
  wres = Semaphore_OptCreateInit(&p->freeSemaphore, numBlocks, numBlocks);
  if (wres == 0)
    wres = Semaphore_OptCreateInit(&p->filledSemaphore, 0, numBlocks);
  return MY_SRes_HRESULT_FROM_WRes(wres);
}


static WRes MtSync_Create_WRes(CMtSync *p, THREAD_FUNC_TYPE startAddress, void *obj)
{
  WRes wres;

  if (p->wasCreated)
    return SZ_OK;

  RINOK_THREAD(CriticalSection_Init(&p->cs))
  p->csWasInitialized = True;
  p->csWasEntered = False;

  RINOK_THREAD(AutoResetEvent_CreateNotSignaled(&p->canStart))
  RINOK_THREAD(AutoResetEvent_CreateNotSignaled(&p->wasStopped))

  p->needStart = True;
  p->exit = True;  /* p->exit is unused before (canStart) Event.
     But in case of some unexpected code failure we will get fast exit from thread */

  // return ERROR_TOO_MANY_POSTS; // for debug
  // return EINVAL; // for debug

#ifdef _WIN32
  if (p->affinityGroup >= 0)
    wres = Thread_Create_With_Group(&p->thread, startAddress, obj,
        (unsigned)(UInt32)p->affinityGroup, (CAffinityMask)p->affinityInGroup);
  else
#endif
  if (p->affinity != 0)
    wres = Thread_Create_With_Affinity(&p->thread, startAddress, obj, (CAffinityMask)p->affinity);
  else
    wres = Thread_Create(&p->thread, startAddress, obj);

  RINOK_THREAD(wres)
  p->wasCreated = True;
  return SZ_OK;
}


Z7_NO_INLINE
static SRes MtSync_Create(CMtSync *p, THREAD_FUNC_TYPE startAddress, void *obj)
{
  const WRes wres = MtSync_Create_WRes(p, startAddress, obj);
  if (wres == 0)
    return 0;
  MtSync_Destruct(p);
  return MY_SRes_HRESULT_FROM_WRes(wres);
}


// ---------- HASH THREAD ----------

#define kMtMaxValForNormalize 0xFFFFFFFF
// #define kMtMaxValForNormalize ((1 << 21)) // for debug
// #define kNormalizeAlign (1 << 7) // alignment for speculated accesses

#ifdef MY_CPU_LE_UNALIGN
  #define GetUi24hi_from32(p) ((UInt32)GetUi32(p) >> 8)
#else
  #define GetUi24hi_from32(p) ((p)[1] ^ ((UInt32)(p)[2] << 8) ^ ((UInt32)(p)[3] << 16))
#endif

#define GetHeads_DECL(name) \
    static void GetHeads ## name(const Byte *p, UInt32 pos, \
      UInt32 *hash, UInt32 hashMask, UInt32 *heads, UInt32 numHeads, const UInt32 *crc)

#define GetHeads_LOOP(v) \
    for (; numHeads != 0; numHeads--) { \
      const UInt32 value = (v); \
      p++; \
      *heads++ = pos - hash[value]; \
      hash[value] = pos++; }

#define DEF_GetHeads2(name, v, action) \
    GetHeads_DECL(name) { action \
    GetHeads_LOOP(v) }
 
#define DEF_GetHeads(name, v) DEF_GetHeads2(name, v, ;)

DEF_GetHeads2(2, GetUi16(p), UNUSED_VAR(hashMask); UNUSED_VAR(crc); )
DEF_GetHeads(3,  (crc[p[0]] ^ GetUi16(p + 1)) & hashMask)
DEF_GetHeads2(3b, GetUi16(p) ^ ((UInt32)(p)[2] << 16), UNUSED_VAR(hashMask); UNUSED_VAR(crc); )
// BT3 is not good for crc collisions for big hashMask values.

/*
GetHeads_DECL(3b)
{
  UNUSED_VAR(hashMask);
  UNUSED_VAR(crc);
  {
  const Byte *pLim = p + numHeads;
  if (numHeads == 0)
    return;
  pLim--;
  while (p < pLim)
  {
    UInt32 v1 = GetUi32(p);
    UInt32 v0 = v1 & 0xFFFFFF;
    UInt32 h0, h1;
    p += 2;
    v1 >>= 8;
    h0 = hash[v0]; hash[v0] = pos; heads[0] = pos - h0; pos++;
    h1 = hash[v1]; hash[v1] = pos; heads[1] = pos - h1; pos++;
    heads += 2;
  }
  if (p == pLim)
  {
    UInt32 v0 = GetUi16(p) ^ ((UInt32)(p)[2] << 16);
    *heads = pos - hash[v0];
    hash[v0] = pos;
  }
  }
}
*/

/*
GetHeads_DECL(4)
{
  unsigned sh = 0;
  UNUSED_VAR(crc)
  while ((hashMask & 0x80000000) == 0)
  {
    hashMask <<= 1;
    sh++;
  }
  GetHeads_LOOP((GetUi32(p) * 0xa54a1) >> sh)
}
#define GetHeads4b GetHeads4
*/

#define USE_GetHeads_LOCAL_CRC

#ifdef USE_GetHeads_LOCAL_CRC

GetHeads_DECL(4)
{
  UInt32 crc0[256];
  UInt32 crc1[256];
  {
    unsigned i;
    for (i = 0; i < 256; i++)
    {
      UInt32 v = crc[i];
      crc0[i] = v & hashMask;
      crc1[i] = (v << kLzHash_CrcShift_1) & hashMask;
      // crc1[i] = rotlFixed(v, 8) & hashMask;
    }
  }
  GetHeads_LOOP(crc0[p[0]] ^ crc1[p[3]] ^ (UInt32)GetUi16(p+1))
}

GetHeads_DECL(4b)
{
  UInt32 crc0[256];
  {
    unsigned i;
    for (i = 0; i < 256; i++)
      crc0[i] = crc[i] & hashMask;
  }
  GetHeads_LOOP(crc0[p[0]] ^ GetUi24hi_from32(p))
}

GetHeads_DECL(5)
{
  UInt32 crc0[256];
  UInt32 crc1[256];
  UInt32 crc2[256];
  {
    unsigned i;
    for (i = 0; i < 256; i++)
    {
      UInt32 v = crc[i];
      crc0[i] = v & hashMask;
      crc1[i] = (v << kLzHash_CrcShift_1) & hashMask;
      crc2[i] = (v << kLzHash_CrcShift_2) & hashMask;
    }
  }
  GetHeads_LOOP(crc0[p[0]] ^ crc1[p[3]] ^ crc2[p[4]] ^ (UInt32)GetUi16(p+1))
}

GetHeads_DECL(5b)
{
  UInt32 crc0[256];
  UInt32 crc1[256];
  {
    unsigned i;
    for (i = 0; i < 256; i++)
    {
      UInt32 v = crc[i];
      crc0[i] = v & hashMask;
      crc1[i] = (v << kLzHash_CrcShift_1) & hashMask;
    }
  }
  GetHeads_LOOP(crc0[p[0]] ^ crc1[p[4]] ^ GetUi24hi_from32(p))
}

#else

DEF_GetHeads(4,  (crc[p[0]] ^ (crc[p[3]] << kLzHash_CrcShift_1) ^ (UInt32)GetUi16(p+1)) & hashMask)
DEF_GetHeads(4b, (crc[p[0]] ^ GetUi24hi_from32(p)) & hashMask)
DEF_GetHeads(5,  (crc[p[0]] ^ (crc[p[3]] << kLzHash_CrcShift_1) ^ (crc[p[4]] << kLzHash_CrcShift_2) ^ (UInt32)GetUi16(p + 1)) & hashMask)
DEF_GetHeads(5b, (crc[p[0]] ^ (crc[p[4]] << kLzHash_CrcShift_1) ^ GetUi24hi_from32(p)) & hashMask)

#endif
 

static void HashThreadFunc(CMatchFinderMt *mt)
{
  CMtSync *p = &mt->hashSync;
    PRF(printf("\nHashThreadFunc\n"));
  
  for (;;)
  {
    UInt32 blockIndex = 0;
      PRF(printf("\nHashThreadFunc : Event_Wait(&p->canStart)\n"));
    Event_Wait(&p->canStart);
      PRF(printf("\nHashThreadFunc : Event_Wait(&p->canStart) : after \n"));
    if (p->exit)
    {
      PRF(printf("\nHashThreadFunc : exit \n"));
      return;
    }

    MatchFinder_Init_HighHash(MF(mt));

    for (;;)
    {
      PRF(printf("Hash thread block = %d pos = %d\n", (unsigned)blockIndex, mt->MatchFinder->pos));

      {
        CMatchFinder *mf = MF(mt);
        if (MatchFinder_NeedMove(mf))
        {
          CriticalSection_Enter(&mt->btSync.cs);
          CriticalSection_Enter(&mt->hashSync.cs);
          {
            const Byte *beforePtr = Inline_MatchFinder_GetPointerToCurrentPos(mf);
            ptrdiff_t offset;
            MatchFinder_MoveBlock(mf);
            offset = beforePtr - Inline_MatchFinder_GetPointerToCurrentPos(mf);
            mt->pointerToCurPos -= offset;
            mt->buffer -= offset;
          }
          CriticalSection_Leave(&mt->hashSync.cs);
          CriticalSection_Leave(&mt->btSync.cs);
          continue;
        }

        Semaphore_Wait(&p->freeSemaphore);

        if (p->exit) // exit is unexpected here. But we check it here for some failure case
          return;

        // for faster stop : we check (p->stopWriting) after Wait(freeSemaphore)
        if (p->stopWriting)
          break;

        MatchFinder_ReadIfRequired(mf);
        {
          UInt32 *heads = mt->hashBuf + GET_HASH_BLOCK_OFFSET(blockIndex++);
          UInt32 num = Inline_MatchFinder_GetNumAvailableBytes(mf);
          heads[0] = 2;
          heads[1] = num;

          /* heads[1] contains the number of avail bytes:
             if (avail < mf->numHashBytes) :
             {
               it means that stream was finished
               HASH_THREAD and BT_TREAD must move position for heads[1] (avail) bytes.
               HASH_THREAD doesn't stop,
               HASH_THREAD fills only the header (2 numbers) for all next blocks:
               {2, NumHashBytes - 1}, {2,0}, {2,0}, ... , {2,0}
             }
             else
             {
               HASH_THREAD and BT_TREAD must move position for (heads[0] - 2) bytes;
             }
          */

          if (num >= mf->numHashBytes)
          {
            num = num - mf->numHashBytes + 1;
            if (num > kMtHashBlockSize - 2)
              num = kMtHashBlockSize - 2;

            if (mf->pos > (UInt32)kMtMaxValForNormalize - num)
            {
              const UInt32 subValue = (mf->pos - mf->historySize - 1); // & ~(UInt32)(kNormalizeAlign - 1);
              MatchFinder_REDUCE_OFFSETS(mf, subValue)
              MatchFinder_Normalize3(subValue, mf->hash + mf->fixedHashSize, (size_t)mf->hashMask + 1);
            }

            heads[0] = 2 + num;
            mt->GetHeadsFunc(mf->buffer, mf->pos, mf->hash + mf->fixedHashSize, mf->hashMask, heads + 2, num, mf->crc);
          }

          mf->pos += num;  // wrap over zero is allowed at the end of stream
          mf->buffer += num;
        }
      }

      Semaphore_Release1(&p->filledSemaphore);
    } // for() processing end

    // p->numBlocks_Sent = blockIndex;
    Event_Set(&p->wasStopped);
  } // for() thread end
}




// ---------- BT THREAD ----------

/* we use one variable instead of two (cyclicBufferPos == pos) before CyclicBuf wrap.
   here we define fixed offset of (p->pos) from (p->cyclicBufferPos) */
#define CYC_TO_POS_OFFSET 0
// #define CYC_TO_POS_OFFSET 1 // for debug

#define MFMT_GM_INLINE

#ifdef MFMT_GM_INLINE

/*
  we use size_t for (pos) instead of UInt32
  to eliminate "movsx" BUG in old MSVC x64 compiler.
*/


UInt32 * Z7_FASTCALL GetMatchesSpecN_2(const Byte *lenLimit, size_t pos, const Byte *cur, CLzRef *son,
    UInt32 _cutValue, UInt32 *d, size_t _maxLen, const UInt32 *hash, const UInt32 *limit, const UInt32 *size,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize,
    UInt32 *posRes);

#endif


static void BtGetMatches(CMatchFinderMt *p, UInt32 *d)
{
  UInt32 numProcessed = 0;
  UInt32 curPos = 2;
  
  /* GetMatchesSpec() functions don't create (len = 1)
     in [len, dist] match pairs, if (p->numHashBytes >= 2)
     Also we suppose here that (matchMaxLen >= 2).
     So the following code for (reserve) is not required
     UInt32 reserve = (p->matchMaxLen * 2);
     const UInt32 kNumHashBytes_Max = 5; // BT_HASH_BYTES_MAX
     if (reserve < kNumHashBytes_Max - 1)
        reserve = kNumHashBytes_Max - 1;
     const UInt32 limit = kMtBtBlockSize - (reserve);
  */

  const UInt32 limit = kMtBtBlockSize - (p->matchMaxLen * 2);

  d[1] = p->hashNumAvail;

  if (p->failure_BT)
  {
    // printf("\n == 1 BtGetMatches() p->failure_BT\n");
    d[0] = 0;
    // d[1] = 0;
    return;
  }
  
  while (curPos < limit)
  {
    if (p->hashBufPos == p->hashBufPosLimit)
    {
      // MatchFinderMt_GetNextBlock_Hash(p);
      UInt32 avail;
      {
        const UInt32 bi = MtSync_GetNextBlock(&p->hashSync);
        const UInt32 k = GET_HASH_BLOCK_OFFSET(bi);
        const UInt32 *h = p->hashBuf + k;
        avail = h[1];
        p->hashBufPosLimit = k + h[0];
        p->hashNumAvail = avail;
        p->hashBufPos = k + 2;
      }

      {
        /* we must prevent UInt32 overflow for avail total value,
           if avail was increased with new hash block */
        UInt32 availSum = numProcessed + avail;
        if (availSum < numProcessed)
          availSum = (UInt32)(Int32)-1;
        d[1] = availSum;
      }

      if (avail >= p->numHashBytes)
        continue;

      // if (p->hashBufPos != p->hashBufPosLimit) exit(1);

      /* (avail < p->numHashBytes)
         It means that stream was finished.
         And (avail) - is a number of remaining bytes,
         we fill (d) for (avail) bytes for LZ_THREAD (receiver).
         but we don't update (p->pos) and (p->cyclicBufferPos) here in BT_THREAD */

      /* here we suppose that we have space enough:
         (kMtBtBlockSize - curPos >= p->hashNumAvail) */
      p->hashNumAvail = 0;
      d[0] = curPos + avail;
      d += curPos;
      for (; avail != 0; avail--)
        *d++ = 0;
      return;
    }
    {
      UInt32 size = p->hashBufPosLimit - p->hashBufPos;
      UInt32 pos = p->pos;
      UInt32 cyclicBufferPos = p->cyclicBufferPos;
      UInt32 lenLimit = p->matchMaxLen;
      if (lenLimit >= p->hashNumAvail)
        lenLimit = p->hashNumAvail;
      {
        UInt32 size2 = p->hashNumAvail - lenLimit + 1;
        if (size2 < size)
          size = size2;
        size2 = p->cyclicBufferSize - cyclicBufferPos;
        if (size2 < size)
          size = size2;
      }
      
      if (pos > (UInt32)kMtMaxValForNormalize - size)
      {
        const UInt32 subValue = (pos - p->cyclicBufferSize); // & ~(UInt32)(kNormalizeAlign - 1);
        pos -= subValue;
        p->pos = pos;
        MatchFinder_Normalize3(subValue, p->son, (size_t)p->cyclicBufferSize * 2);
      }

      #ifndef MFMT_GM_INLINE
      while (curPos < limit && size-- != 0)
      {
        UInt32 *startDistances = d + curPos;
        UInt32 num = (UInt32)(GetMatchesSpec1(lenLimit, pos - p->hashBuf[p->hashBufPos++],
            pos, p->buffer, p->son, cyclicBufferPos, p->cyclicBufferSize, p->cutValue,
            startDistances + 1, p->numHashBytes - 1) - startDistances);
        *startDistances = num - 1;
        curPos += num;
        cyclicBufferPos++;
        pos++;
        p->buffer++;
      }
      #else
      {
        UInt32 posRes = pos;
        const UInt32 *d_end;
        {
          d_end = GetMatchesSpecN_2(
              p->buffer + lenLimit - 1,
              pos, p->buffer, p->son, p->cutValue, d + curPos,
              p->numHashBytes - 1, p->hashBuf + p->hashBufPos,
              d + limit, p->hashBuf + p->hashBufPos + size,
              cyclicBufferPos, p->cyclicBufferSize,
              &posRes);
        }
        {
          if (!d_end)
          {
            // printf("\n == 2 BtGetMatches() p->failure_BT\n");
            // internal data failure
            p->failure_BT = True;
            d[0] = 0;
            // d[1] = 0;
            return;
          }
        }
        curPos = (UInt32)(d_end - d);
        {
          const UInt32 processed = posRes - pos;
          pos = posRes;
          p->hashBufPos += processed;
          cyclicBufferPos += processed;
          p->buffer += processed;
        }
      }
      #endif

      {
        const UInt32 processed = pos - p->pos;
        numProcessed += processed;
        p->hashNumAvail -= processed;
        p->pos = pos;
      }
      if (cyclicBufferPos == p->cyclicBufferSize)
        cyclicBufferPos = 0;
      p->cyclicBufferPos = cyclicBufferPos;
    }
  }
  
  d[0] = curPos;
}


static void BtFillBlock(CMatchFinderMt *p, UInt32 globalBlockIndex)
{
  CMtSync *sync = &p->hashSync;
  
  BUFFER_MUST_BE_UNLOCKED(sync)
  
  if (!sync->needStart)
  {
    LOCK_BUFFER(sync)
  }
  
  BtGetMatches(p, p->btBuf + GET_BT_BLOCK_OFFSET(globalBlockIndex));
  
  /* We suppose that we have called GetNextBlock() from start.
     So buffer is LOCKED */

  UNLOCK_BUFFER(sync)
}


Z7_NO_INLINE
static void BtThreadFunc(CMatchFinderMt *mt)
{
  CMtSync *p = &mt->btSync;
  for (;;)
  {
    UInt32 blockIndex = 0;
    Event_Wait(&p->canStart);

    for (;;)
    {
        PRF(printf("  BT thread block = %d  pos = %d\n", (unsigned)blockIndex, mt->pos));
      /* (p->exit == true) is possible after (p->canStart) at first loop iteration
         and is unexpected after more Wait(freeSemaphore) iterations */
      if (p->exit)
        return;

      Semaphore_Wait(&p->freeSemaphore);
      
      // for faster stop : we check (p->stopWriting) after Wait(freeSemaphore)
      if (p->stopWriting)
        break;

      BtFillBlock(mt, blockIndex++);
      
      Semaphore_Release1(&p->filledSemaphore);
    }

    // we stop HASH_THREAD here
    MtSync_StopWriting(&mt->hashSync);

    // p->numBlocks_Sent = blockIndex;
    Event_Set(&p->wasStopped);
  }
}


void MatchFinderMt_Construct(CMatchFinderMt *p)
{
  p->hashBuf = NULL;
  MtSync_Construct(&p->hashSync);
  MtSync_Construct(&p->btSync);
}

static void MatchFinderMt_FreeMem(CMatchFinderMt *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->hashBuf);
  p->hashBuf = NULL;
}

void MatchFinderMt_Destruct(CMatchFinderMt *p, ISzAllocPtr alloc)
{
  /*
     HASH_THREAD can use CriticalSection(s) btSync.cs and hashSync.cs.
     So we must be sure that HASH_THREAD will not use CriticalSection(s)
     after deleting CriticalSection here.

     we call ReleaseStream(p)
       that calls StopWriting(btSync)
         that calls StopWriting(hashSync), if it's required to stop HASH_THREAD.
     after StopWriting() it's safe to destruct MtSync(s) in any order */

  MatchFinderMt_ReleaseStream(p);

  MtSync_Destruct(&p->btSync);
  MtSync_Destruct(&p->hashSync);

  LOG_ITER(
  printf("\nTree %9d * %7d iter = %9d = sum  :  bytes = %9d\n",
      (UInt32)(g_NumIters_Tree / 1000),
      (UInt32)(((UInt64)g_NumIters_Loop * 1000) / (g_NumIters_Tree + 1)),
      (UInt32)(g_NumIters_Loop / 1000),
      (UInt32)(g_NumIters_Bytes / 1000)
      ));

  MatchFinderMt_FreeMem(p, alloc);
}


#define kHashBufferSize (kMtHashBlockSize * kMtHashNumBlocks)
#define kBtBufferSize (kMtBtBlockSize * kMtBtNumBlocks)


static THREAD_FUNC_DECL HashThreadFunc2(void *p) { HashThreadFunc((CMatchFinderMt *)p);  return 0; }
static THREAD_FUNC_DECL BtThreadFunc2(void *p)
{
  Byte allocaDummy[0x180];
  unsigned i = 0;
  for (i = 0; i < 16; i++)
    allocaDummy[i] = (Byte)0;
  if (allocaDummy[0] == 0)
    BtThreadFunc((CMatchFinderMt *)p);
  return 0;
}


SRes MatchFinderMt_Create(CMatchFinderMt *p, UInt32 historySize, UInt32 keepAddBufferBefore,
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter, ISzAllocPtr alloc)
{
  CMatchFinder *mf = MF(p);
  p->historySize = historySize;
  if (kMtBtBlockSize <= matchMaxLen * 4)
    return SZ_ERROR_PARAM;
  if (!p->hashBuf)
  {
    p->hashBuf = (UInt32 *)ISzAlloc_Alloc(alloc, ((size_t)kHashBufferSize + (size_t)kBtBufferSize) * sizeof(UInt32));
    if (!p->hashBuf)
      return SZ_ERROR_MEM;
    p->btBuf = p->hashBuf + kHashBufferSize;
  }
  keepAddBufferBefore += (kHashBufferSize + kBtBufferSize);
  keepAddBufferAfter += kMtHashBlockSize;
  if (!MatchFinder_Create(mf, historySize, keepAddBufferBefore, matchMaxLen, keepAddBufferAfter, alloc))
    return SZ_ERROR_MEM;

  RINOK(MtSync_Create(&p->hashSync, HashThreadFunc2, p))
  RINOK(MtSync_Create(&p->btSync, BtThreadFunc2, p))
  return SZ_OK;
}


SRes MatchFinderMt_InitMt(CMatchFinderMt *p)
{
  RINOK(MtSync_Init(&p->hashSync, kMtHashNumBlocks))
  return MtSync_Init(&p->btSync, kMtBtNumBlocks);
}


static void MatchFinderMt_Init(void *_p)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  CMatchFinder *mf = MF(p);
  
  p->btBufPos =
  p->btBufPosLimit = NULL;
  p->hashBufPos =
  p->hashBufPosLimit = 0;
  p->hashNumAvail = 0; // 21.03
  
  p->failure_BT = False;

  /* Init without data reading. We don't want to read data in this thread */
  MatchFinder_Init_4(mf);

  MatchFinder_Init_LowHash(mf);
  
  p->pointerToCurPos = Inline_MatchFinder_GetPointerToCurrentPos(mf);
  p->btNumAvailBytes = 0;
  p->failure_LZ_BT = False;
  // p->failure_LZ_LZ = False;
  
  p->lzPos =
      1; // optimal smallest value
      // 0; // for debug: ignores match to start
      // kNormalizeAlign; // for debug

  p->hash = mf->hash;
  p->fixedHashSize = mf->fixedHashSize;
  // p->hash4Mask = mf->hash4Mask;
  p->crc = mf->crc;
  // memcpy(p->crc, mf->crc, sizeof(mf->crc));

  p->son = mf->son;
  p->matchMaxLen = mf->matchMaxLen;
  p->numHashBytes = mf->numHashBytes;
  
  /* (mf->pos) and (mf->streamPos) were already initialized to 1 in MatchFinder_Init_4() */
  // mf->streamPos = mf->pos = 1; // optimal smallest value
      // 0; // for debug: ignores match to start
      // kNormalizeAlign; // for debug

  /* we must init (p->pos = mf->pos) for BT, because
     BT code needs (p->pos == delta_value_for_empty_hash_record == mf->pos) */
  p->pos = mf->pos; // do not change it
  
  p->cyclicBufferPos = (p->pos - CYC_TO_POS_OFFSET);
  p->cyclicBufferSize = mf->cyclicBufferSize;
  p->buffer = mf->buffer;
  p->cutValue = mf->cutValue;
  // p->son[0] = p->son[1] = 0; // unused: to init skipped record for speculated accesses.
}


/* ReleaseStream is required to finish multithreading */
void MatchFinderMt_ReleaseStream(CMatchFinderMt *p)
{
  // Sleep(1); // for debug
  MtSync_StopWriting(&p->btSync);
  // Sleep(200); // for debug
  /* p->MatchFinder->ReleaseStream(); */
}


Z7_NO_INLINE
static UInt32 MatchFinderMt_GetNextBlock_Bt(CMatchFinderMt *p)
{
  if (p->failure_LZ_BT)
    p->btBufPos = p->failureBuf;
  else
  {
    const UInt32 bi = MtSync_GetNextBlock(&p->btSync);
    const UInt32 *bt = p->btBuf + GET_BT_BLOCK_OFFSET(bi);
    {
      const UInt32 numItems = bt[0];
      p->btBufPosLimit = bt + numItems;
      p->btNumAvailBytes = bt[1];
      p->btBufPos = bt + 2;
      if (numItems < 2 || numItems > kMtBtBlockSize)
      {
        p->failureBuf[0] = 0;
        p->btBufPos = p->failureBuf;
        p->btBufPosLimit = p->failureBuf + 1;
        p->failure_LZ_BT = True;
        // p->btNumAvailBytes = 0;
        /* we don't want to decrease AvailBytes, that was load before.
            that can be unxepected for the code that have loaded anopther value before */
      }
    }
  
    if (p->lzPos >= (UInt32)kMtMaxValForNormalize - (UInt32)kMtBtBlockSize)
    {
      /* we don't check (lzPos) over exact avail bytes in (btBuf).
         (fixedHashSize) is small, so normalization is fast */
      const UInt32 subValue = (p->lzPos - p->historySize - 1); // & ~(UInt32)(kNormalizeAlign - 1);
      p->lzPos -= subValue;
      MatchFinder_Normalize3(subValue, p->hash, p->fixedHashSize);
    }
  }
  return p->btNumAvailBytes;
}



static const Byte * MatchFinderMt_GetPointerToCurrentPos(void *_p)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  return p->pointerToCurPos;
}


#define GET_NEXT_BLOCK_IF_REQUIRED if (p->btBufPos == p->btBufPosLimit) MatchFinderMt_GetNextBlock_Bt(p);


static UInt32 MatchFinderMt_GetNumAvailableBytes(void *_p)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  if (p->btBufPos != p->btBufPosLimit)
    return p->btNumAvailBytes;
  return MatchFinderMt_GetNextBlock_Bt(p);
}


// #define CHECK_FAILURE_LZ(_match_, _pos_) if (_match_ >= _pos_) { p->failure_LZ_LZ = True;  return d; }
#define CHECK_FAILURE_LZ(_match_, _pos_)

static UInt32 * MixMatches2(CMatchFinderMt *p, UInt32 matchMinPos, UInt32 *d)
{
  UInt32 h2, c2;
  UInt32 *hash = p->hash;
  const Byte *cur = p->pointerToCurPos;
  const UInt32 m = p->lzPos;
  MT_HASH2_CALC
      
  c2 = hash[h2];
  hash[h2] = m;

  if (c2 >= matchMinPos)
  {
    CHECK_FAILURE_LZ(c2, m)
    if (cur[(ptrdiff_t)c2 - (ptrdiff_t)m] == cur[0])
    {
      *d++ = 2;
      *d++ = m - c2 - 1;
    }
  }
  
  return d;
}

static UInt32 * MixMatches3(CMatchFinderMt *p, UInt32 matchMinPos, UInt32 *d)
{
  UInt32 h2, h3, c2, c3;
  UInt32 *hash = p->hash;
  const Byte *cur = p->pointerToCurPos;
  const UInt32 m = p->lzPos;
  MT_HASH3_CALC

  c2 = hash[h2];
  c3 = (hash + kFix3HashSize)[h3];
  
  hash[h2] = m;
  (hash + kFix3HashSize)[h3] = m;

  if (c2 >= matchMinPos)
  {
    CHECK_FAILURE_LZ(c2, m)
    if (cur[(ptrdiff_t)c2 - (ptrdiff_t)m] == cur[0])
    {
      d[1] = m - c2 - 1;
      if (cur[(ptrdiff_t)c2 - (ptrdiff_t)m + 2] == cur[2])
      {
        d[0] = 3;
        return d + 2;
      }
      d[0] = 2;
      d += 2;
    }
  }
  
  if (c3 >= matchMinPos)
  {
    CHECK_FAILURE_LZ(c3, m)
    if (cur[(ptrdiff_t)c3 - (ptrdiff_t)m] == cur[0])
    {
      *d++ = 3;
      *d++ = m - c3 - 1;
    }
  }
  
  return d;
}


#define INCREASE_LZ_POS p->lzPos++; p->pointerToCurPos++;

/*
static
UInt32* MatchFinderMt_GetMatches_Bt4(CMatchFinderMt *p, UInt32 *d)
{
  const UInt32 *bt = p->btBufPos;
  const UInt32 len = *bt++;
  const UInt32 *btLim = bt + len;
  UInt32 matchMinPos;
  UInt32 avail = p->btNumAvailBytes - 1;
  p->btBufPos = btLim;

  {
    p->btNumAvailBytes = avail;

    #define BT_HASH_BYTES_MAX 5
      
    matchMinPos = p->lzPos;

    if (len != 0)
      matchMinPos -= bt[1];
    else if (avail < (BT_HASH_BYTES_MAX - 1) - 1)
    {
      INCREASE_LZ_POS
      return d;
    }
    else
    {
      const UInt32 hs = p->historySize;
      if (matchMinPos > hs)
        matchMinPos -= hs;
      else
        matchMinPos = 1;
    }
  }

  for (;;)
  {
  
  UInt32 h2, h3, c2, c3;
  UInt32 *hash = p->hash;
  const Byte *cur = p->pointerToCurPos;
  UInt32 m = p->lzPos;
  MT_HASH3_CALC

  c2 = hash[h2];
  c3 = (hash + kFix3HashSize)[h3];
 
  hash[h2] = m;
  (hash + kFix3HashSize)[h3] = m;

  if (c2 >= matchMinPos && cur[(ptrdiff_t)c2 - (ptrdiff_t)m] == cur[0])
  {
    d[1] = m - c2 - 1;
    if (cur[(ptrdiff_t)c2 - (ptrdiff_t)m + 2] == cur[2])
    {
      d[0] = 3;
      d += 2;
      break;
    }
    // else
    {
      d[0] = 2;
      d += 2;
    }
  }
  if (c3 >= matchMinPos && cur[(ptrdiff_t)c3 - (ptrdiff_t)m] == cur[0])
  {
    *d++ = 3;
    *d++ = m - c3 - 1;
  }
  break;
  }

  if (len != 0)
  {
    do
    {
      const UInt32 v0 = bt[0];
      const UInt32 v1 = bt[1];
      bt += 2;
      d[0] = v0;
      d[1] = v1;
      d += 2;
    }
    while (bt != btLim);
  }
  INCREASE_LZ_POS
  return d;
}
*/


static UInt32 * MixMatches4(CMatchFinderMt *p, UInt32 matchMinPos, UInt32 *d)
{
  UInt32 h2, h3, /* h4, */ c2, c3 /* , c4 */;
  UInt32 *hash = p->hash;
  const Byte *cur = p->pointerToCurPos;
  const UInt32 m = p->lzPos;
  MT_HASH3_CALC
  // MT_HASH4_CALC
  c2 = hash[h2];
  c3 = (hash + kFix3HashSize)[h3];
  // c4 = (hash + kFix4HashSize)[h4];
  
  hash[h2] = m;
  (hash + kFix3HashSize)[h3] = m;
  // (hash + kFix4HashSize)[h4] = m;

  // #define BT5_USE_H2
  // #ifdef BT5_USE_H2
  if (c2 >= matchMinPos && cur[(ptrdiff_t)c2 - (ptrdiff_t)m] == cur[0])
  {
    d[1] = m - c2 - 1;
    if (cur[(ptrdiff_t)c2 - (ptrdiff_t)m + 2] == cur[2])
    {
      // d[0] = (cur[(ptrdiff_t)c2 - (ptrdiff_t)m + 3] == cur[3]) ? 4 : 3;
      // return d + 2;

      if (cur[(ptrdiff_t)c2 - (ptrdiff_t)m + 3] == cur[3])
      {
        d[0] = 4;
        return d + 2;
      }
      d[0] = 3;
      d += 2;

      #ifdef BT5_USE_H4
      if (c4 >= matchMinPos)
        if (
          cur[(ptrdiff_t)c4 - (ptrdiff_t)m]     == cur[0] &&
          cur[(ptrdiff_t)c4 - (ptrdiff_t)m + 3] == cur[3]
          )
      {
        *d++ = 4;
        *d++ = m - c4 - 1;
      }
      #endif
      return d;
    }
    d[0] = 2;
    d += 2;
  }
  // #endif
  
  if (c3 >= matchMinPos && cur[(ptrdiff_t)c3 - (ptrdiff_t)m] == cur[0])
  {
    d[1] = m - c3 - 1;
    if (cur[(ptrdiff_t)c3 - (ptrdiff_t)m + 3] == cur[3])
    {
      d[0] = 4;
      return d + 2;
    }
    d[0] = 3;
    d += 2;
  }

  #ifdef BT5_USE_H4
  if (c4 >= matchMinPos)
    if (
      cur[(ptrdiff_t)c4 - (ptrdiff_t)m]     == cur[0] &&
      cur[(ptrdiff_t)c4 - (ptrdiff_t)m + 3] == cur[3]
      )
    {
      *d++ = 4;
      *d++ = m - c4 - 1;
    }
  #endif
  
  return d;
}


static UInt32 * MatchFinderMt2_GetMatches(void *_p, UInt32 *d)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  const UInt32 *bt = p->btBufPos;
  const UInt32 len = *bt++;
  const UInt32 *btLim = bt + len;
  p->btBufPos = btLim;
  p->btNumAvailBytes--;
  INCREASE_LZ_POS
  {
    while (bt != btLim)
    {
      const UInt32 v0 = bt[0];
      const UInt32 v1 = bt[1];
      bt += 2;
      d[0] = v0;
      d[1] = v1;
      d += 2;
    }
  }
  return d;
}



static UInt32 * MatchFinderMt_GetMatches(void *_p, UInt32 *d)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  const UInt32 *bt = p->btBufPos;
  UInt32 len = *bt++;
  const UInt32 avail = p->btNumAvailBytes - 1;
  p->btNumAvailBytes = avail;
  p->btBufPos = bt + len;
  if (len == 0)
  {
    #define BT_HASH_BYTES_MAX 5
    if (avail >= (BT_HASH_BYTES_MAX - 1) - 1)
    {
      UInt32 m = p->lzPos;
      if (m > p->historySize)
        m -= p->historySize;
      else
        m = 1;
      d = p->MixMatchesFunc(p, m, d);
    }
  }
  else
  {
    /*
      first match pair from BinTree: (match_len, match_dist),
      (match_len >= numHashBytes).
      MixMatchesFunc() inserts only hash matches that are nearer than (match_dist)
    */
    d = p->MixMatchesFunc(p, p->lzPos - bt[1], d);
    // if (d) // check for failure
    do
    {
      const UInt32 v0 = bt[0];
      const UInt32 v1 = bt[1];
      bt += 2;
      d[0] = v0;
      d[1] = v1;
      d += 2;
    }
    while (len -= 2);
  }
  INCREASE_LZ_POS
  return d;
}

#define SKIP_HEADER2_MT  do { GET_NEXT_BLOCK_IF_REQUIRED
#define SKIP_HEADER_MT(n) SKIP_HEADER2_MT if (p->btNumAvailBytes-- >= (n)) { const Byte *cur = p->pointerToCurPos; UInt32 *hash = p->hash;
#define SKIP_FOOTER_MT } INCREASE_LZ_POS p->btBufPos += (size_t)*p->btBufPos + 1; } while (--num != 0);

static void MatchFinderMt0_Skip(void *_p, UInt32 num)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  SKIP_HEADER2_MT { p->btNumAvailBytes--;
  SKIP_FOOTER_MT
}

static void MatchFinderMt2_Skip(void *_p, UInt32 num)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  SKIP_HEADER_MT(2)
      UInt32 h2;
      MT_HASH2_CALC
      hash[h2] = p->lzPos;
  SKIP_FOOTER_MT
}

static void MatchFinderMt3_Skip(void *_p, UInt32 num)
{
  CMatchFinderMt *p = (CMatchFinderMt *)_p;
  SKIP_HEADER_MT(3)
      UInt32 h2, h3;
      MT_HASH3_CALC
      (hash + kFix3HashSize)[h3] =
      hash[                h2] =
        p->lzPos;
  SKIP_FOOTER_MT
}

/*
// MatchFinderMt4_Skip() is similar to MatchFinderMt3_Skip().
// The difference is that MatchFinderMt3_Skip() updates hash for last 3 bytes of stream.

static void MatchFinderMt4_Skip(CMatchFinderMt *p, UInt32 num)
{
  SKIP_HEADER_MT(4)
      UInt32 h2, h3; // h4
      MT_HASH3_CALC
      // MT_HASH4_CALC
      // (hash + kFix4HashSize)[h4] =
      (hash + kFix3HashSize)[h3] =
      hash[                h2] =
        p->lzPos;
  SKIP_FOOTER_MT
}
*/

void MatchFinderMt_CreateVTable(CMatchFinderMt *p, IMatchFinder2 *vTable)
{
  vTable->Init = MatchFinderMt_Init;
  vTable->GetNumAvailableBytes = MatchFinderMt_GetNumAvailableBytes;
  vTable->GetPointerToCurrentPos = MatchFinderMt_GetPointerToCurrentPos;
  vTable->GetMatches = MatchFinderMt_GetMatches;
  
  switch (MF(p)->numHashBytes)
  {
    case 2:
      p->GetHeadsFunc = GetHeads2;
      p->MixMatchesFunc = NULL;
      vTable->Skip = MatchFinderMt0_Skip;
      vTable->GetMatches = MatchFinderMt2_GetMatches;
      break;
    case 3:
      p->GetHeadsFunc = MF(p)->bigHash ? GetHeads3b : GetHeads3;
      p->MixMatchesFunc = MixMatches2;
      vTable->Skip = MatchFinderMt2_Skip;
      break;
    case 4:
      p->GetHeadsFunc = MF(p)->bigHash ? GetHeads4b : GetHeads4;

      // it's fast inline version of GetMatches()
      // vTable->GetMatches = MatchFinderMt_GetMatches_Bt4;

      p->MixMatchesFunc = MixMatches3;
      vTable->Skip = MatchFinderMt3_Skip;
      break;
    default:
      p->GetHeadsFunc = MF(p)->bigHash ? GetHeads5b : GetHeads5;
      p->MixMatchesFunc = MixMatches4;
      vTable->Skip =
          MatchFinderMt3_Skip;
          // MatchFinderMt4_Skip;
      break;
  }
}

#undef RINOK_THREAD
#undef PRF
#undef MF
#undef GetUi24hi_from32
#undef LOCK_BUFFER
#undef UNLOCK_BUFFER

/* ================ unit: C/LzFindOpt.c ================ */
/* LzFindOpt.c -- multithreaded Match finder for LZ algorithms
2023-04-02 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// #include "LzFindMt.h"

// #define LOG_ITERS

// #define LOG_THREAD

#ifdef LOG_THREAD
#include <stdio.h>
#define PRF(x) x
#else
// #define PRF(x)
#endif

#ifdef LOG_ITERS
#include <stdio.h>
UInt64 g_NumIters_Tree;
UInt64 g_NumIters_Loop;
UInt64 g_NumIters_Bytes;
#define LOG_ITER(x) x
#else
#define LOG_ITER(x)
#endif

// ---------- BT THREAD ----------

#define USE_SON_PREFETCH
#define USE_LONG_MATCH_OPT

#define kEmptyHashValue 0

// #define CYC_TO_POS_OFFSET 0

// #define CYC_TO_POS_OFFSET 1 // for debug

/*
Z7_NO_INLINE
UInt32 * Z7_FASTCALL GetMatchesSpecN_1(const Byte *lenLimit, size_t pos, const Byte *cur, CLzRef *son,
    UInt32 _cutValue, UInt32 *d, size_t _maxLen, const UInt32 *hash, const UInt32 *limit, const UInt32 *size, UInt32 *posRes)
{
  do
  {
    UInt32 delta;
    if (hash == size)
      break;
    delta = *hash++;

    if (delta == 0 || delta > (UInt32)pos)
      return NULL;

    lenLimit++;

    if (delta == (UInt32)pos)
    {
      CLzRef *ptr1 = son + ((size_t)pos << 1) - CYC_TO_POS_OFFSET * 2;
      *d++ = 0;
      ptr1[0] = kEmptyHashValue;
      ptr1[1] = kEmptyHashValue;
    }
else
{
  UInt32 *_distances = ++d;

  CLzRef *ptr0 = son + ((size_t)(pos) << 1) - CYC_TO_POS_OFFSET * 2 + 1;
  CLzRef *ptr1 = son + ((size_t)(pos) << 1) - CYC_TO_POS_OFFSET * 2;

  const Byte *len0 = cur, *len1 = cur;
  UInt32 cutValue = _cutValue;
  const Byte *maxLen = cur + _maxLen;

  for (LOG_ITER(g_NumIters_Tree++);;)
  {
    LOG_ITER(g_NumIters_Loop++);
    {
      const ptrdiff_t diff = (ptrdiff_t)0 - (ptrdiff_t)delta;
      CLzRef *pair = son + ((size_t)(((ptrdiff_t)pos - CYC_TO_POS_OFFSET) + diff) << 1);
      const Byte *len = (len0 < len1 ? len0 : len1);

    #ifdef USE_SON_PREFETCH
      const UInt32 pair0 = *pair;
    #endif

      if (len[diff] == len[0])
      {
        if (++len != lenLimit && len[diff] == len[0])
          while (++len != lenLimit)
          {
            LOG_ITER(g_NumIters_Bytes++);
            if (len[diff] != len[0])
              break;
          }
        if (maxLen < len)
        {
          maxLen = len;
          *d++ = (UInt32)(len - cur);
          *d++ = delta - 1;
          
          if (len == lenLimit)
          {
            const UInt32 pair1 = pair[1];
            *ptr1 =
              #ifdef USE_SON_PREFETCH
                pair0;
              #else
                pair[0];
              #endif
            *ptr0 = pair1;

            _distances[-1] = (UInt32)(d - _distances);

            #ifdef USE_LONG_MATCH_OPT

                if (hash == size || *hash != delta || lenLimit[diff] != lenLimit[0] || d >= limit)
                  break;

            {
              for (;;)
              {
                hash++;
                pos++;
                cur++;
                lenLimit++;
                {
                  CLzRef *ptr = son + ((size_t)(pos) << 1) - CYC_TO_POS_OFFSET * 2;
                  #if 0
                  *(UInt64 *)(void *)ptr = ((const UInt64 *)(const void *)ptr)[diff];
                  #else
                  const UInt32 p0 = ptr[0 + (diff * 2)];
                  const UInt32 p1 = ptr[1 + (diff * 2)];
                  ptr[0] = p0;
                  ptr[1] = p1;
                  // ptr[0] = ptr[0 + (diff * 2)];
                  // ptr[1] = ptr[1 + (diff * 2)];
                  #endif
                }
                // PrintSon(son + 2, pos - 1);
                // printf("\npos = %x delta = %x\n", pos, delta);
                len++;
                *d++ = 2;
                *d++ = (UInt32)(len - cur);
                *d++ = delta - 1;
                if (hash == size || *hash != delta || lenLimit[diff] != lenLimit[0] || d >= limit)
                  break;
              }
            }
            #endif

            break;
          }
        }
      }

      {
        const UInt32 curMatch = (UInt32)pos - delta; // (UInt32)(pos + diff);
        if (len[diff] < len[0])
        {
          delta = pair[1];
          if (delta >= curMatch)
            return NULL;
          *ptr1 = curMatch;
          ptr1 = pair + 1;
          len1 = len;
        }
        else
        {
          delta = *pair;
          if (delta >= curMatch)
            return NULL;
          *ptr0 = curMatch;
          ptr0 = pair;
          len0 = len;
        }

        delta = (UInt32)pos - delta;
 
        if (--cutValue == 0 || delta >= pos)
        {
          *ptr0 = *ptr1 = kEmptyHashValue;
          _distances[-1] = (UInt32)(d - _distances);
          break;
        }
      }
    }
  } // for (tree iterations)
}
    pos++;
    cur++;
  }
  while (d < limit);
  *posRes = (UInt32)pos;
  return d;
}
*/

/* define cbs if you use 2 functions.
       GetMatchesSpecN_1() :  (pos <  _cyclicBufferSize)
       GetMatchesSpecN_2() :  (pos >= _cyclicBufferSize)

  do not define cbs if you use 1 function:
       GetMatchesSpecN_2()
*/

// #define cbs _cyclicBufferSize

/*
  we use size_t for (pos) and (_cyclicBufferPos_ instead of UInt32
  to eliminate "movsx" BUG in old MSVC x64 compiler.
*/

UInt32 * Z7_FASTCALL GetMatchesSpecN_2(const Byte *lenLimit, size_t pos, const Byte *cur, CLzRef *son,
    UInt32 _cutValue, UInt32 *d, size_t _maxLen, const UInt32 *hash, const UInt32 *limit, const UInt32 *size,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize,
    UInt32 *posRes);

Z7_NO_INLINE
UInt32 * Z7_FASTCALL GetMatchesSpecN_2(const Byte *lenLimit, size_t pos, const Byte *cur, CLzRef *son,
    UInt32 _cutValue, UInt32 *d, size_t _maxLen, const UInt32 *hash, const UInt32 *limit, const UInt32 *size,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize,
    UInt32 *posRes)
{
  do // while (hash != size)
  {
    UInt32 delta;
    
  #ifndef cbs
    UInt32 cbs;
  #endif

    if (hash == size)
      break;

    delta = *hash++;

    if (delta == 0)
      return NULL;

    lenLimit++;

  #ifndef cbs
    cbs = _cyclicBufferSize;
    if ((UInt32)pos < cbs)
    {
      if (delta > (UInt32)pos)
        return NULL;
      cbs = (UInt32)pos;
    }
  #endif

    if (delta >= cbs)
    {
      CLzRef *ptr1 = son + ((size_t)_cyclicBufferPos << 1);
      *d++ = 0;
      ptr1[0] = kEmptyHashValue;
      ptr1[1] = kEmptyHashValue;
    }
else
{
  UInt32 *_distances = ++d;

  CLzRef *ptr0 = son + ((size_t)_cyclicBufferPos << 1) + 1;
  CLzRef *ptr1 = son + ((size_t)_cyclicBufferPos << 1);

  UInt32 cutValue = _cutValue;
  const Byte *len0 = cur, *len1 = cur;
  const Byte *maxLen = cur + _maxLen;

  // if (cutValue == 0) { *ptr0 = *ptr1 = kEmptyHashValue; } else
  for (LOG_ITER(g_NumIters_Tree++);;)
  {
    LOG_ITER(g_NumIters_Loop++);
    {
      // SPEC code
      CLzRef *pair = son + ((size_t)((ptrdiff_t)_cyclicBufferPos - (ptrdiff_t)delta
          + (ptrdiff_t)(UInt32)(_cyclicBufferPos < delta ? cbs : 0)
          ) << 1);

      const ptrdiff_t diff = (ptrdiff_t)0 - (ptrdiff_t)delta;
      const Byte *len = (len0 < len1 ? len0 : len1);

    #ifdef USE_SON_PREFETCH
      const UInt32 pair0 = *pair;
    #endif

      if (len[diff] == len[0])
      {
        if (++len != lenLimit && len[diff] == len[0])
          while (++len != lenLimit)
          {
            LOG_ITER(g_NumIters_Bytes++);
            if (len[diff] != len[0])
              break;
          }
        if (maxLen < len)
        {
          maxLen = len;
          *d++ = (UInt32)(len - cur);
          *d++ = delta - 1;
          
          if (len == lenLimit)
          {
            const UInt32 pair1 = pair[1];
            *ptr1 =
              #ifdef USE_SON_PREFETCH
                pair0;
              #else
                pair[0];
              #endif
            *ptr0 = pair1;

            _distances[-1] = (UInt32)(d - _distances);

            #ifdef USE_LONG_MATCH_OPT

                if (hash == size || *hash != delta || lenLimit[diff] != lenLimit[0] || d >= limit)
                  break;

            {
              for (;;)
              {
                *d++ = 2;
                *d++ = (UInt32)(lenLimit - cur);
                *d++ = delta - 1;
                cur++;
                lenLimit++;
                // SPEC
                _cyclicBufferPos++;
                {
                  // SPEC code
                  CLzRef *dest = son + ((size_t)(_cyclicBufferPos) << 1);
                  const CLzRef *src = dest + ((diff
                      + (ptrdiff_t)(UInt32)((_cyclicBufferPos < delta) ? cbs : 0)) << 1);
                  // CLzRef *ptr = son + ((size_t)(pos) << 1) - CYC_TO_POS_OFFSET * 2;
                  #if 0
                  *(UInt64 *)(void *)dest = *((const UInt64 *)(const void *)src);
                  #else
                  const UInt32 p0 = src[0];
                  const UInt32 p1 = src[1];
                  dest[0] = p0;
                  dest[1] = p1;
                  #endif
                }
                pos++;
                hash++;
                if (hash == size || *hash != delta || lenLimit[diff] != lenLimit[0] || d >= limit)
                  break;
              } // for() end for long matches
            }
            #endif

            break; // break from TREE iterations
          }
        }
      }
      {
        const UInt32 curMatch = (UInt32)pos - delta; // (UInt32)(pos + diff);
        if (len[diff] < len[0])
        {
          delta = pair[1];
          *ptr1 = curMatch;
          ptr1 = pair + 1;
          len1 = len;
          if (delta >= curMatch)
            return NULL;
        }
        else
        {
          delta = *pair;
          *ptr0 = curMatch;
          ptr0 = pair;
          len0 = len;
          if (delta >= curMatch)
            return NULL;
        }
        delta = (UInt32)pos - delta;
 
        if (--cutValue == 0 || delta >= cbs)
        {
          *ptr0 = *ptr1 = kEmptyHashValue;
          _distances[-1] = (UInt32)(d - _distances);
          break;
        }
      }
    }
  } // for (tree iterations)
}
    pos++;
    _cyclicBufferPos++;
    cur++;
  }
  while (d < limit);
  *posRes = (UInt32)pos;
  return d;
}



/*
typedef UInt32 uint32plus; // size_t

UInt32 * Z7_FASTCALL GetMatchesSpecN_3(uint32plus lenLimit, size_t pos, const Byte *cur, CLzRef *son,
    UInt32 _cutValue, UInt32 *d, uint32plus _maxLen, const UInt32 *hash, const UInt32 *limit, const UInt32 *size,
    size_t _cyclicBufferPos, UInt32 _cyclicBufferSize,
    UInt32 *posRes)
{
  do // while (hash != size)
  {
    UInt32 delta;

  #ifndef cbs
    UInt32 cbs;
  #endif

    if (hash == size)
      break;

    delta = *hash++;

    if (delta == 0)
      return NULL;

  #ifndef cbs
    cbs = _cyclicBufferSize;
    if ((UInt32)pos < cbs)
    {
      if (delta > (UInt32)pos)
        return NULL;
      cbs = (UInt32)pos;
    }
  #endif
    
    if (delta >= cbs)
    {
      CLzRef *ptr1 = son + ((size_t)_cyclicBufferPos << 1);
      *d++ = 0;
      ptr1[0] = kEmptyHashValue;
      ptr1[1] = kEmptyHashValue;
    }
else
{
  CLzRef *ptr0 = son + ((size_t)_cyclicBufferPos << 1) + 1;
  CLzRef *ptr1 = son + ((size_t)_cyclicBufferPos << 1);
  UInt32 *_distances = ++d;
  uint32plus len0 = 0, len1 = 0;
  UInt32 cutValue = _cutValue;
  uint32plus maxLen = _maxLen;
  // lenLimit++; // const Byte *lenLimit = cur + _lenLimit;

  for (LOG_ITER(g_NumIters_Tree++);;)
  {
    LOG_ITER(g_NumIters_Loop++);
    {
      // const ptrdiff_t diff = (ptrdiff_t)0 - (ptrdiff_t)delta;
      CLzRef *pair = son + ((size_t)((ptrdiff_t)_cyclicBufferPos - delta
          + (ptrdiff_t)(UInt32)(_cyclicBufferPos < delta ? cbs : 0)
          ) << 1);
      const Byte *pb = cur - delta;
      uint32plus len = (len0 < len1 ? len0 : len1);

    #ifdef USE_SON_PREFETCH
      const UInt32 pair0 = *pair;
    #endif

      if (pb[len] == cur[len])
      {
        if (++len != lenLimit && pb[len] == cur[len])
          while (++len != lenLimit)
            if (pb[len] != cur[len])
              break;
        if (maxLen < len)
        {
          maxLen = len;
          *d++ = (UInt32)len;
          *d++ = delta - 1;
          if (len == lenLimit)
          {
            {
              const UInt32 pair1 = pair[1];
              *ptr0 = pair1;
              *ptr1 =
              #ifdef USE_SON_PREFETCH
                pair0;
              #else
                pair[0];
              #endif
            }

            _distances[-1] = (UInt32)(d - _distances);

            #ifdef USE_LONG_MATCH_OPT

                if (hash == size || *hash != delta || pb[lenLimit] != cur[lenLimit] || d >= limit)
                  break;

            {
              const ptrdiff_t diff = (ptrdiff_t)0 - (ptrdiff_t)delta;
              for (;;)
              {
                *d++ = 2;
                *d++ = (UInt32)lenLimit;
                *d++ = delta - 1;
                _cyclicBufferPos++;
                {
                  CLzRef *dest = son + ((size_t)_cyclicBufferPos << 1);
                  const CLzRef *src = dest + ((diff +
                      (ptrdiff_t)(UInt32)(_cyclicBufferPos < delta ? cbs : 0)) << 1);
                #if 0
                  *(UInt64 *)(void *)dest = *((const UInt64 *)(const void *)src);
                #else
                  const UInt32 p0 = src[0];
                  const UInt32 p1 = src[1];
                  dest[0] = p0;
                  dest[1] = p1;
                #endif
                }
                hash++;
                pos++;
                cur++;
                pb++;
                if (hash == size || *hash != delta || pb[lenLimit] != cur[lenLimit] || d >= limit)
                  break;
              }
            }
            #endif

            break;
          }
        }
      }
      {
        const UInt32 curMatch = (UInt32)pos - delta;
        if (pb[len] < cur[len])
        {
          delta = pair[1];
          *ptr1 = curMatch;
          ptr1 = pair + 1;
          len1 = len;
        }
        else
        {
          delta = *pair;
          *ptr0 = curMatch;
          ptr0 = pair;
          len0 = len;
        }

        {
          if (delta >= curMatch)
            return NULL;
          delta = (UInt32)pos - delta;
          if (delta >= cbs
              // delta >= _cyclicBufferSize || delta >= pos
              || --cutValue == 0)
          {
            *ptr0 = *ptr1 = kEmptyHashValue;
            _distances[-1] = (UInt32)(d - _distances);
            break;
          }
        }
      }
    }
  } // for (tree iterations)
}
    pos++;
    _cyclicBufferPos++;
    cur++;
  }
  while (d < limit);
  *posRes = (UInt32)pos;
  return d;
}
*/

/* ================ unit: C/Lzma2DecMt.c ================ */
/* Lzma2DecMt.c -- LZMA2 Decoder Multi-thread
2023-04-13 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO
// #define Z7_ST

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef Z7_ST
// amalgamation: header emitted in prologue

#define LZMA2DECMT_OUT_BLOCK_MAX_DEFAULT (1 << 28)
#endif


#ifndef Z7_ST
#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif
#define PRF_STR(s) PRF(printf("\n" s "\n");)
#define PRF_STR_INT_2(s, d1, d2) PRF(printf("\n" s " %d %d\n", (unsigned)d1, (unsigned)d2);)
#endif


void Lzma2DecMtProps_Init(CLzma2DecMtProps *p)
{
  p->inBufSize_ST = 1 << 20;
  p->outStep_ST = 1 << 20;

  #ifndef Z7_ST
  p->numThreads = 1;
  p->inBufSize_MT = 1 << 18;
  p->outBlockMax = LZMA2DECMT_OUT_BLOCK_MAX_DEFAULT;
  p->inBlockMax = p->outBlockMax + p->outBlockMax / 16;
  #endif
}



#ifndef Z7_ST

/* ---------- CLzma2DecMtThread ---------- */

typedef struct
{
  CLzma2Dec dec;
  Byte dec_created;
  Byte needInit;
  
  Byte *outBuf;
  size_t outBufSize;

  EMtDecParseState state;
  ELzma2ParseStatus parseStatus;

  size_t inPreSize;
  size_t outPreSize;

  size_t inCodeSize;
  size_t outCodeSize;
  SRes codeRes;

  CAlignOffsetAlloc alloc;

  Byte mtPad[1 << 7];
} CLzma2DecMtThread;

#endif


/* ---------- CLzma2DecMt ---------- */

struct CLzma2DecMt
{
  // ISzAllocPtr alloc;
  ISzAllocPtr allocMid;

  CAlignOffsetAlloc alignOffsetAlloc;
  CLzma2DecMtProps props;
  Byte prop;
  
  ISeqInStreamPtr inStream;
  ISeqOutStreamPtr outStream;
  ICompressProgressPtr progress;

  BoolInt finishMode;
  BoolInt outSize_Defined;
  UInt64 outSize;

  UInt64 outProcessed;
  UInt64 inProcessed;
  BoolInt readWasFinished;
  SRes readRes;

  Byte *inBuf;
  size_t inBufSize;
  Byte dec_created;
  CLzma2Dec dec;

  size_t inPos;
  size_t inLim;

  #ifndef Z7_ST
  UInt64 outProcessed_Parse;
  BoolInt mtc_WasConstructed;
  CMtDec mtc;
  CLzma2DecMtThread coders[MTDEC_THREADS_MAX];
  #endif
};



CLzma2DecMtHandle Lzma2DecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid)
{
  CLzma2DecMt *p = (CLzma2DecMt *)ISzAlloc_Alloc(alloc, sizeof(CLzma2DecMt));
  if (!p)
    return NULL;
  
  // p->alloc = alloc;
  p->allocMid = allocMid;

  AlignOffsetAlloc_CreateVTable(&p->alignOffsetAlloc);
  p->alignOffsetAlloc.numAlignBits = 7;
  p->alignOffsetAlloc.offset = 0;
  p->alignOffsetAlloc.baseAlloc = alloc;

  p->inBuf = NULL;
  p->inBufSize = 0;
  p->dec_created = False;

  // Lzma2DecMtProps_Init(&p->props);

  #ifndef Z7_ST
  p->mtc_WasConstructed = False;
  {
    unsigned i;
    for (i = 0; i < MTDEC_THREADS_MAX; i++)
    {
      CLzma2DecMtThread *t = &p->coders[i];
      t->dec_created = False;
      t->outBuf = NULL;
      t->outBufSize = 0;
    }
  }
  #endif

  return (CLzma2DecMtHandle)(void *)p;
}


#ifndef Z7_ST

static void Lzma2DecMt_FreeOutBufs(CLzma2DecMt *p)
{
  unsigned i;
  for (i = 0; i < MTDEC_THREADS_MAX; i++)
  {
    CLzma2DecMtThread *t = &p->coders[i];
    if (t->outBuf)
    {
      ISzAlloc_Free(p->allocMid, t->outBuf);
      t->outBuf = NULL;
      t->outBufSize = 0;
    }
  }
}

#endif


static void Lzma2DecMt_FreeSt(CLzma2DecMt *p)
{
  if (p->dec_created)
  {
    Lzma2Dec_Free(&p->dec, &p->alignOffsetAlloc.vt);
    p->dec_created = False;
  }
  if (p->inBuf)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBuf = NULL;
  }
  p->inBufSize = 0;
}


// #define GET_CLzma2DecMt_p CLzma2DecMt *p = (CLzma2DecMt *)(void *)pp;

void Lzma2DecMt_Destroy(CLzma2DecMtHandle p)
{
  // GET_CLzma2DecMt_p

  Lzma2DecMt_FreeSt(p);

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
      CLzma2DecMtThread *t = &p->coders[i];
      if (t->dec_created)
      {
        // we don't need to free dict here
        Lzma2Dec_FreeProbs(&t->dec, &t->alloc.vt); // p->alloc !!!
        t->dec_created = False;
      }
    }
  }
  Lzma2DecMt_FreeOutBufs(p);

  #endif

  ISzAlloc_Free(p->alignOffsetAlloc.baseAlloc, p);
}



#ifndef Z7_ST

static void Lzma2DecMt_MtCallback_Parse(void *obj, unsigned coderIndex, CMtDecCallbackInfo *cc)
{
  CLzma2DecMt *me = (CLzma2DecMt *)obj;
  CLzma2DecMtThread *t = &me->coders[coderIndex];

  PRF_STR_INT_2("Parse", coderIndex, cc->srcSize)

  cc->state = MTDEC_PARSE_CONTINUE;

  if (cc->startCall)
  {
    if (!t->dec_created)
    {
      Lzma2Dec_CONSTRUCT(&t->dec)
      t->dec_created = True;
      AlignOffsetAlloc_CreateVTable(&t->alloc);
      {
        /* (1 << 12) is expected size of one way in data cache.
           We optimize alignment for cache line size of 128 bytes and smaller */
        const unsigned kNumAlignBits = 12;
        const unsigned kNumCacheLineBits = 7; /* <= kNumAlignBits */
        t->alloc.numAlignBits = kNumAlignBits;
        t->alloc.offset = ((UInt32)coderIndex * (((unsigned)1 << 11) + (1 << 8) + (1 << 6))) & (((unsigned)1 << kNumAlignBits) - ((unsigned)1 << kNumCacheLineBits));
        t->alloc.baseAlloc = me->alignOffsetAlloc.baseAlloc;
      }
    }
    Lzma2Dec_Init(&t->dec);
    
    t->inPreSize = 0;
    t->outPreSize = 0;
    // t->blockWasFinished = False;
    // t->finishedWithMark = False;
    t->parseStatus = (ELzma2ParseStatus)LZMA_STATUS_NOT_SPECIFIED;
    t->state = MTDEC_PARSE_CONTINUE;

    t->inCodeSize = 0;
    t->outCodeSize = 0;
    t->codeRes = SZ_OK;

    // (cc->srcSize == 0) is allowed
  }

  {
    ELzma2ParseStatus status;
    BoolInt overflow;
    UInt32 unpackRem = 0;
    
    int checkFinishBlock = True;
    size_t limit = me->props.outBlockMax;
    if (me->outSize_Defined)
    {
      UInt64 rem = me->outSize - me->outProcessed_Parse;
      if (limit >= rem)
      {
        limit = (size_t)rem;
        if (!me->finishMode)
          checkFinishBlock = False;
      }
    }

    // checkFinishBlock = False, if we want to decode partial data
    // that must be finished at position <= outBlockMax.

    {
      const size_t srcOrig = cc->srcSize;
      SizeT srcSize_Point = 0;
      SizeT dicPos_Point = 0;
      
      cc->srcSize = 0;
      overflow = False;

      for (;;)
      {
        SizeT srcCur = (SizeT)(srcOrig - cc->srcSize);
        
        status = Lzma2Dec_Parse(&t->dec,
            (SizeT)limit - t->dec.decoder.dicPos,
            cc->src + cc->srcSize, &srcCur,
            checkFinishBlock);

        cc->srcSize += srcCur;

        if (status == LZMA2_PARSE_STATUS_NEW_CHUNK)
        {
          if (t->dec.unpackSize > me->props.outBlockMax - t->dec.decoder.dicPos)
          {
            overflow = True;
            break;
          }
          continue;
        }
        
        if (status == LZMA2_PARSE_STATUS_NEW_BLOCK)
        {
          if (t->dec.decoder.dicPos == 0)
            continue;
          // we decode small blocks in one thread
          if (t->dec.decoder.dicPos >= (1 << 14))
            break;
          dicPos_Point = t->dec.decoder.dicPos;
          srcSize_Point = (SizeT)cc->srcSize;
          continue;
        }

        if ((int)status == LZMA_STATUS_NOT_FINISHED && checkFinishBlock
            // && limit == t->dec.decoder.dicPos
            // && limit == me->props.outBlockMax
            )
        {
          overflow = True;
          break;
        }
        
        unpackRem = Lzma2Dec_GetUnpackExtra(&t->dec);
        break;
      }

      if (dicPos_Point != 0
          && (int)status != LZMA2_PARSE_STATUS_NEW_BLOCK
          && (int)status != LZMA_STATUS_FINISHED_WITH_MARK
          && (int)status != LZMA_STATUS_NOT_SPECIFIED)
      {
        // we revert to latest newBlock state
        status = LZMA2_PARSE_STATUS_NEW_BLOCK;
        unpackRem = 0;
        t->dec.decoder.dicPos = dicPos_Point;
        cc->srcSize = srcSize_Point;
        overflow = False;
      }
    }

    t->inPreSize += cc->srcSize;
    t->parseStatus = status;

    if (overflow)
      cc->state = MTDEC_PARSE_OVERFLOW;
    else
    {
      size_t dicPos = t->dec.decoder.dicPos;

      if ((int)status != LZMA_STATUS_NEEDS_MORE_INPUT)
      {
        if (status == LZMA2_PARSE_STATUS_NEW_BLOCK)
        {
          cc->state = MTDEC_PARSE_NEW;
          cc->srcSize--; // we don't need control byte of next block
          t->inPreSize--;
        }
        else
        {
          cc->state = MTDEC_PARSE_END;
          if ((int)status != LZMA_STATUS_FINISHED_WITH_MARK)
          {
            // (status == LZMA_STATUS_NOT_SPECIFIED)
            // (status == LZMA_STATUS_NOT_FINISHED)
            if (unpackRem != 0)
            {
              /* we also reserve space for max possible number of output bytes of current LZMA chunk */
              size_t rem = limit - dicPos;
              if (rem > unpackRem)
                rem = unpackRem;
              dicPos += rem;
            }
          }
        }
    
        me->outProcessed_Parse += dicPos;
      }
      
      cc->outPos = dicPos;
      t->outPreSize = (size_t)dicPos;
    }

    t->state = cc->state;
    return;
  }
}


static SRes Lzma2DecMt_MtCallback_PreCode(void *pp, unsigned coderIndex)
{
  CLzma2DecMt *me = (CLzma2DecMt *)pp;
  CLzma2DecMtThread *t = &me->coders[coderIndex];
  Byte *dest = t->outBuf;

  if (t->inPreSize == 0)
  {
    t->codeRes = SZ_ERROR_DATA;
    return t->codeRes;
  }

  if (!dest || t->outBufSize < t->outPreSize)
  {
    if (dest)
    {
      ISzAlloc_Free(me->allocMid, dest);
      t->outBuf = NULL;
      t->outBufSize = 0;
    }

    dest = (Byte *)ISzAlloc_Alloc(me->allocMid, t->outPreSize
        // + (1 << 28)
        );
    // Sleep(200);
    if (!dest)
      return SZ_ERROR_MEM;
    t->outBuf = dest;
    t->outBufSize = t->outPreSize;
  }

  t->dec.decoder.dic = dest;
  t->dec.decoder.dicBufSize = (SizeT)t->outPreSize;

  t->needInit = True;

  return Lzma2Dec_AllocateProbs(&t->dec, me->prop, &t->alloc.vt); // alloc.vt
}


static SRes Lzma2DecMt_MtCallback_Code(void *pp, unsigned coderIndex,
    const Byte *src, size_t srcSize, int srcFinished,
    // int finished, int blockFinished,
    UInt64 *inCodePos, UInt64 *outCodePos, int *stop)
{
  CLzma2DecMt *me = (CLzma2DecMt *)pp;
  CLzma2DecMtThread *t = &me->coders[coderIndex];

  UNUSED_VAR(srcFinished)

  PRF_STR_INT_2("Code", coderIndex, srcSize)

  *inCodePos = t->inCodeSize;
  *outCodePos = 0;
  *stop = True;

  if (t->needInit)
  {
    Lzma2Dec_Init(&t->dec);
    t->needInit = False;
  }

  {
    ELzmaStatus status;
    SizeT srcProcessed = (SizeT)srcSize;
    BoolInt blockWasFinished =
        ((int)t->parseStatus == LZMA_STATUS_FINISHED_WITH_MARK
        || t->parseStatus == LZMA2_PARSE_STATUS_NEW_BLOCK);
    
    SRes res = Lzma2Dec_DecodeToDic(&t->dec,
        (SizeT)t->outPreSize,
        src, &srcProcessed,
        blockWasFinished ? LZMA_FINISH_END : LZMA_FINISH_ANY,
        &status);

    t->codeRes = res;

    t->inCodeSize += srcProcessed;
    *inCodePos = t->inCodeSize;
    t->outCodeSize = t->dec.decoder.dicPos;
    *outCodePos = t->dec.decoder.dicPos;

    if (res != SZ_OK)
      return res;

    if (srcProcessed == srcSize)
      *stop = False;

    if (blockWasFinished)
    {
      if (srcSize != srcProcessed)
        return SZ_ERROR_FAIL;
      
      if (t->inPreSize == t->inCodeSize)
      {
        if (t->outPreSize != t->outCodeSize)
          return SZ_ERROR_FAIL;
        *stop = True;
      }
    }
    else
    {
      if (t->outPreSize == t->outCodeSize)
        *stop = True;
    }

    return SZ_OK;
  }
}


#define LZMA2DECMT_STREAM_WRITE_STEP (1 << 24)

static SRes Lzma2DecMt_MtCallback_Write(void *pp, unsigned coderIndex,
    BoolInt needWriteToStream,
    const Byte *src, size_t srcSize, BoolInt isCross,
    BoolInt *needContinue, BoolInt *canRecode)
{
  CLzma2DecMt *me = (CLzma2DecMt *)pp;
  const CLzma2DecMtThread *t = &me->coders[coderIndex];
  size_t size = t->outCodeSize;
  const Byte *data = t->outBuf;
  BoolInt needContinue2 = True;

  UNUSED_VAR(src)
  UNUSED_VAR(srcSize)
  UNUSED_VAR(isCross)

  PRF_STR_INT_2("Write", coderIndex, srcSize)

  *needContinue = False;
  *canRecode = True;

  if (
      // t->parseStatus == LZMA_STATUS_FINISHED_WITH_MARK
         t->state == MTDEC_PARSE_OVERFLOW
      || t->state == MTDEC_PARSE_END)
    needContinue2 = False;


  if (!needWriteToStream)
    return SZ_OK;

  me->mtc.inProcessed += t->inCodeSize;

  if (t->codeRes == SZ_OK)
  if ((int)t->parseStatus == LZMA_STATUS_FINISHED_WITH_MARK
      || t->parseStatus == LZMA2_PARSE_STATUS_NEW_BLOCK)
  if (t->outPreSize != t->outCodeSize
      || t->inPreSize != t->inCodeSize)
    return SZ_ERROR_FAIL;

  *canRecode = False;
    
  if (me->outStream)
  {
    for (;;)
    {
      size_t cur = size;
      size_t written;
      if (cur > LZMA2DECMT_STREAM_WRITE_STEP)
        cur = LZMA2DECMT_STREAM_WRITE_STEP;

      written = ISeqOutStream_Write(me->outStream, data, cur);
      
      me->outProcessed += written;
      // me->mtc.writtenTotal += written;
      if (written != cur)
        return SZ_ERROR_WRITE;
      data += cur;
      size -= cur;
      if (size == 0)
      {
        *needContinue = needContinue2;
        return SZ_OK;
      }
      RINOK(MtProgress_ProgressAdd(&me->mtc.mtProgress, 0, 0))
    }
  }
  
  return SZ_ERROR_FAIL;
  /*
  if (size > me->outBufSize)
    return SZ_ERROR_OUTPUT_EOF;
  memcpy(me->outBuf, data, size);
  me->outBufSize -= size;
  me->outBuf += size;
  *needContinue = needContinue2;
  return SZ_OK;
  */
}

#endif


static SRes Lzma2Dec_Prepare_ST(CLzma2DecMt *p)
{
  if (!p->dec_created)
  {
    Lzma2Dec_CONSTRUCT(&p->dec)
    p->dec_created = True;
  }

  RINOK(Lzma2Dec_Allocate(&p->dec, p->prop, &p->alignOffsetAlloc.vt))

  if (!p->inBuf || p->inBufSize != p->props.inBufSize_ST)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBufSize = 0;
    p->inBuf = (Byte *)ISzAlloc_Alloc(p->allocMid, p->props.inBufSize_ST);
    if (!p->inBuf)
      return SZ_ERROR_MEM;
    p->inBufSize = p->props.inBufSize_ST;
  }

  Lzma2Dec_Init(&p->dec);
  
  return SZ_OK;
}


static SRes Lzma2Dec_Decode_ST(CLzma2DecMt *p
    #ifndef Z7_ST
    , BoolInt tMode
    #endif
    )
{
  SizeT wrPos;
  size_t inPos, inLim;
  const Byte *inData;
  UInt64 inPrev, outPrev;

  CLzma2Dec *dec;

  #ifndef Z7_ST
  if (tMode)
  {
    Lzma2DecMt_FreeOutBufs(p);
    tMode = MtDec_PrepareRead(&p->mtc);
  }
  #endif

  RINOK(Lzma2Dec_Prepare_ST(p))

  dec = &p->dec;

  inPrev = p->inProcessed;
  outPrev = p->outProcessed;

  inPos = 0;
  inLim = 0;
  inData = NULL;
  wrPos = dec->decoder.dicPos;

  for (;;)
  {
    SizeT dicPos;
    SizeT size;
    ELzmaFinishMode finishMode;
    SizeT inProcessed;
    ELzmaStatus status;
    SRes res;

    SizeT outProcessed;
    BoolInt outFinished;
    BoolInt needStop;

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
        p->readRes = ISeqInStream_Read(p->inStream, (void *)(p->inBuf), &inLim);
        // p->readProcessed += inLim;
        // inLim -= 5; p->readWasFinished = True; // for test
        if (inLim == 0 || p->readRes != SZ_OK)
          p->readWasFinished = True;
      }
    }

    dicPos = dec->decoder.dicPos;
    {
      SizeT next = dec->decoder.dicBufSize;
      if (next - wrPos > p->props.outStep_ST)
        next = wrPos + (SizeT)p->props.outStep_ST;
      size = next - dicPos;
    }

    finishMode = LZMA_FINISH_ANY;
    if (p->outSize_Defined)
    {
      const UInt64 rem = p->outSize - p->outProcessed;
      if (size >= rem)
      {
        size = (SizeT)rem;
        if (p->finishMode)
          finishMode = LZMA_FINISH_END;
      }
    }

    inProcessed = (SizeT)(inLim - inPos);
    
    res = Lzma2Dec_DecodeToDic(dec, dicPos + size, inData + inPos, &inProcessed, finishMode, &status);

    inPos += inProcessed;
    p->inProcessed += inProcessed;
    outProcessed = dec->decoder.dicPos - dicPos;
    p->outProcessed += outProcessed;

    outFinished = (p->outSize_Defined && p->outSize <= p->outProcessed);

    needStop = (res != SZ_OK
        || (inProcessed == 0 && outProcessed == 0)
        || status == LZMA_STATUS_FINISHED_WITH_MARK
        || (!p->finishMode && outFinished));

    if (needStop || outProcessed >= size)
    {
      SRes res2;
      {
        size_t writeSize = dec->decoder.dicPos - wrPos;
        size_t written = ISeqOutStream_Write(p->outStream, dec->decoder.dic + wrPos, writeSize);
        res2 = (written == writeSize) ? SZ_OK : SZ_ERROR_WRITE;
      }

      if (dec->decoder.dicPos == dec->decoder.dicBufSize)
        dec->decoder.dicPos = 0;
      wrPos = dec->decoder.dicPos;

      RINOK(res2)

      if (needStop)
      {
        if (res != SZ_OK)
          return res;

        if (status == LZMA_STATUS_FINISHED_WITH_MARK)
        {
          if (p->finishMode)
          {
            if (p->outSize_Defined && p->outSize != p->outProcessed)
              return SZ_ERROR_DATA;
          }
          return SZ_OK;
        }

        if (!p->finishMode && outFinished)
          return SZ_OK;

        if (status == LZMA_STATUS_NEEDS_MORE_INPUT)
          return SZ_ERROR_INPUT_EOF;
        
        return SZ_ERROR_DATA;
      }
    }
    
    if (p->progress)
    {
      UInt64 inDelta = p->inProcessed - inPrev;
      UInt64 outDelta = p->outProcessed - outPrev;
      if (inDelta >= (1 << 22) || outDelta >= (1 << 22))
      {
        RINOK(ICompressProgress_Progress(p->progress, p->inProcessed, p->outProcessed))
        inPrev = p->inProcessed;
        outPrev = p->outProcessed;
      }
    }
  }
}



SRes Lzma2DecMt_Decode(CLzma2DecMtHandle p,
    Byte prop,
    const CLzma2DecMtProps *props,
    ISeqOutStreamPtr outStream, const UInt64 *outDataSize, int finishMode,
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    // const Byte *inData, size_t inDataSize,
    UInt64 *inProcessed,
    // UInt64 *outProcessed,
    int *isMT,
    ICompressProgressPtr progress)
{
  // GET_CLzma2DecMt_p
  #ifndef Z7_ST
  BoolInt tMode;
  #endif

  *inProcessed = 0;

  if (prop > 40)
    return SZ_ERROR_UNSUPPORTED;

  p->prop = prop;
  p->props = *props;

  p->inStream = inStream;
  p->outStream = outStream;
  p->progress = progress;

  p->outSize = 0;
  p->outSize_Defined = False;
  if (outDataSize)
  {
    p->outSize_Defined = True;
    p->outSize = *outDataSize;
  }
  p->finishMode = finishMode;

  p->outProcessed = 0;
  p->inProcessed = 0;

  p->readWasFinished = False;
  p->readRes = SZ_OK;

  *isMT = False;

  
  #ifndef Z7_ST

  tMode = False;

  // p->mtc.parseRes = SZ_OK;

  // p->mtc.numFilledThreads = 0;
  // p->mtc.crossStart = 0;
  // p->mtc.crossEnd = 0;
  // p->mtc.allocError_for_Read_BlockIndex = 0;
  // p->mtc.isAllocError = False;

  if (p->props.numThreads > 1)
  {
    IMtDecCallback2 vt;

    Lzma2DecMt_FreeSt(p);

    p->outProcessed_Parse = 0;

    if (!p->mtc_WasConstructed)
    {
      p->mtc_WasConstructed = True;
      MtDec_Construct(&p->mtc);
    }
    
    p->mtc.progress = progress;
    p->mtc.inStream = inStream;

    // p->outBuf = NULL;
    // p->outBufSize = 0;
    /*
    if (!outStream)
    {
      // p->outBuf = outBuf;
      // p->outBufSize = *outBufSize;
      // *outBufSize = 0;
      return SZ_ERROR_PARAM;
    }
    */

    // p->mtc.inBlockMax = p->props.inBlockMax;
    p->mtc.alloc = &p->alignOffsetAlloc.vt;
      // p->alignOffsetAlloc.baseAlloc;
    // p->mtc.inData = inData;
    // p->mtc.inDataSize = inDataSize;
    p->mtc.mtCallback = &vt;
    p->mtc.mtCallbackObject = p;

    p->mtc.inBufSize = p->props.inBufSize_MT;

    p->mtc.numThreadsMax = p->props.numThreads;

    *isMT = True;

    vt.Parse = Lzma2DecMt_MtCallback_Parse;
    vt.PreCode = Lzma2DecMt_MtCallback_PreCode;
    vt.Code = Lzma2DecMt_MtCallback_Code;
    vt.Write = Lzma2DecMt_MtCallback_Write;

    {
      BoolInt needContinue = False;

      SRes res = MtDec_Code(&p->mtc);

      /*
      if (!outStream)
        *outBufSize = p->outBuf - outBuf;
      */

      *inProcessed = p->mtc.inProcessed;

      needContinue = False;

      if (res == SZ_OK)
      {
        if (p->mtc.mtProgress.res != SZ_OK)
          res = p->mtc.mtProgress.res;
        else
          needContinue = p->mtc.needContinue;
      }

      if (!needContinue)
      {
        if (res == SZ_OK)
          return p->mtc.readRes;
        return res;
      }

      tMode = True;
      p->readRes = p->mtc.readRes;
      p->readWasFinished = p->mtc.readWasFinished;
      p->inProcessed = p->mtc.inProcessed;
      
      PRF_STR("----- decoding ST -----")
    }
  }

  #endif


  *isMT = False;

  {
    SRes res = Lzma2Dec_Decode_ST(p
        #ifndef Z7_ST
        , tMode
        #endif
        );

    *inProcessed = p->inProcessed;

    // res = SZ_OK; // for test
    if (res == SZ_ERROR_INPUT_EOF)
    {
      if (p->readRes != SZ_OK)
        res = p->readRes;
    }
    else if (res == SZ_OK && p->readRes != SZ_OK)
      res = p->readRes;
    
    /*
    #ifndef Z7_ST
    if (res == SZ_OK && tMode && p->mtc.parseRes != SZ_OK)
      res = p->mtc.parseRes;
    #endif
    */
    
    return res;
  }
}


/* ---------- Read from CLzma2DecMtHandle Interface ---------- */

SRes Lzma2DecMt_Init(CLzma2DecMtHandle p,
    Byte prop,
    const CLzma2DecMtProps *props,
    const UInt64 *outDataSize, int finishMode,
    ISeqInStreamPtr inStream)
{
  // GET_CLzma2DecMt_p

  if (prop > 40)
    return SZ_ERROR_UNSUPPORTED;

  p->prop = prop;
  p->props = *props;

  p->inStream = inStream;

  p->outSize = 0;
  p->outSize_Defined = False;
  if (outDataSize)
  {
    p->outSize_Defined = True;
    p->outSize = *outDataSize;
  }
  p->finishMode = finishMode;

  p->outProcessed = 0;
  p->inProcessed = 0;

  p->inPos = 0;
  p->inLim = 0;

  return Lzma2Dec_Prepare_ST(p);
}


SRes Lzma2DecMt_Read(CLzma2DecMtHandle p,
    Byte *data, size_t *outSize,
    UInt64 *inStreamProcessed)
{
  // GET_CLzma2DecMt_p
  ELzmaFinishMode finishMode;
  SRes readRes;
  size_t size = *outSize;

  *outSize = 0;
  *inStreamProcessed = 0;

  finishMode = LZMA_FINISH_ANY;
  if (p->outSize_Defined)
  {
    const UInt64 rem = p->outSize - p->outProcessed;
    if (size >= rem)
    {
      size = (size_t)rem;
      if (p->finishMode)
        finishMode = LZMA_FINISH_END;
    }
  }

  readRes = SZ_OK;

  for (;;)
  {
    SizeT inCur;
    SizeT outCur;
    ELzmaStatus status;
    SRes res;

    if (p->inPos == p->inLim && readRes == SZ_OK)
    {
      p->inPos = 0;
      p->inLim = p->props.inBufSize_ST;
      readRes = ISeqInStream_Read(p->inStream, p->inBuf, &p->inLim);
    }

    inCur = (SizeT)(p->inLim - p->inPos);
    outCur = (SizeT)size;

    res = Lzma2Dec_DecodeToBuf(&p->dec, data, &outCur,
        p->inBuf + p->inPos, &inCur, finishMode, &status);
    
    p->inPos += inCur;
    p->inProcessed += inCur;
    *inStreamProcessed += inCur;
    p->outProcessed += outCur;
    *outSize += outCur;
    size -= outCur;
    data += outCur;
    
    if (res != 0)
      return res;
    
    /*
    if (status == LZMA_STATUS_FINISHED_WITH_MARK)
      return readRes;

    if (size == 0 && status != LZMA_STATUS_NEEDS_MORE_INPUT)
    {
      if (p->finishMode && p->outSize_Defined && p->outProcessed >= p->outSize)
        return SZ_ERROR_DATA;
      return readRes;
    }
    */

    if (inCur == 0 && outCur == 0)
      return readRes;
  }
}

#undef PRF
#undef PRF_STR
#undef PRF_STR_INT_2

/* ================ unit: C/Lzma2Enc.c ================ */
/* Lzma2Enc.c -- LZMA2 Encoder
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

/* #define Z7_ST */

// amalgamation: header emitted in prologue

#ifndef Z7_ST
// amalgamation: header emitted in prologue
#else
#define MTCODER_THREADS_MAX 1
#endif

#define LZMA2_CONTROL_LZMA (1 << 7)
#define LZMA2_CONTROL_COPY_NO_RESET 2
#define LZMA2_CONTROL_COPY_RESET_DIC 1
#define LZMA2_CONTROL_EOF 0

#define LZMA2_LCLP_MAX 4

#define LZMA2_DIC_SIZE_FROM_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

#define LZMA2_PACK_SIZE_MAX (1 << 16)
#define LZMA2_COPY_CHUNK_SIZE LZMA2_PACK_SIZE_MAX
#define LZMA2_UNPACK_SIZE_MAX (1 << 21)
#define LZMA2_KEEP_WINDOW_SIZE LZMA2_UNPACK_SIZE_MAX

#define LZMA2_CHUNK_SIZE_COMPRESSED_MAX ((1 << 16) + 16)


#define PRF(x) /* x */


/* ---------- CLimitedSeqInStream ---------- */

typedef struct
{
  ISeqInStream vt;
  ISeqInStreamPtr realStream;
  UInt64 limit;
  UInt64 processed;
  int finished;
} CLimitedSeqInStream;

static void LimitedSeqInStream_Init(CLimitedSeqInStream *p)
{
  p->limit = (UInt64)(Int64)-1;
  p->processed = 0;
  p->finished = 0;
}

static SRes LimitedSeqInStream_Read(ISeqInStreamPtr pp, void *data, size_t *size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CLimitedSeqInStream)
  size_t size2 = *size;
  SRes res = SZ_OK;
  
  if (p->limit != (UInt64)(Int64)-1)
  {
    const UInt64 rem = p->limit - p->processed;
    if (size2 > rem)
      size2 = (size_t)rem;
  }
  if (size2 != 0)
  {
    res = ISeqInStream_Read(p->realStream, data, &size2);
    p->finished = (size2 == 0 ? 1 : 0);
    p->processed += size2;
  }
  *size = size2;
  return res;
}


/* ---------- CLzma2EncInt ---------- */

typedef struct
{
  CLzmaEncHandle enc;
  Byte propsAreSet;
  Byte propsByte;
  Byte needInitState;
  Byte needInitProp;
  UInt64 srcPos;
} CLzma2EncInt;


static SRes Lzma2EncInt_InitStream(CLzma2EncInt *p, const CLzma2EncProps *props)
{
  if (!p->propsAreSet)
  {
    SizeT propsSize = LZMA_PROPS_SIZE;
    Byte propsEncoded[LZMA_PROPS_SIZE];
    RINOK(LzmaEnc_SetProps(p->enc, &props->lzmaProps))
    RINOK(LzmaEnc_WriteProperties(p->enc, propsEncoded, &propsSize))
    p->propsByte = propsEncoded[0];
    p->propsAreSet = True;
  }
  return SZ_OK;
}

static void Lzma2EncInt_InitBlock(CLzma2EncInt *p)
{
  p->srcPos = 0;
  p->needInitState = True;
  p->needInitProp = True;
}


SRes LzmaEnc_PrepareForLzma2(CLzmaEncHandle p, ISeqInStreamPtr inStream, UInt32 keepWindowSize,
    ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_MemPrepare(CLzmaEncHandle p, const Byte *src, SizeT srcLen,
    UInt32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_CodeOneMemBlock(CLzmaEncHandle p, BoolInt reInit,
    Byte *dest, size_t *destLen, UInt32 desiredPackSize, UInt32 *unpackSize);
const Byte *LzmaEnc_GetCurBuf(CLzmaEncHandle p);
void LzmaEnc_Finish(CLzmaEncHandle p);
void LzmaEnc_SaveState(CLzmaEncHandle p);
void LzmaEnc_RestoreState(CLzmaEncHandle p);

/*
UInt32 LzmaEnc_GetNumAvailableBytes(CLzmaEncHandle p);
*/

static SRes Lzma2EncInt_EncodeSubblock(CLzma2EncInt *p, Byte *outBuf,
    size_t *packSizeRes, ISeqOutStreamPtr outStream)
{
  size_t packSizeLimit = *packSizeRes;
  size_t packSize = packSizeLimit;
  UInt32 unpackSize = LZMA2_UNPACK_SIZE_MAX;
  unsigned lzHeaderSize = 5 + (p->needInitProp ? 1 : 0);
  BoolInt useCopyBlock;
  SRes res;

  *packSizeRes = 0;
  if (packSize < lzHeaderSize)
    return SZ_ERROR_OUTPUT_EOF;
  packSize -= lzHeaderSize;
  
  LzmaEnc_SaveState(p->enc);
  res = LzmaEnc_CodeOneMemBlock(p->enc, p->needInitState,
      outBuf + lzHeaderSize, &packSize, LZMA2_PACK_SIZE_MAX, &unpackSize);
  
  PRF(printf("\npackSize = %7d unpackSize = %7d  ", packSize, unpackSize));

  if (unpackSize == 0)
    return res;

  if (res == SZ_OK)
    useCopyBlock = (packSize + 2 >= unpackSize || packSize > (1 << 16));
  else
  {
    if (res != SZ_ERROR_OUTPUT_EOF)
      return res;
    res = SZ_OK;
    useCopyBlock = True;
  }

  if (useCopyBlock)
  {
    size_t destPos = 0;
    PRF(printf("################# COPY           "));

    while (unpackSize > 0)
    {
      const UInt32 u = (unpackSize < LZMA2_COPY_CHUNK_SIZE) ? unpackSize : LZMA2_COPY_CHUNK_SIZE;
      if (packSizeLimit - destPos < u + 3)
        return SZ_ERROR_OUTPUT_EOF;
      outBuf[destPos++] = (Byte)(p->srcPos == 0 ? LZMA2_CONTROL_COPY_RESET_DIC : LZMA2_CONTROL_COPY_NO_RESET);
      outBuf[destPos++] = (Byte)((u - 1) >> 8);
      outBuf[destPos++] = (Byte)(u - 1);
      memcpy(outBuf + destPos, LzmaEnc_GetCurBuf(p->enc) - unpackSize, u);
      unpackSize -= u;
      destPos += u;
      p->srcPos += u;
      
      if (outStream)
      {
        *packSizeRes += destPos;
        if (ISeqOutStream_Write(outStream, outBuf, destPos) != destPos)
          return SZ_ERROR_WRITE;
        destPos = 0;
      }
      else
        *packSizeRes = destPos;
      /* needInitState = True; */
    }
    
    LzmaEnc_RestoreState(p->enc);
    return SZ_OK;
  }

  {
    size_t destPos = 0;
    const UInt32 u = unpackSize - 1;
    const UInt32 pm = (UInt32)(packSize - 1);
    const unsigned mode = (p->srcPos == 0) ? 3 : (p->needInitState ? (p->needInitProp ? 2 : 1) : 0);

    PRF(printf("               "));

    outBuf[destPos++] = (Byte)(LZMA2_CONTROL_LZMA | (mode << 5) | ((u >> 16) & 0x1F));
    outBuf[destPos++] = (Byte)(u >> 8);
    outBuf[destPos++] = (Byte)u;
    outBuf[destPos++] = (Byte)(pm >> 8);
    outBuf[destPos++] = (Byte)pm;
    
    if (p->needInitProp)
      outBuf[destPos++] = p->propsByte;
    
    p->needInitProp = False;
    p->needInitState = False;
    destPos += packSize;
    p->srcPos += unpackSize;

    if (outStream)
      if (ISeqOutStream_Write(outStream, outBuf, destPos) != destPos)
        return SZ_ERROR_WRITE;
    
    *packSizeRes = destPos;
    return SZ_OK;
  }
}


/* ---------- Lzma2 Props ---------- */

void Lzma2EncProps_Init(CLzma2EncProps *p)
{
  LzmaEncProps_Init(&p->lzmaProps);
  p->blockSize = LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO;
  p->numBlockThreads_Reduced = -1;
  p->numBlockThreads_Max = -1;
  p->numTotalThreads = -1;
  p->numThreadGroups = 0;
}

void Lzma2EncProps_Normalize(CLzma2EncProps *p)
{
  UInt64 fileSize;
  int t1, t1n, t2, t2r, t3;
  {
    CLzmaEncProps lzmaProps = p->lzmaProps;
    LzmaEncProps_Normalize(&lzmaProps);
    t1n = lzmaProps.numThreads;
  }

  t1 = p->lzmaProps.numThreads;
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

  p->lzmaProps.numThreads = t1;

  t2r = t2;

  fileSize = p->lzmaProps.reduceSize;

  if (   p->blockSize != LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID
      && p->blockSize != LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO
      && (p->blockSize < fileSize || fileSize == (UInt64)(Int64)-1))
    p->lzmaProps.reduceSize = p->blockSize;

  LzmaEncProps_Normalize(&p->lzmaProps);

  p->lzmaProps.reduceSize = fileSize;

  t1 = p->lzmaProps.numThreads;

  if (p->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID)
  {
    t2r = t2 = 1;
    t3 = t1;
  }
  else if (p->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO && t2 <= 1)
  {
    /* if there is no block multi-threading, we use SOLID block */
    p->blockSize = LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID;
  }
  else
  {
    if (p->blockSize == LZMA2_ENC_PROPS_BLOCK_SIZE_AUTO)
    {
      const UInt32 kMinSize = (UInt32)1 << 20;
      const UInt32 kMaxSize = (UInt32)1 << 28;
      const UInt32 dictSize = p->lzmaProps.dictSize;
      UInt64 blockSize = (UInt64)dictSize << 2;
      if (blockSize < kMinSize) blockSize = kMinSize;
      if (blockSize > kMaxSize) blockSize = kMaxSize;
      if (blockSize < dictSize) blockSize = dictSize;
      blockSize += (kMinSize - 1);
      blockSize &= ~(UInt64)(kMinSize - 1);
      p->blockSize = blockSize;
    }
    
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


static SRes Progress(ICompressProgressPtr p, UInt64 inSize, UInt64 outSize)
{
  return (p && ICompressProgress_Progress(p, inSize, outSize) != SZ_OK) ? SZ_ERROR_PROGRESS : SZ_OK;
}


/* ---------- Lzma2 ---------- */

struct CLzma2Enc
{
  Byte propEncoded;
  CLzma2EncProps props;
  UInt64 expectedDataSize;
  
  Byte *tempBufLzma;

  ISzAllocPtr alloc;
  ISzAllocPtr allocBig;

  CLzma2EncInt coders[MTCODER_THREADS_MAX];

  #ifndef Z7_ST
  
  ISeqOutStreamPtr outStream;
  Byte *outBuf;
  size_t outBuf_Rem;   /* remainder in outBuf */

  size_t outBufSize;   /* size of allocated outBufs[i] */
  size_t outBufsDataSizes[MTCODER_BLOCKS_MAX];
  BoolInt mtCoder_WasConstructed;
  CMtCoder mtCoder;
  Byte *outBufs[MTCODER_BLOCKS_MAX];

  #endif
};



CLzma2EncHandle Lzma2Enc_Create(ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  CLzma2Enc *p = (CLzma2Enc *)ISzAlloc_Alloc(alloc, sizeof(CLzma2Enc));
  if (!p)
    return NULL;
  Lzma2EncProps_Init(&p->props);
  Lzma2EncProps_Normalize(&p->props);
  p->expectedDataSize = (UInt64)(Int64)-1;
  p->tempBufLzma = NULL;
  p->alloc = alloc;
  p->allocBig = allocBig;
  {
    unsigned i;
    for (i = 0; i < MTCODER_THREADS_MAX; i++)
      p->coders[i].enc = NULL;
  }
  
  #ifndef Z7_ST
  p->mtCoder_WasConstructed = False;
  {
    unsigned i;
    for (i = 0; i < MTCODER_BLOCKS_MAX; i++)
      p->outBufs[i] = NULL;
    p->outBufSize = 0;
  }
  #endif

  return (CLzma2EncHandle)p;
}


#ifndef Z7_ST

static void Lzma2Enc_FreeOutBufs(CLzma2Enc *p)
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

#endif

// #define GET_CLzma2Enc_p  CLzma2Enc *p = (CLzma2Enc *)(void *)p;

void Lzma2Enc_Destroy(CLzma2EncHandle p)
{
  // GET_CLzma2Enc_p
  unsigned i;
  for (i = 0; i < MTCODER_THREADS_MAX; i++)
  {
    CLzma2EncInt *t = &p->coders[i];
    if (t->enc)
    {
      LzmaEnc_Destroy(t->enc, p->alloc, p->allocBig);
      t->enc = NULL;
    }
  }


  #ifndef Z7_ST
  if (p->mtCoder_WasConstructed)
  {
    MtCoder_Destruct(&p->mtCoder);
    p->mtCoder_WasConstructed = False;
  }
  Lzma2Enc_FreeOutBufs(p);
  #endif

  ISzAlloc_Free(p->alloc, p->tempBufLzma);
  p->tempBufLzma = NULL;

  ISzAlloc_Free(p->alloc, p);
}


SRes Lzma2Enc_SetProps(CLzma2EncHandle p, const CLzma2EncProps *props)
{
  // GET_CLzma2Enc_p
  CLzmaEncProps lzmaProps = props->lzmaProps;
  LzmaEncProps_Normalize(&lzmaProps);
  if (lzmaProps.lc + lzmaProps.lp > LZMA2_LCLP_MAX)
    return SZ_ERROR_PARAM;
  p->props = *props;
  Lzma2EncProps_Normalize(&p->props);
  return SZ_OK;
}


void Lzma2Enc_SetDataSize(CLzma2EncHandle p, UInt64 expectedDataSiize)
{
  // GET_CLzma2Enc_p
  p->expectedDataSize = expectedDataSiize;
}


Byte Lzma2Enc_WriteProperties(CLzma2EncHandle p)
{
  // GET_CLzma2Enc_p
  unsigned i;
  UInt32 dicSize = LzmaEncProps_GetDictSize(&p->props.lzmaProps);
  for (i = 0; i < 40; i++)
    if (dicSize <= LZMA2_DIC_SIZE_FROM_PROP(i))
      break;
  return (Byte)i;
}


static SRes Lzma2Enc_EncodeMt1(
    CLzma2Enc *me,
    CLzma2EncInt *p,
    ISeqOutStreamPtr outStream,
    Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    const Byte *inData, size_t inDataSize,
    int finished,
    ICompressProgressPtr progress)
{
  UInt64 unpackTotal = 0;
  UInt64 packTotal = 0;
  size_t outLim = 0;
  CLimitedSeqInStream limitedInStream;

  if (outBuf)
  {
    outLim = *outBufSize;
    *outBufSize = 0;
  }

  if (!p->enc)
  {
    p->propsAreSet = False;
    p->enc = LzmaEnc_Create(me->alloc);
    if (!p->enc)
      return SZ_ERROR_MEM;
  }

  limitedInStream.realStream = inStream;
  if (inStream)
  {
    limitedInStream.vt.Read = LimitedSeqInStream_Read;
  }
  
  if (!outBuf)
  {
    // outStream version works only in one thread. So we use CLzma2Enc::tempBufLzma
    if (!me->tempBufLzma)
    {
      me->tempBufLzma = (Byte *)ISzAlloc_Alloc(me->alloc, LZMA2_CHUNK_SIZE_COMPRESSED_MAX);
      if (!me->tempBufLzma)
        return SZ_ERROR_MEM;
    }
  }

  RINOK(Lzma2EncInt_InitStream(p, &me->props))

  for (;;)
  {
    SRes res = SZ_OK;
    SizeT inSizeCur = 0;

    Lzma2EncInt_InitBlock(p);
    
    LimitedSeqInStream_Init(&limitedInStream);
    limitedInStream.limit = me->props.blockSize;

    if (inStream)
    {
      UInt64 expected = (UInt64)(Int64)-1;
      // inStream version works only in one thread. So we use CLzma2Enc::expectedDataSize
      if (me->expectedDataSize != (UInt64)(Int64)-1
          && me->expectedDataSize >= unpackTotal)
        expected = me->expectedDataSize - unpackTotal;
      if (me->props.blockSize != LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID
          && expected > me->props.blockSize)
        expected = (size_t)me->props.blockSize;

      LzmaEnc_SetDataSize(p->enc, expected);

      RINOK(LzmaEnc_PrepareForLzma2(p->enc,
          &limitedInStream.vt,
          LZMA2_KEEP_WINDOW_SIZE,
          me->alloc,
          me->allocBig))
    }
    else
    {
      inSizeCur = (SizeT)(inDataSize - (size_t)unpackTotal);
      if (me->props.blockSize != LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID
          && inSizeCur > me->props.blockSize)
        inSizeCur = (SizeT)(size_t)me->props.blockSize;
    
      // LzmaEnc_SetDataSize(p->enc, inSizeCur);
      
      RINOK(LzmaEnc_MemPrepare(p->enc,
          inData + (size_t)unpackTotal, inSizeCur,
          LZMA2_KEEP_WINDOW_SIZE,
          me->alloc,
          me->allocBig))
    }

    for (;;)
    {
      size_t packSize = LZMA2_CHUNK_SIZE_COMPRESSED_MAX;
      if (outBuf)
        packSize = outLim - (size_t)packTotal;
      
      res = Lzma2EncInt_EncodeSubblock(p,
          outBuf ? outBuf + (size_t)packTotal : me->tempBufLzma, &packSize,
          outBuf ? NULL : outStream);
      
      if (res != SZ_OK)
        break;

      packTotal += packSize;
      if (outBuf)
        *outBufSize = (size_t)packTotal;
      
      res = Progress(progress, unpackTotal + p->srcPos, packTotal);
      if (res != SZ_OK)
        break;

      /*
      if (LzmaEnc_GetNumAvailableBytes(p->enc) == 0)
        break;
      */

      if (packSize == 0)
        break;
    }
    
    LzmaEnc_Finish(p->enc);
    
    unpackTotal += p->srcPos;
    
    RINOK(res)

    if (p->srcPos != (inStream ? limitedInStream.processed : inSizeCur))
      return SZ_ERROR_FAIL;
    
    if (inStream ? limitedInStream.finished : (unpackTotal == inDataSize))
    {
      if (finished)
      {
        if (outBuf)
        {
          const size_t destPos = *outBufSize;
          if (destPos >= outLim)
            return SZ_ERROR_OUTPUT_EOF;
          outBuf[destPos] = LZMA2_CONTROL_EOF; // 0
          *outBufSize = destPos + 1;
        }
        else
        {
          const Byte b = LZMA2_CONTROL_EOF; // 0;
          if (ISeqOutStream_Write(outStream, &b, 1) != 1)
            return SZ_ERROR_WRITE;
        }
      }
      return SZ_OK;
    }
  }
}



#ifndef Z7_ST

static SRes Lzma2Enc_MtCallback_Code(void *p, unsigned coderIndex, unsigned outBufIndex,
    const Byte *src, size_t srcSize, int finished)
{
  CLzma2Enc *me = (CLzma2Enc *)p;
  size_t destSize = me->outBufSize;
  SRes res;
  CMtProgressThunk progressThunk;

  Byte *dest = me->outBufs[outBufIndex];

  me->outBufsDataSizes[outBufIndex] = 0;

  if (!dest)
  {
    dest = (Byte *)ISzAlloc_Alloc(me->alloc, me->outBufSize);
    if (!dest)
      return SZ_ERROR_MEM;
    me->outBufs[outBufIndex] = dest;
  }

  MtProgressThunk_CreateVTable(&progressThunk);
  progressThunk.mtProgress = &me->mtCoder.mtProgress;
  progressThunk.inSize = 0;
  progressThunk.outSize = 0;

  res = Lzma2Enc_EncodeMt1(me,
      &me->coders[coderIndex],
      NULL, dest, &destSize,
      NULL, src, srcSize,
      finished,
      &progressThunk.vt);

  me->outBufsDataSizes[outBufIndex] = destSize;

  return res;
}


static SRes Lzma2Enc_MtCallback_Write(void *p, unsigned outBufIndex)
{
  CLzma2Enc *me = (CLzma2Enc *)p;
  size_t size = me->outBufsDataSizes[outBufIndex];
  const Byte *data = me->outBufs[outBufIndex];
  
  if (me->outStream)
    return ISeqOutStream_Write(me->outStream, data, size) == size ? SZ_OK : SZ_ERROR_WRITE;
  
  if (size > me->outBuf_Rem)
    return SZ_ERROR_OUTPUT_EOF;
  memcpy(me->outBuf, data, size);
  me->outBuf_Rem -= size;
  me->outBuf += size;
  return SZ_OK;
}

#endif



SRes Lzma2Enc_Encode2(CLzma2EncHandle p,
    ISeqOutStreamPtr outStream,
    Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    const Byte *inData, size_t inDataSize,
    ICompressProgressPtr progress)
{
  // GET_CLzma2Enc_p

  if (inStream && inData)
    return SZ_ERROR_PARAM;

  if (outStream && outBuf)
    return SZ_ERROR_PARAM;

  {
    unsigned i;
    for (i = 0; i < MTCODER_THREADS_MAX; i++)
      p->coders[i].propsAreSet = False;
  }

  #ifndef Z7_ST
  
  if (p->props.numBlockThreads_Reduced > 1)
  {
    IMtCoderCallback2 vt;

    if (!p->mtCoder_WasConstructed)
    {
      p->mtCoder_WasConstructed = True;
      MtCoder_Construct(&p->mtCoder);
    }

    vt.Code = Lzma2Enc_MtCallback_Code;
    vt.Write = Lzma2Enc_MtCallback_Write;

    p->outStream = outStream;
    p->outBuf = NULL;
    p->outBuf_Rem = 0;
    if (!outStream)
    {
      p->outBuf = outBuf;
      p->outBuf_Rem = *outBufSize;
      *outBufSize = 0;
    }

    p->mtCoder.allocBig = p->allocBig;
    p->mtCoder.progress = progress;
    p->mtCoder.inStream = inStream;
    p->mtCoder.inData = inData;
    p->mtCoder.inDataSize = inDataSize;
    p->mtCoder.mtCallback = &vt;
    p->mtCoder.mtCallbackObject = p;

    p->mtCoder.blockSize = (size_t)p->props.blockSize;
    if (p->mtCoder.blockSize != p->props.blockSize)
      return SZ_ERROR_PARAM; /* SZ_ERROR_MEM */

    {
      const size_t destBlockSize = p->mtCoder.blockSize + (p->mtCoder.blockSize >> 10) + 16;
      if (destBlockSize < p->mtCoder.blockSize)
        return SZ_ERROR_PARAM;
      if (p->outBufSize != destBlockSize)
        Lzma2Enc_FreeOutBufs(p);
      p->outBufSize = destBlockSize;
    }

    p->mtCoder.numThreadsMax = (unsigned)p->props.numBlockThreads_Max;
    p->mtCoder.numThreadGroups = p->props.numThreadGroups;
    p->mtCoder.expectedDataSize = p->expectedDataSize;
    
    {
      const SRes res = MtCoder_Code(&p->mtCoder);
      if (!outStream)
        *outBufSize = (size_t)(p->outBuf - outBuf);
      return res;
    }
  }

  #endif


  return Lzma2Enc_EncodeMt1(p,
      &p->coders[0],
      outStream, outBuf, outBufSize,
      inStream, inData, inDataSize,
      True, /* finished */
      progress);
}

#undef PRF

/* ================ unit: C/LzmaEnc.c ================ */
/* LzmaEnc.c -- LZMA Encoder
Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

/* #define SHOW_STAT */
/* #define SHOW_STAT2 */

#if defined(SHOW_STAT) || defined(SHOW_STAT2)
#include <stdio.h>
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
#ifndef Z7_ST
// amalgamation: header emitted in prologue
#endif

/* the following LzmaEnc_* declarations is internal LZMA interface for LZMA2 encoder */

SRes LzmaEnc_PrepareForLzma2(CLzmaEncHandle p, ISeqInStreamPtr inStream, UInt32 keepWindowSize,
    ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_MemPrepare(CLzmaEncHandle p, const Byte *src, SizeT srcLen,
    UInt32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_CodeOneMemBlock(CLzmaEncHandle p, BoolInt reInit,
    Byte *dest, size_t *destLen, UInt32 desiredPackSize, UInt32 *unpackSize);
const Byte *LzmaEnc_GetCurBuf(CLzmaEncHandle p);
void LzmaEnc_Finish(CLzmaEncHandle p);
void LzmaEnc_SaveState(CLzmaEncHandle p);
void LzmaEnc_RestoreState(CLzmaEncHandle p);

#ifdef SHOW_STAT
static unsigned g_STAT_OFFSET = 0;
#endif

/* for good normalization speed we still reserve 256 MB before 4 GB range */
#define kLzmaMaxHistorySize ((UInt32)15 << 28)

// #define kNumTopBits 24
#define kTopValue ((UInt32)1 << 24)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5
#define kProbInitValue (kBitModelTotal >> 1)

#define kNumMoveReducingBits 4
#define kNumBitPriceShiftBits 4
// #define kBitPrice (1 << kNumBitPriceShiftBits)

#define REP_LEN_COUNT 64

void LzmaEncProps_Init(CLzmaEncProps *p)
{
  p->level = 5;
  p->dictSize = p->mc = 0;
  p->reduceSize = (UInt64)(Int64)-1;
  p->lc = p->lp = p->pb = p->algo = p->fb = p->btMode = p->numHashBytes = p->numThreads = -1;
  p->numHashOutBits = 0;
  p->writeEndMark = 0;
  p->affinityGroup = -1;
  p->affinity = 0;
  p->affinityInGroup = 0;
}

void LzmaEncProps_Normalize(CLzmaEncProps *p)
{
  int level = p->level;
  if (level < 0) level = 5;
  p->level = level;
  
  if (p->dictSize == 0)
    p->dictSize = (unsigned)level <= 4 ?
        (UInt32)1 << (level * 2 + 16) :
        (unsigned)level <= sizeof(size_t) / 2 + 4 ?
          (UInt32)1 << (level + 20) :
          (UInt32)1 << (sizeof(size_t) / 2 + 24);

  if (p->dictSize > p->reduceSize)
  {
    UInt32 v = (UInt32)p->reduceSize;
    const UInt32 kReduceMin = ((UInt32)1 << 12);
    if (v < kReduceMin)
      v = kReduceMin;
    if (p->dictSize > v)
      p->dictSize = v;
  }

  if (p->lc < 0) p->lc = 3;
  if (p->lp < 0) p->lp = 0;
  if (p->pb < 0) p->pb = 2;

  if (p->algo < 0) p->algo = (unsigned)level < 5 ? 0 : 1;
  if (p->fb < 0) p->fb = (unsigned)level < 7 ? 32 : 64;
  if (p->btMode < 0) p->btMode = (p->algo == 0 ? 0 : 1);
  if (p->numHashBytes < 0) p->numHashBytes = (p->btMode ? 4 : 5);
  if (p->mc == 0) p->mc = (16 + ((unsigned)p->fb >> 1)) >> (p->btMode ? 0 : 1);
  
  if (p->numThreads < 0)
    p->numThreads =
      #ifndef Z7_ST
      ((p->btMode && p->algo) ? 2 : 1);
      #else
      1;
      #endif
}

UInt32 LzmaEncProps_GetDictSize(const CLzmaEncProps *props2)
{
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);
  return props.dictSize;
}


/*
x86/x64:

BSR:
  IF (SRC == 0) ZF = 1, DEST is undefined;
                  AMD : DEST is unchanged;
  IF (SRC != 0) ZF = 0; DEST is index of top non-zero bit
  BSR is slow in some processors

LZCNT:
  IF (SRC  == 0) CF = 1, DEST is size_in_bits_of_register(src) (32 or 64)
  IF (SRC  != 0) CF = 0, DEST = num_lead_zero_bits
  IF (DEST == 0) ZF = 1;

LZCNT works only in new processors starting from Haswell.
if LZCNT is not supported by processor, then it's executed as BSR.
LZCNT can be faster than BSR, if supported.
*/

// #define LZMA_LOG_BSR

#if defined(MY_CPU_ARM_OR_ARM64) /* || defined(MY_CPU_X86_OR_AMD64) */

  #if (defined(__clang__) && (__clang_major__ >= 6)) \
      || (defined(__GNUC__) && (__GNUC__ >= 6))
      #define LZMA_LOG_BSR
  #elif defined(_MSC_VER) && (_MSC_VER >= 1300)
    // #if defined(MY_CPU_ARM_OR_ARM64)
      #define LZMA_LOG_BSR
    // #endif
  #endif
#endif

// #include <intrin.h>

#ifdef LZMA_LOG_BSR

#if defined(__clang__) \
    || defined(__GNUC__)

/*
  C code:                  : (30 - __builtin_clz(x))
    gcc9/gcc10 for x64 /x86  : 30 - (bsr(x) xor 31)
    clang10 for x64          : 31 + (bsr(x) xor -32)
*/

  #define MY_clz(x)  ((unsigned)__builtin_clz(x))
  // __lzcnt32
  // __builtin_ia32_lzcnt_u32

#else  // #if defined(_MSC_VER)

  #ifdef MY_CPU_ARM_OR_ARM64

    #define MY_clz  _CountLeadingZeros

  #else // if defined(MY_CPU_X86_OR_AMD64)

    // #define MY_clz  __lzcnt  // we can use lzcnt (unsupported by old CPU)
    // _BitScanReverse code is not optimal for some MSVC compilers
    #define BSR2_RET(pos, res) { unsigned long zz; _BitScanReverse(&zz, (pos)); zz--; \
      res = (zz + zz) + (pos >> zz); }

  #endif // MY_CPU_X86_OR_AMD64

#endif // _MSC_VER


#ifndef BSR2_RET

    #define BSR2_RET(pos, res) { unsigned zz = 30 - MY_clz(pos); \
      res = (zz + zz) + (pos >> zz); }

#endif


unsigned GetPosSlot1(UInt32 pos);
unsigned GetPosSlot1(UInt32 pos)
{
  unsigned res;
  BSR2_RET(pos, res)
  return res;
}
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res) }
#define GetPosSlot(pos, res) { if (pos < 2) res = pos; else BSR2_RET(pos, res) }


#else // ! LZMA_LOG_BSR

#define kNumLogBits (11 + sizeof(size_t) / 8 * 3)

#define kDicLogSizeMaxCompress ((kNumLogBits - 1) * 2 + 7)

static void LzmaEnc_FastPosInit(Byte *g_FastPos)
{
  unsigned slot;
  g_FastPos[0] = 0;
  g_FastPos[1] = 1;
  g_FastPos += 2;
  
  for (slot = 2; slot < kNumLogBits * 2; slot++)
  {
    size_t k = ((size_t)1 << ((slot >> 1) - 1));
    size_t j;
    for (j = 0; j < k; j++)
      g_FastPos[j] = (Byte)slot;
    g_FastPos += k;
  }
}

/* we can use ((limit - pos) >> 31) only if (pos < ((UInt32)1 << 31)) */
/*
#define BSR2_RET(pos, res) { unsigned zz = 6 + ((kNumLogBits - 1) & \
  (0 - (((((UInt32)1 << (kNumLogBits + 6)) - 1) - pos) >> 31))); \
  res = p->g_FastPos[pos >> zz] + (zz * 2); }
*/

/*
#define BSR2_RET(pos, res) { unsigned zz = 6 + ((kNumLogBits - 1) & \
  (0 - (((((UInt32)1 << (kNumLogBits)) - 1) - (pos >> 6)) >> 31))); \
  res = p->g_FastPos[pos >> zz] + (zz * 2); }
*/

#define BSR2_RET(pos, res) { unsigned zz = (pos < (1 << (kNumLogBits + 6))) ? 6 : 6 + kNumLogBits - 1; \
  res = p->g_FastPos[pos >> zz] + (zz * 2); }

/*
#define BSR2_RET(pos, res) { res = (pos < (1 << (kNumLogBits + 6))) ? \
  p->g_FastPos[pos >> 6] + 12 : \
  p->g_FastPos[pos >> (6 + kNumLogBits - 1)] + (6 + (kNumLogBits - 1)) * 2; }
*/

#define GetPosSlot1(pos) p->g_FastPos[pos]
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if (pos < kNumFullDistances) res = p->g_FastPos[pos & (kNumFullDistances - 1)]; else BSR2_RET(pos, res); }

#endif // LZMA_LOG_BSR


#define LZMA_NUM_REPS 4

typedef UInt16 CState;
typedef UInt16 CExtra;

typedef struct
{
  UInt32 price;
  CState state;
  CExtra extra;
      // 0   : normal
      // 1   : LIT : MATCH
      // > 1 : MATCH (extra-1) : LIT : REP0 (len)
  UInt32 len;
  UInt32 dist;
  UInt32 reps[LZMA_NUM_REPS];
} COptimal;


// 18.06
#define kNumOpts (1 << 11)
#define kPackReserve (kNumOpts * 8)
// #define kNumOpts (1 << 12)
// #define kPackReserve (1 + kNumOpts * 2)

#define kNumLenToPosStates 4
#define kNumPosSlotBits 6
// #define kDicLogSizeMin 0
#define kDicLogSizeMax 32
#define kDistTableSizeMax (kDicLogSizeMax * 2)

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)
#define kAlignMask (kAlignTableSize - 1)

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

typedef
#ifdef Z7_LZMA_PROB32
  UInt32
#else
  UInt16
#endif
  CLzmaProb;

#define LZMA_PB_MAX 4
#define LZMA_LC_MAX 8
#define LZMA_LP_MAX 4

#define LZMA_NUM_PB_STATES_MAX (1 << LZMA_PB_MAX)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)
#define kLenNumSymbolsTotal (kLenNumLowSymbols * 2 + kLenNumHighSymbols)

#define LZMA_MATCH_LEN_MIN 2
#define LZMA_MATCH_LEN_MAX (LZMA_MATCH_LEN_MIN + kLenNumSymbolsTotal - 1)

#define kNumStates 12


typedef struct
{
  CLzmaProb low[LZMA_NUM_PB_STATES_MAX << (kLenNumLowBits + 1)];
  CLzmaProb high[kLenNumHighSymbols];
} CLenEnc;


typedef struct
{
  unsigned tableSize;
  UInt32 prices[LZMA_NUM_PB_STATES_MAX][kLenNumSymbolsTotal];
  // UInt32 prices1[LZMA_NUM_PB_STATES_MAX][kLenNumLowSymbols * 2];
  // UInt32 prices2[kLenNumSymbolsTotal];
} CLenPriceEnc;

#define GET_PRICE_LEN(p, posState, len) \
    ((p)->prices[posState][(size_t)(len) - LZMA_MATCH_LEN_MIN])

/*
#define GET_PRICE_LEN(p, posState, len) \
    ((p)->prices2[(size_t)(len) - 2] + ((p)->prices1[posState][((len) - 2) & (kLenNumLowSymbols * 2 - 1)] & (((len) - 2 - kLenNumLowSymbols * 2) >> 9)))
*/

typedef struct
{
  UInt32 range;
  unsigned cache;
  UInt64 low;
  UInt64 cacheSize;
  Byte *buf;
  Byte *bufLim;
  Byte *bufBase;
  ISeqOutStreamPtr outStream;
  UInt64 processed;
  SRes res;
} CRangeEnc;


typedef struct
{
  CLzmaProb *litProbs;

  unsigned state;
  UInt32 reps[LZMA_NUM_REPS];

  CLzmaProb posAlignEncoder[1 << kNumAlignBits];
  CLzmaProb isRep[kNumStates];
  CLzmaProb isRepG0[kNumStates];
  CLzmaProb isRepG1[kNumStates];
  CLzmaProb isRepG2[kNumStates];
  CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

  CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  CLzmaProb posEncoders[kNumFullDistances];
  
  CLenEnc lenProbs;
  CLenEnc repLenProbs;

} CSaveState;


typedef UInt32 CProbPrice;


struct CLzmaEnc
{
  void *matchFinderObj;
  IMatchFinder2 matchFinder;

  unsigned optCur;
  unsigned optEnd;

  unsigned longestMatchLen;
  unsigned numPairs;
  UInt32 numAvail;

  unsigned state;
  unsigned numFastBytes;
  unsigned additionalOffset;
  UInt32 reps[LZMA_NUM_REPS];
  unsigned lpMask, pbMask;
  CLzmaProb *litProbs;
  CRangeEnc rc;

  UInt32 backRes;

  unsigned lc, lp, pb;
  unsigned lclp;

  BoolInt fastMode;
  BoolInt writeEndMark;
  BoolInt finished;
  BoolInt multiThread;
  BoolInt needInit;
  // BoolInt _maxMode;

  UInt64 nowPos64;
  
  unsigned matchPriceCount;
  // unsigned alignPriceCount;
  int repLenEncCounter;

  unsigned distTableSize;

  UInt32 dictSize;
  SRes result;

  #ifndef Z7_ST
  BoolInt mtMode;
  // begin of CMatchFinderMt is used in LZ thread
  CMatchFinderMt matchFinderMt;
  // end of CMatchFinderMt is used in BT and HASH threads
  // #else
  // CMatchFinder matchFinderBase;
  #endif
  CMatchFinder matchFinderBase;

  
  // we suppose that we have 8-bytes alignment after CMatchFinder
 
  #ifndef Z7_ST
  Byte pad[128];
  #endif
  
  // LZ thread
  CProbPrice ProbPrices[kBitModelTotal >> kNumMoveReducingBits];

  // we want {len , dist} pairs to be 8-bytes aligned in matches array
  UInt32 matches[LZMA_MATCH_LEN_MAX * 2 + 2];

  // we want 8-bytes alignment here
  UInt32 alignPrices[kAlignTableSize];
  UInt32 posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  UInt32 distancesPrices[kNumLenToPosStates][kNumFullDistances];

  CLzmaProb posAlignEncoder[1 << kNumAlignBits];
  CLzmaProb isRep[kNumStates];
  CLzmaProb isRepG0[kNumStates];
  CLzmaProb isRepG1[kNumStates];
  CLzmaProb isRepG2[kNumStates];
  CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  CLzmaProb posEncoders[kNumFullDistances];
  
  CLenEnc lenProbs;
  CLenEnc repLenProbs;

  #ifndef LZMA_LOG_BSR
  Byte g_FastPos[1 << kNumLogBits];
  #endif

  CLenPriceEnc lenEnc;
  CLenPriceEnc repLenEnc;

  COptimal opt[kNumOpts];

  CSaveState saveState;

  // BoolInt mf_Failure;
  #ifndef Z7_ST
  Byte pad2[128];
  #endif
};


#define MFB (p->matchFinderBase)
/*
#ifndef Z7_ST
#define MFB (p->matchFinderMt.MatchFinder)
#endif
*/

// #define GET_CLzmaEnc_p  CLzmaEnc *p = (CLzmaEnc*)(void *)p;
// #define GET_const_CLzmaEnc_p  const CLzmaEnc *p = (const CLzmaEnc*)(const void *)p;

#define COPY_ARR(dest, src, arr)  memcpy((dest)->arr, (src)->arr, sizeof((src)->arr));

#define COPY_LZMA_ENC_STATE(d, s, p)  \
  (d)->state = (s)->state;  \
  COPY_ARR(d, s, reps)  \
  COPY_ARR(d, s, posAlignEncoder)  \
  COPY_ARR(d, s, isRep)  \
  COPY_ARR(d, s, isRepG0)  \
  COPY_ARR(d, s, isRepG1)  \
  COPY_ARR(d, s, isRepG2)  \
  COPY_ARR(d, s, isMatch)  \
  COPY_ARR(d, s, isRep0Long)  \
  COPY_ARR(d, s, posSlotEncoder)  \
  COPY_ARR(d, s, posEncoders)  \
  (d)->lenProbs = (s)->lenProbs;  \
  (d)->repLenProbs = (s)->repLenProbs;  \
  memcpy((d)->litProbs, (s)->litProbs, ((size_t)0x300 * sizeof(CLzmaProb)) << (p)->lclp);

void LzmaEnc_SaveState(CLzmaEncHandle p)
{
  // GET_CLzmaEnc_p
  CSaveState *v = &p->saveState;
  COPY_LZMA_ENC_STATE(v, p, p)
}

void LzmaEnc_RestoreState(CLzmaEncHandle p)
{
  // GET_CLzmaEnc_p
  const CSaveState *v = &p->saveState;
  COPY_LZMA_ENC_STATE(p, v, p)
}


Z7_NO_INLINE
SRes LzmaEnc_SetProps(CLzmaEncHandle p, const CLzmaEncProps *props2)
{
  // GET_CLzmaEnc_p
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);

  if (props.lc > LZMA_LC_MAX
      || props.lp > LZMA_LP_MAX
      || props.pb > LZMA_PB_MAX)
    return SZ_ERROR_PARAM;


  if (props.dictSize > kLzmaMaxHistorySize)
    props.dictSize = kLzmaMaxHistorySize;

  #ifndef LZMA_LOG_BSR
  {
    const UInt64 dict64 = props.dictSize;
    if (dict64 > ((UInt64)1 << kDicLogSizeMaxCompress))
      return SZ_ERROR_PARAM;
  }
  #endif

  p->dictSize = props.dictSize;
  {
    unsigned fb = (unsigned)props.fb;
    if (fb < 5)
      fb = 5;
    if (fb > LZMA_MATCH_LEN_MAX)
      fb = LZMA_MATCH_LEN_MAX;
    p->numFastBytes = fb;
  }
  p->lc = (unsigned)props.lc;
  p->lp = (unsigned)props.lp;
  p->pb = (unsigned)props.pb;
  p->fastMode = (props.algo == 0);
  // p->_maxMode = True;
  MFB.btMode = (Byte)(props.btMode ? 1 : 0);
  // MFB.btMode = (Byte)(props.btMode);
  {
    unsigned numHashBytes = 4;
    if (props.btMode)
    {
           if (props.numHashBytes <  2) numHashBytes = 2;
      else if (props.numHashBytes <  4) numHashBytes = (unsigned)props.numHashBytes;
    }
    if (props.numHashBytes >= 5) numHashBytes = 5;

    MFB.numHashBytes = numHashBytes;
    // MFB.numHashBytes_Min = 2;
    MFB.numHashOutBits = (Byte)props.numHashOutBits;
  }

  MFB.cutValue = props.mc;

  p->writeEndMark = (BoolInt)props.writeEndMark;

  #ifndef Z7_ST
  /*
  if (newMultiThread != _multiThread)
  {
    ReleaseMatchFinder();
    _multiThread = newMultiThread;
  }
  */
  p->multiThread = (props.numThreads > 1);
  p->matchFinderMt.btSync.affinity =
  p->matchFinderMt.hashSync.affinity = props.affinity;
  p->matchFinderMt.btSync.affinityGroup =
  p->matchFinderMt.hashSync.affinityGroup = props.affinityGroup;
  p->matchFinderMt.btSync.affinityInGroup =
  p->matchFinderMt.hashSync.affinityInGroup = props.affinityInGroup;
  #endif

  return SZ_OK;
}


void LzmaEnc_SetDataSize(CLzmaEncHandle p, UInt64 expectedDataSiize)
{
  // GET_CLzmaEnc_p
  MFB.expectedDataSize = expectedDataSiize;
}


#define kState_Start 0
#define kState_LitAfterMatch 4
#define kState_LitAfterRep   5
#define kState_MatchAfterLit 7
#define kState_RepAfterLit   8

static const Byte kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
static const Byte kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
static const Byte kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
static const Byte kShortRepNextStates[kNumStates]= {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

#define IsLitState(s) ((s) < 7)
#define GetLenToPosState2(len) (((len) < kNumLenToPosStates - 1) ? (len) : kNumLenToPosStates - 1)
#define GetLenToPosState(len) (((len) < kNumLenToPosStates + 1) ? (len) - 2 : kNumLenToPosStates - 1)

#define kInfinityPrice (1 << 30)

static void RangeEnc_Construct(CRangeEnc *p)
{
  p->outStream = NULL;
  p->bufBase = NULL;
}

#define RangeEnc_GetProcessed(p)       (        (p)->processed + (size_t)((p)->buf - (p)->bufBase) +         (p)->cacheSize)
#define RangeEnc_GetProcessed_sizet(p) ((size_t)(p)->processed + (size_t)((p)->buf - (p)->bufBase) + (size_t)(p)->cacheSize)

#define RC_BUF_SIZE (1 << 16)

static int RangeEnc_Alloc(CRangeEnc *p, ISzAllocPtr alloc)
{
  if (!p->bufBase)
  {
    p->bufBase = (Byte *)ISzAlloc_Alloc(alloc, RC_BUF_SIZE);
    if (!p->bufBase)
      return 0;
    p->bufLim = p->bufBase + RC_BUF_SIZE;
  }
  return 1;
}

static void RangeEnc_Free(CRangeEnc *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->bufBase);
  p->bufBase = NULL;
}

static void RangeEnc_Init(CRangeEnc *p)
{
  p->range = 0xFFFFFFFF;
  p->cache = 0;
  p->low = 0;
  p->cacheSize = 0;

  p->buf = p->bufBase;

  p->processed = 0;
  p->res = SZ_OK;
}

Z7_NO_INLINE static void RangeEnc_FlushStream(CRangeEnc *p)
{
  const size_t num = (size_t)(p->buf - p->bufBase);
  if (p->res == SZ_OK)
  {
    if (num != ISeqOutStream_Write(p->outStream, p->bufBase, num))
      p->res = SZ_ERROR_WRITE;
  }
  p->processed += num;
  p->buf = p->bufBase;
}

Z7_NO_INLINE static void Z7_FASTCALL RangeEnc_ShiftLow(CRangeEnc *p)
{
  UInt32 low = (UInt32)p->low;
  unsigned high = (unsigned)(p->low >> 32);
  p->low = (UInt32)(low << 8);
  if (low < (UInt32)0xFF000000 || high != 0)
  {
    {
      Byte *buf = p->buf;
      *buf++ = (Byte)(p->cache + high);
      p->cache = (unsigned)(low >> 24);
      p->buf = buf;
      if (buf == p->bufLim)
        RangeEnc_FlushStream(p);
      if (p->cacheSize == 0)
        return;
    }
    high += 0xFF;
    for (;;)
    {
      Byte *buf = p->buf;
      *buf++ = (Byte)(high);
      p->buf = buf;
      if (buf == p->bufLim)
        RangeEnc_FlushStream(p);
      if (--p->cacheSize == 0)
        return;
    }
  }
  p->cacheSize++;
}

static void RangeEnc_FlushData(CRangeEnc *p)
{
  int i;
  for (i = 0; i < 5; i++)
    RangeEnc_ShiftLow(p);
}

#define RC_NORM(p) if (range < kTopValue) { range <<= 8; RangeEnc_ShiftLow(p); }

#define RC_BIT_PRE(p, prob) \
  ttt = *(prob); \
  newBound = (range >> kNumBitModelTotalBits) * ttt;

// #define Z7_LZMA_ENC_USE_BRANCH

#ifdef Z7_LZMA_ENC_USE_BRANCH

#define RC_BIT(p, prob, bit) { \
  RC_BIT_PRE(p, prob) \
  if (bit == 0) { range = newBound; ttt += (kBitModelTotal - ttt) >> kNumMoveBits; } \
  else { (p)->low += newBound; range -= newBound; ttt -= ttt >> kNumMoveBits; } \
  *(prob) = (CLzmaProb)ttt; \
  RC_NORM(p) \
  }

#else

#define RC_BIT(p, prob, bit) { \
  UInt32 mask; \
  RC_BIT_PRE(p, prob) \
  mask = 0 - (UInt32)bit; \
  range &= mask; \
  mask &= newBound; \
  range -= mask; \
  (p)->low += mask; \
  mask = (UInt32)bit - 1; \
  range += newBound & mask; \
  mask &= (kBitModelTotal - ((1 << kNumMoveBits) - 1)); \
  mask += ((1 << kNumMoveBits) - 1); \
  ttt += (UInt32)((Int32)(mask - ttt) >> kNumMoveBits); \
  *(prob) = (CLzmaProb)ttt; \
  RC_NORM(p) \
  }

#endif




#define RC_BIT_0_BASE(p, prob) \
  range = newBound; *(prob) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));

#define RC_BIT_1_BASE(p, prob) \
  range -= newBound; (p)->low += newBound; *(prob) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits)); \

#define RC_BIT_0(p, prob) \
  RC_BIT_0_BASE(p, prob) \
  RC_NORM(p)

#define RC_BIT_1(p, prob) \
  RC_BIT_1_BASE(p, prob) \
  RC_NORM(p)

static void RangeEnc_EncodeBit_0(CRangeEnc *p, CLzmaProb *prob)
{
  UInt32 range, ttt, newBound;
  range = p->range;
  RC_BIT_PRE(p, prob)
  RC_BIT_0(p, prob)
  p->range = range;
}

static void LitEnc_Encode(CRangeEnc *p, CLzmaProb *probs, UInt32 sym)
{
  UInt32 range = p->range;
  sym |= 0x100;
  do
  {
    UInt32 ttt, newBound;
    // RangeEnc_EncodeBit(p, probs + (sym >> 8), (sym >> 7) & 1);
    CLzmaProb *prob = probs + (sym >> 8);
    UInt32 bit = (sym >> 7) & 1;
    sym <<= 1;
    RC_BIT(p, prob, bit)
  }
  while (sym < 0x10000);
  p->range = range;
}

static void LitEnc_EncodeMatched(CRangeEnc *p, CLzmaProb *probs, UInt32 sym, UInt32 matchByte)
{
  UInt32 range = p->range;
  UInt32 offs = 0x100;
  sym |= 0x100;
  do
  {
    UInt32 ttt, newBound;
    CLzmaProb *prob;
    UInt32 bit;
    matchByte <<= 1;
    // RangeEnc_EncodeBit(p, probs + (offs + (matchByte & offs) + (sym >> 8)), (sym >> 7) & 1);
    prob = probs + (offs + (matchByte & offs) + (sym >> 8));
    bit = (sym >> 7) & 1;
    sym <<= 1;
    offs &= ~(matchByte ^ sym);
    RC_BIT(p, prob, bit)
  }
  while (sym < 0x10000);
  p->range = range;
}



static void LzmaEnc_InitPriceTables(CProbPrice *ProbPrices)
{
  UInt32 i;
  for (i = 0; i < (kBitModelTotal >> kNumMoveReducingBits); i++)
  {
    const unsigned kCyclesBits = kNumBitPriceShiftBits;
    UInt32 w = (i << kNumMoveReducingBits) + (1 << (kNumMoveReducingBits - 1));
    unsigned bitCount = 0;
    unsigned j;
    for (j = 0; j < kCyclesBits; j++)
    {
      w = w * w;
      bitCount <<= 1;
      while (w >= ((UInt32)1 << 16))
      {
        w >>= 1;
        bitCount++;
      }
    }
    ProbPrices[i] = (CProbPrice)(((unsigned)kNumBitModelTotalBits << kCyclesBits) - 15 - bitCount);
    // printf("\n%3d: %5d", i, ProbPrices[i]);
  }
}


#define GET_PRICE(prob, bit) \
  p->ProbPrices[((prob) ^ (unsigned)(((-(int)(bit))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits]

#define GET_PRICEa(prob, bit) \
     ProbPrices[((prob) ^ (unsigned)((-((int)(bit))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits]

#define GET_PRICE_0(prob) p->ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1(prob) p->ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

#define GET_PRICEa_0(prob) ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICEa_1(prob) ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]


static UInt32 LitEnc_GetPrice(const CLzmaProb *probs, UInt32 sym, const CProbPrice *ProbPrices)
{
  UInt32 price = 0;
  sym |= 0x100;
  do
  {
    unsigned bit = sym & 1;
    sym >>= 1;
    price += GET_PRICEa(probs[sym], bit);
  }
  while (sym >= 2);
  return price;
}


static UInt32 LitEnc_Matched_GetPrice(const CLzmaProb *probs, UInt32 sym, UInt32 matchByte, const CProbPrice *ProbPrices)
{
  UInt32 price = 0;
  UInt32 offs = 0x100;
  sym |= 0x100;
  do
  {
    matchByte <<= 1;
    price += GET_PRICEa(probs[offs + (matchByte & offs) + (sym >> 8)], (sym >> 7) & 1);
    sym <<= 1;
    offs &= ~(matchByte ^ sym);
  }
  while (sym < 0x10000);
  return price;
}


static void RcTree_ReverseEncode(CRangeEnc *rc, CLzmaProb *probs, unsigned numBits, unsigned sym)
{
  UInt32 range = rc->range;
  unsigned m = 1;
  do
  {
    UInt32 ttt, newBound;
    unsigned bit = sym & 1;
    // RangeEnc_EncodeBit(rc, probs + m, bit);
    sym >>= 1;
    RC_BIT(rc, probs + m, bit)
    m = (m << 1) | bit;
  }
  while (--numBits);
  rc->range = range;
}



static void LenEnc_Init(CLenEnc *p)
{
  unsigned i;
  for (i = 0; i < (LZMA_NUM_PB_STATES_MAX << (kLenNumLowBits + 1)); i++)
    p->low[i] = kProbInitValue;
  for (i = 0; i < kLenNumHighSymbols; i++)
    p->high[i] = kProbInitValue;
}

static void LenEnc_Encode(CLenEnc *p, CRangeEnc *rc, unsigned sym, unsigned posState)
{
  UInt32 range, ttt, newBound;
  CLzmaProb *probs = p->low;
  range = rc->range;
  RC_BIT_PRE(rc, probs)
  if (sym >= kLenNumLowSymbols)
  {
    RC_BIT_1(rc, probs)
    probs += kLenNumLowSymbols;
    RC_BIT_PRE(rc, probs)
    if (sym >= kLenNumLowSymbols * 2)
    {
      RC_BIT_1(rc, probs)
      rc->range = range;
      // RcTree_Encode(rc, p->high, kLenNumHighBits, sym - kLenNumLowSymbols * 2);
      LitEnc_Encode(rc, p->high, sym - kLenNumLowSymbols * 2);
      return;
    }
    sym -= kLenNumLowSymbols;
  }

  // RcTree_Encode(rc, probs + (posState << kLenNumLowBits), kLenNumLowBits, sym);
  {
    unsigned m;
    unsigned bit;
    RC_BIT_0(rc, probs)
    probs += (posState << (1 + kLenNumLowBits));
    bit = (sym >> 2)    ; RC_BIT(rc, probs + 1, bit)  m = (1 << 1) + bit;
    bit = (sym >> 1) & 1; RC_BIT(rc, probs + m, bit)  m = (m << 1) + bit;
    bit =  sym       & 1; RC_BIT(rc, probs + m, bit)
    rc->range = range;
  }
}

static void SetPrices_3(const CLzmaProb *probs, UInt32 startPrice, UInt32 *prices, const CProbPrice *ProbPrices)
{
  unsigned i;
  for (i = 0; i < 8; i += 2)
  {
    UInt32 price = startPrice;
    UInt32 prob;
    price += GET_PRICEa(probs[1           ], (i >> 2));
    price += GET_PRICEa(probs[2 + (i >> 2)], (i >> 1) & 1);
    prob = probs[4 + (i >> 1)];
    prices[i    ] = price + GET_PRICEa_0(prob);
    prices[i + 1] = price + GET_PRICEa_1(prob);
  }
}


Z7_NO_INLINE static void Z7_FASTCALL LenPriceEnc_UpdateTables(
    CLenPriceEnc *p,
    unsigned numPosStates,
    const CLenEnc *enc,
    const CProbPrice *ProbPrices)
{
  UInt32 b;
 
  {
    unsigned prob = enc->low[0];
    UInt32 a, c;
    unsigned posState;
    b = GET_PRICEa_1(prob);
    a = GET_PRICEa_0(prob);
    c = b + GET_PRICEa_0(enc->low[kLenNumLowSymbols]);
    for (posState = 0; posState < numPosStates; posState++)
    {
      UInt32 *prices = p->prices[posState];
      const CLzmaProb *probs = enc->low + (posState << (1 + kLenNumLowBits));
      SetPrices_3(probs, a, prices, ProbPrices);
      SetPrices_3(probs + kLenNumLowSymbols, c, prices + kLenNumLowSymbols, ProbPrices);
    }
  }

  /*
  {
    unsigned i;
    UInt32 b;
    a = GET_PRICEa_0(enc->low[0]);
    for (i = 0; i < kLenNumLowSymbols; i++)
      p->prices2[i] = a;
    a = GET_PRICEa_1(enc->low[0]);
    b = a + GET_PRICEa_0(enc->low[kLenNumLowSymbols]);
    for (i = kLenNumLowSymbols; i < kLenNumLowSymbols * 2; i++)
      p->prices2[i] = b;
    a += GET_PRICEa_1(enc->low[kLenNumLowSymbols]);
  }
  */
 
  // p->counter = numSymbols;
  // p->counter = 64;

  {
    unsigned i = p->tableSize;
    
    if (i > kLenNumLowSymbols * 2)
    {
      const CLzmaProb *probs = enc->high;
      UInt32 *prices = p->prices[0] + kLenNumLowSymbols * 2;
      i -= kLenNumLowSymbols * 2 - 1;
      i >>= 1;
      b += GET_PRICEa_1(enc->low[kLenNumLowSymbols]);
      do
      {
        /*
        p->prices2[i] = a +
        // RcTree_GetPrice(enc->high, kLenNumHighBits, i - kLenNumLowSymbols * 2, ProbPrices);
        LitEnc_GetPrice(probs, i - kLenNumLowSymbols * 2, ProbPrices);
        */
        // UInt32 price = a + RcTree_GetPrice(probs, kLenNumHighBits - 1, sym, ProbPrices);
        unsigned sym = --i + (1 << (kLenNumHighBits - 1));
        UInt32 price = b;
        do
        {
          const unsigned bit = sym & 1;
          sym >>= 1;
          price += GET_PRICEa(probs[sym], bit);
        }
        while (sym >= 2);

        {
          const unsigned prob = probs[(size_t)i + (1 << (kLenNumHighBits - 1))];
          prices[(size_t)i * 2    ] = price + GET_PRICEa_0(prob);
          prices[(size_t)i * 2 + 1] = price + GET_PRICEa_1(prob);
        }
      }
      while (i);

      {
        unsigned posState;
        const size_t num = (p->tableSize - kLenNumLowSymbols * 2) * sizeof(p->prices[0][0]);
        for (posState = 1; posState < numPosStates; posState++)
          memcpy(p->prices[posState] + kLenNumLowSymbols * 2, p->prices[0] + kLenNumLowSymbols * 2, num);
      }
    }
  }
}

/*
  #ifdef SHOW_STAT
  g_STAT_OFFSET += num;
  printf("\n MovePos %u", num);
  #endif
*/
  
#define MOVE_POS(p, num) { \
    p->additionalOffset += (num); \
    p->matchFinder.Skip(p->matchFinderObj, (UInt32)(num)); }


static unsigned ReadMatchDistances(CLzmaEnc *p, unsigned *numPairsRes)
{
  unsigned numPairs;
  
  p->additionalOffset++;
  p->numAvail = p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
  {
    const UInt32 *d = p->matchFinder.GetMatches(p->matchFinderObj, p->matches);
    // if (!d) { p->mf_Failure = True; *numPairsRes = 0;  return 0; }
    numPairs = (unsigned)(d - p->matches);
  }
  *numPairsRes = numPairs;
  
  #ifdef SHOW_STAT
  printf("\n i = %u numPairs = %u    ", g_STAT_OFFSET, numPairs / 2);
  g_STAT_OFFSET++;
  {
    unsigned i;
    for (i = 0; i < numPairs; i += 2)
      printf("%2u %6u   | ", p->matches[i], p->matches[i + 1]);
  }
  #endif
  
  if (numPairs == 0)
    return 0;
  {
    const unsigned len = p->matches[(size_t)numPairs - 2];
    if (len != p->numFastBytes)
      return len;
    {
      UInt32 numAvail = p->numAvail;
      if (numAvail > LZMA_MATCH_LEN_MAX)
        numAvail = LZMA_MATCH_LEN_MAX;
      {
        const Byte *p1 = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
        const Byte *p2 = p1 + len;
        const ptrdiff_t dif = (ptrdiff_t)-1 - (ptrdiff_t)p->matches[(size_t)numPairs - 1];
        const Byte *lim = p1 + numAvail;
        for (; p2 != lim && *p2 == p2[dif]; p2++)
        {}
        return (unsigned)(p2 - p1);
      }
    }
  }
}

#define MARK_LIT ((UInt32)(Int32)-1)

#define MakeAs_Lit(p)       { (p)->dist = MARK_LIT; (p)->extra = 0; }
#define MakeAs_ShortRep(p)  { (p)->dist = 0; (p)->extra = 0; }
#define IsShortRep(p)       ((p)->dist == 0)


#define GetPrice_ShortRep(p, state, posState) \
  ( GET_PRICE_0(p->isRepG0[state]) + GET_PRICE_0(p->isRep0Long[state][posState]))

#define GetPrice_Rep_0(p, state, posState) ( \
    GET_PRICE_1(p->isMatch[state][posState]) \
  + GET_PRICE_1(p->isRep0Long[state][posState])) \
  + GET_PRICE_1(p->isRep[state]) \
  + GET_PRICE_0(p->isRepG0[state])
  
Z7_FORCE_INLINE
static UInt32 GetPrice_PureRep(const CLzmaEnc *p, unsigned repIndex, size_t state, size_t posState)
{
  UInt32 price;
  UInt32 prob = p->isRepG0[state];
  if (repIndex == 0)
  {
    price = GET_PRICE_0(prob);
    price += GET_PRICE_1(p->isRep0Long[state][posState]);
  }
  else
  {
    price = GET_PRICE_1(prob);
    prob = p->isRepG1[state];
    if (repIndex == 1)
      price += GET_PRICE_0(prob);
    else
    {
      price += GET_PRICE_1(prob);
      price += GET_PRICE(p->isRepG2[state], repIndex - 2);
    }
  }
  return price;
}


static unsigned Backward(CLzmaEnc *p, unsigned cur)
{
  unsigned wr = cur + 1;
  p->optEnd = wr;

  for (;;)
  {
    UInt32 dist = p->opt[cur].dist;
    unsigned len = (unsigned)p->opt[cur].len;
    unsigned extra = (unsigned)p->opt[cur].extra;
    cur -= len;

    if (extra)
    {
      wr--;
      p->opt[wr].len = (UInt32)len;
      cur -= extra;
      len = extra;
      if (extra == 1)
      {
        p->opt[wr].dist = dist;
        dist = MARK_LIT;
      }
      else
      {
        p->opt[wr].dist = 0;
        len--;
        wr--;
        p->opt[wr].dist = MARK_LIT;
        p->opt[wr].len = 1;
      }
    }

    if (cur == 0)
    {
      p->backRes = dist;
      p->optCur = wr;
      return len;
    }
    
    wr--;
    p->opt[wr].dist = dist;
    p->opt[wr].len = (UInt32)len;
  }
}



#define LIT_PROBS(pos, prevByte) \
  (p->litProbs + (UInt32)3 * (((((pos) << 8) + (prevByte)) & p->lpMask) << p->lc))


static unsigned GetOptimum(CLzmaEnc *p, UInt32 position)
{
  unsigned last, cur;
  UInt32 reps[LZMA_NUM_REPS];
  unsigned repLens[LZMA_NUM_REPS];
  UInt32 *matches;

  {
    UInt32 numAvail;
    unsigned numPairs, mainLen, repMaxIndex, i, posState;
    UInt32 matchPrice, repMatchPrice;
    const Byte *data;
    Byte curByte, matchByte;
    
    p->optCur = p->optEnd = 0;
    
    if (p->additionalOffset == 0)
      mainLen = ReadMatchDistances(p, &numPairs);
    else
    {
      mainLen = p->longestMatchLen;
      numPairs = p->numPairs;
    }
    
    numAvail = p->numAvail;
    if (numAvail < 2)
    {
      p->backRes = MARK_LIT;
      return 1;
    }
    if (numAvail > LZMA_MATCH_LEN_MAX)
      numAvail = LZMA_MATCH_LEN_MAX;
    
    data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
    repMaxIndex = 0;
    
    for (i = 0; i < LZMA_NUM_REPS; i++)
    {
      unsigned len;
      const Byte *data2;
      reps[i] = p->reps[i];
      data2 = data - reps[i];
      if (data[0] != data2[0] || data[1] != data2[1])
      {
        repLens[i] = 0;
        continue;
      }
      for (len = 2; len < numAvail && data[len] == data2[len]; len++)
      {}
      repLens[i] = len;
      if (len > repLens[repMaxIndex])
        repMaxIndex = i;
      if (len == LZMA_MATCH_LEN_MAX) // 21.03 : optimization
        break;
    }
    
    if (repLens[repMaxIndex] >= p->numFastBytes)
    {
      unsigned len;
      p->backRes = (UInt32)repMaxIndex;
      len = repLens[repMaxIndex];
      MOVE_POS(p, len - 1)
      return len;
    }
    
    matches = p->matches;
    #define MATCHES  matches
    // #define MATCHES  p->matches
    
    if (mainLen >= p->numFastBytes)
    {
      p->backRes = MATCHES[(size_t)numPairs - 1] + LZMA_NUM_REPS;
      MOVE_POS(p, mainLen - 1)
      return mainLen;
    }
    
    curByte = *data;
    matchByte = *(data - reps[0]);

    last = repLens[repMaxIndex];
    if (last <= mainLen)
      last = mainLen;
    
    if (last < 2 && curByte != matchByte)
    {
      p->backRes = MARK_LIT;
      return 1;
    }
    
    p->opt[0].state = (CState)p->state;
    
    posState = (position & p->pbMask);
    
    {
      const CLzmaProb *probs = LIT_PROBS(position, *(data - 1));
      p->opt[1].price = GET_PRICE_0(p->isMatch[p->state][posState]) +
        (!IsLitState(p->state) ?
          LitEnc_Matched_GetPrice(probs, curByte, matchByte, p->ProbPrices) :
          LitEnc_GetPrice(probs, curByte, p->ProbPrices));
    }

    MakeAs_Lit(&p->opt[1])
    
    matchPrice = GET_PRICE_1(p->isMatch[p->state][posState]);
    repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[p->state]);
    
    // 18.06
    if (matchByte == curByte && repLens[0] == 0)
    {
      UInt32 shortRepPrice = repMatchPrice + GetPrice_ShortRep(p, p->state, posState);
      if (shortRepPrice < p->opt[1].price)
      {
        p->opt[1].price = shortRepPrice;
        MakeAs_ShortRep(&p->opt[1])
      }
      if (last < 2)
      {
        p->backRes = p->opt[1].dist;
        return 1;
      }
    }
   
    p->opt[1].len = 1;
    
    p->opt[0].reps[0] = reps[0];
    p->opt[0].reps[1] = reps[1];
    p->opt[0].reps[2] = reps[2];
    p->opt[0].reps[3] = reps[3];
    
    // ---------- REP ----------
    
    for (i = 0; i < LZMA_NUM_REPS; i++)
    {
      unsigned repLen = repLens[i];
      UInt32 price;
      if (repLen < 2)
        continue;
      price = repMatchPrice + GetPrice_PureRep(p, i, p->state, posState);
      do
      {
        UInt32 price2 = price + GET_PRICE_LEN(&p->repLenEnc, posState, repLen);
        COptimal *opt = &p->opt[repLen];
        if (price2 < opt->price)
        {
          opt->price = price2;
          opt->len = (UInt32)repLen;
          opt->dist = (UInt32)i;
          opt->extra = 0;
        }
      }
      while (--repLen >= 2);
    }
    
    
    // ---------- MATCH ----------
    {
      unsigned len = repLens[0] + 1;
      if (len <= mainLen)
      {
        unsigned offs = 0;
        UInt32 normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[p->state]);

        if (len < 2)
          len = 2;
        else
          while (len > MATCHES[offs])
            offs += 2;
    
        for (; ; len++)
        {
          COptimal *opt;
          UInt32 dist = MATCHES[(size_t)offs + 1];
          UInt32 price = normalMatchPrice + GET_PRICE_LEN(&p->lenEnc, posState, len);
          unsigned lenToPosState = GetLenToPosState(len);
       
          if (dist < kNumFullDistances)
            price += p->distancesPrices[lenToPosState][dist & (kNumFullDistances - 1)];
          else
          {
            unsigned slot;
            GetPosSlot2(dist, slot)
            price += p->alignPrices[dist & kAlignMask];
            price += p->posSlotPrices[lenToPosState][slot];
          }
          
          opt = &p->opt[len];
          
          if (price < opt->price)
          {
            opt->price = price;
            opt->len = (UInt32)len;
            opt->dist = dist + LZMA_NUM_REPS;
            opt->extra = 0;
          }
          
          if (len == MATCHES[offs])
          {
            offs += 2;
            if (offs == numPairs)
              break;
          }
        }
      }
    }
    

    cur = 0;

    #ifdef SHOW_STAT2
    /* if (position >= 0) */
    {
      unsigned i;
      printf("\n pos = %4X", position);
      for (i = cur; i <= last; i++)
      printf("\nprice[%4X] = %u", position - cur + i, p->opt[i].price);
    }
    #endif
  }


  
  // ---------- Optimal Parsing ----------

  for (;;)
  {
    unsigned numAvail;
    UInt32 numAvailFull;
    unsigned newLen, numPairs, prev, state, posState, startLen;
    UInt32 litPrice, matchPrice, repMatchPrice;
    BoolInt nextIsLit;
    Byte curByte, matchByte;
    const Byte *data;
    COptimal *curOpt, *nextOpt;

    if (++cur == last)
      break;
    
    // 18.06
    if (cur >= kNumOpts - 64)
    {
      unsigned j, best;
      UInt32 price = p->opt[cur].price;
      best = cur;
      for (j = cur + 1; j <= last; j++)
      {
        UInt32 price2 = p->opt[j].price;
        if (price >= price2)
        {
          price = price2;
          best = j;
        }
      }
      {
        unsigned delta = best - cur;
        if (delta != 0)
        {
          MOVE_POS(p, delta)
        }
      }
      cur = best;
      break;
    }

    newLen = ReadMatchDistances(p, &numPairs);
    
    if (newLen >= p->numFastBytes)
    {
      p->numPairs = numPairs;
      p->longestMatchLen = newLen;
      break;
    }
    
    curOpt = &p->opt[cur];

    position++;

    // we need that check here, if skip_items in p->opt are possible
    /*
    if (curOpt->price >= kInfinityPrice)
      continue;
    */

    prev = cur - curOpt->len;

    if (curOpt->len == 1)
    {
      state = (unsigned)p->opt[prev].state;
      if (IsShortRep(curOpt))
        state = kShortRepNextStates[state];
      else
        state = kLiteralNextStates[state];
    }
    else
    {
      const COptimal *prevOpt;
      UInt32 b0;
      UInt32 dist = curOpt->dist;

      if (curOpt->extra)
      {
        prev -= (unsigned)curOpt->extra;
        state = kState_RepAfterLit;
        if (curOpt->extra == 1)
          state = (dist < LZMA_NUM_REPS ? kState_RepAfterLit : kState_MatchAfterLit);
      }
      else
      {
        state = (unsigned)p->opt[prev].state;
        if (dist < LZMA_NUM_REPS)
          state = kRepNextStates[state];
        else
          state = kMatchNextStates[state];
      }

      prevOpt = &p->opt[prev];
      b0 = prevOpt->reps[0];

      if (dist < LZMA_NUM_REPS)
      {
        if (dist == 0)
        {
          reps[0] = b0;
          reps[1] = prevOpt->reps[1];
          reps[2] = prevOpt->reps[2];
          reps[3] = prevOpt->reps[3];
        }
        else
        {
          reps[1] = b0;
          b0 = prevOpt->reps[1];
          if (dist == 1)
          {
            reps[0] = b0;
            reps[2] = prevOpt->reps[2];
            reps[3] = prevOpt->reps[3];
          }
          else
          {
            reps[2] = b0;
            reps[0] = prevOpt->reps[dist];
            reps[3] = prevOpt->reps[dist ^ 1];
          }
        }
      }
      else
      {
        reps[0] = (dist - LZMA_NUM_REPS + 1);
        reps[1] = b0;
        reps[2] = prevOpt->reps[1];
        reps[3] = prevOpt->reps[2];
      }
    }
    
    curOpt->state = (CState)state;
    curOpt->reps[0] = reps[0];
    curOpt->reps[1] = reps[1];
    curOpt->reps[2] = reps[2];
    curOpt->reps[3] = reps[3];

    data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
    curByte = *data;
    matchByte = *(data - reps[0]);

    posState = (position & p->pbMask);

    /*
    The order of Price checks:
       <  LIT
       <= SHORT_REP
       <  LIT : REP_0
       <  REP    [ : LIT : REP_0 ]
       <  MATCH  [ : LIT : REP_0 ]
    */

    {
      UInt32 curPrice = curOpt->price;
      unsigned prob = p->isMatch[state][posState];
      matchPrice = curPrice + GET_PRICE_1(prob);
      litPrice = curPrice + GET_PRICE_0(prob);
    }

    nextOpt = &p->opt[(size_t)cur + 1];
    nextIsLit = False;

    // here we can allow skip_items in p->opt, if we don't check (nextOpt->price < kInfinityPrice)
    // 18.new.06
    if ((nextOpt->price < kInfinityPrice
        // && !IsLitState(state)
        && matchByte == curByte)
        || litPrice > nextOpt->price
        )
      litPrice = 0;
    else
    {
      const CLzmaProb *probs = LIT_PROBS(position, *(data - 1));
      litPrice += (!IsLitState(state) ?
          LitEnc_Matched_GetPrice(probs, curByte, matchByte, p->ProbPrices) :
          LitEnc_GetPrice(probs, curByte, p->ProbPrices));
      
      if (litPrice < nextOpt->price)
      {
        nextOpt->price = litPrice;
        nextOpt->len = 1;
        MakeAs_Lit(nextOpt)
        nextIsLit = True;
      }
    }

    repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[state]);
    
    numAvailFull = p->numAvail;
    {
      unsigned temp = kNumOpts - 1 - cur;
      if (numAvailFull > temp)
        numAvailFull = (UInt32)temp;
    }

    // 18.06
    // ---------- SHORT_REP ----------
    if (IsLitState(state)) // 18.new
    if (matchByte == curByte)
    if (repMatchPrice < nextOpt->price) // 18.new
    // if (numAvailFull < 2 || data[1] != *(data - reps[0] + 1))
    if (
        // nextOpt->price >= kInfinityPrice ||
        nextOpt->len < 2   // we can check nextOpt->len, if skip items are not allowed in p->opt
        || (nextOpt->dist != 0
            // && nextOpt->extra <= 1 // 17.old
            )
        )
    {
      UInt32 shortRepPrice = repMatchPrice + GetPrice_ShortRep(p, state, posState);
      // if (shortRepPrice <= nextOpt->price) // 17.old
      if (shortRepPrice < nextOpt->price)  // 18.new
      {
        nextOpt->price = shortRepPrice;
        nextOpt->len = 1;
        MakeAs_ShortRep(nextOpt)
        nextIsLit = False;
      }
    }
    
    if (numAvailFull < 2)
      continue;
    numAvail = (numAvailFull <= p->numFastBytes ? numAvailFull : p->numFastBytes);

    // numAvail <= p->numFastBytes

    // ---------- LIT : REP_0 ----------

    if (!nextIsLit
        && litPrice != 0 // 18.new
        && matchByte != curByte
        && numAvailFull > 2)
    {
      const Byte *data2 = data - reps[0];
      if (data[1] == data2[1] && data[2] == data2[2])
      {
        unsigned len;
        unsigned limit = p->numFastBytes + 1;
        if (limit > numAvailFull)
          limit = numAvailFull;
        for (len = 3; len < limit && data[len] == data2[len]; len++)
        {}
        
        {
          unsigned state2 = kLiteralNextStates[state];
          unsigned posState2 = (position + 1) & p->pbMask;
          UInt32 price = litPrice + GetPrice_Rep_0(p, state2, posState2);
          {
            unsigned offset = cur + len;

            if (last < offset)
              last = offset;
          
            // do
            {
              UInt32 price2;
              COptimal *opt;
              len--;
              // price2 = price + GetPrice_Len_Rep_0(p, len, state2, posState2);
              price2 = price + GET_PRICE_LEN(&p->repLenEnc, posState2, len);

              opt = &p->opt[offset];
              // offset--;
              if (price2 < opt->price)
              {
                opt->price = price2;
                opt->len = (UInt32)len;
                opt->dist = 0;
                opt->extra = 1;
              }
            }
            // while (len >= 3);
          }
        }
      }
    }
    
    startLen = 2; /* speed optimization */

    {
      // ---------- REP ----------
      unsigned repIndex = 0; // 17.old
      // unsigned repIndex = IsLitState(state) ? 0 : 1; // 18.notused
      for (; repIndex < LZMA_NUM_REPS; repIndex++)
      {
        unsigned len;
        UInt32 price;
        const Byte *data2 = data - reps[repIndex];
        if (data[0] != data2[0] || data[1] != data2[1])
          continue;
        
        for (len = 2; len < numAvail && data[len] == data2[len]; len++)
        {}
        
        // if (len < startLen) continue; // 18.new: speed optimization

        {
          unsigned offset = cur + len;
          if (last < offset)
            last = offset;
        }
        {
          unsigned len2 = len;
          price = repMatchPrice + GetPrice_PureRep(p, repIndex, state, posState);
          do
          {
            UInt32 price2 = price + GET_PRICE_LEN(&p->repLenEnc, posState, len2);
            COptimal *opt = &p->opt[cur + len2];
            if (price2 < opt->price)
            {
              opt->price = price2;
              opt->len = (UInt32)len2;
              opt->dist = (UInt32)repIndex;
              opt->extra = 0;
            }
          }
          while (--len2 >= 2);
        }
        
        if (repIndex == 0) startLen = len + 1;  // 17.old
        // startLen = len + 1; // 18.new

        /* if (_maxMode) */
        {
          // ---------- REP : LIT : REP_0 ----------
          // numFastBytes + 1 + numFastBytes

          unsigned len2 = len + 1;
          unsigned limit = len2 + p->numFastBytes;
          if (limit > numAvailFull)
            limit = numAvailFull;
          
          len2 += 2;
          if (len2 <= limit)
          if (data[len2 - 2] == data2[len2 - 2])
          if (data[len2 - 1] == data2[len2 - 1])
          {
            unsigned state2 = kRepNextStates[state];
            unsigned posState2 = (position + len) & p->pbMask;
            price += GET_PRICE_LEN(&p->repLenEnc, posState, len)
                + GET_PRICE_0(p->isMatch[state2][posState2])
                + LitEnc_Matched_GetPrice(LIT_PROBS(position + len, data[(size_t)len - 1]),
                    data[len], data2[len], p->ProbPrices);
            
            // state2 = kLiteralNextStates[state2];
            state2 = kState_LitAfterRep;
            posState2 = (posState2 + 1) & p->pbMask;


            price += GetPrice_Rep_0(p, state2, posState2);

          for (; len2 < limit && data[len2] == data2[len2]; len2++)
          {}
          
          len2 -= len;
          // if (len2 >= 3)
          {
            {
              unsigned offset = cur + len + len2;

              if (last < offset)
                last = offset;
              // do
              {
                UInt32 price2;
                COptimal *opt;
                len2--;
                // price2 = price + GetPrice_Len_Rep_0(p, len2, state2, posState2);
                price2 = price + GET_PRICE_LEN(&p->repLenEnc, posState2, len2);

                opt = &p->opt[offset];
                // offset--;
                if (price2 < opt->price)
                {
                  opt->price = price2;
                  opt->len = (UInt32)len2;
                  opt->extra = (CExtra)(len + 1);
                  opt->dist = (UInt32)repIndex;
                }
              }
              // while (len2 >= 3);
            }
          }
          }
        }
      }
    }


    // ---------- MATCH ----------
    /* for (unsigned len = 2; len <= newLen; len++) */
    if (newLen > numAvail)
    {
      newLen = numAvail;
      for (numPairs = 0; newLen > MATCHES[numPairs]; numPairs += 2);
      MATCHES[numPairs] = (UInt32)newLen;
      numPairs += 2;
    }
    
    // startLen = 2; /* speed optimization */

    if (newLen >= startLen)
    {
      UInt32 normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[state]);
      UInt32 dist;
      unsigned offs, posSlot, len;
      
      {
        unsigned offset = cur + newLen;
        if (last < offset)
          last = offset;
      }

      offs = 0;
      while (startLen > MATCHES[offs])
        offs += 2;
      dist = MATCHES[(size_t)offs + 1];
      
      // if (dist >= kNumFullDistances)
      GetPosSlot2(dist, posSlot)
      
      for (len = /*2*/ startLen; ; len++)
      {
        UInt32 price = normalMatchPrice + GET_PRICE_LEN(&p->lenEnc, posState, len);
        {
          COptimal *opt;
          unsigned lenNorm = len - 2;
          lenNorm = GetLenToPosState2(lenNorm);
          if (dist < kNumFullDistances)
            price += p->distancesPrices[lenNorm][dist & (kNumFullDistances - 1)];
          else
            price += p->posSlotPrices[lenNorm][posSlot] + p->alignPrices[dist & kAlignMask];
          
          opt = &p->opt[cur + len];
          if (price < opt->price)
          {
            opt->price = price;
            opt->len = (UInt32)len;
            opt->dist = dist + LZMA_NUM_REPS;
            opt->extra = 0;
          }
        }

        if (len == MATCHES[offs])
        {
          // if (p->_maxMode) {
          // MATCH : LIT : REP_0

          const Byte *data2 = data - dist - 1;
          unsigned len2 = len + 1;
          unsigned limit = len2 + p->numFastBytes;
          if (limit > numAvailFull)
            limit = numAvailFull;
          
          len2 += 2;
          if (len2 <= limit)
          if (data[len2 - 2] == data2[len2 - 2])
          if (data[len2 - 1] == data2[len2 - 1])
          {
          for (; len2 < limit && data[len2] == data2[len2]; len2++)
          {}
          
          len2 -= len;
          
          // if (len2 >= 3)
          {
            unsigned state2 = kMatchNextStates[state];
            unsigned posState2 = (position + len) & p->pbMask;
            unsigned offset;
            price += GET_PRICE_0(p->isMatch[state2][posState2]);
            price += LitEnc_Matched_GetPrice(LIT_PROBS(position + len, data[(size_t)len - 1]),
                    data[len], data2[len], p->ProbPrices);

            // state2 = kLiteralNextStates[state2];
            state2 = kState_LitAfterMatch;

            posState2 = (posState2 + 1) & p->pbMask;
            price += GetPrice_Rep_0(p, state2, posState2);

            offset = cur + len + len2;

            if (last < offset)
              last = offset;
            // do
            {
              UInt32 price2;
              COptimal *opt;
              len2--;
              // price2 = price + GetPrice_Len_Rep_0(p, len2, state2, posState2);
              price2 = price + GET_PRICE_LEN(&p->repLenEnc, posState2, len2);
              opt = &p->opt[offset];
              // offset--;
              if (price2 < opt->price)
              {
                opt->price = price2;
                opt->len = (UInt32)len2;
                opt->extra = (CExtra)(len + 1);
                opt->dist = dist + LZMA_NUM_REPS;
              }
            }
            // while (len2 >= 3);
          }

          }
        
          offs += 2;
          if (offs == numPairs)
            break;
          dist = MATCHES[(size_t)offs + 1];
          // if (dist >= kNumFullDistances)
            GetPosSlot2(dist, posSlot)
        }
      }
    }
  }

  do
    p->opt[last].price = kInfinityPrice;
  while (--last);

  return Backward(p, cur);
}



#define ChangePair(smallDist, bigDist) (((bigDist) >> 7) > (smallDist))



static unsigned GetOptimumFast(CLzmaEnc *p)
{
  UInt32 numAvail, mainDist;
  unsigned mainLen, numPairs, repIndex, repLen, i;
  const Byte *data;

  if (p->additionalOffset == 0)
    mainLen = ReadMatchDistances(p, &numPairs);
  else
  {
    mainLen = p->longestMatchLen;
    numPairs = p->numPairs;
  }

  numAvail = p->numAvail;
  p->backRes = MARK_LIT;
  if (numAvail < 2)
    return 1;
  // if (mainLen < 2 && p->state == 0) return 1; // 18.06.notused
  if (numAvail > LZMA_MATCH_LEN_MAX)
    numAvail = LZMA_MATCH_LEN_MAX;
  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
  repLen = repIndex = 0;
  
  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
    unsigned len;
    const Byte *data2 = data - p->reps[i];
    if (data[0] != data2[0] || data[1] != data2[1])
      continue;
    for (len = 2; len < numAvail && data[len] == data2[len]; len++)
    {}
    if (len >= p->numFastBytes)
    {
      p->backRes = (UInt32)i;
      MOVE_POS(p, len - 1)
      return len;
    }
    if (len > repLen)
    {
      repIndex = i;
      repLen = len;
    }
  }

  if (mainLen >= p->numFastBytes)
  {
    p->backRes = p->matches[(size_t)numPairs - 1] + LZMA_NUM_REPS;
    MOVE_POS(p, mainLen - 1)
    return mainLen;
  }

  mainDist = 0; /* for GCC */
  
  if (mainLen >= 2)
  {
    mainDist = p->matches[(size_t)numPairs - 1];
    while (numPairs > 2)
    {
      UInt32 dist2;
      if (mainLen != p->matches[(size_t)numPairs - 4] + 1)
        break;
      dist2 = p->matches[(size_t)numPairs - 3];
      if (!ChangePair(dist2, mainDist))
        break;
      numPairs -= 2;
      mainLen--;
      mainDist = dist2;
    }
    if (mainLen == 2 && mainDist >= 0x80)
      mainLen = 1;
  }

  if (repLen >= 2)
    if (    repLen + 1 >= mainLen
        || (repLen + 2 >= mainLen && mainDist >= (1 << 9))
        || (repLen + 3 >= mainLen && mainDist >= (1 << 15)))
  {
    p->backRes = (UInt32)repIndex;
    MOVE_POS(p, repLen - 1)
    return repLen;
  }
  
  if (mainLen < 2 || numAvail <= 2)
    return 1;

  {
    unsigned len1 = ReadMatchDistances(p, &p->numPairs);
    p->longestMatchLen = len1;
  
    if (len1 >= 2)
    {
      UInt32 newDist = p->matches[(size_t)p->numPairs - 1];
      if (   (len1 >= mainLen && newDist < mainDist)
          || (len1 == mainLen + 1 && !ChangePair(mainDist, newDist))
          || (len1 >  mainLen + 1)
          || (len1 + 1 >= mainLen && mainLen >= 3 && ChangePair(newDist, mainDist)))
        return 1;
    }
  }
  
  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
  
  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
    unsigned len, limit;
    const Byte *data2 = data - p->reps[i];
    if (data[0] != data2[0] || data[1] != data2[1])
      continue;
    limit = mainLen - 1;
    for (len = 2;; len++)
    {
      if (len >= limit)
        return 1;
      if (data[len] != data2[len])
        break;
    }
  }
  
  p->backRes = mainDist + LZMA_NUM_REPS;
  if (mainLen != 2)
  {
    MOVE_POS(p, mainLen - 2)
  }
  return mainLen;
}




static void WriteEndMarker(CLzmaEnc *p, unsigned posState)
{
  UInt32 range;
  range = p->rc.range;
  {
    UInt32 ttt, newBound;
    CLzmaProb *prob = &p->isMatch[p->state][posState];
    RC_BIT_PRE(&p->rc, prob)
    RC_BIT_1(&p->rc, prob)
    prob = &p->isRep[p->state];
    RC_BIT_PRE(&p->rc, prob)
    RC_BIT_0(&p->rc, prob)
  }
  p->state = kMatchNextStates[p->state];
  
  p->rc.range = range;
  LenEnc_Encode(&p->lenProbs, &p->rc, 0, posState);
  range = p->rc.range;

  {
    // RcTree_Encode_PosSlot(&p->rc, p->posSlotEncoder[0], (1 << kNumPosSlotBits) - 1);
    CLzmaProb *probs = p->posSlotEncoder[0];
    unsigned m = 1;
    do
    {
      UInt32 ttt, newBound;
      RC_BIT_PRE(p, probs + m)
      RC_BIT_1(&p->rc, probs + m)
      m = (m << 1) + 1;
    }
    while (m < (1 << kNumPosSlotBits));
  }
  {
    // RangeEnc_EncodeDirectBits(&p->rc, ((UInt32)1 << (30 - kNumAlignBits)) - 1, 30 - kNumAlignBits);    UInt32 range = p->range;
    unsigned numBits = 30 - kNumAlignBits;
    do
    {
      range >>= 1;
      p->rc.low += range;
      RC_NORM(&p->rc)
    }
    while (--numBits);
  }
   
  {
    // RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, kAlignMask);
    CLzmaProb *probs = p->posAlignEncoder;
    unsigned m = 1;
    do
    {
      UInt32 ttt, newBound;
      RC_BIT_PRE(p, probs + m)
      RC_BIT_1(&p->rc, probs + m)
      m = (m << 1) + 1;
    }
    while (m < kAlignTableSize);
  }
  p->rc.range = range;
}


static SRes CheckErrors(CLzmaEnc *p)
{
  if (p->result != SZ_OK)
    return p->result;
  if (p->rc.res != SZ_OK)
    p->result = SZ_ERROR_WRITE;

  #ifndef Z7_ST
  if (
      // p->mf_Failure ||
        (p->mtMode &&
          ( // p->matchFinderMt.failure_LZ_LZ ||
            p->matchFinderMt.failure_LZ_BT))
     )
  {
    p->result = MY_HRES_ERROR_INTERNAL_ERROR;
    // printf("\nCheckErrors p->matchFinderMt.failureLZ\n");
  }
  #endif

  if (MFB.result != SZ_OK)
    p->result = SZ_ERROR_READ;
  
  if (p->result != SZ_OK)
    p->finished = True;
  return p->result;
}


Z7_NO_INLINE static SRes Flush(CLzmaEnc *p, UInt32 nowPos)
{
  /* ReleaseMFStream(); */
  p->finished = True;
  if (p->writeEndMark)
    WriteEndMarker(p, nowPos & p->pbMask);
  RangeEnc_FlushData(&p->rc);
  RangeEnc_FlushStream(&p->rc);
  return CheckErrors(p);
}


Z7_NO_INLINE static void FillAlignPrices(CLzmaEnc *p)
{
  unsigned i;
  const CProbPrice *ProbPrices = p->ProbPrices;
  const CLzmaProb *probs = p->posAlignEncoder;
  // p->alignPriceCount = 0;
  for (i = 0; i < kAlignTableSize / 2; i++)
  {
    UInt32 price = 0;
    unsigned sym = i;
    unsigned m = 1;
    unsigned bit;
    UInt32 prob;
    bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[m], bit); m = (m << 1) + bit;
    bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[m], bit); m = (m << 1) + bit;
    bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[m], bit); m = (m << 1) + bit;
    prob = probs[m];
    p->alignPrices[i    ] = price + GET_PRICEa_0(prob);
    p->alignPrices[i + 8] = price + GET_PRICEa_1(prob);
    // p->alignPrices[i] = RcTree_ReverseGetPrice(p->posAlignEncoder, kNumAlignBits, i, p->ProbPrices);
  }
}


Z7_NO_INLINE static void FillDistancesPrices(CLzmaEnc *p)
{
  // int y; for (y = 0; y < 100; y++) {

  UInt32 tempPrices[kNumFullDistances];
  unsigned i, lps;

  const CProbPrice *ProbPrices = p->ProbPrices;
  p->matchPriceCount = 0;

  for (i = kStartPosModelIndex / 2; i < kNumFullDistances / 2; i++)
  {
    unsigned posSlot = GetPosSlot1(i);
    unsigned footerBits = (posSlot >> 1) - 1;
    unsigned base = ((2 | (posSlot & 1)) << footerBits);
    const CLzmaProb *probs = p->posEncoders + (size_t)base * 2;
    // tempPrices[i] = RcTree_ReverseGetPrice(p->posEncoders + base, footerBits, i - base, p->ProbPrices);
    UInt32 price = 0;
    unsigned m = 1;
    unsigned sym = i;
    unsigned offset = (unsigned)1 << footerBits;
    base += i;
    
    if (footerBits)
    do
    {
      unsigned bit = sym & 1;
      sym >>= 1;
      price += GET_PRICEa(probs[m], bit);
      m = (m << 1) + bit;
    }
    while (--footerBits);

    {
      unsigned prob = probs[m];
      tempPrices[base         ] = price + GET_PRICEa_0(prob);
      tempPrices[base + offset] = price + GET_PRICEa_1(prob);
    }
  }

  for (lps = 0; lps < kNumLenToPosStates; lps++)
  {
    unsigned slot;
    unsigned distTableSize2 = (p->distTableSize + 1) >> 1;
    UInt32 *posSlotPrices = p->posSlotPrices[lps];
    const CLzmaProb *probs = p->posSlotEncoder[lps];
    
    for (slot = 0; slot < distTableSize2; slot++)
    {
      // posSlotPrices[slot] = RcTree_GetPrice(encoder, kNumPosSlotBits, slot, p->ProbPrices);
      UInt32 price;
      unsigned bit;
      unsigned sym = slot + (1 << (kNumPosSlotBits - 1));
      unsigned prob;
      bit = sym & 1; sym >>= 1; price  = GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      prob = probs[(size_t)slot + (1 << (kNumPosSlotBits - 1))];
      posSlotPrices[(size_t)slot * 2    ] = price + GET_PRICEa_0(prob);
      posSlotPrices[(size_t)slot * 2 + 1] = price + GET_PRICEa_1(prob);
    }
    
    {
      UInt32 delta = ((UInt32)((kEndPosModelIndex / 2 - 1) - kNumAlignBits) << kNumBitPriceShiftBits);
      for (slot = kEndPosModelIndex / 2; slot < distTableSize2; slot++)
      {
        posSlotPrices[(size_t)slot * 2    ] += delta;
        posSlotPrices[(size_t)slot * 2 + 1] += delta;
        delta += ((UInt32)1 << kNumBitPriceShiftBits);
      }
    }

    {
      UInt32 *dp = p->distancesPrices[lps];
      
      dp[0] = posSlotPrices[0];
      dp[1] = posSlotPrices[1];
      dp[2] = posSlotPrices[2];
      dp[3] = posSlotPrices[3];

      for (i = 4; i < kNumFullDistances; i += 2)
      {
        UInt32 slotPrice = posSlotPrices[GetPosSlot1(i)];
        dp[i    ] = slotPrice + tempPrices[i];
        dp[i + 1] = slotPrice + tempPrices[i + 1];
      }
    }
  }
  // }
}



static void LzmaEnc_Construct(CLzmaEnc *p)
{
  RangeEnc_Construct(&p->rc);
  MatchFinder_Construct(&MFB);
  
  #ifndef Z7_ST
  p->matchFinderMt.MatchFinder = &MFB;
  MatchFinderMt_Construct(&p->matchFinderMt);
  #endif

  {
    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    LzmaEnc_SetProps((CLzmaEncHandle)(void *)p, &props);
  }

  #ifndef LZMA_LOG_BSR
  LzmaEnc_FastPosInit(p->g_FastPos);
  #endif

  LzmaEnc_InitPriceTables(p->ProbPrices);
  p->litProbs = NULL;
  p->saveState.litProbs = NULL;
}

CLzmaEncHandle LzmaEnc_Create(ISzAllocPtr alloc)
{
  CLzmaEncHandle p = (CLzmaEncHandle)ISzAlloc_Alloc(alloc, sizeof(CLzmaEnc));
  if (p)
    LzmaEnc_Construct(p);
  return p;
}

static void LzmaEnc_FreeLits(CLzmaEnc *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->litProbs);
  ISzAlloc_Free(alloc, p->saveState.litProbs);
  p->litProbs = NULL;
  p->saveState.litProbs = NULL;
}

static void LzmaEnc_Destruct(CLzmaEnc *p, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  #ifndef Z7_ST
  MatchFinderMt_Destruct(&p->matchFinderMt, allocBig);
  #endif
  
  MatchFinder_Free(&MFB, allocBig);
  LzmaEnc_FreeLits(p, alloc);
  RangeEnc_Free(&p->rc, alloc);
}

void LzmaEnc_Destroy(CLzmaEncHandle p, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  // GET_CLzmaEnc_p
  LzmaEnc_Destruct(p, alloc, allocBig);
  ISzAlloc_Free(alloc, p);
}


Z7_NO_INLINE
static SRes LzmaEnc_CodeOneBlock(CLzmaEnc *p, UInt32 maxPackSize, UInt32 maxUnpackSize)
{
  UInt32 nowPos32, startPos32;
  if (p->needInit)
  {
    #ifndef Z7_ST
    if (p->mtMode)
    {
      RINOK(MatchFinderMt_InitMt(&p->matchFinderMt))
    }
    #endif
    p->matchFinder.Init(p->matchFinderObj);
    p->needInit = 0;
  }

  if (p->finished)
    return p->result;
  RINOK(CheckErrors(p))

  nowPos32 = (UInt32)p->nowPos64;
  startPos32 = nowPos32;

  if (p->nowPos64 == 0)
  {
    unsigned numPairs;
    Byte curByte;
    if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
      return Flush(p, nowPos32);
    ReadMatchDistances(p, &numPairs);
    RangeEnc_EncodeBit_0(&p->rc, &p->isMatch[kState_Start][0]);
    // p->state = kLiteralNextStates[p->state];
    curByte = *(p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset);
    LitEnc_Encode(&p->rc, p->litProbs, curByte);
    p->additionalOffset--;
    nowPos32++;
  }

  if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) != 0)
  
  for (;;)
  {
    UInt32 dist;
    unsigned len, posState;
    UInt32 range, ttt, newBound;
    CLzmaProb *probs;
  
    if (p->fastMode)
      len = GetOptimumFast(p);
    else
    {
      unsigned oci = p->optCur;
      if (p->optEnd == oci)
        len = GetOptimum(p, nowPos32);
      else
      {
        const COptimal *opt = &p->opt[oci];
        len = opt->len;
        p->backRes = opt->dist;
        p->optCur = oci + 1;
      }
    }

    posState = (unsigned)nowPos32 & p->pbMask;
    range = p->rc.range;
    probs = &p->isMatch[p->state][posState];
    
    RC_BIT_PRE(&p->rc, probs)
    
    dist = p->backRes;

    #ifdef SHOW_STAT2
    printf("\n pos = %6X, len = %3u  pos = %6u", nowPos32, len, dist);
    #endif

    if (dist == MARK_LIT)
    {
      Byte curByte;
      const Byte *data;
      unsigned state;

      RC_BIT_0(&p->rc, probs)
      p->rc.range = range;
      data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
      probs = LIT_PROBS(nowPos32, *(data - 1));
      curByte = *data;
      state = p->state;
      p->state = kLiteralNextStates[state];
      if (IsLitState(state))
        LitEnc_Encode(&p->rc, probs, curByte);
      else
        LitEnc_EncodeMatched(&p->rc, probs, curByte, *(data - p->reps[0]));
    }
    else
    {
      RC_BIT_1(&p->rc, probs)
      probs = &p->isRep[p->state];
      RC_BIT_PRE(&p->rc, probs)
      
      if (dist < LZMA_NUM_REPS)
      {
        RC_BIT_1(&p->rc, probs)
        probs = &p->isRepG0[p->state];
        RC_BIT_PRE(&p->rc, probs)
        if (dist == 0)
        {
          RC_BIT_0(&p->rc, probs)
          probs = &p->isRep0Long[p->state][posState];
          RC_BIT_PRE(&p->rc, probs)
          if (len != 1)
          {
            RC_BIT_1_BASE(&p->rc, probs)
          }
          else
          {
            RC_BIT_0_BASE(&p->rc, probs)
            p->state = kShortRepNextStates[p->state];
          }
        }
        else
        {
          RC_BIT_1(&p->rc, probs)
          probs = &p->isRepG1[p->state];
          RC_BIT_PRE(&p->rc, probs)
          if (dist == 1)
          {
            RC_BIT_0_BASE(&p->rc, probs)
            dist = p->reps[1];
          }
          else
          {
            RC_BIT_1(&p->rc, probs)
            probs = &p->isRepG2[p->state];
            RC_BIT_PRE(&p->rc, probs)
            if (dist == 2)
            {
              RC_BIT_0_BASE(&p->rc, probs)
              dist = p->reps[2];
            }
            else
            {
              RC_BIT_1_BASE(&p->rc, probs)
              dist = p->reps[3];
              p->reps[3] = p->reps[2];
            }
            p->reps[2] = p->reps[1];
          }
          p->reps[1] = p->reps[0];
          p->reps[0] = dist;
        }

        RC_NORM(&p->rc)

        p->rc.range = range;

        if (len != 1)
        {
          LenEnc_Encode(&p->repLenProbs, &p->rc, len - LZMA_MATCH_LEN_MIN, posState);
          --p->repLenEncCounter;
          p->state = kRepNextStates[p->state];
        }
      }
      else
      {
        unsigned posSlot;
        RC_BIT_0(&p->rc, probs)
        p->rc.range = range;
        p->state = kMatchNextStates[p->state];

        LenEnc_Encode(&p->lenProbs, &p->rc, len - LZMA_MATCH_LEN_MIN, posState);
        // --p->lenEnc.counter;

        dist -= LZMA_NUM_REPS;
        p->reps[3] = p->reps[2];
        p->reps[2] = p->reps[1];
        p->reps[1] = p->reps[0];
        p->reps[0] = dist + 1;
        
        p->matchPriceCount++;
        GetPosSlot(dist, posSlot)
        // RcTree_Encode_PosSlot(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], posSlot);
        {
          UInt32 sym = (UInt32)posSlot + (1 << kNumPosSlotBits);
          range = p->rc.range;
          probs = p->posSlotEncoder[GetLenToPosState(len)];
          do
          {
            CLzmaProb *prob = probs + (sym >> kNumPosSlotBits);
            UInt32 bit = (sym >> (kNumPosSlotBits - 1)) & 1;
            sym <<= 1;
            RC_BIT(&p->rc, prob, bit)
          }
          while (sym < (1 << kNumPosSlotBits * 2));
          p->rc.range = range;
        }
        
        if (dist >= kStartPosModelIndex)
        {
          unsigned footerBits = ((posSlot >> 1) - 1);

          if (dist < kNumFullDistances)
          {
            unsigned base = ((2 | (posSlot & 1)) << footerBits);
            RcTree_ReverseEncode(&p->rc, p->posEncoders + base, footerBits, (unsigned)(dist /* - base */));
          }
          else
          {
            UInt32 pos2 = (dist | 0xF) << (32 - footerBits);
            range = p->rc.range;
            // RangeEnc_EncodeDirectBits(&p->rc, posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
            /*
            do
            {
              range >>= 1;
              p->rc.low += range & (0 - ((dist >> --footerBits) & 1));
              RC_NORM(&p->rc)
            }
            while (footerBits > kNumAlignBits);
            */
            do
            {
              range >>= 1;
              p->rc.low += range & (0 - (pos2 >> 31));
              pos2 += pos2;
              RC_NORM(&p->rc)
            }
            while (pos2 != 0xF0000000);


            // RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, posReduced & kAlignMask);

            {
              unsigned m = 1;
              unsigned bit;
              bit = dist & 1; dist >>= 1; RC_BIT(&p->rc, p->posAlignEncoder + m, bit)  m = (m << 1) + bit;
              bit = dist & 1; dist >>= 1; RC_BIT(&p->rc, p->posAlignEncoder + m, bit)  m = (m << 1) + bit;
              bit = dist & 1; dist >>= 1; RC_BIT(&p->rc, p->posAlignEncoder + m, bit)  m = (m << 1) + bit;
              bit = dist & 1;             RC_BIT(&p->rc, p->posAlignEncoder + m, bit)
              p->rc.range = range;
              // p->alignPriceCount++;
            }
          }
        }
      }
    }

    nowPos32 += (UInt32)len;
    p->additionalOffset -= len;
    
    if (p->additionalOffset == 0)
    {
      UInt32 processed;

      if (!p->fastMode)
      {
        /*
        if (p->alignPriceCount >= 16) // kAlignTableSize
          FillAlignPrices(p);
        if (p->matchPriceCount >= 128)
          FillDistancesPrices(p);
        if (p->lenEnc.counter <= 0)
          LenPriceEnc_UpdateTables(&p->lenEnc, 1 << p->pb, &p->lenProbs, p->ProbPrices);
        */
        if (p->matchPriceCount >= 64)
        {
          FillAlignPrices(p);
          // { int y; for (y = 0; y < 100; y++) {
          FillDistancesPrices(p);
          // }}
          LenPriceEnc_UpdateTables(&p->lenEnc, (unsigned)1 << p->pb, &p->lenProbs, p->ProbPrices);
        }
        if (p->repLenEncCounter <= 0)
        {
          p->repLenEncCounter = REP_LEN_COUNT;
          LenPriceEnc_UpdateTables(&p->repLenEnc, (unsigned)1 << p->pb, &p->repLenProbs, p->ProbPrices);
        }
      }
    
      if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
        break;
      processed = nowPos32 - startPos32;
      
      if (maxPackSize)
      {
        if (processed + kNumOpts + 300 >= maxUnpackSize
            || RangeEnc_GetProcessed_sizet(&p->rc) + kPackReserve >= maxPackSize)
          break;
      }
      else if (processed >= (1 << 17))
      {
        p->nowPos64 += nowPos32 - startPos32;
        return CheckErrors(p);
      }
    }
  }

  p->nowPos64 += nowPos32 - startPos32;
  return Flush(p, nowPos32);
}



#define kBigHashDicLimit ((UInt32)1 << 24)

static SRes LzmaEnc_Alloc(CLzmaEnc *p, UInt32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  UInt32 beforeSize = kNumOpts;
  UInt32 dictSize;

  if (!RangeEnc_Alloc(&p->rc, alloc))
    return SZ_ERROR_MEM;

  #ifndef Z7_ST
  p->mtMode = (p->multiThread && !p->fastMode && (MFB.btMode != 0));
  #endif

  {
    const unsigned lclp = p->lc + p->lp;
    if (!p->litProbs || !p->saveState.litProbs || p->lclp != lclp)
    {
      LzmaEnc_FreeLits(p, alloc);
      p->litProbs =           (CLzmaProb *)ISzAlloc_Alloc(alloc, ((size_t)0x300 * sizeof(CLzmaProb)) << lclp);
      p->saveState.litProbs = (CLzmaProb *)ISzAlloc_Alloc(alloc, ((size_t)0x300 * sizeof(CLzmaProb)) << lclp);
      if (!p->litProbs || !p->saveState.litProbs)
      {
        LzmaEnc_FreeLits(p, alloc);
        return SZ_ERROR_MEM;
      }
      p->lclp = lclp;
    }
  }

  MFB.bigHash = (Byte)(p->dictSize > kBigHashDicLimit ? 1 : 0);


  dictSize = p->dictSize;
  if (dictSize == ((UInt32)2 << 30) ||
      dictSize == ((UInt32)3 << 30))
  {
    /* 21.03 : here we reduce the dictionary for 2 reasons:
       1) we don't want 32-bit back_distance matches in decoder for 2 GB dictionary.
       2) we want to elimate useless last MatchFinder_Normalize3() for corner cases,
          where data size is aligned for 1 GB: 5/6/8 GB.
          That reducing must be >= 1 for such corner cases. */
    dictSize -= 1;
  }

  if (beforeSize + dictSize < keepWindowSize)
    beforeSize = keepWindowSize - dictSize;

  /* in worst case we can look ahead for
        max(LZMA_MATCH_LEN_MAX, numFastBytes + 1 + numFastBytes) bytes.
     we send larger value for (keepAfter) to MantchFinder_Create():
        (numFastBytes + LZMA_MATCH_LEN_MAX + 1)
  */

  #ifndef Z7_ST
  if (p->mtMode)
  {
    RINOK(MatchFinderMt_Create(&p->matchFinderMt, dictSize, beforeSize,
        p->numFastBytes, LZMA_MATCH_LEN_MAX + 1 /* 18.04 */
        , allocBig))
    p->matchFinderObj = &p->matchFinderMt;
    MFB.bigHash = (Byte)(MFB.hashMask >= 0xFFFFFF ? 1 : 0);
    MatchFinderMt_CreateVTable(&p->matchFinderMt, &p->matchFinder);
  }
  else
  #endif
  {
    if (!MatchFinder_Create(&MFB, dictSize, beforeSize,
        p->numFastBytes, LZMA_MATCH_LEN_MAX + 1 /* 21.03 */
        , allocBig))
      return SZ_ERROR_MEM;
    p->matchFinderObj = &MFB;
    MatchFinder_CreateVTable(&MFB, &p->matchFinder);
  }
  
  return SZ_OK;
}

static void LzmaEnc_Init(CLzmaEnc *p)
{
  unsigned i;
  p->state = 0;
  p->reps[0] =
  p->reps[1] =
  p->reps[2] =
  p->reps[3] = 1;

  RangeEnc_Init(&p->rc);

  for (i = 0; i < (1 << kNumAlignBits); i++)
    p->posAlignEncoder[i] = kProbInitValue;

  for (i = 0; i < kNumStates; i++)
  {
    unsigned j;
    for (j = 0; j < LZMA_NUM_PB_STATES_MAX; j++)
    {
      p->isMatch[i][j] = kProbInitValue;
      p->isRep0Long[i][j] = kProbInitValue;
    }
    p->isRep[i] = kProbInitValue;
    p->isRepG0[i] = kProbInitValue;
    p->isRepG1[i] = kProbInitValue;
    p->isRepG2[i] = kProbInitValue;
  }

  {
    for (i = 0; i < kNumLenToPosStates; i++)
    {
      CLzmaProb *probs = p->posSlotEncoder[i];
      unsigned j;
      for (j = 0; j < (1 << kNumPosSlotBits); j++)
        probs[j] = kProbInitValue;
    }
  }
  {
    for (i = 0; i < kNumFullDistances; i++)
      p->posEncoders[i] = kProbInitValue;
  }

  {
    const size_t num = (size_t)0x300 << (p->lp + p->lc);
    size_t k;
    CLzmaProb *probs = p->litProbs;
    for (k = 0; k < num; k++)
      probs[k] = kProbInitValue;
  }


  LenEnc_Init(&p->lenProbs);
  LenEnc_Init(&p->repLenProbs);

  p->optEnd = 0;
  p->optCur = 0;

  {
    for (i = 0; i < kNumOpts; i++)
      p->opt[i].price = kInfinityPrice;
  }

  p->additionalOffset = 0;

  p->pbMask = ((unsigned)1 << p->pb) - 1;
  p->lpMask = ((UInt32)0x100 << p->lp) - ((unsigned)0x100 >> p->lc);

  // p->mf_Failure = False;
}


static void LzmaEnc_InitPrices(CLzmaEnc *p)
{
  if (!p->fastMode)
  {
    FillDistancesPrices(p);
    FillAlignPrices(p);
  }

  p->lenEnc.tableSize =
  p->repLenEnc.tableSize =
      p->numFastBytes + 1 - LZMA_MATCH_LEN_MIN;

  p->repLenEncCounter = REP_LEN_COUNT;

  LenPriceEnc_UpdateTables(&p->lenEnc, (unsigned)1 << p->pb, &p->lenProbs, p->ProbPrices);
  LenPriceEnc_UpdateTables(&p->repLenEnc, (unsigned)1 << p->pb, &p->repLenProbs, p->ProbPrices);
}

static SRes LzmaEnc_AllocAndInit(CLzmaEnc *p, UInt32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  unsigned i;
  for (i = kEndPosModelIndex / 2; i < kDicLogSizeMax; i++)
    if (p->dictSize <= ((UInt32)1 << i))
      break;
  p->distTableSize = i * 2;

  p->finished = False;
  p->result = SZ_OK;
  p->nowPos64 = 0;
  p->needInit = 1;
  RINOK(LzmaEnc_Alloc(p, keepWindowSize, alloc, allocBig))
  LzmaEnc_Init(p);
  LzmaEnc_InitPrices(p);
  return SZ_OK;
}

static SRes LzmaEnc_Prepare(CLzmaEncHandle p,
    ISeqOutStreamPtr outStream,
    ISeqInStreamPtr inStream,
    ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  // GET_CLzmaEnc_p
  MatchFinder_SET_STREAM(&MFB, inStream)
  p->rc.outStream = outStream;
  return LzmaEnc_AllocAndInit(p, 0, alloc, allocBig);
}

SRes LzmaEnc_PrepareForLzma2(CLzmaEncHandle p,
    ISeqInStreamPtr inStream, UInt32 keepWindowSize,
    ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  // GET_CLzmaEnc_p
  MatchFinder_SET_STREAM(&MFB, inStream)
  return LzmaEnc_AllocAndInit(p, keepWindowSize, alloc, allocBig);
}

SRes LzmaEnc_MemPrepare(CLzmaEncHandle p,
    const Byte *src, SizeT srcLen,
    UInt32 keepWindowSize,
    ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  // GET_CLzmaEnc_p
  MatchFinder_SET_DIRECT_INPUT_BUF(&MFB, src, srcLen)
  LzmaEnc_SetDataSize(p, srcLen);
  return LzmaEnc_AllocAndInit(p, keepWindowSize, alloc, allocBig);
}

void LzmaEnc_Finish(CLzmaEncHandle p)
{
  #ifndef Z7_ST
  // GET_CLzmaEnc_p
  if (p->mtMode)
    MatchFinderMt_ReleaseStream(&p->matchFinderMt);
  #else
  UNUSED_VAR(p)
  #endif
}


typedef struct
{
  ISeqOutStream vt;
  Byte *data;
  size_t rem;
  BoolInt overflow;
} CLzmaEnc_SeqOutStreamBuf;

static size_t SeqOutStreamBuf_Write(ISeqOutStreamPtr pp, const void *data, size_t size)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CLzmaEnc_SeqOutStreamBuf)
  if (p->rem < size)
  {
    size = p->rem;
    p->overflow = True;
  }
  if (size != 0)
  {
    memcpy(p->data, data, size);
    p->rem -= size;
    p->data += size;
  }
  return size;
}


/*
UInt32 LzmaEnc_GetNumAvailableBytes(CLzmaEncHandle p)
{
  GET_const_CLzmaEnc_p
  return p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
}
*/

const Byte *LzmaEnc_GetCurBuf(CLzmaEncHandle p)
{
  // GET_const_CLzmaEnc_p
  return p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
}


// (desiredPackSize == 0) is not allowed
SRes LzmaEnc_CodeOneMemBlock(CLzmaEncHandle p, BoolInt reInit,
    Byte *dest, size_t *destLen, UInt32 desiredPackSize, UInt32 *unpackSize)
{
  // GET_CLzmaEnc_p
  UInt64 nowPos64;
  SRes res;
  CLzmaEnc_SeqOutStreamBuf outStream;

  outStream.vt.Write = SeqOutStreamBuf_Write;
  outStream.data = dest;
  outStream.rem = *destLen;
  outStream.overflow = False;

  p->writeEndMark = False;
  p->finished = False;
  p->result = SZ_OK;

  if (reInit)
    LzmaEnc_Init(p);
  LzmaEnc_InitPrices(p);
  RangeEnc_Init(&p->rc);
  p->rc.outStream = &outStream.vt;
  nowPos64 = p->nowPos64;
  
  res = LzmaEnc_CodeOneBlock(p, desiredPackSize, *unpackSize);
  
  *unpackSize = (UInt32)(p->nowPos64 - nowPos64);
  *destLen -= outStream.rem;
  if (outStream.overflow)
    return SZ_ERROR_OUTPUT_EOF;

  return res;
}


Z7_NO_INLINE
static SRes LzmaEnc_Encode2(CLzmaEnc *p, ICompressProgressPtr progress)
{
  SRes res = SZ_OK;

  #ifndef Z7_ST
  Byte allocaDummy[0x300];
  allocaDummy[0] = 0;
  allocaDummy[1] = allocaDummy[0];
  #endif

  for (;;)
  {
    res = LzmaEnc_CodeOneBlock(p, 0, 0);
    if (res != SZ_OK || p->finished)
      break;
    if (progress)
    {
      res = ICompressProgress_Progress(progress, p->nowPos64, RangeEnc_GetProcessed(&p->rc));
      if (res != SZ_OK)
      {
        res = SZ_ERROR_PROGRESS;
        break;
      }
    }
  }
  
  LzmaEnc_Finish((CLzmaEncHandle)(void *)p);

  /*
  if (res == SZ_OK && !Inline_MatchFinder_IsFinishedOK(&MFB))
    res = SZ_ERROR_FAIL;
  }
  */

  return res;
}


SRes LzmaEnc_Encode(CLzmaEncHandle p, ISeqOutStreamPtr outStream, ISeqInStreamPtr inStream, ICompressProgressPtr progress,
    ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  // GET_CLzmaEnc_p
  RINOK(LzmaEnc_Prepare(p, outStream, inStream, alloc, allocBig))
  return LzmaEnc_Encode2(p, progress);
}


SRes LzmaEnc_WriteProperties(CLzmaEncHandle p, Byte *props, SizeT *size)
{
  if (*size < LZMA_PROPS_SIZE)
    return SZ_ERROR_PARAM;
  *size = LZMA_PROPS_SIZE;
  {
    // GET_CLzmaEnc_p
    const UInt32 dictSize = p->dictSize;
    UInt32 v;
    props[0] = (Byte)((p->pb * 5 + p->lp) * 9 + p->lc);
    
    // we write aligned dictionary value to properties for lzma decoder
    if (dictSize >= ((UInt32)1 << 21))
    {
      const UInt32 kDictMask = ((UInt32)1 << 20) - 1;
      v = (dictSize + kDictMask) & ~kDictMask;
      if (v < dictSize)
        v = dictSize;
    }
    else
    {
      unsigned i = 11 * 2;
      do
      {
        v = (UInt32)(2 + (i & 1)) << (i >> 1);
        i++;
      }
      while (v < dictSize);
    }

    SetUi32(props + 1, v)
    return SZ_OK;
  }
}


unsigned LzmaEnc_IsWriteEndMark(CLzmaEncHandle p)
{
  // GET_CLzmaEnc_p
  return (unsigned)p->writeEndMark;
}


SRes LzmaEnc_MemEncode(CLzmaEncHandle p, Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
    int writeEndMark, ICompressProgressPtr progress, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  SRes res;
  // GET_CLzmaEnc_p

  CLzmaEnc_SeqOutStreamBuf outStream;

  outStream.vt.Write = SeqOutStreamBuf_Write;
  outStream.data = dest;
  outStream.rem = *destLen;
  outStream.overflow = False;

  p->writeEndMark = writeEndMark;
  p->rc.outStream = &outStream.vt;

  res = LzmaEnc_MemPrepare(p, src, srcLen, 0, alloc, allocBig);
  
  if (res == SZ_OK)
  {
    res = LzmaEnc_Encode2(p, progress);
    if (res == SZ_OK && p->nowPos64 != srcLen)
      res = SZ_ERROR_FAIL;
  }

  *destLen -= (SizeT)outStream.rem;
  if (outStream.overflow)
    return SZ_ERROR_OUTPUT_EOF;
  return res;
}


SRes LzmaEncode(Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
    const CLzmaEncProps *props, Byte *propsEncoded, SizeT *propsSize, int writeEndMark,
    ICompressProgressPtr progress, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  CLzmaEncHandle p = LzmaEnc_Create(alloc);
  SRes res;
  if (!p)
    return SZ_ERROR_MEM;

  res = LzmaEnc_SetProps(p, props);
  if (res == SZ_OK)
  {
    res = LzmaEnc_WriteProperties(p, propsEncoded, propsSize);
    if (res == SZ_OK)
      res = LzmaEnc_MemEncode(p, dest, destLen, src, srcLen,
          writeEndMark, progress, alloc, allocBig);
  }

  LzmaEnc_Destroy(p, alloc, allocBig);
  return res;
}


/*
#ifndef Z7_ST
void LzmaEnc_GetLzThreads(CLzmaEncHandle p, HANDLE lz_threads[2])
{
  GET_const_CLzmaEnc_p
  lz_threads[0] = p->matchFinderMt.hashSync.thread;
  lz_threads[1] = p->matchFinderMt.btSync.thread;
}
#endif
*/

/* ================ unit: C/Md5.c ================ */
/* Md5.c -- MD5 Hash
: Igor Pavlov : Public domain
This code is based on Colin Plumb's public domain md5.c code */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define MD5_UPDATE_BLOCKS(p) Md5_UpdateBlocks

Z7_NO_INLINE
void Md5_Init(CMd5 *p)
{
  p->count = 0;
  p->state[0] = 0x67452301;
  p->state[1] = 0xefcdab89;
  p->state[2] = 0x98badcfe;
  p->state[3] = 0x10325476;
}

#if 0 && !defined(MY_CPU_LE_UNALIGN)
// optional optimization for Big-endian processors or processors without unaligned access:
// it is intended to reduce the number of complex LE32 memory reading from 64 to 16.
// But some compilers (sparc, armt) are better without this optimization.
#define Z7_MD5_USE_DATA32_ARRAY
#endif

#define LOAD_DATA(i)  GetUi32((const UInt32 *)(const void *)data + (i))

#ifdef Z7_MD5_USE_DATA32_ARRAY
#define D(i)  data32[i]
#else
#define D(i)  LOAD_DATA(i)
#endif

#define F1(x, y, z)   (z ^ (x & (y ^ z)))
#define F2(x, y, z)   F1(z, x, y)
#define F3(x, y, z)   (x ^ y ^ z)
#define F4(x, y, z)   (y ^ (x | ~z))

#define R1(i, f, start, step, w, x, y, z, s, k) \
    w += D((start + step * (i)) % 16) + k; \
    w += f(x, y, z); \
    w = rotlFixed(w, s) + x; \

#define R4(i4,  f, start, step, s0,s1,s2,s3, k0,k1,k2,k3) \
    R1 (i4*4+0, f, start, step, a,b,c,d, s0, k0) \
    R1 (i4*4+1, f, start, step, d,a,b,c, s1, k1) \
    R1 (i4*4+2, f, start, step, c,d,a,b, s2, k2) \
    R1 (i4*4+3, f, start, step, b,c,d,a, s3, k3) \

#define R16(f, start, step, s0,s1,s2,s3, k00,k01,k02,k03, k10,k11,k12,k13, k20,k21,k22,k23, k30,k31,k32,k33)  \
    R4 (0,  f, start, step, s0,s1,s2,s3, k00,k01,k02,k03) \
    R4 (1,  f, start, step, s0,s1,s2,s3, k10,k11,k12,k13) \
    R4 (2,  f, start, step, s0,s1,s2,s3, k20,k21,k22,k23) \
    R4 (3,  f, start, step, s0,s1,s2,s3, k30,k31,k32,k33) \

static
Z7_NO_INLINE
void Z7_FASTCALL Md5_UpdateBlocks(UInt32 state[4], const Byte *data, size_t numBlocks)
{
  UInt32 a, b, c, d;
  // if (numBlocks == 0) return;
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  do
  {
#ifdef Z7_MD5_USE_DATA32_ARRAY
    UInt32 data32[MD5_NUM_BLOCK_WORDS];
    {
#define LOAD_data32_x4(i) { \
      data32[i    ] = LOAD_DATA(i    ); \
      data32[i + 1] = LOAD_DATA(i + 1); \
      data32[i + 2] = LOAD_DATA(i + 2); \
      data32[i + 3] = LOAD_DATA(i + 3); }
#if 1
      LOAD_data32_x4 (0 * 4)
      LOAD_data32_x4 (1 * 4)
      LOAD_data32_x4 (2 * 4)
      LOAD_data32_x4 (3 * 4)
#else
      unsigned i;
      for (i = 0; i < MD5_NUM_BLOCK_WORDS; i += 4)
      {
        LOAD_data32_x4(i)
      }
#endif
    }
#endif

    R16 (F1, 0, 1,  7,12,17,22, 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                                0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                                0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                                0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821)
    R16 (F2, 1, 5,  5, 9,14,20, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                                0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                                0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                                0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a)
    R16 (F3, 5, 3,  4,11,16,23, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                                0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                                0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                                0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665)
    R16 (F4, 0, 7,  6,10,15,21, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                                0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                                0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                                0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391)

    a += state[0];
    b += state[1];
    c += state[2];
    d += state[3];
    
    state[0] = a;
    state[1] = b;
    state[2] = c;
    state[3] = d;
    
    data += MD5_BLOCK_SIZE;
  }
  while (--numBlocks);
}


#define Md5_UpdateBlock(p) MD5_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Md5_Update(CMd5 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->count & (MD5_BLOCK_SIZE - 1);
    const unsigned num = MD5_BLOCK_SIZE - pos;
    p->count += size;
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
      Md5_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 6;
    if (numBlocks)
    MD5_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= MD5_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}


void Md5_Final(CMd5 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->count & (MD5_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (MD5_BLOCK_SIZE - 4 * 2))
  {
    while (pos != MD5_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, MD5_BLOCK_SIZE - pos);
    Md5_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (MD5_BLOCK_SIZE - 4 * 2) - pos);
  {
    const UInt64 numBits = p->count << 3;
#if defined(MY_CPU_LE_UNALIGN)
    SetUi64 (p->buffer + MD5_BLOCK_SIZE - 4 * 2, numBits)
#else
    SetUi32a(p->buffer + MD5_BLOCK_SIZE - 4 * 2, (UInt32)(numBits))
    SetUi32a(p->buffer + MD5_BLOCK_SIZE - 4 * 1, (UInt32)(numBits >> 32))
#endif
  }
  Md5_UpdateBlock(p);

  SetUi32(digest,      p->state[0])
  SetUi32(digest + 4,  p->state[1])
  SetUi32(digest + 8,  p->state[2])
  SetUi32(digest + 12, p->state[3])
  
  Md5_Init(p);
}

#undef R1
#undef R4
#undef R16
#undef D
#undef LOAD_DATA
#undef LOAD_data32_x4
#undef F1
#undef F2
#undef F3
#undef F4

/* ================ unit: C/MtCoder.c ================ */
/* MtCoder.c -- Multi-thread Coder
: Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#ifndef Z7_ST

static SRes MtProgressThunk_Progress(ICompressProgressPtr pp, UInt64 inSize, UInt64 outSize)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CMtProgressThunk)
  UInt64 inSize2 = 0;
  UInt64 outSize2 = 0;
  if (inSize != (UInt64)(Int64)-1)
  {
    inSize2 = inSize - p->inSize;
    p->inSize = inSize;
  }
  if (outSize != (UInt64)(Int64)-1)
  {
    outSize2 = outSize - p->outSize;
    p->outSize = outSize;
  }
  return MtProgress_ProgressAdd(p->mtProgress, inSize2, outSize2);
}


void MtProgressThunk_CreateVTable(CMtProgressThunk *p)
{
  p->vt.Progress = MtProgressThunk_Progress;
}



#define RINOK_THREAD(x) { if ((x) != 0) return SZ_ERROR_THREAD; }


static THREAD_FUNC_DECL ThreadFunc(void *pp);


static SRes MtCoderThread_CreateAndStart(CMtCoderThread *t
#ifdef _WIN32
    , CMtCoder * const mtc
#endif
    )
{
  WRes wres = AutoResetEvent_OptCreate_And_Reset(&t->startEvent);
  // printf("\n====== MtCoderThread_CreateAndStart : \n");
  if (wres == 0)
  {
    t->stop = False;
    if (!Thread_WasCreated(&t->thread))
    {
#ifdef _WIN32
      if (mtc->numThreadGroups)
        wres = Thread_Create_With_Group(&t->thread, ThreadFunc, t,
            ThreadNextGroup_GetNext(&mtc->nextGroup), // group
            0); // affinityMask
      else
#endif
        wres = Thread_Create(&t->thread, ThreadFunc, t);
    }
    if (wres == 0)
      wres = Event_Set(&t->startEvent);
  }
  if (wres == 0)
    return SZ_OK;
  return MY_SRes_HRESULT_FROM_WRes(wres);
}


Z7_FORCE_INLINE
static void MtCoderThread_Destruct(CMtCoderThread *t)
{
  if (Thread_WasCreated(&t->thread))
  {
    t->stop = 1;
    Event_Set(&t->startEvent);
    Thread_Wait_Close(&t->thread);
  }

  Event_Close(&t->startEvent);

  if (t->inBuf)
  {
    ISzAlloc_Free(t->mtCoder->allocBig, t->inBuf);
    t->inBuf = NULL;
  }
}




/*
  ThreadFunc2() returns:
  SZ_OK           - in all normal cases (even for stream error or memory allocation error)
  SZ_ERROR_THREAD - in case of failure in system synch function
*/

static SRes ThreadFunc2(CMtCoderThread *t)
{
  CMtCoder * const mtc = t->mtCoder;

  for (;;)
  {
    unsigned bi;
    SRes res;
    SRes res2;
    BoolInt finished;
    unsigned bufIndex;
    size_t size;
    const Byte *inData;
    UInt64 readProcessed = 0;
    
    RINOK_THREAD(Event_Wait(&mtc->readEvent))

    /* after Event_Wait(&mtc->readEvent) we must call Event_Set(&mtc->readEvent) in any case to unlock another threads */

    if (mtc->stopReading)
    {
      return Event_Set(&mtc->readEvent) == 0 ? SZ_OK : SZ_ERROR_THREAD;
    }

    res = MtProgress_GetError(&mtc->mtProgress);
    
    size = 0;
    inData = NULL;
    finished = True;

    if (res == SZ_OK)
    {
      size = mtc->blockSize;
      if (mtc->inStream)
      {
        if (!t->inBuf)
        {
          t->inBuf = (Byte *)ISzAlloc_Alloc(mtc->allocBig, mtc->blockSize);
          if (!t->inBuf)
            res = SZ_ERROR_MEM;
        }
        if (res == SZ_OK)
        {
          res = SeqInStream_ReadMax(mtc->inStream, t->inBuf, &size);
          readProcessed = mtc->readProcessed + size;
          mtc->readProcessed = readProcessed;
        }
        if (res != SZ_OK)
        {
          mtc->readRes = res;
          /* after reading error - we can stop encoding of previous blocks */
          MtProgress_SetError(&mtc->mtProgress, res);
        }
        else
          finished = (size != mtc->blockSize);
      }
      else
      {
        size_t rem;
        readProcessed = mtc->readProcessed;
        rem = mtc->inDataSize - (size_t)readProcessed;
        if (size > rem)
          size = rem;
        inData = mtc->inData + (size_t)readProcessed;
        readProcessed += size;
        mtc->readProcessed = readProcessed;
        finished = (mtc->inDataSize == (size_t)readProcessed);
      }
    }

    /* we must get some block from blocksSemaphore before Event_Set(&mtc->readEvent) */

    res2 = SZ_OK;

    if (Semaphore_Wait(&mtc->blocksSemaphore) != 0)
    {
      res2 = SZ_ERROR_THREAD;
      if (res == SZ_OK)
      {
        res = res2;
        // MtProgress_SetError(&mtc->mtProgress, res);
      }
    }

    bi = mtc->blockIndex;

    if (++mtc->blockIndex >= mtc->numBlocksMax)
      mtc->blockIndex = 0;

    bufIndex = (unsigned)(int)-1;

    if (res == SZ_OK)
      res = MtProgress_GetError(&mtc->mtProgress);

    if (res != SZ_OK)
      finished = True;

    if (!finished)
    {
      if (mtc->numStartedThreads < mtc->numStartedThreadsLimit
          && mtc->expectedDataSize != readProcessed)
      {
        res = MtCoderThread_CreateAndStart(&mtc->threads[mtc->numStartedThreads]
#ifdef _WIN32
            , mtc
#endif
            );
        if (res == SZ_OK)
          mtc->numStartedThreads++;
        else
        {
          MtProgress_SetError(&mtc->mtProgress, res);
          finished = True;
        }
      }
    }

    if (finished)
      mtc->stopReading = True;

    RINOK_THREAD(Event_Set(&mtc->readEvent))

    if (res2 != SZ_OK)
      return res2;

    if (res == SZ_OK)
    {
      CriticalSection_Enter(&mtc->cs);
      bufIndex = mtc->freeBlockHead;
      mtc->freeBlockHead = mtc->freeBlockList[bufIndex];
      CriticalSection_Leave(&mtc->cs);
      
      res = mtc->mtCallback->Code(mtc->mtCallbackObject, t->index, bufIndex,
          mtc->inStream ? t->inBuf : inData, size, finished);
      
      // MtProgress_Reinit(&mtc->mtProgress, t->index);

      if (res != SZ_OK)
        MtProgress_SetError(&mtc->mtProgress, res);
    }

    {
      CMtCoderBlock * const block = &mtc->blocks[bi];
      block->res = res;
      block->bufIndex = bufIndex;
      block->finished = finished;
    }
    
    #ifdef MTCODER_USE_WRITE_THREAD
      RINOK_THREAD(Event_Set(&mtc->writeEvents[bi]))
    #else
    {
      unsigned wi;
      {
        CriticalSection_Enter(&mtc->cs);
        wi = mtc->writeIndex;
        if (wi == bi)
          mtc->writeIndex = (unsigned)(int)-1;
        else
          mtc->ReadyBlocks[bi] = True;
        CriticalSection_Leave(&mtc->cs);
      }

      if (wi != bi)
      {
        if (res != SZ_OK || finished)
          return 0;
        continue;
      }

      if (mtc->writeRes != SZ_OK)
        res = mtc->writeRes;

      for (;;)
      {
        if (res == SZ_OK && bufIndex != (unsigned)(int)-1)
        {
          res = mtc->mtCallback->Write(mtc->mtCallbackObject, bufIndex);
          if (res != SZ_OK)
          {
            mtc->writeRes = res;
            MtProgress_SetError(&mtc->mtProgress, res);
          }
        }

        if (++wi >= mtc->numBlocksMax)
          wi = 0;
        {
          BoolInt isReady;

          CriticalSection_Enter(&mtc->cs);
          
          if (bufIndex != (unsigned)(int)-1)
          {
            mtc->freeBlockList[bufIndex] = mtc->freeBlockHead;
            mtc->freeBlockHead = bufIndex;
          }
          
          isReady = mtc->ReadyBlocks[wi];
          
          if (isReady)
            mtc->ReadyBlocks[wi] = False;
          else
            mtc->writeIndex = wi;
          
          CriticalSection_Leave(&mtc->cs);

          RINOK_THREAD(Semaphore_Release1(&mtc->blocksSemaphore))

          if (!isReady)
            break;
        }

        {
          CMtCoderBlock *block = &mtc->blocks[wi];
          if (res == SZ_OK && block->res != SZ_OK)
            res = block->res;
          bufIndex = block->bufIndex;
          finished = block->finished;
        }
      }
    }
    #endif
      
    if (finished || res != SZ_OK)
      return 0;
  }
}


static THREAD_FUNC_DECL ThreadFunc(void *pp)
{
  CMtCoderThread * const t = (CMtCoderThread *)pp;
  for (;;)
  {
    if (Event_Wait(&t->startEvent) != 0)
      return (THREAD_FUNC_RET_TYPE)SZ_ERROR_THREAD;
    if (t->stop)
      return 0;
    {
      const SRes res = ThreadFunc2(t);
      CMtCoder *mtc = t->mtCoder;
      if (res != SZ_OK)
      {
        MtProgress_SetError(&mtc->mtProgress, res);
      }
      
      #ifndef MTCODER_USE_WRITE_THREAD
      {
        const unsigned numFinished = (unsigned)InterlockedIncrement(&mtc->numFinishedThreads);
        if (numFinished == mtc->numStartedThreads)
          if (Event_Set(&mtc->finishedEvent) != 0)
            return (THREAD_FUNC_RET_TYPE)SZ_ERROR_THREAD;
      }
      #endif
    }
  }
}



void MtCoder_Construct(CMtCoder *p)
{
  unsigned i;
  
  p->blockSize = 0;
  p->numThreadsMax = 0;
  p->numThreadGroups = 0;
  p->expectedDataSize = (UInt64)(Int64)-1;

  p->inStream = NULL;
  p->inData = NULL;
  p->inDataSize = 0;

  p->progress = NULL;
  p->allocBig = NULL;

  p->mtCallback = NULL;
  p->mtCallbackObject = NULL;

  p->allocatedBufsSize = 0;

  Event_Construct(&p->readEvent);
  Semaphore_Construct(&p->blocksSemaphore);

  for (i = 0; i < MTCODER_THREADS_MAX; i++)
  {
    CMtCoderThread *t = &p->threads[i];
    t->mtCoder = p;
    t->index = i;
    t->inBuf = NULL;
    t->stop = False;
    Event_Construct(&t->startEvent);
    Thread_CONSTRUCT(&t->thread)
  }

  #ifdef MTCODER_USE_WRITE_THREAD
    for (i = 0; i < MTCODER_BLOCKS_MAX; i++)
      Event_Construct(&p->writeEvents[i]);
  #else
    Event_Construct(&p->finishedEvent);
  #endif

  CriticalSection_Init(&p->cs);
  CriticalSection_Init(&p->mtProgress.cs);
}




static void MtCoder_Free(CMtCoder *p)
{
  unsigned i;

  /*
  p->stopReading = True;
  if (Event_IsCreated(&p->readEvent))
    Event_Set(&p->readEvent);
  */

  for (i = 0; i < MTCODER_THREADS_MAX; i++)
    MtCoderThread_Destruct(&p->threads[i]);

  Event_Close(&p->readEvent);
  Semaphore_Close(&p->blocksSemaphore);

  #ifdef MTCODER_USE_WRITE_THREAD
    for (i = 0; i < MTCODER_BLOCKS_MAX; i++)
      Event_Close(&p->writeEvents[i]);
  #else
    Event_Close(&p->finishedEvent);
  #endif
}


void MtCoder_Destruct(CMtCoder *p)
{
  MtCoder_Free(p);

  CriticalSection_Delete(&p->cs);
  CriticalSection_Delete(&p->mtProgress.cs);
}


SRes MtCoder_Code(CMtCoder *p)
{
  unsigned numThreads = p->numThreadsMax;
  unsigned numBlocksMax;
  unsigned i;
  SRes res = SZ_OK;

  // printf("\n====== MtCoder_Code : \n");

  if (numThreads > MTCODER_THREADS_MAX)
      numThreads = MTCODER_THREADS_MAX;
  numBlocksMax = MTCODER_GET_NUM_BLOCKS_FROM_THREADS(numThreads);
  
  if (p->blockSize < ((UInt32)1 << 26)) numBlocksMax++;
  if (p->blockSize < ((UInt32)1 << 24)) numBlocksMax++;
  if (p->blockSize < ((UInt32)1 << 22)) numBlocksMax++;

  if (numBlocksMax > MTCODER_BLOCKS_MAX)
      numBlocksMax = MTCODER_BLOCKS_MAX;

  if (p->blockSize != p->allocatedBufsSize)
  {
    for (i = 0; i < MTCODER_THREADS_MAX; i++)
    {
      CMtCoderThread *t = &p->threads[i];
      if (t->inBuf)
      {
        ISzAlloc_Free(p->allocBig, t->inBuf);
        t->inBuf = NULL;
      }
    }
    p->allocatedBufsSize = p->blockSize;
  }

  p->readRes = SZ_OK;

  MtProgress_Init(&p->mtProgress, p->progress);

  #ifdef MTCODER_USE_WRITE_THREAD
    for (i = 0; i < numBlocksMax; i++)
    {
      RINOK_THREAD(AutoResetEvent_OptCreate_And_Reset(&p->writeEvents[i]))
    }
  #else
    RINOK_THREAD(AutoResetEvent_OptCreate_And_Reset(&p->finishedEvent))
  #endif

  {
    RINOK_THREAD(AutoResetEvent_OptCreate_And_Reset(&p->readEvent))
    RINOK_THREAD(Semaphore_OptCreateInit(&p->blocksSemaphore, (UInt32)numBlocksMax, (UInt32)numBlocksMax))
  }

  for (i = 0; i < MTCODER_BLOCKS_MAX - 1; i++)
    p->freeBlockList[i] = i + 1;
  p->freeBlockList[MTCODER_BLOCKS_MAX - 1] = (unsigned)(int)-1;
  p->freeBlockHead = 0;

  p->readProcessed = 0;
  p->blockIndex = 0;
  p->numBlocksMax = numBlocksMax;
  p->stopReading = False;

  #ifndef MTCODER_USE_WRITE_THREAD
    p->writeIndex = 0;
    p->writeRes = SZ_OK;
    for (i = 0; i < MTCODER_BLOCKS_MAX; i++)
      p->ReadyBlocks[i] = False;
    p->numFinishedThreads = 0;
  #endif

  p->numStartedThreadsLimit = numThreads;
  p->numStartedThreads = 0;
  ThreadNextGroup_Init(&p->nextGroup, p->numThreadGroups, 0); // startGroup

  // for (i = 0; i < numThreads; i++)
  {
    // here we create new thread for first block.
    // And each new thread will create another new thread after block reading
    // until numStartedThreadsLimit is reached.
    CMtCoderThread *nextThread = &p->threads[p->numStartedThreads++];
    {
      const SRes res2 = MtCoderThread_CreateAndStart(nextThread
#ifdef _WIN32
            , p
#endif
            );
      RINOK(res2)
    }
  }

  RINOK_THREAD(Event_Set(&p->readEvent))

  #ifdef MTCODER_USE_WRITE_THREAD
  {
    unsigned bi = 0;

    for (;; bi++)
    {
      if (bi >= numBlocksMax)
        bi = 0;

      RINOK_THREAD(Event_Wait(&p->writeEvents[bi]))

      {
        const CMtCoderBlock * const block = &p->blocks[bi];
        const unsigned bufIndex = block->bufIndex;
        const BoolInt finished = block->finished;
        if (res == SZ_OK && block->res != SZ_OK)
          res = block->res;

        if (bufIndex != (unsigned)(int)-1)
        {
          if (res == SZ_OK)
          {
            res = p->mtCallback->Write(p->mtCallbackObject, bufIndex);
            if (res != SZ_OK)
              MtProgress_SetError(&p->mtProgress, res);
          }
          
          CriticalSection_Enter(&p->cs);
          {
            p->freeBlockList[bufIndex] = p->freeBlockHead;
            p->freeBlockHead = bufIndex;
          }
          CriticalSection_Leave(&p->cs);
        }
        
        RINOK_THREAD(Semaphore_Release1(&p->blocksSemaphore))

        if (finished)
          break;
      }
    }
  }
  #else
  {
    const WRes wres = Event_Wait(&p->finishedEvent);
    res = MY_SRes_HRESULT_FROM_WRes(wres);
  }
  #endif

  if (res == SZ_OK)
    res = p->readRes;

  if (res == SZ_OK)
    res = p->mtProgress.res;

  #ifndef MTCODER_USE_WRITE_THREAD
    if (res == SZ_OK)
      res = p->writeRes;
  #endif

  if (res != SZ_OK)
    MtCoder_Free(p);
  return res;
}

#endif

#undef RINOK_THREAD

/* ================ unit: C/MtDec.c ================ */
/* MtDec.c -- Multi-thread Decoder
2024-02-20 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO

// #include <stdio.h>
#include <string.h>

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

// amalgamation: header emitted in prologue

#ifndef Z7_ST

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#define PRF_STR_INT(s, d) PRF(printf("\n" s " %d\n", (unsigned)d))

void MtProgress_Init(CMtProgress *p, ICompressProgressPtr progress)
{
  p->progress = progress;
  p->res = SZ_OK;
  p->totalInSize = 0;
  p->totalOutSize = 0;
}


SRes MtProgress_Progress_ST(CMtProgress *p)
{
  if (p->res == SZ_OK && p->progress)
    if (ICompressProgress_Progress(p->progress, p->totalInSize, p->totalOutSize) != SZ_OK)
      p->res = SZ_ERROR_PROGRESS;
  return p->res;
}


SRes MtProgress_ProgressAdd(CMtProgress *p, UInt64 inSize, UInt64 outSize)
{
  SRes res;
  CriticalSection_Enter(&p->cs);
  
  p->totalInSize += inSize;
  p->totalOutSize += outSize;
  if (p->res == SZ_OK && p->progress)
    if (ICompressProgress_Progress(p->progress, p->totalInSize, p->totalOutSize) != SZ_OK)
      p->res = SZ_ERROR_PROGRESS;
  res = p->res;
  
  CriticalSection_Leave(&p->cs);
  return res;
}


SRes MtProgress_GetError(CMtProgress *p)
{
  SRes res;
  CriticalSection_Enter(&p->cs);
  res = p->res;
  CriticalSection_Leave(&p->cs);
  return res;
}


void MtProgress_SetError(CMtProgress *p, SRes res)
{
  CriticalSection_Enter(&p->cs);
  if (p->res == SZ_OK)
    p->res = res;
  CriticalSection_Leave(&p->cs);
}


#define RINOK_THREAD(x) RINOK_WRes(x)


struct CMtDecBufLink_
{
  struct CMtDecBufLink_ *next;
  void *pad[3];
};

typedef struct CMtDecBufLink_ CMtDecBufLink;

#define MTDEC__LINK_DATA_OFFSET sizeof(CMtDecBufLink)
#define MTDEC__DATA_PTR_FROM_LINK(link) ((Byte *)(link) + MTDEC__LINK_DATA_OFFSET)



static THREAD_FUNC_DECL MtDec_ThreadFunc(void *pp);


static WRes MtDecThread_CreateEvents(CMtDecThread *t)
{
  WRes wres = AutoResetEvent_OptCreate_And_Reset(&t->canWrite);
  if (wres == 0)
  {
    wres = AutoResetEvent_OptCreate_And_Reset(&t->canRead);
    if (wres == 0)
      return SZ_OK;
  }
  return wres;
}


static SRes MtDecThread_CreateAndStart(CMtDecThread *t)
{
  WRes wres = MtDecThread_CreateEvents(t);
  // wres = 17; // for test
  if (wres == 0)
  {
    if (Thread_WasCreated(&t->thread))
      return SZ_OK;
    wres = Thread_Create(&t->thread, MtDec_ThreadFunc, t);
    if (wres == 0)
      return SZ_OK;
  }
  return MY_SRes_HRESULT_FROM_WRes(wres);
}


void MtDecThread_FreeInBufs(CMtDecThread *t)
{
  if (t->inBuf)
  {
    void *link = t->inBuf;
    t->inBuf = NULL;
    do
    {
      void *next = ((CMtDecBufLink *)link)->next;
      ISzAlloc_Free(t->mtDec->alloc, link);
      link = next;
    }
    while (link);
  }
}


static void MtDecThread_CloseThread(CMtDecThread *t)
{
  if (Thread_WasCreated(&t->thread))
  {
    Event_Set(&t->canWrite); /* we can disable it. There are no threads waiting canWrite in normal cases */
    Event_Set(&t->canRead);
    Thread_Wait_Close(&t->thread);
  }

  Event_Close(&t->canRead);
  Event_Close(&t->canWrite);
}

static void MtDec_CloseThreads(CMtDec *p)
{
  unsigned i;
  for (i = 0; i < MTDEC_THREADS_MAX; i++)
    MtDecThread_CloseThread(&p->threads[i]);
}

static void MtDecThread_Destruct(CMtDecThread *t)
{
  MtDecThread_CloseThread(t);
  MtDecThread_FreeInBufs(t);
}



static SRes MtDec_GetError_Spec(CMtDec *p, UInt64 interruptIndex, BoolInt *wasInterrupted)
{
  SRes res;
  CriticalSection_Enter(&p->mtProgress.cs);
  *wasInterrupted = (p->needInterrupt && interruptIndex > p->interruptIndex);
  res = p->mtProgress.res;
  CriticalSection_Leave(&p->mtProgress.cs);
  return res;
}

static SRes MtDec_Progress_GetError_Spec(CMtDec *p, UInt64 inSize, UInt64 outSize, UInt64 interruptIndex, BoolInt *wasInterrupted)
{
  SRes res;
  CriticalSection_Enter(&p->mtProgress.cs);

  p->mtProgress.totalInSize += inSize;
  p->mtProgress.totalOutSize += outSize;
  if (p->mtProgress.res == SZ_OK && p->mtProgress.progress)
    if (ICompressProgress_Progress(p->mtProgress.progress, p->mtProgress.totalInSize, p->mtProgress.totalOutSize) != SZ_OK)
      p->mtProgress.res = SZ_ERROR_PROGRESS;

  *wasInterrupted = (p->needInterrupt && interruptIndex > p->interruptIndex);
  res = p->mtProgress.res;
  
  CriticalSection_Leave(&p->mtProgress.cs);

  return res;
}

static void MtDec_Interrupt(CMtDec *p, UInt64 interruptIndex)
{
  CriticalSection_Enter(&p->mtProgress.cs);
  if (!p->needInterrupt || interruptIndex < p->interruptIndex)
  {
    p->interruptIndex = interruptIndex;
    p->needInterrupt = True;
  }
  CriticalSection_Leave(&p->mtProgress.cs);
}

Byte *MtDec_GetCrossBuff(CMtDec *p)
{
  Byte *cr = p->crossBlock;
  if (!cr)
  {
    cr = (Byte *)ISzAlloc_Alloc(p->alloc, MTDEC__LINK_DATA_OFFSET + p->inBufSize);
    if (!cr)
      return NULL;
    p->crossBlock = cr;
  }
  return MTDEC__DATA_PTR_FROM_LINK(cr);
}


/*
  MtDec_ThreadFunc2() returns:
  0      - in all normal cases (even for stream error or memory allocation error)
  (!= 0) - WRes error return by system threading function
*/

// #define MTDEC_ProgessStep (1 << 22)
#define MTDEC_ProgessStep (1 << 0)

static WRes MtDec_ThreadFunc2(CMtDecThread *t)
{
  CMtDec *p = t->mtDec;

  PRF_STR_INT("MtDec_ThreadFunc2", t->index)

  // SetThreadAffinityMask(GetCurrentThread(), 1 << t->index);

  for (;;)
  {
    SRes res, codeRes;
    BoolInt wasInterrupted, isAllocError, overflow, finish;
    SRes threadingErrorSRes;
    BoolInt needCode, needWrite, needContinue;
    
    size_t inDataSize_Start;
    UInt64 inDataSize;
    // UInt64 inDataSize_Full;
    
    UInt64 blockIndex;

    UInt64 inPrev = 0;
    UInt64 outPrev = 0;
    UInt64 inCodePos;
    UInt64 outCodePos;
    
    Byte *afterEndData = NULL;
    size_t afterEndData_Size = 0;
    BoolInt afterEndData_IsCross = False;

    BoolInt canCreateNewThread = False;
    // CMtDecCallbackInfo parse;
    CMtDecThread *nextThread;

    PRF_STR_INT("=============== Event_Wait(&t->canRead)", t->index)

    RINOK_THREAD(Event_Wait(&t->canRead))
    if (p->exitThread)
      return 0;

    PRF_STR_INT("after Event_Wait(&t->canRead)", t->index)

    // if (t->index == 3) return 19; // for test

    blockIndex = p->blockIndex++;

    // PRF(printf("\ncanRead\n"))

    res = MtDec_Progress_GetError_Spec(p, 0, 0, blockIndex, &wasInterrupted);

    finish = p->readWasFinished;
    needCode = False;
    needWrite = False;
    isAllocError = False;
    overflow = False;

    inDataSize_Start = 0;
    inDataSize = 0;
    // inDataSize_Full = 0;

    if (res == SZ_OK && !wasInterrupted)
    {
      // if (p->inStream)
      {
        CMtDecBufLink *prev = NULL;
        CMtDecBufLink *link = (CMtDecBufLink *)t->inBuf;
        size_t crossSize = p->crossEnd - p->crossStart;

        PRF(printf("\ncrossSize = %d\n", crossSize));

        for (;;)
        {
          if (!link)
          {
            link = (CMtDecBufLink *)ISzAlloc_Alloc(p->alloc, MTDEC__LINK_DATA_OFFSET + p->inBufSize);
            if (!link)
            {
              finish = True;
              // p->allocError_for_Read_BlockIndex = blockIndex;
              isAllocError = True;
              break;
            }
            link->next = NULL;
            if (prev)
            {
              // static unsigned g_num = 0;
              // printf("\n%6d : %x", ++g_num, (unsigned)(size_t)((Byte *)link - (Byte *)prev));
              prev->next = link;
            }
            else
              t->inBuf = (void *)link;
          }

          {
            Byte *data = MTDEC__DATA_PTR_FROM_LINK(link);
            Byte *parseData = data;
            size_t size;

            if (crossSize != 0)
            {
              inDataSize = crossSize;
              // inDataSize_Full = inDataSize;
              inDataSize_Start = crossSize;
              size = crossSize;
              parseData = MTDEC__DATA_PTR_FROM_LINK(p->crossBlock) + p->crossStart;
              PRF(printf("\ncross : crossStart = %7d  crossEnd = %7d finish = %1d",
                  (int)p->crossStart, (int)p->crossEnd, (int)finish));
            }
            else
            {
              size = p->inBufSize;
              
              res = SeqInStream_ReadMax(p->inStream, data, &size);
              
              // size = 10; // test

              inDataSize += size;
              // inDataSize_Full = inDataSize;
              if (!prev)
                inDataSize_Start = size;

              p->readProcessed += size;
              finish = (size != p->inBufSize);
              if (finish)
                p->readWasFinished = True;
              
              // res = E_INVALIDARG; // test

              if (res != SZ_OK)
              {
                // PRF(printf("\nRead error = %d\n", res))
                // we want to decode all data before error
                p->readRes = res;
                // p->readError_BlockIndex = blockIndex;
                p->readWasFinished = True;
                finish = True;
                res = SZ_OK;
                // break;
              }

              if (inDataSize - inPrev >= MTDEC_ProgessStep)
              {
                res = MtDec_Progress_GetError_Spec(p, 0, 0, blockIndex, &wasInterrupted);
                if (res != SZ_OK || wasInterrupted)
                  break;
                inPrev = inDataSize;
              }
            }

            {
              CMtDecCallbackInfo parse;

              parse.startCall = (prev == NULL);
              parse.src = parseData;
              parse.srcSize = size;
              parse.srcFinished = finish;
              parse.canCreateNewThread = True;

              PRF(printf("\nParse size = %d\n", (unsigned)size));

              p->mtCallback->Parse(p->mtCallbackObject, t->index, &parse);

              PRF(printf("   Parse processed = %d, state = %d \n", (unsigned)parse.srcSize, (unsigned)parse.state));

              needWrite = True;
              canCreateNewThread = parse.canCreateNewThread;

              // printf("\n\n%12I64u %12I64u", (UInt64)p->mtProgress.totalInSize, (UInt64)p->mtProgress.totalOutSize);
              
              if (
                  // parseRes != SZ_OK ||
                  // inDataSize - (size - parse.srcSize) > p->inBlockMax
                  // ||
                  parse.state == MTDEC_PARSE_OVERFLOW
                  // || wasInterrupted
                  )
              {
                // Overflow or Parse error - switch from MT decoding to ST decoding
                finish = True;
                overflow = True;

                {
                  PRF(printf("\n Overflow"));
                  // PRF(printf("\nisBlockFinished = %d", (unsigned)parse.blockWasFinished));
                  PRF(printf("\n inDataSize = %d", (unsigned)inDataSize));
                }
                
                if (crossSize != 0)
                  memcpy(data, parseData, size);
                p->crossStart = 0;
                p->crossEnd = 0;
                break;
              }

              if (crossSize != 0)
              {
                memcpy(data, parseData, parse.srcSize);
                p->crossStart += parse.srcSize;
              }

              if (parse.state != MTDEC_PARSE_CONTINUE || finish)
              {
                // we don't need to parse in current thread anymore

                if (parse.state == MTDEC_PARSE_END)
                  finish = True;

                needCode = True;
                // p->crossFinished = finish;

                if (parse.srcSize == size)
                {
                  // full parsed - no cross transfer
                  p->crossStart = 0;
                  p->crossEnd = 0;
                  break;
                }

                if (parse.state == MTDEC_PARSE_END)
                {
                  afterEndData = parseData + parse.srcSize;
                  afterEndData_Size = size - parse.srcSize;
                  if (crossSize != 0)
                    afterEndData_IsCross = True;
                  // we reduce data size to required bytes (parsed only)
                  inDataSize -= afterEndData_Size;
                  if (!prev)
                    inDataSize_Start = parse.srcSize;
                  break;
                }

                {
                  // partial parsed - need cross transfer
                  if (crossSize != 0)
                    inDataSize = parse.srcSize; // it's only parsed now
                  else
                  {
                    // partial parsed - is not in initial cross block - we need to copy new data to cross block
                    Byte *cr = MtDec_GetCrossBuff(p);
                    if (!cr)
                    {
                      {
                        PRF(printf("\ncross alloc error error\n"));
                        // res = SZ_ERROR_MEM;
                        finish = True;
                        // p->allocError_for_Read_BlockIndex = blockIndex;
                        isAllocError = True;
                        break;
                      }
                    }

                    {
                      size_t crSize = size - parse.srcSize;
                      inDataSize -= crSize;
                      p->crossEnd = crSize;
                      p->crossStart = 0;
                      memcpy(cr, parseData + parse.srcSize, crSize);
                    }
                  }

                  // inDataSize_Full = inDataSize;
                  if (!prev)
                    inDataSize_Start = parse.srcSize; // it's partial size (parsed only)

                  finish = False;
                  break;
                }
              }

              if (parse.srcSize != size)
              {
                res = SZ_ERROR_FAIL;
                PRF(printf("\nfinished error SZ_ERROR_FAIL = %d\n", res));
                break;
              }
            }
          }
          
          prev = link;
          link = link->next;

          if (crossSize != 0)
          {
            crossSize = 0;
            p->crossStart = 0;
            p->crossEnd = 0;
          }
        }
      }

      if (res == SZ_OK)
        res = MtDec_GetError_Spec(p, blockIndex, &wasInterrupted);
    }

    codeRes = SZ_OK;

    if (res == SZ_OK && needCode && !wasInterrupted)
    {
      codeRes = p->mtCallback->PreCode(p->mtCallbackObject, t->index);
      if (codeRes != SZ_OK)
      {
        needCode = False;
        finish = True;
        // SZ_ERROR_MEM is expected error here.
        //   if (codeRes == SZ_ERROR_MEM) - we will try single-thread decoding later.
        //   if (codeRes != SZ_ERROR_MEM) - we can stop decoding or try single-thread decoding.
      }
    }
    
    if (res != SZ_OK || wasInterrupted)
      finish = True;
    
    nextThread = NULL;
    threadingErrorSRes = SZ_OK;

    if (!finish)
    {
      if (p->numStartedThreads < p->numStartedThreads_Limit && canCreateNewThread)
      {
        SRes res2 = MtDecThread_CreateAndStart(&p->threads[p->numStartedThreads]);
        if (res2 == SZ_OK)
        {
          // if (p->numStartedThreads % 1000 == 0) PRF(printf("\n numStartedThreads=%d\n", p->numStartedThreads));
          p->numStartedThreads++;
        }
        else
        {
          PRF(printf("\nERROR: numStartedThreads=%d\n", p->numStartedThreads));
          if (p->numStartedThreads == 1)
          {
            // if only one thread is possible, we leave muti-threading code
            finish = True;
            needCode = False;
            threadingErrorSRes = res2;
          }
          else
            p->numStartedThreads_Limit = p->numStartedThreads;
        }
      }
      
      if (!finish)
      {
        unsigned nextIndex = t->index + 1;
        nextThread = &p->threads[nextIndex >= p->numStartedThreads ? 0 : nextIndex];
        RINOK_THREAD(Event_Set(&nextThread->canRead))
        // We have started executing for new iteration (with next thread)
        // And that next thread now is responsible for possible exit from decoding (threading_code)
      }
    }

    // each call of Event_Set(&nextThread->canRead) must be followed by call of Event_Set(&nextThread->canWrite)
    // if ( !finish ) we must call Event_Set(&nextThread->canWrite) in any case
    // if (  finish ) we switch to single-thread mode and there are 2 ways at the end of current iteration (current block):
    //   - if (needContinue) after Write(&needContinue), we restore decoding with new iteration
    //   - otherwise we stop decoding and exit from MtDec_ThreadFunc2()

    // Don't change (finish) variable in the further code


    // ---------- CODE ----------

    inPrev = 0;
    outPrev = 0;
    inCodePos = 0;
    outCodePos = 0;

    if (res == SZ_OK && needCode && codeRes == SZ_OK)
    {
      BoolInt isStartBlock = True;
      CMtDecBufLink *link = (CMtDecBufLink *)t->inBuf;

      for (;;)
      {
        size_t inSize;
        int stop;

        if (isStartBlock)
          inSize = inDataSize_Start;
        else
        {
          UInt64 rem = inDataSize - inCodePos;
          inSize = p->inBufSize;
          if (inSize > rem)
            inSize = (size_t)rem;
        }

        inCodePos += inSize;
        stop = True;

        codeRes = p->mtCallback->Code(p->mtCallbackObject, t->index,
            (const Byte *)MTDEC__DATA_PTR_FROM_LINK(link), inSize,
            (inCodePos == inDataSize), // srcFinished
            &inCodePos, &outCodePos, &stop);
        
        if (codeRes != SZ_OK)
        {
          PRF(printf("\nCode Interrupt error = %x\n", codeRes));
          // we interrupt only later blocks
          MtDec_Interrupt(p, blockIndex);
          break;
        }

        if (stop || inCodePos == inDataSize)
          break;
  
        {
          const UInt64 inDelta = inCodePos - inPrev;
          const UInt64 outDelta = outCodePos - outPrev;
          if (inDelta >= MTDEC_ProgessStep || outDelta >= MTDEC_ProgessStep)
          {
            // Sleep(1);
            res = MtDec_Progress_GetError_Spec(p, inDelta, outDelta, blockIndex, &wasInterrupted);
            if (res != SZ_OK || wasInterrupted)
              break;
            inPrev = inCodePos;
            outPrev = outCodePos;
          }
        }

        link = link->next;
        isStartBlock = False;
      }
    }


    // ---------- WRITE ----------
   
    RINOK_THREAD(Event_Wait(&t->canWrite))

  {
    BoolInt isErrorMode = False;
    BoolInt canRecode = True;
    BoolInt needWriteToStream = needWrite;

    if (p->exitThread) return 0; // it's never executed in normal cases

    if (p->wasInterrupted)
      wasInterrupted = True;
    else
    {
      if (codeRes != SZ_OK) // || !needCode // check it !!!
      {
        p->wasInterrupted = True;
        p->codeRes = codeRes;
        if (codeRes == SZ_ERROR_MEM)
          isAllocError = True;
      }
      
      if (threadingErrorSRes)
      {
        p->wasInterrupted = True;
        p->threadingErrorSRes = threadingErrorSRes;
        needWriteToStream = False;
      }
      if (isAllocError)
      {
        p->wasInterrupted = True;
        p->isAllocError = True;
        needWriteToStream = False;
      }
      if (overflow)
      {
        p->wasInterrupted = True;
        p->overflow = True;
        needWriteToStream = False;
      }
    }

    if (needCode)
    {
      if (wasInterrupted)
      {
        inCodePos = 0;
        outCodePos = 0;
      }
      {
        const UInt64 inDelta = inCodePos - inPrev;
        const UInt64 outDelta = outCodePos - outPrev;
        // if (inDelta != 0 || outDelta != 0)
        res = MtProgress_ProgressAdd(&p->mtProgress, inDelta, outDelta);
      }
    }

    needContinue = (!finish);

    // if (res == SZ_OK && needWrite && !wasInterrupted)
    if (needWrite)
    {
      // p->inProcessed += inCodePos;

      PRF(printf("\n--Write afterSize = %d\n", (unsigned)afterEndData_Size));

      res = p->mtCallback->Write(p->mtCallbackObject, t->index,
          res == SZ_OK && needWriteToStream && !wasInterrupted, // needWrite
          afterEndData, afterEndData_Size, afterEndData_IsCross,
          &needContinue,
          &canRecode);

      // res = SZ_ERROR_FAIL; // for test

      PRF(printf("\nAfter Write needContinue = %d\n", (unsigned)needContinue));
      PRF(printf("\nprocessed = %d\n", (unsigned)p->inProcessed));

      if (res != SZ_OK)
      {
        PRF(printf("\nWrite error = %d\n", res));
        isErrorMode = True;
        p->wasInterrupted = True;
      }
      if (res != SZ_OK
          || (!needContinue && !finish))
      {
        PRF(printf("\nWrite Interrupt error = %x\n", res));
        MtDec_Interrupt(p, blockIndex);
      }
    }

    if (canRecode)
    if (!needCode
        || res != SZ_OK
        || p->wasInterrupted
        || codeRes != SZ_OK
        || wasInterrupted
        || p->numFilledThreads != 0
        || isErrorMode)
    {
      if (p->numFilledThreads == 0)
        p->filledThreadStart = t->index;
      if (inDataSize != 0 || !finish)
      {
        t->inDataSize_Start = inDataSize_Start;
        t->inDataSize = inDataSize;
        p->numFilledThreads++;
      }
      PRF(printf("\np->numFilledThreads = %d\n", p->numFilledThreads));
      PRF(printf("p->filledThreadStart = %d\n", p->filledThreadStart));
    }

    if (!finish)
    {
      RINOK_THREAD(Event_Set(&nextThread->canWrite))
    }
    else
    {
      if (needContinue)
      {
        // we restore decoding with new iteration
        RINOK_THREAD(Event_Set(&p->threads[0].canWrite))
      }
      else
      {
        // we exit from decoding
        if (t->index == 0)
          return SZ_OK;
        p->exitThread = True;
      }
      RINOK_THREAD(Event_Set(&p->threads[0].canRead))
    }
  }
  }
}

#ifdef _WIN32
#define USE_ALLOCA
#endif

#ifdef USE_ALLOCA
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#endif


typedef
  #ifdef _WIN32
    UINT_PTR
  #elif 1
    uintptr_t
  #else
    ptrdiff_t
  #endif
    MY_uintptr_t;

static THREAD_FUNC_DECL MtDec_ThreadFunc1(void *pp)
{
  WRes res;

  CMtDecThread *t = (CMtDecThread *)pp;
  CMtDec *p;

  // fprintf(stdout, "\n%d = %p\n", t->index, &t);

  res = MtDec_ThreadFunc2(t);
  p = t->mtDec;
  if (res == 0)
    return (THREAD_FUNC_RET_TYPE)(MY_uintptr_t)p->exitThreadWRes;
  {
    // it's unexpected situation for some threading function error
    if (p->exitThreadWRes == 0)
      p->exitThreadWRes = res;
    PRF(printf("\nthread exit error = %d\n", res));
    p->exitThread = True;
    Event_Set(&p->threads[0].canRead);
    Event_Set(&p->threads[0].canWrite);
    MtProgress_SetError(&p->mtProgress, MY_SRes_HRESULT_FROM_WRes(res));
  }
  return (THREAD_FUNC_RET_TYPE)(MY_uintptr_t)res;
}

static Z7_NO_INLINE THREAD_FUNC_DECL MtDec_ThreadFunc(void *pp)
{
  #ifdef USE_ALLOCA
  CMtDecThread *t = (CMtDecThread *)pp;
  // fprintf(stderr, "\n%d = %p - before", t->index, &t);
  t->allocaPtr = alloca(t->index * 128);
  #endif
  return MtDec_ThreadFunc1(pp);
}


int MtDec_PrepareRead(CMtDec *p)
{
  if (p->crossBlock && p->crossStart == p->crossEnd)
  {
    ISzAlloc_Free(p->alloc, p->crossBlock);
    p->crossBlock = NULL;
  }
    
  {
    unsigned i;
    for (i = 0; i < MTDEC_THREADS_MAX; i++)
      if (i > p->numStartedThreads
          || p->numFilledThreads <=
            (i >= p->filledThreadStart ?
              i - p->filledThreadStart :
              i + p->numStartedThreads - p->filledThreadStart))
        MtDecThread_FreeInBufs(&p->threads[i]);
  }

  return (p->numFilledThreads != 0) || (p->crossStart != p->crossEnd);
}

    
const Byte *MtDec_Read(CMtDec *p, size_t *inLim)
{
  while (p->numFilledThreads != 0)
  {
    CMtDecThread *t = &p->threads[p->filledThreadStart];
    
    if (*inLim != 0)
    {
      {
        void *link = t->inBuf;
        void *next = ((CMtDecBufLink *)link)->next;
        ISzAlloc_Free(p->alloc, link);
        t->inBuf = next;
      }
      
      if (t->inDataSize == 0)
      {
        MtDecThread_FreeInBufs(t);
        if (--p->numFilledThreads == 0)
          break;
        if (++p->filledThreadStart == p->numStartedThreads)
          p->filledThreadStart = 0;
        t = &p->threads[p->filledThreadStart];
      }
    }
    
    {
      size_t lim = t->inDataSize_Start;
      if (lim != 0)
        t->inDataSize_Start = 0;
      else
      {
        UInt64 rem = t->inDataSize;
        lim = p->inBufSize;
        if (lim > rem)
          lim = (size_t)rem;
      }
      t->inDataSize -= lim;
      *inLim = lim;
      return (const Byte *)MTDEC__DATA_PTR_FROM_LINK(t->inBuf);
    }
  }

  {
    size_t crossSize = p->crossEnd - p->crossStart;
    if (crossSize != 0)
    {
      const Byte *data = MTDEC__DATA_PTR_FROM_LINK(p->crossBlock) + p->crossStart;
      *inLim = crossSize;
      p->crossStart = 0;
      p->crossEnd = 0;
      return data;
    }
    *inLim = 0;
    if (p->crossBlock)
    {
      ISzAlloc_Free(p->alloc, p->crossBlock);
      p->crossBlock = NULL;
    }
    return NULL;
  }
}


void MtDec_Construct(CMtDec *p)
{
  unsigned i;
  
  p->inBufSize = (size_t)1 << 18;

  p->numThreadsMax = 0;

  p->inStream = NULL;
  
  // p->inData = NULL;
  // p->inDataSize = 0;

  p->crossBlock = NULL;
  p->crossStart = 0;
  p->crossEnd = 0;

  p->numFilledThreads = 0;

  p->progress = NULL;
  p->alloc = NULL;

  p->mtCallback = NULL;
  p->mtCallbackObject = NULL;

  p->allocatedBufsSize = 0;

  for (i = 0; i < MTDEC_THREADS_MAX; i++)
  {
    CMtDecThread *t = &p->threads[i];
    t->mtDec = p;
    t->index = i;
    t->inBuf = NULL;
    Event_Construct(&t->canRead);
    Event_Construct(&t->canWrite);
    Thread_CONSTRUCT(&t->thread)
  }

  // Event_Construct(&p->finishedEvent);

  CriticalSection_Init(&p->mtProgress.cs);
}


static void MtDec_Free(CMtDec *p)
{
  unsigned i;

  p->exitThread = True;

  for (i = 0; i < MTDEC_THREADS_MAX; i++)
    MtDecThread_Destruct(&p->threads[i]);

  // Event_Close(&p->finishedEvent);

  if (p->crossBlock)
  {
    ISzAlloc_Free(p->alloc, p->crossBlock);
    p->crossBlock = NULL;
  }
}


void MtDec_Destruct(CMtDec *p)
{
  MtDec_Free(p);

  CriticalSection_Delete(&p->mtProgress.cs);
}


SRes MtDec_Code(CMtDec *p)
{
  unsigned i;

  p->inProcessed = 0;

  p->blockIndex = 1; // it must be larger than not_defined index (0)
  p->isAllocError = False;
  p->overflow = False;
  p->threadingErrorSRes = SZ_OK;

  p->needContinue = True;

  p->readWasFinished = False;
  p->needInterrupt = False;
  p->interruptIndex = (UInt64)(Int64)-1;

  p->readProcessed = 0;
  p->readRes = SZ_OK;
  p->codeRes = SZ_OK;
  p->wasInterrupted = False;

  p->crossStart = 0;
  p->crossEnd = 0;

  p->filledThreadStart = 0;
  p->numFilledThreads = 0;

  {
    unsigned numThreads = p->numThreadsMax;
    if (numThreads > MTDEC_THREADS_MAX)
      numThreads = MTDEC_THREADS_MAX;
    p->numStartedThreads_Limit = numThreads;
    p->numStartedThreads = 0;
  }

  if (p->inBufSize != p->allocatedBufsSize)
  {
    for (i = 0; i < MTDEC_THREADS_MAX; i++)
    {
      CMtDecThread *t = &p->threads[i];
      if (t->inBuf)
        MtDecThread_FreeInBufs(t);
    }
    if (p->crossBlock)
    {
      ISzAlloc_Free(p->alloc, p->crossBlock);
      p->crossBlock = NULL;
    }

    p->allocatedBufsSize = p->inBufSize;
  }

  MtProgress_Init(&p->mtProgress, p->progress);

  // RINOK_THREAD(AutoResetEvent_OptCreate_And_Reset(&p->finishedEvent))
  p->exitThread = False;
  p->exitThreadWRes = 0;

  {
    WRes wres;
    SRes sres;
    CMtDecThread *nextThread = &p->threads[p->numStartedThreads++];
    // wres = MtDecThread_CreateAndStart(nextThread);
    wres = MtDecThread_CreateEvents(nextThread);
    if (wres == 0) { wres = Event_Set(&nextThread->canWrite);
    if (wres == 0) { wres = Event_Set(&nextThread->canRead);
    if (wres == 0) { THREAD_FUNC_RET_TYPE res = MtDec_ThreadFunc(nextThread);
    wres = (WRes)(MY_uintptr_t)res;
    if (wres != 0)
    {
      p->needContinue = False;
      MtDec_CloseThreads(p);
    }}}}

    // wres = 17; // for test
    // wres = Event_Wait(&p->finishedEvent);

    sres = MY_SRes_HRESULT_FROM_WRes(wres);

    if (sres != 0)
      p->threadingErrorSRes = sres;

    if (
        // wres == 0
        // wres != 0
        // || p->mtc.codeRes == SZ_ERROR_MEM
        p->isAllocError
        || p->threadingErrorSRes != SZ_OK
        || p->overflow)
    {
      // p->needContinue = True;
    }
    else
      p->needContinue = False;
    
    if (p->needContinue)
      return SZ_OK;

    // if (sres != SZ_OK)
    return sres;
    // return SZ_ERROR_FAIL;
  }
}

#endif

#undef PRF

/* ================ unit: C/Ppmd7Enc.c ================ */
/* Ppmd7Enc.c -- Ppmd7z (PPMdH with 7z Range Coder) Encoder
2023-09-07 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.H (2001): Dmitry Shkarin : Public domain */


// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define kTopValue ((UInt32)1 << 24)

#define R (&p->rc.enc)

void Ppmd7z_Init_RangeEnc(CPpmd7 *p)
{
  R->Low = 0;
  R->Range = 0xFFFFFFFF;
  R->Cache = 0;
  R->CacheSize = 1;
}

Z7_NO_INLINE
static void Ppmd7z_RangeEnc_ShiftLow(CPpmd7 *p)
{
  if ((UInt32)R->Low < (UInt32)0xFF000000 || (unsigned)(R->Low >> 32) != 0)
  {
    Byte temp = R->Cache;
    do
    {
      IByteOut_Write(R->Stream, (Byte)(temp + (Byte)(R->Low >> 32)));
      temp = 0xFF;
    }
    while (--R->CacheSize != 0);
    R->Cache = (Byte)((UInt32)R->Low >> 24);
  }
  R->CacheSize++;
  R->Low = (UInt32)((UInt32)R->Low << 8);
}

#define RC_NORM_BASE(p) if (R->Range < kTopValue) { R->Range <<= 8;  Ppmd7z_RangeEnc_ShiftLow(p);
#define RC_NORM_1(p)    RC_NORM_BASE(p) }
#define RC_NORM(p)      RC_NORM_BASE(p)  RC_NORM_BASE(p) }}

// we must use only one type of Normalization from two: LOCAL or REMOTE
#define RC_NORM_LOCAL(p)    // RC_NORM(p)
#define RC_NORM_REMOTE(p)   RC_NORM(p)

/*
#define Ppmd7z_RangeEnc_Encode(p, start, _size_) \
  { UInt32 size = _size_; \
    R->Low += start * R->Range; \
    R->Range *= size; \
    RC_NORM_LOCAL(p); }
*/

Z7_FORCE_INLINE
// Z7_NO_INLINE
static void Ppmd7z_RangeEnc_Encode(CPpmd7 *p, UInt32 start, UInt32 size)
{
  R->Low += start * R->Range;
  R->Range *= size;
  RC_NORM_LOCAL(p)
}

void Ppmd7z_Flush_RangeEnc(CPpmd7 *p)
{
  unsigned i;
  for (i = 0; i < 5; i++)
    Ppmd7z_RangeEnc_ShiftLow(p);
}



#define RC_Encode(start, size)  Ppmd7z_RangeEnc_Encode(p, start, size);
#define RC_EncodeFinal(start, size)  RC_Encode(start, size) RC_NORM_REMOTE(p)

#define CTX(ref) ((CPpmd7_Context *)Ppmd7_GetContext(p, ref))
#define SUFFIX(ctx) CTX((ctx)->Suffix)
// typedef CPpmd7_Context * CTX_PTR;
#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)

void Ppmd7_UpdateModel(CPpmd7 *p);

#define MASK(sym)  ((Byte *)charMask)[sym]

Z7_FORCE_INLINE
static
void Ppmd7z_EncodeSymbol(CPpmd7 *p, int symbol)
{
  size_t charMask[256 / sizeof(size_t)];
  
  if (p->MinContext->NumStats != 1)
  {
    CPpmd_State *s = Ppmd7_GetStats(p, p->MinContext);
    UInt32 sum;
    unsigned i;
   

    
    
    R->Range /= p->MinContext->Union2.SummFreq;
    
    if (s->Symbol == symbol)
    {
      // R->Range /= p->MinContext->Union2.SummFreq;
      RC_EncodeFinal(0, s->Freq)
      p->FoundState = s;
      Ppmd7_Update1_0(p);
      return;
    }
    p->PrevSuccess = 0;
    sum = s->Freq;
    i = (unsigned)p->MinContext->NumStats - 1;
    do
    {
      if ((++s)->Symbol == symbol)
      {
        // R->Range /= p->MinContext->Union2.SummFreq;
        RC_EncodeFinal(sum, s->Freq)
        p->FoundState = s;
        Ppmd7_Update1(p);
        return;
      }
      sum += s->Freq;
    }
    while (--i);

    // R->Range /= p->MinContext->Union2.SummFreq;
    RC_Encode(sum, p->MinContext->Union2.SummFreq - sum)
    
    p->HiBitsFlag = PPMD7_HiBitsFlag_3(p->FoundState->Symbol);
    PPMD_SetAllBitsIn256Bytes(charMask)
    // MASK(s->Symbol) = 0;
    // i = p->MinContext->NumStats - 1;
    // do { MASK((--s)->Symbol) = 0; } while (--i);
    {
      CPpmd_State *s2 = Ppmd7_GetStats(p, p->MinContext);
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
  else
  {
    UInt16 *prob = Ppmd7_GetBinSumm(p);
    CPpmd_State *s = Ppmd7Context_OneState(p->MinContext);
    UInt32 pr = *prob;
    const UInt32 bound = (R->Range >> 14) * pr;
    pr = PPMD_UPDATE_PROB_1(pr);
    if (s->Symbol == symbol)
    {
      *prob = (UInt16)(pr + (1 << PPMD_INT_BITS));
      // RangeEnc_EncodeBit_0(p, bound);
      R->Range = bound;
      RC_NORM_1(p)
      
      // p->FoundState = s;
      // Ppmd7_UpdateBin(p);
      {
        const unsigned freq = s->Freq;
        CPpmd7_Context *c = CTX(SUCCESSOR(s));
        p->FoundState = s;
        p->PrevSuccess = 1;
        p->RunLength++;
        s->Freq = (Byte)(freq + (freq < 128));
        // NextContext(p);
        if (p->OrderFall == 0 && (const Byte *)c > p->Text)
          p->MaxContext = p->MinContext = c;
        else
          Ppmd7_UpdateModel(p);
      }
      return;
    }

    *prob = (UInt16)pr;
    p->InitEsc = p->ExpEscape[pr >> 10];
    // RangeEnc_EncodeBit_1(p, bound);
    R->Low += bound;
    R->Range -= bound;
    RC_NORM_LOCAL(p)
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    MASK(s->Symbol) = 0;
    p->PrevSuccess = 0;
  }

  for (;;)
  {
    CPpmd_See *see;
    CPpmd_State *s;
    UInt32 sum, escFreq;
    CPpmd7_Context *mc;
    unsigned i, numMasked;
    
    RC_NORM_REMOTE(p)

    mc = p->MinContext;
    numMasked = mc->NumStats;

    do
    {
      p->OrderFall++;
      if (!mc->Suffix)
        return; /* EndMarker (symbol = -1) */
      mc = Ppmd7_GetContext(p, mc->Suffix);
      i = mc->NumStats;
    }
    while (i == numMasked);

    p->MinContext = mc;
    
    // see = Ppmd7_MakeEscFreq(p, numMasked, &escFreq);
    {
      if (i != 256)
      {
        unsigned nonMasked = i - numMasked;
        see = p->See[(unsigned)p->NS2Indx[(size_t)nonMasked - 1]]
            + p->HiBitsFlag
            + (nonMasked < (unsigned)SUFFIX(mc)->NumStats - i)
            + 2 * (unsigned)(mc->Union2.SummFreq < 11 * i)
            + 4 * (unsigned)(numMasked > nonMasked);
        {
          // if (see->Summ) field is larger than 16-bit, we need only low 16 bits of Summ
          unsigned summ = (UInt16)see->Summ; // & 0xFFFF
          unsigned r = (summ >> see->Shift);
          see->Summ = (UInt16)(summ - r);
          escFreq = r + (r == 0);
        }
      }
      else
      {
        see = &p->DummySee;
        escFreq = 1;
      }
    }

    s = Ppmd7_GetStats(p, mc);
    sum = 0;
    // i = mc->NumStats;

    do
    {
      const unsigned cur = s->Symbol;
      if ((int)cur == symbol)
      {
        const UInt32 low = sum;
        const UInt32 freq = s->Freq;
        unsigned num2;

        Ppmd_See_UPDATE(see)
        p->FoundState = s;
        sum += escFreq;

        num2 = i / 2;
        i &= 1;
        sum += freq & (0 - (UInt32)i);
        if (num2 != 0)
        {
          s += i;
          do
          {
            const unsigned sym0 = s[0].Symbol;
            const unsigned sym1 = s[1].Symbol;
            s += 2;
            sum += (s[-2].Freq & (unsigned)(MASK(sym0)));
            sum += (s[-1].Freq & (unsigned)(MASK(sym1)));
          }
          while (--num2);
        }

        
        R->Range /= sum;
        RC_EncodeFinal(low, freq)
        Ppmd7_Update2(p);
        return;
      }
      sum += (s->Freq & (unsigned)(MASK(cur)));
      s++;
    }
    while (--i);
    
    {
      const UInt32 total = sum + escFreq;
      see->Summ = (UInt16)(see->Summ + total);

      R->Range /= total;
      RC_Encode(sum, escFreq)
    }

    {
      const CPpmd_State *s2 = Ppmd7_GetStats(p, p->MinContext);
      s--;
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
}


void Ppmd7z_EncodeSymbols(CPpmd7 *p, const Byte *buf, const Byte *lim)
{
  for (; buf < lim; buf++)
  {
    Ppmd7z_EncodeSymbol(p, *buf);
  }
}

#undef kTopValue
#undef WRITE_BYTE
#undef RC_NORM_BASE
#undef RC_NORM_1
#undef RC_NORM
#undef RC_NORM_LOCAL
#undef RC_NORM_REMOTE
#undef R
#undef RC_Encode
#undef RC_EncodeFinal
#undef SUFFIX
#undef CTX
#undef SUCCESSOR
#undef MASK

/* ================ unit: C/Ppmd7aDec.c ================ */
/* Ppmd7aDec.c -- PPMd7a (PPMdH) Decoder
2023-09-07 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.H (2001): Dmitry Shkarin : Public domain
  Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define kTop ((UInt32)1 << 24)
#define kBot ((UInt32)1 << 15)

#define READ_BYTE(p) IByteIn_Read((p)->Stream)

BoolInt Ppmd7a_RangeDec_Init(CPpmd7_RangeDec *p)
{
  unsigned i;
  p->Code = 0;
  p->Range = 0xFFFFFFFF;
  p->Low = 0;
  
  for (i = 0; i < 4; i++)
    p->Code = (p->Code << 8) | READ_BYTE(p);
  return (p->Code < 0xFFFFFFFF);
}

#define RC_NORM(p) \
  while ((p->Low ^ (p->Low + p->Range)) < kTop \
    || (p->Range < kBot && ((p->Range = (0 - p->Low) & (kBot - 1)), 1))) { \
      p->Code = (p->Code << 8) | READ_BYTE(p); \
      p->Range <<= 8; p->Low <<= 8; }

// we must use only one type of Normalization from two: LOCAL or REMOTE
#define RC_NORM_LOCAL(p)    // RC_NORM(p)
#define RC_NORM_REMOTE(p)   RC_NORM(p)

#define R (&p->rc.dec)

Z7_FORCE_INLINE
// Z7_NO_INLINE
static void Ppmd7a_RD_Decode(CPpmd7 *p, UInt32 start, UInt32 size)
{
  start *= R->Range;
  R->Low += start;
  R->Code -= start;
  R->Range *= size;
  RC_NORM_LOCAL(R)
}

#define RC_Decode(start, size)  Ppmd7a_RD_Decode(p, start, size);
#define RC_DecodeFinal(start, size)  RC_Decode(start, size)  RC_NORM_REMOTE(R)
#define RC_GetThreshold(total)  (R->Code / (R->Range /= (total)))


#define CTX(ref) ((CPpmd7_Context *)Ppmd7_GetContext(p, ref))
typedef CPpmd7_Context * CTX_PTR;
#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
void Ppmd7_UpdateModel(CPpmd7 *p);

#define MASK(sym)  ((Byte *)charMask)[sym]


int Ppmd7a_DecodeSymbol(CPpmd7 *p)
{
  size_t charMask[256 / sizeof(size_t)];

  if (p->MinContext->NumStats != 1)
  {
    CPpmd_State *s = Ppmd7_GetStats(p, p->MinContext);
    unsigned i;
    UInt32 count, hiCnt;
    const UInt32 summFreq = p->MinContext->Union2.SummFreq;

    if (summFreq > R->Range)
      return PPMD7_SYM_ERROR;

    count = RC_GetThreshold(summFreq);
    hiCnt = count;
    
    if ((Int32)(count -= s->Freq) < 0)
    {
      Byte sym;
      RC_DecodeFinal(0, s->Freq)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd7_Update1_0(p);
      return sym;
    }
  
    p->PrevSuccess = 0;
    i = (unsigned)p->MinContext->NumStats - 1;
    
    do
    {
      if ((Int32)(count -= (++s)->Freq) < 0)
      {
        Byte sym;
        RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)
        p->FoundState = s;
        sym = s->Symbol;
        Ppmd7_Update1(p);
        return sym;
      }
    }
    while (--i);
    
    if (hiCnt >= summFreq)
      return PPMD7_SYM_ERROR;
    
    hiCnt -= count;
    RC_Decode(hiCnt, summFreq - hiCnt)

    p->HiBitsFlag = PPMD7_HiBitsFlag_3(p->FoundState->Symbol);
    PPMD_SetAllBitsIn256Bytes(charMask)
    // i = p->MinContext->NumStats - 1;
    // do { MASK((--s)->Symbol) = 0; } while (--i);
    {
      CPpmd_State *s2 = Ppmd7_GetStats(p, p->MinContext);
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
  else
  {
    CPpmd_State *s = Ppmd7Context_OneState(p->MinContext);
    UInt16 *prob = Ppmd7_GetBinSumm(p);
    UInt32 pr = *prob;
    UInt32 size0 = (R->Range >> 14) * pr;
    pr = PPMD_UPDATE_PROB_1(pr);

    if (R->Code < size0)
    {
      Byte sym;
      *prob = (UInt16)(pr + (1 << PPMD_INT_BITS));
      
      // RangeDec_DecodeBit0(size0);
      R->Range = size0;
      RC_NORM(R)
      
        
        
      // sym = (p->FoundState = Ppmd7Context_OneState(p->MinContext))->Symbol;
      // Ppmd7_UpdateBin(p);
      {
        unsigned freq = s->Freq;
        CTX_PTR c = CTX(SUCCESSOR(s));
        sym = s->Symbol;
        p->FoundState = s;
        p->PrevSuccess = 1;
        p->RunLength++;
        s->Freq = (Byte)(freq + (freq < 128));
        // NextContext(p);
        if (p->OrderFall == 0 && (const Byte *)c > p->Text)
          p->MaxContext = p->MinContext = c;
        else
          Ppmd7_UpdateModel(p);
      }
      return sym;
    }

    *prob = (UInt16)pr;
    p->InitEsc = p->ExpEscape[pr >> 10];

    // RangeDec_DecodeBit1(size0);
    R->Low += size0;
    R->Code -= size0;
    R->Range = (R->Range & ~((UInt32)PPMD_BIN_SCALE - 1)) - size0;
    RC_NORM_LOCAL(R)
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    MASK(Ppmd7Context_OneState(p->MinContext)->Symbol) = 0;
    p->PrevSuccess = 0;
  }

  for (;;)
  {
    CPpmd_State *s, *s2;
    UInt32 freqSum, count, hiCnt;

    CPpmd_See *see;
    CPpmd7_Context *mc;
    unsigned numMasked;
    RC_NORM_REMOTE(R)
    mc = p->MinContext;
    numMasked = mc->NumStats;

    do
    {
      p->OrderFall++;
      if (!mc->Suffix)
        return PPMD7_SYM_END;
      mc = Ppmd7_GetContext(p, mc->Suffix);
    }
    while (mc->NumStats == numMasked);
    
    s = Ppmd7_GetStats(p, mc);

    {
      unsigned num = mc->NumStats;
      unsigned num2 = num / 2;
      
      num &= 1;
      hiCnt = (s->Freq & (UInt32)(MASK(s->Symbol))) & (0 - (UInt32)num);
      s += num;
      p->MinContext = mc;

      do
      {
        const unsigned sym0 = s[0].Symbol;
        const unsigned sym1 = s[1].Symbol;
        s += 2;
        hiCnt += (s[-2].Freq & (UInt32)(MASK(sym0)));
        hiCnt += (s[-1].Freq & (UInt32)(MASK(sym1)));
      }
      while (--num2);
    }

    see = Ppmd7_MakeEscFreq(p, numMasked, &freqSum);
    freqSum += hiCnt;

    if (freqSum > R->Range)
      return PPMD7_SYM_ERROR;

    count = RC_GetThreshold(freqSum);
    
    if (count < hiCnt)
    {
      Byte sym;

      s = Ppmd7_GetStats(p, p->MinContext);
      hiCnt = count;
      // count -= s->Freq & (UInt32)(MASK(s->Symbol));
      // if ((Int32)count >= 0)
      {
        for (;;)
        {
          count -= s->Freq & (UInt32)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
          // count -= s->Freq & (UInt32)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
        }
      }
      s--;
      RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)

      // new (see->Summ) value can overflow over 16-bits in some rare cases
      Ppmd_See_UPDATE(see)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd7_Update2(p);
      return sym;
    }

    if (count >= freqSum)
      return PPMD7_SYM_ERROR;
    
    RC_Decode(hiCnt, freqSum - hiCnt)

    // We increase (see->Summ) for sum of Freqs of all non_Masked symbols.
    // new (see->Summ) value can overflow over 16-bits in some rare cases
    see->Summ = (UInt16)(see->Summ + freqSum);

    s = Ppmd7_GetStats(p, p->MinContext);
    s2 = s + p->MinContext->NumStats;
    do
    {
      MASK(s->Symbol) = 0;
      s++;
    }
    while (s != s2);
  }
}

#undef kTop
#undef kBot
#undef READ_BYTE
#undef RC_NORM_BASE
#undef RC_NORM_1
#undef RC_NORM
#undef RC_NORM_LOCAL
#undef RC_NORM_REMOTE
#undef R
#undef RC_Decode
#undef RC_DecodeFinal
#undef RC_GetThreshold
#undef CTX
#undef SUCCESSOR
#undef MASK

/* ================ unit: C/Ppmd8Enc.c ================ */
/* Ppmd8Enc.c -- Ppmd8 (PPMdI) Encoder
2023-09-07 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.I (2002): Dmitry Shkarin : Public domain
  Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define kTop ((UInt32)1 << 24)
#define kBot ((UInt32)1 << 15)

#define WRITE_BYTE(p) IByteOut_Write(p->Stream.Out, (Byte)(p->Low >> 24))

void Ppmd8_Flush_RangeEnc(CPpmd8 *p)
{
  unsigned i;
  for (i = 0; i < 4; i++, p->Low <<= 8 )
    WRITE_BYTE(p);
}






#define RC_NORM(p) \
  while ((p->Low ^ (p->Low + p->Range)) < kTop \
    || (p->Range < kBot && ((p->Range = (0 - p->Low) & (kBot - 1)), 1))) \
    { WRITE_BYTE(p); p->Range <<= 8; p->Low <<= 8; }













// we must use only one type of Normalization from two: LOCAL or REMOTE
#define RC_NORM_LOCAL(p)    // RC_NORM(p)
#define RC_NORM_REMOTE(p)   RC_NORM(p)

// #define RC_PRE(total) p->Range /= total;
// #define RC_PRE(total)

#define R p




Z7_FORCE_INLINE
// Z7_NO_INLINE
static void Ppmd8_RangeEnc_Encode(CPpmd8 *p, UInt32 start, UInt32 size, UInt32 total)
{
  R->Low += start * (R->Range /= total);
  R->Range *= size;
  RC_NORM_LOCAL(R)
}










#define RC_Encode(start, size, total)  Ppmd8_RangeEnc_Encode(p, start, size, total);
#define RC_EncodeFinal(start, size, total)  RC_Encode(start, size, total)  RC_NORM_REMOTE(p)

#define CTX(ref) ((CPpmd8_Context *)Ppmd8_GetContext(p, ref))

// typedef CPpmd8_Context * CTX_PTR;
#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)

void Ppmd8_UpdateModel(CPpmd8 *p);

#define MASK(sym)  ((Byte *)charMask)[sym]

// Z7_FORCE_INLINE
// static
void Ppmd8_EncodeSymbol(CPpmd8 *p, int symbol)
{
  size_t charMask[256 / sizeof(size_t)];
  
  if (p->MinContext->NumStats != 0)
  {
    CPpmd_State *s = Ppmd8_GetStats(p, p->MinContext);
    UInt32 sum;
    unsigned i;
    UInt32 summFreq = p->MinContext->Union2.SummFreq;
    
    PPMD8_CORRECT_SUM_RANGE(p, summFreq)

    // RC_PRE(summFreq);

    if (s->Symbol == symbol)
    {

      RC_EncodeFinal(0, s->Freq, summFreq)
      p->FoundState = s;
      Ppmd8_Update1_0(p);
      return;
    }
    p->PrevSuccess = 0;
    sum = s->Freq;
    i = p->MinContext->NumStats;
    do
    {
      if ((++s)->Symbol == symbol)
      {

        RC_EncodeFinal(sum, s->Freq, summFreq)
        p->FoundState = s;
        Ppmd8_Update1(p);
        return;
      }
      sum += s->Freq;
    }
    while (--i);
    
    
    RC_Encode(sum, summFreq - sum, summFreq)
    
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    // MASK(s->Symbol) = 0;
    // i = p->MinContext->NumStats;
    // do { MASK((--s)->Symbol) = 0; } while (--i);
    {
      CPpmd_State *s2 = Ppmd8_GetStats(p, p->MinContext);
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
  else
  {
    UInt16 *prob = Ppmd8_GetBinSumm(p);
    CPpmd_State *s = Ppmd8Context_OneState(p->MinContext);
    UInt32 pr = *prob;
    const UInt32 bound = (R->Range >> 14) * pr;
    pr = PPMD_UPDATE_PROB_1(pr);
    if (s->Symbol == symbol)
    {
      *prob = (UInt16)(pr + (1 << PPMD_INT_BITS));
      // RangeEnc_EncodeBit_0(p, bound);
      R->Range = bound;
      RC_NORM(R)

      // p->FoundState = s;
      // Ppmd8_UpdateBin(p);
      {
        const unsigned freq = s->Freq;
        CPpmd8_Context *c = CTX(SUCCESSOR(s));
        p->FoundState = s;
        p->PrevSuccess = 1;
        p->RunLength++;
        s->Freq = (Byte)(freq + (freq < 196)); // Ppmd8 (196)
        // NextContext(p);
        if (p->OrderFall == 0 && (const Byte *)c >= p->UnitsStart)
          p->MaxContext = p->MinContext = c;
        else
          Ppmd8_UpdateModel(p);
      }
      return;
    }

    *prob = (UInt16)pr;
    p->InitEsc = p->ExpEscape[pr >> 10];
    // RangeEnc_EncodeBit_1(p, bound);
    R->Low += bound;
    R->Range = (R->Range & ~((UInt32)PPMD_BIN_SCALE - 1)) - bound;
    RC_NORM_LOCAL(R)

    PPMD_SetAllBitsIn256Bytes(charMask)
    MASK(s->Symbol) = 0;
    p->PrevSuccess = 0;
  }

  for (;;)
  {
    CPpmd_See *see;
    CPpmd_State *s;
    UInt32 sum, escFreq;
    CPpmd8_Context *mc;
    unsigned i, numMasked;

    RC_NORM_REMOTE(p)

    mc = p->MinContext;
    numMasked = mc->NumStats;

    do
    {
      p->OrderFall++;
      if (!mc->Suffix)
        return; /* EndMarker (symbol = -1) */
      mc = Ppmd8_GetContext(p, mc->Suffix);

    }
    while (mc->NumStats == numMasked);
    
    p->MinContext = mc;

    see = Ppmd8_MakeEscFreq(p, numMasked, &escFreq);
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    

    
    
    
    
    
    
    
    s = Ppmd8_GetStats(p, p->MinContext);
    sum = 0;
    i = (unsigned)p->MinContext->NumStats + 1;

    do
    {
      const unsigned cur = s->Symbol;
      if ((int)cur == symbol)
      {
        const UInt32 low = sum;
        const UInt32 freq = s->Freq;
        unsigned num2;

        Ppmd_See_UPDATE(see)
        p->FoundState = s;
        sum += escFreq;

        num2 = i / 2;
        i &= 1;
        sum += freq & (0 - (UInt32)i);
        if (num2 != 0)
        {
          s += i;
          do
          {
            const unsigned sym0 = s[0].Symbol;
            const unsigned sym1 = s[1].Symbol;
            s += 2;
            sum += (s[-2].Freq & (unsigned)(MASK(sym0)));
            sum += (s[-1].Freq & (unsigned)(MASK(sym1)));
          }
          while (--num2);
        }

        PPMD8_CORRECT_SUM_RANGE(p, sum)

        RC_EncodeFinal(low, freq, sum)
        Ppmd8_Update2(p);
        return;
      }
      sum += (s->Freq & (unsigned)(MASK(cur)));
      s++;
    }
    while (--i);
    
    {
      UInt32 total = sum + escFreq;
      see->Summ = (UInt16)(see->Summ + total);
      PPMD8_CORRECT_SUM_RANGE(p, total)

      RC_Encode(sum, total - sum, total)
    }

    {
      const CPpmd_State *s2 = Ppmd8_GetStats(p, p->MinContext);
      s--;
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
}









#undef kTop
#undef kBot
#undef WRITE_BYTE
#undef RC_NORM_BASE
#undef RC_NORM_1
#undef RC_NORM
#undef RC_NORM_LOCAL
#undef RC_NORM_REMOTE
#undef R
#undef RC_Encode
#undef RC_EncodeFinal

#undef CTX
#undef SUCCESSOR
#undef MASK

/* ================ unit: C/Sha1.c ================ */
/* Sha1.c -- SHA-1 Hash
: Igor Pavlov : Public domain
This code is based on public domain code of Steve Reid from Wei Dai's Crypto++ library. */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef MY_CPU_X86_OR_AMD64
  #if   defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 30800) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 50100) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 40900) \
     || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1600) \
     || defined(_MSC_VER) && (_MSC_VER >= 1200)
      #define Z7_COMPILER_SHA1_SUPPORTED
  #endif
#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE) \
   && (!defined(Z7_MSC_VER_ORIGINAL) || (_MSC_VER >= 1929) && (_MSC_FULL_VER >= 192930037))
  #if   defined(__ARM_FEATURE_SHA2) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define Z7_COMPILER_SHA1_SUPPORTED
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
      #define Z7_COMPILER_SHA1_SUPPORTED
    #endif
    #endif
    #endif
  #endif
#endif

void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks);

#ifdef Z7_COMPILER_SHA1_SUPPORTED
  void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);

  static SHA1_FUNC_UPDATE_BLOCKS g_SHA1_FUNC_UPDATE_BLOCKS = Sha1_UpdateBlocks;
  static SHA1_FUNC_UPDATE_BLOCKS g_SHA1_FUNC_UPDATE_BLOCKS_HW;

  #define SHA1_UPDATE_BLOCKS(p) p->v.vars.func_UpdateBlocks
#else
  #define SHA1_UPDATE_BLOCKS(p) Sha1_UpdateBlocks
#endif


BoolInt Sha1_SetFunction(CSha1 *p, unsigned algo)
{
  SHA1_FUNC_UPDATE_BLOCKS func = Sha1_UpdateBlocks;
  
  #ifdef Z7_COMPILER_SHA1_SUPPORTED
    if (algo != SHA1_ALGO_SW)
    {
      if (algo == SHA1_ALGO_DEFAULT)
        func = g_SHA1_FUNC_UPDATE_BLOCKS;
      else
      {
        if (algo != SHA1_ALGO_HW)
          return False;
        func = g_SHA1_FUNC_UPDATE_BLOCKS_HW;
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
// #define Z7_SHA1_UNROLL

// allowed unroll steps: (1, 2, 4, 5, 20)

#undef Z7_SHA1_BIG_W
#ifdef Z7_SHA1_UNROLL
  #define STEP_PRE  20
  #define STEP_MAIN 20
#else
  #define Z7_SHA1_BIG_W
  #define STEP_PRE  5
  #define STEP_MAIN 5
#endif


#ifdef Z7_SHA1_BIG_W
  #define kNumW 80
  #define w(i) W[i]
#else
  #define kNumW 16
  #define w(i) W[(i)&15]
#endif

#define w0(i) (W[i] = GetBe32(data + (size_t)(i) * 4))
#define w1(i) (w(i) = rotlFixed(w((size_t)(i)-3) ^ w((size_t)(i)-8) ^ w((size_t)(i)-14) ^ w((size_t)(i)-16), 1))

#define f0(x,y,z)  ( 0x5a827999 + (z^(x&(y^z))) )
#define f1(x,y,z)  ( 0x6ed9eba1 + (x^y^z) )
#define f2(x,y,z)  ( 0x8f1bbcdc + ((x&y)|(z&(x|y))) )
#define f3(x,y,z)  ( 0xca62c1d6 + (x^y^z) )

/*
#define T1(fx, ww) \
    tmp = e + fx(b,c,d) + ww + rotlFixed(a, 5); \
    e = d; \
    d = c; \
    c = rotlFixed(b, 30); \
    b = a; \
    a = tmp; \
*/

#define T5(a,b,c,d,e, fx, ww) \
    e += fx(b,c,d) + ww + rotlFixed(a, 5); \
    b = rotlFixed(b, 30); \


/*
#define R1(i, fx, wx) \
    T1 ( fx, wx(i)); \

#define R2(i, fx, wx) \
    R1 ( (i)    , fx, wx); \
    R1 ( (i) + 1, fx, wx); \

#define R4(i, fx, wx) \
    R2 ( (i)    , fx, wx); \
    R2 ( (i) + 2, fx, wx); \
*/

#define M5(i, fx, wx0, wx1) \
    T5 ( a,b,c,d,e, fx, wx0((i)  ) ) \
    T5 ( e,a,b,c,d, fx, wx1((i)+1) ) \
    T5 ( d,e,a,b,c, fx, wx1((i)+2) ) \
    T5 ( c,d,e,a,b, fx, wx1((i)+3) ) \
    T5 ( b,c,d,e,a, fx, wx1((i)+4) ) \

#define R5(i, fx, wx) \
    M5 ( i, fx, wx, wx) \


#if STEP_PRE > 5

  #define R20_START \
    R5 (  0, f0, w0) \
    R5 (  5, f0, w0) \
    R5 ( 10, f0, w0) \
    M5 ( 15, f0, w0, w1) \
  
  #elif STEP_PRE == 5
  
  #define R20_START \
    { size_t i; for (i = 0; i < 15; i += STEP_PRE) \
      { R5(i, f0, w0) } } \
    M5 ( 15, f0, w0, w1) \

#else

  #if STEP_PRE == 1
    #define R_PRE R1
  #elif STEP_PRE == 2
    #define R_PRE R2
  #elif STEP_PRE == 4
    #define R_PRE R4
  #endif

  #define R20_START \
    { size_t i; for (i = 0; i < 16; i += STEP_PRE) \
      { R_PRE(i, f0, w0) } } \
    R4 ( 16, f0, w1) \

#endif



#if STEP_MAIN > 5

  #define R20(ii, fx) \
    R5 ( (ii)     , fx, w1) \
    R5 ( (ii) + 5 , fx, w1) \
    R5 ( (ii) + 10, fx, w1) \
    R5 ( (ii) + 15, fx, w1) \

#else

  #if STEP_MAIN == 1
    #define R_MAIN R1
  #elif STEP_MAIN == 2
    #define R_MAIN R2
  #elif STEP_MAIN == 4
    #define R_MAIN R4
  #elif STEP_MAIN == 5
    #define R_MAIN R5
  #endif

  #define R20(ii, fx)  \
    { size_t i; for (i = (ii); i < (ii) + 20; i += STEP_MAIN) \
      { R_MAIN(i, fx, w1) } } \

#endif



void Sha1_InitState(CSha1 *p)
{
  p->v.vars.count = 0;
  p->state[0] = 0x67452301;
  p->state[1] = 0xEFCDAB89;
  p->state[2] = 0x98BADCFE;
  p->state[3] = 0x10325476;
  p->state[4] = 0xC3D2E1F0;
}

void Sha1_Init(CSha1 *p)
{
  p->v.vars.func_UpdateBlocks =
  #ifdef Z7_COMPILER_SHA1_SUPPORTED
      g_SHA1_FUNC_UPDATE_BLOCKS;
  #else
      NULL;
  #endif
  Sha1_InitState(p);
}


Z7_NO_INLINE
void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  UInt32 a, b, c, d, e;
  UInt32 W[kNumW];

  if (numBlocks == 0)
    return;

  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  do
  {
  #if STEP_PRE < 5 || STEP_MAIN < 5
  UInt32 tmp;
  #endif

  R20_START
  R20(20, f1)
  R20(40, f2)
  R20(60, f3)

  a += state[0];
  b += state[1];
  c += state[2];
  d += state[3];
  e += state[4];

  state[0] = a;
  state[1] = b;
  state[2] = c;
  state[3] = d;
  state[4] = e;

  data += SHA1_BLOCK_SIZE;
  }
  while (--numBlocks);
}


#define Sha1_UpdateBlock(p) SHA1_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Sha1_Update(CSha1 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->v.vars.count & (SHA1_BLOCK_SIZE - 1);
    const unsigned num = SHA1_BLOCK_SIZE - pos;
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
      Sha1_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 6;
    // if (numBlocks)
    SHA1_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= SHA1_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}


void Sha1_Final(CSha1 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->v.vars.count & (SHA1_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (SHA1_BLOCK_SIZE - 4 * 2))
  {
    while (pos != SHA1_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, SHA1_BLOCK_SIZE - pos);
    Sha1_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (SHA1_BLOCK_SIZE - 4 * 2) - pos);
  {
    const UInt64 numBits = p->v.vars.count << 3;
    SetBe32(p->buffer + SHA1_BLOCK_SIZE - 4 * 2, (UInt32)(numBits >> 32))
    SetBe32(p->buffer + SHA1_BLOCK_SIZE - 4 * 1, (UInt32)(numBits))
  }
  Sha1_UpdateBlock(p);

  SetBe32(digest,      p->state[0])
  SetBe32(digest + 4,  p->state[1])
  SetBe32(digest + 8,  p->state[2])
  SetBe32(digest + 12, p->state[3])
  SetBe32(digest + 16, p->state[4])
  
  Sha1_InitState(p);
}


void Sha1_PrepareBlock(const CSha1 *p, Byte *block, unsigned size)
{
  const UInt64 numBits = (p->v.vars.count + size) << 3;
  SetBe32(&((UInt32 *)(void *)block)[SHA1_NUM_BLOCK_WORDS - 2], (UInt32)(numBits >> 32))
  SetBe32(&((UInt32 *)(void *)block)[SHA1_NUM_BLOCK_WORDS - 1], (UInt32)(numBits))
  // SetBe32((UInt32 *)(block + size), 0x80000000);
  SetUi32((UInt32 *)(void *)(block + size), 0x80)
  size += 4;
  while (size != (SHA1_NUM_BLOCK_WORDS - 2) * 4)
  {
    *((UInt32 *)(void *)(block + size)) = 0;
    size += 4;
  }
}

void Sha1_GetBlockDigest(const CSha1 *p, const Byte *data, Byte *destDigest)
{
  MY_ALIGN (16)
  UInt32 st[SHA1_NUM_DIGEST_WORDS];

  st[0] = p->state[0];
  st[1] = p->state[1];
  st[2] = p->state[2];
  st[3] = p->state[3];
  st[4] = p->state[4];
  
  SHA1_UPDATE_BLOCKS(p)(st, data, 1);

  SetBe32(destDigest + 0    , st[0])
  SetBe32(destDigest + 1 * 4, st[1])
  SetBe32(destDigest + 2 * 4, st[2])
  SetBe32(destDigest + 3 * 4, st[3])
  SetBe32(destDigest + 4 * 4, st[4])
}


void Sha1Prepare(void)
{
#ifdef Z7_COMPILER_SHA1_SUPPORTED
  SHA1_FUNC_UPDATE_BLOCKS f, f_hw;
  f = Sha1_UpdateBlocks;
  f_hw = NULL;
#ifdef MY_CPU_X86_OR_AMD64
  if (CPU_IsSupported_SHA()
      && CPU_IsSupported_SSSE3()
      )
#else
  if (CPU_IsSupported_SHA1())
#endif
  {
    // printf("\n========== HW SHA1 ======== \n");
#if 1 && defined(MY_CPU_ARM_OR_ARM64) && defined(Z7_MSC_VER_ORIGINAL) && (_MSC_FULL_VER < 192930037)
    /* there was bug in MSVC compiler for ARM64 -O2 before version VS2019 16.10 (19.29.30037).
       It generated incorrect SHA-1 code. */
      #pragma message("== SHA1 code can work incorrectly with this compiler")
      #error Stop_Compiling_MSC_Compiler_BUG_SHA1
#endif
      {
        f = f_hw = Sha1_UpdateBlocks_HW;
      }
  }
  g_SHA1_FUNC_UPDATE_BLOCKS    = f;
  g_SHA1_FUNC_UPDATE_BLOCKS_HW = f_hw;
#endif
}

#undef kNumW
#undef w
#undef w0
#undef w1
#undef f0
#undef f1
#undef f2
#undef f3
#undef T1
#undef T5
#undef M5
#undef R1
#undef R2
#undef R4
#undef R5
#undef R20_START
#undef R_PRE
#undef R_MAIN
#undef STEP_PRE
#undef STEP_MAIN
#undef Z7_SHA1_BIG_W
#undef Z7_SHA1_UNROLL
#undef Z7_COMPILER_SHA1_SUPPORTED

/* ================ unit: C/Sha1Opt.c ================ */
/* Sha1Opt.c -- SHA-1 optimized code for SHA-1 hardware instructions
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

// #pragma message("Sha1 HW")




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
SHA1 uses:
SSE2:
  _mm_loadu_si128
  _mm_storeu_si128
  _mm_set_epi32
  _mm_add_epi32
  _mm_shuffle_epi32 / pshufd
  _mm_xor_si128
  _mm_cvtsi128_si32
  _mm_cvtsi32_si128
SSSE3:
  _mm_shuffle_epi8 / pshufb

SHA:
  _mm_sha1*
*/

#define XOR_SI128(dest, src)      dest = _mm_xor_si128(dest, src);
#define SHUFFLE_EPI8(dest, mask)  dest = _mm_shuffle_epi8(dest, mask);
#define SHUFFLE_EPI32(dest, mask) dest = _mm_shuffle_epi32(dest, mask);
#ifdef __clang__
#define SHA1_RNDS4_RET_TYPE_CAST (__m128i)
#else
#define SHA1_RNDS4_RET_TYPE_CAST
#endif
#define SHA1_RND4(abcd, e0, f)    abcd = SHA1_RNDS4_RET_TYPE_CAST _mm_sha1rnds4_epu32(abcd, e0, f);
#define SHA1_NEXTE(e, m)          e = _mm_sha1nexte_epu32(e, m);
#define ADD_EPI32(dest, src)      dest = _mm_add_epi32(dest, src);
#define SHA1_MSG1(dest, src)      dest = _mm_sha1msg1_epu32(dest, src);
#define SHA1_MSG2(dest, src)      dest = _mm_sha1msg2_epu32(dest, src);

#define LOAD_SHUFFLE(m, k) \
    m = _mm_loadu_si128((const __m128i *)(const void *)(data + (k) * 16)); \
    SHUFFLE_EPI8(m, mask) \

#define NNN(m0, m1, m2, m3)

#define SM1(m0, m1, m2, m3) \
    SHA1_MSG1(m0, m1) \

#define SM2(m0, m1, m2, m3) \
    XOR_SI128(m3, m1) \
    SHA1_MSG2(m3, m2) \

#define SM3(m0, m1, m2, m3) \
    XOR_SI128(m3, m1) \
    SM1(m0, m1, m2, m3) \
    SHA1_MSG2(m3, m2) \

#define R4(k, m0, m1, m2, m3, e0, e1, OP) \
    e1 = abcd; \
    SHA1_RND4(abcd, e0, (k) / 5) \
    SHA1_NEXTE(e1, m1) \
    OP(m0, m1, m2, m3) \



#define R16(k, mx, OP0, OP1, OP2, OP3) \
    R4 ( (k)*4+0, m0,m1,m2,m3, e0,e1, OP0 ) \
    R4 ( (k)*4+1, m1,m2,m3,m0, e1,e0, OP1 ) \
    R4 ( (k)*4+2, m2,m3,m0,m1, e0,e1, OP2 ) \
    R4 ( (k)*4+3, m3,mx,m1,m2, e1,e0, OP3 ) \

#define PREPARE_STATE \
    SHUFFLE_EPI32 (abcd, 0x1B) \
    SHUFFLE_EPI32 (e0,   0x1B) \





void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  const __m128i mask = _mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);

  
  __m128i abcd, e0;

  if (numBlocks == 0)
    return;
  
  abcd = _mm_loadu_si128((const __m128i *) (const void *) &state[0]); // dbca
  e0 = _mm_cvtsi32_si128((int)state[4]); // 000e
  
  PREPARE_STATE
  
  do
  {
    __m128i abcd_save, e2;
    __m128i m0, m1, m2, m3;
    __m128i e1;
    

    abcd_save = abcd;
    e2 = e0;

    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)

    ADD_EPI32(e0, m0)
    
    R16 ( 0, m0, SM1, SM3, SM3, SM3 )
    R16 ( 1, m0, SM3, SM3, SM3, SM3 )
    R16 ( 2, m0, SM3, SM3, SM3, SM3 )
    R16 ( 3, m0, SM3, SM3, SM3, SM3 )
    R16 ( 4, e2, SM2, NNN, NNN, NNN )
    
    ADD_EPI32(abcd, abcd_save)
    
    data += 64;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm_storeu_si128((__m128i *) (void *) state, abcd);
  *(state + 4) = (UInt32)_mm_cvtsi128_si32(e0);
}

#endif // USE_HW_SHA

#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE) \
   && (!defined(Z7_MSC_VER_ORIGINAL) || (_MSC_VER >= 1929) && (_MSC_FULL_VER >= 192930037))
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

// #pragma message("=== Sha1 HW === ")
// __ARM_FEATURE_CRYPTO macro is deprecated in favor of the finer grained feature macro __ARM_FEATURE_SHA2

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
// the bug in clang 3.8.1:
// __builtin_neon_vgetq_lane_i32((int8x16_t)__s0, __p1);
#if defined(__clang__) && (__clang_major__ <= 9)
#pragma GCC diagnostic ignored "-Wvector-conversion"
#endif

#ifdef MY_CPU_BE
  #define MY_rev32_for_LE(x) x
#else
  #define MY_rev32_for_LE(x) vrev32q_u8(x)
#endif

#define LOAD_128_32(_p)       vld1q_u32(_p)
#define LOAD_128_8(_p)        vld1q_u8 (_p)
#define STORE_128_32(_p, _v)  vst1q_u32(_p, _v)

#define LOAD_SHUFFLE(m, k) \
    m = vreinterpretq_u32_u8( \
        MY_rev32_for_LE( \
        LOAD_128_8(data + (k) * 16))); \

#define N0(dest, src2, src3)
#define N1(dest, src)
#define U0(dest, src2, src3)  dest = vsha1su0q_u32(dest, src2, src3);
#define U1(dest, src)         dest = vsha1su1q_u32(dest, src);
#define C(e)                  abcd = vsha1cq_u32(abcd, e, t)
#define P(e)                  abcd = vsha1pq_u32(abcd, e, t)
#define M(e)                  abcd = vsha1mq_u32(abcd, e, t)
#define H(e)                  e = vsha1h_u32(vgetq_lane_u32(abcd, 0))
#define T(m, c)               t = vaddq_u32(m, c)

#define R16(d0,d1,d2,d3, f0,z0, f1,z1, f2,z2, f3,z3, w0,w1,w2,w3) \
    T(m0, d0);  f0(m3, m0, m1)  z0(m2, m1)  H(e1);  w0(e0); \
    T(m1, d1);  f1(m0, m1, m2)  z1(m3, m2)  H(e0);  w1(e1); \
    T(m2, d2);  f2(m1, m2, m3)  z2(m0, m3)  H(e1);  w2(e0); \
    T(m3, d3);  f3(m2, m3, m0)  z3(m1, m0)  H(e0);  w3(e1); \


void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  v128 abcd;
  v128 c0, c1, c2, c3;
  uint32_t e0;

  if (numBlocks == 0)
    return;

  c0 = vdupq_n_u32(0x5a827999);
  c1 = vdupq_n_u32(0x6ed9eba1);
  c2 = vdupq_n_u32(0x8f1bbcdc);
  c3 = vdupq_n_u32(0xca62c1d6);

  abcd = LOAD_128_32(&state[0]);
  e0 = state[4];
  
  do
  {
    v128 abcd_save;
    v128 m0, m1, m2, m3;
    v128 t;
    uint32_t e0_save, e1;

    abcd_save = abcd;
    e0_save = e0;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)
                     
    R16 ( c0,c0,c0,c0, N0,N1, U0,N1, U0,U1, U0,U1, C,C,C,C )
    R16 ( c0,c1,c1,c1, U0,U1, U0,U1, U0,U1, U0,U1, C,P,P,P )
    R16 ( c1,c1,c2,c2, U0,U1, U0,U1, U0,U1, U0,U1, P,P,M,M )
    R16 ( c2,c2,c2,c3, U0,U1, U0,U1, U0,U1, U0,U1, M,M,M,P )
    R16 ( c3,c3,c3,c3, U0,U1, N0,U1, N0,N1, N0,N1, P,P,P,P )
                                                                                                                     
    abcd = vaddq_u32(abcd, abcd_save);
    e0 += e0_save;
    
    data += 64;
  }
  while (--numBlocks);

  STORE_128_32(&state[0], abcd);
  state[4] = e0;
}

#endif // USE_HW_SHA

#endif // MY_CPU_ARM_OR_ARM64

#if !defined(USE_HW_SHA) && defined(Z7_USE_HW_SHA_STUB)
// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>
// #include "Sha1.h"
// #if defined(_MSC_VER)
#pragma message("Sha1   HW-SW stub was used")
// #endif
void Z7_FASTCALL Sha1_UpdateBlocks   (UInt32 state[5], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  Sha1_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}
#endif

#undef U0
#undef U1
#undef N0
#undef N1
#undef C
#undef P
#undef M
#undef H
#undef T
#undef MY_rev32_for_LE
#undef NNN
#undef LOAD_128
#undef STORE_128
#undef LOAD_SHUFFLE
#undef SM1
#undef SM2
#undef SM3
#undef NNN
#undef R4
#undef R16
#undef PREPARE_STATE
#undef USE_HW_SHA
#undef ATTRIB_SHA
#undef USE_VER_MIN
#undef Z7_USE_HW_SHA_STUB

/* ================ unit: C/Sha256.c ================ */
/* Sha256.c -- SHA-256 Hash
: Igor Pavlov : Public domain
This code is based on public domain code from Wei Dai's Crypto++ library. */

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef MY_CPU_X86_OR_AMD64
  #if   defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 30800) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 50100) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 40900) \
     || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1600) \
     || defined(_MSC_VER) && (_MSC_VER >= 1200)
      #define Z7_COMPILER_SHA256_SUPPORTED
  #endif
#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE)

  #if   defined(__ARM_FEATURE_SHA2) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define Z7_COMPILER_SHA256_SUPPORTED
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
      #define Z7_COMPILER_SHA256_SUPPORTED
    #endif
    #endif
    #endif
  #endif
#endif

void Z7_FASTCALL Sha256_UpdateBlocks(UInt32 state[8], const Byte *data, size_t numBlocks);

#ifdef Z7_COMPILER_SHA256_SUPPORTED
  void Z7_FASTCALL Sha256_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);

  static SHA256_FUNC_UPDATE_BLOCKS g_SHA256_FUNC_UPDATE_BLOCKS = Sha256_UpdateBlocks;
  static SHA256_FUNC_UPDATE_BLOCKS g_SHA256_FUNC_UPDATE_BLOCKS_HW;

  #define SHA256_UPDATE_BLOCKS(p) p->v.vars.func_UpdateBlocks
#else
  #define SHA256_UPDATE_BLOCKS(p) Sha256_UpdateBlocks
#endif


BoolInt Sha256_SetFunction(CSha256 *p, unsigned algo)
{
  SHA256_FUNC_UPDATE_BLOCKS func = Sha256_UpdateBlocks;
  
  #ifdef Z7_COMPILER_SHA256_SUPPORTED
    if (algo != SHA256_ALGO_SW)
    {
      if (algo == SHA256_ALGO_DEFAULT)
        func = g_SHA256_FUNC_UPDATE_BLOCKS;
      else
      {
        if (algo != SHA256_ALGO_HW)
          return False;
        func = g_SHA256_FUNC_UPDATE_BLOCKS_HW;
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

#ifdef Z7_SFX
  #define STEP_PRE 1
  #define STEP_MAIN 1
#else
  #define STEP_PRE 2
  #define STEP_MAIN 4
  // #define Z7_SHA256_UNROLL
#endif

#undef Z7_SHA256_BIG_W
#if STEP_MAIN != 16
  #define Z7_SHA256_BIG_W
#endif




void Sha256_InitState(CSha256 *p)
{
  p->v.vars.count = 0;
  p->state[0] = 0x6a09e667;
  p->state[1] = 0xbb67ae85;
  p->state[2] = 0x3c6ef372;
  p->state[3] = 0xa54ff53a;
  p->state[4] = 0x510e527f;
  p->state[5] = 0x9b05688c;
  p->state[6] = 0x1f83d9ab;
  p->state[7] = 0x5be0cd19;
}








void Sha256_Init(CSha256 *p)
{
  p->v.vars.func_UpdateBlocks =
  #ifdef Z7_COMPILER_SHA256_SUPPORTED
      g_SHA256_FUNC_UPDATE_BLOCKS;
  #else
      NULL;
  #endif
  Sha256_InitState(p);
}

#define S0(x) (rotrFixed(x, 2) ^ rotrFixed(x,13) ^ rotrFixed(x,22))
#define S1(x) (rotrFixed(x, 6) ^ rotrFixed(x,11) ^ rotrFixed(x,25))
#define s0(x) (rotrFixed(x, 7) ^ rotrFixed(x,18) ^ (x >> 3))
#define s1(x) (rotrFixed(x,17) ^ rotrFixed(x,19) ^ (x >>10))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))


#define W_PRE(i) (W[(i) + (size_t)(j)] = GetBe32(data + ((size_t)(j) + i) * 4))

#define blk2_main(j, i)  s1(w(j, (i)-2)) + w(j, (i)-7) + s0(w(j, (i)-15))

#ifdef Z7_SHA256_BIG_W
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

#if (!defined(Z7_SHA256_UNROLL) || STEP_MAIN < 8) && (STEP_MAIN >= 4)
#define R2_MAIN(i) \
    R1_MAIN(i) \
    R1_MAIN(i + 1) \

#endif



#if defined(Z7_SHA256_UNROLL) && STEP_MAIN >= 8

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
MY_ALIGN(64) const UInt32 SHA256_K_ARRAY[64];
MY_ALIGN(64) const UInt32 SHA256_K_ARRAY[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};





#define K SHA256_K_ARRAY

Z7_NO_INLINE
void Z7_FASTCALL Sha256_UpdateBlocks(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  UInt32 W
#ifdef Z7_SHA256_BIG_W
      [64];
#else
      [16];
#endif
  unsigned j;
  UInt32 a,b,c,d,e,f,g,h;
#if !defined(Z7_SHA256_UNROLL) || (STEP_MAIN <= 4) || (STEP_PRE <= 4)
  UInt32 tmp;
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

  for (j = 16; j < 64; j += STEP_MAIN)
  {
    #if defined(Z7_SHA256_UNROLL) && STEP_MAIN >= 8

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

  data += SHA256_BLOCK_SIZE;
  }
  while (--numBlocks);
}


#define Sha256_UpdateBlock(p) SHA256_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Sha256_Update(CSha256 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->v.vars.count & (SHA256_BLOCK_SIZE - 1);
    const unsigned num = SHA256_BLOCK_SIZE - pos;
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
      Sha256_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 6;
    // if (numBlocks)
    SHA256_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= SHA256_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}


void Sha256_Final(CSha256 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->v.vars.count & (SHA256_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (SHA256_BLOCK_SIZE - 4 * 2))
  {
    while (pos != SHA256_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, SHA256_BLOCK_SIZE - pos);
    Sha256_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (SHA256_BLOCK_SIZE - 4 * 2) - pos);
  {
    const UInt64 numBits = p->v.vars.count << 3;
    SetBe32(p->buffer + SHA256_BLOCK_SIZE - 4 * 2, (UInt32)(numBits >> 32))
    SetBe32(p->buffer + SHA256_BLOCK_SIZE - 4 * 1, (UInt32)(numBits))
  }
  Sha256_UpdateBlock(p);
#if 1 && defined(MY_CPU_BE)
  memcpy(digest, p->state, SHA256_DIGEST_SIZE);
#else
  {
    unsigned i;
    for (i = 0; i < 8; i += 2)
    {
      const UInt32 v0 = p->state[i];
      const UInt32 v1 = p->state[(size_t)i + 1];
      SetBe32(digest    , v0)
      SetBe32(digest + 4, v1)
      digest += 4 * 2;
    }
  }




#endif
  Sha256_InitState(p);
}


void Sha256Prepare(void)
{
#ifdef Z7_COMPILER_SHA256_SUPPORTED
  SHA256_FUNC_UPDATE_BLOCKS f, f_hw;
  f = Sha256_UpdateBlocks;
  f_hw = NULL;
#ifdef MY_CPU_X86_OR_AMD64
  if (CPU_IsSupported_SHA()
      && CPU_IsSupported_SSSE3()
      )
#else
  if (CPU_IsSupported_SHA2())
#endif
  {
    // printf("\n========== HW SHA256 ======== \n");
    f = f_hw = Sha256_UpdateBlocks_HW;
  }
  g_SHA256_FUNC_UPDATE_BLOCKS    = f;
  g_SHA256_FUNC_UPDATE_BLOCKS_HW = f_hw;
#endif
}

#undef U64C
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
#undef Z7_SHA256_BIG_W
#undef Z7_SHA256_UNROLL
#undef Z7_COMPILER_SHA256_SUPPORTED
