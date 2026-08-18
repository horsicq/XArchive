/* XArchive amalgamation of 7-Zip 26.01 -- Archive/7z reader.
 *
 * 8 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Archive/7z/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- CPP/7zip/Common/MethodId.h ---- */
// MethodId.h

#ifndef ZIP7_INC_7Z_METHOD_ID_H
#define ZIP7_INC_7Z_METHOD_ID_H

// amalgamation: header emitted in prologue

typedef UInt64 CMethodId;

#endif

/* ---- CPP/7zip/Common/CreateCoder.h ---- */
// CreateCoder.h

#ifndef ZIP7_INC_CREATE_CODER_H
#define ZIP7_INC_CREATE_CODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

/*
  if Z7_EXTERNAL_CODECS is not defined, the code supports only codecs that
      are statically linked at compile-time and link-time.

  if Z7_EXTERNAL_CODECS is defined, the code supports also codecs from another
      executable modules, that can be linked dynamically at run-time:
        - EXE module can use codecs from external DLL files.
        - DLL module can use codecs from external EXE and DLL files.
     
      CExternalCodecs contains information about codecs and interfaces to create them.
  
  The order of codecs:
    1) Internal codecs
    2) External codecs
*/

#ifdef Z7_EXTERNAL_CODECS

struct CCodecInfoEx
{
  CMethodId Id;
  AString Name;
  UInt32 NumStreams;
  bool EncoderIsAssigned;
  bool DecoderIsAssigned;
  bool IsFilter; // it's unused
  
  CCodecInfoEx(): EncoderIsAssigned(false), DecoderIsAssigned(false), IsFilter(false) {}
};

struct CHasherInfoEx
{
  CMethodId Id;
  AString Name;
};

#define Z7_PUBLIC_ISetCompressCodecsInfo_IFEC \
    public ISetCompressCodecsInfo,
#define Z7_COM_QI_ENTRY_ISetCompressCodecsInfo_IFEC \
    Z7_COM_QI_ENTRY(ISetCompressCodecsInfo)
#define DECL_ISetCompressCodecsInfo \
    Z7_COM7F_IMP(SetCompressCodecsInfo(ICompressCodecsInfo *compressCodecsInfo))
#define IMPL_ISetCompressCodecsInfo2(cls) \
    Z7_COM7F_IMF(cls::SetCompressCodecsInfo(ICompressCodecsInfo *compressCodecsInfo)) \
    { COM_TRY_BEGIN _externalCodecs.GetCodecs = compressCodecsInfo; \
    return _externalCodecs.Load(); COM_TRY_END }
#define IMPL_ISetCompressCodecsInfo  IMPL_ISetCompressCodecsInfo2(CHandler)

struct CExternalCodecs
{
  CMyComPtr<ICompressCodecsInfo> GetCodecs;
  CMyComPtr<IHashers> GetHashers;

  CObjectVector<CCodecInfoEx> Codecs;
  CObjectVector<CHasherInfoEx> Hashers;

  bool IsSet() const { return GetCodecs != NULL || GetHashers != NULL; }

  HRESULT Load();

  void ClearAndRelease()
  {
    Hashers.Clear();
    Codecs.Clear();
    GetHashers.Release();
    GetCodecs.Release();
  }

  ~CExternalCodecs()
  {
    GetHashers.Release();
    GetCodecs.Release();
  }
};

extern CExternalCodecs g_ExternalCodecs;

#define EXTERNAL_CODECS_VARS2   (_externalCodecs.IsSet() ? &_externalCodecs : &g_ExternalCodecs)
#define EXTERNAL_CODECS_VARS2_L (&_externalCodecs)
#define EXTERNAL_CODECS_VARS2_G (&g_ExternalCodecs)

#define DECL_EXTERNAL_CODECS_VARS CExternalCodecs _externalCodecs;

#define EXTERNAL_CODECS_VARS   EXTERNAL_CODECS_VARS2,
#define EXTERNAL_CODECS_VARS_L EXTERNAL_CODECS_VARS2_L,
#define EXTERNAL_CODECS_VARS_G EXTERNAL_CODECS_VARS2_G,

#define DECL_EXTERNAL_CODECS_LOC_VARS2      const CExternalCodecs *_externalCodecs
#define DECL_EXTERNAL_CODECS_LOC_VARS       DECL_EXTERNAL_CODECS_LOC_VARS2,
#define DECL_EXTERNAL_CODECS_LOC_VARS_DECL  DECL_EXTERNAL_CODECS_LOC_VARS2;

#define EXTERNAL_CODECS_LOC_VARS2   _externalCodecs
#define EXTERNAL_CODECS_LOC_VARS    EXTERNAL_CODECS_LOC_VARS2,

#else

#define Z7_PUBLIC_ISetCompressCodecsInfo_IFEC
#define Z7_COM_QI_ENTRY_ISetCompressCodecsInfo_IFEC
#define DECL_ISetCompressCodecsInfo
#define IMPL_ISetCompressCodecsInfo
#define EXTERNAL_CODECS_VARS2
#define DECL_EXTERNAL_CODECS_VARS
#define EXTERNAL_CODECS_VARS
#define EXTERNAL_CODECS_VARS_L
#define EXTERNAL_CODECS_VARS_G
#define DECL_EXTERNAL_CODECS_LOC_VARS2
#define DECL_EXTERNAL_CODECS_LOC_VARS
#define DECL_EXTERNAL_CODECS_LOC_VARS_DECL
#define EXTERNAL_CODECS_LOC_VARS2
#define EXTERNAL_CODECS_LOC_VARS

#endif

int FindMethod_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const AString &name,
    bool encode,
    CMethodId &methodId,
    UInt32 &numStreams,
    bool &isFilter);

bool FindMethod(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId,
    AString &name);

bool FindHashMethod(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const AString &name,
    CMethodId &methodId);

void GetHashMethods(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CRecordVector<CMethodId> &methods);


struct CCreatedCoder
{
  CMyComPtr<ICompressCoder> Coder;
  CMyComPtr<ICompressCoder2> Coder2;
  
  bool IsExternal;
  bool IsFilter; // = true, if Coder was created from filter
  UInt32 NumStreams;

  // CCreatedCoder(): IsExternal(false), IsFilter(false), NumStreams(1) {}
};


HRESULT CreateCoder_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    unsigned codecIndex, bool encode,
    CMyComPtr<ICompressFilter> &filter,
    CCreatedCoder &cod);

HRESULT CreateCoder_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    unsigned index, bool encode,
    CCreatedCoder &cod);

HRESULT CreateCoder_Id(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CMyComPtr<ICompressFilter> &filter,
    CCreatedCoder &cod);

HRESULT CreateCoder_Id(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CCreatedCoder &cod);

HRESULT CreateCoder_Id(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CMyComPtr<ICompressCoder> &coder);

HRESULT CreateFilter(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CMyComPtr<ICompressFilter> &filter);

HRESULT CreateHasher(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId,
    AString &name,
    CMyComPtr<IHasher> &hasher);

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

/* ---- CPP/Windows/Handle.h ---- */
// Windows/Handle.h

#ifndef ZIP7_INC_WINDOWS_HANDLE_H
#define ZIP7_INC_WINDOWS_HANDLE_H

// amalgamation: header emitted in prologue

#ifdef _WIN32
namespace NWindows {

class CHandle  MY_UNCOPYABLE
{
protected:
  HANDLE _handle;
public:
  operator HANDLE() { return _handle; }
  CHandle(): _handle(NULL) {}
  ~CHandle() { Close(); }
  bool IsCreated() const { return (_handle != NULL); }
  bool Close()
  {
    if (_handle == NULL)
      return true;
    if (!::CloseHandle(_handle))
      return false;
    _handle = NULL;
    return true;
  }
  void Attach(HANDLE handle) { _handle = handle; }
  HANDLE Detach()
  {
    const HANDLE handle = _handle;
    _handle = NULL;
    return handle;
  }
};

}
#endif // _WIN32

#endif

/* ---- CPP/Windows/Synchronization.h ---- */
// Windows/Synchronization.h

#ifndef ZIP7_INC_WINDOWS_SYNCHRONIZATION_H
#define ZIP7_INC_WINDOWS_SYNCHRONIZATION_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#ifdef _WIN32
// amalgamation: header emitted in prologue
#endif

namespace NWindows {
namespace NSynchronization {

class CBaseEvent  MY_UNCOPYABLE
{
protected:
  ::CEvent _object;
public:
  bool IsCreated() { return Event_IsCreated(&_object) != 0; }

  CBaseEvent() { Event_Construct(&_object); }
  ~CBaseEvent() { Close(); }
  WRes Close() { return Event_Close(&_object); }

  #ifdef _WIN32
  operator HANDLE() { return _object; }
  WRes Create(bool manualReset, bool initiallyOwn, LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES sa = NULL)
  {
    _object = ::CreateEvent(sa, BoolToBOOL(manualReset), BoolToBOOL(initiallyOwn), name);
    if (name == NULL && _object != NULL)
      return 0;
    return ::GetLastError();
  }
  WRes Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _object = ::OpenEvent(desiredAccess, BoolToBOOL(inheritHandle), name);
    if (_object != NULL)
      return 0;
    return ::GetLastError();
  }
  #endif

  WRes Set() { return Event_Set(&_object); }
  // bool Pulse() { return BOOLToBool(::PulseEvent(_handle)); }
  WRes Reset() { return Event_Reset(&_object); }
  WRes Lock() { return Event_Wait(&_object); }
};

class CManualResetEvent: public CBaseEvent
{
public:
  WRes Create(bool initiallyOwn = false)
  {
    return ManualResetEvent_Create(&_object, initiallyOwn ? 1: 0);
  }
  WRes CreateIfNotCreated_Reset()
  {
    if (IsCreated())
      return Reset();
    return ManualResetEvent_CreateNotSignaled(&_object);
  }
  #ifdef _WIN32
  WRes CreateWithName(bool initiallyOwn, LPCTSTR name)
  {
    return CBaseEvent::Create(true, initiallyOwn, name);
  }
  #endif
};

class CAutoResetEvent: public CBaseEvent
{
public:
  WRes Create()
  {
    return AutoResetEvent_CreateNotSignaled(&_object);
  }
  WRes CreateIfNotCreated_Reset()
  {
    if (IsCreated())
      return Reset();
    return AutoResetEvent_CreateNotSignaled(&_object);
  }
};


/*
#ifdef _WIN32

class CObject: public CHandle
{
public:
  WRes Lock(DWORD timeoutInterval = INFINITE)
    { return (::WaitForSingleObject(_handle, timeoutInterval) == WAIT_OBJECT_0 ? 0 : ::GetLastError()); }
};

class CMutex: public CObject
{
public:
  WRes Create(bool initiallyOwn, LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES sa = NULL)
  {
    _handle = ::CreateMutex(sa, BoolToBOOL(initiallyOwn), name);
    if (name == NULL && _handle != 0)
      return 0;
    return ::GetLastError();
  }
  #ifndef UNDER_CE
  WRes Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _handle = ::OpenMutex(desiredAccess, BoolToBOOL(inheritHandle), name);
    if (_handle != 0)
      return 0;
    return ::GetLastError();
  }
  #endif
  WRes Release()
  {
    return ::ReleaseMutex(_handle) ? 0 : ::GetLastError();
  }
};

class CMutexLock  MY_UNCOPYABLE
{
  CMutex *_object;
public:
  CMutexLock(CMutex &object): _object(&object) { _object->Lock(); }
  ~CMutexLock() { _object->Release(); }
};

#endif // _WIN32
*/


class CSemaphore  MY_UNCOPYABLE
{
  ::CSemaphore _object;
public:
  CSemaphore() { Semaphore_Construct(&_object); }
  ~CSemaphore() { Close(); }
  WRes Close() { return Semaphore_Close(&_object); }

  #ifdef _WIN32
  operator HANDLE() { return _object; }
  #endif

  // bool IsCreated() const { return Semaphore_IsCreated(&_object) != 0; }

  WRes Create(UInt32 initCount, UInt32 maxCount)
  {
    return Semaphore_Create(&_object, initCount, maxCount);
  }
  WRes OptCreateInit(UInt32 initCount, UInt32 maxCount)
  {
    return Semaphore_OptCreateInit(&_object, initCount, maxCount);
  }
  WRes Release() { return Semaphore_Release1(&_object); }
  WRes Release(UInt32 releaseCount) { return Semaphore_ReleaseN(&_object, releaseCount); }
  WRes Lock() { return Semaphore_Wait(&_object); }
};

class CCriticalSection  MY_UNCOPYABLE
{
  ::CCriticalSection _object;
public:
  CCriticalSection() { CriticalSection_Init(&_object); }
  ~CCriticalSection() { CriticalSection_Delete(&_object); }
  void Enter() { CriticalSection_Enter(&_object); }
  void Leave() { CriticalSection_Leave(&_object); }
};

class CCriticalSectionLock  MY_UNCOPYABLE
{
  CCriticalSection *_object;
  void Unlock()  { _object->Leave(); }
public:
  CCriticalSectionLock(CCriticalSection &object): _object(&object) {_object->Enter(); }
  ~CCriticalSectionLock() { Unlock(); }
};


#ifdef _WIN32

typedef HANDLE CHandle_WFMO;
typedef CSemaphore CSemaphore_WFMO;
typedef CAutoResetEvent CAutoResetEvent_WFMO;
typedef CManualResetEvent CManualResetEvent_WFMO;

inline DWORD WINAPI WaitForMultiObj_Any_Infinite(DWORD count, const CHandle_WFMO *handles)
{
  return ::WaitForMultipleObjects(count, handles, FALSE, INFINITE);
}

#define SYNC_OBJ_DECL(obj)
#define SYNC_WFMO(x)
#define SYNC_PARAM(x)
#define SYNC_PARAM_DECL(x)

#else //  _WIN32

// POSIX sync objects for WaitForMultipleObjects

#define SYNC_WFMO(x) x
#define SYNC_PARAM(x) x,
#define SYNC_PARAM_DECL(x) NWindows::NSynchronization::CSynchro *x
#define SYNC_OBJ_DECL(x) NWindows::NSynchronization::CSynchro x;

class CSynchro  MY_UNCOPYABLE
{
  pthread_mutex_t _mutex;
  pthread_cond_t _cond;
  bool _isValid;

public:
  CSynchro() { _isValid = false; }
  ~CSynchro()
  {
    if (_isValid)
    {
      ::pthread_mutex_destroy(&_mutex);
      ::pthread_cond_destroy(&_cond);
    }
    _isValid = false;
  }
  WRes Create()
  {
    RINOK(::pthread_mutex_init(&_mutex, NULL))
    const WRes ret = ::pthread_cond_init(&_cond, NULL);
    _isValid = 1;
    return ret;
  }
  WRes Enter()
  {
#if defined(Z7_LLVM_CLANG_VERSION) && (__clang_major__ == 13) \
      && defined(__FreeBSD__)
  #pragma GCC diagnostic ignored "-Wthread-safety-negative"
  #pragma GCC diagnostic ignored "-Wthread-safety-analysis"
#endif
    return ::pthread_mutex_lock(&_mutex);
  }
  WRes Leave()
  {
    return ::pthread_mutex_unlock(&_mutex);
  }
  WRes WaitCond()
  {
    return ::pthread_cond_wait(&_cond, &_mutex);
  }
  WRes LeaveAndSignal()
  {
    const WRes res1 = ::pthread_cond_broadcast(&_cond);
    const WRes res2 = ::pthread_mutex_unlock(&_mutex);
    return (res2 ? res2 : res1);
  }
};


struct CBaseHandle_WFMO;
typedef NWindows::NSynchronization::CBaseHandle_WFMO *CHandle_WFMO;

// these constants are from Windows
#define WAIT_OBJECT_0 0
#define WAIT_FAILED ((DWORD)0xFFFFFFFF)

DWORD WINAPI WaitForMultiObj_Any_Infinite(DWORD count, const CHandle_WFMO *handles);


struct CBaseHandle_WFMO  MY_UNCOPYABLE
{
  CSynchro *_sync;

  CBaseHandle_WFMO(): _sync(NULL) {}
  virtual ~CBaseHandle_WFMO();

  operator CHandle_WFMO() { return this; }
  virtual bool IsSignaledAndUpdate() = 0;
};


class CBaseEvent_WFMO : public CBaseHandle_WFMO
{
  bool _manual_reset;
  bool _state;

public:

  // bool IsCreated()  { return (this->_sync != NULL); }
  // CBaseEvent_WFMO()  { ; }
  // ~CBaseEvent_WFMO() Z7_override { Close(); }

  WRes Close() { this->_sync = NULL; return 0; }

  WRes Create(
      CSynchro *sync,
      bool manualReset, bool initiallyOwn)
  {
    this->_sync         = sync;
    this->_manual_reset = manualReset;
    this->_state        = initiallyOwn;
    return 0;
  }

  WRes Set()
  {
    RINOK(this->_sync->Enter())
    this->_state = true;
    return this->_sync->LeaveAndSignal();
  }

  WRes Reset()
  {
    RINOK(this->_sync->Enter())
    this->_state = false;
    return this->_sync->Leave();
  }
  
  virtual bool IsSignaledAndUpdate() Z7_override;
};


class CManualResetEvent_WFMO Z7_final: public CBaseEvent_WFMO
{
public:
  WRes Create(CSynchro *sync, bool initiallyOwn = false) { return CBaseEvent_WFMO::Create(sync, true, initiallyOwn); }
};


class CAutoResetEvent_WFMO Z7_final: public CBaseEvent_WFMO
{
public:
  WRes Create(CSynchro *sync) { return CBaseEvent_WFMO::Create(sync, false, false); }
  WRes CreateIfNotCreated_Reset(CSynchro *sync)
  {
    return Create(sync);
  }
};


class CSemaphore_WFMO Z7_final: public CBaseHandle_WFMO
{
  UInt32 _count;
  UInt32 _maxCount;

public:
  CSemaphore_WFMO() : _count(0), _maxCount(0) {}
  
  WRes Close() { this->_sync = NULL; return 0; }

  WRes Create(CSynchro *sync, UInt32 initCount, UInt32 maxCount)
  {
    if (initCount > maxCount || maxCount < 1)
      return EINVAL;
    this->_sync     = sync;
    this->_count    = initCount;
    this->_maxCount = maxCount;
    return 0;
  }
  
  WRes Release(UInt32 releaseCount = 1)
  {
    if (releaseCount < 1)
      return EINVAL;

    RINOK(this->_sync->Enter())
    UInt32 newCount = this->_count + releaseCount;
    if (newCount > this->_maxCount)
    {
      RINOK(this->_sync->Leave())
      return ERROR_TOO_MANY_POSTS; // EINVAL
    }
    this->_count = newCount;

    return this->_sync->LeaveAndSignal();
  }

  virtual bool IsSignaledAndUpdate() Z7_override;
};

#endif // _WIN32

}}

#endif

/* ---- CPP/7zip/Common/StreamBinder.h ---- */
// StreamBinder.h

#ifndef ZIP7_INC_STREAM_BINDER_H
#define ZIP7_INC_STREAM_BINDER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

/*
We can use one from two code versions here: with Event or with Semaphore to unlock Writer thread
The difference for cases where Reading must be closed before Writing closing

1) Event Version: _canWrite_Event
  We call _canWrite_Event.Set() without waiting _canRead_Event in CloseRead() function.
  The writer thread can get (_readingWasClosed) status in one from two iterations.
  It's ambiguity of processing flow. But probably it's SAFE to use, if Event functions provide memory barriers.
  reader thread:
     _canWrite_Event.Set();
     _readingWasClosed = true;
     _canWrite_Event.Set();
  writer thread:
     _canWrite_Event.Wait()
      if (_readingWasClosed)

2) Semaphore Version: _canWrite_Semaphore
  writer thread always will detect closing of reading in latest iteration after all data processing iterations
*/

class CStreamBinder
{
  NWindows::NSynchronization::CAutoResetEvent _canRead_Event;
  // NWindows::NSynchronization::CAutoResetEvent _canWrite_Event;
  NWindows::NSynchronization::CSemaphore _canWrite_Semaphore;

  // bool _readingWasClosed;  // set it in reader thread and check it in write thread
  bool _readingWasClosed2; // use it in writer thread
  // bool WritingWasCut;
  bool _waitWrite;         // use it in reader thread
  UInt32 _bufSize;
  const void *_buf;
public:
  UInt64 ProcessedSize;   // the size that was read by reader thread

  void CreateStreams2(CMyComPtr<ISequentialInStream> &inStream, CMyComPtr<ISequentialOutStream> &outStream);
  
  HRESULT Create_ReInit();
  
  HRESULT Read(void *data, UInt32 size, UInt32 *processedSize);
  HRESULT Write(const void *data, UInt32 size, UInt32 *processedSize);

  void CloseRead_CallOnce()
  {
    // call it only once: for example, in destructor
    
    /*
    _readingWasClosed = true;
    _canWrite_Event.Set();
    */

    /*
    We must relase Semaphore only once !!!
    we must release at least 2 items of Semaphore:
      one item to unlock partial Write(), if Read() have read some items
      then additional item to stop writing (_bufSize will be 0)
    */
    _canWrite_Semaphore.Release(2);
  }
  
  void CloseWrite()
  {
    _buf = NULL;
    _bufSize = 0;
    _canRead_Event.Set();
  }
};

#endif

/* ---- CPP/Windows/Thread.h ---- */
// Windows/Thread.h

#ifndef ZIP7_INC_WINDOWS_THREAD_H
#define ZIP7_INC_WINDOWS_THREAD_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NWindows {

class CThread  MY_UNCOPYABLE
{
  ::CThread thread;
public:
  CThread() { Thread_CONSTRUCT(&thread) }
  ~CThread() { Close(); }
  bool IsCreated() { return Thread_WasCreated(&thread) != 0; }
  WRes Close()  { return Thread_Close(&thread); }
  // WRes Wait() { return Thread_Wait(&thread); }
  WRes Wait_Close() { return Thread_Wait_Close(&thread); }

  WRes Create(THREAD_FUNC_TYPE startAddress, LPVOID param)
    { return Thread_Create(&thread, startAddress, param); }
  WRes Create_With_Affinity(THREAD_FUNC_TYPE startAddress, LPVOID param, CAffinityMask affinity)
    { return Thread_Create_With_Affinity(&thread, startAddress, param, affinity); }
  WRes Create_With_CpuSet(THREAD_FUNC_TYPE startAddress, LPVOID param, const CCpuSet *cpuSet)
    { return Thread_Create_With_CpuSet(&thread, startAddress, param, cpuSet); }
 
#ifdef _WIN32
  WRes Create_With_Group(THREAD_FUNC_TYPE startAddress, LPVOID param, unsigned group, CAffinityMask affinity = 0)
    { return Thread_Create_With_Group(&thread, startAddress, param, group, affinity); }
  operator HANDLE() { return thread; }
  void Attach(HANDLE handle) { thread = handle; }
  HANDLE Detach() { HANDLE h = thread; thread = NULL; return h; }
  DWORD Resume() { return ::ResumeThread(thread); }
  DWORD Suspend() { return ::SuspendThread(thread); }
  bool Terminate(DWORD exitCode) { return BOOLToBool(::TerminateThread(thread, exitCode)); }
  int GetPriority() { return ::GetThreadPriority(thread); }
  bool SetPriority(int priority) { return BOOLToBool(::SetThreadPriority(thread, priority)); }
#endif
};

}

#endif

/* ---- CPP/7zip/Common/VirtThread.h ---- */
// VirtThread.h

#ifndef ZIP7_INC_VIRT_THREAD_H
#define ZIP7_INC_VIRT_THREAD_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

struct CVirtThread
{
  NWindows::NSynchronization::CAutoResetEvent StartEvent;
  NWindows::NSynchronization::CAutoResetEvent FinishedEvent;
  NWindows::CThread Thread;
  bool Exit;

  virtual ~CVirtThread() { WaitThreadFinish(); }
  void WaitThreadFinish(); // call it in destructor of child class !
  WRes Create();
  WRes Start();
  virtual void Execute() = 0;
  WRes WaitExecuteFinish() { return FinishedEvent.Lock(); }
};

#endif

/* ---- CPP/7zip/Archive/Common/CoderMixer2.h ---- */
// CoderMixer2.h

#ifndef ZIP7_INC_CODER_MIXER2_H
#define ZIP7_INC_CODER_MIXER2_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#ifdef Z7_ST
  #define USE_MIXER_ST
#else
  #define USE_MIXER_MT
  #ifndef Z7_SFX
    #define USE_MIXER_ST
  #endif
#endif

#ifdef USE_MIXER_MT
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#endif



#ifdef USE_MIXER_ST

Z7_CLASS_IMP_COM_1(
  CSequentialInStreamCalcSize
  , ISequentialInStream
)
  bool _wasFinished;
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    _wasFinished = false;
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  bool WasFinished() const { return _wasFinished; }
};


Z7_CLASS_IMP_COM_2(
  COutStreamCalcSize
  , ISequentialOutStream
  , IOutStreamFinish
)
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _size = 0; }
  UInt64 GetSize() const { return _size; }
};

#endif


  
namespace NCoderMixer2 {

struct CBond
{
  UInt32 PackIndex;
  UInt32 UnpackIndex;

  UInt32 Get_InIndex(bool encodeMode) const { return encodeMode ? UnpackIndex : PackIndex; }
  UInt32 Get_OutIndex(bool encodeMode) const { return encodeMode ? PackIndex : UnpackIndex; }
};


struct CCoderStreamsInfo
{
  UInt32 NumStreams;
};


struct CBindInfo
{
  CRecordVector<CCoderStreamsInfo> Coders;
  CRecordVector<CBond> Bonds;
  CRecordVector<UInt32> PackStreams;
  unsigned UnpackCoder;

  unsigned GetNum_Bonds_and_PackStreams() const { return Bonds.Size() + PackStreams.Size(); }

  int FindBond_for_PackStream(UInt32 packStream) const
  {
    FOR_VECTOR (i, Bonds)
      if (Bonds[i].PackIndex == packStream)
        return (int)i;
    return -1;
  }

  int FindBond_for_UnpackStream(UInt32 unpackStream) const
  {
    FOR_VECTOR (i, Bonds)
      if (Bonds[i].UnpackIndex == unpackStream)
        return (int)i;
    return -1;
  }

  bool SetUnpackCoder()
  {
    bool isOk = false;
    FOR_VECTOR (i, Coders)
    {
      if (FindBond_for_UnpackStream(i) < 0)
      {
        if (isOk)
          return false;
        UnpackCoder = i;
        isOk = true;
      }
    }
    return isOk;
  }
  
  bool IsStream_in_PackStreams(UInt32 streamIndex) const
  {
    return FindStream_in_PackStreams(streamIndex) >= 0;
  }

  int FindStream_in_PackStreams(UInt32 streamIndex) const
  {
    FOR_VECTOR (i, PackStreams)
      if (PackStreams[i] == streamIndex)
        return (int)i;
    return -1;
  }

  
  // that function is used before Maps is calculated

  UInt32 GetStream_for_Coder(UInt32 coderIndex) const
  {
    UInt32 streamIndex = 0;
    for (UInt32 i = 0; i < coderIndex; i++)
      streamIndex += Coders[i].NumStreams;
    return streamIndex;
  }
  
  // ---------- Maps Section ----------
  
  CRecordVector<UInt32> Coder_to_Stream;
  CRecordVector<UInt32> Stream_to_Coder;

  void ClearMaps();
  bool CalcMapsAndCheck();

  // ---------- End of Maps Section ----------

  void Clear()
  {
    Coders.Clear();
    Bonds.Clear();
    PackStreams.Clear();

    ClearMaps();
  }
  
  void GetCoder_for_Stream(UInt32 streamIndex, UInt32 &coderIndex, UInt32 &coderStreamIndex) const
  {
    coderIndex = Stream_to_Coder[streamIndex];
    coderStreamIndex = streamIndex - Coder_to_Stream[coderIndex];
  }
};



class CCoder
{
  Z7_CLASS_NO_COPY(CCoder)
public:
  CMyComPtr<ICompressCoder> Coder;
  CMyComPtr<ICompressCoder2> Coder2;
  UInt32 NumStreams;
  bool Finish;

  UInt64 UnpackSize;
  const UInt64 *UnpackSizePointer;

  CRecordVector<UInt64> PackSizes;
  CRecordVector<const UInt64 *> PackSizePointers;

  CCoder(): Finish(false) {}

  void SetCoderInfo(const UInt64 *unpackSize, const UInt64 * const *packSizes, bool finish);

  HRESULT CheckDataAfterEnd(bool &dataAfterEnd_Error /* , bool &InternalPackSizeError */) const;

  IUnknown *GetUnknown() const
  {
    return Coder ? (IUnknown *)Coder : (IUnknown *)Coder2;
  }

  HRESULT QueryInterface(REFGUID iid, void** pp) const
  {
    return GetUnknown()->QueryInterface(iid, pp);
  }
};



class CMixer
{
  bool Is_PackSize_Correct_for_Stream(UInt32 streamIndex);

protected:
  CBindInfo _bi;

  int FindBond_for_Stream(bool forInputStream, UInt32 streamIndex) const
  {
    if (EncodeMode == forInputStream)
      return _bi.FindBond_for_UnpackStream(streamIndex);
    else
      return _bi.FindBond_for_PackStream(streamIndex);
  }

  CBoolVector IsFilter_Vector;
  CBoolVector IsExternal_Vector;
  bool EncodeMode;
public:
  unsigned MainCoderIndex;

  // bool InternalPackSizeError;

  CMixer(bool encodeMode):
      EncodeMode(encodeMode),
      MainCoderIndex(0)
      // , InternalPackSizeError(false)
      {}

  virtual ~CMixer() {}
  /*
  Sequence of calling:

      SetBindInfo();
      for each coder
        AddCoder();
      SelectMainCoder();
 
      for each file
      {
        ReInit()
        for each coder
          SetCoderInfo();
        Code();
      }
  */

  virtual HRESULT SetBindInfo(const CBindInfo &bindInfo)
  {
    _bi = bindInfo;
    IsFilter_Vector.Clear();
    MainCoderIndex = 0;
    return S_OK;
  }

