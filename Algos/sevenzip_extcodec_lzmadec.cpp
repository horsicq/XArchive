/* XArchive amalgamation of public-domain LZMA SDK 26.01 decoder LzmaDec.
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
 * The retained SDK units are public domain and carry their notices below.
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

/* ================ unit bodies ================ */

/* ================ unit: C/LzmaDec.c ================ */
/* LzmaDec.c -- LZMA Decoder
2023-04-07 : Igor Pavlov : Public domain */

// amalgamation: header emitted in prologue

#include <string.h>

/* #include "CpuArch.h" */
// amalgamation: header emitted in prologue

// #define kNumTopBits 24
#define kTopValue ((UInt32)1 << 24)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)

#define RC_INIT_SIZE 5

#ifndef Z7_LZMA_DEC_OPT

#define kNumMoveBits 5
#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * (UInt32)ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) \
  { UPDATE_0(p)  i = (i + i); A0; } else \
  { UPDATE_1(p)  i = (i + i) + 1; A1; }

#define TREE_GET_BIT(probs, i) { GET_BIT2(probs + i, i, ;, ;); }

#define REV_BIT(p, i, A0, A1) IF_BIT_0(p + i) \
  { UPDATE_0(p + i)  A0; } else \
  { UPDATE_1(p + i)  A1; }
#define REV_BIT_VAR(  p, i, m) REV_BIT(p, i, i += m; m += m, m += m; i += m; )
#define REV_BIT_CONST(p, i, m) REV_BIT(p, i, i += m;       , i += m * 2; )
#define REV_BIT_LAST( p, i, m) REV_BIT(p, i, i -= m        , ; )

#define TREE_DECODE(probs, limit, i) \
  { i = 1; do { TREE_GET_BIT(probs, i); } while (i < limit); i -= limit; }

/* #define Z7_LZMA_SIZE_OPT */

#ifdef Z7_LZMA_SIZE_OPT
#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
#define TREE_6_DECODE(probs, i) \
  { i = 1; \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  i -= 0x40; }
#endif

#define NORMAL_LITER_DEC TREE_GET_BIT(prob, symbol)
#define MATCHED_LITER_DEC \
  matchByte += matchByte; \
  bit = offs; \
  offs &= matchByte; \
  probLit = prob + (offs + bit + symbol); \
  GET_BIT2(probLit, symbol, offs ^= bit; , ;)

#endif // Z7_LZMA_DEC_OPT


#define NORMALIZE_CHECK if (range < kTopValue) { if (buf >= bufLimit) return DUMMY_INPUT_EOF; range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0_CHECK(p) ttt = *(p); NORMALIZE_CHECK bound = (range >> kNumBitModelTotalBits) * (UInt32)ttt; if (code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1) IF_BIT_0_CHECK(p) \
  { UPDATE_0_CHECK  i = (i + i); A0; } else \
  { UPDATE_1_CHECK  i = (i + i) + 1; A1; }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i, ; , ;)
#define TREE_DECODE_CHECK(probs, limit, i) \
  { i = 1; do { GET_BIT_CHECK(probs + i, i) } while (i < limit); i -= limit; }


#define REV_BIT_CHECK(p, i, m) IF_BIT_0_CHECK(p + i) \
  { UPDATE_0_CHECK  i += m; m += m; } else \
  { UPDATE_1_CHECK  m += m; i += m; }


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenLow 0
#define LenHigh (LenLow + 2 * (kNumPosStatesMax << kLenNumLowBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)

#define LenChoice LenLow
#define LenChoice2 (LenLow + (1 << kLenNumLowBits))

#define kNumStates 12
#define kNumStates2 16
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols * 2 + kLenNumHighSymbols)

#define kMatchSpecLen_Error_Data (1 << 9)
#define kMatchSpecLen_Error_Fail (kMatchSpecLen_Error_Data - 1)

/* External ASM code needs same CLzmaProb array layout. So don't change it. */

/* (probs_1664) is faster and better for code size at some platforms */
/*
#ifdef MY_CPU_X86_OR_AMD64
*/
#define kStartOffset 1664
#define GET_PROBS p->probs_1664
/*
#define GET_PROBS p->probs + kStartOffset
#else
#define kStartOffset 0
#define GET_PROBS p->probs
#endif
*/

#define SpecPos (-kStartOffset)
#define IsRep0Long (SpecPos + kNumFullDistances)
#define RepLenCoder (IsRep0Long + (kNumStates2 << kNumPosBitsMax))
#define LenCoder (RepLenCoder + kNumLenProbs)
#define IsMatch (LenCoder + kNumLenProbs)
#define Align (IsMatch + (kNumStates2 << kNumPosBitsMax))
#define IsRep (Align + kAlignTableSize)
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define PosSlot (IsRepG2 + kNumStates)
#define Literal (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define NUM_BASE_PROBS (Literal + kStartOffset)

#if Align != 0 && kStartOffset != 0
  #error Stop_Compiling_Bad_LZMA_kAlign
#endif

