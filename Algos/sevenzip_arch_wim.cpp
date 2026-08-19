/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Archive/wim reader.
 *
 * 4 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Archive/Wim/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- CPP/Common/AutoPtr.h ---- */
// Common/AutoPtr.h

#ifndef ZIP7_INC_COMMON_AUTOPTR_H
#define ZIP7_INC_COMMON_AUTOPTR_H

template<class T> class CMyUniquePtr
// CMyAutoPtr
{
  T *_p;
  
  CMyUniquePtr(CMyUniquePtr<T>& p); // : _p(p.release()) {}
  CMyUniquePtr<T>& operator=(T *p);
  CMyUniquePtr<T>& operator=(CMyUniquePtr<T>& p);
  /*
  {
    reset(p.release());
    return (*this);
  }
  */
  void reset(T* p = NULL)
  {
    if (p != _p)
      delete _p;
    _p = p;
  }
public:
  CMyUniquePtr(T *p = NULL) : _p(p) {}
  ~CMyUniquePtr() { delete _p; }
  T& operator*() const { return *_p; }
  T* operator->() const { return _p; }
  // operator bool() const { return _p != NULL; }
  T* get() const { return _p; }
  T* release()
  {
    T *tmp = _p;
    _p = NULL;
    return tmp;
  }
  void Create_if_Empty()
  {
    if (!_p)
      _p = new T;
  }
};

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

/* ---- CPP/Common/MyXml.h ---- */
// MyXml.h

#ifndef ZIP7_INC_MY_XML_H
#define ZIP7_INC_MY_XML_H

// amalgamation: header emitted in prologue

struct CXmlProp
{
  AString Name;
  AString Value;
};

class CXmlItem
{
public:
  AString Name;
  bool IsTag;
  CObjectVector<CXmlProp> Props;
  CObjectVector<CXmlItem> SubItems;
  
  const char * ParseItem(const char *s, int numAllowedLevels);

  bool IsTagged(const char *tag) const throw();
  int FindProp(const char *propName) const throw();
  AString GetPropVal(const char *propName) const;
  AString GetSubString() const;
  const AString * GetSubStringPtr() const throw();
  int FindSubTag(const char *tag) const throw();
  const CXmlItem *FindSubTag_GetPtr(const char *tag) const throw();
  AString GetSubStringForTag(const char *tag) const;
  void AppendTo(AString &s) const;
};

struct CXml
{
  CXmlItem Root;

  bool Parse(const char *s);
  // void AppendTo(AString &s) const;
};

void z7_xml_DecodeString(AString &s);

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

/* ---- CPP/7zip/Compress/HuffmanDecoder.h ---- */
// Compress/HuffmanDecoder.h

#ifndef ZIP7_INC_COMPRESS_HUFFMAN_DECODER_H
#define ZIP7_INC_COMPRESS_HUFFMAN_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NHuffman {

// const unsigned kNumTableBits_Default = 9;

#if 0 || 0 && defined(MY_CPU_64BIT)
// for debug or optimization:
// 64-BIT limit array can be faster for some compilers.
// for debug or optimization:
#define Z7_HUFF_USE_64BIT_LIMIT
#else
// sizet value variable allows to eliminate some move operation in some compilers.
// for debug or optimization:
// #define Z7_HUFF_USE_SIZET_VALUE
#endif

// v0 must normalized to (32 bits) : (v0 < ((UInt64)1 << 32))

#ifdef Z7_HUFF_USE_64BIT_LIMIT
  typedef UInt64 CLimitInt;
  typedef UInt64 CValueInt;
  // all _limits[*] are normalized and limited by ((UInt64)1 << 32).
  // we don't use (v1) in this branch
  #define Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax)  32
  #define Z7_HUFF_TABLE_COMPARE(huf, kNumTableBits, v0, v1) \
      ((NCompress::NHuffman::CLimitInt)v0 >= (huf)->_limits[0])
  #define Z7_HUFF_GET_VAL_FOR_LIMITS(v0, v1, kNumBitsMax, kNumTableBits)  (v0)
  #define Z7_HUFF_GET_VAL_FOR_TABLE( v0, v1, kNumBitsMax, kNumTableBits)  ((v0) >> (32 - kNumTableBits))
  #define Z7_HUFF_PRECALC_V1(kNumTableBits, v0, v1)
#else
  typedef UInt32 CLimitInt;
  typedef
    #ifdef Z7_HUFF_USE_SIZET_VALUE
      size_t
    #else
      UInt32
    #endif
      CValueInt;
  // v1 must be precalculated from v0 in this branch
  // _limits[0] and (v1) are normalized and limited by (1 << kNumTableBits).
  // _limits[non_0]      are normalized and limited by (1 << kNumBitsMax).
  #define Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax)  (kNumBitsMax)
  #define Z7_HUFF_TABLE_COMPARE(huf, kNumTableBits, v0, v1) \
      ((NCompress::NHuffman::CLimitInt)v1 >= (huf)->_limits[0])
  #define Z7_HUFF_GET_VAL_FOR_LIMITS(v0, v1, kNumBitsMax, kNumTableBits)  ((v0) >> (32 - kNumBitsMax))
  #define Z7_HUFF_GET_VAL_FOR_TABLE( v0, v1, kNumBitsMax, kNumTableBits)  (v1)
  #define Z7_HUFF_PRECALC_V1(kNumTableBits, v0, v1)  const UInt32 v1 = ((v0) >> (32 - kNumTableBits));
#endif


enum enum_BuildMode
{
  k_BuildMode_Partial       = 0,
  k_BuildMode_Full          = 1,
  k_BuildMode_Full_or_Empty = 2
};


template <class symType, class symType2, class symType4, unsigned kNumBitsMax, unsigned m_NumSymbols, unsigned kNumTableBits /* = kNumTableBits_Default */>
struct CDecoderBase
{
  CLimitInt _limits[kNumBitsMax + 2 - kNumTableBits];
  UInt32 _poses[kNumBitsMax - kNumTableBits]; // unsigned
union
{
  // if  defined(MY_CPU_64BIT), we need 64-bit alignment for _symbols.
  // if !defined(MY_CPU_64BIT), we need 32-bit alignment for _symbols
  // but we provide alignment for _lens.
  // _symbols also will be aligned, if _lens are aligned
  #if defined(MY_CPU_64BIT)
    UInt64
  #else
    UInt32
  #endif
    _pad_align[m_NumSymbols < (1u << sizeof(symType) * 8) ? 1 : -1];
  /* if symType is Byte, we use 16-bytes padding to avoid cache
     bank conflict between _lens and _symbols: */
  Byte _lens[(1 << kNumTableBits) + (sizeof(symType) == 1 ? 16 : 0)];
} _u;
  symType _symbols[(1 << kNumTableBits) + m_NumSymbols - (kNumTableBits + 1)];

  /*
  Z7_FORCE_INLINE
  bool IsFull() const
  {
    return _limits[kNumBitsMax - kNumTableBits] ==
        (CLimitInt)1u << Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax);
  }
  Z7_FORCE_INLINE
  bool IsEmpty() const
  {
    return _limits[kNumBitsMax - kNumTableBits] == 0;
  }
  Z7_FORCE_INLINE
  bool Is_Full_or_Empty() const
  {
    return 0 == (_limits[kNumBitsMax - kNumTableBits] &
        ~((CLimitInt)1 << Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax)));
  }
  */

  Z7_FORCE_INLINE
  bool Build(const Byte *lens, enum_BuildMode buidMode = k_BuildMode_Partial) throw()
  {
    unsigned counts[kNumBitsMax + 1];
    size_t i;
    for (i = 0; i <= kNumBitsMax; i++)
      counts[i] = 0;
    for (i = 0; i < m_NumSymbols; i++)
      counts[lens[i]]++;
    
    UInt32 sum = 0;
    for (i = 1; i <= kNumTableBits; i++)
    {
      sum <<= 1;
      sum += counts[i];
    }

    CLimitInt startPos = (CLimitInt)sum;
    _limits[0] =
      #ifdef Z7_HUFF_USE_64BIT_LIMIT
          startPos << (Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - kNumTableBits);
      #else
          startPos;
      #endif

    for (i = kNumTableBits + 1; i <= kNumBitsMax; i++)
    {
      startPos <<= 1;
      _poses[i - (kNumTableBits + 1)] = (UInt32)(startPos - sum);
      const unsigned cnt = counts[i];
      counts[i] = sum;
      sum += cnt;
      startPos += cnt;
      _limits[i - kNumTableBits] = startPos << (Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - i);
    }

    _limits[kNumBitsMax + 1 - kNumTableBits] =
        (CLimitInt)1 << Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax);

    if (buidMode == k_BuildMode_Partial)
    {
      if (startPos > (1u << kNumBitsMax)) return false;
    }
    else
    {
      if (buidMode != k_BuildMode_Full && startPos == 0) return true;
      if (startPos != (1u << kNumBitsMax)) return false;
    }
    size_t sum2 = 0;
    for (i = 1; i <= kNumTableBits; i++)
    {
      const unsigned cnt = counts[i] << (kNumTableBits - i);
      counts[i] = (unsigned)sum2 >> (kNumTableBits - i);
      memset(_u._lens + sum2, (int)i, cnt);
      sum2 += cnt;
    }
    
#ifdef MY_CPU_64BIT
    symType4
    // UInt64 // for symType = UInt16
    // UInt32 // for symType = Byte
#else
    UInt32
#endif
    v = 0;
    for (i = 0; i < m_NumSymbols; i++,
      v +=
          1
          + (  (UInt32)1 << (sizeof(symType) * 8 * 1))
          // 0x00010001 // for symType = UInt16
          // 0x00000101 // for symType = Byte
#ifdef MY_CPU_64BIT
          + ((symType4)1 << (sizeof(symType) * 8 * 2))
          + ((symType4)1 << (sizeof(symType) * 8 * 3))
          // 0x0001000100010001 // for symType = UInt16
          // 0x0000000001010101 // for symType = Byte
#endif
      )
    {
      const unsigned len = lens[i];
      if (len == 0)
        continue;
      const size_t offset = counts[len];
      counts[len] = (unsigned)offset + 1;
      if (len >= kNumTableBits)
        _symbols[offset] = (symType)v;
      else
      {
        Byte *s2 = (Byte *)(void *)_symbols + (offset <<
            (kNumTableBits + sizeof(symType) / 2 - len));
        Byte *lim = s2 + ((size_t)1 <<
            (kNumTableBits + sizeof(symType) / 2 - len));
        if (len >= kNumTableBits - 2)
        {
          *(symType2 *)(void *)(s2                       ) = (symType2)v;
          *(symType2 *)(void *)(lim - sizeof(symType) * 2) = (symType2)v;
        }
        else
        {
#ifdef MY_CPU_64BIT
          symType4 *s = (symType4 *)(void *)s2;
          do
          {
            s[0] = v;  s[1] = v;  s += 2;
          }
          while (s != (const symType4 *)(const void *)lim);
#else
          symType2 *s = (symType2 *)(void *)s2;
          do
          {
            s[0] = (symType2)v;  s[1] = (symType2)v;  s += 2;
            s[0] = (symType2)v;  s[1] = (symType2)v;  s += 2;
          }
          while (s != (const symType2 *)(const void *)lim);
#endif
        }
      }
    }
    return true;
  }


#define Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES(_numBits_, kNumBitsMax, error_op)  if (_numBits_ > kNumBitsMax) { error_op }
#define Z7_HUFF_DECODE_ERROR_SYM_CHECK_NO( _numBits_, kNumBitsMax, error_op)

  
#define Z7_HUFF_DECODE_BASE_TREE_BRANCH(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      get_val_for_limits, \
      check_op, error_op, _numBits_) \
{ \
    const NHuffman::CValueInt _val = get_val_for_limits(v0, v1, kNumBitsMax, kNumTableBits); \
    _numBits_ = kNumTableBits + 1; \
    if ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[1]) \
    do { _numBits_++; } \
    while ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[_numBits_ - kNumTableBits]); \
    check_op(_numBits_, kNumBitsMax, error_op) \
    sym = (huf)->_symbols[(/* (UInt32) */ (_val >> ((Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - (unsigned)_numBits_)))) \
        - (huf)->_poses[_numBits_ - (kNumTableBits + 1)]]; \
}

/*
    Z7_HUFF_DECODE_BASE_TREE_BRANCH(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      get_val_for_limits, \
      check_op, error_op, _numBits_) \

*/

#define Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      get_val_for_table, get_val_for_limits, \
      check_op, error_op, move_pos_op, after_op, bs) \
{ \
  if (Z7_HUFF_TABLE_COMPARE(huf, kNumTableBits, v0, v1)) \
  { \
    const NHuffman::CValueInt _val = get_val_for_limits(v0, v1, kNumBitsMax, kNumTableBits); \
    size_t _numBits_ = kNumTableBits + 1; \
    if ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[1]) \
    do { _numBits_++; } \
    while ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[_numBits_ - kNumTableBits]); \
    check_op(_numBits_, kNumBitsMax, error_op) \
    sym = (huf)->_symbols[(/* (UInt32) */ (_val >> ((Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - (unsigned)_numBits_)))) \
        - (huf)->_poses[_numBits_ - (kNumTableBits + 1)]]; \
    move_pos_op(bs, _numBits_); \
  } \
  else \
  { \
    const size_t _val = get_val_for_table(v0, v1, kNumBitsMax, kNumTableBits); \
    const size_t _numBits_ = (huf)->_u._lens[_val]; \
    sym = (huf)->_symbols[_val]; \
    move_pos_op(bs, _numBits_); \
  } \
  after_op \
}

#define Z7_HUFF_DECODE_10(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      check_op, error_op, move_pos_op, after_op, bs) \
    Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      Z7_HUFF_GET_VAL_FOR_TABLE, \
      Z7_HUFF_GET_VAL_FOR_LIMITS, \
      check_op, error_op, move_pos_op, after_op, bs) \


#define Z7_HUFF_DECODE_VAL_IN_HIGH32(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, \
      check_op, error_op, move_pos_op, after_op, bs) \
{ \
    Z7_HUFF_PRECALC_V1(kNumTableBits, v0, _v1_temp) \
    Z7_HUFF_DECODE_10(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, _v1_temp, \
      check_op, error_op, move_pos_op, after_op, bs) \
}

#if 0 || defined(Z7_HUFF_USE_64BIT_LIMIT)
// this branch uses bitStream->GetValue_InHigh32bits().
#define Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, \
      check_op, error_op, move_pos_op) \
{ \
  const UInt32 v0 = (bitStream)->GetValue_InHigh32bits(); \
  Z7_HUFF_PRECALC_V1(kNumTableBits, v0, v1); \
  Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits, \
      v0, v1,  \
      Z7_HUFF_GET_VAL_FOR_TABLE, \
      Z7_HUFF_GET_VAL_FOR_LIMITS, \
       check_op, error_op, move_pos_op, {}, bitStream) \
}
#else
/*
this branch uses bitStream->GetValue().
So we use SIMPLE versions for v0, v1 calculation:
  v0 is normalized for kNumBitsMax
  v1 is normalized for kNumTableBits
*/
#define Z7_HUFF_GET_VAL_FOR_LIMITS_SIMPLE(v0, v1, kNumBitsMax, kNumTableBits)  v0
#define Z7_HUFF_GET_VAL_FOR_TABLE_SIMPLE( v0, v1, kNumBitsMax, kNumTableBits)  v1
#define Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op, move_pos_op) \
{ \
  const UInt32 v0 = (bitStream)->GetValue(kNumBitsMax); \
  const UInt32 v1 = v0 >> (kNumBitsMax - kNumTableBits); \
  Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits, \
      v0, v1,  \
      Z7_HUFF_GET_VAL_FOR_TABLE_SIMPLE, \
      Z7_HUFF_GET_VAL_FOR_LIMITS_SIMPLE, \
      check_op, error_op, move_pos_op, {}, bitStream) \
}
#endif

#define Z7_HUFF_bitStream_MovePos(bitStream, numBits)  (bitStream)->MovePos((unsigned)(numBits))

#define Z7_HUFF_DECODE_1(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op) \
        Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op, \
          Z7_HUFF_bitStream_MovePos)

// MovePosCheck

#define Z7_HUFF_DECODE_2(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op) \
        Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op, \
          Z7_HUFF_bitStream_MovePos)

// MovePosCheck

#define Z7_HUFF_DECODE_CHECK(sym, huf, kNumBitsMax, kNumTableBits, bitStream, error_op) \
        Z7_HUFF_DECODE_1(    sym, huf, kNumBitsMax, kNumTableBits, bitStream, \
        Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES, error_op)

  template <class TBitDecoder>
  Z7_FORCE_INLINE
  bool Decode2(TBitDecoder *bitStream, unsigned &sym) const
  {
    Z7_HUFF_DECODE_CHECK(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      { return false; }
    )
    return true;
  }

  template <class TBitDecoder>
  Z7_FORCE_INLINE
  bool Decode_SymCheck_MovePosCheck(TBitDecoder *bitStream, unsigned &sym) const
  {
    Z7_HUFF_DECODE_0(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES,
      { return false; },
      { return (bitStream)->MovePosCheck; }
    )
  }

  template <class TBitDecoder>
  Z7_FORCE_INLINE
  unsigned Decode(TBitDecoder *bitStream) const
  {
    unsigned sym;
    Z7_HUFF_DECODE_CHECK(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      { return (unsigned)(int)(Int32)0xffffffff; }
    )
    return sym;
  }

  
  template <class TBitDecoder>
  Z7_FORCE_INLINE
  unsigned DecodeFull(TBitDecoder *bitStream) const
  {
    /*
    const UInt32 val = bitStream->GetValue(kNumBitsMax);
    if (val < _limits[kNumTableBits])
    {
      const unsigned pair = _u._lens[(size_t)(val >> (kNumBitsMax - kNumTableBits))];
      bitStream->MovePos(pair & kPairLenMask);
      return pair >> kNumPairLenBits;
    }

    unsigned numBits;
    for (numBits = kNumTableBits + 1; val >= _limits[numBits]; numBits++);
    
    bitStream->MovePos(numBits);
    return _symbols[_poses[numBits] + (unsigned)
        ((val - _limits[(size_t)numBits - 1]) >> (kNumBitsMax - numBits))];
    */
    unsigned sym;
    Z7_HUFF_DECODE_2(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      Z7_HUFF_DECODE_ERROR_SYM_CHECK_NO, {}
    )
    return sym;
  }
};


template <unsigned kNumBitsMax, unsigned m_NumSymbols, unsigned kNumTableBits /* = kNumTableBits_Default */>
struct CDecoder: public CDecoderBase
  <UInt16, UInt32, UInt64, kNumBitsMax, m_NumSymbols, kNumTableBits> {};

template <unsigned kNumBitsMax, unsigned m_NumSymbols, unsigned kNumTableBits /* = 7 */>
struct CDecoder256: public CDecoderBase
  <Byte, UInt16, UInt32, kNumBitsMax, m_NumSymbols, kNumTableBits> {};


template <unsigned numSymbols>
class CDecoder7b
{
public:
  Byte _lens[1 << 7];

  bool Build(const Byte *lens, bool full) throw()
  {
    const unsigned kNumBitsMax = 7;
    
    unsigned counts[kNumBitsMax + 1];
    unsigned _poses[kNumBitsMax + 1];
    unsigned _limits[kNumBitsMax + 1];
    unsigned i;
    for (i = 0; i <= kNumBitsMax; i++)
      counts[i] = 0;
    for (i = 0; i < numSymbols; i++)
      counts[lens[i]]++;
    
    _limits[0] = 0;
    const unsigned kMaxValue = 1u << kNumBitsMax;
    unsigned startPos = 0;
    unsigned sum = 0;
    
    for (i = 1; i <= kNumBitsMax; i++)
    {
      const unsigned cnt = counts[i];
      startPos += cnt << (kNumBitsMax - i);
      _limits[i] = startPos;
      counts[i] = sum;
      _poses[i] = sum;
      sum += cnt;
    }

    counts[0] = sum;
    _poses[0] = sum;

    if (full)
    {
      if (startPos != kMaxValue)
        return false;
    }
    else
    {
      if (startPos > kMaxValue)
        return false;
    }


    for (i = 0; i < numSymbols; i++)
    {
      const unsigned len = lens[i];
      if (len == 0)
        continue;
      const unsigned offset = counts[len]++;
      {
        Byte *dest = _lens + _limits[(size_t)len - 1]
            + ((offset - _poses[len]) << (kNumBitsMax - len));
        const unsigned num = (unsigned)1 << (kNumBitsMax - len);
        const unsigned val = (i << 3) + len;
        for (unsigned k = 0; k < num; k++)
          dest[k] = (Byte)val;
      }
    }

    if (!full)
    {
      const unsigned limit = _limits[kNumBitsMax];
      const unsigned num = ((unsigned)1 << kNumBitsMax) - limit;
      Byte *dest = _lens + limit;
      for (unsigned k = 0; k < num; k++)
        dest[k] = (Byte)
          // (0x1f << 3);
          ((0x1f << 3) + 0x7);
    }
    
    return true;
  }

#define Z7_HUFF_DECODER_7B_DECODE(dest, huf, get_val, move_pos, bs) \
  { \
    const unsigned pair = huf->_lens[(size_t)get_val(7)]; \
    const unsigned numBits = pair & 0x7; \
    move_pos(bs, numBits); \
    dest = pair >> 3; \
  }

  template <class TBitDecoder>
  unsigned Decode(TBitDecoder *bitStream) const
  {
    const unsigned pair = _lens[(size_t)bitStream->GetValue(7)];
    bitStream->MovePos(pair & 0x7);
    return pair >> 3;
  }
};

}}

#endif

/* ---- CPP/7zip/Compress/LzmsDecoder.h ---- */
// LzmsDecoder.h
// The code is based on LZMS description from wimlib code

#ifndef ZIP7_INC_LZMS_DECODER_H
#define ZIP7_INC_LZMS_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzms {

const unsigned k_NumLitSyms = 256;
const unsigned k_NumLenSyms = 54;
const unsigned k_NumPosSyms = 799;
const unsigned k_NumPowerSyms = 8;

const unsigned k_NumProbBits = 6;
const unsigned k_ProbLimit = 1 << k_NumProbBits;
const unsigned k_InitialProb = 48;
const UInt32 k_InitialHist = 0x55555555;

const unsigned k_NumReps = 3;

const unsigned k_NumMainProbs  = 16;
const unsigned k_NumMatchProbs = 32;
const unsigned k_NumRepProbs   = 64;

const unsigned k_NumHuffmanBits = 15;

template <UInt32 m_NumSyms, UInt32 m_RebuildFreq, unsigned numTableBits>
class CHuffDecoder: public NCompress::NHuffman::CDecoder<k_NumHuffmanBits, m_NumSyms, numTableBits>
{
public:
  UInt32 RebuildRem;
  UInt32 NumSyms;
  UInt32 Freqs[m_NumSyms];

  void Generate() throw()
  {
    UInt32 vals[m_NumSyms];
    Byte levels[m_NumSyms];

    // We need to check that our algorithm is OK, when optimal Huffman tree uses more than 15 levels !!!
    Huffman_Generate(Freqs, vals, levels, NumSyms, k_NumHuffmanBits);

    for (UInt32 i = NumSyms; i < m_NumSyms; i++)
      levels[i] = 0;

    this->Build(levels, /* NumSyms, */ NHuffman::k_BuildMode_Full);
  }
  
  void Rebuild() throw()
  {
    Generate();
    RebuildRem = m_RebuildFreq;
    const UInt32 num = NumSyms;
    for (UInt32 i = 0; i < num; i++)
      Freqs[i] = (Freqs[i] >> 1) + 1;
  }

public:
  void Init(UInt32 numSyms = m_NumSyms) throw()
  {
    RebuildRem = m_RebuildFreq;
    NumSyms = numSyms;
    for (UInt32 i = 0; i < numSyms; i++)
      Freqs[i] = 1;
    // for (; i < m_NumSyms; i++) Freqs[i] = 0;
    Generate();
  }
};


struct CProbEntry
{
  UInt32 Prob;
  UInt64 Hist;

  void Init()
  {
    Prob = k_InitialProb;
    Hist = k_InitialHist;
  }

  UInt32 GetProb() const throw()
  {
    UInt32 prob = Prob;
    if (prob == 0)
      prob = 1;
    else if (prob == k_ProbLimit)
      prob = k_ProbLimit - 1;
    return prob;
  }

  void Update(unsigned bit) throw()
  {
    Prob += (UInt32)((Int32)(Hist >> (k_ProbLimit - 1)) - (Int32)bit);
    Hist = (Hist << 1) | bit;
  }
};


struct CRangeDecoder
{
  UInt32 range;
  UInt32 code;
  const Byte *cur;
  // const Byte *end;

  void Init(const Byte *data, size_t /* size */) throw()
  {
    range = 0xFFFFFFFF;
    code = (((UInt32)GetUi16(data)) << 16) | GetUi16(data + 2);
    cur = data + 4;
    // end = data + size;
  }

  void Normalize()
  {
    if (range <= 0xFFFF)
    {
      range <<= 16;
      code <<= 16;
      // if (cur >= end) throw 1;
      code |= GetUi16(cur);
      cur += 2;
    }
  }

  unsigned Decode(UInt32 *state, UInt32 numStates, struct CProbEntry *probs)
  {
    UInt32 st = *state;
    CProbEntry *entry = &probs[st];
    st = (st << 1) & (numStates - 1);

    const UInt32 prob = entry->GetProb();

    if (range <= 0xFFFF)
    {
      range <<= 16;
      code <<= 16;
      // if (cur >= end) throw 1;
      code |= GetUi16(cur);
      cur += 2;
    }

    const UInt32 bound = (range >> k_NumProbBits) * prob;
    
    if (code < bound)
    {
      range = bound;
      *state = st;
      entry->Update(0);
      return 0;
    }
    else
    {
      range -= bound;
      code -= bound;
      *state = st | 1;
      entry->Update(1);
      return 1;
    }
  }
};


class CDecoder
{
  // CRangeDecoder _rc;
  size_t _pos;

  UInt32 _reps[k_NumReps + 1];
  UInt64 _deltaReps[k_NumReps + 1];

  UInt32 mainState;
  UInt32 matchState;
  UInt32 lzRepStates[k_NumReps];
  UInt32 deltaRepStates[k_NumReps];

  struct CProbEntry mainProbs[k_NumMainProbs];
  struct CProbEntry matchProbs[k_NumMatchProbs];
  
  struct CProbEntry lzRepProbs[k_NumReps][k_NumRepProbs];
  struct CProbEntry deltaRepProbs[k_NumReps][k_NumRepProbs];

  CHuffDecoder<k_NumLitSyms, 1024, 9> m_LitDecoder;
  CHuffDecoder<k_NumPosSyms, 1024, 9> m_PosDecoder;
  CHuffDecoder<k_NumLenSyms, 512, 8> m_LenDecoder;
  CHuffDecoder<k_NumPowerSyms, 512, 6> m_PowerDecoder;
  CHuffDecoder<k_NumPosSyms, 1024, 9> m_DeltaDecoder;

  Int32 *_x86_history;

  HRESULT CodeReal(const Byte *in, size_t inSize, Byte *out, size_t outSize);
public:
  CDecoder();
  ~CDecoder();

  HRESULT Code(const Byte *in, size_t inSize, Byte *out, size_t outSize);
  size_t GetUnpackSize() const { return _pos; }
};

}}

#endif

/* ---- CPP/7zip/Compress/Lzx.h ---- */
// Lzx.h

#ifndef ZIP7_INC_COMPRESS_LZX_H
#define ZIP7_INC_COMPRESS_LZX_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzx {

const unsigned kBlockType_NumBits = 3;
const unsigned kBlockType_Verbatim = 1;
const unsigned kBlockType_Aligned = 2;
const unsigned kBlockType_Uncompressed = 3;