  virtual void AddCoder(const CCreatedCoder &cod) = 0;
  virtual CCoder &GetCoder(unsigned index) = 0;
  virtual void SelectMainCoder(bool useFirst) = 0;
  virtual HRESULT ReInit2() = 0;
  virtual void SetCoderInfo(unsigned coderIndex, const UInt64 *unpackSize, const UInt64 * const *packSizes, bool finish) = 0;
  virtual HRESULT Code(
      ISequentialInStream * const *inStreams,
      ISequentialOutStream * const *outStreams,
      ICompressProgressInfo *progress,
      bool &dataAfterEnd_Error) = 0;
  virtual UInt64 GetBondStreamSize(unsigned bondIndex) const = 0;

  bool Is_UnpackSize_Correct_for_Coder(UInt32 coderIndex);
  bool Is_PackSize_Correct_for_Coder(UInt32 coderIndex);
  bool IsThere_ExternalCoder_in_PackTree(UInt32 coderIndex);
};




#ifdef USE_MIXER_ST

struct CCoderST: public CCoder
{
  bool CanRead;
  bool CanWrite;
  
  CCoderST(): CanRead(false), CanWrite(false) {}
};


struct CStBinderStream
{
  CSequentialInStreamCalcSize *InStreamSpec;
  COutStreamCalcSize *OutStreamSpec;
  CMyComPtr<IUnknown> StreamRef;

  CStBinderStream(): InStreamSpec(NULL), OutStreamSpec(NULL) {}
};


class CMixerST:
  public IUnknown,
  public CMixer,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_0
  Z7_CLASS_NO_COPY(CMixerST)

  HRESULT GetInStream2(ISequentialInStream * const *inStreams, /* const UInt64 * const *inSizes, */
      UInt32 outStreamIndex, ISequentialInStream **inStreamRes);
  HRESULT GetInStream(ISequentialInStream * const *inStreams, /* const UInt64 * const *inSizes, */
      UInt32 inStreamIndex, ISequentialInStream **inStreamRes);
  HRESULT GetOutStream(ISequentialOutStream * const *outStreams, /* const UInt64 * const *outSizes, */
      UInt32 outStreamIndex, ISequentialOutStream **outStreamRes);

  HRESULT FinishStream(UInt32 streamIndex);
  HRESULT FinishCoder(UInt32 coderIndex);

public:
  CObjectVector<CCoderST> _coders;
  
  CObjectVector<CStBinderStream> _binderStreams;

  CMixerST(bool encodeMode);
  ~CMixerST() Z7_DESTRUCTOR_override;

  virtual void AddCoder(const CCreatedCoder &cod) Z7_override;
  virtual CCoder &GetCoder(unsigned index) Z7_override;
  virtual void SelectMainCoder(bool useFirst) Z7_override;
  virtual HRESULT ReInit2() Z7_override;
  virtual void SetCoderInfo(unsigned coderIndex, const UInt64 *unpackSize, const UInt64 * const *packSizes, bool finish) Z7_override
    { _coders[coderIndex].SetCoderInfo(unpackSize, packSizes, finish); }
  virtual HRESULT Code(
      ISequentialInStream * const *inStreams,
      ISequentialOutStream * const *outStreams,
      ICompressProgressInfo *progress,
      bool &dataAfterEnd_Error) Z7_override;
  virtual UInt64 GetBondStreamSize(unsigned bondIndex) const Z7_override;

  HRESULT GetMainUnpackStream(
      ISequentialInStream * const *inStreams,
      ISequentialInStream **inStreamRes);
};

#endif




#ifdef USE_MIXER_MT

class CCoderMT: public CCoder, public CVirtThread
{
  Z7_CLASS_NO_COPY(CCoderMT)
  CRecordVector<ISequentialInStream*> InStreamPointers;
  CRecordVector<ISequentialOutStream*> OutStreamPointers;

private:
  virtual void Execute() Z7_override;
public:
  bool EncodeMode;
  HRESULT Result;
  CObjectVector< CMyComPtr<ISequentialInStream> > InStreams;
  CObjectVector< CMyComPtr<ISequentialOutStream> > OutStreams;

  void Release()
  {
    InStreamPointers.Clear();
    OutStreamPointers.Clear();
    unsigned i;
    for (i = 0; i < InStreams.Size(); i++)
      InStreams[i].Release();
    for (i = 0; i < OutStreams.Size(); i++)
      OutStreams[i].Release();
  }

  class CReleaser
  {
    Z7_CLASS_NO_COPY(CReleaser)
    CCoderMT &_c;
  public:
    CReleaser(CCoderMT &c): _c(c) {}
    ~CReleaser() { _c.Release(); }
  };

  CCoderMT(): EncodeMode(false) {}
  ~CCoderMT() Z7_DESTRUCTOR_override
  {
    /* WaitThreadFinish() will be called in ~CVirtThread().
       But we need WaitThreadFinish() call before CCoder destructor,
       and before destructors of this class members.
    */
    CVirtThread::WaitThreadFinish();
  }
  
  void Code(ICompressProgressInfo *progress);
};


class CMixerMT:
  public IUnknown,
  public CMixer,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_0
  Z7_CLASS_NO_COPY(CMixerMT)

  CObjectVector<CStreamBinder> _streamBinders;

  HRESULT Init(ISequentialInStream * const *inStreams, ISequentialOutStream * const *outStreams);
  HRESULT ReturnIfError(HRESULT code);

  // virtual ~CMixerMT() {}
public:
  CObjectVector<CCoderMT> _coders;

  virtual HRESULT SetBindInfo(const CBindInfo &bindInfo) Z7_override;
  virtual void AddCoder(const CCreatedCoder &cod) Z7_override;
  virtual CCoder &GetCoder(unsigned index) Z7_override;
  virtual void SelectMainCoder(bool useFirst) Z7_override;
  virtual HRESULT ReInit2() Z7_override;
  virtual void SetCoderInfo(unsigned coderIndex, const UInt64 *unpackSize, const UInt64 * const *packSizes, bool finish) Z7_override
    { _coders[coderIndex].SetCoderInfo(unpackSize, packSizes, finish); }
  virtual HRESULT Code(
      ISequentialInStream * const *inStreams,
      ISequentialOutStream * const *outStreams,
      ICompressProgressInfo *progress,
      bool &dataAfterEnd_Error) Z7_override;
  virtual UInt64 GetBondStreamSize(unsigned bondIndex) const Z7_override;

  CMixerMT(bool encodeMode): CMixer(encodeMode) {}
};

#endif

}

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

/* ---- CPP/7zip/IPassword.h ---- */
// IPassword.h

#ifndef ZIP7_INC_IPASSWORD_H
#define ZIP7_INC_IPASSWORD_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_PURE_INTERFACES_BEGIN

#define Z7_IFACE_CONSTR_PASSWORD(i, n) \
  Z7_DECL_IFACE_7ZIP(i, 5, n) \
  { Z7_IFACE_COM7_PURE(i) };

/*
How to use output parameter (BSTR *password):

in:  The caller is required to set BSTR value as NULL (no string).
     The callee (in 7-Zip code) ignores the input value stored in BSTR variable,

out: The callee rewrites BSTR variable (*password) with new allocated string pointer.
     The caller must free BSTR string with function SysFreeString();
*/

#define Z7_IFACEM_ICryptoGetTextPassword(x) \
  x(CryptoGetTextPassword(BSTR *password))
Z7_IFACE_CONSTR_PASSWORD(ICryptoGetTextPassword, 0x10)


/*
CryptoGetTextPassword2()
in:
  The caller is required to set BSTR value as NULL (no string).
  The caller is not required to set (*passwordIsDefined) value.

out:
  Return code: != S_OK : error code
  Return code:    S_OK : success
   
  if (*passwordIsDefined == 1), the variable (*password) contains password string
    
  if (*passwordIsDefined == 0), the password is not defined,
     but the callee still could set (*password) to some allocated string, for example, as empty string.
  
  The caller must free BSTR string with function SysFreeString()
*/

#define Z7_IFACEM_ICryptoGetTextPassword2(x) \
  x(CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password))
Z7_IFACE_CONSTR_PASSWORD(ICryptoGetTextPassword2, 0x11)

Z7_PURE_INTERFACES_END
#endif

/* ---- CPP/Common/MyException.h ---- */
// Common/Exception.h

#ifndef ZIP7_INC_COMMON_EXCEPTION_H
#define ZIP7_INC_COMMON_EXCEPTION_H

// amalgamation: header emitted in prologue

struct CSystemException
{
  HRESULT ErrorCode;
  CSystemException(HRESULT errorCode): ErrorCode(errorCode) {}
};

#endif

/* ---- CPP/7zip/Common/InBuffer.h ---- */
// InBuffer.h

#ifndef ZIP7_INC_IN_BUFFER_H
#define ZIP7_INC_IN_BUFFER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef Z7_NO_EXCEPTIONS
struct CInBufferException: public CSystemException
{
  CInBufferException(HRESULT errorCode): CSystemException(errorCode) {}
};
#endif

class CInBufferBase
{
protected:
  Byte *_buf;
  Byte *_bufLim;
  Byte *_bufBase;

  ISequentialInStream *_stream;
  UInt64 _processedSize;
  size_t _bufSize; // actually it's number of Bytes for next read. The buf can be larger
                   // only up to 32-bits values now are supported!
  bool _wasFinished;

  bool ReadBlock();
  bool ReadByte_FromNewBlock(Byte &b);
  Byte ReadByte_FromNewBlock();

public:
  #ifdef Z7_NO_EXCEPTIONS
  HRESULT ErrorCode;
  #endif
  UInt32 NumExtraBytes;

  CInBufferBase() throw();

  // the size of portion of data in real stream that was already read from this object
  // it doesn't include unused data in buffer
  // it doesn't include virtual Extra bytes after the end of real stream data
  UInt64 GetStreamSize() const { return _processedSize + (size_t)(_buf - _bufBase); }
  
  // the size of virtual data that was read from this object
  // it doesn't include unused data in buffers
  // it includes any virtual Extra bytes after the end of real data
  UInt64 GetProcessedSize() const { return _processedSize + NumExtraBytes + (size_t)(_buf - _bufBase); }

  bool WasFinished() const { return _wasFinished; }

  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  void ClearStreamPtr() { _stream = NULL; }
  
  void SetBuf(Byte *buf, size_t bufSize, size_t end, size_t pos)
  {
    _bufBase = buf;
    _bufSize = bufSize;
    _processedSize = 0;
    _buf = buf + pos;
    _bufLim = buf + end;
    _wasFinished = false;
    #ifdef Z7_NO_EXCEPTIONS
    ErrorCode = S_OK;
    #endif
    NumExtraBytes = 0;
  }

  void Init() throw();
  
  Z7_FORCE_INLINE
  bool ReadByte(Byte &b)
  {
    if (_buf >= _bufLim)
      return ReadByte_FromNewBlock(b);
    b = *_buf++;
    return true;
  }

  Z7_FORCE_INLINE
  bool ReadByte_FromBuf(Byte &b)
  {
    if (_buf >= _bufLim)
      return false;
    b = *_buf++;
    return true;
  }
  
  Z7_FORCE_INLINE
  Byte ReadByte()
  {
    if (_buf >= _bufLim)
      return ReadByte_FromNewBlock();
    return *_buf++;
  }
  
  size_t ReadBytesPart(Byte *buf, size_t size);
  size_t ReadBytes(Byte *buf, size_t size);
  const Byte *Lookahead(size_t &rem)
  {
    rem = (size_t)(_bufLim - _buf);
    if (!rem)
    {
      ReadBlock();
      rem = (size_t)(_bufLim - _buf);
    }
    return _buf;
  }
  size_t Skip(size_t size);
};

class CInBuffer: public CInBufferBase
{
public:
  ~CInBuffer() { Free(); }
  bool Create(size_t bufSize) throw(); // only up to 32-bits values now are supported!
  void Free() throw();
};

#endif

/* ---- CPP/7zip/Archive/7z/7zHeader.h ---- */
// 7z/7zHeader.h

#ifndef ZIP7_INC_7Z_HEADER_H
#define ZIP7_INC_7Z_HEADER_H

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

const unsigned kSignatureSize = 6;
extern Byte kSignature[kSignatureSize];

// #define Z7_7Z_VOL
// 7z-MultiVolume is not finished yet.
// It can work already, but I still do not like some
// things of that new multivolume format.
// So please keep it commented.

#ifdef Z7_7Z_VOL
extern Byte kFinishSignature[kSignatureSize];
#endif

struct CArchiveVersion
{
  Byte Major;
  Byte Minor;
};

const Byte kMajorVersion = 0;

struct CStartHeader
{
  UInt64 NextHeaderOffset;
  UInt64 NextHeaderSize;
  UInt32 NextHeaderCRC;
};

const UInt32 kStartHeaderSize = 20;

#ifdef Z7_7Z_VOL
struct CFinishHeader: public CStartHeader
{
  UInt64 ArchiveStartOffset;  // data offset from end if that struct
  UInt64 AdditionalStartBlockSize; // start  signature & start header size
};

const UInt32 kFinishHeaderSize = kStartHeaderSize + 16;
#endif

namespace NID
{
  enum EEnum
  {
    kEnd,

    kHeader,

    kArchiveProperties,
    
    kAdditionalStreamsInfo,
    kMainStreamsInfo,
    kFilesInfo,
    
    kPackInfo,
    kUnpackInfo,
    kSubStreamsInfo,

    kSize,
    kCRC,

    kFolder,

    kCodersUnpackSize,
    kNumUnpackStream,

    kEmptyStream,
    kEmptyFile,
    kAnti,

    kName,
    kCTime,
    kATime,
    kMTime,
    kWinAttrib,
    kComment,

    kEncodedHeader,

    kStartPos,
    kDummy

    // kNtSecure,
    // kParent,
    // kIsAux
  };
}


const UInt32 k_Copy = 0;
const UInt32 k_Delta = 3;
const UInt32 k_ARM64 = 0xa;
const UInt32 k_RISCV = 0xb;

const UInt32 k_LZMA2 = 0x21;

const UInt32 k_SWAP2 = 0x20302;
const UInt32 k_SWAP4 = 0x20304;

const UInt32 k_LZMA  = 0x30101;
const UInt32 k_PPMD  = 0x30401;

const UInt32 k_Deflate   = 0x40108;
const UInt32 k_Deflate64 = 0x40109;
const UInt32 k_BZip2     = 0x40202;

const UInt32 k_BCJ   = 0x3030103;
const UInt32 k_BCJ2  = 0x303011B;
const UInt32 k_PPC   = 0x3030205;
const UInt32 k_IA64  = 0x3030401;
const UInt32 k_ARM   = 0x3030501;
const UInt32 k_ARMT  = 0x3030701;
const UInt32 k_SPARC = 0x3030805;

const UInt32 k_AES   = 0x6F10701;

// const UInt32 k_ZSTD = 0x4015D; // winzip zstd
// 0x4F71101, 7z-zstd

inline bool IsFilterMethod(UInt64 m)
{
  if (m > (UInt32)0xFFFFFFFF)
    return false;
  switch ((UInt32)m)
  {
    case k_Delta:
    case k_ARM64:
    case k_RISCV:
    case k_BCJ:
    case k_BCJ2:
    case k_PPC:
    case k_IA64:
    case k_ARM:
    case k_ARMT:
    case k_SPARC:
    case k_SWAP2:
    case k_SWAP4:
      return true;
    default: break;
  }
  return false;
}

}}

#endif

/* ---- CPP/7zip/Archive/7z/7zItem.h ---- */
// 7zItem.h

#ifndef ZIP7_INC_7Z_ITEM_H
#define ZIP7_INC_7Z_ITEM_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

typedef UInt32 CNum;
const CNum kNumMax     = 0x7FFFFFFF;
const CNum kNumNoIndex = 0xFFFFFFFF;

struct CCoderInfo
{
  CMethodId MethodID;
  CByteBuffer Props;
  UInt32 NumStreams;
  
  bool IsSimpleCoder() const { return NumStreams == 1; }
};


struct CBond
{
  UInt32 PackIndex;
  UInt32 UnpackIndex;
};


struct CFolder
{
  Z7_CLASS_NO_COPY(CFolder)
public:
  CObjArray2<CCoderInfo> Coders;
  CObjArray2<CBond> Bonds;
  CObjArray2<UInt32> PackStreams;

  CFolder() {}

  bool IsDecodingSupported() const { return Coders.Size() <= 32; }

  int Find_in_PackStreams(UInt32 packStream) const
  {
    FOR_VECTOR(i, PackStreams)
      if (PackStreams[i] == packStream)
        return (int)i;
    return -1;
  }

  int FindBond_for_PackStream(UInt32 packStream) const
  {
    FOR_VECTOR(i, Bonds)
      if (Bonds[i].PackIndex == packStream)
        return (int)i;
    return -1;
  }
  
  /*
  int FindBond_for_UnpackStream(UInt32 unpackStream) const
  {
    FOR_VECTOR(i, Bonds)
      if (Bonds[i].UnpackIndex == unpackStream)
        return i;
    return -1;
  }

  int FindOutCoder() const
  {
    for (int i = (int)Coders.Size() - 1; i >= 0; i--)
      if (FindBond_for_UnpackStream(i) < 0)
        return i;
    return -1;
  }
  */

  bool IsEncrypted() const
  {
    FOR_VECTOR(i, Coders)
      if (Coders[i].MethodID == k_AES)
        return true;
    return false;
  }
};


struct CUInt32DefVector
{
  CBoolVector Defs;
  CRecordVector<UInt32> Vals;

  void ClearAndSetSize(unsigned newSize)
  {
    Defs.ClearAndSetSize(newSize);
    Vals.ClearAndSetSize(newSize);
  }

  void Clear()
  {
    Defs.Clear();
    Vals.Clear();
  }

  void ReserveDown()
  {
    Defs.ReserveDown();
    Vals.ReserveDown();
  }

  bool GetItem(unsigned index, UInt32 &value) const
  {
    if (index < Defs.Size() && Defs[index])
    {
      value = Vals[index];
      return true;
    }
    value = 0;
    return false;
  }

  bool ValidAndDefined(unsigned i) const { return i < Defs.Size() && Defs[i]; }

  bool CheckSize(unsigned size) const { return Defs.Size() == size || Defs.Size() == 0; }

  void SetItem(unsigned index, bool defined, UInt32 value);
  void if_NonEmpty_FillResidue_with_false(unsigned numItems)
  {
    if (Defs.Size() != 0 && Defs.Size() < numItems)
      SetItem(numItems - 1, false, 0);
  }
};


struct CUInt64DefVector
{
  CBoolVector Defs;
  CRecordVector<UInt64> Vals;
  
  void Clear()
  {
    Defs.Clear();
    Vals.Clear();
  }
  
  void ReserveDown()
  {
    Defs.ReserveDown();
    Vals.ReserveDown();
  }

  bool GetItem(unsigned index, UInt64 &value) const
  {
    if (index < Defs.Size() && Defs[index])
    {
      value = Vals[index];
      return true;
    }
    value = 0;
    return false;
  }
  
  bool CheckSize(unsigned size) const { return Defs.Size() == size || Defs.Size() == 0; }

  void SetItem(unsigned index, bool defined, UInt64 value);
};


struct CFileItem
{
  UInt64 Size;
  UInt32 Crc;
  /*
  int Parent;
  bool IsAltStream;
  */
  bool HasStream; // Test it !!! it means that there is
                  // stream in some folder. It can be empty stream
  bool IsDir;
  bool CrcDefined;

  /*
  void Clear()
  {
    HasStream = true;
    IsDir = false;
    CrcDefined = false;
  }

  CFileItem():
    // Parent(-1),
    // IsAltStream(false),
    HasStream(true),
    IsDir(false),
    CrcDefined(false),
      {}
  */
};

}}

#endif

/* ---- CPP/7zip/Archive/7z/7zIn.h ---- */
// 7zIn.h

#ifndef ZIP7_INC_7Z_IN_H
#define ZIP7_INC_7Z_IN_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
 
namespace NArchive {
namespace N7z {

/*
  We don't need to init isEncrypted and passwordIsDefined
  We must upgrade them only */

#ifdef Z7_NO_CRYPTO
#define Z7_7Z_DECODER_CRYPRO_VARS_DECL
#define Z7_7Z_DECODER_CRYPRO_VARS
#else
#define Z7_7Z_DECODER_CRYPRO_VARS_DECL , ICryptoGetTextPassword *getTextPassword, bool &isEncrypted, bool &passwordIsDefined, UString &password
#define Z7_7Z_DECODER_CRYPRO_VARS , getTextPassword, isEncrypted, passwordIsDefined, password
#endif

struct CParsedMethods
{
  Byte Lzma2Prop;
  UInt32 LzmaDic;
  CRecordVector<UInt64> IDs;

  CParsedMethods(): Lzma2Prop(0), LzmaDic(0) {}
};

struct CFolderEx: public CFolder
{
  unsigned UnpackCoder;
};

struct CFolders
{
  CNum NumPackStreams;
  CNum NumFolders;

  CObjArray<UInt64> PackPositions; // NumPackStreams + 1
  // CUInt32DefVector PackCRCs; // we don't use PackCRCs now

  CUInt32DefVector FolderCRCs;             // NumFolders
  CObjArray<CNum> NumUnpackStreamsVector;  // NumFolders

  CObjArray<UInt64> CoderUnpackSizes;      // including unpack sizes of bond coders
  CObjArray<CNum> FoToCoderUnpackSizes;    // NumFolders + 1
  CObjArray<CNum> FoStartPackStreamIndex;  // NumFolders + 1
  CObjArray<Byte> FoToMainUnpackSizeIndex; // NumFolders
  
  CObjArray<size_t> FoCodersDataOffset;    // NumFolders + 1
  CByteBuffer CodersData;

  CParsedMethods ParsedMethods;

  void ParseFolderInfo(unsigned folderIndex, CFolder &folder) const;
  void ParseFolderEx(unsigned folderIndex, CFolderEx &folder) const
  {
    ParseFolderInfo(folderIndex, folder);
    folder.UnpackCoder = FoToMainUnpackSizeIndex[folderIndex];
  }
  
  unsigned GetNumFolderUnpackSizes(unsigned folderIndex) const
  {
    return (unsigned)(FoToCoderUnpackSizes[folderIndex + 1] - FoToCoderUnpackSizes[folderIndex]);
  }

  UInt64 GetFolderUnpackSize(unsigned folderIndex) const
  {
    return CoderUnpackSizes[FoToCoderUnpackSizes[folderIndex] + FoToMainUnpackSizeIndex[folderIndex]];
  }

  UInt64 GetStreamPackSize(unsigned index) const
  {
    return PackPositions[index + 1] - PackPositions[index];
  }

  CFolders(): NumPackStreams(0), NumFolders(0) {}

  void Clear()
  {
    NumPackStreams = 0;
    PackPositions.Free();
    // PackCRCs.Clear();

    NumFolders = 0;
    FolderCRCs.Clear();
    NumUnpackStreamsVector.Free();
    CoderUnpackSizes.Free();
    FoToCoderUnpackSizes.Free();
    FoStartPackStreamIndex.Free();
    FoToMainUnpackSizeIndex.Free();
    FoCodersDataOffset.Free();
    CodersData.Free();
  }
};

struct CDatabase: public CFolders
{
  CRecordVector<CFileItem> Files;

  CUInt64DefVector CTime;
  CUInt64DefVector ATime;
  CUInt64DefVector MTime;
  CUInt64DefVector StartPos;
  CUInt32DefVector Attrib;
  CBoolVector IsAnti;
  /*
  CBoolVector IsAux;
  CByteBuffer SecureBuf;
  CRecordVector<UInt32> SecureIDs;
  */

  CByteBuffer NamesBuf;
  CObjArray<size_t> NameOffsets; // numFiles + 1, offsets of utf-16 symbols

  /*
  void ClearSecure()
  {
    SecureBuf.Free();
    SecureIDs.Clear();
  }
  */

  void Clear()
  {
    CFolders::Clear();
    // ClearSecure();

    NamesBuf.Free();
    NameOffsets.Free();
    
    Files.Clear();
    CTime.Clear();
    ATime.Clear();
    MTime.Clear();
    StartPos.Clear();
    Attrib.Clear();
    IsAnti.Clear();
    // IsAux.Clear();
  }

  bool IsSolid() const
  {
    for (CNum i = 0; i < NumFolders; i++)
      if (NumUnpackStreamsVector[i] > 1)
        return true;
    return false;
  }
  bool IsItemAnti(unsigned index) const { return (index < IsAnti.Size() && IsAnti[index]); }
  // bool IsItemAux(unsigned index) const { return (index < IsAux.Size() && IsAux[index]); }

  /*
  const void* GetName(unsigned index) const
  {
    if (!NameOffsets || !NamesBuf)
      return NULL;
    return (void *)((const Byte *)NamesBuf + NameOffsets[index] * 2);
  };
  */
  void GetPath(unsigned index, UString &path) const;
  HRESULT GetPath_Prop(unsigned index, PROPVARIANT *path) const throw();
};


struct CInArchiveInfo
{
  CArchiveVersion Version;
  UInt64 StartPosition;               // in stream
  UInt64 StartPositionAfterHeader;    // in stream
  UInt64 DataStartPosition;           // in stream
  UInt64 DataStartPosition2;          // in stream. it's for headers
  CRecordVector<UInt64> FileInfoPopIDs;
  
  void Clear()
  {
    StartPosition = 0;
    StartPositionAfterHeader = 0;
    DataStartPosition = 0;
    DataStartPosition2 = 0;
    FileInfoPopIDs.Clear();
  }
};


struct CDbEx: public CDatabase
{
  CInArchiveInfo ArcInfo;
  
  CObjArray<CNum> FolderStartFileIndex;
  CObjArray<CNum> FileIndexToFolderIndexMap;

  UInt64 HeadersSize;
  UInt64 PhySize;
  // UInt64 EndHeaderOffset; // relative to position after StartHeader (32 bytes)

  /*
  CRecordVector<size_t> SecureOffsets;
  bool IsTree;
  bool ThereAreAltStreams;
  */

  bool IsArc;
  bool PhySizeWasConfirmed;

  bool ThereIsHeaderError;
  bool UnexpectedEnd;
  // bool UnsupportedVersion;

  bool StartHeaderWasRecovered;
  bool UnsupportedFeatureWarning;
  bool UnsupportedFeatureError;

  /*
  void ClearSecureEx()
  {
    ClearSecure();
    SecureOffsets.Clear();
  }
  */

  void Clear()
  {
    IsArc = false;
    PhySizeWasConfirmed = false;

    ThereIsHeaderError = false;
    UnexpectedEnd = false;
    // UnsupportedVersion = false;

    StartHeaderWasRecovered = false;
    UnsupportedFeatureError = false;
    UnsupportedFeatureWarning = false;

    /*
    IsTree = false;
    ThereAreAltStreams = false;
    */

    CDatabase::Clear();
    
    // SecureOffsets.Clear();
    ArcInfo.Clear();
    FolderStartFileIndex.Free();
    FileIndexToFolderIndexMap.Free();

    HeadersSize = 0;
    PhySize = 0;
    // EndHeaderOffset = 0;
  }

  bool CanUpdate() const
  {
    if (ThereIsHeaderError
        || UnexpectedEnd
        || StartHeaderWasRecovered
        || UnsupportedFeatureError)
      return false;
    return true;
  }

  void FillLinks();
  
  UInt64 GetFolderStreamPos(size_t folderIndex, size_t indexInFolder) const
  {
    return ArcInfo.DataStartPosition + PackPositions.ConstData()
        [FoStartPackStreamIndex.ConstData()[folderIndex] + indexInFolder];
  }
  
  UInt64 GetFolderFullPackSize(size_t folderIndex) const
  {
    return
      PackPositions[FoStartPackStreamIndex.ConstData()[folderIndex + 1]] -
      PackPositions[FoStartPackStreamIndex.ConstData()[folderIndex]];
  }
  
  UInt64 GetFolderPackStreamSize(size_t folderIndex, size_t streamIndex) const
  {
    const size_t i = FoStartPackStreamIndex.ConstData()[folderIndex] + streamIndex;
    return PackPositions.ConstData()[i + 1] -
           PackPositions.ConstData()[i];
  }

  /*
  UInt64 GetFilePackSize(size_t fileIndex) const
  {
    const CNum folderIndex = FileIndexToFolderIndexMap[fileIndex];
    if (folderIndex != kNumNoIndex)
      if (FolderStartFileIndex[folderIndex] == fileIndex)
        return GetFolderFullPackSize(folderIndex);
    return 0;
  }
  */
};

const unsigned kNumBufLevelsMax = 4;

struct CInByte2
{
  const Byte *_buffer;
public:
  size_t _size;
  size_t _pos;
  
  size_t GetRem() const { return _size - _pos; }
  const Byte *GetPtr() const { return _buffer + _pos; }
  void Init(const Byte *buffer, size_t size)
  {
    _buffer = buffer;
    _size = size;
    _pos = 0;
  }
  Byte ReadByte();
  void ReadBytes(Byte *data, size_t size);
  void SkipDataNoCheck(UInt64 size) { _pos += (size_t)size; }
  void SkipData(UInt64 size);

  void SkipData();
  void SkipRem() { _pos = _size; }
  UInt64 ReadNumber();
  CNum ReadNum();
  UInt32 ReadUInt32();
  UInt64 ReadUInt64();

  void ParseFolder(CFolder &folder);
};

class CStreamSwitch;

const UInt32 kHeaderSize = 32;

class CInArchive
{
  friend class CStreamSwitch;

  CMyComPtr<IInStream> _stream;

  unsigned _numInByteBufs;
  CInByte2 _inByteVector[kNumBufLevelsMax];

  CInByte2 *_inByteBack;
  bool ThereIsHeaderError;
 
  UInt64 _arhiveBeginStreamPosition;
  UInt64 _fileEndPosition;

  UInt64 _rangeLimit; // relative to position after StartHeader (32 bytes)

  Byte _header[kHeaderSize];

  UInt64 HeadersSize;

  bool _useMixerMT;

  void AddByteStream(const Byte *buffer, size_t size);
  
  void DeleteByteStream(bool needUpdatePos)
  {
    _numInByteBufs--;
    if (_numInByteBufs > 0)
    {
      _inByteBack = &_inByteVector[_numInByteBufs - 1];
      if (needUpdatePos)
        _inByteBack->_pos += _inByteVector[_numInByteBufs]._pos;
    }
  }

  HRESULT FindAndReadSignature(IInStream *stream, const UInt64 *searchHeaderSizeLimit);
  