#if NUM_BASE_PROBS != 1984
  #error Stop_Compiling_Bad_LZMA_PROBS
#endif


#define LZMA_LIT_SIZE 0x300

#define LzmaProps_GetNumProbs(p) (NUM_BASE_PROBS + ((UInt32)LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))


#define CALC_POS_STATE(processedPos, pbMask) (((processedPos) & (pbMask)) << 4)
#define COMBINED_PS_STATE (posState + state)
#define GET_LEN_STATE (posState)

#define LZMA_DIC_MIN (1 << 12)

/*
p->remainLen : shows status of LZMA decoder:
    < kMatchSpecLenStart  : the number of bytes to be copied with (p->rep0) offset
    = kMatchSpecLenStart  : the LZMA stream was finished with end mark
    = kMatchSpecLenStart + 1  : need init range coder
    = kMatchSpecLenStart + 2  : need init range coder and state
    = kMatchSpecLen_Error_Fail                : Internal Code Failure
    = kMatchSpecLen_Error_Data + [0 ... 273]  : LZMA Data Error
*/

/* ---------- LZMA_DECODE_REAL ---------- */
/*
LzmaDec_DecodeReal_3() can be implemented in external ASM file.
3 - is the code compatibility version of that function for check at link time.
*/

#define LZMA_DECODE_REAL LzmaDec_DecodeReal_3

/*
LZMA_DECODE_REAL()
In:
  RangeCoder is normalized
  if (p->dicPos == limit)
  {
    LzmaDec_TryDummy() was called before to exclude LITERAL and MATCH-REP cases.
    So first symbol can be only MATCH-NON-REP. And if that MATCH-NON-REP symbol
    is not END_OF_PAYALOAD_MARKER, then the function doesn't write any byte to dictionary,
    the function returns SZ_OK, and the caller can use (p->remainLen) and (p->reps[0]) later.
  }

Processing:
  The first LZMA symbol will be decoded in any case.
  All main checks for limits are at the end of main loop,
  It decodes additional LZMA-symbols while (p->buf < bufLimit && dicPos < limit),
  RangeCoder is still without last normalization when (p->buf < bufLimit) is being checked.
  But if (p->buf < bufLimit), the caller provided at least (LZMA_REQUIRED_INPUT_MAX + 1) bytes for
  next iteration  before limit (bufLimit + LZMA_REQUIRED_INPUT_MAX),
  that is enough for worst case LZMA symbol with one additional RangeCoder normalization for one bit.
  So that function never reads bufLimit [LZMA_REQUIRED_INPUT_MAX] byte.

Out:
  RangeCoder is normalized
  Result:
    SZ_OK - OK
      p->remainLen:
        < kMatchSpecLenStart : the number of bytes to be copied with (p->reps[0]) offset
        = kMatchSpecLenStart : the LZMA stream was finished with end mark

    SZ_ERROR_DATA - error, when the MATCH-Symbol refers out of dictionary
      p->remainLen : undefined
      p->reps[*]    : undefined
*/


#ifdef Z7_LZMA_DEC_OPT

int Z7_FASTCALL LZMA_DECODE_REAL(CLzmaDec *p, SizeT limit, const Byte *bufLimit);

#else