const unsigned kNumHuffmanBits = 16;
const unsigned kNumReps = 3;

const unsigned kNumLenSlots = 8;
const unsigned kMatchMinLen = 2;
const unsigned kNumLenSymbols = 249;
const unsigned kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const unsigned kNumAlignLevelBits = 3;
const unsigned kNumAlignBits = 3;
const unsigned kAlignTableSize = 1 << kNumAlignBits;

const unsigned kNumPosSlots = 50;
const unsigned kNumPosLenSlots = kNumPosSlots * kNumLenSlots;

const unsigned kMainTableSize = 256 + kNumPosLenSlots;
const unsigned kLevelTableSize = 20;
const unsigned kMaxTableSize = kMainTableSize;

const unsigned kNumLevelBits = 4;

const unsigned kLevelSym_Zero1 = 17;
const unsigned kLevelSym_Zero2 = 18;
const unsigned kLevelSym_Same = 19;

const unsigned kLevelSym_Zero1_Start = 4;
const unsigned kLevelSym_Zero1_NumBits = 4;

const unsigned kLevelSym_Zero2_Start = kLevelSym_Zero1_Start + (1 << kLevelSym_Zero1_NumBits);
const unsigned kLevelSym_Zero2_NumBits = 5;

const unsigned kLevelSym_Same_NumBits = 1;
const unsigned kLevelSym_Same_Start = 4;
 
const unsigned kNumDictBits_Min = 15;
const unsigned kNumDictBits_Max = 21;
const UInt32 kDictSize_Max = (UInt32)1 << kNumDictBits_Max;

const unsigned kNumLinearPosSlotBits = 17;
// const unsigned kNumPowerPosSlots = 38;
// const unsigned kNumPowerPosSlots = (kNumLinearPosSlotBits + 1) * 2; // non-including two first linear slots.
const unsigned kNumPowerPosSlots = (kNumLinearPosSlotBits + 2) * 2; // including two first linear slots.

}}

#endif

/* ---- CPP/7zip/Compress/LzxDecoder.h ---- */
// LzxDecoder.h

#ifndef ZIP7_INC_LZX_DECODER_H
#define ZIP7_INC_LZX_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzx {

const unsigned kAdditionalOutputBufSize = 32 * 2;

const unsigned kNumTableBits_Main = 11;
const unsigned kNumTableBits_Len = 8;

// if (kNumLenSymols_Big <= 256) we can  use NHuffman::CDecoder256
// if (kNumLenSymols_Big >  256) we must use NHuffman::CDecoder
// const unsigned kNumLenSymols_Big_Start = kNumLenSlots - 1 + kMatchMinLen;  // 8 - 1 + 2
const unsigned kNumLenSymols_Big_Start = 0;
// const unsigned kNumLenSymols_Big_Start = 0;
const unsigned kNumLenSymols_Big = kNumLenSymols_Big_Start + kNumLenSymbols;

#if 1
  // for smallest structure size:
  const unsigned kPosSlotOffset = 0;
#else
  // use virtual entries for mispredicted branches:
  const unsigned kPosSlotOffset = 256 / kNumLenSlots;
#endif

class CBitByteDecoder;

class CDecoder
{
public:
  UInt32 _pos;
  UInt32 _winSize;
  Byte *_win;

  bool _overDict;
  bool _isUncompressedBlock;
  bool _skipByte;
  bool _keepHistory;
  bool _keepHistoryForNext;
  bool _needAlloc;
  bool _wimMode;
  Byte _numDictBits;

  // unsigned _numAlignBits_PosSlots;
  // unsigned _numAlignBits;
  UInt32 _numAlignBits_Dist;
private:
  unsigned _numPosLenSlots;
  UInt32 _unpackBlockSize;

  UInt32 _writePos;

  UInt32 _x86_translationSize;
  UInt32 _x86_processedSize;
  Byte *_x86_buf;

  Byte *_unpackedData;
public:
  Byte  _extra[kPosSlotOffset + kNumPosSlots];
  UInt32 _reps[kPosSlotOffset + kNumPosSlots];

  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, kNumTableBits_Main> _mainDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kNumLenSymols_Big, kNumTableBits_Len> _lenDecoder;
  NHuffman::CDecoder7b<kAlignTableSize> _alignDecoder;
private:
  Byte _mainLevels[kMainTableSize];
  Byte _lenLevels[kNumLenSymols_Big];

  HRESULT Flush() throw();
  bool ReadTables(CBitByteDecoder &_bitStream) throw();

  HRESULT CodeSpec(const Byte *inData, size_t inSize, UInt32 outSize) throw();
  HRESULT SetParams2(unsigned numDictBits) throw();
public:
  CDecoder() throw();
  ~CDecoder() throw();
  
  void Set_WimMode(bool wimMode) { _wimMode = wimMode; }
  void Set_KeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
  void Set_KeepHistoryForNext(bool keepHistoryForNext) { _keepHistoryForNext = keepHistoryForNext; }

  HRESULT Set_ExternalWindow_DictBits(Byte *win, unsigned numDictBits)
  {
    _needAlloc = false;
    _win = win;
    _winSize = (UInt32)1 << numDictBits;
    return SetParams2(numDictBits);
  }
  HRESULT Set_DictBits_and_Alloc(unsigned numDictBits) throw();

  HRESULT Code_WithExceedReadWrite(const Byte *inData, size_t inSize, UInt32 outSize) throw();
  
  bool WasBlockFinished()     const { return _unpackBlockSize == 0; }
  const Byte *GetUnpackData() const { return _unpackedData; }
  UInt32 GetUnpackSize()      const { return _pos - _writePos; }
};

}}

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

/* ---- CPP/7zip/Archive/Wim/WimIn.h ---- */
// Archive/WimIn.h

#ifndef ZIP7_INC_ARCHIVE_WIM_IN_H
#define ZIP7_INC_ARCHIVE_WIM_IN_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NWim {

/*
WIM versions:
hexVer : headerSize : ver
 : 1.07.01 - 1.08.01 : Longhorn.4001-4015 - another header, no signature, CAB compression
10900 : 60 : 1.09 : Longhorn.4029-4039 (2003)
10A00 : 60 : 1.10 : Longhorn.4083 (2004) image starting from 1
10B00 : ?? : 1.11 : ??
10C00 : 74 : 1.12 : Longhorn.4093 - VistaBeta1.5112 (2005) - (Multi-Part, SHA1)
10D00 : D0 : 1.13 : VistaBeta2 - Win10, (NumImages, BootIndex, IntegrityResource)
00E00 : D0 : 0.14 : LZMS, solid, esd, dism
*/

const unsigned kDirRecordSizeOld = 62;
const unsigned kDirRecordSize = 102;

/*
  There is error in WIM specification about dwReparseTag, dwReparseReserved and liHardLink fields.

  Correct DIRENTRY structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt32  Attrib;
     C    UInt32  SecurityId;
    
    10    UInt64  SubdirOffset; // = 0 for files

    18    UInt64  unused1; // = 0?
    20    UInt64  unused2; // = 0?
    
    28    UInt64  CTime;
    30    UInt64  ATime;
    38    UInt64  MTime;
    
    40    Byte    Sha1[20];
    
    54    UInt32  Unknown1; // is it 0 always?

       
    union
    {
    58    UInt64  NtNodeId;
        {
    58    UInt32  ReparseTag;
    5C    UInt32  ReparseFlags; // is it 0 always? Check with new imagex.
        }
    }

    60    UInt16  Streams;
    
    62    UInt16  ShortNameLen;
    64    UInt16  FileNameLen;
    
    66    UInt16  Name[];
          UInt16  ShortName[];
  }

  // DIRENTRY for WIM_VERSION <= 1.10
  DIRENTRY_OLD structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt32  Attrib;
     C    UInt32  SecurityId;

    union
    {
    10    UInt64  SubdirOffset; //

    10    UInt32  OldWimFileId; // used for files in old WIMs
    14    UInt32  OldWimFileId_Reserved; // = 0
    }

    18    UInt64  CTime;
    20    UInt64  ATime;
    28    UInt64  MTime;
    
    30    UInt64  Unknown; // NtNodeId ?

    38    UInt16  Streams;
    3A    UInt16  ShortNameLen;
    3C    UInt16  FileNameLen;
    3E    UInt16  FileName[];
          UInt16  ShortName[];
  }

  ALT_STREAM structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt64  Unused;
    10    Byte    Sha1[20];
    24    UInt16  FileNameLen;
    26    UInt16  FileName[];
  }

  ALT_STREAM_OLD structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt64  StreamId; // 32-bit value
    10    UInt16  FileNameLen;
    12    UInt16  FileName[];
  }

  If item is file (not Directory) and there are alternative streams,
  there is additional ALT_STREAM item of main "unnamed" stream in Streams array.

*/


namespace NResourceFlags
{
  // const Byte kFree = 1 << 0;
  const Byte kMetadata = 1 << 1;
  const Byte kCompressed = 1 << 2;
  // const Byte kSpanned = 1 << 3;
  const Byte kSolid = 1 << 4;
}

const UInt64 k_SolidBig_Resource_Marker = (UInt64)1 << 32;

struct CResource
{
  UInt64 PackSize;
  UInt64 Offset;
  UInt64 UnpackSize;
  Byte Flags;
  bool KeepSolid;
  int SolidIndex;

  void Clear()
  {
    PackSize = 0;
    Offset = 0;
    UnpackSize = 0;
    Flags = 0;
    KeepSolid = false;
    SolidIndex = -1;
  }

  UInt64 GetEndLimit() const { return Offset + PackSize; }
  void Parse(const Byte *p);
  void ParseAndUpdatePhySize(const Byte *p, UInt64 &phySize)
  {
    Parse(p);
    UInt64 v = GetEndLimit();
    if (phySize < v)
      phySize = v;
  }

  void WriteTo(Byte *p) const;

  bool IsMetadata() const { return (Flags & NResourceFlags::kMetadata) != 0; }
  bool IsCompressed() const { return (Flags & NResourceFlags::kCompressed) != 0; }
  bool IsSolid() const { return (Flags & NResourceFlags::kSolid) != 0; }
  bool IsSolidBig() const { return IsSolid() && UnpackSize == k_SolidBig_Resource_Marker; }
  bool IsSolidSmall() const { return IsSolid() && UnpackSize == 0; }

  bool IsEmpty() const { return (UnpackSize == 0); }
};


struct CSolid
{
  unsigned StreamIndex;
  // unsigned NumRefs;
  int FirstSmallStream;
  
  UInt64 SolidOffset;
  
  UInt64 UnpackSize;
  int Method;
  unsigned ChunkSizeBits;

  UInt64 HeadersSize;
  // size_t NumChunks;
  CObjArray<UInt64> Chunks; // [NumChunks + 1] (start offset)

  UInt64 GetChunkPackSize(size_t chunkIndex) const { return Chunks[chunkIndex + 1] - Chunks[chunkIndex]; }

  CSolid():
      FirstSmallStream(-1),
      // NumRefs(0),
      Method(-1)
      {}
};


namespace NHeaderFlags
{
  const UInt32 kCompression  = 1 << 1;
  const UInt32 kReadOnly     = 1 << 2;
  const UInt32 kSpanned      = 1 << 3;
  const UInt32 kResourceOnly = 1 << 4;
  const UInt32 kMetadataOnly = 1 << 5;
  const UInt32 kWriteInProgress = 1 << 6;
  const UInt32 kReparsePointFixup = 1 << 7;
  
  const UInt32 kXPRESS       = (UInt32)1 << 17;
  const UInt32 kLZX          = (UInt32)1 << 18;
  const UInt32 kLZMS         = (UInt32)1 << 19;
  const UInt32 kXPRESS2      = (UInt32)1 << 21; // XPRESS with nonstandard chunk size ?

  const UInt32 kMethodMask   = 0xFFFE0000;
}


namespace NMethod
{
  const UInt32 kXPRESS = 1;
  const UInt32 kLZX    = 2;
  const UInt32 kLZMS   = 3;
}


const UInt32 k_Version_NonSolid = 0x10D00;
const UInt32 k_Version_Solid = 0xE00;

const unsigned kHeaderSizeMax = 0xD0;
const unsigned kSignatureSize = 8;
extern const Byte kSignature[kSignatureSize];

const unsigned kChunkSizeBits = 15;
const UInt32 kChunkSize = (UInt32)1 << kChunkSizeBits;


struct CHeader
{
  UInt32 Version;
  UInt32 Flags;
  UInt32 ChunkSize;
  unsigned ChunkSizeBits;
  Byte Guid[16];
  UInt16 PartNumber;
  UInt16 NumParts;
  UInt32 NumImages;
  UInt32 BootIndex;

  bool _isOldVersion; // 1.10-
  bool _isNewVersion; // 1.13+ or 0.14

  CResource OffsetResource;
  CResource XmlResource;
  CResource MetadataResource;
  CResource IntegrityResource;

  void SetDefaultFields(bool useLZX);

  void WriteTo(Byte *p) const;
  HRESULT Parse(const Byte *p, UInt64 &phySize);
  
  bool IsCompressed() const { return (Flags & NHeaderFlags::kCompression) != 0; }
  
  bool IsSupported() const
  {
    return (!IsCompressed()
        || (Flags & NHeaderFlags::kLZX) != 0
        || (Flags & NHeaderFlags::kXPRESS) != 0
        || (Flags & NHeaderFlags::kLZMS) != 0
        || (Flags & NHeaderFlags::kXPRESS2) != 0);
  }
  
  unsigned GetMethod() const
  {
    if (!IsCompressed())
      return 0;
    UInt32 mask = (Flags & NHeaderFlags::kMethodMask);
    if (mask == 0) return 0;
    if (mask == NHeaderFlags::kXPRESS) return NMethod::kXPRESS;
    if (mask == NHeaderFlags::kLZX) return NMethod::kLZX;
    if (mask == NHeaderFlags::kLZMS) return NMethod::kLZMS;
    if (mask == NHeaderFlags::kXPRESS2) return NMethod::kXPRESS;
    return mask;
  }

  bool IsOldVersion() const { return _isOldVersion; }
  bool IsNewVersion() const { return _isNewVersion; }
  bool IsSolidVersion() const { return (Version == k_Version_Solid); }

  bool AreFromOnArchive(const CHeader &h)
  {
    return (memcmp(Guid, h.Guid, sizeof(Guid)) == 0) && (h.NumParts == NumParts);
  }
};


const unsigned kHashSize = 20;

inline bool IsEmptySha(const Byte *data)
{
  for (unsigned i = 0; i < kHashSize; i++)
    if (data[i] != 0)
      return false;
  return true;
}

const unsigned kStreamInfoSize = 24 + 2 + 4 + kHashSize;

struct CStreamInfo
{
  CResource Resource;
  UInt16 PartNumber;      // for NEW WIM format, we set it to 1 for OLD WIM format
  UInt32 RefCount;
  UInt32 Id;              // for OLD WIM format
  Byte Hash[kHashSize];

  bool IsEmptyHash() const { return IsEmptySha(Hash); }
  
  void WriteTo(Byte *p) const;
};


struct CItem
{
  size_t Offset;
  int IndexInSorted;
  int StreamIndex;
  int Parent;
  int ImageIndex; // -1 means that file is unreferenced in Images (deleted item?)
  bool IsDir;
  bool IsAltStream;
  unsigned DirLevel;
  size_t SubDirOffset;

  bool HasMetadata() const { return ImageIndex >= 0; }

  CItem():
    IndexInSorted(-1),
    StreamIndex(-1),
    Parent(-1),
    IsDir(false),
    IsAltStream(false),
    DirLevel(0),
    SubDirOffset(0)
    {}
};

struct CImage
{
  CByteBuffer Meta;
  CRecordVector<UInt32> SecurOffsets;
  unsigned StartItem;
  unsigned NumItems;
  unsigned NumEmptyRootItems;
  int VirtualRootIndex; // index in CDatabase::VirtualRoots[]
  UString RootName;
  CByteBuffer RootNameBuf;

  CImage(): VirtualRootIndex(-1) {}
};


struct CImageInfo
{
  bool CTimeDefined;
  bool MTimeDefined;
  bool NameDefined;
  bool IndexDefined;
  
  FILETIME CTime;
  FILETIME MTime;
  UString Name;

  UInt64 DirCount;
  UInt64 FileCount;
  UInt32 Index;

  int ItemIndexInXml;

  UInt64 GetTotalFilesAndDirs() const { return DirCount + FileCount; }
  
  CImageInfo(): CTimeDefined(false), MTimeDefined(false), NameDefined(false),
      IndexDefined(false), ItemIndexInXml(-1) {}
  void Parse(const CXmlItem &item);
};


struct CWimXml
{
  CByteBuffer Data;
  CXml Xml;

  UInt16 VolIndex;
  CObjectVector<CImageInfo> Images;

  UString FileName;
  bool IsEncrypted;

  UInt64 GetTotalFilesAndDirs() const
  {
    UInt64 sum = 0;
    FOR_VECTOR (i, Images)
      sum += Images[i].GetTotalFilesAndDirs();
    return sum;
  }

  void ToUnicode(UString &s);
  bool Parse();

  CWimXml(): IsEncrypted(false) {}
};


struct CVolume
{
  CHeader Header;
  CMyComPtr<IInStream> Stream;
};


class CDatabase
{
  Byte *DirData;
  size_t DirSize;
  size_t DirProcessed;
  size_t DirStartOffset;
  IArchiveOpenCallback *OpenCallback;

  HRESULT ParseDirItem(size_t pos, int parent, unsigned dirLevel);
  HRESULT ParseImageDirs(CByteBuffer &buf, int parent);

public:
  CRecordVector<CStreamInfo> DataStreams;
  CRecordVector<CStreamInfo> MetaStreams;

  CObjectVector<CSolid> Solids;
  
  CRecordVector<CItem> Items;
  CObjectVector<CByteBuffer> ReparseItems;
  CIntVector ItemToReparse; // from index_in_Items to index_in_ReparseItems
                            // -1 means no reparse;
  
  CObjectVector<CImage> Images;
  
  bool IsOldVersion9;
  bool IsOldVersion;
  bool ThereAreDeletedStreams;
  bool ThereAreAltStreams;
  bool RefCountError;
  bool HeadersError;

  unsigned GetStartImageIndex() const { return IsOldVersion9 ? 0 : 1; }
  unsigned GetDirAlignMask() const { return IsOldVersion9 ? 3 : 7; }
  
  // User Items can contain all images or just one image from all.
  CUIntVector SortedItems;
  int IndexOfUserImage;    // -1 : if more than one images was filled to Sorted Items
  
  unsigned NumExcludededItems;
  int ExludedItem;          // -1 : if there are no exclude items
  CUIntVector VirtualRoots; // we use them for old 1.10 WIM archives

  bool ThereIsError() const { return RefCountError || HeadersError; }

  unsigned GetNumUserItemsInImage(unsigned imageIndex) const
  {
    if (IndexOfUserImage >= 0 && imageIndex != (unsigned)IndexOfUserImage)
      return 0;
    if (imageIndex >= Images.Size())
      return 0;
    return Images[imageIndex].NumItems - NumExcludededItems;
  }

  bool ItemHasStream(const CItem &item) const;

  UInt64 Get_UnpackSize_of_Resource(const CResource &r) const
  {
    if (!r.IsSolid())
      return r.UnpackSize;
    if (r.IsSolidSmall())
      return r.PackSize;
    if (r.IsSolidBig() && r.SolidIndex >= 0)
      return Solids[(unsigned)r.SolidIndex].UnpackSize;
    return 0;
  }

  UInt64 Get_PackSize_of_Resource(unsigned streamIndex) const
  {
    const CResource &r = DataStreams[streamIndex].Resource;
    if (!r.IsSolidSmall())
      return r.PackSize;
    if (r.SolidIndex >= 0)
    {
      const CSolid &ss = Solids[(unsigned)r.SolidIndex];
      if (ss.FirstSmallStream == (int)streamIndex)
        return DataStreams[ss.StreamIndex].Resource.PackSize;
    }
    return 0;
  }

  UInt64 GetUnpackSize() const
  {
    UInt64 res = 0;
    FOR_VECTOR (i, DataStreams)
      res += DataStreams[i].Resource.UnpackSize;
    return res;
  }

  UInt64 GetPackSize() const
  {
    UInt64 res = 0;
    FOR_VECTOR (i, DataStreams)
      res += DataStreams[i].Resource.PackSize;
    return res;
  }

  void Clear()
  {
    DataStreams.Clear();
    MetaStreams.Clear();
    Solids.Clear();
    
    Items.Clear();
    ReparseItems.Clear();
    ItemToReparse.Clear();

    SortedItems.Clear();
    
    Images.Clear();
    VirtualRoots.Clear();

    IsOldVersion = false;
    ThereAreDeletedStreams = false;
    ThereAreAltStreams = false;
    RefCountError = false;
    HeadersError = false;
  }

  CDatabase():
    RefCountError(false),
    HeadersError(false)
    {}

  void GetShortName(unsigned index, NWindows::NCOM::CPropVariant &res) const;
  void GetItemName(unsigned index1, NWindows::NCOM::CPropVariant &res) const;
  void GetItemPath(unsigned index, bool showImageNumber, NWindows::NCOM::CPropVariant &res) const;

  HRESULT OpenXml(IInStream *inStream, const CHeader &h, CByteBuffer &xml);
  HRESULT Open(IInStream *inStream, const CHeader &h, unsigned numItemsReserve, IArchiveOpenCallback *openCallback);
  HRESULT FillAndCheck(const CObjectVector<CVolume> &volumes);

  /*
    imageIndex showImageNumber NumImages
         *        true           *       Show Image_Number
        -1           *          >1       Show Image_Number
        -1        false          1       Don't show Image_Number
         N        false          *       Don't show Image_Number
  */
  HRESULT GenerateSortedItems(int imageIndex, bool showImageNumber);

  HRESULT ExtractReparseStreams(const CObjectVector<CVolume> &volumes, IArchiveOpenCallback *openCallback);
};

HRESULT ReadHeader(IInStream *inStream, CHeader &header, UInt64 &phySize);


struct CMidBuf
{
  Byte *Data;
  size_t _size;

  CMidBuf(): Data(NULL), _size(0) {}

  void EnsureCapacity(size_t size)
  {
    if (size > _size)
    {
      ::z7_AlignedFree(Data);
      _size = 0;
      Data = (Byte *)::z7_AlignedAlloc(size);
      if (Data)
        _size = size;
    }
  }

  ~CMidBuf() { ::z7_AlignedFree(Data); }
};


class CUnpacker
{
  CMyComPtr2<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyUniquePtr<NCompress::NLzx::CDecoder> lzxDecoder;
  CMyUniquePtr<NCompress::NLzms::CDecoder> lzmsDecoder;

  CByteBuffer sizesBuf;

  CMidBuf packBuf;
  CMidBuf unpackBuf;

  // solid resource
  int _solidIndex;
  size_t _unpackedChunkIndex;

  HRESULT UnpackChunk(
      ISequentialInStream *inStream,
      unsigned method, unsigned chunkSizeBits,
      size_t inSize, size_t outSize,
      ISequentialOutStream *outStream);

  HRESULT Unpack2(
      IInStream *inStream,
      const CResource &res,
      const CHeader &header,
      const CDatabase *db,
      ISequentialOutStream *outStream,
      ICompressProgressInfo *progress);

public:
  UInt64 TotalPacked;

  CUnpacker():
      lzmsDecoder(NULL),
      _solidIndex(-1),
      _unpackedChunkIndex(0),
      TotalPacked(0)
      {}

  HRESULT Unpack(
      IInStream *inStream,
      const CResource &res,
      const CHeader &header,
      const CDatabase *db,
      ISequentialOutStream *outStream,
      ICompressProgressInfo *progress,
      Byte *digest);

  HRESULT UnpackData(IInStream *inStream,
      const CResource &resource, const CHeader &header,
      const CDatabase *db,
      CByteBuffer &buf, Byte *digest);
};

}}
  
#endif

/* ---- CPP/7zip/Archive/Wim/WimHandler.h ---- */
// WimHandler.h

#ifndef ZIP7_INC_ARCHIVE_WIM_HANDLER_H
#define ZIP7_INC_ARCHIVE_WIM_HANDLER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NWim {

const Int32 kNumImagesMaxUpdate = 1 << 10;

Z7_CLASS_IMP_CHandler_IInArchive_5(
    IArchiveGetRawProps
  , IArchiveGetRootProps
  , IArchiveKeepModeForNextOpen
  , ISetProperties
  , IOutArchive
)
  CDatabase _db;
  UInt32 _version;
  UInt32 _bootIndex;

  CObjectVector<CVolume> _volumes;
  CObjectVector<CWimXml> _xmls;
  // unsigned _nameLenForStreams;
 
  unsigned _numXmlItems;
  unsigned _numIgnoreItems;

  bool _isOldVersion;
  bool _xmlInComments;

  bool _xmlError;
  bool _isArc;
  bool _unsupported;

  bool _set_use_ShowImageNumber;
  bool _set_showImageNumber;
  int _defaultImageNumber;

  bool _showImageNumber;
  bool _keepMode_ShowImageNumber;
  bool _disable_Sha1Check;

  UInt64 _phySize;
  Int32 _firstVolumeIndex;

  CHandlerTimeOptions _timeOptions;

  void InitDefaults()
  {
    _disable_Sha1Check = false;
    _set_use_ShowImageNumber = false;
    _set_showImageNumber = false;
    _defaultImageNumber = -1;
    _timeOptions.Init();
  }

  bool IsUpdateSupported() const
  {
    if (ThereIsError()) return false;
    if (_db.Images.Size() > kNumImagesMaxUpdate) return false;

    // Solid format is complicated. So we disable updating now.
    if (!_db.Solids.IsEmpty()) return false;

    if (_volumes.Size() == 0)
      return true;
    
    if (_volumes.Size() != 2) return false;
    if (_volumes[0].Stream) return false;
    if (_version != k_Version_NonSolid
        // && _version != k_Version_Solid
        ) return false;
    
    return true;
  }

  bool ThereIsError() const { return _xmlError || _db.ThereIsError(); }
  HRESULT GetSecurity(UInt32 realIndex, const void **data, UInt32 *dataSize, UInt32 *propType);

  HRESULT GetOutProperty(IArchiveUpdateCallback *callback, UInt32 callbackIndex, Int32 arcIndex, PROPID propID, PROPVARIANT *value);
  HRESULT        GetTime(IArchiveUpdateCallback *callback, UInt32 callbackIndex, Int32 arcIndex, PROPID propID, FILETIME &ft);
public:
  CHandler();
};

}}

#endif

/* ---- CPP/Common/MyBuffer2.h ---- */
// Common/MyBuffer2.h

#ifndef ZIP7_INC_COMMON_MY_BUFFER2_H
#define ZIP7_INC_COMMON_MY_BUFFER2_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

class CMidBuffer
{
  Byte *_data;
  size_t _size;

  Z7_CLASS_NO_COPY(CMidBuffer)

public:
  CMidBuffer(): _data(NULL), _size(0) {}
  ~CMidBuffer() { ::MidFree(_data); }

  void Free() { ::MidFree(_data); _data = NULL; _size = 0; }

  bool IsAllocated() const { return _data != NULL; }
  operator       Byte *()       { return _data; }
  operator const Byte *() const { return _data; }
  size_t Size() const { return _size; }

  void Alloc(size_t size)
  {
    if (!_data || size != _size)
    {
      ::MidFree(_data);
      _size = 0;
      _data = NULL;
      _data = (Byte *)::MidAlloc(size);
      if (_data)
        _size = size;
    }
  }