  void ReadBytes(Byte *data, size_t size) { _inByteBack->ReadBytes(data, size); }
  Byte ReadByte() { return _inByteBack->ReadByte(); }
  UInt64 ReadNumber() { return _inByteBack->ReadNumber(); }
  CNum ReadNum() { return _inByteBack->ReadNum(); }
  UInt64 ReadID() { return _inByteBack->ReadNumber(); }
  UInt32 ReadUInt32() { return _inByteBack->ReadUInt32(); }
  UInt64 ReadUInt64() { return _inByteBack->ReadUInt64(); }
  void SkipData(UInt64 size) { _inByteBack->SkipData(size); }
  void SkipData() { _inByteBack->SkipData(); }
  void WaitId(UInt64 id);

  void Read_UInt32_Vector(CUInt32DefVector &v);

  void ReadArchiveProperties(CInArchiveInfo &archiveInfo);
  void ReadHashDigests(unsigned numItems, CUInt32DefVector &crcs);
  
  void ReadPackInfo(CFolders &f);
  
  void ReadUnpackInfo(
      const CObjectVector<CByteBuffer> *dataVector,
      CFolders &folders);
  
  void ReadSubStreamsInfo(
      CFolders &folders,
      CRecordVector<UInt64> &unpackSizes,
      CUInt32DefVector &digests);

  void ReadStreamsInfo(
      const CObjectVector<CByteBuffer> *dataVector,
      UInt64 &dataOffset,
      CFolders &folders,
      CRecordVector<UInt64> &unpackSizes,
      CUInt32DefVector &digests);

  void ReadBoolVector(unsigned numItems, CBoolVector &v);
  void ReadBoolVector2(unsigned numItems, CBoolVector &v);
  void ReadUInt64DefVector(const CObjectVector<CByteBuffer> &dataVector,
      CUInt64DefVector &v, unsigned numItems);
  HRESULT ReadAndDecodePackedStreams(
      DECL_EXTERNAL_CODECS_LOC_VARS
      UInt64 baseOffset, UInt64 &dataOffset,
      CObjectVector<CByteBuffer> &dataVector
      Z7_7Z_DECODER_CRYPRO_VARS_DECL
      );
  HRESULT ReadHeader(
      DECL_EXTERNAL_CODECS_LOC_VARS
      CDbEx &db
      Z7_7Z_DECODER_CRYPRO_VARS_DECL
      );
  HRESULT ReadDatabase2(
      DECL_EXTERNAL_CODECS_LOC_VARS
      CDbEx &db
      Z7_7Z_DECODER_CRYPRO_VARS_DECL
      );
public:
  CInArchive(bool useMixerMT):
      _numInByteBufs(0),
      _useMixerMT(useMixerMT)
      {}
  
  HRESULT Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit); // S_FALSE means is not archive
  void Close();

  HRESULT ReadDatabase(
      DECL_EXTERNAL_CODECS_LOC_VARS
      CDbEx &db
      Z7_7Z_DECODER_CRYPRO_VARS_DECL
      );
};
  
}}
  
#endif

/* ---- CPP/7zip/Archive/7z/7zDecode.h ---- */
// 7zDecode.h

#ifndef ZIP7_INC_7Z_DECODE_H
#define ZIP7_INC_7Z_DECODE_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

struct CBindInfoEx: public NCoderMixer2::CBindInfo
{
  CRecordVector<CMethodId> CoderMethodIDs;

  void Clear()
  {
    CBindInfo::Clear();
    CoderMethodIDs.Clear();
  }
};

class CDecoder
{
  bool _bindInfoPrev_Defined;
  #ifdef USE_MIXER_ST
  #ifdef USE_MIXER_MT
    bool _useMixerMT;
  #endif
  #endif
  CBindInfoEx _bindInfoPrev;
  
  #ifdef USE_MIXER_ST
    NCoderMixer2::CMixerST *_mixerST;
  #endif
  
  #ifdef USE_MIXER_MT
    NCoderMixer2::CMixerMT *_mixerMT;
  #endif
  
  NCoderMixer2::CMixer *_mixer;
  CMyComPtr<IUnknown> _mixerRef;

public:

  CDecoder(bool useMixerMT);
  
  HRESULT Decode(
      DECL_EXTERNAL_CODECS_LOC_VARS
      IInStream *inStream,
      UInt64 startPos,
      const CFolders &folders, unsigned folderIndex,
      const UInt64 *unpackSize // if (!unpackSize), then full folder is required
                               // if (unpackSize), then only *unpackSize bytes from folder are required

      , ISequentialOutStream *outStream
      , ICompressProgressInfo *compressProgress

      , ISequentialInStream **inStreamMainRes
      , bool &dataAfterEnd_Error
      
      Z7_7Z_DECODER_CRYPRO_VARS_DECL
      
      #if !defined(Z7_ST)
      , bool mtMode, UInt32 numThreads, UInt64 memUsage
      #endif
      );
};

}}

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

/* ---- CPP/7zip/Common/FilterCoder.h ---- */
// FilterCoder.h

#ifndef ZIP7_INC_FILTER_CODER_H
#define ZIP7_INC_FILTER_CODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef Z7_NO_CRYPTO
// amalgamation: header emitted in prologue
#endif

#define Z7_COM_QI_ENTRY_AG(i, sub0, sub) else if (iid == IID_ ## i) \
  { if (!sub) RINOK(sub0->QueryInterface(IID_ ## i, (void **)&sub)) \
    *outObject = (void *)(i *)this; }


struct CAlignedMidBuffer
{
  Byte *_buf;

  CAlignedMidBuffer(): _buf(NULL) {}
  ~CAlignedMidBuffer();
  void AllocAligned(size_t size);
};


class CFilterCoder Z7_final :
  public ICompressCoder,
  
  public ICompressSetOutStreamSize,
  public ICompressInitEncoder,
 
  public ICompressSetInStream,
  public ISequentialInStream,
  
  public ICompressSetOutStream,
  public ISequentialOutStream,
  public IOutStreamFinish,
  
  public ICompressSetBufSize,

  #ifndef Z7_NO_CRYPTO
  public ICryptoSetPassword,
  public ICryptoProperties,
  #endif
  
  #ifndef Z7_EXTRACT_ONLY
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public ICompressSetCoderPropertiesOpt,
  // public ICryptoResetSalt,
  public ICryptoResetInitVector,
  #endif
  
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp,
  public CAlignedMidBuffer
{
  UInt32 _bufSize;
  UInt32 _inBufSize;
  UInt32 _outBufSize;

  bool _encodeMode;
  bool _outSize_Defined;
  UInt64 _outSize;
  UInt64 _nowPos64;

  CMyComPtr<ISequentialInStream> _inStream;
  CMyComPtr<ISequentialOutStream> _outStream;
  UInt32 _bufPos;
  UInt32 _convPos;    // current pos in buffer for converted data
  UInt32 _convSize;   // size of converted data starting from _convPos
  
  void InitSpecVars()
  {
    _bufPos = 0;
    _convPos = 0;
    _convSize = 0;

    _outSize_Defined = false;
    _outSize = 0;
    _nowPos64 = 0;
  }

  HRESULT Alloc();
  HRESULT Init_and_Alloc();
  HRESULT Flush2();

  #ifndef Z7_NO_CRYPTO
  CMyComPtr<ICryptoSetPassword> _setPassword;
  CMyComPtr<ICryptoProperties> _cryptoProperties;
  #endif

  #ifndef Z7_EXTRACT_ONLY
  CMyComPtr<ICompressSetCoderProperties> _setCoderProperties;
  CMyComPtr<ICompressWriteCoderProperties> _writeCoderProperties;
  CMyComPtr<ICompressSetCoderPropertiesOpt> _setCoderPropertiesOpt;
  // CMyComPtr<ICryptoResetSalt> _cryptoResetSalt;
  CMyComPtr<ICryptoResetInitVector> _cryptoResetInitVector;
  #endif

  CMyComPtr<ICompressSetDecoderProperties2> _setDecoderProperties2;

public:
  CMyComPtr<ICompressFilter> Filter;

  CFilterCoder(bool encodeMode);

  struct C_InStream_Releaser
  {
    CFilterCoder *FilterCoder;
    C_InStream_Releaser(): FilterCoder(NULL) {}
    ~C_InStream_Releaser() { if (FilterCoder) FilterCoder->ReleaseInStream(); }
  };
  
  struct C_OutStream_Releaser
  {
    CFilterCoder *FilterCoder;
    C_OutStream_Releaser(): FilterCoder(NULL) {}
    ~C_OutStream_Releaser() { if (FilterCoder) FilterCoder->ReleaseOutStream(); }
  };

  struct C_Filter_Releaser
  {
    CFilterCoder *FilterCoder;
    C_Filter_Releaser(): FilterCoder(NULL) {}
    ~C_Filter_Releaser() { if (FilterCoder) FilterCoder->Filter.Release(); }
  };
  
private:
  Z7_COM_QI_BEGIN2(ICompressCoder)

    Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
    Z7_COM_QI_ENTRY(ICompressInitEncoder)
    
    Z7_COM_QI_ENTRY(ICompressSetInStream)
    Z7_COM_QI_ENTRY(ISequentialInStream)
    
    Z7_COM_QI_ENTRY(ICompressSetOutStream)
    Z7_COM_QI_ENTRY(ISequentialOutStream)
    Z7_COM_QI_ENTRY(IOutStreamFinish)
    
    Z7_COM_QI_ENTRY(ICompressSetBufSize)

    #ifndef Z7_NO_CRYPTO
    Z7_COM_QI_ENTRY_AG(ICryptoSetPassword, Filter, _setPassword)
    Z7_COM_QI_ENTRY_AG(ICryptoProperties, Filter, _cryptoProperties)
    #endif

    #ifndef Z7_EXTRACT_ONLY
    Z7_COM_QI_ENTRY_AG(ICompressSetCoderProperties, Filter, _setCoderProperties)
    Z7_COM_QI_ENTRY_AG(ICompressWriteCoderProperties, Filter, _writeCoderProperties)
    Z7_COM_QI_ENTRY_AG(ICompressSetCoderPropertiesOpt, Filter, _setCoderPropertiesOpt)
    // Z7_COM_QI_ENTRY_AG(ICryptoResetSalt, Filter, _cryptoResetSalt)
    Z7_COM_QI_ENTRY_AG(ICryptoResetInitVector, Filter, _cryptoResetInitVector)
    #endif

    Z7_COM_QI_ENTRY_AG(ICompressSetDecoderProperties2, Filter, _setDecoderProperties2)
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
public:
  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP(ICompressInitEncoder)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
private:
  Z7_IFACE_COM7_IMP(ISequentialInStream)
public:
  Z7_IFACE_COM7_IMP(ICompressSetOutStream)
private:
  Z7_IFACE_COM7_IMP(ISequentialOutStream)
public:
  Z7_IFACE_COM7_IMP(IOutStreamFinish)
private:
  
  Z7_IFACE_COM7_IMP(ICompressSetBufSize)

  #ifndef Z7_NO_CRYPTO
  Z7_IFACE_COM7_IMP(ICryptoSetPassword)
  Z7_IFACE_COM7_IMP(ICryptoProperties)
  #endif
  
  #ifndef Z7_EXTRACT_ONLY
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
  Z7_IFACE_COM7_IMP(ICompressWriteCoderProperties)
  Z7_IFACE_COM7_IMP(ICompressSetCoderPropertiesOpt)
  // Z7_IFACE_COM7_IMP(ICryptoResetSalt)
  Z7_IFACE_COM7_IMP(ICryptoResetInitVector)
  #endif
  
public:
  Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  
  HRESULT Init_NoSubFilterInit();
};

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

/* ---- CPP/Windows/FileIO.h ---- */
// Windows/FileIO.h

#ifndef ZIP7_INC_WINDOWS_FILE_IO_H
#define ZIP7_INC_WINDOWS_FILE_IO_H

// amalgamation: header emitted in prologue

#define Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT  (0xA0000003L)
#define Z7_WIN_IO_REPARSE_TAG_SYMLINK      (0xA000000CL)
#define Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK   (0xA000001DL)

#define Z7_WIN_SYMLINK_FLAG_RELATIVE 1

#define Z7_WIN_LX_SYMLINK_VERSION_2 2

#ifdef _WIN32

#if defined(_WIN32) && !defined(UNDER_CE)
#include <winioctl.h>
#endif

#else

#include <sys/types.h>
#include <sys/stat.h>

#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

HRESULT GetLastError_noZero_HRESULT();

#define my_FSCTL_SET_REPARSE_POINT     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER
#define my_FSCTL_GET_REPARSE_POINT     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)     // REPARSE_DATA_BUFFER
#define my_FSCTL_DELETE_REPARSE_POINT  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER

namespace NWindows {
namespace NFile {

#if defined(_WIN32) && !defined(UNDER_CE)
/*
  in:  (CByteBuffer &dest) is empty
  in:  (path) uses Windows path separator (\).
  out: (path) uses   Linux path separator (/).
       if (isAbsPath == true), then "c:\\" prefix is replaced to "/mnt/c/" prefix
*/
void Convert_WinPath_to_WslLinuxPath(FString &path, bool convertDrivePath);
// (path) must use Linux path separator (/).
void FillLinkData_WslLink(CByteBuffer &dest, const wchar_t *path);

/*
  in:  (CByteBuffer &dest) is empty
  if (isSymLink == false) : MOUNT_POINT : (path) must be absolute.
  if (isSymLink == true)  : SYMLINK : Windows
  (path) must use Windows path separator (\).
  (path) must be without link "\\??\\" prefix.
  link "\\??\\" prefix will be added inside FillLinkData(), if path is absolute.
*/
void FillLinkData_WinLink(CByteBuffer &dest, const wchar_t *path, bool isSymLink);
// in: (CByteBuffer &dest) is empty
inline void FillLinkData(CByteBuffer &dest, const wchar_t *path, bool isSymLink, bool isWSL)
{
  if (isWSL)
    FillLinkData_WslLink(dest, path);
  else
    FillLinkData_WinLink(dest, path, isSymLink);
}
#endif

struct CReparseShortInfo
{
  unsigned Offset;
  unsigned Size;

  bool Parse(const Byte *p, size_t size);
};

struct CReparseAttr
{
  UInt32 Tag;
  UInt32 Flags;
  UString SubsName;
  UString PrintName;
  AString WslName;

  bool HeaderError;
  bool TagIsUnknown;
  bool MinorError;
  DWORD ErrorCode;

  CReparseAttr(): Tag(0), Flags(0) {}

  // returns (true) and (ErrorCode = 0), if (it's correct known link)
  // returns (false) and (ErrorCode = ERROR_REPARSE_TAG_INVALID), if unknown tag
  bool Parse(const Byte *p, size_t size);

  bool IsMountPoint()  const { return Tag == Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT; } // it's Junction
  bool IsSymLink_Win() const { return Tag == Z7_WIN_IO_REPARSE_TAG_SYMLINK; }
  bool IsSymLink_WSL() const { return Tag == Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK; }

  // note: "/dir1/path" is marked as relative.
  bool IsRelative_Win() const { return Flags == Z7_WIN_SYMLINK_FLAG_RELATIVE; }

  bool IsRelative_WSL() const
  {
    return WslName[0] != '/'; // WSL uses unix path separator
  }

  bool IsOkNamePair() const;
  UString GetPath() const;
};

#ifdef _WIN32
#define CFiInfo BY_HANDLE_FILE_INFORMATION
#define ST_MTIME(st) (st).ftLastWriteTime
#else
#define CFiInfo stat
#endif

#ifdef _WIN32

namespace NIO {

bool GetReparseData(CFSTR path, CByteBuffer &reparseData, BY_HANDLE_FILE_INFORMATION *fileInfo = NULL);
bool SetReparseData(CFSTR path, bool isDir, const void *data, DWORD size);
bool DeleteReparseData(CFSTR path);

class CFileBase  MY_UNCOPYABLE
{
protected:
  HANDLE _handle;
  
  bool Create(CFSTR path, DWORD desiredAccess,
      DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);

public:

  bool DeviceIoControl(DWORD controlCode, LPVOID inBuffer, DWORD inSize,
      LPVOID outBuffer, DWORD outSize, LPDWORD bytesReturned, LPOVERLAPPED overlapped = NULL) const
  {
    return BOOLToBool(::DeviceIoControl(_handle, controlCode, inBuffer, inSize,
        outBuffer, outSize, bytesReturned, overlapped));
  }

  bool DeviceIoControlOut(DWORD controlCode, LPVOID outBuffer, DWORD outSize, LPDWORD bytesReturned) const
  {
    return DeviceIoControl(controlCode, NULL, 0, outBuffer, outSize, bytesReturned);
  }

  bool DeviceIoControlOut(DWORD controlCode, LPVOID outBuffer, DWORD outSize) const
  {
    DWORD bytesReturned;
    return DeviceIoControlOut(controlCode, outBuffer, outSize, &bytesReturned);
  }

public:
  bool PreserveATime;
#if 0
  bool IsStdStream;
  bool IsStdPipeStream;
#endif
#ifdef Z7_DEVICE_FILE
  bool IsDeviceFile;
  bool SizeDefined;
  UInt64 Size; // it can be larger than real available size
#endif

  CFileBase():
    _handle(INVALID_HANDLE_VALUE),
    PreserveATime(false)
#if 0
    , IsStdStream(false),
    , IsStdPipeStream(false)
#endif
    {}
  ~CFileBase() { Close(); }

  HANDLE GetHandle() const { return _handle; }

  // void Detach() { _handle = INVALID_HANDLE_VALUE; }

  bool Close() throw();

  bool GetPosition(UInt64 &position) const throw();
  bool GetLength(UInt64 &length) const throw();

  bool Seek(Int64 distanceToMove, DWORD moveMethod, UInt64 &newPosition) const throw();
  bool Seek(UInt64 position, UInt64 &newPosition) const throw();
  bool SeekToBegin() const throw();
  bool SeekToEnd(UInt64 &newPosition) const throw();
  
  bool GetFileInformation(BY_HANDLE_FILE_INFORMATION *info) const
    { return BOOLToBool(GetFileInformationByHandle(_handle, info)); }

  static bool GetFileInformation(CFSTR path, BY_HANDLE_FILE_INFORMATION *info)
  {
    // probably it can work for complex paths: unsupported by another things
    NIO::CFileBase file;
    if (!file.Create(path, 0, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS))
      return false;
    return file.GetFileInformation(info);
  }
};

#ifndef UNDER_CE
#define IOCTL_CDROM_BASE  FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY  CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
// #define IOCTL_CDROM_MEDIA_REMOVAL  CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)

// IOCTL_DISK_GET_DRIVE_GEOMETRY_EX works since WinXP
#define my_IOCTL_DISK_GET_DRIVE_GEOMETRY_EX  CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct my_DISK_GEOMETRY_EX
{
  DISK_GEOMETRY Geometry;
  LARGE_INTEGER DiskSize;
  BYTE Data[1];
};
#endif

class CInFile: public CFileBase
{
  #ifdef Z7_DEVICE_FILE

  #ifndef UNDER_CE
  
  bool GetGeometry(DISK_GEOMETRY *res) const
    { return DeviceIoControlOut(IOCTL_DISK_GET_DRIVE_GEOMETRY, res, sizeof(*res)); }

  bool GetGeometryEx(my_DISK_GEOMETRY_EX *res) const
    { return DeviceIoControlOut(my_IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, res, sizeof(*res)); }

  bool GetCdRomGeometry(DISK_GEOMETRY *res) const
    { return DeviceIoControlOut(IOCTL_CDROM_GET_DRIVE_GEOMETRY, res, sizeof(*res)); }
  
  bool GetPartitionInfo(PARTITION_INFORMATION *res)
    { return DeviceIoControlOut(IOCTL_DISK_GET_PARTITION_INFO, LPVOID(res), sizeof(*res)); }
  
  #endif

  void CorrectDeviceSize();
  void CalcDeviceSize(CFSTR name);
  
  #endif

public:
  bool Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
  bool OpenShared(CFSTR fileName, bool shareForWrite);
  bool Open(CFSTR fileName);

#if 0
  bool AttachStdIn()
  {
    IsDeviceFile = false;
    const HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || !h)
      return false;
    IsStdStream = true;
    IsStdPipeStream = true;
    _handle = h;
    return true;
  }
#endif

  #ifndef UNDER_CE

  bool Open_for_ReadAttributes(CFSTR fileName)
  {
    return Create(fileName, FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS);
    // we must use (FILE_FLAG_BACKUP_SEMANTICS) to open handle of directory.
  }

  bool Open_for_FileRenameInformation(CFSTR fileName)
  {
    return Create(fileName, DELETE | SYNCHRONIZE | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
    // we must use (FILE_FLAG_BACKUP_SEMANTICS) to open handle of directory.
  }

  bool OpenReparse(CFSTR fileName)
  {
    // 17.02 fix: to support Windows XP compatibility junctions:
    //   we use Create() with (desiredAccess = 0) instead of Open() with GENERIC_READ
    return
        Create(fileName, 0,
        // Open(fileName,
        FILE_SHARE_READ, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS);
  }
  
  #endif

  bool Read1(void *data, UInt32 size, UInt32 &processedSize) throw();
  bool ReadPart(void *data, UInt32 size, UInt32 &processedSize) throw();
  bool Read(void *data, UInt32 size, UInt32 &processedSize) throw();
  bool ReadFull(void *data, size_t size, size_t &processedSize) throw();
};

class COutFile: public CFileBase
{
  bool Open_Disposition(CFSTR fileName, DWORD creationDisposition);
public:
  bool Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open_EXISTING(CFSTR fileName)
    { return Open_Disposition(fileName, OPEN_EXISTING); }
  bool Create_ALWAYS_or_Open_ALWAYS(CFSTR fileName, bool createAlways)
    { return Open_Disposition(fileName, createAlways ? CREATE_ALWAYS : OPEN_ALWAYS); }
  bool Create_ALWAYS_or_NEW(CFSTR fileName, bool createAlways)
    { return Open_Disposition(fileName, createAlways ? CREATE_ALWAYS : CREATE_NEW); }
  bool Create_ALWAYS(CFSTR fileName)
    { return Open_Disposition(fileName, CREATE_ALWAYS); }
  bool Create_NEW(CFSTR fileName)
    { return Open_Disposition(fileName, CREATE_NEW); }
  
  bool Create_ALWAYS_with_Attribs(CFSTR fileName, DWORD flagsAndAttributes);

  bool SetTime(const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime) throw();
  bool SetMTime(const CFiTime *mTime) throw();
  bool WritePart(const void *data, UInt32 size, UInt32 &processedSize) throw();
  bool Write(const void *data, UInt32 size, UInt32 &processedSize) throw();
  bool WriteFull(const void *data, size_t size) throw();
  bool SetEndOfFile() throw();
  bool SetLength(UInt64 length) throw();
  bool SetLength_KeepPosition(UInt64 length) throw();
};

}


#else // _WIN32

namespace NIO {

bool GetReparseData(CFSTR path, CByteBuffer &reparseData);
// bool SetReparseData(CFSTR path, bool isDir, const void *data, DWORD size);

// parameters are in reverse order of symlink() function !!!
bool SetSymLink(CFSTR from, CFSTR to);
bool SetSymLink_UString(CFSTR from, const UString &to);


class CFileBase
{
protected:
  int _handle;

  /*
  bool IsDeviceFile;
  bool SizeDefined;
  UInt64 Size; // it can be larger than real available size
  */

  bool OpenBinary(const char *name, int flags, mode_t mode = 0666);
public:
  bool PreserveATime;
#if 0
  bool IsStdStream;
#endif

  CFileBase(): _handle(-1), PreserveATime(false)
#if 0
    , IsStdStream(false)
#endif
  {}
  ~CFileBase() { Close(); }
  // void Detach() { _handle = -1; }
  bool Close();
  bool GetLength(UInt64 &length) const;
  off_t seek(off_t distanceToMove, int moveMethod) const;
  off_t seekToBegin() const throw();
  off_t seekToCur() const throw();
  // bool SeekToBegin() throw();
  int my_fstat(struct stat *st) const  { return fstat(_handle, st); }
  /*
  int my_ioctl_BLKGETSIZE64(unsigned long long *val);
  int GetDeviceSize_InBytes(UInt64 &size);
  void CalcDeviceSize(CFSTR s);
  */
};

class CInFile: public CFileBase
{
public:
  bool Open(const char *name);
  bool OpenShared(const char *name, bool shareForWrite);
#if 0
  bool AttachStdIn()
  {
    _handle = GetStdHandle(STD_INPUT_HANDLE);
    if (_handle == INVALID_HANDLE_VALUE || !_handle)
      return false;
    IsStdStream = true;
  }
#endif
  ssize_t read_part(void *data, size_t size) throw();
  // ssize_t read_full(void *data, size_t size, size_t &processed);
  bool ReadFull(void *data, size_t size, size_t &processedSize) throw();
};

class COutFile: public CFileBase
{
  bool CTime_defined;
  bool ATime_defined;
  bool MTime_defined;
  CFiTime CTime;
  CFiTime ATime;
  CFiTime MTime;

  AString Path;
  ssize_t write_part(const void *data, size_t size) throw();
  bool OpenBinary_forWrite_oflag(const char *name, int oflag);
public:
  mode_t mode_for_Create;

  COutFile():
      CTime_defined(false),
      ATime_defined(false),
      MTime_defined(false),
      mode_for_Create(0666)
      {}

  bool Close();

  bool Open_EXISTING(CFSTR fileName);
  bool Create_ALWAYS_or_Open_ALWAYS(CFSTR fileName, bool createAlways);
  bool Create_ALWAYS(CFSTR fileName);
  bool Create_NEW(CFSTR fileName);
  // bool Create_ALWAYS_or_NEW(CFSTR fileName, bool createAlways);
  // bool Open_Disposition(const char *name, DWORD creationDisposition);

  ssize_t write_full(const void *data, size_t size, size_t &processed) throw();

  bool WriteFull(const void *data, size_t size) throw()
  {
    size_t processed;
    ssize_t res = write_full(data, size, processed);
    if (res == -1)
      return false;
    return processed == size;
  }

  bool SetLength(UInt64 length) throw();
  bool SetLength_KeepPosition(UInt64 length) throw()
  {
    return SetLength(length);
  }
  bool SetTime(const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime) throw();
  bool SetMTime(const CFiTime *mTime) throw();
};

}

#endif  // _WIN32

}}


#endif

/* ---- CPP/Windows/FileDir.h ---- */
// Windows/FileDir.h

#ifndef ZIP7_INC_WINDOWS_FILE_DIR_H
#define ZIP7_INC_WINDOWS_FILE_DIR_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NWindows {
namespace NFile {
namespace NDir {

bool GetWindowsDir(FString &path);
bool GetSystemDir(FString &path);

/*
WIN32 API : SetFileTime() doesn't allow to set zero timestamps in file
but linux : allows unix time = 0 in filesystem
*/
/*
SetDirTime() can be used to set time for file or for dir.
If path is symbolic link, SetDirTime() will follow symbolic link,
and it will set timestamps of symbolic link's target file or dir.
*/
bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime);

/*
SetLinkFileTime() doesn't follow symbolic link,
and it sets timestamps for symbolic link file itself.
If (path) is not symbolic link, it still can work (at least in some new OS versions).
*/
bool SetLinkFileTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime);


#ifdef _WIN32

bool SetFileAttrib(CFSTR path, DWORD attrib);

/*
  Some programs store posix attributes in high 16 bits of windows attributes field.
  Also some programs use additional flag markers: 0x8000 or 0x4000.
  SetFileAttrib_PosixHighDetect() tries to detect posix field, and it extracts only attribute
  bits that are related to current system only.
*/
#else

int my_chown(CFSTR path, uid_t owner, gid_t group);

#endif

bool SetFileAttrib_PosixHighDetect(CFSTR path, DWORD attrib);


#ifndef _WIN32
#define PROGRESS_CONTINUE   0
#define PROGRESS_CANCEL     1
// #define PROGRESS_STOP       2
// #define PROGRESS_QUIET      3
#endif
Z7_PURE_INTERFACES_BEGIN
DECLARE_INTERFACE(ICopyFileProgress)
{
  // in: total, current: include all/processed alt streams.
  // it returns PROGRESS_CONTINUE or PROGRESS_CANCEL.
  virtual DWORD CopyFileProgress(UInt64 total, UInt64 current) = 0;
};
Z7_PURE_INTERFACES_END

bool MyMoveFile(CFSTR existFileName, CFSTR newFileName);
// (progress == NULL) is allowed
bool MyMoveFile_with_Progress(CFSTR oldFile, CFSTR newFile,
    ICopyFileProgress *progress);


#ifndef UNDER_CE
bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName);
#endif

bool RemoveDir(CFSTR path);
bool CreateDir(CFSTR path);

/* CreateComplexDir returns true, if directory can contain files after the call (two cases):
    1) the directory already exists (network shares and drive paths are supported)
    2) the directory was created
  path can be WITH or WITHOUT trailing path separator. */

bool CreateComplexDir(CFSTR path);

bool DeleteFileAlways(CFSTR name);
bool RemoveDirWithSubItems(const FString &path);
#ifdef _WIN32
bool RemoveDirAlways_if_Empty(const FString &path);
#else
#define RemoveDirAlways_if_Empty RemoveDir
#endif

bool MyGetFullPathName(CFSTR path, FString &resFullPath);
bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName);
bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix);

#ifndef UNDER_CE

bool SetCurrentDir(CFSTR path);
bool GetCurrentDir(FString &resultPath);

#endif

bool MyGetTempPath(FString &resultPath);

bool CreateTempFile2(CFSTR prefix, bool addRandom, AString &postfix, NIO::COutFile *outFile);

class CTempFile  MY_UNCOPYABLE
{
  bool _mustBeDeleted;
  FString _path;
  void DisableDeleting() { _mustBeDeleted = false; }
public:
  CTempFile(): _mustBeDeleted(false) {}
  ~CTempFile() { Remove(); }
  const FString &GetPath() const { return _path; }
  bool Create(CFSTR pathPrefix, NIO::COutFile *outFile); // pathPrefix is not folder prefix
  bool CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile *outFile);
  bool Remove();
  // bool MoveTo(CFSTR name, bool deleteDestBefore);
  bool MoveTo(CFSTR name, bool deleteDestBefore,
      ICopyFileProgress *progress);
};


#ifdef _WIN32
class CTempDir  MY_UNCOPYABLE
{
  bool _mustBeDeleted;
  FString _path;
public:
  CTempDir(): _mustBeDeleted(false) {}
  ~CTempDir() { Remove();  }
  const FString &GetPath() const { return _path; }
  void DisableDeleting() { _mustBeDeleted = false; }
  bool Create(CFSTR namePrefix) ;
  bool Remove();
};
#endif