static
int Z7_FASTCALL LZMA_DECODE_REAL(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
  CLzmaProb *probs = GET_PROBS;
  unsigned state = (unsigned)p->state;
  UInt32 rep0 = p->reps[0], rep1 = p->reps[1], rep2 = p->reps[2], rep3 = p->reps[3];
  unsigned pbMask = ((unsigned)1 << (p->prop.pb)) - 1;
  unsigned lc = p->prop.lc;
  unsigned lpMask = ((unsigned)0x100 << p->prop.lp) - ((unsigned)0x100 >> lc);

  Byte *dic = p->dic;
  SizeT dicBufSize = p->dicBufSize;
  SizeT dicPos = p->dicPos;
  
  UInt32 processedPos = p->processedPos;
  UInt32 checkDicSize = p->checkDicSize;
  unsigned len = 0;

  const Byte *buf = p->buf;
  UInt32 range = p->range;
  UInt32 code = p->code;

  do
  {
    CLzmaProb *prob;
    UInt32 bound;
    unsigned ttt;
    unsigned posState = CALC_POS_STATE(processedPos, pbMask);

    prob = probs + IsMatch + COMBINED_PS_STATE;
    IF_BIT_0(prob)
    {
      unsigned symbol;
      UPDATE_0(prob)
      prob = probs + Literal;
      if (processedPos != 0 || checkDicSize != 0)
        prob += (UInt32)3 * ((((processedPos << 8) + dic[(dicPos == 0 ? dicBufSize : dicPos) - 1]) & lpMask) << lc);
      processedPos++;

      if (state < kNumLitStates)
      {
        state -= (state < 4) ? state : 3;
        symbol = 1;
        #ifdef Z7_LZMA_SIZE_OPT
        do { NORMAL_LITER_DEC } while (symbol < 0x100);
        #else
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        #endif
      }
      else
      {
        unsigned matchByte = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
        unsigned offs = 0x100;
        state -= (state < 10) ? 3 : 6;
        symbol = 1;
        #ifdef Z7_LZMA_SIZE_OPT
        do
        {
          unsigned bit;
          CLzmaProb *probLit;
          MATCHED_LITER_DEC
        }
        while (symbol < 0x100);
        #else
        {
          unsigned bit;
          CLzmaProb *probLit;
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
        }
        #endif
      }

      dic[dicPos++] = (Byte)symbol;
      continue;
    }
    
    {
      UPDATE_1(prob)
      prob = probs + IsRep + state;
      IF_BIT_0(prob)
      {
        UPDATE_0(prob)
        state += kNumStates;
        prob = probs + LenCoder;
      }
      else
      {
        UPDATE_1(prob)
        prob = probs + IsRepG0 + state;
        IF_BIT_0(prob)
        {
          UPDATE_0(prob)
          prob = probs + IsRep0Long + COMBINED_PS_STATE;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob)
  
            // that case was checked before with kBadRepCode
            // if (checkDicSize == 0 && processedPos == 0) { len = kMatchSpecLen_Error_Data + 1; break; }
            // The caller doesn't allow (dicPos == limit) case here
            // so we don't need the following check:
            // if (dicPos == limit) { state = state < kNumLitStates ? 9 : 11; len = 1; break; }
            
            dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
            dicPos++;
            processedPos++;
            state = state < kNumLitStates ? 9 : 11;
            continue;
          }
          UPDATE_1(prob)
        }
        else
        {
          UInt32 distance;
          UPDATE_1(prob)
          prob = probs + IsRepG1 + state;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob)
            distance = rep1;
          }
          else
          {
            UPDATE_1(prob)
            prob = probs + IsRepG2 + state;
            IF_BIT_0(prob)
            {
              UPDATE_0(prob)
              distance = rep2;
            }
            else
            {
              UPDATE_1(prob)
              distance = rep3;
              rep3 = rep2;
            }
            rep2 = rep1;
          }
          rep1 = rep0;
          rep0 = distance;
        }
        state = state < kNumLitStates ? 8 : 11;
        prob = probs + RepLenCoder;
      }
      
      #ifdef Z7_LZMA_SIZE_OPT
      {
        unsigned lim, offset;
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0(probLen)
        {
          UPDATE_0(probLen)
          probLen = prob + LenLow + GET_LEN_STATE;
          offset = 0;
          lim = (1 << kLenNumLowBits);
        }
        else
        {
          UPDATE_1(probLen)
          probLen = prob + LenChoice2;
          IF_BIT_0(probLen)
          {
            UPDATE_0(probLen)
            probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
            offset = kLenNumLowSymbols;
            lim = (1 << kLenNumLowBits);
          }
          else
          {
            UPDATE_1(probLen)
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols * 2;
            lim = (1 << kLenNumHighBits);
          }
        }
        TREE_DECODE(probLen, lim, len)
        len += offset;
      }
      #else
      {
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0(probLen)
        {
          UPDATE_0(probLen)
          probLen = prob + LenLow + GET_LEN_STATE;
          len = 1;
          TREE_GET_BIT(probLen, len)
          TREE_GET_BIT(probLen, len)
          TREE_GET_BIT(probLen, len)
          len -= 8;
        }
        else
        {
          UPDATE_1(probLen)
          probLen = prob + LenChoice2;
          IF_BIT_0(probLen)
          {
            UPDATE_0(probLen)
            probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
            len = 1;
            TREE_GET_BIT(probLen, len)
            TREE_GET_BIT(probLen, len)
            TREE_GET_BIT(probLen, len)
          }
          else
          {
            UPDATE_1(probLen)
            probLen = prob + LenHigh;
            TREE_DECODE(probLen, (1 << kLenNumHighBits), len)
            len += kLenNumLowSymbols * 2;
          }
        }
      }
      #endif

      if (state >= kNumStates)
      {
        UInt32 distance;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
        TREE_6_DECODE(prob, distance)
        if (distance >= kStartPosModelIndex)
        {
          unsigned posSlot = (unsigned)distance;
          unsigned numDirectBits = (unsigned)(((distance >> 1) - 1));
          distance = (2 | (distance & 1));
          if (posSlot < kEndPosModelIndex)
          {
            distance <<= numDirectBits;
            prob = probs + SpecPos;
            {
              UInt32 m = 1;
              distance++;
              do
              {
                REV_BIT_VAR(prob, distance, m)
              }
              while (--numDirectBits);
              distance -= m;
            }
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE
              range >>= 1;
              
              {
                UInt32 t;
                code -= range;
                t = (0 - ((UInt32)code >> 31)); /* (UInt32)((Int32)code >> 31) */
                distance = (distance << 1) + (t + 1);
                code += range & t;
              }
              /*
              distance <<= 1;
              if (code >= range)
              {
                code -= range;
                distance |= 1;
              }
              */
            }
            while (--numDirectBits);
            prob = probs + Align;
            distance <<= kNumAlignBits;
            {
              unsigned i = 1;
              REV_BIT_CONST(prob, i, 1)
              REV_BIT_CONST(prob, i, 2)
              REV_BIT_CONST(prob, i, 4)
              REV_BIT_LAST (prob, i, 8)
              distance |= i;
            }
            if (distance == (UInt32)0xFFFFFFFF)
            {
              len = kMatchSpecLenStart;
              state -= kNumStates;
              break;
            }
          }
        }
        
        rep3 = rep2;
        rep2 = rep1;
        rep1 = rep0;
        rep0 = distance + 1;
        state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
        if (distance >= (checkDicSize == 0 ? processedPos: checkDicSize))
        {
          len += kMatchSpecLen_Error_Data + kMatchMinLen;
          // len = kMatchSpecLen_Error_Data;
          // len += kMatchMinLen;
          break;
        }
      }

      len += kMatchMinLen;

      {
        SizeT rem;
        unsigned curLen;
        SizeT pos;
        
        if ((rem = limit - dicPos) == 0)
        {
          /*
          We stop decoding and return SZ_OK, and we can resume decoding later.
          Any error conditions can be tested later in caller code.
          For more strict mode we can stop decoding with error
          // len += kMatchSpecLen_Error_Data;
          */
          break;
        }
        
        curLen = ((rem < len) ? (unsigned)rem : len);
        pos = dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0);

        processedPos += (UInt32)curLen;

        len -= curLen;
        if (curLen <= dicBufSize - pos)
        {
          Byte *dest = dic + dicPos;
          ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
          const Byte *lim = dest + curLen;
          dicPos += (SizeT)curLen;
          do
            *(dest) = (Byte)*(dest + src);
          while (++dest != lim);
        }
        else
        {
          do
          {
            dic[dicPos++] = dic[pos];
            if (++pos == dicBufSize)
              pos = 0;
          }
          while (--curLen != 0);
        }
      }
    }
  }
  while (dicPos < limit && buf < bufLimit);

  NORMALIZE
  
  p->buf = buf;
  p->range = range;
  p->code = code;
  p->remainLen = (UInt32)len; // & (kMatchSpecLen_Error_Data - 1); // we can write real length for error matches too.
  p->dicPos = dicPos;
  p->processedPos = processedPos;
  p->reps[0] = rep0;
  p->reps[1] = rep1;
  p->reps[2] = rep2;
  p->reps[3] = rep3;
  p->state = (UInt32)state;
  if (len >= kMatchSpecLen_Error_Data)
    return SZ_ERROR_DATA;
  return SZ_OK;
}
#endif