  void AllocAtLeast(size_t size)
  {
    if (!_data || size > _size)
    {
      ::MidFree(_data);
      const size_t kMinSize = (size_t)1 << 16;
      if (size < kMinSize)
        size = kMinSize;
      _size = 0;
      _data = NULL;
      _data = (Byte *)::MidAlloc(size);
      if (_data)
        _size = size;
    }
  }
};


class CAlignedBuffer1
{
  Byte *_data;

  Z7_CLASS_NO_COPY(CAlignedBuffer1)

public:
  ~CAlignedBuffer1()
  {
    z7_AlignedFree(_data);
  }

  CAlignedBuffer1(size_t size)
  {
    _data = NULL;
    _data = (Byte *)z7_AlignedAlloc(size);
    if (!_data)
      throw 1;
  }

  operator       Byte *()       { return _data; }
  operator const Byte *() const { return _data; }
};


class CAlignedBuffer
{
  Byte *_data;
  size_t _size;

  Z7_CLASS_NO_COPY(CAlignedBuffer)

public:
  CAlignedBuffer(): _data(NULL), _size(0) {}
  ~CAlignedBuffer()
  {
    z7_AlignedFree(_data);
  }

  /*
  CAlignedBuffer(size_t size): _size(0)
  {
    _data = NULL;
    _data = (Byte *)z7_AlignedAlloc(size);
    if (!_data)
      throw 1;
    _size = size;
  }
  */

  void Free()
  {
    z7_AlignedFree(_data);
    _data = NULL;
    _size = 0;
  }

  bool IsAllocated() const { return _data != NULL; }
  operator       Byte *()       { return _data; }
  operator const Byte *() const { return _data; }
  size_t Size() const { return _size; }

  void Alloc(size_t size)
  {
    if (!_data || size != _size)
    {
      z7_AlignedFree(_data);
      _size = 0;
      _data = NULL;
      _data = (Byte *)z7_AlignedAlloc(size);
      if (_data)
        _size = size;
    }
  }

  void AllocAtLeast(size_t size)
  {
    if (!_data || size > _size)
    {
      z7_AlignedFree(_data);
      _size = 0;
      _data = NULL;
      _data = (Byte *)z7_AlignedAlloc(size);
      if (_data)
        _size = size;
    }
  }

  // (size <= size_max)
  void AllocAtLeast_max(size_t size, size_t size_max)
  {
    if (!_data || size > _size)
    {
      z7_AlignedFree(_data);
      _size = 0;
      _data = NULL;
      if (size_max < size) size_max = size; // optional check
      const size_t delta = size / 2;
      size += delta;
      if (size < delta || size > size_max)
        size = size_max;
      _data = (Byte *)z7_AlignedAlloc(size);
      if (_data)
        _size = size;
    }
  }
};

/*
  CMidAlignedBuffer must return aligned pointer.
   - in Windows it uses CMidBuffer(): MidAlloc() : VirtualAlloc()
       VirtualAlloc(): Memory allocated is automatically initialized to zero.
       MidAlloc(0) returns NULL
   - in non-Windows systems it uses g_AlignedAlloc.
     g_AlignedAlloc::Alloc(size = 0) can return non NULL.
*/

typedef
#ifdef _WIN32
  CMidBuffer
#else
  CAlignedBuffer
#endif
  CMidAlignedBuffer;


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

/* ---- CPP/Common/Wildcard.h ---- */
// Common/Wildcard.h

#ifndef ZIP7_INC_COMMON_WILDCARD_H
#define ZIP7_INC_COMMON_WILDCARD_H

// amalgamation: header emitted in prologue

int CompareFileNames(const wchar_t *s1, const wchar_t *s2) STRING_UNICODE_THROW;
#ifndef USE_UNICODE_FSTRING
  int CompareFileNames(const char *s1, const char *s2);
#endif

bool IsPath1PrefixedByPath2(const wchar_t *s1, const wchar_t *s2);

void SplitPathToParts(const UString &path, UStringVector &pathParts);
void SplitPathToParts_2(const UString &path, UString &dirPrefix, UString &name);
void SplitPathToParts_Smart(const UString &path, UString &dirPrefix, UString &name); // ignores dir delimiter at the end of (path)

UString ExtractDirPrefixFromPath(const UString &path);
UString ExtractFileNameFromPath(const UString &path);

bool DoesNameContainWildcard(const UString &path);
bool DoesWildcardMatchName(const UString &mask, const UString &name);

namespace NWildcard {

#ifdef _WIN32
// returns true, if name is like "a:", "c:", ...
bool IsDriveColonName(const wchar_t *s);
unsigned GetNumPrefixParts_if_DrivePath(UStringVector &pathParts);
#endif

struct CItem
{
  UStringVector PathParts;
  bool Recursive;
  bool ForFile;
  bool ForDir;
  bool WildcardMatching;
  
  #ifdef _WIN32
  bool IsDriveItem() const
  {
    return PathParts.Size() == 1 && !ForFile && ForDir && IsDriveColonName(PathParts[0]);
  }
  #endif

  // CItem(): WildcardMatching(true) {}

  bool AreAllAllowed() const;
  bool CheckPath(const UStringVector &pathParts, bool isFile) const;
};



const Byte kMark_FileOrDir = 0;
const Byte kMark_StrictFile = 1;
const Byte kMark_StrictFile_IfWildcard = 2;

struct CCensorPathProps
{
  bool Recursive;
  bool WildcardMatching;
  Byte MarkMode;
  
  CCensorPathProps():
      Recursive(false),
      WildcardMatching(true),
      MarkMode(kMark_FileOrDir)
      {}
};


class CCensorNode  MY_UNCOPYABLE
{
  CCensorNode *Parent;
  
  bool CheckPathCurrent(bool include, const UStringVector &pathParts, bool isFile) const;
  void AddItemSimple(bool include, CItem &item);
public:
  // bool ExcludeDirItems;

  CCensorNode():
      Parent(NULL)
      // , ExcludeDirItems(false)
      {}

  CCensorNode(const UString &name, CCensorNode *parent):
      Parent(parent)
      // , ExcludeDirItems(false)
      , Name(name)
      {}

  UString Name; // WIN32 doesn't support wildcards in file names
  CObjectVector<CCensorNode> SubNodes;
  CObjectVector<CItem> IncludeItems;
  CObjectVector<CItem> ExcludeItems;

  CCensorNode &Find_SubNode_Or_Add_New(const UString &name)
  {
    int i = FindSubNode(name);
    if (i >= 0)
      return SubNodes[(unsigned)i];
    // return SubNodes.Add(CCensorNode(name, this));
    CCensorNode &node = SubNodes.AddNew();
    node.Parent = this;
    node.Name = name;
    return node;
  }

  bool AreAllAllowed() const;

  int FindSubNode(const UString &path) const;

  void AddItem(bool include, CItem &item, int ignoreWildcardIndex = -1);
  // void AddItem(bool include, const UString &path, const CCensorPathProps &props);
  void Add_Wildcard()
  {
    CItem item;
    item.PathParts.Add(L"*");
    item.Recursive = false;
    item.ForFile = true;
    item.ForDir = true;
    item.WildcardMatching = true;
    AddItem(
        true // include
        , item);
  }

  // NeedCheckSubDirs() returns true, if there are IncludeItems rules that affect items in subdirs
  bool NeedCheckSubDirs() const;
  bool AreThereIncludeItems() const;

  /*
  CheckPathVect() doesn't check path in Parent CCensorNode
  so use CheckPathVect() for root CCensorNode
  OUT:
    returns (true) && (include = false) - file in exlude list
    returns (true) && (include = true)  - file in include list and is not in exlude list
    returns (false)  - file is not in (include/exlude) list
  */
  bool CheckPathVect(const UStringVector &pathParts, bool isFile, bool &include) const;

  // bool CheckPath2(bool isAltStream, const UString &path, bool isFile, bool &include) const;
  // bool CheckPath(bool isAltStream, const UString &path, bool isFile) const;

  // CheckPathToRoot_Change() changes pathParts !!!
  bool CheckPathToRoot_Change(bool include, UStringVector &pathParts, bool isFile) const;
  bool CheckPathToRoot(bool include, const UStringVector &pathParts, bool isFile) const;

  // bool CheckPathToRoot(const UString &path, bool isFile, bool include) const;
  void ExtendExclude(const CCensorNode &fromNodes);
};


struct CPair  MY_UNCOPYABLE
{
  UString Prefix;
  CCensorNode Head;
  
  // CPair(const UString &prefix): Prefix(prefix) { };
};


enum ECensorPathMode
{
  k_RelatPath,  // absolute prefix as Prefix, remain path in Tree
  k_FullPath,   // drive prefix as Prefix, remain path in Tree
  k_AbsPath     // full path in Tree
};


struct CCensorPath
{
  UString Path;
  bool Include;
  CCensorPathProps Props;

  CCensorPath():
      Include(true)
      {}
};


class CCensor  MY_UNCOPYABLE
{
  int FindPairForPrefix(const UString &prefix) const;
public:
  CObjectVector<CPair> Pairs;

  bool ExcludeDirItems;
  bool ExcludeFileItems;

  CCensor():
      ExcludeDirItems(false),
      ExcludeFileItems(false)
      {}

  CObjectVector<NWildcard::CCensorPath> CensorPaths;
  
  bool AllAreRelative() const
    { return (Pairs.Size() == 1 && Pairs.Front().Prefix.IsEmpty()); }
  
  void AddItem(ECensorPathMode pathMode, bool include, const UString &path, const CCensorPathProps &props);
  // bool CheckPath(bool isAltStream, const UString &path, bool isFile) const;
  void ExtendExclude();

  void AddPathsToCensor(NWildcard::ECensorPathMode censorPathMode);
  void AddPreItem(bool include, const UString &path, const CCensorPathProps &props);

  void AddPreItem_NoWildcard(const UString &path)
  {
    CCensorPathProps props;
    props.WildcardMatching = false;
    AddPreItem(
        true,  // include
        path, props);
  }
  void AddPreItem_Wildcard()
  {
    CCensorPathProps props;
    // props.WildcardMatching = true;
    AddPreItem(
        true,  // include
        UString("*"), props);
  }
};

}

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

/* ---- CPP/7zip/Common/UniqBlocks.h ---- */
// UniqBlocks.h

#ifndef ZIP7_INC_UNIQ_BLOCKS_H
#define ZIP7_INC_UNIQ_BLOCKS_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

struct C_UInt32_UString_Map
{
  CRecordVector<UInt32> Numbers;
  UStringVector Strings;
  
  void Add_UInt32(const UInt32 n)
  {
    Numbers.AddToUniqueSorted(n);
  }
  int Find(const UInt32 n)
  {
    return Numbers.FindInSorted(n);
  }
};


struct CUniqBlocks
{
  CObjectVector<CByteBuffer> Bufs;
  CUIntVector Sorted;
  CUIntVector BufIndexToSortedIndex;

  unsigned AddUniq(const Byte *data, size_t size);
  UInt64 GetTotalSizeInBytes() const;
  void GetReverseMap();

  bool IsOnlyEmpty() const
  {
    return (Bufs.Size() == 0 || (Bufs.Size() == 1 && Bufs[0].Size() == 0));
  }
};

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

/* ---- CPP/7zip/Crypto/RandGen.h ---- */
// RandGen.h

#ifndef ZIP7_INC_CRYPTO_RAND_GEN_H
#define ZIP7_INC_CRYPTO_RAND_GEN_H

// amalgamation: header emitted in prologue

#ifdef _WIN64
// #define USE_STATIC_SYSTEM_RAND
#endif

#ifdef USE_STATIC_SYSTEM_RAND

#ifdef _WIN32
#include <ntsecapi.h>
#define MY_RAND_GEN(data, size) RtlGenRandom(data, size)
#else
#define MY_RAND_GEN(data, size) getrandom(data, size, 0)
#endif

#else

class CRandomGenerator
{
  Byte _buff[SHA256_DIGEST_SIZE];
  bool _needInit;

  void Init();
public:
  CRandomGenerator(): _needInit(true) {}
  void Generate(Byte *data, unsigned size);
};

MY_ALIGN (16)
extern CRandomGenerator g_RandomGenerator;

#define MY_RAND_GEN(data, size) g_RandomGenerator.Generate(data, size)

#endif

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

/* ---- CPP/7zip/Crypto/Sha1Cls.h ---- */
// Crypto/Sha1Cls.h

#ifndef ZIP7_INC_CRYPTO_SHA1_CLS_H
#define ZIP7_INC_CRYPTO_SHA1_CLS_H

// amalgamation: header emitted in prologue

namespace NCrypto {
namespace NSha1 {

const unsigned kNumBlockWords = SHA1_NUM_BLOCK_WORDS;
const unsigned kNumDigestWords = SHA1_NUM_DIGEST_WORDS;

const unsigned kBlockSize = SHA1_BLOCK_SIZE;
const unsigned kDigestSize = SHA1_DIGEST_SIZE;

class CContext
{
  CSha1 _s;
 
public:
  void Init() throw() { Sha1_Init(&_s); }
  void Update(const Byte *data, size_t size) throw() { Sha1_Update(&_s, data, size); }
  void Final(Byte *digest) throw() { Sha1_Final(&_s, digest); }
  void PrepareBlock(Byte *block, unsigned size) const throw()
  {
    Sha1_PrepareBlock(&_s, block, size);
  }
  void GetBlockDigest(const Byte *blockData, Byte *destDigest) const throw()
  {
    Sha1_GetBlockDigest(&_s, blockData, destDigest);
  }
};

}}

#endif

/* ---- CPP/7zip/Archive/Common/OutStreamWithSha1.h ---- */
// OutStreamWithSha1.h

#ifndef ZIP7_INC_OUT_STREAM_WITH_SHA1_H
#define ZIP7_INC_OUT_STREAM_WITH_SHA1_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithSha1
  , ISequentialOutStream
)
  bool _calculate;
  CMyComPtr<ISequentialOutStream> _stream;
  CAlignedBuffer1 _sha;
  UInt64 _size;

  CSha1 *Sha() { return (CSha1 *)(void *)(Byte *)_sha; }
public:
  COutStreamWithSha1(): _sha(sizeof(CSha1)) {}
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(bool calculate = true)
  {
    _calculate = calculate;
    _size = 0;
    Sha1_Init(Sha());
  }
  void InitSha1() { Sha1_Init(Sha()); }
  UInt64 GetSize() const { return _size; }
  void Final(Byte *digest) { Sha1_Final(Sha(), digest); }
};


Z7_CLASS_IMP_NOQIB_1(
  CInStreamWithSha1
  , ISequentialInStream
)
  CMyComPtr<ISequentialInStream> _stream;
  CAlignedBuffer1 _sha;
  UInt64 _size;

  CSha1 *Sha() { return (CSha1 *)(void *)(Byte *)_sha; }
public:
  CInStreamWithSha1(): _sha(sizeof(CSha1)) {}
  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    Sha1_Init(Sha());
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  void Final(Byte *digest) { Sha1_Final(Sha(), digest); }
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

/* ---- CPP/7zip/Compress/XpressDecoder.h ---- */
// XpressDecoder.h

#ifndef ZIP7_INC_XPRESS_DECODER_H
#define ZIP7_INC_XPRESS_DECODER_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NXpress {

// (out) buffer size must be larger than (outSize) for the following value:
const unsigned kAdditionalOutputBufSize = 32;

HRESULT Decode_WithExceedWrite(const Byte *in, size_t inSize, Byte *out, size_t outSize);

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

/* ================ unit: CPP/7zip/Archive/Wim/WimHandler.cpp ================ */
// WimHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

using namespace NWindows;

namespace NArchive {
namespace NWim {

#define FILES_DIR_NAME "[DELETED]"

// #define WIM_DETAILS

static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidCTime,
  kpidATime,
  kpidAttrib,
  kpidMethod,
  kpidSolid,
  kpidShortName,
  kpidINode,
  kpidLinks,
  kpidIsAltStream,
  kpidNumAltStreams,
  
  #ifdef WIM_DETAILS
  , kpidVolume
  , kpidOffset
  #endif
};

enum
{
  kpidNumImages = kpidUserDefined,
  kpidBootImage
};

static const CStatProp kArcProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidUnpackVer, VT_BSTR},
  { NULL, kpidIsVolume, VT_BOOL},
  { NULL, kpidVolume, VT_UI4},
  { NULL, kpidNumVolumes, VT_UI4},
  { "Images", kpidNumImages, VT_UI4},
  { "Boot Image", kpidBootImage, VT_UI4}
};


static const char * const k_Methods[] =
{
    "Copy"
  , "XPress"
  , "LZX"
  , "LZMS"
};



IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