#if !defined(UNDER_CE)
class CCurrentDirRestorer  MY_UNCOPYABLE
{
  FString _path;
public:
  bool NeedRestore;

  CCurrentDirRestorer(): NeedRestore(true)
  {
    GetCurrentDir(_path);
  }
  ~CCurrentDirRestorer()
  {
    if (!NeedRestore)
      return;
    FString s;
    if (GetCurrentDir(s))
      if (s != _path)
        SetCurrentDir(_path);
  }
};
#endif

}}}

#endif

/* ---- CPP/7zip/Common/InOutTempBuffer.h ---- */
// InOutTempBuffer.h

#ifndef ZIP7_INC_IN_OUT_TEMP_BUFFER_H
#define ZIP7_INC_IN_OUT_TEMP_BUFFER_H

// #ifdef _WIN32
#define USE_InOutTempBuffer_FILE
// #endif

#ifdef USE_InOutTempBuffer_FILE
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

class CInOutTempBuffer
{
  UInt64 _size;
  void **_bufs;
  size_t _numBufs;
  size_t _numFilled;

 #ifdef USE_InOutTempBuffer_FILE
  
  bool _tempFile_Created;
  bool _useMemOnly;
  UInt32 _crc;
  // COutFile object must be declared after CTempFile object for correct destructor order
  NWindows::NFile::NDir::CTempFile _tempFile;
  NWindows::NFile::NIO::COutFile _outFile;

 #endif

  void *GetBuf(size_t index);

  Z7_CLASS_NO_COPY(CInOutTempBuffer)
public:
  CInOutTempBuffer();
  ~CInOutTempBuffer();
  HRESULT Write_HRESULT(const void *data, UInt32 size);
  HRESULT WriteToStream(ISequentialOutStream *stream);
  UInt64 GetDataSize() const { return _size; }
};

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

/* ---- CPP/7zip/Archive/7z/7zCompressionMode.h ---- */
// 7zCompressionMode.h

#ifndef ZIP7_INC_7Z_COMPRESSION_MODE_H
#define ZIP7_INC_7Z_COMPRESSION_MODE_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

struct CMethodFull: public CMethodProps
{
  CMethodId Id;
  UInt32 NumStreams;
  int CodecIndex;
  UInt32 NumThreads;
  bool Set_NumThreads;

  CMethodFull(): CodecIndex(-1), NumThreads(1), Set_NumThreads(false) {}
  bool IsSimpleCoder() const { return NumStreams == 1; }
};

struct CBond2
{
  UInt32 OutCoder;
  UInt32 OutStream;
  UInt32 InCoder;
};

struct CCompressionMethodMode
{
  /*
    if (Bonds.Empty()), then default bonds must be created
    if (Filter_was_Inserted)
    {
      Methods[0] is filter method
      Bonds don't contain bonds for filter (these bonds must be created)
    }
  */

  CObjectVector<CMethodFull> Methods;
  CRecordVector<CBond2> Bonds;

  bool IsThereBond_to_Coder(unsigned coderIndex) const
  {
    FOR_VECTOR(i, Bonds)
      if (Bonds[i].InCoder == coderIndex)
        return true;
    return false;
  }

  bool DefaultMethod_was_Inserted;
  bool Filter_was_Inserted;
  bool PasswordIsDefined;
  bool MemoryUsageLimit_WasSet;

  #ifndef Z7_ST
  bool NumThreads_WasForced;
  bool MultiThreadMixer;
  UInt32 NumThreads;
  UInt32 NumThreadGroups;
  #endif

  UString Password; // _Wipe
  UInt64 MemoryUsageLimit;
 
  bool IsEmpty() const { return (Methods.IsEmpty() && !PasswordIsDefined); }
  CCompressionMethodMode():
        DefaultMethod_was_Inserted(false)
      , Filter_was_Inserted(false)
      , PasswordIsDefined(false)
      , MemoryUsageLimit_WasSet(false)
      #ifndef Z7_ST
      , NumThreads_WasForced(false)
      , MultiThreadMixer(true)
      , NumThreads(1)
      , NumThreadGroups(0)
      #endif
      , MemoryUsageLimit((UInt64)1 << 30)
  {}

#ifdef Z7_CPP_IS_SUPPORTED_default
  CCompressionMethodMode(const CCompressionMethodMode &) = default;
  CCompressionMethodMode& operator =(const CCompressionMethodMode &) = default;
#endif
  ~CCompressionMethodMode() { Password.Wipe_and_Empty(); }
};

}}

#endif

/* ---- CPP/7zip/Archive/7z/7zEncode.h ---- */
// 7zEncode.h

#ifndef ZIP7_INC_7Z_ENCODE_H
#define ZIP7_INC_7Z_ENCODE_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

Z7_CLASS_IMP_COM_1(
  CMtEncMultiProgress,
  ICompressProgressInfo
)
  CMyComPtr<ICompressProgressInfo> _progress;
  #ifndef Z7_ST
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  #endif

public:
  UInt64 OutSize;

  CMtEncMultiProgress(): OutSize(0) {}

  void Init(ICompressProgressInfo *progress);

  void AddOutSize(UInt64 addOutSize)
  {
    #ifndef Z7_ST
    NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
    #endif
    OutSize += addOutSize;
  }
};


class CEncoder Z7_final MY_UNCOPYABLE
{
  #ifdef USE_MIXER_ST
    NCoderMixer2::CMixerST *_mixerST;
  #endif
  #ifdef USE_MIXER_MT
    NCoderMixer2::CMixerMT *_mixerMT;
  #endif
  
  NCoderMixer2::CMixer *_mixer;
  CMyComPtr<IUnknown> _mixerRef;

  CCompressionMethodMode _options;
  NCoderMixer2::CBindInfo _bindInfo;
  CRecordVector<CMethodId> _decompressionMethods;

  CRecordVector<UInt32> SrcIn_to_DestOut;
  CRecordVector<UInt32> SrcOut_to_DestIn;
  // CRecordVector<UInt32> DestIn_to_SrcOut;
  CRecordVector<UInt32> DestOut_to_SrcIn;

  void InitBindConv();
  void SetFolder(CFolder &folder);

  HRESULT CreateMixerCoder(DECL_EXTERNAL_CODECS_LOC_VARS
      const UInt64 *inSizeForReduce);

  bool _constructed;
public:

  CEncoder(const CCompressionMethodMode &options);
  ~CEncoder();
  HRESULT EncoderConstr();
  HRESULT Encode1(
      DECL_EXTERNAL_CODECS_LOC_VARS
      ISequentialInStream *inStream,
      // const UInt64 *inStreamSize,
      const UInt64 *inSizeForReduce,
      UInt64 expectedDataSize,
      CFolder &folderItem,
      // CRecordVector<UInt64> &coderUnpackSizes,
      // UInt64 &unpackSize,
      ISequentialOutStream *outStream,
      CRecordVector<UInt64> &packSizes,
      ICompressProgressInfo *compressProgress);

  void Encode_Post(
      UInt64 unpackSize,
      CRecordVector<UInt64> &coderUnpackSizes);

};

}}

#endif

/* ---- CPP/7zip/Archive/7z/7zSpecStream.h ---- */
// 7zSpecStream.h

#ifndef ZIP7_INC_7Z_SPEC_STREAM_H
#define ZIP7_INC_7Z_SPEC_STREAM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

/*
#define Z7_COM_QI_ENTRY_AG_2(i, sub0, sub) else if (iid == IID_ ## i) \
  { if (!sub) RINOK(sub0->QueryInterface(IID_ ## i, (void **)&sub)) \
    { i *ti = this;  *outObject = ti; }  }

class CSequentialInStreamSizeCount2 Z7_final:
  public ISequentialInStream,
  public ICompressGetSubStreamSize,
  public ICompressInSubStreams,
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ISequentialInStream)
    Z7_COM_QI_ENTRY(ICompressGetSubStreamSize)
    Z7_COM_QI_ENTRY_AG_2(ISequentialInStream, _stream, _compressGetSubStreamSize)
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ISequentialInStream)
  Z7_IFACE_COM7_IMP(ICompressGetSubStreamSize)
  Z7_IFACE_COM7_IMP(ICompressInSubStreams)

  CMyComPtr<ISequentialInStream> _stream;
  CMyComPtr<ICompressGetSubStreamSize> _getSubStreamSize;
  CMyComPtr<ICompressInSubStreams> _compressGetSubStreamSize;
  UInt64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _size = 0;
    _getSubStreamSize.Release();
    _compressGetSubStreamSize.Release();
    _stream = stream;
    _stream.QueryInterface(IID_ICompressGetSubStreamSize, &_getSubStreamSize);
    _stream.QueryInterface(IID_ICompressInSubStreams, &_compressGetSubStreamSize);
  }
  UInt64 GetSize() const { return _size; }
};
*/

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

/* ---- CPP/7zip/Archive/7z/7zHandler.h ---- */
// 7z/Handler.h

#ifndef ZIP7_7Z_HANDLER_H
#define ZIP7_7Z_HANDLER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#ifndef Z7_7Z_SET_PROPERTIES

#ifdef Z7_EXTRACT_ONLY
  #if !defined(Z7_ST) && !defined(Z7_SFX)
    #define Z7_7Z_SET_PROPERTIES
  #endif
#else
  #define Z7_7Z_SET_PROPERTIES
#endif

#endif

// #ifdef Z7_7Z_SET_PROPERTIES
// amalgamation: header emitted in prologue
// #endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {


#ifndef Z7_EXTRACT_ONLY

class COutHandler: public CMultiMethodProps
{
  HRESULT SetSolidFromString(const UString &s);
  HRESULT SetSolidFromPROPVARIANT(const PROPVARIANT &value);
public:
  UInt64 _numSolidFiles;
  UInt64 _numSolidBytes;
  bool _numSolidBytesDefined;
  bool _solidExtension;
  bool _useTypeSorting;

  bool _compressHeaders;
  bool _encryptHeadersSpecified;
  bool _encryptHeaders;
  // bool _useParents; 9.26

  CHandlerTimeOptions TimeOptions;

  CBoolPair Write_Attrib;

  bool _useMultiThreadMixer;
  bool _removeSfxBlock;
  // bool _volumeMode;

  UInt32 _decoderCompatibilityVersion;
  CUIntVector _enabledFilters;
  CUIntVector _disabledFilters;
  
  void InitSolidFiles() { _numSolidFiles = (UInt64)(Int64)(-1); }
  void InitSolidSize()  { _numSolidBytes = (UInt64)(Int64)(-1); }
  void InitSolid()
  {
    InitSolidFiles();
    InitSolidSize();
    _solidExtension = false;
    _numSolidBytesDefined = false;
  }

  void InitProps7z();
  void InitProps();

  COutHandler() { InitProps7z(); }

  HRESULT SetProperty(const wchar_t *name, const PROPVARIANT &value);
};

#endif

class CHandler Z7_final:
  public IInArchive,
  public IArchiveGetRawProps,
  
  #ifdef Z7_7Z_SET_PROPERTIES
  public ISetProperties,
  #endif
  
  #ifndef Z7_EXTRACT_ONLY
  public IOutArchive,
  #endif
  
  Z7_PUBLIC_ISetCompressCodecsInfo_IFEC
  
  public CMyUnknownImp,

  #ifndef Z7_EXTRACT_ONLY
    public COutHandler
  #else
    public CCommonMethodProps
  #endif
{
  Z7_COM_QI_BEGIN2(IInArchive)
  Z7_COM_QI_ENTRY(IArchiveGetRawProps)
 #ifdef Z7_7Z_SET_PROPERTIES
  Z7_COM_QI_ENTRY(ISetProperties)
 #endif
 #ifndef Z7_EXTRACT_ONLY
  Z7_COM_QI_ENTRY(IOutArchive)
 #endif
  Z7_COM_QI_ENTRY_ISetCompressCodecsInfo_IFEC
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(IInArchive)
  Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
 #ifdef Z7_7Z_SET_PROPERTIES
  Z7_IFACE_COM7_IMP(ISetProperties)
 #endif
 #ifndef Z7_EXTRACT_ONLY
  Z7_IFACE_COM7_IMP(IOutArchive)
 #endif
  DECL_ISetCompressCodecsInfo

private:
  CMyComPtr<IInStream> _inStream;
  NArchive::N7z::CDbEx _db;
  
 #ifndef Z7_NO_CRYPTO
  bool _isEncrypted;
  bool _passwordIsDefined;
  UString _password; // _Wipe
 #endif

  #ifdef Z7_EXTRACT_ONLY
  
  #ifdef Z7_7Z_SET_PROPERTIES
  bool _useMultiThreadMixer;
  #endif

  UInt32 _crcSize;

  #else
  
  CRecordVector<CBond2> _bonds;

  HRESULT PropsMethod_To_FullMethod(CMethodFull &dest, const COneMethodInfo &m);
  HRESULT SetHeaderMethod(CCompressionMethodMode &headerMethod);
  HRESULT SetMainMethod(CCompressionMethodMode &method);

  #endif

  bool IsFolderEncrypted(CNum folderIndex) const;
  #ifndef Z7_SFX

  CRecordVector<UInt64> _fileInfoPopIDs;
  void FillPopIDs();
  void AddMethodName(AString &s, UInt64 id);
  HRESULT SetMethodToProp(CNum folderIndex, PROPVARIANT *prop) const;

  #endif

  DECL_EXTERNAL_CODECS_VARS

public:
  CHandler();
  ~CHandler()
  {
    Close();
  }
};

}}

#endif

/* ---- CPP/7zip/Archive/7z/7zFolderInStream.h ---- */
// 7zFolderInStream.h

#ifndef ZIP7_INC_7Z_FOLDER_IN_STREAM_H
#define ZIP7_INC_7Z_FOLDER_IN_STREAM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// #include "../Common/InStreamWithCRC.h"

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

Z7_CLASS_IMP_COM_2(
  CFolderInStream
  , ISequentialInStream
  , ICompressGetSubStreamSize
)
  /*
  Z7_COM7F_IMP(GetNextStream(UInt64 *streamIndex))
  Z7_IFACE_COM7_IMP(ICompressInSubStreams)
  */

  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _totalSize_for_Coder;
  UInt64 _pos;
  UInt32 _crc;
  bool _size_Defined;
  bool _times_Defined;
  UInt64 _size;
  FILETIME _mTime;
  FILETIME _cTime;
  FILETIME _aTime;
  UInt32 _attrib;

  unsigned _numFiles;
  const UInt32 *_indexes;

  CMyComPtr<IArchiveUpdateCallback> _updateCallback;

  void ClearFileInfo();
  HRESULT OpenStream();
  HRESULT AddFileInfo(bool isProcessed);
  // HRESULT CloseCrcStream();
public:
  bool Need_MTime;
  bool Need_CTime;
  bool Need_ATime;
  bool Need_Attrib;
  // bool Need_Crc;
  // bool Need_FolderCrc;
  // unsigned AlignLog;
  
  CRecordVector<bool> Processed;
  CRecordVector<UInt64> Sizes;
  CRecordVector<UInt32> CRCs;
  CRecordVector<UInt32> Attribs;
  CRecordVector<bool> TimesDefined;
  CRecordVector<UInt64> MTimes;
  CRecordVector<UInt64> CTimes;
  CRecordVector<UInt64> ATimes;
  // UInt32 FolderCrc;

  // UInt32 GetFolderCrc() const { return CRC_GET_DIGEST(FolderCrc); }
  // CSequentialInStreamWithCRC *_crcStream_Spec;
  // CMyComPtr<ISequentialInStream> _crcStream;
  // CMyComPtr<IArchiveUpdateCallbackArcProp> _reportArcProp;

  void Init(IArchiveUpdateCallback *updateCallback, const UInt32 *indexes, unsigned numFiles);

  bool WasFinished() const { return Processed.Size() == _numFiles; }

  UInt64 Get_TotalSize_for_Coder() const { return _totalSize_for_Coder; }
  /*
  UInt64 GetFullSize() const
  {
    UInt64 size = 0;
    FOR_VECTOR (i, Sizes)
      size += Sizes[i];
    return size;
  }
  */

  CFolderInStream():
      Need_MTime(false),
      Need_CTime(false),
      Need_ATime(false),
      Need_Attrib(false)
      // , Need_Crc(true)
      // , Need_FolderCrc(false)
      // , AlignLog(0)
      {}
};

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

/* ---- CPP/7zip/Archive/7z/7zProperties.h ---- */
// 7zProperties.h

#ifndef ZIP7_INC_7Z_PROPERTIES_H
#define ZIP7_INC_7Z_PROPERTIES_H

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

// #define Z7_7Z_SHOW_PACK_STREAMS_SIZES // for debug

#ifdef Z7_7Z_SHOW_PACK_STREAMS_SIZES
enum
{
  kpidPackedSize0 = kpidUserDefined,
  kpidPackedSize1,
  kpidPackedSize2,
  kpidPackedSize3,
  kpidPackedSize4
};
#endif

}}

#endif

/* ---- CPP/7zip/Archive/Common/ParseProperties.h ---- */
// ParseProperties.h

#ifndef ZIP7_INC_PARSE_PROPERTIES_H
#define ZIP7_INC_PARSE_PROPERTIES_H

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

/* ---- CPP/7zip/Common/OutBuffer.h ---- */
// OutBuffer.h

#ifndef ZIP7_INC_OUT_BUFFER_H
#define ZIP7_INC_OUT_BUFFER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef Z7_NO_EXCEPTIONS
struct COutBufferException: public CSystemException
{
  COutBufferException(HRESULT errorCode): CSystemException(errorCode) {}
};
#endif

class COutBuffer
{
protected:
  Byte *_buf;
  UInt32 _pos;
  UInt32 _limitPos;
  UInt32 _streamPos;
  UInt32 _bufSize;
  ISequentialOutStream *_stream;
  UInt64 _processedSize;
  Byte  *_buf2;
  bool _overDict;

  HRESULT FlushPart() throw();
public:
  #ifdef Z7_NO_EXCEPTIONS
  HRESULT ErrorCode;
  #endif

  COutBuffer(): _buf(NULL), _pos(0), _stream(NULL), _buf2(NULL) {}
  ~COutBuffer() { Free(); }
  
  bool Create(UInt32 bufSize) throw();
  void Free() throw();

  void SetMemStream(Byte *buf) { _buf2 = buf; }
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void Init() throw();
  HRESULT Flush() throw();
  void FlushWithCheck();

  Z7_FORCE_INLINE
  void WriteByte(Byte b)
  {
    UInt32 pos = _pos;
    _buf[pos] = b;
    pos++;
    _pos = pos;
    if (pos == _limitPos)
      FlushWithCheck();
  }
  
  void WriteBytes(const void *data, size_t size)
  {
    while (size)
    {
      UInt32 pos = _pos;
      size_t cur = (size_t)(_limitPos - pos);
      if (cur >= size)
        cur = size;
      size -= cur;
      Byte *dest = _buf + pos;
      pos += (UInt32)cur;
      _pos = pos;
#if 0
      memcpy(dest, data, cur);
      data = (const void *)((const Byte *)data + cur);
#else
      const Byte * const lim = (const Byte *)data + cur;
      do
      {
        *dest++ = *(const Byte *)data;
        data = (const void *)((const Byte *)data + 1);
      }
      while (data != lim);
#endif
      if (pos == _limitPos)
        FlushWithCheck();
    }
  }

  Byte *GetOutBuffer(size_t &avail)
  {
    const UInt32 pos = _pos;
    avail = (size_t)(_limitPos - pos);
    return _buf + pos;
  }

  void SkipWrittenBytes(size_t num)
  {
    const UInt32 pos = _pos;
    const UInt32 rem = _limitPos - pos;
    if (rem > num)
    {
      _pos = pos + (UInt32)num;
      return;
    }
    // (rem <= num)
    // the caller must not call it with (rem < num)
    // so (rem == num)
    _pos = _limitPos;
    FlushWithCheck();
  }
  /*
  void WriteBytesBig(const void *data, size_t size)
  {
    while (size)
    {
      UInt32 pos = _pos;
      UInt32 rem = _limitPos - pos;
      if (rem > size)
      {
        _pos = pos + size;
        memcpy(_buf + pos, data, size);
        return;
      }
      memcpy(_buf + pos, data, rem);
      _pos = pos + rem;
      FlushWithCheck();
    }
  }
  */

  UInt64 GetProcessedSize() const throw();
};

#endif

/* ---- CPP/7zip/Archive/7z/7zOut.h ---- */
// 7zOut.h

#ifndef ZIP7_INC_7Z_OUT_H
#define ZIP7_INC_7Z_OUT_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

const unsigned k_StartHeadersRewriteSize = 32;

class CWriteBufferLoc
{
  Byte *_data;
  Byte *_dataLim;
  Byte *_dataBase;
public:
  // CWriteBufferLoc(): _data(NULL), _dataLim(NULL), _dataBase(NULL) {}
  void Init(Byte *data, size_t size)
  {
    _data = data;
    _dataBase = data;
    _dataLim = data + size;
  }
  
  Byte *GetDest_and_Update(size_t size)
  {
    Byte *dest = _data;
    if (size > (size_t)(_dataLim - dest))
      throw 1;
    _data = dest + size;
    return dest;
  }
  void WriteBytes(const void *data, size_t size)
  {
    if (size == 0)
      return;
    Byte *dest = GetDest_and_Update(size);
    memcpy(dest, data, size);
  }
  void WriteByte(Byte b)
  {
    Byte *dest = _data;
    if (dest == _dataLim)
      throw 1;
    *dest++ = b;
    _data = dest;
  }
  size_t GetPos() const { return (size_t)(_data - _dataBase); }
};


struct CHeaderOptions
{
  bool CompressMainHeader;
  /*
  bool WriteCTime;
  bool WriteATime;
  bool WriteMTime;
  */

  CHeaderOptions():
      CompressMainHeader(true)
      /*
      , WriteCTime(false)
      , WriteATime(false)
      , WriteMTime(true)
      */
      {}
};


struct CFileItem2
{
  UInt64 CTime;
  UInt64 ATime;
  UInt64 MTime;
  UInt64 StartPos;
  UInt32 Attrib;

  bool CTimeDefined;
  bool ATimeDefined;
  bool MTimeDefined;
  bool StartPosDefined;
  bool AttribDefined;
  bool IsAnti;
  // bool IsAux;

  /*
  void Init()
  {
    CTimeDefined = false;
    ATimeDefined = false;
    MTimeDefined = false;
    StartPosDefined = false;
    AttribDefined = false;
    IsAnti = false;
    // IsAux = false;
  }
  */
};


struct COutFolders
{
  CUInt32DefVector FolderUnpackCRCs; // Now we use it for headers only.

  CRecordVector<CNum> NumUnpackStreamsVector;
  CRecordVector<UInt64> CoderUnpackSizes; // including unpack sizes of bond coders

  void OutFoldersClear()
  {
    FolderUnpackCRCs.Clear();
    NumUnpackStreamsVector.Clear();
    CoderUnpackSizes.Clear();
  }

  void OutFoldersReserveDown()
  {
    FolderUnpackCRCs.ReserveDown();
    NumUnpackStreamsVector.ReserveDown();
    CoderUnpackSizes.ReserveDown();
  }
};


struct CArchiveDatabaseOut: public COutFolders
{
  CRecordVector<UInt64> PackSizes;
  CUInt32DefVector PackCRCs;
  CObjectVector<CFolder> Folders;

  CRecordVector<CFileItem> Files;
  UStringVector Names;
  CUInt64DefVector CTime;
  CUInt64DefVector ATime;
  CUInt64DefVector MTime;
  CUInt64DefVector StartPos;
  CUInt32DefVector Attrib;
  CBoolVector IsAnti;

  /*
  CBoolVector IsAux;

  CByteBuffer SecureBuf;
  CRecordVector<UInt32> SecureSizes;
  CRecordVector<UInt32> SecureIDs;

  void ClearSecure()
  {
    SecureBuf.Free();
    SecureSizes.Clear();
    SecureIDs.Clear();
  }
  */

  void Clear()
  {
    OutFoldersClear();

    PackSizes.Clear();
    PackCRCs.Clear();
    Folders.Clear();
  
    Files.Clear();
    Names.Clear();
    CTime.Clear();
    ATime.Clear();
    MTime.Clear();
    StartPos.Clear();
    Attrib.Clear();
    IsAnti.Clear();

    /*
    IsAux.Clear();
    ClearSecure();
    */
  }

  void ReserveDown()
  {
    OutFoldersReserveDown();

    PackSizes.ReserveDown();
    PackCRCs.ReserveDown();
    Folders.ReserveDown();
    
    Files.ReserveDown();
    Names.ReserveDown();
    CTime.ReserveDown();
    ATime.ReserveDown();
    MTime.ReserveDown();
    StartPos.ReserveDown();
    Attrib.ReserveDown();
    IsAnti.ReserveDown();

    /*
    IsAux.ReserveDown();
    */
  }

  bool IsEmpty() const
  {
    return (
      PackSizes.IsEmpty() &&
      NumUnpackStreamsVector.IsEmpty() &&
      Folders.IsEmpty() &&
      Files.IsEmpty());
  }

  bool CheckNumFiles() const
  {
    unsigned size = Files.Size();
    return (
           CTime.CheckSize(size)
        && ATime.CheckSize(size)
        && MTime.CheckSize(size)
        && StartPos.CheckSize(size)
        && Attrib.CheckSize(size)
        && (size == IsAnti.Size() || IsAnti.Size() == 0));
  }

  bool IsItemAnti(unsigned index) const { return (index < IsAnti.Size() && IsAnti[index]); }
  // bool IsItemAux(unsigned index) const { return (index < IsAux.Size() && IsAux[index]); }

  void SetItem_Anti(unsigned index, bool isAnti)
  {
    while (index >= IsAnti.Size())
      IsAnti.Add(false);
    IsAnti[index] = isAnti;
  }
  /*
  void SetItem_Aux(unsigned index, bool isAux)
  {
    while (index >= IsAux.Size())
      IsAux.Add(false);
    IsAux[index] = isAux;
  }
  */

  void AddFile(const CFileItem &file, const CFileItem2 &file2, const UString &name);
};


class COutArchive
{
  HRESULT WriteDirect(const void *data, UInt32 size) { return WriteStream(SeqStream, data, size); }
  
  UInt64 GetPos() const;
  void WriteBytes(const void *data, size_t size);
  void WriteBytes(const CByteBuffer &data) { WriteBytes(data, data.Size()); }
  void WriteByte(Byte b);
  void WriteByte_ToStream(Byte b)
  {
    _outByte.WriteByte(b);
    // _crc = CRC_UPDATE_BYTE(_crc, b);
  }
  // void WriteUInt32(UInt32 value);
  // void WriteUInt64(UInt64 value);
  void WriteNumber(UInt64 value);
  void WriteID(UInt64 value) { WriteNumber(value); }

  void WriteFolder(const CFolder &folder);
  HRESULT WriteFileHeader(const CFileItem &itemInfo);
  void Write_BoolVector(const CBoolVector &boolVector);
  void Write_BoolVector_numDefined(const CBoolVector &boolVector, unsigned numDefined);
  void WritePropBoolVector(Byte id, const CBoolVector &boolVector);

  void WriteHashDigests(const CUInt32DefVector &digests);

  void WritePackInfo(
      UInt64 dataOffset,
      const CRecordVector<UInt64> &packSizes,
      const CUInt32DefVector &packCRCs);

  void WriteUnpackInfo(
      const CObjectVector<CFolder> &folders,
      const COutFolders &outFolders);

  void WriteSubStreamsInfo(
      const CObjectVector<CFolder> &folders,
      const COutFolders &outFolders,
      const CRecordVector<UInt64> &unpackSizes,
      const CUInt32DefVector &digests);

  void SkipToAligned(unsigned pos, unsigned alignShifts);
  void WriteAlignedBools(const CBoolVector &v, unsigned numDefined, Byte type, unsigned itemSizeShifts);
  void Write_UInt32DefVector_numDefined(const CUInt32DefVector &v, unsigned numDefined);
  void Write_UInt64DefVector_type(const CUInt64DefVector &v, Byte type);

  HRESULT EncodeStream(
      DECL_EXTERNAL_CODECS_LOC_VARS
      CEncoder &encoder, const CByteBuffer &data,
      CRecordVector<UInt64> &packSizes, CObjectVector<CFolder> &folders, COutFolders &outFolders);
  void WriteHeader(
      const CArchiveDatabaseOut &db,
      // const CHeaderOptions &headerOptions,
      UInt64 &headerOffset);
  
  bool _countMode;
  bool _writeToStream;
  bool _useAlign;
  #ifdef Z7_7Z_VOL
  bool _endMarker;
  #endif
  // UInt32 _crc;
  size_t _countSize;
  CWriteBufferLoc _outByte2;
  COutBuffer _outByte;
  UInt64 _signatureHeaderPos;
  CMyComPtr<IOutStream> Stream;

  #ifdef Z7_7Z_VOL
  HRESULT WriteFinishSignature();
  HRESULT WriteFinishHeader(const CFinishHeader &h);
  #endif
  HRESULT WriteStartHeader(const CStartHeader &h);

public:
  CMyComPtr<ISequentialOutStream> SeqStream;

  // COutArchive();
  HRESULT Create_and_WriteStartPrefix(ISequentialOutStream *stream /* , bool endMarker */);
  void Close();
  HRESULT WriteDatabase(
      DECL_EXTERNAL_CODECS_LOC_VARS
      const CArchiveDatabaseOut &db,
      const CCompressionMethodMode *options,
      const CHeaderOptions &headerOptions);

  #ifdef Z7_7Z_VOL
  static UInt32 GetVolHeadersSize(UInt64 dataSize, int nameLength = 0, bool props = false);
  static UInt64 GetVolPureSize(UInt64 volSize, int nameLength = 0, bool props = false);
  #endif
};

}}

#endif

/* ---- CPP/7zip/Archive/7z/7zUpdate.h ---- */
// 7zUpdate.h

#ifndef ZIP7_INC_7Z_UPDATE_H
#define ZIP7_INC_7Z_UPDATE_H

// amalgamation: header emitted in prologue

// #include "../../Common/UniqBlocks.h"

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

/*
struct CTreeFolder
{
  UString Name;
  int Parent;
  CIntVector SubFolders;
  int UpdateItemIndex;
  int SortIndex;
  int SortIndexEnd;

  CTreeFolder(): UpdateItemIndex(-1) {}
};
*/