static void Z7_FASTCALL LzmaDec_WriteRem(CLzmaDec *p, SizeT limit)
{
  unsigned len = (unsigned)p->remainLen;
  if (len == 0 /* || len >= kMatchSpecLenStart */)
    return;
  {
    SizeT dicPos = p->dicPos;
    Byte *dic;
    SizeT dicBufSize;
    SizeT rep0;   /* we use SizeT to avoid the BUG of VC14 for AMD64 */
    {
      SizeT rem = limit - dicPos;
      if (rem < len)
      {
        len = (unsigned)(rem);
        if (len == 0)
          return;
      }
    }

    if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= len)
      p->checkDicSize = p->prop.dicSize;

    p->processedPos += (UInt32)len;
    p->remainLen -= (UInt32)len;
    dic = p->dic;
    rep0 = p->reps[0];
    dicBufSize = p->dicBufSize;
    do
    {
      dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
      dicPos++;
    }
    while (--len);
    p->dicPos = dicPos;
  }
}


/*
At staring of new stream we have one of the following symbols:
  - Literal        - is allowed
  - Non-Rep-Match  - is allowed only if it's end marker symbol
  - Rep-Match      - is not allowed
We use early check of (RangeCoder:Code) over kBadRepCode to simplify main decoding code
*/

#define kRange0 0xFFFFFFFF
#define kBound0 ((kRange0 >> kNumBitModelTotalBits) << (kNumBitModelTotalBits - 1))
#define kBadRepCode (kBound0 + (((kRange0 - kBound0) >> kNumBitModelTotalBits) << (kNumBitModelTotalBits - 1)))
#if kBadRepCode != (0xC0000000 - 0x400)
  #error Stop_Compiling_Bad_LZMA_Check
#endif


/*
LzmaDec_DecodeReal2():
  It calls LZMA_DECODE_REAL() and it adjusts limit according (p->checkDicSize).

We correct (p->checkDicSize) after LZMA_DECODE_REAL() and in LzmaDec_WriteRem(),
and we support the following state of (p->checkDicSize):
  if (total_processed < p->prop.dicSize) then
  {
    (total_processed == p->processedPos)
    (p->checkDicSize == 0)
  }
  else
    (p->checkDicSize == p->prop.dicSize)
*/