static void AddErrorMessage(AString &s, const char *message)
{
  if (!s.IsEmpty())
    s += ". ";
  s += message;
}


Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  const CImageInfo *image = NULL;
  if (_xmls.Size() == 1)
  {
    const CWimXml &xml = _xmls[0];
    if (xml.Images.Size() == 1)
      image = &xml.Images[0];
  }

  switch (propID)
  {
    case kpidPhySize:  prop = _phySize; break;
    case kpidSize: prop = _db.GetUnpackSize(); break;
    case kpidPackSize: prop = _db.GetPackSize(); break;
    
    case kpidCTime:
      if (_xmls.Size() == 1)
      {
        const CWimXml &xml = _xmls[0];
        int index = -1;
        FOR_VECTOR (i, xml.Images)
        {
          const CImageInfo &image2 = xml.Images[i];
          if (image2.CTimeDefined)
            if (index < 0 || ::CompareFileTime(&image2.CTime, &xml.Images[index].CTime) < 0)
              index = (int)i;
        }
        if (index >= 0)
          prop = xml.Images[index].CTime;
      }
      break;

    case kpidMTime:
      if (_xmls.Size() == 1)
      {
        const CWimXml &xml = _xmls[0];
        int index = -1;
        FOR_VECTOR (i, xml.Images)
        {
          const CImageInfo &image2 = xml.Images[i];
          if (image2.MTimeDefined)
            if (index < 0 || ::CompareFileTime(&image2.MTime, &xml.Images[index].MTime) > 0)
              index = (int)i;
        }
        if (index >= 0)
          prop = xml.Images[index].MTime;
      }
      break;

    case kpidComment:
      if (image)
      {
        if (_xmlInComments)
        {
          UString s;
          _xmls[0].ToUnicode(s);
          prop = s;
        }
        else if (image->NameDefined)
          prop = image->Name;
      }
      break;

    case kpidUnpackVer:
    {
      UInt32 ver1 = _version >> 16;
      UInt32 ver2 = (_version >> 8) & 0xFF;
      UInt32 ver3 = (_version) & 0xFF;

      AString res;
      res.Add_UInt32(ver1);
      res.Add_Dot();
      res.Add_UInt32(ver2);
      if (ver3 != 0)
      {
        res.Add_Dot();
        res.Add_UInt32(ver3);
      }
      prop = res;
      break;
    }

    case kpidIsVolume:
      if (_xmls.Size() > 0)
      {
        UInt16 volIndex = _xmls[0].VolIndex;
        if (volIndex < _volumes.Size())
          prop = (_volumes[volIndex].Header.NumParts > 1);
      }
      break;
    case kpidVolume:
      if (_xmls.Size() > 0)
      {
        UInt16 volIndex = _xmls[0].VolIndex;
        if (volIndex < _volumes.Size())
          prop = (UInt32)_volumes[volIndex].Header.PartNumber;
      }
      break;
    case kpidNumVolumes: if (_volumes.Size() > 0) prop = (UInt32)(_volumes.Size() - 1); break;
    
    case kpidClusterSize:
      if (_xmls.Size() > 0)
      {
        UInt16 volIndex = _xmls[0].VolIndex;
        if (volIndex < _volumes.Size())
        {
          const CHeader &h = _volumes[volIndex].Header;
          prop = (UInt32)1 << h.ChunkSizeBits;
        }
      }
      break;

    case kpidName:
      if (_firstVolumeIndex >= 0)
      {
        const CHeader &h = _volumes[_firstVolumeIndex].Header;
        if (GetUi32(h.Guid) != 0)
        {
          char temp[64];
          RawLeGuidToString(h.Guid, temp);
          temp[8] = 0; // for reduced GUID
          AString s (temp);
          const char *ext = ".wim";
          if (h.NumParts != 1)
          {
            s += '_';
            if (h.PartNumber != 1)
              s.Add_UInt32(h.PartNumber);
            ext = ".swm";
          }
          s += ext;
          prop = s;
        }
      }
      break;

    case kpidExtension:
      if (_firstVolumeIndex >= 0)
      {
        const CHeader &h = _volumes[_firstVolumeIndex].Header;
        if (h.NumParts > 1)
        {
          AString s;
          if (h.PartNumber != 1)
          {
            s.Add_UInt32(h.PartNumber);
            s.Add_Dot();
          }
          s += "swm";
          prop = s;
        }
      }
      break;

    case kpidNumImages: prop = (UInt32)_db.Images.Size(); break;
    case kpidBootImage: if (_bootIndex != 0) prop = (UInt32)_bootIndex; break;
    
    case kpidMethod:
    {
      UInt32 methodUnknown = 0;
      UInt32 methodMask = 0;
      unsigned chunkSizeBits = 0;
      
      {
        FOR_VECTOR (i, _xmls)
        {
          const CHeader &header = _volumes[_xmls[i].VolIndex].Header;
          unsigned method = header.GetMethod();
          if (method < Z7_ARRAY_SIZE(k_Methods))
            methodMask |= ((UInt32)1 << method);
          else
            methodUnknown = method;
          if (chunkSizeBits < header.ChunkSizeBits)
            chunkSizeBits = header.ChunkSizeBits;
        }
      }

      AString res;

      unsigned numMethods = 0;

      for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_Methods); i++)
      {
        if (methodMask & ((UInt32)1 << i))
        {
          res.Add_Space_if_NotEmpty();
          res += k_Methods[i];
          numMethods++;
        }
      }

      if (methodUnknown != 0)
      {
        res.Add_Space_if_NotEmpty();
        res.Add_UInt32(methodUnknown);
        numMethods++;
      }

      if (numMethods == 1 && chunkSizeBits != 0)
      {
        res.Add_Colon();
        res.Add_UInt32((UInt32)chunkSizeBits);
      }

      prop = res;
      break;
    }
    
    case kpidIsTree: prop = true; break;
    case kpidIsAltStream: prop = _db.ThereAreAltStreams; break;
    case kpidIsAux: prop = true; break;
    // WIM uses special prefix to represent deleted items
    // case kpidIsDeleted: prop = _db.ThereAreDeletedStreams; break;
    case kpidINode: prop = true; break;

    case kpidErrorFlags:
    {
      UInt32 flags = 0;
      if (!_isArc) flags |= kpv_ErrorFlags_IsNotArc;
      if (_db.HeadersError) flags |= kpv_ErrorFlags_HeadersError;
      if (_unsupported) flags |= kpv_ErrorFlags_UnsupportedMethod;
      prop = flags;
      break;
    }

    case kpidWarning:
    {
      AString s;
      if (_xmlError)
        AddErrorMessage(s, "XML error");
      if (_db.RefCountError)
        AddErrorMessage(s, "Some files have incorrect reference count");
      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidReadOnly:
    {
      bool readOnly = !IsUpdateSupported();
      if (readOnly)
        prop = readOnly;
      break;
    }
  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static void GetFileTime(const Byte *p, NCOM::CPropVariant &prop)
{
  prop.vt = VT_FILETIME;
  prop.filetime.dwLowDateTime = Get32(p);
  prop.filetime.dwHighDateTime = Get32(p + 4);
  prop.Set_FtPrec(k_PropVar_TimePrec_100ns);
}


static void MethodToProp(int method, int chunksSizeBits, NCOM::CPropVariant &prop)
{
  if (method >= 0)
  {
    char temp[32];
    
    if ((unsigned)method < Z7_ARRAY_SIZE(k_Methods))
      MyStringCopy(temp, k_Methods[(unsigned)method]);
    else
      ConvertUInt32ToString((UInt32)(unsigned)method, temp);
    
    if (chunksSizeBits >= 0)
    {
      size_t pos = strlen(temp);
      temp[pos++] = ':';
      ConvertUInt32ToString((unsigned)chunksSizeBits, temp + pos);
    }
    
    prop = temp;
  }
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  if (index < _db.SortedItems.Size())
  {
    unsigned realIndex = _db.SortedItems[index];
    const CItem &item = _db.Items[realIndex];
    const CStreamInfo *si = NULL;
    const CVolume *vol = NULL;
    if (item.StreamIndex >= 0)
    {
      si = &_db.DataStreams[item.StreamIndex];
      vol = &_volumes[si->PartNumber];
    }

    const CItem *mainItem = &item;
    if (item.IsAltStream)
      mainItem = &_db.Items[item.Parent];
    const Byte *metadata = NULL;
    if (mainItem->ImageIndex >= 0)
      metadata = _db.Images[mainItem->ImageIndex].Meta + mainItem->Offset;

    switch (propID)
    {
      case kpidPath:
        if (item.ImageIndex >= 0)
          _db.GetItemPath(realIndex, _showImageNumber, prop);
        else
        {
          /*
          while (s.Len() < _nameLenForStreams)
            s = '0' + s;
          */
          /*
          if (si->Resource.IsFree())
            s = (AString)("[Free]" STRING_PATH_SEPARATOR) + sz;
          else
          */
          AString s (FILES_DIR_NAME STRING_PATH_SEPARATOR);
          s.Add_UInt32((UInt32)(Int32)item.StreamIndex);
          prop = s;
        }
        break;
      
      case kpidName:
        if (item.ImageIndex >= 0)
          _db.GetItemName(realIndex, prop);
        else
        {
          char sz[16];
          ConvertUInt32ToString((UInt32)(Int32)item.StreamIndex, sz);
          /*
          AString s = sz;
          while (s.Len() < _nameLenForStreams)
            s = '0' + s;
          */
          prop = sz;
        }
        break;

      case kpidShortName:
        if (item.ImageIndex >= 0 && !item.IsAltStream)
          _db.GetShortName(realIndex, prop);
        break;

      case kpidPackSize:
      {
        if (si)
        {
          if (!si->Resource.IsSolidSmall())
            prop = si->Resource.PackSize;
          else
          {
            if (si->Resource.SolidIndex >= 0)
            {
              const CSolid &ss = _db.Solids[(unsigned)si->Resource.SolidIndex];
              if (ss.FirstSmallStream == item.StreamIndex)
                prop = _db.DataStreams[ss.StreamIndex].Resource.PackSize;
            }
          }
        }
        else if (!item.IsDir)
          prop = (UInt64)0;

        break;
      }

      case kpidSize:
      {
        if (si)
        {
          if (si->Resource.IsSolid())
          {
            if (si->Resource.IsSolidBig())
            {
              if (si->Resource.SolidIndex >= 0)
              {
                const CSolid &ss = _db.Solids[(unsigned)si->Resource.SolidIndex];
                prop = ss.UnpackSize;
              }
            }
            else
              prop = si->Resource.PackSize;
          }
          else
            prop = si->Resource.UnpackSize;
        }
        else if (!item.IsDir)
          prop = (UInt64)0;

        break;
      }
      
      case kpidIsDir: prop = item.IsDir; break;
      case kpidIsAltStream: prop = item.IsAltStream; break;
      case kpidNumAltStreams:
      {
        if (!item.IsAltStream && mainItem->HasMetadata())
        {
          UInt32 dirRecordSize = _db.IsOldVersion ? kDirRecordSizeOld : kDirRecordSize;
          UInt32 numAltStreams = Get16(metadata + dirRecordSize - 6);
          if (numAltStreams != 0)
          {
            if (!item.IsDir)
              numAltStreams--;
            prop = numAltStreams;
          }
        }
        break;
      }

      case kpidAttrib:
        if (!item.IsAltStream && mainItem->ImageIndex >= 0)
        {
          /*
          if (fileNameLen == 0 && isDir && !item.HasStream())
            item.Attrib = 0x10; // some swm archives have system/hidden attributes for root
          */
          prop = (UInt32)Get32(metadata + 8);
        }
        break;
      case kpidCTime: if (mainItem->HasMetadata()) GetFileTime(metadata + (_db.IsOldVersion ? 0x18: 0x28), prop); break;
      case kpidATime: if (mainItem->HasMetadata()) GetFileTime(metadata + (_db.IsOldVersion ? 0x20: 0x30), prop); break;
      case kpidMTime: if (mainItem->HasMetadata()) GetFileTime(metadata + (_db.IsOldVersion ? 0x28: 0x38), prop); break;

      case kpidINode:
        if (mainItem->HasMetadata() && !_isOldVersion)
        {
          UInt32 attrib = (UInt32)Get32(metadata + 8);
          if ((attrib & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
          {
            // we don't know about that field in OLD WIM format
            unsigned offset = 0x58; // (_db.IsOldVersion ? 0x30: 0x58);
            UInt64 val = Get64(metadata + offset);
            if (val != 0)
              prop = val;
          }
        }
        break;

      case kpidStreamId:
        if (item.StreamIndex >= 0)
          prop = (UInt32)item.StreamIndex;
        break;

      case kpidMethod:
          if (si)
          {
            const CResource &r = si->Resource;
            if (r.IsSolid())
            {
              if (r.SolidIndex >= 0)
              {
                CSolid &ss = _db.Solids[r.SolidIndex];
                MethodToProp(ss.Method, (int)ss.ChunkSizeBits, prop);
              }
            }
            else
            {
              int method = 0;
              int chunkSizeBits = -1;
              if (r.IsCompressed())
              {
                method = (int)vol->Header.GetMethod();
                chunkSizeBits = (int)vol->Header.ChunkSizeBits;
              }
              MethodToProp(method, chunkSizeBits, prop);
            }
          }
          break;

      case kpidSolid: if (si) prop = si->Resource.IsSolid(); break;
      case kpidLinks: if (si) prop = (UInt32)si->RefCount; break;
      #ifdef WIM_DETAILS
      case kpidVolume: if (si) prop = (UInt32)si->PartNumber; break;
      case kpidOffset: if (si)  prop = (UInt64)si->Resource.Offset; break;
      #endif
    }
  }
  else
  {
    index -= _db.SortedItems.Size();
    if (index < _numXmlItems)
    {
      switch (propID)
      {
        case kpidPath:
        case kpidName: prop = _xmls[index].FileName; break;
        case kpidIsDir: prop = false; break;
        case kpidPackSize:
        case kpidSize: prop = (UInt64)_xmls[index].Data.Size(); break;
        case kpidMethod: /* prop = k_Method_Copy; */ break;
      }
    }
    else
    {
      index -= _numXmlItems;
      switch (propID)
      {
        case kpidPath:
        case kpidName:
          if (index < (UInt32)_db.VirtualRoots.Size())
            prop = _db.Images[_db.VirtualRoots[index]].RootName;
          else
            prop = FILES_DIR_NAME;
          break;
        case kpidIsDir: prop = true; break;
        case kpidIsAux: prop = true; break;
      }
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetRootProp(PROPID propID, PROPVARIANT *value))
{
  // COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  if (_db.Images.Size() != 0 && _db.NumExcludededItems != 0)
  {
    const CImage &image = _db.Images[_db.IndexOfUserImage];
    const CItem &item = _db.Items[image.StartItem];
    if (!item.IsDir || item.ImageIndex != _db.IndexOfUserImage)
      return E_FAIL;
    const Byte *metadata = image.Meta + item.Offset;

    switch (propID)
    {
      case kpidIsDir: prop = true; break;
      case kpidAttrib: prop = (UInt32)Get32(metadata + 8); break;
      case kpidCTime: GetFileTime(metadata + (_db.IsOldVersion ? 0x18: 0x28), prop); break;
      case kpidATime: GetFileTime(metadata + (_db.IsOldVersion ? 0x20: 0x30), prop); break;
      case kpidMTime: GetFileTime(metadata + (_db.IsOldVersion ? 0x28: 0x38), prop); break;
    }
  }
  prop.Detach(value);
  return S_OK;
  // COM_TRY_END
}

HRESULT CHandler::GetSecurity(UInt32 realIndex, const void **data, UInt32 *dataSize, UInt32 *propType)
{
  const CItem &item = _db.Items[realIndex];
  if (item.IsAltStream || item.ImageIndex < 0)
    return S_OK;
  const CImage &image = _db.Images[item.ImageIndex];
  const Byte *metadata = image.Meta + item.Offset;
  const UInt32 securId = Get32(metadata + 0xC);
  if (// securId == (UInt32)(Int32)-1 ||
         securId     >= image.SecurOffsets.Size()
      || securId + 1 >= image.SecurOffsets.Size())
    return S_OK;
  const UInt32 offs = image.SecurOffsets[securId];
  const UInt32 len = image.SecurOffsets[securId + 1] - offs;
  const CByteBuffer &buf = image.Meta;
  if (offs <= buf.Size() && buf.Size() - offs >= len)
  {
    *data = buf + offs;
    *dataSize = len;
    *propType = NPropDataType::kRaw;
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRootRawProp(PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;
  if (propID == kpidNtSecure && _db.Images.Size() != 0 && _db.NumExcludededItems != 0)
  {
    const CImage &image = _db.Images[_db.IndexOfUserImage];
    const CItem &item = _db.Items[image.StartItem];
    if (!item.IsDir || item.ImageIndex != _db.IndexOfUserImage)
      return S_OK; // E_FAIL;
    return GetSecurity(image.StartItem, data, dataSize, propType);
  }
  return S_OK;
}

static const Byte kRawProps[] =
{
  kpidSha1,
  kpidNtReparse,
  kpidNtSecure
};


Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *numProps))
{
  *numProps = Z7_ARRAY_SIZE(kRawProps);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *propID))
{
  *propID = kRawProps[index];
  *name = NULL;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType))
{
  *parentType = NParentType::kDir;
  *parent = (UInt32)(Int32)-1;
  if (index >= _db.SortedItems.Size())
    return S_OK;

  const CItem &item = _db.Items[_db.SortedItems[index]];
  
  if (item.ImageIndex >= 0)
  {
    *parentType = item.IsAltStream ? NParentType::kAltStream : NParentType::kDir;
    if (item.Parent >= 0)
    {
      if (_db.ExludedItem != item.Parent)
        *parent = (unsigned)_db.Items[item.Parent].IndexInSorted;
    }
    else
    {
      CImage &image = _db.Images[item.ImageIndex];
      if (image.VirtualRootIndex >= 0)
        *parent = _db.SortedItems.Size() + _numXmlItems + (unsigned)image.VirtualRootIndex;
    }
  }
  else
    *parent = _db.SortedItems.Size() + _numXmlItems + _db.VirtualRoots.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  if (propID == kpidName)
  {
    if (index < _db.SortedItems.Size())
    {
      const CItem &item = _db.Items[_db.SortedItems[index]];
      if (item.ImageIndex < 0)
        return S_OK;
      const CImage &image = _db.Images[item.ImageIndex];
      *propType = NPropDataType::kUtf16z;
      if (image.NumEmptyRootItems != 0 && item.Parent < 0)
      {
        const CByteBuffer &buf = _db.Images[item.ImageIndex].RootNameBuf;
        *data = (void *)(const Byte *)buf;
        *dataSize = (UInt32)buf.Size();
        return S_OK;
      }
      const Byte *meta = image.Meta + item.Offset +
          (item.IsAltStream ?
          (_isOldVersion ? 0x10 : 0x24) :
          (_isOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2));
      *data = (const void *)(meta + 2);
      *dataSize = (UInt32)Get16(meta) + 2;
      return S_OK;
    }
    {
      index -= _db.SortedItems.Size();
      if (index < _numXmlItems)
        return S_OK;
      index -= _numXmlItems;
      if (index >= (UInt32)_db.VirtualRoots.Size())
        return S_OK;
      const CByteBuffer &buf = _db.Images[_db.VirtualRoots[index]].RootNameBuf;
      *data = (void *)(const Byte *)buf;
      *dataSize = (UInt32)buf.Size();
      *propType = NPropDataType::kUtf16z;
      return S_OK;
    }
  }

  if (index >= _db.SortedItems.Size())
    return S_OK;

  unsigned index2 = _db.SortedItems[index];
  
  if (propID == kpidNtSecure)
  {
    return GetSecurity(index2, data, dataSize, propType);
  }
  
  const CItem &item = _db.Items[index2];
  if (propID == kpidSha1)
  {
    if (item.StreamIndex >= 0)
      *data = _db.DataStreams[item.StreamIndex].Hash;
    else
    {
      if (_isOldVersion)
        return S_OK;
      const Byte *sha1 = _db.Images[item.ImageIndex].Meta + item.Offset + (item.IsAltStream ? 0x10 : 0x40);
      if (IsEmptySha(sha1))
        return S_OK;
      *data = sha1;
    }
    *dataSize = kHashSize;
    *propType = NPropDataType::kRaw;
    return S_OK;
  }
  
  if (propID == kpidNtReparse && !_isOldVersion)
  {
    // we don't know about Reparse field in OLD WIM format

    if (item.StreamIndex < 0)
      return S_OK;
    if (index2 >= _db.ItemToReparse.Size())
      return S_OK;
    int reparseIndex = _db.ItemToReparse[index2];
    if (reparseIndex < 0)
      return S_OK;
    const CByteBuffer &buf = _db.ReparseItems[reparseIndex];
    if (buf.Size() == 0)
      return S_OK;
    *data = buf;
    *dataSize = (UInt32)buf.Size();
    *propType = NPropDataType::kRaw;
    return S_OK;
  }

  return S_OK;
}

class CVolumeName
{
  UString _before;
  UString _after;
public:
  void InitName(const UString &name)
  {
    int dotPos = name.ReverseFind_Dot();
    if (dotPos < 0)
      dotPos = (int)name.Len();
    _before.SetFrom(name.Ptr(), (unsigned)dotPos);
    _after = name.Ptr(dotPos);
  }

  UString GetNextName(UInt32 index) const
  {
    UString s = _before;
    s.Add_UInt32(index);
    s += _after;
    return s;
  }
};

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN

  Close();
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    
    CVolumeName seqName;
    if (callback)
      callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);

    UInt32 numVolumes = 1;
    
    for (UInt32 i = 1; i <= numVolumes; i++)
    {
      CMyComPtr<IInStream> curStream;
      
      if (i == 1)
        curStream = inStream;
      else
      {
        if (!openVolumeCallback)
          continue;
        const UString fullName = seqName.GetNextName(i);
        const HRESULT result = openVolumeCallback->GetStream(fullName, &curStream);
        if (result == S_FALSE)
          continue;
        if (result != S_OK)
          return result;
        if (!curStream)
          break;
      }
      
      CHeader header;
      HRESULT res = NWim::ReadHeader(curStream, header, _phySize);
      
      if (res != S_OK)
      {
        if (i != 1 && res == S_FALSE)
          continue;
        return res;
      }
      
      _isArc = true;
      _bootIndex = header.BootIndex;
      _version = header.Version;
      _isOldVersion = header.IsOldVersion();
      if (_firstVolumeIndex >= 0)
        if (!header.AreFromOnArchive(_volumes[_firstVolumeIndex].Header))
          break;
      if (_volumes.Size() > header.PartNumber && _volumes[header.PartNumber].Stream)
        break;
      CWimXml xml;
      xml.VolIndex = header.PartNumber;
      res = _db.OpenXml(curStream, header, xml.Data);
      
      if (res == S_OK)
      {
        if (!xml.Parse())
          _xmlError = true;

        if (xml.IsEncrypted)
        {
          _unsupported = true;
          return S_FALSE;
        }

        UInt64 totalFiles = xml.GetTotalFilesAndDirs() + xml.Images.Size();
        totalFiles += 16 + xml.Images.Size() * 4; // we reserve some additional items
        if (totalFiles >= ((UInt32)1 << 30))
          totalFiles = 0;
        res = _db.Open(curStream, header, (unsigned)totalFiles, callback);
      }
      
      if (res != S_OK)
      {
        if (i != 1 && res == S_FALSE)
          continue;
        return res;
      }
      
      while (_volumes.Size() <= header.PartNumber)
        _volumes.AddNew();
      CVolume &volume = _volumes[header.PartNumber];
      volume.Header = header;
      volume.Stream = curStream;
      
      _firstVolumeIndex = header.PartNumber;
      
      if (_xmls.IsEmpty() || xml.Data != _xmls[0].Data)
      {
        xml.FileName = '[';
        xml.FileName.Add_UInt32(xml.VolIndex);
        xml.FileName += "].xml";
        _xmls.Add(xml);
      }
      
      if (i == 1)
      {
        if (header.PartNumber != 1)
          break;
        if (!openVolumeCallback)
          break;
        numVolumes = header.NumParts;
        {
          NCOM::CPropVariant prop;
          RINOK(openVolumeCallback->GetProperty(kpidName, &prop))
          if (prop.vt != VT_BSTR)
            break;
          seqName.InitName(prop.bstrVal);
        }
      }
    }

    RINOK(_db.FillAndCheck(_volumes))
    int defaultImageIndex = (int)_defaultImageNumber - 1;
    
    bool showImageNumber = (_db.Images.Size() != 1 && defaultImageIndex < 0);
    if (!showImageNumber && _set_use_ShowImageNumber)
      showImageNumber = _set_showImageNumber;

    if (!showImageNumber && _keepMode_ShowImageNumber)
      showImageNumber = true;

    _showImageNumber = showImageNumber;

    RINOK(_db.GenerateSortedItems(defaultImageIndex, showImageNumber))
    RINOK(_db.ExtractReparseStreams(_volumes, callback))

    /*
    wchar_t sz[16];
    ConvertUInt32ToString(_db.DataStreams.Size(), sz);
    _nameLenForStreams = MyStringLen(sz);
    */

    _xmlInComments = !_showImageNumber;
    _numXmlItems = (_xmlInComments ? 0 : _xmls.Size());
    _numIgnoreItems = _db.ThereAreDeletedStreams ? 1 : 0;
  }
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::Close())
{
  _firstVolumeIndex = -1;
  _phySize = 0;
  _db.Clear();
  _volumes.Clear();
  _xmls.Clear();
  // _nameLenForStreams = 0;
  _xmlInComments = false;
  _numXmlItems = 0;
  _numIgnoreItems = 0;
  _xmlError = false;
  _isArc = false;
  _unsupported = false;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);

  if (allFilesMode)
    numItems = _db.SortedItems.Size() + _numXmlItems + _db.VirtualRoots.Size() + _numIgnoreItems;
  if (numItems == 0)
    return S_OK;

  UInt32 i;
  UInt64 totalSize = 0;

  for (i = 0; i < numItems; i++)
  {
    UInt32 index = allFilesMode ? i : indices[i];
    if (index < _db.SortedItems.Size())
    {
      int streamIndex = _db.Items[_db.SortedItems[index]].StreamIndex;
      if (streamIndex >= 0)
      {
        const CStreamInfo &si = _db.DataStreams[streamIndex];
        totalSize += _db.Get_UnpackSize_of_Resource(si.Resource);
      }
    }
    else
    {
      index -= _db.SortedItems.Size();
      if (index < _numXmlItems)
        totalSize += _xmls[index].Data.Size();
    }
  }

  RINOK(extractCallback->SetTotal(totalSize))

  totalSize = 0;
  UInt64 currentItemUnPacked;
  
  int prevSuccessStreamIndex = -1;

  CUnpacker unpacker;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);

  for (i = 0;; i++,
      totalSize += currentItemUnPacked)
  {
    currentItemUnPacked = 0;
    lps->InSize = unpacker.TotalPacked;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;

    UInt32 index = allFilesMode ? i : indices[i];
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;

    CMyComPtr<ISequentialOutStream> realOutStream;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))

    if (index >= _db.SortedItems.Size())
    {
      if (!testMode && !realOutStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode))
      index -= _db.SortedItems.Size();
      if (index < _numXmlItems)
      {
        const CByteBuffer &data = _xmls[index].Data;
        currentItemUnPacked = data.Size();
        if (realOutStream)
        {
          RINOK(WriteStream(realOutStream, (const Byte *)data, data.Size()))
          realOutStream.Release();
        }
      }
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }

    const CItem &item = _db.Items[_db.SortedItems[index]];
    const int streamIndex = item.StreamIndex;
    if (streamIndex < 0)
    {
      if (!item.IsDir)
        if (!testMode && !realOutStream)
          continue;
      RINOK(extractCallback->PrepareOperation(askMode))
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(!item.IsDir && _db.ItemHasStream(item) ?
          NExtract::NOperationResult::kDataError :
          NExtract::NOperationResult::kOK))
      continue;
    }

    const CStreamInfo &si = _db.DataStreams[streamIndex];
    currentItemUnPacked = _db.Get_UnpackSize_of_Resource(si.Resource);
    // currentItemPacked = _db.Get_PackSize_of_Resource(streamIndex);

    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))
    Int32 opRes = NExtract::NOperationResult::kOK;
    
    if (streamIndex != prevSuccessStreamIndex || realOutStream)
    {
      Byte digest[kHashSize];
      const CVolume &vol = _volumes[si.PartNumber];
      const bool needDigest = !si.IsEmptyHash() && !_disable_Sha1Check;
      const HRESULT res = unpacker.Unpack(vol.Stream, si.Resource, vol.Header, &_db,
          realOutStream, lps, needDigest ? digest : NULL);
      
      if (res == S_OK)
      {
        if (!needDigest || memcmp(digest, si.Hash, kHashSize) == 0)
          prevSuccessStreamIndex = streamIndex;
        else
          opRes = NExtract::NOperationResult::kCRCError;
      }
      else if (res == S_FALSE)
        opRes = NExtract::NOperationResult::kDataError;
      else if (res == E_NOTIMPL)
        opRes = NExtract::NOperationResult::kUnsupportedMethod;
      else
        return res;
    }
    
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _db.SortedItems.Size() +
      _numXmlItems +
      _db.VirtualRoots.Size() +
      _numIgnoreItems;
  return S_OK;
}