struct CUpdateItem
{
  int IndexInArchive;
  unsigned IndexInClient;
  
  UInt64 CTime;
  UInt64 ATime;
  UInt64 MTime;

  UInt64 Size;
  UString Name;
  /*
  bool IsAltStream;
  int ParentFolderIndex;
  int TreeFolderIndex;
  */

  // that code is not used in 9.26
  // int ParentSortIndex;
  // int ParentSortIndexEnd;

  UInt32 Attrib;
  
  bool NewData;
  bool NewProps;

  bool IsAnti;
  bool IsDir;

  bool AttribDefined;
  bool CTimeDefined;
  bool ATimeDefined;
  bool MTimeDefined;

  // bool ATime_WasReadByAnalysis;

  // int SecureIndex; // 0 means (no_security)

  bool HasStream() const { return !IsDir && !IsAnti && Size != 0; }
  // bool HasStream() const { return !IsDir && !IsAnti /* && Size != 0 */; } // for test purposes

  CUpdateItem():
      // ParentSortIndex(-1),
      // IsAltStream(false),
      IsAnti(false),
      IsDir(false),
      AttribDefined(false),
      CTimeDefined(false),
      ATimeDefined(false),
      MTimeDefined(false)
      // , ATime_WasReadByAnalysis(false)
      // SecureIndex(0)
      {}
  void SetDirStatusFromAttrib() { IsDir = ((Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0); }

  // unsigned GetExtensionPos() const;
  // UString GetExtension() const;
};

struct CUpdateOptions
{
  const CCompressionMethodMode *Method;
  const CCompressionMethodMode *HeaderMethod;
  bool UseFilters; // use additional filters for some files
  bool MaxFilter;  // use BCJ2 filter instead of BCJ
  int AnalysisLevel;

  UInt64 NumSolidFiles;
  UInt64 NumSolidBytes;
  bool SolidExtension;
  
  bool UseTypeSorting;
  
  bool RemoveSfxBlock;
  bool MultiThreadMixer;

  bool Need_CTime;
  bool Need_ATime;
  bool Need_MTime;
  bool Need_Attrib;
  // bool Need_Crc;

  CHeaderOptions HeaderOptions;

  CUIntVector DisabledFilterIDs;

  void Add_DisabledFilter_for_id(UInt32 id,
      const CUIntVector &enabledFilters)
  {
    if (enabledFilters.FindInSorted(id) < 0)
      DisabledFilterIDs.AddToUniqueSorted(id);
  }

  void SetFilterSupporting_ver_enabled_disabled(
      UInt32 compatVer,
      const CUIntVector &enabledFilters,
      const CUIntVector &disabledFilters)
  {
    DisabledFilterIDs = disabledFilters;
    if (compatVer < 2300) Add_DisabledFilter_for_id(k_ARM64, enabledFilters);
    if (compatVer < 2402) Add_DisabledFilter_for_id(k_RISCV, enabledFilters);
  }

  CUpdateOptions():
      Method(NULL),
      HeaderMethod(NULL),
      UseFilters(false),
      MaxFilter(false),
      AnalysisLevel(-1),
      NumSolidFiles((UInt64)(Int64)(-1)),
      NumSolidBytes((UInt64)(Int64)(-1)),
      SolidExtension(false),
      UseTypeSorting(true),
      RemoveSfxBlock(false),
      MultiThreadMixer(true),
      Need_CTime(false),
      Need_ATime(false),
      Need_MTime(false),
      Need_Attrib(false)
      // , Need_Crc(true)
  {
    DisabledFilterIDs.Add(k_RISCV);
  }
};

HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    const CDbEx *db,
    CObjectVector<CUpdateItem> &updateItems,
    // const CObjectVector<CTreeFolder> &treeFolders, // treeFolders[0] is root
    // const CUniqBlocks &secureBlocks,
    ISequentialOutStream *seqOutStream,
    IArchiveUpdateCallback *updateCallback,
    const CUpdateOptions &options);
}}

#endif

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/7z/7zCompressionMode.cpp ================ */
// CompressionMethod.cpp

// amalgamation: header emitted in prologue

/* ================ unit: CPP/7zip/Archive/7z/7zDecode.cpp ================ */
// 7zDecode.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

Z7_CLASS_IMP_COM_1(
  CDecProgress
  , ICompressProgressInfo
)
  CMyComPtr<ICompressProgressInfo> _progress;
public:
  CDecProgress(ICompressProgressInfo *progress): _progress(progress) {}
};

Z7_COM7F_IMF(CDecProgress::SetRatioInfo(const UInt64 * /* inSize */, const UInt64 *outSize))
{
  return _progress->SetRatioInfo(NULL, outSize);
}

static void Convert_FolderInfo_to_BindInfo(const CFolderEx &folder, CBindInfoEx &bi)
{
  bi.Clear();
  
  bi.Bonds.ClearAndSetSize(folder.Bonds.Size());
  unsigned i;
  for (i = 0; i < folder.Bonds.Size(); i++)
  {
    NCoderMixer2::CBond &bond = bi.Bonds[i];
    const N7z::CBond &folderBond = folder.Bonds[i];
    bond.PackIndex = folderBond.PackIndex;
    bond.UnpackIndex = folderBond.UnpackIndex;
  }

  bi.Coders.ClearAndSetSize(folder.Coders.Size());
  bi.CoderMethodIDs.ClearAndSetSize(folder.Coders.Size());
  for (i = 0; i < folder.Coders.Size(); i++)
  {
    const CCoderInfo &coderInfo = folder.Coders[i];
    bi.Coders[i].NumStreams = coderInfo.NumStreams;
    bi.CoderMethodIDs[i] = coderInfo.MethodID;
  }
  
  /*
  if (!bi.SetUnpackCoder())
    throw 1112;
  */
  bi.UnpackCoder = folder.UnpackCoder;
  bi.PackStreams.ClearAndSetSize(folder.PackStreams.Size());
  for (i = 0; i < folder.PackStreams.Size(); i++)
    bi.PackStreams[i] = folder.PackStreams[i];
}

static inline bool AreCodersEqual(
    const NCoderMixer2::CCoderStreamsInfo &a1,
    const NCoderMixer2::CCoderStreamsInfo &a2)
{
  return (a1.NumStreams == a2.NumStreams);
}

static inline bool AreBondsEqual(
    const NCoderMixer2::CBond &a1,
    const NCoderMixer2::CBond &a2)
{
  return
    (a1.PackIndex == a2.PackIndex) &&
    (a1.UnpackIndex == a2.UnpackIndex);
}

static bool AreBindInfoExEqual(const CBindInfoEx &a1, const CBindInfoEx &a2)
{
  if (a1.Coders.Size() != a2.Coders.Size())
    return false;
  unsigned i;
  for (i = 0; i < a1.Coders.Size(); i++)
    if (!AreCodersEqual(a1.Coders[i], a2.Coders[i]))
      return false;
  
  if (a1.Bonds.Size() != a2.Bonds.Size())
    return false;
  for (i = 0; i < a1.Bonds.Size(); i++)
    if (!AreBondsEqual(a1.Bonds[i], a2.Bonds[i]))
      return false;
  
  for (i = 0; i < a1.CoderMethodIDs.Size(); i++)
    if (a1.CoderMethodIDs[i] != a2.CoderMethodIDs[i])
      return false;
  
  if (a1.PackStreams.Size() != a2.PackStreams.Size())
    return false;
  for (i = 0; i < a1.PackStreams.Size(); i++)
    if (a1.PackStreams[i] != a2.PackStreams[i])
      return false;

  /*
  if (a1.UnpackCoder != a2.UnpackCoder)
    return false;
  */
  return true;
}

CDecoder::CDecoder(bool useMixerMT):
    _bindInfoPrev_Defined(false)
{
  #if defined(USE_MIXER_ST) && defined(USE_MIXER_MT)
  _useMixerMT = useMixerMT;
  #else
  UNUSED_VAR(useMixerMT)
  #endif
}


Z7_CLASS_IMP_COM_0(
  CLockedInStream
)
public:
  CMyComPtr<IInStream> Stream;
  UInt64 Pos;

  #ifdef USE_MIXER_MT
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  #endif
};


#ifdef USE_MIXER_MT

Z7_CLASS_IMP_COM_1(
  CLockedSequentialInStreamMT
  , ISequentialInStream
)
  CLockedInStream *_glob;
  UInt64 _pos;
  CMyComPtr<IUnknown> _globRef;
public:
  void Init(CLockedInStream *lockedInStream, UInt64 startPos)
  {
    _globRef = lockedInStream;
    _glob = lockedInStream;
    _pos = startPos;
  }
};

Z7_COM7F_IMF(CLockedSequentialInStreamMT::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  NWindows::NSynchronization::CCriticalSectionLock lock(_glob->CriticalSection);

  if (_pos != _glob->Pos)
  {
    RINOK(InStream_SeekSet(_glob->Stream, _pos))
    _glob->Pos = _pos;
  }

  UInt32 realProcessedSize = 0;
  const HRESULT res = _glob->Stream->Read(data, size, &realProcessedSize);
  _pos += realProcessedSize;
  _glob->Pos = _pos;
  if (processedSize)
    *processedSize = realProcessedSize;
  return res;
}

#endif


#ifdef USE_MIXER_ST

Z7_CLASS_IMP_COM_1(
  CLockedSequentialInStreamST
  , ISequentialInStream
)
  CLockedInStream *_glob;
  UInt64 _pos;
  CMyComPtr<IUnknown> _globRef;
public:
  void Init(CLockedInStream *lockedInStream, UInt64 startPos)
  {
    _globRef = lockedInStream;
    _glob = lockedInStream;
    _pos = startPos;
  }
};

Z7_COM7F_IMF(CLockedSequentialInStreamST::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (_pos != _glob->Pos)
  {
    RINOK(InStream_SeekSet(_glob->Stream, _pos))
    _glob->Pos = _pos;
  }

  UInt32 realProcessedSize = 0;
  const HRESULT res = _glob->Stream->Read(data, size, &realProcessedSize);
  _pos += realProcessedSize;
  _glob->Pos = _pos;
  if (processedSize)
    *processedSize = realProcessedSize;
  return res;
}

#endif



HRESULT CDecoder::Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    UInt64 startPos,
    const CFolders &folders, unsigned folderIndex,
    const UInt64 *unpackSize

    , ISequentialOutStream *outStream
    , ICompressProgressInfo *compressProgress
    
    , ISequentialInStream **
        #ifdef USE_MIXER_ST
        inStreamMainRes
        #endif

    , bool &dataAfterEnd_Error
    
    Z7_7Z_DECODER_CRYPRO_VARS_DECL

    #if !defined(Z7_ST)
    , bool mtMode, UInt32 numThreads, UInt64 memUsage
    #endif
    )
{
  dataAfterEnd_Error = false;

  const UInt64 *packPositions = &folders.PackPositions[folders.FoStartPackStreamIndex[folderIndex]];
  CFolderEx folderInfo;
  folders.ParseFolderEx(folderIndex, folderInfo);

  if (!folderInfo.IsDecodingSupported())
    return E_NOTIMPL;

  CBindInfoEx bindInfo;
  Convert_FolderInfo_to_BindInfo(folderInfo, bindInfo);
  if (!bindInfo.CalcMapsAndCheck())
    return E_NOTIMPL;
  
  UInt64 folderUnpackSize = folders.GetFolderUnpackSize(folderIndex);
  bool fullUnpack = true;
  if (unpackSize)
  {
    if (*unpackSize > folderUnpackSize)
      return E_FAIL;
    fullUnpack = (*unpackSize == folderUnpackSize);
  }

  /*
  We don't need to init isEncrypted and passwordIsDefined
  We must upgrade them only
  
  #ifndef Z7_NO_CRYPTO
  isEncrypted = false;
  passwordIsDefined = false;
  #endif
  */
  
  if (!_bindInfoPrev_Defined || !AreBindInfoExEqual(bindInfo, _bindInfoPrev))
  {
    _bindInfoPrev_Defined = false;
    _mixerRef.Release();

    #ifdef USE_MIXER_MT
    #ifdef USE_MIXER_ST
    if (_useMixerMT)
    #endif
    {
      _mixerMT = new NCoderMixer2::CMixerMT(false);
      _mixerRef = _mixerMT;
      _mixer = _mixerMT;
    }
    #ifdef USE_MIXER_ST
    else
    #endif
    #endif
    {
      #ifdef USE_MIXER_ST
      _mixerST = new NCoderMixer2::CMixerST(false);
      _mixerRef = _mixerST;
      _mixer = _mixerST;
      #endif
    }
    
    RINOK(_mixer->SetBindInfo(bindInfo))
    
    FOR_VECTOR(i, folderInfo.Coders)
    {
      const CCoderInfo &coderInfo = folderInfo.Coders[i];

      #ifndef Z7_SFX
      // we don't support RAR codecs here
      if ((coderInfo.MethodID >> 8) == 0x403)
        return E_NOTIMPL;
      #endif
  
      CCreatedCoder cod;
      RINOK(CreateCoder_Id(
          EXTERNAL_CODECS_LOC_VARS
          coderInfo.MethodID, false, cod))
    
      if (coderInfo.IsSimpleCoder())
      {
        if (!cod.Coder)
          return E_NOTIMPL;
        // CMethodId m = coderInfo.MethodID;
        // isFilter = (IsFilterMethod(m) || m == k_AES);
      }
      else
      {
        if (!cod.Coder2 || cod.NumStreams != coderInfo.NumStreams)
          return E_NOTIMPL;
      }
      _mixer->AddCoder(cod);
      
      // now there is no codec that uses another external codec
      /*
      #ifdef Z7_EXTERNAL_CODECS
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      decoderUnknown.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        // we must use g_ExternalCodecs also
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(_externalCodecs->GetCodecs));
      }
      #endif
      */
    }
    
    _bindInfoPrev = bindInfo;
    _bindInfoPrev_Defined = true;
  }

  RINOK(_mixer->ReInit2())
  
  UInt32 packStreamIndex = 0;
  UInt32 unpackStreamIndexStart = folders.FoToCoderUnpackSizes[folderIndex];

  unsigned i;

  #if !defined(Z7_ST)
  bool mt_wasUsed = false;
  #endif

  for (i = 0; i < folderInfo.Coders.Size(); i++)
  {
    const CCoderInfo &coderInfo = folderInfo.Coders[i];
    IUnknown *decoder = _mixer->GetCoder(i).GetUnknown();

    // now there is no codec that uses another external codec
    /*
    #ifdef Z7_EXTERNAL_CODECS
    {
      Z7_DECL_CMyComPtr_QI_FROM(ISetCompressCodecsInfo,
          setCompressCodecsInfo, decoder)
      if (setCompressCodecsInfo)
      {
        // we must use g_ExternalCodecs also
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(_externalCodecs->GetCodecs))
      }
    }
    #endif
    */

    #if !defined(Z7_ST)
    if (!mt_wasUsed)
    {
      if (mtMode)
      {
        Z7_DECL_CMyComPtr_QI_FROM(ICompressSetCoderMt,
            setCoderMt, decoder)
        if (setCoderMt)
        {
          mt_wasUsed = true;
          RINOK(setCoderMt->SetNumberOfThreads(numThreads))
        }
      }
      // if (memUsage != 0)
      {
        Z7_DECL_CMyComPtr_QI_FROM(ICompressSetMemLimit,
            setMemLimit, decoder)
        if (setMemLimit)
        {
          mt_wasUsed = true;
          RINOK(setMemLimit->SetMemLimit(memUsage))
        }
      }
    }
    #endif

    {
      Z7_DECL_CMyComPtr_QI_FROM(
          ICompressSetDecoderProperties2,
          setDecoderProperties, decoder)
      const CByteBuffer &props = coderInfo.Props;
      const UInt32 size32 = (UInt32)props.Size();
      if (props.Size() != size32)
        return E_NOTIMPL;
      if (setDecoderProperties)
      {
        HRESULT res = setDecoderProperties->SetDecoderProperties2((const Byte *)props, size32);
        if (res == E_INVALIDARG)
          res = E_NOTIMPL;
        RINOK(res)
      }
      else if (size32 != 0)
      {
        // v23: we fail, if decoder doesn't support properties
        return E_NOTIMPL;
      }
    }

    #ifndef Z7_NO_CRYPTO
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          ICryptoSetPassword,
          cryptoSetPassword, decoder)
      if (cryptoSetPassword)
      {
        isEncrypted = true;
        if (!getTextPassword)
          return E_NOTIMPL;
        CMyComBSTR_Wipe passwordBSTR;
        RINOK(getTextPassword->CryptoGetTextPassword(&passwordBSTR))
        passwordIsDefined = true;
        password.Wipe_and_Empty();
        size_t len = 0;
        if (passwordBSTR)
        {
          password = passwordBSTR;
          len = password.Len();
        }
        CByteBuffer_Wipe buffer(len * 2);
        const LPCOLESTR psw = passwordBSTR;
        for (size_t k = 0; k < len; k++)
        {
          const wchar_t c = psw[k];
          ((Byte *)buffer)[k * 2] = (Byte)c;
          ((Byte *)buffer)[k * 2 + 1] = (Byte)(c >> 8);
        }
        RINOK(cryptoSetPassword->CryptoSetPassword((const Byte *)buffer, (UInt32)buffer.Size()))
      }
    }
    #endif

    bool finishMode = false;
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          ICompressSetFinishMode,
          setFinishMode, decoder)
      if (setFinishMode)
      {
        finishMode = fullUnpack;
        RINOK(setFinishMode->SetFinishMode(BoolToUInt(finishMode)))
      }
    }
    
    UInt32 numStreams = (UInt32)coderInfo.NumStreams;
    
    CObjArray<UInt64> packSizes(numStreams);
    CObjArray<const UInt64 *> packSizesPointers(numStreams);
       
    for (UInt32 j = 0; j < numStreams; j++, packStreamIndex++)
    {
      int bond = folderInfo.FindBond_for_PackStream(packStreamIndex);
      
      if (bond >= 0)
        packSizesPointers[j] = &folders.CoderUnpackSizes[unpackStreamIndexStart + folderInfo.Bonds[(unsigned)bond].UnpackIndex];
      else
      {
        int index = folderInfo.Find_in_PackStreams(packStreamIndex);
        if (index < 0)
          return E_NOTIMPL;
        packSizes[j] = packPositions[(unsigned)index + 1] - packPositions[(unsigned)index];
        packSizesPointers[j] = &packSizes[j];
      }
    }

    const UInt64 *unpackSizesPointer =
        (unpackSize && i == bindInfo.UnpackCoder) ?
            unpackSize :
            &folders.CoderUnpackSizes[unpackStreamIndexStart + i];
    
    _mixer->SetCoderInfo(i, unpackSizesPointer, packSizesPointers, finishMode);
  }

  if (outStream)
  {
    _mixer->SelectMainCoder(!fullUnpack);
  }

  CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;
  
  CMyComPtr2_Create<IUnknown, CLockedInStream> lockedInStream;

  #ifdef USE_MIXER_MT
  #ifdef USE_MIXER_ST
  bool needMtLock = _useMixerMT;
  #endif
  #endif

  if (folderInfo.PackStreams.Size() > 1)
  {
    // lockedInStream.Pos = (UInt64)(Int64)-1;
    // RINOK(InStream_GetPos(inStream, lockedInStream.Pos))
    RINOK(inStream->Seek((Int64)(startPos + packPositions[0]), STREAM_SEEK_SET, &lockedInStream->Pos))
    lockedInStream->Stream = inStream;

    #ifdef USE_MIXER_MT
    #ifdef USE_MIXER_ST
    /*
      For ST-mixer mode:
      If parallel input stream reading from pack streams is possible,
      we must use MT-lock for packed streams.
      Internal decoders in 7-Zip will not read pack streams in parallel in ST-mixer mode.
      So we force to needMtLock mode only if there is unknown (external) decoder.
    */
    if (!needMtLock && _mixer->IsThere_ExternalCoder_in_PackTree(_mixer->MainCoderIndex))
      needMtLock = true;
    #endif
    #endif
  }

  for (unsigned j = 0; j < folderInfo.PackStreams.Size(); j++)
  {
    CMyComPtr<ISequentialInStream> packStream;
    const UInt64 packPos = startPos + packPositions[j];

    if (folderInfo.PackStreams.Size() == 1)
    {
      RINOK(InStream_SeekSet(inStream, packPos))
      packStream = inStream;
    }
    else
    {
      #ifdef USE_MIXER_MT
      #ifdef USE_MIXER_ST
      if (needMtLock)
      #endif
      {
        CLockedSequentialInStreamMT *lockedStreamImpSpec = new CLockedSequentialInStreamMT;
        packStream = lockedStreamImpSpec;
        lockedStreamImpSpec->Init(lockedInStream.ClsPtr(), packPos);
      }
      #ifdef USE_MIXER_ST
      else
      #endif
      #endif
      {
        #ifdef USE_MIXER_ST
        CLockedSequentialInStreamST *lockedStreamImpSpec = new CLockedSequentialInStreamST;
        packStream = lockedStreamImpSpec;
        lockedStreamImpSpec->Init(lockedInStream.ClsPtr(), packPos);
        #endif
      }
    }

    CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
    inStreams.AddNew() = streamSpec;
    streamSpec->SetStream(packStream);
    streamSpec->Init(packPositions[j + 1] - packPositions[j]);
  }
  
  const unsigned num = inStreams.Size();
  CObjArray<ISequentialInStream *> inStreamPointers(num);
  for (i = 0; i < num; i++)
    inStreamPointers[i] = inStreams[i];

  if (outStream)
  {
    CMyComPtr<ICompressProgressInfo> progress2;
    if (compressProgress && !_mixer->Is_PackSize_Correct_for_Coder(_mixer->MainCoderIndex))
      progress2 = new CDecProgress(compressProgress);

    ISequentialOutStream *outStreamPointer = outStream;
    return _mixer->Code(inStreamPointers, &outStreamPointer,
        progress2 ? (ICompressProgressInfo *)progress2 : compressProgress,
        dataAfterEnd_Error);
  }
  
  #ifdef USE_MIXER_ST
    return _mixerST->GetMainUnpackStream(inStreamPointers, inStreamMainRes);
  #else
    return E_FAIL;
  #endif
}

}}

/* ================ unit: CPP/7zip/Archive/7z/7zEncode.cpp ================ */
// 7zEncode.cpp

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

namespace NArchive {
namespace N7z {

void CEncoder::InitBindConv()
{
  unsigned numIn = _bindInfo.Coders.Size();
  
  SrcIn_to_DestOut.ClearAndSetSize(numIn);
  DestOut_to_SrcIn.ClearAndSetSize(numIn);

  unsigned numOut = _bindInfo.GetNum_Bonds_and_PackStreams();
  SrcOut_to_DestIn.ClearAndSetSize(numOut);
  // _DestIn_to_SrcOut.ClearAndSetSize(numOut);

  UInt32 destIn = 0;
  UInt32 destOut = 0;

  for (unsigned i = _bindInfo.Coders.Size(); i != 0;)
  {
    i--;

    const NCoderMixer2::CCoderStreamsInfo &coder = _bindInfo.Coders[i];

    numIn--;
    numOut -= coder.NumStreams;
    
    SrcIn_to_DestOut[numIn] = destOut;
    DestOut_to_SrcIn[destOut] = numIn;

    destOut++;
  
    for (UInt32 j = 0; j < coder.NumStreams; j++, destIn++)
    {
      UInt32 index = numOut + j;
      SrcOut_to_DestIn[index] = destIn;
      // _DestIn_to_SrcOut[destIn] = index;
    }
  }
}

void CEncoder::SetFolder(CFolder &folder)
{
  folder.Bonds.SetSize(_bindInfo.Bonds.Size());
  
  unsigned i;

  for (i = 0; i < _bindInfo.Bonds.Size(); i++)
  {
    CBond &fb = folder.Bonds[i];
    const NCoderMixer2::CBond &mixerBond = _bindInfo.Bonds[_bindInfo.Bonds.Size() - 1 - i];
    fb.PackIndex = SrcOut_to_DestIn[mixerBond.PackIndex];
    fb.UnpackIndex = SrcIn_to_DestOut[mixerBond.UnpackIndex];
  }
  
  folder.Coders.SetSize(_bindInfo.Coders.Size());
  
  for (i = 0; i < _bindInfo.Coders.Size(); i++)
  {
    CCoderInfo &coderInfo = folder.Coders[i];
    const NCoderMixer2::CCoderStreamsInfo &coderStreamsInfo = _bindInfo.Coders[_bindInfo.Coders.Size() - 1 - i];
    
    coderInfo.NumStreams = coderStreamsInfo.NumStreams;
    coderInfo.MethodID = _decompressionMethods[i];
    // we don't free coderInfo.Props here. So coderInfo.Props can be non-empty.
  }
  
  folder.PackStreams.SetSize(_bindInfo.PackStreams.Size());
  
  for (i = 0; i < _bindInfo.PackStreams.Size(); i++)
    folder.PackStreams[i] = SrcOut_to_DestIn[_bindInfo.PackStreams[i]];
}



static HRESULT SetCoderProps2(const CProps &props, const UInt64 *dataSizeReduce, IUnknown *coder)
{
  Z7_DECL_CMyComPtr_QI_FROM(
      ICompressSetCoderProperties,
      setCoderProperties, coder)
  if (setCoderProperties)
    return props.SetCoderProps(setCoderProperties, dataSizeReduce);
  return props.AreThereNonOptionalProps() ? E_INVALIDARG : S_OK;
}



void CMtEncMultiProgress::Init(ICompressProgressInfo *progress)
{
  _progress = progress;
  OutSize = 0;
}

Z7_COM7F_IMF(CMtEncMultiProgress::SetRatioInfo(const UInt64 *inSize, const UInt64 * /* outSize */))
{
  UInt64 outSize2;
  {
    #ifndef Z7_ST
    NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
    #endif
    outSize2 = OutSize;
  }
  
  if (_progress)
    return _progress->SetRatioInfo(inSize, &outSize2);
   
  return S_OK;
}



HRESULT CEncoder::CreateMixerCoder(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const UInt64 *inSizeForReduce)
{
  #ifdef USE_MIXER_MT
  #ifdef USE_MIXER_ST
  if (_options.MultiThreadMixer)
  #endif
  {
    _mixerMT = new NCoderMixer2::CMixerMT(true);
    _mixerRef = _mixerMT;
    _mixer = _mixerMT;
  }
  #ifdef USE_MIXER_ST
  else
  #endif
  #endif
  {
    #ifdef USE_MIXER_ST
    _mixerST = new NCoderMixer2::CMixerST(true);
    _mixerRef = _mixerST;
    _mixer = _mixerST;
    #endif
  }

  RINOK(_mixer->SetBindInfo(_bindInfo))

  FOR_VECTOR (m, _options.Methods)
  {
    const CMethodFull &methodFull = _options.Methods[m];

    CCreatedCoder cod;
    
    if (methodFull.CodecIndex >= 0)
    {
      RINOK(CreateCoder_Index(
        EXTERNAL_CODECS_LOC_VARS
        (unsigned)methodFull.CodecIndex, true, cod))
    }
    else
    {
      RINOK(CreateCoder_Id(
        EXTERNAL_CODECS_LOC_VARS
        methodFull.Id, true, cod))
    }

    if (!cod.Coder && !cod.Coder2)
    {
      return E_NOTIMPL; // unsupported method, if encoder
      // return E_FAIL;
    }

    if (cod.NumStreams != methodFull.NumStreams)
      return E_FAIL;

    CMyComPtr<IUnknown> encoderCommon = cod.Coder ? (IUnknown *)cod.Coder : (IUnknown *)cod.Coder2;
   
    #ifndef Z7_ST
    if (methodFull.Set_NumThreads)
    {
      CMyComPtr<ICompressSetCoderMt> setCoderMt;
      encoderCommon.QueryInterface(IID_ICompressSetCoderMt, &setCoderMt);
      if (setCoderMt)
      {
        RINOK(setCoderMt->SetNumberOfThreads(
            /* _options.NumThreads */
            methodFull.NumThreads
            ))
      }
    }
    #endif
        
    RINOK(SetCoderProps2(methodFull, inSizeForReduce, encoderCommon))

    /*
    CMyComPtr<ICryptoResetSalt> resetSalt;
    encoderCommon.QueryInterface(IID_ICryptoResetSalt, (void **)&resetSalt);
    if (resetSalt)
    {
      resetSalt->ResetSalt();
    }
    */

    // now there is no codec that uses another external codec
    /*
    #ifdef Z7_EXTERNAL_CODECS
    CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
    encoderCommon.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
    if (setCompressCodecsInfo)
    {
      // we must use g_ExternalCodecs also
      RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(_externalCodecs->GetCodecs));
    }
    #endif
    */
    
    CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
    encoderCommon.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword);

    if (cryptoSetPassword)
    {
      const unsigned sizeInBytes = _options.Password.Len() * 2;
      CByteBuffer_Wipe buffer(sizeInBytes);
      for (unsigned i = 0; i < _options.Password.Len(); i++)
      {
        wchar_t c = _options.Password[i];
        ((Byte *)buffer)[i * 2] = (Byte)c;
        ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
      }
      RINOK(cryptoSetPassword->CryptoSetPassword((const Byte *)buffer, (UInt32)sizeInBytes))
    }

    _mixer->AddCoder(cod);
  }
  return S_OK;
}



Z7_CLASS_IMP_COM_1(
  CSequentialOutTempBufferImp2
  , ISequentialOutStream
)
public:
  CInOutTempBuffer TempBuffer;
  CMtEncMultiProgress *_mtProgressSpec;
  
  CSequentialOutTempBufferImp2(): _mtProgressSpec(NULL) {}
};

Z7_COM7F_IMF(CSequentialOutTempBufferImp2::Write(const void *data, UInt32 size, UInt32 *processed))
{
  COM_TRY_BEGIN
  if (processed)
    *processed = 0;
  RINOK(TempBuffer.Write_HRESULT(data, size))
  if (processed)
    *processed = size;
  if (_mtProgressSpec)
    _mtProgressSpec->AddOutSize(size);
  return S_OK;
  COM_TRY_END
}


Z7_CLASS_IMP_COM_1(
  CSequentialOutMtNotify
  , ISequentialOutStream
)
public:
  CMyComPtr<ISequentialOutStream> _stream;
  CMtEncMultiProgress *_mtProgressSpec;
  
  CSequentialOutMtNotify(): _mtProgressSpec(NULL) {}
};