static int Z7_FASTCALL LzmaDec_DecodeReal2(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
  if (p->checkDicSize == 0)
  {
    UInt32 rem = p->prop.dicSize - p->processedPos;
    if (limit - p->dicPos > rem)
      limit = p->dicPos + rem;
  }
  {
    int res = LZMA_DECODE_REAL(p, limit, bufLimit);
    if (p->checkDicSize == 0 && p->processedPos >= p->prop.dicSize)
      p->checkDicSize = p->prop.dicSize;
    return res;
  }
}



typedef enum
{
  DUMMY_INPUT_EOF, /* need more input data */
  DUMMY_LIT,
  DUMMY_MATCH,
  DUMMY_REP
} ELzmaDummy;


#define IS_DUMMY_END_MARKER_POSSIBLE(dummyRes) ((dummyRes) == DUMMY_MATCH)

static ELzmaDummy LzmaDec_TryDummy(const CLzmaDec *p, const Byte *buf, const Byte **bufOut)
{
  UInt32 range = p->range;
  UInt32 code = p->code;
  const Byte *bufLimit = *bufOut;
  const CLzmaProb *probs = GET_PROBS;
  unsigned state = (unsigned)p->state;
  ELzmaDummy res;

  for (;;)
  {
    const CLzmaProb *prob;
    UInt32 bound;
    unsigned ttt;
    unsigned posState = CALC_POS_STATE(p->processedPos, ((unsigned)1 << p->prop.pb) - 1);

    prob = probs + IsMatch + COMBINED_PS_STATE;
    IF_BIT_0_CHECK(prob)
    {
      UPDATE_0_CHECK

      prob = probs + Literal;
      if (p->checkDicSize != 0 || p->processedPos != 0)
        prob += ((UInt32)LZMA_LIT_SIZE *
            ((((p->processedPos) & (((unsigned)1 << (p->prop.lp)) - 1)) << p->prop.lc) +
            ((unsigned)p->dic[(p->dicPos == 0 ? p->dicBufSize : p->dicPos) - 1] >> (8 - p->prop.lc))));

      if (state < kNumLitStates)
      {
        unsigned symbol = 1;
        do { GET_BIT_CHECK(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        unsigned matchByte = p->dic[p->dicPos - p->reps[0] +
            (p->dicPos < p->reps[0] ? p->dicBufSize : 0)];
        unsigned offs = 0x100;
        unsigned symbol = 1;
        do
        {
          unsigned bit;
          const CLzmaProb *probLit;
          matchByte += matchByte;
          bit = offs;
          offs &= matchByte;
          probLit = prob + (offs + bit + symbol);
          GET_BIT2_CHECK(probLit, symbol, offs ^= bit; , ; )
        }
        while (symbol < 0x100);
      }
      res = DUMMY_LIT;
    }
    else
    {
      unsigned len;
      UPDATE_1_CHECK

      prob = probs + IsRep + state;
      IF_BIT_0_CHECK(prob)
      {
        UPDATE_0_CHECK
        state = 0;
        prob = probs + LenCoder;
        res = DUMMY_MATCH;
      }
      else
      {
        UPDATE_1_CHECK
        res = DUMMY_REP;
        prob = probs + IsRepG0 + state;
        IF_BIT_0_CHECK(prob)
        {
          UPDATE_0_CHECK
          prob = probs + IsRep0Long + COMBINED_PS_STATE;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK
            break;
          }
          else
          {
            UPDATE_1_CHECK
          }
        }
        else
        {
          UPDATE_1_CHECK
          prob = probs + IsRepG1 + state;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK
          }
          else
          {
            UPDATE_1_CHECK
            prob = probs + IsRepG2 + state;
            IF_BIT_0_CHECK(prob)
            {
              UPDATE_0_CHECK
            }
            else
            {
              UPDATE_1_CHECK
            }
          }
        }
        state = kNumStates;
        prob = probs + RepLenCoder;
      }
      {
        unsigned limit, offset;
        const CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0_CHECK(probLen)
        {
          UPDATE_0_CHECK
          probLen = prob + LenLow + GET_LEN_STATE;
          offset = 0;
          limit = 1 << kLenNumLowBits;
        }
        else
        {
          UPDATE_1_CHECK
          probLen = prob + LenChoice2;
          IF_BIT_0_CHECK(probLen)
          {
            UPDATE_0_CHECK
            probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
            offset = kLenNumLowSymbols;
            limit = 1 << kLenNumLowBits;
          }
          else
          {
            UPDATE_1_CHECK
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols * 2;
            limit = 1 << kLenNumHighBits;
          }
        }
        TREE_DECODE_CHECK(probLen, limit, len)
        len += offset;
      }

      if (state < 4)
      {
        unsigned posSlot;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates - 1 ? len : kNumLenToPosStates - 1) <<
            kNumPosSlotBits);
        TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot)
        if (posSlot >= kStartPosModelIndex)
        {
          unsigned numDirectBits = ((posSlot >> 1) - 1);

          if (posSlot < kEndPosModelIndex)
          {
            prob = probs + SpecPos + ((2 | (posSlot & 1)) << numDirectBits);
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE_CHECK
              range >>= 1;
              code -= range & (((code - range) >> 31) - 1);
              /* if (code >= range) code -= range; */
            }
            while (--numDirectBits);
            prob = probs + Align;
            numDirectBits = kNumAlignBits;
          }
          {
            unsigned i = 1;
            unsigned m = 1;
            do
            {
              REV_BIT_CHECK(prob, i, m)
            }
            while (--numDirectBits);
          }
        }
      }
    }
    break;
  }
  NORMALIZE_CHECK

  *bufOut = buf;
  return res;
}