CHandler::CHandler()
{
  _keepMode_ShowImageNumber = false;
  InitDefaults();
  _xmlError = false;
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  InitDefaults();

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
    else if (name.IsEqualTo("is"))
    {
      RINOK(PROPVARIANT_to_bool(prop, _set_showImageNumber))
      _set_use_ShowImageNumber = true;
    }
    else if (name.IsEqualTo("im"))
    {
      UInt32 image = 9;
      RINOK(ParsePropToUInt32(L"", prop, image))
      _defaultImageNumber = (int)image;
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("mt"))
    {
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("memuse"))
    {
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("crc"))
    {
      name.Delete(0, 3);
      UInt32 crcSize = 1;
      RINOK(ParsePropToUInt32(name, prop, crcSize))
      _disable_Sha1Check = (crcSize == 0);
    }
    else
    {
      bool processed = false;
      RINOK(_timeOptions.Parse(name, prop, processed))
      if (!processed)
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::KeepModeForNextOpen())
{
  _keepMode_ShowImageNumber = _showImageNumber;
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Archive/Wim/WimHandlerOut.cpp ================ */
// WimHandlerOut.cpp

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

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

namespace NArchive {
namespace NWim {

static const unsigned k_NumSubVectors_Bits = 12; // must be <= 16

struct CSortedIndex
{
  CObjectVector<CUIntVector> Vectors;

  CSortedIndex()
  {
    const unsigned k_NumSubVectors = 1 << k_NumSubVectors_Bits;
    Vectors.ClearAndReserve(k_NumSubVectors);
    for (unsigned i = 0; i < k_NumSubVectors; i++)
      Vectors.AddNew();
  }
};

static int AddUniqHash(const CStreamInfo *streams, CSortedIndex &sorted2, const Byte *h, int streamIndexForInsert)
{
  const unsigned hash = (((unsigned)h[0] << 8) | (unsigned)h[1]) >> (16 - k_NumSubVectors_Bits);
  CUIntVector &sorted = sorted2.Vectors[hash];
  unsigned left = 0, right = sorted.Size();
  while (left != right)
  {
    const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
    const unsigned index = sorted[mid];
    const Byte *hash2 = streams[index].Hash;
    
    unsigned i;
    for (i = 0; i < kHashSize; i++)
      if (h[i] != hash2[i])
        break;
    
    if (i == kHashSize)
      return (int)index;
  
    if (h[i] < hash2[i])
      right = mid;
    else
      left = mid + 1;
  }

  if (streamIndexForInsert != -1)
    sorted.Insert(left, (unsigned)streamIndexForInsert);
 
  return -1;
}


struct CAltStream
{
  int UpdateIndex;
  int HashIndex;
  UInt64 Size;
  UString Name;
  bool Skip;

  CAltStream(): UpdateIndex(-1), HashIndex(-1), Skip(false) {}
};


struct CMetaItem
{
  int UpdateIndex;
  int HashIndex;
  
  UInt64 Size;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  UInt64 FileID;
  UInt64 VolID;

  UString Name;
  UString ShortName;

  UInt32 Attrib;
  int SecurityId;       // -1: means no secutity ID
  bool IsDir;
  bool Skip;
  unsigned NumSkipAltStreams;
  CObjectVector<CAltStream> AltStreams;

  CByteBuffer Reparse;

  unsigned GetNumAltStreams() const { return AltStreams.Size() - NumSkipAltStreams; }
  CMetaItem():
        UpdateIndex(-1)
      , HashIndex(-1)
      , Size(0)
      , FileID(0)
      , VolID(0)
      , Attrib(0)
      , SecurityId(-1)
      , IsDir(false)
      , Skip(false)
      , NumSkipAltStreams(0)
  {
    FILETIME_Clear(CTime);
    FILETIME_Clear(ATime);
    FILETIME_Clear(MTime);
  }
};


static int Compare_HardLink_MetaItems(const CMetaItem &a1, const CMetaItem &a2)
{
  if (a1.VolID < a2.VolID) return -1;
  if (a1.VolID > a2.VolID) return 1;
  if (a1.FileID < a2.FileID) return -1;
  if (a1.FileID > a2.FileID) return 1;
  if (a1.Size < a2.Size) return -1;
  if (a1.Size > a2.Size) return 1;
  return ::CompareFileTime(&a1.MTime, &a2.MTime);
}


static int AddToHardLinkList(const CObjectVector<CMetaItem> &metaItems, unsigned indexOfItem, CUIntVector &indexes)
{
  const CMetaItem &mi = metaItems[indexOfItem];
  unsigned left = 0, right = indexes.Size();
  while (left != right)
  {
    const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
    const unsigned index = indexes[mid];
    const int comp = Compare_HardLink_MetaItems(mi, metaItems[index]);
    if (comp == 0)
      return (int)index;
    if (comp < 0)
      right = mid;
    else
      left = mid + 1;
  }
  indexes.Insert(left, indexOfItem);
  return -1;
}


struct CUpdateItem
{
  unsigned CallbackIndex; // index in callback
  
  int MetaIndex;          // index in in MetaItems[]
  
  int AltStreamIndex;     // index in CMetaItem::AltStreams vector
                          // -1: if not alt stream?
  
  int InArcIndex;         // >= 0, if we use OLD Data
                          //   -1, if we use NEW Data
 
  CUpdateItem(): MetaIndex(-1), AltStreamIndex(-1), InArcIndex(-1) {}
};


struct CDir
{
  int MetaIndex;
  CObjectVector<CDir> Dirs;
  CUIntVector Files; // indexes in MetaItems[]

  CDir(): MetaIndex(-1) {}
  unsigned GetNumDirs() const;
  unsigned GetNumFiles() const;
  UInt64 GetTotalSize(const CObjectVector<CMetaItem> &metaItems) const;
  bool FindDir(const CObjectVector<CMetaItem> &items, const UString &name, unsigned &index);
};

/* imagex counts Junctions as files (not as dirs).
   We suppose that it's not correct */

unsigned CDir::GetNumDirs() const
{
  unsigned num = Dirs.Size();
  FOR_VECTOR (i, Dirs)
    num += Dirs[i].GetNumDirs();
  return num;
}

unsigned CDir::GetNumFiles() const
{
  unsigned num = Files.Size();
  FOR_VECTOR (i, Dirs)
    num += Dirs[i].GetNumFiles();
  return num;
}

UInt64 CDir::GetTotalSize(const CObjectVector<CMetaItem> &metaItems) const
{
  UInt64 sum = 0;
  unsigned i;
  for (i = 0; i < Files.Size(); i++)
    sum += metaItems[Files[i]].Size;
  for (i = 0; i < Dirs.Size(); i++)
    sum += Dirs[i].GetTotalSize(metaItems);
  return sum;
}

bool CDir::FindDir(const CObjectVector<CMetaItem> &items, const UString &name, unsigned &index)
{
  unsigned left = 0, right = Dirs.Size();
  while (left != right)
  {
    const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
    const int comp = CompareFileNames(name, items[Dirs[mid].MetaIndex].Name);
    if (comp == 0)
    {
      index = mid;
      return true;
    }
    if (comp < 0)
      right = mid;
    else
      left = mid + 1;
  }
  index = left;
  return false;
}


Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *type))
{
  *type = NFileTimeType::kWindows;
  return S_OK;
}


HRESULT CHandler::GetOutProperty(IArchiveUpdateCallback *callback, UInt32 callbackIndex, Int32 arcIndex, PROPID propID, PROPVARIANT *value)
{
  if (arcIndex != -1)
    return GetProperty((UInt32)arcIndex, propID, value);
  return callback->GetProperty(callbackIndex, propID, value);
}


HRESULT CHandler::GetTime(IArchiveUpdateCallback *callback, UInt32 callbackIndex, Int32 arcIndex, PROPID propID, FILETIME &ft)
{
  ft.dwLowDateTime = ft.dwHighDateTime = 0;
  NCOM::CPropVariant prop;
  RINOK(GetOutProperty(callback, callbackIndex, arcIndex, propID, &prop))
  if (prop.vt == VT_FILETIME)
    ft = prop.filetime;
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  return S_OK;
}


static HRESULT GetRootTime(
    IArchiveGetRootProps *callback,
    IArchiveGetRootProps *arcRoot,
    PROPID propID, FILETIME &ft)
{
  NCOM::CPropVariant prop;
  if (callback)
  {
    RINOK(callback->GetRootProp(propID, &prop))
    if (prop.vt == VT_FILETIME)
    {
      ft = prop.filetime;
      return S_OK;
    }
    if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
  }
  if (arcRoot)
  {
    RINOK(arcRoot->GetRootProp(propID, &prop))
    if (prop.vt == VT_FILETIME)
    {
      ft = prop.filetime;
      return S_OK;
    }
    if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
  }
  return S_OK;
}

#define Set16(p, d) SetUi16(p, d)
#define Set32(p, d) SetUi32(p, d)
#define Set64(p, d) SetUi64(p, d)

void CResource::WriteTo(Byte *p) const
{
  Set64(p, PackSize)
  p[7] = Flags;
  Set64(p + 8, Offset)
  Set64(p + 16, UnpackSize)
}


void CHeader::WriteTo(Byte *p) const
{
  memcpy(p, kSignature, kSignatureSize);
  Set32(p + 8, kHeaderSizeMax)
  Set32(p + 0xC, Version)
  Set32(p + 0x10, Flags)
  Set32(p + 0x14, ChunkSize)
  memcpy(p + 0x18, Guid, 16);
  Set16(p + 0x28, PartNumber)
  Set16(p + 0x2A, NumParts)
  Set32(p + 0x2C, NumImages)
  OffsetResource.WriteTo(p + 0x30);
  XmlResource.WriteTo(p + 0x48);
  MetadataResource.WriteTo(p + 0x60);
  IntegrityResource.WriteTo(p + 0x7C);
  Set32(p + 0x78, BootIndex)
  memset(p + 0x94, 0, 60);
}


void CStreamInfo::WriteTo(Byte *p) const
{
  Resource.WriteTo(p);
  Set16(p + 0x18, PartNumber)
  Set32(p + 0x1A, RefCount)
  memcpy(p + 0x1E, Hash, kHashSize);
}


static void SetFileTimeToMem(Byte *p, const FILETIME &ft)
{
  Set32(p, ft.dwLowDateTime)
  Set32(p + 4, ft.dwHighDateTime)
}

static size_t WriteItem_Dummy(const CMetaItem &item)
{
  if (item.Skip)
    return 0;
  unsigned fileNameLen = item.Name.Len() * 2;
  // we write fileNameLen + 2 + 2 to be same as original WIM.
  unsigned fileNameLen2 = (fileNameLen == 0 ? 0 : fileNameLen + 2);

  const unsigned shortNameLen = item.ShortName.Len() * 2;
  const unsigned shortNameLen2 = (shortNameLen == 0 ? 2 : shortNameLen + 4);

  size_t totalLen = ((kDirRecordSize + fileNameLen2 + shortNameLen2 + 6) & ~(unsigned)7);
  if (item.GetNumAltStreams() != 0)
  {
    if (!item.IsDir)
    {
      const UInt32 curLen = (((0x26 + 0) + 6) & ~(unsigned)7);
      totalLen += curLen;
    }
    FOR_VECTOR (i, item.AltStreams)
    {
      const CAltStream &ss = item.AltStreams[i];
      if (ss.Skip)
        continue;
      fileNameLen = ss.Name.Len() * 2;
      fileNameLen2 = (fileNameLen == 0 ? 0 : fileNameLen + 2 + 2);
      const UInt32 curLen = (((0x26 + fileNameLen2) + 6) & ~(unsigned)7);
      totalLen += curLen;
    }
  }
  return totalLen;
}


static size_t WriteItem(const CStreamInfo *streams, const CMetaItem &item, Byte *p)
{
  if (item.Skip)
    return 0;
  unsigned fileNameLen = item.Name.Len() * 2;
  unsigned fileNameLen2 = (fileNameLen == 0 ? 0 : fileNameLen + 2);
  unsigned shortNameLen = item.ShortName.Len() * 2;
  unsigned shortNameLen2 = (shortNameLen == 0 ? 2 : shortNameLen + 4);

  size_t totalLen = ((kDirRecordSize + fileNameLen2 + shortNameLen2 + 6) & ~(unsigned)7);
  
  memset(p, 0, totalLen);
  Set64(p, totalLen)
  Set64(p + 8, item.Attrib)
  Set32(p + 0xC, (UInt32)(Int32)item.SecurityId)
  SetFileTimeToMem(p + 0x28, item.CTime);
  SetFileTimeToMem(p + 0x30, item.ATime);
  SetFileTimeToMem(p + 0x38, item.MTime);
  
  /* WIM format probably doesn't support hard links to symbolic links.
     In these cases it just stores symbolic links (REPARSE TAGS).
     Check it in new versions of WIM software form MS !!!
     We also follow that scheme */

  if (item.Reparse.Size() != 0)
  {
    UInt32 tag = GetUi32(item.Reparse);
    Set32(p + 0x58, tag)
    // Set32(p + 0x5C, 0); // probably it's always ZERO
  }
  else if (item.FileID != 0)
  {
    Set64(p + 0x58, item.FileID)
  }
  
  Set16(p + 0x62, (UInt16)shortNameLen)
  Set16(p + 0x64, (UInt16)fileNameLen)
  unsigned i;
  for (i = 0; i * 2 < fileNameLen; i++)
    Set16(p + kDirRecordSize + i * 2, (UInt16)item.Name[i])
  for (i = 0; i * 2 < shortNameLen; i++)
    Set16(p + kDirRecordSize + fileNameLen2 + i * 2, (UInt16)item.ShortName[i])
  
  if (item.GetNumAltStreams() == 0)
  {
    if (item.HashIndex >= 0)
      memcpy(p + 0x40, streams[item.HashIndex].Hash, kHashSize);
  }
  else
  {
    Set16(p + 0x60, (UInt16)(item.GetNumAltStreams() + (item.IsDir ? 0 : 1)))
    p += totalLen;
    
    if (!item.IsDir)
    {
      const UInt32 curLen = (((0x26 + 0) + 6) & ~(unsigned)7);
      memset(p, 0, curLen);
      Set64(p, curLen)
      if (item.HashIndex >= 0)
        memcpy(p + 0x10, streams[item.HashIndex].Hash, kHashSize);
      totalLen += curLen;
      p += curLen;
    }
    
    FOR_VECTOR (si, item.AltStreams)
    {
      const CAltStream &ss = item.AltStreams[si];
      if (ss.Skip)
        continue;
      
      fileNameLen = ss.Name.Len() * 2;
      fileNameLen2 = (fileNameLen == 0 ? 0 : fileNameLen + 2 + 2);
      UInt32 curLen = (((0x26 + fileNameLen2) + 6) & ~(unsigned)7);
      memset(p, 0, curLen);
      
      Set64(p, curLen)
      if (ss.HashIndex >= 0)
        memcpy(p + 0x10, streams[ss.HashIndex].Hash, kHashSize);
      Set16(p + 0x24, (UInt16)fileNameLen)
      for (i = 0; i * 2 < fileNameLen; i++)
        Set16(p + 0x26 + i * 2, (UInt16)ss.Name[i])
      totalLen += curLen;
      p += curLen;
    }
  }
  
  return totalLen;
}


struct CDb
{
  CMetaItem DefaultDirItem;
  const CStreamInfo *Hashes;
  CObjectVector<CMetaItem> MetaItems;
  CRecordVector<CUpdateItem> UpdateItems;
  CUIntVector UpdateIndexes; /* indexes in UpdateItems in order of writing data streams
                                to disk (the order of tree items). */

  size_t WriteTree_Dummy(const CDir &tree) const;
  void WriteTree(const CDir &tree, Byte *dest, size_t &pos)  const;
  void WriteOrderList(const CDir &tree);
};


size_t CDb::WriteTree_Dummy(const CDir &tree) const
{
  unsigned i;
  size_t pos = 0;
  for (i = 0; i < tree.Files.Size(); i++)
    pos += WriteItem_Dummy(MetaItems[tree.Files[i]]);
  for (i = 0; i < tree.Dirs.Size(); i++)
  {
    const CDir &subDir = tree.Dirs[i];
    pos += WriteItem_Dummy(MetaItems[subDir.MetaIndex]);
    pos += WriteTree_Dummy(subDir);
  }
  return pos + 8;
}


void CDb::WriteTree(const CDir &tree, Byte *dest, size_t &pos) const
{
  unsigned i;
  for (i = 0; i < tree.Files.Size(); i++)
    pos += WriteItem(Hashes, MetaItems[tree.Files[i]], dest + pos);

  size_t posStart = pos;
  for (i = 0; i < tree.Dirs.Size(); i++)
    pos += WriteItem_Dummy(MetaItems[tree.Dirs[i].MetaIndex]);

  Set64(dest + pos, 0)

  pos += 8;

  for (i = 0; i < tree.Dirs.Size(); i++)
  {
    const CDir &subDir = tree.Dirs[i];
    const CMetaItem &metaItem = MetaItems[subDir.MetaIndex];
    bool needCreateTree = (metaItem.Reparse.Size() == 0)
        || !subDir.Files.IsEmpty()
        || !subDir.Dirs.IsEmpty();
    size_t len = WriteItem(Hashes, metaItem, dest + posStart);
    posStart += len;
    if (needCreateTree)
    {
      Set64(dest + posStart - len + 0x10, pos) // subdirOffset
      WriteTree(subDir, dest, pos);
    }
  }
}


void CDb::WriteOrderList(const CDir &tree)
{
  if (tree.MetaIndex >= 0)
  {
    const CMetaItem &mi = MetaItems[tree.MetaIndex];
    if (mi.UpdateIndex >= 0)
      UpdateIndexes.Add((unsigned)mi.UpdateIndex);
    FOR_VECTOR (si, mi.AltStreams)
      UpdateIndexes.Add((unsigned)mi.AltStreams[si].UpdateIndex);
  }

  unsigned i;
  for (i = 0; i < tree.Files.Size(); i++)
  {
    const CMetaItem &mi = MetaItems[tree.Files[i]];
    UpdateIndexes.Add((unsigned)mi.UpdateIndex);
    FOR_VECTOR (si, mi.AltStreams)
      UpdateIndexes.Add((unsigned)mi.AltStreams[si].UpdateIndex);
  }

  for (i = 0; i < tree.Dirs.Size(); i++)
    WriteOrderList(tree.Dirs[i]);
}


static void AddTag_ToString(AString &s, const char *name, const char *value)
{
  s.Add_Char('<');
  s += name;
  s.Add_Char('>');
  s += value;
  s.Add_Char('<');
  s.Add_Slash();
  s += name;
  s.Add_Char('>');
}


static void AddTagUInt64_ToString(AString &s, const char *name, UInt64 value)
{
  char temp[32];
  ConvertUInt64ToString(value, temp);
  AddTag_ToString(s, name, temp);
}


static CXmlItem &AddUniqueTag(CXmlItem &parentItem, const char *name)
{
  const int index = parentItem.FindSubTag(name);
  if (index < 0)
  {
    CXmlItem &subItem = parentItem.SubItems.AddNew();
    subItem.IsTag = true;
    subItem.Name = name;
    return subItem;
  }
  CXmlItem &subItem = parentItem.SubItems[index];
  subItem.SubItems.Clear();
  return subItem;
}


static void AddTag_UInt64_2(CXmlItem &item, UInt64 value)
{
  CXmlItem &subItem = item.SubItems.AddNew();
  subItem.IsTag = false;
  char temp[32];
  ConvertUInt64ToString(value, temp);
  subItem.Name = temp;
}


static void AddTag_UInt64(CXmlItem &parentItem, const char *name, UInt64 value)
{
  AddTag_UInt64_2(AddUniqueTag(parentItem, name), value);
}


static void AddTag_Hex(CXmlItem &item, const char *name, UInt32 value)
{
  item.IsTag = true;
  item.Name = name;
  char temp[16];
  temp[0] = '0';
  temp[1] = 'x';
  ConvertUInt32ToHex8Digits(value, temp + 2);
  CXmlItem &subItem = item.SubItems.AddNew();
  subItem.IsTag = false;
  subItem.Name = temp;
}


static void AddTag_Time_2(CXmlItem &item, const FILETIME &ft)
{
  AddTag_Hex(item.SubItems.AddNew(), "HIGHPART", ft.dwHighDateTime);
  AddTag_Hex(item.SubItems.AddNew(), "LOWPART", ft.dwLowDateTime);
}


static void AddTag_Time(CXmlItem &parentItem, const char *name, const FILETIME &ft)
{
  AddTag_Time_2(AddUniqueTag(parentItem, name), ft);
}


static void AddTag_String_IfEmpty(CXmlItem &parentItem, const char *name, const char *value)
{
  if (parentItem.FindSubTag(name) >= 0)
    return;
  CXmlItem &tag = parentItem.SubItems.AddNew();
  tag.IsTag = true;
  tag.Name = name;
  CXmlItem &subItem = tag.SubItems.AddNew();
  subItem.IsTag = false;
  subItem.Name = value;
}


void CHeader::SetDefaultFields(bool useLZX)
{
  Version = k_Version_NonSolid;
  Flags = NHeaderFlags::kReparsePointFixup;
  ChunkSize = 0;
  if (useLZX)
  {
    Flags |= NHeaderFlags::kCompression | NHeaderFlags::kLZX;
    ChunkSize = kChunkSize;
    ChunkSizeBits = kChunkSizeBits;
  }
  MY_RAND_GEN(Guid, 16);
  PartNumber = 1;
  NumParts = 1;
  NumImages = 1;
  BootIndex = 0;
  OffsetResource.Clear();
  XmlResource.Clear();
  MetadataResource.Clear();
  IntegrityResource.Clear();
}


static void AddTrees(CObjectVector<CDir> &trees, CObjectVector<CMetaItem> &metaItems, const CMetaItem &ri, int curTreeIndex)
{
  while (curTreeIndex >= (int)trees.Size())
    trees.AddNew().Dirs.AddNew().MetaIndex = (int)metaItems.Add(ri);
}


#define IS_LETTER_CHAR(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))



Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outSeqStream, UInt32 numItems, IArchiveUpdateCallback *callback))
{
  COM_TRY_BEGIN

  if (!IsUpdateSupported())
    return E_NOTIMPL;

  bool isUpdate = (_volumes.Size() != 0);
  int defaultImageIndex = _defaultImageNumber - 1;
  bool showImageNumber;

  if (isUpdate)
  {
    showImageNumber = _showImageNumber;
    if (!showImageNumber)
      defaultImageIndex = _db.IndexOfUserImage;
  }
  else
  {
    showImageNumber = (_set_use_ShowImageNumber && _set_showImageNumber);
    if (!showImageNumber)
      defaultImageIndex = 0;
  }

  if (defaultImageIndex >= kNumImagesMaxUpdate)
    return E_NOTIMPL;

  CMyComPtr<IOutStream> outStream;
  RINOK(outSeqStream->QueryInterface(IID_IOutStream, (void **)&outStream))
  if (!outStream)
    return E_NOTIMPL;
  if (!callback)
    return E_FAIL;

  CDb db;
  CObjectVector<CDir> trees;

  CMetaItem ri; // default DIR item
  FILETIME ftCur;
  NTime::GetCurUtcFileTime(ftCur);
  // ri.MTime = ri.ATime = ri.CTime = ftCur;
  ri.Attrib = FILE_ATTRIBUTE_DIRECTORY;
  ri.IsDir = true;


  // ---------- Detect changed images ----------

  unsigned i;
  CBoolVector isChangedImage;
  {
    CUIntVector numUnchangedItemsInImage;
    for (i = 0; i < _db.Images.Size(); i++)
    {
      numUnchangedItemsInImage.Add(0);
      isChangedImage.Add(false);
    }
    
    for (i = 0; i < numItems; i++)
    {
      UInt32 indexInArchive;
      Int32 newData, newProps;
      RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArchive))
      if (newProps == 0)
      {
        if (indexInArchive >= _db.SortedItems.Size())
          continue;
        const CItem &item = _db.Items[_db.SortedItems[indexInArchive]];
        if (newData == 0)
        {
          if (item.ImageIndex >= 0)
            numUnchangedItemsInImage[item.ImageIndex]++;
        }
        else
        {
          // oldProps & newData. Current version of 7-Zip doesn't use it
          if (item.ImageIndex >= 0)
            isChangedImage[item.ImageIndex] = true;
        }
      }
      else if (!showImageNumber)
      {
        if (defaultImageIndex >= 0 && defaultImageIndex < (int)isChangedImage.Size())
          isChangedImage[defaultImageIndex] = true;
      }
      else
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidPath, &prop))
        
        if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        const wchar_t *path = prop.bstrVal;
        if (!path)
          return E_INVALIDARG;

        const wchar_t *end;
        UInt64 val = ConvertStringToUInt64(path, &end);
        if (end == path)
          return E_INVALIDARG;
        if (val == 0 || val > kNumImagesMaxUpdate)
          return E_INVALIDARG;
        wchar_t c = *end;
        if (c != 0 && c != ':' && c != L'/' && c != WCHAR_PATH_SEPARATOR)
          return E_INVALIDARG;
        unsigned imageIndex = (unsigned)val - 1;
        if (imageIndex < _db.Images.Size())
          isChangedImage[imageIndex] = true;
        if (_defaultImageNumber > 0 && val != (unsigned)_defaultImageNumber)
          return E_INVALIDARG;
      }
    }
    
    for (i = 0; i < _db.Images.Size(); i++)
      if (!isChangedImage[i])
        isChangedImage[i] = _db.GetNumUserItemsInImage(i) != numUnchangedItemsInImage[i];
  }

  if (defaultImageIndex >= 0)
  {
    for (i = 0; i < _db.Images.Size(); i++)
      if ((int)i != defaultImageIndex)
        isChangedImage[i] = false;
  }

  CMyComPtr<IArchiveGetRawProps> getRawProps;
  callback->QueryInterface(IID_IArchiveGetRawProps, (void **)&getRawProps);

  CMyComPtr<IArchiveGetRootProps> getRootProps;
  callback->QueryInterface(IID_IArchiveGetRootProps, (void **)&getRootProps);

  CObjectVector<CUniqBlocks> secureBlocks;

  if (!showImageNumber && (getRootProps || isUpdate) &&
      (
        defaultImageIndex >= (int)isChangedImage.Size()
        || defaultImageIndex < 0 // test it
        || isChangedImage[defaultImageIndex]
      ))
  {
    // Fill Root Item: Metadata and security
    CMetaItem rootItem = ri;
    {
      const void *data = NULL;
      UInt32 dataSize = 0;
      UInt32 propType = 0;
      if (getRootProps)
      {
        RINOK(getRootProps->GetRootRawProp(kpidNtSecure, &data, &dataSize, &propType))
      }
      if (dataSize == 0 && isUpdate)
      {
        RINOK(GetRootRawProp(kpidNtSecure, &data, &dataSize, &propType))
      }
      if (dataSize != 0)
      {
        if (propType != NPropDataType::kRaw)
          return E_FAIL;
        while (defaultImageIndex >= (int)secureBlocks.Size())
          secureBlocks.AddNew();
        CUniqBlocks &secUniqBlocks = secureBlocks[defaultImageIndex];
        rootItem.SecurityId = (int)secUniqBlocks.AddUniq((const Byte *)data, dataSize);
      }
    }
    
    IArchiveGetRootProps *thisGetRoot = isUpdate ? this : NULL;
    
    if (_timeOptions.Write_CTime.Val) RINOK(GetRootTime(getRootProps, thisGetRoot, kpidCTime, rootItem.CTime))
    if (_timeOptions.Write_ATime.Val) RINOK(GetRootTime(getRootProps, thisGetRoot, kpidATime, rootItem.ATime))
    if (_timeOptions.Write_MTime.Val) RINOK(GetRootTime(getRootProps, thisGetRoot, kpidMTime, rootItem.MTime))
    
    {
      NCOM::CPropVariant prop;
      if (getRootProps)
      {
        RINOK(getRootProps->GetRootProp(kpidAttrib, &prop))
        if (prop.vt == VT_UI4)
          rootItem.Attrib = prop.ulVal;
        else if (prop.vt != VT_EMPTY)
          return E_INVALIDARG;
      }
      if (prop.vt == VT_EMPTY && thisGetRoot)
      {
        RINOK(GetRootProp(kpidAttrib, &prop))
        if (prop.vt == VT_UI4)
          rootItem.Attrib = prop.ulVal;
        else if (prop.vt != VT_EMPTY)
          return E_INVALIDARG;
      }
      rootItem.Attrib |= FILE_ATTRIBUTE_DIRECTORY;
    }
    
    AddTrees(trees, db.MetaItems, ri, defaultImageIndex);
    db.MetaItems[trees[defaultImageIndex].Dirs[0].MetaIndex] = rootItem;
  }

  // ---------- Request Metadata for changed items ----------

  UString fileName;
  
  for (i = 0; i < numItems; i++)
  {
    CUpdateItem ui;
    UInt32 indexInArchive;
    Int32 newData, newProps;
    RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArchive))

    if (newData == 0 || newProps == 0)
    {
      if (indexInArchive >= _db.SortedItems.Size())
        continue;
      
      const CItem &item = _db.Items[_db.SortedItems[indexInArchive]];
      
      if (item.ImageIndex >= 0)
      {
        if (!isChangedImage[item.ImageIndex])
        {
          if (newData == 0 && newProps == 0)
            continue;
          return E_FAIL;
        }
      }
      else
      {
        // if deleted item was not renamed, we just skip it
        if (newProps == 0)
          continue;
        if (item.StreamIndex >= 0)
        {
          // we don't support property change for SolidBig streams
          if (_db.DataStreams[item.StreamIndex].Resource.IsSolidBig())
            return E_NOTIMPL;
        }
      }
    
      if (newData == 0)
        ui.InArcIndex = (Int32)indexInArchive;
    }

    // we set arcIndex only if we must use old props
    const Int32 arcIndex = (newProps ? -1 : (Int32)indexInArchive);

    bool isDir = false;
    {
      NCOM::CPropVariant prop;
      RINOK(GetOutProperty(callback, i, arcIndex, kpidIsDir, &prop))
      if (prop.vt == VT_BOOL)
        isDir = (prop.boolVal != VARIANT_FALSE);
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }

    bool isAltStream = false;
    {
      NCOM::CPropVariant prop;
      RINOK(GetOutProperty(callback, i, arcIndex, kpidIsAltStream, &prop))
      if (prop.vt == VT_BOOL)
        isAltStream = (prop.boolVal != VARIANT_FALSE);
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }

    if (isDir && isAltStream)
      return E_INVALIDARG;

    UInt64 size = 0;
    UInt64 iNode = 0;

    if (!isDir)
    {
      if (!newData)
      {
        NCOM::CPropVariant prop;
        GetProperty(indexInArchive, kpidINode, &prop);
        if (prop.vt == VT_UI8)
          iNode = prop.uhVal.QuadPart;
      }

      NCOM::CPropVariant prop;
      
      if (newData)
      {
        RINOK(callback->GetProperty(i, kpidSize, &prop))
      }
      else
      {
        RINOK(GetProperty(indexInArchive, kpidSize, &prop))
      }
     
      if (prop.vt == VT_UI8)
        size = prop.uhVal.QuadPart;
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }

    {
      NCOM::CPropVariant propPath;
      const wchar_t *path = NULL;
      RINOK(GetOutProperty(callback, i, arcIndex, kpidPath, &propPath))
      if (propPath.vt == VT_BSTR)
        path = propPath.bstrVal;
      else if (propPath.vt != VT_EMPTY)
        return E_INVALIDARG;
    
    if (!path)
      return E_INVALIDARG;

    CDir *curItem = NULL;
    bool isRootImageDir = false;
    fileName.Empty();

    int imageIndex;
    
    if (!showImageNumber)
    {
      imageIndex = defaultImageIndex;
      AddTrees(trees, db.MetaItems, ri, imageIndex);
      curItem = &trees[imageIndex].Dirs[0];
    }
    else
    {
      const wchar_t *end;
      UInt64 val = ConvertStringToUInt64(path, &end);
      if (end == path)
        return E_INVALIDARG;
      if (val == 0 || val > kNumImagesMaxUpdate)
        return E_INVALIDARG;
      
      imageIndex = (int)val - 1;
      if (imageIndex < (int)isChangedImage.Size())
       if (!isChangedImage[imageIndex])
          return E_FAIL;

      AddTrees(trees, db.MetaItems, ri, imageIndex);
      curItem = &trees[imageIndex].Dirs[0];
      wchar_t c = *end;
      
      if (c == 0)
      {
        if (!isDir || isAltStream)
          return E_INVALIDARG;
        ui.MetaIndex = curItem->MetaIndex;
        isRootImageDir = true;
      }
      else if (c == ':')
      {
        if (isDir || !isAltStream)
          return E_INVALIDARG;
        ui.MetaIndex = curItem->MetaIndex;
        CAltStream ss;
        ss.Size = size;
        ss.Name = end + 1;
        ss.UpdateIndex = (int)db.UpdateItems.Size();
        ui.AltStreamIndex = (int)db.MetaItems[ui.MetaIndex].AltStreams.Add(ss);
      }
      else if (c == WCHAR_PATH_SEPARATOR || c == L'/')
      {
        path = end + 1;
        if (*path == 0)
          return E_INVALIDARG;
      }
      else
        return E_INVALIDARG;
    }
      
    if (ui.MetaIndex < 0)
    {
      for (;;)
      {
        const wchar_t c = *path++;
        if (c == 0)
          break;
        if (c == WCHAR_PATH_SEPARATOR || c == L'/')
        {
          unsigned indexOfDir;
          if (!curItem->FindDir(db.MetaItems, fileName, indexOfDir))
          {
            CDir &dir = curItem->Dirs.InsertNew(indexOfDir);
            dir.MetaIndex = (int)db.MetaItems.Add(ri);
            db.MetaItems.Back().Name = fileName;
          }
          curItem = &curItem->Dirs[indexOfDir];
          fileName.Empty();
        }
        else
        {
          /*
          #if WCHAR_MAX > 0xffff
          if (c >= 0x10000)
          {
            c -= 0x10000;

            if (c < (1 << 20))
            {
              wchar_t c0 = 0xd800 + ((c >> 10) & 0x3FF);
              fileName += c0;
              c = 0xdc00 + (c & 0x3FF);
            }
            else
              c = '_'; // we change character unsupported by UTF16
          }
          #endif
          */

          fileName += c;
        }
      }

      if (isAltStream)
      {
        int colonPos = fileName.Find(L':');
        if (colonPos < 0)
          return E_INVALIDARG;
        
        // we want to support cases of c::substream, where c: is drive name
        if (colonPos == 1 && fileName[2] == L':' && IS_LETTER_CHAR(fileName[0]))
          colonPos = 2;
        const UString mainName = fileName.Left((unsigned)colonPos);
        unsigned indexOfDir;
        
        if (mainName.IsEmpty())
          ui.MetaIndex = curItem->MetaIndex;
        else if (curItem->FindDir(db.MetaItems, mainName, indexOfDir))
          ui.MetaIndex = curItem->Dirs[indexOfDir].MetaIndex;
        else
        {
          for (int j = (int)curItem->Files.Size() - 1; j >= 0; j--)
          {
            const unsigned metaIndex = curItem->Files[j];
            const CMetaItem &mi = db.MetaItems[metaIndex];
            if (CompareFileNames(mainName, mi.Name) == 0)
            {
              ui.MetaIndex = (int)metaIndex;
              break;
            }
          }
        }
        
        if (ui.MetaIndex >= 0)
        {
          CAltStream ss;
          ss.Size = size;
          ss.Name = fileName.Ptr(colonPos + 1);
          ss.UpdateIndex = (int)db.UpdateItems.Size();
          ui.AltStreamIndex = (int)db.MetaItems[ui.MetaIndex].AltStreams.Add(ss);
        }
      }
    }


    if (ui.MetaIndex < 0 || isRootImageDir)
    {
      if (!isRootImageDir)
      {
        ui.MetaIndex = (int)db.MetaItems.Size();
        db.MetaItems.AddNew();
      }
    
      CMetaItem &mi = db.MetaItems[ui.MetaIndex];
      mi.Size = size;
      mi.IsDir = isDir;
      mi.Name = fileName;
      mi.UpdateIndex = (int)db.UpdateItems.Size();
      {
        NCOM::CPropVariant prop;
        RINOK(GetOutProperty(callback, i, arcIndex, kpidAttrib, &prop))
        if (prop.vt == VT_EMPTY)
          mi.Attrib = 0;
        else if (prop.vt == VT_UI4)
          mi.Attrib = prop.ulVal;
        else
          return E_INVALIDARG;
        if (isDir)
          mi.Attrib |= FILE_ATTRIBUTE_DIRECTORY;
      }

      if (arcIndex != -1 || _timeOptions.Write_CTime.Val)
        RINOK(GetTime(callback, i, arcIndex, kpidCTime, mi.CTime))
      if (arcIndex != -1 || _timeOptions.Write_ATime.Val)
        RINOK(GetTime(callback, i, arcIndex, kpidATime, mi.ATime))
      if (arcIndex != -1 || _timeOptions.Write_MTime.Val)
        RINOK(GetTime(callback, i, arcIndex, kpidMTime, mi.MTime))

      {
        NCOM::CPropVariant prop;
        RINOK(GetOutProperty(callback, i, arcIndex, kpidShortName, &prop))
        if (prop.vt == VT_BSTR)
          mi.ShortName.SetFromBstr(prop.bstrVal);
        else if (prop.vt != VT_EMPTY)
          return E_INVALIDARG;
      }

      while (imageIndex >= (int)secureBlocks.Size())
        secureBlocks.AddNew();
      
      if (!isAltStream && (getRawProps || arcIndex >= 0))
      {
        CUniqBlocks &secUniqBlocks = secureBlocks[imageIndex];
        const void *data;
        UInt32 dataSize;
        UInt32 propType;
        
        data = NULL;
        dataSize = 0;
        propType = 0;
        
        if (arcIndex >= 0)
        {
          GetRawProp((UInt32)arcIndex, kpidNtSecure, &data, &dataSize, &propType);
        }
        else
        {
          getRawProps->GetRawProp(i, kpidNtSecure, &data, &dataSize, &propType);
        }
        
        if (dataSize != 0)
        {
          if (propType != NPropDataType::kRaw)
            return E_FAIL;
          mi.SecurityId = (int)secUniqBlocks.AddUniq((const Byte *)data, dataSize);
        }

        data = NULL;
        dataSize = 0;
        propType = 0;
        
        if (arcIndex >= 0)
        {
          GetRawProp((UInt32)arcIndex, kpidNtReparse, &data, &dataSize, &propType);
        }
        else
        {
          getRawProps->GetRawProp(i, kpidNtReparse, &data, &dataSize, &propType);
        }
      
        if (dataSize != 0)
        {
          if (propType != NPropDataType::kRaw)
            return E_FAIL;
          mi.Reparse.CopyFrom((const Byte *)data, dataSize);
        }
      }

      if (!isRootImageDir)
      {
        if (isDir)
        {
          unsigned indexOfDir;
          if (curItem->FindDir(db.MetaItems, fileName, indexOfDir))
            curItem->Dirs[indexOfDir].MetaIndex = ui.MetaIndex;
          else
            curItem->Dirs.InsertNew(indexOfDir).MetaIndex = ui.MetaIndex;
        }
        else
          curItem->Files.Add((unsigned)ui.MetaIndex);
      }
    }
    
    }
    
    if (iNode != 0 && ui.MetaIndex >= 0 && ui.AltStreamIndex < 0)
      db.MetaItems[ui.MetaIndex].FileID = iNode;

    ui.CallbackIndex = i;
    db.UpdateItems.Add(ui);
  }

  unsigned numNewImages = trees.Size();
  for (i = numNewImages; i < isChangedImage.Size(); i++)
    if (!isChangedImage[i])
      numNewImages = i + 1;

  AddTrees(trees, db.MetaItems, ri, (int)numNewImages - 1);

  for (i = 0; i < trees.Size(); i++)
    if (i >= isChangedImage.Size() || isChangedImage[i])
      db.WriteOrderList(trees[i]);


  UInt64 complexity = 0;

  unsigned numDataStreams = _db.DataStreams.Size();
  CUIntArr streamsRefs(numDataStreams);
  for (i = 0; i < numDataStreams; i++)
    streamsRefs[i] = 0;

  // ---------- Calculate Streams Refs Counts in unchanged images

  for (i = 0; i < _db.Images.Size(); i++)
  {
    if (isChangedImage[i])
      continue;
    complexity += _db.MetaStreams[i].Resource.PackSize;
    const CImage &image = _db.Images[i];
    unsigned endItem = image.StartItem + image.NumItems;
    for (unsigned k = image.StartItem; k < endItem; k++)
    {
      const CItem &item = _db.Items[k];
      if (item.StreamIndex >= 0)
        streamsRefs[(unsigned)item.StreamIndex]++;
    }
  }


  // ---------- Update Streams Refs Counts in changed images

  for (i = 0; i < db.UpdateIndexes.Size(); i++)
  {
    const CUpdateItem &ui = db.UpdateItems[db.UpdateIndexes[i]];
    
    if (ui.InArcIndex >= 0)
    {
      if ((unsigned)ui.InArcIndex >= _db.SortedItems.Size())
        continue;
      const CItem &item = _db.Items[_db.SortedItems[ui.InArcIndex]];
      if (item.StreamIndex >= 0)
        streamsRefs[(unsigned)item.StreamIndex]++;
    }
    else
    {
      const CMetaItem &mi = db.MetaItems[ui.MetaIndex];
      UInt64 size;
      if (ui.AltStreamIndex < 0)
        size = mi.Size;
      else
        size = mi.AltStreams[ui.AltStreamIndex].Size;
      complexity += size;
    }
  }

  // Clear ref counts for SolidBig streams
  
  for (i = 0; i < _db.DataStreams.Size(); i++)
    if (_db.DataStreams[i].Resource.IsSolidBig())
      streamsRefs[i] = 0;

  // Set ref counts for SolidBig streams
  
  for (i = 0; i < _db.DataStreams.Size(); i++)
    if (streamsRefs[i] != 0)
    {
      const CResource &rs = _db.DataStreams[i].Resource;
      if (rs.IsSolidSmall())
        streamsRefs[_db.Solids[rs.SolidIndex].StreamIndex] = 1;
    }

  for (i = 0; i < _db.DataStreams.Size(); i++)
    if (streamsRefs[i] != 0)
    {
      const CResource &rs = _db.DataStreams[i].Resource;
      if (!rs.IsSolidSmall())
        complexity += rs.PackSize;
    }
      
  RINOK(callback->SetTotal(complexity))
  UInt64 totalComplexity = complexity;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(callback, true);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

  complexity = 0;

  // bool useResourceCompression = false;
  // use useResourceCompression only if CHeader::Flags compression is also set

  CHeader header;
  header.SetDefaultFields(false);

  if (isUpdate)
  {
    const CHeader &srcHeader = _volumes[1].Header;
    header.Flags = srcHeader.Flags;
    header.Version = srcHeader.Version;
    header.ChunkSize = srcHeader.ChunkSize;
    header.ChunkSizeBits = srcHeader.ChunkSizeBits;
  }

  CMyComPtr<IStreamSetRestriction> setRestriction;
  outSeqStream->QueryInterface(IID_IStreamSetRestriction, (void **)&setRestriction);
  if (setRestriction)
    RINOK(setRestriction->SetRestriction(0, kHeaderSizeMax))

  {
    Byte buf[kHeaderSizeMax];
    header.WriteTo(buf);
    RINOK(WriteStream(outStream, buf, kHeaderSizeMax))
  }

  UInt64 curPos = kHeaderSizeMax;

  CMyComPtr2_Create<ISequentialInStream, CInStreamWithSha1> inShaStream;

  CLimitedSequentialInStream *inStreamLimitedSpec = NULL;
  CMyComPtr<ISequentialInStream> inStreamLimited;
  if (_volumes.Size() == 2)
  {
    inStreamLimitedSpec = new CLimitedSequentialInStream;
    inStreamLimited = inStreamLimitedSpec;
    inStreamLimitedSpec->SetStream(_volumes[1].Stream);
  }

  
  CRecordVector<CStreamInfo> streams;
  CSortedIndex sortedHashes; // indexes to streams, sorted by SHA1
  
  // ---------- Copy unchanged data streams ----------

  UInt64 solidRunOffset = 0;
  UInt64 curSolidSize = 0;

  for (i = 0; i < _db.DataStreams.Size(); i++)
  {
    const CStreamInfo &siOld = _db.DataStreams[i];
    const CResource &rs = siOld.Resource;
    
    const unsigned numRefs = streamsRefs[i];

    if (numRefs == 0)
    {
      if (!rs.IsSolidSmall())
        continue;
      if (streamsRefs[_db.Solids[rs.SolidIndex].StreamIndex] == 0)
        continue;
    }

    lps->InSize = lps->OutSize = complexity;
    RINOK(lps->SetCur())

    const unsigned streamIndex = streams.Size();
    CStreamInfo s;
    s.Resource = rs;
    s.PartNumber = 1;
    s.RefCount = numRefs;

    memcpy(s.Hash, siOld.Hash, kHashSize);

    if (rs.IsSolid())
    {
      CSolid &ss = _db.Solids[rs.SolidIndex];
      if (rs.IsSolidSmall())
      {
        UInt64 oldOffset = ss.SolidOffset;
        if (rs.Offset < oldOffset)
          return E_FAIL;
        UInt64 relatOffset = rs.Offset - oldOffset;
        s.Resource.Offset = solidRunOffset + relatOffset;
      }
      else
      {
        // IsSolidBig
        solidRunOffset += curSolidSize;
        curSolidSize = ss.UnpackSize;
      }
    }
    else
    {
      solidRunOffset = 0;
      curSolidSize = 0;
    }
     
    if (!rs.IsSolid() || rs.IsSolidSmall())
    {
      const int find = AddUniqHash(streams.ConstData(), sortedHashes, siOld.Hash, (int)streamIndex);
      if (find != -1)
        return E_FAIL; // two streams with same SHA-1
    }
   
    if (!rs.IsSolid() || rs.IsSolidBig())
    {
      RINOK(InStream_SeekSet(_volumes[siOld.PartNumber].Stream, rs.Offset))
      inStreamLimitedSpec->Init(rs.PackSize);
      RINOK(copyCoder.Interface()->Code(inStreamLimited, outStream, NULL, NULL, lps))
      if (copyCoder->TotalSize != rs.PackSize)
        return E_FAIL;
      s.Resource.Offset = curPos;
      curPos += rs.PackSize;
      lps->ProgressOffset += rs.PackSize;
    }

    streams.Add(s);
  }

  
  // ---------- Write new items ----------

  CUIntVector hlIndexes; // sorted indexes for hard link items

  for (i = 0; i < db.UpdateIndexes.Size(); i++)
  {
    lps->InSize = lps->OutSize = complexity;
    RINOK(lps->SetCur())
    const CUpdateItem &ui = db.UpdateItems[db.UpdateIndexes[i]];
    CMetaItem &mi = db.MetaItems[ui.MetaIndex];
    UInt64 size = 0;
    
    if (ui.AltStreamIndex >= 0)
    {
      if (mi.Skip)
        continue;
      size = mi.AltStreams[ui.AltStreamIndex].Size;
    }
    else
    {
      size = mi.Size;
      if (mi.IsDir)
      {
        // we support LINK files here
        if (mi.Reparse.Size() == 0)
          continue;
      }
    }

    if (ui.InArcIndex >= 0)
    {
      // data streams with OLD Data were written already
      // we just need to find HashIndex in hashes.

      if ((unsigned)ui.InArcIndex >= _db.SortedItems.Size())
        return E_FAIL;
      
      const CItem &item = _db.Items[_db.SortedItems[ui.InArcIndex]];
      
      if (item.StreamIndex < 0)
      {
        if (size == 0)
          continue;
        // if (_db.ItemHasStream(item))
        return E_FAIL;
      }

      // We support empty file (size = 0, but with stream and SHA-1) from old archive
      
      const CStreamInfo &siOld = _db.DataStreams[item.StreamIndex];

      const int index = AddUniqHash(streams.ConstData(), sortedHashes, siOld.Hash, -1);
      // we must have written that stream already
      if (index == -1)
        return E_FAIL;

      if (ui.AltStreamIndex < 0)
        mi.HashIndex = index;
      else
        mi.AltStreams[ui.AltStreamIndex].HashIndex = index;
      
      continue;
    }

    CMyComPtr<ISequentialInStream> fileInStream;
    HRESULT res = callback->GetStream(ui.CallbackIndex, &fileInStream);
    
    if (res == S_FALSE)
    {
      if (ui.AltStreamIndex >= 0)
      {
        mi.NumSkipAltStreams++;
        mi.AltStreams[ui.AltStreamIndex].Skip = true;
      }
      else
        mi.Skip = true;
    }
    else
    {
      RINOK(res)

      int miIndex = -1;
      
      if (!fileInStream)
      {
        if (!mi.IsDir)
          return E_INVALIDARG;
      }
      else if (ui.AltStreamIndex < 0)
      {
        CMyComPtr<IStreamGetProps2> getProps2;
        fileInStream->QueryInterface(IID_IStreamGetProps2, (void **)&getProps2);
        if (getProps2)
        {
          CStreamFileProps props;
          if (getProps2->GetProps2(&props) == S_OK)
          {
            mi.Attrib = props.Attrib;
            if (_timeOptions.Write_CTime.Val) mi.CTime = props.CTime;
            if (_timeOptions.Write_ATime.Val) mi.ATime = props.ATime;
            if (_timeOptions.Write_MTime.Val) mi.MTime = props.MTime;
            mi.FileID = props.FileID_Low;
            if (props.NumLinks <= 1)
              mi.FileID = 0;
            mi.VolID = props.VolID;
            if (mi.FileID != 0)
              miIndex = AddToHardLinkList(db.MetaItems, (unsigned)ui.MetaIndex, hlIndexes);

            if (props.Size != size && props.Size != (UInt64)(Int64)-1)
            {
              const Int64 delta = (Int64)props.Size - (Int64)size;
              const Int64 newComplexity = (Int64)totalComplexity + delta;
              if (newComplexity > 0)
              {
                totalComplexity = (UInt64)newComplexity;
                callback->SetTotal(totalComplexity);
              }
              mi.Size = props.Size;
              size = props.Size;
            }
          }
        }
      }
      
      if (miIndex >= 0)
      {
        mi.HashIndex = db.MetaItems[miIndex].HashIndex;
        if (mi.HashIndex >= 0)
          streams[mi.HashIndex].RefCount++;
        // fix for future: maybe we need to check also that real size is equal to size from IStreamGetProps2
      }
      else if (ui.AltStreamIndex < 0 && mi.Reparse.Size() != 0)
      {
        if (mi.Reparse.Size() < 8)
          return E_FAIL;
        NCrypto::NSha1::CContext sha1;
        sha1.Init();
        const size_t packSize = mi.Reparse.Size() - 8;
        sha1.Update((const Byte *)mi.Reparse + 8, packSize);
        Byte hash[kHashSize];
        sha1.Final(hash);
        
        int index = AddUniqHash(streams.ConstData(), sortedHashes, hash, (int)streams.Size());

        if (index != -1)
          streams[index].RefCount++;
        else
        {
          index = (int)streams.Size();
          RINOK(WriteStream(outStream, (const Byte *)mi.Reparse + 8, packSize))
          CStreamInfo s;
          s.Resource.PackSize = packSize;
          s.Resource.Offset = curPos;
          s.Resource.UnpackSize = packSize;
          s.Resource.Flags = 0; // check it
          /*
            if (useResourceCompression)
              s.Resource.Flags = NResourceFlags::Compressed;
          */
          s.PartNumber = 1;
          s.RefCount = 1;
          memcpy(s.Hash, hash, kHashSize);
          curPos += packSize;

          streams.Add(s);
        }
        
        mi.HashIndex = index;
      }
      else
      {
        inShaStream->SetStream(fileInStream);

        CMyComPtr<IInStream> inSeekStream;
        fileInStream.QueryInterface(IID_IInStream, (void **)&inSeekStream);
        
        fileInStream.Release();
        inShaStream->Init();
        UInt64 offsetBlockSize = 0;
        /*
        if (useResourceCompression)
        {
          for (UInt64 t = kChunkSize; t < size; t += kChunkSize)
          {
            Byte buf[8];
            SetUi32(buf, (UInt32)t);
            RINOK(WriteStream(outStream, buf, 4));
            offsetBlockSize += 4;
          }
        }
        */

        // 22.02: we use additional read-only pass to calculate SHA-1
        bool needWritePass = true;
        int index = -1;
        
        if (inSeekStream /* && !sortedHashes.IsEmpty() */)
        {
          RINOK(copyCoder.Interface()->Code(inShaStream, NULL, NULL, NULL, lps))
          size = copyCoder->TotalSize;
          if (size == 0)
            needWritePass = false;
          else
          {
            Byte hash[kHashSize];
            inShaStream->Final(hash);

            index = AddUniqHash(streams.ConstData(), sortedHashes, hash, -1);
            if (index != -1)
            {
              streams[index].RefCount++;
              needWritePass = false;
            }
            else
            {
              RINOK(InStream_SeekToBegin(inSeekStream))
              inShaStream->Init();
            }
          }
        }
        
        if (needWritePass)
        {
          RINOK(copyCoder.Interface()->Code(inShaStream, outStream, NULL, NULL, lps))
          size = copyCoder->TotalSize;
        }
       
        if (size != 0)
        {
          if (needWritePass)
          {
            Byte hash[kHashSize];
            const UInt64 packSize = offsetBlockSize + size;
            inShaStream->Final(hash);
            
            index = AddUniqHash(streams.ConstData(), sortedHashes, hash, (int)streams.Size());
            
            if (index != -1)
            {
              streams[index].RefCount++;
              outStream->Seek(-(Int64)packSize, STREAM_SEEK_CUR, &curPos);
              outStream->SetSize(curPos);
            }
            else
            {
              index = (int)streams.Size();
              CStreamInfo s;
              s.Resource.PackSize = packSize;
              s.Resource.Offset = curPos;
              s.Resource.UnpackSize = size;
              s.Resource.Flags = 0;
              /*
              if (useResourceCompression)
              s.Resource.Flags = NResourceFlags::Compressed;
              */
              s.PartNumber = 1;
              s.RefCount = 1;
              memcpy(s.Hash, hash, kHashSize);
              curPos += packSize;
              
              streams.Add(s);
            }
          } // needWritePass
          if (ui.AltStreamIndex < 0)
            mi.HashIndex = index;
          else
            mi.AltStreams[ui.AltStreamIndex].HashIndex = index;
        } // (size != 0)
      }
    }
    fileInStream.Release();
    complexity += size;
    RINOK(callback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
  }

  while (secureBlocks.Size() < numNewImages)
    secureBlocks.AddNew();

  
  
  // ---------- Write Images ----------

  for (i = 0; i < numNewImages; i++)
  {
    lps->InSize = lps->OutSize = complexity;
    RINOK(lps->SetCur())
    if (i < isChangedImage.Size() && !isChangedImage[i])
    {
      CStreamInfo s = _db.MetaStreams[i];
      
      RINOK(InStream_SeekSet(_volumes[1].Stream, s.Resource.Offset))
      inStreamLimitedSpec->Init(s.Resource.PackSize);
      RINOK(copyCoder.Interface()->Code(inStreamLimited, outStream, NULL, NULL, lps))
      if (copyCoder->TotalSize != s.Resource.PackSize)
        return E_FAIL;

      s.Resource.Offset = curPos;
      s.PartNumber = 1;
      s.RefCount = 1;
      streams.Add(s);

      if (_bootIndex != 0 && _bootIndex == (UInt32)i + 1)
      {
        header.MetadataResource = s.Resource;
        header.BootIndex = _bootIndex;
      }

      lps->ProgressOffset += s.Resource.PackSize;
      curPos += s.Resource.PackSize;
      // printf("\nWrite old image %x\n", i + 1);
      continue;
    }

    const CDir &tree = trees[i];
    const UInt32 kSecuritySize = 8;
    
    size_t pos = kSecuritySize;

    const CUniqBlocks &secUniqBlocks = secureBlocks[i];
    const CObjectVector<CByteBuffer> &secBufs = secUniqBlocks.Bufs;
    pos += (size_t)secUniqBlocks.GetTotalSizeInBytes();
    pos += secBufs.Size() * 8;
    pos = (pos + 7) & ~(size_t)7;
    
    db.DefaultDirItem = ri;
    pos += db.WriteTree_Dummy(tree);
    
    CByteArr meta(pos);
    
    Set32((Byte *)meta + 4, secBufs.Size()) // num security entries
    pos = kSecuritySize;
    
    if (secBufs.Size() == 0)
    {
      // we can write 0 here only if there is no security data, imageX does it,
      // but some programs expect size = 8
      Set32((Byte *)meta, 8) // size of security data
      // Set32((Byte *)meta, 0);
    }
    else
    {
      unsigned k;
      for (k = 0; k < secBufs.Size(); k++, pos += 8)
      {
        Set64(meta + pos, secBufs[k].Size())
      }
      for (k = 0; k < secBufs.Size(); k++)
      {
        const CByteBuffer &buf = secBufs[k];
        size_t size = buf.Size();
        if (size != 0)
        {
          memcpy(meta + pos, buf, size);
          pos += size;
        }
      }
      while ((pos & 7) != 0)
        meta[pos++] = 0;
      Set32((Byte *)meta, (UInt32)pos) // size of security data
    }
    
    db.Hashes = streams.ConstData();
    db.WriteTree(tree, (Byte *)meta, pos);

    {
      NCrypto::NSha1::CContext sha;
      sha.Init();
      sha.Update((const Byte *)meta, pos);

      Byte digest[kHashSize];
      sha.Final(digest);
      
      CStreamInfo s;
      s.Resource.PackSize = pos;
      s.Resource.Offset = curPos;
      s.Resource.UnpackSize = pos;
      s.Resource.Flags = NResourceFlags::kMetadata;
      s.PartNumber = 1;
      s.RefCount = 1;
      memcpy(s.Hash, digest, kHashSize);
      streams.Add(s);

      if (_bootIndex != 0 && _bootIndex == (UInt32)i + 1)
      {
        header.MetadataResource = s.Resource;
        header.BootIndex = _bootIndex;
      }

      RINOK(WriteStream(outStream, (const Byte *)meta, pos))
      meta.Free();
      curPos += pos;
    }
  }

  lps->InSize = lps->OutSize = complexity;
  RINOK(lps->SetCur())

  header.OffsetResource.UnpackSize = header.OffsetResource.PackSize = (UInt64)streams.Size() * kStreamInfoSize;
  header.OffsetResource.Offset = curPos;
  header.OffsetResource.Flags = NResourceFlags::kMetadata;

  
  
  // ---------- Write Streams Info Tables ----------

  for (i = 0; i < streams.Size(); i++)
  {
    Byte buf[kStreamInfoSize];
    streams[i].WriteTo(buf);
    RINOK(WriteStream(outStream, buf, kStreamInfoSize))
    curPos += kStreamInfoSize;
  }

  AString xml ("<WIM>");
  AddTagUInt64_ToString(xml, "TOTALBYTES", curPos);
  for (i = 0; i < trees.Size(); i++)
  {
    const CDir &tree = trees[i];

    CXmlItem item;
    if (_xmls.Size() == 1)
    {
      const CWimXml &_oldXml = _xmls[0];
      if (i < _oldXml.Images.Size())
      {
        // int ttt = _oldXml.Images[i].ItemIndexInXml;
        item = _oldXml.Xml.Root.SubItems[_oldXml.Images[i].ItemIndexInXml];
      }
    }
    if (i >= isChangedImage.Size() || isChangedImage[i])
    {
      char temp[16];
      if (item.Name.IsEmpty())
      {
        ConvertUInt32ToString(i + 1, temp);
        item.Name = "IMAGE";
        item.IsTag = true;
        CXmlProp &prop = item.Props.AddNew();
        prop.Name = "INDEX";
        prop.Value = temp;
      }
      
      AddTag_String_IfEmpty(item, "NAME", temp);
      AddTag_UInt64(item, "DIRCOUNT", tree.GetNumDirs() - 1);
      AddTag_UInt64(item, "FILECOUNT", tree.GetNumFiles());
      AddTag_UInt64(item, "TOTALBYTES", tree.GetTotalSize(db.MetaItems));
      
      AddTag_Time(item, "CREATIONTIME", ftCur);
      AddTag_Time(item, "LASTMODIFICATIONTIME", ftCur);
    }

    item.AppendTo(xml);
  }
  xml += "</WIM>";

  size_t xmlSize;
  {
    UString utf16;
    if (!ConvertUTF8ToUnicode(xml, utf16))
      return S_FALSE;
    xmlSize = ((size_t)utf16.Len() + 1) * 2;

    CByteArr xmlBuf(xmlSize);
    Set16((Byte *)xmlBuf, 0xFEFF)
    for (i = 0; i < (unsigned)utf16.Len(); i++)
    {
      Set16((Byte *)xmlBuf + 2 + (size_t)i * 2, (UInt16)utf16[i])
    }
    RINOK(WriteStream(outStream, (const Byte *)xmlBuf, xmlSize))
  }
  
  header.XmlResource.UnpackSize =
  header.XmlResource.PackSize = xmlSize;
  header.XmlResource.Offset = curPos;
  header.XmlResource.Flags = NResourceFlags::kMetadata;

  outStream->Seek(0, STREAM_SEEK_SET, NULL);
  header.NumImages = trees.Size();
  {
    Byte buf[kHeaderSizeMax];
    header.WriteTo(buf);
    RINOK(WriteStream(outStream, buf, kHeaderSizeMax))
  }

  if (setRestriction)
    RINOK(setRestriction->SetRestriction(0, 0))

  return S_OK;

  COM_TRY_END
}

}}