Z7_COM7F_IMF(CSequentialOutMtNotify::Write(const void *data, UInt32 size, UInt32 *processed))
{
  UInt32 realProcessed = 0;
  HRESULT res = _stream->Write(data, size, &realProcessed);
  if (processed)
    *processed = realProcessed;
  if (_mtProgressSpec)
    _mtProgressSpec->AddOutSize(size);
  return res;
}


static HRESULT FillProps_from_Coder(IUnknown *coder, CByteBuffer &props)
{
  Z7_DECL_CMyComPtr_QI_FROM(
      ICompressWriteCoderProperties,
      writeCoderProperties, coder)
  if (writeCoderProperties)
  {
    CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> outStreamSpec;
    outStreamSpec->Init();
    RINOK(writeCoderProperties->WriteCoderProperties(outStreamSpec))
    outStreamSpec->CopyToBuffer(props);
  }
  else
    props.Free();
  return S_OK;
}

HRESULT CEncoder::Encode1(
    DECL_EXTERNAL_CODECS_LOC_VARS
    ISequentialInStream *inStream,
    // const UInt64 *inStreamSize,
    const UInt64 *inSizeForReduce,
    UInt64 expectedDataSize,
    CFolder &folderItem,
    // CRecordVector<UInt64> &coderUnpackSizes,
    // UInt64 &unpackSize,
    ISequentialOutStream *outStream,
    CRecordVector<UInt64> &packSizes,
    ICompressProgressInfo *compressProgress)
{
  RINOK(EncoderConstr())

  if (!_mixerRef)
  {
    RINOK(CreateMixerCoder(EXTERNAL_CODECS_LOC_VARS inSizeForReduce))
  }
  
  RINOK(_mixer->ReInit2())

  CMyComPtr2<ICompressProgressInfo, CMtEncMultiProgress> mtProgress;
  CMyComPtr2<ISequentialOutStream, CSequentialOutMtNotify> mtOutStreamNotify;

  CRecordVector<CSequentialOutTempBufferImp2 *> tempBufferSpecs;
  CObjectVector<CMyComPtr<ISequentialOutStream> > tempBuffers;
  
  unsigned i;

  for (i = 1; i < _bindInfo.PackStreams.Size(); i++)
  {
    CSequentialOutTempBufferImp2 *tempBufferSpec = new CSequentialOutTempBufferImp2();
    CMyComPtr<ISequentialOutStream> tempBuffer = tempBufferSpec;
    tempBufferSpecs.Add(tempBufferSpec);
    tempBuffers.Add(tempBuffer);
  }

  const unsigned numMethods = _bindInfo.Coders.Size();

  for (i = 0; i < numMethods; i++)
    _mixer->SetCoderInfo(i, NULL, NULL, false);


  /* inStreamSize can be used by BCJ2 to set optimal range of conversion.
     But current BCJ2 encoder uses also another way to check exact size of current file.
     So inStreamSize is not required. */

  /*
  if (inStreamSize)
    _mixer->SetCoderInfo(_bindInfo.UnpackCoder, inStreamSize, NULL);
  */

  
  /*
  CSequentialInStreamSizeCount2 *inStreamSizeCountSpec = new CSequentialInStreamSizeCount2;
  CMyComPtr<ISequentialInStream> inStreamSizeCount = inStreamSizeCountSpec;
  */

  CSequentialOutStreamSizeCount *outStreamSizeCountSpec = NULL;
  CMyComPtr<ISequentialOutStream> outStreamSizeCount;

  // inStreamSizeCountSpec->Init(inStream);

  // ISequentialInStream *inStreamPointer = inStreamSizeCount;
  ISequentialInStream *inStreamPointer = inStream;

  CRecordVector<ISequentialOutStream *> outStreamPointers;
  
  SetFolder(folderItem);

  for (i = 0; i < numMethods; i++)
  {
    IUnknown *coder = _mixer->GetCoder(i).GetUnknown();
    /*
    {
      CEncoder *sfEncoder = NULL;
      Z7_DECL_CMyComPtr_QI_FROM(
          IGetSfEncoderInternal,
          sf, coder)
      if (sf)
      {
        RINOK(sf->GetSfEncoder(&sfEncoder));
        if (!sfEncoder)
          return E_FAIL;

      }
    }
    */
    /*
    #ifdef Z7_EXTERNAL_CODECS
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          ISetCompressCodecsInfo,
          setCompressCodecsInfo, coder)
      if (setCompressCodecsInfo)
      {
        // we must use g_ExternalCodecs also
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(_externalCodecs->GetCodecs))
      }
    }
    #endif
    */
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          ICryptoResetInitVector,
          resetInitVector, coder)
      if (resetInitVector)
      {
        RINOK(resetInitVector->ResetInitVector())
      }
    }
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          ICompressSetCoderPropertiesOpt,
          optProps, coder)
      if (optProps)
      {
        const PROPID propID = NCoderPropID::kExpectedDataSize;
        NWindows::NCOM::CPropVariant prop = (UInt64)expectedDataSize;
        RINOK(optProps->SetCoderPropertiesOpt(&propID, &prop, 1))
      }
    }
    // we must write properties from coder after ResetInitVector()
    RINOK(FillProps_from_Coder(coder, folderItem.Coders[numMethods - 1 - i].Props))
  }

  _mixer->SelectMainCoder(false);
  const UInt32 mainCoder = _mixer->MainCoderIndex;

  bool useMtProgress = false;
  if (!_mixer->Is_PackSize_Correct_for_Coder(mainCoder))
  {
    #ifdef Z7_ST
    if (!_mixer->IsThere_ExternalCoder_in_PackTree(mainCoder))
    #endif
      useMtProgress = true;
  }

  if (useMtProgress)
  {
    mtProgress.SetFromCls(new CMtEncMultiProgress);
    mtProgress->Init(compressProgress);
    
    mtOutStreamNotify.SetFromCls(new CSequentialOutMtNotify);
    mtOutStreamNotify->_stream = outStream;
    mtOutStreamNotify->_mtProgressSpec = mtProgress.ClsPtr();
    
    FOR_VECTOR (t, tempBufferSpecs)
    {
      tempBufferSpecs[t]->_mtProgressSpec = mtProgress.ClsPtr();
    }
  }
  
  
  if (_bindInfo.PackStreams.Size() != 0)
  {
    outStreamSizeCountSpec = new CSequentialOutStreamSizeCount;
    outStreamSizeCount = outStreamSizeCountSpec;
    outStreamSizeCountSpec->SetStream(mtOutStreamNotify.IsDefined() ?
        mtOutStreamNotify.Interface() : outStream);
    outStreamSizeCountSpec->Init();
    outStreamPointers.Add(outStreamSizeCount);
  }

  for (i = 1; i < _bindInfo.PackStreams.Size(); i++)
    outStreamPointers.Add(tempBuffers[i - 1]);

  bool dataAfterEnd_Error;

  RINOK(_mixer->Code(
      &inStreamPointer,
      outStreamPointers.ConstData(),
      mtProgress.IsDefined() ? mtProgress.Interface() :
        compressProgress, dataAfterEnd_Error))
  
  if (_bindInfo.PackStreams.Size() != 0)
    packSizes.Add(outStreamSizeCountSpec->GetSize());
  
  for (i = 1; i < _bindInfo.PackStreams.Size(); i++)
  {
    CInOutTempBuffer &iotb = tempBufferSpecs[i - 1]->TempBuffer;
    RINOK(iotb.WriteToStream(outStream))
    packSizes.Add(iotb.GetDataSize());
  }

  /* Code() in some future codec can change properties.
     v23: so we fill properties again after Code() */
  for (i = 0; i < numMethods; i++)
  {
    IUnknown *coder = _mixer->GetCoder(i).GetUnknown();
    RINOK(FillProps_from_Coder(coder, folderItem.Coders[numMethods - 1 - i].Props))
  }

  return S_OK;
}


void CEncoder::Encode_Post(
      UInt64 unpackSize,
      CRecordVector<UInt64> &coderUnpackSizes)
{
  // unpackSize = 0;
  for (unsigned i = 0; i < _bindInfo.Coders.Size(); i++)
  {
    const int bond = _bindInfo.FindBond_for_UnpackStream(DestOut_to_SrcIn[i]);
    UInt64 streamSize;
    if (bond < 0)
    {
      // streamSize = inStreamSizeCountSpec->GetSize();
      // unpackSize = streamSize;
      streamSize = unpackSize;
    }
    else
      streamSize = _mixer->GetBondStreamSize((unsigned)bond);
    coderUnpackSizes.Add(streamSize);
  }
}


CEncoder::CEncoder(const CCompressionMethodMode &options):
    _constructed(false)
{
  if (options.IsEmpty())
    throw 1;

  _options = options;

  #ifdef USE_MIXER_ST
    _mixerST = NULL;
  #endif
  
  #ifdef USE_MIXER_MT
    _mixerMT = NULL;
  #endif

  _mixer = NULL;
}


HRESULT CEncoder::EncoderConstr()
{
  if (_constructed)
    return S_OK;
  if (_options.Methods.IsEmpty())
  {
    // it has only password method;
    if (!_options.PasswordIsDefined)
      throw 1;
    if (!_options.Bonds.IsEmpty())
      throw 1;

    CMethodFull method;
    method.Id = k_AES;
    method.NumStreams = 1;
    _options.Methods.Add(method);

    NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
    coderStreamsInfo.NumStreams = 1;
    _bindInfo.Coders.Add(coderStreamsInfo);
  
    _bindInfo.PackStreams.Add(0);
    _bindInfo.UnpackCoder = 0;
  }
  else
  {

  UInt32 numOutStreams = 0;
  unsigned i;
  
  for (i = 0; i < _options.Methods.Size(); i++)
  {
    const CMethodFull &methodFull = _options.Methods[i];
    NCoderMixer2::CCoderStreamsInfo cod;
    
    cod.NumStreams = methodFull.NumStreams;

    if (_options.Bonds.IsEmpty())
    {
      // if there are no bonds in options, we create bonds via first streams of coders
      if (i != _options.Methods.Size() - 1)
      {
        NCoderMixer2::CBond bond;
        bond.PackIndex = numOutStreams;
        bond.UnpackIndex = i + 1; // it's next coder
        _bindInfo.Bonds.Add(bond);
      }
      else if (cod.NumStreams != 0)
        _bindInfo.PackStreams.Insert(0, numOutStreams);
      
      for (UInt32 j = 1; j < cod.NumStreams; j++)
        _bindInfo.PackStreams.Add(numOutStreams + j);
    }
    
    numOutStreams += cod.NumStreams;

    _bindInfo.Coders.Add(cod);
  }

  if (!_options.Bonds.IsEmpty())
  {
    for (i = 0; i < _options.Bonds.Size(); i++)
    {
      NCoderMixer2::CBond mixerBond;
      const CBond2 &bond = _options.Bonds[i];
      if (bond.InCoder >= _bindInfo.Coders.Size()
          || bond.OutCoder >= _bindInfo.Coders.Size()
          || bond.OutStream >= _bindInfo.Coders[bond.OutCoder].NumStreams)
        return E_INVALIDARG;
      mixerBond.PackIndex = _bindInfo.GetStream_for_Coder(bond.OutCoder) + bond.OutStream;
      mixerBond.UnpackIndex = bond.InCoder;
      _bindInfo.Bonds.Add(mixerBond);
    }

    for (i = 0; i < numOutStreams; i++)
      if (_bindInfo.FindBond_for_PackStream(i) == -1)
        _bindInfo.PackStreams.Add(i);
  }

  if (!_bindInfo.SetUnpackCoder())
    return E_INVALIDARG;

  if (!_bindInfo.CalcMapsAndCheck())
    return E_INVALIDARG;

  if (_bindInfo.PackStreams.Size() != 1)
  {
    /* main_PackStream is pack stream of main path of coders tree.
       We find main_PackStream, and place to start of list of out streams.
       It allows to use more optimal memory usage for temp buffers,
       if main_PackStream is largest stream. */

    UInt32 ci = _bindInfo.UnpackCoder;
    
    for (;;)
    {
      if (_bindInfo.Coders[ci].NumStreams == 0)
        break;
      
      const UInt32 outIndex = _bindInfo.Coder_to_Stream[ci];
      const int bond = _bindInfo.FindBond_for_PackStream(outIndex);
      if (bond >= 0)
      {
        ci = _bindInfo.Bonds[(unsigned)bond].UnpackIndex;
        continue;
      }
      
      const int si = _bindInfo.FindStream_in_PackStreams(outIndex);
      if (si >= 0)
        _bindInfo.PackStreams.MoveToFront((unsigned)si);
      break;
    }
  }

  if (_options.PasswordIsDefined)
  {
    unsigned numCryptoStreams = _bindInfo.PackStreams.Size();

    unsigned numInStreams = _bindInfo.Coders.Size();
    
    for (i = 0; i < numCryptoStreams; i++)
    {
      NCoderMixer2::CBond bond;
      bond.UnpackIndex = numInStreams + i;
      bond.PackIndex = _bindInfo.PackStreams[i];
      _bindInfo.Bonds.Add(bond);
    }
    _bindInfo.PackStreams.Clear();

    /*
    if (numCryptoStreams == 0)
      numCryptoStreams = 1;
    */

    for (i = 0; i < numCryptoStreams; i++)
    {
      CMethodFull method;
      method.NumStreams = 1;
      method.Id = k_AES;
      _options.Methods.Add(method);

      NCoderMixer2::CCoderStreamsInfo cod;
      cod.NumStreams = 1;
      _bindInfo.Coders.Add(cod);

      _bindInfo.PackStreams.Add(numOutStreams++);
    }
  }

  }

  for (unsigned i = _options.Methods.Size(); i != 0;)
    _decompressionMethods.Add(_options.Methods[--i].Id);

  if (_bindInfo.Coders.Size() > 16)
    return E_INVALIDARG;
  if (_bindInfo.GetNum_Bonds_and_PackStreams() > 16)
    return E_INVALIDARG;

  if (!_bindInfo.CalcMapsAndCheck())
    return E_INVALIDARG;

  InitBindConv();
  _constructed = true;
  return S_OK;
}

CEncoder::~CEncoder() {}

}}

/* ================ unit: CPP/7zip/Archive/7z/7zExtract.cpp ================ */
// 7zExtract.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// EXTERN_g_ExternalCodecs

namespace NArchive {
namespace N7z {

Z7_CLASS_IMP_COM_1(
  CFolderOutStream
  , ISequentialOutStream
  /* , ICompressGetSubStreamSize */
)
  CMyComPtr<ISequentialOutStream> _stream;
public:
  bool TestMode;
  bool CheckCrc;
private:
  bool _fileIsOpen;
  bool _calcCrc;
  UInt32 _crc;
  UInt64 _rem;

  const UInt32 *_indexes;
  // unsigned _startIndex;
  unsigned _numFiles;
  unsigned _fileIndex;

  HRESULT OpenFile(bool isCorrupted = false);
  HRESULT CloseFile_and_SetResult(Int32 res);
  HRESULT CloseFile();
  HRESULT ProcessEmptyFiles();

public:
  const CDbEx *_db;
  CMyComPtr<IArchiveExtractCallback> ExtractCallback;

  bool ExtraWriteWasCut;

  CFolderOutStream():
      TestMode(false),
      CheckCrc(true)
      {}

  HRESULT Init(unsigned startIndex, const UInt32 *indexes, unsigned numFiles);
  HRESULT FlushCorrupted(Int32 callbackOperationResult);

  bool WasWritingFinished() const { return _numFiles == 0; }
};


HRESULT CFolderOutStream::Init(unsigned startIndex, const UInt32 *indexes, unsigned numFiles)
{
  // _startIndex = startIndex;
  _fileIndex = startIndex;
  _indexes = indexes;
  _numFiles = numFiles;
  
  _fileIsOpen = false;
  ExtraWriteWasCut = false;
  
  return ProcessEmptyFiles();
}

HRESULT CFolderOutStream::OpenFile(bool isCorrupted)
{
  const CFileItem &fi = _db->Files[_fileIndex];
  const UInt32 nextFileIndex = (_indexes ? *_indexes : _fileIndex);
  Int32 askMode = (_fileIndex == nextFileIndex) ? TestMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract :
      NExtract::NAskMode::kSkip;

  if (isCorrupted
      && askMode == NExtract::NAskMode::kExtract
      && !_db->IsItemAnti(_fileIndex)
      && !fi.IsDir)
    askMode = NExtract::NAskMode::kTest;
  
  CMyComPtr<ISequentialOutStream> realOutStream;
  RINOK(ExtractCallback->GetStream(_fileIndex, &realOutStream, askMode))
  
  _stream = realOutStream;
  _crc = CRC_INIT_VAL;
  _calcCrc = (CheckCrc && fi.CrcDefined && !fi.IsDir);

  _fileIsOpen = true;
  _rem = fi.Size;
  
  if (askMode == NExtract::NAskMode::kExtract
      && !realOutStream
      && !_db->IsItemAnti(_fileIndex)
      && !fi.IsDir)
    askMode = NExtract::NAskMode::kSkip;
  return ExtractCallback->PrepareOperation(askMode);
}

HRESULT CFolderOutStream::CloseFile_and_SetResult(Int32 res)
{
  _stream.Release();
  _fileIsOpen = false;
  
  if (!_indexes)
    _numFiles--;
  else if (*_indexes == _fileIndex)
  {
    _indexes++;
    _numFiles--;
  }

  _fileIndex++;
  return ExtractCallback->SetOperationResult(res);
}

HRESULT CFolderOutStream::CloseFile()
{
  const CFileItem &fi = _db->Files[_fileIndex];
  return CloseFile_and_SetResult((!_calcCrc || fi.Crc == CRC_GET_DIGEST(_crc)) ?
      NExtract::NOperationResult::kOK :
      NExtract::NOperationResult::kCRCError);
}

HRESULT CFolderOutStream::ProcessEmptyFiles()
{
  while (_numFiles != 0 && _db->Files[_fileIndex].Size == 0)
  {
    RINOK(OpenFile())
    RINOK(CloseFile())
  }
  return S_OK;
}

Z7_COM7F_IMF(CFolderOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  
  while (size != 0)
  {
    if (_fileIsOpen)
    {
      UInt32 cur = (size < _rem ? size : (UInt32)_rem);
      if (_calcCrc)
      {
        const UInt32 k_Step = (UInt32)1 << 20;
        if (cur > k_Step)
          cur = k_Step;
      }
      HRESULT result = S_OK;
      if (_stream)
        result = _stream->Write(data, cur, &cur);
      if (_calcCrc)
        _crc = CrcUpdate(_crc, data, cur);
      if (processedSize)
        *processedSize += cur;
      data = (const Byte *)data + cur;
      size -= cur;
      _rem -= cur;
      if (_rem == 0)
      {
        RINOK(CloseFile())
        RINOK(ProcessEmptyFiles())
      }
      RINOK(result)
      if (cur == 0)
        break;
      continue;
    }
  
    RINOK(ProcessEmptyFiles())
    if (_numFiles == 0)
    {
      // we support partial extracting
      /*
      if (processedSize)
        *processedSize += size;
      break;
      */
      ExtraWriteWasCut = true;
      // return S_FALSE;
      return k_My_HRESULT_WritingWasCut;
    }
    RINOK(OpenFile())
  }
  
  return S_OK;
}

HRESULT CFolderOutStream::FlushCorrupted(Int32 callbackOperationResult)
{
  while (_numFiles != 0)
  {
    if (_fileIsOpen)
    {
      RINOK(CloseFile_and_SetResult(callbackOperationResult))
    }
    else
    {
      RINOK(OpenFile(true))
    }
  }
  return S_OK;
}

/*
Z7_COM7F_IMF(CFolderOutStream::GetSubStreamSize(UInt64 subStream, UInt64 *value))
{
  *value = 0;
  // const unsigned numFiles_Original = _numFiles + _fileIndex - _startIndex;
  const unsigned numFiles_Original = _numFiles;
  if (subStream >= numFiles_Original)
    return S_FALSE; // E_FAIL;
  *value = _db->Files[_startIndex + (unsigned)subStream].Size;
  return S_OK;
}
*/


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallbackSpec))
{
  // for GCC
  // CFolderOutStream *folderOutStream = new CFolderOutStream;
  // CMyComPtr<ISequentialOutStream> outStream(folderOutStream);

  COM_TRY_BEGIN
  
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  
  UInt64 importantTotalUnpacked = 0;

  // numItems = (UInt32)(Int32)-1;

  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _db.Files.Size();

  if (numItems == 0)
    return S_OK;

  {
    CNum prevFolder = kNumNoIndex;
    UInt32 nextFile = 0;
    
    UInt32 i;
    
    for (i = 0; i < numItems; i++)
    {
      const UInt32 fileIndex = allFilesMode ? i : indices[i];
      const CNum folderIndex = _db.FileIndexToFolderIndexMap[fileIndex];
      if (folderIndex == kNumNoIndex)
        continue;
      if (folderIndex != prevFolder || fileIndex < nextFile)
        nextFile = _db.FolderStartFileIndex[folderIndex];
      for (CNum index = nextFile; index <= fileIndex; index++)
        importantTotalUnpacked += _db.Files[index].Size;
      nextFile = fileIndex + 1;
      prevFolder = folderIndex;
    }
  }

  RINOK(extractCallback->SetTotal(importantTotalUnpacked))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);

  CDecoder decoder(
    #if !defined(USE_MIXER_MT)
      false
    #elif !defined(USE_MIXER_ST)
      true
    #elif !defined(Z7_7Z_SET_PROPERTIES)
      #ifdef Z7_ST
        false
      #else
        true
      #endif
    #else
      _useMultiThreadMixer
    #endif
    );

  UInt64 curPacked, curUnpacked;

  CMyComPtr<IArchiveExtractCallbackMessage2> callbackMessage;
  extractCallback.QueryInterface(IID_IArchiveExtractCallbackMessage2, &callbackMessage);

  CFolderOutStream *folderOutStream = new CFolderOutStream;
  CMyComPtr<ISequentialOutStream> outStream(folderOutStream);

  folderOutStream->_db = &_db;
  folderOutStream->ExtractCallback = extractCallback;
  folderOutStream->TestMode = (testModeSpec != 0);
  folderOutStream->CheckCrc = (_crcSize != 0);

  for (UInt32 i = 0;; lps->OutSize += curUnpacked, lps->InSize += curPacked)
  {
    RINOK(lps->SetCur())

    if (i >= numItems)
      break;

    curUnpacked = 0;
    curPacked = 0;

    UInt32 fileIndex = allFilesMode ? i : indices[i];
    const CNum folderIndex = _db.FileIndexToFolderIndexMap[fileIndex];

    UInt32 numSolidFiles = 1;

    if (folderIndex != kNumNoIndex)
    {
      curPacked = _db.GetFolderFullPackSize(folderIndex);
      UInt32 nextFile = fileIndex + 1;
      fileIndex = _db.FolderStartFileIndex[folderIndex];
      UInt32 k;

      for (k = i + 1; k < numItems; k++)
      {
        const UInt32 fileIndex2 = allFilesMode ? k : indices[k];
        if (_db.FileIndexToFolderIndexMap[fileIndex2] != folderIndex
            || fileIndex2 < nextFile)
          break;
        nextFile = fileIndex2 + 1;
      }
      
      numSolidFiles = k - i;
      
      for (k = fileIndex; k < nextFile; k++)
        curUnpacked += _db.Files[k].Size;
    }

    {
      const HRESULT result = folderOutStream->Init(fileIndex,
          allFilesMode ? NULL : indices + i,
          numSolidFiles);

      i += numSolidFiles;

      RINOK(result)
    }

    if (folderOutStream->WasWritingFinished())
    {
      // for debug: to test zero size stream unpacking
      // if (folderIndex == kNumNoIndex)  // enable this check for debug
      continue;
    }

    if (folderIndex == kNumNoIndex)
      return E_FAIL;

    #ifndef Z7_NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (extractCallback)
      extractCallback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    #endif

    try
    {
      #ifndef Z7_NO_CRYPTO
        bool isEncrypted = false;
        bool passwordIsDefined = false;
        UString_Wipe password;
      #endif

      bool dataAfterEnd_Error = false;

      const HRESULT result = decoder.Decode(
          EXTERNAL_CODECS_VARS
          _inStream,
          _db.ArcInfo.DataStartPosition,
          _db, folderIndex,
          &curUnpacked,

          outStream,
          lps,
          NULL // *inStreamMainRes
          , dataAfterEnd_Error
          
          Z7_7Z_DECODER_CRYPRO_VARS
          #if !defined(Z7_ST)
            , true, _numThreads, _memUsage_Decompress
          #endif
          );

      if (result == S_FALSE || result == E_NOTIMPL || dataAfterEnd_Error)
      {
        const bool wasFinished = folderOutStream->WasWritingFinished();

        int resOp = NExtract::NOperationResult::kDataError;
        
        if (result != S_FALSE)
        {
          if (result == E_NOTIMPL)
            resOp = NExtract::NOperationResult::kUnsupportedMethod;
          else if (wasFinished && dataAfterEnd_Error)
            resOp = NExtract::NOperationResult::kDataAfterEnd;
        }

        RINOK(folderOutStream->FlushCorrupted(resOp))

        if (wasFinished)
        {
          // we don't show error, if it's after required files
          if (/* !folderOutStream->ExtraWriteWasCut && */ callbackMessage)
          {
            RINOK(callbackMessage->ReportExtractResult(NEventIndexType::kBlockIndex, folderIndex, resOp))
          }
        }
        continue;
      }
      
      if (result != S_OK)
        return result;

      RINOK(folderOutStream->FlushCorrupted(NExtract::NOperationResult::kDataError))
      continue;
    }
    catch(...)
    {
      RINOK(folderOutStream->FlushCorrupted(NExtract::NOperationResult::kDataError))
      // continue;
      // return E_FAIL;
      throw;
    }
  }

  return S_OK;

  COM_TRY_END
}

}}

/* ================ unit: CPP/7zip/Archive/7z/7zFolderInStream.cpp ================ */
// 7zFolderInStream.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

void CFolderInStream::Init(IArchiveUpdateCallback *updateCallback,
    const UInt32 *indexes, unsigned numFiles)
{
  _updateCallback = updateCallback;
  _indexes = indexes;
  _numFiles = numFiles;

  _totalSize_for_Coder = 0;
  ClearFileInfo();
  
  Processed.ClearAndReserve(numFiles);
  Sizes.ClearAndReserve(numFiles);
  CRCs.ClearAndReserve(numFiles);
  TimesDefined.ClearAndReserve(numFiles);
  MTimes.ClearAndReserve(Need_MTime ? numFiles : (unsigned)0);
  CTimes.ClearAndReserve(Need_CTime ? numFiles : (unsigned)0);
  ATimes.ClearAndReserve(Need_ATime ? numFiles : (unsigned)0);
  Attribs.ClearAndReserve(Need_Attrib ? numFiles : (unsigned)0);

  // FolderCrc = CRC_INIT_VAL;
  _stream.Release();
}

void CFolderInStream::ClearFileInfo()
{
  _pos = 0;
  _crc = CRC_INIT_VAL;
  _size_Defined = false;
  _times_Defined = false;
  _size = 0;
  FILETIME_Clear(_mTime);
  FILETIME_Clear(_cTime);
  FILETIME_Clear(_aTime);
  _attrib = 0;
}

HRESULT CFolderInStream::OpenStream()
{
  while (Processed.Size() < _numFiles)
  {
    CMyComPtr<ISequentialInStream> stream;
    const HRESULT result = _updateCallback->GetStream(_indexes[Processed.Size()], &stream);
    if (result != S_OK && result != S_FALSE)
      return result;

    _stream = stream;
    
    if (stream)
    {
      {
        CMyComPtr<IStreamGetProps> getProps;
        stream.QueryInterface(IID_IStreamGetProps, (void **)&getProps);
        if (getProps)
        {
          // access could be changed in first myx pass
          if (getProps->GetProps(&_size,
              Need_CTime ? &_cTime : NULL,
              Need_ATime ? &_aTime : NULL,
              Need_MTime ? &_mTime : NULL,
              Need_Attrib ? &_attrib : NULL)
              == S_OK)
          {
            _size_Defined = true;
            _times_Defined = true;
          }
          return S_OK;
        }
      }
      {
        CMyComPtr<IStreamGetSize> streamGetSize;
        stream.QueryInterface(IID_IStreamGetSize, &streamGetSize);
        if (streamGetSize)
        {
          if (streamGetSize->GetSize(&_size) == S_OK)
            _size_Defined = true;
        }
        return S_OK;
      }
    }
    
    RINOK(AddFileInfo(result == S_OK))
  }
  return S_OK;
}

static void AddFt(CRecordVector<UInt64> &vec, const FILETIME &ft)
{
  vec.AddInReserved(FILETIME_To_UInt64(ft));
}

/*
HRESULT ReportItemProps(IArchiveUpdateCallbackArcProp *reportArcProp,
    UInt32 index, UInt64 size, const UInt32 *crc)
{
  PROPVARIANT prop;
  prop.vt = VT_EMPTY;
  prop.wReserved1 = 0;
  
  NWindows::NCOM::PropVarEm_Set_UInt64(&prop, size);
  RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidSize, &prop));
  if (crc)
  {
    NWindows::NCOM::PropVarEm_Set_UInt32(&prop, *crc);
    RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidCRC, &prop));
  }
  return reportArcProp->ReportFinished(NEventIndexType::kOutArcIndex, index, NUpdate::NOperationResult::kOK);
}
*/

