/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Archive/tar reader.
 *
 * 7 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/Common/NewHandler.h ---- */
// Common/NewHandler.h

#ifndef ZIP7_INC_COMMON_NEW_HANDLER_H
#define ZIP7_INC_COMMON_NEW_HANDLER_H

/*
NewHandler.h and NewHandler.cpp allows to solve problem with compilers that
don't throw exception in operator new().

This file must be included before any code that uses operators new() or delete()
and you must compile and link "NewHandler.cpp", if you use some old MSVC compiler.

DOCs:
  Since ISO C++98, operator new throws std::bad_alloc when memory allocation fails.
  MSVC 6.0 returned a null pointer on an allocation failure.
  Beginning in VS2002, operator new conforms to the standard and throws on failure.

  By default, the compiler also generates defensive null checks to prevent
  these older-style allocators from causing an immediate crash on failure.
  The /Zc:throwingNew option tells the compiler to leave out these null checks,
  on the assumption that all linked memory allocators conform to the standard.

The operator new() in some MSVC versions doesn't throw exception std::bad_alloc.
MSVC 6.0 (_MSC_VER == 1200) doesn't throw exception.
The code produced by some another MSVC compilers also can be linked
to library that doesn't throw exception.
We suppose that code compiled with VS2015+ (_MSC_VER >= 1900) throws exception std::bad_alloc.
For older _MSC_VER versions we redefine operator new() and operator delete().
Our version of operator new() throws CNewException() exception on failure.

It's still allowed to use redefined version of operator new() from "NewHandler.cpp"
with any compiler. 7-Zip's code can work with std::bad_alloc and CNewException() exceptions.
But if you use some additional code (outside of 7-Zip's code), you must check
that redefined version of operator new() is not problem for your code.
*/

#include <stddef.h>

#ifdef _WIN32
// We can compile my_new and my_delete with _fastcall
/*
void * my_new(size_t size);
void my_delete(void *p) throw();
// void * my_Realloc(void *p, size_t newSize, size_t oldSize);
*/
#endif


#if defined(_MSC_VER) && (_MSC_VER < 1600)
  // If you want to use default operator new(), you can disable the following line
  #define Z7_REDEFINE_OPERATOR_NEW
#endif


#ifdef Z7_REDEFINE_OPERATOR_NEW

// std::bad_alloc can require additional DLL dependency.
// So we don't define CNewException as std::bad_alloc here.

class CNewException {};

void *
#ifdef _MSC_VER
__cdecl
#endif
operator new(size_t size);

/*
#if 0 && defined(_MSC_VER) && _MSC_VER == 1600
  #define Z7_OPERATOR_DELETE_SPEC_THROW0
#else
  #define Z7_OPERATOR_DELETE_SPEC_THROW0 throw()
#endif
*/
#if defined(_MSC_VER) && _MSC_VER == 1600
#pragma warning(push)
#pragma warning(disable : 4986) // 'operator delete': exception specification does not match previous declaration
#endif

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p) throw();

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p, size_t n) throw();

#if defined(_MSC_VER) && _MSC_VER == 1600
#pragma warning(pop)
#endif


#else

#include <new>

#define CNewException std::bad_alloc

#endif

/*
#ifdef _WIN32
void *
#ifdef _MSC_VER
__cdecl
#endif
operator new[](size_t size);

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete[](void *p) throw();
#endif
*/

#endif

/* ---- CPP/Common/Common0.h ---- */
// Common0.h

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif

#ifndef ZIP7_INC_COMMON0_H
#define ZIP7_INC_COMMON0_H

// amalgamation: header emitted in prologue

/*
This file contains compiler related things for cpp files.
This file is included to all cpp files in 7-Zip via "Common.h".
Also this file is included in "IDecl.h" (that is included in interface files).
So external modules can use 7-Zip interfaces without
predefined macros defined in "Common.h".
*/

#ifdef _MSC_VER
  #pragma warning(disable : 4710) // function not inlined
  // 'CUncopyable::CUncopyable':
  #pragma warning(disable : 4514) // unreferenced inline function has been removed
  #if _MSC_VER < 1300
    #pragma warning(disable : 4702) // unreachable code
    #pragma warning(disable : 4714) // function marked as __forceinline not inlined
    #pragma warning(disable : 4786) // identifier was truncated to '255' characters in the debug information
  #endif
  #if _MSC_VER < 1400
    #pragma warning(disable : 4511) // copy constructor could not be generated    // #pragma warning(disable : 4512) // assignment operator could not be generated
    #pragma warning(disable : 4512) // assignment operator could not be generated
  #endif
  #if _MSC_VER > 1400 && _MSC_VER <= 1900
    // #pragma warning(disable : 4996)
       // strcat: This function or variable may be unsafe
       // GetVersion was declared deprecated
  #endif

#if _MSC_VER > 1200
// -Wall warnings

#if _MSC_VER <= 1600
#pragma warning(disable : 4917) // 'OLE_HANDLE' : a GUID can only be associated with a class, interface or namespace
#endif

// #pragma warning(disable : 4061) // enumerator '' in switch of enum '' is not explicitly handled by a case label
// #pragma warning(disable : 4266) // no override available for virtual member function from base ''; function is hidden
#pragma warning(disable : 4625) // copy constructor was implicitly defined as deleted
#pragma warning(disable : 4626) // assignment operator was implicitly defined as deleted
#if _MSC_VER >= 1600 && _MSC_VER < 1920
#pragma warning(disable : 4571) // Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
#endif
#if _MSC_VER >= 1600
#pragma warning(disable : 4365) // 'initializing' : conversion from 'int' to 'unsigned int', signed / unsigned mismatch
#endif
#if _MSC_VER < 1800
// we disable the warning, if we don't use 'final' in class
#pragma warning(disable : 4265) // class has virtual functions, but destructor is not virtual
#endif

#if _MSC_VER >= 1900
#pragma warning(disable : 5026) // move constructor was implicitly defined as deleted
#pragma warning(disable : 5027) // move assignment operator was implicitly defined as deleted
#endif
#if _MSC_VER >= 1912
#pragma warning(disable : 5039) // pointer or reference to potentially throwing function passed to 'extern "C"' function under - EHc.Undefined behavior may occur if this function throws an exception.
#endif
#if _MSC_VER >= 1925
// #pragma warning(disable : 5204) // 'ISequentialInStream' : class has virtual functions, but its trivial destructor is not virtual; instances of objects derived from this class may not be destructed correctly
#endif
#if _MSC_VER >= 1934
// #pragma warning(disable : 5264) // const variable is not used
#endif

#endif // _MSC_VER > 1200
#endif // _MSC_VER


#if defined(_MSC_VER) // && !defined(__clang__)
#define Z7_DECLSPEC_NOTHROW   __declspec(nothrow)
#elif defined(__clang__) || defined(__GNUC__)
#define Z7_DECLSPEC_NOTHROW   __attribute__((nothrow))
#else
#define Z7_DECLSPEC_NOTHROW
#endif

/*
#if defined (_MSC_VER) && _MSC_VER >= 1900 \
    || defined(__clang__) && __clang_major__ >= 6 \
    || defined(__GNUC__) && __GNUC__ >= 6
  #define Z7_noexcept noexcept
#else
  #define Z7_noexcept throw()
#endif
*/


#if defined(__clang__)

#if /* defined(_WIN32) && */ __clang_major__ >= 16
#pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#endif

#if __clang_major__ >= 4 && __clang_major__ < 12 && !defined(_WIN32)
/*
if compiled with new GCC libstdc++, GCC libstdc++ can use:
13.2.0/include/c++/
    <new> : #define _NEW
    <stdlib.h> : #define _GLIBCXX_STDLIB_H 1
*/
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#endif

// noexcept, final, = delete
#pragma GCC diagnostic ignored "-Wc++98-compat"
#if __clang_major__ >= 4
// throw() dynamic exception specifications are deprecated
#pragma GCC diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

#if __clang_major__ <= 6 // check it
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"

#if defined(Z7_LLVM_CLANG_VERSION) && __clang_major__ >= 18 /* 18.1.0RC */ \
  || defined(Z7_APPLE_CLANG_VERSION) && __clang_major__ >= 16 // for APPLE=17 (LLVM=19)
  #pragma GCC diagnostic ignored "-Wswitch-default"
#endif
// #pragma GCC diagnostic ignored "-Wunused-private-field"
// #pragma GCC diagnostic ignored "-Wnonportable-system-include-path"
// #pragma GCC diagnostic ignored "-Wsuggest-override"
// #pragma GCC diagnostic ignored "-Wsign-conversion"
// #pragma GCC diagnostic ignored "-Winconsistent-missing-override"
// #pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
// #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
// #pragma GCC diagnostic ignored "-Wdeprecated-copy-with-user-provided-dtor"
// #pragma GCC diagnostic ignored "-Wdeprecated-copy-dtor"
// #ifndef _WIN32
// #pragma GCC diagnostic ignored "-Wweak-vtables"
// #endif
/*
#if   defined(Z7_GCC_VERSION)   && (Z7_GCC_VERSION   >= 40400) \
   || defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30000)
// enumeration values not explicitly handled in switch
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif
*/
#endif // __clang__


#ifdef __GNUC__
// #pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif


/* There is BUG in MSVC 6.0 compiler for operator new[]:
   It doesn't check overflow, when it calculates size in bytes for allocated array.
   So we can use Z7_ARRAY_NEW macro instead of new[] operator. */

#if defined(_MSC_VER) && (_MSC_VER == 1200) && !defined(_WIN64)
  #define Z7_ARRAY_NEW(p, T, size)  p = new T[((size) > 0xFFFFFFFFu / sizeof(T)) ? 0xFFFFFFFFu / sizeof(T) : (size)];
#else
  #define Z7_ARRAY_NEW(p, T, size)  p = new T[size];
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 8))
  #define Z7_ATTR_NORETURN  __attribute__((noreturn))
#elif (defined(__clang__) && (__clang_major__ >= 3))
  #if __has_feature(cxx_attributes)
    #define Z7_ATTR_NORETURN  [[noreturn]]
  #else
    #define Z7_ATTR_NORETURN  __attribute__((noreturn))
  #endif
#elif (defined(_MSC_VER) && (_MSC_VER >= 1900))
  #define Z7_ATTR_NORETURN  [[noreturn]]
#else
  #define Z7_ATTR_NORETURN
#endif


// final in "GCC 4.7.0"
// In C++98 and C++03 code the alternative spelling __final can be used instead (this is a GCC extension.)

#if defined (__cplusplus) && __cplusplus >= 201103L \
    || defined(_MSC_VER) && _MSC_VER >= 1800 \
    || defined(__clang__) && __clang_major__ >= 4 \
    /* || defined(__GNUC__) && __GNUC__ >= 9 */
  #define Z7_final  final
  #if defined(__clang__) && __cplusplus < 201103L
    #pragma GCC diagnostic ignored "-Wc++11-extensions"
  #endif
#elif defined (__cplusplus) && __cplusplus >= 199711L \
    && defined(__GNUC__) && __GNUC__ >= 4 && !defined(__clang__)
  #define Z7_final __final
#else
  #define Z7_final
  #if defined(__clang__) && __clang_major__ >= 4 \
     || defined(__GNUC__) && __GNUC__ >= 4
    #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
    #pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
  #endif
#endif

#define Z7_class_final(c)  class c Z7_final


#if defined (__cplusplus) && __cplusplus >= 201103L \
    || (defined(_MSC_VER) && _MSC_VER >= 1800)
  #define Z7_CPP_IS_SUPPORTED_default
  #define Z7_eq_delete  = delete
  // #define Z7_DECL_DEFAULT_COPY_CONSTRUCTOR_IF_SUPPORTED(c) c(const c& k) = default;
#else
  #define Z7_eq_delete
  // #define Z7_DECL_DEFAULT_COPY_CONSTRUCTOR_IF_SUPPORTED(c)
#endif


#if defined(__cplusplus) && (__cplusplus >= 201103L) \
    || defined(_MSC_VER) && (_MSC_VER >= 1400) /* && (_MSC_VER != 1600) */ \
    || defined(__clang__) && __clang_major__ >= 4
  #if defined(_MSC_VER) && (_MSC_VER == 1600) /* && (_MSC_VER != 1600) */
    #pragma warning(disable : 4481) // nonstandard extension used: override specifier 'override'
    #define Z7_DESTRUCTOR_override
  #else
    #define Z7_DESTRUCTOR_override  override
  #endif
  #define Z7_override  override
#else
  #define Z7_override
  #define Z7_DESTRUCTOR_override
#endif



#define Z7_CLASS_NO_COPY(cls) \
  private: \
  cls(const cls &) Z7_eq_delete; \
  cls &operator=(const cls &) Z7_eq_delete;

class CUncopyable
{
protected:
  CUncopyable() {} // allow constructor
  // ~CUncopyable() {}
  Z7_CLASS_NO_COPY(CUncopyable)
};

#define MY_UNCOPYABLE  :private CUncopyable
// #define MY_UNCOPYABLE


// typedef void (*Z7_void_Function)(void);

#if defined(__clang__) || defined(__GNUC__)
#define Z7_CAST_FUNC(t, e) reinterpret_cast<t>(reinterpret_cast<Z7_void_Function>(e))
#else
#define Z7_CAST_FUNC(t, e) reinterpret_cast<t>(reinterpret_cast<void*>(e))
// #define Z7_CAST_FUNC(t, e) reinterpret_cast<t>(e)
#endif

#define Z7_GET_PROC_ADDRESS(func_type, hmodule, func_name)  \
    Z7_CAST_FUNC(func_type, GetProcAddress(hmodule, func_name))

// || defined(__clang__)
// || defined(__GNUC__)

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define Z7_DECLSPEC_NOVTABLE __declspec(novtable)
#else
#define Z7_DECLSPEC_NOVTABLE
#endif

#ifdef __clang__
#define Z7_PURE_INTERFACES_BEGIN \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wnon-virtual-dtor\"")
_Pragma("GCC diagnostic ignored \"-Wweak-vtables\"")
#define Z7_PURE_INTERFACES_END \
_Pragma("GCC diagnostic pop")
#else
#define Z7_PURE_INTERFACES_BEGIN
#define Z7_PURE_INTERFACES_END
#endif

// NewHandler.h and NewHandler.cpp redefine operator new() to throw exceptions, if compiled with old MSVC compilers
// amalgamation: header emitted in prologue

/*
// #define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)  Z7_ARRAY_SIZE(a)
#endif
*/

#endif // ZIP7_INC_COMMON0_H



// #define Z7_REDEFINE_NULL

#if defined(Z7_REDEFINE_NULL) /* && (!defined(__clang__) || defined(_MSC_VER)) */

// NULL is defined in <stddef.h>
#include <stddef.h>
#undef NULL

#ifdef __cplusplus
  #if defined (__cplusplus) && __cplusplus >= 201103L \
    || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #define NULL  nullptr
  #else
    #define NULL  0
  #endif
#else
  #define NULL  ((void *)0)
#endif

#else // Z7_REDEFINE_NULL

#if defined(__clang__) && __clang_major__ >= 5
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#endif // Z7_REDEFINE_NULL

// for precompiler:
// #include "MyWindows.h"

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

/* ---- CPP/Common/MyTypes.h ---- */
// Common/MyTypes.h

#ifndef ZIP7_INC_COMMON_MY_TYPES_H
#define ZIP7_INC_COMMON_MY_TYPES_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// typedef int HRes;
// typedef HRESULT HRes;

struct CBoolPair
{
  bool Val;
  bool Def;

  CBoolPair(): Val(false), Def(false) {}
  
  void Init()
  {
    Val = false;
    Def = false;
  }

  void SetTrueTrue()
  {
    Val = true;
    Def = true;
  }

  void SetVal_as_Defined(bool val)
  {
    Val = val;
    Def = true;
  }
};

#endif

/* ---- CPP/Common/MyGuidDef.h ---- */
// Common/MyGuidDef.h

// #pragma message "Common/MyGuidDef.h"

#ifndef GUID_DEFINED
#define GUID_DEFINED

// #pragma message "GUID_DEFINED"

// amalgamation: header emitted in prologue

typedef struct {
  UInt32 Data1;
  UInt16 Data2;
  UInt16 Data3;
  Byte Data4[8];
} GUID;

#ifdef __cplusplus
#define REFGUID const GUID &
#else
#define REFGUID const GUID *
#endif

// typedef GUID IID;
typedef GUID CLSID;

#define REFCLSID REFGUID
#define REFIID REFGUID

#ifdef __cplusplus
inline int operator==(REFGUID g1, REFGUID g2)
{
  for (unsigned i = 0; i < sizeof(g1); i++)
    if (((const Byte *)&g1)[i] != ((const Byte *)&g2)[i])
      return 0;
  return 1;
}
inline int operator!=(REFGUID g1, REFGUID g2) { return !(g1 == g2); }
#endif

#endif // GUID_DEFINED

#ifndef EXTERN_C
#ifdef __cplusplus
  #define EXTERN_C extern "C"
#else
  #define EXTERN_C extern
#endif
#endif

#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif

#ifdef INITGUID
  #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name; \
    EXTERN_C const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#else
  #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name
#endif

/* ---- CPP/Common/MyWindows.h ---- */
// MyWindows.h

#ifdef Z7_DEFINE_GUID
#undef Z7_DEFINE_GUID
#endif

#ifdef INITGUID
  #define Z7_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name; \
    EXTERN_C const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#else
  #define Z7_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name
#endif


#ifndef ZIP7_INC_MY_WINDOWS_H
#define ZIP7_INC_MY_WINDOWS_H

#ifdef _WIN32

// amalgamation: header emitted in prologue

#else // _WIN32

#include <stddef.h> // for wchar_t
#include <string.h>
// #include <stdint.h> // for uintptr_t

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// WINAPI is __stdcall in Windows-MSVC in windef.h
#define WINAPI

typedef char CHAR;
typedef unsigned char UCHAR;

#undef BYTE
typedef unsigned char BYTE;

typedef short SHORT;
typedef unsigned short USHORT;

#undef WORD
typedef unsigned short WORD;
typedef short VARIANT_BOOL;

#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l) ((WORD)((DWORD_PTR)(l) >> 16))

// MS uses long for BOOL, but long is 32-bit in MS. So we use int.
// typedef long BOOL;
typedef int BOOL;

#ifndef FALSE
  #define FALSE 0
  #define TRUE 1
#endif

// typedef size_t ULONG_PTR;
// typedef size_t DWORD_PTR;
// typedef uintptr_t UINT_PTR;
// typedef ptrdiff_t UINT_PTR;

typedef Int64 LONGLONG;
typedef UInt64 ULONGLONG;

typedef struct { LONGLONG QuadPart; } LARGE_INTEGER;
typedef struct { ULONGLONG QuadPart; } ULARGE_INTEGER;

typedef const CHAR *LPCSTR;
typedef CHAR TCHAR;
typedef const TCHAR *LPCTSTR;
typedef wchar_t WCHAR;
typedef WCHAR OLECHAR;
typedef const WCHAR *LPCWSTR;
typedef OLECHAR *BSTR;
typedef const OLECHAR *LPCOLESTR;
typedef OLECHAR *LPOLESTR;

typedef struct
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;

#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr)    ((HRESULT)(hr) < 0)
typedef ULONG PROPID;
typedef LONG SCODE;


#define S_OK    ((HRESULT)0x00000000L)
#define S_FALSE ((HRESULT)0x00000001L)
#define E_NOTIMPL     ((HRESULT)0x80004001L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_ABORT       ((HRESULT)0x80004004L)
#define E_FAIL        ((HRESULT)0x80004005L)
#define STG_E_INVALIDFUNCTION     ((HRESULT)0x80030001L)
#define CLASS_E_CLASSNOTAVAILABLE ((HRESULT)0x80040111L)


#ifdef _MSC_VER
#define STDMETHODCALLTYPE __stdcall
#define STDAPICALLTYPE    __stdcall
#else
// do we need __export here?
#define STDMETHODCALLTYPE
#define STDAPICALLTYPE
#endif

#define STDAPI  EXTERN_C HRESULT STDAPICALLTYPE

#ifndef DECLSPEC_NOTHROW
#define DECLSPEC_NOTHROW    Z7_DECLSPEC_NOTHROW
#endif

#ifndef DECLSPEC_NOVTABLE
#define DECLSPEC_NOVTABLE   Z7_DECLSPEC_NOVTABLE
#endif

#ifndef COM_DECLSPEC_NOTHROW
#ifdef COM_STDMETHOD_CAN_THROW
  #define COM_DECLSPEC_NOTHROW
#else
  #define COM_DECLSPEC_NOTHROW  DECLSPEC_NOTHROW
#endif
#endif

#define DECLARE_INTERFACE(iface)              struct DECLSPEC_NOVTABLE iface
#define DECLARE_INTERFACE_(iface, baseiface)  struct DECLSPEC_NOVTABLE iface : public baseiface

#define STDMETHOD_(t, f)  virtual COM_DECLSPEC_NOTHROW t STDMETHODCALLTYPE f
#define STDMETHOD(f)      STDMETHOD_(HRESULT, f)
#define STDMETHODIMP_(t)  COM_DECLSPEC_NOTHROW t STDMETHODCALLTYPE
#define STDMETHODIMP      STDMETHODIMP_(HRESULT)


#define PURE = 0

// #define MIDL_INTERFACE(x) struct


#ifdef __cplusplus

/*
  p7zip and 7-Zip before v23 used virtual destructor in IUnknown,
  if _WIN32 is not defined.
  It used virtual destructor, because some compilers don't like virtual
  interfaces without virtual destructor.
  IUnknown in Windows (_WIN32) doesn't use virtual destructor in IUnknown.
  We still can define Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN here,
  if we want to be compatible with old plugin interface of p7zip and 7-Zip before v23.

v23:
  In new 7-Zip v23 we try to be more compatible with original IUnknown from _WIN32.
  So we do not define Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN here,
*/
// #define Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN

#ifdef Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Winconsistent-missing-destructor-override"
#endif
#endif

Z7_PURE_INTERFACES_BEGIN

DEFINE_GUID(IID_IUnknown,
0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
struct IUnknown
{
  STDMETHOD(QueryInterface) (REFIID iid, void **outObject) =0;
  STDMETHOD_(ULONG, AddRef)() =0;
  STDMETHOD_(ULONG, Release)() =0;
 #ifdef Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN
  virtual ~IUnknown() {}
 #endif
};

typedef IUnknown *LPUNKNOWN;

Z7_PURE_INTERFACES_END

#endif // __cplusplus

#define VARIANT_TRUE ((VARIANT_BOOL)-1)
#define VARIANT_FALSE ((VARIANT_BOOL)0)

enum VARENUM
{
  VT_EMPTY = 0,
  VT_NULL = 1,
  VT_I2 = 2,
  VT_I4 = 3,
  VT_R4 = 4,
  VT_R8 = 5,
  VT_CY = 6,
  VT_DATE = 7,
  VT_BSTR = 8,
  VT_DISPATCH = 9,
  VT_ERROR = 10,
  VT_BOOL = 11,
  VT_VARIANT = 12,
  VT_UNKNOWN = 13,
  VT_DECIMAL = 14,

  VT_I1 = 16,
  VT_UI1 = 17,
  VT_UI2 = 18,
  VT_UI4 = 19,
  VT_I8 = 20,
  VT_UI8 = 21,
  VT_INT = 22,
  VT_UINT = 23,
  VT_VOID = 24,
  VT_HRESULT = 25,
  VT_FILETIME = 64
};

typedef unsigned short VARTYPE;
typedef WORD PROPVAR_PAD1;
typedef WORD PROPVAR_PAD2;
typedef WORD PROPVAR_PAD3;

typedef struct tagPROPVARIANT
{
  VARTYPE vt;
  PROPVAR_PAD1 wReserved1;
  PROPVAR_PAD2 wReserved2;
  PROPVAR_PAD3 wReserved3;
  union
  {
    CHAR cVal;
    UCHAR bVal;
    SHORT iVal;
    USHORT uiVal;
    LONG lVal;
    ULONG ulVal;
    INT intVal;
    UINT uintVal;
    LARGE_INTEGER hVal;
    ULARGE_INTEGER uhVal;
    VARIANT_BOOL boolVal;
    SCODE scode;
    FILETIME filetime;
    BSTR bstrVal;
  };
} PROPVARIANT;

typedef PROPVARIANT tagVARIANT;
typedef tagVARIANT VARIANT;
typedef VARIANT VARIANTARG;

EXTERN_C HRESULT VariantClear(VARIANTARG *prop);
EXTERN_C HRESULT VariantCopy(VARIANTARG *dest, const VARIANTARG *src);

typedef struct tagSTATPROPSTG
{
  LPOLESTR lpwstrName;
  PROPID propid;
  VARTYPE vt;
} STATPROPSTG;

EXTERN_C BSTR SysAllocStringByteLen(LPCSTR psz, UINT len);
EXTERN_C BSTR SysAllocStringLen(const OLECHAR *sz, UINT len);
EXTERN_C BSTR SysAllocString(const OLECHAR *sz);
EXTERN_C void SysFreeString(BSTR bstr);
EXTERN_C UINT SysStringByteLen(BSTR bstr);
EXTERN_C UINT SysStringLen(BSTR bstr);

EXTERN_C DWORD GetLastError();
EXTERN_C void SetLastError(DWORD dwCode);
EXTERN_C LONG CompareFileTime(const FILETIME* ft1, const FILETIME* ft2);

EXTERN_C DWORD GetCurrentThreadId();
EXTERN_C DWORD GetCurrentProcessId();

#define MAX_PATH 1024

#define CP_ACP    0
#define CP_OEMCP  1
#define CP_UTF8   65001

typedef enum tagSTREAM_SEEK
{
  STREAM_SEEK_SET = 0,
  STREAM_SEEK_CUR = 1,
  STREAM_SEEK_END = 2
} STREAM_SEEK;



typedef struct
{
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
} SYSTEMTIME;

BOOL WINAPI FileTimeToLocalFileTime(const FILETIME *fileTime, FILETIME *localFileTime);
BOOL WINAPI LocalFileTimeToFileTime(const FILETIME *localFileTime, FILETIME *fileTime);
BOOL WINAPI FileTimeToSystemTime(const FILETIME *fileTime, SYSTEMTIME *systemTime);
// VOID WINAPI GetSystemTimeAsFileTime(FILETIME *systemTimeAsFileTime);

DWORD GetTickCount();


/*
#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5
*/

#endif // _WIN32

#endif

/* ---- CPP/Common/Common.h ---- */
// Common.h

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif

#ifndef ZIP7_INC_COMMON_H
#define ZIP7_INC_COMMON_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

/*
This file is included to all cpp files in 7-Zip.
Each folder contains StdAfx.h file that includes "Common.h".
So 7-Zip includes "Common.h" in both modes:
  with precompiled StdAfx.h
and
  without precompiled StdAfx.h

include "Common.h" before other h files of 7-zip,
   if you need predefined macros.
do not include "Common.h", if you need only interfaces,
   and you don't need predefined macros.
*/

#endif

/* ---- CPP/7zip/Archive/Tar/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

#endif

/* ---- CPP/Common/ComTry.h ---- */
// ComTry.h

#ifndef ZIP7_INC_COM_TRY_H
#define ZIP7_INC_COM_TRY_H

// amalgamation: header emitted in prologue
// #include "Exception.h"
// #include "NewHandler.h"

#define COM_TRY_BEGIN try {
#define COM_TRY_END } catch(...) { return E_OUTOFMEMORY; }
  
/*
#define COM_TRY_END } \
  catch(const CNewException &) { return E_OUTOFMEMORY; } \
  catch(...) { return HRESULT_FROM_WIN32(ERROR_NOACCESS); } \
*/
  // catch(const CSystemException &e) { return e.ErrorCode; }
  // catch(...) { return E_FAIL; }

#endif

/* ---- CPP/Common/IntToString.h ---- */
// Common/IntToString.h

#ifndef ZIP7_INC_COMMON_INT_TO_STRING_H
#define ZIP7_INC_COMMON_INT_TO_STRING_H

// amalgamation: header emitted in prologue

// return: the pointer to the "terminating" null character after written characters

char * ConvertUInt32ToString(UInt32 value, char *s) throw();
char * ConvertUInt64ToString(UInt64 value, char *s) throw();

wchar_t * ConvertUInt32ToString(UInt32 value, wchar_t *s) throw();
wchar_t * ConvertUInt64ToString(UInt64 value, wchar_t *s) throw();
void ConvertInt64ToString(Int64 value, char *s) throw();
void ConvertInt64ToString(Int64 value, wchar_t *s) throw();

void ConvertUInt64ToOct(UInt64 value, char *s) throw();

extern const char k_Hex_Upper[16];
extern const char k_Hex_Lower[16];

#define GET_HEX_CHAR_UPPER(t)  (k_Hex_Upper[t])
#define GET_HEX_CHAR_LOWER(t)  (k_Hex_Lower[t])
/*
// #define GET_HEX_CHAR_UPPER(t) ((char)(((t < 10) ? ('0' + t) : ('A' + (t - 10)))))
static inline unsigned GetHex_Lower(unsigned v)
{
  const unsigned v0 = v + '0';
  v += 'a' - 10;
  if (v < 'a')
    v = v0;
  return v;
}
static inline char GetHex_Upper(unsigned v)
{
  return (char)((v < 10) ? ('0' + v) : ('A' + (v - 10)));
}
*/


void ConvertUInt32ToHex(UInt32 value, char *s) throw();
void ConvertUInt64ToHex(UInt64 value, char *s) throw();
void ConvertUInt32ToHex8Digits(UInt32 value, char *s) throw();
// void ConvertUInt32ToHex8Digits(UInt32 value, wchar_t *s) throw();

// use RawLeGuid only for RAW bytes that contain stored GUID as Little-endian.
char *RawLeGuidToString(const Byte *guid, char *s) throw();
char *RawLeGuidToString_Braced(const Byte *guid, char *s) throw();

void ConvertDataToHex_Lower(char *dest, const Byte *src, size_t size) throw();
void ConvertDataToHex_Upper(char *dest, const Byte *src, size_t size) throw();

#endif

/* ---- CPP/Common/MyVector.h ---- */
// Common/MyVector.h

#ifndef ZIP7_INC_COMMON_MY_VECTOR_H
#define ZIP7_INC_COMMON_MY_VECTOR_H

#include <string.h>

// amalgamation: header emitted in prologue

const unsigned k_VectorSizeMax = ((unsigned)1 << 31) - 1;

template <class T>
class CRecordVector
{
  T *_items;
  unsigned _size;
  unsigned _capacity;
  
  void MoveItems(unsigned destIndex, unsigned srcIndex)
  {
    memmove(_items + destIndex, _items + srcIndex, (size_t)(_size - srcIndex) * sizeof(T));
  }

  void ReAllocForNewCapacity(const unsigned newCapacity)
  {
    T *p;
    Z7_ARRAY_NEW(p, T, newCapacity)
    // p = new T[newCapacity];
    if (_size != 0)
      memcpy(p, _items, (size_t)_size * sizeof(T));
    delete []_items;
    _items = p;
    _capacity = newCapacity;
  }

public:

  void ReserveOnePosition()
  {
    if (_size != _capacity)
      return;
    if (_capacity >= k_VectorSizeMax)
      throw 2021;
    const unsigned rem = k_VectorSizeMax - _capacity;
    unsigned add = (_capacity >> 2) + 1;
    if (add > rem)
      add = rem;
    ReAllocForNewCapacity(_capacity + add);
  }

  CRecordVector(): _items(NULL), _size(0), _capacity(0) {}
  
  CRecordVector(const CRecordVector &v): _items(NULL), _size(0), _capacity(0)
  {
    const unsigned size = v.Size();
    if (size != 0)
    {
      // Z7_ARRAY_NEW(_items, T, size)
      _items = new T[size];
      _size = size;
      _capacity = size;
      memcpy(_items, v._items, (size_t)size * sizeof(T));
    }
  }
  
  unsigned Size() const { return _size; }
  bool IsEmpty() const { return _size == 0; }
  
  void ConstructReserve(unsigned size)
  {
    if (size != 0)
    {
      Z7_ARRAY_NEW(_items, T, size)
      // _items = new T[size];
      _capacity = size;
    }
  }

  void Reserve(unsigned newCapacity)
  {
    if (newCapacity > _capacity)
    {
      if (newCapacity > k_VectorSizeMax)
        throw 2021;
      ReAllocForNewCapacity(newCapacity);
    }
  }

  void ChangeSize_KeepData(unsigned newSize)
  {
    Reserve(newSize);
    _size = newSize;
  }

  void ClearAndReserve(unsigned newCapacity)
  {
    Clear();
    if (newCapacity > _capacity)
    {
      if (newCapacity > k_VectorSizeMax)
        throw 2021;
      delete []_items;
      _items = NULL;
      _capacity = 0;
      Z7_ARRAY_NEW(_items, T, newCapacity)
      // _items = new T[newCapacity];
      _capacity = newCapacity;
    }
  }

  void ClearAndSetSize(unsigned newSize)
  {
    ClearAndReserve(newSize);
    _size = newSize;
  }

  void ReserveDown()
  {
    if (_size == _capacity)
      return;
    T *p = NULL;
    if (_size != 0)
    {
      // Z7_ARRAY_NEW(p, T, _size)
      p = new T[_size];
      memcpy(p, _items, (size_t)_size * sizeof(T));
    }
    delete []_items;
    _items = p;
    _capacity = _size;
  }
  
  ~CRecordVector() { delete []_items; }
  
  void ClearAndFree()
  {
    delete []_items;
    _items = NULL;
    _size = 0;
    _capacity = 0;
  }
  
  void Clear() { _size = 0; }

  void DeleteBack() { _size--; }
  
  void DeleteFrom(unsigned index)
  {
    // if (index <= _size)
      _size = index;
  }
  
  void DeleteFrontal(unsigned num)
  {
    if (num != 0)
    {
      MoveItems(0, num);
      _size -= num;
    }
  }

  void Delete(unsigned index)
  {
    MoveItems(index, index + 1);
    _size -= 1;
  }

  /*
  void Delete(unsigned index, unsigned num)
  {
    if (num > 0)
    {
      MoveItems(index, index + num);
      _size -= num;
    }
  }
  */

  CRecordVector& operator=(const CRecordVector &v)
  {
    if (&v == this)
      return *this;
    const unsigned size = v.Size();
    if (size > _capacity)
    {
      delete []_items;
      _capacity = 0;
      _size = 0;
      _items = NULL;
      _items = new T[size];
      _capacity = size;
    }
    _size = size;
    if (size != 0)
      memcpy(_items, v._items, (size_t)size * sizeof(T));
    return *this;
  }

  CRecordVector& operator+=(const CRecordVector &v)
  {
    const unsigned size = v.Size();
    if (size != 0)
    {
      if (_size >= k_VectorSizeMax || size > k_VectorSizeMax - _size)
        throw 2021;
      const unsigned newSize = _size + size;
      Reserve(newSize);
      memcpy(_items + _size, v._items, (size_t)size * sizeof(T));
      _size = newSize;
    }
    return *this;
  }
  
  unsigned Add(const T item)
  {
    ReserveOnePosition();
    const unsigned size = _size;
    _size = size + 1;
    _items[size] = item;
    return size;
  }

  /*
  unsigned Add2(const T &item)
  {
    ReserveOnePosition();
    const unsigned size = _size;
    _size = size + 1;
    _items[size] = item;
    return size;
  }
  */

  unsigned AddInReserved(const T item)
  {
    const unsigned size = _size;
    _size = size + 1;
    _items[size] = item;
    return size;
  }

  void Insert(unsigned index, const T item)
  {
    ReserveOnePosition();
    MoveItems(index + 1, index);
    _items[index] = item;
    _size++;
  }

  void InsertInReserved(unsigned index, const T item)
  {
    MoveItems(index + 1, index);
    _items[index] = item;
    _size++;
  }

  void MoveToFront(unsigned index)
  {
    if (index != 0)
    {
      const T temp = _items[index];
      memmove(_items + 1, _items, (size_t)index * sizeof(T));
      _items[0] = temp;
    }
  }

  const T& operator[](unsigned index) const { return _items[index]; }
        T& operator[](unsigned index)       { return _items[index]; }
  const T& operator[](int index) const { return _items[(unsigned)index]; }
        T& operator[](int index)       { return _items[(unsigned)index]; }

  const T* ConstData()    const { return _items; }
        T* NonConstData() const { return _items; }
        T* NonConstData()       { return _items; }

  const T* Data() const         { return _items; }
        T* Data()               { return _items; }

  const T& FrontItem() const { return _items[0]; }
        T& FrontItem()       { return _items[0]; }
  /*
  const T Front() const { return _items[0]; }
        T Front()       { return _items[0]; }
  const T& Front() const { return _items[0]; }
        T& Front()       { return _items[0]; }
  */
  const T& Back() const  { return _items[(size_t)_size - 1]; }
        T& Back()        { return _items[(size_t)_size - 1]; }

  /*
  void Swap(unsigned i, unsigned j)
  {
    const T temp = _items[i];
    _items[i] = _items[j];
    _items[j] = temp;
  }
  */

  int FindInSorted(const T item, unsigned left, unsigned right) const
  {
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T midVal = (*this)[mid];
      if (item == midVal)
        return (int)mid;
      if (item < midVal)
        right = mid;
      else
        left = mid + 1;
    }
    return -1;
  }

  int FindInSorted2(const T &item, unsigned left, unsigned right) const
  {
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T& midVal = (*this)[mid];
      const int comp = item.Compare(midVal);
      if (comp == 0)
        return (int)mid;
      if (comp < 0)
        right = mid;
      else
        left = mid + 1;
    }
    return -1;
  }

  int FindInSorted(const T item) const
  {
    return FindInSorted(item, 0, _size);
  }

  int FindInSorted2(const T &item) const
  {
    return FindInSorted2(item, 0, _size);
  }

  unsigned AddToUniqueSorted(const T item)
  {
    unsigned left = 0, right = _size;
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T midVal = (*this)[mid];
      if (item == midVal)
        return mid;
      if (item < midVal)
        right = mid;
      else
        left = mid + 1;
    }
    Insert(right, item);
    return right;
  }

  unsigned AddToUniqueSorted2(const T &item)
  {
    unsigned left = 0, right = _size;
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T& midVal = (*this)[mid];
      const int comp = item.Compare(midVal);
      if (comp == 0)
        return mid;
      if (comp < 0)
        right = mid;
      else
        left = mid + 1;
    }
    Insert(right, item);
    return right;
  }

  static void SortRefDown(T* p, unsigned k, unsigned size, int (*compare)(const T*, const T*, void *), void *param)
  {
    const T temp = p[k];
    for (;;)
    {
      unsigned s = (k << 1);
      if (s > size)
        break;
      if (s < size && compare(p + s + 1, p + s, param) > 0)
        s++;
      if (compare(&temp, p + s, param) >= 0)
        break;
      p[k] = p[s];
      k = s;
    }
    p[k] = temp;
  }

  void Sort(int (*compare)(const T*, const T*, void *), void *param)
  {
    unsigned size = _size;
    if (size <= 1)
      return;
    T* p = _items - 1;
    {
      unsigned i = size >> 1;
      do
        SortRefDown(p, i, size, compare, param);
      while (--i);
    }
    do
    {
      const T temp = p[size];
      p[size--] = p[1];
      p[1] = temp;
      SortRefDown(p, 1, size, compare, param);
    }
    while (size > 1);
  }

  static void SortRefDown2(T* p, unsigned k, unsigned size)
  {
    const T temp = p[k];
    for (;;)
    {
      unsigned s = (k << 1);
      if (s > size)
        break;
      if (s < size && p[(size_t)s + 1].Compare(p[s]) > 0)
        s++;
      if (temp.Compare(p[s]) >= 0)
        break;
      p[k] = p[s];
      k = s;
    }
    p[k] = temp;
  }

  void Sort2()
  {
    unsigned size = _size;
    if (size <= 1)
      return;
    T* p = _items - 1;
    {
      unsigned i = size >> 1;
      do
        SortRefDown2(p, i, size);
      while (--i);
    }
    do
    {
      const T temp = p[size];
      p[size--] = p[1];
      p[1] = temp;
      SortRefDown2(p, 1, size);
    }
    while (size > 1);
  }
};

typedef CRecordVector<int> CIntVector;
typedef CRecordVector<unsigned int> CUIntVector;
typedef CRecordVector<bool> CBoolVector;
typedef CRecordVector<unsigned char> CByteVector;
typedef CRecordVector<void *> CPointerVector;

template <class T>
class CObjectVector
{
  CPointerVector _v;
public:
  unsigned Size() const { return _v.Size(); }
  bool IsEmpty() const { return _v.IsEmpty(); }
  void ReserveDown() { _v.ReserveDown(); }
  // void Reserve(unsigned newCapacity) { _v.Reserve(newCapacity); }
  void ClearAndReserve(unsigned newCapacity) { Clear(); _v.ClearAndReserve(newCapacity); }

  CObjectVector() {}
  CObjectVector(const CObjectVector &v)
  {
    const unsigned size = v.Size();
    _v.ConstructReserve(size);
    for (unsigned i = 0; i < size; i++)
      AddInReserved(v[i]);
  }
  CObjectVector& operator=(const CObjectVector &v)
  {
    if (&v == this)
      return *this;
    Clear();
    const unsigned size = v.Size();
    _v.Reserve(size);
    for (unsigned i = 0; i < size; i++)
      AddInReserved(v[i]);
    return *this;
  }

  CObjectVector& operator+=(const CObjectVector &v)
  {
    const unsigned addSize = v.Size();
    if (addSize != 0)
    {
      const unsigned size = Size();
      if (size >= k_VectorSizeMax || addSize > k_VectorSizeMax - size)
        throw 2021;
      _v.Reserve(size + addSize);
      for (unsigned i = 0; i < addSize; i++)
        AddInReserved(v[i]);
    }
    return *this;
  }
  
  const T& operator[](unsigned index) const { return *((T *)_v[index]); }
        T& operator[](unsigned index)       { return *((T *)_v[index]); }
  const T& operator[](int index) const { return *((T *)_v[(unsigned)index]); }
        T& operator[](int index)       { return *((T *)_v[(unsigned)index]); }
  const T& Front() const { return operator[](0); }
        T& Front()       { return operator[](0); }
  const T& Back() const  { return *(T *)_v.Back(); }
        T& Back()        { return *(T *)_v.Back(); }
  
  void MoveToFront(unsigned index) { _v.MoveToFront(index); }

  unsigned Add(const T& item)
  {
    _v.ReserveOnePosition();
    return AddInReserved(item);
  }
  
  unsigned AddInReserved(const T& item)
  {
    return _v.AddInReserved(new T(item));
  }

  void ReserveOnePosition()
  {
    _v.ReserveOnePosition();
  }

  unsigned AddInReserved_Ptr_of_new(T *ptr)
  {
    return _v.AddInReserved(ptr);
  }

  #define VECTOR_ADD_NEW_OBJECT(v, a) \
    (v).ReserveOnePosition(); \
    (v).AddInReserved_Ptr_of_new(new a);
  
  
  T& AddNew()
  {
    _v.ReserveOnePosition();
    T *p = new T;
    _v.AddInReserved(p);
    return *p;
  }
  
  T& AddNewInReserved()
  {
    T *p = new T;
    _v.AddInReserved(p);
    return *p;
  }
  
  void Insert(unsigned index, const T& item)
  {
    _v.ReserveOnePosition();
    _v.InsertInReserved(index, new T(item));
  }
  
  T& InsertNew(unsigned index)
  {
    _v.ReserveOnePosition();
    T *p = new T;
    _v.InsertInReserved(index, p);
    return *p;
  }

  ~CObjectVector()
  {
    for (unsigned i = _v.Size(); i != 0;)
      delete (T *)_v[--i];
  }
  
  void ClearAndFree()
  {
    Clear();
    _v.ClearAndFree();
  }
  
  void Clear()
  {
    for (unsigned i = _v.Size(); i != 0;)
      delete (T *)_v[--i];
    _v.Clear();
  }
  
  void DeleteFrom(unsigned index)
  {
    const unsigned size = _v.Size();
    for (unsigned i = index; i < size; i++)
      delete (T *)_v[i];
    _v.DeleteFrom(index);
  }

  void DeleteFrontal(unsigned num)
  {
    for (unsigned i = 0; i < num; i++)
      delete (T *)_v[i];
    _v.DeleteFrontal(num);
  }

  void DeleteBack()
  {
    delete (T *)_v.Back();
    _v.DeleteBack();
  }

  void Delete(unsigned index)
  {
    delete (T *)_v[index];
    _v.Delete(index);
  }
  // void Delete(int index) { Delete((unsigned)index); }

  /*
  void Delete(unsigned index, unsigned num)
  {
    for (unsigned i = 0; i < num; i++)
      delete (T *)_v[index + i];
    _v.Delete(index, num);
  }
  */

  /*
  int Find(const T& item) const
  {
    unsigned size = Size();
    for (unsigned i = 0; i < size; i++)
      if (item == (*this)[i])
        return i;
    return -1;
  }
  */
  
  int FindInSorted(const T& item) const
  {
    unsigned left = 0, right = Size();
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T& midVal = (*this)[mid];
      const int comp = item.Compare(midVal);
      if (comp == 0)
        return (int)mid;
      if (comp < 0)
        right = mid;
      else
        left = mid + 1;
    }
    return -1;
  }

  unsigned AddToUniqueSorted(const T& item)
  {
    unsigned left = 0, right = Size();
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T& midVal = (*this)[mid];
      const int comp = item.Compare(midVal);
      if (comp == 0)
        return mid;
      if (comp < 0)
        right = mid;
      else
        left = mid + 1;
    }
    Insert(right, item);
    return right;
  }

  /*
  unsigned AddToSorted(const T& item)
  {
    unsigned left = 0, right = Size();
    while (left != right)
    {
      // const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const unsigned mid = (left + right) / 2;
      const T& midVal = (*this)[mid];
      const int comp = item.Compare(midVal);
      if (comp == 0)
      {
        right = mid + 1;
        break;
      }
      if (comp < 0)
        right = mid;
      else
        left = mid + 1;
    }
    Insert(right, item);
    return right;
  }
  */

  void Sort(int (*compare)(void *const *, void *const *, void *), void *param)
    { _v.Sort(compare, param); }

  static int CompareObjectItems(void *const *a1, void *const *a2, void * /* param */)
    { return (*(*((const T *const *)a1))).Compare(*(*((const T *const *)a2))); }

  void Sort() { _v.Sort(CompareObjectItems, NULL); }
};

#define FOR_VECTOR(_i_, _v_) for (unsigned _i_ = 0; _i_ < (_v_).Size(); _i_++)

#endif

/* ---- CPP/Common/MyString.h ---- */
// Common/MyString.h

#ifndef ZIP7_INC_COMMON_MY_STRING_H
#define ZIP7_INC_COMMON_MY_STRING_H

#include <string.h>

#ifndef _WIN32
#include <wctype.h>
#include <wchar.h>
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue


/* if (DEBUG_FSTRING_INHERITS_ASTRING is defined), then
     FString inherits from AString, so we can find bugs related to FString at compile time.
   DON'T define DEBUG_FSTRING_INHERITS_ASTRING in release code */
   
// #define DEBUG_FSTRING_INHERITS_ASTRING

#ifdef DEBUG_FSTRING_INHERITS_ASTRING
class FString;
#endif


#ifdef _MSC_VER
  #ifdef _NATIVE_WCHAR_T_DEFINED
    #define MY_NATIVE_WCHAR_T_DEFINED
  #endif
#else
    #define MY_NATIVE_WCHAR_T_DEFINED
#endif

/*
  native support for wchar_t:
 _MSC_VER == 1600 : /Zc:wchar_t is not supported
 _MSC_VER == 1310 (VS2003)
 ? _MSC_VER == 1400 (VS2005) : wchar_t <- unsigned short
              /Zc:wchar_t  : wchar_t <- __wchar_t, _WCHAR_T_DEFINED and _NATIVE_WCHAR_T_DEFINED
 _MSC_VER > 1400 (VS2008+)
              /Zc:wchar_t[-]
              /Zc:wchar_t is on by default
*/

#ifdef _WIN32
#define IS_PATH_SEPAR(c) ((c) == '\\' || (c) == '/')
#else
#define IS_PATH_SEPAR(c) ((c) == CHAR_PATH_SEPARATOR)
#endif

inline bool IsPathSepar(char    c) { return IS_PATH_SEPAR(c); }
inline bool IsPathSepar(wchar_t c) { return IS_PATH_SEPAR(c); }

inline unsigned MyStringLen(const char *s)
{
  unsigned i;
  for (i = 0; s[i] != 0; i++);
  return i;
}

inline void MyStringCopy(char *dest, const char *src)
{
  while ((*dest++ = *src++) != 0);
}

inline char *MyStpCpy(char *dest, const char *src)
{
  for (;;)
  {
    const char c = *src;
    *dest = c;
    if (c == 0)
      return dest;
    src++;
    dest++;
  }
}

inline void MyStringCat(char *dest, const char *src)
{
  for (; *dest != 0; dest++);
  while ((*dest++ = *src++) != 0);
  // MyStringCopy(dest + MyStringLen(dest), src);
}

inline unsigned MyStringLen(const wchar_t *s)
{
  unsigned i;
  for (i = 0; s[i] != 0; i++);
  return i;
}

inline void MyStringCopy(wchar_t *dest, const wchar_t *src)
{
  while ((*dest++ = *src++) != 0);
}

inline void MyStringCat(wchar_t *dest, const wchar_t *src)
{
  for (; *dest != 0; dest++);
  while ((*dest++ = *src++) != 0);
  // MyStringCopy(dest + MyStringLen(dest), src);
}


/*
inline wchar_t *MyWcpCpy(wchar_t *dest, const wchar_t *src)
{
  for (;;)
  {
    const wchar_t c = *src;
    *dest = c;
    if (c == 0)
      return dest;
    src++;
    dest++;
  }
}
*/

int FindCharPosInString(const char *s, char c) throw();
int FindCharPosInString(const wchar_t *s, wchar_t c) throw();

#ifdef _WIN32
  #ifndef _UNICODE
    #define STRING_UNICODE_THROW
  #endif
#endif

#ifndef STRING_UNICODE_THROW
  #define STRING_UNICODE_THROW throw()
#endif


inline char MyCharUpper_Ascii(char c)
{
  if (c >= 'a' && c <= 'z')
    return (char)((unsigned char)c - 0x20);
  return c;
}

/*
inline wchar_t MyCharUpper_Ascii(wchar_t c)
{
  if (c >= 'a' && c <= 'z')
    return (wchar_t)(c - 0x20);
  return c;
}
*/

inline char MyCharLower_Ascii(char c)
{
  if (c >= 'A' && c <= 'Z')
    return (char)((unsigned char)c + 0x20);
  return c;
}

inline wchar_t MyCharLower_Ascii(wchar_t c)
{
  if (c >= 'A' && c <= 'Z')
    return (wchar_t)(c + 0x20);
  return c;
}

wchar_t MyCharUpper_WIN(wchar_t c) throw();

inline wchar_t MyCharUpper(wchar_t c) throw()
{
  if (c < 'a') return c;
  if (c <= 'z') return (wchar_t)(c - 0x20);
  if (c <= 0x7F) return c;
  #ifdef _WIN32
    #ifdef _UNICODE
      return (wchar_t)(unsigned)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)(unsigned)c);
    #else
      return (wchar_t)MyCharUpper_WIN(c);
    #endif
  #else
    return (wchar_t)towupper((wint_t)c);
  #endif
}

/*
wchar_t MyCharLower_WIN(wchar_t c) throw();

inline wchar_t MyCharLower(wchar_t c) throw()
{
  if (c < 'A') return c;
  if (c <= 'Z') return (wchar_t)(c + 0x20);
  if (c <= 0x7F) return c;
  #ifdef _WIN32
    #ifdef _UNICODE
      return (wchar_t)(unsigned)(UINT_PTR)CharLowerW((LPWSTR)(UINT_PTR)(unsigned)c);
    #else
      return (wchar_t)MyCharLower_WIN(c);
    #endif
  #else
    return (wchar_t)tolower(c);
  #endif
}
*/

// char *MyStringUpper(char *s) throw();
// char *MyStringLower(char *s) throw();

// void MyStringUpper_Ascii(char *s) throw();
// void MyStringUpper_Ascii(wchar_t *s) throw();
void MyStringLower_Ascii(char *s) throw();
void MyStringLower_Ascii(wchar_t *s) throw();
// wchar_t *MyStringUpper(wchar_t *s) STRING_UNICODE_THROW;
// wchar_t *MyStringLower(wchar_t *s) STRING_UNICODE_THROW;

bool StringsAreEqualNoCase(const wchar_t *s1, const wchar_t *s2) throw();

bool IsString1PrefixedByString2(const char *s1, const char *s2) throw();
bool IsString1PrefixedByString2(const wchar_t *s1, const wchar_t *s2) throw();
bool IsString1PrefixedByString2(const wchar_t *s1, const char *s2) throw();
bool IsString1PrefixedByString2_NoCase_Ascii(const char *s1, const char *s2) throw();
bool IsString1PrefixedByString2_NoCase_Ascii(const wchar_t *u, const char *a) throw();
bool IsString1PrefixedByString2_NoCase(const wchar_t *s1, const wchar_t *s2) throw();

#define MyStringCompare(s1, s2) wcscmp(s1, s2)
int MyStringCompareNoCase(const wchar_t *s1, const wchar_t *s2) throw();
// int MyStringCompareNoCase_N(const wchar_t *s1, const wchar_t *s2, unsigned num) throw();

// ---------- ASCII ----------
// char values in ASCII strings must be less then 128
bool StringsAreEqual_Ascii(const char *u, const char *a) throw();
bool StringsAreEqual_Ascii(const wchar_t *u, const char *a) throw();
bool StringsAreEqualNoCase_Ascii(const char *s1, const char *s2) throw();
bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const char *s2) throw();
bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const wchar_t *s2) throw();

#define MY_STRING_DELETE(_p_) { delete [](_p_); }
// #define MY_STRING_DELETE(_p_) my_delete(_p_);


#define FORBID_STRING_OPS_2(cls, t) \
  void Find(t) const; \
  void Find(t, unsigned startIndex) const; \
  void ReverseFind(t) const; \
  void InsertAtFront(t); \
  void RemoveChar(t); \
  void Replace(t, t); \

#define FORBID_STRING_OPS(cls, t) \
  explicit cls(t); \
  explicit cls(const t *); \
  cls &operator=(t); \
  cls &operator=(const t *); \
  cls &operator+=(t); \
  cls &operator+=(const t *); \
  FORBID_STRING_OPS_2(cls, t) \

/*
  cls &operator+(t); \
  cls &operator+(const t *); \
*/

#define FORBID_STRING_OPS_AString(t) FORBID_STRING_OPS(AString, t)
#define FORBID_STRING_OPS_UString(t) FORBID_STRING_OPS(UString, t)
#define FORBID_STRING_OPS_UString2(t) FORBID_STRING_OPS(UString2, t)

class AString
{
  char *_chars;
  unsigned _len;
  unsigned _limit;

  void MoveItems(unsigned dest, unsigned src)
  {
    memmove(_chars + dest, _chars + src, (size_t)(_len - src + 1) * sizeof(char));
  }
  
  void InsertSpace(unsigned &index, unsigned size);
  
  void ReAlloc(unsigned newLimit);
  void ReAlloc2(unsigned newLimit);
  void SetStartLen(unsigned len);
  
  Z7_NO_INLINE
  void Grow_1();
  void Grow(unsigned n);

  AString(unsigned num, const char *s);
  AString(unsigned num, const AString &s);
  AString(const AString &s, char c); // it's for String + char
  AString(const char *s1, unsigned num1, const char *s2, unsigned num2);

  friend AString operator+(const AString &s, char c) { return AString(s, c); }
  // friend AString operator+(char c, const AString &s); // is not supported

  friend AString operator+(const AString &s1, const AString &s2);
  friend AString operator+(const AString &s1, const char    *s2);
  friend AString operator+(const char    *s1, const AString &s2);

  // ---------- forbidden functions ----------

  #ifdef MY_NATIVE_WCHAR_T_DEFINED
  FORBID_STRING_OPS_AString(wchar_t)
  #endif

  FORBID_STRING_OPS_AString(signed char)
  FORBID_STRING_OPS_AString(unsigned char)
  FORBID_STRING_OPS_AString(short)
  FORBID_STRING_OPS_AString(unsigned short)
  FORBID_STRING_OPS_AString(int)
  FORBID_STRING_OPS_AString(unsigned)
  FORBID_STRING_OPS_AString(long)
  FORBID_STRING_OPS_AString(unsigned long)

 #ifdef DEBUG_FSTRING_INHERITS_ASTRING
  AString(const FString &s);
  AString &operator=(const FString &s);
  AString &operator+=(const FString &s);
 #endif

public:
  explicit AString();
  explicit AString(char c);
  explicit AString(const char *s);
  AString(const AString &s);
  ~AString() { MY_STRING_DELETE(_chars) }

  unsigned Len() const { return _len; }
  bool IsEmpty() const { return _len == 0; }
  void Empty() { _len = 0; _chars[0] = 0; }

  operator const char *() const { return _chars; }
  char *Ptr_non_const() const { return _chars; }
  const char *Ptr() const { return _chars; }
  const char *Ptr(unsigned pos) const { return _chars + pos; }
  const char *Ptr(int pos) const { return _chars + (unsigned)pos; }
  const char *RightPtr(unsigned num) const { return _chars + _len - num; }
  char Back() const { return _chars[(size_t)_len - 1]; }

  void ReplaceOneCharAtPos(unsigned pos, char c) { _chars[pos] = c; }

  char *GetBuf() { return _chars; }
  /* GetBuf(minLen): provides the buffer that can store
     at least (minLen) characters and additional null terminator.
     9.35: GetBuf doesn't preserve old characters and terminator */
  char *GetBuf(unsigned minLen)
  {
    if (minLen > _limit)
      ReAlloc2(minLen);
    return _chars;
  }
  char *GetBuf_SetEnd(unsigned minLen)
  {
    if (minLen > _limit)
      ReAlloc2(minLen);
    char *chars = _chars;
    chars[minLen] = 0;
    _len = minLen;
    return chars;
  }

  void ReleaseBuf_SetLen(unsigned newLen) { _len = newLen; }
  void ReleaseBuf_SetEnd(unsigned newLen) { _len = newLen; _chars[newLen] = 0; }
  void ReleaseBuf_CalcLen(unsigned maxLen)
  {
    char *chars = _chars;
    chars[maxLen] = 0;
    _len = MyStringLen(chars);
  }

  AString &operator=(char c);
  AString &operator=(const char *s);
  AString &operator=(const AString &s);
  void SetFromWStr_if_Ascii(const wchar_t *s);
  // void SetFromBstr_if_Ascii(BSTR s);

// private:
  Z7_FORCE_INLINE
  AString &operator+=(char c)
  {
    if (_limit == _len)
      Grow_1();
    unsigned len = _len;
    char *chars = _chars;
    chars[len++] = c;
    chars[len] = 0;
    _len = len;
    return *this;
  }
public:
  void Add_Space();
  void Add_Space_if_NotEmpty();
  void Add_OptSpaced(const char *s);
  void Add_Char(char c);
  void Add_LF();
  void Add_Slash();
  void Add_Dot();
  void Add_Minus();
  void Add_Colon();
  void Add_PathSepar() { operator+=(CHAR_PATH_SEPARATOR); }

  AString &operator+=(const char *s);
  AString &operator+=(const AString &s);

  void Add_UInt32(UInt32 v);
  void Add_UInt64(UInt64 v);

  void AddFrom(const char *s, unsigned len); // no check
  void SetFrom(const char *s, unsigned len); // no check
  void SetFrom_Chars_SizeT(const char* s, size_t len); // no check
  void SetFrom(const char* s, int len) // no check
  {
    SetFrom(s, (unsigned)len); // no check
  }
  void SetFrom_CalcLen(const char *s, unsigned len);

  AString Mid(unsigned startIndex, unsigned count) const { return AString(count, _chars + startIndex); }
  AString Left(unsigned count) const { return AString(count, *this); }
  // void MakeUpper() { MyStringUpper(_chars); }
  // void MakeLower() { MyStringLower(_chars); }
  void MakeLower_Ascii() { MyStringLower_Ascii(_chars); }


  bool IsEqualTo(const char *s) const { return strcmp(_chars, s) == 0; }
  bool IsEqualTo_Ascii_NoCase(const char *s) const { return StringsAreEqualNoCase_Ascii(_chars, s); }
  // int Compare(const char *s) const { return MyStringCompare(_chars, s); }
  // int Compare(const AString &s) const { return MyStringCompare(_chars, s._chars); }
  // int CompareNoCase(const char *s) const { return MyStringCompareNoCase(_chars, s); }
  // int CompareNoCase(const AString &s) const { return MyStringCompareNoCase(_chars, s._chars); }
  bool IsPrefixedBy(const char *s) const { return IsString1PrefixedByString2(_chars, s); }
  bool IsPrefixedBy_Ascii_NoCase(const char *s) const { return IsString1PrefixedByString2_NoCase_Ascii(_chars, s); }
 
  bool IsAscii() const
  {
    const unsigned len = Len();
    const char *s = _chars;
    for (unsigned i = 0; i < len; i++)
      if ((unsigned char)s[i] >= 0x80)
        return false;
    return true;
  }
  int Find(char c) const { return FindCharPosInString(_chars, c); }
  int Find(char c, unsigned startIndex) const
  {
    const int pos = FindCharPosInString(_chars + startIndex, c);
    return pos < 0 ? -1 : (int)startIndex + pos;
  }
  int Find(char c, int startIndex) const
  {
    return Find(c, (unsigned)startIndex);
  }
  
  int ReverseFind(char c) const throw();
  int ReverseFind_Dot() const throw() { return ReverseFind('.'); }
  int ReverseFind_PathSepar() const throw();

  int Find(const char *s) const { return Find(s, 0); }
  int Find(const char *s, unsigned startIndex) const throw();
  
  void TrimLeft() throw();
  void TrimRight() throw();
  void Trim()
  {
    TrimRight();
    TrimLeft();
  }

  void InsertAtFront(char c);
  // void Insert(unsigned index, char c);
  void Insert(unsigned index, const char *s);
  void Insert(unsigned index, const AString &s);

  void RemoveChar(char ch) throw();
  
  void Replace(char oldChar, char newChar) throw();
  void Replace(const AString &oldString, const AString &newString);

  void Delete(unsigned index) throw();
  void Delete(unsigned index, unsigned count) throw();
  void DeleteFrontal(unsigned num) throw();
  void DeleteBack() { _chars[--_len] = 0; }
  void DeleteFrom(unsigned index)
  {
    if (index < _len)
    {
      _len = index;
      _chars[index] = 0;
    }
  }
  void DeleteFrom(int index)
  {
    DeleteFrom((unsigned)index);
  }

  
  void Wipe_and_Empty()
  {
    if (_chars)
    {
      memset(_chars, 0, (_limit + 1) * sizeof(*_chars));
      _len = 0;
    }
  }
};


class AString_Wipe: public AString
{
  Z7_CLASS_NO_COPY(AString_Wipe)
public:
  AString_Wipe(): AString() {}
  // AString_Wipe(const AString &s): AString(s) {}
  // AString_Wipe &operator=(const AString &s) { AString::operator=(s); return *this; }
  // AString_Wipe &operator=(const char *s) { AString::operator=(s); return *this; }
  ~AString_Wipe() { Wipe_and_Empty(); }
};


bool operator<(const AString &s1, const AString &s2);
bool operator>(const AString &s1, const AString &s2);

/*
bool operator==(const AString &s1, const AString &s2);
bool operator==(const AString &s1, const char    *s2);
bool operator==(const char    *s1, const AString &s2);

bool operator!=(const AString &s1, const AString &s2);
bool operator!=(const AString &s1, const char    *s2);
bool operator!=(const char    *s1, const AString &s2);
*/

inline bool operator==(const AString &s1, const AString &s2) { return s1.Len() == s2.Len() && strcmp(s1, s2) == 0; }
inline bool operator==(const AString &s1, const char    *s2) { return strcmp(s1, s2) == 0; }
inline bool operator==(const char    *s1, const AString &s2) { return strcmp(s1, s2) == 0; }

inline bool operator!=(const AString &s1, const AString &s2) { return s1.Len() != s2.Len() || strcmp(s1, s2) != 0; }
inline bool operator!=(const AString &s1, const char    *s2) { return strcmp(s1, s2) != 0; }
inline bool operator!=(const char    *s1, const AString &s2) { return strcmp(s1, s2) != 0; }

// ---------- forbidden functions ----------

void operator==(char c1, const AString &s2);
void operator==(const AString &s1, char c2);

void operator+(char c, const AString &s); // this function can be OK, but we don't use it

void operator+(const AString &s, int c);
void operator+(const AString &s, unsigned c);
void operator+(int c, const AString &s);
void operator+(unsigned c, const AString &s);
void operator-(const AString &s, int c);
void operator-(const AString &s, unsigned c);


class UString
{
  wchar_t *_chars;
  unsigned _len;
  unsigned _limit;

  void MoveItems(unsigned dest, unsigned src)
  {
    memmove(_chars + dest, _chars + src, (size_t)(_len - src + 1) * sizeof(wchar_t));
  }
  
  void InsertSpace(unsigned index, unsigned size);
  
  void ReAlloc(unsigned newLimit);
  void ReAlloc2(unsigned newLimit);
  void SetStartLen(unsigned len);
  void Grow_1();
  void Grow(unsigned n);

  UString(unsigned num, const wchar_t *s); // for Mid
  UString(unsigned num, const UString &s); // for Left
  UString(const UString &s, wchar_t c); // it's for String + char
  UString(const wchar_t *s1, unsigned num1, const wchar_t *s2, unsigned num2);

  friend UString operator+(const UString &s, wchar_t c) { return UString(s, c); }
  // friend UString operator+(wchar_t c, const UString &s); // is not supported

  friend UString operator+(const UString &s1, const UString &s2);
  friend UString operator+(const UString &s1, const wchar_t *s2);
  friend UString operator+(const wchar_t *s1, const UString &s2);

  // ---------- forbidden functions ----------
  
  FORBID_STRING_OPS_UString(signed char)
  FORBID_STRING_OPS_UString(unsigned char)
  FORBID_STRING_OPS_UString(short)
  
  #ifdef MY_NATIVE_WCHAR_T_DEFINED
  FORBID_STRING_OPS_UString(unsigned short)
  #endif

  FORBID_STRING_OPS_UString(int)
  FORBID_STRING_OPS_UString(unsigned)
  FORBID_STRING_OPS_UString(long)
  FORBID_STRING_OPS_UString(unsigned long)

  FORBID_STRING_OPS_2(UString, char)

 #ifdef DEBUG_FSTRING_INHERITS_ASTRING
  UString(const FString &s);
  UString &operator=(const FString &s);
  UString &operator+=(const FString &s);
 #endif

public:
  UString();
  explicit UString(wchar_t c);
  explicit UString(char c);
  explicit UString(const char *s);
  explicit UString(const AString &s);
  UString(const wchar_t *s);
  UString(const UString &s);
  ~UString() { MY_STRING_DELETE(_chars) }

  unsigned Len() const { return _len; }
  bool IsEmpty() const { return _len == 0; }
  void Empty() { _len = 0; _chars[0] = 0; }

  operator const wchar_t *() const { return _chars; }
  wchar_t *Ptr_non_const() const { return _chars; }
  const wchar_t *Ptr() const { return _chars; }
  const wchar_t *Ptr(int pos) const { return _chars + (unsigned)pos; }
  const wchar_t *Ptr(unsigned pos) const { return _chars + pos; }
  const wchar_t *RightPtr(unsigned num) const { return _chars + _len - num; }
  wchar_t Back() const { return _chars[(size_t)_len - 1]; }

  void ReplaceOneCharAtPos(unsigned pos, wchar_t c) { _chars[pos] = c; }

  wchar_t *GetBuf() { return _chars; }

  /*
  wchar_t *GetBuf_GetMaxAvail(unsigned &availBufLen)
  {
    availBufLen = _limit;
    return _chars;
  }
  */

  wchar_t *GetBuf(unsigned minLen)
  {
    if (minLen > _limit)
      ReAlloc2(minLen);
    return _chars;
  }
  wchar_t *GetBuf_SetEnd(unsigned minLen)
  {
    if (minLen > _limit)
      ReAlloc2(minLen);
    wchar_t *chars = _chars;
    chars[minLen] = 0;
    _len = minLen;
    return chars;
  }

  void ReleaseBuf_SetLen(unsigned newLen) { _len = newLen; }
  void ReleaseBuf_SetEnd(unsigned newLen) { _len = newLen; _chars[newLen] = 0; }
  void ReleaseBuf_CalcLen(unsigned maxLen)
  {
    wchar_t *chars = _chars;
    chars[maxLen] = 0;
    _len = MyStringLen(chars);
  }

  UString &operator=(wchar_t c);
  UString &operator=(char c) { return (*this)=((wchar_t)(unsigned char)c); }
  UString &operator=(const wchar_t *s);
  UString &operator=(const UString &s);
  void SetFrom(const wchar_t *s, unsigned len); // no check
  void SetFromBstr(LPCOLESTR s);
  UString &operator=(const char *s);
  UString &operator=(const AString &s) { return operator=(s.Ptr()); }

// private:
  Z7_FORCE_INLINE
  UString &operator+=(wchar_t c)
  {
    if (_limit == _len)
      Grow_1();
    unsigned len = _len;
    wchar_t *chars = _chars;
    chars[len++] = c;
    chars[len] = 0;
    _len = len;
    return *this;
  }

private:
  UString &operator+=(char c); //  { return (*this)+=((wchar_t)(unsigned char)c); }
public:
  void Add_Char(char c);
  // void Add_WChar(wchar_t c);
  void Add_Space();
  void Add_Space_if_NotEmpty();
  void Add_LF();
  void Add_Dot();
  void Add_Minus();
  void Add_Colon();
  void Add_PathSepar() { operator+=(WCHAR_PATH_SEPARATOR); }

  UString &operator+=(const wchar_t *s);
  UString &operator+=(const UString &s);
  UString &operator+=(const char *s);
  UString &operator+=(const AString &s) { return operator+=(s.Ptr()); }

  void Add_UInt32(UInt32 v);
  void Add_UInt64(UInt64 v);

  UString Mid(unsigned startIndex, unsigned count) const { return UString(count, _chars + startIndex); }
  UString Left(unsigned count) const { return UString(count, *this); }
  UString Left(int count) const { return Left((unsigned)count); }

  // void MakeUpper() { MyStringUpper(_chars); }
  // void MakeUpper() { MyStringUpper_Ascii(_chars); }
  // void MakeUpper_Ascii() { MyStringUpper_Ascii(_chars); }
  void MakeLower_Ascii() { MyStringLower_Ascii(_chars); }

  bool IsEqualTo(const char *s) const { return StringsAreEqual_Ascii(_chars, s); }
  bool IsEqualTo_NoCase(const wchar_t *s) const { return StringsAreEqualNoCase(_chars, s); }
  bool IsEqualTo_Ascii_NoCase(const char *s) const { return StringsAreEqualNoCase_Ascii(_chars, s); }
  int Compare(const wchar_t *s) const { return wcscmp(_chars, s); }
  // int Compare(const UString &s) const { return MyStringCompare(_chars, s._chars); }
  // int CompareNoCase(const wchar_t *s) const { return MyStringCompareNoCase(_chars, s); }
  // int CompareNoCase(const UString &s) const { return MyStringCompareNoCase(_chars, s._chars); }
  bool IsPrefixedBy(const wchar_t *s) const { return IsString1PrefixedByString2(_chars, s); }
  bool IsPrefixedBy(const char *s) const { return IsString1PrefixedByString2(_chars, s); }
  bool IsPrefixedBy_NoCase(const wchar_t *s) const { return IsString1PrefixedByString2_NoCase(_chars, s); }
  bool IsPrefixedBy_Ascii_NoCase(const char *s) const { return IsString1PrefixedByString2_NoCase_Ascii(_chars, s); }

  bool IsAscii() const
  {
    const unsigned len = Len();
    const wchar_t *s = _chars;
    for (unsigned i = 0; i < len; i++)
      if ((unsigned)(int)s[i] >= 0x80)
        return false;
    return true;
  }
  int Find(wchar_t c) const { return FindCharPosInString(_chars, c); }
  int Find(wchar_t c, unsigned startIndex) const
  {
    const int pos = FindCharPosInString(_chars + startIndex, c);
    return pos < 0 ? -1 : (int)startIndex + pos;
  }

  int ReverseFind(wchar_t c) const throw();
  int ReverseFind_Dot() const throw() { return ReverseFind(L'.'); }
  int ReverseFind_PathSepar() const throw();

  int Find(const wchar_t *s) const { return Find(s, 0); }
  int Find(const wchar_t *s, unsigned startIndex) const throw();

  void TrimLeft() throw();
  void TrimRight() throw();
  void Trim()
  {
    TrimRight();
    TrimLeft();
  }

  void InsertAtFront(wchar_t c);
  // void Insert_wchar_t(unsigned index, wchar_t c);
  void Insert(unsigned index, const wchar_t *s);
  void Insert(unsigned index, const UString &s);

  void RemoveChar(wchar_t ch) throw();
  
  void Replace(wchar_t oldChar, wchar_t newChar) throw();
  void Replace(const UString &oldString, const UString &newString);

  void Delete(int index) throw() { Delete((unsigned)index); }
  void Delete(unsigned index) throw();
  void Delete(unsigned index, unsigned count) throw();
  void DeleteFrontal(unsigned num) throw();
  void DeleteBack() { _chars[--_len] = 0; }
  void DeleteFrom(int index) { DeleteFrom((unsigned)index); }
  void DeleteFrom(unsigned index)
  {
    if (index < _len)
    {
      _len = index;
      _chars[index] = 0;
    }
  }
  
  void Wipe_and_Empty()
  {
    if (_chars)
    {
      memset(_chars, 0, (_limit + 1) * sizeof(*_chars));
      _len = 0;
    }
  }
};


class UString_Wipe: public UString
{
  Z7_CLASS_NO_COPY(UString_Wipe)
public:
  UString_Wipe(): UString() {}
  // UString_Wipe(const UString &s): UString(s) {}
  // UString_Wipe &operator=(const UString &s) { UString::operator=(s); return *this; }
  // UString_Wipe &operator=(const wchar_t *s) { UString::operator=(s); return *this; }
  ~UString_Wipe() { Wipe_and_Empty(); }
};


bool operator<(const UString &s1, const UString &s2);
bool operator>(const UString &s1, const UString &s2);

inline bool operator==(const UString &s1, const UString &s2) { return s1.Len() == s2.Len() && wcscmp(s1, s2) == 0; }
inline bool operator==(const UString &s1, const wchar_t *s2) { return wcscmp(s1, s2) == 0; }
inline bool operator==(const wchar_t *s1, const UString &s2) { return wcscmp(s1, s2) == 0; }

inline bool operator!=(const UString &s1, const UString &s2) { return s1.Len() != s2.Len() || wcscmp(s1, s2) != 0; }
inline bool operator!=(const UString &s1, const wchar_t *s2) { return wcscmp(s1, s2) != 0; }
inline bool operator!=(const wchar_t *s1, const UString &s2) { return wcscmp(s1, s2) != 0; }


// ---------- forbidden functions ----------

void operator==(wchar_t c1, const UString &s2);
void operator==(const UString &s1, wchar_t c2);

void operator+(wchar_t c, const UString &s); // this function can be OK, but we don't use it

void operator+(const AString &s1, const UString &s2);
void operator+(const UString &s1, const AString &s2);

void operator+(const UString &s1, const char *s2);
void operator+(const char *s1, const UString &s2);

void operator+(const UString &s, char c);
void operator+(const UString &s, unsigned char c);
void operator+(char c, const UString &s);
void operator+(unsigned char c, const UString &s);
void operator-(const UString &s1, wchar_t c);

#ifdef _WIN32
// can we forbid these functions, if wchar_t is 32-bit ?
void operator+(const UString &s, int c);
void operator+(const UString &s, unsigned c);
void operator+(int c, const UString &s);
void operator+(unsigned c, const UString &s);
void operator-(const UString &s1, int c);
void operator-(const UString &s1, unsigned c);
#endif







class UString2
{
  wchar_t *_chars;
  unsigned _len;

  void ReAlloc2(unsigned newLimit);
  void SetStartLen(unsigned len);

  // ---------- forbidden functions ----------
  
  FORBID_STRING_OPS_UString2(char)
  FORBID_STRING_OPS_UString2(signed char)
  FORBID_STRING_OPS_UString2(unsigned char)
  FORBID_STRING_OPS_UString2(short)

  UString2 &operator=(wchar_t c);

  UString2(const AString &s);
  UString2 &operator=(const AString &s);
  UString2 &operator+=(const AString &s);

 #ifdef DEBUG_FSTRING_INHERITS_ASTRING
  UString2(const FString &s);
  UString2 &operator=(const FString &s);
  UString2 &operator+=(const FString &s);
 #endif

public:
  UString2(): _chars(NULL), _len(0) {}
  UString2(const wchar_t *s);
  UString2(const UString2 &s);
  ~UString2() { if (_chars) { MY_STRING_DELETE(_chars) } }

  unsigned Len() const { return _len; }
  bool IsEmpty() const { return _len == 0; }
  // void Empty() { _len = 0; _chars[0] = 0; }

  // operator const wchar_t *() const { return _chars; }
  const wchar_t *GetRawPtr() const { return _chars; }

  int Compare(const wchar_t *s) const { return wcscmp(_chars, s); }

  wchar_t *GetBuf(unsigned minLen)
  {
    if (!_chars || minLen > _len)
      ReAlloc2(minLen);
    return _chars;
  }
  void ReleaseBuf_SetLen(unsigned newLen) { _len = newLen; }

  UString2 &operator=(const wchar_t *s);
  UString2 &operator=(const UString2 &s);
  void SetFromAscii(const char *s);
};

bool operator==(const UString2 &s1, const UString2 &s2);
bool operator==(const UString2 &s1, const wchar_t *s2);
bool operator==(const wchar_t *s1, const UString2 &s2);

inline bool operator!=(const UString2 &s1, const UString2 &s2) { return !(s1 == s2); }
inline bool operator!=(const UString2 &s1, const wchar_t *s2) { return !(s1 == s2); }
inline bool operator!=(const wchar_t *s1, const UString2 &s2) { return !(s1 == s2); }


// ---------- forbidden functions ----------

void operator==(wchar_t c1, const UString2 &s2);
void operator==(const UString2 &s1, wchar_t c2);
bool operator<(const UString2 &s1, const UString2 &s2);
bool operator>(const UString2 &s1, const UString2 &s2);

void operator+(const UString2 &s1, const UString2 &s2);
void operator+(const UString2 &s1, const wchar_t *s2);
void operator+(const wchar_t *s1, const UString2 &s2);
void operator+(wchar_t c, const UString2 &s);
void operator+(const UString2 &s, wchar_t c);
void operator+(const UString2 &s, char c);
void operator+(const UString2 &s, unsigned char c);
void operator+(char c, const UString2 &s);
void operator+(unsigned char c, const UString2 &s);
void operator-(const UString2 &s1, wchar_t c);






typedef CObjectVector<AString> AStringVector;
typedef CObjectVector<UString> UStringVector;

#ifdef _UNICODE
  typedef UString CSysString;
#else
  typedef AString CSysString;
#endif

typedef CObjectVector<CSysString> CSysStringVector;


// ---------- FString ----------

#ifndef DEBUG_FSTRING_INHERITS_ASTRING
#ifdef _WIN32
  #define USE_UNICODE_FSTRING
#endif
#endif

#ifdef USE_UNICODE_FSTRING

  #define MY_FTEXT(quote) L##quote

  typedef wchar_t FChar;
  typedef UString FString;

  #define fs2us(_x_) (_x_)
  #define us2fs(_x_) (_x_)
  FString fas2fs(const char *s);
  FString fas2fs(const AString &s);
  AString fs2fas(const FChar *s);

#else // USE_UNICODE_FSTRING

  #define MY_FTEXT(quote) quote

  typedef char FChar;

 #ifdef DEBUG_FSTRING_INHERITS_ASTRING

  class FString: public AString
  {
    // FString &operator=(const char *s);
    FString &operator=(const AString &s);
    // FString &operator+=(const AString &s);
  public:
    FString(const AString &s): AString(s.Ptr()) {}
    FString(const FString &s): AString(s.Ptr()) {}
    FString(const char *s): AString(s) {}
    FString() {}
    FString &operator=(const FString &s)  { AString::operator=((const AString &)s); return *this; }
    FString &operator=(char c) { AString::operator=(c); return *this; }
    FString &operator+=(char c) { AString::operator+=(c); return *this; }
    FString &operator+=(const FString &s) { AString::operator+=((const AString &)s); return *this; }
    FString Left(unsigned count) const  { return FString(AString::Left(count)); }
  };
  void operator+(const AString &s1, const FString &s2);
  void operator+(const FString &s1, const AString &s2);

  inline FString operator+(const FString &s1, const FString &s2)
  {
    AString s =(const AString &)s1 + (const AString &)s2;
    return FString(s.Ptr());
    // return FString((const AString &)s1 + (const AString &)s2);
  }
  inline FString operator+(const FString &s1, const FChar *s2)
  {
    return s1 + (FString)s2;
  }
  /*
  inline FString operator+(const FChar *s1, const FString &s2)
  {
    return (FString)s1 + s2;
  }
  */

  inline FString fas2fs(const char *s)  { return FString(s); }

 #else // DEBUG_FSTRING_INHERITS_ASTRING
  typedef AString FString;
  #define fas2fs(_x_) (_x_)
 #endif // DEBUG_FSTRING_INHERITS_ASTRING

  UString fs2us(const FChar *s);
  UString fs2us(const FString &s);
  FString us2fs(const wchar_t *s);
  #define fs2fas(_x_) (_x_)

#endif // USE_UNICODE_FSTRING

#define FTEXT(quote) MY_FTEXT(quote)

#define FCHAR_PATH_SEPARATOR FTEXT(CHAR_PATH_SEPARATOR)
#define FSTRING_PATH_SEPARATOR FTEXT(STRING_PATH_SEPARATOR)

// #define FCHAR_ANY_MASK FTEXT('*')
// #define FSTRING_ANY_MASK FTEXT("*")

typedef const FChar *CFSTR;

typedef CObjectVector<FString> FStringVector;


class CStringFinder
{
  AString _temp;
public:
  // list - is list of low case Ascii strings separated by space " ".
  // the function returns true, if it can find exact word (str) in (list).
  bool FindWord_In_LowCaseAsciiList_NoCase(const char *list, const wchar_t *str);
};

void SplitString(const UString &srcString, UStringVector &destStrings);

#endif



#if defined(_WIN32)
  // #include <wchar.h>
  // WCHAR_MAX is defined as ((wchar_t)-1)
  #define Z7_WCHART_IS_16BIT 1
#elif (defined(WCHAR_MAX) && (WCHAR_MAX <= 0xffff)) \
   || (defined(__SIZEOF_WCHAR_T__) && (__SIZEOF_WCHAR_T__ == 2))
  #define Z7_WCHART_IS_16BIT 1
#endif

#if WCHAR_PATH_SEPARATOR == L'\\'
// WSL scheme
#define WCHAR_IN_FILE_NAME_BACKSLASH_REPLACEMENT  ((wchar_t)((unsigned)(0xF000) + (unsigned)'\\'))
// #define WCHAR_IN_FILE_NAME_BACKSLASH_REPLACEMENT  '_'
#endif

/* ---- CPP/Common/StringConvert.h ---- */
// Common/StringConvert.h

#ifndef ZIP7_INC_COMMON_STRING_CONVERT_H
#define ZIP7_INC_COMMON_STRING_CONVERT_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

UString MultiByteToUnicodeString(const AString &src, UINT codePage = CP_ACP);
UString MultiByteToUnicodeString(const char *src, UINT codePage = CP_ACP);

// optimized versions that work faster for ASCII strings
void MultiByteToUnicodeString2(UString &dest, const AString &src, UINT codePage = CP_ACP);
// void UnicodeStringToMultiByte2(AString &dest, const UString &s, UINT codePage, char defaultChar, bool &defaultCharWasUsed);
void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT codePage);

AString UnicodeStringToMultiByte(const UString &src, UINT codePage, char defaultChar, bool &defaultCharWasUsed);
AString UnicodeStringToMultiByte(const UString &src, UINT codePage = CP_ACP);

inline const wchar_t* GetUnicodeString(const wchar_t *u)  { return u; }
inline const UString& GetUnicodeString(const UString &u)  { return u; }

inline UString GetUnicodeString(const AString &a)  { return MultiByteToUnicodeString(a); }
inline UString GetUnicodeString(const char *a)     { return MultiByteToUnicodeString(a); }

inline UString GetUnicodeString(const AString &a, UINT codePage)
  { return MultiByteToUnicodeString(a, codePage); }
inline UString GetUnicodeString(const char *a, UINT codePage)
  { return MultiByteToUnicodeString(a, codePage); }

inline const wchar_t* GetUnicodeString(const wchar_t *u, UINT) { return u; }
inline const UString& GetUnicodeString(const UString &u, UINT) { return u; }

inline const char*    GetAnsiString(const char    *a) { return a; }
inline const AString& GetAnsiString(const AString &a) { return a; }

inline AString GetAnsiString(const wchar_t *u) { return UnicodeStringToMultiByte(UString(u)); }
inline AString GetAnsiString(const UString &u) { return UnicodeStringToMultiByte(u); }

/*
inline const char* GetOemString(const char* oem)
  { return oem; }
inline const AString& GetOemString(const AString &oem)
  { return oem; }
*/
const char* GetOemString(const char* oem);
const AString& GetOemString(const AString &oem);
inline AString GetOemString(const UString &u)
  { return UnicodeStringToMultiByte(u, CP_OEMCP); }

#ifdef _UNICODE
  inline const wchar_t* GetSystemString(const wchar_t *u) { return u;}
  inline const UString& GetSystemString(const UString &u) { return u;}
  inline const wchar_t* GetSystemString(const wchar_t *u, UINT /* codePage */) { return u;}
  inline const UString& GetSystemString(const UString &u, UINT /* codePage */) { return u;}
  
  inline UString GetSystemString(const AString &a, UINT codePage) { return MultiByteToUnicodeString(a, codePage); }
  inline UString GetSystemString(const char    *a, UINT codePage) { return MultiByteToUnicodeString(a, codePage); }
  inline UString GetSystemString(const AString &a) { return MultiByteToUnicodeString(a); }
  inline UString GetSystemString(const char    *a) { return MultiByteToUnicodeString(a); }
#else
  inline const char*    GetSystemString(const char    *a) { return a; }
  inline const AString& GetSystemString(const AString &a) { return a; }
  inline const char*    GetSystemString(const char    *a, UINT) { return a; }
  inline const AString& GetSystemString(const AString &a, UINT) { return a; }
  
  inline AString GetSystemString(const wchar_t *u) { return UnicodeStringToMultiByte(UString(u)); }
  inline AString GetSystemString(const UString &u) { return UnicodeStringToMultiByte(u); }
  inline AString GetSystemString(const UString &u, UINT codePage) { return UnicodeStringToMultiByte(u, codePage); }



  /*
  inline AString GetSystemString(const wchar_t *u)
  {
    UString s;
    s = u;
    return UnicodeStringToMultiByte(s);
  }
  */

#endif

#ifndef UNDER_CE
AString SystemStringToOemString(const CSysString &src);
#endif


#ifdef _WIN32
/* we don't need locale functions in Windows
   but we can define ENV_HAVE_LOCALE here for debug purposes */
// #define ENV_HAVE_LOCALE
#else
#define ENV_HAVE_LOCALE
#endif

#ifdef ENV_HAVE_LOCALE
void MY_SetLocale();
const char *GetLocale(void);
#endif

#if !defined(_WIN32) || defined(ENV_HAVE_LOCALE)
bool IsNativeUTF8();
#endif

#ifndef _WIN32
extern bool g_ForceToUTF8;
#endif

#endif

/* ---- CPP/Common/Defs.h ---- */
// Common/Defs.h

#ifndef ZIP7_INC_COMMON_DEFS_H
#define ZIP7_INC_COMMON_DEFS_H

template <class T> inline T MyMin(T a, T b) { return a < b ? a : b; }
template <class T> inline T MyMax(T a, T b) { return a > b ? a : b; }

template <class T> inline int MyCompare(T a, T b)
  { return a == b ? 0 : (a < b ? -1 : 1); }

inline int BoolToInt(bool v) { return (v ? 1 : 0); }
inline unsigned BoolToUInt(bool v) { return (v ? 1u : 0u); }
inline bool IntToBool(int v) { return (v != 0); }

#endif

/* ---- CPP/Common/MyBuffer.h ---- */
// Common/MyBuffer.h

#ifndef ZIP7_INC_COMMON_MY_BUFFER_H
#define ZIP7_INC_COMMON_MY_BUFFER_H

#include <string.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

/* 7-Zip now uses CBuffer only as CByteBuffer.
   So there is no need to use Z7_ARRAY_NEW macro in CBuffer code. */

template <class T> class CBuffer
{
  T *_items;
  size_t _size;

public:
  void Free()
  {
    if (_items)
    {
      delete []_items;
      _items = NULL;
    }
    _size = 0;
  }
  
  CBuffer(): _items(NULL), _size(0) {}
  CBuffer(size_t size): _items(NULL), _size(0)
  {
    if (size != 0)
    {
      _items = new T[size];
      _size = size;
    }
  }
  CBuffer(const CBuffer &buffer): _items(NULL), _size(0)
  {
    const size_t size = buffer._size;
    if (size != 0)
    {
      _items = new T[size];
      memcpy(_items, buffer._items, size * sizeof(T));
      _size = size;
    }
  }

  ~CBuffer() { delete []_items; }

  operator       T *()       { return _items; }
  operator const T *() const { return _items; }
  const T* ConstData()    const { return _items; }
        T* NonConstData() const { return _items; }
        T* NonConstData()       { return _items; }
  // const T* Data() const         { return _items; }
  //       T* Data()               { return _items; }

  size_t Size() const { return _size; }

  void Alloc(size_t size)
  {
    if (size != _size)
    {
      Free();
      if (size != 0)
      {
        _items = new T[size];
        _size = size;
      }
    }
  }

  void AllocAtLeast(size_t size)
  {
    if (size > _size)
    {
      Free();
      _items = new T[size];
      _size = size;
    }
  }

  void CopyFrom(const T *data, size_t size)
  {
    Alloc(size);
    if (size != 0)
      memcpy(_items, data, size * sizeof(T));
  }

  void ChangeSize_KeepData(size_t newSize, size_t keepSize)
  {
    if (newSize == _size)
      return;
    T *newBuffer = NULL;
    if (newSize != 0)
    {
      newBuffer = new T[newSize];
      if (keepSize > _size)
        keepSize = _size;
      if (keepSize != 0)
        memcpy(newBuffer, _items, MyMin(keepSize, newSize) * sizeof(T));
    }
    delete []_items;
    _items = newBuffer;
    _size = newSize;
  }

  void Wipe()
  {
    if (_size != 0)
      memset(_items, 0, _size * sizeof(T));
  }

  CBuffer& operator=(const CBuffer &buffer)
  {
    if (&buffer != this)
      CopyFrom(buffer, buffer._size);
    return *this;
  }
};

template <class T>
bool operator==(const CBuffer<T>& b1, const CBuffer<T>& b2)
{
  size_t size1 = b1.Size();
  if (size1 != b2.Size())
    return false;
  if (size1 == 0)
    return true;
  return memcmp(b1, b2, size1 * sizeof(T)) == 0;
}

template <class T>
bool operator!=(const CBuffer<T>& b1, const CBuffer<T>& b2)
{
  size_t size1 = b1.Size();
  if (size1 != b2.Size())
    return true;
  if (size1 == 0)
    return false;
  return memcmp(b1, b2, size1 * sizeof(T)) != 0;
}


// typedef CBuffer<char> CCharBuffer;
// typedef CBuffer<wchar_t> CWCharBuffer;
typedef CBuffer<Byte> CByteBuffer;


class CByteBuffer_Wipe: public CByteBuffer
{
  Z7_CLASS_NO_COPY(CByteBuffer_Wipe)
public:
  // CByteBuffer_Wipe(): CBuffer<Byte>() {}
  CByteBuffer_Wipe(size_t size): CBuffer<Byte>(size) {}
  ~CByteBuffer_Wipe() { Wipe(); }
};



template <class T> class CObjArray
{
protected:
  T *_items;
private:
  // we disable copy
  CObjArray(const CObjArray &buffer);
  void operator=(const CObjArray &buffer);
public:
  void Free()
  {
    delete []_items;
    _items = NULL;
  }
  CObjArray(size_t size): _items(NULL)
  {
    if (size != 0)
    {
      Z7_ARRAY_NEW(_items, T, size)
      // _items = new T[size];
    }
  }
  CObjArray(): _items(NULL) {}
  ~CObjArray() { delete []_items; }
  
  operator       T *()       { return _items; }
  operator const T *() const { return _items; }
  const T* ConstData()    const { return _items; }
        T* NonConstData() const { return _items; }
        T* NonConstData()       { return _items; }
  // const T* Data() const         { return _items; }
  //       T* Data()               { return _items; }
  
  void Alloc(size_t newSize)
  {
    delete []_items;
    _items = NULL;
    Z7_ARRAY_NEW(_items, T, newSize)
    // _items = new T[newSize];
  }
};


/* CSmallObjArray can be used for Byte arrays
   or for arrays whose total size in bytes does not exceed size_t ranges.
   So there is no need to use Z7_ARRAY_NEW macro in CSmallObjArray code. */
template <class T> class CSmallObjArray
{
protected:
  T *_items;
private:
  // we disable copy
  CSmallObjArray(const CSmallObjArray &buffer);
  void operator=(const CSmallObjArray &buffer);
public:
  void Free()
  {
    delete []_items;
    _items = NULL;
  }
  CSmallObjArray(size_t size): _items(NULL)
  {
    if (size != 0)
    {
      // Z7_ARRAY_NEW(_items, T, size)
      _items = new T[size];
    }
  }
  CSmallObjArray(): _items(NULL) {}
  ~CSmallObjArray() { delete []_items; }
  
  operator       T *()       { return _items; }
  operator const T *() const { return _items; }
  const T* ConstData()    const { return _items; }
        T* NonConstData() const { return _items; }
        T* NonConstData()       { return _items; }
  // const T* Data() const         { return _items; }
  //       T* Data()               { return _items; }
  
  void Alloc(size_t newSize)
  {
    delete []_items;
    _items = NULL;
    // Z7_ARRAY_NEW(_items, T, newSize)
    _items = new T[newSize];
  }
};

typedef CSmallObjArray<Byte> CByteArr;
typedef CObjArray<bool> CBoolArr;
typedef CObjArray<int> CIntArr;
typedef CObjArray<unsigned> CUIntArr;


template <class T> class CObjArray2
{
  T *_items;
  unsigned _size;

  // we disable copy
  CObjArray2(const CObjArray2 &buffer);
  void operator=(const CObjArray2 &buffer);
public:
  
  void Free()
  {
    delete []_items;
    _items = NULL;
    _size = 0;
  }
  CObjArray2(): _items(NULL), _size(0) {}
  /*
  CObjArray2(const CObjArray2 &buffer): _items(NULL), _size(0)
  {
    size_t newSize = buffer._size;
    if (newSize != 0)
    {
      T *newBuffer = new T[newSize];;
      _items = newBuffer;
      _size = newSize;
      const T *src = buffer;
      for (size_t i = 0; i < newSize; i++)
        newBuffer[i] = src[i];
    }
  }
  */
  /*
  CObjArray2(size_t size): _items(NULL), _size(0)
  {
    if (size != 0)
    {
      _items = new T[size];
      _size = size;
    }
  }
  */

  ~CObjArray2() { delete []_items; }
  
  operator       T *()       { return _items; }
  operator const T *() const { return _items; }
  
  unsigned Size() const { return (unsigned)_size; }
  bool IsEmpty() const { return _size == 0; }

  // SetSize doesn't keep old items. It allocates new array if size is not equal
  void SetSize(unsigned size)
  {
    if (size == _size)
      return;
    T *newBuffer = NULL;
    if (size != 0)
    {
      Z7_ARRAY_NEW(newBuffer, T, size)
      // newBuffer = new T[size];
    }
    delete []_items;
    _items = newBuffer;
    _size = size;
  }

  /*
  CObjArray2& operator=(const CObjArray2 &buffer)
  {
    Free();
    size_t newSize = buffer._size;
    if (newSize != 0)
    {
      T *newBuffer = new T[newSize];;
      _items = newBuffer;
      _size = newSize;
      const T *src = buffer;
      for (size_t i = 0; i < newSize; i++)
        newBuffer[i] = src[i];
    }
    return *this;
  }
  */
};

#endif

/* ---- CPP/Common/UTFConvert.h ---- */
// Common/UTFConvert.h

#ifndef ZIP7_INC_COMMON_UTF_CONVERT_H
#define ZIP7_INC_COMMON_UTF_CONVERT_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

struct CUtf8Check
{
  // Byte MaxByte;     // in original src stream
  bool NonUtf;
  bool ZeroChar;
  bool SingleSurrogate;
  bool Escape;
  bool Truncated;
  UInt32 MaxHighPoint;  // only for points >= 0x80

  CUtf8Check() { Clear(); }

  void Clear()
  {
    // MaxByte = 0;
    NonUtf = false;
    ZeroChar = false;
    SingleSurrogate = false;
    Escape = false;
    Truncated = false;
    MaxHighPoint = 0;
  }

  void Update(const CUtf8Check &c)
  {
    if (c.NonUtf) NonUtf = true;
    if (c.ZeroChar) ZeroChar = true;
    if (c.SingleSurrogate) SingleSurrogate = true;
    if (c.Escape) Escape = true;
    if (c.Truncated) Truncated = true;
    if (MaxHighPoint < c.MaxHighPoint) MaxHighPoint = c.MaxHighPoint;
  }

  void PrintStatus(AString &s) const
  {
    s.Empty();

    // s.Add_OptSpaced("MaxByte=");
    // s.Add_UInt32(MaxByte);

    if (NonUtf)          s.Add_OptSpaced("non-UTF8");
    if (ZeroChar)        s.Add_OptSpaced("ZeroChar");
    if (SingleSurrogate) s.Add_OptSpaced("SingleSurrogate");
    if (Escape)          s.Add_OptSpaced("Escape");
    if (Truncated)       s.Add_OptSpaced("Truncated");

    if (MaxHighPoint != 0)
    {
      s.Add_OptSpaced("MaxUnicode=");
      s.Add_UInt32(MaxHighPoint);
    }
  }


  bool IsOK(bool allowReduced = false) const
  {
    if (NonUtf || SingleSurrogate || ZeroChar)
      return false;
    if (MaxHighPoint >= 0x110000)
      return false;
    if (Truncated && !allowReduced)
      return false;
    return true;
  }

  // it checks full buffer as specified in (size) and it doesn't stop on zero char
  void Check_Buf(const char *src, size_t size) throw();

  void Check_AString(const AString &s) throw()
  {
    Check_Buf(s.Ptr(), s.Len());
  }
};

/*
if (allowReduced == false) - all UTF-8 character sequences must be finished.
if (allowReduced == true)  - it allows truncated last character-Utf8-sequence
*/

bool Check_UTF8_Buf(const char *src, size_t size, bool allowReduced) throw();
bool CheckUTF8_AString(const AString &s) throw();

#define Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR    (1 << 0)
#define Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE         (1 << 1)
#define Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT (1 << 2)

/*
Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR

   if (flag is NOT set)
   {
     it processes SINGLE-SURROGATE-8 as valid Unicode point.
     it converts  SINGLE-SURROGATE-8 to SINGLE-SURROGATE-16
     Note: some sequencies of two SINGLE-SURROGATE-8 points
           will generate correct SURROGATE-16-PAIR, and
           that SURROGATE-16-PAIR later will be converted to correct
           UTF8-SURROGATE-21 point. So we don't restore original
           STR-8 sequence in that case.
   }
   
   if (flag is set)
   {
     if (Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE is defined)
        it generates ESCAPE for SINGLE-SURROGATE-8,
     if (Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE is not defined)
        it generates U+fffd for SINGLE-SURROGATE-8,
   }


Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE

   if (flag is NOT set)
     it generates (U+fffd) code for non-UTF-8 (invalid) characters

   if (flag is set)
   {
     It generates (ESCAPE) codes for NON-UTF-8 (invalid) characters.
     And later we can restore original UTF-8-RAW characters from (ESCAPE-16-21) codes.
   }

Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT

   if (flag is NOT set)
   {
     it process ESCAPE-8 points as another Unicode points.
     In Linux: ESCAPE-16 will mean two different ESCAPE-8 seqences,
       so we need HIGH-ESCAPE-PLANE-21 to restore UTF-8-RAW -> UTF-16 -> UTF-8-RAW
   }

   if (flag is set)
   {
     it generates ESCAPE-16-21 for ESCAPE-8 points
     so we can restore UTF-8-RAW -> UTF-16 -> UTF-8-RAW without HIGH-ESCAPE-PLANE-21.
   }


Main USE CASES with UTF-8 <-> UTF-16 conversions:

 WIN32:   UTF-16-RAW -> UTF-8 (Archive) -> UTF-16-RAW
   {
            set Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE
     Do NOT set Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR
     Do NOT set Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT
     
     So we restore original SINGLE-SURROGATE-16 from single SINGLE-SURROGATE-8.
   }

 Linux:   UTF-8-RAW -> UTF-16 (Intermediate / Archive) -> UTF-8-RAW
   {
     we want restore original UTF-8-RAW sequence later from that ESCAPE-16.
     Set the flags:
       Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR
       Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE
       Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT
   }

 MacOS:   UTF-8-RAW -> UTF-16 (Intermediate / Archive) -> UTF-8-RAW
   {
     we want to restore correct UTF-8 without any BMP processing:
     Set the flags:
       Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR
       Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE
   }

*/

// zero char is not allowed in (src) buf
bool Convert_UTF8_Buf_To_Unicode(const char *src, size_t srcSize, UString &dest, unsigned flags = 0);

bool ConvertUTF8ToUnicode_Flags(const AString &src, UString &dest, unsigned flags = 0);
bool ConvertUTF8ToUnicode(const AString &src, UString &dest);

#define Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR    (1 << 8)
#define Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE (1 << 9)
// #define Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE  (1 << 10)

/*
Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR

  if (flag is NOT set)
  {
     we extract SINGLE-SURROGATE as normal UTF-8
     
     In Windows : for UTF-16-RAW <-> UTF-8 (archive) <-> UTF-16-RAW in .
     
     In Linux :
       use-case-1: UTF-8 -> UTF-16 -> UTF-8  doesn't generate UTF-16 SINGLE-SURROGATE,
                   if (Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR) is used.
       use-case 2: UTF-16-7z (with SINGLE-SURROGATE from Windows) -> UTF-8 (Linux)
                   will generate SINGLE-SURROGATE-UTF-8 here.
  }

  if (flag is set)
  {
     we generate UTF_REPLACEMENT_CHAR (0xfffd) for SINGLE_SURROGATE
     it can be used for compatibility mode with WIN32 UTF function
     or if we want UTF-8 stream without any errors
  }


Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE
  
  if (flag is NOT set) it doesn't extract  raw 8-bit symbol from Escape-Plane-16
  if (flag is set)     it         extracts raw 8-bit symbol from Escape-Plane-16

  in Linux we need some way to extract NON-UTF8 RAW 8-bits from BMP (UTF-16 7z archive):
  if (we       use High-Escape-Plane), we can transfer BMP escapes to High-Escape-Plane.
  if (we don't use High-Escape-Plane), we must use Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE.
    

Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE
  // that flag affects the code only if (wchar_t is 32-bit)
  // that mode with high-escape can be disabled now in UTFConvert.cpp
  if (flag is NOT set)
     it doesn't extract raw 8-bit symbol from High-Escape-Plane
  if (flag is set)
     it        extracts raw 8-bit symbol from High-Escape-Plane

Main use cases:

WIN32 : UTF-16-RAW -> UTF-8 (archive) -> UTF-16-RAW
   {
     Do NOT set Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE.
     Do NOT set Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR.
     So we restore original UTF-16-RAW.
   }

Linix : UTF-8 with Escapes -> UTF-16 (7z archive) -> UTF-8 with Escapes
     set Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE to extract non-UTF from 7z archive
     set Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE for intermediate UTF-16.
     Note: high esacape mode can be ignored now in UTFConvert.cpp

macOS:
     the system doesn't support incorrect UTF-8 in file names.
     set Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR
*/

extern unsigned g_Unicode_To_UTF8_Flags;

void ConvertUnicodeToUTF8_Flags(const UString &src, AString &dest, unsigned flags = 0);
void ConvertUnicodeToUTF8(const UString &src, AString &dest);

void Convert_Unicode_To_UTF8_Buf(const UString &src, CByteBuffer &dest);

/*
#ifndef _WIN32
void Convert_UTF16_To_UTF32(const UString &src, UString &dest);
void Convert_UTF32_To_UTF16(const UString &src, UString &dest);
bool UTF32_IsThere_BigPoint(const UString &src);
bool Unicode_IsThere_BmpEscape(const UString &src);
#endif

bool Unicode_IsThere_Utf16SurrogateError(const UString &src);
*/

#ifdef Z7_WCHART_IS_16BIT
#define Convert_UnicodeEsc16_To_UnicodeEscHigh(s)
#else
void Convert_UnicodeEsc16_To_UnicodeEscHigh(UString &s);
#endif

/*
// #include "../../C/CpuArch.h"

// ---------- Utf16 Little endian functions ----------

// We store 16-bit surrogates even in 32-bit WCHARs in Linux.
// So now we don't use the following code:

#if WCHAR_MAX > 0xffff

// void *p     : pointer to src bytes stream
// size_t len  : num Utf16 characters : it can include or not include NULL character

inline size_t Utf16LE__Get_Num_WCHARs(const void *p, size_t len)
{
  #if WCHAR_MAX > 0xffff
  size_t num_wchars = 0;
  for (size_t i = 0; i < len; i++)
  {
    wchar_t c = GetUi16(p);
    p = (const void *)((const Byte *)p + 2);
    if (c >= 0xd800 && c < 0xdc00 && i + 1 != len)
    {
      wchar_t c2 = GetUi16(p);
      if (c2 >= 0xdc00 && c2 < 0xe000)
      {
        c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff);
        p = (const void *)((const Byte *)p + 2);
        i++;
      }
    }
    num_wchars++;
  }
  return num_wchars;
  #else
  UNUSED_VAR(p)
  return len;
  #endif
}

// #include <stdio.h>

inline wchar_t *Utf16LE__To_WCHARs_Sep(const void *p, size_t len, wchar_t *dest)
{
  for (size_t i = 0; i < len; i++)
  {
    wchar_t c = GetUi16(p);
    p = (const void *)((const Byte *)p + 2);
    
    #if WCHAR_PATH_SEPARATOR != L'/'
    if (c == L'/')
      c = WCHAR_PATH_SEPARATOR;
    #endif
    
    #if WCHAR_MAX > 0xffff
    
    if (c >= 0xd800 && c < 0xdc00 && i + 1 != len)
    {
      wchar_t c2 = GetUi16(p);
      if (c2 >= 0xdc00 && c2 < 0xe000)
      {
        // printf("\nSurragate : %4x %4x -> ", (int)c, (int)c2);
        c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff);
        p = (const void *)((const Byte *)p + 2);
        i++;
        // printf("%4x\n", (int)c);
      }
    }
    
    #endif
    
    *dest++ = c;
  }
  return dest;
}


inline size_t Get_Num_Utf16_chars_from_wchar_string(const wchar_t *p)
{
  size_t num = 0;
  for (;;)
  {
    wchar_t c = *p++;
    if (c == 0)
      return num;
    num += ((c >= 0x10000 && c < 0x110000) ? 2 : 1);
  }
  return num;
}

inline Byte *wchars_to_Utf16LE(const wchar_t *p, Byte *dest)
{
  for (;;)
  {
    wchar_t c = *p++;
    if (c == 0)
      return dest;
    if (c >= 0x10000 && c < 0x110000)
    {
      SetUi16(dest    , (UInt16)(0xd800 + ((c >> 10) & 0x3FF)));
      SetUi16(dest + 2, (UInt16)(0xdc00 + ( c        & 0x3FF)));
      dest += 4;
    }
    else
    {
      SetUi16(dest, c);
      dest += 2;
    }
  }
}

#endif
*/

#endif

/* ---- CPP/Windows/PropVariant.h ---- */
// Windows/PropVariant.h

#ifndef ZIP7_INC_WINDOWS_PROP_VARIANT_H
#define ZIP7_INC_WINDOWS_PROP_VARIANT_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NWindows {
namespace NCOM {

BSTR AllocBstrFromAscii(const char *s) throw();

HRESULT PropVariant_Clear(PROPVARIANT *p) throw();

HRESULT PropVarEm_Alloc_Bstr(PROPVARIANT *p, unsigned numChars) throw();
HRESULT PropVarEm_Set_Str(PROPVARIANT *p, const char *s) throw();

inline void PropVarEm_Set_UInt32(PROPVARIANT *p, UInt32 v) throw()
{
  p->vt = VT_UI4;
  p->ulVal = v;
}

inline void PropVarEm_Set_UInt64(PROPVARIANT *p, UInt64 v) throw()
{
  p->vt = VT_UI8;
  p->uhVal.QuadPart = v;
}

inline void PropVarEm_Set_FileTime64_Prec(PROPVARIANT *p, UInt64 v, unsigned prec) throw()
{
  p->vt = VT_FILETIME;
  p->filetime.dwLowDateTime = (DWORD)v;
  p->filetime.dwHighDateTime = (DWORD)(v >> 32);
  p->wReserved1 = (WORD)prec;
  p->wReserved2 = 0;
  p->wReserved3 = 0;
}

inline void PropVarEm_Set_Bool(PROPVARIANT *p, bool b) throw()
{
  p->vt = VT_BOOL;
  p->boolVal = (b ? VARIANT_TRUE : VARIANT_FALSE);
}


class CPropVariant : public tagPROPVARIANT
{
  // ---------- forbidden functions ----------
  CPropVariant(const char *s);
  // CPropVariant(const UString &s);
 #ifdef DEBUG_FSTRING_INHERITS_ASTRING
  CPropVariant(const FString &s);
  CPropVariant& operator=(const FString &s);
 #endif

public:
  CPropVariant()
  {
    vt = VT_EMPTY;
    wReserved1 = 0;
    // wReserved2 = 0;
    // wReserved3 = 0;
    // uhVal.QuadPart = 0;
    bstrVal = NULL;
  }


  void Set_FtPrec(unsigned prec)
  {
    wReserved1 = (WORD)prec;
    wReserved2 = 0;
    wReserved3 = 0;
  }

  void SetAsTimeFrom_FT_Prec(const FILETIME &ft, unsigned prec)
  {
    operator=(ft);
    Set_FtPrec(prec);
  }

  void SetAsTimeFrom_Ft64_Prec(UInt64 v, unsigned prec)
  {
    FILETIME ft;
    ft.dwLowDateTime = (DWORD)(UInt32)v;
    ft.dwHighDateTime = (DWORD)(UInt32)(v >> 32);
    operator=(ft);
    Set_FtPrec(prec);
  }

  void SetAsTimeFrom_FT_Prec_Ns100(const FILETIME &ft, unsigned prec, unsigned ns100)
  {
    operator=(ft);
    wReserved1 = (WORD)prec;
    wReserved2 = (WORD)ns100;
    wReserved3 = 0;
  }

  unsigned Get_Ns100() const
  {
    const unsigned prec = wReserved1;
    const unsigned ns100 = wReserved2;
    if (prec == 0
        && prec <= k_PropVar_TimePrec_1ns
        && ns100 < 100
        && wReserved3 == 0)
      return ns100;
    return 0;
  }

  ~CPropVariant() throw();
  CPropVariant(const PROPVARIANT &varSrc);
  CPropVariant(const CPropVariant &varSrc);
  CPropVariant(BSTR bstrSrc);
  CPropVariant(LPCOLESTR lpszSrc);
  CPropVariant(bool bSrc) { vt = VT_BOOL; wReserved1 = 0; boolVal = (bSrc ? VARIANT_TRUE : VARIANT_FALSE); }
  CPropVariant(Byte value) { vt = VT_UI1; wReserved1 = 0; bVal = value; }

private:
  CPropVariant(UInt16 value); // { vt = VT_UI2; wReserved1 = 0; uiVal = value; }
  CPropVariant(Int16 value); // { vt = VT_I2; wReserved1 = 0; iVal = value; }
  CPropVariant(Int32 value); // { vt = VT_I4; wReserved1 = 0; lVal = value; }
  CPropVariant(Int64 value); // { vt = VT_I8; wReserved1 = 0; hVal.QuadPart = value; }

public:
  CPropVariant(UInt32 value) { vt = VT_UI4; wReserved1 = 0; ulVal = value; }
  CPropVariant(UInt64 value) { vt = VT_UI8; wReserved1 = 0; uhVal.QuadPart = value; }
  CPropVariant(const FILETIME &value) { vt = VT_FILETIME; wReserved1 = 0; filetime = value; }

  CPropVariant& operator=(const CPropVariant &varSrc);
  CPropVariant& operator=(const PROPVARIANT &varSrc);
  CPropVariant& operator=(BSTR bstrSrc);
  CPropVariant& operator=(LPCOLESTR lpszSrc);
  CPropVariant& operator=(const UString &s);
  CPropVariant& operator=(const UString2 &s);
  CPropVariant& operator=(const char *s);
  CPropVariant& operator=(const AString &s)
    { return (*this)=(const char *)s; }
  
  CPropVariant& operator=(bool bSrc) throw();
  CPropVariant& operator=(Byte value) throw();
  
private:
  CPropVariant& operator=(Int16 value) throw();
  CPropVariant& operator=(UInt16 value) throw();
  CPropVariant& operator=(Int32 value) throw();
  CPropVariant& operator=(Int64 value) throw();

public:
  CPropVariant& operator=(UInt32 value) throw();
  CPropVariant& operator=(UInt64 value) throw();
  CPropVariant& operator=(const FILETIME &value) throw();

  void Set_Int32(Int32 value) throw();
  void Set_Int64(Int64 value) throw();

  BSTR AllocBstr(unsigned numChars);

  HRESULT Clear() throw();
  HRESULT Copy(const PROPVARIANT *pSrc) throw();
  HRESULT Attach(PROPVARIANT *pSrc) throw();
  HRESULT Detach(PROPVARIANT *pDest) throw();

  HRESULT InternalClear() throw();
  void InternalCopy(const PROPVARIANT *pSrc);
  int Compare(const CPropVariant &a) throw();
};

}}

#endif

/* ---- CPP/Windows/TimeUtils.h ---- */
// Windows/TimeUtils.h

#ifndef ZIP7_INC_WINDOWS_TIME_UTILS_H
#define ZIP7_INC_WINDOWS_TIME_UTILS_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

inline UInt64 FILETIME_To_UInt64(const FILETIME &ft)
{
  return (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
}

inline void FILETIME_Clear(FILETIME &ft)
{
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;
}

inline bool FILETIME_IsZero(const FILETIME &ft)
{
  return (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0);
}


#ifdef _WIN32
  #define CFiTime FILETIME
  #define Compare_FiTime ::CompareFileTime
  inline void FiTime_To_FILETIME(const CFiTime &ts, FILETIME &ft)
  {
    ft = ts;
  }
  /*
  inline void FILETIME_To_FiTime(const FILETIME &ft, CFiTime &ts)
  {
    ts = ft;
  }
  */
  inline void FiTime_Clear(CFiTime &ft)
  {
    ft.dwLowDateTime = 0;
    ft.dwHighDateTime = 0;
  }
#else

  #include <sys/stat.h>

 #if defined(_AIX)
   #define CFiTime st_timespec
 #else
   #define CFiTime timespec
 #endif
  int Compare_FiTime(const CFiTime *a1, const CFiTime *a2);
  bool FILETIME_To_timespec(const FILETIME &ft, CFiTime &ts);
  void FiTime_To_FILETIME(const CFiTime &ts, FILETIME &ft);
  void FiTime_To_FILETIME_ns100(const CFiTime &ts, FILETIME &ft, unsigned &ns100);
  inline void FiTime_Clear(CFiTime &ft)
  {
    ft.tv_sec = 0;
    ft.tv_nsec = 0;
  }

 #ifdef __APPLE__
  #define ST_MTIME(st) st.st_mtimespec
  #define ST_ATIME(st) st.st_atimespec
  #define ST_CTIME(st) st.st_ctimespec
 #elif defined(__QNXNTO__) && defined(__ARM__) && !defined(__aarch64__)
  // QNX armv7le (32-bit) for "struct stat" timestamps uses time_t instead of timespec
  inline CFiTime ST_MTIME(const struct stat &st)
    { timespec ts;  ts.tv_sec = st.st_mtime; ts.tv_nsec = 0;  return ts; }
  inline CFiTime ST_ATIME(const struct stat &st)
    { timespec ts;  ts.tv_sec = st.st_atime; ts.tv_nsec = 0;  return ts; }
  inline CFiTime ST_CTIME(const struct stat &st)
    { timespec ts;  ts.tv_sec = st.st_ctime; ts.tv_nsec = 0;  return ts; }
 #else
  #define ST_MTIME(st) st.st_mtim
  #define ST_ATIME(st) st.st_atim
  #define ST_CTIME(st) st.st_ctim
 #endif

#endif

// void FiTime_Normalize_With_Prec(CFiTime &ft, unsigned prec);

namespace NWindows {
namespace NTime {

bool DosTime_To_FileTime(UInt32 dosTime, FILETIME &fileTime) throw();
bool UtcFileTime_To_LocalDosTime(const FILETIME &utc, UInt32 &dosTime) throw();
bool FileTime_To_DosTime(const FILETIME &fileTime, UInt32 &dosTime) throw();

// UInt32 Unix Time : for dates 1970-2106
UInt64 UnixTime_To_FileTime64(UInt32 unixTime) throw();
void UnixTime_To_FileTime(UInt32 unixTime, FILETIME &fileTime) throw();

// Int64 Unix Time : negative values for dates before 1970
UInt64 UnixTime64_To_FileTime64(Int64 unixTime) throw(); // no check
bool UnixTime64_To_FileTime64(Int64 unixTime, UInt64 &fileTime) throw();
bool UnixTime64_To_FileTime(Int64 unixTime, FILETIME &fileTime) throw();

Int64 FileTime64_To_UnixTime64(UInt64 ft64) throw();
bool FileTime_To_UnixTime(const FILETIME &fileTime, UInt32 &unixTime) throw();
Int64 FileTime_To_UnixTime64(const FILETIME &ft) throw();
Int64 FileTime_To_UnixTime64_and_Quantums(const FILETIME &ft, UInt32 &quantums) throw();

bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds) throw();

void GetCurUtc_FiTime(CFiTime &ft) throw();
#ifdef _WIN32
#define GetCurUtcFileTime GetCurUtc_FiTime
#else
void GetCurUtcFileTime(FILETIME &ft) throw();
#endif

}}

inline void PropVariant_SetFrom_UnixTime(NWindows::NCOM::CPropVariant &prop, UInt32 unixTime)
{
  FILETIME ft;
  NWindows::NTime::UnixTime_To_FileTime(unixTime, ft);
  prop.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_Unix);
}

inline void PropVariant_SetFrom_NtfsTime(NWindows::NCOM::CPropVariant &prop, const FILETIME &ft)
{
  prop.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_100ns);
}

inline void PropVariant_SetFrom_FiTime(NWindows::NCOM::CPropVariant &prop, const CFiTime &fts)
{
 #ifdef _WIN32
  PropVariant_SetFrom_NtfsTime(prop, fts);
 #else
  unsigned ns100;
  FILETIME ft;
  FiTime_To_FILETIME_ns100(fts, ft, ns100);
  prop.SetAsTimeFrom_FT_Prec_Ns100(ft, k_PropVar_TimePrec_1ns, ns100);
 #endif
}

inline bool PropVariant_SetFrom_DosTime(NWindows::NCOM::CPropVariant &prop, UInt32 dosTime)
{
  FILETIME localFileTime, utc;
  if (!NWindows::NTime::DosTime_To_FileTime(dosTime, localFileTime))
    return false;
  if (!LocalFileTimeToFileTime(&localFileTime, &utc))
    return false;
  prop.SetAsTimeFrom_FT_Prec(utc, k_PropVar_TimePrec_DOS);
  return true;
}

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

/* ---- CPP/Common/MyCom.h ---- */
// MyCom.h

#ifndef ZIP7_INC_MY_COM_H
#define ZIP7_INC_MY_COM_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

template <class T>
class CMyComPtr
{
  T* _p;
public:
  CMyComPtr(): _p(NULL) {}
  CMyComPtr(T* p) throw() { if ((_p = p) != NULL) p->AddRef(); }
  CMyComPtr(const CMyComPtr<T>& lp) throw() { if ((_p = lp._p) != NULL) _p->AddRef(); }
  ~CMyComPtr() { if (_p) _p->Release(); }
  void Release() { if (_p) { _p->Release(); _p = NULL; } }
  operator T*() const {  return (T*)_p;  }
  T* Interface() const {  return (T*)_p;  }
  // T& operator*() const {  return *_p; }
  T** operator&() { return &_p; }
  T* operator->() const { return _p; }
  T* operator=(T* p)
  {
    if (p)
      p->AddRef();
    if (_p)
      _p->Release();
    _p = p;
    return p;
  }
  T* operator=(const CMyComPtr<T>& lp) { return (*this = lp._p); }
  bool operator!() const { return (_p == NULL); }
  // bool operator==(T* pT) const {  return _p == pT; }
  void Attach(T* p2)
  {
    Release();
    _p = p2;
  }
  T* Detach()
  {
    T* pt = _p;
    _p = NULL;
    return pt;
  }
  #ifdef _WIN32
  HRESULT CoCreateInstance(REFCLSID rclsid, REFIID iid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
  {
    return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, (void**)&_p);
  }
  #endif
  /*
  HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
  {
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
    ATLASSERT(_p == NULL);
    if (SUCCEEDED(hr))
      hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&_p);
    return hr;
  }
  */
  template <class Q>
  HRESULT QueryInterface(REFGUID iid, Q** pp) const throw()
  {
    // if (*pp) throw 20220216; // for debug
    return _p->QueryInterface(iid, (void**)pp);
  }
};


template <class iface, class cls>
class CMyComPtr2
{
  cls* _p;
  
  CMyComPtr2(const CMyComPtr2<iface, cls>& lp);
  CMyComPtr2(cls* p);
  CMyComPtr2(iface* p);
  iface* operator=(const CMyComPtr2<iface, cls>& lp);
  iface* operator=(cls* p);
  iface* operator=(iface* p);
public:
  CMyComPtr2(): _p(NULL) {}
  ~CMyComPtr2()
  {
    if (_p)
    {
      iface *ip = _p;
      ip->Release();
    }
  }
  // void Release() { if (_p) { (iface *)_p->Release(); _p = NULL; } }
  cls* operator->() const { return _p; }
  cls* ClsPtr() const { return _p; }
  operator iface*() const
  {
    iface *ip = _p;
    return ip;
  }
  iface* Interface() const
  {
    iface *ip = _p;
    return ip;
  }
  // operator bool() const {  return _p != NULL; }
  bool IsDefined() const {  return _p != NULL; }
  void Create_if_Empty()
  {
    if (!_p)
    {
      _p = new cls;
      iface *ip = _p;
      ip->AddRef();
    }
  }
  iface* Detach()
  {
    iface *ip = _p;
    _p = NULL;
    return ip;
  }
  void SetFromCls(cls *src)
  {
    if (src)
    {
      iface *ip = src;
      ip->AddRef();
    }
    if (_p)
    {
      iface *ip = _p;
      ip->Release();
    }
    _p = src;
  }
};


template <class iface, class cls>
class CMyComPtr2_Create
{
  cls* _p;

  CMyComPtr2_Create(const CMyComPtr2_Create<iface, cls>& lp);
  CMyComPtr2_Create(cls* p);
  CMyComPtr2_Create(iface* p);
  iface* operator=(const CMyComPtr2_Create<iface, cls>& lp);
  iface* operator=(cls* p);
  iface* operator=(iface* p);
public:
  CMyComPtr2_Create(): _p(new cls)
  {
    iface *ip = _p;
    ip->AddRef();
  }
  ~CMyComPtr2_Create()
  {
    iface *ip = _p;
    ip->Release();
  }
  cls* operator->() const { return _p; }
  cls* ClsPtr() const { return _p; }
  operator iface*() const
  {
    iface *ip = _p;
    return ip;
  }
  iface* Interface() const
  {
    iface *ip = _p;
    return ip;
  }
};


#define Z7_DECL_CMyComPtr_QI_FROM(i, v, unk) \
  CMyComPtr<i> v; (unk)->QueryInterface(IID_ ## i, (void **)&v);


//////////////////////////////////////////////////////////

inline HRESULT StringToBstr(LPCOLESTR src, BSTR *bstr)
{
  *bstr = ::SysAllocString(src);
  return (*bstr) ? S_OK : E_OUTOFMEMORY;
}

class CMyComBSTR
{
  BSTR m_str;
  Z7_CLASS_NO_COPY(CMyComBSTR)
public:
  CMyComBSTR(): m_str(NULL) {}
  ~CMyComBSTR() { ::SysFreeString(m_str); }
  BSTR* operator&() { return &m_str; }
  operator LPCOLESTR() const { return m_str; }
  // operator bool() const { return m_str != NULL; }
  // bool operator!() const { return m_str == NULL; }

  void Wipe_and_Free()
  {
    if (m_str)
    {
      memset(m_str, 0, ::SysStringLen(m_str) * sizeof(*m_str));
      Empty();
    }
  }

private:
  // operator BSTR() const { return m_str; }

  CMyComBSTR(LPCOLESTR src) { m_str = ::SysAllocString(src); }
  // CMyComBSTR(int nSize) { m_str = ::SysAllocStringLen(NULL, nSize); }
  // CMyComBSTR(int nSize, LPCOLESTR sz) { m_str = ::SysAllocStringLen(sz, nSize);  }
  // CMyComBSTR(const CMyComBSTR& src) { m_str = src.MyCopy(); }
  
  /*
  CMyComBSTR(REFGUID src)
  {
    LPOLESTR szGuid;
    StringFromCLSID(src, &szGuid);
    m_str = ::SysAllocString(szGuid);
    CoTaskMemFree(szGuid);
  }
  */
  
  /*
  CMyComBSTR& operator=(const CMyComBSTR& src)
  {
    if (m_str != src.m_str)
    {
      if (m_str)
        ::SysFreeString(m_str);
      m_str = src.MyCopy();
    }
    return *this;
  }
  */
  
  CMyComBSTR& operator=(LPCOLESTR src)
  {
    ::SysFreeString(m_str);
    m_str = ::SysAllocString(src);
    return *this;
  }
  
  unsigned Len() const { return ::SysStringLen(m_str); }

  BSTR MyCopy() const
  {
    // We don't support Byte BSTRs here
    return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
    /*
    UINT byteLen = ::SysStringByteLen(m_str);
    BSTR res = ::SysAllocStringByteLen(NULL, byteLen);
    if (res && byteLen != 0 && m_str)
      memcpy(res, m_str, byteLen);
    return res;
    */
  }
  
  /*
  void Attach(BSTR src) { m_str = src; }
  BSTR Detach()
  {
    BSTR s = m_str;
    m_str = NULL;
    return s;
  }
  */

  void Empty()
  {
    ::SysFreeString(m_str);
    m_str = NULL;
  }
};


class CMyComBSTR_Wipe: public CMyComBSTR
{
  Z7_CLASS_NO_COPY(CMyComBSTR_Wipe)
public:
  CMyComBSTR_Wipe(): CMyComBSTR() {}
  ~CMyComBSTR_Wipe() { Wipe_and_Free(); }
};



/*
  If CMyUnknownImp doesn't use virtual destructor, the code size is smaller.
  But if some class_1 derived from CMyUnknownImp
    uses Z7_COM_ADDREF_RELEASE and IUnknown::Release()
    and some another class_2 is derived from class_1,
    then class_1 must use virtual destructor:
      virtual ~class_1();
    In that case, class_1::Release() calls correct destructor of class_2.
  We can use virtual ~CMyUnknownImp() to disable warning
    "class has virtual functions, but destructor is not virtual".
  Also we can use virtual ~IUnknown() {} in MyWindows.h
*/

class CMyUnknownImp
{
  Z7_CLASS_NO_COPY(CMyUnknownImp)
protected:
  ULONG _m_RefCount;
  CMyUnknownImp(): _m_RefCount(0) {}

  #ifdef _WIN32
  #if defined(__GNUC__) || defined(__clang__)
  // virtual ~CMyUnknownImp() {} // to disable GCC/CLANG varnings
  #endif
  #endif
};



#define Z7_COM_QI_BEGIN \
  private: STDMETHOD(QueryInterface) (REFGUID iid, void **outObject) throw() Z7_override Z7_final \
    { *outObject = NULL;

#define Z7_COM_QI_ENTRY(i) \
  else if (iid == IID_ ## i) \
    { i *ti = this;  *outObject = ti; }
//   { *outObject = (void *)(i *)this; }

#define Z7_COM_QI_ENTRY_UNKNOWN_0 \
  if (iid == IID_IUnknown) \
    { IUnknown *tu = this;  *outObject = tu; }

#define Z7_COM_QI_ENTRY_UNKNOWN(i) \
  if (iid == IID_IUnknown) \
    { i *ti = this;  IUnknown *tu = ti;  *outObject = tu; }
//    { *outObject = (void *)(IUnknown *)(i *)this; }

#define Z7_COM_QI_BEGIN2(i) \
  Z7_COM_QI_BEGIN \
  Z7_COM_QI_ENTRY_UNKNOWN(i) \
  Z7_COM_QI_ENTRY(i)


#define Z7_COM_ADDREF_RELEASE_MT \
  private: \
  STDMETHOD_(ULONG, AddRef)() Z7_override Z7_final \
    { return (ULONG)InterlockedIncrement((LONG *)&_m_RefCount); } \
  STDMETHOD_(ULONG, Release)() Z7_override Z7_final \
    { const LONG v = InterlockedDecrement((LONG *)&_m_RefCount); \
      if (v != 0) return (ULONG)v; \
      delete this;  return 0; }

#define Z7_COM_QI_END_MT \
  else return E_NOINTERFACE; \
  InterlockedIncrement((LONG *)&_m_RefCount); /* AddRef(); */ return S_OK; }

// you can define Z7_COM_USE_ATOMIC,
// if you want to call Release() from different threads (for example, for .NET code)
// #define Z7_COM_USE_ATOMIC

#if defined(Z7_COM_USE_ATOMIC) && !defined(Z7_ST)

#ifndef _WIN32
#if 0
// amalgamation: header emitted in prologue
#else
EXTERN_C_BEGIN
LONG InterlockedIncrement(LONG volatile *addend);
LONG InterlockedDecrement(LONG volatile *addend);
EXTERN_C_END
#endif
#endif // _WIN32

#define Z7_COM_ADDREF_RELEASE  Z7_COM_ADDREF_RELEASE_MT
#define Z7_COM_QI_END          Z7_COM_QI_END_MT

#else // !Z7_COM_USE_ATOMIC

#define Z7_COM_ADDREF_RELEASE \
  private: \
  STDMETHOD_(ULONG, AddRef)() throw() Z7_override Z7_final \
    { return ++_m_RefCount; } \
  STDMETHOD_(ULONG, Release)() throw() Z7_override Z7_final \
    { if (--_m_RefCount != 0) return _m_RefCount; \
      delete this;  return 0; }

#define Z7_COM_QI_END \
  else return E_NOINTERFACE; \
  ++_m_RefCount; /* AddRef(); */ return S_OK; }

#endif // !Z7_COM_USE_ATOMIC


#define Z7_COM_UNKNOWN_IMP_SPEC(i) \
  Z7_COM_QI_BEGIN \
  i \
  Z7_COM_QI_END \
  Z7_COM_ADDREF_RELEASE


#define Z7_COM_UNKNOWN_IMP_0 \
  Z7_COM_QI_BEGIN \
  Z7_COM_QI_ENTRY_UNKNOWN_0 \
  Z7_COM_QI_END \
  Z7_COM_ADDREF_RELEASE

#define Z7_COM_UNKNOWN_IMP_1(i) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i) \
  Z7_COM_QI_ENTRY(i) \
  )

#define Z7_COM_UNKNOWN_IMP_2(i1, i2) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  )

#define Z7_COM_UNKNOWN_IMP_3(i1, i2, i3) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  Z7_COM_QI_ENTRY(i3) \
  )

#define Z7_COM_UNKNOWN_IMP_4(i1, i2, i3, i4) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  Z7_COM_QI_ENTRY(i3) \
  Z7_COM_QI_ENTRY(i4) \
  )

#define Z7_COM_UNKNOWN_IMP_5(i1, i2, i3, i4, i5) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  Z7_COM_QI_ENTRY(i3) \
  Z7_COM_QI_ENTRY(i4) \
  Z7_COM_QI_ENTRY(i5) \
  )

#define Z7_COM_UNKNOWN_IMP_6(i1, i2, i3, i4, i5, i6) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  Z7_COM_QI_ENTRY(i3) \
  Z7_COM_QI_ENTRY(i4) \
  Z7_COM_QI_ENTRY(i5) \
  Z7_COM_QI_ENTRY(i6) \
  )

#define Z7_COM_UNKNOWN_IMP_7(i1, i2, i3, i4, i5, i6, i7) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  Z7_COM_QI_ENTRY(i3) \
  Z7_COM_QI_ENTRY(i4) \
  Z7_COM_QI_ENTRY(i5) \
  Z7_COM_QI_ENTRY(i6) \
  Z7_COM_QI_ENTRY(i7) \
  )

#define Z7_COM_UNKNOWN_IMP_8(i1, i2, i3, i4, i5, i6, i7, i8) \
  Z7_COM_UNKNOWN_IMP_SPEC( \
  Z7_COM_QI_ENTRY_UNKNOWN(i1) \
  Z7_COM_QI_ENTRY(i1) \
  Z7_COM_QI_ENTRY(i2) \
  Z7_COM_QI_ENTRY(i3) \
  Z7_COM_QI_ENTRY(i4) \
  Z7_COM_QI_ENTRY(i5) \
  Z7_COM_QI_ENTRY(i6) \
  Z7_COM_QI_ENTRY(i7) \
  Z7_COM_QI_ENTRY(i8) \
  )


#define Z7_IFACES_IMP_UNK_1(i1) \
  Z7_COM_UNKNOWN_IMP_1(i1) \
  Z7_IFACE_COM7_IMP(i1) \

#define Z7_IFACES_IMP_UNK_2(i1, i2) \
  Z7_COM_UNKNOWN_IMP_2(i1, i2) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \

#define Z7_IFACES_IMP_UNK_3(i1, i2, i3) \
  Z7_COM_UNKNOWN_IMP_3(i1, i2, i3) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \

#define Z7_IFACES_IMP_UNK_4(i1, i2, i3, i4) \
  Z7_COM_UNKNOWN_IMP_4(i1, i2, i3, i4) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  Z7_IFACE_COM7_IMP(i4) \

#define Z7_IFACES_IMP_UNK_5(i1, i2, i3, i4, i5) \
  Z7_COM_UNKNOWN_IMP_5(i1, i2, i3, i4, i5) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  Z7_IFACE_COM7_IMP(i4) \
  Z7_IFACE_COM7_IMP(i5) \

#define Z7_IFACES_IMP_UNK_6(i1, i2, i3, i4, i5, i6) \
  Z7_COM_UNKNOWN_IMP_6(i1, i2, i3, i4, i5, i6) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  Z7_IFACE_COM7_IMP(i4) \
  Z7_IFACE_COM7_IMP(i5) \
  Z7_IFACE_COM7_IMP(i6) \

#define Z7_IFACES_IMP_UNK_7(i1, i2, i3, i4, i5, i6, i7) \
  Z7_COM_UNKNOWN_IMP_7(i1, i2, i3, i4, i5, i6, i7) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  Z7_IFACE_COM7_IMP(i4) \
  Z7_IFACE_COM7_IMP(i5) \
  Z7_IFACE_COM7_IMP(i6) \
  Z7_IFACE_COM7_IMP(i7) \


#define Z7_CLASS_IMP_COM_0(c) \
  Z7_class_final(c) : \
  public IUnknown, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_0 \
  private:

#define Z7_CLASS_IMP_COM_1(c, i1) \
  Z7_class_final(c) : \
  public i1, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_1(i1) \
  private:

#define Z7_CLASS_IMP_COM_2(c, i1, i2) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_2(i1, i2) \
  private:

#define Z7_CLASS_IMP_COM_3(c, i1, i2, i3) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_3(i1, i2, i3) \
  private:

#define Z7_CLASS_IMP_COM_4(c, i1, i2, i3, i4) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public i4, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_4(i1, i2, i3, i4) \
  private:

#define Z7_CLASS_IMP_COM_5(c, i1, i2, i3, i4, i5) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public i4, \
  public i5, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_5(i1, i2, i3, i4, i5) \
  private:

#define Z7_CLASS_IMP_COM_6(c, i1, i2, i3, i4, i5, i6) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public i4, \
  public i5, \
  public i6, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_6(i1, i2, i3, i4, i5, i6) \
  private:


#define Z7_CLASS_IMP_COM_7(c, i1, i2, i3, i4, i5, i6, i7) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public i4, \
  public i5, \
  public i6, \
  public i7, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_7(i1, i2, i3, i4, i5, i6, i7) \
  private:


/*
#define Z7_CLASS_IMP_NOQIB_0(c) \
  Z7_class_final(c) : \
  public IUnknown, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_0 \
  private:
*/

#define Z7_CLASS_IMP_NOQIB_1(c, i1) \
  Z7_class_final(c) : \
  public i1, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_0 \
  Z7_IFACE_COM7_IMP(i1) \
  private:

#define Z7_CLASS_IMP_NOQIB_2(c, i1, i2) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_1(i2) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  private:

#define Z7_CLASS_IMP_NOQIB_3(c, i1, i2, i3) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_2(i2, i3) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  private:

#define Z7_CLASS_IMP_NOQIB_4(c, i1, i2, i3, i4) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public i4, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_3(i2, i3, i4) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  Z7_IFACE_COM7_IMP(i4) \

/*
#define Z7_CLASS_IMP_NOQIB_5(c, i1, i2, i3, i4, i5) \
  Z7_class_final(c) : \
  public i1, \
  public i2, \
  public i3, \
  public i4, \
  public i5, \
  public CMyUnknownImp { \
  Z7_COM_UNKNOWN_IMP_4(i2, i3, i4, i5) \
  Z7_IFACE_COM7_IMP(i1) \
  Z7_IFACE_COM7_IMP(i2) \
  Z7_IFACE_COM7_IMP(i3) \
  Z7_IFACE_COM7_IMP(i4) \
  Z7_IFACE_COM7_IMP(i5) \
*/


#define Z7_CLASS_IMP_IInStream(c) \
  class c Z7_final : \
  public IInStream, \
  public CMyUnknownImp { \
  Z7_IFACES_IMP_UNK_2(ISequentialInStream, IInStream) \


#define k_My_HRESULT_WritingWasCut 0x20000010

#endif

/* ---- CPP/Common/MyUnknown.h ---- */
// MyUnknown.h

#ifndef ZIP7_INC_MY_UNKNOWN_H
#define ZIP7_INC_MY_UNKNOWN_H

// amalgamation: header emitted in prologue

#endif

/* ---- CPP/7zip/IDecl.h ---- */
// IDecl.h

#ifndef ZIP7_INC_IDECL_H
#define ZIP7_INC_IDECL_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define k_7zip_GUID_Data1 0x23170F69
#define k_7zip_GUID_Data2 0x40C1

#define k_7zip_GUID_Data3_Common  0x278A

#define k_7zip_GUID_Data3_Decoder 0x2790
#define k_7zip_GUID_Data3_Encoder 0x2791
#define k_7zip_GUID_Data3_Hasher  0x2792

#define Z7_DECL_IFACE_7ZIP_SUB(i, _base, groupId, subId) \
  Z7_DEFINE_GUID(IID_ ## i, \
    k_7zip_GUID_Data1, \
    k_7zip_GUID_Data2, \
    k_7zip_GUID_Data3_Common, \
    0, 0, 0, (groupId), 0, (subId), 0, 0); \
    struct Z7_DECLSPEC_NOVTABLE i: public _base
 
#define Z7_DECL_IFACE_7ZIP(i,           groupId, subId) \
    Z7_DECL_IFACE_7ZIP_SUB(i, IUnknown, groupId, subId)


#ifdef COM_DECLSPEC_NOTHROW
#define Z7_COMWF_B        COM_DECLSPEC_NOTHROW STDMETHODIMP
#define Z7_COMWF_B_(t)    COM_DECLSPEC_NOTHROW STDMETHODIMP_(t)
#else
#define Z7_COMWF_B        STDMETHODIMP
#define Z7_COMWF_B_(t)    STDMETHODIMP_(t)
#endif

#if defined(_MSC_VER) && !defined(COM_DECLSPEC_NOTHROW)
#define Z7_COM7F_B        __declspec(nothrow) STDMETHODIMP
#define Z7_COM7F_B_(t)    __declspec(nothrow) STDMETHODIMP_(t)
#else
#define Z7_COM7F_B        Z7_COMWF_B
#define Z7_COM7F_B_(t)    Z7_COMWF_B_(t)
#endif

// #define Z7_COM7F_E            Z7_noexcept
#define Z7_COM7F_E            throw()
#define Z7_COM7F_EO           Z7_COM7F_E  Z7_override
#define Z7_COM7F_EOF          Z7_COM7F_EO Z7_final
#define Z7_COM7F_IMF(f)       Z7_COM7F_B     f Z7_COM7F_E
#define Z7_COM7F_IMF2(t, f)   Z7_COM7F_B_(t) f Z7_COM7F_E

#define Z7_COM7F_PURE(f)              virtual Z7_COM7F_IMF(f) =0;
#define Z7_COM7F_PURE2(t, f)          virtual Z7_COM7F_IMF2(t, f) =0;
#define Z7_COM7F_IMP(f)               Z7_COM7F_IMF(f)     Z7_override Z7_final;
#define Z7_COM7F_IMP2(t, f)           Z7_COM7F_IMF2(t, f) Z7_override Z7_final;
#define Z7_COM7F_IMP_NONFINAL(f)      Z7_COM7F_IMF(f)     Z7_override;
#define Z7_COM7F_IMP_NONFINAL2(t, f)  Z7_COM7F_IMF2(t, f) Z7_override;

#define Z7_IFACE_PURE(name)               Z7_IFACEN_ ## name(=0;)
#define Z7_IFACE_IMP(name)                Z7_IFACEN_ ## name(Z7_override Z7_final;)

#define Z7_IFACE_COM7_PURE(name)          Z7_IFACEM_ ## name(Z7_COM7F_PURE)
#define Z7_IFACE_COM7_IMP(name)           Z7_IFACEM_ ## name(Z7_COM7F_IMP)
#define Z7_IFACE_COM7_IMP_NONFINAL(name)  Z7_IFACEM_ ## name(Z7_COM7F_IMP_NONFINAL)


#define Z7_IFACE_DECL_PURE(name) \
    DECLARE_INTERFACE(name) \
    { Z7_IFACE_PURE(name) };

#define Z7_IFACE_DECL_PURE_(name, baseiface) \
    DECLARE_INTERFACE_(name, baseiface) \
    { Z7_IFACE_PURE(name) };

#endif

/* ---- CPP/7zip/IStream.h ---- */
// IStream.h

#ifndef ZIP7_INC_ISTREAM_H
#define ZIP7_INC_ISTREAM_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_PURE_INTERFACES_BEGIN

#define Z7_IFACE_CONSTR_STREAM_SUB(i, base, n) \
  Z7_DECL_IFACE_7ZIP_SUB(i, base, 3, n) \
  { Z7_IFACE_COM7_PURE(i) };

#define Z7_IFACE_CONSTR_STREAM(i, n) \
        Z7_IFACE_CONSTR_STREAM_SUB(i, IUnknown, n)


/*
ISequentialInStream::Read()
  The requirement for caller: (processedSize != NULL).
  The callee can allow (processedSize == NULL) for compatibility reasons.

  if (size == 0), this function returns S_OK and (*processedSize) is set to 0.

  if (size != 0)
  {
    Partial read is allowed: (*processedSize <= avail_size && *processedSize <= size),
      where (avail_size) is the size of remaining bytes in stream.
    If (avail_size != 0), this function must read at least 1 byte: (*processedSize > 0).
    You must call Read() in loop, if you need to read exact amount of data.
  }

  If seek pointer before Read() call was changed to position past the end of stream:
    if (seek_pointer >= stream_size), this function returns S_OK and (*processedSize) is set to 0.

  ERROR CASES:
    If the function returns error code, then (*processedSize) is size of
    data written to (data) buffer (it can be data before error or data with errors).
    The recommended way for callee to work with reading errors:
      1) write part of data before error to (data) buffer and return S_OK.
      2) return error code for further calls of Read().
*/
#define Z7_IFACEM_ISequentialInStream(x) \
  x(Read(void *data, UInt32 size, UInt32 *processedSize))
Z7_IFACE_CONSTR_STREAM(ISequentialInStream, 0x01)


/*
ISequentialOutStream::Write()
  The requirement for caller: (processedSize != NULL).
  The callee can allow (processedSize == NULL) for compatibility reasons.

  if (size != 0)
  {
    Partial write is allowed: (*processedSize <= size),
    but this function must write at least 1 byte: (*processedSize > 0).
    You must call Write() in loop, if you need to write exact amount of data.
  }

  ERROR CASES:
    If the function returns error code, then (*processedSize) is size of
    data written from (data) buffer.
*/
#define Z7_IFACEM_ISequentialOutStream(x) \
  x(Write(const void *data, UInt32 size, UInt32 *processedSize))
Z7_IFACE_CONSTR_STREAM(ISequentialOutStream, 0x02)


#ifdef _WIN32

#ifdef __HRESULT_FROM_WIN32
#define HRESULT_WIN32_ERROR_NEGATIVE_SEEK __HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK)
#else
#define HRESULT_WIN32_ERROR_NEGATIVE_SEEK   HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK)
#endif

#else

#define HRESULT_WIN32_ERROR_NEGATIVE_SEEK   MY_E_ERROR_NEGATIVE_SEEK

#endif


/*
IInStream::Seek() / IOutStream::Seek()
  If you seek to position before the beginning of the stream,
  Seek() function returns error code:
      Recommended error code is __HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK).
      or STG_E_INVALIDFUNCTION
  It is allowed to seek past the end of the stream.
  if Seek() returns error, then the value of *newPosition is undefined.
*/

#define Z7_IFACEM_IInStream(x) \
  x(Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
Z7_IFACE_CONSTR_STREAM_SUB(IInStream, ISequentialInStream, 0x03)

#define Z7_IFACEM_IOutStream(x) \
  x(Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)) \
  x(SetSize(UInt64 newSize))
Z7_IFACE_CONSTR_STREAM_SUB(IOutStream, ISequentialOutStream, 0x04)

#define Z7_IFACEM_IStreamGetSize(x) \
  x(GetSize(UInt64 *size))
Z7_IFACE_CONSTR_STREAM(IStreamGetSize, 0x06)

#define Z7_IFACEM_IOutStreamFinish(x) \
  x(OutStreamFinish())
Z7_IFACE_CONSTR_STREAM(IOutStreamFinish, 0x07)

#define Z7_IFACEM_IStreamGetProps(x) \
  x(GetProps(UInt64 *size, FILETIME *cTime, FILETIME *aTime, FILETIME *mTime, UInt32 *attrib))
Z7_IFACE_CONSTR_STREAM(IStreamGetProps, 0x08)


struct CStreamFileProps
{
  UInt64 Size;
  UInt64 VolID;
  UInt64 FileID_Low;
  UInt64 FileID_High;
  UInt32 NumLinks;
  UInt32 Attrib;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
};


#define Z7_IFACEM_IStreamGetProps2(x) \
  x(GetProps2(CStreamFileProps *props))
Z7_IFACE_CONSTR_STREAM(IStreamGetProps2, 0x09)

#define Z7_IFACEM_IStreamGetProp(x) \
  x(GetProperty(PROPID propID, PROPVARIANT *value)) \
  x(ReloadProps())
Z7_IFACE_CONSTR_STREAM(IStreamGetProp, 0x0a)


/*
IStreamSetRestriction::SetRestriction(UInt64 begin, UInt64 end)
  
  It sets region of data in output stream that is restricted.
  For restricted region it's expected (or allowed)
  that the caller can write to same region with different calls of Write()/SetSize().
  Another regions of output stream will be supposed as non-restricted:
    - The callee usually doesn't flush the data in restricted region.
    - The callee usually can flush data from non-restricted region after writing.

Actual restiction rules depend also from current stream position.
It's recommended to call SetRestriction() just before the Write() call.
So the callee can optimize writing and flushing, if that Write()
operation is not restricted.

Note: Each new call of SetRestriction() sets new restictions,
so previous restrction calls has no effect anymore.

inputs:
 
  (begin > end) is not allowed, and returns E_FAIL;
  
  if (begin == end)
  {
    No restriction.
    The caller will call Write() in sequential order.
    After SetRestriction(begin, begin), but before next call of SetRestriction()
    {
      Additional condition:
        it's expected that current stream seek position is equal to stream size.
      The callee can make final flushing for any data before current stream seek position.
      For each Write(size) call:
        The callee can make final flushing for that new written data.
    }
    The pair of values (begin == 0 && end == 0) is recommended to remove write restriction.
  }
  
  if (begin < end)
  {
    it means that callee must NOT flush any data in region [begin, end).
    The caller is allowed to Seek() to that region and rewrite the
    data in that restriction region.
    if (end == (UInt64)(Int64)-1)
    {
      there is no upper bound for restricted region.
      So non-restricted region will be [0, begin) in that case
    }
  }

 returns:
  - if (begin > end) it return ERROR code (E_FAIL)
  - S_OK : if no errors.
  - Also the call of SetRestriction() can initiate the flushing of already written data.
    So it can return the result of that flushing.
 
 Note: IOutStream::SetSize() also can change the data.
    So it's not expected the call
    IOutStream::SetSize() to region that was written before as unrestricted.
*/

#define Z7_IFACEM_IStreamSetRestriction(x) \
  x(SetRestriction(UInt64 begin, UInt64 end)) \

Z7_IFACE_CONSTR_STREAM(IStreamSetRestriction, 0x10)

Z7_PURE_INTERFACES_END
#endif

/* ---- CPP/7zip/Common/StreamUtils.h ---- */
// StreamUtils.h

#ifndef ZIP7_INC_STREAM_UTILS_H
#define ZIP7_INC_STREAM_UTILS_H

// amalgamation: header emitted in prologue

inline HRESULT InStream_SeekSet(IInStream *stream, UInt64 offset) throw()
  {  return stream->Seek((Int64)offset, STREAM_SEEK_SET, NULL); }
inline HRESULT InStream_GetPos(IInStream *stream, UInt64 &curPosRes) throw()
  {  return stream->Seek(0, STREAM_SEEK_CUR, &curPosRes); }
inline HRESULT InStream_GetSize_SeekToEnd(IInStream *stream, UInt64 &sizeRes) throw()
  { return stream->Seek(0, STREAM_SEEK_END, &sizeRes); }

HRESULT InStream_SeekToBegin(IInStream *stream) throw();
HRESULT InStream_AtBegin_GetSize(IInStream *stream, UInt64 &size) throw();
HRESULT InStream_GetPos_GetSize(IInStream *stream, UInt64 &curPosRes, UInt64 &sizeRes) throw();

inline HRESULT InStream_GetSize_SeekToBegin(IInStream *stream, UInt64 &sizeRes) throw()
{
  RINOK(InStream_SeekToBegin(stream))
  return InStream_AtBegin_GetSize(stream, sizeRes);
}


HRESULT ReadStream(ISequentialInStream *stream, void *data, size_t *size) throw();
HRESULT ReadStream_FALSE(ISequentialInStream *stream, void *data, size_t size) throw();
HRESULT ReadStream_FAIL(ISequentialInStream *stream, void *data, size_t size) throw();
HRESULT WriteStream(ISequentialOutStream *stream, const void *data, size_t size) throw();

#endif

/* ---- CPP/7zip/Common/LimitedStreams.h ---- */
// LimitedStreams.h

#ifndef ZIP7_INC_LIMITED_STREAMS_H
#define ZIP7_INC_LIMITED_STREAMS_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_COM_1(
  CLimitedSequentialInStream
  , ISequentialInStream
)
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
  UInt64 _pos;
  bool _wasFinished;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(UInt64 streamSize)
  {
    _size = streamSize;
    _pos = 0;
    _wasFinished = false;
  }
  UInt64 GetSize() const { return _pos; }
  UInt64 GetRem() const { return _size - _pos; }
  bool WasFinished() const { return _wasFinished; }
};


Z7_CLASS_IMP_IInStream(
  CLimitedInStream
)
  CMyComPtr<IInStream> _stream;
  UInt64 _virtPos;
  UInt64 _physPos;
  UInt64 _size;
  UInt64 _startOffset;

  HRESULT SeekToPhys() { return InStream_SeekSet(_stream, _physPos); }
public:
  void SetStream(IInStream *stream) { _stream = stream; }
  HRESULT InitAndSeek(UInt64 startOffset, UInt64 size)
  {
    _startOffset = startOffset;
    _physPos = startOffset;
    _virtPos = 0;
    _size = size;
    return SeekToPhys();
  }
  HRESULT SeekToStart() { return Seek(0, STREAM_SEEK_SET, NULL); }
};

HRESULT CreateLimitedInStream(IInStream *inStream, UInt64 pos, UInt64 size, ISequentialInStream **resStream);


Z7_CLASS_IMP_IInStream(
  CClusterInStream
)
  UInt64 _virtPos;
  UInt64 _physPos;
  UInt32 _curRem;
public:
  unsigned BlockSizeLog;
  UInt64 Size;
  CMyComPtr<IInStream> Stream;
  CRecordVector<UInt32> Vector;
  UInt64 StartOffset;

  HRESULT SeekToPhys() { return InStream_SeekSet(Stream, _physPos); }

  HRESULT InitAndSeek()
  {
    _curRem = 0;
    _virtPos = 0;
    _physPos = StartOffset;
    if (Vector.Size() > 0)
    {
      _physPos = StartOffset + (Vector[0] << BlockSizeLog);
      return SeekToPhys();
    }
    return S_OK;
  }
};



const UInt64 k_SeekExtent_Phy_Type_ZeroFill = (UInt64)(Int64)-1;

struct CSeekExtent
{
  UInt64 Virt;
  UInt64 Phy;

  void SetAs_ZeroFill() { Phy = k_SeekExtent_Phy_Type_ZeroFill; }
  bool Is_ZeroFill() const { return Phy == k_SeekExtent_Phy_Type_ZeroFill; }
};


Z7_CLASS_IMP_IInStream(
  CExtentsStream
)
  UInt64 _virtPos;
  UInt64 _phyPos;
  unsigned _prevExtentIndex;
public:
  CMyComPtr<IInStream> Stream;
  CRecordVector<CSeekExtent> Extents;

  void ReleaseStream() { Stream.Release(); }
  void Init()
  {
    _virtPos = 0;
    _phyPos = (UInt64)0 - 1; // we need Seek() for Stream
    _prevExtentIndex = 0;
  }
};



Z7_CLASS_IMP_COM_1(
  CLimitedSequentialOutStream
  , ISequentialOutStream
)
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
  bool _overflow;
  bool _overflowIsAllowed;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(UInt64 size, bool overflowIsAllowed = false)
  {
    _size = size;
    _overflow = false;
    _overflowIsAllowed = overflowIsAllowed;
  }
  bool IsFinishedOK() const { return (_size == 0 && !_overflow); }
  UInt64 GetRem() const { return _size; }
};


Z7_CLASS_IMP_IInStream(
  CTailInStream
)
  UInt64 _virtPos;
public:
  CMyComPtr<IInStream> Stream;
  UInt64 Offset;

  void Init()
  {
    _virtPos = 0;
  }
  HRESULT SeekToStart() { return InStream_SeekSet(Stream, Offset); }
};


Z7_CLASS_IMP_IInStream(
  CLimitedCachedInStream
)
  CMyComPtr<IInStream> _stream;
  UInt64 _virtPos;
  UInt64 _physPos;
  UInt64 _size;
  UInt64 _startOffset;
  
  const Byte *_cache;
  size_t _cacheSize;
  size_t _cachePhyPos;

  HRESULT SeekToPhys() { return InStream_SeekSet(_stream, _physPos); }
public:
  CByteBuffer Buffer;

  void SetStream(IInStream *stream) { _stream = stream; }
  void SetCache(size_t cacheSize, size_t cachePos)
  {
    _cache = Buffer;
    _cacheSize = cacheSize;
    _cachePhyPos = cachePos;
  }

  HRESULT InitAndSeek(UInt64 startOffset, UInt64 size)
  {
    _startOffset = startOffset;
    _physPos = startOffset;
    _virtPos = 0;
    _size = size;
    return SeekToPhys();
  }
 
  HRESULT SeekToStart() { return Seek(0, STREAM_SEEK_SET, NULL); }
};


class CTailOutStream Z7_final :
  public IOutStream,
  public CMyUnknownImp
{
  Z7_IFACES_IMP_UNK_2(ISequentialOutStream, IOutStream)

  UInt64 _virtPos;
  UInt64 _virtSize;
public:
  CMyComPtr<IOutStream> Stream;
  UInt64 Offset;
  
  void Init()
  {
    _virtPos = 0;
    _virtSize = 0;
  }
};

#endif

/* ---- CPP/Windows/Defs.h ---- */
// Windows/Defs.h

#ifndef ZIP7_INC_WINDOWS_DEFS_H
#define ZIP7_INC_WINDOWS_DEFS_H

// amalgamation: header emitted in prologue

#ifdef _WIN32
inline BOOL BoolToBOOL(bool v) { return (v ? TRUE: FALSE); }
#endif

inline bool BOOLToBool(BOOL v) { return (v != FALSE); }

inline VARIANT_BOOL BoolToVARIANT_BOOL(bool v) { return (v ? VARIANT_TRUE: VARIANT_FALSE); }
inline bool VARIANT_BOOLToBool(VARIANT_BOOL v) { return (v != VARIANT_FALSE); }

#endif

/* ---- CPP/7zip/ICoder.h ---- */
// ICoder.h

#ifndef ZIP7_INC_ICODER_H
#define ZIP7_INC_ICODER_H

// amalgamation: header emitted in prologue

Z7_PURE_INTERFACES_BEGIN

#define Z7_IFACE_CONSTR_CODER(i, n) \
  Z7_DECL_IFACE_7ZIP(i, 4, n) \
  { Z7_IFACE_COM7_PURE(i) };

#define Z7_IFACEM_ICompressProgressInfo(x) \
  x(SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
Z7_IFACE_CONSTR_CODER(ICompressProgressInfo, 0x04)
  /*
    SetRatioInfo()
     (inSize)  can be NULL, if unknown
     (outSize) can be NULL, if unknown
  returns:
    S_OK
    E_ABORT  : Break by user
    another error codes
  */

#define Z7_IFACEM_ICompressCoder(x) \
  x(Code(ISequentialInStream *inStream, ISequentialOutStream *outStream, \
      const UInt64 *inSize, const UInt64 *outSize, \
      ICompressProgressInfo *progress))
Z7_IFACE_CONSTR_CODER(ICompressCoder, 0x05)

#define Z7_IFACEM_ICompressCoder2(x) \
  x(Code(ISequentialInStream * const *inStreams, const UInt64  *const *inSizes, UInt32 numInStreams, \
      ISequentialOutStream *const *outStreams, const UInt64 *const *outSizes, UInt32 numOutStreams, \
      ICompressProgressInfo *progress))
Z7_IFACE_CONSTR_CODER(ICompressCoder2, 0x18)

/*
  ICompressCoder::Code
  ICompressCoder2::Code
  
  returns:
    S_OK     : OK
    S_FALSE  : data error (for decoders)
    E_OUTOFMEMORY : memory allocation error
    E_NOTIMPL : unsupported encoding method (for decoders)
    another error code : some error. For example, it can be error code received from inStream or outStream function.
  
  Parameters:
    (inStream != NULL)
    (outStream != NULL)

    if (inSize != NULL)
    {
      Encoders in 7-Zip ignore (inSize).
      Decoder can use (*inSize) to check that stream was decoded correctly.
      Some decoders in 7-Zip check it, if (full_decoding mode was set via ICompressSetFinishMode)
    }

    If it's required to limit the reading from input stream (inStream), it can
      be done with ISequentialInStream implementation.

    if (outSize != NULL)
    {
      Encoders in 7-Zip ignore (outSize).
      Decoder unpacks no more than (*outSize) bytes.
    }
    
    (progress == NULL) is allowed.


  Decoding with Code() function
  -----------------------------
   
  You can request some interfaces before decoding
   - ICompressSetDecoderProperties2
   - ICompressSetFinishMode

  If you need to decode full stream:
  {
    1) try to set full_decoding mode with ICompressSetFinishMode::SetFinishMode(1);
    2) call the Code() function with specified (inSize) and (outSize), if these sizes are known.
  }

  If you need to decode only part of stream:
  {
    1) try to set partial_decoding mode with ICompressSetFinishMode::SetFinishMode(0);
    2) Call the Code() function with specified (inSize = NULL) and specified (outSize).
  }

  Encoding with Code() function
  -----------------------------
  
  You can request some interfaces :
  - ICompressSetCoderProperties   - use it before encoding to set properties
  - ICompressWriteCoderProperties - use it before or after encoding to request encoded properties.

  ICompressCoder2 is used when (numInStreams != 1 || numOutStreams != 1)
     The rules are similar to ICompressCoder rules
*/


namespace NCoderPropID
{
  enum EEnum
  {
    kDefaultProp = 0,
    kDictionarySize,    // VT_UI4
    kUsedMemorySize,    // VT_UI4
    kOrder,             // VT_UI4
    kBlockSize,         // VT_UI4 or VT_UI8
    kPosStateBits,      // VT_UI4
    kLitContextBits,    // VT_UI4
    kLitPosBits,        // VT_UI4
    kNumFastBytes,      // VT_UI4
    kMatchFinder,       // VT_BSTR
    kMatchFinderCycles, // VT_UI4
    kNumPasses,         // VT_UI4
    kAlgorithm,         // VT_UI4
    kNumThreads,        // VT_UI4
    kEndMarker,         // VT_BOOL
    kLevel,             // VT_UI4
    kReduceSize,        // VT_UI8 : it's estimated size of largest data stream that will be compressed
                        //   encoder can use this value to reduce dictionary size and allocate data buffers

    kExpectedDataSize,  // VT_UI8 : for ICompressSetCoderPropertiesOpt :
                        //   it's estimated size of current data stream
                        //   real data size can differ from that size
                        //   encoder can use this value to optimize encoder initialization

    kBlockSize2,        // VT_UI4 or VT_UI8
    kCheckSize,         // VT_UI4 : size of digest in bytes
    kFilter,            // VT_BSTR
    kMemUse,            // VT_UI8
    kAffinity,          // VT_UI8
    kBranchOffset,      // VT_UI4
    kHashBits,          // VT_UI4
    kNumThreadGroups,   // VT_UI4
    kThreadGroup,       // VT_UI4
    kAffinityInGroup,   // VT_UI8
    /*
    // kHash3Bits,          // VT_UI4
    // kHash2Bits,          // VT_UI4
    // kChainBits,         // VT_UI4
    kChainSize,         // VT_UI4
    kNativeLevel,       // VT_UI4
    kFast,              // VT_UI4
    kMinMatch,          // VT_UI4 The minimum slen is 3 and the maximum is 7.
    kOverlapLog,        // VT_UI4 The minimum ovlog is 0 and the maximum is 9.  (default: 6)
    kRowMatchFinder,    // VT_BOOL
    kLdmEnable,         // VT_BOOL
    // kLdmWindowSizeLog,  // VT_UI4
    kLdmWindowSize,     // VT_UI4
    kLdmHashLog,        // VT_UI4 The minimum ldmhlog is 6 and the maximum is 26 (default: 20).
    kLdmMinMatchLength, // VT_UI4 The minimum ldmslen is 4 and the maximum is 4096 (default: 64).
    kLdmBucketSizeLog,  // VT_UI4 The minimum ldmblog is 0 and the maximum is 8 (default: 3).
    kLdmHashRateLog,    // VT_UI4 The default value is wlog - ldmhlog.
    kWriteUnpackSizeFlag, // VT_BOOL
    kUsePledged,        // VT_BOOL
    kUseSizeHintPledgedForSmall, // VT_BOOL
    kUseSizeHintForEach, // VT_BOOL
    kUseSizeHintGlobal, // VT_BOOL
    kParamSelectMode,   // VT_UI4
    // kSearchLog,         // VT_UI4 The minimum slog is 1 and the maximum is 26
    // kTargetLen,         // VT_UI4 The minimum tlen is 0 and the maximum is 999.
    */
    k_NUM_DEFINED
  };
}

#define Z7_IFACEM_ICompressSetCoderPropertiesOpt(x) \
  x(SetCoderPropertiesOpt(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
Z7_IFACE_CONSTR_CODER(ICompressSetCoderPropertiesOpt, 0x1F)


#define Z7_IFACEM_ICompressSetCoderProperties(x) \
  x(SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
Z7_IFACE_CONSTR_CODER(ICompressSetCoderProperties, 0x20)

/*
#define Z7_IFACEM_ICompressSetDecoderProperties(x) \
  x(SetDecoderProperties(ISequentialInStream *inStream))
Z7_IFACE_CONSTR_CODER(ICompressSetDecoderProperties, 0x21)
*/

#define Z7_IFACEM_ICompressSetDecoderProperties2(x) \
  x(SetDecoderProperties2(const Byte *data, UInt32 size))
Z7_IFACE_CONSTR_CODER(ICompressSetDecoderProperties2, 0x22)
  /* returns:
    S_OK
    E_NOTIMP      : unsupported properties
    E_INVALIDARG  : incorrect (or unsupported) properties
    E_OUTOFMEMORY : memory allocation error
  */


#define Z7_IFACEM_ICompressWriteCoderProperties(x) \
  x(WriteCoderProperties(ISequentialOutStream *outStream))
Z7_IFACE_CONSTR_CODER(ICompressWriteCoderProperties, 0x23)

#define Z7_IFACEM_ICompressGetInStreamProcessedSize(x) \
  x(GetInStreamProcessedSize(UInt64 *value))
Z7_IFACE_CONSTR_CODER(ICompressGetInStreamProcessedSize, 0x24)

#define Z7_IFACEM_ICompressSetCoderMt(x) \
  x(SetNumberOfThreads(UInt32 numThreads))
Z7_IFACE_CONSTR_CODER(ICompressSetCoderMt, 0x25)

#define Z7_IFACEM_ICompressSetFinishMode(x) \
  x(SetFinishMode(UInt32 finishMode))
Z7_IFACE_CONSTR_CODER(ICompressSetFinishMode, 0x26)
  /* finishMode:
    0 : partial decoding is allowed. It's default mode for ICompressCoder::Code(), if (outSize) is defined.
    1 : full decoding. The stream must be finished at the end of decoding. */

#define Z7_IFACEM_ICompressGetInStreamProcessedSize2(x) \
  x(GetInStreamProcessedSize2(UInt32 streamIndex, UInt64 *value))
Z7_IFACE_CONSTR_CODER(ICompressGetInStreamProcessedSize2, 0x27)

#define Z7_IFACEM_ICompressSetMemLimit(x) \
  x(SetMemLimit(UInt64 memUsage))
Z7_IFACE_CONSTR_CODER(ICompressSetMemLimit, 0x28)


/*
  ICompressReadUnusedFromInBuf is supported by ICoder object
  call ReadUnusedFromInBuf() after ICoder::Code(inStream, ...).
  ICoder::Code(inStream, ...) decodes data, and the ICoder object is allowed
  to read from inStream to internal buffers more data than minimal data required for decoding.
  So we can call ReadUnusedFromInBuf() from same ICoder object to read unused input
  data from the internal buffer.
  in ReadUnusedFromInBuf(): the Coder is not allowed to use (ISequentialInStream *inStream) object, that was sent to ICoder::Code().
*/
#define Z7_IFACEM_ICompressReadUnusedFromInBuf(x) \
  x(ReadUnusedFromInBuf(void *data, UInt32 size, UInt32 *processedSize))
Z7_IFACE_CONSTR_CODER(ICompressReadUnusedFromInBuf, 0x29)


#define Z7_IFACEM_ICompressGetSubStreamSize(x) \
  x(GetSubStreamSize(UInt64 subStream, UInt64 *value))
Z7_IFACE_CONSTR_CODER(ICompressGetSubStreamSize, 0x30)
  /* returns:
    S_OK     : (*value) contains the size or estimated size (can be incorrect size)
    S_FALSE  : size is undefined
    E_NOTIMP : the feature is not implemented
  Let's (read_size) is size of data that was already read by ISequentialInStream::Read().
  The caller should call GetSubStreamSize() after each Read() and check sizes:
    if (start_of_subStream + *value < read_size)
    {
      // (*value) is correct, and it's allowed to call GetSubStreamSize() for next subStream:
      start_of_subStream += *value;
      subStream++;
    }
  */

#define Z7_IFACEM_ICompressSetInStream(x) \
  x(SetInStream(ISequentialInStream *inStream)) \
  x(ReleaseInStream())
Z7_IFACE_CONSTR_CODER(ICompressSetInStream, 0x31)

#define Z7_IFACEM_ICompressSetOutStream(x) \
  x(SetOutStream(ISequentialOutStream *outStream)) \
  x(ReleaseOutStream())
Z7_IFACE_CONSTR_CODER(ICompressSetOutStream, 0x32)

/*
#define Z7_IFACEM_ICompressSetInStreamSize(x) \
  x(SetInStreamSize(const UInt64 *inSize)) \
Z7_IFACE_CONSTR_CODER(ICompressSetInStreamSize, 0x33)
*/

#define Z7_IFACEM_ICompressSetOutStreamSize(x) \
  x(SetOutStreamSize(const UInt64 *outSize))
Z7_IFACE_CONSTR_CODER(ICompressSetOutStreamSize, 0x34)
  /* That function initializes decoder structures.
     Call this function only for stream version of decoder.
       if (outSize == NULL), then output size is unknown
       if (outSize != NULL), then the decoder must stop decoding after (*outSize) bytes. */

#define Z7_IFACEM_ICompressSetBufSize(x) \
  x(SetInBufSize(UInt32 streamIndex, UInt32 size)) \
  x(SetOutBufSize(UInt32 streamIndex, UInt32 size))
 
Z7_IFACE_CONSTR_CODER(ICompressSetBufSize, 0x35)

#define Z7_IFACEM_ICompressInitEncoder(x) \
  x(InitEncoder())
Z7_IFACE_CONSTR_CODER(ICompressInitEncoder, 0x36)
  /* That function initializes encoder structures.
     Call this function only for stream version of encoder. */

#define Z7_IFACEM_ICompressSetInStream2(x) \
  x(SetInStream2(UInt32 streamIndex, ISequentialInStream *inStream)) \
  x(ReleaseInStream2(UInt32 streamIndex))
Z7_IFACE_CONSTR_CODER(ICompressSetInStream2, 0x37)

/*
#define Z7_IFACEM_ICompressSetOutStream2(x) \
  x(SetOutStream2(UInt32 streamIndex, ISequentialOutStream *outStream))
  x(ReleaseOutStream2(UInt32 streamIndex))
Z7_IFACE_CONSTR_CODER(ICompressSetOutStream2, 0x38)

#define Z7_IFACEM_ICompressSetInStreamSize2(x) \
  x(SetInStreamSize2(UInt32 streamIndex, const UInt64 *inSize))
Z7_IFACE_CONSTR_CODER(ICompressSetInStreamSize2, 0x39)
*/

/*
#define Z7_IFACEM_ICompressInSubStreams(x) \
  x(GetNextInSubStream(UInt64 *streamIndexRes, ISequentialInStream **stream))
Z7_IFACE_CONSTR_CODER(ICompressInSubStreams, 0x3A)

#define Z7_IFACEM_ICompressOutSubStreams(x) \
  x(GetNextOutSubStream(UInt64 *streamIndexRes, ISequentialOutStream **stream))
Z7_IFACE_CONSTR_CODER(ICompressOutSubStreams, 0x3B)
*/

/*
  ICompressFilter
  Filter(Byte *data, UInt32 size)
  (size)
     converts as most as possible bytes required for fast processing.
     Some filters have (smallest_fast_block).
     For example, (smallest_fast_block == 16) for AES CBC/CTR filters.
     If data stream is not finished, caller must call Filter() for larger block:
     where (size >= smallest_fast_block).
     if (size >= smallest_fast_block)
     {
       The filter can leave some bytes at the end of data without conversion:
       if there are data alignment reasons or speed reasons.
       The caller can read additional data from stream and call Filter() again.
     }
     If data stream was finished, caller can call Filter() for (size < smallest_fast_block)

  (data) parameter:
     Some filters require alignment for any Filter() call:
        1) (stream_offset % alignment_size) == (data % alignment_size)
        2) (alignment_size == 2^N)
     where (stream_offset) - is the number of bytes that were already filtered before.
     The callers of Filter() are required to meet these requirements.
     (alignment_size) can be different:
           16 : for AES filters
       4 or 2 : for some branch convert filters
            1 : for another filters
     (alignment_size >= 16) is enough for all current filters of 7-Zip.
     But the caller can use larger (alignment_size).
     Recommended alignment for (data) of Filter() call is (alignment_size == 64).
     Also it's recommended to use aligned value for (size):
       (size % alignment_size == 0),
     if it's not last call of Filter() for current stream.

  returns: (outSize):
       if (outSize == 0) : Filter have not converted anything.
           So the caller can stop processing, if data stream was finished.
       if (outSize <= size) : Filter have converted outSize bytes
       if (outSize >  size) : Filter have not converted anything.
           and it needs at least outSize bytes to convert one block
           (it's for crypto block algorithms).
*/

#define Z7_IFACEM_ICompressFilter(x) \
  x(Init()) \
  x##2(UInt32, Filter(Byte *data, UInt32 size))
Z7_IFACE_CONSTR_CODER(ICompressFilter, 0x40)


#define Z7_IFACEM_ICompressCodecsInfo(x) \
  x(GetNumMethods(UInt32 *numMethods)) \
  x(GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  x(CreateDecoder(UInt32 index, const GUID *iid, void* *coder)) \
  x(CreateEncoder(UInt32 index, const GUID *iid, void* *coder))
Z7_IFACE_CONSTR_CODER(ICompressCodecsInfo, 0x60)

#define Z7_IFACEM_ISetCompressCodecsInfo(x) \
  x(SetCompressCodecsInfo(ICompressCodecsInfo *compressCodecsInfo))
Z7_IFACE_CONSTR_CODER(ISetCompressCodecsInfo, 0x61)

#define Z7_IFACEM_ICryptoProperties(x) \
  x(SetKey(const Byte *data, UInt32 size)) \
  x(SetInitVector(const Byte *data, UInt32 size))
Z7_IFACE_CONSTR_CODER(ICryptoProperties, 0x80)

/*
  x(ResetSalt())
Z7_IFACE_CONSTR_CODER(ICryptoResetSalt, 0x88)
*/

#define Z7_IFACEM_ICryptoResetInitVector(x) \
  x(ResetInitVector())
Z7_IFACE_CONSTR_CODER(ICryptoResetInitVector, 0x8C)
  /* Call ResetInitVector() only for encoding.
     Call ResetInitVector() before encoding and before WriteCoderProperties().
     Crypto encoder can create random IV in that function. */

#define Z7_IFACEM_ICryptoSetPassword(x) \
  x(CryptoSetPassword(const Byte *data, UInt32 size))
Z7_IFACE_CONSTR_CODER(ICryptoSetPassword, 0x90)

#define Z7_IFACEM_ICryptoSetCRC(x) \
  x(CryptoSetCRC(UInt32 crc))
Z7_IFACE_CONSTR_CODER(ICryptoSetCRC, 0xA0)


namespace NMethodPropID
{
  enum EEnum
  {
    kID,
    kName,
    kDecoder,
    kEncoder,
    kPackStreams,
    kUnpackStreams,
    kDescription,
    kDecoderIsAssigned,
    kEncoderIsAssigned,
    kDigestSize,
    kIsFilter
  };
}

namespace NModuleInterfaceType
{
  /*
    virtual destructor in IUnknown:
    - no  : 7-Zip (Windows)
    - no  : 7-Zip (Linux) (v23) in default mode
    - yes : p7zip
    - yes : 7-Zip (Linux) before v23
    - yes : 7-Zip (Linux) (v23), if Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN is defined
  */
  const UInt32 k_IUnknown_VirtDestructor_No  = 0;
  const UInt32 k_IUnknown_VirtDestructor_Yes = 1;
  const UInt32 k_IUnknown_VirtDestructor_ThisModule =
  #if !defined(_WIN32) && defined(Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN)
    k_IUnknown_VirtDestructor_Yes;
  #else
    k_IUnknown_VirtDestructor_No;
  #endif
}

namespace NModulePropID
{
  enum EEnum
  {
    kInterfaceType,   // VT_UI4
    kVersion          // VT_UI4
  };
}


#define Z7_IFACEM_IHasher(x) \
  x##2(void, Init()) \
  x##2(void, Update(const void *data, UInt32 size)) \
  x##2(void, Final(Byte *digest)) \
  x##2(UInt32, GetDigestSize())
Z7_IFACE_CONSTR_CODER(IHasher, 0xC0)

#define Z7_IFACEM_IHashers(x) \
  x##2(UInt32, GetNumHashers()) \
  x(GetHasherProp(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  x(CreateHasher(UInt32 index, IHasher **hasher))
Z7_IFACE_CONSTR_CODER(IHashers, 0xC1)

extern "C"
{
  typedef HRESULT (WINAPI *Func_GetNumberOfMethods)(UInt32 *numMethods);
  typedef HRESULT (WINAPI *Func_GetMethodProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  typedef HRESULT (WINAPI *Func_CreateDecoder)(UInt32 index, const GUID *iid, void **outObject);
  typedef HRESULT (WINAPI *Func_CreateEncoder)(UInt32 index, const GUID *iid, void **outObject);

  typedef HRESULT (WINAPI *Func_GetHashers)(IHashers **hashers);
  
  typedef HRESULT (WINAPI *Func_SetCodecs)(ICompressCodecsInfo *compressCodecsInfo);
  typedef HRESULT (WINAPI *Func_GetModuleProp)(PROPID propID, PROPVARIANT *value);
}

Z7_PURE_INTERFACES_END
#endif

/* ---- CPP/7zip/Common/MethodProps.h ---- */
// MethodProps.h

#ifndef ZIP7_INC_7Z_METHOD_PROPS_H
#define ZIP7_INC_7Z_METHOD_PROPS_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// UInt64 GetMemoryUsage_LZMA(UInt32 dict, bool isBt, UInt32 numThreads);

inline UInt64 Calc_From_Val_Percents_Less100(UInt64 val, UInt64 percents)
{
  if (percents == 0)
    return 0;
  if (val <= (UInt64)(Int64)-1 / percents)
    return val * percents / 100;
  return val / 100 * percents;
}

UInt64 Calc_From_Val_Percents(UInt64 val, UInt64 percents);

bool StringToBool(const wchar_t *s, bool &res);
HRESULT PROPVARIANT_to_bool(const PROPVARIANT &prop, bool &dest);
unsigned ParseStringToUInt32(const UString &srcString, UInt32 &number);

/*
if (name.IsEmpty() && prop.vt == VT_EMPTY), it doesn't change (resValue) and returns S_OK.
  So you must set (resValue) for default value before calling */
HRESULT ParsePropToUInt32(const UString &name, const PROPVARIANT &prop, UInt32 &resValue);

/* input: (numThreads = the_number_of_processors) */
HRESULT ParseMtProp2(const UString &name, const PROPVARIANT &prop, UInt32 &numThreads, bool &force);

inline HRESULT ParseMtProp(const UString &name, const PROPVARIANT &prop, UInt32 numCPUs, UInt32 &numThreads)
{
  bool forced = false;
  numThreads = numCPUs;
  return ParseMtProp2(name, prop, numThreads, forced);
}


struct CProp
{
  PROPID Id;
  bool IsOptional;
  NWindows::NCOM::CPropVariant Value;
  CProp(): IsOptional(false) {}
};

struct CProps
{
  CObjectVector<CProp> Props;

  void Clear() { Props.Clear(); }

  bool AreThereNonOptionalProps() const
  {
    FOR_VECTOR (i, Props)
      if (!Props[i].IsOptional)
        return true;
    return false;
  }

  void AddProp32(PROPID propid, UInt32 val);
  
  void AddPropBool(PROPID propid, bool val);

  void AddProp_Ascii(PROPID propid, const char *s)
  {
    CProp &prop = Props.AddNew();
    prop.IsOptional = true;
    prop.Id = propid;
    prop.Value = s;
  }

  HRESULT SetCoderProps(ICompressSetCoderProperties *scp, const UInt64 *dataSizeReduce = NULL) const;
  HRESULT SetCoderProps_DSReduce_Aff(ICompressSetCoderProperties *scp,
      const UInt64 *dataSizeReduce,
      const UInt64 *affinity,
      const UInt32 *affinityGroup,
      const UInt64 *affinityInGroup) const;
};

class CMethodProps: public CProps
{
  HRESULT SetParam(const UString &name, const UString &value);
public:
  unsigned GetLevel() const;
  int Get_NumThreads() const
  {
    const int i = FindProp(NCoderPropID::kNumThreads);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_UI4)
        return (int)val.ulVal;
    }
    return -1;
  }

  bool Get_DicSize(UInt64 &res) const
  {
    res = 0;
    const int i = FindProp(NCoderPropID::kDictionarySize);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_UI4)
      {
        res = val.ulVal;
        return true;
      }
      if (val.vt == VT_UI8)
      {
        res = val.uhVal.QuadPart;
        return true;
      }
    }
    return false;
  }

  int FindProp(PROPID id) const;

  UInt32 Get_Lzma_Algo() const
  {
    const int i = FindProp(NCoderPropID::kAlgorithm);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_UI4)
        return val.ulVal;
    }
    return GetLevel() >= 5 ? 1 : 0;
  }

  UInt64 Get_Lzma_DicSize() const
  {
    UInt64 v;
    if (Get_DicSize(v))
      return v;
    const unsigned level = GetLevel();
    const UInt32 dictSize = level <= 4 ?
        (UInt32)1 << (level * 2 + 16) :
        level <= sizeof(size_t) / 2 + 4 ?
          (UInt32)1 << (level + 20) :
          (UInt32)1 << (sizeof(size_t) / 2 + 24);
    return dictSize;
  }

  bool Get_Lzma_MatchFinder_IsBt() const
  {
    const int i = FindProp(NCoderPropID::kMatchFinder);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_BSTR)
        return ((val.bstrVal[0] | 0x20) != 'h'); // check for "hc"
    }
    return GetLevel() >= 5;
  }

  bool Get_Lzma_Eos() const
  {
    const int i = FindProp(NCoderPropID::kEndMarker);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_BOOL)
        return VARIANT_BOOLToBool(val.boolVal);
    }
    return false;
  }

  bool Are_Lzma_Model_Props_Defined() const
  {
    if (FindProp(NCoderPropID::kPosStateBits) >= 0) return true;
    if (FindProp(NCoderPropID::kLitContextBits) >= 0) return true;
    if (FindProp(NCoderPropID::kLitPosBits) >= 0) return true;
    return false;
  }

  UInt32 Get_Lzma_NumThreads() const
  {
    if (Get_Lzma_Algo() == 0)
      return 1;
    int numThreads = Get_NumThreads();
    if (numThreads >= 0)
      return numThreads < 2 ? 1 : 2;
    return 2;
  }

  UInt64 Get_Lzma_MemUsage(bool addSlidingWindowSize) const;

  /* returns -1, if numThreads is unknown */
  int Get_Xz_NumThreads(UInt32 &lzmaThreads) const
  {
    lzmaThreads = 1;
    int numThreads = Get_NumThreads();
    if (numThreads >= 0 && numThreads <= 1)
      return 1;
    if (Get_Lzma_Algo() != 0)
      lzmaThreads = 2;
    return numThreads;
  }

  UInt64 GetProp_BlockSize(PROPID id) const
  {
    const int i = FindProp(id);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_UI4) { return val.ulVal; }
      if (val.vt == VT_UI8) { return val.uhVal.QuadPart; }
    }
    return 0;
  }

  UInt64 Get_Xz_BlockSize() const
  {
    {
      UInt64 blockSize1 = GetProp_BlockSize(NCoderPropID::kBlockSize);
      UInt64 blockSize2 = GetProp_BlockSize(NCoderPropID::kBlockSize2);
      UInt64 minSize = MyMin(blockSize1, blockSize2);
      if (minSize != 0)
        return minSize;
      UInt64 maxSize = MyMax(blockSize1, blockSize2);
      if (maxSize != 0)
        return maxSize;
    }
    const UInt32 kMinSize = (UInt32)1 << 20;
    const UInt32 kMaxSize = (UInt32)1 << 28;
    const UInt64 dictSize = Get_Lzma_DicSize();
    /* lzma2 code uses fake 4 GiB to calculate ChunkSize. So we do same */
    UInt64 blockSize = (UInt64)dictSize << 2;
    if (blockSize < kMinSize) blockSize = kMinSize;
    if (blockSize > kMaxSize) blockSize = kMaxSize;
    if (blockSize < dictSize) blockSize = dictSize;
    blockSize += (kMinSize - 1);
    blockSize &= ~(UInt64)(kMinSize - 1);
    return blockSize;
  }


  UInt32 Get_BZip2_NumThreads(bool &fixedNumber) const
  {
    fixedNumber = false;
    int numThreads = Get_NumThreads();
    if (numThreads >= 0)
    {
      fixedNumber = true;
      if (numThreads < 1) return 1;
      const unsigned kNumBZip2ThreadsMax = 64;
      if ((unsigned)numThreads > kNumBZip2ThreadsMax) return kNumBZip2ThreadsMax;
      return (unsigned)numThreads;
    }
    return 1;
  }

  UInt32 Get_BZip2_BlockSize() const
  {
    const int i = FindProp(NCoderPropID::kDictionarySize);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_UI4)
      {
        UInt32 blockSize = val.ulVal;
        const UInt32 kDicSizeMin = 100000;
        const UInt32 kDicSizeMax = 900000;
        if (blockSize < kDicSizeMin) blockSize = kDicSizeMin;
        if (blockSize > kDicSizeMax) blockSize = kDicSizeMax;
        return blockSize;
      }
    }
    const unsigned level = GetLevel();
    return 100000 * (level >= 5 ? 9 : (level >= 1 ? level * 2 - 1: 1));
  }

  UInt64 Get_Ppmd_MemSize() const
  {
    const int i = FindProp(NCoderPropID::kUsedMemorySize);
    if (i >= 0)
    {
      const NWindows::NCOM::CPropVariant &val = Props[(unsigned)i].Value;
      if (val.vt == VT_UI4)
        return val.ulVal;
      if (val.vt == VT_UI8)
        return val.uhVal.QuadPart;
    }
    const unsigned level = GetLevel();
    const UInt32 mem = (UInt32)1 << (level + 19);
    return mem;
  }

  void AddProp_Level(UInt32 level)
  {
    AddProp32(NCoderPropID::kLevel, level);
  }

  void AddProp_NumThreads(UInt32 numThreads)
  {
    AddProp32(NCoderPropID::kNumThreads, numThreads);
  }

  void AddProp_EndMarker_if_NotFound(bool eos)
  {
    if (FindProp(NCoderPropID::kEndMarker) < 0)
      AddPropBool(NCoderPropID::kEndMarker, eos);
  }

  void AddProp_BlockSize2(UInt64 blockSize2)
  {
    if (FindProp(NCoderPropID::kBlockSize2) < 0)
    {
      CProp &prop = Props.AddNew();
      prop.IsOptional = true;
      prop.Id = NCoderPropID::kBlockSize2;
      prop.Value = blockSize2;
    }
  }

  HRESULT ParseParamsFromString(const UString &srcString);
  HRESULT ParseParamsFromPROPVARIANT(const UString &realName, const PROPVARIANT &value);
};

class COneMethodInfo: public CMethodProps
{
public:
  AString MethodName;
  UString PropsString;
  
  void Clear()
  {
    CMethodProps::Clear();
    MethodName.Empty();
    PropsString.Empty();
  }
  bool IsEmpty() const { return MethodName.IsEmpty() && Props.IsEmpty(); }
  HRESULT ParseMethodFromPROPVARIANT(const UString &realName, const PROPVARIANT &value);
  HRESULT ParseMethodFromString(const UString &s);
};

#endif

/* ---- CPP/7zip/IProgress.h ---- */
// IProgress.h

#ifndef ZIP7_INC_IPROGRESS_H
#define ZIP7_INC_IPROGRESS_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_PURE_INTERFACES_BEGIN

#define Z7_IFACEM_IProgress(x) \
  x(SetTotal(UInt64 total)) \
  x(SetCompleted(const UInt64 *completeValue)) \

Z7_DECL_IFACE_7ZIP(IProgress, 0, 5)
  { Z7_IFACE_COM7_PURE(IProgress) };

Z7_PURE_INTERFACES_END
#endif

/* ---- CPP/7zip/Common/ProgressUtils.h ---- */
// ProgressUtils.h

#ifndef ZIP7_INC_PROGRESS_UTILS_H
#define ZIP7_INC_PROGRESS_UTILS_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

Z7_CLASS_IMP_COM_1(
  CLocalProgress
  , ICompressProgressInfo
)
public:
  bool SendRatio;
  bool SendProgress;
private:
  bool _inSizeIsMain;
  CMyComPtr<IProgress> _progress;
  CMyComPtr<ICompressProgressInfo> _ratioProgress;
public:
  UInt64 ProgressOffset;
  UInt64 InSize;
  UInt64 OutSize;

  CLocalProgress();

  void Init(IProgress *progress, bool inSizeIsMain);
  HRESULT SetCur();
};

#endif

/* ---- CPP/7zip/Common/StreamObjects.h ---- */
// StreamObjects.h

#ifndef ZIP7_INC_STREAM_OBJECTS_H
#define ZIP7_INC_STREAM_OBJECTS_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_IInStream(
  CBufferInStream
)
  UInt64 _pos;
public:
  CByteBuffer Buf;
  void Init() { _pos = 0; }
};


Z7_CLASS_IMP_COM_0(
  CReferenceBuf
)
public:
  CByteBuffer Buf;
};


Z7_CLASS_IMP_IInStream(
  CBufInStream
)
  const Byte *_data;
  UInt64 _pos;
  size_t _size;
  CMyComPtr<IUnknown> _ref;
public:
  void Init(const Byte *data, size_t size, IUnknown *ref = NULL)
  {
    _data = data;
    _size = size;
    _pos = 0;
    _ref = ref;
  }
  void Init(CReferenceBuf *ref) { Init(ref->Buf, ref->Buf.Size(), ref); }

  // Seek() is allowed here. So reading order could be changed
  bool WasFinished() const { return _pos == _size; }
};


void Create_BufInStream_WithReference(const void *data, size_t size, IUnknown *ref, ISequentialInStream **stream);
void Create_BufInStream_WithNewBuffer(const void *data, size_t size, ISequentialInStream **stream);
inline void Create_BufInStream_WithNewBuffer(const CByteBuffer &buf, ISequentialInStream **stream)
  { Create_BufInStream_WithNewBuffer(buf, buf.Size(), stream); }


class CByteDynBuffer Z7_final
{
  size_t _capacity;
  Byte *_buf;
  Z7_CLASS_NO_COPY(CByteDynBuffer)
public:
  CByteDynBuffer(): _capacity(0), _buf(NULL) {}
  // there is no copy constructor. So don't copy this object.
  ~CByteDynBuffer() { Free(); }
  void Free() throw();
  size_t GetCapacity() const { return _capacity; }
  operator Byte*() const { return _buf; }
  operator const Byte*() const { return _buf; }
  bool EnsureCapacity(size_t capacity) throw();
};


Z7_CLASS_IMP_COM_1(
  CDynBufSeqOutStream
  , ISequentialOutStream
)
  CByteDynBuffer _buffer;
  size_t _size;
public:
  CDynBufSeqOutStream(): _size(0) {}
  void Init() { _size = 0;  }
  size_t GetSize() const { return _size; }
  const Byte *GetBuffer() const { return _buffer; }
  void CopyToBuffer(CByteBuffer &dest) const;
  Byte *GetBufPtrForWriting(size_t addSize);
  void UpdateSize(size_t addSize) { _size += addSize; }
};


Z7_CLASS_IMP_COM_1(
  CBufPtrSeqOutStream
  , ISequentialOutStream
)
  Byte *_buffer;
  size_t _size;
  size_t _pos;
public:
  void Init(Byte *buffer, size_t size)
  {
    _buffer = buffer;
    _pos = 0;
    _size = size;
  }
  size_t GetPos() const { return _pos; }
};


Z7_CLASS_IMP_COM_1(
  CSequentialOutStreamSizeCount
  , ISequentialOutStream
)
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void Init() { _size = 0; }
  UInt64 GetSize() const { return _size; }
};


class CCachedInStream:
  public IInStream,
  public CMyUnknownImp
{
  Z7_IFACES_IMP_UNK_2(ISequentialInStream, IInStream)

  UInt64 *_tags;
  Byte *_data;
  size_t _dataSize;
  unsigned _blockSizeLog;
  unsigned _numBlocksLog;
  UInt64 _size;
  UInt64 _pos;
protected:
  virtual HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) = 0;
public:
  CCachedInStream(): _tags(NULL), _data(NULL) {}
  virtual ~CCachedInStream() { Free(); } // the destructor must be virtual (Release() calls it) !!!
  void Free() throw();
  bool Alloc(unsigned blockSizeLog, unsigned numBlocksLog) throw();
  void Init(UInt64 size) throw();
};

#endif

/* ---- CPP/7zip/Archive/Common/ItemNameUtils.h ---- */
// Archive/Common/ItemNameUtils.h

#ifndef ZIP7_INC_ARCHIVE_ITEM_NAME_UTILS_H
#define ZIP7_INC_ARCHIVE_ITEM_NAME_UTILS_H

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NItemName {

void ReplaceSlashes_OsToUnix(UString &name);
  
UString GetOsPath(const UString &name);
UString GetOsPath_Remove_TailSlash(const UString &name);
  
#if WCHAR_PATH_SEPARATOR != L'/'
void ReplaceToWinSlashes(UString &name, bool useBackslashReplacement);
#endif
void ReplaceToOsSlashes_Remove_TailSlash(UString &name, bool useBackslashReplacement = false);
void NormalizeSlashes_in_FileName_for_OsPath(wchar_t *s, unsigned len);
void NormalizeSlashes_in_FileName_for_OsPath(UString &name);
  
bool HasTailSlash(const AString &name, UINT codePage);
  
#ifdef _WIN32
  inline UString WinPathToOsPath(const UString &name)  { return name; }
#else
  UString WinPathToOsPath(const UString &name);
#endif

}}

#endif

/* ---- CPP/7zip/Compress/CopyCoder.h ---- */
// Compress/CopyCoder.h

#ifndef ZIP7_INC_COMPRESS_COPY_CODER_H
#define ZIP7_INC_COMPRESS_COPY_CODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {

Z7_CLASS_IMP_COM_5(
  CCopyCoder
  , ICompressCoder
  , ICompressSetInStream
  , ISequentialInStream
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  Byte *_buf;
  CMyComPtr<ISequentialInStream> _inStream;
public:
  UInt64 TotalSize;
  
  CCopyCoder(): _buf(NULL), TotalSize(0) {}
  ~CCopyCoder();
};

HRESULT CopyStream(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress);
HRESULT CopyStream_ExactSize(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt64 size, ICompressProgressInfo *progress);

}

#endif

/* ---- CPP/Windows/System.h ---- */
// Windows/System.h

#ifndef ZIP7_INC_WINDOWS_SYSTEM_H
#define ZIP7_INC_WINDOWS_SYSTEM_H

#ifndef _WIN32
// #include <sched.h>
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NWindows {
namespace NSystem {

UInt32 GetNumberOfProcessors();

#ifdef _WIN32

struct CCpuGroups
{
  CRecordVector<UInt32> GroupSizes;
  UInt32 NumThreadsTotal; // sum of threads in all groups
  // bool Is_Win11_Groups; // useless
  
  void Get_GroupSize_Min_Max(UInt32 &minSize, UInt32 &maxSize) const
  {
    unsigned num = GroupSizes.Size();
    UInt32 minSize2 = 0, maxSize2 = 0;
    if (num)
    {
      minSize2 = (UInt32)0 - 1;
      do
      {
        const UInt32 v = GroupSizes[--num];
        if (minSize2 > v) minSize2 = v;
        if (maxSize2 < v) maxSize2 = v;
      }
      while (num);
    }
    minSize = minSize2;
    maxSize = maxSize2;
  }
  bool Load();
  CCpuGroups(): NumThreadsTotal(0) {}
};

UInt32 CountAffinity(DWORD_PTR mask);

struct CProcessAffinity
{
  // UInt32 numProcessThreads;
  // UInt32 numSysThreads;
  DWORD_PTR processAffinityMask;
  DWORD_PTR systemAffinityMask;

  CCpuGroups Groups;
  bool IsGroupMode;
    /*
      IsGroupMode == true, if
          Groups.GroupSizes.Size() > 1) && { dafalt affinity was not changed }
      IsGroupMode == false, if single group or affinity was changed
    */
  
  UInt32 Load_and_GetNumberOfThreads();

  void InitST()
  {
    // numProcessThreads = 1;
    // numSysThreads = 1;
    processAffinityMask = 1;
    systemAffinityMask = 1;
    IsGroupMode = false;
    // Groups.NumThreadsTotal = 0;
    // Groups.Is_Win11_Groups = false;
  }

/*
  void CpuZero()
  {
    processAffinityMask = 0;
  }

  void CpuSet(unsigned cpuIndex)
  {
    processAffinityMask |= ((DWORD_PTR)1 << cpuIndex);
  }
*/

  UInt32 GetNumProcessThreads() const
  {
    if (IsGroupMode)
      return Groups.NumThreadsTotal;
    // IsGroupMode == false
    // so we don't want to use groups
    // we return number of threads in default primary group:
    return CountAffinity(processAffinityMask);
  }
  UInt32 GetNumSystemThreads() const
  {
    if (Groups.GroupSizes.Size() > 1 && Groups.NumThreadsTotal)
      return Groups.NumThreadsTotal;
    return CountAffinity(systemAffinityMask);
  }

  // it returns normilized number of threads
  void Get_and_return_NumProcessThreads_and_SysThreads(UInt32 &numProcessThreads, UInt32 &numSysThreads)
  {
    UInt32 num1 = 0, num2 = 0;
    if (Get())
    {
      num1 = GetNumProcessThreads();
      num2 = GetNumSystemThreads();
    }
    if (num1 == 0)
      num1 = NSystem::GetNumberOfProcessors();
    if (num1 == 0)
        num1 = 1;
    if (num2 < num1)
        num2 = num1;
    numProcessThreads = num1;
    numSysThreads = num2;
  }

  BOOL Get();

  BOOL SetProcAffinity() const
  {
    return SetProcessAffinityMask(GetCurrentProcess(), processAffinityMask);
  }
};


#else // WIN32

struct CProcessAffinity
{
  UInt32 numSysThreads;

  UInt32 GetNumSystemThreads() const { return (UInt32)numSysThreads; }
  BOOL Get();

  #ifdef Z7_AFFINITY_SUPPORTED

  CCpuSet cpu_set;

  void InitST()
  {
    numSysThreads = 1;
    CpuSet_Zero(&cpu_set);
    CpuSet_Set(&cpu_set, 0);
  }

  UInt32 GetNumProcessThreads() const { return (UInt32)CPU_COUNT(&cpu_set); }
  void CpuZero()              { CpuSet_Zero(&cpu_set); }
  void CpuSet(unsigned cpuIndex)   { CpuSet_Set(&cpu_set, cpuIndex); }
  // CpuSet_IsSet (CPU_ISSET) can return (unsigned long) in some <sched.h> implementations
  int IsCpuSet(unsigned cpuIndex) const { return CpuSet_IsSet(&cpu_set, cpuIndex) != 0; }
  // void CpuClr(int cpuIndex) { CPU_CLR(cpuIndex, &cpu_set); }

  BOOL SetProcAffinity() const
  {
    return sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == 0;
  }

  #else // Z7_AFFINITY_SUPPORTED

  void InitST()
  {
    numSysThreads = 1;
  }
  
  UInt32 GetNumProcessThreads() const
  {
    return numSysThreads;
    /*
    UInt32 num = 0;
    for (unsigned i = 0; i < sizeof(cpu_set) * 8; i++)
      num += (UInt32)((cpu_set >> i) & 1);
    return num;
    */
  }
  
  void CpuZero() { }
  void CpuSet(unsigned /* cpuIndex */) { /* UNUSED_VAR(cpuIndex) */ }
  int IsCpuSet(unsigned cpuIndex) const { return (cpuIndex < numSysThreads) ? 1 : 0; }

  BOOL SetProcAffinity() const
  {
    errno = ENOSYS;
    return FALSE;
  }
  
  #endif // Z7_AFFINITY_SUPPORTED
};

#endif // _WIN32


bool GetRamSize(size_t &size); // returns false, if unknown ram size

unsigned long Get_File_OPEN_MAX();
unsigned Get_File_OPEN_MAX_Reduced_for_3_tasks();

}}

#endif

/* ---- CPP/7zip/Archive/Common/HandlerOut.h ---- */
// HandlerOut.h

#ifndef ZIP7_INC_HANDLER_OUT_H
#define ZIP7_INC_HANDLER_OUT_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {

bool ParseSizeString(const wchar_t *name, const PROPVARIANT &prop, UInt64 percentsBase, UInt64 &res);

class CCommonMethodProps
{
protected:
  void InitCommon()
  {
    // _Write_MTime = true;
    {
#ifndef Z7_ST
      _numThreads_WasForced = false;
      UInt32 numThreads;
#ifdef _WIN32
      NWindows::NSystem::CProcessAffinity aff;
      numThreads = aff.Load_and_GetNumberOfThreads();
      _numThreadGroups = aff.IsGroupMode ? aff.Groups.GroupSizes.Size() : 0;
#else
      numThreads = NWindows::NSystem::GetNumberOfProcessors();
#endif // _WIN32
      _numProcessors = _numThreads = numThreads;
#endif // Z7_ST
    }
    
    size_t memAvail = (size_t)sizeof(size_t) << 28;
    _memAvail = memAvail;
    _memUsage_Compress = memAvail;
    _memUsage_Decompress = memAvail;
    _memUsage_WasSet = NWindows::NSystem::GetRamSize(memAvail);
    if (_memUsage_WasSet)
    {
      _memAvail = memAvail;
      unsigned bits = sizeof(size_t) * 8;
      if (bits == 32)
      {
        const UInt32 limit2 = (UInt32)7 << 28;
        if (memAvail > limit2)
          memAvail = limit2;
      }
      // 80% - is auto usage limit in handlers
      // _memUsage_Compress = memAvail * 4 / 5;
      // _memUsage_Compress = Calc_From_Val_Percents(memAvail, 80);
      _memUsage_Compress = Calc_From_Val_Percents_Less100(memAvail, 80);
      _memUsage_Decompress = memAvail / 32 * 17;
    }
  }

public:
#ifndef Z7_ST
  UInt32 _numThreads;
  UInt32 _numProcessors;
#ifdef _WIN32
  UInt32 _numThreadGroups;
#endif
  bool _numThreads_WasForced;
#endif

  bool _memUsage_WasSet;
  UInt64 _memUsage_Compress;
  UInt64 _memUsage_Decompress;
  size_t _memAvail;

  bool SetCommonProperty(const UString &name, const PROPVARIANT &value, HRESULT &hres);

  CCommonMethodProps() { InitCommon(); }
};


#ifndef Z7_EXTRACT_ONLY

class CMultiMethodProps: public CCommonMethodProps
{
  UInt32 _level;
  int _analysisLevel;

  void InitMulti();
public:
  UInt32 _crcSize;
  CObjectVector<COneMethodInfo> _methods;
  COneMethodInfo _filterMethod;
  bool _autoFilter;

  
  void SetGlobalLevelTo(COneMethodInfo &oneMethodInfo) const;

#ifndef Z7_ST
  static void SetMethodThreadsTo_IfNotFinded(CMethodProps &props, UInt32 numThreads);
  static void SetMethodThreadsTo_Replace(CMethodProps &props, UInt32 numThreads);
  
  static void Set_Method_NumThreadGroups_IfNotFinded(CMethodProps &props, UInt32 numThreadGroups);
#endif


  unsigned GetNumEmptyMethods() const
  {
    unsigned i;
    for (i = 0; i < _methods.Size(); i++)
      if (!_methods[i].IsEmpty())
        break;
    return i;
  }

  int GetLevel() const { return _level == (UInt32)(Int32)-1 ? 5 : (int)_level; }
  int GetAnalysisLevel() const { return _analysisLevel; }

  void Init();
  CMultiMethodProps() { InitMulti(); }

  HRESULT SetProperty(const wchar_t *name, const PROPVARIANT &value);
};


class CSingleMethodProps: public COneMethodInfo, public CCommonMethodProps
{
  UInt32 _level;

  void InitSingle()
  {
    _level = (UInt32)(Int32)-1;
  }

public:
  void Init();
  CSingleMethodProps() { InitSingle(); }
  
  int GetLevel() const { return _level == (UInt32)(Int32)-1 ? 5 : (int)_level; }
  HRESULT SetProperty(const wchar_t *name, const PROPVARIANT &values);
  HRESULT SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps);
};

#endif

struct CHandlerTimeOptions
{
  CBoolPair Write_MTime;
  CBoolPair Write_ATime;
  CBoolPair Write_CTime;
  UInt32 Prec;

  void Init()
  {
    Write_MTime.Init();
    Write_MTime.Val = true;
    Write_ATime.Init();
    Write_CTime.Init();
    Prec = (UInt32)(Int32)-1;
  }

  CHandlerTimeOptions()
  {
    Init();
  }

  HRESULT Parse(const UString &name, const PROPVARIANT &prop, bool &processed);
};

}

#endif

/* ---- CPP/7zip/PropID.h ---- */
// PropID.h

#ifndef ZIP7_INC_7ZIP_PROP_ID_H
#define ZIP7_INC_7ZIP_PROP_ID_H

// amalgamation: header emitted in prologue

enum
{
  kpidNoProperty = 0,
  kpidMainSubfile,
  kpidHandlerItemIndex,
  kpidPath,
  kpidName,
  kpidExtension,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidAttrib,
  kpidCTime,
  kpidATime,
  kpidMTime,
  kpidSolid,
  kpidCommented,
  kpidEncrypted,
  kpidSplitBefore,
  kpidSplitAfter,
  kpidDictionarySize,
  kpidCRC,
  kpidType,
  kpidIsAnti,
  kpidMethod,
  kpidHostOS,
  kpidFileSystem,
  kpidUser,
  kpidGroup,
  kpidBlock,
  kpidComment,
  kpidPosition,
  kpidPrefix,
  kpidNumSubDirs,
  kpidNumSubFiles,
  kpidUnpackVer,
  kpidVolume,
  kpidIsVolume,
  kpidOffset,
  kpidLinks,
  kpidNumBlocks,
  kpidNumVolumes,
  kpidTimeType,
  kpidBit64,
  kpidBigEndian,
  kpidCpu,
  kpidPhySize,
  kpidHeadersSize,
  kpidChecksum,
  kpidCharacts,
  kpidVa,
  kpidId,
  kpidShortName,
  kpidCreatorApp,
  kpidSectorSize,
  kpidPosixAttrib,
  kpidSymLink,
  kpidError,
  kpidTotalSize,
  kpidFreeSpace,
  kpidClusterSize,
  kpidVolumeName,
  kpidLocalName,
  kpidProvider,
  kpidNtSecure,
  kpidIsAltStream,
  kpidIsAux,
  kpidIsDeleted,
  kpidIsTree,
  kpidSha1,
  kpidSha256,
  kpidErrorType,
  kpidNumErrors,
  kpidErrorFlags,
  kpidWarningFlags,
  kpidWarning,
  kpidNumStreams,
  kpidNumAltStreams,
  kpidAltStreamsSize,
  kpidVirtualSize,
  kpidUnpackSize,
  kpidTotalPhySize,
  kpidVolumeIndex,
  kpidSubType,
  kpidShortComment,
  kpidCodePage,
  kpidIsNotArcType,
  kpidPhySizeCantBeDetected,
  kpidZerosTailIsAllowed,
  kpidTailSize,
  kpidEmbeddedStubSize,
  kpidNtReparse,
  kpidHardLink,
  kpidINode,
  kpidStreamId,
  kpidReadOnly,
  kpidOutName,
  kpidCopyLink,
  kpidArcFileName,
  kpidIsHash,
  kpidChangeTime,
  kpidUserId,
  kpidGroupId,
  kpidDeviceMajor,
  kpidDeviceMinor,
  kpidDevMajor,
  kpidDevMinor,

  kpid_NUM_DEFINED,

  kpidUserDefined = 0x10000
};

extern const Byte k7z_PROPID_To_VARTYPE[kpid_NUM_DEFINED]; // VARTYPE

const UInt32 kpv_ErrorFlags_IsNotArc              = 1 << 0;
const UInt32 kpv_ErrorFlags_HeadersError          = 1 << 1;
const UInt32 kpv_ErrorFlags_EncryptedHeadersError = 1 << 2;
const UInt32 kpv_ErrorFlags_UnavailableStart      = 1 << 3;
const UInt32 kpv_ErrorFlags_UnconfirmedStart      = 1 << 4;
const UInt32 kpv_ErrorFlags_UnexpectedEnd         = 1 << 5;
const UInt32 kpv_ErrorFlags_DataAfterEnd          = 1 << 6;
const UInt32 kpv_ErrorFlags_UnsupportedMethod     = 1 << 7;
const UInt32 kpv_ErrorFlags_UnsupportedFeature    = 1 << 8;
const UInt32 kpv_ErrorFlags_DataError             = 1 << 9;
const UInt32 kpv_ErrorFlags_CrcError              = 1 << 10;
// const UInt32 kpv_ErrorFlags_Unsupported           = 1 << 11;

/*
linux ctime :
   file metadata was last changed.
   changing the file modification time
   counts as a metadata change, so will also have the side effect of updating the ctime.

PROPVARIANT for timestamps in 7-Zip:
{
  vt = VT_FILETIME
  wReserved1: set precision level
    0      : base value (backward compatibility value)
             only filetime is used (7 digits precision).
             wReserved2 and wReserved3 can contain random data
    1      : Unix (1 sec)
    2      : DOS  (2 sec)
    3      : High Precision (1 ns)
    16 - 3 : (reserved) = 1 day
    16 - 2 : (reserved) = 1 hour
    16 - 1 : (reserved) = 1 minute
    16 + 0 : 1 sec (0 digits after point)
    16 + (1,2,3,4,5,6,7,8,9) : set subsecond precision level :
         (number of decimal digits after point)
    16 + 9 : 1 ns  (9 digits after point)
  wReserved2 = ns % 100 : if     (8 or 9 digits pecision)
             = 0        : if not (8 or 9 digits pecision)
  wReserved3 = 0;
  filetime
}

NOTE: TAR-PAX archives created by GNU TAR don't keep
  whole information about original level of precision,
  and timestamp are stored in reduced form, where tail zero
  digits after point are removed.
  So 7-Zip can return different precision levels for different items for such TAR archives.
*/

/*
TimePrec returned by IOutArchive::GetFileTimeType()
is used only for updating, when we compare MTime timestamp
from archive with timestamp from directory.
*/

#endif

/* ---- CPP/7zip/Archive/IArchive.h ---- */
// IArchive.h

#ifndef ZIP7_INC_IARCHIVE_H
#define ZIP7_INC_IARCHIVE_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

Z7_PURE_INTERFACES_BEGIN


#define Z7_IFACE_CONSTR_ARCHIVE_SUB(i, base, n) \
  Z7_DECL_IFACE_7ZIP_SUB(i, base, 6, n) \
  { Z7_IFACE_COM7_PURE(i) };

#define Z7_IFACE_CONSTR_ARCHIVE(i, n) \
  Z7_IFACE_CONSTR_ARCHIVE_SUB(i, IUnknown, n)

/*
How the function in 7-Zip returns object for output parameter via pointer

1) The caller sets the value of variable before function call:
  PROPVARIANT  :  vt = VT_EMPTY
  BSTR         :  NULL
  IUnknown* and derived interfaces  :  NULL
  another scalar types  :  any non-initialized value is allowed

2) The callee in current 7-Zip code now can free input object for output parameter:
  PROPVARIANT   : the callee calls VariantClear(propvaiant_ptr) for input
                  value stored in variable
  another types : the callee ignores stored value.

3) The callee writes new value to variable for output parameter and
  returns execution to caller.

4) The caller must free or release object returned by the callee:
  PROPVARIANT   : VariantClear(&propvaiant)
  BSTR          : SysFreeString(bstr)
  IUnknown* and derived interfaces  :  if (ptr) ptr->Relase()
*/


namespace NFileTimeType
{
  enum EEnum
  {
    kNotDefined = -1,
    kWindows = 0,
    kUnix,
    kDOS,
    k1ns
  };
}

namespace NArcInfoFlags
{
  const UInt32 kKeepName        = 1 << 0;  // keep name of file in archive name
  const UInt32 kAltStreams      = 1 << 1;  // the handler supports alt streams
  const UInt32 kNtSecure        = 1 << 2;  // the handler supports NT security
  const UInt32 kFindSignature   = 1 << 3;  // the handler can find start of archive
  const UInt32 kMultiSignature  = 1 << 4;  // there are several signatures
  const UInt32 kUseGlobalOffset = 1 << 5;  // the seek position of stream must be set as global offset
  const UInt32 kStartOpen       = 1 << 6;  // call handler for each start position
  const UInt32 kPureStartOpen   = 1 << 7;  // call handler only for start of file
  const UInt32 kBackwardOpen    = 1 << 8;  // archive can be open backward
  const UInt32 kPreArc          = 1 << 9;  // such archive can be stored before real archive (like SFX stub)
  const UInt32 kSymLinks        = 1 << 10; // the handler supports symbolic links
  const UInt32 kHardLinks       = 1 << 11; // the handler supports hard links
  const UInt32 kByExtOnlyOpen   = 1 << 12; // call handler only if file extension matches
  const UInt32 kHashHandler     = 1 << 13; // the handler contains the hashes (checksums)
  const UInt32 kCTime           = 1 << 14;
  const UInt32 kCTime_Default   = 1 << 15;
  const UInt32 kATime           = 1 << 16;
  const UInt32 kATime_Default   = 1 << 17;
  const UInt32 kMTime           = 1 << 18;
  const UInt32 kMTime_Default   = 1 << 19;
  // const UInt32 kTTime_Reserved         = 1 << 20;
  // const UInt32 kTTime_Reserved_Default = 1 << 21;
}

namespace NArcInfoTimeFlags
{
  const unsigned kTime_Prec_Mask_bit_index = 0;
  const unsigned kTime_Prec_Mask_num_bits = 26;

  const unsigned kTime_Prec_Default_bit_index = 27;
  const unsigned kTime_Prec_Default_num_bits = 5;
}

#define TIME_PREC_TO_ARC_FLAGS_MASK(v) \
  ((UInt32)1 << (NArcInfoTimeFlags::kTime_Prec_Mask_bit_index + (v)))

#define TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT(v) \
  ((UInt32)(v) << NArcInfoTimeFlags::kTime_Prec_Default_bit_index)

namespace NArchive
{
  namespace NHandlerPropID
  {
    enum
    {
      kName = 0,        // VT_BSTR
      kClassID,         // binary GUID in VT_BSTR
      kExtension,       // VT_BSTR
      kAddExtension,    // VT_BSTR
      kUpdate,          // VT_BOOL
      kKeepName,        // VT_BOOL
      kSignature,       // binary in VT_BSTR
      kMultiSignature,  // binary in VT_BSTR
      kSignatureOffset, // VT_UI4
      kAltStreams,      // VT_BOOL
      kNtSecure,        // VT_BOOL
      kFlags,           // VT_UI4
      kTimeFlags        // VT_UI4
    };
  }

  namespace NExtract
  {
    namespace NAskMode
    {
      enum
      {
        kExtract = 0,
        kTest,
        kSkip,
        kReadExternal
      };
    }
  
    namespace NOperationResult
    {
      enum
      {
        kOK = 0,
        kUnsupportedMethod,
        kDataError,
        kCRCError,
        kUnavailable,
        kUnexpectedEnd,
        kDataAfterEnd,
        kIsNotArc,
        kHeadersError,
        kWrongPassword
        // , kMemError
      };
    }
  }

  namespace NEventIndexType
  {
    enum
    {
      kNoIndex = 0,
      kInArcIndex,
      kBlockIndex,
      kOutArcIndex
      // kArcProp
    };
  }
  
  namespace NUpdate
  {
    namespace NOperationResult
    {
      enum
      {
        kOK = 0
        // kError = 1,
        // kError_FileChanged
      };
    }
  }
}

#define Z7_IFACEM_IArchiveOpenCallback(x) \
  x(SetTotal(const UInt64 *files, const UInt64 *bytes)) \
  x(SetCompleted(const UInt64 *files, const UInt64 *bytes)) \

Z7_IFACE_CONSTR_ARCHIVE(IArchiveOpenCallback, 0x10)

/*
IArchiveExtractCallback::

7-Zip doesn't call IArchiveExtractCallback functions
  GetStream()
  PrepareOperation()
  SetOperationResult()
from different threads simultaneously.
But 7-Zip can call functions for IProgress or ICompressProgressInfo functions
from another threads simultaneously with calls for IArchiveExtractCallback interface.

IArchiveExtractCallback::GetStream()
  UInt32 index - index of item in Archive
  Int32 askExtractMode  (Extract::NAskMode)
    if (askMode != NExtract::NAskMode::kExtract)
    {
      then the callee doesn't write data to stream: (*outStream == NULL)
    }
  
  Out:
      (*outStream == NULL) - for directories
      (*outStream == NULL) - if link (hard link or symbolic link) was created
      if (*outStream == NULL && askMode == NExtract::NAskMode::kExtract)
      {
        then the caller must skip extracting of that file.
      }

  returns:
    S_OK     : OK
    S_FALSE  : data error (for decoders)

if (IProgress::SetTotal() was called)
{
  IProgress::SetCompleted(completeValue) uses
    packSize   - for some stream formats (xz, gz, bz2, lzma, z, ppmd).
    unpackSize - for another formats.
}
else
{
  IProgress::SetCompleted(completeValue) uses packSize.
}

SetOperationResult()
  7-Zip calls SetOperationResult at the end of extracting,
  so the callee can close the file, set attributes, timestamps and security information.

  Int32 opRes (NExtract::NOperationResult)
*/

// INTERFACE_IProgress(x)

#define Z7_IFACEM_IArchiveExtractCallback(x) \
  x(GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)) \
  x(PrepareOperation(Int32 askExtractMode)) \
  x(SetOperationResult(Int32 opRes)) \

Z7_IFACE_CONSTR_ARCHIVE_SUB(IArchiveExtractCallback, IProgress, 0x20)



/*
v23:
IArchiveExtractCallbackMessage2 can be requested from IArchiveExtractCallback object
  by Extract() or UpdateItems() functions to report about extracting errors
ReportExtractResult()
  UInt32 indexType (NEventIndexType)
  UInt32 index
  Int32 opRes (NExtract::NOperationResult)
*/
/*
before v23:
#define Z7_IFACEM_IArchiveExtractCallbackMessage(x) \
  x(ReportExtractResult(UInt32 indexType, UInt32 index, Int32 opRes))
Z7_IFACE_CONSTR_ARCHIVE_SUB(IArchiveExtractCallbackMessage, IProgress, 0x21)
*/
#define Z7_IFACEM_IArchiveExtractCallbackMessage2(x) \
  x(ReportExtractResult(UInt32 indexType, UInt32 index, Int32 opRes))
Z7_IFACE_CONSTR_ARCHIVE(IArchiveExtractCallbackMessage2, 0x22)

#define Z7_IFACEM_IArchiveOpenVolumeCallback(x) \
  x(GetProperty(PROPID propID, PROPVARIANT *value)) \
  x(GetStream(const wchar_t *name, IInStream **inStream))
Z7_IFACE_CONSTR_ARCHIVE(IArchiveOpenVolumeCallback, 0x30)


#define Z7_IFACEM_IInArchiveGetStream(x) \
  x(GetStream(UInt32 index, ISequentialInStream **stream))
Z7_IFACE_CONSTR_ARCHIVE(IInArchiveGetStream, 0x40)

#define Z7_IFACEM_IArchiveOpenSetSubArchiveName(x) \
  x(SetSubArchiveName(const wchar_t *name))
Z7_IFACE_CONSTR_ARCHIVE(IArchiveOpenSetSubArchiveName, 0x50)


/*
IInArchive::Open
    stream
      if (kUseGlobalOffset), stream current position can be non 0.
      if (!kUseGlobalOffset), stream current position is 0.
    if (maxCheckStartPosition == NULL), the handler can try to search archive start in stream
    if (*maxCheckStartPosition == 0), the handler must check only current position as archive start

IInArchive::Extract:
  indices must be sorted
  numItems = (UInt32)(Int32)-1 = 0xFFFFFFFF means "all files"
  testMode != 0 means "test files without writing to outStream"

IInArchive::GetArchiveProperty:
  kpidOffset  - start offset of archive.
      VT_EMPTY : means offset = 0.
      VT_UI4, VT_UI8, VT_I8 : result offset; negative values is allowed
  kpidPhySize - size of archive. VT_EMPTY means unknown size.
    kpidPhySize is allowed to be larger than file size. In that case it must show
    supposed size.

  kpidIsDeleted:
  kpidIsAltStream:
  kpidIsAux:
  kpidINode:
    must return VARIANT_TRUE (VT_BOOL), if archive can support that property in GetProperty.


Notes:
  Don't call IInArchive functions for same IInArchive object from different threads simultaneously.
  Some IInArchive handlers will work incorrectly in that case.
*/

#if defined(_MSC_VER) && !defined(__clang__)
  #define MY_NO_THROW_DECL_ONLY  Z7_COM7F_E
#else
  #define MY_NO_THROW_DECL_ONLY
#endif

#define Z7_IFACEM_IInArchive(x) \
  x(Open(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openCallback)) \
  x(Close()) \
  x(GetNumberOfItems(UInt32 *numItems)) \
  x(GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  x(Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback)) \
  x(GetArchiveProperty(PROPID propID, PROPVARIANT *value)) \
  x(GetNumberOfProperties(UInt32 *numProps)) \
  x(GetPropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \
  x(GetNumberOfArchiveProperties(UInt32 *numProps)) \
  x(GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \

Z7_IFACE_CONSTR_ARCHIVE(IInArchive, 0x60)

namespace NParentType
{
  enum
  {
    kDir = 0,
    kAltStream
  };
}

namespace NPropDataType
{
  const UInt32 kMask_ZeroEnd   = 1 << 4;
  // const UInt32 kMask_BigEndian = 1 << 5;
  const UInt32 kMask_Utf       = 1 << 6;
  const UInt32 kMask_Utf8  = kMask_Utf | 0;
  const UInt32 kMask_Utf16 = kMask_Utf | 1;
  // const UInt32 kMask_Utf32 = kMask_Utf | 2;

  const UInt32 kNotDefined = 0;
  const UInt32 kRaw = 1;

  const UInt32 kUtf8z  = kMask_Utf8  | kMask_ZeroEnd;
  const UInt32 kUtf16z = kMask_Utf16 | kMask_ZeroEnd;
}

// UTF string (pointer to wchar_t) with zero end and little-endian.
#define PROP_DATA_TYPE_wchar_t_PTR_Z_LE ((NPropDataType::kMask_Utf | NPropDataType::kMask_ZeroEnd) + (sizeof(wchar_t) >> 1))


/*
GetRawProp:
  Result:
    S_OK - even if property is not set
*/

#define Z7_IFACEM_IArchiveGetRawProps(x) \
  x(GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType)) \
  x(GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType)) \
  x(GetNumRawProps(UInt32 *numProps)) \
  x(GetRawPropInfo(UInt32 index, BSTR *name, PROPID *propID))

Z7_IFACE_CONSTR_ARCHIVE(IArchiveGetRawProps, 0x70)

#define Z7_IFACEM_IArchiveGetRootProps(x) \
  x(GetRootProp(PROPID propID, PROPVARIANT *value)) \
  x(GetRootRawProp(PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType)) \
 
Z7_IFACE_CONSTR_ARCHIVE(IArchiveGetRootProps, 0x71)

#define Z7_IFACEM_IArchiveOpenSeq(x) \
  x(OpenSeq(ISequentialInStream *stream)) \

Z7_IFACE_CONSTR_ARCHIVE(IArchiveOpenSeq, 0x61)

/*
  OpenForSize
  Result:
    S_FALSE - is not archive
    ? - DATA error
*/
    
/*
const UInt32 kOpenFlags_RealPhySize = 1 << 0;
const UInt32 kOpenFlags_NoSeek = 1 << 1;
// const UInt32 kOpenFlags_BeforeExtract = 1 << 2;
*/

/*
Flags:
   0 - opens archive with IInStream, if IInStream interface is supported
     - if phySize is not available, it doesn't try to make full parse to get phySize
   kOpenFlags_NoSeek -  ArcOpen2 function doesn't use IInStream interface, even if it's available
   kOpenFlags_RealPhySize - the handler will try to get PhySize, even if it requires full decompression for file
   
  if handler is not allowed to use IInStream and the flag kOpenFlags_RealPhySize is not specified,
  the handler can return S_OK, but it doesn't check even Signature.
  So next Extract can be called for that sequential stream.
*/
/*
#define Z7_IFACEM_IArchiveOpen2(x) \
  x(ArcOpen2(ISequentialInStream *stream, UInt32 flags, IArchiveOpenCallback *openCallback))
Z7_IFACE_CONSTR_ARCHIVE(IArchiveOpen2, 0x62)
*/

// ---------- UPDATE ----------

/*
GetUpdateItemInfo outs:
*newData  *newProps
   0        0      - Copy data and properties from archive
   0        1      - Copy data from archive, request new properties
   1        0      - that combination is unused now
   1        1      - Request new data and new properties. It can be used even for folders

  indexInArchive = -1 if there is no item in archive, or if it doesn't matter.


GetStream out:
  Result:
    S_OK:
      (*inStream == NULL) - only for directories
                          - the bug was fixed in 9.33: (*Stream == NULL) was in case of anti-file
      (*inStream != NULL) - for any file, even for empty file or anti-file
    S_FALSE - skip that file (don't add item to archive) - (client code can't open stream of that file by some reason)
      (*inStream == NULL)

The order of calling for hard links:
  - GetStream()
  - GetProperty(kpidHardLink)

SetOperationResult()
  Int32 opRes (NExtract::NOperationResult::kOK)
*/

// INTERFACE_IProgress(x)
#define Z7_IFACEM_IArchiveUpdateCallback(x) \
  x(GetUpdateItemInfo(UInt32 index, Int32 *newData, Int32 *newProps, UInt32 *indexInArchive)) \
  x(GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  x(GetStream(UInt32 index, ISequentialInStream **inStream)) \
  x(SetOperationResult(Int32 operationResult)) \

Z7_IFACE_CONSTR_ARCHIVE_SUB(IArchiveUpdateCallback, IProgress, 0x80)

// INTERFACE_IArchiveUpdateCallback(x)
#define Z7_IFACEM_IArchiveUpdateCallback2(x) \
  x(GetVolumeSize(UInt32 index, UInt64 *size)) \
  x(GetVolumeStream(UInt32 index, ISequentialOutStream **volumeStream)) \

Z7_IFACE_CONSTR_ARCHIVE_SUB(IArchiveUpdateCallback2, IArchiveUpdateCallback, 0x82)

namespace NUpdateNotifyOp
{
  enum
  {
    kAdd = 0,
    kUpdate,
    kAnalyze,
    kReplicate,
    kRepack,
    kSkip,
    kDelete,
    kHeader,
    kHashRead,
    kInFileChanged
    // , kOpFinished
    // , kNumDefined
  };
}

/*
IArchiveUpdateCallbackFile::ReportOperation
  UInt32 indexType (NEventIndexType)
  UInt32 index
  UInt32 notifyOp (NUpdateNotifyOp)
*/

#define Z7_IFACEM_IArchiveUpdateCallbackFile(x) \
  x(GetStream2(UInt32 index, ISequentialInStream **inStream, UInt32 notifyOp)) \
  x(ReportOperation(UInt32 indexType, UInt32 index, UInt32 notifyOp)) \

Z7_IFACE_CONSTR_ARCHIVE(IArchiveUpdateCallbackFile, 0x83)


#define Z7_IFACEM_IArchiveGetDiskProperty(x) \
  x(GetDiskProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  
Z7_IFACE_CONSTR_ARCHIVE(IArchiveGetDiskProperty, 0x84)

/*
#define Z7_IFACEM_IArchiveUpdateCallbackArcProp(x) \
  x(ReportProp(UInt32 indexType, UInt32 index, PROPID propID, const PROPVARIANT *value)) \
  x(ReportRawProp(UInt32 indexType, UInt32 index, PROPID propID, const void *data, UInt32 dataSize, UInt32 propType)) \
  x(ReportFinished(UInt32 indexType, UInt32 index, Int32 opRes)) \
  x(DoNeedArcProp(PROPID propID, Int32 *answer)) \
 
Z7_IFACE_CONSTR_ARCHIVE(IArchiveUpdateCallbackArcProp, 0x85)
*/

/*
UpdateItems()
-------------

  outStream: output stream. (the handler) MUST support the case when
    Seek position in outStream is not ZERO.
    but the caller calls with empty outStream and seek position is ZERO??
 
  archives with stub:

  If archive is open and the handler and (Offset > 0), then the handler
  knows about stub size.
  UpdateItems():
  1) the handler MUST copy that stub to outStream
  2) the caller MUST NOT copy the stub to outStream, if
     "rsfx" property is set with SetProperties

  the handler must support the case where
    ISequentialOutStream *outStream
*/


#define Z7_IFACEM_IOutArchive(x) \
  x(UpdateItems(ISequentialOutStream *outStream, UInt32 numItems, IArchiveUpdateCallback *updateCallback)) \
  x(GetFileTimeType(UInt32 *type))

Z7_IFACE_CONSTR_ARCHIVE(IOutArchive, 0xA0)


/*
ISetProperties::SetProperties()
  PROPVARIANT values[i].vt:
    VT_EMPTY
    VT_BOOL
    VT_UI4   - if 32-bit number
    VT_UI8   - if 64-bit number
    VT_BSTR
*/

#define Z7_IFACEM_ISetProperties(x) \
  x(SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))

Z7_IFACE_CONSTR_ARCHIVE(ISetProperties, 0x03)

#define Z7_IFACEM_IArchiveKeepModeForNextOpen(x) \
  x(KeepModeForNextOpen()) \

Z7_IFACE_CONSTR_ARCHIVE(IArchiveKeepModeForNextOpen, 0x04)

/* Exe handler: the handler for executable format (PE, ELF, Mach-O).
   SFX archive: executable stub + some tail data.
     before 9.31: exe handler didn't parse SFX archives as executable format.
     for 9.31+: exe handler parses SFX archives as executable format, only if AllowTail(1) was called */

#define Z7_IFACEM_IArchiveAllowTail(x) \
  x(AllowTail(Int32 allowTail)) \

Z7_IFACE_CONSTR_ARCHIVE(IArchiveAllowTail, 0x05)


namespace NRequestMemoryUseFlags
{
  const UInt32 k_AllowedSize_WasForced    = 1 << 0;  // (*allowedSize) was forced by -mmemx or -smemx
  const UInt32 k_DefaultLimit_Exceeded    = 1 << 1;  // default limit of archive format was exceeded
  const UInt32 k_MLimit_Exceeded          = 1 << 2;  // -mmemx value was exceeded
  const UInt32 k_SLimit_Exceeded          = 1 << 3;  // -smemx value was exceeded
  
  const UInt32 k_NoErrorMessage           = 1 << 10; // do not show error message, and show only request
  const UInt32 k_IsReport                 = 1 << 11; // only report is required, without user request
  
  const UInt32 k_SkipArc_IsExpected       = 1 << 12; // NRequestMemoryAnswerFlags::k_SkipArc flag answer is expected
  const UInt32 k_Report_SkipArc           = 1 << 13; // report about SkipArc operation

  // const UInt32 k_SkipBigFile_IsExpected   = 1 << 14; // NRequestMemoryAnswerFlags::k_SkipBigFiles flag answer is expected (unused)
  // const UInt32 k_Report_SkipBigFile       = 1 << 15; // report about SkipFile operation (unused)

  // const UInt32 k_SkipBigFiles_IsExpected  = 1 << 16; // NRequestMemoryAnswerFlags::k_SkipBigFiles flag answer is expected (unused)
  // const UInt32 k_Report_SkipBigFiles      = 1 << 17; // report that all big files will be skipped (unused)
}

namespace NRequestMemoryAnswerFlags
{
  const UInt32 k_Allow          = 1 << 0;  // allow further archive extraction
  const UInt32 k_Stop           = 1 << 1;  // for exit (and return_code == E_ABORT is used)
  const UInt32 k_SkipArc        = 1 << 2;  // skip current archive extraction
  // const UInt32 k_SkipBigFile    = 1 << 4;  // skip extracting of files that exceed limit (unused)
  // const UInt32 k_SkipBigFiles   = 1 << 5;  // skip extracting of files that exceed limit (unused)
  const UInt32 k_Limit_Exceeded  = 1 << 10;  // limit was exceeded
}

/*
  *allowedSize is in/out:
    in  : default allowed memory usage size or forced size, if it was changed by switch -mmemx.
    out : value specified by user or unchanged value.

  *answerFlags is in/out:
    *answerFlags must be set by caller before calling for default action,

  indexType : must be set with NEventIndexType::* constant
          (indexType == kNoIndex), if request for whole archive.
  index : must be set for some (indexType) types (if
          fileIndex , if (indexType == NEventIndexType::kInArcIndex)
          0, if       if (indexType == kNoIndex)
  path : NULL can be used for any indexType.
*/
#define Z7_IFACEM_IArchiveRequestMemoryUseCallback(x) \
  x(RequestMemoryUse(UInt32 flags, UInt32 indexType, UInt32 index, const wchar_t *path, \
    UInt64 requiredSize, UInt64 *allowedSize, UInt32 *answerFlags))
Z7_IFACE_CONSTR_ARCHIVE(IArchiveRequestMemoryUseCallback, 0x09)


struct CStatProp
{
  const char *Name;
  UInt32 PropID;
  VARTYPE vt;
};

namespace NWindows {
namespace NCOM {
// PropVariant.cpp
BSTR AllocBstrFromAscii(const char *s) throw();
}}


#define IMP_IInArchive_GetProp_Base(fn, f, k) \
  Z7_COM7F_IMF(CHandler::fn(UInt32 *numProps)) \
    { *numProps = Z7_ARRAY_SIZE(k); return S_OK; } \
  Z7_COM7F_IMF(CHandler::f(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \
    { if (index >= Z7_ARRAY_SIZE(k)) return E_INVALIDARG; \

#define IMP_IInArchive_GetProp_NO_NAME(fn, f, k) \
  IMP_IInArchive_GetProp_Base(fn, f, k) \
    *propID = k[index]; \
    *varType = k7z_PROPID_To_VARTYPE[(unsigned)*propID]; \
    *name = NULL; return S_OK; } \

#define IMP_IInArchive_GetProp_WITH_NAME(fn, f, k) \
  IMP_IInArchive_GetProp_Base(fn, f, k) \
    const CStatProp &prop = k[index]; \
    *propID = (PROPID)prop.PropID; \
    *varType = prop.vt; \
    *name = NWindows::NCOM::AllocBstrFromAscii(prop.Name); return S_OK; } \


#define IMP_IInArchive_Props \
  IMP_IInArchive_GetProp_NO_NAME(GetNumberOfProperties, GetPropertyInfo, kProps)

#define IMP_IInArchive_Props_WITH_NAME \
  IMP_IInArchive_GetProp_WITH_NAME(GetNumberOfProperties, GetPropertyInfo, kProps)

#define IMP_IInArchive_ArcProps \
  IMP_IInArchive_GetProp_NO_NAME(GetNumberOfArchiveProperties, GetArchivePropertyInfo, kArcProps)

#define IMP_IInArchive_ArcProps_WITH_NAME \
  IMP_IInArchive_GetProp_WITH_NAME(GetNumberOfArchiveProperties, GetArchivePropertyInfo, kArcProps)

#define IMP_IInArchive_ArcProps_NO_Table \
  Z7_COM7F_IMF(CHandler::GetNumberOfArchiveProperties(UInt32 *numProps)) \
    { *numProps = 0; return S_OK; } \
  Z7_COM7F_IMF(CHandler::GetArchivePropertyInfo(UInt32, BSTR *, PROPID *, VARTYPE *)) \
    { return E_NOTIMPL; } \

#define IMP_IInArchive_ArcProps_NO \
  IMP_IInArchive_ArcProps_NO_Table \
  Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID, PROPVARIANT *value)) \
    { value->vt = VT_EMPTY; return S_OK; }


#define Z7_class_CHandler_final \
        Z7_class_final(CHandler)


#define Z7_CLASS_IMP_CHandler_IInArchive_0 \
  Z7_CLASS_IMP_COM_1(CHandler, IInArchive)
#define Z7_CLASS_IMP_CHandler_IInArchive_1(i1) \
  Z7_CLASS_IMP_COM_2(CHandler, IInArchive, i1)
#define Z7_CLASS_IMP_CHandler_IInArchive_2(i1, i2) \
  Z7_CLASS_IMP_COM_3(CHandler, IInArchive, i1, i2)
#define Z7_CLASS_IMP_CHandler_IInArchive_3(i1, i2, i3) \
  Z7_CLASS_IMP_COM_4(CHandler, IInArchive, i1, i2, i3)
#define Z7_CLASS_IMP_CHandler_IInArchive_4(i1, i2, i3, i4) \
  Z7_CLASS_IMP_COM_5(CHandler, IInArchive, i1, i2, i3, i4)
#define Z7_CLASS_IMP_CHandler_IInArchive_5(i1, i2, i3, i4, i5) \
  Z7_CLASS_IMP_COM_6(CHandler, IInArchive, i1, i2, i3, i4, i5)



#define k_IsArc_Res_NO   0
#define k_IsArc_Res_YES  1
#define k_IsArc_Res_NEED_MORE 2
// #define k_IsArc_Res_YES_LOW_PROB 3

#define API_FUNC_IsArc EXTERN_C UInt32 WINAPI
#define API_FUNC_static_IsArc extern "C" { static UInt32 WINAPI

extern "C"
{
  typedef HRESULT (WINAPI *Func_CreateObject)(const GUID *clsID, const GUID *iid, void **outObject);

  typedef UInt32 (WINAPI *Func_IsArc)(const Byte *p, size_t size);
  typedef HRESULT (WINAPI *Func_GetIsArc)(UInt32 formatIndex, Func_IsArc *isArc);

  typedef HRESULT (WINAPI *Func_GetNumberOfFormats)(UInt32 *numFormats);
  typedef HRESULT (WINAPI *Func_GetHandlerProperty)(PROPID propID, PROPVARIANT *value);
  typedef HRESULT (WINAPI *Func_GetHandlerProperty2)(UInt32 index, PROPID propID, PROPVARIANT *value);

  typedef HRESULT (WINAPI *Func_SetCaseSensitive)(Int32 caseSensitive);
  typedef HRESULT (WINAPI *Func_SetLargePageMode)();
  typedef HRESULT (WINAPI *Func_SetLargePageMode2)(UInt32 flags, size_t pageSize, size_t threshold);
  // typedef HRESULT (WINAPI *Func_SetClientVersion)(UInt32 version);

  typedef IOutArchive * (*Func_CreateOutArchive)();
  typedef IInArchive * (*Func_CreateInArchive)();
}


/*
  if there is no time in archive, external MTime of archive
  will be used instead of _item.Time from archive.
  For 7-zip before 22.00 we need to return some supported value.
  But (kpidTimeType > kDOS) is not allowed in 7-Zip before 22.00.
  So we return highest precision value supported by old 7-Zip.
  new 7-Zip 22.00 doesn't use that value in usual cases.
*/


#define DECLARE_AND_SET_CLIENT_VERSION_VAR
#define GET_FileTimeType_NotDefined_for_GetFileTimeType \
      NFileTimeType::kWindows

/*
extern UInt32 g_ClientVersion;

#define GET_CLIENT_VERSION(major, minor)  \
  ((UInt32)(((UInt32)(major) << 16) | (UInt32)(minor)))

#define DECLARE_AND_SET_CLIENT_VERSION_VAR \
  UInt32 g_ClientVersion = GET_CLIENT_VERSION(MY_VER_MAJOR, MY_VER_MINOR);

#define GET_FileTimeType_NotDefined_for_GetFileTimeType \
      ((UInt32)(g_ClientVersion >= GET_CLIENT_VERSION(22, 0) ? \
        (UInt32)(Int32)NFileTimeType::kNotDefined : \
        NFileTimeType::kWindows))
*/

Z7_PURE_INTERFACES_END
#endif

/* ---- CPP/Common/MyLinux.h ---- */
// MyLinux.h

#ifndef ZIP7_INC_COMMON_MY_LINUX_H
#define ZIP7_INC_COMMON_MY_LINUX_H

// #include "../../C/7zTypes.h"

#define MY_LIN_DT_UNKNOWN   0
#define MY_LIN_DT_FIFO      1
#define MY_LIN_DT_CHR       2
#define MY_LIN_DT_DIR       4
#define MY_LIN_DT_BLK       6
#define MY_LIN_DT_REG       8
#define MY_LIN_DT_LNK       10
#define MY_LIN_DT_SOCK      12
#define MY_LIN_DT_WHT       14

#define MY_LIN_S_IFMT  00170000
#define MY_LIN_S_IFSOCK 0140000
#define MY_LIN_S_IFLNK  0120000
#define MY_LIN_S_IFREG  0100000
#define MY_LIN_S_IFBLK  0060000
#define MY_LIN_S_IFDIR  0040000
#define MY_LIN_S_IFCHR  0020000
#define MY_LIN_S_IFIFO  0010000

#define MY_LIN_S_ISLNK(m)   (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFLNK)
#define MY_LIN_S_ISREG(m)   (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFREG)
#define MY_LIN_S_ISDIR(m)   (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFDIR)
#define MY_LIN_S_ISCHR(m)   (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFCHR)
#define MY_LIN_S_ISBLK(m)   (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFBLK)
#define MY_LIN_S_ISFIFO(m)  (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFIFO)
#define MY_LIN_S_ISSOCK(m)  (((m) & MY_LIN_S_IFMT) == MY_LIN_S_IFSOCK)

#define MY_LIN_S_ISUID 0004000
#define MY_LIN_S_ISGID 0002000
#define MY_LIN_S_ISVTX 0001000

#define MY_LIN_S_IRWXU 00700
#define MY_LIN_S_IRUSR 00400
#define MY_LIN_S_IWUSR 00200
#define MY_LIN_S_IXUSR 00100

#define MY_LIN_S_IRWXG 00070
#define MY_LIN_S_IRGRP 00040
#define MY_LIN_S_IWGRP 00020
#define MY_LIN_S_IXGRP 00010

#define MY_LIN_S_IRWXO 00007
#define MY_LIN_S_IROTH 00004
#define MY_LIN_S_IWOTH 00002
#define MY_LIN_S_IXOTH 00001

/*
// major/minor encoding for makedev(): MMMMMmmmmmmMMMmm:

inline UInt32 MY_dev_major(UInt64 dev)
{
  return ((UInt32)(dev >> 8) & (UInt32)0xfff) | ((UInt32)(dev >> 32) & ~(UInt32)0xfff);
}

inline UInt32 MY_dev_minor(UInt64 dev)
{
  return ((UInt32)(dev) & 0xff) | ((UInt32)(dev >> 12) & ~0xff);
}

inline UInt64 MY_dev_makedev(UInt32 __major, UInt32 __minor)
{
  return (__minor & 0xff) | ((__major & 0xfff) << 8)
      | ((UInt64) (__minor & ~0xff)  << 12)
      | ((UInt64) (__major & ~0xfff) << 32);
}
*/

#endif

/* ---- CPP/7zip/Archive/Tar/TarHeader.h ---- */
// Archive/TarHeader.h

#ifndef ZIP7_INC_ARCHIVE_TAR_HEADER_H
#define ZIP7_INC_ARCHIVE_TAR_HEADER_H

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

namespace NFileHeader
{
  const unsigned kRecordSize = 512;
  const unsigned kNameSize = 100;
  const unsigned kUserNameSize = 32;
  const unsigned kGroupNameSize = 32;
  const unsigned kPrefixSize = 155;

  const unsigned kUstarMagic_Offset = 257;

  /*
  struct CHeader
  {
    char Name[kNameSize];
    char Mode[8];
    char UID[8];
    char GID[8];
    char Size[12];
    char ModificationTime[12];
    char CheckSum[8];
    char LinkFlag;
    char LinkName[kNameSize];
    char Magic[8];
    char UserName[kUserNameSize];
    char GroupName[kGroupNameSize];
    char DeviceMajor[8];
    char DeviceMinor[8];
    char Prefix[155];
  };
  union CRecord
  {
    CHeader Header;
    Byte Padding[kRecordSize];
  };
  */

  namespace NLinkFlag
  {
    const char kOldNormal    = 0;   // Normal disk file, Unix compatible
    const char kNormal       = '0'; // Normal disk file
    const char kHardLink     = '1'; // Link to previously dumped file
    const char kSymLink      = '2'; // Symbolic link
    const char kCharacter    = '3'; // Character special file
    const char kBlock        = '4'; // Block special file
    const char kDirectory    = '5'; // Directory
    const char kFIFO         = '6'; // FIFO special file
    const char kContiguous   = '7'; // Contiguous file
    const char kGnu_LongLink = 'K';
    const char kGnu_LongName = 'L';
    const char kSparse       = 'S';
    const char kLabel        = 'V';
    const char kPax          = 'x'; // Extended header with meta data for the next file in the archive (POSIX.1-2001)
    const char kPax_2        = 'X';
    const char kGlobal       = 'g'; //  Global extended header with meta data (POSIX.1-2001)
    const char kDumpDir      = 'D'; /* GNUTYPE_DUMPDIR.
      data: list of files created by the --incremental (-G) option
      Each file name is preceded by either
        - 'Y' (file should be in this archive)
        - 'N' (file is a directory, or is not stored in the archive.)
        Each file name is terminated by a null + an additional null after
        the last file name. */
    // 'A'-'Z'  Vendor specific extensions (POSIX.1-1988)
  }

  extern const char * const kLongLink;  //   = "././@LongLink";
  extern const char * const kLongLink2; //   = "@LongLink";

  namespace NMagic
  {
    // extern const char * const kUsTar;  //  = "ustar"; // 5 chars
    // extern const char * const kGNUTar; //  = "GNUtar "; // 7 chars and a null
    // extern const char * const kEmpty;  //  = "\0\0\0\0\0\0\0\0"
    extern const char k_Posix_ustar_00[8];
    extern const char k_GNU_ustar[8];
  }
}

}}

#endif

/* ---- CPP/7zip/Archive/Tar/TarItem.h ---- */
// TarItem.h

#ifndef ZIP7_INC_ARCHIVE_TAR_ITEM_H
#define ZIP7_INC_ARCHIVE_TAR_ITEM_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

struct CSparseBlock
{
  UInt64 Offset;
  UInt64 Size;
};


enum EPaxTimeRemoveZeroMode
{
  k_PaxTimeMode_DontRemoveZero,
  k_PaxTimeMode_RemoveZero_if_PureSecondOnly,
  k_PaxTimeMode_RemoveZero_Always
};

struct CTimeOptions
{
  EPaxTimeRemoveZeroMode RemoveZeroMode;
  unsigned NumDigitsMax;

  void Init()
  {
    RemoveZeroMode = k_PaxTimeMode_RemoveZero_if_PureSecondOnly;
    NumDigitsMax = 0;
  }
  CTimeOptions() { Init(); }
};


struct CPaxTime
{
  Int32 NumDigits; // -1 means undefined
  UInt32 Ns;  // it's smaller than 1G. Even if (Sec < 0), larger (Ns) value means newer files.
  Int64 Sec;  // can be negative

  Int64 GetSec() const { return NumDigits != -1 ? Sec : 0; }

  bool IsDefined() const { return NumDigits != -1; }
  // bool IsDefined_And_nonZero() const { return NumDigits != -1 && (Sec != 0 || Ns != 0); }
  
  void Clear()
  {
    NumDigits = -1;
    Ns = 0;
    Sec = 0;
  }
  CPaxTime() { Clear(); }

  /*
  void ReducePrecison(int numDigits)
  {
    // we don't use this->NumDigits here
    if (numDigits > 0)
    {
      if (numDigits >= 9)
        return;
      UInt32 r = 1;
      for (unsigned i = numDigits; i < 9; i++)
        r *= 10;
      Ns /= r;
      Ns *= r;
      return;
    }
    Ns = 0;
    if (numDigits == 0)
      return;
    UInt32 r;
         if (numDigits == -1) r = 60;
    else if (numDigits == -2) r = 60 * 60;
    else if (numDigits == -3) r = 60 * 60 * 24;
    else return;
    Sec /= r;
    Sec *= r;
  }
  */
};


struct CPaxTimes
{
  CPaxTime MTime;
  CPaxTime ATime;
  CPaxTime CTime;
  
  void Clear()
  {
    MTime.Clear();
    ATime.Clear();
    CTime.Clear();
  }

  /*
  void ReducePrecison(int numDigits)
  {
    MTime.ReducePrecison(numDigits);
    CTime.ReducePrecison(numDigits);
    ATime.ReducePrecison(numDigits);
  }
  */
};


struct CItem
{
  UInt64 PackSize;
  UInt64 Size;
  Int64 MTime;

  char LinkFlag;
  bool DeviceMajor_Defined;
  bool DeviceMinor_Defined;

  UInt32 Mode;
  UInt32 UID;
  UInt32 GID;
  UInt32 DeviceMajor;
  UInt32 DeviceMinor;

  AString Name;
  AString LinkName;
  AString User;
  AString Group;

  char Magic[8];

  CPaxTimes PaxTimes;

  CRecordVector<CSparseBlock> SparseBlocks;

  void SetMagic_Posix(bool posixMode)
  {
    memcpy(Magic, posixMode ?
        NFileHeader::NMagic::k_Posix_ustar_00 :
        NFileHeader::NMagic::k_GNU_ustar,
        8);
  }

  bool Is_SymLink()  const { return LinkFlag == NFileHeader::NLinkFlag::kSymLink && (Size == 0); }
  bool Is_HardLink() const { return LinkFlag == NFileHeader::NLinkFlag::kHardLink; }
  bool Is_Sparse()   const { return LinkFlag == NFileHeader::NLinkFlag::kSparse; }
  
  UInt64 Get_UnpackSize() const { return Is_SymLink() ? LinkName.Len() : Size; }
  
  bool Is_PaxExtendedHeader() const
  {
    switch (LinkFlag)
    {
      case NFileHeader::NLinkFlag::kPax:
      case NFileHeader::NLinkFlag::kPax_2:
      case NFileHeader::NLinkFlag::kGlobal:
        return true;
      default: break;
    }
    return false;
  }

  UInt32 Get_Combined_Mode() const
  {
    return (Mode & ~(UInt32)MY_LIN_S_IFMT) | Get_FileTypeMode_from_LinkFlag();
  }

  void Set_LinkFlag_for_File(UInt32 mode)
  {
    char                            lf = NFileHeader::NLinkFlag::kNormal;
         if (MY_LIN_S_ISCHR(mode))  lf = NFileHeader::NLinkFlag::kCharacter;
    else if (MY_LIN_S_ISBLK(mode))  lf = NFileHeader::NLinkFlag::kBlock;
    else if (MY_LIN_S_ISFIFO(mode)) lf = NFileHeader::NLinkFlag::kFIFO;
    // else if (MY_LIN_S_ISDIR(mode))  lf = NFileHeader::NLinkFlag::kDirectory;
    // else if (MY_LIN_S_ISLNK(mode))  lf = NFileHeader::NLinkFlag::kSymLink;
    LinkFlag = lf;
  }
  
  UInt32 Get_FileTypeMode_from_LinkFlag() const
  {
    switch (LinkFlag)
    {
      /*
      case NFileHeader::NLinkFlag::kDirectory:
      case NFileHeader::NLinkFlag::kDumpDir:
        return MY_LIN_S_IFDIR;
      */
      case NFileHeader::NLinkFlag::kSymLink:    return MY_LIN_S_IFLNK;
      case NFileHeader::NLinkFlag::kBlock:      return MY_LIN_S_IFBLK;
      case NFileHeader::NLinkFlag::kCharacter:  return MY_LIN_S_IFCHR;
      case NFileHeader::NLinkFlag::kFIFO:       return MY_LIN_S_IFIFO;
      // case return MY_LIN_S_IFSOCK;
      default: break;
    }

    if (IsDir())
      return MY_LIN_S_IFDIR;
    return MY_LIN_S_IFREG;
  }

  bool IsDir() const
  {
    switch (LinkFlag)
    {
      case NFileHeader::NLinkFlag::kDirectory:
      case NFileHeader::NLinkFlag::kDumpDir:
        return true;
      case NFileHeader::NLinkFlag::kOldNormal:
      case NFileHeader::NLinkFlag::kNormal:
      case NFileHeader::NLinkFlag::kSymLink:
        if (Name.IsEmpty())
          return false;
        // GNU TAR uses last character as directory marker
        // we also do it
        return Name.Back() == '/';
        // return NItemName::HasTailSlash(Name, CP_OEMCP);
      default: break;
    }
    return false;
  }

  bool IsMagic_ustar_5chars() const
  {
    for (unsigned i = 0; i < 5; i++)
      if (Magic[i] != NFileHeader::NMagic::k_GNU_ustar[i])
        return false;
    return true;
  }

  bool IsMagic_Posix_ustar_00() const
  {
    for (unsigned i = 0; i < 8; i++)
      if (Magic[i] != NFileHeader::NMagic::k_Posix_ustar_00[i])
        return false;
    return true;
  }

  bool IsMagic_GNU() const
  {
    for (unsigned i = 0; i < 8; i++)
      if (Magic[i] != NFileHeader::NMagic::k_GNU_ustar[i])
        return false;
    return true;
  }

  UInt64 Get_PackSize_Aligned() const { return (PackSize + 0x1FF) & (~((UInt64)0x1FF)); }

  bool IsThereWarning() const
  {
    // that Header Warning is possible if (Size != 0) for dir item
    return (PackSize < Size) && (LinkFlag == NFileHeader::NLinkFlag::kDirectory);
  }
};



struct CEncodingCharacts
{
  bool IsAscii;
  // bool Oem_Checked;
  // bool Oem_Ok;
  // bool Utf_Checked;
  CUtf8Check UtfCheck;
  
  void Clear()
  {
    IsAscii = true;
    // Oem_Checked = false;
    // Oem_Ok = false;
    // Utf_Checked = false;
    UtfCheck.Clear();
  }

  void Update(const CEncodingCharacts &ec)
  {
    if (!ec.IsAscii)
      IsAscii = false;

    // if (ec.Utf_Checked)
    {
      UtfCheck.Update(ec.UtfCheck);
      // Utf_Checked = true;
    }
  }

  CEncodingCharacts() { Clear(); }
  void Check(const AString &s);
  AString GetCharactsString() const;
};


struct CPaxExtra
{
  AString RecordPath;
  AString RawLines;

  void Clear()
  {
    RecordPath.Empty();
    RawLines.Empty();
  }

  void Print_To_String(AString &s) const
  {
    if (!RecordPath.IsEmpty())
    {
      s += RecordPath;
      s.Add_LF();
    }
    if (!RawLines.IsEmpty())
      s += RawLines;
  }
};


struct CItemEx: public CItem
{
  bool HeaderError;
  bool Method_Error;
  
  bool IsSignedChecksum;
  bool Prefix_WasUsed;

  bool Pax_Error;
  bool Pax_Overflow;
  bool pax_path_WasUsed;
  bool pax_link_WasUsed;
  bool pax_size_WasUsed;

  bool MTime_IsBin;
  bool PackSize_IsBin;
  bool Size_IsBin;

  bool LongName_WasUsed;
  bool LongName_WasUsed_2;

  bool LongLink_WasUsed;
  bool LongLink_WasUsed_2;

  // bool Name_CouldBeReduced;
  // bool LinkName_CouldBeReduced;

  UInt64 HeaderPos;
  UInt64 HeaderSize;
  
  UInt64 Num_Pax_Records;
  CPaxExtra PaxExtra;
  AString SCHILY_fflags;

  CEncodingCharacts EncodingCharacts;

  UInt64 Get_DataPos() const { return HeaderPos + HeaderSize; }
  // UInt64 GetFullSize() const { return HeaderSize + PackSize; }
  UInt64 Get_FullSize_Aligned() const { return HeaderSize + Get_PackSize_Aligned(); }
};

}}

#endif

/* ---- CPP/7zip/Archive/Tar/TarIn.h ---- */
// TarIn.h

#ifndef ZIP7_INC_ARCHIVE_TAR_IN_H
#define ZIP7_INC_ARCHIVE_TAR_IN_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {
  
enum EErrorType
{
  k_ErrorType_OK,
  k_ErrorType_Corrupted,
  k_ErrorType_UnexpectedEnd
  // , k_ErrorType_Warning
};


struct CTempBuffer
{
  CByteBuffer Buffer;
  size_t StringSize; // num characters before zero Byte (StringSize <= item.PackSize)
  bool IsNonZeroTail;
  bool StringSize_IsConfirmed;

  void CopyToString(AString &s)
  {
    s.Empty();
    if (StringSize != 0)
      s.SetFrom((const char *)(const void *)(const Byte *)Buffer, (unsigned)StringSize);
  }

  void Init()
  {
    StringSize = 0;
    IsNonZeroTail = false;
    StringSize_IsConfirmed = false;
  }
};


class CArchive
{
public:
  bool _phySize_Defined;
  bool _is_Warning;
  bool PaxGlobal_Defined;
  bool _is_PaxGlobal_Error;
  bool _are_Pax_Items;
  bool _are_Gnu;
  bool _are_Posix;
  bool _are_Pax;
  bool _are_mtime;
  bool _are_atime;
  bool _are_ctime;
  bool _are_pax_path;
  bool _are_pax_link;
  bool _are_LongName;
  bool _are_LongLink;
  bool _pathPrefix_WasUsed;
  bool _are_SCHILY_fflags;
  // bool _isSparse;

  // temp internal vars for ReadItem():
  bool filled;
private:
  EErrorType error;

public:
  UInt64 _phySize;
  UInt64 _headersSize;
  EErrorType _error;

  ISequentialInStream *SeqStream;
  IInStream *InStream;
  IArchiveOpenCallback *OpenCallback;
  UInt64 NumFiles;
  UInt64 NumFiles_Prev;
  UInt64 Pos_Prev;
  // UInt64 NumRecords;
  // UInt64 NumRecords_Prev;

  CPaxExtra PaxGlobal;

  void Clear()
  {
    SeqStream = NULL;
    InStream = NULL;
    OpenCallback = NULL;
    NumFiles = 0;
    NumFiles_Prev = 0;
    Pos_Prev = 0;
    // NumRecords = 0;
    // NumRecords_Prev = 0;

    PaxGlobal.Clear();
    PaxGlobal_Defined = false;
    _is_PaxGlobal_Error = false;
    _are_Pax_Items = false; // if there are final paxItems
    _are_Gnu = false;
    _are_Posix = false;
    _are_Pax = false;
    _are_mtime = false;
    _are_atime = false;
    _are_ctime = false;
    _are_pax_path = false;
    _are_pax_link = false;
    _are_LongName = false;
    _are_LongLink = false;
    _pathPrefix_WasUsed = false;
    _are_SCHILY_fflags = false;
    // _isSparse = false;

    _is_Warning = false;
    _error = k_ErrorType_OK;

    _phySize_Defined = false;
    _phySize = 0;
    _headersSize = 0;
  }

private:
  CTempBuffer NameBuf;
  CTempBuffer LinkBuf;
  CTempBuffer PaxBuf;
  CTempBuffer PaxBuf_global;

  CByteBuffer Buffer;

  HRESULT ReadDataToBuffer(const CItemEx &item, CTempBuffer &tb, size_t stringLimit);
  HRESULT Progress(const CItemEx &item, UInt64 posOffset);
  HRESULT GetNextItemReal(CItemEx &item);
  HRESULT ReadItem2(CItemEx &itemInfo);
public:
  CArchive()
  {
    // we will call Clear() in CHandler::Close().
    // Clear(); // it's not required here
  }
  HRESULT ReadItem(CItemEx &itemInfo);
};


API_FUNC_IsArc IsArc_Tar(const Byte *p, size_t size);

}}
  
#endif

/* ---- CPP/7zip/Archive/Tar/TarHandler.h ---- */
// TarHandler.h

#ifndef ZIP7_INC_TAR_HANDLER_H
#define ZIP7_INC_TAR_HANDLER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

Z7_CLASS_IMP_CHandler_IInArchive_4(
    IArchiveOpenSeq
  , IInArchiveGetStream
  , ISetProperties
  , IOutArchive
)
public:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
private:
  bool _isArc;
  bool _posixMode_WasForced;
  bool _posixMode;
  bool _forceCodePage;
  UInt32 _specifiedCodePage;
  UInt32 _curCodePage;
  UInt32 _openCodePage;
  // CTimeOptions TimeOptions;
  CHandlerTimeOptions _handlerTimeOptions;
  CEncodingCharacts _encodingCharacts;

  UInt32 _curIndex;
  bool _latestIsRead;
  CItemEx _latestItem;

  CArchive _arc;

  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);
  HRESULT SkipTo(UInt32 index);
  void TarStringToUnicode(const AString &s, NWindows::NCOM::CPropVariant &prop, bool toOs = false) const;
public:
  void Init();
  CHandler();
};

}}

#endif

/* ---- CPP/7zip/Archive/Tar/TarUpdate.h ---- */
// TarUpdate.h

#ifndef ZIP7_INC_TAR_UPDATE_H
#define ZIP7_INC_TAR_UPDATE_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

struct CUpdateItem
{
  int IndexInArc;
  unsigned IndexInClient;
  UInt64 Size;
  // Int64 MTime;
  UInt32 Mode;
  bool NewData;
  bool NewProps;
  bool IsDir;
  bool DeviceMajor_Defined;
  bool DeviceMinor_Defined;
  UInt32 UID;
  UInt32 GID;
  UInt32 DeviceMajor;
  UInt32 DeviceMinor;
  AString Name;
  AString User;
  AString Group;

  CPaxTimes PaxTimes;

  CUpdateItem():
      Size(0),
      IsDir(false),
      DeviceMajor_Defined(false),
      DeviceMinor_Defined(false),
      UID(0),
      GID(0)
      {}
};


struct CUpdateOptions
{
  UINT CodePage;
  unsigned UtfFlags;
  bool PosixMode;
  CBoolPair Write_MTime;
  CBoolPair Write_ATime;
  CBoolPair Write_CTime;
  CTimeOptions TimeOptions;
};


HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    const CUpdateOptions &options,
    IArchiveUpdateCallback *updateCallback);

HRESULT GetPropString(IArchiveUpdateCallback *callback, UInt32 index, PROPID propId, AString &res,
    UINT codePage, unsigned utfFlags, bool convertSlash);

HRESULT Prop_To_PaxTime(const NWindows::NCOM::CPropVariant &prop, CPaxTime &pt);

void Get_AString_From_UString(const UString &s, AString &res,
    UINT codePage, unsigned utfFlags);

}}

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

/* ---- CPP/Common/StringToInt.h ---- */
// Common/StringToInt.h

#ifndef ZIP7_INC_COMMON_STRING_TO_INT_H
#define ZIP7_INC_COMMON_STRING_TO_INT_H

// amalgamation: header emitted in prologue

UInt32 ConvertStringToUInt32(const char *s, const char **end) throw();
UInt64 ConvertStringToUInt64(const char *s, const char **end) throw();
UInt32 ConvertStringToUInt32(const wchar_t *s, const wchar_t **end) throw();
UInt64 ConvertStringToUInt64(const wchar_t *s, const wchar_t **end) throw();

// Int32 ConvertStringToInt32(const char *s, const char **end) throw();
Int32 ConvertStringToInt32(const wchar_t *s, const wchar_t **end) throw();

UInt32 ConvertOctStringToUInt32(const char *s, const char **end) throw();
UInt64 ConvertOctStringToUInt64(const char *s, const char **end) throw();

UInt32 ConvertHexStringToUInt32(const char *s, const char **end) throw();
UInt64 ConvertHexStringToUInt64(const char *s, const char **end) throw();

#define Z7_PARSE_HEX_DIGIT(c, err_op) \
{ c -= '0'; \
  if (c > 9) { \
    c -= 'A' - '0'; \
    c &= ~0x20u; \
    if (c > 5) { err_op } \
    c += 10; \
  } \
}

const char *FindNonHexChar(const char *s) throw();

// in: (dest != NULL)
// returns: pointer in dest array after last written byte
Byte *ParseHexString(const char *s, Byte *dest) throw();

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

/* ---- CPP/7zip/Archive/Tar/TarOut.h ---- */
// Archive/TarOut.h

#ifndef ZIP7_INC_ARCHIVE_TAR_OUT_H
#define ZIP7_INC_ARCHIVE_TAR_OUT_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

class COutArchive
{
  CMyComPtr<ISequentialOutStream> Stream;

  AString Glob_Name;
  AString Prefix;

  HRESULT WriteHeaderReal(const CItem &item, bool isPax = false
      // , bool zero_PackSize = false
      // , bool zero_MTime = false
      );

  HRESULT Write_Data(const void *data, unsigned size);
  HRESULT Write_Data_And_Residual(const void *data, unsigned size);

public:
  UInt64 Pos;
  bool IsPosixMode;
  // bool IsPrefixAllowed; // it's used only if (IsPosixMode == true)
  CTimeOptions TimeOptions;

  void Create(ISequentialOutStream *outStream)
  {
    Stream = outStream;
  }
  HRESULT WriteHeader(const CItem &item);
  HRESULT Write_AfterDataResidual(UInt64 dataSize);
  HRESULT WriteFinishHeader();

  COutArchive():
      Pos(0),
      IsPosixMode(false)
      // , IsPrefixAllowed(true)
      {}
};

}}

#endif

/* ---- CPP/7zip/Common/RegisterArc.h ---- */
// RegisterArc.h

#ifndef ZIP7_INC_REGISTER_ARC_H
#define ZIP7_INC_REGISTER_ARC_H

// amalgamation: header emitted in prologue

struct CArcInfo
{
  UInt32 Flags;
  Byte Id;
  Byte SignatureSize;
  UInt16 SignatureOffset;
  
  const Byte *Signature;
  const char *Name;
  const char *Ext;
  const char *AddExt;
  
  UInt32 TimeFlags;

  Func_CreateInArchive CreateInArchive;
  Func_CreateOutArchive CreateOutArchive;
  Func_IsArc IsArc;

  bool IsMultiSignature() const { return (Flags & NArcInfoFlags::kMultiSignature) != 0; }
};

void RegisterArc(const CArcInfo *arcInfo) throw();


#define IMP_CreateArcIn_2(c) \
  static IInArchive *CreateArc() { return new c; }

#define IMP_CreateArcIn IMP_CreateArcIn_2(CHandler())

#ifdef Z7_EXTRACT_ONLY
  #define IMP_CreateArcOut
  #define CreateArcOut NULL
#else
  #define IMP_CreateArcOut static IOutArchive *CreateArcOut() { return new CHandler(); }
#endif

#define REGISTER_ARC_V(n, e, ae, id, sigSize, sig, offs, flags, tf, crIn, crOut, isArc) \
  static const CArcInfo g_ArcInfo = { flags, id, sigSize, offs, sig, n, e, ae, tf, crIn, crOut, isArc } ; \

#define REGISTER_ARC_R(n, e, ae, id, sigSize, sig, offs, flags, tf, crIn, crOut, isArc) \
  REGISTER_ARC_V      (n, e, ae, id, sigSize, sig, offs, flags, tf, crIn, crOut, isArc) \
  struct CRegisterArc { CRegisterArc() { RegisterArc(&g_ArcInfo); }}; \
  static CRegisterArc g_RegisterArc;


#define REGISTER_ARC_I_CLS(cls, n, e, ae, id, sig, offs, flags, isArc) \
  IMP_CreateArcIn_2(cls) \
  REGISTER_ARC_R(n, e, ae, id, Z7_ARRAY_SIZE(sig), sig, offs, flags, 0, CreateArc, NULL, isArc)

#define REGISTER_ARC_I_CLS_NO_SIG(cls, n, e, ae, id, offs, flags, isArc) \
  IMP_CreateArcIn_2(cls) \
  REGISTER_ARC_R(n, e, ae, id, 0, NULL, offs, flags, 0, CreateArc, NULL, isArc)

#define REGISTER_ARC_I(n, e, ae, id, sig, offs, flags, isArc) \
  REGISTER_ARC_I_CLS(CHandler(), n, e, ae, id, sig, offs, flags, isArc)

#define REGISTER_ARC_I_NO_SIG(n, e, ae, id, offs, flags, isArc) \
  REGISTER_ARC_I_CLS_NO_SIG(CHandler(), n, e, ae, id, offs, flags, isArc)


#define REGISTER_ARC_IO(n, e, ae, id, sig, offs, flags, tf, isArc) \
  IMP_CreateArcIn \
  IMP_CreateArcOut \
  REGISTER_ARC_R(n, e, ae, id, Z7_ARRAY_SIZE(sig), sig, offs, flags, tf, CreateArc, CreateArcOut, isArc)

#define REGISTER_ARC_IO_DECREMENT_SIG(n, e, ae, id, sig, offs, flags, tf, isArc) \
  IMP_CreateArcIn \
  IMP_CreateArcOut \
  REGISTER_ARC_V(n, e, ae, id, Z7_ARRAY_SIZE(sig), sig, offs, flags, tf, CreateArc, CreateArcOut, isArc) \
  struct CRegisterArcDecSig { CRegisterArcDecSig() { sig[0]--; RegisterArc(&g_ArcInfo); }}; \
  static CRegisterArcDecSig g_RegisterArc;

#endif

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/Tar/TarHandler.cpp ================ */
// TarHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

namespace NArchive {
namespace NTar {

// 21.02: we use UTF8 code page by default, even if some files show error
// before 21.02 : CP_OEMCP;
// static const UINT k_DefaultCodePage = CP_UTF8;


static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidCTime,
  kpidATime,
  kpidPosixAttrib,
#if 0
  kpidAttrib,
#endif
  kpidUser,
  kpidGroup,
  kpidUserId,
  kpidGroupId,
  kpidSymLink,
  kpidHardLink,
  kpidCharacts,
  kpidComment
  , kpidDeviceMajor
  , kpidDeviceMinor
  // , kpidDevice
  // , kpidHeadersSize // for debug
  // , kpidOffset // for debug
};

static const Byte kArcProps[] =
{
  kpidHeadersSize,
  kpidCodePage,
  kpidCharacts,
  kpidComment
};

static const char * const k_Characts_Prefix = "PREFIX";

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize:     if (_arc._phySize_Defined) prop = _arc._phySize; break;
    case kpidHeadersSize: if (_arc._phySize_Defined) prop = _arc._headersSize; break;
    case kpidErrorFlags:
    {
      UInt32 flags = 0;
      if (!_isArc)
        flags |= kpv_ErrorFlags_IsNotArc;
      else switch ((int)_arc._error)
      {
        case k_ErrorType_UnexpectedEnd: flags = kpv_ErrorFlags_UnexpectedEnd; break;
        case k_ErrorType_Corrupted: flags = kpv_ErrorFlags_HeadersError; break;
        // case k_ErrorType_OK: break;
        // case k_ErrorType_Warning: break;
        // case k_ErrorType_OK:
        default: break;
      }
      if (flags != 0)
        prop = flags;
      break;
    }

    case kpidWarningFlags:
    {
      if (_arc._is_Warning)
        prop = kpv_ErrorFlags_HeadersError;
      break;
    }

    case kpidCodePage:
    {
      char sz[16];
      const char *name = NULL;
      switch (_openCodePage)
      {
        case CP_OEMCP: name = "OEM"; break;
        case CP_UTF8: name = "UTF-8"; break;
        default: break;
      }
      if (!name)
      {
        ConvertUInt32ToString(_openCodePage, sz);
        name = sz;
      }
      prop = name;
      break;
    }

    case kpidCharacts:
    {
      AString s;
      if (_arc._are_Gnu) s.Add_OptSpaced("GNU");
      if (_arc._are_Posix) s.Add_OptSpaced("POSIX");
      if (_arc._are_Pax_Items) s.Add_OptSpaced("PAX_ITEM");
      if (_arc._pathPrefix_WasUsed) s.Add_OptSpaced(k_Characts_Prefix);
      if (_arc._are_LongName) s.Add_OptSpaced("LongName");
      if (_arc._are_LongLink) s.Add_OptSpaced("LongLink");
      if (_arc._are_Pax) s.Add_OptSpaced("PAX");
      if (_arc._are_pax_path) s.Add_OptSpaced("path");
      if (_arc._are_pax_link) s.Add_OptSpaced("linkpath");
      if (_arc._are_mtime) s.Add_OptSpaced("mtime");
      if (_arc._are_atime) s.Add_OptSpaced("atime");
      if (_arc._are_ctime) s.Add_OptSpaced("ctime");
      if (_arc._are_SCHILY_fflags) s.Add_OptSpaced("SCHILY.fflags");
      if (_arc._is_PaxGlobal_Error) s.Add_OptSpaced("PAX_GLOBAL_ERROR");
      s.Add_OptSpaced(_encodingCharacts.GetCharactsString());
      prop = s;
      break;
    }

    case kpidComment:
    {
      if (_arc.PaxGlobal_Defined)
      {
        AString s;
        _arc.PaxGlobal.Print_To_String(s);
        if (!s.IsEmpty())
          prop = s;
      }
      break;
    }
    default: break;
  }
  prop.Detach(value);
  return S_OK;
}


void CEncodingCharacts::Check(const AString &s)
{
  IsAscii = s.IsAscii();
  if (!IsAscii)
  {
    /*
    {
      Oem_Checked = true;
      UString u;
      MultiByteToUnicodeString2(u, s, CP_OEMCP);
      Oem_Ok = (u.Find((wchar_t)0xfffd) <= 0);
    }
    Utf_Checked = true;
    */
    UtfCheck.Check_AString(s);
  }
}


AString CEncodingCharacts::GetCharactsString() const
{
  AString s;
  if (IsAscii)
  {
    s += "ASCII";
  }
  /*
  if (Oem_Checked)
  {
    s.Add_Space_if_NotEmpty();
    s += (Oem_Ok ? "oem-ok" : "oem-error");
  }
  if (Utf_Checked)
  */
  else
  {
    s.Add_Space_if_NotEmpty();
    s += (UtfCheck.IsOK() ? "UTF8" : "UTF8-ERROR"); // "UTF8-error"
    {
      AString s2;
      UtfCheck.PrintStatus(s2);
      s.Add_Space_if_NotEmpty();
      s += s2;
    }
  }
  return s;
}


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  UInt64 endPos;
  {
    RINOK(InStream_AtBegin_GetSize(stream, endPos))
  }
  
  _arc._phySize_Defined = true;
  
  // bool utf8_OK = true;

  _arc.SeqStream = stream;
  _arc.InStream = stream;
  _arc.OpenCallback = callback;

  CItemEx item;
  for (;;)
  {
    _arc.NumFiles = _items.Size();
    RINOK(_arc.ReadItem(item))
    if (!_arc.filled)
      break;

    _isArc = true;

    /*
    if (!_forceCodePage)
    {
      if (utf8_OK) utf8_OK = CheckUTF8(item.Name, item.NameCouldBeReduced);
      if (utf8_OK) utf8_OK = CheckUTF8(item.LinkName, item.LinkNameCouldBeReduced);
      if (utf8_OK) utf8_OK = CheckUTF8(item.User);
      if (utf8_OK) utf8_OK = CheckUTF8(item.Group);
    }
    */
 
    item.EncodingCharacts.Check(item.Name);
    _encodingCharacts.Update(item.EncodingCharacts);

    _items.Add(item);

    RINOK(stream->Seek((Int64)item.Get_PackSize_Aligned(), STREAM_SEEK_CUR, &_arc._phySize))
    if (_arc._phySize > endPos)
    {
      _arc._error = k_ErrorType_UnexpectedEnd;
      break;
    }
    /*
    if (_phySize == endPos)
    {
      _errorMessage = "There are no trailing zero-filled records";
      break;
    }
    */
    /*
    if (callback)
    {
      if (_items.Size() == 1)
      {
        RINOK(callback->SetTotal(NULL, &endPos));
      }
      if ((_items.Size() & 0x3FF) == 0)
      {
        const UInt64 numFiles = _items.Size();
        RINOK(callback->SetCompleted(&numFiles, &_phySize));
      }
    }
    */
  }

  /*
  if (!_forceCodePage)
  {
    if (!utf8_OK)
      _curCodePage = k_DefaultCodePage;
  }
  */
  _openCodePage = _curCodePage;

  if (_items.Size() == 0)
  {
    if (_arc._error != k_ErrorType_OK)
    {
      _isArc = false;
      return S_FALSE;
    }
    if (!callback)
      return S_FALSE;
    Z7_DECL_CMyComPtr_QI_FROM(
        IArchiveOpenVolumeCallback,
        openVolumeCallback, callback)
    if (!openVolumeCallback)
      return S_FALSE;
    NCOM::CPropVariant prop;
    if (openVolumeCallback->GetProperty(kpidName, &prop) != S_OK)
      return S_FALSE;
    if (prop.vt != VT_BSTR)
      return S_FALSE;
    unsigned len = MyStringLen(prop.bstrVal);
    if (len < 4 || MyStringCompareNoCase(prop.bstrVal + len - 4, L".tar") != 0)
      return S_FALSE;
  }

  _isArc = true;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *openArchiveCallback))
{
  COM_TRY_BEGIN
  // for (int i = 0; i < 10; i++) // for debug
  {
    Close();
    RINOK(Open2(stream, openArchiveCallback))
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  Close();
  _seqStream = stream;
  _isArc = true;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Close())
{
  _isArc = false;

  _arc.Clear();

  _curIndex = 0;
  _latestIsRead = false;
  _encodingCharacts.Clear();
  _items.Clear();
  _seqStream.Release();
  _stream.Release();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = (_stream ? _items.Size() : (UInt32)(Int32)-1);
  return S_OK;
}

CHandler::CHandler()
{
  // copyCoder = new NCompress::CCopyCoder();
  // copyCoder = copyCoder;
  _openCodePage = CP_UTF8;
  Init();
}

HRESULT CHandler::SkipTo(UInt32 index)
{
  while (_curIndex < index || !_latestIsRead)
  {
    if (_latestIsRead)
    {
      const UInt64 packSize = _latestItem.Get_PackSize_Aligned();
      RINOK(copyCoder.Interface()->Code(_seqStream, NULL, &packSize, &packSize, NULL))
      _arc._phySize += copyCoder->TotalSize;
      if (copyCoder->TotalSize != packSize)
      {
        _arc._error = k_ErrorType_UnexpectedEnd;
        return S_FALSE;
      }
      _latestIsRead = false;
      _curIndex++;
    }
    else
    {
      _arc.SeqStream = _seqStream;
      _arc.InStream = NULL;
      RINOK(_arc.ReadItem(_latestItem))
      if (!_arc.filled)
      {
        _arc._phySize_Defined = true;
        return E_INVALIDARG;
      }
      _latestIsRead = true;
    }
  }
  return S_OK;
}

void CHandler::TarStringToUnicode(const AString &s, NWindows::NCOM::CPropVariant &prop, bool toOs) const
{
  UString dest;
  if (_curCodePage == CP_UTF8)
    ConvertUTF8ToUnicode(s, dest);
  else
    MultiByteToUnicodeString2(dest, s, _curCodePage);
  if (toOs)
    NItemName::ReplaceToOsSlashes_Remove_TailSlash(dest,
        true); // useBackslashReplacement
  prop = dest;
}


// CPaxTime is defined (NumDigits >= 0)
static void PaxTimeToProp(const CPaxTime &pt, NWindows::NCOM::CPropVariant &prop)
{
  UInt64 v;
  if (!NTime::UnixTime64_To_FileTime64(pt.Sec, v))
    return;
  if (pt.Ns != 0)
    v += pt.Ns / 100;
  FILETIME ft;
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
  prop.SetAsTimeFrom_FT_Prec_Ns100(ft,
      k_PropVar_TimePrec_Base + (unsigned)pt.NumDigits, pt.Ns % 100);
}


static void AddSpecCharToString(const char c, AString &s)
{
  if ((Byte)c <= 0x20 || (Byte)c > 127)
  {
    s.Add_Char('[');
    s.Add_Char(GET_HEX_CHAR_LOWER((Byte)c >> 4));
    s.Add_Char(GET_HEX_CHAR_LOWER(c & 15));
    s.Add_Char(']');
  }
  else
    s.Add_Char(c);
}

static void AddSpecUInt64(AString &s, const char *name, UInt64 v)
{
  if (v != 0)
  {
    s.Add_OptSpaced(name);
    if (v > 1)
    {
      s.Add_Colon();
      s.Add_UInt64(v);
    }
  }
}

static void AddSpecBools(AString &s, const char *name, bool b1, bool b2)
{
  if (b1)
  {
    s.Add_OptSpaced(name);
    if (b2)
      s.Add_Char('*');
  }
}


#if 0
static bool Parse_Attrib_from_SCHILY_fflags(const AString &s, UInt32 &attribRes)
{
  UInt32 attrib = 0;
  attribRes = attrib;
  unsigned pos = 0;
  while (pos < s.Len())
  {
    int pos2 = s.Find(',', pos);
    if (pos2 < 0)
      pos2 = (int)s.Len();
    const AString str = s.Mid(pos, (unsigned)pos2 - pos);
          if (str.IsEqualTo("hidden"))  attrib |= FILE_ATTRIBUTE_HIDDEN;
    else  if (str.IsEqualTo("rdonly"))  attrib |= FILE_ATTRIBUTE_READONLY;
    else  if (str.IsEqualTo("system"))  attrib |= FILE_ATTRIBUTE_SYSTEM;
    else
      return false;
    pos = (unsigned)pos2 + 1;
  }
  attribRes = attrib;
  return true;
}
#endif


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  const CItemEx *item;
  if (_stream)
    item = &_items[index];
  else
  {
    if (index < _curIndex)
      return E_INVALIDARG;
    else
    {
      RINOK(SkipTo(index))
      item = &_latestItem;
    }
  }

  switch (propID)
  {
    case kpidPath: TarStringToUnicode(item->Name, prop, true); break;
    case kpidIsDir: prop = item->IsDir(); break;
    case kpidSize: prop = item->Get_UnpackSize(); break;
    case kpidPackSize: prop = item->Get_PackSize_Aligned(); break;
    case kpidMTime:
    {
      /*
      // for debug:
      PropVariant_SetFrom_UnixTime(prop, 1 << 30);
      prop.wReserved1 = k_PropVar_TimePrec_Base + 1;
      prop.wReserved2 = 12;
      break;
      */

      if (item->PaxTimes.MTime.IsDefined())
        PaxTimeToProp(item->PaxTimes.MTime, prop);
      else
      // if (item->MTime != 0)
      {
        // we allow (item->MTime == 0)
        FILETIME ft;
        if (NTime::UnixTime64_To_FileTime(item->MTime, ft))
        {
          unsigned prec = k_PropVar_TimePrec_Unix;
          if (item->MTime_IsBin)
          {
            /* we report here that it's Int64-UnixTime
               instead of basic UInt32-UnixTime range */
            prec = k_PropVar_TimePrec_Base;
          }
          prop.SetAsTimeFrom_FT_Prec(ft, prec);
        }
      }
      break;
    }
    case kpidATime:
      if (item->PaxTimes.ATime.IsDefined())
        PaxTimeToProp(item->PaxTimes.ATime, prop);
      break;
    case kpidCTime:
      if (item->PaxTimes.CTime.IsDefined())
        PaxTimeToProp(item->PaxTimes.CTime, prop);
      break;
    case kpidPosixAttrib: prop = item->Get_Combined_Mode(); break;

    // kpidAttrib has priority over kpidPosixAttrib in 7-Zip.
    // but if we want kpidPosixAttrib priority for TAR, we disable kpidAttrib.
#if 0
    case kpidAttrib:
    {
      if (!item->SCHILY_fflags.IsEmpty())
      {
        UInt32 attrib = 0;
        if (Parse_Attrib_from_SCHILY_fflags(item->SCHILY_fflags, attrib))
        {
          if (attrib != 0)
          {
            if (item->IsDir())
              attrib |= FILE_ATTRIBUTE_DIRECTORY;
            attrib |= ((UInt32)item->Get_Combined_Mode() << 16) | 0x8000; // FILE_ATTRIBUTE_UNIX_EXTENSION;
            prop = attrib;
          }
        }
      }
      break;
    }
#endif
      
    case kpidUser:
      if (!item->User.IsEmpty())
        TarStringToUnicode(item->User, prop);
      break;
    case kpidGroup:
      if (!item->Group.IsEmpty())
        TarStringToUnicode(item->Group, prop);
      break;

    case kpidUserId:
      // if (item->UID != 0)
        prop = (UInt32)item->UID;
      break;
    case kpidGroupId:
      // if (item->GID != 0)
        prop = (UInt32)item->GID;
      break;

    case kpidDeviceMajor:
      if (item->DeviceMajor_Defined)
      // if (item->DeviceMajor != 0)
        prop = (UInt32)item->DeviceMajor;
      break;

    case kpidDeviceMinor:
      if (item->DeviceMinor_Defined)
      // if (item->DeviceMinor != 0)
        prop = (UInt32)item->DeviceMinor;
      break;
    /*
    case kpidDevice:
      if (item->DeviceMajor_Defined)
      if (item->DeviceMinor_Defined)
        prop = (UInt64)MY_dev_makedev(item->DeviceMajor, item->DeviceMinor);
      break;
    */

    case kpidSymLink:
      if (item->Is_SymLink())
        if (!item->LinkName.IsEmpty())
          TarStringToUnicode(item->LinkName, prop);
      break;
    case kpidHardLink:
      if (item->Is_HardLink())
        if (!item->LinkName.IsEmpty())
          TarStringToUnicode(item->LinkName, prop);
      break;

    case kpidCharacts:
    {
      AString s;
      {
        s.Add_Space_if_NotEmpty();
        AddSpecCharToString(item->LinkFlag, s);
      }
      if (item->IsMagic_GNU())
        s.Add_OptSpaced("GNU");
      else if (item->IsMagic_Posix_ustar_00())
        s.Add_OptSpaced("POSIX");
      else
      {
        s.Add_Space_if_NotEmpty();
        for (unsigned i = 0; i < sizeof(item->Magic); i++)
          AddSpecCharToString(item->Magic[i], s);
      }

      if (item->IsSignedChecksum)
        s.Add_OptSpaced("SignedChecksum");

      if (item->Prefix_WasUsed)
        s.Add_OptSpaced(k_Characts_Prefix);

      s.Add_OptSpaced(item->EncodingCharacts.GetCharactsString());

      // AddSpecUInt64(s, "LongName", item->Num_LongName_Records);
      // AddSpecUInt64(s, "LongLink", item->Num_LongLink_Records);
      AddSpecBools(s, "LongName", item->LongName_WasUsed, item->LongName_WasUsed_2);
      AddSpecBools(s, "LongLink", item->LongLink_WasUsed, item->LongLink_WasUsed_2);

      if (item->MTime_IsBin)
        s.Add_OptSpaced("bin_mtime");
      if (item->PackSize_IsBin)
        s.Add_OptSpaced("bin_psize");
      if (item->Size_IsBin)
        s.Add_OptSpaced("bin_size");
      
      AddSpecUInt64(s, "PAX", item->Num_Pax_Records);

      if (item->PaxTimes.MTime.IsDefined()) s.Add_OptSpaced("mtime");
      if (item->PaxTimes.ATime.IsDefined()) s.Add_OptSpaced("atime");
      if (item->PaxTimes.CTime.IsDefined()) s.Add_OptSpaced("ctime");

      if (item->pax_path_WasUsed)
        s.Add_OptSpaced("pax_path");
      if (item->pax_link_WasUsed)
        s.Add_OptSpaced("pax_linkpath");
      if (item->pax_size_WasUsed)
        s.Add_OptSpaced("pax_size");
      if (!item->SCHILY_fflags.IsEmpty())
      {
        s.Add_OptSpaced("SCHILY.fflags=");
        s += item->SCHILY_fflags;
      }
      if (item->Is_Sparse())
        s.Add_OptSpaced("SPARSE");
      if (item->IsThereWarning())
        s.Add_OptSpaced("WARNING");
      if (item->HeaderError)
        s.Add_OptSpaced("ERROR");
      if (item->Method_Error)
        s.Add_OptSpaced("METHOD_ERROR");
      if (item->Pax_Error)
        s.Add_OptSpaced("PAX_error");
      if (!item->PaxExtra.RawLines.IsEmpty())
        s.Add_OptSpaced("PAX_unsupported_line");
      if (item->Pax_Overflow)
        s.Add_OptSpaced("PAX_overflow");
      if (!s.IsEmpty())
        prop = s;
      break;
    }
    case kpidComment:
    {
      AString s;
      item->PaxExtra.Print_To_String(s);
      if (!s.IsEmpty())
        prop = s;
      break;
    }
    // case kpidHeadersSize: prop = item->HeaderSize; break; // for debug
    // case kpidOffset: prop = item->HeaderPos; break; // for debug
    default: break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  ISequentialInStream *stream = _seqStream;
  const bool seqMode = (_stream == NULL);
  if (!seqMode)
    stream = _stream;

  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (_stream && numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _items[allFilesMode ? i : indices[i]].Get_UnpackSize();
  RINOK(extractCallback->SetTotal(totalSize))

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStream;
  inStream->SetStream(stream);
  CMyComPtr2_Create<ISequentialOutStream, CLimitedSequentialOutStream> outStreamSpec;

  for (i = 0; ; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems && !seqMode)
      break;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CItemEx *item;
    if (seqMode)
    {
      const HRESULT res = SkipTo(index);
      if (res == E_INVALIDARG)
        break;
      RINOK(res)
      item = &_latestItem;
    }
    else
      item = &_items[index];

    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    CMyComPtr<ISequentialOutStream> realOutStream;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
    const UInt64 unpackSize = item->Get_UnpackSize();
    totalSize += unpackSize;
    totalPackSize += item->Get_PackSize_Aligned();
    if (item->IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      // realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }
    bool skipMode = false;
    if (!testMode && !realOutStream)
    {
      if (!seqMode)
      {
        /*
        // GetStream() creates link.
        // so we can show extracting info in GetStream() instead
        if (item->Is_HardLink() ||
            item->Is_SymLink())
        {
          RINOK(extractCallback->PrepareOperation(askMode))
          RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
        }
        */
        continue;
      }
      skipMode = true;
      askMode = NExtract::NAskMode::kSkip;
    }
    RINOK(extractCallback->PrepareOperation(askMode))

    outStreamSpec->SetStream(realOutStream);
    realOutStream.Release();
    outStreamSpec->Init(skipMode ? 0 : unpackSize, true);

    Int32 opRes = NExtract::NOperationResult::kOK;
    CMyComPtr<ISequentialInStream> inStream2;
    if (!item->Is_Sparse())
      inStream2 = inStream;
    else
    {
      const HRESULT hres = GetStream(index, &inStream2);
      if (hres == E_NOTIMPL)
        opRes = NExtract::NOperationResult::kHeadersError; // kUnsupportedMethod
      else if (!inStream2)
      {
        opRes = NExtract::NOperationResult::kDataError;
        // return E_FAIL;
      }
    }
    if (opRes == NExtract::NOperationResult::kOK)
    {
      if (item->Is_SymLink())
      {
        RINOK(WriteStream(outStreamSpec, (const char *)item->LinkName, item->LinkName.Len()))
      }
      else
      {
        if (!seqMode)
        {
          RINOK(InStream_SeekSet(_stream, item->Get_DataPos()))
        }
        inStream->Init(item->Get_PackSize_Aligned());
        RINOK(copyCoder.Interface()->Code(inStream2, outStreamSpec, NULL, NULL, lps))
      }
      if (outStreamSpec->GetRem() != 0)
        opRes = NExtract::NOperationResult::kDataError;
    }
    if (seqMode)
    {
      _latestIsRead = false;
      _curIndex++;
    }
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  return S_OK;
  COM_TRY_END
}


Z7_CLASS_IMP_IInStream(
  CSparseStream
)
  UInt64 _phyPos;
  UInt64 _virtPos;
  bool _needStartSeek;

public:
  unsigned ItemIndex;
  CHandler *Handler;
  CMyComPtr<IUnknown> HandlerRef;
  CRecordVector<UInt64> PhyOffsets;

  void Init()
  {
    _virtPos = 0;
    _phyPos = 0;
    _needStartSeek = true;
  }
};


Z7_COM7F_IMF(CSparseStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  const CItemEx &item = Handler->_items[ItemIndex];
  if (_virtPos >= item.Size)
    return S_OK;
  {
    const UInt64 rem = item.Size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }
  
  HRESULT res = S_OK;

  if (item.SparseBlocks.IsEmpty())
    memset(data, 0, size);
  else
  {
    unsigned left = 0, right = item.SparseBlocks.Size();
    for (;;)
    {
      const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      if (mid == left)
        break;
      if (_virtPos < item.SparseBlocks[mid].Offset)
        right = mid;
      else
        left = mid;
    }
    
    const CSparseBlock &sb = item.SparseBlocks[left];
    const UInt64 relat = _virtPos - sb.Offset;
    
    if (_virtPos >= sb.Offset && relat < sb.Size)
    {
      const UInt64 rem = sb.Size - relat;
      if (size > rem)
        size = (UInt32)rem;
      const UInt64 phyPos = PhyOffsets[left] + relat;
      if (_needStartSeek || _phyPos != phyPos)
      {
        RINOK(InStream_SeekSet(Handler->_stream, item.Get_DataPos() + phyPos))
        _needStartSeek = false;
        _phyPos = phyPos;
      }
      res = Handler->_stream->Read(data, size, &size);
      _phyPos += size;
    }
    else
    {
      UInt64 next = item.Size;
      if (_virtPos < sb.Offset)
        next = sb.Offset;
      else if (left + 1 < item.SparseBlocks.Size())
        next = item.SparseBlocks[left + 1].Offset;
      const UInt64 rem = next - _virtPos;
      if (size > rem)
        size = (UInt32)rem;
      memset(data, 0, size);
    }
  }
  
  _virtPos += size;
  if (processedSize)
    *processedSize = size;
  return res;
}

Z7_COM7F_IMF(CSparseStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += Handler->_items[ItemIndex].Size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  
  const CItemEx &item = _items[index];

  if (item.Is_Sparse())
  {
    if (item.Method_Error)
      return E_NOTIMPL; // S_FALSE
    CSparseStream *streamSpec = new CSparseStream;
    CMyComPtr<IInStream> streamTemp = streamSpec;
    streamSpec->Init();
    streamSpec->Handler = this;
    streamSpec->HandlerRef = (IInArchive *)this;
    streamSpec->ItemIndex = index;
    streamSpec->PhyOffsets.Reserve(item.SparseBlocks.Size());
    UInt64 offs = 0;
    FOR_VECTOR(i, item.SparseBlocks)
    {
      const CSparseBlock &sb = item.SparseBlocks[i];
      streamSpec->PhyOffsets.AddInReserved(offs);
      offs += sb.Size;
    }
    *stream = streamTemp.Detach();
    return S_OK;
  }
  
  if (item.Is_SymLink())
  {
    Create_BufInStream_WithReference((const Byte *)(const char *)item.LinkName, item.LinkName.Len(), (IInArchive *)this, stream);
    return S_OK;
  }
  
  return CreateLimitedInStream(_stream, item.Get_DataPos(), item.PackSize, stream);
  
  COM_TRY_END
}


void CHandler::Init()
{
  _forceCodePage = false;
  _curCodePage = _specifiedCodePage = CP_UTF8;  // CP_OEMCP;
  _posixMode = false;
  _posixMode_WasForced = false;
  // TimeOptions.Clear();
  _handlerTimeOptions.Init();
  // _handlerTimeOptions.Write_MTime.Val = true; // it's default already
}


Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  Init();

  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeLower_Ascii();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &prop = values[i];

    if (name[0] == L'x')
    {
      // some clients write 'x' property. So we support it
      UInt32 level = 0;
      RINOK(ParsePropToUInt32(name.Ptr(1), prop, level))
    }
    else if (name.IsEqualTo("cp"))
    {
      UInt32 cp = CP_OEMCP;
      RINOK(ParsePropToUInt32(L"", prop, cp))
      _forceCodePage = true;
      _curCodePage = _specifiedCodePage = cp;
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("mt"))
    {
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("memuse"))
    {
    }
    else if (name.IsEqualTo("m"))
    {
      if (prop.vt != VT_BSTR)
        return E_INVALIDARG;
      const UString s = prop.bstrVal;
      if (s.IsEqualTo_Ascii_NoCase("pax") ||
          s.IsEqualTo_Ascii_NoCase("posix"))
        _posixMode = true;
      else if (s.IsEqualTo_Ascii_NoCase("gnu"))
        _posixMode = false;
      else
        return E_INVALIDARG;
      _posixMode_WasForced = true;
    }
    else
    {
      /*
      if (name.IsPrefixedBy_Ascii_NoCase("td"))
      {
        name.Delete(0, 3);
        if (prop.vt == VT_EMPTY)
        {
          if (name.IsEqualTo_Ascii_NoCase("n"))
          {
            // TimeOptions.UseNativeDigits = true;
          }
          else if (name.IsEqualTo_Ascii_NoCase("r"))
          {
            // TimeOptions.RemoveZeroDigits = true;
          }
          else
            return E_INVALIDARG;
        }
        else
        {
          UInt32 numTimeDigits = 0;
          RINOK(ParsePropToUInt32(name, prop, numTimeDigits));
          TimeOptions.NumDigits_WasForced = true;
          TimeOptions.NumDigits = numTimeDigits;
        }
      }
      */
      bool processed = false;
      RINOK(_handlerTimeOptions.Parse(name, prop, processed))
      if (processed)
        continue;
      return E_INVALIDARG;
    }
  }
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Archive/Tar/TarHandlerOut.cpp ================ */
// TarHandlerOut.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

using namespace NWindows;

namespace NArchive {
namespace NTar {

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *type))
{
  UInt32 t = NFileTimeType::kUnix;
  const UInt32 prec = _handlerTimeOptions.Prec;
  if (prec != (UInt32)(Int32)-1)
  {
    t = NFileTimeType::kWindows;
    if (prec == k_PropVar_TimePrec_0 ||
        prec == k_PropVar_TimePrec_100ns)
      t = NFileTimeType::kWindows;
    else if (prec == k_PropVar_TimePrec_HighPrec)
      t = k_PropVar_TimePrec_1ns;
    else if (prec >= k_PropVar_TimePrec_Base)
      t = prec;
  }
  // 7-Zip before 22.00 fails, if unknown typeType.
  *type = t;
  return S_OK;
}


void Get_AString_From_UString(const UString &s, AString &res,
    UINT codePage, unsigned utfFlags)
{
  if (codePage == CP_UTF8)
    ConvertUnicodeToUTF8_Flags(s, res, utfFlags);
  else
    UnicodeStringToMultiByte2(res, s, codePage);
}


HRESULT GetPropString(IArchiveUpdateCallback *callback, UInt32 index, PROPID propId, AString &res,
    UINT codePage, unsigned utfFlags, bool convertSlash)
{
  NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(index, propId, &prop))
  
  if (prop.vt == VT_BSTR)
  {
    UString s = prop.bstrVal;
    if (convertSlash)
      NItemName::ReplaceSlashes_OsToUnix(s);
    Get_AString_From_UString(s, res, codePage, utfFlags);
  }
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;

  return S_OK;
}


// sort old files with original order.

static int CompareUpdateItems(void *const *p1, void *const *p2, void *)
{
  const CUpdateItem &u1 = *(*((const CUpdateItem *const *)p1));
  const CUpdateItem &u2 = *(*((const CUpdateItem *const *)p2));
  if (!u1.NewProps)
  {
    if (u2.NewProps)
      return -1;
    return MyCompare(u1.IndexInArc, u2.IndexInArc);
  }
  if (!u2.NewProps)
    return 1;
  return MyCompare(u1.IndexInClient, u2.IndexInClient);
}


static HRESULT GetTime(UInt32 i, UInt32 pid, IArchiveUpdateCallback *callback,
    CPaxTime &pt)
{
  pt.Clear();
  NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(i, pid, &prop))
  return Prop_To_PaxTime(prop, pt);
}


/*
static HRESULT GetDevice(IArchiveUpdateCallback *callback, UInt32 i,
    UInt32 &majo, UInt32 &mino, bool &majo_defined, bool &mino_defined)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(i, kpidDevice, &prop));
  if (prop.vt == VT_EMPTY)
    return S_OK;
  if (prop.vt != VT_UI8)
    return E_INVALIDARG;
  {
    const UInt64 v = prop.uhVal.QuadPart;
    majo = MY_dev_major(v);
    mino = MY_dev_minor(v);
    majo_defined = true;
    mino_defined = true;
  }
  return S_OK;
}
*/

static HRESULT GetDevice(IArchiveUpdateCallback *callback, UInt32 i,
    UInt32 pid, UInt32 &id, bool &defined)
{
  defined = false;
  NWindows::NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(i, pid, &prop))
  if (prop.vt == VT_EMPTY)
    return S_OK;
  if (prop.vt == VT_UI4)
  {
    id = prop.ulVal;
    defined = true;
    return S_OK;
  }
  return E_INVALIDARG;
}


static HRESULT GetUser(IArchiveUpdateCallback *callback, UInt32 i,
    UInt32 pidName, UInt32 pidId, AString &name, UInt32 &id,
    UINT codePage, unsigned utfFlags)
{
  // printf("\ncallback->GetProperty(i, pidId, &prop))\n");
   
  bool isSet = false;
  {
    NWindows::NCOM::CPropVariant prop;
    RINOK(callback->GetProperty(i, pidId, &prop))
    if (prop.vt == VT_UI4)
    {
      isSet = true;
      id = prop.ulVal;
      // printf("\ncallback->GetProperty(i, pidId, &prop)); = %d \n", (unsigned)id);
      name.Empty();
    }
    else if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
  }
  {
    NWindows::NCOM::CPropVariant prop;
    RINOK(callback->GetProperty(i, pidName, &prop))
    if (prop.vt == VT_BSTR)
    {
      const UString s = prop.bstrVal;
      Get_AString_From_UString(s, name, codePage, utfFlags);
      if (!isSet)
        id = 0;
    }
    else if (prop.vt == VT_UI4)
    {
      id = prop.ulVal;
      name.Empty();
    }
    else if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
  }
  return S_OK;
}



Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *callback))
{
  COM_TRY_BEGIN

  if ((_stream && (_arc._error != k_ErrorType_OK || _arc._is_Warning
      /* || _isSparse */
      )) || _seqStream)
    return E_NOTIMPL;
  CObjectVector<CUpdateItem> updateItems;
  const UINT codePage = (_forceCodePage ? _specifiedCodePage : _openCodePage);
  const unsigned utfFlags = g_Unicode_To_UTF8_Flags;
  /*
  // for debug only:
  unsigned utfFlags = 0;
  utfFlags |= Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE;
  utfFlags |= Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR;
  */

  for (UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItem ui;
    Int32 newData;
    Int32 newProps;
    UInt32 indexInArc;
    
    if (!callback)
      return E_FAIL;
    
    RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArc))
    
    ui.NewProps = IntToBool(newProps);
    ui.NewData = IntToBool(newData);
    ui.IndexInArc = (int)indexInArc;
    ui.IndexInClient = i;

    if (IntToBool(newProps))
    {
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidIsDir, &prop))
        if (prop.vt == VT_EMPTY)
          ui.IsDir = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
          ui.IsDir = (prop.boolVal != VARIANT_FALSE);
      }

      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidPosixAttrib, &prop))
        if (prop.vt == VT_EMPTY)
          ui.Mode =
                MY_LIN_S_IRWXO
              | MY_LIN_S_IRWXG
              | MY_LIN_S_IRWXU
              | (ui.IsDir ? MY_LIN_S_IFDIR : MY_LIN_S_IFREG);
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
          ui.Mode = prop.ulVal;
        // 21.07 : we clear high file type bits as GNU TAR.
        // we will clear it later
        // ui.Mode &= ~(UInt32)MY_LIN_S_IFMT;
      }

      if (_handlerTimeOptions.Write_MTime.Val)
        RINOK(GetTime(i, kpidMTime, callback, ui.PaxTimes.MTime))
      if (_handlerTimeOptions.Write_ATime.Val)
        RINOK(GetTime(i, kpidATime, callback, ui.PaxTimes.ATime))
      if (_handlerTimeOptions.Write_CTime.Val)
        RINOK(GetTime(i, kpidCTime, callback, ui.PaxTimes.CTime))

      RINOK(GetPropString(callback, i, kpidPath, ui.Name, codePage, utfFlags, true))
      if (ui.IsDir && !ui.Name.IsEmpty() && ui.Name.Back() != '/')
        ui.Name.Add_Slash();
      // ui.Name.Add_Slash(); // for debug

      if (_posixMode)
      {
        RINOK(GetDevice(callback, i, kpidDeviceMajor, ui.DeviceMajor, ui.DeviceMajor_Defined))
        RINOK(GetDevice(callback, i, kpidDeviceMinor, ui.DeviceMinor, ui.DeviceMinor_Defined))
      }

      RINOK(GetUser(callback, i, kpidUser,  kpidUserId,  ui.User,  ui.UID, codePage, utfFlags))
      RINOK(GetUser(callback, i, kpidGroup, kpidGroupId, ui.Group, ui.GID, codePage, utfFlags))
    }

    if (IntToBool(newData))
    {
      NCOM::CPropVariant prop;
      RINOK(callback->GetProperty(i, kpidSize, &prop))
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      ui.Size = prop.uhVal.QuadPart;
      /*
      // now we support GNU extension for big files
      if (ui.Size >= ((UInt64)1 << 33))
        return E_INVALIDARG;
      */
    }
    
    updateItems.Add(ui);
  }
  
  if (_stream && _arc._are_Pax_Items)
  {
    // we restore original order of files, if there are pax items
    updateItems.Sort(CompareUpdateItems, NULL);
  }

  CUpdateOptions options;

  options.CodePage = codePage;
  options.UtfFlags = utfFlags;
  options.PosixMode = _posixMode;
  
  options.Write_MTime = _handlerTimeOptions.Write_MTime;
  options.Write_ATime = _handlerTimeOptions.Write_ATime;
  options.Write_CTime = _handlerTimeOptions.Write_CTime;
  
  // options.TimeOptions = TimeOptions;
  
  const UInt32 prec = _handlerTimeOptions.Prec;
  if (prec != (UInt32)(Int32)-1)
  {
    unsigned numDigits = 0;
    if (prec == 0)
      numDigits = 7;
    else if (prec == k_PropVar_TimePrec_HighPrec
          || prec >= k_PropVar_TimePrec_1ns)
      numDigits = 9;
    else if (prec >= k_PropVar_TimePrec_Base)
      numDigits = prec - k_PropVar_TimePrec_Base;
    options.TimeOptions.NumDigitsMax = numDigits;
    // options.TimeOptions.RemoveZeroMode =
        // k_PaxTimeMode_DontRemoveZero; // pure for debug
        // k_PaxTimeMode_RemoveZero_if_PureSecondOnly; // optimized code
        // k_PaxTimeMode_RemoveZero_Always; // original pax code
  }

  return UpdateArchive(_stream, outStream, _items, updateItems,
      options, callback);
  
  COM_TRY_END
}

}}

/* ================ unit: CPP/7zip/Archive/Tar/TarHeader.cpp ================ */
// Archive/TarHeader.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {
namespace NFileHeader {

  const char * const kLongLink = "././@LongLink";
  const char * const kLongLink2 = "@LongLink";

  // The magic field is filled with this if uname and gname are valid.
  namespace NMagic
  {
    // const char * const kUsTar  = "ustar";   // 5 chars
    // const char * const kGNUTar = "GNUtar "; // 7 chars and a null
    // const char * const kEmpty = "\0\0\0\0\0\0\0\0";
    // 7-Zip used kUsTar_00 before 21.07:
    const char k_Posix_ustar_00[8]  = { 'u', 's', 't', 'a', 'r', 0, '0', '0' } ;
    // GNU TAR uses such header:
    const char k_GNU_ustar[8] = { 'u', 's', 't', 'a', 'r', ' ', ' ', 0 } ;
  }

/*
pre-POSIX.1-1988 (i.e. v7) tar header:
-----
Link indicator:
'0' or 0 : Normal file
'1' : Hard link
'2' : Symbolic link
Some pre-POSIX.1-1988 tar implementations indicated a directory by having
a trailing slash (/) in the name.

Numeric values : octal with leading zeroes.
For historical reasons, a final NUL or space character should also be used.
Thus only 11 octal digits can be stored from 12 bytes field.

2001 star : introduced a base-256 coding that is indicated by
setting the high-order bit of the leftmost byte of a numeric field.
GNU-tar and BSD-tar followed this idea.
 
versions of tar from before the first POSIX standard from 1988
pad the values with spaces instead of zeroes.

UStar
-----
UStar (Unix Standard TAR) : POSIX IEEE P1003.1 : 1988.
    257 signature: "ustar", 0, "00"
    265 32  Owner user name
    297 32  Owner group name
    329 8 Device major number
    337 8 Device minor number
    345 155 Filename prefix

POSIX.1-2001/pax
----
format is known as extended tar format or pax format
vendor-tagged vendor-specific enhancements.
tags Defined by the POSIX standard:
  atime, mtime, path, linkpath, uname, gname, size, uid, gid, ...


PAX EXTENSION
-----------
Hard links
A further difference from the ustar header block is that data blocks
for files of typeflag 1 (hard link) may be included,
which means that the size field may be greater than zero.
Archives created by pax -o linkdata shall include these data
blocks with the hard links.
*

compatiblity
------------
  7-Zip 16.03 supports "PaxHeader/"
  7-Zip 20.01 supports "PaxHeaders.X/" with optional "./"
  7-Zip 21.02 supports "@PaxHeader" with optional "./" "./"

  GNU tar --format=posix uses "PaxHeaders/" in folder of file
  

GNU TAR format
==============
v7      - Unix V7
oldgnu  - GNU tar <=1.12  : writes zero in last character in name
gnu     - GNU tar 1.13    : doesn't write  zero in last character in name
                            as 7-zip 21.07
ustar       - POSIX.1-1988
posix (pax) - POSIX.1-2001

  gnu tar:
  if (S_ISCHR (st->stat.st_mode) || S_ISBLK (st->stat.st_mode)) {
      major_t devmajor = major (st->stat.st_rdev);
      minor_t devminor = minor (st->stat.st_rdev); }
*/

}}}

/* ================ unit: CPP/7zip/Archive/Tar/TarIn.cpp ================ */
// TarIn.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define NUM_UNROLL_BYTES (8 * 4)

Z7_NO_INLINE static bool IsBufNonZero(const void *data, size_t size);
Z7_NO_INLINE static bool IsBufNonZero(const void *data, size_t size)
{
  const Byte *p = (const Byte *)data;

  for (; size != 0 && ((unsigned)(ptrdiff_t)p & (NUM_UNROLL_BYTES - 1)) != 0; size--)
    if (*p++ != 0)
      return true;

  if (size >= NUM_UNROLL_BYTES)
  {
    const Byte *lim = p + size;
    size &= (NUM_UNROLL_BYTES - 1);
    lim -= size;
    do
    {
      if (*(const UInt64 *)(const void *)(p        ) != 0) return true;
      if (*(const UInt64 *)(const void *)(p + 8 * 1) != 0) return true;
      if (*(const UInt64 *)(const void *)(p + 8 * 2) != 0) return true;
      if (*(const UInt64 *)(const void *)(p + 8 * 3) != 0) return true;
      // if (*(const UInt32 *)(const void *)(p        ) != 0) return true;
      // if (*(const UInt32 *)(const void *)(p + 4 * 1) != 0) return true;
      // if (*(const UInt32 *)(const void *)(p + 4 * 2) != 0) return true;
      // if (*(const UInt32 *)(const void *)(p + 4 * 3) != 0) return true;
      p += NUM_UNROLL_BYTES;
    }
    while (p != lim);
  }
  
  for (; size != 0; size--)
    if (*p++ != 0)
      return true;

  return false;
}


namespace NArchive {
namespace NTar {
 
static void MyStrNCpy(char *dest, const char *src, unsigned size)
{
  for (unsigned i = 0; i < size; i++)
  {
    char c = src[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
}

static bool OctalToNumber(const char *srcString, unsigned size, UInt64 &res, bool allowEmpty = false)
{
  res = 0;
  char sz[32];
  MyStrNCpy(sz, srcString, size);
  sz[size] = 0;
  const char *end;
  unsigned i;
  for (i = 0; sz[i] == ' '; i++);
  if (sz[i] == 0)
    return allowEmpty;
  res = ConvertOctStringToUInt64(sz + i, &end);
  return (*end == ' ' || *end == 0);
}

static bool OctalToNumber32(const char *srcString, UInt32 &res, bool allowEmpty = false)
{
  const unsigned kSize = 8;
  UInt64 res64;
  if (!OctalToNumber(srcString, kSize, res64, allowEmpty))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

#define RIF(x) { if (!(x)) return S_OK; }

static void ReadString(const char *s, unsigned size, AString &result)
{
  result.SetFrom_CalcLen(s, size);
}

static bool ParseInt64(const char *p, Int64 &val, bool &isBin)
{
  const UInt32 h = GetBe32(p);
  val = (Int64)GetBe64(p + 4);
  isBin = true;
  if (h == (UInt32)1 << 31)
    return ((val >> 63) & 1) == 0;
  if (h == (UInt32)(Int32)-1)
    return ((val >> 63) & 1) != 0;
  isBin = false;
  UInt64 u;
  const bool res = OctalToNumber(p, 12, u);
  val = (Int64)u;
  return res;
}

static bool ParseInt64_MTime(const char *p, Int64 &val, bool &isBin)
{
  // rare case tar : ZEROs in Docker-Windows TARs
  // rare case tar : pax record : single ZERO byte followed by junk data
  // rare case tar : spaces
  isBin = false;
  // if (GetUi32(p))
  if (p[0])
  for (unsigned i = 0; i < 12; i++)
    if (p[i] != ' ')
      return ParseInt64(p, val, isBin);
  val = 0;
  return true;
}

static bool ParseSize(const char *p, UInt64 &val, bool &isBin)
{
  if (GetBe32(p) == (UInt32)1 << 31)
  {
    // GNU extension
    isBin = true;
    val = GetBe64(p + 4);
    return ((val >> 63) & 1) == 0;
  }
  isBin = false;
  return OctalToNumber(p, 12, val,
      true // 20.03: allow empty size for 'V' Label entry
      );
}

static bool ParseSize(const char *p, UInt64 &val)
{
  bool isBin;
  return ParseSize(p, val, isBin);
}

#define CHECK(x) { if (!(x)) return k_IsArc_Res_NO; }

API_FUNC_IsArc IsArc_Tar(const Byte *p2, size_t size)
{
  if (size < NFileHeader::kRecordSize)
    return k_IsArc_Res_NEED_MORE;

  const char *p = (const char *)p2;
  p += NFileHeader::kNameSize;

  UInt32 mode;
  // we allow empty Mode value for LongName prefix items
  CHECK(OctalToNumber32(p, mode, true)) p += 8;

  // if (!OctalToNumber32(p, item.UID)) item.UID = 0;
  p += 8;
  // if (!OctalToNumber32(p, item.GID)) item.GID = 0;
  p += 8;

  UInt64 packSize;
  Int64 time;
  UInt32 checkSum;
  bool isBin;
  CHECK(ParseSize(p, packSize, isBin)) p += 12;
  CHECK(ParseInt64_MTime(p, time, isBin)) p += 12;
  CHECK(OctalToNumber32(p, checkSum))
  return k_IsArc_Res_YES;
}


HRESULT CArchive::GetNextItemReal(CItemEx &item)
{
  char buf[NFileHeader::kRecordSize];

  item.Method_Error = false;
  error = k_ErrorType_OK;
  filled = false;

  bool thereAreEmptyRecords = false;
  for (;;)
  {
    size_t processedSize = NFileHeader::kRecordSize;
    RINOK(ReadStream(SeqStream, buf, &processedSize))
    if (processedSize == 0)
    {
      if (!thereAreEmptyRecords)
        error = k_ErrorType_UnexpectedEnd; // "There are no trailing zero-filled records";
      return S_OK;
    }
    if (processedSize != NFileHeader::kRecordSize)
    {
      if (!thereAreEmptyRecords)
        error = k_ErrorType_UnexpectedEnd; // error = "There is no correct record at the end of archive";
      else
      {
        /*
        if (IsEmptyData(buf, processedSize))
          error = k_ErrorType_UnexpectedEnd;
        else
        {
          // extraReadSize = processedSize;
          // error = k_ErrorType_Corrupted; // some data after the end tail zeros
        }
        */
      }

      return S_OK;
    }
    if (IsBufNonZero(buf, NFileHeader::kRecordSize))
      break;
    item.HeaderSize += NFileHeader::kRecordSize;
    thereAreEmptyRecords = true;
    RINOK(Progress(item, 0))
  }
  if (thereAreEmptyRecords)
  {
    // error = "There are data after end of archive";
    return S_OK;
  }
  
  char *p = buf;

  error = k_ErrorType_Corrupted;

  // ReadString(p, NFileHeader::kNameSize, item.Name);
  p += NFileHeader::kNameSize;

  /*
  item.Name_CouldBeReduced =
      (item.Name.Len() == NFileHeader::kNameSize ||
       item.Name.Len() == NFileHeader::kNameSize - 1);
  */

  // we allow empty Mode value for LongName prefix items
  RIF(OctalToNumber32(p, item.Mode, true)) p += 8;

  if (!OctalToNumber32(p, item.UID)) { item.UID = 0; }  p += 8;
  if (!OctalToNumber32(p, item.GID)) { item.GID = 0; }  p += 8;

  RIF(ParseSize(p, item.PackSize, item.PackSize_IsBin))
  item.Size = item.PackSize;
  item.Size_IsBin = item.PackSize_IsBin;
  p += 12;
  RIF(ParseInt64_MTime(p, item.MTime, item.MTime_IsBin)) p += 12;
  
  UInt32 checkSum;
  RIF(OctalToNumber32(p, checkSum))
  memset(p, ' ', 8); p += 8;

  item.LinkFlag = *p++;

  ReadString(p, NFileHeader::kNameSize, item.LinkName); p += NFileHeader::kNameSize;
  
  /*
  item.LinkName_CouldBeReduced =
      (item.LinkName.Len() == NFileHeader::kNameSize ||
       item.LinkName.Len() == NFileHeader::kNameSize - 1);
  */

  memcpy(item.Magic, p, 8); p += 8;

  ReadString(p, NFileHeader::kUserNameSize, item.User); p += NFileHeader::kUserNameSize;
  ReadString(p, NFileHeader::kGroupNameSize, item.Group); p += NFileHeader::kGroupNameSize;

  item.DeviceMajor_Defined = (p[0] != 0); if (item.DeviceMajor_Defined) { RIF(OctalToNumber32(p, item.DeviceMajor)) } p += 8;
  item.DeviceMinor_Defined = (p[0] != 0); if (item.DeviceMinor_Defined) { RIF(OctalToNumber32(p, item.DeviceMinor)) } p += 8;

  if (p[0] != 0
      && item.IsMagic_ustar_5chars()
      && (item.LinkFlag != 'L' ))
  {
    item.Prefix_WasUsed = true;
    ReadString(p, NFileHeader::kPrefixSize, item.Name);
    item.Name.Add_Slash();
    unsigned i;
    for (i = 0; i < NFileHeader::kNameSize; i++)
      if (buf[i] == 0)
        break;
    item.Name.AddFrom(buf, i);
  }
  else
    ReadString(buf, NFileHeader::kNameSize, item.Name);
  
  p += NFileHeader::kPrefixSize;

  if (item.LinkFlag == NFileHeader::NLinkFlag::kHardLink)
  {
    item.PackSize = 0;
    item.Size = 0;
  }

  if (item.LinkFlag == NFileHeader::NLinkFlag::kDirectory)
  {
    // GNU tar ignores Size field, if LinkFlag is kDirectory
    // 21.02 : we set PackSize = 0 to be more compatible with GNU tar
    item.PackSize = 0;
    // item.Size = 0;
  }

  /*
    TAR standard requires sum of unsigned byte values.
    But some old TAR programs use sum of signed byte values.
    So we check both values.
  */
  // for (int y = 0; y < 100; y++) // for debug
  {
    UInt32 sum0 = 0;
    {
      for (unsigned i = 0; i < NFileHeader::kRecordSize; i++)
        sum0 += (Byte)buf[i];
    }
    if (sum0 != checkSum)
    {
      Int32 sum = 0;
      for (unsigned i = 0; i < NFileHeader::kRecordSize; i++)
        sum += (signed char)buf[i];
      if ((UInt32)sum != checkSum)
        return S_OK;
      item.IsSignedChecksum = true;
    }
  }

  item.HeaderSize += NFileHeader::kRecordSize;

  if (item.LinkFlag == NFileHeader::NLinkFlag::kSparse)
  {
    // OLD GNU format: parse sparse file information:
    // PackSize = cumulative size of all non-empty blocks of the file.
    // We read actual file size from 'realsize' member of oldgnu_header:
    RIF(ParseSize(buf + 483, item.Size, item.Size_IsBin))
    if (item.Size < item.PackSize) // additional check
      return S_OK;

    p = buf + 386;
    
    UInt64 end = 0, packSum = 0;
    unsigned numRecords = 4;
    unsigned isExtended = (Byte)p[4 * 24]; // (Byte)p[numRecords * 24];
    // the list of blocks contains non-empty blocks. All another data is empty.

    for (;;)
    {
      // const unsigned isExtended = (Byte)p[numRecords * 24];
      if (isExtended > 1)
        return S_OK;
      do
      {
        if (GetBe32(p) == 0)
        {
          if (isExtended)
            return S_OK;
          break;
        }
        CSparseBlock sb;
        RIF(ParseSize(p, sb.Offset))
        RIF(ParseSize(p + 12, sb.Size))
        p += 24;
        /* for all non-last blocks we expect :
             ((sb.Size & 0x1ff) == 0) && ((sb.Offset & 0x1ff) == 0)
           for last block : (sb.Size == 0) is possible.
        */
        if (sb.Offset < end
            || item.Size < sb.Offset
            || item.Size - sb.Offset < sb.Size)
          return S_OK;
        // optional check:
        if (sb.Size && ((end & 0x1ff) || (sb.Offset & 0x1ff)))
        {
          item.Method_Error = true; // relaxed check
          // return S_OK;
        }
        end = sb.Offset + sb.Size;
        packSum += sb.Size;
        item.SparseBlocks.Add(sb);
      }
      while (--numRecords);

      if (!isExtended)
        break;

      size_t processedSize = NFileHeader::kRecordSize;
      RINOK(ReadStream(SeqStream, buf, &processedSize))
      if (processedSize != NFileHeader::kRecordSize)
      {
        error = k_ErrorType_UnexpectedEnd;
        return S_OK;
      }
      item.HeaderSize += NFileHeader::kRecordSize;
      RINOK(Progress(item, 0))
      p = buf;
      numRecords = 21;
      isExtended = (Byte)p[21 * 24]; // (Byte)p[numRecords * 24];
    }
    // optional checks for strict size consistency:
    if (end != item.Size || packSum != item.PackSize)
    {
      item.Method_Error = true; // relaxed check
      // return S_OK;
    }
  }
 
  if (item.PackSize >= (UInt64)1 << 63) // optional check. It was checked in ParseSize() already
    return S_OK;
  filled = true;
  error = k_ErrorType_OK;
  return S_OK;
}


HRESULT CArchive::Progress(const CItemEx &item, UInt64 posOffset)
{
  if (!OpenCallback)
    return S_OK;
  const UInt64 pos = item.Get_DataPos() + posOffset;
  if (NumFiles - NumFiles_Prev < (1 << 16)
      // && NumRecords - NumRecords_Prev < (1 << 16)
      && pos - Pos_Prev < ((UInt32)1 << 28))
    return S_OK;
  {
    Pos_Prev = pos;
    NumFiles_Prev = NumFiles;
    // NumRecords_Prev = NumRecords;
    // Sleep(100); // for debug
    return OpenCallback->SetCompleted(&NumFiles, &pos);
  }
}


HRESULT CArchive::ReadDataToBuffer(const CItemEx &item,
    CTempBuffer &tb, size_t stringLimit)
{
  tb.Init();
  UInt64 packSize = item.Get_PackSize_Aligned();
  if (packSize == 0)
    return S_OK;

  UInt64 pos;

  {
    size_t size = stringLimit;
    if (size > packSize)
      size = (size_t)packSize;
    tb.Buffer.AllocAtLeast(size);
    size_t processedSize = size;
    const HRESULT res = ReadStream(SeqStream, tb.Buffer, &processedSize);
    pos = processedSize;
    if (processedSize != size)
    {
      error = k_ErrorType_UnexpectedEnd;
      return res;
    }
    RINOK(res)

    packSize -= size;
   
    size_t i;
    const Byte *p = tb.Buffer;
    for (i = 0; i < size; i++)
      if (p[i] == 0)
        break;
    if (i >= item.PackSize)
      tb.StringSize_IsConfirmed = true;
    if (i > item.PackSize)
    {
      tb.StringSize = (size_t)item.PackSize;
      tb.IsNonZeroTail = true;
    }
    else
    {
      tb.StringSize = i;
      if (i != size)
      {
        tb.StringSize_IsConfirmed = true;
        if (IsBufNonZero(p + i, size - i))
          tb.IsNonZeroTail = true;
      }
    }
   
    if (packSize == 0)
      return S_OK;
  }

  if (InStream)
  {
    RINOK(InStream->Seek((Int64)packSize, STREAM_SEEK_CUR, NULL))
    return S_OK;
  }
  const unsigned kBufSize = 1 << 15;
  Buffer.AllocAtLeast(kBufSize);

  do
  {
    RINOK(Progress(item, pos))

    unsigned size = kBufSize;
    if (size > packSize)
      size = (unsigned)packSize;
    size_t processedSize = size;
    const HRESULT res = ReadStream(SeqStream, Buffer, &processedSize);
    if (processedSize != size)
    {
      error = k_ErrorType_UnexpectedEnd;
      return res;
    }
    if (!tb.IsNonZeroTail)
    {
      if (IsBufNonZero(Buffer, size))
        tb.IsNonZeroTail = true;
    }
    packSize -= size;
    pos += size;
  }
  while (packSize != 0);
  return S_OK;
}
 


struct CPaxInfo: public CPaxTimes
{
  bool DoubleTagError;
  bool TagParsingError;
  bool UnknownLines_Overflow;
  bool Size_Defined;
  bool UID_Defined;
  bool GID_Defined;
  bool Path_Defined;
  bool Link_Defined;
  bool User_Defined;
  bool Group_Defined;
  bool SCHILY_fflags_Defined;
  
  UInt64 Size;
  UInt32 UID;
  UInt32 GID;

  AString Path;
  AString Link;
  AString User;
  AString Group;
  AString UnknownLines;
  AString SCHILY_fflags;
  
  bool ParseID(const AString &val, bool &defined, UInt32 &res)
  {
    if (defined)
      DoubleTagError = true;
    if (val.IsEmpty())
      return false;
    const char *end2;
    res = ConvertStringToUInt32(val.Ptr(), &end2);
    if (*end2 != 0)
      return false;
    defined = true;
    return true;
  }

  bool ParsePax(const CTempBuffer &tb, bool isFile);
};


static bool ParsePaxTime(const AString &src, CPaxTime &pt, bool &doubleTagError)
{
  if (pt.IsDefined())
    doubleTagError = true;
  pt.Clear();
  const char *s = src.Ptr();
  bool isNegative = false;
  if (*s == '-')
  {
    isNegative = true;
    s++;
  }
  const char *end;
  {
    UInt64 sec = ConvertStringToUInt64(s, &end);
    if (s == end)
      return false;
    if (sec >= ((UInt64)1 << 63))
      return false;
    if (isNegative)
      sec = (UInt64)-(Int64)sec;
    pt.Sec = (Int64)sec;
  }
  if (*end == 0)
  {
    pt.Ns = 0;
    pt.NumDigits = 0;
    return true;
  }
  if (*end != '.')
    return false;
  s = end + 1;

  UInt32 ns = 0;
  unsigned i;
  const unsigned kNsDigits = 9;
  for (i = 0;; i++)
  {
    const char c = s[i];
    if (c == 0)
      break;
    if (c < '0' || c > '9')
      return false;
    // we ignore digits after 9 digits as GNU TAR
    if (i < kNsDigits)
    {
      ns *= 10;
      ns += (unsigned)(c - '0');
    }
  }
  pt.NumDigits = (int)(i < kNsDigits ? i : kNsDigits);
  while (i < kNsDigits)
  {
    ns *= 10;
    i++;
  }
  if (isNegative && ns != 0)
  {
    pt.Sec--;
    ns = (UInt32)1000 * 1000 * 1000 - ns;
  }
  pt.Ns = ns;
  return true;
}


bool CPaxInfo::ParsePax(const CTempBuffer &tb, bool isFile)
{
  DoubleTagError = false;
  TagParsingError = false;
  UnknownLines_Overflow = false;
  Size_Defined = false;
  UID_Defined = false;
  GID_Defined = false;
  Path_Defined = false;
  Link_Defined = false;
  User_Defined = false;
  Group_Defined = false;
  SCHILY_fflags_Defined = false;
  
  // CPaxTimes::Clear();

  const char *s = (const char *)(const void *)(const Byte *)tb.Buffer;
  size_t rem = tb.StringSize;

  Clear();

  AString name, val;

  while (rem != 0)
  {
    unsigned i;
    for (i = 0;; i++)
    {
      if (i > 24 || i >= rem) // we use limitation for size of (size) field
        return false;
      if (s[i] == ' ')
        break;
    }
    if (i == 0)
      return false;
    const char *end;
    const UInt32 size = ConvertStringToUInt32(s, &end);
    const unsigned offset = (unsigned)(end - s) + 1;
    if (size > rem
        || size <= offset + 1
        || offset != i + 1
        || s[size - 1] != '\n')
      return false;

    for (i = offset; i < size; i++)
      if (s[i] == 0)
        return false;

    for (i = offset; i < size - 1; i++)
      if (s[i] == '=')
        break;
    if (i == size - 1)
      return false;

    name.SetFrom(s + offset, i - offset);
    val.SetFrom(s + i + 1, (unsigned)(size - 1 - (i + 1)));
    
    bool parsed = false;
    if (isFile)
    {
      bool isDetectedName = true;
      // only lower case (name) is supported
      if (name.IsEqualTo("path"))
      {
        if (Path_Defined)
          DoubleTagError = true;
        Path = val;
        Path_Defined = true;
        parsed = true;
      }
      else if (name.IsEqualTo("linkpath"))
      {
        if (Link_Defined)
          DoubleTagError = true;
        Link = val;
        Link_Defined = true;
        parsed = true;
      }
      else if (name.IsEqualTo("uname"))
      {
        if (User_Defined)
          DoubleTagError = true;
        User = val;
        User_Defined = true;
        parsed = true;
      }
      else if (name.IsEqualTo("gname"))
      {
        if (Group_Defined)
          DoubleTagError = true;
        Group = val;
        Group_Defined = true;
        parsed = true;
      }
      else if (name.IsEqualTo("uid"))
      {
        parsed = ParseID(val, UID_Defined, UID);
      }
      else if (name.IsEqualTo("gid"))
      {
        parsed = ParseID(val, GID_Defined, GID);
      }
      else if (name.IsEqualTo("size"))
      {
        if (Size_Defined)
          DoubleTagError = true;
        Size_Defined = false;
        if (!val.IsEmpty())
        {
          const char *end2;
          Size = ConvertStringToUInt64(val.Ptr(), &end2);
          if (*end2 == 0)
          {
            Size_Defined = true;
            parsed = true;
          }
        }
      }
      else if (name.IsEqualTo("mtime"))
        { parsed = ParsePaxTime(val, MTime, DoubleTagError); }
      else if (name.IsEqualTo("atime"))
        { parsed = ParsePaxTime(val, ATime, DoubleTagError); }
      else if (name.IsEqualTo("ctime"))
        { parsed = ParsePaxTime(val, CTime, DoubleTagError); }
      else if (name.IsEqualTo("SCHILY.fflags"))
      {
        if (SCHILY_fflags_Defined)
          DoubleTagError = true;
        SCHILY_fflags = val;
        SCHILY_fflags_Defined = true;
        parsed = true;
      }
      else
        isDetectedName = false;
      if (isDetectedName && !parsed)
        TagParsingError = true;
    }
    if (!parsed)
    {
      if (!UnknownLines_Overflow)
      {
        const unsigned addSize = size - offset;
        if (UnknownLines.Len() + addSize < (1 << 16))
          UnknownLines.AddFrom(s + offset, addSize);
        else
          UnknownLines_Overflow = true;
      }
    }

    s += size;
    rem -= size;
  }
  return true;
}


HRESULT CArchive::ReadItem2(CItemEx &item)
{
  // CItem
  
  item.SparseBlocks.Clear();
  item.PaxTimes.Clear();

  // CItemEx

  item.HeaderSize = 0;
  item.Num_Pax_Records = 0;

  item.LongName_WasUsed = false;
  item.LongName_WasUsed_2 = false;

  item.LongLink_WasUsed = false;
  item.LongLink_WasUsed_2 = false;

  item.HeaderError = false;
  item.Method_Error = false;
  item.IsSignedChecksum = false;
  item.Prefix_WasUsed = false;
  
  item.Pax_Error = false;
  item.Pax_Overflow = false;
  item.pax_path_WasUsed = false;
  item.pax_link_WasUsed = false;
  item.pax_size_WasUsed = false;
  
  item.PaxExtra.Clear();
  item.SCHILY_fflags.Empty();

  item.EncodingCharacts.Clear();
  
  // CArchive temp variable

  NameBuf.Init();
  LinkBuf.Init();
  PaxBuf.Init();
  PaxBuf_global.Init();
  
  UInt64 numExtraRecords = 0;

  for (;;)
  {
    RINOK(Progress(item, 0))
    RINOK(GetNextItemReal(item))
    // NumRecords++;

    if (!filled)
    {
      if (error == k_ErrorType_OK)
      if (numExtraRecords != 0
          || item.LongName_WasUsed
          || item.LongLink_WasUsed
          || item.Num_Pax_Records != 0)
        error = k_ErrorType_Corrupted;
      return S_OK;
    }
    if (error != k_ErrorType_OK)
      return S_OK;

    numExtraRecords++;

    const char lf = item.LinkFlag;
    if (lf == NFileHeader::NLinkFlag::kGnu_LongName ||
        lf == NFileHeader::NLinkFlag::kGnu_LongLink)
    {
      // GNU tar ignores item.Name after LinkFlag test
      // 22.00 : now we also ignore item.Name here
      /*
      if (item.Name != NFileHeader::kLongLink &&
          item.Name != NFileHeader::kLongLink2)
      {
        break;
        // return S_OK;
      }
      */

      CTempBuffer *tb =
          lf == NFileHeader::NLinkFlag::kGnu_LongName ?
          &NameBuf :
          &LinkBuf;
      
      /*
      if (item.PackSize > (1 << 29))
      {
        // break;
        return S_OK;
      }
      */

      const unsigned kLongNameSizeMax = (unsigned)1 << 14;
      RINOK(ReadDataToBuffer(item, *tb, kLongNameSizeMax))
      if (error != k_ErrorType_OK)
        return S_OK;

      if (lf == NFileHeader::NLinkFlag::kGnu_LongName)
      {
        item.LongName_WasUsed_2 =
        item.LongName_WasUsed;
        item.LongName_WasUsed = true;
      }
      else
      {
        item.LongLink_WasUsed_2 =
        item.LongLink_WasUsed;
        item.LongLink_WasUsed = true;
      }

      if (!tb->StringSize_IsConfirmed)
        tb->StringSize = 0;
      item.HeaderSize += item.Get_PackSize_Aligned();
      if (tb->StringSize == 0 ||
          tb->StringSize + 1 != item.PackSize)
        item.HeaderError = true;
      if (tb->IsNonZeroTail)
        item.HeaderError = true;
      continue;
    }

    if (lf == NFileHeader::NLinkFlag::kGlobal ||
        lf == NFileHeader::NLinkFlag::kPax ||
        lf == NFileHeader::NLinkFlag::kPax_2)
    {
      // GNU tar ignores item.Name after LinkFlag test
      // 22.00 : now we also ignore item.Name here
      /*
      if (item.PackSize > (UInt32)1 << 26)
      {
        break; // we don't want big PaxBuf files
        // return S_OK;
      }
      */
      const unsigned kParsingPaxSizeMax = (unsigned)1 << 26;

      const bool isStartHeader = (item.HeaderSize == NFileHeader::kRecordSize);

      CTempBuffer *tb = (lf == NFileHeader::NLinkFlag::kGlobal ? &PaxBuf_global : &PaxBuf);

      RINOK(ReadDataToBuffer(item, *tb, kParsingPaxSizeMax))
      if (error != k_ErrorType_OK)
        return S_OK;

      item.HeaderSize += item.Get_PackSize_Aligned();

      if (tb->StringSize != item.PackSize
          || tb->StringSize == 0
          || tb->IsNonZeroTail)
        item.Pax_Error = true;

      item.Num_Pax_Records++;
      if (lf != NFileHeader::NLinkFlag::kGlobal)
      {
        item.PaxExtra.RecordPath = item.Name;
        continue;
      }
      // break; // for debug
      {
        if (PaxGlobal_Defined)
          _is_PaxGlobal_Error = true;
        CPaxInfo paxInfo;
        if (paxInfo.ParsePax(PaxBuf_global, false))
        {
          PaxGlobal.RawLines = paxInfo.UnknownLines;
          PaxGlobal.RecordPath = item.Name;
          PaxGlobal_Defined = true;
        }
        else
          _is_PaxGlobal_Error = true;
        
        if (isStartHeader
            && item.Num_Pax_Records == 1
            && numExtraRecords == 1)
        {
          // we skip global pax header info after parsing
          item.HeaderPos += item.HeaderSize;
          item.HeaderSize = 0;
          item.Num_Pax_Records = 0;
          numExtraRecords = 0;
        }
        else
          _is_PaxGlobal_Error = true;
      }
      continue;
    }
    
    /*
    if (lf == NFileHeader::NLinkFlag::kDumpDir ||
        lf == NFileHeader::NLinkFlag::kSparse)
    {
      // GNU Extensions to the Archive Format
      break;
    }
    if (lf > '7' || (lf < '0' && lf != 0))
    {
      break;
      // return S_OK;
    }
    */
    break;
  }
    
  // we still use name from main header, if long_name is bad
  if (item.LongName_WasUsed && NameBuf.StringSize != 0)
  {
    NameBuf.CopyToString(item.Name);
    // item.Name_CouldBeReduced = false;
  }
  
  if (item.LongLink_WasUsed)
  {
    // we use empty link, if long_link is bad
    LinkBuf.CopyToString(item.LinkName);
    // item.LinkName_CouldBeReduced = false;
  }
  
  error = k_ErrorType_OK;
  
  if (PaxBuf.StringSize != 0)
  {
    CPaxInfo paxInfo;
    if (!paxInfo.ParsePax(PaxBuf, true))
      item.Pax_Error = true;
    else
    {
      if (paxInfo.Path_Defined) //  if (!paxInfo.Path.IsEmpty())
      {
        item.Name = paxInfo.Path;
        item.pax_path_WasUsed = true;
      }
      if (paxInfo.Link_Defined) // (!paxInfo.Link.IsEmpty())
      {
        item.LinkName = paxInfo.Link;
        item.pax_link_WasUsed = true;
      }
      if (paxInfo.User_Defined)
      {
        item.User = paxInfo.User;
        // item.pax_uname_WasUsed = true;
      }
      if (paxInfo.Group_Defined)
      {
        item.Group = paxInfo.Group;
        // item.pax_gname_WasUsed = true;
      }
      if (paxInfo.SCHILY_fflags_Defined)
      {
        item.SCHILY_fflags = paxInfo.SCHILY_fflags;
        // item.SCHILY_fflags_WasUsed = true;
      }
      if (paxInfo.UID_Defined)
      {
        item.UID = (UInt32)paxInfo.UID;
      }
      if (paxInfo.GID_Defined)
      {
        item.GID = (UInt32)paxInfo.GID;
      }
      
      if (paxInfo.Size_Defined)
      {
        const UInt64 piSize = paxInfo.Size;
        // GNU TAR ignores (item.Size) in that case
        if (item.Size != 0 && item.Size != piSize)
          item.Pax_Error = true;
        if (piSize >= ((UInt64)1 << 63))
          item.Pax_Error = true;
        else
        {
          item.Size = piSize;
          item.PackSize = piSize;
          item.pax_size_WasUsed = true;
        }
      }
      
      item.PaxTimes = paxInfo;
      item.PaxExtra.RawLines = paxInfo.UnknownLines;
      if (paxInfo.UnknownLines_Overflow)
        item.Pax_Overflow = true;
      if (paxInfo.TagParsingError)
        item.Pax_Error = true;
      if (paxInfo.DoubleTagError)
        item.Pax_Error = true;
    }
  }

  return S_OK;
}



HRESULT CArchive::ReadItem(CItemEx &item)
{
  error = k_ErrorType_OK;
  filled = false;
  item.HeaderPos = _phySize;
  
  const HRESULT res = ReadItem2(item);
  
  /*
  if (error == k_ErrorType_Warning)
    _is_Warning = true;
  else
  */
  
  if (error != k_ErrorType_OK)
    _error = error;
  
  RINOK(res)

  if (filled)
  {
    if (item.IsMagic_GNU())
      _are_Gnu = true;
    else if (item.IsMagic_Posix_ustar_00())
      _are_Posix = true;
    
    if (item.Num_Pax_Records != 0)
      _are_Pax = true;

    if (item.PaxTimes.MTime.IsDefined())  _are_mtime = true;
    if (item.PaxTimes.ATime.IsDefined())  _are_atime = true;
    if (item.PaxTimes.CTime.IsDefined())  _are_ctime = true;
    if (!item.SCHILY_fflags.IsEmpty())  _are_SCHILY_fflags = true;

    if (item.pax_path_WasUsed)
      _are_pax_path = true;
    if (item.pax_link_WasUsed)
      _are_pax_link = true;
    if (item.LongName_WasUsed)
      _are_LongName = true;
    if (item.LongLink_WasUsed)
      _are_LongLink = true;
    if (item.Prefix_WasUsed)
      _pathPrefix_WasUsed = true;
    /*
    if (item.IsSparse())
      _isSparse = true;
    */
    if (item.Is_PaxExtendedHeader())
      _are_Pax_Items = true;
    if (item.IsThereWarning()
        || item.HeaderError
        || item.Pax_Error)
      _is_Warning = true;
  }

  const UInt64 headerEnd = item.HeaderPos + item.HeaderSize;
  // _headersSize += headerEnd - _phySize;
  // we don't count skipped records
  _headersSize += item.HeaderSize;
  _phySize = headerEnd;
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Archive/Tar/TarOut.cpp ================ */
// TarOut.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

using namespace NFileHeader;

// it's path prefix assigned by 7-Zip to show that file path was cut
#define K_PREFIX_PATH_CUT "@PathCut"

static const UInt32 k_7_oct_digits_Val_Max = ((UInt32)1 << (7 * 3)) - 1;

static void WriteOctal_8(char *s, UInt32 val)
{
  const unsigned kNumDigits = 8 - 1;
  if (val >= ((UInt32)1 << (kNumDigits * 3)))
  {
    val = 0;
    // return false;
  }
  for (unsigned i = 0; i < kNumDigits; i++)
  {
    s[kNumDigits - 1 - i] = (char)('0' + (val & 7));
    val >>= 3;
  }
  // return true;
}

static void WriteBin_64bit(char *s, UInt64 val)
{
  for (unsigned i = 0; i < 8; i++, val <<= 8)
    s[i] = (char)(val >> 56);
}

static void WriteOctal_12(char *s, UInt64 val)
{
  const unsigned kNumDigits = 12 - 1;
  if (val >= ((UInt64)1 << (kNumDigits * 3)))
  {
    // GNU extension;
    s[0] = (char)(Byte)0x80;
    s[1] = s[2] = s[3] = 0;
    WriteBin_64bit(s + 4, val);
    return;
  }
  for (unsigned i = 0; i < kNumDigits; i++)
  {
    s[kNumDigits - 1 - i] = (char)('0' + (val & 7));
    val >>= 3;
  }
}

static void WriteOctal_12_Signed(char *s, const Int64 val)
{
  if (val >= 0)
  {
    WriteOctal_12(s, (UInt64)val);
    return;
  }
  s[0] = s[1] = s[2] = s[3] = (char)(Byte)0xFF;
  WriteBin_64bit(s + 4, (UInt64)val);
}

static void CopyString(char *dest, const AString &src, const unsigned maxSize)
{
  unsigned len = src.Len();
  if (len == 0)
    return;
  // 21.07: new gnu : we don't require additional 0 character at the end
  // if (len >= maxSize)
  if (len > maxSize)
  {
    len = maxSize;
    /*
    // oldgnu needs 0 character at the end
    len = maxSize - 1;
    dest[len] = 0;
    */
  }
  memcpy(dest, src.Ptr(), len);
}

// #define RETURN_IF_NOT_TRUE(x) { if (!(x)) return E_INVALIDARG; }
#define RETURN_IF_NOT_TRUE(x) { x; }

#define COPY_STRING_CHECK(dest, src, size) \
    CopyString(dest, src, size);   dest += (size);

#define WRITE_OCTAL_8_CHECK(dest, src) \
    RETURN_IF_NOT_TRUE(WriteOctal_8(dest, src))
    

HRESULT COutArchive::WriteHeaderReal(const CItem &item, bool isPax
    // , bool zero_PackSize
    // , bool zero_MTime
    )
{
  /*
    if (isPax) { we don't use Glob_Name and Prefix }
    if (!isPax)
    {
      we use Glob_Name if it's not empty
      we use Prefix    if it's not empty
    }
  */
  char record[kRecordSize];
  memset(record, 0, kRecordSize);
  char *cur = record;

  COPY_STRING_CHECK (cur,
      (!isPax && !Glob_Name.IsEmpty()) ? Glob_Name : item.Name,
      kNameSize)

  WRITE_OCTAL_8_CHECK (cur, item.Mode)  cur += 8; // & k_7_oct_digits_Val_Max
  WRITE_OCTAL_8_CHECK (cur, item.UID)   cur += 8;
  WRITE_OCTAL_8_CHECK (cur, item.GID)   cur += 8;

  WriteOctal_12 (cur, /* zero_PackSize ? 0 : */ item.PackSize); cur += 12;
  WriteOctal_12_Signed (cur, /* zero_MTime ? 0 : */ item.MTime); cur += 12;
  
  // we will use binary init for checksum instead of memset
  // checksum field:
  // memset(cur, ' ', 8);
  cur += 8;

  *cur++ = item.LinkFlag;

  COPY_STRING_CHECK (cur, item.LinkName, kNameSize)

  memcpy(cur, item.Magic, 8);
  cur += 8;

  COPY_STRING_CHECK (cur, item.User, kUserNameSize)
  COPY_STRING_CHECK (cur, item.Group, kGroupNameSize)

  const bool needDevice = (IsPosixMode && !isPax);
  
  if (item.DeviceMajor_Defined)
    WRITE_OCTAL_8_CHECK (cur, item.DeviceMajor)
  else if (needDevice)
    WRITE_OCTAL_8_CHECK (cur, 0)
  cur += 8;
  
  if (item.DeviceMinor_Defined)
    WRITE_OCTAL_8_CHECK (cur, item.DeviceMinor)
  else if (needDevice)
    WRITE_OCTAL_8_CHECK (cur, 0)
  cur += 8;

  if (!isPax && !Prefix.IsEmpty())
  {
    COPY_STRING_CHECK (cur, Prefix, kPrefixSize)
  }

  if (item.Is_Sparse())
  {
    record[482] = (char)(item.SparseBlocks.Size() > 4 ? 1 : 0);
    WriteOctal_12(record + 483, item.Size);
    for (unsigned i = 0; i < item.SparseBlocks.Size() && i < 4; i++)
    {
      const CSparseBlock &sb = item.SparseBlocks[i];
      char *p = record + 386 + 24 * i;
      WriteOctal_12(p, sb.Offset);
      WriteOctal_12(p + 12, sb.Size);
    }
  }

  {
    UInt32 sum = (unsigned)(' ') * 8; // we use binary init
    {
      for (unsigned i = 0; i < kRecordSize; i++)
        sum += (Byte)record[i];
    }
    /* checksum field is formatted differently from the
       other fields: it has [6] digits, a null, then a space. */
    // WRITE_OCTAL_8_CHECK(record + 148, sum);
    const unsigned kNumDigits = 6;
    for (unsigned i = 0; i < kNumDigits; i++)
    {
      record[148 + kNumDigits - 1 - i] = (char)('0' + (sum & 7));
      sum >>= 3;
    }
    // record[148 + 6] = 0; // we need it, if we use memset(' ') init
    record[148 + 7] = ' '; // we need it, if we use binary init
  }

  RINOK(Write_Data(record, kRecordSize))

  if (item.Is_Sparse())
  {
    for (unsigned i = 4; i < item.SparseBlocks.Size();)
    {
      memset(record, 0, kRecordSize);
      for (unsigned t = 0; t < 21 && i < item.SparseBlocks.Size(); t++, i++)
      {
        const CSparseBlock &sb = item.SparseBlocks[i];
        char *p = record + 24 * t;
        WriteOctal_12(p, sb.Offset);
        WriteOctal_12(p + 12, sb.Size);
      }
      record[21 * 24] = (char)(i < item.SparseBlocks.Size() ? 1 : 0);
      RINOK(Write_Data(record, kRecordSize))
    }
  }

  return S_OK;
}


static void AddPaxLine(AString &s, const char *name, const AString &val)
{
  // s.Add_LF(); // for debug
  const unsigned len = 3 + (unsigned)strlen(name) + val.Len();
  AString n;
  for (unsigned numDigits = 1;; numDigits++)
  {
    n.Empty();
    n.Add_UInt32(numDigits + len);
    if (numDigits == n.Len())
      break;
  }
  s += n;
  s.Add_Space();
  s += name;
  s.Add_Char('=');
  s += val;
  s.Add_LF();
}

// pt is defined : (pt.NumDigits >= 0)
static void AddPaxTime(AString &s, const char *name, const CPaxTime &pt,
    const CTimeOptions &options)
{
  unsigned numDigits = (unsigned)pt.NumDigits;
  if (numDigits > options.NumDigitsMax)
    numDigits = options.NumDigitsMax;

  bool needNs = false;
  UInt32 ns = 0;
  if (numDigits != 0)
  {
    ns = pt.Ns;
    // if (ns != 0) before reduction, we show all digits after digits reduction
    needNs = (ns != 0 || options.RemoveZeroMode == k_PaxTimeMode_DontRemoveZero);
    UInt32 d = 1;
    for (unsigned k = numDigits; k < 9; k++)
      d *= 10;
    ns /= d;
    ns *= d;
  }

  AString v;
  {
    Int64 sec = pt.Sec;
    if (pt.Sec < 0)
    {
      sec = -sec;
      v.Add_Minus();
      if (ns != 0)
      {
        ns = 1000*1000*1000 - ns;
        sec--;
      }
    }
    v.Add_UInt64((UInt64)sec);
  }
  
  if (needNs)
  {
    AString d;
    d.Add_UInt32(ns);
    while (d.Len() < 9)
      d.InsertAtFront('0');
    // here we have precision
    while (d.Len() > (unsigned)numDigits)
      d.DeleteBack();
    // GNU TAR reduces '0' digits.
    if (options.RemoveZeroMode == k_PaxTimeMode_RemoveZero_Always)
    while (!d.IsEmpty() && d.Back() == '0')
      d.DeleteBack();

    if (!d.IsEmpty())
    {
      v.Add_Dot();
      v += d;
      // v += "1234567009999"; // for debug
      // for (int y = 0; y < 1000; y++) v += '8'; // for debug
    }
  }

  AddPaxLine(s, name, v);
}


static void AddPax_UInt32_ifBig(AString &s, const char *name, const UInt32 &v)
{
  if (v > k_7_oct_digits_Val_Max)
  {
    AString s2;
    s2.Add_UInt32(v);
    AddPaxLine(s, name, s2);
  }
}
  

/* OLD_GNU_TAR: writes name with zero at the end
   NEW_GNU_TAR: can write name filled with all kNameSize characters */

static const unsigned kNameSize_Max =
    kNameSize;     // NEW_GNU_TAR / 7-Zip 21.07
    // kNameSize - 1; // OLD_GNU_TAR / old 7-Zip

#define DOES_NAME_FIT_IN_FIELD(name) ((name).Len() <= kNameSize_Max)


HRESULT COutArchive::WriteHeader(const CItem &item)
{
  Glob_Name.Empty();
  Prefix.Empty();

  unsigned namePos = 0;
  bool needPathCut = false;
  bool allowPrefix = false;

  if (!DOES_NAME_FIT_IN_FIELD(item.Name))
  {
    const char *s = item.Name;
    const char *p = s + item.Name.Len() - 1;
    for (; *p == '/' && p != s; p--)
      {}
    for (; p != s && p[-1] != '/'; p--)
      {}
    namePos = (unsigned)(p - s);
    needPathCut = true;
  }

  if (IsPosixMode)
  {
    AString s;
    
    if (needPathCut)
    {
      const unsigned nameLen = item.Name.Len() - namePos;
      if (   item.LinkFlag >= NLinkFlag::kNormal
          && item.LinkFlag <= NLinkFlag::kDirectory
          && namePos > 1
          && nameLen != 0
          // && IsPrefixAllowed
          && item.IsMagic_Posix_ustar_00())
      {
        /* GNU TAR decoder supports prefix field, only if (magic)
           signature matches 6-bytes "ustar\0".
           so here we use prefix field only in posix mode with posix signature */

        allowPrefix = true;
        // allowPrefix = false; // for debug
        if (namePos <= kPrefixSize + 1 && nameLen <= kNameSize_Max)
        {
          needPathCut = false;
          /* we will set Prefix and Glob_Name later, for such conditions:
             if (!DOES_NAME_FIT_IN_FIELD(item.Name) && !needPathCut) */
        }
      }

      if (needPathCut)
        AddPaxLine(s, "path", item.Name);
    }
    
    // AddPaxLine(s, "testname", AString("testval")); // for debug
    
    if (item.LinkName.Len() > kNameSize_Max)
      AddPaxLine(s, "linkpath", item.LinkName);
    
    const UInt64 kPaxSize_Limit = ((UInt64)1 << 33);
    // const UInt64 kPaxSize_Limit = ((UInt64)1 << 1); // for debug
    // bool zero_PackSize = false;
    if (item.PackSize >= kPaxSize_Limit)
    {
      /* GNU TAR in pax mode sets PackSize = 0 in main record, if pack_size >= 8 GiB
         But old 7-Zip doesn't detect "size" property from pax header.
         So we write real size (>= 8 GiB) to main record in binary format,
         and old 7-Zip can decode size correctly */
      // zero_PackSize = true;
      AString v;
      v.Add_UInt64(item.PackSize);
      AddPaxLine(s, "size", v);
    }

    /* GNU TAR encoder can set "devmajor" / "devminor" attributes,
       but GNU TAR decoder doesn't parse "devmajor" / "devminor" */
    if (item.DeviceMajor_Defined)
      AddPax_UInt32_ifBig(s, "devmajor", item.DeviceMajor);
    if (item.DeviceMinor_Defined)
      AddPax_UInt32_ifBig(s, "devminor", item.DeviceMinor);

    AddPax_UInt32_ifBig(s, "uid", item.UID);
    AddPax_UInt32_ifBig(s, "gid", item.GID);
    
    const UInt64 kPax_MTime_Limit = ((UInt64)1 << 33);
    const bool zero_MTime = (
        item.MTime < 0 ||
        item.MTime >= (Int64)kPax_MTime_Limit);

    const CPaxTime &mtime = item.PaxTimes.MTime;
    if (mtime.IsDefined())
    {
      bool needPax = false;
      if (zero_MTime)
        needPax = true;
      else if (TimeOptions.NumDigitsMax > 0)
        if (mtime.Ns != 0 ||
            (mtime.NumDigits != 0 &&
            TimeOptions.RemoveZeroMode == k_PaxTimeMode_DontRemoveZero))
          needPax = true;
      if (needPax)
        AddPaxTime(s, "mtime", mtime, TimeOptions);
    }
    
    if (item.PaxTimes.ATime.IsDefined())
      AddPaxTime(s, "atime", item.PaxTimes.ATime, TimeOptions);
    if (item.PaxTimes.CTime.IsDefined())
      AddPaxTime(s, "ctime", item.PaxTimes.CTime, TimeOptions);

    if (item.User.Len() > kUserNameSize)
      AddPaxLine(s, "uname", item.User);
    if (item.Group.Len() > kGroupNameSize)
      AddPaxLine(s, "gname", item.Group);

    /*
    // for debug
    AString a ("11"); for (int y = 0; y < (1 << 24); y++) AddPaxLine(s, "temp", a);
    */

    const unsigned paxSize = s.Len();
    if (paxSize != 0)
    {
      CItem mi = item;
      mi.LinkName.Empty();
      // SparseBlocks will be ignored by Is_Sparse()
      // mi.SparseBlocks.Clear();
      //  we use "PaxHeader/*" for compatibility with previous 7-Zip decoder

      // GNU TAR writes empty for these fields;
      mi.User.Empty();
      mi.Group.Empty();
      mi.UID = 0;
      mi.GID = 0;

      mi.DeviceMajor_Defined = false;
      mi.DeviceMinor_Defined = false;

      mi.Name = "PaxHeader/@PaxHeader";
      mi.Mode = 0644; // octal
      if (zero_MTime)
        mi.MTime = 0;
      mi.LinkFlag = NLinkFlag::kPax;
      // mi.LinkFlag = 'Z'; // for debug
      mi.PackSize = paxSize;
      // for (unsigned y = 0; y < 1; y++) { // for debug
      RINOK(WriteHeaderReal(mi, true)) // isPax
      RINOK(Write_Data_And_Residual(s, paxSize))
      // } // for debug
      /*
        we can send (zero_MTime) for compatibility with gnu tar output.
        we can send (zero_MTime = false) for better compatibility with old 7-Zip
      */
      // return WriteHeaderReal(item);
      /*
      false, // isPax
      false, // zero_PackSize
      false); // zero_MTime
      */
    }
  }
  else // !PosixMode
  if (!DOES_NAME_FIT_IN_FIELD(item.Name) ||
      !DOES_NAME_FIT_IN_FIELD(item.LinkName))
  {
    // here we can get all fields from main (item) or create new empty item
    /*
    CItem mi;
    mi.SetDefaultWriteFields();
    */
    CItem mi = item;
    mi.LinkName.Empty();
    // SparseBlocks will be ignored by Is_Sparse()
    // mi.SparseBlocks.Clear();
    mi.Name = kLongLink;
    // mi.Name = "././@BAD_LONG_LINK_TEST"; // for debug
    // 21.07 : we set Mode and MTime props as in GNU TAR:
    mi.Mode = 0644; // octal
    mi.MTime = 0;

    mi.User.Empty();
    mi.Group.Empty();
    /*
      gnu tar sets "root" for such items:
        uid_to_uname (0, &uname);
        gid_to_gname (0, &gname);
    */
    /*
    mi.User = "root";
    mi.Group = "root";
    */
    mi.UID = 0;
    mi.GID = 0;
    mi.DeviceMajor_Defined = false;
    mi.DeviceMinor_Defined = false;

    
    for (unsigned i = 0; i < 2; i++)
    {
      const AString *name;
      // We suppose that GNU TAR also writes item for long link before item for LongName?
      if (i == 0)
      {
        mi.LinkFlag = NLinkFlag::kGnu_LongLink;
        name = &item.LinkName;
      }
      else
      {
        mi.LinkFlag = NLinkFlag::kGnu_LongName;
        name = &item.Name;
      }
      if (DOES_NAME_FIT_IN_FIELD(*name))
        continue;
      // GNU TAR writes null character after NAME to file. We do same here:
      const unsigned nameStreamSize = name->Len() + 1;
      mi.PackSize = nameStreamSize;
      // for (unsigned y = 0; y < 3; y++) { // for debug
      RINOK(WriteHeaderReal(mi))
      RINOK(Write_Data_And_Residual(name->Ptr(), nameStreamSize))
      // }
      
      // for debug
      /*
      const unsigned kSize = (1 << 29) + 16;
      CByteBuffer buf;
      buf.Alloc(kSize);
      memset(buf, 0, kSize);
      memcpy(buf, name->Ptr(), name->Len());
      const unsigned nameStreamSize = kSize;
      mi.PackSize = nameStreamSize;
      // for (unsigned y = 0; y < 3; y++) { // for debug
      RINOK(WriteHeaderReal(mi));
      RINOK(WriteBytes(buf, nameStreamSize));
      RINOK(FillDataResidual(nameStreamSize));
      */
    }
  }

  // bool fals = false; if (fals) // for debug: for bit-to-bit output compatibility with GNU TAR

  if (!DOES_NAME_FIT_IN_FIELD(item.Name))
  {
    const unsigned nameLen = item.Name.Len() - namePos;
    if (!needPathCut)
      Prefix.SetFrom(item.Name, namePos - 1);
    else
    {
      Glob_Name = K_PREFIX_PATH_CUT "/_pc_";
      
      if (namePos == 0)
        Glob_Name += "root";
      else
      {
        Glob_Name += "crc32/";
        char temp[12];
        ConvertUInt32ToHex8Digits(CrcCalc(item.Name, namePos - 1), temp);
        Glob_Name += temp;
      }
      
      if (!allowPrefix || Glob_Name.Len() + 1 + nameLen <= kNameSize_Max)
        Glob_Name.Add_Slash();
      else
      {
        Prefix = Glob_Name;
        Glob_Name.Empty();
      }
    }
    Glob_Name.AddFrom(item.Name.Ptr(namePos), nameLen);
  }

  return WriteHeaderReal(item);
}


HRESULT COutArchive::Write_Data(const void *data, unsigned size)
{
  Pos += size;
  return WriteStream(Stream, data, size);
}

HRESULT COutArchive::Write_AfterDataResidual(UInt64 dataSize)
{
  const unsigned v = ((unsigned)dataSize & (kRecordSize - 1));
  if (v == 0)
    return S_OK;
  const unsigned rem = kRecordSize - v;
  Byte buf[kRecordSize];
  memset(buf, 0, rem);
  return Write_Data(buf, rem);
}


HRESULT COutArchive::Write_Data_And_Residual(const void *data, unsigned size)
{
  RINOK(Write_Data(data, size))
  return Write_AfterDataResidual(size);
}


HRESULT COutArchive::WriteFinishHeader()
{
  Byte record[kRecordSize];
  memset(record, 0, kRecordSize);

  const unsigned kNumFinishRecords = 2;

  /* GNU TAR by default uses --blocking-factor=20 (512 * 20 = 10 KiB)
     we also can use cluster alignment:
  const unsigned numBlocks = (unsigned)(Pos / kRecordSize) + kNumFinishRecords;
  const unsigned kNumClusterBlocks = (1 << 3); // 8 blocks = 4 KiB
  const unsigned numFinishRecords = kNumFinishRecords + ((kNumClusterBlocks - numBlocks) & (kNumClusterBlocks - 1));
  */
  
  for (unsigned i = 0; i < kNumFinishRecords; i++)
  {
    RINOK(Write_Data(record, kRecordSize))
  }
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Archive/Tar/TarRegister.cpp ================ */
// TarRegister.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

static const Byte k_Signature[] = { 'u', 's', 't', 'a', 'r' };

REGISTER_ARC_IO(
  "tar", "tar ova", NULL, 0xEE,
  k_Signature,
  NFileHeader::kUstarMagic_Offset,
    NArcInfoFlags::kStartOpen
  | NArcInfoFlags::kSymLinks
  | NArcInfoFlags::kHardLinks
  | NArcInfoFlags::kMTime
  | NArcInfoFlags::kMTime_Default
  // | NArcInfoTimeFlags::kCTime
  // | NArcInfoTimeFlags::kATime
  , TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kWindows)
  | TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kUnix)
  | TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::k1ns)
  | TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT (NFileTimeType::kUnix)
  , IsArc_Tar)
 
}}

/* ================ unit: CPP/7zip/Archive/Tar/TarUpdate.cpp ================ */
// TarUpdate.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace NTar {

static void FILETIME_To_PaxTime(const FILETIME &ft, CPaxTime &pt)
{
  UInt32 ns;
  pt.Sec = NWindows::NTime::FileTime_To_UnixTime64_and_Quantums(ft, ns);
  pt.Ns = ns * 100;
  pt.NumDigits = 7;
}


HRESULT Prop_To_PaxTime(const NWindows::NCOM::CPropVariant &prop, CPaxTime &pt)
{
  pt.Clear();
  if (prop.vt == VT_EMPTY)
  {
    // pt.Sec = 0;
    return S_OK;
  }
  if (prop.vt != VT_FILETIME)
    return E_INVALIDARG;
  {
    UInt32 ns;
    pt.Sec = NWindows::NTime::FileTime_To_UnixTime64_and_Quantums(prop.filetime, ns);
    ns *= 100;
    pt.NumDigits = 7;
    const unsigned prec = prop.wReserved1;
    if (prec >= k_PropVar_TimePrec_Base)
    {
      pt.NumDigits = (int)(prec - k_PropVar_TimePrec_Base);
      if (prop.wReserved2 < 100)
        ns += prop.wReserved2;
    }
    pt.Ns = ns;
    return S_OK;
  }
}


static HRESULT GetTime(IStreamGetProp *getProp, UInt32 pid, CPaxTime &pt)
{
  pt.Clear();
  NWindows::NCOM::CPropVariant prop;
  RINOK(getProp->GetProperty(pid, &prop))
  return Prop_To_PaxTime(prop, pt);
}


static HRESULT GetUser(IStreamGetProp *getProp,
    UInt32 pidName, UInt32 pidId, AString &name, UInt32 &id,
    UINT codePage, unsigned utfFlags)
{
  // printf("\nGetUser\n");
  // we keep  old values, if both   GetProperty() return VT_EMPTY
  // we clear old values, if any of GetProperty() returns non-VT_EMPTY;
  bool isSet = false;
  {
    NWindows::NCOM::CPropVariant prop;
    RINOK(getProp->GetProperty(pidId, &prop))
    if (prop.vt == VT_UI4)
    {
      isSet = true;
      id = prop.ulVal;
      name.Empty();
    }
    else if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
  }
  {
    NWindows::NCOM::CPropVariant prop;
    RINOK(getProp->GetProperty(pidName, &prop))
    if (prop.vt == VT_BSTR)
    {
      const UString s = prop.bstrVal;
      Get_AString_From_UString(s, name, codePage, utfFlags);
      // printf("\ngetProp->GetProperty(pidName, &prop) : %s" , name.Ptr());
      if (!isSet)
        id = 0;
    }
    else if (prop.vt == VT_UI4)
    {
      id = prop.ulVal;
      name.Empty();
    }
    else if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
  }
  return S_OK;
}


/*
static HRESULT GetDevice(IStreamGetProp *getProp,
    UInt32 &majo, UInt32 &mino, bool &majo_defined, bool &mino_defined)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(getProp->GetProperty(kpidDevice, &prop));
  if (prop.vt == VT_EMPTY)
    return S_OK;
  if (prop.vt != VT_UI8)
    return E_INVALIDARG;
  {
    printf("\nTarUpdate.cpp :: GetDevice()\n");
    const UInt64 v = prop.uhVal.QuadPart;
    majo = MY_dev_major(v);
    mino = MY_dev_minor(v);
    majo_defined = true;
    mino_defined = true;
  }
  return S_OK;
}
*/

static HRESULT GetDevice(IStreamGetProp *getProp,
    UInt32 pid, UInt32 &id, bool &defined)
{
  defined = false;
  NWindows::NCOM::CPropVariant prop;
  RINOK(getProp->GetProperty(pid, &prop))
  if (prop.vt == VT_EMPTY)
    return S_OK;
  if (prop.vt == VT_UI4)
  {
    id = prop.ulVal;
    defined = true;
    return S_OK;
  }
  return E_INVALIDARG;
}


HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<NArchive::NTar::CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    const CUpdateOptions &options,
    IArchiveUpdateCallback *updateCallback)
{
  COutArchive outArchive;
  outArchive.Create(outStream);
  outArchive.Pos = 0;
  outArchive.IsPosixMode = options.PosixMode;
  outArchive.TimeOptions = options.TimeOptions;

  Z7_DECL_CMyComPtr_QI_FROM(IOutStream, outSeekStream, outStream)
  Z7_DECL_CMyComPtr_QI_FROM(IStreamSetRestriction, setRestriction, outStream)
  Z7_DECL_CMyComPtr_QI_FROM(IArchiveUpdateCallbackFile, opCallback, outStream)

  if (outSeekStream)
  {
    /*
    // for debug
    Byte buf[1 << 14];
    memset (buf, 0, sizeof(buf));
    RINOK(outStream->Write(buf, sizeof(buf), NULL));
    */
    // we need real outArchive.Pos, if outSeekStream->SetSize() will be used.
    RINOK(outSeekStream->Seek(0, STREAM_SEEK_CUR, &outArchive.Pos))
  }
  if (setRestriction)
    RINOK(setRestriction->SetRestriction(0, 0))

  UInt64 complexity = 0;

  unsigned i;
  for (i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
    {
      if (ui.Size == (UInt64)(Int64)-1)
        break;
      complexity += ui.Size;
    }
    else
      complexity += inputItems[(unsigned)ui.IndexInArc].Get_FullSize_Aligned();
  }

  if (i == updateItems.Size())
    RINOK(updateCallback->SetTotal(complexity))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStreamLimited;
  inStreamLimited->SetStream(inStream);

  complexity = 0;

  // const int kNumReduceDigits = -1; // for debug

  for (i = 0;; i++)
  {
    lps->InSize = lps->OutSize = complexity;
    RINOK(lps->SetCur())

    if (i == updateItems.Size())
    {
      if (outSeekStream && setRestriction)
        RINOK(setRestriction->SetRestriction(0, 0))
      return outArchive.WriteFinishHeader();
    }

    const CUpdateItem &ui = updateItems[i];
    CItem item;
  
    if (ui.NewProps)
    {
      item.SetMagic_Posix(options.PosixMode);
      item.Name = ui.Name;
      item.User = ui.User;
      item.Group = ui.Group;
      item.UID = ui.UID;
      item.GID = ui.GID;
      item.DeviceMajor = ui.DeviceMajor;
      item.DeviceMinor = ui.DeviceMinor;
      item.DeviceMajor_Defined = ui.DeviceMajor_Defined;
      item.DeviceMinor_Defined = ui.DeviceMinor_Defined;
      
      if (ui.IsDir)
      {
        item.LinkFlag = NFileHeader::NLinkFlag::kDirectory;
        item.PackSize = 0;
      }
      else
      {
        item.PackSize = ui.Size;
        item.Set_LinkFlag_for_File(ui.Mode);
      }

      // 22.00
      item.Mode = ui.Mode & ~(UInt32)MY_LIN_S_IFMT;
      item.PaxTimes = ui.PaxTimes;
      // item.PaxTimes.ReducePrecison(kNumReduceDigits); // for debug
      item.MTime = ui.PaxTimes.MTime.GetSec();
    }
    else
      item = inputItems[(unsigned)ui.IndexInArc];

    AString symLink;
    if (ui.NewData || ui.NewProps)
    {
      RINOK(GetPropString(updateCallback, ui.IndexInClient, kpidSymLink, symLink,
          options.CodePage, options.UtfFlags, true))
      if (!symLink.IsEmpty())
      {
        item.LinkFlag = NFileHeader::NLinkFlag::kSymLink;
        item.LinkName = symLink;
      }
    }

    if (ui.NewData)
    {
      item.SparseBlocks.Clear();
      item.PackSize = ui.Size;
      item.Size = ui.Size;
#if 0
      if (ui.Size == (UInt64)(Int64)-1)
        return E_INVALIDARG;
#endif
      CMyComPtr<ISequentialInStream> fileInStream;

      bool needWrite = true;
      
      if (!symLink.IsEmpty())
      {
        item.PackSize = 0;
        item.Size = 0;
      }
      else
      {
        const HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        
        if (res == S_FALSE)
          needWrite = false;
        else
        {
          RINOK(res)
          
          if (!fileInStream)
          {
            item.PackSize = 0;
            item.Size = 0;
          }
          else
          {
            Z7_DECL_CMyComPtr_QI_FROM(IStreamGetProp, getProp, fileInStream)
            if (getProp)
            {
              if (options.Write_MTime.Val) RINOK(GetTime(getProp, kpidMTime, item.PaxTimes.MTime))
              if (options.Write_ATime.Val) RINOK(GetTime(getProp, kpidATime, item.PaxTimes.ATime))
              if (options.Write_CTime.Val) RINOK(GetTime(getProp, kpidCTime, item.PaxTimes.CTime))

              if (options.PosixMode)
              {
                /*
                RINOK(GetDevice(getProp, item.DeviceMajor, item.DeviceMinor,
                    item.DeviceMajor_Defined, item.DeviceMinor_Defined));
                */
                bool defined = false;
                UInt32 val = 0;
                RINOK(GetDevice(getProp, kpidDeviceMajor, val, defined))
                if (defined)
                {
                  item.DeviceMajor = val;
                  item.DeviceMajor_Defined = true;
                  item.DeviceMinor = 0;
                  item.DeviceMinor_Defined = false;
                  RINOK(GetDevice(getProp, kpidDeviceMinor, item.DeviceMinor, item.DeviceMinor_Defined))
                }
              }

              RINOK(GetUser(getProp, kpidUser,  kpidUserId,  item.User,  item.UID, options.CodePage, options.UtfFlags))
              RINOK(GetUser(getProp, kpidGroup, kpidGroupId, item.Group, item.GID, options.CodePage, options.UtfFlags))

              {
                NWindows::NCOM::CPropVariant prop;
                RINOK(getProp->GetProperty(kpidPosixAttrib, &prop))
                if (prop.vt == VT_EMPTY)
                  item.Mode =
                    MY_LIN_S_IRWXO
                  | MY_LIN_S_IRWXG
                  | MY_LIN_S_IRWXU
                  | (ui.IsDir ? MY_LIN_S_IFDIR : MY_LIN_S_IFREG);
                else if (prop.vt != VT_UI4)
                  return E_INVALIDARG;
                else
                  item.Mode = prop.ulVal;
                // 21.07 : we clear high file type bits as GNU TAR.
                item.Set_LinkFlag_for_File(item.Mode);
                item.Mode &= ~(UInt32)MY_LIN_S_IFMT;
              }

              {
                NWindows::NCOM::CPropVariant prop;
                RINOK(getProp->GetProperty(kpidSize, &prop))
                if (prop.vt != VT_UI8)
                  return E_INVALIDARG;
                const UInt64 size = prop.uhVal.QuadPart;
                // printf("\nTAR after GetProperty(kpidSize size = %8d\n", (unsigned)size);
                item.PackSize = size;
                item.Size = size;
              }
              /*
              printf("\nNum digits = %d %d\n",
                  (int)item.PaxTimes.MTime.NumDigits,
                  (int)item.PaxTimes.MTime.Ns);
              */
            }
            else
            {
              Z7_DECL_CMyComPtr_QI_FROM(IStreamGetProps, getProps, fileInStream)
              if (getProps)
              {
                FILETIME mTime, aTime, cTime;
                UInt64 size2;
                if (getProps->GetProps(&size2,
                    options.Write_CTime.Val ? &cTime : NULL,
                    options.Write_ATime.Val ? &aTime : NULL,
                    options.Write_MTime.Val ? &mTime : NULL,
                    NULL) == S_OK)
                {
                  item.PackSize = size2;
                  item.Size = size2;
                  if (options.Write_MTime.Val) FILETIME_To_PaxTime(mTime, item.PaxTimes.MTime);
                  if (options.Write_ATime.Val) FILETIME_To_PaxTime(aTime, item.PaxTimes.ATime);
                  if (options.Write_CTime.Val) FILETIME_To_PaxTime(cTime, item.PaxTimes.CTime);
                }
              }
            }
          }

          {
            // we must request kpidHardLink after updateCallback->GetStream()
            AString hardLink;
            RINOK(GetPropString(updateCallback, ui.IndexInClient, kpidHardLink, hardLink,
                options.CodePage, options.UtfFlags, true))
            if (!hardLink.IsEmpty())
            {
              item.LinkFlag = NFileHeader::NLinkFlag::kHardLink;
              item.LinkName = hardLink;
              item.PackSize = 0;
              item.Size = 0;
              fileInStream.Release();
            }
          }
        }
      }

      // item.PaxTimes.ReducePrecison(kNumReduceDigits); // for debug

      if (ui.NewProps)
        item.MTime = item.PaxTimes.MTime.GetSec();

      if (needWrite)
      {
        if (fileInStream)
        // if (item.PackSize == (UInt64)(Int64)-1)
        if (item.Size == (UInt64)(Int64)-1)
          return E_INVALIDARG;

        const UInt64 headerPos = outArchive.Pos;
        // item.PackSize = ((UInt64)1 << 33); // for debug

        if (outSeekStream && setRestriction)
          RINOK(setRestriction->SetRestriction(outArchive.Pos, (UInt64)(Int64)-1))

        RINOK(outArchive.WriteHeader(item))
        if (fileInStream)
        {
          for (unsigned numPasses = 0;; numPasses++)
          {
            // printf("\nTAR numPasses = %d" " old size = %8d\n", numPasses, (unsigned)item.PackSize);
            /* we support 2 attempts to write header:
                pass-0: main pass:
                pass-1: additional pass, if size_of_file and size_of_header are changed */
            if (numPasses >= 2)
            {
              // opRes = NArchive::NUpdate::NOperationResult::kError_FileChanged;
              // break;
              return E_FAIL;
            }
            
            const UInt64 dataPos = outArchive.Pos;
            RINOK(copyCoder.Interface()->Code(fileInStream, outStream, NULL, NULL, lps))
            outArchive.Pos += copyCoder->TotalSize;
            RINOK(outArchive.Write_AfterDataResidual(copyCoder->TotalSize))
            // printf("\nTAR after Code old size = %8d copyCoder->TotalSize = %8d \n", (unsigned)item.PackSize, (unsigned)copyCoder->TotalSize);
            // if (numPasses >= 10) // for debug
            if (copyCoder->TotalSize == item.PackSize)
              break;
            
            if (opCallback)
            {
              RINOK(opCallback->ReportOperation(
                  NEventIndexType::kOutArcIndex, (UInt32)ui.IndexInClient,
                  NUpdateNotifyOp::kInFileChanged))
            }
            
            if (!outSeekStream)
              return E_FAIL;
            const UInt64 nextPos = outArchive.Pos;
            RINOK(outSeekStream->Seek(-(Int64)(nextPos - headerPos), STREAM_SEEK_CUR, NULL))
            outArchive.Pos = headerPos;
            item.PackSize = copyCoder->TotalSize;
            
            RINOK(outArchive.WriteHeader(item))
            
            // if (numPasses >= 10) // for debug
            if (outArchive.Pos == dataPos)
            {
              const UInt64 alignedSize = nextPos - dataPos;
              if (alignedSize != 0)
              {
                RINOK(outSeekStream->Seek((Int64)alignedSize, STREAM_SEEK_CUR, NULL))
                outArchive.Pos += alignedSize;
              }
              break;
            }
            
            // size of header was changed.
            // we remove data after header and try new attempt, if required
            Z7_DECL_CMyComPtr_QI_FROM(IInStream, fileSeekStream, fileInStream)
            if (!fileSeekStream)
              return E_FAIL;
            RINOK(InStream_SeekToBegin(fileSeekStream))
            RINOK(outSeekStream->SetSize(outArchive.Pos))
            if (item.PackSize == 0)
              break;
          }
        }
      }
      
      complexity += item.PackSize;
      fileInStream.Release();
      RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
    }
    else
    {
      // (ui.NewData == false)

      if (opCallback)
      {
        RINOK(opCallback->ReportOperation(
            NEventIndexType::kInArcIndex, (UInt32)ui.IndexInArc,
            NUpdateNotifyOp::kReplicate))
      }

      const CItemEx &existItem = inputItems[(unsigned)ui.IndexInArc];
      UInt64 size, pos;
      
      if (ui.NewProps)
      {
        // memcpy(item.Magic, NFileHeader::NMagic::kEmpty, 8);
        
        if (!symLink.IsEmpty())
        {
          item.PackSize = 0;
          item.Size = 0;
        }
        else
        {
          if (ui.IsDir == existItem.IsDir())
            item.LinkFlag = existItem.LinkFlag;
        
          item.SparseBlocks = existItem.SparseBlocks;
          item.Size = existItem.Size;
          item.PackSize = existItem.PackSize;
        }
        
        item.DeviceMajor_Defined = existItem.DeviceMajor_Defined;
        item.DeviceMinor_Defined = existItem.DeviceMinor_Defined;
        item.DeviceMajor = existItem.DeviceMajor;
        item.DeviceMinor = existItem.DeviceMinor;
        item.UID = existItem.UID;
        item.GID = existItem.GID;
        
        RINOK(outArchive.WriteHeader(item))
        size = existItem.Get_PackSize_Aligned();
        pos = existItem.Get_DataPos();
      }
      else
      {
        size = existItem.Get_FullSize_Aligned();
        pos = existItem.HeaderPos;
      }

      if (size != 0)
      {
        RINOK(InStream_SeekSet(inStream, pos))
        inStreamLimited->Init(size);
        if (outSeekStream && setRestriction)
          RINOK(setRestriction->SetRestriction(0, 0))
        // 22.00 : we copy Residual data from old archive to new archive instead of zeroing
        RINOK(copyCoder.Interface()->Code(inStreamLimited, outStream, NULL, NULL, lps))
        if (copyCoder->TotalSize != size)
          return E_FAIL;
        outArchive.Pos += size;
        // RINOK(outArchive.Write_AfterDataResidual(existItem.PackSize));
        complexity += size;
      }
    }
  }
}

}}