void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState);
void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState)
{
  p->remainLen = kMatchSpecLenStart + 1;
  p->tempBufSize = 0;

  if (initDic)
  {
    p->processedPos = 0;
    p->checkDicSize = 0;
    p->remainLen = kMatchSpecLenStart + 2;
  }
  if (initState)
    p->remainLen = kMatchSpecLenStart + 2;
}

void LzmaDec_Init(CLzmaDec *p)
{
  p->dicPos = 0;
  LzmaDec_InitDicAndState(p, True, True);
}


/*
LZMA supports optional end_marker.
So the decoder can lookahead for one additional LZMA-Symbol to check end_marker.
That additional LZMA-Symbol can require up to LZMA_REQUIRED_INPUT_MAX bytes in input stream.
When the decoder reaches dicLimit, it looks (finishMode) parameter:
  if (finishMode == LZMA_FINISH_ANY), the decoder doesn't lookahead
  if (finishMode != LZMA_FINISH_ANY), the decoder lookahead, if end_marker is possible for current position

When the decoder lookahead, and the lookahead symbol is not end_marker, we have two ways:
  1) Strict mode (default) : the decoder returns SZ_ERROR_DATA.
  2) The relaxed mode (alternative mode) : we could return SZ_OK, and the caller
     must check (status) value. The caller can show the error,
     if the end of stream is expected, and the (status) is noit
     LZMA_STATUS_FINISHED_WITH_MARK or LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK.
*/


#define RETURN_NOT_FINISHED_FOR_FINISH \
  *status = LZMA_STATUS_NOT_FINISHED; \
  return SZ_ERROR_DATA; // for strict mode
  // return SZ_OK; // for relaxed mode