HRESULT CFolderInStream::AddFileInfo(bool isProcessed)
{
  // const UInt32 index = _indexes[Processed.Size()];
  Processed.AddInReserved(isProcessed);
  Sizes.AddInReserved(_pos);
  CRCs.AddInReserved(CRC_GET_DIGEST(_crc));
  if (Need_Attrib) Attribs.AddInReserved(_attrib);
  TimesDefined.AddInReserved(_times_Defined);
  if (Need_MTime) AddFt(MTimes, _mTime);
  if (Need_CTime) AddFt(CTimes, _cTime);
  if (Need_ATime) AddFt(ATimes, _aTime);
  ClearFileInfo();
  /*
  if (isProcessed && _reportArcProp)
    RINOK(ReportItemProps(_reportArcProp, index, _pos, &crc))
  */
  return _updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

Z7_COM7F_IMF(CFolderInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  while (size != 0)
  {
    if (_stream)
    {
      /*
      if (_pos == 0)
      {
        const UInt32 align = (UInt32)1 << AlignLog;
        const UInt32 offset = (UInt32)_totalSize_for_Coder & (align - 1);
        if (offset != 0)
        {
          UInt32 cur = align - offset;
          if (cur > size)
            cur = size;
          memset(data, 0, cur);
          data = (Byte *)data + cur;
          size -= cur;
          // _pos += cur; // for debug
          _totalSize_for_Coder += cur;
          if (processedSize)
            *processedSize += cur;
          continue;
        }
      }
      */
      UInt32 cur = size;
      const UInt32 kMax = (UInt32)1 << 20;
      if (cur > kMax)
        cur = kMax;
      RINOK(_stream->Read(data, cur, &cur))
      if (cur != 0)
      {
        // if (Need_Crc)
        _crc = CrcUpdate(_crc, data, cur);
        /*
        if (FolderCrc)
          FolderCrc = CrcUpdate(FolderCrc, data, cur);
        */
        _pos += cur;
        _totalSize_for_Coder += cur;
        if (processedSize)
          *processedSize = cur; // use +=cur, if continue is possible in loop
        return S_OK;
      }
      
      _stream.Release();
      RINOK(AddFileInfo(true))
    }
    
    if (Processed.Size() >= _numFiles)
      break;
    RINOK(OpenStream())
  }
  return S_OK;
}

Z7_COM7F_IMF(CFolderInStream::GetSubStreamSize(UInt64 subStream, UInt64 *value))
{
  *value = 0;
  if (subStream > Sizes.Size())
    return S_FALSE; // E_FAIL;
  
  const unsigned index = (unsigned)subStream;
  if (index < Sizes.Size())
  {
    *value = Sizes[index];
    return S_OK;
  }
  
  if (!_size_Defined)
  {
    *value = _pos;
    return S_FALSE;
  }
  
  *value = (_pos > _size ? _pos : _size);
  return S_OK;
}


/*
HRESULT CFolderInStream::CloseCrcStream()
{
  if (!_crcStream)
    return S_OK;
  if (!_crcStream_Spec->WasFinished())
    return E_FAIL;
  _crc = _crcStream_Spec->GetCRC() ^ CRC_INIT_VAL; // change it
  const UInt64 size = _crcStream_Spec->GetSize();
  _pos = size;
  _totalSize_for_Coder += size;
  _crcStream.Release();
  // _crcStream->ReleaseStream();
  _stream.Release();
  return AddFileInfo(true);
}

Z7_COM7F_IMF(CFolderInStream::GetNextInSubStream(UInt64 *streamIndexRes, ISequentialInStream **stream)
{
  RINOK(CloseCrcStream())
  *stream = NULL;
  *streamIndexRes = Processed.Size();
  if (Processed.Size() >= _numFiles)
    return S_OK;
  RINOK(OpenStream());
  if (!_stream)
    return S_OK;
  if (!_crcStream)
  {
    _crcStream_Spec = new CSequentialInStreamWithCRC;
    _crcStream = _crcStream_Spec;
  }
  _crcStream_Spec->Init();
  _crcStream_Spec->SetStream(_stream);
  *stream = _crcStream;
  _crcStream->AddRef();
  return S_OK;
}
*/

}}

/* ================ unit: CPP/7zip/Archive/7z/7zHandler.cpp ================ */
// 7zHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef Z7_7Z_SET_PROPERTIES
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef Z7_7Z_SET_PROPERTIES
#ifdef Z7_EXTRACT_ONLY
// amalgamation: header emitted in prologue
#endif
#endif

using namespace NWindows;
using namespace NCOM;

namespace NArchive {
namespace N7z {

CHandler::CHandler()
{
  #ifndef Z7_NO_CRYPTO
  _isEncrypted = false;
  _passwordIsDefined = false;
  #endif

  #ifdef Z7_EXTRACT_ONLY
  
  _crcSize = 4;
  
  #ifdef Z7_7Z_SET_PROPERTIES
  _useMultiThreadMixer = true;
  #endif
  
  #endif
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _db.Files.Size();
  return S_OK;
}

#ifdef Z7_SFX

IMP_IInArchive_ArcProps_NO_Table

Z7_COM7F_IMF(CHandler::GetNumberOfProperties(UInt32 *numProps))
{
  *numProps = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetPropertyInfo(UInt32 /* index */,
      BSTR * /* name */, PROPID * /* propID */, VARTYPE * /* varType */))
{
  return E_NOTIMPL;
}

#else

static const Byte kArcProps[] =
{
  kpidHeadersSize,
  kpidMethod,
  kpidSolid,
  kpidNumBlocks
  // , kpidIsTree
};

IMP_IInArchive_ArcProps

static inline char GetHex(unsigned value)
{
  return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

static unsigned ConvertMethodIdToString_Back(char *s, UInt64 id)
{
  int len = 0;
  do
  {
    s[--len] = GetHex((unsigned)id & 0xF); id >>= 4;
    s[--len] = GetHex((unsigned)id & 0xF); id >>= 4;
  }
  while (id != 0);
  return (unsigned)-len;
}

static void ConvertMethodIdToString(AString &res, UInt64 id)
{
  const unsigned kLen = 32;
  char s[kLen];
  unsigned len = kLen - 1;
  s[len] = 0;
  res += s + len - ConvertMethodIdToString_Back(s + len, id);
}


static char *GetStringForSizeValue(char *s, UInt32 val)
{
  for (unsigned i = 0; i < 32; i++)
    if (((UInt32)1 << i) == val)
    {
      if (i >= 10)
      {
        *s++= (char)('0' + i / 10);
        i %= 10;
      }
      *s++ = (char)('0' + i);
      *s = 0;
      return s;
    }
  
  char c = 'b';
  if      ((val & ((1 << 20) - 1)) == 0) { val >>= 20; c = 'm'; }
  else if ((val & ((1 << 10) - 1)) == 0) { val >>= 10; c = 'k'; }
  s = ConvertUInt32ToString(val, s);
  *s++ = c;
  *s = 0;
  return s;
}


static void GetLzma2String(char *s, unsigned d)
{
  if (d > 40)
  {
    *s = 0;
    return;
    // s = MyStpCpy(s, "unsup");
  }
  else if ((d & 1) == 0)
    d = (d >> 1) + 12;
  else
  {
    // s = GetStringForSizeValue(s, (UInt32)3 << ((d >> 1) + 11));
    d = (d >> 1) + 1;
    char c = 'k';
    if (d >= 10)
    {
      c = 'm';
      d -= 10;
    }
    s = ConvertUInt32ToString((UInt32)3 << d, s);
    *s++ = c;
    *s = 0;
    return;
  }
  ConvertUInt32ToString(d, s);
}


/*
static inline void AddHexToString(UString &res, Byte value)
{
  res += GetHex((Byte)(value >> 4));
  res += GetHex((Byte)(value & 0xF));
}
*/

static char *AddProp32(char *s, const char *name, UInt32 v)
{
  *s++ = ':';
  s = MyStpCpy(s, name);
  return ConvertUInt32ToString(v, s);
}
 
void CHandler::AddMethodName(AString &s, UInt64 id)
{
  AString name;
  FindMethod(EXTERNAL_CODECS_VARS id, name);
  if (name.IsEmpty())
    ConvertMethodIdToString(s, id);
  else
    s += name;
}

#endif

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  #ifndef Z7_SFX
  COM_TRY_BEGIN
  #endif
  NCOM::CPropVariant prop;
  switch (propID)
  {
    #ifndef Z7_SFX
    case kpidMethod:
    {
      AString s;
      const CParsedMethods &pm = _db.ParsedMethods;
      FOR_VECTOR (i, pm.IDs)
      {
        UInt64 id = pm.IDs[i];
        s.Add_Space_if_NotEmpty();
        char temp[16];
        if (id == k_LZMA2)
        {
          s += "LZMA2:";
          GetLzma2String(temp, pm.Lzma2Prop);
          s += temp;
        }
        else if (id == k_LZMA)
        {
          s += "LZMA:";
          GetStringForSizeValue(temp, pm.LzmaDic);
          s += temp;
        }
        /*
        else if (id == k_ZSTD)
        {
          s += "ZSTD";
        }
        */
        else
          AddMethodName(s, id);
      }
      prop = s;
      break;
    }
    case kpidSolid: prop = _db.IsSolid(); break;
    case kpidNumBlocks: prop = (UInt32)_db.NumFolders; break;
    case kpidHeadersSize:  prop = _db.HeadersSize; break;
    case kpidPhySize:  prop = _db.PhySize; break;
    case kpidOffset: if (_db.ArcInfo.StartPosition != 0) prop = _db.ArcInfo.StartPosition; break;
    /*
    case kpidIsTree: if (_db.IsTree) prop = true; break;
    case kpidIsAltStream: if (_db.ThereAreAltStreams) prop = true; break;
    case kpidIsAux: if (_db.IsTree) prop = true; break;
    */
    // case kpidError: if (_db.ThereIsHeaderError) prop = "Header error"; break;
    #endif
    
    case kpidWarningFlags:
    {
      UInt32 v = 0;
      if (_db.StartHeaderWasRecovered) v |= kpv_ErrorFlags_HeadersError;
      if (_db.UnsupportedFeatureWarning) v |= kpv_ErrorFlags_UnsupportedFeature;
      if (v != 0)
        prop = v;
      break;
    }
    
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_db.IsArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_db.ThereIsHeaderError) v |= kpv_ErrorFlags_HeadersError;
      if (_db.UnexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
      // if (_db.UnsupportedVersion) v |= kpv_ErrorFlags_Unsupported;
      if (_db.UnsupportedFeatureError) v |= kpv_ErrorFlags_UnsupportedFeature;
      prop = v;
      break;
    }

    case kpidReadOnly:
    {
      if (!_db.CanUpdate())
        prop = true;
      break;
    }
    default: break;
  }
  return prop.Detach(value);
  #ifndef Z7_SFX
  COM_TRY_END
  #endif
}

static void SetFileTimeProp_From_UInt64Def(PROPVARIANT *prop, const CUInt64DefVector &v, unsigned index)
{
  UInt64 value;
  if (v.GetItem(index, value))
    PropVarEm_Set_FileTime64_Prec(prop, value, k_PropVar_TimePrec_100ns);
}

bool CHandler::IsFolderEncrypted(CNum folderIndex) const
{
  if (folderIndex == kNumNoIndex)
    return false;
  const size_t startPos = _db.FoCodersDataOffset[folderIndex];
  const Byte *p = _db.CodersData.ConstData() + startPos;
  const size_t size = _db.FoCodersDataOffset[folderIndex + 1] - startPos;
  CInByte2 inByte;
  inByte.Init(p, size);
  
  CNum numCoders = inByte.ReadNum();
  for (; numCoders != 0; numCoders--)
  {
    const Byte mainByte = inByte.ReadByte();
    const unsigned idSize = (mainByte & 0xF);
    const Byte *longID = inByte.GetPtr();
    UInt64 id64 = 0;
    for (unsigned j = 0; j < idSize; j++)
      id64 = ((id64 << 8) | longID[j]);
    inByte.SkipDataNoCheck(idSize);
    if (id64 == k_AES)
      return true;
    if ((mainByte & 0x20) != 0)
      inByte.SkipDataNoCheck(inByte.ReadNum());
  }
  return false;
}

Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *numProps))
{
  *numProps = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 /* index */, BSTR *name, PROPID *propID))
{
  *name = NULL;
  *propID = kpidNtSecure;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 /* index */, UInt32 *parent, UInt32 *parentType))
{
  /*
  const CFileItem &file = _db.Files[index];
  *parentType = (file.IsAltStream ? NParentType::kAltStream : NParentType::kDir);
  *parent = (UInt32)(Int32)file.Parent;
  */
  *parentType = NParentType::kDir;
  *parent = (UInt32)(Int32)-1;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  if (/* _db.IsTree && propID == kpidName ||
      !_db.IsTree && */ propID == kpidPath)
  {
    if (_db.NameOffsets && _db.NamesBuf)
    {
      const size_t offset = _db.NameOffsets[index];
      const size_t size = (_db.NameOffsets[index + 1] - offset) * 2;
      if (size < ((UInt32)1 << 31))
      {
        *data = (const void *)(_db.NamesBuf.ConstData() + offset * 2);
        *dataSize = (UInt32)size;
        *propType = NPropDataType::kUtf16z;
      }
    }
    return S_OK;
  }
  /*
  if (propID == kpidNtSecure)
  {
    if (index < (UInt32)_db.SecureIDs.Size())
    {
      int id = _db.SecureIDs[index];
      size_t offs = _db.SecureOffsets[id];
      size_t size = _db.SecureOffsets[id + 1] - offs;
      if (size >= 0)
      {
        *data = _db.SecureBuf + offs;
        *dataSize = (UInt32)size;
        *propType = NPropDataType::kRaw;
      }
    }
  }
  */
  return S_OK;
}

#ifndef Z7_SFX

HRESULT CHandler::SetMethodToProp(CNum folderIndex, PROPVARIANT *prop) const
{
  PropVariant_Clear(prop);
  if (folderIndex == kNumNoIndex)
    return S_OK;
  // for (int ttt = 0; ttt < 1; ttt++) {
  const unsigned kTempSize = 256;
  char temp[kTempSize];
  unsigned pos = kTempSize;
  temp[--pos] = 0;
 
  const size_t startPos = _db.FoCodersDataOffset[folderIndex];
  const Byte *p = _db.CodersData.ConstData() + startPos;
  const size_t size = _db.FoCodersDataOffset[folderIndex + 1] - startPos;
  CInByte2 inByte;
  inByte.Init(p, size);
  
  // numCoders == 0 ???
  CNum numCoders = inByte.ReadNum();
  bool needSpace = false;
  
  for (; numCoders != 0; numCoders--, needSpace = true)
  {
    if (pos < 32) // max size of property
      break;
    const Byte mainByte = inByte.ReadByte();
    UInt64 id64 = 0;
    const unsigned idSize = (mainByte & 0xF);
    const Byte *longID = inByte.GetPtr();
    for (unsigned j = 0; j < idSize; j++)
      id64 = ((id64 << 8) | longID[j]);
    inByte.SkipDataNoCheck(idSize);

    if ((mainByte & 0x10) != 0)
    {
      inByte.ReadNum(); // NumInStreams
      inByte.ReadNum(); // NumOutStreams
    }
  
    CNum propsSize = 0;
    const Byte *props = NULL;
    if ((mainByte & 0x20) != 0)
    {
      propsSize = inByte.ReadNum();
      props = inByte.GetPtr();
      inByte.SkipDataNoCheck(propsSize);
    }
    
    const char *name = NULL;
    char s[32];
    s[0] = 0;
    
    if (id64 <= (UInt32)0xFFFFFFFF)
    {
      const UInt32 id = (UInt32)id64;
      if (id == k_LZMA)
      {
        name = "LZMA";
        if (propsSize == 5)
        {
          const UInt32 dicSize = GetUi32((const Byte *)props + 1);
          char *dest = GetStringForSizeValue(s, dicSize);
          UInt32 d = props[0];
          if (d != 0x5D)
          {
            const UInt32 lc = d % 9;
            d /= 9;
            const UInt32 pb = d / 5;
            const UInt32 lp = d % 5;
            if (lc != 3) dest = AddProp32(dest, "lc", lc);
            if (lp != 0) dest = AddProp32(dest, "lp", lp);
            if (pb != 2) dest = AddProp32(dest, "pb", pb);
          }
        }
      }
      else if (id == k_LZMA2)
      {
        name = "LZMA2";
        if (propsSize == 1)
          GetLzma2String(s, props[0]);
      }
      else if (id == k_PPMD)
      {
        name = "PPMD";
        if (propsSize == 5)
        {
          char *dest = s;
          *dest++ = 'o';
          dest = ConvertUInt32ToString(*props, dest);
          dest = MyStpCpy(dest, ":mem");
          GetStringForSizeValue(dest, GetUi32(props + 1));
        }
      }
      else if (id == k_Delta)
      {
        name = "Delta";
        if (propsSize == 1)
          ConvertUInt32ToString((UInt32)props[0] + 1, s);
      }
      else if (id == k_ARM64 || id == k_RISCV)
      {
        name = id == k_ARM64 ? "ARM64" : "RISCV";
        if (propsSize == 4)
          ConvertUInt32ToString(GetUi32(props), s);
        /*
        else if (propsSize != 0)
          MyStringCopy(s, "unsupported");
        */
      }
      else if (id == k_BCJ2) name = "BCJ2";
      else if (id == k_BCJ) name = "BCJ";
      else if (id == k_AES)
      {
        name = "7zAES";
        if (propsSize >= 1)
        {
          const Byte firstByte = props[0];
          const UInt32 numCyclesPower = firstByte & 0x3F;
          ConvertUInt32ToString(numCyclesPower, s);
        }
      }
    }
    
    if (name)
    {
      const unsigned nameLen = MyStringLen(name);
      const unsigned propsLen = MyStringLen(s);
      unsigned totalLen = nameLen + propsLen;
      if (propsLen != 0)
        totalLen++;
      if (needSpace)
        totalLen++;
      if (totalLen + 5 >= pos)
        break;
      pos -= totalLen;
      MyStringCopy(temp + pos, name);
      if (propsLen != 0)
      {
        char *dest = temp + pos + nameLen;
        *dest++ = ':';
        MyStringCopy(dest, s);
      }
      if (needSpace)
        temp[pos + totalLen - 1] = ' ';
    }
    else
    {
      AString methodName;
      FindMethod(EXTERNAL_CODECS_VARS id64, methodName);
      if (needSpace)
        temp[--pos] = ' ';
      if (methodName.IsEmpty())
        pos -= ConvertMethodIdToString_Back(temp + pos, id64);
      else
      {
        const unsigned len = methodName.Len();
        if (len + 5 > pos)
          break;
        pos -= len;
        for (unsigned i = 0; i < len; i++)
          temp[pos + i] = methodName[i];
      }
    }
  }
  
  if (numCoders != 0 && pos >= 4)
  {
    temp[--pos] = ' ';
    temp[--pos] = '.';
    temp[--pos] = '.';
    temp[--pos] = '.';
  }
  
  return PropVarEm_Set_Str(prop, temp + pos);
  // }
}

#endif

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  RINOK(PropVariant_Clear(value))
  // COM_TRY_BEGIN
  // NCOM::CPropVariant prop;
  
  /*
  const CRef2 &ref2 = _refs[index];
  if (ref2.Refs.IsEmpty())
    return E_FAIL;
  const CRef &ref = ref2.Refs.Front();
  */
  
  const CFileItem &item = _db.Files[index];
  const UInt32 index2 = index;

  switch (propID)
  {
    case kpidIsDir: PropVarEm_Set_Bool(value, item.IsDir); break;
    case kpidSize:
    {
      PropVarEm_Set_UInt64(value, item.Size);
      // prop = ref2.Size;
      break;
    }
    case kpidPackSize:
    {
      // prop = ref2.PackSize;
      {
        const CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          if (_db.FolderStartFileIndex[folderIndex] == (CNum)index2)
            PropVarEm_Set_UInt64(value, _db.GetFolderFullPackSize(folderIndex));
          /*
          else
            PropVarEm_Set_UInt64(value, 0);
          */
        }
        else
          PropVarEm_Set_UInt64(value, 0);
      }
      break;
    }
    // case kpidIsAux: prop = _db.IsItemAux(index2); break;
    case kpidPosition:  { UInt64 v; if (_db.StartPos.GetItem(index2, v)) PropVarEm_Set_UInt64(value, v); break; }
    case kpidCTime:  SetFileTimeProp_From_UInt64Def(value, _db.CTime, index2); break;
    case kpidATime:  SetFileTimeProp_From_UInt64Def(value, _db.ATime, index2); break;
    case kpidMTime:  SetFileTimeProp_From_UInt64Def(value, _db.MTime, index2); break;
    case kpidAttrib:  if (_db.Attrib.ValidAndDefined(index2)) PropVarEm_Set_UInt32(value, _db.Attrib.Vals[index2]); break;
    case kpidCRC:  if (item.CrcDefined) PropVarEm_Set_UInt32(value, item.Crc); break;
    case kpidEncrypted:  PropVarEm_Set_Bool(value, IsFolderEncrypted(_db.FileIndexToFolderIndexMap[index2])); break;
    case kpidIsAnti:  PropVarEm_Set_Bool(value, _db.IsItemAnti(index2)); break;
    /*
    case kpidIsAltStream:  prop = item.IsAltStream; break;
    case kpidNtSecure:
      {
        int id = _db.SecureIDs[index];
        size_t offs = _db.SecureOffsets[id];
        size_t size = _db.SecureOffsets[id + 1] - offs;
        if (size >= 0)
        {
          prop.SetBlob(_db.SecureBuf + offs, (ULONG)size);
        }
        break;
      }
    */

    case kpidPath: return _db.GetPath_Prop(index, value);
    
    #ifndef Z7_SFX
    
    case kpidMethod: return SetMethodToProp(_db.FileIndexToFolderIndexMap[index2], value);
    case kpidBlock:
      {
        const CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
          PropVarEm_Set_UInt32(value, (UInt32)folderIndex);
      }
      break;
   #ifdef Z7_7Z_SHOW_PACK_STREAMS_SIZES
    case kpidPackedSize0:
    case kpidPackedSize1:
    case kpidPackedSize2:
    case kpidPackedSize3:
    case kpidPackedSize4:
      {
        const CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          if (_db.FolderStartFileIndex[folderIndex] == (CNum)index2 &&
              _db.FoStartPackStreamIndex[folderIndex + 1] -
              _db.FoStartPackStreamIndex[folderIndex] > (propID - kpidPackedSize0))
          {
            PropVarEm_Set_UInt64(value, _db.GetFolderPackStreamSize(folderIndex, propID - kpidPackedSize0));
          }
        }
        else
          PropVarEm_Set_UInt64(value, 0);
      }
      break;
   #endif
    
    #endif
    default: break;
  }
  // return prop.Detach(value);
  return S_OK;
  // COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback))
{
  COM_TRY_BEGIN
  Close();
  #ifndef Z7_SFX
  _fileInfoPopIDs.Clear();
  #endif
  
  try
  {
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackTemp = openArchiveCallback;

    #ifndef Z7_NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (openArchiveCallback)
      openArchiveCallbackTemp.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    #endif

    CInArchive archive(
          #ifdef Z7_7Z_SET_PROPERTIES
          _useMultiThreadMixer
          #else
          true
          #endif
          );
    _db.IsArc = false;
    RINOK(archive.Open(stream, maxCheckStartPosition))
    _db.IsArc = true;
    
    HRESULT result = archive.ReadDatabase(
        EXTERNAL_CODECS_VARS
        _db
        #ifndef Z7_NO_CRYPTO
          , getTextPassword, _isEncrypted, _passwordIsDefined, _password
        #endif
        );
    RINOK(result)
    
    _inStream = stream;
  }
  catch(...)
  {
    Close();
    // return E_INVALIDARG;
    // return S_FALSE;
    // we must return out_of_memory here
    return E_OUTOFMEMORY;
  }
  // _inStream = stream;
  #ifndef Z7_SFX
  FillPopIDs();
  #endif
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  COM_TRY_BEGIN
  _inStream.Release();
  _db.Clear();
  #ifndef Z7_NO_CRYPTO
  _isEncrypted = false;
  _passwordIsDefined = false;
  _password.Wipe_and_Empty();
  #endif
  return S_OK;
  COM_TRY_END
}

#ifdef Z7_7Z_SET_PROPERTIES
#ifdef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  COM_TRY_BEGIN
  
  InitCommon();
  _useMultiThreadMixer = true;

  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeLower_Ascii();
    if (name.IsEmpty())
      return E_INVALIDARG;
    const PROPVARIANT &value = values[i];
    UInt32 number;
    const unsigned index = ParseStringToUInt32(name, number);
    if (index == 0)
    {
      if (name.IsEqualTo("mtf"))
      {
        RINOK(PROPVARIANT_to_bool(value, _useMultiThreadMixer))
        continue;
      }
      {
        HRESULT hres;
        if (SetCommonProperty(name, value, hres))
        {
          RINOK(hres)
          continue;
        }
      }
      return E_INVALIDARG;
    }
  }
  return S_OK;
  COM_TRY_END
}

#endif
#endif

IMPL_ISetCompressCodecsInfo

}}

/* ================ unit: CPP/7zip/Archive/7z/7zHandlerOut.cpp ================ */
// 7zHandlerOut.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef Z7_EXTRACT_ONLY

using namespace NWindows;

namespace NArchive {
namespace N7z {

static const UInt32 k_decoderCompatibilityVersion = 2301;
// 7-Zip version 2301 supports ARM64 filter

#define k_LZMA_Name "LZMA"
#define kDefaultMethodName "LZMA2"
#define k_Copy_Name "Copy"

#define k_MatchFinder_ForHeaders "BT2"

static const UInt32 k_NumFastBytes_ForHeaders = 273;
static const UInt32 k_Level_ForHeaders = 5;
static const UInt32 k_Dictionary_ForHeaders =
  #ifdef UNDER_CE
  1 << 18;
  #else
  1 << 20;
  #endif

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *type))
{
  *type = NFileTimeType::kWindows;
  return S_OK;
}

HRESULT CHandler::PropsMethod_To_FullMethod(CMethodFull &dest, const COneMethodInfo &m)
{
  bool isFilter;
  dest.CodecIndex = FindMethod_Index(
      EXTERNAL_CODECS_VARS
      m.MethodName, true,
      dest.Id, dest.NumStreams, isFilter);
  if (dest.CodecIndex < 0)
    return E_INVALIDARG;
  (CProps &)dest = (CProps &)m;
  return S_OK;
}

HRESULT CHandler::SetHeaderMethod(CCompressionMethodMode &headerMethod)
{
  if (!_compressHeaders)
    return S_OK;
  COneMethodInfo m;
  m.MethodName = k_LZMA_Name;
  m.AddProp_Ascii(NCoderPropID::kMatchFinder, k_MatchFinder_ForHeaders);
  m.AddProp_Level(k_Level_ForHeaders);
  m.AddProp32(NCoderPropID::kNumFastBytes, k_NumFastBytes_ForHeaders);
  m.AddProp32(NCoderPropID::kDictionarySize, k_Dictionary_ForHeaders);
  m.AddProp_NumThreads(1);

  CMethodFull &methodFull = headerMethod.Methods.AddNew();
  return PropsMethod_To_FullMethod(methodFull, m);
}