/* ================ unit: CPP/7zip/Archive/Wim/WimIn.cpp ================ */
// Archive/WimIn.cpp

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

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

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

namespace NArchive {
namespace NWim {

static bool inline GetLog_val_min_dest(const UInt32 val, unsigned i, unsigned &dest)
{
  UInt32 v = (UInt32)1 << i;
  for (; i < 32; i++)
  {
    if (v == val)
    {
      dest = i;
      return true;
    }
    v += v;
  }
  return false;
}


HRESULT CUnpacker::UnpackChunk(
    ISequentialInStream *inStream,
    unsigned method, unsigned chunkSizeBits,
    size_t inSize, size_t outSize,
    ISequentialOutStream *outStream)
{
  if (inSize == outSize)
  {
  }
  else if (method == NMethod::kXPRESS)
  {
  }
  else if (method == NMethod::kLZX)
  {
    lzxDecoder.Create_if_Empty();
    lzxDecoder->Set_WimMode(true);
  }
  else if (method == NMethod::kLZMS)
  {
    lzmsDecoder.Create_if_Empty();
  }
  else
    return E_NOTIMPL;

  const size_t chunkSize = (size_t)1 << chunkSizeBits;

  {
    const unsigned
        kAdditionalOutputBufSize = MyMax(NCompress::NLzx::
        kAdditionalOutputBufSize,        NCompress::NXpress::
        kAdditionalOutputBufSize);
    unpackBuf.EnsureCapacity(chunkSize + kAdditionalOutputBufSize);
    if (!unpackBuf.Data)
      return E_OUTOFMEMORY;
  }
  
  HRESULT res = S_FALSE;
  size_t unpackedSize = 0;
  
  if (inSize == outSize)
  {
    unpackedSize = outSize;
    res = ReadStream(inStream, unpackBuf.Data, &unpackedSize);
    TotalPacked += unpackedSize;
  }
  else if (inSize < chunkSize)
  {
    const unsigned kAdditionalInputSize = 32;
    packBuf.EnsureCapacity(chunkSize + kAdditionalInputSize);
    if (!packBuf.Data)
      return E_OUTOFMEMORY;
    
    RINOK(ReadStream_FALSE(inStream, packBuf.Data, inSize))
    memset(packBuf.Data + inSize, 0xff, kAdditionalInputSize);

    TotalPacked += inSize;
    
    if (method == NMethod::kXPRESS)
    {
      res = NCompress::NXpress::Decode_WithExceedWrite(packBuf.Data, inSize, unpackBuf.Data, outSize);
      if (res == S_OK)
        unpackedSize = outSize;
    }
    else if (method == NMethod::kLZX)
    {
      res = lzxDecoder->Set_ExternalWindow_DictBits(unpackBuf.Data, chunkSizeBits);
      if (res != S_OK)
        return E_NOTIMPL;
      lzxDecoder->Set_KeepHistoryForNext(false);
      lzxDecoder->Set_KeepHistory(false);
      res = lzxDecoder->Code_WithExceedReadWrite(packBuf.Data, inSize, (UInt32)outSize);
      unpackedSize = lzxDecoder->GetUnpackSize();
      if (res == S_OK && !lzxDecoder->WasBlockFinished())
        res = S_FALSE;
    }
    else
    {
      res = lzmsDecoder->Code(packBuf.Data, inSize, unpackBuf.Data, outSize);
      unpackedSize = lzmsDecoder->GetUnpackSize();
    }
  }
  
  if (unpackedSize != outSize)
  {
    if (res == S_OK)
      res = S_FALSE;
    
    if (unpackedSize > outSize)
      res = S_FALSE;
    else
      memset(unpackBuf.Data + unpackedSize, 0, outSize - unpackedSize);
  }
  
  if (outStream)
  {
    RINOK(WriteStream(outStream, unpackBuf.Data, outSize))
  }
  
  return res;
}


HRESULT CUnpacker::Unpack2(
    IInStream *inStream,
    const CResource &resource,
    const CHeader &header,
    const CDatabase *db,
    ISequentialOutStream *outStream,
    ICompressProgressInfo *progress)
{
  if (!resource.IsCompressed() && !resource.IsSolid())
  {
    copyCoder.Create_if_Empty();

    CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> limitedStream;
    limitedStream->SetStream(inStream);
    
    RINOK(InStream_SeekSet(inStream, resource.Offset))
    if (resource.PackSize != resource.UnpackSize)
      return S_FALSE;

    limitedStream->Init(resource.PackSize);
    TotalPacked += resource.PackSize;
    
    HRESULT res = copyCoder.Interface()->Code(limitedStream, outStream, NULL, NULL, progress);
    
    if (res == S_OK && copyCoder->TotalSize != resource.UnpackSize)
      res = S_FALSE;
    return res;
  }
  
  if (resource.IsSolid())
  {
    if (!db || resource.SolidIndex < 0)
      return E_NOTIMPL;
    if (resource.IsCompressed())
      return E_NOTIMPL;

    const CSolid &ss = db->Solids[resource.SolidIndex];
    
    const unsigned chunkSizeBits = ss.ChunkSizeBits;
    const size_t chunkSize = (size_t)1 << chunkSizeBits;
    
    size_t chunkIndex = 0;
    UInt64 rem = ss.UnpackSize;
    size_t offsetInChunk = 0;
    
    if (resource.IsSolidSmall())
    {
      UInt64 offs = resource.Offset;
      if (offs < ss.SolidOffset)
        return E_NOTIMPL;
      offs -= ss.SolidOffset;
      if (offs > ss.UnpackSize)
        return E_NOTIMPL;
      rem = resource.PackSize;
      if (rem > ss.UnpackSize - offs)
        return E_NOTIMPL;
      chunkIndex = (size_t)(offs >> chunkSizeBits);
      offsetInChunk = (size_t)offs & (chunkSize - 1);
    }
    
    UInt64 packProcessed = 0;
    UInt64 outProcessed = 0;
    
    if (_solidIndex == resource.SolidIndex && _unpackedChunkIndex == chunkIndex)
    {
      size_t cur = chunkSize - offsetInChunk;
      if (cur > rem)
        cur = (size_t)rem;
      RINOK(WriteStream(outStream, unpackBuf.Data + offsetInChunk, cur))
      outProcessed += cur;
      rem -= cur;
      offsetInChunk = 0;
      chunkIndex++;
    }
    
    for (;;)
    {
      if (rem == 0)
        return S_OK;
    
      const UInt64 offset = ss.Chunks[chunkIndex];
      const UInt64 packSize = ss.GetChunkPackSize(chunkIndex);
      const CResource &rs = db->DataStreams[ss.StreamIndex].Resource;
      RINOK(InStream_SeekSet(inStream, rs.Offset + ss.HeadersSize + offset))
      
      size_t cur = chunkSize;
      const UInt64 unpackRem = ss.UnpackSize - ((UInt64)chunkIndex << chunkSizeBits);
      if (cur > unpackRem)
        cur = (size_t)unpackRem;
      
      _solidIndex = -1;
      _unpackedChunkIndex = 0;
      
      const HRESULT res = UnpackChunk(inStream, (unsigned)ss.Method, chunkSizeBits, (size_t)packSize, cur, NULL);
      
      if (res != S_OK)
      {
        // We ignore data errors in solid stream. SHA will show what files are bad.
        if (res != S_FALSE)
          return res;
      }
      
      _solidIndex = resource.SolidIndex;
      _unpackedChunkIndex = chunkIndex;

      if (cur < offsetInChunk)
        return E_FAIL;
      
      cur -= offsetInChunk;
        
      if (cur > rem)
        cur = (size_t)rem;
      
      RINOK(WriteStream(outStream, unpackBuf.Data + offsetInChunk, cur))
      
      if (progress)
      {
        RINOK(progress->SetRatioInfo(&packProcessed, &outProcessed))
        packProcessed += packSize;
        outProcessed += cur;
      }
      
      rem -= cur;
      offsetInChunk = 0;
      chunkIndex++;
    }
  }


  // ---------- NON Solid ----------

  const UInt64 unpackSize = resource.UnpackSize;
  if (unpackSize == 0)
  {
    if (resource.PackSize == 0)
      return S_OK;
    return S_FALSE;
  }

  if (unpackSize > ((UInt64)1 << 63))
    return E_NOTIMPL;

  const unsigned chunkSizeBits = header.ChunkSizeBits;
  const unsigned entrySizeShifts = (resource.UnpackSize < ((UInt64)1 << 32) ? 2 : 3);

  UInt64 baseOffset = resource.Offset;
  UInt64 packDataSize;
  size_t numChunks;
  {
    const UInt64 numChunks64 = (unpackSize + (((UInt32)1 << chunkSizeBits) - 1)) >> chunkSizeBits;
    const UInt64 sizesBufSize64 = (numChunks64 - 1) << entrySizeShifts;
    if (sizesBufSize64 > resource.PackSize)
      return S_FALSE;
    packDataSize = resource.PackSize - sizesBufSize64;
    const size_t sizesBufSize = (size_t)sizesBufSize64;
    if (sizesBufSize != sizesBufSize64)
      return E_OUTOFMEMORY;
    sizesBuf.AllocAtLeast(sizesBufSize);
    RINOK(InStream_SeekSet(inStream, baseOffset))
    RINOK(ReadStream_FALSE(inStream, sizesBuf, sizesBufSize))
    baseOffset += sizesBufSize64;
    numChunks = (size_t)numChunks64;
  }

  _solidIndex = -1;
  _unpackedChunkIndex = 0;

  UInt64 outProcessed = 0;
  UInt64 offset = 0;
  
  for (size_t i = 0; i < numChunks; i++)
  {
    UInt64 nextOffset = packDataSize;
    
    if (i + 1 < numChunks)
    {
      const Byte *p = (const Byte *)sizesBuf + (i << entrySizeShifts);
      nextOffset = (entrySizeShifts == 2) ? Get32(p): Get64(p);
    }
    
    if (nextOffset < offset)
      return S_FALSE;

    UInt64 inSize64 = nextOffset - offset;
    size_t inSize = (size_t)inSize64;
    if (inSize != inSize64)
      return S_FALSE;

    RINOK(InStream_SeekSet(inStream, baseOffset + offset))

    if (progress)
    {
      RINOK(progress->SetRatioInfo(&offset, &outProcessed))
    }
    
    size_t outSize = (size_t)1 << chunkSizeBits;
    const UInt64 rem = unpackSize - outProcessed;
    if (outSize > rem)
      outSize = (size_t)rem;

    RINOK(UnpackChunk(inStream, header.GetMethod(), chunkSizeBits, inSize, outSize, outStream))

    outProcessed += outSize;
    offset = nextOffset;
  }
  
  return S_OK;
}


HRESULT CUnpacker::Unpack(IInStream *inStream, const CResource &resource, const CHeader &header, const CDatabase *db,
    ISequentialOutStream *outStream, ICompressProgressInfo *progress, Byte *digest)
{
  CMyComPtr2_Create<ISequentialOutStream, COutStreamWithSha1> shaStream;
  // outStream can be NULL, so we use COutStreamWithSha1 even if sha1 is not required
  shaStream->SetStream(outStream);
  shaStream->Init(digest != NULL);
  const HRESULT res = Unpack2(inStream, resource, header, db, shaStream, progress);
  if (digest)
    shaStream->Final(digest);
  return res;
}


HRESULT CUnpacker::UnpackData(IInStream *inStream,
    const CResource &resource, const CHeader &header,
    const CDatabase *db,
    CByteBuffer &buf, Byte *digest)
{
  // if (resource.IsSolid()) return E_NOTIMPL;
  UInt64 unpackSize64 = resource.UnpackSize;
  if (db)
    unpackSize64 = db->Get_UnpackSize_of_Resource(resource);
  const size_t size = (size_t)unpackSize64;
  if (size != unpackSize64)
    return E_OUTOFMEMORY;
  buf.Alloc(size);

  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> outStream;
  outStream->Init((Byte *)buf, size);
  return Unpack(inStream, resource, header, db, outStream, NULL, digest);
}


void CResource::Parse(const Byte *p)
{
  Flags = p[7];
  PackSize = Get64(p) & (((UInt64)1 << 56) - 1);
  Offset = Get64(p + 8);
  UnpackSize = Get64(p + 16);
  KeepSolid = false;
  SolidIndex = -1;
}

#define GET_RESOURCE(_p_, res) res.ParseAndUpdatePhySize(_p_, phySize)

static inline void ParseStream(bool oldVersion, const Byte *p, CStreamInfo &s)
{
  s.Resource.Parse(p);
  if (oldVersion)
  {
    s.PartNumber = 1;
    s.Id = Get32(p + 24);
    p += 28;
  }
  else
  {
    s.PartNumber = Get16(p + 24);
    p += 26;
  }
  s.RefCount = Get32(p);
  memcpy(s.Hash, p + 4, kHashSize);
}


#define kLongPath "[LONG_PATH]" STRING_PATH_SEPARATOR "[LONG_PATH_ITEM]"

void CDatabase::GetShortName(unsigned index, NWindows::NCOM::CPropVariant &name) const
{
  const CItem &item = Items[index];
  const CImage &image = Images[item.ImageIndex];
  if (item.Parent < 0 && image.NumEmptyRootItems != 0)
  {
    name.Clear();
    return;
  }
  const Byte *meta = image.Meta + item.Offset +
      (IsOldVersion ? kDirRecordSizeOld : kDirRecordSize);
  UInt32 fileNameLen = Get16(meta - 2);
  UInt32 shortLen = Get16(meta - 4) / 2;
  wchar_t *s = name.AllocBstr(shortLen);
  if (fileNameLen != 0)
    meta += fileNameLen + 2;
  for (UInt32 i = 0; i < shortLen; i++)
    s[i] = Get16(meta + i * 2);
  s[shortLen] = 0;
  // empty shortName has no ZERO at the end ?
}


void CDatabase::GetItemName(unsigned index, NWindows::NCOM::CPropVariant &name) const
{
  const CItem &item = Items[index];
  const CImage &image = Images[item.ImageIndex];
  if (item.Parent < 0 && image.NumEmptyRootItems != 0)
  {
    name = image.RootName;
    return;
  }
  const Byte *meta = image.Meta + item.Offset +
      (item.IsAltStream ?
      (IsOldVersion ? 0x10 : 0x24) :
      (IsOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2));
  UInt32 len = Get16(meta) / 2;
  wchar_t *s = name.AllocBstr(len);
  meta += 2;
  len++;
  for (UInt32 i = 0; i < len; i++)
    s[i] = Get16(meta + i * 2);
}


void CDatabase::GetItemPath(unsigned index1, bool showImageNumber, NWindows::NCOM::CPropVariant &path) const
{
  unsigned size = 0;
  int index = (int)index1;
  const int imageIndex = Items[index].ImageIndex;
  const CImage &image = Images[imageIndex];
  
  unsigned newLevel = 0;
  bool needColon = false;

  for (;;)
  {
    const CItem &item = Items[index];
    if (item.DirLevel > (1 << 12))
    {
      path = kLongPath;
      return;
    }
    index = item.Parent;
    if (index >= 0 || image.NumEmptyRootItems == 0)
    {
      const Byte *meta = image.Meta + item.Offset;
      meta += item.IsAltStream ?
          (IsOldVersion ? 0x10 : 0x24) :
          (IsOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2);
      needColon = item.IsAltStream;
      size += Get16(meta) / 2;
      size += newLevel;
      newLevel = 1;
      if (size >= ((UInt32)1 << 15))
      {
        path = kLongPath;
        return;
      }
    }
    if (index < 0)
      break;
  }

  if (showImageNumber)
  {
    size += image.RootName.Len();
    size += newLevel;
  }
  else if (needColon)
    size++;

  wchar_t *s = path.AllocBstr(size);
  s[size] = 0;
  
  if (showImageNumber)
  {
    MyStringCopy(s, (const wchar_t *)image.RootName);
    if (newLevel)
      s[image.RootName.Len()] = (wchar_t)(needColon ? L':' : WCHAR_PATH_SEPARATOR);
  }
  else if (needColon)
    s[0] = L':';

  index = (int)index1;
  wchar_t separator = 0;
  
  for (;;)
  {
    const CItem &item = Items[index];
    index = item.Parent;
    if (index >= 0 || image.NumEmptyRootItems == 0)
    {
      if (separator != 0)
        s[--size] = separator;
      const Byte *meta = image.Meta + item.Offset;
      meta += (item.IsAltStream) ?
          (IsOldVersion ? 0x10: 0x24) :
          (IsOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2);
      unsigned len = Get16(meta) / 2;
      size -= len;
      wchar_t *dest = s + size;
      meta += 2;
      for (unsigned i = 0; i < len; i++)
      {
        wchar_t c = Get16(meta + i * 2);
        if (c == L'/')
          c = L'_';
        #if WCHAR_PATH_SEPARATOR != L'/'
        else if (c == L'\\')
          c = WCHAR_IN_FILE_NAME_BACKSLASH_REPLACEMENT; // 22.00 : WSL scheme
        #endif
        dest[i] = c;
      }
    }
    if (index < 0)
      return;
    separator = item.IsAltStream ? L':' : WCHAR_PATH_SEPARATOR;
  }
}


// if (ver <= 1.10), root folder contains real items.
// if (ver >= 1.12), root folder contains only one folder with empty name.

HRESULT CDatabase::ParseDirItem(size_t pos, int parent, unsigned dirLevel)
{
  // if (++level > (1 << 10)) return S_FALSE;
  CImage &image = Images.Back();
  const unsigned align = GetDirAlignMask();
  if (pos & align)
    return S_FALSE;

  for (unsigned numItems = 0;; numItems++)
  {
    if (OpenCallback && (Items.Size() & 0xFFFF) == 0)
    {
      const UInt64 numFiles = Items.Size();
      RINOK(OpenCallback->SetCompleted(&numFiles, NULL))
    }
    
    const size_t rem = DirSize - pos;
    if (pos < DirStartOffset || pos > DirSize || rem < 8 || DirSize - DirProcessed < 8)
      return S_FALSE;
    const Byte *p = DirData + pos;
    const UInt64 len = Get64(p);
    if (len == 0)
    {
      DirProcessed += 8;
      return S_OK;
    }
    if ((len & align) || rem < len || DirSize - DirProcessed < len)
      return S_FALSE;
    DirProcessed += (size_t)len;

    const unsigned dirRecordSize = IsOldVersion ? kDirRecordSizeOld : kDirRecordSize;
    if (len < dirRecordSize)
      return S_FALSE;

    CItem item;
    const UInt32 attrib = Get32(p + 8);
    item.IsDir = ((attrib & 0x10) != 0);
    {
      const UInt32 securId = Get32(p + 0xC);
      if (securId != (UInt32)(Int32)-1)
         if (securId     >= image.SecurOffsets.Size() ||
             securId + 1 >= image.SecurOffsets.Size())
        HeadersError = true;
    }
    size_t subdirOffset;
    {
      const UInt64 subdirOffset64 = Get64(p + 0x10);
      if (subdirOffset64 > DirSize)
        return S_FALSE;
      subdirOffset = (size_t)subdirOffset64;
    }
    const UInt32 numAltStreams = Get16(p + dirRecordSize - 6);
    const UInt32 shortNameLen = Get16(p + dirRecordSize - 4);
    const UInt32 fileNameLen = Get16(p + dirRecordSize - 2);
    if ((shortNameLen & 1) || (fileNameLen & 1))
      return S_FALSE;
    const UInt32 shortNameLen2 = (shortNameLen == 0 ? shortNameLen : shortNameLen + 2);
    const UInt32 fileNameLen2 = (fileNameLen == 0 ? fileNameLen : fileNameLen + 2);
    if (((dirRecordSize + fileNameLen2 + shortNameLen2 + align) & ~align) > len)
      return S_FALSE;
    
    p += dirRecordSize;
    {
      if (*(const UInt16 *)(const void *)(p + fileNameLen))
        return S_FALSE;
      for (UInt32 j = 0; j < fileNameLen; j += 2)
        if (*(const UInt16 *)(const void *)(p + j) == 0)
          return S_FALSE;
    }
    // PRF(printf("\n%S", p));

    if (shortNameLen)
    {
      // empty shortName has no ZERO at the end ?
      const Byte *p2 = p + fileNameLen2;
      if (*(const UInt16 *)(const void *)(p2 + shortNameLen))
        return S_FALSE;
      for (UInt32 j = 0; j < shortNameLen; j += 2)
        if (*(const UInt16 *)(const void *)(p2 + j) == 0)
          return S_FALSE;
    }
      
    item.Offset = pos;
    item.Parent = parent;
    item.DirLevel = dirLevel;
    item.ImageIndex = (int)Images.Size() - 1;
    
    const unsigned prevIndex = Items.Add(item);

    pos += (size_t)len;

    for (UInt32 i = 0; i < numAltStreams; i++)
    {
      const size_t rem2 = DirSize - pos;
      if (pos < DirStartOffset || pos > DirSize || rem2 < 8)
        return S_FALSE;
      const Byte *p2 = DirData + pos;
      const UInt64 len2 = Get64(p2);
      if ((len2 & align) || rem2 < len2
          || DirSize - DirProcessed < len2
          || len2 < (unsigned)(IsOldVersion ? 0x18 : 0x28))
        return S_FALSE;
      DirProcessed += (size_t)len2;

      unsigned extraOffset = 0;
      if (IsOldVersion)
        extraOffset = 0x10;
      else
      {
        if (Get64(p2 + 8))
          return S_FALSE;
        extraOffset = 0x24;
      }
      
      const UInt32 fileNameLen111 = Get16(p2 + extraOffset);
      if (fileNameLen111 & 1)
        return S_FALSE;
      /* Probably different versions of ImageX can use different number of
         additional ZEROs. So we don't use exact check. */
      const UInt32 fileNameLen222 = (fileNameLen111 == 0 ? fileNameLen111 : fileNameLen111 + 2);
      if (((extraOffset + 2 + fileNameLen222 + align) & ~align) > len2)
        return S_FALSE;
      {
        const Byte *p3 = p2 + extraOffset + 2;
        if (*(const UInt16 *)(const void *)(p3 + fileNameLen111))
          return S_FALSE;
        for (UInt32 j = 0; j < fileNameLen111; j += 2)
          if (*(const UInt16 *)(const void *)(p3 + j) == 0)
            return S_FALSE;
        // PRF(printf("\n  %S", p3));
      }
      /* wim uses alt streams list, if there is at least one alt stream.
         And alt stream without name is main stream. */

      // Why wimlib writes two alt streams for REPARSE_POINT, with empty second alt stream?
      
      Byte *prevMeta = DirData + item.Offset;

      if (fileNameLen111 == 0 &&
          ((attrib & FILE_ATTRIBUTE_REPARSE_POINT) || !item.IsDir)
          && (IsOldVersion || IsEmptySha(prevMeta + 0x40)))
      {
        if (IsOldVersion)
          memcpy(prevMeta + 0x10, p2 + 8, 4); // It's 32-bit Id
        else if (!IsEmptySha(p2 + 0x10))
        {
          // if (IsEmptySha(prevMeta + 0x40))
            memcpy(prevMeta + 0x40, p2 + 0x10, kHashSize);
          // else HeadersError = true;
        }
      }
      else
      {
        ThereAreAltStreams = true;
        CItem item2;
        item2.Offset = pos;
        item2.IsAltStream = true;
        item2.Parent = (int)prevIndex;
        item2.DirLevel = dirLevel;
        item2.ImageIndex = (int)Images.Size() - 1;
        Items.Add(item2);
      }

      pos += (size_t)len2;
    }

    if (parent < 0 && numItems == 0 && shortNameLen == 0 && fileNameLen == 0 && item.IsDir)
    {
      const Byte *p2 = DirData + pos;
      if (DirSize - pos >= 8 && Get64(p2) == 0)
      {
        image.NumEmptyRootItems = 1;

        if (pos + 8 < subdirOffset
            && DirSize - pos >= 16
            && Get64(p2 + 8))
        {
          // Longhorn.4093 contains hidden files after empty root folder and before items of next folder. Why?
          // That code shows them. If we want to ignore them, we need to update DirProcessed.
#if 1 // 0 : for debug : to ignore hidden files
          // we parse hidden files and then parse main files:
          subdirOffset = pos + 8;
#else // ignore hidden files
          DirProcessed += subdirOffset - (pos + 8);
#endif
          // printf("\ndirOffset = %5d hiddenOffset = %5d\n", (int)subdirOffset, (int)pos + 8);
          // return S_FALSE;
        }
      }
    }
    Items[prevIndex].SubDirOffset = subdirOffset;
    /*
    if (item.IsDir && subdirOffset)
    {
      RINOK(ParseDirItem(subdirOffset, (int)prevIndex))
    }
    */
  }
}


HRESULT CDatabase::ParseImageDirs(CByteBuffer &buf, int parent)
{
  DirData = buf;
  DirSize = buf.Size();
  if (DirSize < 8)
    return S_FALSE;
  size_t pos = 0;
  CImage &image = Images.Back();
  const Byte * const p = DirData;

  if (IsOldVersion)
  {
    const UInt32 numEntries = Get32(p + 4);
    if (numEntries >= (1 << 28) ||
        numEntries > (DirSize >> 3))
      return S_FALSE;
    UInt32 sum = 8;
    if (numEntries)
      sum = numEntries * 8;
    image.SecurOffsets.ClearAndReserve(numEntries + 1);
    image.SecurOffsets.AddInReserved(sum);
    for (UInt32 i = 0; i < numEntries; i++)
    {
      const Byte *pp = p + (size_t)i * 8;
      const UInt32 len = Get32(pp);
      if (i && Get32(pp + 4))
        return S_FALSE;
      if (len > DirSize - sum)
        return S_FALSE;
      sum += len;
      if (sum < len)
        return S_FALSE;
      image.SecurOffsets.AddInReserved(sum);
    }
    pos = sum;
    const size_t align = GetDirAlignMask();
    pos = (pos + align) & ~(size_t)align;
  }
  else
  {
    const UInt32 totalLen = Get32(p);
    pos = 8;
    if (totalLen)
    {
      if (totalLen < 8)
        return S_FALSE;
      UInt32 numEntries = Get32(p + 4);
      if (totalLen > DirSize || numEntries > ((totalLen - 8) >> 3))
        return S_FALSE;
      UInt32 sum = (UInt32)pos + numEntries * 8;
      numEntries++;
      image.SecurOffsets.ClearAndReserve(numEntries);
      for (;;)
      {
        image.SecurOffsets.AddInReserved(sum);
        if (--numEntries == 0)
          break;
        const UInt64 len = Get64(p + pos);
        pos += 8;
        if (len > totalLen - sum)
          return S_FALSE;
        sum += (UInt32)len;
      }
      pos = sum;
      pos = (pos + 7) & ~(size_t)7;
      if (pos != (((size_t)totalLen + 7) & ~(size_t)7))
        return S_FALSE;
    }
  }
  
  if (pos > DirSize)
    return S_FALSE;
  DirStartOffset = DirProcessed = pos;
  image.StartItem = Items.Size();

  RINOK(ParseDirItem(pos, parent, 0)) // dirLevel = 0
  {
    for (unsigned i = image.StartItem; i < Items.Size(); i++)
    {
      const CItem &item = Items[i];
      if (item.IsDir && item.SubDirOffset)
      {
        RINOK(ParseDirItem(item.SubDirOffset, (int)i, item.DirLevel + 1))
      }
    }
  }
  
  image.NumItems = Items.Size() - image.StartItem;
  if (DirProcessed == DirSize)
    return S_OK;
  /* Original program writes additional 8 bytes (END_OF_ROOT_FOLDER),
     but the reference to that folder is empty */
  // we can't use DirProcessed - DirStartOffset == 112 check if there is alt stream in root
  if (DirProcessed == DirSize - 8 && Get64(p + DirSize - 8) != 0)
    return S_OK;

  // 18.06: we support cases, when some old dism can capture images
  // where DirProcessed much smaller than DirSize
  HeadersError = true;
  return S_OK;
  // return S_FALSE;
}


HRESULT CHeader::Parse(const Byte *p, UInt64 &phySize)
{
  UInt32 headerSize = Get32(p + 8);
  phySize = headerSize;
  Version = Get32(p + 0x0C);
  Flags = Get32(p + 0x10);
  if (!IsSupported())
    return S_FALSE;
  
  {
    ChunkSize = Get32(p + 0x14);
    ChunkSizeBits = kChunkSizeBits;
    if (ChunkSize != 0)
    {
      if (!GetLog_val_min_dest(ChunkSize, 12, ChunkSizeBits))
        return S_FALSE;
    }
  }

  _isOldVersion = false;
  _isNewVersion = false;
  
  if (IsSolidVersion())
    _isNewVersion = true;
  else
  {
    if (Version < 0x010900)
      return S_FALSE;
    _isOldVersion = (Version <= 0x010A00);
    // We don't know details about 1.11 version. So we use headerSize to guess exact features.
    if (Version == 0x010B00 && headerSize == 0x60)
      _isOldVersion = true;
    _isNewVersion = (Version >= 0x010D00);
  }

  unsigned offset;
  
  if (IsOldVersion())
  {
    if (headerSize != 0x60)
      return S_FALSE;
    memset(Guid, 0, 16);
    offset = 0x18;
    PartNumber = 1;
    NumParts = 1;
  }
  else
  {
    if (headerSize < 0x74)
      return S_FALSE;
    memcpy(Guid, p + 0x18, 16);
    PartNumber = Get16(p + 0x28);
    NumParts = Get16(p + 0x2A);
    if (PartNumber == 0 || PartNumber > NumParts)
      return S_FALSE;
    offset = 0x2C;
    if (IsNewVersion())
    {
      // if (headerSize < 0xD0)
      if (headerSize != 0xD0)
        return S_FALSE;
      NumImages = Get32(p + offset);
      offset += 4;
    }
  }
  
  GET_RESOURCE(p + offset       , OffsetResource);
  GET_RESOURCE(p + offset + 0x18, XmlResource);
  GET_RESOURCE(p + offset + 0x30, MetadataResource);
  BootIndex = 0;
  
  if (IsNewVersion())
  {
    BootIndex = Get32(p + offset + 0x48);
    GET_RESOURCE(p + offset + 0x4C, IntegrityResource);
  }

  return S_OK;
}


const Byte kSignature[kSignatureSize] = { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 };

HRESULT ReadHeader(IInStream *inStream, CHeader &h, UInt64 &phySize)
{
  Byte p[kHeaderSizeMax];
  RINOK(ReadStream_FALSE(inStream, p, kHeaderSizeMax))
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  return h.Parse(p, phySize);
}


static HRESULT ReadStreams(IInStream *inStream, const CHeader &h, CDatabase &db)
{
  CByteBuffer offsetBuf;
  
  CUnpacker unpacker;
  RINOK(unpacker.UnpackData(inStream, h.OffsetResource, h, NULL, offsetBuf, NULL))
  
  const size_t streamInfoSize = h.IsOldVersion() ? kStreamInfoSize + 2 : kStreamInfoSize;
  {
    const unsigned numItems = (unsigned)(offsetBuf.Size() / streamInfoSize);
    if ((size_t)numItems * streamInfoSize != offsetBuf.Size())
      return S_FALSE;
    const unsigned numItems2 = db.DataStreams.Size() + numItems;
    if (numItems2 < numItems)
      return S_FALSE;
    db.DataStreams.Reserve(numItems2);
  }

  bool keepSolid = false;

  for (size_t i = 0; i < offsetBuf.Size(); i += streamInfoSize)
  {
    CStreamInfo s;
    ParseStream(h.IsOldVersion(), (const Byte *)offsetBuf + i, s);

    PRF(printf("\n"));
    PRF(printf(s.Resource.IsMetadata() ? "### META" : "    DATA"));
    PRF(printf(" %2X", s.Resource.Flags));
    PRF(printf(" %9I64X", s.Resource.Offset));
    PRF(printf(" %9I64X", s.Resource.PackSize));
    PRF(printf(" %9I64X", s.Resource.UnpackSize));
    PRF(printf(" %d", s.RefCount));
    
    if (s.PartNumber != h.PartNumber)
      continue;

    if (s.Resource.IsSolid())
    {
      s.Resource.KeepSolid = keepSolid;
      keepSolid = true;
    }
    else
    {
      s.Resource.KeepSolid = false;
      keepSolid = false;
    }

    if (!s.Resource.IsMetadata())
      db.DataStreams.AddInReserved(s);
    else
    {
      if (s.Resource.IsSolid())
        return E_NOTIMPL;
      if (s.RefCount == 0)
      {
        // some wims have such (deleted?) metadata stream.
        // examples: boot.wim in VistaBeta2, WinPE.wim from WAIK.
        // db.DataStreams.Add(s);
        // we can show these delete images, if we comment "continue" command;
        continue;
      }
      
      if (s.RefCount > 1)
      {
        return S_FALSE;
        // s.RefCount--;
        // db.DataStreams.Add(s);
      }

      db.MetaStreams.Add(s);
    }
  }
  
  PRF(printf("\n"));
  
  return S_OK;
}


HRESULT CDatabase::OpenXml(IInStream *inStream, const CHeader &h, CByteBuffer &xml)
{
  CUnpacker unpacker;
  return unpacker.UnpackData(inStream, h.XmlResource, h, this, xml, NULL);
}

static void SetRootNames(CImage &image, unsigned value)
{
  wchar_t temp[16];
  ConvertUInt32ToString(value, temp);
  image.RootName = temp;
  image.RootNameBuf.Alloc(image.RootName.Len() * 2 + 2);
  Byte *p = image.RootNameBuf;
  unsigned len = image.RootName.Len() + 1;
  for (unsigned k = 0; k < len; k++)
  {
    p[k * 2] = (Byte)temp[k];
    p[k * 2 + 1] = 0;
  }
}


HRESULT CDatabase::Open(IInStream *inStream, const CHeader &h, unsigned numItemsReserve, IArchiveOpenCallback *openCallback)
{
  OpenCallback = openCallback;
  IsOldVersion = h.IsOldVersion();
  IsOldVersion9 = (h.Version == 0x10900);

  RINOK(ReadStreams(inStream, h, *this))

  bool needBootMetadata = !h.MetadataResource.IsEmpty();
  unsigned numNonDeletedImages = 0;

  CUnpacker unpacker;

  FOR_VECTOR (i, MetaStreams)
  {
    const CStreamInfo &si = MetaStreams[i];

    if (h.PartNumber != 1 || si.PartNumber != h.PartNumber)
      continue;

    const unsigned userImage = Images.Size() + GetStartImageIndex();
    CImage &image = Images.AddNew();
    SetRootNames(image, userImage);
    
    CByteBuffer &metadata = image.Meta;
    Byte hash[kHashSize];
    
    RINOK(unpacker.UnpackData(inStream, si.Resource, h, this, metadata, hash))
   
    if (memcmp(hash, si.Hash, kHashSize) != 0 &&
        !(h.IsOldVersion() && IsEmptySha(si.Hash)))
      return S_FALSE;
    
    image.NumEmptyRootItems = 0;
    
    if (Items.IsEmpty())
      Items.ClearAndReserve(numItemsReserve);

    RINOK(ParseImageDirs(metadata, -1))
    
    if (needBootMetadata)
    {
      bool sameRes = (h.MetadataResource.Offset == si.Resource.Offset);
      if (sameRes)
        needBootMetadata = false;
      if (h.IsNewVersion())
      {
        if (si.RefCount == 1)
        {
          numNonDeletedImages++;
          bool isBootIndex = (h.BootIndex == numNonDeletedImages);
          if (sameRes && !isBootIndex)
            return S_FALSE;
          if (isBootIndex && !sameRes)
            return S_FALSE;
        }
      }
    }
  }
  
  if (needBootMetadata)
    return S_FALSE;
  return S_OK;
}


bool CDatabase::ItemHasStream(const CItem &item) const
{
  if (item.ImageIndex < 0)
    return true;
  const Byte *meta = Images[item.ImageIndex].Meta + item.Offset;
  if (IsOldVersion)
  {
    // old wim use same field for file_id and dir_offset;
    if (item.IsDir)
      return false;
    meta += (item.IsAltStream ? 0x8 : 0x10);
    UInt32 id = GetUi32(meta);
    return id != 0;
  }
  meta += (item.IsAltStream ? 0x10 : 0x40);
  return !IsEmptySha(meta);
}


#define RINOZ(x) { int _tt_ = (x); if (_tt_ != 0) return _tt_; }

static int CompareStreamsByPos(const CStreamInfo *p1, const CStreamInfo *p2, void * /* param */)
{
  RINOZ(MyCompare(p1->PartNumber, p2->PartNumber))
  RINOZ(MyCompare(p1->Resource.Offset, p2->Resource.Offset))
  return MyCompare(p1->Resource.PackSize, p2->Resource.PackSize);
}

static int CompareIDs(const unsigned *p1, const unsigned *p2, void *param)
{
  const CStreamInfo *streams = (const CStreamInfo *)param;
  return MyCompare(streams[*p1].Id, streams[*p2].Id);
}

static int CompareHashRefs(const unsigned *p1, const unsigned *p2, void *param)
{
  const CStreamInfo *streams = (const CStreamInfo *)param;
  return memcmp(streams[*p1].Hash, streams[*p2].Hash, kHashSize);
}

static int FindId(const CStreamInfo *streams, const CUIntVector &sorted, UInt32 id)
{
  unsigned left = 0, right = sorted.Size();
  while (left != right)
  {
    const unsigned mid = (left + right) / 2;
    const unsigned streamIndex = sorted[mid];
    const UInt32 id2 = streams[streamIndex].Id;
    if (id == id2)
      return (int)streamIndex;
    if (id < id2)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

static int FindHash(const CStreamInfo *streams, const CUIntVector &sorted, const Byte *hash)
{
  unsigned left = 0, right = sorted.Size();
  while (left != right)
  {
    const unsigned mid = (left + right) / 2;
    const unsigned streamIndex = sorted[mid];
    const Byte *hash2 = streams[streamIndex].Hash;
    unsigned i;
    for (i = 0; i < kHashSize; i++)
      if (hash[i] != hash2[i])
        break;
    if (i == kHashSize)
      return (int)streamIndex;
    if (hash[i] < hash2[i])
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

static int CompareItems(const unsigned *a1, const unsigned *a2, void *param)
{
  const CRecordVector<CItem> &items = ((CDatabase *)param)->Items;
  const CItem &i1 = items[*a1];
  const CItem &i2 = items[*a2];

  if (i1.IsDir != i2.IsDir)
    return i1.IsDir ? -1 : 1;
  if (i1.IsAltStream != i2.IsAltStream)
    return i1.IsAltStream ? 1 : -1;
  RINOZ(MyCompare(i1.StreamIndex, i2.StreamIndex))
  RINOZ(MyCompare(i1.ImageIndex, i2.ImageIndex))
  return MyCompare(i1.Offset, i2.Offset);
}


HRESULT CDatabase::FillAndCheck(const CObjectVector<CVolume> &volumes)
{
  CUIntVector sortedByHash;
  sortedByHash.Reserve(DataStreams.Size());
  {
    CByteBuffer sizesBuf;

    for (unsigned iii = 0; iii < DataStreams.Size();)
    {
      {
        const CResource &r = DataStreams[iii].Resource;
        if (!r.IsSolid())
        {
          sortedByHash.AddInReserved(iii++);
          continue;
        }
      }

      UInt64 solidRunOffset = 0;
      unsigned k;
      unsigned numSolidsStart = Solids.Size();

      for (k = iii; k < DataStreams.Size(); k++)
      {
        CStreamInfo &si = DataStreams[k];
        CResource &r = si.Resource;

        if (!r.IsSolid())
          break;
        if (!r.KeepSolid && k != iii)
          break;

        if (r.Flags != NResourceFlags::kSolid)
          return S_FALSE;

        if (!r.IsSolidBig())
          continue;

        if (!si.IsEmptyHash())
          return S_FALSE;
        if (si.RefCount != 1)
          return S_FALSE;

        r.SolidIndex = (int)Solids.Size();

        CSolid &ss = Solids.AddNew();
        ss.StreamIndex = k;
        ss.SolidOffset = solidRunOffset;
        {
          const size_t kSolidHeaderSize = 8 + 4 + 4;
          Byte header[kSolidHeaderSize];

          if (si.PartNumber >= volumes.Size())
            return S_FALSE;

          const CVolume &vol = volumes[si.PartNumber];
          IInStream *inStream = vol.Stream;
          RINOK(InStream_SeekSet(inStream, r.Offset))
          RINOK(ReadStream_FALSE(inStream, (Byte *)header, kSolidHeaderSize))
          
          ss.UnpackSize = GetUi64(header);

          if (ss.UnpackSize > ((UInt64)1 << 63))
            return S_FALSE;

          solidRunOffset += ss.UnpackSize;
          if (solidRunOffset < ss.UnpackSize)
            return S_FALSE;

          const UInt32 solidChunkSize = GetUi32(header + 8);
          if (!GetLog_val_min_dest(solidChunkSize, 8, ss.ChunkSizeBits))
            return S_FALSE;
          ss.Method = (Int32)GetUi32(header + 12);
          
          const UInt64 numChunks64 = (ss.UnpackSize + (((UInt32)1 << ss.ChunkSizeBits) - 1)) >> ss.ChunkSizeBits;
          const UInt64 sizesBufSize64 = 4 * numChunks64;
          ss.HeadersSize = kSolidHeaderSize + sizesBufSize64;
          const size_t sizesBufSize = (size_t)sizesBufSize64;
          if (sizesBufSize != sizesBufSize64)
            return E_OUTOFMEMORY;
          sizesBuf.AllocAtLeast(sizesBufSize);
          
          RINOK(ReadStream_FALSE(inStream, sizesBuf, sizesBufSize))
          
          const size_t numChunks = (size_t)numChunks64;
          ss.Chunks.Alloc(numChunks + 1);

          UInt64 offset = 0;
          
          size_t c;
          for (c = 0; c < numChunks; c++)
          {
            ss.Chunks[c] = offset;
            UInt32 packSize = GetUi32((const Byte *)sizesBuf + c * 4);
            offset += packSize;
            if (offset < packSize)
              return S_FALSE;
          }
          ss.Chunks[c] = offset;

          if (ss.Chunks[0] != 0)
            return S_FALSE;
          if (ss.HeadersSize + offset != r.PackSize)
            return S_FALSE;
        }
      }
      
      unsigned solidLim = k;

      for (k = iii; k < solidLim; k++)
      {
        CStreamInfo &si = DataStreams[k];
        CResource &r = si.Resource;

        if (!r.IsSolidSmall())
          continue;

        if (si.IsEmptyHash())
          return S_FALSE;

        unsigned solidIndex;
        {
          UInt64 offset = r.Offset;
          for (solidIndex = numSolidsStart;; solidIndex++)
          {
            if (solidIndex == Solids.Size())
              return S_FALSE;
            UInt64 unpackSize = Solids[solidIndex].UnpackSize;
            if (offset < unpackSize)
              break;
            offset -= unpackSize;
          }
        }
        CSolid &ss = Solids[solidIndex];
        if (r.Offset < ss.SolidOffset)
          return S_FALSE;
        const UInt64 relat = r.Offset - ss.SolidOffset;
        if (relat > ss.UnpackSize)
          return S_FALSE;
        if (r.PackSize > ss.UnpackSize - relat)
          return S_FALSE;
        r.SolidIndex = (int)solidIndex;
        if (ss.FirstSmallStream < 0)
          ss.FirstSmallStream = (int)k;

        sortedByHash.AddInReserved(k);
        // ss.NumRefs++;
      }
      
      iii = solidLim;
    }
  }

  if (Solids.IsEmpty())
  {
    /* We want to check that streams layout is OK.
       So we need resources sorted by offset.
       Another code can work with non-sorted streams.
       NOTE: all WIM programs probably create wim archives with
         sorted data streams. So it doesn't call Sort() here. */
       
    {
      unsigned i;
      for (i = 1; i < DataStreams.Size(); i++)
      {
        const CStreamInfo &s0 = DataStreams[i - 1];
        const CStreamInfo &s1 = DataStreams[i];
        if (s0.PartNumber < s1.PartNumber) continue;
        if (s0.PartNumber > s1.PartNumber) break;
        if (s0.Resource.Offset < s1.Resource.Offset) continue;
        if (s0.Resource.Offset > s1.Resource.Offset) break;
        if (s0.Resource.PackSize > s1.Resource.PackSize) break;
      }
      
      if (i < DataStreams.Size())
      {
        // return E_FAIL;
        DataStreams.Sort(CompareStreamsByPos, NULL);
      }
    }

    for (unsigned i = 1; i < DataStreams.Size(); i++)
    {
      const CStreamInfo &s0 = DataStreams[i - 1];
      const CStreamInfo &s1 = DataStreams[i];
      if (s0.PartNumber == s1.PartNumber)
        if (s0.Resource.GetEndLimit() > s1.Resource.Offset)
          return S_FALSE;
    }
  }
  
  {
    {
      const CStreamInfo *streams = DataStreams.ConstData();

      if (IsOldVersion)
      {
        sortedByHash.Sort(CompareIDs, (void *)streams);
        
        for (unsigned i = 1; i < sortedByHash.Size(); i++)
          if (streams[sortedByHash[i - 1]].Id >=
              streams[sortedByHash[i]].Id)
            return S_FALSE;
      }
      else
      {
        sortedByHash.Sort(CompareHashRefs, (void *)streams);

        if (!sortedByHash.IsEmpty())
        {
          if (IsEmptySha(streams[sortedByHash[0]].Hash))
            HeadersError = true;
          
          for (unsigned i = 1; i < sortedByHash.Size(); i++)
            if (memcmp(
                streams[sortedByHash[i - 1]].Hash,
                streams[sortedByHash[i]].Hash,
                kHashSize) >= 0)
              return S_FALSE;
        }
      }
    }
    
    FOR_VECTOR (i, Items)
    {
      CItem &item = Items[i];
      item.StreamIndex = -1;
      const Byte *hash = Images[item.ImageIndex].Meta + item.Offset;
      if (IsOldVersion)
      {
        if (!item.IsDir)
        {
          hash += (item.IsAltStream ? 0x8 : 0x10);
          UInt32 id = GetUi32(hash);
          if (id != 0)
            item.StreamIndex = FindId(DataStreams.ConstData(), sortedByHash, id);
        }
      }
      /*
      else if (item.IsDir)
      {
        // reparse points can have dirs some dir
      }
      */
      else
      {
        hash += (item.IsAltStream ? 0x10 : 0x40);
        if (!IsEmptySha(hash))
        {
          item.StreamIndex = FindHash(DataStreams.ConstData(), sortedByHash, hash);
        }
      }
    }
  }
  {
    CUIntVector refCounts;
    refCounts.ClearAndSetSize(DataStreams.Size());
    unsigned i;

    for (i = 0; i < DataStreams.Size(); i++)
    {
      UInt32 startVal = 0;
      // const CStreamInfo &s = DataStreams[i];
      /*
      if (s.Resource.IsMetadata() && s.PartNumber == 1)
        startVal = 1;
      */
      refCounts[i] = startVal;
    }
    
    for (i = 0; i < Items.Size(); i++)
    {
      const int streamIndex = Items[i].StreamIndex;
      if (streamIndex >= 0)
        refCounts[streamIndex]++;
    }
    
    for (i = 0; i < DataStreams.Size(); i++)
    {
      const CStreamInfo &s = DataStreams[i];
      if (s.RefCount != refCounts[i]
          && !s.Resource.IsSolidBig())
      {
        /*
        printf("\ni=%5d  si.Ref=%2d  realRefs=%2d size=%8d offset=%8x id=%4d ",
          i, s.RefCount, refCounts[i], (unsigned)s.Resource.UnpackSize, (unsigned)s.Resource.Offset, s.Id);
        */
        RefCountError = true;
      }
      
      if (refCounts[i] == 0)
      {
        const CResource &r = DataStreams[i].Resource;
        if (!r.IsSolidBig() || Solids[r.SolidIndex].FirstSmallStream < 0)
        {
          CItem item;
          item.Offset = 0;
          item.StreamIndex = (int)i;
          item.ImageIndex = -1;
          Items.Add(item);
          ThereAreDeletedStreams = true;
        }
      }
    }
  }

  return S_OK;
}


HRESULT CDatabase::GenerateSortedItems(int imageIndex, bool showImageNumber)
{
  SortedItems.Clear();
  VirtualRoots.Clear();
  IndexOfUserImage = imageIndex;
  NumExcludededItems = 0;
  ExludedItem = -1;

  if (Images.Size() != 1 && imageIndex < 0)
    showImageNumber = true;

  unsigned startItem = 0;
  unsigned endItem = 0;
  
  if (imageIndex < 0)
  {
    endItem = Items.Size();
    if (Images.Size() == 1)
    {
      IndexOfUserImage = 0;
      const CImage &image = Images[0];
      if (!showImageNumber)
        NumExcludededItems = image.NumEmptyRootItems;
    }
  }
  else if ((unsigned)imageIndex < Images.Size())
  {
    const CImage &image = Images[imageIndex];
    startItem = image.StartItem;
    endItem = startItem + image.NumItems;
    if (!showImageNumber)
      NumExcludededItems = image.NumEmptyRootItems;
  }
  
  if (NumExcludededItems != 0)
  {
    ExludedItem = (int)startItem;
    startItem += NumExcludededItems;
  }

  unsigned num = endItem - startItem;
  SortedItems.ClearAndSetSize(num);
  unsigned i;
  for (i = 0; i < num; i++)
    SortedItems[i] = startItem + i;

  SortedItems.Sort(CompareItems, this);
  for (i = 0; i < SortedItems.Size(); i++)
    Items[SortedItems[i]].IndexInSorted = (int)i;

  if (showImageNumber)
    for (i = 0; i < Images.Size(); i++)
    {
      CImage &image = Images[i];
      if (image.NumEmptyRootItems != 0)
        continue;
      image.VirtualRootIndex = (int)VirtualRoots.Size();
      VirtualRoots.Add(i);
    }

  return S_OK;
}


static void IntVector_SetMinusOne_IfNeed(CIntVector &v, unsigned size)
{
  if (v.Size() == size)
    return;
  v.ClearAndSetSize(size);
  int *vals = &v[0];
  for (unsigned i = 0; i < size; i++)
    vals[i] = -1;
}


HRESULT CDatabase::ExtractReparseStreams(const CObjectVector<CVolume> &volumes, IArchiveOpenCallback *openCallback)
{
  ItemToReparse.Clear();
  ReparseItems.Clear();
  
  // we don't know about Reparse field for OLD WIM format
  if (IsOldVersion)
    return S_OK;

  CIntVector streamToReparse;
  CUnpacker unpacker;
  UInt64 totalPackedPrev = 0;

  FOR_VECTOR(indexInSorted, SortedItems)
  {
    // we use sorted items for faster access
    unsigned itemIndex = SortedItems[indexInSorted];
    const CItem &item = Items[itemIndex];
    
    if (!item.HasMetadata() || item.IsAltStream)
      continue;
    
    if (item.ImageIndex < 0)
      continue;
    
    const Byte *metadata = Images[item.ImageIndex].Meta + item.Offset;
    
    const UInt32 attrib = Get32(metadata + 8);
    if ((attrib & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
      continue;
    
    if (item.StreamIndex < 0)
      continue; // it's ERROR
    
    const CStreamInfo &si = DataStreams[item.StreamIndex];
    if (si.Resource.UnpackSize >= (1 << 16))
      continue; // reparse data can not be larger than 64 KB

    IntVector_SetMinusOne_IfNeed(streamToReparse, DataStreams.Size());
    IntVector_SetMinusOne_IfNeed(ItemToReparse, Items.Size());
    
    const unsigned offset = 0x58; // we don't know about Reparse field for OLD WIM format
    UInt32 tag = Get32(metadata + offset);
    int reparseIndex = streamToReparse[item.StreamIndex];
    CByteBuffer buf;

    if (openCallback)
    {
      if ((unpacker.TotalPacked - totalPackedPrev) >= ((UInt32)1 << 16))
      {
        UInt64 numFiles = Items.Size();
        RINOK(openCallback->SetCompleted(&numFiles, &unpacker.TotalPacked))
        totalPackedPrev = unpacker.TotalPacked;
      }
    }

    if (reparseIndex >= 0)
    {
      const CByteBuffer &reparse = ReparseItems[reparseIndex];
      if (tag == Get32(reparse))
      {
        ItemToReparse[itemIndex] = reparseIndex;
        continue;
      }
      buf = reparse;
      // we support that strange and unusual situation with different tags and same reparse data.
    }
    else
    {
      /*
      if (si.PartNumber >= volumes.Size())
        continue;
      */
      const CVolume &vol = volumes[si.PartNumber];
      /*
      if (!vol.Stream)
        continue;
      */
      
      Byte digest[kHashSize];
      HRESULT res = unpacker.UnpackData(vol.Stream, si.Resource, vol.Header, this, buf, digest);

      if (res == S_FALSE)
        continue;

      RINOK(res)
      
      if (memcmp(digest, si.Hash, kHashSize) != 0
        // && !(h.IsOldVersion() && IsEmptySha(si.Hash))
        )
      {
        // setErrorStatus;
        continue;
      }
    }
    
    CByteBuffer &reparse = ReparseItems.AddNew();
    reparse.Alloc(8 + buf.Size());
    Byte *dest = (Byte *)reparse;
    SetUi32(dest, tag)
    SetUi32(dest + 4, (UInt32)buf.Size())
    if (buf.Size() != 0)
      memcpy(dest + 8, buf, buf.Size());
    ItemToReparse[itemIndex] = (int)ReparseItems.Size() - 1;
  }

  return S_OK;
}



static bool ParseNumber64(const AString &s, UInt64 &res)
{
  const char *end;
  if (s.IsPrefixedBy("0x"))
  {
    if (s.Len() == 2)
      return false;
    res = ConvertHexStringToUInt64(s.Ptr(2), &end);
  }
  else
  {
    if (s.IsEmpty())
      return false;
    res = ConvertStringToUInt64(s, &end);
  }
  return *end == 0;
}


static bool ParseNumber32(const AString &s, UInt32 &res)
{
  UInt64 res64;
  if (!ParseNumber64(s, res64) || res64 >= ((UInt64)1 << 32))
    return false;
  res = (UInt32)res64;
  return true;
}


static bool ParseTime(const CXmlItem &item, FILETIME &ft, const char *tag)
{
  const CXmlItem *timeItem = item.FindSubTag_GetPtr(tag);
  if (timeItem)
  {
    UInt32 low = 0, high = 0;
    if (ParseNumber32(timeItem->GetSubStringForTag("LOWPART"), low) &&
        ParseNumber32(timeItem->GetSubStringForTag("HIGHPART"), high))
    {
      ft.dwLowDateTime = low;
      ft.dwHighDateTime = high;
      return true;
    }
  }
  return false;
}


void CImageInfo::Parse(const CXmlItem &item)
{
  CTimeDefined = ParseTime(item, CTime, "CREATIONTIME");
  MTimeDefined = ParseTime(item, MTime, "LASTMODIFICATIONTIME");
  NameDefined = true;
  ConvertUTF8ToUnicode(item.GetSubStringForTag("NAME"), Name);

  ParseNumber64(item.GetSubStringForTag("DIRCOUNT"), DirCount);
  ParseNumber64(item.GetSubStringForTag("FILECOUNT"), FileCount);
  IndexDefined = ParseNumber32(item.GetPropVal("INDEX"), Index);
}

void CWimXml::ToUnicode(UString &s)
{
  size_t size = Data.Size();
  if (size < 2 || (size & 1) != 0 || size > (1 << 24))
    return;
  const Byte *p = Data;
  if (Get16(p) != 0xFEFF)
    return;
  wchar_t *chars = s.GetBuf((unsigned)(size / 2));
  for (size_t i = 2; i < size; i += 2)
  {
    wchar_t c = Get16(p + i);
    if (c == 0)
      break;
    *chars++ = c;
  }
  *chars = 0;
  s.ReleaseBuf_SetLen((unsigned)(chars - (const wchar_t *)s));
}


bool CWimXml::Parse()
{
  IsEncrypted = false;
  AString utf;
  {
    UString s;
    ToUnicode(s);
    // if (!ConvertUnicodeToUTF8(s, utf)) return false;
    ConvertUnicodeToUTF8(s, utf);
  }

  if (!Xml.Parse(utf))
    return false;
  if (!Xml.Root.Name.IsEqualTo("WIM"))
    return false;

  FOR_VECTOR (i, Xml.Root.SubItems)
  {
    const CXmlItem &item = Xml.Root.SubItems[i];
    
    if (item.IsTagged("IMAGE"))
    {
      CImageInfo imageInfo;
      imageInfo.Parse(item);
      if (!imageInfo.IndexDefined)
        return false;

      if (imageInfo.Index != (UInt32)Images.Size() + 1)
      {
        // old wim (1.09) uses zero based image index
        if (imageInfo.Index != (UInt32)Images.Size())
          return false;
      }

      imageInfo.ItemIndexInXml = (int)i;
      Images.Add(imageInfo);
    }

    if (item.IsTagged("ESD"))
    {
      FOR_VECTOR (k, item.SubItems)
      {
        const CXmlItem &item2 = item.SubItems[k];
        if (item2.IsTagged("ENCRYPTED"))
          IsEncrypted = true;
      }
    }
  }

  return true;
}

}}

/* ================ unit: CPP/7zip/Archive/Wim/WimRegister.cpp ================ */
// WimRegister.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NWim {

REGISTER_ARC_IO(
    "wim", "wim swm esd ppkg", NULL, 0xE6
  , kSignature, 0
  , NArcInfoFlags::kAltStreams
  | NArcInfoFlags::kNtSecure
  | NArcInfoFlags::kSymLinks
  | NArcInfoFlags::kHardLinks
  | NArcInfoFlags::kCTime
  // | NArcInfoFlags::kCTime_Default
  | NArcInfoFlags::kATime
  // | NArcInfoFlags::kATime_Default
  | NArcInfoFlags::kMTime
  | NArcInfoFlags::kMTime_Default
  , TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kWindows)
  | TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT (NFileTimeType::kWindows)
  , NULL)

}}