SRes LzmaDec_DecodeToDic(CLzmaDec *p, SizeT dicLimit, const Byte *src, SizeT *srcLen,
    ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT inSize = *srcLen;
  (*srcLen) = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;

  if (p->remainLen > kMatchSpecLenStart)
  {
    if (p->remainLen > kMatchSpecLenStart + 2)
      return p->remainLen == kMatchSpecLen_Error_Fail ? SZ_ERROR_FAIL : SZ_ERROR_DATA;

    for (; inSize > 0 && p->tempBufSize < RC_INIT_SIZE; (*srcLen)++, inSize--)
      p->tempBuf[p->tempBufSize++] = *src++;
    if (p->tempBufSize != 0 && p->tempBuf[0] != 0)
      return SZ_ERROR_DATA;
    if (p->tempBufSize < RC_INIT_SIZE)
    {
      *status = LZMA_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }
    p->code =
        ((UInt32)p->tempBuf[1] << 24)
      | ((UInt32)p->tempBuf[2] << 16)
      | ((UInt32)p->tempBuf[3] << 8)
      | ((UInt32)p->tempBuf[4]);

    if (p->checkDicSize == 0
        && p->processedPos == 0
        && p->code >= kBadRepCode)
      return SZ_ERROR_DATA;

    p->range = 0xFFFFFFFF;
    p->tempBufSize = 0;

    if (p->remainLen > kMatchSpecLenStart + 1)
    {
      SizeT numProbs = LzmaProps_GetNumProbs(&p->prop);
      SizeT i;
      CLzmaProb *probs = p->probs;
      for (i = 0; i < numProbs; i++)
        probs[i] = kBitModelTotal >> 1;
      p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
      p->state = 0;
    }

    p->remainLen = 0;
  }

  for (;;)
  {
    if (p->remainLen == kMatchSpecLenStart)
    {
      if (p->code != 0)
        return SZ_ERROR_DATA;
      *status = LZMA_STATUS_FINISHED_WITH_MARK;
      return SZ_OK;
    }

    LzmaDec_WriteRem(p, dicLimit);

    {
      // (p->remainLen == 0 || p->dicPos == dicLimit)

      int checkEndMarkNow = 0;

      if (p->dicPos >= dicLimit)
      {
        if (p->remainLen == 0 && p->code == 0)
        {
          *status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
          return SZ_OK;
        }
        if (finishMode == LZMA_FINISH_ANY)
        {
          *status = LZMA_STATUS_NOT_FINISHED;
          return SZ_OK;
        }
        if (p->remainLen != 0)
        {
          RETURN_NOT_FINISHED_FOR_FINISH
        }
        checkEndMarkNow = 1;
      }

      // (p->remainLen == 0)

      if (p->tempBufSize == 0)
      {
        const Byte *bufLimit;
        int dummyProcessed = -1;
        
        if (inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          const Byte *bufOut = src + inSize;
          
          ELzmaDummy dummyRes = LzmaDec_TryDummy(p, src, &bufOut);
          
          if (dummyRes == DUMMY_INPUT_EOF)
          {
            size_t i;
            if (inSize >= LZMA_REQUIRED_INPUT_MAX)
              break;
            (*srcLen) += inSize;
            p->tempBufSize = (unsigned)inSize;
            for (i = 0; i < inSize; i++)
              p->tempBuf[i] = src[i];
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
 
          dummyProcessed = (int)(bufOut - src);
          if ((unsigned)dummyProcessed > LZMA_REQUIRED_INPUT_MAX)
            break;
          
          if (checkEndMarkNow && !IS_DUMMY_END_MARKER_POSSIBLE(dummyRes))
          {
            unsigned i;
            (*srcLen) += (unsigned)dummyProcessed;
            p->tempBufSize = (unsigned)dummyProcessed;
            for (i = 0; i < (unsigned)dummyProcessed; i++)
              p->tempBuf[i] = src[i];
            // p->remainLen = kMatchSpecLen_Error_Data;
            RETURN_NOT_FINISHED_FOR_FINISH
          }
          
          bufLimit = src;
          // we will decode only one iteration
        }
        else
          bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;

        p->buf = src;
        
        {
          int res = LzmaDec_DecodeReal2(p, dicLimit, bufLimit);
          
          SizeT processed = (SizeT)(p->buf - src);

          if (dummyProcessed < 0)
          {
            if (processed > inSize)
              break;
          }
          else if ((unsigned)dummyProcessed != processed)
            break;

          src += processed;
          inSize -= processed;
          (*srcLen) += processed;

          if (res != SZ_OK)
          {
            p->remainLen = kMatchSpecLen_Error_Data;
            return SZ_ERROR_DATA;
          }
        }
        continue;
      }

      {
        // we have some data in (p->tempBuf)
        // in strict mode: tempBufSize is not enough for one Symbol decoding.
        // in relaxed mode: tempBufSize not larger than required for one Symbol decoding.

        unsigned rem = p->tempBufSize;
        unsigned ahead = 0;
        int dummyProcessed = -1;
        
        while (rem < LZMA_REQUIRED_INPUT_MAX && ahead < inSize)
          p->tempBuf[rem++] = src[ahead++];
        
        // ahead - the size of new data copied from (src) to (p->tempBuf)
        // rem   - the size of temp buffer including new data from (src)
        
        if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          const Byte *bufOut = p->tempBuf + rem;
        
          ELzmaDummy dummyRes = LzmaDec_TryDummy(p, p->tempBuf, &bufOut);
          
          if (dummyRes == DUMMY_INPUT_EOF)
          {
            if (rem >= LZMA_REQUIRED_INPUT_MAX)
              break;
            p->tempBufSize = rem;
            (*srcLen) += (SizeT)ahead;
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
          
          dummyProcessed = (int)(bufOut - p->tempBuf);

          if ((unsigned)dummyProcessed < p->tempBufSize)
            break;

          if (checkEndMarkNow && !IS_DUMMY_END_MARKER_POSSIBLE(dummyRes))
          {
            (*srcLen) += (unsigned)dummyProcessed - p->tempBufSize;
            p->tempBufSize = (unsigned)dummyProcessed;
            // p->remainLen = kMatchSpecLen_Error_Data;
            RETURN_NOT_FINISHED_FOR_FINISH
          }
        }

        p->buf = p->tempBuf;
        
        {
          // we decode one symbol from (p->tempBuf) here, so the (bufLimit) is equal to (p->buf)
          int res = LzmaDec_DecodeReal2(p, dicLimit, p->buf);

          SizeT processed = (SizeT)(p->buf - p->tempBuf);
          rem = p->tempBufSize;
          
          if (dummyProcessed < 0)
          {
            if (processed > LZMA_REQUIRED_INPUT_MAX)
              break;
            if (processed < rem)
              break;
          }
          else if ((unsigned)dummyProcessed != processed)
            break;
          
          processed -= rem;

          src += processed;
          inSize -= processed;
          (*srcLen) += processed;
          p->tempBufSize = 0;
          
          if (res != SZ_OK)
          {
            p->remainLen = kMatchSpecLen_Error_Data;
            return SZ_ERROR_DATA;
          }
        }
      }
    }
  }

  /*  Some unexpected error: internal error of code, memory corruption or hardware failure */
  p->remainLen = kMatchSpecLen_Error_Fail;
  return SZ_ERROR_FAIL;
}



SRes LzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT outSize = *destLen;
  SizeT inSize = *srcLen;
  *srcLen = *destLen = 0;
  for (;;)
  {
    SizeT inSizeCur = inSize, outSizeCur, dicPos;
    ELzmaFinishMode curFinishMode;
    SRes res;
    if (p->dicPos == p->dicBufSize)
      p->dicPos = 0;
    dicPos = p->dicPos;
    if (outSize > p->dicBufSize - dicPos)
    {
      outSizeCur = p->dicBufSize;
      curFinishMode = LZMA_FINISH_ANY;
    }
    else
    {
      outSizeCur = dicPos + outSize;
      curFinishMode = finishMode;
    }

    res = LzmaDec_DecodeToDic(p, outSizeCur, src, &inSizeCur, curFinishMode, status);
    src += inSizeCur;
    inSize -= inSizeCur;
    *srcLen += inSizeCur;
    outSizeCur = p->dicPos - dicPos;
    memcpy(dest, p->dic + dicPos, outSizeCur);
    dest += outSizeCur;
    outSize -= outSizeCur;
    *destLen += outSizeCur;
    if (res != 0)
      return res;
    if (outSizeCur == 0 || outSize == 0)
      return SZ_OK;
  }
}