HRESULT CHandler::SetMainMethod(CCompressionMethodMode &methodMode)
{
  methodMode.Bonds = _bonds;

  // we create local copy of _methods. So we can modify it.
  CObjectVector<COneMethodInfo> methods = _methods;

  {
    FOR_VECTOR (i, methods)
    {
      AString &methodName = methods[i].MethodName;
      if (methodName.IsEmpty())
        methodName = kDefaultMethodName;
    }
    if (methods.IsEmpty())
    {
      COneMethodInfo &m = methods.AddNew();
      m.MethodName = (GetLevel() == 0 ? k_Copy_Name : kDefaultMethodName);
      methodMode.DefaultMethod_was_Inserted = true;
    }
  }

  if (!_filterMethod.MethodName.IsEmpty())
  {
    // if (methodMode.Bonds.IsEmpty())
    {
      FOR_VECTOR (k, methodMode.Bonds)
      {
        CBond2 &bond = methodMode.Bonds[k];
        bond.InCoder++;
        bond.OutCoder++;
      }
      methods.Insert(0, _filterMethod);
      methodMode.Filter_was_Inserted = true;
    }
  }

  const UInt64 kSolidBytes_Min = 1 << 24;
  const UInt64 kSolidBytes_Max = (UInt64)1 << 32;  // for non-LZMA2 methods

  bool needSolid = false;
  
  FOR_VECTOR (i, methods)
  {
    COneMethodInfo &oneMethodInfo = methods[i];

    SetGlobalLevelTo(oneMethodInfo);

#ifndef Z7_ST
    const bool numThreads_WasSpecifiedInMethod = (oneMethodInfo.Get_NumThreads() >= 0);
    if (!numThreads_WasSpecifiedInMethod)
    {
      // here we set the (NCoderPropID::kNumThreads) property in each method, only if there is no such property already
      CMultiMethodProps::SetMethodThreadsTo_IfNotFinded(oneMethodInfo, methodMode.NumThreads);
    }
    if (methodMode.NumThreadGroups > 1)
      CMultiMethodProps::Set_Method_NumThreadGroups_IfNotFinded(oneMethodInfo, methodMode.NumThreadGroups);
#endif

    CMethodFull &methodFull = methodMode.Methods.AddNew();
    RINOK(PropsMethod_To_FullMethod(methodFull, oneMethodInfo))

#ifndef Z7_ST
    methodFull.Set_NumThreads = true;
    methodFull.NumThreads = methodMode.NumThreads;
#endif

    if (methodFull.Id != k_Copy)
      needSolid = true;

    UInt64 dicSize;
    switch (methodFull.Id)
    {
      case k_LZMA:
      case k_LZMA2: dicSize = oneMethodInfo.Get_Lzma_DicSize(); break;
      case k_PPMD: dicSize = oneMethodInfo.Get_Ppmd_MemSize(); break;
      case k_Deflate: dicSize = (UInt32)1 << 15; break;
      case k_Deflate64: dicSize = (UInt32)1 << 16; break;
      case k_BZip2: dicSize = oneMethodInfo.Get_BZip2_BlockSize(); break;
      // case k_ZSTD: dicSize = 1 << 23; break;
      default: continue;
    }

    UInt64 numSolidBytes;

    /*
    if (methodFull.Id == k_ZSTD)
    {
      // continue;
      NCompress::NZstd::CEncoderProps encoderProps;
      RINOK(oneMethodInfo.Set_PropsTo_zstd(encoderProps));
      CZstdEncProps &zstdProps = encoderProps.EncProps;
      ZstdEncProps_NormalizeFull(&zstdProps);
      UInt64 cs = (UInt64)(zstdProps.jobSize);
      UInt32 winSize = (UInt32)(1 << zstdProps.windowLog);
      if (cs < winSize)
        cs = winSize;
      numSolidBytes = cs << 6;
      const UInt64 kSolidBytes_Zstd_Max = ((UInt64)1 << 34);
      if (numSolidBytes > kSolidBytes_Zstd_Max)
        numSolidBytes = kSolidBytes_Zstd_Max;

      methodFull.Set_NumThreads = false; // we don't use ICompressSetCoderMt::SetNumberOfThreads() for LZMA2 encoder

      #ifndef Z7_ST
      if (!numThreads_WasSpecifiedInMethod
          && !methodMode.NumThreads_WasForced
          && methodMode.MemoryUsageLimit_WasSet
          )
      {
        const UInt32 numThreads_Original = methodMode.NumThreads;
        const UInt32 numThreads_New = ZstdEncProps_GetNumThreads_for_MemUsageLimit(
            &zstdProps,
            methodMode.MemoryUsageLimit,
            numThreads_Original);
        if (numThreads_Original != numThreads_New)
        {
          CMultiMethodProps::SetMethodThreadsTo_Replace(methodFull, numThreads_New);
        }
      }
      #endif
    }
    else
    */
    if (methodFull.Id == k_LZMA2)
    {
      // he we calculate default chunk Size for LZMA2 as defined in LZMA2 encoder code
      /* lzma2 code use dictionary up to fake 4 GiB to calculate ChunkSize.
         So we do same */
      UInt64 cs = (UInt64)dicSize << 2;
      const UInt32 kMinSize = (UInt32)1 << 20;
      const UInt32 kMaxSize = (UInt32)1 << 28;
      if (cs < kMinSize) cs = kMinSize;
      if (cs > kMaxSize) cs = kMaxSize;
      if (cs < dicSize) cs = dicSize;
      cs += (kMinSize - 1);
      cs &= ~(UInt64)(kMinSize - 1);
      // we want to use at least 64 chunks (threads) per one solid block.

      // here we don't use chunkSize property
      numSolidBytes = cs << 6;

      // here we get real chunkSize
      cs = oneMethodInfo.Get_Xz_BlockSize();
      if (dicSize > cs)
          dicSize = cs;

      const UInt64 kSolidBytes_Lzma2_Max = (UInt64)1 << 34;
      if (numSolidBytes > kSolidBytes_Lzma2_Max)
          numSolidBytes = kSolidBytes_Lzma2_Max;

      methodFull.Set_NumThreads = false; // we don't use ICompressSetCoderMt::SetNumberOfThreads() for LZMA2 encoder

      #ifndef Z7_ST
      if (!numThreads_WasSpecifiedInMethod
          && !methodMode.NumThreads_WasForced
          && methodMode.MemoryUsageLimit_WasSet)
      {
        const UInt32 lzmaThreads = oneMethodInfo.Get_Lzma_NumThreads();
        const UInt32 numBlockThreads_Original = methodMode.NumThreads / lzmaThreads;

        if (numBlockThreads_Original > 1)
        {
          /*
            const UInt32 kNumThreads_Max = 1024;
            if (numBlockThreads > kNumMaxThreads)
            numBlockThreads = kNumMaxThreads;
          */

          UInt32 numBlockThreads = numBlockThreads_Original;
          const UInt64 lzmaMemUsage = oneMethodInfo.Get_Lzma_MemUsage(false); // solid
          
          for (; numBlockThreads > 1; numBlockThreads--)
          {
            UInt64 size = numBlockThreads * (lzmaMemUsage + cs);
            UInt32 numPackChunks = numBlockThreads + (numBlockThreads / 8) + 1;
            if (cs < ((UInt32)1 << 26)) numPackChunks++;
            if (cs < ((UInt32)1 << 24)) numPackChunks++;
            if (cs < ((UInt32)1 << 22)) numPackChunks++;
            size += numPackChunks * cs;
            // printf("\nnumBlockThreads = %d, size = %d\n", (unsigned)(numBlockThreads), (unsigned)(size >> 20));
            if (size <= methodMode.MemoryUsageLimit)
              break;
          }

          if (numBlockThreads == 0)
            numBlockThreads = 1;
          if (numBlockThreads != numBlockThreads_Original)
          {
            const UInt32 numThreads_New = numBlockThreads * lzmaThreads;
            CMultiMethodProps::SetMethodThreadsTo_Replace(methodFull, numThreads_New);
          }
        }
      }
      #endif
    }
    else
    {
      numSolidBytes = (UInt64)dicSize << 7;
      if (numSolidBytes > kSolidBytes_Max)
          numSolidBytes = kSolidBytes_Max;
    }

    if (_numSolidBytesDefined)
      continue;

    if (numSolidBytes < kSolidBytes_Min)
        numSolidBytes = kSolidBytes_Min;
    _numSolidBytes = numSolidBytes;
    _numSolidBytesDefined = true;
  }

  if (!_numSolidBytesDefined)
  {
    if (needSolid)
      _numSolidBytes = kSolidBytes_Max;
    else
      _numSolidBytes = 0;
  }
  _numSolidBytesDefined = true;


  return S_OK;
}



static HRESULT GetTime(IArchiveUpdateCallback *updateCallback, unsigned index, PROPID propID, UInt64 &ft, bool &ftDefined)
{
  // ft = 0;
  // ftDefined = false;
  NCOM::CPropVariant prop;
  RINOK(updateCallback->GetProperty(index, propID, &prop))
  if (prop.vt == VT_FILETIME)
  {
    ft = prop.filetime.dwLowDateTime | ((UInt64)prop.filetime.dwHighDateTime << 32);
    ftDefined = true;
  }
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  else
  {
    ft = 0;
    ftDefined = false;
  }
  return S_OK;
}

/*

#ifdef _WIN32
static const wchar_t kDirDelimiter1 = L'\\';
#endif
static const wchar_t kDirDelimiter2 = L'/';

static inline bool IsCharDirLimiter(wchar_t c)
{
  return (
    #ifdef _WIN32
    c == kDirDelimiter1 ||
    #endif
    c == kDirDelimiter2);
}

static int FillSortIndex(CObjectVector<CTreeFolder> &treeFolders, int cur, int curSortIndex)
{
  CTreeFolder &tf = treeFolders[cur];
  tf.SortIndex = curSortIndex++;
  for (int i = 0; i < tf.SubFolders.Size(); i++)
    curSortIndex = FillSortIndex(treeFolders, tf.SubFolders[i], curSortIndex);
  tf.SortIndexEnd = curSortIndex;
  return curSortIndex;
}

static int FindSubFolder(const CObjectVector<CTreeFolder> &treeFolders, int cur, const UString &name, int &insertPos)
{
  const CIntVector &subFolders = treeFolders[cur].SubFolders;
  int left = 0, right = subFolders.Size();
  insertPos = -1;
  for (;;)
  {
    if (left == right)
    {
      insertPos = left;
      return -1;
    }
    int mid = (left + right) / 2;
    int midFolder = subFolders[mid];
    int compare = CompareFileNames(name, treeFolders[midFolder].Name);
    if (compare == 0)
      return midFolder;
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
}

static int AddFolder(CObjectVector<CTreeFolder> &treeFolders, int cur, const UString &name)
{
  int insertPos;
  int folderIndex = FindSubFolder(treeFolders, cur, name, insertPos);
  if (folderIndex < 0)
  {
    folderIndex = treeFolders.Size();
    CTreeFolder &newFolder = treeFolders.AddNew();
    newFolder.Parent = cur;
    newFolder.Name = name;
    treeFolders[cur].SubFolders.Insert(insertPos, folderIndex);
  }
  // else if (treeFolders[folderIndex].IsAltStreamFolder != isAltStreamFolder) throw 1123234234;
  return folderIndex;
}
*/

Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback))
{
  COM_TRY_BEGIN

  const CDbEx *db = NULL;
  #ifdef Z7_7Z_VOL
  if (_volumes.Size() > 1)
    return E_FAIL;
  const CVolume *volume = 0;
  if (_volumes.Size() == 1)
  {
    volume = &_volumes.Front();
    db = &volume->Database;
  }
  #else
  if (_inStream)
    db = &_db;
  #endif

  if (db && !db->CanUpdate())
    return E_NOTIMPL;

  /*
  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveGetRawProps,
      getRawProps, updateCallback)

  CUniqBlocks secureBlocks;
  secureBlocks.AddUniq(NULL, 0);

  CObjectVector<CTreeFolder> treeFolders;
  {
    CTreeFolder folder;
    folder.Parent = -1;
    treeFolders.Add(folder);
  }
  */

  CObjectVector<CUpdateItem> updateItems;

  bool need_CTime = (TimeOptions.Write_CTime.Def && TimeOptions.Write_CTime.Val);
  bool need_ATime = (TimeOptions.Write_ATime.Def && TimeOptions.Write_ATime.Val);
  bool need_MTime = (TimeOptions.Write_MTime.Def ? TimeOptions.Write_MTime.Val : true);
  bool need_Attrib = (Write_Attrib.Def ? Write_Attrib.Val : true);
  
  if (db && !db->Files.IsEmpty())
  {
    if (!TimeOptions.Write_CTime.Def) need_CTime = !db->CTime.Defs.IsEmpty();
    if (!TimeOptions.Write_ATime.Def) need_ATime = !db->ATime.Defs.IsEmpty();
    if (!TimeOptions.Write_MTime.Def) need_MTime = !db->MTime.Defs.IsEmpty();
    if (!Write_Attrib.Def) need_Attrib = !db->Attrib.Defs.IsEmpty();
  }

  // UString s;
  UString name;

  for (UInt32 i = 0; i < numItems; i++)
  {
    Int32 newData, newProps;
    UInt32 indexInArchive;
    if (!updateCallback)
      return E_FAIL;
    RINOK(updateCallback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArchive))
    CUpdateItem ui;
    ui.NewProps = IntToBool(newProps);
    ui.NewData = IntToBool(newData);
    ui.IndexInArchive = (int)indexInArchive;
    ui.IndexInClient = i;
    ui.IsAnti = false;
    ui.Size = 0;

    name.Empty();
    // bool isAltStream = false;
    if (ui.IndexInArchive != -1)
    {
      if (!db || (unsigned)ui.IndexInArchive >= db->Files.Size())
        return E_INVALIDARG;
      const CFileItem &fi = db->Files[(unsigned)ui.IndexInArchive];
      if (!ui.NewProps)
      {
        _db.GetPath((unsigned)ui.IndexInArchive, name);
      }
      ui.IsDir = fi.IsDir;
      ui.Size = fi.Size;
      // isAltStream = fi.IsAltStream;
      ui.IsAnti = db->IsItemAnti((unsigned)ui.IndexInArchive);
      
      if (!ui.NewProps)
      {
        ui.CTimeDefined = db->CTime.GetItem((unsigned)ui.IndexInArchive, ui.CTime);
        ui.ATimeDefined = db->ATime.GetItem((unsigned)ui.IndexInArchive, ui.ATime);
        ui.MTimeDefined = db->MTime.GetItem((unsigned)ui.IndexInArchive, ui.MTime);
      }
    }

    if (ui.NewProps)
    {
      bool folderStatusIsDefined;
      if (need_Attrib)
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidAttrib, &prop))
        if (prop.vt == VT_EMPTY)
          ui.AttribDefined = false;
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
        {
          ui.Attrib = prop.ulVal;
          ui.AttribDefined = true;
        }
      }
      
      // we need MTime to sort files.
      if (need_CTime) RINOK(GetTime(updateCallback, i, kpidCTime, ui.CTime, ui.CTimeDefined))
      if (need_ATime) RINOK(GetTime(updateCallback, i, kpidATime, ui.ATime, ui.ATimeDefined))
      if (need_MTime) RINOK(GetTime(updateCallback, i, kpidMTime, ui.MTime, ui.MTimeDefined))

      /*
      if (getRawProps)
      {
        const void *data;
        UInt32 dataSize;
        UInt32 propType;

        getRawProps->GetRawProp(i, kpidNtSecure, &data, &dataSize, &propType);
        if (dataSize != 0 && propType != NPropDataType::kRaw)
          return E_FAIL;
        ui.SecureIndex = secureBlocks.AddUniq((const Byte *)data, dataSize);
      }
      */

      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidPath, &prop))
        if (prop.vt == VT_EMPTY)
        {
        }
        else if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        else
        {
          name = prop.bstrVal;
          NItemName::ReplaceSlashes_OsToUnix(name);
        }
      }
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidIsDir, &prop))
        if (prop.vt == VT_EMPTY)
          folderStatusIsDefined = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
        {
          ui.IsDir = (prop.boolVal != VARIANT_FALSE);
          folderStatusIsDefined = true;
        }
      }

      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidIsAnti, &prop))
        if (prop.vt == VT_EMPTY)
          ui.IsAnti = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
          ui.IsAnti = (prop.boolVal != VARIANT_FALSE);
      }

      /*
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidIsAltStream, &prop));
        if (prop.vt == VT_EMPTY)
          isAltStream = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
          isAltStream = (prop.boolVal != VARIANT_FALSE);
      }
      */

      if (ui.IsAnti)
      {
        ui.AttribDefined = false;

        ui.CTimeDefined = false;
        ui.ATimeDefined = false;
        ui.MTimeDefined = false;
        
        ui.Size = 0;
      }

      if (!folderStatusIsDefined && ui.AttribDefined)
        ui.SetDirStatusFromAttrib();
    }
    else
    {
      /*
      if (_db.SecureIDs.IsEmpty())
        ui.SecureIndex = secureBlocks.AddUniq(NULL, 0);
      else
      {
        int id = _db.SecureIDs[ui.IndexInArchive];
        size_t offs = _db.SecureOffsets[id];
        size_t size = _db.SecureOffsets[id + 1] - offs;
        ui.SecureIndex = secureBlocks.AddUniq(_db.SecureBuf + offs, size);
      }
      */
    }

    /*
    {
      int folderIndex = 0;
      if (_useParents)
      {
        int j;
        s.Empty();
        for (j = 0; j < name.Len(); j++)
        {
          wchar_t c = name[j];
          if (IsCharDirLimiter(c))
          {
            folderIndex = AddFolder(treeFolders, folderIndex, s);
            s.Empty();
            continue;
          }
          s += c;
        }
        if (isAltStream)
        {
          int colonPos = s.Find(':');
          if (colonPos < 0)
          {
            // isAltStream = false;
            return E_INVALIDARG;
          }
          UString mainName = s.Left(colonPos);
          int newFolderIndex = AddFolder(treeFolders, folderIndex, mainName);
          if (treeFolders[newFolderIndex].UpdateItemIndex < 0)
          {
            for (int j = updateItems.Size() - 1; j >= 0; j--)
            {
              CUpdateItem &ui2 = updateItems[j];
              if (ui2.ParentFolderIndex == folderIndex
                  && ui2.Name == mainName)
              {
                ui2.TreeFolderIndex = newFolderIndex;
                treeFolders[newFolderIndex].UpdateItemIndex = j;
              }
            }
          }
          folderIndex = newFolderIndex;
          s.Delete(0, colonPos + 1);
        }
        ui.Name = s;
      }
      else
        ui.Name = name;
      ui.IsAltStream = isAltStream;
      ui.ParentFolderIndex = folderIndex;
      ui.TreeFolderIndex = -1;
      if (ui.IsDir && !s.IsEmpty())
      {
        ui.TreeFolderIndex = AddFolder(treeFolders, folderIndex, s);
        treeFolders[ui.TreeFolderIndex].UpdateItemIndex = updateItems.Size();
      }
    }
    */
    ui.Name = name;

    if (ui.NewData)
    {
      ui.Size = 0;
      if (!ui.IsDir)
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidSize, &prop))
        if (prop.vt != VT_UI8)
          return E_INVALIDARG;
        ui.Size = (UInt64)prop.uhVal.QuadPart;
        if (ui.Size != 0 && ui.IsAnti)
          return E_INVALIDARG;
      }
    }
    
    updateItems.Add(ui);
  }

  /*
  FillSortIndex(treeFolders, 0, 0);
  for (i = 0; i < (UInt32)updateItems.Size(); i++)
  {
    CUpdateItem &ui = updateItems[i];
    ui.ParentSortIndex = treeFolders[ui.ParentFolderIndex].SortIndex;
    ui.ParentSortIndexEnd = treeFolders[ui.ParentFolderIndex].SortIndexEnd;
  }
  */

  CCompressionMethodMode methodMode, headerMethod;

  methodMode.MemoryUsageLimit = _memUsage_Compress;
  methodMode.MemoryUsageLimit_WasSet = _memUsage_WasSet;

  #ifndef Z7_ST
  {
    UInt32 numThreads = _numThreads;
    const UInt32 kNumThreads_Max = 1024;
    if (numThreads > kNumThreads_Max)
      numThreads = kNumThreads_Max;
    methodMode.NumThreads = numThreads;
    methodMode.NumThreads_WasForced = _numThreads_WasForced;
    methodMode.MultiThreadMixer = _useMultiThreadMixer;
#ifdef _WIN32
    methodMode.NumThreadGroups = _numThreadGroups; // _change it
#endif
    // headerMethod.NumThreads = 1;
    headerMethod.MultiThreadMixer = _useMultiThreadMixer;
  }
  #endif

  const HRESULT res = SetMainMethod(methodMode);
  RINOK(res)

  RINOK(SetHeaderMethod(headerMethod))
  
  Z7_DECL_CMyComPtr_QI_FROM(
    ICryptoGetTextPassword2,
    getPassword2, updateCallback)

  methodMode.PasswordIsDefined = false;
  methodMode.Password.Wipe_and_Empty();
  if (getPassword2)
  {
    CMyComBSTR_Wipe password;
    Int32 passwordIsDefined;
    RINOK(getPassword2->CryptoGetTextPassword2(&passwordIsDefined, &password))
    methodMode.PasswordIsDefined = IntToBool(passwordIsDefined);
    if (methodMode.PasswordIsDefined && password)
      methodMode.Password = password;
  }

  bool compressMainHeader = _compressHeaders;  // check it

  bool encryptHeaders = false;

  #ifndef Z7_NO_CRYPTO
  if (!methodMode.PasswordIsDefined && _passwordIsDefined)
  {
    // if header is compressed, we use that password for updated archive
    methodMode.PasswordIsDefined = true;
    methodMode.Password = _password;
  }
  #endif

  if (methodMode.PasswordIsDefined)
  {
    if (_encryptHeadersSpecified)
      encryptHeaders = _encryptHeaders;
    #ifndef Z7_NO_CRYPTO
    else
      encryptHeaders = _passwordIsDefined;
    #endif
    compressMainHeader = true;
    if (encryptHeaders)
    {
      headerMethod.PasswordIsDefined = methodMode.PasswordIsDefined;
      headerMethod.Password = methodMode.Password;
    }
  }

  if (numItems < 2)
    compressMainHeader = false;

  const int level = GetLevel();

  CUpdateOptions options;
  options.Need_CTime = need_CTime;
  options.Need_ATime = need_ATime;
  options.Need_MTime = need_MTime;
  options.Need_Attrib = need_Attrib;
  // options.Need_Crc = (_crcSize != 0); // for debug

  options.Method = &methodMode;
  options.HeaderMethod = (_compressHeaders || encryptHeaders) ? &headerMethod : NULL;
  options.UseFilters = (level != 0 && _autoFilter && !methodMode.Filter_was_Inserted);
  options.MaxFilter = (level >= 8);
  options.AnalysisLevel = GetAnalysisLevel();

  options.SetFilterSupporting_ver_enabled_disabled(
      _decoderCompatibilityVersion,
      _enabledFilters,
      _disabledFilters);

  options.HeaderOptions.CompressMainHeader = compressMainHeader;
  /*
  options.HeaderOptions.WriteCTime = Write_CTime;
  options.HeaderOptions.WriteATime = Write_ATime;
  options.HeaderOptions.WriteMTime = Write_MTime;
  options.HeaderOptions.WriteAttrib = Write_Attrib;
  */
  
  options.NumSolidFiles = _numSolidFiles;
  options.NumSolidBytes = _numSolidBytes;
  options.SolidExtension = _solidExtension;
  options.UseTypeSorting = _useTypeSorting;

  options.RemoveSfxBlock = _removeSfxBlock;
  // options.VolumeMode = _volumeMode;

  options.MultiThreadMixer = _useMultiThreadMixer;

  /*
  if (secureBlocks.Sorted.Size() > 1)
  {
    secureBlocks.GetReverseMap();
    for (int i = 0; i < updateItems.Size(); i++)
    {
      int &secureIndex = updateItems[i].SecureIndex;
      secureIndex = secureBlocks.BufIndexToSortedIndex[secureIndex];
    }
  }
  */

  return Update(
      EXTERNAL_CODECS_VARS
      #ifdef Z7_7Z_VOL
      volume ? volume->Stream: 0,
      volume ? db : 0,
      #else
      _inStream,
      db,
      #endif
      updateItems,
      // treeFolders,
      // secureBlocks,
      outStream, updateCallback, options);

  COM_TRY_END
}

static HRESULT ParseBond(UString &srcString, UInt32 &coder, UInt32 &stream)
{
  stream = 0;
  {
    const unsigned index = ParseStringToUInt32(srcString, coder);
    if (index == 0)
      return E_INVALIDARG;
    srcString.DeleteFrontal(index);
  }
  if (srcString[0] == 's')
  {
    srcString.Delete(0);
    const unsigned index = ParseStringToUInt32(srcString, stream);
    if (index == 0)
      return E_INVALIDARG;
    srcString.DeleteFrontal(index);
  }
  return S_OK;
}

void COutHandler::InitProps7z()
{
  _removeSfxBlock = false;
  _compressHeaders = true;
  _encryptHeadersSpecified = false;
  _encryptHeaders = false;
  // _useParents = false;
  
  TimeOptions.Init();
  Write_Attrib.Init();

  _useMultiThreadMixer = true;

  // _volumeMode = false;

  InitSolid();
  _useTypeSorting = false;

  _decoderCompatibilityVersion = k_decoderCompatibilityVersion;
  _enabledFilters.Clear();
  _disabledFilters.Clear();
}

void COutHandler::InitProps()
{
  CMultiMethodProps::Init();
  InitProps7z();
}



HRESULT COutHandler::SetSolidFromString(const UString &s)
{
  UString s2 = s;
  s2.MakeLower_Ascii();
  for (unsigned i = 0; i < s2.Len();)
  {
    const wchar_t *start = ((const wchar_t *)s2) + i;
    const wchar_t *end;
    UInt64 v = ConvertStringToUInt64(start, &end);
    if (start == end)
    {
      if (s2[i++] != 'e')
        return E_INVALIDARG;
      _solidExtension = true;
      continue;
    }
    i += (unsigned)(end - start);
    if (i == s2.Len())
      return E_INVALIDARG;
    const wchar_t c = s2[i++];
    if (c == 'f')
    {
      if (v < 1)
        v = 1;
      _numSolidFiles = v;
    }
    else
    {
      unsigned numBits;
      switch (c)
      {
        case 'b': numBits =  0; break;
        case 'k': numBits = 10; break;
        case 'm': numBits = 20; break;
        case 'g': numBits = 30; break;
        case 't': numBits = 40; break;
        default: return E_INVALIDARG;
      }
      _numSolidBytes = (v << numBits);
      _numSolidBytesDefined = true;
      /*
      if (_numSolidBytes == 0)
        _numSolidFiles = 1;
      */
    }
  }
  return S_OK;
}

HRESULT COutHandler::SetSolidFromPROPVARIANT(const PROPVARIANT &value)
{
  bool isSolid;
  switch (value.vt)
  {
    case VT_EMPTY: isSolid = true; break;
    case VT_BOOL: isSolid = (value.boolVal != VARIANT_FALSE); break;
    case VT_BSTR:
      if (StringToBool(value.bstrVal, isSolid))
        break;
      return SetSolidFromString(value.bstrVal);
    default: return E_INVALIDARG;
  }
  if (isSolid)
    InitSolid();
  else
    _numSolidFiles = 1;
  return S_OK;
}

static HRESULT PROPVARIANT_to_BoolPair(const PROPVARIANT &prop, CBoolPair &dest)
{
  RINOK(PROPVARIANT_to_bool(prop, dest.Val))
  dest.Def = true;
  return S_OK;
}

struct C_Id_Name_pair
{
  UInt32 Id;
  const char *Name;
};

static const C_Id_Name_pair g_filter_pairs[] =
{
  { k_Delta, "Delta" },
  { k_ARM64, "ARM64" },
  { k_RISCV, "RISCV" },
  { k_SWAP2, "SWAP2" },
  { k_SWAP4, "SWAP4" },
  { k_BCJ,   "BCJ" },
  { k_BCJ2 , "BCJ2" },
  { k_PPC,   "PPC" },
  { k_IA64,  "IA64" },
  { k_ARM,   "ARM" },
  { k_ARMT,  "ARMT" },
  { k_SPARC, "SPARC" }
};


HRESULT COutHandler::SetProperty(const wchar_t *nameSpec, const PROPVARIANT &value)
{
  UString name = nameSpec;
  name.MakeLower_Ascii();
  if (name.IsEmpty())
    return E_INVALIDARG;
  
  if (name[0] == L's')
  {
    name.Delete(0);
    if (name.IsEmpty())
      return SetSolidFromPROPVARIANT(value);
    if (value.vt != VT_EMPTY)
      return E_INVALIDARG;
    return SetSolidFromString(name);
  }

  UInt32 number;
  const unsigned index = ParseStringToUInt32(name, number);
  // UString realName = name.Ptr(index);
  if (index == 0)
  {
    if (name.IsEqualTo("rsfx")) return PROPVARIANT_to_bool(value, _removeSfxBlock);
    if (name.IsEqualTo("hc")) return PROPVARIANT_to_bool(value, _compressHeaders);
    // if (name.IsEqualToNoCase(L"HS")) return PROPVARIANT_to_bool(value, _useParents);
    
    if (name.IsEqualTo("hcf"))
    {
      bool compressHeadersFull = true;
      RINOK(PROPVARIANT_to_bool(value, compressHeadersFull))
      return compressHeadersFull ? S_OK: E_INVALIDARG;
    }
    
    if (name.IsEqualTo("he"))
    {
      RINOK(PROPVARIANT_to_bool(value, _encryptHeaders))
      _encryptHeadersSpecified = true;
      return S_OK;
    }
    
    {
      bool processed;
      RINOK(TimeOptions.Parse(name, value, processed))
      if (processed)
      {
        if (   TimeOptions.Prec != (UInt32)(Int32)-1
            && TimeOptions.Prec != k_PropVar_TimePrec_0
            && TimeOptions.Prec != k_PropVar_TimePrec_HighPrec
            && TimeOptions.Prec != k_PropVar_TimePrec_100ns)
          return E_INVALIDARG;
        return S_OK;
      }
    }

    if (name.IsEqualTo("tr")) return PROPVARIANT_to_BoolPair(value, Write_Attrib);
    
    if (name.IsEqualTo("mtf")) return PROPVARIANT_to_bool(value, _useMultiThreadMixer);

    if (name.IsEqualTo("qs")) return PROPVARIANT_to_bool(value, _useTypeSorting);

    if (name.IsPrefixedBy_Ascii_NoCase("yv"))
    {
      name.Delete(0, 2);
      UInt32 v = 1 << 16;  // if no number is noit specified, we use big value
      RINOK(ParsePropToUInt32(name, value, v))
      _decoderCompatibilityVersion = v;
      // if (v == 0) _decoderCompatibilityVersion = k_decoderCompatibilityVersion;
      return S_OK;
    }

    if (name.IsPrefixedBy_Ascii_NoCase("yf"))
    {
      name.Delete(0, 2);
      CUIntVector *vec;
           if (name.IsEqualTo_Ascii_NoCase("a")) vec = &_enabledFilters;
      else if (name.IsEqualTo_Ascii_NoCase("d")) vec = &_disabledFilters;
      else return E_INVALIDARG;

      if (value.vt != VT_BSTR)
        return E_INVALIDARG;
      for (unsigned k = 0;; k++)
      {
        if (k == Z7_ARRAY_SIZE(g_filter_pairs))
        {
          // maybe we can ignore unsupported filter names here?
          return E_INVALIDARG;
        }
        const C_Id_Name_pair &pair = g_filter_pairs[k];
        if (StringsAreEqualNoCase_Ascii(value.bstrVal, pair.Name))
        {
          vec->AddToUniqueSorted(pair.Id);
          break;
        }
      }
      return S_OK;
    }

    // if (name.IsEqualTo("v"))  return PROPVARIANT_to_bool(value, _volumeMode);
  }
  return CMultiMethodProps::SetProperty(name, value);
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  COM_TRY_BEGIN
  _bonds.Clear();
  InitProps();

  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeLower_Ascii();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &value = values[i];

    if (name.Find(L':') >= 0) // 'b' was used as NCoderPropID::kBlockSize2 before v23
    if (name[0] == 'b')
    {
      if (value.vt != VT_EMPTY)
        return E_INVALIDARG;
      name.Delete(0);
      
      CBond2 bond;
      RINOK(ParseBond(name, bond.OutCoder, bond.OutStream))
      if (name[0] != ':')
        return E_INVALIDARG;
      name.Delete(0);
      UInt32 inStream = 0;
      RINOK(ParseBond(name, bond.InCoder, inStream))
      if (inStream != 0)
        return E_INVALIDARG;
      if (!name.IsEmpty())
        return E_INVALIDARG;
      _bonds.Add(bond);
      continue;
    }

    RINOK(SetProperty(name, value))
  }

  unsigned numEmptyMethods = GetNumEmptyMethods();
  if (numEmptyMethods > 0)
  {
    unsigned k;
    for (k = 0; k < _bonds.Size(); k++)
    {
      const CBond2 &bond = _bonds[k];
      if (bond.InCoder < (UInt32)numEmptyMethods ||
          bond.OutCoder < (UInt32)numEmptyMethods)
        return E_INVALIDARG;
    }
    for (k = 0; k < _bonds.Size(); k++)
    {
      CBond2 &bond = _bonds[k];
      bond.InCoder -= (UInt32)numEmptyMethods;
      bond.OutCoder -= (UInt32)numEmptyMethods;
    }
    _methods.DeleteFrontal(numEmptyMethods);
  }
  
  FOR_VECTOR (k, _bonds)
  {
    const CBond2 &bond = _bonds[k];
    if (bond.InCoder >= (UInt32)_methods.Size() ||
        bond.OutCoder >= (UInt32)_methods.Size())
      return E_INVALIDARG;
  }

  return S_OK;
  COM_TRY_END
}

}}

#endif

/* ================ unit: CPP/7zip/Archive/7z/7zHeader.cpp ================ */
// 7zHeader.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace N7z {

Byte kSignature[kSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
#ifdef Z7_7Z_VOL
Byte kFinishSignature[kSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C + 1};
#endif

// We can change signature. So file doesn't contain correct signature.
// struct SignatureInitializer { SignatureInitializer() { kSignature[0]--; } };
// static SignatureInitializer g_SignatureInitializer;

}}