void LzmaDec_FreeProbs(CLzmaDec *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->probs);
  p->probs = NULL;
}

static void LzmaDec_FreeDict(CLzmaDec *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->dic);
  p->dic = NULL;
}

void LzmaDec_Free(CLzmaDec *p, ISzAllocPtr alloc)
{
  LzmaDec_FreeProbs(p, alloc);
  LzmaDec_FreeDict(p, alloc);
}

SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size)
{
  UInt32 dicSize;
  Byte d;
  
  if (size < LZMA_PROPS_SIZE)
    return SZ_ERROR_UNSUPPORTED;
  else
    dicSize = data[1] | ((UInt32)data[2] << 8) | ((UInt32)data[3] << 16) | ((UInt32)data[4] << 24);
 
  if (dicSize < LZMA_DIC_MIN)
    dicSize = LZMA_DIC_MIN;
  p->dicSize = dicSize;

  d = data[0];
  if (d >= (9 * 5 * 5))
    return SZ_ERROR_UNSUPPORTED;

  p->lc = (Byte)(d % 9);
  d /= 9;
  p->pb = (Byte)(d / 5);
  p->lp = (Byte)(d % 5);

  return SZ_OK;
}

static SRes LzmaDec_AllocateProbs2(CLzmaDec *p, const CLzmaProps *propNew, ISzAllocPtr alloc)
{
  UInt32 numProbs = LzmaProps_GetNumProbs(propNew);
  if (!p->probs || numProbs != p->numProbs)
  {
    LzmaDec_FreeProbs(p, alloc);
    p->probs = (CLzmaProb *)ISzAlloc_Alloc(alloc, numProbs * sizeof(CLzmaProb));
    if (!p->probs)
      return SZ_ERROR_MEM;
    p->probs_1664 = p->probs + 1664;
    p->numProbs = numProbs;
  }
  return SZ_OK;
}

SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc)
{
  CLzmaProps propNew;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize))
  RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc))
  p->prop = propNew;
  return SZ_OK;
}

SRes LzmaDec_Allocate(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc)
{
  CLzmaProps propNew;
  SizeT dicBufSize;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize))
  RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc))

  {
    UInt32 dictSize = propNew.dicSize;
    SizeT mask = ((UInt32)1 << 12) - 1;
         if (dictSize >= ((UInt32)1 << 30)) mask = ((UInt32)1 << 22) - 1;
    else if (dictSize >= ((UInt32)1 << 22)) mask = ((UInt32)1 << 20) - 1;
    dicBufSize = ((SizeT)dictSize + mask) & ~mask;
    if (dicBufSize < dictSize)
      dicBufSize = dictSize;
  }

  if (!p->dic || dicBufSize != p->dicBufSize)
  {
    LzmaDec_FreeDict(p, alloc);
    p->dic = (Byte *)ISzAlloc_Alloc(alloc, dicBufSize);
    if (!p->dic)
    {
      LzmaDec_FreeProbs(p, alloc);
      return SZ_ERROR_MEM;
    }
  }
  p->dicBufSize = dicBufSize;
  p->prop = propNew;
  return SZ_OK;
}

SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode,
    ELzmaStatus *status, ISzAllocPtr alloc)
{
  CLzmaDec p;
  SRes res;
  SizeT outSize = *destLen, inSize = *srcLen;
  *destLen = *srcLen = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;
  if (inSize < RC_INIT_SIZE)
    return SZ_ERROR_INPUT_EOF;
  LzmaDec_CONSTRUCT(&p)
  RINOK(LzmaDec_AllocateProbs(&p, propData, propSize, alloc))
  p.dic = dest;
  p.dicBufSize = outSize;
  LzmaDec_Init(&p);
  *srcLen = inSize;
  res = LzmaDec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
  *destLen = p.dicPos;
  if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
    res = SZ_ERROR_INPUT_EOF;
  LzmaDec_FreeProbs(&p, alloc);
  return res;
}
