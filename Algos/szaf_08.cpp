/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Archive standalone handler.
 *
 * 14 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Archive/StdAfx.h ---- */
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

/* ---- CPP/Windows/PropVariantUtils.h ---- */
// Windows/PropVariantUtils.h

#ifndef ZIP7_INC_PROP_VARIANT_UTILS_H
#define ZIP7_INC_PROP_VARIANT_UTILS_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

struct CUInt32PCharPair
{
  UInt32 Value;
  const char *Name;
};

AString TypePairToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 value);
void PairToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 value, NWindows::NCOM::CPropVariant &prop);

AString FlagsToString(const char * const *names, unsigned num, UInt32 flags);
AString FlagsToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags);
void FlagsToProp(const char * const *names, unsigned num, UInt32 flags, NWindows::NCOM::CPropVariant &prop);
void FlagsToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags, NWindows::NCOM::CPropVariant &prop);

AString TypeToString(const char * const table[], unsigned num, UInt32 value);
void TypeToProp(const char * const table[], unsigned num, UInt32 value, NWindows::NCOM::CPropVariant &prop);

#define PAIR_TO_PROP(pairs, value, prop) PairToProp(pairs, Z7_ARRAY_SIZE(pairs), value, prop)
#define FLAGS_TO_PROP(pairs, value, prop) FlagsToProp(pairs, Z7_ARRAY_SIZE(pairs), value, prop)
#define TYPE_TO_PROP(table, value, prop) TypeToProp(table, Z7_ARRAY_SIZE(table), value, prop)

void Flags64ToProp(const CUInt32PCharPair *pairs, unsigned num, UInt64 flags, NWindows::NCOM::CPropVariant &prop);
#define FLAGS64_TO_PROP(pairs, value, prop) Flags64ToProp(pairs, Z7_ARRAY_SIZE(pairs), value, prop)

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

/* ---- CPP/7zip/Archive/HandlerCont.h ---- */
// HandlerCont.h

#ifndef ZIP7_INC_HANDLER_CONT_H
#define ZIP7_INC_HANDLER_CONT_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {

#define Z7_IFACEM_IInArchive_Cont(x) \
  x(Open(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openCallback)) \
  x(Close()) \
  x(GetNumberOfItems(UInt32 *numItems)) \
  x(GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  /* x(Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback)) */ \
  x(GetArchiveProperty(PROPID propID, PROPVARIANT *value)) \
  x(GetNumberOfProperties(UInt32 *numProps)) \
  x(GetPropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \
  x(GetNumberOfArchiveProperties(UInt32 *numProps)) \
  x(GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \


//  #define Z7_COM7F_PUREO(f)     virtual Z7_COM7F_IMF(f)     Z7_override =0;
//  #define Z7_COM7F_PUREO2(t, f) virtual Z7_COM7F_IMF2(t, f) Z7_override =0;

class CHandlerCont:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_2(
      IInArchive,
      IInArchiveGetStream)
  /*
  Z7_IFACEM_IInArchive_Cont(Z7_COM7F_PUREO)
  // Z7_IFACE_COM7_PURE(IInArchive_Cont)
  */
  Z7_COM7F_IMP(Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback))
protected:
  Z7_IFACE_COM7_IMP(IInArchiveGetStream)

  CMyComPtr<IInStream> _stream;
  virtual int GetItem_ExtractInfo(UInt32 index, UInt64 &pos, UInt64 &size) const = 0;
  // destructor must be virtual for this class
  virtual ~CHandlerCont() {}
};



#define Z7_IFACEM_IInArchive_Img(x) \
  /* x(Open(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openCallback)) */ \
  x(Close()) \
  /* x(GetNumberOfItems(UInt32 *numItems)) */ \
  x(GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) \
  /* x(Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback)) */ \
  x(GetArchiveProperty(PROPID propID, PROPVARIANT *value)) \
  x(GetNumberOfProperties(UInt32 *numProps)) \
  x(GetPropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \
  x(GetNumberOfArchiveProperties(UInt32 *numProps)) \
  x(GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)) \


class CHandlerImg:
  public IInArchive,
  public IInArchiveGetStream,
  public IInStream,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_4(
      IInArchive,
      IInArchiveGetStream,
      ISequentialInStream,
      IInStream)

  Z7_COM7F_IMP(Open(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openCallback))
  Z7_COM7F_IMP(GetNumberOfItems(UInt32 *numItems))
  Z7_COM7F_IMP(Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback))
  Z7_IFACE_COM7_IMP(IInStream)
  // Z7_IFACEM_IInArchive_Img(Z7_COM7F_PUREO)

protected:
  bool _stream_unavailData;
  bool _stream_unsupportedMethod;
  bool _stream_dataError;
  // bool _stream_UsePackSize;
  // UInt64 _stream_PackSize;
  UInt64 _virtPos;
  UInt64 _posInArc;
  UInt64 _size;
  CMyComPtr<IInStream> Stream;
  const char *_imgExt;
  
  void Reset_PosInArc() { _posInArc = (UInt64)0 - 1; }
  void Reset_VirtPos() { _virtPos = (UInt64)0; }

  void ClearStreamVars()
  {
    _stream_unavailData = false;
    _stream_unsupportedMethod = false;
    _stream_dataError = false;
    // _stream_UsePackSize = false;
    // _stream_PackSize = 0;
  }

  void Clear_HandlerImg_Vars(); // it doesn't Release (Stream) var.

  virtual HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openCallback) = 0;
  virtual void CloseAtError();
  
  // returns (true), if Get_PackSizeProcessed() is required in Extract()
  virtual bool Init_PackSizeProcessed()
  {
    return false;
  }
public:
  virtual bool Get_PackSizeProcessed(UInt64 &size)
  {
    size = 0;
    return false;
  }

  CHandlerImg();
  // destructor must be virtual for this class
  virtual ~CHandlerImg() {}
};


HRESULT ReadZeroTail(ISequentialInStream *stream, bool &areThereNonZeros, UInt64 &numZeros, UInt64 maxSize);

}

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

/* ---- CPP/7zip/Archive/Common/MultiStream.h ---- */
// MultiStream.h

#ifndef ZIP7_INC_MULTI_STREAM_H
#define ZIP7_INC_MULTI_STREAM_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

Z7_CLASS_IMP_IInStream(
  CMultiStream
)

  unsigned _streamIndex;
  UInt64 _pos;
  UInt64 _totalLength;

public:

  struct CSubStreamInfo
  {
    CMyComPtr<IInStream> Stream;
    UInt64 Size;
    UInt64 GlobalOffset;
    UInt64 LocalPos;
    CSubStreamInfo(): Size(0), GlobalOffset(0), LocalPos(0) {}
  };

  CMyComPtr<IArchiveUpdateCallbackFile> updateCallbackFile;
  CObjectVector<CSubStreamInfo> Streams;
 
  HRESULT Init()
  {
    UInt64 total = 0;
    FOR_VECTOR (i, Streams)
    {
      CSubStreamInfo &s = Streams[i];
      s.GlobalOffset = total;
      total += s.Size;
      s.LocalPos = 0;
      {
        // it was already set to start
        // RINOK(InStream_GetPos(s.Stream, s.LocalPos));
      }
    }
    _totalLength = total;
    _pos = 0;
    _streamIndex = 0;
    return S_OK;
  }
};

/*
Z7_CLASS_IMP_COM_1(
  COutMultiStream,
  IOutStream
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  unsigned _streamIndex; // required stream
  UInt64 _offsetPos; // offset from start of _streamIndex index
  UInt64 _absPos;
  UInt64 _length;

  struct CSubStreamInfo
  {
    CMyComPtr<ISequentialOutStream> Stream;
    UInt64 Size;
    UInt64 Pos;
 };
  CObjectVector<CSubStreamInfo> Streams;
public:
  CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
  void Init()
  {
    _streamIndex = 0;
    _offsetPos = 0;
    _absPos = 0;
    _length = 0;
  }
};
*/

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

/* ---- CPP/7zip/Common/CWrappers.h ---- */
// CWrappers.h

#ifndef ZIP7_INC_C_WRAPPERS_H
#define ZIP7_INC_C_WRAPPERS_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

SRes HRESULT_To_SRes(HRESULT res, SRes defaultRes) throw();
HRESULT SResToHRESULT(SRes res) throw();

struct CCompressProgressWrap
{
  ICompressProgress vt;
  ICompressProgressInfo *Progress;
  HRESULT Res;
  
  void Init(ICompressProgressInfo *progress) throw();
};


struct CSeqInStreamWrap
{
  ISeqInStream vt;
  ISequentialInStream *Stream;
  HRESULT Res;
  UInt64 Processed;
  
  void Init(ISequentialInStream *stream) throw();
};


struct CSeekInStreamWrap
{
  ISeekInStream vt;
  IInStream *Stream;
  HRESULT Res;
  
  void Init(IInStream *stream) throw();
};


struct CSeqOutStreamWrap
{
  ISeqOutStream vt;
  ISequentialOutStream *Stream;
  HRESULT Res;
  UInt64 Processed;
  
  void Init(ISequentialOutStream *stream) throw();
};


struct CByteInBufWrap
{
  IByteIn vt;
  const Byte *Cur;
  const Byte *Lim;
  Byte *Buf;
  UInt32 Size;
  ISequentialInStream *Stream;
  UInt64 Processed;
  bool Extra;
  HRESULT Res;
  
  CByteInBufWrap() throw();
  ~CByteInBufWrap() { Free(); }
  void Free() throw();
  bool Alloc(UInt32 size) throw();
  void Init()
  {
    Lim = Cur = Buf;
    Processed = 0;
    Extra = false;
    Res = S_OK;
  }
  UInt64 GetProcessed() const { return Processed + (size_t)(Cur - Buf); }
  Byte ReadByteFromNewBlock() throw();
  Byte ReadByte()
  {
    if (Cur != Lim)
      return *Cur++;
    return ReadByteFromNewBlock();
  }
};


/*
struct CLookToSequentialWrap
{
  Byte *BufBase;
  UInt32 Size;
  ISequentialInStream *Stream;
  UInt64 Processed;
  bool Extra;
  HRESULT Res;
  
  CLookToSequentialWrap(): BufBase(NULL) {}
  ~CLookToSequentialWrap() { Free(); }
  void Free() throw();
  bool Alloc(UInt32 size) throw();
  void Init()
  {
    // Lim = Cur = Buf;
    Processed = 0;
    Extra = false;
    Res = S_OK;
  }
  // UInt64 GetProcessed() const { return Processed + (Cur - Buf); }

  Byte ReadByteFromNewBlock() throw();
  Byte ReadByte()
  {
    if (Cur != Lim)
      return *Cur++;
    return ReadByteFromNewBlock();
  }
};

EXTERN_C_BEGIN
// void CLookToSequentialWrap_Look(ILookInSeqStream *pp);
EXTERN_C_END
*/



struct CByteOutBufWrap
{
  IByteOut vt;
  Byte *Cur;
  const Byte *Lim;
  Byte *Buf;
  size_t Size;
  ISequentialOutStream *Stream;
  UInt64 Processed;
  HRESULT Res;
  
  CByteOutBufWrap() throw();
  ~CByteOutBufWrap() { Free(); }
  void Free() throw();
  bool Alloc(size_t size) throw();
  void Init()
  {
    Cur = Buf;
    Lim = Buf + Size;
    Processed = 0;
    Res = S_OK;
  }
  UInt64 GetProcessed() const { return Processed + (size_t)(Cur - Buf); }
  HRESULT Flush() throw();
  void WriteByte(Byte b)
  {
    *Cur++ = b;
    if (Cur == Lim)
      Flush();
  }
};


/*
struct CLookOutWrap
{
  ILookOutStream vt;
  Byte *Buf;
  size_t Size;
  ISequentialOutStream *Stream;
  UInt64 Processed;
  HRESULT Res;
  
  CLookOutWrap() throw();
  ~CLookOutWrap() { Free(); }
  void Free() throw();
  bool Alloc(size_t size) throw();
  void Init()
  {
    Processed = 0;
    Res = S_OK;
  }
};
*/

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

/* ---- CPP/7zip/Compress/BitlDecoder.h ---- */
// BitlDecoder.h -- the Least Significant Bit of byte is First

#ifndef ZIP7_INC_BITL_DECODER_H
#define ZIP7_INC_BITL_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NBitl {

const unsigned kNumBigValueBits = 8 * 4;
const unsigned kNumValueBytes = 3;
const unsigned kNumValueBits = 8 * kNumValueBytes;
const UInt32 kMask = (1 << kNumValueBits) - 1;

#if !defined(Z7_BITL_USE_REVERSE_BITS_TABLE)
#if 1 && defined(MY_CPU_ARM_OR_ARM64) \
    && (defined(MY_CPU_ARM64) || defined(__ARM_ARCH_6T2__) \
       || defined(__ARM_ARCH) && (__ARM_ARCH >= 7)) \
    && (defined(__GNUC__) && (__GNUC__ >= 4) \
       || defined(__clang__) && (__clang_major__ >= 4))
  #define Z7_BITL_USE_REVERSE_BITS_INSTRUCTION
#elif 1
  #define Z7_BITL_USE_REVERSE_BITS_TABLE
#endif
#endif

#if defined(Z7_BITL_USE_REVERSE_BITS_TABLE)
extern Byte kReverseTable[256];
#endif

inline unsigned ReverseBits8(unsigned i)
{
#if defined(Z7_BITL_USE_REVERSE_BITS_TABLE)
  return kReverseTable[i];
#elif defined(Z7_BITL_USE_REVERSE_BITS_INSTRUCTION)
  // rbit is available in ARMv6T2 and above
  asm ("rbit "
#if defined(MY_CPU_ARM)
    "%0,%0"   // it uses default register size,
              // but we need 32-bit register here.
              // we must use it only if default register size is 32-bit.
              // it will work incorrectly for ARM64.
#else
    "%w0,%w0" // it uses 32-bit registers in ARM64.
              // compiler for (MY_CPU_ARM) can't compile it.
#endif
    : "+r" (i));
  return i >> 24;
#else
  unsigned
      x = ((i & 0x55) << 1) | ((i >> 1) & 0x55);
      x = ((x & 0x33) << 2) | ((x >> 2) & 0x33);
  return  ((x & 0x0f) << 4) |  (x >> 4);
#endif
}


/* TInByte must support "Extra Bytes" (bytes that can be read after the end of stream
   TInByte::ReadByte() returns 0xFF after the end of stream
   TInByte::NumExtraBytes contains the number "Extra Bytes"
       
   Bitl decoder can read up to 4 bytes ahead to internal buffer. */

template<class TInByte>
class CBaseDecoder
{
protected:
  unsigned _bitPos;
  UInt32 _value;
  TInByte _stream;
public:
  bool Create(UInt32 bufSize) { return _stream.Create(bufSize); }
  void SetStream(ISequentialInStream *inStream) { _stream.SetStream(inStream); }
  void ClearStreamPtr() { _stream.ClearStreamPtr(); }
  void Init()
  {
    _stream.Init();
    _bitPos = kNumBigValueBits;
    _value = 0;
  }

  // the size of portion data in real stream that was already read from this object.
  // it doesn't include unused data in BitStream object buffer (up to 4 bytes)
  // it doesn't include unused data in TInByte buffers
  // it doesn't include virtual Extra bytes after the end of real stream data
  UInt64 GetStreamSize() const
  {
    return ExtraBitsWereRead() ?
        _stream.GetStreamSize():
        GetProcessedSize();
  }

  // the size of virtual data that was read from this object.
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() - ((kNumBigValueBits - _bitPos) >> 3); }

  bool ThereAreDataInBitsBuffer() const { return this->_bitPos != kNumBigValueBits; }
  
  Z7_FORCE_INLINE
  void Normalize()
  {
    for (; _bitPos >= 8; _bitPos -= 8)
      _value = ((UInt32)_stream.ReadByte() << (kNumBigValueBits - _bitPos)) | _value;
  }
  
  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits)
  {
    Normalize();
    UInt32 res = _value & ((1 << numBits) - 1);
    _bitPos += numBits;
    _value >>= numBits;
    return res;
  }

  bool ExtraBitsWereRead() const
  {
    return (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
  }
  
  bool ExtraBitsWereRead_Fast() const
  {
    // full version is not inlined in vc6.
    // return _stream.NumExtraBytes != 0 && (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
    
    // (_stream.NumExtraBytes > 4) is fast overread detection. It's possible that
    // it doesn't return true, if small number of extra bits were read.
    return (_stream.NumExtraBytes > 4);
  }

  // it must be fixed !!! with extra bits
  // UInt32 GetNumExtraBytes() const { return _stream.NumExtraBytes; }
};

template<class TInByte>
class CDecoder: public CBaseDecoder<TInByte>
{
  UInt32 _normalValue;

public:
  void Init()
  {
    CBaseDecoder<TInByte>::Init();
    _normalValue = 0;
  }
  
  Z7_FORCE_INLINE
  void Normalize()
  {
    for (; this->_bitPos >= 8; this->_bitPos -= 8)
    {
      const unsigned b = this->_stream.ReadByte();
      _normalValue = ((UInt32)b << (kNumBigValueBits - this->_bitPos)) | _normalValue;
      this->_value = (this->_value << 8) | ReverseBits8(b);
    }
  }
  
  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits)
  {
    Normalize();
    return ((this->_value >> (8 - this->_bitPos)) & kMask) >> (kNumValueBits - numBits);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits()
  {
    Normalize();
    return this->_value << this->_bitPos;
  }
  
  Z7_FORCE_INLINE
  void MovePos(size_t numBits)
  {
    this->_bitPos += (unsigned)numBits;
    _normalValue >>= numBits;
  }
  
  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits)
  {
    Normalize();
    UInt32 res = _normalValue & ((1 << numBits) - 1);
    MovePos(numBits);
    return res;
  }

  void AlignToByte() { MovePos((32 - this->_bitPos) & 7); }
  
  Z7_FORCE_INLINE
  Byte ReadDirectByte() { return this->_stream.ReadByte(); }

  Z7_FORCE_INLINE
  size_t ReadDirectBytesPart(Byte *buf, size_t size) { return this->_stream.ReadBytesPart(buf, size); }
  
  Z7_FORCE_INLINE
  Byte ReadAlignedByte()
  {
    if (this->_bitPos == kNumBigValueBits)
      return this->_stream.ReadByte();
    Byte b = (Byte)(_normalValue & 0xFF);
    MovePos(8);
    return b;
  }

  // call it only if the object is aligned for byte.
  Z7_FORCE_INLINE
  bool ReadAlignedByte_FromBuf(Byte &b)
  {
    if (this->_stream.NumExtraBytes != 0)
      if (this->_stream.NumExtraBytes >= 4
          || kNumBigValueBits - this->_bitPos <= (this->_stream.NumExtraBytes << 3))
        return false;
    if (this->_bitPos == kNumBigValueBits)
      return this->_stream.ReadByte_FromBuf(b);
    b = (Byte)(_normalValue & 0xFF);
    MovePos(8);
    return true;
  }
};

}

#endif

/* ---- CPP/7zip/Compress/DeflateConst.h ---- */
// DeflateConst.h

#ifndef ZIP7_INC_DEFLATE_CONST_H
#define ZIP7_INC_DEFLATE_CONST_H

namespace NCompress {
namespace NDeflate {

const unsigned kNumHuffmanBits = 15;

const UInt32 kHistorySize32 = (1 << 15);
const UInt32 kHistorySize64 = (1 << 16);

const unsigned kDistTableSize32 = 30;
const unsigned kDistTableSize64 = 32;
  
const unsigned kNumLenSymbols32 = 256;
const unsigned kNumLenSymbols64 = 255; // don't change it. It must be <= 255.
const unsigned kNumLenSymbolsMax = kNumLenSymbols32;
  
const unsigned kNumLenSlots = 29;

const unsigned kFixedDistTableSize = 32;
const unsigned kFixedLenTableSize = 31;

const unsigned kSymbolEndOfBlock = 0x100;
const unsigned kSymbolMatch = kSymbolEndOfBlock + 1;

const unsigned kMainTableSize = kSymbolMatch + kNumLenSlots;
const unsigned kFixedMainTableSize = kSymbolMatch + kFixedLenTableSize;

const unsigned kLevelTableSize = 19;

const unsigned kTableDirectLevels = 16;
const unsigned kTableLevelRepNumber = kTableDirectLevels;
const unsigned kTableLevel0Number = kTableLevelRepNumber + 1;
const unsigned kTableLevel0Number2 = kTableLevel0Number + 1;

const unsigned kLevelMask = 0xF;

const Byte kLenStart32[kFixedLenTableSize] =
  {0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224, 255, 0, 0};
const Byte kLenStart64[kFixedLenTableSize] =
  {0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224, 0, 0, 0};

const Byte kLenDirectBits32[kFixedLenTableSize] =
  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 0, 0, 0};
const Byte kLenDirectBits64[kFixedLenTableSize] =
  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 16, 0, 0};

const UInt32 kDistStart[kDistTableSize64] =
  {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,
  1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152};
const Byte kDistDirectBits[kDistTableSize64] =
  {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14};

const Byte kLevelDirectBits[3] = {2, 3, 7};

const Byte kCodeLengthAlphabetOrder[kLevelTableSize] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

const unsigned kMatchMinLen = 3;
const unsigned kMatchMaxLen32 = kNumLenSymbols32 + kMatchMinLen - 1; // 256 + 2
const unsigned kMatchMaxLen64 = kNumLenSymbols64 + kMatchMinLen - 1; // 255 + 2
const unsigned kMatchMaxLen = kMatchMaxLen32;

const unsigned kFinalBlockFieldSize = 1;

namespace NFinalBlockField
{
  enum
  {
    kNotFinalBlock = 0,
    kFinalBlock = 1
  };
}

const unsigned kBlockTypeFieldSize = 2;

namespace NBlockType
{
  enum
  {
    kStored = 0,
    kFixedHuffman = 1,
    kDynamicHuffman = 2
  };
}

const unsigned kNumLenCodesFieldSize = 5;
const unsigned kNumDistCodesFieldSize = 5;
const unsigned kNumLevelCodesFieldSize = 4;

const unsigned kNumLitLenCodesMin = 257;
const unsigned kNumDistCodesMin = 1;
const unsigned kNumLevelCodesMin = 4;

const unsigned kLevelFieldSize = 3;

const unsigned kStoredBlockLengthFieldSize = 16;

struct CLevels
{
  Byte litLenLevels[kFixedMainTableSize];
  Byte distLevels[kFixedDistTableSize];

  void SubClear()
  {
    unsigned i;
    for (i = kNumLitLenCodesMin; i < kFixedMainTableSize; i++)
      litLenLevels[i] = 0;
    for (i = 0; i < kFixedDistTableSize; i++)
      distLevels[i] = 0;
  }

  void SetFixedLevels()
  {
    unsigned i = 0;
    
    for (; i < 144; i++) litLenLevels[i] = 8;
    for (; i < 256; i++) litLenLevels[i] = 9;
    for (; i < 280; i++) litLenLevels[i] = 7;
    for (; i < 288; i++) litLenLevels[i] = 8;
    
    for (i = 0; i < kFixedDistTableSize; i++)  // test it: InfoZip only uses kDistTableSize
      distLevels[i] = 5;
  }
};

}}

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

/* ---- CPP/7zip/Compress/LzOutWindow.h ---- */
// LzOutWindow.h

#ifndef ZIP7_INC_LZ_OUT_WINDOW_H
#define ZIP7_INC_LZ_OUT_WINDOW_H

// amalgamation: header emitted in prologue

#ifndef Z7_NO_EXCEPTIONS
typedef COutBufferException CLzOutWindowException;
#endif

class CLzOutWindow: public COutBuffer
{
public:
  void Init(bool solid = false) throw();
  
  // distance >= 0, len > 0,
  bool CopyBlock(UInt32 distance, UInt32 len)
  {
    UInt32 pos = _pos - distance - 1;
    if (distance >= _pos)
    {
      if (!_overDict || distance >= _bufSize)
        return false;
      pos += _bufSize;
    }
    if (_limitPos - _pos > len && _bufSize - pos > len)
    {
      const Byte *src = _buf + pos;
      Byte *dest = _buf + _pos;
      _pos += len;
      do
        *dest++ = *src++;
      while (--len != 0);
    }
    else do
    {
      UInt32 pos2;
      if (pos == _bufSize)
        pos = 0;
      pos2 = _pos;
      _buf[pos2++] = _buf[pos++];
      _pos = pos2;
      if (pos2 == _limitPos)
        FlushWithCheck();
    }
    while (--len != 0);
    return true;
  }
  
  void PutByte(Byte b)
  {
    UInt32 pos = _pos;
    _buf[pos++] = b;
    _pos = pos;
    if (pos == _limitPos)
      FlushWithCheck();
  }

  void PutBytes(const Byte *data, UInt32 size)
  {
    if (size == 0)
      return;
    UInt32 pos = _pos;
    Byte *buf = _buf;
    buf[pos++] = *data++;
    size--;
    for (;;)
    {
      UInt32 limitPos = _limitPos;
      UInt32 rem = limitPos - pos;
      if (rem == 0)
      {
        _pos = pos;
        FlushWithCheck();
        pos = _pos;
        continue;
      }
      
      if (size == 0)
        break;
      
      if (rem > size)
        rem = size;
      size -= rem;
      do
        buf[pos++] = *data++;
      while (--rem);
    }
    _pos = pos;
  }
  
  Byte GetByte(UInt32 distance) const
  {
    UInt32 pos = _pos - distance - 1;
    if (distance >= _pos)
      pos += _bufSize;
    return _buf[pos];
  }
};

#endif

/* ---- CPP/7zip/Compress/DeflateDecoder.h ---- */
// DeflateDecoder.h

#ifndef ZIP7_INC_DEFLATE_DECODER_H
#define ZIP7_INC_DEFLATE_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

const int kLenIdFinished = -1;
const int kLenIdNeedInit = -2;

const unsigned kNumTableBits_Main = 10;
const unsigned kNumTableBits_Dist = 6;

class CCoder:
  public ICompressCoder,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressReadUnusedFromInBuf,
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
#ifndef Z7_NO_READ_FROM_CODER
  public ISequentialInStream,
#endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressReadUnusedFromInBuf)
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_COM_QI_ENTRY(ISequentialInStream)
#endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
public:
  Z7_IFACE_COM7_IMP(ICompressReadUnusedFromInBuf)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
private:
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ISequentialInStream)
#endif

  CLzOutWindow m_OutWindowStream;
  NBitl::CDecoder<CInBuffer> m_InBitStream;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kFixedMainTableSize, kNumTableBits_Main> m_MainDecoder;
  NCompress::NHuffman::CDecoder256<kNumHuffmanBits, kFixedDistTableSize, kNumTableBits_Dist> m_DistDecoder;
  NCompress::NHuffman::CDecoder7b<kLevelTableSize> m_LevelDecoder;

  UInt32 m_StoredBlockSize;

  unsigned _numDistLevels;
  bool m_FinalBlock;
  bool m_StoredMode;

  bool _deflateNSIS;
  bool _deflate64Mode;
  bool _keepHistory;
  bool _needFinishInput;
  
  bool _needInitInStream;
  bool _needReadTable;
  Int32 _remainLen;
  UInt32 _rep0;

  bool _outSizeDefined;
  CMyComPtr<ISequentialInStream> m_InStreamRef;
  UInt64 _outSize;
  UInt64 _outStartPos;

  void SetOutStreamSizeResume(const UInt64 *outSize);
  UInt64 GetOutProcessedCur() const { return m_OutWindowStream.GetProcessedSize() - _outStartPos; }

  UInt32 ReadBits(unsigned numBits);

  bool DecodeLevels(Byte *levels, unsigned numSymbols);
  bool ReadTables();
  
  HRESULT Flush() { return m_OutWindowStream.Flush(); }
  class CCoderReleaser
  {
    CCoder *_coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CCoder *coder): _coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        _coder->Flush();
    }
  };
  friend class CCoderReleaser;

  HRESULT CodeSpec(UInt32 curSize, bool finishInputStream, UInt32 inputProgressLimit = 0);
public:

  CCoder(bool deflate64Mode);
  virtual ~CCoder() {}

  void SetNsisMode(bool nsisMode) { _deflateNSIS = nsisMode; }

  void Set_KeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
  void Set_NeedFinishInput(bool needFinishInput) { _needFinishInput = needFinishInput; }

  bool IsFinished() const { return _remainLen == kLenIdFinished; }
  bool IsFinalBlock() const { return m_FinalBlock; }

  HRESULT CodeReal(ISequentialOutStream *outStream, ICompressProgressInfo *progress);

public:
  HRESULT CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress);
  HRESULT InitInStream(bool needInit);

  void AlignToByte() { m_InBitStream.AlignToByte(); }
  Byte ReadAlignedByte();
  UInt32 ReadAligned_UInt16() // aligned for Byte range
  {
    const UInt32 v = m_InBitStream.ReadAlignedByte();
    return v | ((UInt32)m_InBitStream.ReadAlignedByte() << 8);
  }
  bool InputEofError() const { return m_InBitStream.ExtraBitsWereRead(); }

  // size of used real data from input stream
  UInt64 GetStreamSize() const { return m_InBitStream.GetStreamSize(); }

  // size of virtual input stream processed
  UInt64 GetInputProcessedSize() const { return m_InBitStream.GetProcessedSize(); }
};

class CCOMCoder     : public CCoder { public: CCOMCoder(): CCoder(false) {} };
class CCOMCoder64   : public CCoder { public: CCOMCoder64(): CCoder(true) {} };

}}}

#endif

/* ---- CPP/7zip/Compress/ZlibDecoder.h ---- */
// ZlibDecoder.h

#ifndef ZIP7_INC_ZLIB_DECODER_H
#define ZIP7_INC_ZLIB_DECODER_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZlib {

const UInt32 ADLER_INIT_VAL = 1;

Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithAdler
  , ISequentialOutStream
)
  UInt32 _adler;
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _adler = ADLER_INIT_VAL; _size = 0; }
  UInt32 GetAdler() const { return _adler; }
  UInt64 GetSize() const { return _size; }
};

Z7_CLASS_IMP_NOQIB_1(
  CDecoder
  , ICompressCoder
)
  CMyComPtr2<ISequentialOutStream, COutStreamWithAdler> AdlerStream;
  CMyComPtr2<ICompressCoder, NDeflate::NDecoder::CCOMCoder> DeflateDecoder;
  Int32 _inputProcessedSize_Additional;
public:
  bool IsAdlerOptional;
  
  CDecoder(): IsAdlerOptional(false) {}
  UInt64 GetInputProcessedSize() const
  {
    return (UInt64)(
      (Int64)DeflateDecoder->GetInputProcessedSize() +
      (Int64)_inputProcessedSize_Additional);
  }
  UInt64 GetOutputProcessedSize() const { return AdlerStream->GetSize(); }
};

static bool inline IsZlib(const Byte *p)
{
  if ((p[0] & 0xF) != 8) // method
    return false;
  if (((unsigned)p[0] >> 4) > 7) // logar_window_size minus 8.
    return false;
  if ((p[1] & 0x20) != 0) // dictPresent
    return false;
  if ((((UInt32)p[0] << 8) + p[1]) % 31 != 0)
    return false;
  return true;
}

// IsZlib_3bytes checks 2 bytes of zlib header and starting byte of Deflate stream

static bool inline IsZlib_3bytes(const Byte *p)
{
  if (!IsZlib(p))
    return false;
  const unsigned val = p[2];
  const unsigned blockType = (val >> 1) & 0x3;
  if (blockType == 3) // unsupported block type for deflate
    return false;
  if (blockType == NCompress::NDeflate::NBlockType::kStored && (val >> 3) != 0)
    return false;
  return true;
}

}}

#endif

/* ---- CPP/7zip/Compress/LzmaDecoder.h ---- */
// LzmaDecoder.h

#ifndef ZIP7_INC_LZMA_DECODER_H
#define ZIP7_INC_LZMA_DECODER_H

// #include "../../../C/Alloc.h"
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzma {

class CDecoder Z7_final:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressSetBufSize,
 #ifndef Z7_NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetDecoderProperties2)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressSetBufSize)
 #ifndef Z7_NO_READ_FROM_CODER
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
  Z7_COM_QI_ENTRY(ISequentialInStream)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
public:
  Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
private:
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
  // Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)

  Z7_IFACE_COM7_IMP(ICompressSetBufSize)

 #ifndef Z7_NO_READ_FROM_CODER
public:
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
private:
  Z7_IFACE_COM7_IMP(ISequentialInStream)
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
 #else
  Z7_COM7F_IMF(SetOutStreamSize(const UInt64 *outSize));
 #endif

public:
  bool FinishStream; // set it before decoding, if you need to decode full LZMA stream
private:
  bool _propsWereSet;
  bool _outSizeDefined;

  UInt32 _outStep;
  UInt32 _inBufSize;
  UInt32 _inBufSizeNew;

  ELzmaStatus _lzmaStatus;
  UInt32 _inPos;
  UInt32 _inLim;
  Byte *_inBuf;
 
  UInt64 _outSize;
  UInt64 _inProcessed;
  UInt64 _outProcessed;

  // CAlignOffsetAlloc _alloc;

  CLzmaDec _state;

  HRESULT CreateInputBuffer();
  HRESULT CodeSpec(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress);
  void SetOutStreamSizeResume(const UInt64 *outSize);

 #ifndef Z7_NO_READ_FROM_CODER
private:
  CMyComPtr<ISequentialInStream> _inStream;
public:
  HRESULT CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress);
  HRESULT ReadFromInputStream(void *data, UInt32 size, UInt32 *processedSize);
 #endif

public:
  CDecoder();
  ~CDecoder();

  UInt64 GetInputProcessedSize() const { return _inProcessed; }
  UInt64 GetOutputProcessedSize() const { return _outProcessed; }
  bool NeedsMoreInput() const { return _lzmaStatus == LZMA_STATUS_NEEDS_MORE_INPUT; }
  bool CheckFinishStatus(bool withEndMark) const
  {
    return _lzmaStatus == (withEndMark ?
        LZMA_STATUS_FINISHED_WITH_MARK :
        LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK);
  }
};

}}

#endif

/* ---- CPP/7zip/Archive/Common/DummyOutStream.h ---- */
// DummyOutStream.h

#ifndef ZIP7_INC_DUMMY_OUT_STREAM_H
#define ZIP7_INC_DUMMY_OUT_STREAM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_NOQIB_1(
  CDummyOutStream
  , ISequentialOutStream
)
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialOutStream *outStream) { _stream = outStream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _size = 0; }
  UInt64 GetSize() const { return _size; }
};

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

/* ---- CPP/7zip/Compress/LzmaEncoder.h ---- */
// LzmaEncoder.h

#ifndef ZIP7_INC_LZMA_ENCODER_H
#define ZIP7_INC_LZMA_ENCODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzma {

class CEncoder Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public ICompressSetCoderPropertiesOpt,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_4(
      ICompressCoder,
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties,
      ICompressSetCoderPropertiesOpt)
  Z7_IFACE_COM7_IMP(ICompressCoder)
public:
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
  Z7_IFACE_COM7_IMP(ICompressWriteCoderProperties)
  Z7_IFACE_COM7_IMP(ICompressSetCoderPropertiesOpt)

  CLzmaEncHandle _encoder;
  UInt64 _inputProcessed;

  CEncoder();
  ~CEncoder();

  UInt64 GetInputProcessedSize() const { return _inputProcessed; }
  bool IsWriteEndMark() const { return LzmaEnc_IsWriteEndMark(_encoder) != 0; }
};

}}

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

/* ---- CPP/7zip/Compress/BitlEncoder.h ---- */
// BitlEncoder.h -- the Least Significant Bit of byte is First

#ifndef ZIP7_INC_BITL_ENCODER_H
#define ZIP7_INC_BITL_ENCODER_H

// amalgamation: header emitted in prologue

class CBitlEncoder
{
  COutBuffer _stream;
  unsigned _bitPos;
  Byte _curByte;
public:
  bool Create(UInt32 bufSize) { return _stream.Create(bufSize); }
  void SetStream(ISequentialOutStream *outStream) { _stream.SetStream(outStream); }
  // unsigned GetBitPosition() const { return (8 - _bitPos); }
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() + ((8 - _bitPos + 7) >> 3); }
  void Init()
  {
    _stream.Init();
    _bitPos = 8;
    _curByte = 0;
  }
  HRESULT Flush()
  {
    FlushByte();
    return _stream.Flush();
  }
  void FlushByte()
  {
    if (_bitPos < 8)
      _stream.WriteByte(_curByte);
    _bitPos = 8;
    _curByte = 0;
  }
  Z7_FORCE_INLINE
  void WriteBits(UInt32 value, unsigned numBits)
  {
    while (numBits > 0)
    {
      if (numBits < _bitPos)
      {
        _curByte |= (Byte)((value & ((1 << numBits) - 1)) << (8 - _bitPos));
        _bitPos -= numBits;
        return;
      }
      numBits -= _bitPos;
      _stream.WriteByte((Byte)(_curByte | (value << (8 - _bitPos))));
      value >>= _bitPos;
      _bitPos = 8;
      _curByte = 0;
    }
  }
  void WriteByte(Byte b) { _stream.WriteByte(b);}
};

#endif

/* ---- CPP/7zip/Compress/DeflateEncoder.h ---- */
// DeflateEncoder.h

#ifndef ZIP7_INC_DEFLATE_ENCODER_H
#define ZIP7_INC_DEFLATE_ENCODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NDeflate {
namespace NEncoder {

struct CCodeValue
{
  UInt16 Len;
  UInt16 Pos;
  void SetAsLiteral() { Len = (1 << 15); }
  bool IsLiteral() const { return (Len >= (1 << 15)); }
};

struct COptimal
{
  UInt32 Price;
  UInt16 PosPrev;
  UInt16 BackPrev;
};

const UInt32 kNumOptsBase = 1 << 12;
const UInt32 kNumOpts = kNumOptsBase + kMatchMaxLen;

class CCoder;

struct CTables: public CLevels
{
  bool UseSubBlocks;
  bool StoreMode;
  bool StaticMode;
  UInt32 BlockSizeRes;
  UInt32 m_Pos;
  void InitStructures();
};


struct CEncProps
{
  int Level;
  int algo;
  int fb;
  int btMode;
  UInt32 mc;
  UInt32 numPasses;

  CEncProps()
  {
    Level = -1;
    mc = 0;
    algo = fb = btMode = -1;
    numPasses = (UInt32)(Int32)-1;
  }
  void Normalize();
};

class CCoder
{
  CMatchFinder _lzInWindow;
  CBitlEncoder m_OutStream;

public:
  CCodeValue *m_Values;

  UInt16 *m_MatchDistances;
  UInt32 m_NumFastBytes;
  bool _fastMode;
  bool _btMode;

  UInt16 *m_OnePosMatchesMemory;
  UInt16 *m_DistanceMemory;

  UInt32 m_Pos;

  unsigned m_NumPasses;
  unsigned m_NumDivPasses;
  bool m_CheckStatic;
  bool m_IsMultiPass;
  UInt32 m_ValueBlockSize;

  UInt32 m_NumLenCombinations;
  UInt32 m_MatchMaxLen;
  const Byte *m_LenStart;
  const Byte *m_LenDirectBits;

  bool m_Created;
  bool m_Deflate64Mode;

  Byte m_LevelLevels[kLevelTableSize];
  unsigned m_NumLitLenLevels;
  unsigned m_NumDistLevels;
  UInt32 m_NumLevelCodes;
  UInt32 m_ValueIndex;

  bool m_SecondPass;
  UInt32 m_AdditionalOffset;

  UInt32 m_OptimumEndIndex;
  UInt32 m_OptimumCurrentIndex;
  
  Byte  m_LiteralPrices[256];
  Byte  m_LenPrices[kNumLenSymbolsMax];
  Byte  m_PosPrices[kDistTableSize64];

  CLevels m_NewLevels;
  UInt32 mainFreqs[kFixedMainTableSize];
  UInt32 distFreqs[kDistTableSize64];
  UInt32 mainCodes[kFixedMainTableSize];
  UInt32 distCodes[kDistTableSize64];
  UInt32 levelCodes[kLevelTableSize];
  Byte levelLens[kLevelTableSize];

  UInt32 BlockSizeRes;

  CTables *m_Tables;
  COptimal m_Optimum[kNumOpts];

  UInt32 m_MatchFinderCycles;

  void GetMatches();
  void MovePos(UInt32 num);
  UInt32 Backward(UInt32 &backRes, UInt32 cur);
  UInt32 GetOptimal(UInt32 &backRes);
  UInt32 GetOptimalFast(UInt32 &backRes);

  void LevelTableDummy(const Byte *levels, unsigned numLevels, UInt32 *freqs);

  void WriteBits(UInt32 value, unsigned numBits);
  void LevelTableCode(const Byte *levels, unsigned numLevels, const Byte *lens, const UInt32 *codes);

  void MakeTables(unsigned maxHuffLen);
  UInt32 GetLzBlockPrice() const;
  void TryBlock();
  UInt32 TryDynBlock(unsigned tableIndex, UInt32 numPasses);

  UInt32 TryFixedBlock(unsigned tableIndex);

  void SetPrices(const CLevels &levels);
  void WriteBlock();

  HRESULT Create();
  void Free();

  void WriteStoreBlock(UInt32 blockSize, UInt32 additionalOffset, bool finalBlock);
  void WriteTables(bool writeMode, bool finalBlock);
  
  void WriteBlockData(bool writeMode, bool finalBlock);

  UInt32 GetBlockPrice(unsigned tableIndex, unsigned numDivPasses);
  void CodeBlock(unsigned tableIndex, bool finalBlock);

  void SetProps(const CEncProps *props2);
public:
  CCoder(bool deflate64Mode = false);
  ~CCoder();

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  HRESULT BaseSetEncoderProperties2(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
};


class CCOMCoder Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public CMyUnknownImp,
  public CCoder
{
  Z7_IFACES_IMP_UNK_2(ICompressCoder, ICompressSetCoderProperties)
public:
  CCOMCoder(): CCoder(false) {}
};

class CCOMCoder64 Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public CMyUnknownImp,
  public CCoder
{
  Z7_IFACES_IMP_UNK_2(ICompressCoder, ICompressSetCoderProperties)
public:
  CCOMCoder64(): CCoder(true) {}
};

}}}

#endif

/* ---- CPP/7zip/Compress/ZlibEncoder.h ---- */
// ZlibEncoder.h

#ifndef ZIP7_INC_ZLIB_ENCODER_H
#define ZIP7_INC_ZLIB_ENCODER_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZlib {

Z7_CLASS_IMP_NOQIB_1(
  CInStreamWithAdler
  , ISequentialInStream
)
  CMyComPtr<ISequentialInStream> _stream;
  UInt32 _adler;
  UInt64 _size;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _adler = 1; _size = 0; } // ADLER_INIT_VAL
  UInt32 GetAdler() const { return _adler; }
  UInt64 GetSize() const { return _size; }
};

Z7_CLASS_IMP_NOQIB_1(
  CEncoder
  , ICompressCoder
)
  CInStreamWithAdler *AdlerSpec;
  CMyComPtr<ISequentialInStream> AdlerStream;
  CMyComPtr<ICompressCoder> DeflateEncoder;
public:
  NCompress::NDeflate::NEncoder::CCOMCoder *DeflateEncoderSpec;
  
  void Create();
  UInt64 GetInputProcessedSize() const { return AdlerSpec->GetSize(); }
};

}}

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

/* ---- CPP/7zip/Compress/BitmDecoder.h ---- */
// BitmDecoder.h -- the Most Significant Bit of byte is First

#ifndef ZIP7_INC_BITM_DECODER_H
#define ZIP7_INC_BITM_DECODER_H

// amalgamation: header emitted in prologue

namespace NBitm {

const unsigned kNumBigValueBits = 8 * 4;
const unsigned kNumValueBytes = 3;
const unsigned kNumValueBits = 8 * kNumValueBytes;

const UInt32 kMask = (1 << kNumValueBits) - 1;

// _bitPos - the number of free bits (high bits in _value)
// (kNumBigValueBits - _bitPos) = (32 - _bitPos) == the number of ready to read bits (low bits of _value)

template<class TInByte>
class CDecoder
{
  unsigned _bitPos;
  UInt32 _value;
  TInByte _stream;
public:
  bool Create(UInt32 bufSize) { return _stream.Create(bufSize); }
  void SetStream(ISequentialInStream *inStream) { _stream.SetStream(inStream);}

  void Init()
  {
    _stream.Init();
    _bitPos = kNumBigValueBits;
    _value = 0;
    Normalize();
  }
  
  UInt64 GetStreamSize() const { return _stream.GetStreamSize(); }
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() - ((kNumBigValueBits - _bitPos) >> 3); }

  bool ExtraBitsWereRead() const
  {
    return (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
  }

  bool ExtraBitsWereRead_Fast() const
  {
    return (_stream.NumExtraBytes > 4);
  }
  
  Z7_FORCE_INLINE
  void Normalize()
  {
    for (; _bitPos >= 8; _bitPos -= 8)
      _value = (_value << 8) | _stream.ReadByte();
  }

  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits) const
  {
    // return (_value << _bitPos) >> (kNumBigValueBits - numBits);
    return ((_value >> (8 - _bitPos)) & kMask) >> (kNumValueBits - numBits);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits() const
  {
    return this->_value << this->_bitPos;
  }

  Z7_FORCE_INLINE
  void MovePos(unsigned numBits)
  {
    _bitPos += numBits;
    Normalize();
  }

  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits)
  {
    UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }

  /*
  unsigned ReadBit()
  {
    UInt32 res = ((_value >> (8 - _bitPos)) & kMask) >> (kNumValueBits - 1);
    if (++_bitPos >= 8)
    {
      _value = (_value << 8) | _stream.ReadByte();
      _bitPos -= 8;
    }
    return (unsigned)res;
  }
  */

  void AlignToByte() { MovePos((kNumBigValueBits - _bitPos) & 7); }

  Z7_FORCE_INLINE
  UInt32 ReadAlignBits() { return ReadBits((kNumBigValueBits - _bitPos) & 7); }
};

}

#endif

/* ---- CPP/7zip/Compress/LzhDecoder.h ---- */
// LzhDecoder.h

#ifndef ZIP7_INC_COMPRESS_LZH_DECODER_H
#define ZIP7_INC_COMPRESS_LZH_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzh {
namespace NDecoder {

const unsigned kMatchMinLen = 3;
const unsigned kMatchMaxLen = 256;
const unsigned NC = 256 + kMatchMaxLen - kMatchMinLen + 1;
const unsigned NUM_CODE_BITS = 16;
const unsigned NUM_DIC_BITS_MAX = 25;
const unsigned NT = NUM_CODE_BITS + 3;
const unsigned NP = NUM_DIC_BITS_MAX + 1;
const unsigned NPT = NP; // Max(NT, NP)

class CCoder
{
  CLzOutWindow _outWindow;
  NBitm::CDecoder<CInBuffer> _inBitStream;

  int _symbolT;
  int _symbolC;
  UInt32 DictSize;
  // bool FinishMode;

  NHuffman::CDecoder256<NUM_CODE_BITS, NPT, 7> _decoderT;
  NHuffman::CDecoder<NUM_CODE_BITS, NC, 10> _decoderC;

  class CCoderReleaser
  {
    CCoder *_coder;
  public:
    CCoderReleaser(CCoder *coder): _coder(coder) {}
    void Disable() { _coder = NULL; }
    ~CCoderReleaser() { if (_coder) _coder->_outWindow.Flush(); }
  };
  friend class CCoderReleaser;

  bool ReadTP(unsigned num, unsigned numBits, int spec);
  bool ReadC();

  HRESULT CodeReal(UInt32 outSize, ICompressProgressInfo *progress);
public:
  CCoder(): DictSize(1 << 16)
      // , FinishMode(true)
      {}
  void SetDictSize(UInt32 dictSize) { DictSize = dictSize; }
  UInt64 GetInputProcessedSize() const { return _inBitStream.GetProcessedSize(); }
  HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      UInt32 outSize, ICompressProgressInfo *progress);
};

}}}

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

/* ---- CPP/7zip/Compress/BZip2Const.h ---- */
// Compress/BZip2Const.h

#ifndef ZIP7_INC_COMPRESS_BZIP2_CONST_H
#define ZIP7_INC_COMPRESS_BZIP2_CONST_H

namespace NCompress {
namespace NBZip2 {

const Byte kArSig0 = 'B';
const Byte kArSig1 = 'Z';
const Byte kArSig2 = 'h';
const Byte kArSig3 = '0';

const Byte kFinSig0 = 0x17;
const Byte kFinSig1 = 0x72;
const Byte kFinSig2 = 0x45;
const Byte kFinSig3 = 0x38;
const Byte kFinSig4 = 0x50;
const Byte kFinSig5 = 0x90;

const Byte kBlockSig0 = 0x31;
const Byte kBlockSig1 = 0x41;
const Byte kBlockSig2 = 0x59;
const Byte kBlockSig3 = 0x26;
const Byte kBlockSig4 = 0x53;
const Byte kBlockSig5 = 0x59;

const unsigned kNumOrigBits = 24;

const unsigned kNumTablesBits = 3;
const unsigned kNumTablesMin = 2;
const unsigned kNumTablesMax = 6;

const unsigned kNumLevelsBits = 5;

const unsigned kMaxHuffmanLen = 20; // Check it

const unsigned kMaxAlphaSize = 258;

const unsigned kGroupSize = 50;

const unsigned kBlockSizeMultMin = 1;
const unsigned kBlockSizeMultMax = 9;

const UInt32 kBlockSizeStep = 100000;
const UInt32 kBlockSizeMax = kBlockSizeMultMax * kBlockSizeStep;

const unsigned kNumSelectorsBits = 15;
const unsigned kNumSelectorsMax = 2 + kBlockSizeMax / kGroupSize;

const unsigned kRleModeRepSize = 4;

/*
The number of selectors stored in bzip2 block:
(numSelectors <= 18001) - must work with any decoder.
(numSelectors == 18002) - works with bzip2 1.0.6 decoder and all derived decoders.
(numSelectors  > 18002)
   lbzip2 2.5: encoder can write up to (18001 + 7) selectors.

   7-Zip before 19.03: decoder doesn't support it.
   7-Zip        19.03: decoder allows 8 additional selector records for lbzip2 compatibility.
   
   bzip2 1.0.6: decoder can overflow selector[18002] arrays. But there are another
               arrays after selector arrays. So the compiled code works.
   bzip2 1.0.7: decoder doesn't support it.
   bzip2 1.0.8: decoder allows additional selector records for lbzip2 compatibility.
*/

}}

#endif

/* ---- CPP/7zip/Compress/BZip2Crc.h ---- */
// BZip2Crc.h

#ifndef ZIP7_INC_BZIP2_CRC_H
#define ZIP7_INC_BZIP2_CRC_H

// amalgamation: header emitted in prologue

class CBZip2Crc
{
  UInt32 _value;
  static UInt32 Table[256];
public:
  static void InitTable();
  CBZip2Crc(UInt32 initVal = 0xFFFFFFFF): _value(initVal) {}
  void Init(UInt32 initVal = 0xFFFFFFFF) { _value = initVal; }
  void UpdateByte(Byte b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
  void UpdateByte(unsigned b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
  UInt32 GetDigest() const { return _value ^ 0xFFFFFFFF; }
};

class CBZip2CombinedCrc
{
  UInt32 _value;
public:
  CBZip2CombinedCrc(): _value(0) {}
  void Init() { _value = 0; }
  void Update(UInt32 v) { _value = ((_value << 1) | (_value >> 31)) ^ v; }
  UInt32 GetDigest() const { return _value ; }
};

#endif

/* ---- CPP/7zip/Compress/Mtf8.h ---- */
// Mtf8.h

#ifndef ZIP7_INC_COMPRESS_MTF8_H
#define ZIP7_INC_COMPRESS_MTF8_H

// amalgamation: header emitted in prologue

namespace NCompress {

struct CMtf8Encoder
{
  Byte Buf[256];

  unsigned FindAndMove(Byte v) throw()
  {
#if 1
    Byte b = Buf[0];
    if (v == b)
      return 0;
    Buf[0] = v;
    for (unsigned pos = 0;;)
    {
      Byte a;
      a = Buf[++pos];  Buf[pos] = b;  if (v == a) return pos;
      b = Buf[++pos];  Buf[pos] = a;  if (v == b) return pos;
    }
#else
    size_t pos;
    for (pos = 0; Buf[pos] != v; pos++);
    const unsigned resPos = (unsigned)pos;
    for (; pos >= 8; pos -= 8)
    {
      Buf[pos] = Buf[pos - 1];
      Buf[pos - 1] = Buf[pos - 2];
      Buf[pos - 2] = Buf[pos - 3];
      Buf[pos - 3] = Buf[pos - 4];
      Buf[pos - 4] = Buf[pos - 5];
      Buf[pos - 5] = Buf[pos - 6];
      Buf[pos - 6] = Buf[pos - 7];
      Buf[pos - 7] = Buf[pos - 8];
    }
    for (; pos != 0; pos--)
      Buf[pos] = Buf[pos - 1];
    Buf[0] = v;
    return resPos;
#endif
  }
};

/*
struct CMtf8Decoder
{
  Byte Buf[256];

  void StartInit() { memset(Buf, 0, sizeof(Buf)); }
  void Add(unsigned pos, Byte val) { Buf[pos] = val;  }
  Byte GetHead() const { return Buf[0]; }
  Byte GetAndMove(unsigned pos)
  {
    Byte res = Buf[pos];
    for (; pos >= 8; pos -= 8)
    {
      Buf[pos] = Buf[pos - 1];
      Buf[pos - 1] = Buf[pos - 2];
      Buf[pos - 2] = Buf[pos - 3];
      Buf[pos - 3] = Buf[pos - 4];
      Buf[pos - 4] = Buf[pos - 5];
      Buf[pos - 5] = Buf[pos - 6];
      Buf[pos - 6] = Buf[pos - 7];
      Buf[pos - 7] = Buf[pos - 8];
    }
    for (; pos > 0; pos--)
      Buf[pos] = Buf[pos - 1];
    Buf[0] = res;
    return res;
  }
};
*/

#ifdef MY_CPU_64BIT
  typedef UInt64 CMtfVar;
  #define Z7_MTF_MOVS 3
#else
  typedef UInt32 CMtfVar;
  #define Z7_MTF_MOVS 2
#endif

#define Z7_MTF_MASK ((1 << Z7_MTF_MOVS) - 1)


struct CMtf8Decoder
{
  CMtfVar Buf[256 >> Z7_MTF_MOVS];

  void StartInit() { memset(Buf, 0, sizeof(Buf)); }
  void Add(unsigned pos, Byte val) { Buf[pos >> Z7_MTF_MOVS] |= ((CMtfVar)val << ((pos & Z7_MTF_MASK) << 3));  }
  Byte GetHead() const { return (Byte)Buf[0]; }

  Z7_FORCE_INLINE
  Byte GetAndMove(unsigned pos) throw()
  {
    const UInt32 lim = ((UInt32)pos >> Z7_MTF_MOVS);
    pos = (pos & Z7_MTF_MASK) << 3;
    CMtfVar prev = (Buf[lim] >> pos) & 0xFF;

    UInt32 i = 0;
    

    /*
    if ((lim & 1) != 0)
    {
      CMtfVar next = Buf[0];
      Buf[0] = (next << 8) | prev;
      prev = (next >> (Z7_MTF_MASK << 3));
      i = 1;
      lim -= 1;
    }
    for (; i < lim; i += 2)
    {
      CMtfVar n0 = Buf[i];
      CMtfVar n1 = Buf[i + 1];
      Buf[i    ] = (n0 << 8) | prev;
      Buf[i + 1] = (n1 << 8) | (n0 >> (Z7_MTF_MASK << 3));
      prev = (n1 >> (Z7_MTF_MASK << 3));
    }
    */

    for (; i < lim; i++)
    {
      const CMtfVar n0 = Buf[i];
      Buf[i    ] = (n0 << 8) | prev;
      prev = (n0 >> (Z7_MTF_MASK << 3));
    }


    const CMtfVar next = Buf[i];
    const CMtfVar mask = (((CMtfVar)0x100 << pos) - 1);
    Buf[i] = (next & ~mask) | (((next << 8) | prev) & mask);
    return (Byte)Buf[0];
  }
};

/*
const int kSmallSize = 64;
class CMtf8Decoder
{
  Byte SmallBuffer[kSmallSize];
  int SmallSize;
  int Counts[16];
  int Size;
public:
  Byte Buf[256];

  Byte GetHead() const
  {
    if (SmallSize > 0)
      return SmallBuffer[kSmallSize - SmallSize];
    return Buf[0];
  }

  void Init(int size)
  {
    Size = size;
    SmallSize = 0;
    for (int i = 0; i < 16; i++)
    {
      Counts[i] = ((size >= 16) ? 16 : size);
      size -= Counts[i];
    }
  }

  void Add(unsigned pos, Byte val)
  {
    Buf[pos] = val;
  }

  Byte GetAndMove(int pos)
  {
    if (pos < SmallSize)
    {
      Byte *p = SmallBuffer + kSmallSize - SmallSize;
      Byte res = p[pos];
      for (; pos > 0; pos--)
        p[pos] = p[pos - 1];
      SmallBuffer[kSmallSize - SmallSize] = res;
      return res;
    }
    if (SmallSize == kSmallSize)
    {
      int i = Size - 1;
      int g = 16;
      do
      {
        g--;
        int offset = (g << 4);
        for (int t = Counts[g] - 1; t >= 0; t--, i--)
          Buf[i] = Buf[offset + t];
      }
      while (g != 0);
      
      for (i = kSmallSize - 1; i >= 0; i--)
        Buf[i] = SmallBuffer[i];
      Init(Size);
    }
    pos -= SmallSize;
    int g;
    for (g = 0; pos >= Counts[g]; g++)
      pos -= Counts[g];
    int offset = (g << 4);
    Byte res = Buf[offset + pos];
    for (pos; pos < 16 - 1; pos++)
      Buf[offset + pos] = Buf[offset + pos + 1];
    
    SmallSize++;
    SmallBuffer[kSmallSize - SmallSize] = res;

    Counts[g]--;
    return res;
  }
};
*/

}

#endif

/* ---- CPP/7zip/Compress/BZip2Decoder.h ---- */
// Compress/BZip2Decoder.h

#ifndef ZIP7_INC_COMPRESS_BZIP2_DECODER_H
#define ZIP7_INC_COMPRESS_BZIP2_DECODER_H

// amalgamation: header emitted in prologue

// #define Z7_NO_READ_FROM_CODER
// #define Z7_ST

#ifndef Z7_ST
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBZip2 {

bool IsEndSig(const Byte *p) throw();
bool IsBlockSig(const Byte *p) throw();

const unsigned kNumTableBits = 9;

typedef NHuffman::CDecoder<kMaxHuffmanLen, kMaxAlphaSize, kNumTableBits> CHuffmanDecoder;


struct CBlockProps
{
  UInt32 blockSize;
  UInt32 origPtr;
  unsigned randMode;
  
  CBlockProps(): blockSize(0), origPtr(0), randMode(0) {}
};


struct CBitDecoder
{
  unsigned _numBits;
  UInt32 _value;
  const Byte *_buf;
  const Byte *_lim;

  void InitBitDecoder()
  {
    _numBits = 0;
    _value = 0;
  }

  void AlignToByte()
  {
    unsigned bits = _numBits & 7;
    _numBits -= bits;
    _value <<= bits;
  }

  /*
  bool AreRemainByteBitsEmpty() const
  {
    unsigned bits = _numBits & 7;
    if (bits != 0)
      return (_value >> (32 - bits)) == 0;
    return true;
  }
  */

  SRes ReadByte(int &b);

  CBitDecoder():
    _buf(NULL),
    _lim(NULL)
  {
    InitBitDecoder();
  }
};


// 19.03: we allow additional 8 selectors to support files created by lbzip2.
const UInt32 kNumSelectorsMax_Decoder = kNumSelectorsMax + 8;

struct CBase: public CBitDecoder
{
  unsigned numInUse;
  UInt32 groupIndex;
  UInt32 groupSize;
  unsigned runPower;
  UInt32 runCounter;
  UInt32 blockSize;

  UInt32 *Counters;
  UInt32 blockSizeMax;

  unsigned state;
  UInt32 state2;
  unsigned state3;
  unsigned state4;
  unsigned state5;
  unsigned numTables;
  UInt32 numSelectors;

  CBlockProps Props;

private:
  CMtf8Decoder mtf;
  Byte selectors[kNumSelectorsMax_Decoder];
  CHuffmanDecoder huffs[kNumTablesMax];

  Byte lens[kMaxAlphaSize];

  Byte temp[10];

public:
  UInt32 crc;
  CBZip2CombinedCrc CombinedCrc;
  
  bool IsBz;
  bool StreamCrcError;
  bool MinorError;
  bool NeedMoreInput;

  bool DecodeAllStreams;
  
  UInt64 NumStreams;
  UInt64 NumBlocks;
  UInt64 FinishedPackSize;

  ISequentialInStream *InStream;

  #ifndef Z7_NO_READ_FROM_CODER
  CMyComPtr<ISequentialInStream> InStreamRef;
  #endif

  CBase():
      StreamCrcError(false),
      MinorError(false),
      NeedMoreInput(false),

      DecodeAllStreams(false),
      
      NumStreams(0),
      NumBlocks(0),
      FinishedPackSize(0)
      {}

  void InitNumStreams2()
  {
    StreamCrcError = false;
    MinorError = false;
    NeedMoreInput = 0;
    NumStreams = 0;
    NumBlocks = 0;
    FinishedPackSize = 0;
  }

  SRes ReadStreamSignature2();
  SRes ReadBlockSignature2();

  /* ReadBlock2() : Props->randMode:
       in:  need read randMode bit
       out: randMode status */
  SRes ReadBlock2();
};


class CSpecState
{
  UInt32 _tPos;
  unsigned _prevByte;
  int _reps;

public:
  CBZip2Crc _crc;
  UInt32 _blockSize;
  UInt32 *_tt;

  int _randToGo;
  unsigned _randIndex;

  void Init(UInt32 origPtr, unsigned randMode) throw();

  bool Finished() const { return _reps <= 0 && _blockSize == 0; }

  Byte *Decode(Byte *data, size_t size) throw();
};


  
 
class CDecoder:
  public ICompressCoder,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressReadUnusedFromInBuf,
#ifndef Z7_NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
#endif
#ifndef Z7_ST
  public ICompressSetCoderMt,
#endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressReadUnusedFromInBuf)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
  Z7_COM_QI_ENTRY(ISequentialInStream)
#endif
#ifndef Z7_ST
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
#endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
  Z7_IFACE_COM7_IMP(ICompressReadUnusedFromInBuf)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP_NONFINAL(ISequentialInStream)
#endif
public:
#ifndef Z7_ST
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
#endif

private:
  Byte *_outBuf;
  size_t _outPos;
  UInt64 _outWritten;
  ISequentialOutStream *_outStream;
  HRESULT _writeRes;

protected:
  HRESULT ErrorResult; // for ISequentialInStream::Read mode only

public:

  UInt32 _calcedBlockCrc;
  bool _blockFinished;
  bool BlockCrcError;

  bool FinishMode;
  bool _outSizeDefined;
  UInt64 _outSize;
  UInt64 _outPosTotal;

  CSpecState _spec;
  UInt32 *_counters;

  #ifndef Z7_ST

  struct CBlock
  {
    bool StopScout;
    
    bool WasFinished;
    bool Crc_Defined;
    // bool NextCrc_Defined;
    
    UInt32 Crc;
    UInt32 NextCrc;
    HRESULT Res;
    UInt64 PackPos;
    
    CBlockProps Props;
  };

  CBlock _block;

  bool NeedWaitScout;
  bool MtMode;

  NWindows::CThread Thread;
  NWindows::NSynchronization::CAutoResetEvent DecoderEvent;
  NWindows::NSynchronization::CAutoResetEvent ScoutEvent;
  // HRESULT ScoutRes;
  
  Byte MtPad[1 << 7]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.


  void RunScout();

  void WaitScout()
  {
    if (NeedWaitScout)
    {
      DecoderEvent.Lock();
      NeedWaitScout = false;
    }
  }

  class CWaitScout_Releaser
  {
    CDecoder *_decoder;
  public:
    CWaitScout_Releaser(CDecoder *decoder): _decoder(decoder) {}
    ~CWaitScout_Releaser() { _decoder->WaitScout(); }
  };

  HRESULT CreateThread();

  #endif

  Byte *_inBuf;
  UInt64 _inProcessed;
  bool _inputFinished;
  HRESULT _inputRes;

  CBase Base;

  bool GetCrcError() const { return BlockCrcError || Base.StreamCrcError; }

  void InitOutSize(const UInt64 *outSize);
  
  bool CreateInputBufer();

  void InitInputBuffer()
  {
    // We use InitInputBuffer() before stream init.
    // So don't read from stream here
    _inProcessed = 0;
    Base._buf = _inBuf;
    Base._lim = _inBuf;
    Base.InitBitDecoder();
  }

  UInt64 GetInputProcessedSize() const
  {
    // for NSIS case : we need also look the number of bits in bitDecoder
    return _inProcessed + (size_t)(Base._buf - _inBuf);
  }

  UInt64 GetInStreamSize() const
  {
    return _inProcessed + (size_t)(Base._buf - _inBuf) - (Base._numBits >> 3);
  }

  UInt64 GetOutProcessedSize() const { return _outWritten + _outPos; }

  HRESULT ReadInput();

  void StartNewStream();
  
  HRESULT ReadStreamSignature();
  HRESULT StartRead();

  HRESULT ReadBlockSignature();
  HRESULT ReadBlock();

  HRESULT Flush();
  HRESULT DecodeBlock(const CBlockProps &props);
  HRESULT DecodeStreams(ICompressProgressInfo *progress);

  UInt64 GetNumStreams() const { return Base.NumStreams; }
  UInt64 GetNumBlocks() const { return Base.NumBlocks; }

  CDecoder();
  virtual ~CDecoder();
};



#ifndef Z7_NO_READ_FROM_CODER

class CNsisDecoder Z7_final: public CDecoder
{
  Z7_IFACE_COM7_IMP(ISequentialInStream)
};

#endif

}}

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

/* ---- CPP/7zip/Compress/XzDecoder.h ---- */
// XzDecoder.h

#ifndef ZIP7_INC_XZ_DECODER_H
#define ZIP7_INC_XZ_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NXz {

struct CDecoder
{
  CXzDecMtHandle xz;
  int _tryMt;
  UInt32 _numThreads;
  UInt64 _memUsage;

  SRes MainDecodeSRes; // it's not HRESULT
  bool MainDecodeSRes_wasUsed;
  CXzStatInfo Stat;

  CDecoder():
      xz(NULL),
      _tryMt(True),
      _numThreads(1),
      _memUsage((UInt64)(sizeof(size_t)) << 28),
      MainDecodeSRes(SZ_OK),
      MainDecodeSRes_wasUsed(false)
    {}
  
  ~CDecoder()
  {
    if (xz)
      XzDecMt_Destroy(xz);
  }

  /* Decode() can return S_OK, if there is data after good xz streams, and that data is not new xz stream.
     check also (Stat.DataAfterEnd) flag */

  HRESULT Decode(ISequentialInStream *seqInStream, ISequentialOutStream *outStream,
      const UInt64 *outSizeLimit, bool finishStream, ICompressProgressInfo *compressProgress);
};


class CComDecoder Z7_final:
  public ICompressCoder,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
 #ifndef Z7_ST
  public ICompressSetCoderMt,
  public ICompressSetMemLimit,
 #endif
  public CMyUnknownImp,
  public CDecoder
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
 #ifndef Z7_ST
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
  Z7_COM_QI_ENTRY(ICompressSetMemLimit)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
 #ifndef Z7_ST
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
  Z7_IFACE_COM7_IMP(ICompressSetMemLimit)
 #endif

  bool _finishStream;

public:
  CComDecoder(): _finishStream(false) {}
};

}}

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

/* ---- CPP/7zip/Compress/XzEncoder.h ---- */
// XzEncoder.h

#ifndef ZIP7_INC_XZ_ENCODER_H
#define ZIP7_INC_XZ_ENCODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NXz {

Z7_CLASS_IMP_COM_3(
  CEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
  , ICompressSetCoderPropertiesOpt
)
  CXzEncHandle _encoder;
public:
  CXzProps xzProps;

  void InitCoderProps();
  HRESULT SetCheckSize(UInt32 checkSizeInBytes);
  HRESULT SetCoderProp(PROPID propID, const PROPVARIANT &prop);

  CEncoder();
  ~CEncoder();
};

}}

#endif

/* ---- CPP/7zip/Compress/ZDecoder.h ---- */
// ZDecoder.h

#ifndef ZIP7_INC_COMPRESS_Z_DECODER_H
#define ZIP7_INC_COMPRESS_Z_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZ {

// Z decoder decodes Z data stream, including 3 bytes of header.
  
class CDecoder
{
  UInt16 *_parents;
  Byte *_suffixes;
  Byte *_stack;
  unsigned _numMaxBits;

public:
  CDecoder(): _parents(NULL), _suffixes(NULL), _stack(NULL), /* _prop(0), */ _numMaxBits(0) {}
  ~CDecoder();
  void Free();
  // UInt64 PackSize;

  HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    ICompressProgressInfo *progress);
};

/*
  There is no end_of_payload_marker in Z stream.
  Z decoder stops decoding, if it reaches end of input stream.
   
  CheckStream function:
    (size) must be at least 3 bytes (size of Z header).
    if (size) is larger than size of real Z stream in (data), CheckStream can return false.
*/

const unsigned kRecommendedCheckSize = 64;

bool CheckStream(const Byte *data, size_t size);

}}

#endif

/* ---- CPP/7zip/Compress/ZstdDecoder.h ---- */
// ZstdDecoder.h

#ifndef ZIP7_INC_ZSTD_DECODER_H
#define ZIP7_INC_ZSTD_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZstd {

#ifdef Z7_NO_READ_FROM_CODER
#define Z7_NO_READ_FROM_CODER_ZSTD
#endif

#ifndef Z7_NO_READ_FROM_CODER_ZSTD
// #define Z7_NO_READ_FROM_CODER_ZSTD
#endif

class CDecoder Z7_final:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressReadUnusedFromInBuf,
  public ICompressSetBufSize,
 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetDecoderProperties2)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressReadUnusedFromInBuf)
  Z7_COM_QI_ENTRY(ICompressSetBufSize)
 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
  Z7_COM_QI_ENTRY(ISequentialInStream)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
  Z7_IFACE_COM7_IMP(ICompressReadUnusedFromInBuf)
  Z7_IFACE_COM7_IMP(ICompressSetBufSize)
 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
 #endif

  HRESULT Prepare(const UInt64 *outSize);

  UInt32 _outStepMask;
  CZstdDecHandle _dec;
public:
  UInt64 _inProcessed;
  CZstdDecState _state;

private:
  UInt32 _inBufSize;
  UInt32 _inBufSize_Allocated;
  Byte *_inBuf;
  size_t _afterDecoding_tempPos;

 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  CMyComPtr<ISequentialInStream> _inStream;
  HRESULT _hres_Read;
  HRESULT _hres_Decode;
  UInt64 _writtenSize;
  bool _readWasFinished;
  bool _wasFinished;
 #endif

public:
  bool FinishMode;
  Byte DisableHash;
  CZstdDecResInfo ResInfo;

  HRESULT GetFinishResult();

  CDecoder();
  ~CDecoder();
};

}}

#endif

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/RpmHandler.cpp ================ */
// RpmHandler.cpp

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

// #define Z7_RPM_SHOW_METADATA

using namespace NWindows;

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)

namespace NArchive {
namespace NRpm {

static const unsigned kNameSize = 66;
static const unsigned kLeadSize = kNameSize + 30;
static const unsigned k_HeaderSig_Size = 16;
static const unsigned k_Entry_Size = 16;

#define RPMSIG_NONE         0  // Old signature
#define RPMSIG_PGP262_1024  1  // Old signature
#define RPMSIG_HEADERSIG    5  // New signature

enum
{
  kRpmType_Bin = 0,
  kRpmType_Src = 1
};

// There are two sets of TAGs: signature tags and header tags

// ----- Signature TAGs -----

#define RPMSIGTAG_SIZE 1000 // Header + Payload size (32bit)

// ----- Header TAGs -----

#define RPMTAG_NAME       1000
#define RPMTAG_VERSION    1001
#define RPMTAG_RELEASE    1002
#define RPMTAG_BUILDTIME  1006
#define RPMTAG_OS         1021  // string (old version used int?)
#define RPMTAG_ARCH       1022  // string (old version used int?)
#define RPMTAG_PAYLOADFORMAT      1124
#define RPMTAG_PAYLOADCOMPRESSOR  1125
// #define RPMTAG_PAYLOADFLAGS       1126

enum
{
  k_EntryType_NULL,
  k_EntryType_CHAR,
  k_EntryType_INT8,
  k_EntryType_INT16,
  k_EntryType_INT32,
  k_EntryType_INT64,
  k_EntryType_STRING,
  k_EntryType_BIN,
  k_EntryType_STRING_ARRAY,
  k_EntryType_I18NSTRING
};

static const char * const k_CPUs[] =
{
    "noarch"
  , "i386"
  , "alpha"
  , "sparc"
  , "mips"
  , "ppc"
  , "m68k"
  , "sgi"
  , "rs6000"
  , "ia64"
  , "sparc64"  // 10 ???
  , "mipsel"
  , "arm"
  , "m68kmint"
  , "s390"
  , "s390x"
  , "ppc64"
  , "sh"
  , "xtensa"
  , "aarch64"       // 19
  , "mipsr6"        // 20
  , "mips64r6"      // 21
  , "riscv64"       // 22
  , "loongarch64"   // 23
  // , "24"
  // , "25"
  // , "loongarch64"   // 26  : why 23 and 26 for loongarch64?
  // 255 for some non specified arch
};

static const char * const k_OS[] =
{
    "0"
  , "Linux"
  , "Irix"
  , "solaris"
  , "SunOS"
  , "AmigaOS" // AIX
  , "HP-UX"
  , "osf"
  , "FreeBSD"
  , "SCO_SV"
  , "Irix64"
  , "NextStep"
  , "bsdi"
  , "machten"
  , "cygwin32-NT"
  , "cygwin32-95"
  , "MP_RAS"
  , "MiNT"
  , "OS/390"
  , "VM/ESA"
  , "Linux/390"  // "Linux/ESA"
  , "Darwin" // "MacOSX" 21
};

struct CLead
{
  Byte Major;
  // Byte Minor;
  UInt16 Type;
  UInt16 Cpu;
  UInt16 Os;
  UInt16 SignatureType;
  char Name[kNameSize];
  // char Reserved[16];

  void Parse(const Byte *p)
  {
    Major = p[4];
    // Minor = p[5];
    Type = Get16(p + 6);
    Cpu= Get16(p + 8);
    memcpy(Name, p + 10, kNameSize);
    p += 10 + kNameSize;
    Os = Get16(p);
    SignatureType = Get16(p + 2);
  }

  bool IsSupported() const { return Major >= 3 && Type <= 1; }
};

struct CEntry
{
  UInt32 Tag;
  UInt32 Type;
  UInt32 Offset;
  UInt32 Count;
  
  void Parse(const Byte *p)
  {
    Tag = Get32(p + 0);
    Type = Get32(p + 4);
    Offset = Get32(p + 8);
    Count = Get32(p + 12);
  }
};


#ifdef Z7_RPM_SHOW_METADATA
struct CMetaFile
{
  UInt32 Tag;
  UInt32 Offset;
  UInt32 Size;
};
#endif

Z7_class_CHandler_final: public CHandlerCont
{
  Z7_IFACE_COM7_IMP(IInArchive_Cont)

  UInt64 _headersSize; // is equal to start offset of payload data
  UInt64 _payloadSize;
  UInt64 _size;
    // _size = _payloadSize, if (_payloadSize_Defined)
    // _size = (fileSize - _headersSize), if (!_payloadSize_Defined)
  UInt64 _phySize; // _headersSize + _payloadSize, if (_phySize_Defined)
  UInt32 _headerPlusPayload_Size;
  UInt32 _buildTime;
  
  bool _payloadSize_Defined;
  bool _phySize_Defined;
  bool _headerPlusPayload_Size_Defined;
  bool _time_Defined;

  Byte _payloadSig[6]; // start data of payload

  AString _name;    // p7zip
  AString _version; // 9.20.1
  AString _release; // 8.1.1
  AString _arch;    // x86_64
  AString _os;      // linux
  
  AString _format;      // cpio
  AString _compressor;  // xz, gzip, bzip2, lzma, zstd

  CLead _lead;

  #ifdef Z7_RPM_SHOW_METADATA
  AString _metadata;
  CRecordVector<CMetaFile> _metaFiles;
  #endif

  void SetTime(NCOM::CPropVariant &prop) const
  {
    if (_time_Defined && _buildTime != 0)
      PropVariant_SetFrom_UnixTime(prop, _buildTime);
  }

  void SetStringProp(const AString &s, NCOM::CPropVariant &prop) const
  {
    UString us;
    if (!ConvertUTF8ToUnicode(s, us))
      us = GetUnicodeString(s);
    if (!us.IsEmpty())
      prop = us;
  }

  void AddCPU(AString &s) const;
  AString GetBaseName() const;
  void AddSubFileExtension(AString &res) const;

  HRESULT ReadHeader(ISequentialInStream *stream, bool isMainHeader);
  HRESULT Open2(ISequentialInStream *stream);

  virtual int GetItem_ExtractInfo(UInt32 /* index */, UInt64 &pos, UInt64 &size) const Z7_override
  {
    pos = _headersSize;
    size = _size;
    return NExtract::NOperationResult::kOK;
  }
};

static const Byte kArcProps[] =
{
  kpidHeadersSize,
  kpidCpu,
  kpidHostOS,
  kpidCTime
  #ifdef Z7_RPM_SHOW_METADATA
  , kpidComment
  #endif
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidCTime
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

void CHandler::AddCPU(AString &s) const
{
  if (!_arch.IsEmpty())
    s += _arch;
  else
  {
    if (_lead.Type == kRpmType_Bin)
    {
      if (_lead.Cpu < Z7_ARRAY_SIZE(k_CPUs))
        s += k_CPUs[_lead.Cpu];
      else
        s.Add_UInt32(_lead.Cpu);
    }
  }
}

AString CHandler::GetBaseName() const
{
  AString s;
  if (!_name.IsEmpty())
  {
    s = _name;
    if (!_version.IsEmpty())
    {
      s.Add_Minus();
      s += _version;
    }
    if (!_release.IsEmpty())
    {
      s.Add_Minus();
      s += _release;
    }
  }
  else
    s.SetFrom_CalcLen(_lead.Name, kNameSize);

  s.Add_Dot();
  if (_lead.Type == kRpmType_Src)
    s += "src";
  else
    AddCPU(s);
  return s;
}

void CHandler::AddSubFileExtension(AString &res) const
{
  if (!_format.IsEmpty())
    res += _format;
  else
    res += "cpio";
  res.Add_Dot();
  
  const char *s;
  
  if (!_compressor.IsEmpty())
  {
    s = _compressor;
    if (_compressor.IsEqualTo("bzip2"))
      s = "bz2";
    else if (_compressor.IsEqualTo("gzip"))
      s = "gz";
    else if (_compressor.IsEqualTo("zstd"))
      s = "zst";
  }
  else
  {
    const Byte *p = _payloadSig;
    if (p[0] == 0x1F && p[1] == 0x8B && p[2] == 8)
      s = "gz";
    else if (p[0] == 0xFD && p[1] == '7' && p[2] == 'z' && p[3] == 'X' && p[4] == 'Z' && p[5] == 0)
      s = "xz";
    else if (p[0] == 'B' && p[1] == 'Z' && p[2] == 'h' && p[3] >= '1' && p[3] <= '9')
      s = "bz2";
    else if (p[0] == 0x28 && p[1] == 0xb5 && p[2] == 0x2f && p[3] == 0xfd)
      s = "zst";
    else
      s = "lzma";
  }
  
  res += s;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    
    case kpidHeadersSize: prop = _headersSize; break;
    case kpidPhySize: if (_phySize_Defined) prop = _phySize; break;
    
    case kpidMTime:
    case kpidCTime:
      SetTime(prop);
      break;

    case kpidCpu:
      {
        AString s;
        AddCPU(s);
        /*
        if (_lead.Type == kRpmType_Src)
          s = "src";
        */
        SetStringProp(s, prop);
        break;
      }

    case kpidHostOS:
      {
        if (!_os.IsEmpty())
          SetStringProp(_os, prop);
        else
        {
          TYPE_TO_PROP(k_OS, _lead.Os, prop);
        }
        break;
      }

    #ifdef Z7_RPM_SHOW_METADATA
    // case kpidComment: SetStringProp(_metadata, prop); break;
    #endif

    case kpidName:
    {
      SetStringProp(GetBaseName() + ".rpm", prop);
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  if (index == 0)
  switch (propID)
  {
    case kpidSize:
    case kpidPackSize:
      prop = _size;
      break;

    case kpidMTime:
    case kpidCTime:
      SetTime(prop);
      break;

    case kpidPath:
    {
      AString s (GetBaseName());
      s.Add_Dot();
      AddSubFileExtension(s);
      SetStringProp(s, prop);
      break;
    }

    /*
    case kpidExtension:
    {
      prop = GetSubFileExtension();
      break;
    }
    */
  }
  #ifdef Z7_RPM_SHOW_METADATA
  else
  {
    index--;
    if (index > _metaFiles.Size())
      return E_INVALIDARG;
    const CMetaFile &meta = _metaFiles[index];
    switch (propID)
    {
      case kpidSize:
      case kpidPackSize:
        prop = meta.Size;
        break;
      
      case kpidMTime:
      case kpidCTime:
        SetTime(prop);
        break;
      
      case kpidPath:
      {
        AString s ("[META]");
        s.Add_PathSepar();
        s.Add_UInt32(meta.Tag);
        prop = s;
        break;
      }
    }
  }
  #endif

  prop.Detach(value);
  return S_OK;
}


HRESULT CHandler::ReadHeader(ISequentialInStream *stream, bool isMainHeader)
{
  UInt32 numEntries;
  UInt32 dataLen;
  {
    char buf[k_HeaderSig_Size];
    RINOK(ReadStream_FALSE(stream, buf, k_HeaderSig_Size))
    if (Get32(buf) != 0x8EADE801) // buf[3] = 0x01 - is version
      return S_FALSE;
    // reserved = Get32(buf + 4);
    numEntries = Get32(buf + 8);
    dataLen = Get32(buf + 12);
    if (numEntries >= 1 << 24)
      return S_FALSE;
  }
  size_t indexSize = (size_t)numEntries * k_Entry_Size;
  size_t headerSize = indexSize + dataLen;
  if (headerSize < dataLen)
    return S_FALSE;
  CByteBuffer buffer(headerSize);
  RINOK(ReadStream_FALSE(stream, buffer, headerSize))
  
  for (UInt32 i = 0; i < numEntries; i++)
  {
    CEntry entry;

    entry.Parse(buffer + (size_t)i * k_Entry_Size);
    if (entry.Offset > dataLen)
      return S_FALSE;

    const Byte *p = buffer + indexSize + entry.Offset;
    size_t rem = dataLen - entry.Offset;
    
    if (!isMainHeader)
    {
      if (entry.Tag == RPMSIGTAG_SIZE &&
          entry.Type == k_EntryType_INT32)
      {
        if (rem < 4 || entry.Count != 1)
          return S_FALSE;
        _headerPlusPayload_Size = Get32(p);
        _headerPlusPayload_Size_Defined = true;
      }
    }
    else
    {
      #ifdef Z7_RPM_SHOW_METADATA
      {
        _metadata.Add_UInt32(entry.Tag);
        _metadata += ": ";
      }
      #endif

      if (entry.Type == k_EntryType_STRING)
      {
        if (entry.Count != 1)
          return S_FALSE;
        size_t j;
        for (j = 0; j < rem && p[j] != 0; j++);
        if (j == rem)
          return S_FALSE;
        AString s((const char *)p);
        switch (entry.Tag)
        {
          case RPMTAG_NAME: _name = s; break;
          case RPMTAG_VERSION: _version = s; break;
          case RPMTAG_RELEASE: _release = s; break;
          case RPMTAG_ARCH: _arch = s; break;
          case RPMTAG_OS: _os = s; break;
          case RPMTAG_PAYLOADFORMAT: _format = s; break;
          case RPMTAG_PAYLOADCOMPRESSOR: _compressor = s; break;
        }

        #ifdef Z7_RPM_SHOW_METADATA
        _metadata += s;
        #endif
      }
      else if (entry.Type == k_EntryType_INT32)
      {
        if (rem / 4 < entry.Count)
          return S_FALSE;
        if (entry.Tag == RPMTAG_BUILDTIME)
        {
          if (entry.Count != 1)
            return S_FALSE;
          _buildTime = Get32(p);
          _time_Defined = true;
        }
        
        #ifdef Z7_RPM_SHOW_METADATA
        for (UInt32 t = 0; t < entry.Count; t++)
        {
          if (t != 0)
            _metadata.Add_Space();
          _metadata.Add_UInt32(Get32(p + t * 4));
        }
        #endif
      }

      #ifdef Z7_RPM_SHOW_METADATA

      else if (
          entry.Type == k_EntryType_STRING_ARRAY ||
          entry.Type == k_EntryType_I18NSTRING)
      {
        const Byte *p2 = p;
        size_t rem2 = rem;
        for (UInt32 t = 0; t < entry.Count; t++)
        {
          if (rem2 == 0)
            return S_FALSE;
          if (t != 0)
            _metadata.Add_LF();
          size_t j;
          for (j = 0; j < rem2 && p2[j] != 0; j++);
          if (j == rem2)
            return S_FALSE;
          _metadata += (const char *)p2;
          j++;
          p2 += j;
          rem2 -= j;
        }
      }
      else if (entry.Type == k_EntryType_INT16)
      {
        if (rem / 2 < entry.Count)
          return S_FALSE;
        for (UInt32 t = 0; t < entry.Count; t++)
        {
          if (t != 0)
            _metadata.Add_Space();
          _metadata.Add_UInt32(Get16(p + t * 2));
        }
      }
      else if (entry.Type == k_EntryType_BIN)
      {
        if (rem < entry.Count)
          return S_FALSE;
        for (UInt32 t = 0; t < entry.Count; t++)
        {
          const unsigned b = p[t];
          _metadata += GET_HEX_CHAR_UPPER(b >> 4);
          _metadata += GET_HEX_CHAR_UPPER(b & 0xF);
        }
      }
      else
      {
        // p = p;
      }

      _metadata.Add_LF();
      #endif
    }
    
    #ifdef Z7_RPM_SHOW_METADATA
    CMetaFile meta;
    meta.Offset = entry.Offset;
    meta.Tag = entry.Tag;
    meta.Size = entry.Count;
    _metaFiles.Add(meta);
    #endif
  }

  headerSize += k_HeaderSig_Size;
  _headersSize += headerSize;
  if (isMainHeader && _headerPlusPayload_Size_Defined)
  {
    if (_headerPlusPayload_Size < headerSize)
      return S_FALSE;
    _payloadSize = _headerPlusPayload_Size - headerSize;
    _size = _payloadSize;
    _phySize = _headersSize + _payloadSize;
    _payloadSize_Defined = true;
    _phySize_Defined = true;
  }
  return S_OK;
}

HRESULT CHandler::Open2(ISequentialInStream *stream)
{
  {
    Byte buf[kLeadSize];
    RINOK(ReadStream_FALSE(stream, buf, kLeadSize))
    if (Get32(buf) != 0xEDABEEDB)
      return S_FALSE;
    _lead.Parse(buf);
    if (!_lead.IsSupported())
      return S_FALSE;
  }

  _headersSize = kLeadSize;

  if (_lead.SignatureType == RPMSIG_NONE)
  {

  }
  else if (_lead.SignatureType == RPMSIG_PGP262_1024)
  {
    Byte temp[256];
    RINOK(ReadStream_FALSE(stream, temp, sizeof(temp)))
  }
  else if (_lead.SignatureType == RPMSIG_HEADERSIG)
  {
    RINOK(ReadHeader(stream, false))
    unsigned pos = (unsigned)_headersSize & 7;
    if (pos != 0)
    {
      Byte temp[8];
      unsigned num = 8 - pos;
      RINOK(ReadStream_FALSE(stream, temp, num))
      _headersSize += num;
    }
  }
  else
    return S_FALSE;

  return ReadHeader(stream, true);
}


Z7_COM7F_IMF(CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *))
{
  COM_TRY_BEGIN
  {
    Close();
    RINOK(Open2(inStream))

    // start of payload is allowed to be unaligned
    RINOK(ReadStream_FALSE(inStream, _payloadSig, sizeof(_payloadSig)))

    if (!_payloadSize_Defined)
    {
      UInt64 endPos;
      RINOK(InStream_GetSize_SeekToEnd(inStream, endPos))
      _size = endPos - _headersSize;
    }
    _stream = inStream;
    return S_OK;
  }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _headersSize = 0;
  _payloadSize = 0;
  _size = 0;
  _phySize = 0;
  _headerPlusPayload_Size = 0;

  _payloadSize_Defined = false;
  _phySize_Defined = false;
  _headerPlusPayload_Size_Defined = false;
  _time_Defined = false;

  _name.Empty();
  _version.Empty();
  _release.Empty();
  _arch.Empty();
  _os.Empty();
  
  _format.Empty();
  _compressor.Empty();

  #ifdef Z7_RPM_SHOW_METADATA
  _metadata.Empty();
  _metaFiles.Size();
  #endif

  _stream.Release();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1
  #ifdef Z7_RPM_SHOW_METADATA
    + _metaFiles.Size()
  #endif
  ;

  return S_OK;
}

static const Byte k_Signature[] = { 0xED, 0xAB, 0xEE, 0xDB};

REGISTER_ARC_I(
  "Rpm", "rpm", NULL, 0xEB,
  k_Signature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/SparseHandler.cpp ================ */
// SparseHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

#define G16(_offs_, dest) dest = Get16(p + (_offs_));
#define G32(_offs_, dest) dest = Get32(p + (_offs_));

using namespace NWindows;

namespace NArchive {
namespace NSparse {

// libsparse and simg2img

struct CHeader
{
  // UInt32 magic;          /* 0xed26ff3a */
  // UInt16 major_version;  /* (0x1) - reject images with higher major versions */
  // UInt16 minor_version;  /* (0x0) - allow images with higer minor versions */
  UInt16 file_hdr_sz;    /* 28 bytes for first revision of the file format */
  UInt16 chunk_hdr_sz;   /* 12 bytes for first revision of the file format */
  UInt32 BlockSize;      /* block size in bytes, must be a multiple of 4 (4096) */
  UInt32 NumBlocks;      /* total blocks in the non-sparse output image */
  UInt32 NumChunks;      /* total chunks in the sparse input image */
  // UInt32 image_checksum; /* CRC32 checksum of the original data, counting "don't care" as 0. */

  void Parse(const Byte *p)
  {
    // G16 (4, major_version);
    // G16 (6, minor_version);
    G16 (8, file_hdr_sz)
    G16 (10, chunk_hdr_sz)
    G32 (12, BlockSize)
    G32 (16, NumBlocks)
    G32 (20, NumChunks)
    // G32 (24, image_checksum);
  }
};
 
// #define SPARSE_HEADER_MAGIC 0xed26ff3a

#define CHUNK_TYPE_RAW        0xCAC1
#define CHUNK_TYPE_FILL       0xCAC2
#define CHUNK_TYPE_DONT_CARE  0xCAC3
#define CHUNK_TYPE_CRC32      0xCAC4

#define MY_CHUNK_TYPE_FILL       0
#define MY_CHUNK_TYPE_DONT_CARE  1
#define MY_CHUNK_TYPE_RAW_START 2

static const char * const g_Methods[] =
{
    "RAW"
  , "FILL"
  , "SPARSE" // "DONT_CARE"
  , "CRC32"
};

static const unsigned kFillSize = 4;

struct CChunk
{
  UInt32 VirtBlock;
  Byte Fill [kFillSize];
  UInt64 PhyOffset;

  CChunk()
  {
    Fill[0] =
    Fill[1] =
    Fill[2] =
    Fill[3] =
      0;
  }
};

static const Byte k_Signature[] = { 0x3a, 0xff, 0x26, 0xed, 1, 0 };


Z7_class_CHandler_final: public CHandlerImg
{
  Z7_IFACE_COM7_IMP(IInArchive_Img)

  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)

  CRecordVector<CChunk> Chunks;
  UInt64 _virtSize_fromChunks;
  unsigned _blockSizeLog;
  UInt32 _chunkIndexPrev;

  UInt64 _packSizeProcessed;
  UInt64 _phySize;
  UInt32 _methodFlags;
  bool _isArc;
  bool _headersError;
  bool _unexpectedEnd;
  // bool _unsupported;
  UInt32 NumChunks; // from header

  HRESULT Seek2(UInt64 offset)
  {
    _posInArc = offset;
    return InStream_SeekSet(Stream, offset);
  }

  void InitSeekPositions()
  {
    /* (_virtPos) and (_posInArc) is used only in Read() (that calls ReadPhy()).
       So we must reset these variables before first call of Read() */
    Reset_VirtPos();
    Reset_PosInArc();
    _chunkIndexPrev = 0;
    _packSizeProcessed = 0;
  }

  // virtual functions
  bool Init_PackSizeProcessed() Z7_override
  {
    _packSizeProcessed = 0;
    return true;
  }
  bool Get_PackSizeProcessed(UInt64 &size) Z7_override
  {
    size = _packSizeProcessed;
    return true;
  }

  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openCallback) Z7_override;
  HRESULT ReadPhy(UInt64 offset, void *data, UInt32 size, UInt32 &processed);
};



static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidClusterSize,
  kpidNumBlocks,
  kpidMethod
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidClusterSize: prop = (UInt32)((UInt32)1 << _blockSizeLog); break;
    case kpidNumBlocks: prop = (UInt32)NumChunks; break;
    case kpidPhySize: if (_phySize != 0) prop = _phySize; break;

    case kpidMethod:
    {
      FLAGS_TO_PROP(g_Methods, _methodFlags, prop);
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc)        v |= kpv_ErrorFlags_IsNotArc;
      if (_headersError)  v |= kpv_ErrorFlags_HeadersError;
      if (_unexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
      // if (_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
      if (!Stream && v == 0 && _isArc)
        v = kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidSize: prop = _size; break;
    case kpidPackSize: prop = _phySize; break;
    case kpidExtension: prop = (_imgExt ? _imgExt : "img"); break;
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


static unsigned GetLogSize(UInt32 size)
{
  unsigned k;
  for (k = 0; k < 32; k++)
    if (((UInt32)1 << k) == size)
      return k;
  return k;
}


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openCallback)
{
  const unsigned kHeaderSize = 28;
  const unsigned kChunkHeaderSize = 12;
  CHeader h;
  {
    Byte buf[kHeaderSize];
    RINOK(ReadStream_FALSE(stream, buf, kHeaderSize))
    if (memcmp(buf, k_Signature, 6) != 0)
      return S_FALSE;
    h.Parse(buf);
  }

  if (h.file_hdr_sz != kHeaderSize ||
      h.chunk_hdr_sz != kChunkHeaderSize)
    return S_FALSE;

  NumChunks = h.NumChunks;

  const unsigned logSize = GetLogSize(h.BlockSize);
  if (logSize < 2 || logSize >= 32)
    return S_FALSE;
  _blockSizeLog = logSize;

  _size = (UInt64)h.NumBlocks << logSize;

  if (h.NumChunks >= (UInt32)(Int32)-2) // it's our limit
    return S_FALSE;

  _isArc = true;
  Chunks.Reserve(h.NumChunks + 1);
  UInt64 offset = kHeaderSize;
  UInt32 virtBlock = 0;
  UInt32 i;

  for (i = 0; i < h.NumChunks; i++)
  {
    {
      const UInt32 mask = ((UInt32)1 << 16) - 1;
      if ((i & mask) == mask && openCallback)
      {
        RINOK(openCallback->SetCompleted(NULL, &offset))
      }
    }
    Byte buf[kChunkHeaderSize];
    {
      size_t processed = kChunkHeaderSize;
      RINOK(ReadStream(stream, buf, &processed))
      if (kChunkHeaderSize != processed)
      {
        offset += kChunkHeaderSize;
        break;
      }
    }
    const UInt32 type = Get32(&buf[0]);
    const UInt32 numBlocks = Get32(&buf[4]);
    UInt32 size = Get32(&buf[8]);

    if (type < CHUNK_TYPE_RAW ||
        type > CHUNK_TYPE_CRC32)
      return S_FALSE;
    if (size < kChunkHeaderSize)
      return S_FALSE;
    CChunk c;
    c.PhyOffset = offset + kChunkHeaderSize;
    c.VirtBlock = virtBlock;
    offset += size;
    size -= kChunkHeaderSize;
    _methodFlags |= ((UInt32)1 << (type - CHUNK_TYPE_RAW));

    if (numBlocks > h.NumBlocks - virtBlock)
      return S_FALSE;
    
    if (type == CHUNK_TYPE_CRC32)
    {
      // crc chunk must be last chunk (i == h.NumChunks -1);
      if (size != kFillSize || numBlocks != 0)
        return S_FALSE;
      {
        size_t processed = kFillSize;
        RINOK(ReadStream(stream, c.Fill, &processed))
        if (kFillSize != processed)
          break;
      }
      continue;
    }
    // else
    {
      if (numBlocks == 0)
        return S_FALSE;
      
      if (type == CHUNK_TYPE_DONT_CARE)
      {
        if (size != 0)
          return S_FALSE;
        c.PhyOffset = MY_CHUNK_TYPE_DONT_CARE;
      }
      else if (type == CHUNK_TYPE_FILL)
      {
        if (size != kFillSize)
          return S_FALSE;
        c.PhyOffset = MY_CHUNK_TYPE_FILL;
        size_t processed = kFillSize;
        RINOK(ReadStream(stream, c.Fill, &processed))
        if (kFillSize != processed)
          break;
      }
      else if (type == CHUNK_TYPE_RAW)
      {
        /* Here we require (size == virtSize).
           Probably original decoder also requires it.
           But maybe size of last chunk can be non-aligned with blockSize ? */
        const UInt32 virtSize = (numBlocks << _blockSizeLog);
        if (size != virtSize || numBlocks != (virtSize >> _blockSizeLog))
          return S_FALSE;
      }
      else
        return S_FALSE;

      virtBlock += numBlocks;
      Chunks.AddInReserved(c);
      if (type == CHUNK_TYPE_RAW)
        RINOK(InStream_SeekSet(stream, offset))
    }
  }

  if (i != h.NumChunks)
    _unexpectedEnd = true;
  else if (virtBlock != h.NumBlocks)
    _headersError = true;

  _phySize = offset;

  {
    CChunk c;
    c.VirtBlock = virtBlock;
    c.PhyOffset = offset;
    Chunks.AddInReserved(c);
  }
  _virtSize_fromChunks = (UInt64)virtBlock << _blockSizeLog;

  Stream = stream;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Close())
{
  Chunks.Clear();
  _isArc = false;
  _virtSize_fromChunks = 0;
  // _unsupported = false;
  _headersError = false;
  _unexpectedEnd = false;
  _phySize = 0;
  _methodFlags = 0;

  _chunkIndexPrev = 0;
  _packSizeProcessed = 0;

  // CHandlerImg:
  Clear_HandlerImg_Vars();
  Stream.Release();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  if (Chunks.Size() < 1)
    return S_FALSE;
  if (Chunks.Size() < 2 && _virtSize_fromChunks != 0)
    return S_FALSE;
  // if (_unsupported) return S_FALSE;
  InitSeekPositions();
  CMyComPtr<ISequentialInStream> streamTemp = this;
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}



HRESULT CHandler::ReadPhy(UInt64 offset, void *data, UInt32 size, UInt32 &processed)
{
  processed = 0;
  if (offset > _phySize || offset + size > _phySize)
  {
    // we don't expect these cases, if (_phySize) was set correctly.
    return S_FALSE;
  }
  if (offset != _posInArc)
  {
    const HRESULT res = Seek2(offset);
    if (res != S_OK)
    {
      Reset_PosInArc(); // we don't trust seek_pos in case of error
      return res;
    }
  }
  {
    size_t size2 = size;
    const HRESULT res = ReadStream(Stream, data, &size2);
    processed = (UInt32)size2;
    _packSizeProcessed += size2;
    _posInArc += size2;
    if (res != S_OK)
      Reset_PosInArc(); // we don't trust seek_pos in case of reading error
    return res;
  }
}


Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  // const unsigned kLimit = (1 << 16) + 1; if (size > kLimit) size = kLimit; // for debug
  if (_virtPos >= _virtSize_fromChunks)
    return S_OK;
  {
    const UInt64 rem = _virtSize_fromChunks - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size == 0)
      return S_OK;
  }

  UInt32 chunkIndex = _chunkIndexPrev;
  if (chunkIndex + 1 >= Chunks.Size())
    return S_FALSE;
  {
    const UInt32 blockIndex = (UInt32)(_virtPos >> _blockSizeLog);
    if (blockIndex <  Chunks[chunkIndex    ].VirtBlock ||
        blockIndex >= Chunks[chunkIndex + 1].VirtBlock)
    {
      unsigned left = 0, right = Chunks.Size() - 1;
      for (;;)
      {
        const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
        if (mid == left)
          break;
        if (blockIndex < Chunks[mid].VirtBlock)
          right = mid;
        else
          left = mid;
      }
      chunkIndex = left;
      _chunkIndexPrev = chunkIndex;
    }
  }

  const CChunk &c = Chunks[chunkIndex];
  const UInt64 offset = _virtPos - ((UInt64)c.VirtBlock << _blockSizeLog);
  {
    const UInt32 numBlocks = Chunks[chunkIndex + 1].VirtBlock - c.VirtBlock;
    const UInt64 rem = ((UInt64)numBlocks << _blockSizeLog) - offset;
    if (size > rem)
      size = (UInt32)rem;
  }

  const UInt64 phyOffset = c.PhyOffset;
  
  if (phyOffset >= MY_CHUNK_TYPE_RAW_START)
  {
    UInt32 processed = 0;
    const HRESULT res = ReadPhy(phyOffset + offset, data, size, processed);
    if (processedSize)
      *processedSize = processed;
    _virtPos += processed;
    return res;
  }

  Byte b = 0;
  
  if (phyOffset == MY_CHUNK_TYPE_FILL)
  {
    const Byte b0 = c.Fill [0];
    const Byte b1 = c.Fill [1];
    const Byte b2 = c.Fill [2];
    const Byte b3 = c.Fill [3];
    if (b0 != b1 ||
        b0 != b2 ||
        b0 != b3)
    {
      if (processedSize)
        *processedSize = size;
      _virtPos += size;
      Byte *dest = (Byte *)data;
      while (size >= 4)
      {
        dest[0] = b0;
        dest[1] = b1;
        dest[2] = b2;
        dest[3] = b3;
        dest += 4;
        size -= 4;
      }
      if (size > 0) dest[0] = b0;
      if (size > 1) dest[1] = b1;
      if (size > 2) dest[2] = b2;
      return S_OK;
    }
    b = b0;
  }
  else if (phyOffset != MY_CHUNK_TYPE_DONT_CARE)
    return S_FALSE;
  
  memset(data, b, size);
  _virtPos += size;
  if (processedSize)
    *processedSize = size;
  return S_OK;
}

REGISTER_ARC_I(
  "Sparse", "simg img", NULL, 0xc2,
  k_Signature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/SplitHandler.cpp ================ */
// SplitHandler.cpp

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
namespace NSplit {

static const Byte kProps[] =
{
  kpidPath,
  kpidSize
};

static const Byte kArcProps[] =
{
  kpidNumVolumes,
  kpidTotalPhySize
};


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IInArchiveGetStream
)
  CObjectVector<CMyComPtr<IInStream> > _streams;
  CRecordVector<UInt64> _sizes;
  UString _subName;
  UInt64 _totalSize;

  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidPhySize: if (!_sizes.IsEmpty()) prop = _sizes[0]; break;
    case kpidTotalPhySize: prop = _totalSize; break;
    case kpidNumVolumes: prop = (UInt32)_streams.Size(); break;
    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

struct CSeqName
{
  UString _unchangedPart;
  UString _changedPart;
  bool _splitStyle;
  
  bool GetNextName(UString &s)
  {
    {
      unsigned i = _changedPart.Len();
      for (;;)
      {
        wchar_t c = _changedPart[--i];
        
        if (_splitStyle)
        {
          if (c == 'z')
          {
            _changedPart.ReplaceOneCharAtPos(i, L'a');
            if (i == 0)
              return false;
            continue;
          }
          else if (c == 'Z')
          {
            _changedPart.ReplaceOneCharAtPos(i, L'A');
            if (i == 0)
              return false;
            continue;
          }
        }
        else
        {
          if (c == '9')
          {
            _changedPart.ReplaceOneCharAtPos(i, L'0');
            if (i == 0)
            {
              _changedPart.InsertAtFront(L'1');
              break;
            }
            continue;
          }
        }

        c++;
        _changedPart.ReplaceOneCharAtPos(i, c);
        break;
      }
    }
    
    s = _unchangedPart + _changedPart;
    return true;
  }
};


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  Close();
  if (!callback)
    return S_FALSE;

  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveOpenVolumeCallback,
      volumeCallback, callback)
  if (!volumeCallback)
    return S_FALSE;
  
  UString name;
  {
    NCOM::CPropVariant prop;
    RINOK(volumeCallback->GetProperty(kpidName, &prop))
    if (prop.vt != VT_BSTR)
      return S_FALSE;
    name = prop.bstrVal;
  }
  
  const int dotPos = name.ReverseFind_Dot();
  const UString prefix = name.Left((unsigned)(dotPos + 1));
  const UString ext = name.Ptr((unsigned)(dotPos + 1));
  UString ext2 = ext;
  ext2.MakeLower_Ascii();
  
  CSeqName seqName;
  
  unsigned numLetters = 2;
  bool splitStyle = false;
  
  if (ext2.Len() >= 2 && StringsAreEqual_Ascii(ext2.RightPtr(2), "aa"))
  {
    splitStyle = true;
    while (numLetters < ext2.Len())
    {
      if (ext2[ext2.Len() - numLetters - 1] != 'a')
        break;
      numLetters++;
    }
  }
  else if (ext2.Len() >= 2 && (
         StringsAreEqual_Ascii(ext2.RightPtr(2), "01")
      || StringsAreEqual_Ascii(ext2.RightPtr(2), "00")
      ))
  {
    while (numLetters < ext2.Len())
    {
      if (ext2[ext2.Len() - numLetters - 1] != '0')
        break;
      numLetters++;
    }
    if (numLetters != ext2.Len())
      return S_FALSE;
  }
  else
    return S_FALSE;
  
  seqName._unchangedPart = prefix + ext.Left(ext2.Len() - numLetters);
  seqName._changedPart = ext.RightPtr(numLetters);
  seqName._splitStyle = splitStyle;
  
  if (prefix.Len() < 1)
    _subName = "file";
  else
    _subName.SetFrom(prefix, prefix.Len() - 1);
  
  UInt64 size;
  {
    /*
    NCOM::CPropVariant prop;
    RINOK(volumeCallback->GetProperty(kpidSize, &prop))
    if (prop.vt != VT_UI8)
      return E_INVALIDARG;
    size = prop.uhVal.QuadPart;
    */
  }
  RINOK(InStream_AtBegin_GetSize(stream, size))
  
  _totalSize += size;
  _sizes.Add(size);
  _streams.Add(stream);
  
  {
    const UInt64 numFiles = _streams.Size();
    RINOK(callback->SetCompleted(&numFiles, NULL))
  }
  
  for (;;)
  {
    UString fullName;
    if (!seqName.GetNextName(fullName))
      break;
    CMyComPtr<IInStream> nextStream;
    const HRESULT result = volumeCallback->GetStream(fullName, &nextStream);
    if (result == S_FALSE)
      break;
    if (result != S_OK)
      return result;
    if (!nextStream)
      break;
    RINOK(InStream_AtBegin_GetSize(nextStream, size))
    _totalSize += size;
    _sizes.Add(size);
    _streams.Add(nextStream);
    {
      const UInt64 numFiles = _streams.Size();
      RINOK(callback->SetCompleted(&numFiles, NULL))
    }
  }

  if (_streams.Size() == 1)
  {
    if (splitStyle)
      return S_FALSE;
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  const HRESULT res = Open2(stream, callback);
  if (res != S_OK)
    Close();
  return res;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  _subName.Empty();
  _streams.Clear();
  _sizes.Clear();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _streams.IsEmpty() ? 0 : 1;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPath: prop = _subName; break;
    case kpidSize:
    case kpidPackSize:
      prop = _totalSize;
      break;
    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  UInt64 currentTotalSize = 0;
  RINOK(extractCallback->SetTotal(_totalSize))
  CMyComPtr<ISequentialOutStream> outStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &outStream, askMode))
  if (!testMode && !outStream)
    return S_OK;
  RINOK(extractCallback->PrepareOperation(askMode))
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (unsigned i = 0;; i++)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur())
    if (i == _streams.Size())
      break;
    IInStream *inStream = _streams[i];
    RINOK(InStream_SeekToBegin(inStream))
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress))
    currentTotalSize += copyCoderSpec->TotalSize;
  }
  outStream.Release();
  return extractCallback->SetOperationResult(NExtract::NOperationResult::kOK);
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  if (index != 0)
    return E_INVALIDARG;
  *stream = NULL;
  CMultiStream *streamSpec = new CMultiStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  FOR_VECTOR (i, _streams)
  {
    CMultiStream::CSubStreamInfo subStreamInfo;
    subStreamInfo.Stream = _streams[i];
    subStreamInfo.Size = _sizes[i];
    streamSpec->Streams.Add(subStreamInfo);
  }
  streamSpec->Init();
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

REGISTER_ARC_I_NO_SIG(
  "Split", "001", NULL, 0xEA,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/SquashfsHandler.cpp ================ */
// SquashfsHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#include "lz4.h"

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
// #include "../Compress/LzmaDecoder.h"

namespace NArchive {
namespace NSquashfs {

static const UInt32 kNumFilesMax = 1 << 28;
static const unsigned kNumDirLevelsMax = 1 << 10;

// Layout: Header, Data, inodes, Directories, Fragments, UIDs, GIDs

/*
#define Get16(p) (be ? GetBe16(p) : GetUi16(p))
#define Get32(p) (be ? GetBe32(p) : GetUi32(p))
#define Get64(p) (be ? GetBe64(p) : GetUi64(p))
*/

static UInt16 Get16b(const Byte *p, bool be) { return be ? GetBe16(p) : GetUi16(p); }
static UInt32 Get32b(const Byte *p, bool be) { return be ? GetBe32(p) : GetUi32(p); }
static UInt64 Get64b(const Byte *p, bool be) { return be ? GetBe64(p) : GetUi64(p); }

#define Get16(p) Get16b(p, be)
#define Get32(p) Get32b(p, be)
#define Get64(p) Get64b(p, be)

#define LE_16(offs, dest) dest = GetUi16(p + (offs))
#define LE_32(offs, dest) dest = GetUi32(p + (offs))
#define LE_64(offs, dest) dest = GetUi64(p + (offs))

#define GET_16(offs, dest) dest = Get16(p + (offs))
#define GET_32(offs, dest) dest = Get32(p + (offs))
#define GET_64(offs, dest) dest = Get64(p + (offs))

static const UInt32 kSignature32_LE = 0x73717368;
static const UInt32 kSignature32_BE = 0x68737173;
static const UInt32 kSignature32_LZ = 0x71736873;
static const UInt32 kSignature32_B2 = 0x73687371;

#define kMethod_ZLIB 1
#define kMethod_LZMA 2
#define kMethod_LZO  3
#define kMethod_XZ   4
#define kMethod_LZ4  5
#define kMethod_ZSTD 6

static const char * const k_Methods[] =
{
    "0"
  , "ZLIB"
  , "LZMA"
  , "LZO"
  , "XZ"
  , "LZ4"
  , "ZSTD"
};

static const unsigned kMetadataBlockSizeLog = 13;
static const UInt32 kMetadataBlockSize = (1 << kMetadataBlockSizeLog);

enum
{
  kType_IPC,
  kType_DIR,
  kType_FILE,
  kType_LNK,
  kType_BLK,
  kType_CHR,
  kType_FIFO,
  kType_SOCK
};

static const UInt32 k_TypeToMode[] =
{
  0,
  MY_LIN_S_IFDIR, MY_LIN_S_IFREG, MY_LIN_S_IFLNK, MY_LIN_S_IFBLK, MY_LIN_S_IFCHR, MY_LIN_S_IFIFO, MY_LIN_S_IFSOCK,
  MY_LIN_S_IFDIR, MY_LIN_S_IFREG, MY_LIN_S_IFLNK, MY_LIN_S_IFBLK, MY_LIN_S_IFCHR, MY_LIN_S_IFIFO, MY_LIN_S_IFSOCK
};


enum
{
  kFlag_UNC_INODES,
  kFlag_UNC_DATA,
  kFlag_CHECK,
  kFlag_UNC_FRAGS,
  kFlag_NO_FRAGS,
  kFlag_ALWAYS_FRAG,
  kFlag_DUPLICATE,
  kFlag_EXPORT
};

static const char * const k_Flags[] =
{
    "UNCOMPRESSED_INODES"
  , "UNCOMPRESSED_DATA"
  , "CHECK"
  , "UNCOMPRESSED_FRAGMENTS"
  , "NO_FRAGMENTS"
  , "ALWAYS_FRAGMENTS"
  , "DUPLICATES_REMOVED"
  , "EXPORTABLE"
  , "UNCOMPRESSED_XATTRS"
  , "NO_XATTRS"
  , "COMPRESSOR_OPTIONS"
  , "UNCOMPRESSED_IDS"
};

static const UInt32 kNotCompressedBit16 = 1 << 15;
static const UInt32 kNotCompressedBit32 = 1 << 24;

#define GET_COMPRESSED_BLOCK_SIZE(size) ((size) & ~kNotCompressedBit32)
#define IS_COMPRESSED_BLOCK(size) (((size) & kNotCompressedBit32) == 0)

// static const UInt32 kHeaderSize1 = 0x33;
// static const UInt32 kHeaderSize2 = 0x3F;
static const UInt32 kHeaderSize3 = 0x77;
// static const UInt32 kHeaderSize4 = 0x60;

struct CHeader
{
  bool be;
  bool SeveralMethods;
  Byte NumUids;
  Byte NumGids;

  UInt32 NumInodes;
  UInt32 CTime;
  UInt32 BlockSize;
  UInt32 NumFrags;
  UInt16 Method;
  UInt16 BlockSizeLog;
  UInt16 Flags;
  UInt16 NumIDs;
  UInt16 Major;
  UInt16 Minor;
  UInt64 RootInode;
  UInt64 Size;
  UInt64 UidTable;
  UInt64 GidTable;
  UInt64 XattrIdTable;
  UInt64 InodeTable;
  UInt64 DirTable;
  UInt64 FragTable;
  UInt64 LookupTable;

  void Parse3(const Byte *p)
  {
    Method = kMethod_ZLIB;
    GET_32 (0x08, Size);
    GET_32 (0x0C, UidTable);
    GET_32 (0x10, GidTable);
    GET_32 (0x14, InodeTable);
    GET_32 (0x18, DirTable);
    GET_16 (0x20, BlockSize);
    GET_16 (0x22, BlockSizeLog);
    Flags   = p[0x24];
    NumUids = p[0x25];
    NumGids = p[0x26];
    GET_32 (0x27, CTime);
    GET_64 (0x2B, RootInode);
    NumFrags = 0;
    FragTable = UidTable;

    if (Major >= 2)
    {
      GET_32 (0x33, BlockSize);
      GET_32 (0x37, NumFrags);
      GET_32 (0x3B, FragTable);
      if (Major == 3)
      {
        GET_64 (0x3F, Size);
        GET_64 (0x47, UidTable);
        GET_64 (0x4F, GidTable);
        GET_64 (0x57, InodeTable);
        GET_64 (0x5F, DirTable);
        GET_64 (0x67, FragTable);
        GET_64 (0x6F, LookupTable);
      }
    }
  }

  void Parse4(const Byte *p)
  {
    LE_32 (0x08, CTime);
    LE_32 (0x0C, BlockSize);
    LE_32 (0x10, NumFrags);
    LE_16 (0x14, Method);
    LE_16 (0x16, BlockSizeLog);
    LE_16 (0x18, Flags);
    LE_16 (0x1A, NumIDs);
    LE_64 (0x20, RootInode);
    LE_64 (0x28, Size);
    LE_64 (0x30, UidTable);
    LE_64 (0x38, XattrIdTable);
    LE_64 (0x40, InodeTable);
    LE_64 (0x48, DirTable);
    LE_64 (0x50, FragTable);
    LE_64 (0x58, LookupTable);
    GidTable = 0;
  }

  bool Parse(const Byte *p)
  {
    be = false;
    SeveralMethods = false;
    switch (GetUi32(p))
    {
      case kSignature32_LE: break;
      case kSignature32_BE: be = true; break;
      case kSignature32_LZ: SeveralMethods = true; break;
      case kSignature32_B2: SeveralMethods = true; be = true; break;
      default: return false;
    }
    GET_32 (4, NumInodes);
    GET_16 (0x1C, Major);
    GET_16 (0x1E, Minor);
    if (Major <= 3)
      Parse3(p);
    else
    {
      if (be)
        return false;
      Parse4(p);
    }
    return
      InodeTable < DirTable &&
      DirTable <= FragTable &&
      FragTable <= Size &&
      UidTable <= Size &&
      BlockSizeLog >= 12 &&
      BlockSizeLog < 31 &&
      BlockSize == ((UInt32)1 << BlockSizeLog);
  }

  bool IsSupported() const { return Major > 0 && Major <= 4 && BlockSizeLog <= 23; }
  bool IsOldVersion() const { return Major < 4; }
  bool NeedCheckData() const { return (Flags & (1 << kFlag_CHECK)) != 0; }
  unsigned GetFileNameOffset() const { return Major <= 2 ? 3 : (Major == 3 ? 5 : 8); }
  unsigned GetSymLinkOffset() const { return Major <= 1 ? 5: (Major <= 2 ? 6: (Major == 3 ? 18 : 24)); }
  unsigned GetSpecGuidIndex() const { return Major <= 1 ? 0xF: 0xFF; }
};

static const UInt32 kFrag_Empty = (UInt32)(Int32)-1;
// static const UInt32 kXattr_Empty = (UInt32)(Int32)-1;

struct CNode
{
  UInt16 Type;
  UInt16 Mode;
  UInt16 Uid;
  UInt16 Gid;
  UInt32 Frag;
  UInt32 Offset;
  // UInt32 MTime;
  // UInt32 Number;
  // UInt32 NumLinks;
  // UInt32 RDev;
  // UInt32 Xattr;
  // UInt32 Parent;
  
  UInt64 FileSize;
  UInt64 StartBlock;
  // UInt64 Sparse;
  
  UInt32 Parse1(const Byte *p, UInt32 size, const CHeader &_h);
  UInt32 Parse2(const Byte *p, UInt32 size, const CHeader &_h);
  UInt32 Parse3(const Byte *p, UInt32 size, const CHeader &_h);
  UInt32 Parse4(const Byte *p, UInt32 size, const CHeader &_h);
  
  bool IsDir() const { return (Type == kType_DIR || Type == kType_DIR + 7); }
  bool IsLink() const { return (Type == kType_LNK || Type == kType_LNK + 7); }
  UInt64 GetSize() const { return IsDir() ? 0 : FileSize; }
  
  bool ThereAreFrags() const { return Frag != kFrag_Empty; }
  UInt64 GetNumBlocks(const CHeader &_h) const
  {
    return (FileSize >> _h.BlockSizeLog) +
      (!ThereAreFrags() && (FileSize & (_h.BlockSize - 1)) != 0);
  }
};

UInt32 CNode::Parse1(const Byte *p, UInt32 size, const CHeader &_h)
{
  const bool be = _h.be;
  if (size < 4)
    return 0;
  {
    const UInt32 t = Get16(p);
    if (be)
    {
      Type = (UInt16)(t >> 12);
      Mode = (UInt16)(t & 0xFFF);
      Uid = (UInt16)(p[2] >> 4);
      Gid = (UInt16)(p[2] & 0xF);
    }
    else
    {
      Type = (UInt16)(t & 0xF);
      Mode = (UInt16)(t >> 4);
      Uid = (UInt16)(p[2] & 0xF);
      Gid = (UInt16)(p[2] >> 4);
    }
  }

  // Xattr = kXattr_Empty;
  // MTime = 0;
  FileSize = 0;
  StartBlock = 0;
  Frag = kFrag_Empty;

  if (Type == 0)
  {
    Byte t = p[3];
    if (be)
    {
      Type = (UInt16)(t >> 4);
      Offset = (UInt16)(t & 0xF);
    }
    else
    {
      Type = (UInt16)(t & 0xF);
      Offset = (UInt16)(t >> 4);
    }
    return (Type == kType_FIFO || Type == kType_SOCK) ? 4 : 0;
  }

  Type--;
  Uid = (UInt16)(Uid + (Type / 5) * 16);
  Type = (UInt16)((Type % 5) + 1);

  if (Type == kType_FILE)
  {
    if (size < 15)
      return 0;
    // GET_32 (3, MTime);
    GET_32 (7, StartBlock);
    UInt32 t;
    GET_32 (11, t);
    FileSize = t;
    UInt32 numBlocks = t >> _h.BlockSizeLog;
    if ((t & (_h.BlockSize - 1)) != 0)
      numBlocks++;
    UInt32 pos = numBlocks * 2 + 15;
    return (pos <= size) ? pos : 0;
  }
  
  if (Type == kType_DIR)
  {
    if (size < 14)
      return 0;
    UInt32 t = Get32(p + 3);
    if (be)
    {
      FileSize = t >> 13;
      Offset = t & 0x1FFF;
    }
    else
    {
      FileSize = t & 0x7FFFF;
      Offset = t >> 19;
    }
    // GET_32 (7, MTime);
    GET_32 (10, StartBlock);
    if (be)
      StartBlock &= 0xFFFFFF;
    else
      StartBlock >>= 8;
    return 14;
  }
  
  if (size < 5)
    return 0;

  if (Type == kType_LNK)
  {
    UInt32 len;
    GET_16 (3, len);
    FileSize = len;
    len += 5;
    return (len <= size) ? len : 0;
  }

  // GET_32 (3, RDev);
  return 5;
}

UInt32 CNode::Parse2(const Byte *p, UInt32 size, const CHeader &_h)
{
  const bool be = _h.be;
  if (size < 4)
    return 0;
  {
    const UInt32 t = Get16(p);
    if (be)
    {
      Type = (UInt16)(t >> 12);
      Mode = (UInt16)(t & 0xFFF);
    }
    else
    {
      Type = (UInt16)(t & 0xF);
      Mode = (UInt16)(t >> 4);
    }
  }

  Uid = p[2];
  Gid = p[3];

  // Xattr = kXattr_Empty;

  if (Type == kType_FILE)
  {
    if (size < 24)
      return 0;
    // GET_32 (4, MTime);
    GET_32 (8, StartBlock);
    GET_32 (12, Frag);
    GET_32 (16, Offset);
    UInt32 t;
    GET_32 (20, t);
    FileSize = t;
    UInt32 numBlocks = t >> _h.BlockSizeLog;
    if (!ThereAreFrags() && (t & (_h.BlockSize - 1)) != 0)
      numBlocks++;
    UInt32 pos = numBlocks * 4 + 24;
    return (pos <= size) ? (UInt32)pos : 0;
  }

  FileSize = 0;
  // MTime = 0;
  StartBlock = 0;
  Frag = kFrag_Empty;
  
  if (Type == kType_DIR)
  {
    if (size < 15)
      return 0;
    UInt32 t = Get32(p + 4);
    if (be)
    {
      FileSize = t >> 13;
      Offset = t & 0x1FFF;
    }
    else
    {
      FileSize = t & 0x7FFFF;
      Offset = t >> 19;
    }
    // GET_32 (8, MTime);
    GET_32 (11, StartBlock);
    if (be)
      StartBlock &= 0xFFFFFF;
    else
      StartBlock >>= 8;
    return 15;
  }
  
  if (Type == kType_DIR + 7)
  {
    if (size < 18)
      return 0;
    UInt32 t = Get32(p + 4);
    UInt32 t2 = Get16(p + 7);
    if (be)
    {
      FileSize = t >> 5;
      Offset = t2 & 0x1FFF;
    }
    else
    {
      FileSize = t & 0x7FFFFFF;
      Offset = t2 >> 3;
    }
    // GET_32 (9, MTime);
    GET_32 (12, StartBlock);
    if (be)
      StartBlock &= 0xFFFFFF;
    else
      StartBlock >>= 8;
    UInt32 iCount;
    GET_16 (16, iCount);
    UInt32 pos = 18;
    for (UInt32 i = 0; i < iCount; i++)
    {
      // 27 bits: index
      // 29 bits: startBlock
      if (pos + 8 > size)
        return 0;
      pos += 8 + (UInt32)p[pos + 7] + 1; // nameSize
      if (pos > size)
        return 0;
    }
    return pos;
  }

  if (Type == kType_FIFO || Type == kType_SOCK)
    return 4;

  if (size < 6)
    return 0;
  
  if (Type == kType_LNK)
  {
    UInt32 len;
    GET_16 (4, len);
    FileSize = len;
    len += 6;
    return (len <= size) ? len : 0;
  }
  
  if (Type == kType_BLK || Type == kType_CHR)
  {
    // GET_16 (4, RDev);
    return 6;
  }
  
  return 0;
}

UInt32 CNode::Parse3(const Byte *p, UInt32 size, const CHeader &_h)
{
  const bool be = _h.be;
  if (size < 12)
    return 0;
  
  {
    const UInt32 t = Get16(p);
    if (be)
    {
      Type = (UInt16)(t >> 12);
      Mode = (UInt16)(t & 0xFFF);
    }
    else
    {
      Type = (UInt16)(t & 0xF);
      Mode = (UInt16)(t >> 4);
    }
  }

  Uid = p[2];
  Gid = p[3];
  // GET_32 (4, MTime);
  // GET_32 (8, Number);
  // Xattr = kXattr_Empty;
  FileSize = 0;
  StartBlock = 0;
  
  if (Type == kType_FILE || Type == kType_FILE + 7)
  {
    UInt32 offset;
    if (Type == kType_FILE)
    {
      if (size < 32)
        return 0;
      GET_64 (12, StartBlock);
      GET_32 (20, Frag);
      GET_32 (24, Offset);
      GET_32 (28, FileSize);
      offset = 32;
    }
    else
    {
      if (size < 40)
        return 0;
      // GET_32 (12, NumLinks);
      GET_64 (16, StartBlock);
      GET_32 (24, Frag);
      GET_32 (28, Offset);
      GET_64 (32, FileSize);
      offset = 40;
    }
    UInt64 pos = GetNumBlocks(_h) * 4 + offset;
    return (pos <= size) ? (UInt32)pos : 0;
  }

  if (size < 16)
    return 0;
  // GET_32 (12, NumLinks);
 
  if (Type == kType_DIR)
  {
    if (size < 28)
      return 0;
    UInt32 t = Get32(p + 16);
    if (be)
    {
      FileSize = t >> 13;
      Offset = t & 0x1FFF;
    }
    else
    {
      FileSize = t & 0x7FFFF;
      Offset = t >> 19;
    }
    GET_32 (20, StartBlock);
    // GET_32 (24, Parent);
    return 28;
  }
  
  if (Type == kType_DIR + 7)
  {
    if (size < 31)
      return 0;
    UInt32 t = Get32(p + 16);
    UInt32 t2 = Get16(p + 19);
    if (be)
    {
      FileSize = t >> 5;
      Offset = t2 & 0x1FFF;
    }
    else
    {
      FileSize = t & 0x7FFFFFF;
      Offset = t2 >> 3;
    }
    GET_32 (21, StartBlock);
    UInt32 iCount;
    GET_16 (25, iCount);
    // GET_32 (27, Parent);
    UInt32 pos = 31;
    for (UInt32 i = 0; i < iCount; i++)
    {
      // UInt32 index
      // UInt32 startBlock
      if (pos + 9 > size)
        return 0;
      pos += 9 + (unsigned)p[pos + 8] + 1; // nameSize
      if (pos > size)
        return 0;
    }
    return pos;
  }

  if (Type == kType_FIFO || Type == kType_SOCK)
    return 16;
  
  if (size < 18)
    return 0;
  if (Type == kType_LNK)
  {
    UInt32 len;
    GET_16 (16, len);
    FileSize = len;
    len += 18;
    return (len <= size) ? len : 0;
  }

  if (Type == kType_BLK || Type == kType_CHR)
  {
    // GET_16 (16, RDev);
    return 18;
  }
  
  return 0;
}

UInt32 CNode::Parse4(const Byte *p, UInt32 size, const CHeader &_h)
{
  if (size < 20)
    return 0;
  LE_16 (0, Type);
  LE_16 (2, Mode);
  LE_16 (4, Uid);
  LE_16 (6, Gid);
  // LE_32 (8, MTime);
  // LE_32 (12, Number);
  
  // Xattr = kXattr_Empty;
  FileSize = 0;
  StartBlock = 0;
  
  if (Type == kType_FILE || Type == kType_FILE + 7)
  {
    UInt32 offset;
    if (Type == kType_FILE)
    {
      if (size < 32)
        return 0;
      LE_32 (16, StartBlock);
      LE_32 (20, Frag);
      LE_32 (24, Offset);
      LE_32 (28, FileSize);
      offset = 32;
    }
    else
    {
      if (size < 56)
        return 0;
      LE_64 (16, StartBlock);
      LE_64 (24, FileSize);
      // LE_64 (32, Sparse);
      // LE_32 (40, NumLinks);
      LE_32 (44, Frag);
      LE_32 (48, Offset);
      // LE_32 (52, Xattr);
      offset = 56;
    }
    UInt64 pos = GetNumBlocks(_h) * 4 + offset;
    return (pos <= size) ? (UInt32)pos : 0;
  }
  
  if (Type == kType_DIR)
  {
    if (size < 32)
      return 0;
    LE_32 (16, StartBlock);
    // LE_32 (20, NumLinks);
    LE_16 (24, FileSize);
    LE_16 (26, Offset);
    // LE_32 (28, Parent);
    return 32;
  }
  
  // LE_32 (16, NumLinks);

  if (Type == kType_DIR + 7)
  {
    if (size < 40)
      return 0;
    LE_32 (20, FileSize);
    LE_32 (24, StartBlock);
    // LE_32 (28, Parent);
    UInt32 iCount;
    LE_16 (32, iCount);
    LE_16 (34, Offset);
    // LE_32 (36, Xattr);

    UInt32 pos = 40;
    for (UInt32 i = 0; i < iCount; i++)
    {
      // UInt32 index
      // UInt32 startBlock
      if (pos + 12 > size)
        return 0;
      UInt32 nameLen = GetUi32(p + pos + 8);
      pos += 12 + nameLen + 1;
      if (pos > size || nameLen > (1 << 10))
        return 0;
    }
    return pos;
  }
  
  unsigned offset = 20;
  switch (Type)
  {
    case kType_FIFO: case kType_FIFO + 7:
    case kType_SOCK: case kType_SOCK + 7:
      break;
    case kType_LNK: case kType_LNK + 7:
    {
      if (size < 24)
        return 0;
      UInt32 len;
      LE_32 (20, len);
      FileSize = len;
      offset = len + 24;
      if (size < offset || len > (1 << 30))
        return 0;
      break;
    }
    case kType_BLK: case kType_BLK + 7:
    case kType_CHR: case kType_CHR + 7:
      if (size < 24)
        return 0;
      // LE_32 (20, RDev);
      offset = 24;
      break;
    default:
      return 0;
  }
  
  if (Type >= 8)
  {
    if (size < offset + 4)
      return 0;
    // LE_32 (offset, Xattr);
    offset += 4;
  }
  return offset;
}

struct CItem
{
  int Node;
  int Parent;
  UInt32 Ptr;

  CItem(): Node(-1), Parent(-1), Ptr(0) {}
};

struct CData
{
  CByteBuffer Data;
  CRecordVector<UInt32> PackPos;
  CRecordVector<UInt32> UnpackPos;
  
  UInt32 GetNumBlocks() const { return PackPos.Size(); }
  void Clear()
  {
    Data.Free();
    PackPos.Clear();
    UnpackPos.Clear();
  }
};

struct CFrag
{
  UInt64 StartBlock;
  UInt32 Size;
};


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IInArchiveGetStream
)
  bool _noPropsLZMA;
  bool _needCheckLzma;

  CRecordVector<CItem> _items;
  CRecordVector<CNode> _nodes;
  CRecordVector<UInt32> _nodesPos;
  CData _inodesData;
  CData _dirs;
  CRecordVector<CFrag> _frags;
  CByteBuffer _uids;
  CByteBuffer _gids;
  CHeader _h;
  
  UInt64 _sizeCalculated;
  CMyComPtr<IInStream> _stream;

  IArchiveOpenCallback *_openCallback;

  UInt32 _openCodePage;
  int _nodeIndex;
  CRecordVector<bool> _blockCompressed;
  CRecordVector<UInt64> _blockOffsets;
  
  CByteBuffer _cachedBlock;
  UInt64 _cachedBlockStartPos;
  UInt32 _cachedPackBlockSize;
  UInt32 _cachedUnpackBlockSize;

  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> _limitedInStream;
  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> _outStream;
  CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> _dynOutStream;

  // CMyComPtr2<ICompressCoder, NCompress::NLzma::CDecoder> _lzmaDecoder;
  CMyComPtr2<ICompressCoder, NCompress::NZlib::CDecoder> _zlibDecoder;
  
  CXzUnpacker _xz;
  CZstdDecHandle _zstd;

  CByteBuffer _inputBuffer;

  void ClearCache()
  {
    _cachedBlockStartPos = 0;
    _cachedPackBlockSize = 0;
    _cachedUnpackBlockSize = 0;
  }

  HRESULT Seek2(UInt64 offset)
  {
    return InStream_SeekSet(_stream, offset);
  }

  HRESULT Decompress(ISequentialOutStream *outStream, Byte *outBuf, bool *outBufWasWritten, UInt32 *outBufWasWrittenSize,
      UInt32 inSize, UInt32 outSizeMax);
  HRESULT ReadMetadataBlock(UInt32 &packSize);
  HRESULT ReadMetadataBlock2();
  HRESULT ReadData(CData &data, UInt64 start, UInt64 end);

  HRESULT OpenDir(int parent, UInt32 startBlock, UInt32 offset, unsigned level, int &nodeIndex);
  HRESULT ScanInodes(UInt64 ptr);
  HRESULT ReadUids(UInt64 start, UInt32 num, CByteBuffer &ids);
  HRESULT Open2(IInStream *inStream);
  AString GetPath(unsigned index) const;
  bool GetPackSize(unsigned index, UInt64 &res, bool fillOffsets);

public:
  CHandler();
  ~CHandler()
  {
    XzUnpacker_Free(&_xz);
    if (_zstd)
      ZstdDec_Destroy(_zstd);
  }

  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize);
};


CHandler::CHandler():
    _zstd(NULL)
{
  XzUnpacker_Construct(&_xz, &g_Alloc);
}

static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidPosixAttrib,
  kpidUserId,
  kpidGroupId
  // kpidLinks,
  // kpidOffset
};

static const Byte kArcProps[] =
{
  kpidHeadersSize,
  kpidFileSystem,
  kpidMethod,
  kpidClusterSize,
  kpidBigEndian,
  kpidCTime,
  kpidCharacts,
  kpidCodePage
  // kpidNumBlocks
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static HRESULT LzoDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen)
{
  SizeT destRem = *destLen;
  SizeT srcRem = *srcLen;
  *destLen = 0;
  *srcLen = 0;
  const Byte *destStart = dest;
  const Byte *srcStart = src;
  unsigned mode = 0;

  {
    if (srcRem == 0)
      return S_FALSE;
    UInt32 b = *src;
    if (b > 17)
    {
      src++;
      srcRem--;
      b -= 17;
      mode = (b < 4 ? 1 : 4);
      if (b > srcRem || b > destRem)
        return S_FALSE;
      srcRem -= b;
      destRem -= b;
      do
        *dest++ = *src++;
      while (--b);
    }
  }

  for (;;)
  {
    if (srcRem < 3)
      return S_FALSE;
    UInt32 b = *src++;
    srcRem--;
    UInt32 len, back;
    
    if (b >= 64)
    {
      srcRem--;
      back = ((b >> 2) & 7) + ((UInt32)*src++ << 3);
      len = (b >> 5) + 1;
    }
    else if (b < 16)
    {
      if (mode == 0)
      {
        if (b == 0)
        {
          for (b = 15;; b += 255)
          {
            if (srcRem == 0)
              return S_FALSE;
            UInt32 b2 = *src++;
            srcRem--;
            if (b2 != 0)
            {
              b += b2;
              break;
            }
          }
        }
      
        b += 3;
        if (b > srcRem || b > destRem)
          return S_FALSE;
        srcRem -= b;
        destRem -= b;
        mode = 4;
        do
          *dest++ = *src++;
        while (--b);
        continue;
      }
      
      srcRem--;
      back = (b >> 2) + ((UInt32)*src++ << 2);
      len = 2;
      if (mode == 4)
      {
        back += (1 << 11);
        len = 3;
      }
    }
    else
    {
      UInt32 bOld = b;
      b = (b < 32 ? 7 : 31);
      len = bOld & b;
      
      if (len == 0)
      {
        for (len = b;; len += 255)
        {
          if (srcRem == 0)
            return S_FALSE;
          UInt32 b2 = *src++;
          srcRem--;
          if (b2 != 0)
          {
            len += b2;
            break;
          }
        }
      }
      
      len += 2;
      if (srcRem < 2)
        return S_FALSE;
      b = *src;
      back = (b >> 2) + ((UInt32)src[1] << 6);
      src += 2;
      srcRem -= 2;
      if (bOld < 32)
      {
        back += ((bOld & 8) << 11);
        if (back == 0)
        {
          *destLen = (size_t)(dest - destStart);
          *srcLen = (size_t)(src - srcStart);
          return S_OK;
        }
        back += (1 << 14) - 1;
      }
    }
    
    back++;
    if (len > destRem || (size_t)(dest - destStart) < back)
      return S_FALSE;
    destRem -= len;
    Byte *destTemp = dest - back;
    dest += len;
    
    do
    {
      *(destTemp + back) = *destTemp;
      destTemp++;
    }
    while (--len);
    
    b &= 3;
    mode = b;
    if (b == 0)
      continue;
    if (b > srcRem || b > destRem)
      return S_FALSE;
    srcRem -= b;
    destRem -= b;
    *dest++ = *src++;
    if (b > 1)
    {
      *dest++ = *src++;
      if (b > 2)
        *dest++ = *src++;
    }
  }
}

static HRESULT Lz4Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen)
{
  const int compressedSize = (int)*srcLen;
  const int dstCapacity = (int)*destLen;
  const int result = LZ4_decompress_safe((const char *)src, (char *)dest, compressedSize, dstCapacity);
  if (result <= 0)
    return S_FALSE;

  *destLen = (SizeT)result;
  *srcLen = (SizeT)compressedSize;
  return S_OK;
}

HRESULT CHandler::Decompress(ISequentialOutStream *outStream, Byte *outBuf, bool *outBufWasWritten, UInt32 *outBufWasWrittenSize, UInt32 inSize, UInt32 outSizeMax)
{
  if (outBuf)
  {
    *outBufWasWritten = false;
    *outBufWasWrittenSize = 0;
  }
  UInt32 method = _h.Method;
  if (_h.SeveralMethods)
  {
    Byte b;
    RINOK(ReadStream_FALSE(_stream, &b, 1))
    RINOK(_stream->Seek(-1, STREAM_SEEK_CUR, NULL))
    method = (b == 0x5D ? kMethod_LZMA : kMethod_ZLIB);
  }

  if (method == kMethod_ZLIB && _needCheckLzma)
  {
    Byte b;
    RINOK(ReadStream_FALSE(_stream, &b, 1))
    RINOK(_stream->Seek(-1, STREAM_SEEK_CUR, NULL))
    if (b == 0)
    {
      _noPropsLZMA = true;
      method = _h.Method = kMethod_LZMA;
    }
    _needCheckLzma = false;
  }
  
  if (method == kMethod_ZLIB)
  {
    _zlibDecoder.Create_if_Empty();
    RINOK(_zlibDecoder.Interface()->Code(_limitedInStream, outStream, NULL, NULL, NULL))
    if (inSize != _zlibDecoder->GetInputProcessedSize())
      return S_FALSE;
  }
  /*
  else if (method == kMethod_LZMA)
  {
    _lzmaDecoder.Create_if_Empty();
    // _lzmaDecoder->FinishStream = true;
    const UInt32 kPropsSize = LZMA_PROPS_SIZE + 8;
    Byte props[kPropsSize];
    UInt32 propsSize;
    UInt64 outSize;
    if (_noPropsLZMA)
    {
      props[0] = 0x5D;
      SetUi32(&props[1], _h.BlockSize);
      propsSize = 0;
      outSize = outSizeMax;
    }
    else
    {
      RINOK(ReadStream_FALSE(_limitedInStream, props, kPropsSize));
      propsSize = kPropsSize;
      outSize = GetUi64(&props[LZMA_PROPS_SIZE]);
      if (outSize > outSizeMax)
        return S_FALSE;
    }
    RINOK(_lzmaDecoderSpec->SetDecoderProperties2(props, LZMA_PROPS_SIZE));
    RINOK(_lzmaDecoder->Code(_limitedInStream, outStream, NULL, &outSize, NULL));
    if (inSize != propsSize + _lzmaDecoderSpec->GetInputProcessedSize())
      return S_FALSE;
  }
  */
  else
  {
    if (_inputBuffer.Size() < inSize)
      _inputBuffer.Alloc(inSize);
    RINOK(ReadStream_FALSE(_stream, _inputBuffer, inSize))

    Byte *dest = outBuf;
    if (!outBuf)
    {
      dest = _dynOutStream->GetBufPtrForWriting(outSizeMax);
      if (!dest)
        return E_OUTOFMEMORY;
    }
    
    SizeT destLen = outSizeMax, srcLen = inSize;

    if (method == kMethod_LZO)
    {
      RINOK(LzoDecode(dest, &destLen, _inputBuffer, &srcLen))
    }
    else if (method == kMethod_LZ4)
    {
      RINOK(Lz4Decode(dest, &destLen, _inputBuffer, &srcLen))
    }
    else if (method == kMethod_LZMA)
    {
      Byte props[5];
      const Byte *src = _inputBuffer;

      if (_noPropsLZMA)
      {
        props[0] = 0x5D;
        SetUi32(&props[1], _h.BlockSize)
      }
      else
      {
        const UInt32 kPropsSize = LZMA_PROPS_SIZE + 8;
        if (inSize < kPropsSize)
          return S_FALSE;
        memcpy(props, src, LZMA_PROPS_SIZE);
        UInt64 outSize = GetUi64(src + LZMA_PROPS_SIZE);
        if (outSize > outSizeMax)
          return S_FALSE;
        destLen = (SizeT)outSize;
        src += kPropsSize;
        inSize -= kPropsSize;
        srcLen = inSize;
      }

      ELzmaStatus status;
      SRes res = LzmaDecode(dest, &destLen,
          src, &srcLen,
          props, LZMA_PROPS_SIZE,
          LZMA_FINISH_END,
          &status, &g_Alloc);
      if (res != 0)
        return SResToHRESULT(res);
      if (status != LZMA_STATUS_FINISHED_WITH_MARK
          && status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
        return S_FALSE;
    }
    else if (method == kMethod_ZSTD)
    {
      const Byte *src = _inputBuffer;

      if (!_zstd)
      {
        _zstd = ZstdDec_Create(&g_AlignedAlloc, &g_AlignedAlloc);
        if (!_zstd)
          return E_OUTOFMEMORY;
      }

      CZstdDecState state;
      ZstdDecState_Clear(&state);

      state.inBuf = src;
      state.inLim = srcLen; //  + 1; for debug
      // state.outStep = outSizeMax;
      
      state.outBuf_fromCaller = dest;
      state.outBufSize_fromCaller = outSizeMax;
      // state.mustBeFinished = True;

      ZstdDec_Init(_zstd);
      SRes sres;
      for (;;)
      {
        sres = ZstdDec_Decode(_zstd, &state);
        if (sres != SZ_OK)
          break;
        if (state.inLim == state.inPos
            && (state.status == ZSTD_STATUS_NEEDS_MORE_INPUT ||
                state.status == ZSTD_STATUS_FINISHED_FRAME))
          break;
        // sres = sres;
        // break; // for debug
      }

      CZstdDecResInfo info;
      // ZstdDecInfo_Clear(&stat);
      // stat->InSize = state.inPos;
      ZstdDec_GetResInfo(_zstd, &state, sres, &info);
      sres = info.decode_SRes;
      if (sres == SZ_OK)
      {
        if (state.status != ZSTD_STATUS_FINISHED_FRAME
            // ||stat.UnexpededEnd
            || info.extraSize != 0
            || state.inLim != state.inPos)
          sres = SZ_ERROR_DATA;
      }
      if (sres != SZ_OK)
        return SResToHRESULT(sres);
      if (state.winPos > outSizeMax)
        return E_FAIL;
      // memcpy(dest, state.dic, state.dicPos);
      destLen = state.winPos;
    }
    else
    {
      ECoderStatus status;
      const SRes res = XzUnpacker_CodeFull(&_xz,
          dest, &destLen,
          _inputBuffer, &srcLen,
          CODER_FINISH_END, &status);
      if (res != 0)
        return SResToHRESULT(res);
      if (status != CODER_STATUS_NEEDS_MORE_INPUT || !XzUnpacker_IsStreamWasFinished(&_xz))
        return S_FALSE;
    }
    
    if (inSize != srcLen)
      return S_FALSE;
    if (outBuf)
    {
      *outBufWasWritten = true;
      *outBufWasWrittenSize = (UInt32)destLen;
    }
    else
      _dynOutStream->UpdateSize(destLen);
  }
  return S_OK;
}

// in  : packSize : allowed packSize limit
// out : packSize : real packSize in block
// out(packSize) <= in(packSize)
HRESULT CHandler::ReadMetadataBlock(UInt32 &packSize)
{
  Byte temp[3];
  const unsigned offset = _h.NeedCheckData() ? 3 : 2;
  if (offset > packSize)
    return S_FALSE;
  RINOK(ReadStream_FALSE(_stream, temp, offset))
  // if (NeedCheckData && Major < 4) checkByte must be = 0xFF
  const bool be = _h.be;
  UInt32 size = Get16(temp);
  const bool isCompressed = ((size & kNotCompressedBit16) == 0);
  if (size != kNotCompressedBit16)
    size &= ~kNotCompressedBit16;
  if (size > kMetadataBlockSize)
    return S_FALSE;
  const UInt32 packSize2 = offset + size;
  if (packSize2 > packSize)
    return S_FALSE;
  packSize = packSize2;
  if (isCompressed)
  {
    _limitedInStream->Init(size);
    RINOK(Decompress(_dynOutStream, NULL, NULL, NULL, size, kMetadataBlockSize))
  }
  else
  {
    // size != 0 here
    Byte *buf = _dynOutStream->GetBufPtrForWriting(size);
    if (!buf)
      return E_OUTOFMEMORY;
    RINOK(ReadStream_FALSE(_stream, buf, size))
    _dynOutStream->UpdateSize(size);
  }
  return S_OK;
}


HRESULT CHandler::ReadMetadataBlock2()
{
  _dynOutStream->Init();
  UInt32 packSize = kMetadataBlockSize + 3; // check it
  return ReadMetadataBlock(packSize);
}

HRESULT CHandler::ReadData(CData &data, UInt64 start, UInt64 end)
{
  if (end < start)
    return S_FALSE;
  const UInt64 size64 = end - start;
  if (size64 >= ((UInt64)1 << 32))
    return S_FALSE;
  const UInt32 size = (UInt32)size64;
  RINOK(Seek2(start))
  _dynOutStream->Init();
  UInt32 packPos = 0;
  while (packPos != size)
  {
    data.PackPos.Add(packPos);
    data.UnpackPos.Add((UInt32)_dynOutStream->GetSize());
    UInt32 packSize = size - packPos;
    RINOK(ReadMetadataBlock(packSize))
    {
      const size_t tSize = _dynOutStream->GetSize();
      if (tSize != (UInt32)tSize)
        return S_FALSE;
    }
    packPos += packSize;
  }
  _dynOutStream->CopyToBuffer(data.Data);
  return S_OK;
}

struct CTempItem
{
  UInt32 StartBlock;
  // UInt32 iNodeNumber1;
  UInt32 Offset;
  // UInt16 iNodeNumber2;
  UInt16 Type;
};
  
HRESULT CHandler::OpenDir(int parent, UInt32 startBlock, UInt32 offset, unsigned level, int &nodeIndex)
{
  if (level > kNumDirLevelsMax)
    return S_FALSE;

  int blockIndex = _inodesData.PackPos.FindInSorted(startBlock);
  if (blockIndex < 0)
    return S_FALSE;
  UInt32 unpackPos = _inodesData.UnpackPos[blockIndex] + offset;
  if (unpackPos < offset)
    return S_FALSE;

  nodeIndex = _nodesPos.FindInSorted(unpackPos);
  if (nodeIndex < 0)
    return S_FALSE;
  
  const CNode &n = _nodes[nodeIndex];
  if (!n.IsDir())
    return S_OK;
  if ((UInt32)n.StartBlock != n.StartBlock)
    return S_FALSE;
  blockIndex = _dirs.PackPos.FindInSorted((UInt32)n.StartBlock);
  if (blockIndex < 0)
    return S_FALSE;
  unpackPos = _dirs.UnpackPos[blockIndex] + n.Offset;
  if (unpackPos < n.Offset || unpackPos > _dirs.Data.Size())
    return S_FALSE;

  UInt32 rem = (UInt32)_dirs.Data.Size() - unpackPos;
  const Byte *p = _dirs.Data + unpackPos;
  UInt32 fileSize = (UInt32)n.FileSize;

  // for some squashfs files: fileSize = rem + 3  !!!
  if (_h.Major >= 3)
  {
    if (fileSize < 3)
      return S_FALSE;
    fileSize -= 3;
  }
  if (fileSize > rem)
    return S_FALSE;
  rem = fileSize;

  AString tempString;

  CRecordVector<CTempItem> tempItems;
  while (rem != 0)
  {
    const bool be = _h.be;
    UInt32 count;
    CTempItem tempItem;
    if (_h.Major <= 2)
    {
      if (rem < 4)
        return S_FALSE;
      count = p[0];
      tempItem.StartBlock = Get32(p);
      if (be)
        tempItem.StartBlock &= 0xFFFFFF;
      else
        tempItem.StartBlock >>= 8;
      p += 4;
      rem -= 4;
    }
    else
    {
      if (_h.Major == 3)
      {
        if (rem < 9)
          return S_FALSE;
        count = p[0];
        p += 1;
        rem -= 1;
      }
      else
      {
        if (rem < 12)
          return S_FALSE;
        count = GetUi32(p);
        p += 4;
        rem -= 4;
      }
      GET_32 (0, tempItem.StartBlock);
      // GET_32 (4, tempItem.iNodeNumber1);
      p += 8;
      rem -= 8;
    }
    count++;
    
    for (UInt32 i = 0; i < count; i++)
    {
      if (rem == 0)
        return S_FALSE;

      UInt32 nameOffset = _h.GetFileNameOffset();
      if (rem < nameOffset)
        return S_FALSE;

      if (_items.Size() >= kNumFilesMax)
        return S_FALSE;
      if (_openCallback)
      {
        UInt64 numFiles = _items.Size();
        if ((numFiles & 0xFFFF) == 0)
        {
          RINOK(_openCallback->SetCompleted(&numFiles, NULL))
        }
      }
      
      CItem item;
      item.Ptr = (UInt32)(p - (const Byte *)_dirs.Data);

      UInt32 size;
      if (_h.IsOldVersion())
      {
        UInt32 t = Get16(p);
        if (be)
        {
          tempItem.Offset = t >> 3;
          tempItem.Type = (UInt16)(t & 0x7);
        }
        else
        {
          tempItem.Offset = t & 0x1FFF;
          tempItem.Type = (UInt16)(t >> 13);
        }
        size = (UInt32)p[2];
        /*
        if (_h.Major > 2)
          tempItem.iNodeNumber2 = Get16(p + 3);
        */
      }
      else
      {
        GET_16 (0, tempItem.Offset);
        // GET_16 (2, tempItem.iNodeNumber2);
        GET_16 (4, tempItem.Type);
        GET_16 (6, size);
      }
      p += nameOffset;
      rem -= nameOffset;
      size++;
      if (rem < size)
        return S_FALSE;

      if (_openCodePage == CP_UTF8)
      {
        tempString.SetFrom_CalcLen((const char *)p, size);
        if (!CheckUTF8_AString(tempString))
          _openCodePage = CP_OEMCP;
      }

      p += size;
      rem -= size;
      item.Parent = parent;
      _items.Add(item);
      tempItems.Add(tempItem);
    }
  }

  const unsigned startItemIndex = _items.Size() - tempItems.Size();
  FOR_VECTOR (i, tempItems)
  {
    const CTempItem &tempItem = tempItems[i];
    const unsigned index = startItemIndex + i;
    CItem &item = _items[index];
    RINOK(OpenDir((int)index, tempItem.StartBlock, tempItem.Offset, level + 1, item.Node))
  }

  return S_OK;
}

HRESULT CHandler::ReadUids(UInt64 start, UInt32 num, CByteBuffer &ids)
{
  const size_t size = (size_t)num * 4;
  ids.Alloc(size);
  if (num == 0)
    return S_OK;
  RINOK(Seek2(start))
  return ReadStream_FALSE(_stream, ids, size);
}

HRESULT CHandler::Open2(IInStream *inStream)
{
  {
    Byte buf[kHeaderSize3];
    RINOK(ReadStream_FALSE(inStream, buf, kHeaderSize3))
    if (!_h.Parse(buf))
      return S_FALSE;
    if (!_h.IsSupported())
      return E_NOTIMPL;
    
    _noPropsLZMA = false;
    _needCheckLzma = false;
    switch (_h.Method)
    {
      case kMethod_ZLIB: _needCheckLzma = true; break;
      case kMethod_LZMA:
      case kMethod_LZO:
      case kMethod_XZ:
      case kMethod_LZ4:
      case kMethod_ZSTD:
        break;
      default:
        return E_NOTIMPL;
    }
  }

  _stream = inStream;

  if (_h.NumFrags != 0)
  {
    if (_h.NumFrags > kNumFilesMax)
      return S_FALSE;
    _frags.ClearAndReserve(_h.NumFrags);
    const unsigned bigFrag = (_h.Major > 2);
    
    const unsigned fragPtrsInBlockLog = kMetadataBlockSizeLog - (3 + bigFrag);
    const UInt32 numBlocks = (_h.NumFrags + (1 << fragPtrsInBlockLog) - 1) >> fragPtrsInBlockLog;
    const size_t numBlocksBytes = (size_t)numBlocks << (2 + bigFrag);
    CByteBuffer data(numBlocksBytes);
    RINOK(Seek2(_h.FragTable))
    RINOK(ReadStream_FALSE(inStream, data, numBlocksBytes))
    const bool be = _h.be;
    
    for (UInt32 i = 0; i < numBlocks; i++)
    {
      const UInt64 offset = bigFrag ? Get64(data + i * 8) : Get32(data + i * 4);
      RINOK(Seek2(offset))
      RINOK(ReadMetadataBlock2())
      const UInt32 unpackSize = (UInt32)_dynOutStream->GetSize();
      if (unpackSize != kMetadataBlockSize)
        if (i != numBlocks - 1 || unpackSize != ((_h.NumFrags << (3 + bigFrag)) & (kMetadataBlockSize - 1)))
          return S_FALSE;
      const Byte *buf = _dynOutStream->GetBuffer();
      for (UInt32 j = 0; j < kMetadataBlockSize && j < unpackSize;)
      {
        CFrag frag;
        if (bigFrag)
        {
          frag.StartBlock = Get64(buf + j);
          frag.Size = Get32(buf + j + 8);
          // some archives contain nonzero in unused (buf + j + 12)
          j += 16;
        }
        else
        {
          frag.StartBlock = Get32(buf + j);
          frag.Size = Get32(buf + j + 4);
          j += 8;
        }
        _frags.Add(frag);
      }
    }
    if ((UInt32)_frags.Size() != _h.NumFrags)
      return S_FALSE;
  }

  RINOK(ReadData(_inodesData, _h.InodeTable, _h.DirTable))
  RINOK(ReadData(_dirs, _h.DirTable, _h.FragTable))

  const UInt64 absOffset = _h.RootInode >> 16;
  if (absOffset >= ((UInt64)1 << 32))
    return S_FALSE;
  {
    const UInt32 totalSize = (UInt32)_inodesData.Data.Size();
    const unsigned kMinNodeParseSize = 4;
    if (_h.NumInodes > totalSize / kMinNodeParseSize)
      return S_FALSE;
    _nodesPos.ClearAndReserve(_h.NumInodes);
    _nodes.ClearAndReserve(_h.NumInodes);
    UInt32 pos = 0;
    for (UInt32 i = 0; i < _h.NumInodes; i++)
    {
      CNode n;
      const Byte *p = _inodesData.Data + pos;
      UInt32 size = totalSize - pos;

      switch (_h.Major)
      {
        case 1:  size = n.Parse1(p, size, _h); break;
        case 2:  size = n.Parse2(p, size, _h); break;
        case 3:  size = n.Parse3(p, size, _h); break;
        default: size = n.Parse4(p, size, _h); break;
      }
      if (size == 0)
        return S_FALSE;
      _nodesPos.AddInReserved(pos);
      _nodes.AddInReserved(n);
      pos += size;
    }
    if (pos != totalSize)
      return S_FALSE;
  }
  int rootNodeIndex;
  RINOK(OpenDir(-1, (UInt32)absOffset, (UInt32)_h.RootInode & 0xFFFF, 0, rootNodeIndex))

  if (_h.Major < 4)
  {
    RINOK(ReadUids(_h.UidTable, _h.NumUids, _uids))
    RINOK(ReadUids(_h.GidTable, _h.NumGids, _gids))
  }
  else
  {
    const UInt32 size = (UInt32)_h.NumIDs * 4;
    _uids.Alloc(size);

    const UInt32 numBlocks = (size + kMetadataBlockSize - 1) / kMetadataBlockSize;
    const UInt32 numBlocksBytes = numBlocks << 3;
    CByteBuffer data(numBlocksBytes);
    RINOK(Seek2(_h.UidTable))
    RINOK(ReadStream_FALSE(inStream, data, numBlocksBytes))

    for (UInt32 i = 0; i < numBlocks; i++)
    {
      const UInt64 offset = GetUi64(data + i * 8);
      RINOK(Seek2(offset))
      // RINOK(ReadMetadataBlock(NULL, _uids + kMetadataBlockSize * i, packSize, unpackSize));
      RINOK(ReadMetadataBlock2())
      const size_t unpackSize = _dynOutStream->GetSize();
      const UInt32 remSize = (i == numBlocks - 1)  ?
          (size & (kMetadataBlockSize - 1)) : kMetadataBlockSize;
      if (unpackSize != remSize)
        return S_FALSE;
      memcpy(_uids + kMetadataBlockSize * i, _dynOutStream->GetBuffer(), remSize);
    }
  }

  {
    const UInt32 alignSize = 1 << 12;
    Byte buf[alignSize];
    RINOK(Seek2(_h.Size))
    UInt32 rem = (UInt32)(0 - _h.Size) & (alignSize - 1);
    _sizeCalculated = _h.Size;
    if (rem != 0)
    {
      if (ReadStream_FALSE(_stream, buf, rem) == S_OK)
      {
        size_t i;
        for (i = 0; i < rem && buf[i] == 0; i++);
        if (i == rem)
          _sizeCalculated = _h.Size + rem;
      }
    }
  }
  return S_OK;
}

AString CHandler::GetPath(unsigned index) const
{
  unsigned len = 0;
  const unsigned indexMem = index;
  const bool be = _h.be;
  for (;;)
  {
    const CItem &item = _items[index];
    const Byte *p = _dirs.Data + item.Ptr;
    const unsigned size = (_h.IsOldVersion() ? (unsigned)p[2] : (unsigned)Get16(p + 6)) + 1;
    p += _h.GetFileNameOffset();
    unsigned i;
    for (i = 0; i < size && p[i]; i++);
    len += i + 1;
    index = (unsigned)item.Parent;
    if (item.Parent < 0)
      break;
  }
  len--;

  AString path;
  char *dest = path.GetBuf_SetEnd(len) + len;
  index = indexMem;
  for (;;)
  {
    const CItem &item = _items[index];
    const Byte *p = _dirs.Data + item.Ptr;
    const unsigned size = (_h.IsOldVersion() ? (unsigned)p[2] : (unsigned)Get16(p + 6)) + 1;
    p += _h.GetFileNameOffset();
    unsigned i;
    for (i = 0; i < size && p[i]; i++);
    dest -= i;
    memcpy(dest, p, i);
    index = (unsigned)item.Parent;
    if (item.Parent < 0)
      break;
    *(--dest) = CHAR_PATH_SEPARATOR;
  }
  return path;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  {
    Close();
    _limitedInStream->SetStream(stream);
    HRESULT res;
    try
    {
      _openCallback = callback;
      res = Open2(stream);
    }
    catch(...)
    {
      Close();
      throw;
    }
    if (res != S_OK)
    {
      Close();
      return res;
    }
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _openCodePage = CP_UTF8;
  _sizeCalculated = 0;

  _limitedInStream->ReleaseStream();
  _stream.Release();

  _items.Clear();
  _nodes.Clear();
  _nodesPos.Clear();
  _frags.Clear();
  _inodesData.Clear();
  _dirs.Clear();

  _uids.Free();
  _gids.Free();

  _cachedBlock.Free();
  ClearCache();

  return S_OK;
}

bool CHandler::GetPackSize(unsigned index, UInt64 &totalPack, bool fillOffsets)
{
  totalPack = 0;
  const CItem &item = _items[index];
  const CNode &node = _nodes[item.Node];
  const UInt32 ptr = _nodesPos[item.Node];
  const Byte *p = _inodesData.Data + ptr;
  const bool be = _h.be;

  const UInt32 type = node.Type;
  UInt32 offset;
  if (node.IsLink() || node.FileSize == 0)
  {
    totalPack = node.FileSize;
    return true;
  }

  const UInt32 numBlocks = (UInt32)node.GetNumBlocks(_h);

  if (fillOffsets)
  {
    _blockOffsets.Clear();
    _blockCompressed.Clear();
    _blockOffsets.Add(totalPack);
  }

  if (_h.Major <= 1)
  {
    offset = 15;
    p += offset;
    
    for (UInt32 i = 0; i < numBlocks; i++)
    {
      UInt32 t = Get16(p + i * 2);
      if (fillOffsets)
        _blockCompressed.Add((t & kNotCompressedBit16) == 0);
      if (t != kNotCompressedBit16)
        t &= ~kNotCompressedBit16;
      totalPack += t;
      if (fillOffsets)
        _blockOffsets.Add(totalPack);
    }
  }
  else
  {
    if (_h.Major <= 2)
      offset = 24;
    else if (type == kType_FILE)
      offset = 32;
    else if (type == kType_FILE + 7)
      offset = (_h.Major <= 3 ? 40 : 56);
    else
      return false;
    
    p += offset;
    
    for (UInt64 i = 0; i < numBlocks; i++)
    {
      UInt32 t = Get32(p + i * 4);
      if (fillOffsets)
        _blockCompressed.Add(IS_COMPRESSED_BLOCK(t));
      UInt32 size = GET_COMPRESSED_BLOCK_SIZE(t);
      if (size > _h.BlockSize)
        return false;
      totalPack += size;
      if (fillOffsets)
        _blockOffsets.Add(totalPack);
    }

    if (node.ThereAreFrags())
    {
      if (node.Frag >= (UInt32)_frags.Size())
        return false;
      const CFrag &frag = _frags[node.Frag];
      if (node.Offset == 0)
      {
        UInt32 size = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
        if (size > _h.BlockSize)
          return false;
        totalPack += size;
      }
    }
  }
  return true;
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMethod:
    {
      char sz[16];
      const char *s;
      if (_noPropsLZMA)
        s = "LZMA Spec";
      else if (_h.SeveralMethods)
        s = "LZMA ZLIB";
      else
      {
        s = NULL;
        if (_h.Method < Z7_ARRAY_SIZE(k_Methods))
          s = k_Methods[_h.Method];
        if (!s)
        {
          ConvertUInt32ToString(_h.Method, sz);
          s = sz;
        }
      }
      prop = s;
      break;
    }
    case kpidFileSystem:
    {
      AString res ("SquashFS");
      if (_h.SeveralMethods)
        res += "-LZMA";
      res.Add_Space();
      res.Add_UInt32(_h.Major);
      res.Add_Dot();
      res.Add_UInt32(_h.Minor);
      prop = res;
      break;
    }
    case kpidClusterSize: prop = _h.BlockSize; break;
    case kpidBigEndian: prop = _h.be; break;
    case kpidCTime:
      if (_h.CTime != 0)
        PropVariant_SetFrom_UnixTime(prop, _h.CTime);
      break;
    case kpidCharacts: FLAGS_TO_PROP(k_Flags, _h.Flags, prop); break;
    // case kpidNumBlocks: prop = _h.NumFrags; break;
    case kpidPhySize: prop = _sizeCalculated; break;
    case kpidHeadersSize:
      if (_sizeCalculated >= _h.InodeTable)
        prop = _sizeCalculated - _h.InodeTable;
      break;

    case kpidCodePage:
    {
      char sz[16];
      const char *name = NULL;
      switch (_openCodePage)
      {
        case CP_OEMCP: name = "OEM"; break;
        case CP_UTF8: name = "UTF-8"; break;
      }
      if (!name)
      {
        ConvertUInt32ToString(_openCodePage, sz);
        name = sz;
      }
      prop = name;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = _items[index];
  const CNode &node = _nodes[item.Node];
  const bool isDir = node.IsDir();
  const bool be = _h.be;

  switch (propID)
  {
    case kpidPath:
    {
      AString path (GetPath(index));
      UString s;
      if (_openCodePage == CP_UTF8)
        ConvertUTF8ToUnicode(path, s);
      else
        MultiByteToUnicodeString2(s, path, _openCodePage);
      prop = s;
      break;
    }
    case kpidIsDir: prop = isDir; break;
    // case kpidOffset: if (!node.IsLink()) prop = (UInt64)node.StartBlock; break;
    case kpidSize: if (!isDir) prop = node.GetSize(); break;
    case kpidPackSize:
      if (!isDir)
      {
        UInt64 size;
        if (GetPackSize(index, size, false))
          prop = size;
      }
      break;
    case kpidMTime:
    {
      UInt32 offset = 0;
      switch (_h.Major)
      {
        case 1:
          if (node.Type == kType_FILE)
            offset = 3;
          else if (node.Type == kType_DIR)
            offset = 7;
          break;
        case 2:
          if (node.Type == kType_FILE)
            offset = 4;
          else if (node.Type == kType_DIR)
            offset = 8;
          else if (node.Type == kType_DIR + 7)
            offset = 9;
          break;
        case 3: offset = 4; break;
        case 4: offset = 8; break;
      }
      if (offset != 0)
      {
        const Byte *p = _inodesData.Data + _nodesPos[item.Node] + offset;
        PropVariant_SetFrom_UnixTime(prop, Get32(p));
      }
      break;
    }
    case kpidPosixAttrib:
    {
      if (node.Type != 0 && node.Type < Z7_ARRAY_SIZE(k_TypeToMode))
        prop = (UInt32)(node.Mode & 0xFFF) | k_TypeToMode[node.Type];
      break;
    }
    case kpidUserId:
    case kpidGroupId:
    {
      UInt32 id = node.Uid;
      const CByteBuffer *ids = &_uids;
      if (propID == kpidGroupId)
      {
        id = node.Gid;
        if (_h.Major < 4)
        {
          if (id == _h.GetSpecGuidIndex())
            id = node.Uid;
          else
            ids = &_gids;
        }
      }
      const UInt32 offset = (UInt32)id * 4;
      if (offset < ids->Size())
        prop = (UInt32)Get32(*ids + offset);
      break;
    }
    /*
    case kpidLinks:
      if (_h.Major >= 3 && node.Type != kType_FILE)
        prop = node.NumLinks;
      break;
    */
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CSquashfsInStream: public CCachedInStream
{
  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) Z7_override;
public:
  CHandler *Handler;
};

HRESULT CSquashfsInStream::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize)
{
  return Handler->ReadBlock(blockIndex, dest, blockSize);
}

HRESULT CHandler::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize)
{
  const CNode &node = _nodes[_nodeIndex];
  UInt64 blockOffset;
  UInt32 packBlockSize;
  UInt32 offsetInBlock = 0;
  bool compressed;
  if (blockIndex < _blockCompressed.Size())
  {
    compressed = _blockCompressed[(unsigned)blockIndex];
    blockOffset = _blockOffsets[(unsigned)blockIndex];
    packBlockSize = (UInt32)(_blockOffsets[(unsigned)blockIndex + 1] - blockOffset);
    blockOffset += node.StartBlock;
  }
  else
  {
    if (!node.ThereAreFrags())
      return S_FALSE;
    const CFrag &frag = _frags[node.Frag];
    offsetInBlock = node.Offset;
    if (offsetInBlock > _h.BlockSize)
      return S_FALSE;
    blockOffset = frag.StartBlock;
    packBlockSize = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
    compressed = IS_COMPRESSED_BLOCK(frag.Size);
  }

  if (packBlockSize == 0)
  {
    // sparse file ???
    memset(dest, 0, blockSize);
    return S_OK;
  }

  if (blockOffset != _cachedBlockStartPos ||
      packBlockSize != _cachedPackBlockSize)
  {
    ClearCache();
    RINOK(Seek2(blockOffset))
    _limitedInStream->Init(packBlockSize);
    
    if (compressed)
    {
      _outStream->Init((Byte *)_cachedBlock, _h.BlockSize);
      bool outBufWasWritten;
      UInt32 outBufWasWrittenSize;
      HRESULT res = Decompress(_outStream, _cachedBlock, &outBufWasWritten, &outBufWasWrittenSize, packBlockSize, _h.BlockSize);
      RINOK(res)
      if (outBufWasWritten)
        _cachedUnpackBlockSize = outBufWasWrittenSize;
      else
        _cachedUnpackBlockSize = (UInt32)_outStream->GetPos();
    }
    else
    {
      if (packBlockSize > _h.BlockSize)
        return S_FALSE;
      RINOK(ReadStream_FALSE(_limitedInStream, _cachedBlock, packBlockSize))
      _cachedUnpackBlockSize = packBlockSize;
    }
    _cachedBlockStartPos = blockOffset;
    _cachedPackBlockSize = packBlockSize;
  }

  if (_cachedUnpackBlockSize < offsetInBlock ||
      _cachedUnpackBlockSize - offsetInBlock < blockSize)
    return S_FALSE;
  if (blockSize != 0)
    memcpy(dest, _cachedBlock + offsetInBlock, blockSize);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CItem &item = _items[allFilesMode ? i : indices[i]];
    const CNode &node = _nodes[item.Node];
    totalSize += node.GetSize();
  }
  RINOK(extractCallback->SetTotal(totalSize))

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

  for (i = 0;; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;

    int res;
   {
    CMyComPtr<ISequentialOutStream> outStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CItem &item = _items[index];
    const CNode &node = _nodes[item.Node];
    RINOK(extractCallback->GetStream(index, &outStream, askMode))
    // const Byte *p = _data + item.Offset;

    if (node.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }
    const UInt64 unpackSize = node.GetSize();
    totalSize += unpackSize;
    UInt64 packSize;
    if (GetPackSize(index, packSize, false))
      totalPackSize += packSize;

    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))

    res = NExtract::NOperationResult::kDataError;
    {
      CMyComPtr<ISequentialInStream> inSeqStream;
      HRESULT hres = GetStream(index, &inSeqStream);
      if (hres == S_FALSE || !inSeqStream)
      {
        if (hres == E_OUTOFMEMORY)
          return hres;
        res = NExtract::NOperationResult::kUnsupportedMethod;
      }
      else
      {
        RINOK(hres)
        {
          hres = copyCoder.Interface()->Code(inSeqStream, outStream, NULL, NULL, lps);
          if (hres == S_OK)
          {
            if (copyCoder->TotalSize == unpackSize)
              res = NExtract::NOperationResult::kOK;
          }
          else if (hres == E_NOTIMPL)
          {
            res = NExtract::NOperationResult::kUnsupportedMethod;
          }
          else if (hres != S_FALSE)
          {
            RINOK(hres)
          }
        }
      }
    }
   }
    RINOK(extractCallback->SetOperationResult(res))
  }
  
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN

  const CItem &item = _items[index];
  const CNode &node = _nodes[item.Node];

  if (node.IsDir())
    return E_FAIL;

  const Byte *p = _inodesData.Data + _nodesPos[item.Node];

  if (node.FileSize == 0 || node.IsLink())
  {
    CBufInStream *streamSpec = new CBufInStream;
    CMyComPtr<IInStream> streamTemp = streamSpec;
    if (node.IsLink())
      streamSpec->Init(p + _h.GetSymLinkOffset(), (size_t)node.FileSize);
    else
      streamSpec->Init(NULL, 0);
    *stream = streamTemp.Detach();
    return S_OK;
  }

  UInt64 packSize;
  if (!GetPackSize(index, packSize, true))
    return S_FALSE;

  _nodeIndex = item.Node;

  size_t cacheSize = _h.BlockSize;
  if (_cachedBlock.Size() != cacheSize)
  {
    ClearCache();
    _cachedBlock.Alloc(cacheSize);
  }

  CSquashfsInStream *streamSpec = new CSquashfsInStream;
  CMyComPtr<IInStream> streamTemp = streamSpec;
  streamSpec->Handler = this;
  unsigned cacheSizeLog = 22;
  if (cacheSizeLog <= _h.BlockSizeLog)
    cacheSizeLog = _h.BlockSizeLog + 1;
  if (!streamSpec->Alloc(_h.BlockSizeLog, cacheSizeLog - _h.BlockSizeLog))
    return E_OUTOFMEMORY;
  streamSpec->Init(node.FileSize);
  *stream = streamTemp.Detach();

  return S_OK;

  COM_TRY_END
}

static const Byte k_Signature[] = {
    4, 'h', 's', 'q', 's',
    4, 's', 'q', 's', 'h',
    4, 's', 'h', 's', 'q',
    4, 'q', 's', 'h', 's' };

REGISTER_ARC_I(
  "SquashFS", "squashfs", NULL, 0xD2,
  k_Signature,
  0,
  NArcInfoFlags::kMultiSignature,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/SwfHandler.cpp ================ */
// SwfHandler.cpp

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

// amalgamation: header emitted in prologue

// #define Z7_SWF_UPDATE

#ifdef Z7_SWF_UPDATE

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
 
#endif

using namespace NWindows;

namespace NArchive {

static const UInt32 kFileSizeMax = (UInt32)1 << 29;

namespace NSwfc {

static const unsigned kHeaderBaseSize = 8;
static const unsigned kHeaderLzmaSize = 17;

static const Byte SWF_UNCOMPRESSED = 'F';
static const Byte SWF_COMPRESSED_ZLIB = 'C';
static const Byte SWF_COMPRESSED_LZMA = 'Z';

static const Byte SWF_MIN_COMPRESSED_ZLIB_VER = 6;
static const Byte SWF_MIN_COMPRESSED_LZMA_VER = 13;

static const Byte kVerLim = 64;

API_FUNC_static_IsArc IsArc_Swf(const Byte *p, size_t size)
{
  if (size < kHeaderBaseSize)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != SWF_UNCOMPRESSED ||
      p[1] != 'W' ||
      p[2] != 'S' ||
      p[3] >= kVerLim)
    return k_IsArc_Res_NO;
  UInt32 uncompressedSize = GetUi32(p + 4);
  if (uncompressedSize > kFileSizeMax)
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

API_FUNC_static_IsArc IsArc_Swfc(const Byte *p, size_t size)
{
  if (size < kHeaderBaseSize + 2 + 1) // 2 + 1 (for zlib check)
    return k_IsArc_Res_NEED_MORE;
  if ((p[0] != SWF_COMPRESSED_ZLIB &&
      p[0] != SWF_COMPRESSED_LZMA) ||
      p[1] != 'W' ||
      p[2] != 'S' ||
      p[3] >= kVerLim)
    return k_IsArc_Res_NO;
  UInt32 uncompressedSize = GetUi32(p + 4);
  if (uncompressedSize > kFileSizeMax)
    return k_IsArc_Res_NO;

  if (p[0] == SWF_COMPRESSED_ZLIB)
  {
    if (!NCompress::NZlib::IsZlib_3bytes(p + 8))
      return k_IsArc_Res_NO;
  }
  else
  {
    if (size < kHeaderLzmaSize + 2)
      return k_IsArc_Res_NEED_MORE;
    if (p[kHeaderLzmaSize] != 0 ||
        (p[kHeaderLzmaSize + 1] & 0x80) != 0)
      return k_IsArc_Res_NO;
    UInt32 lzmaPackSize = GetUi32(p + 8);
    UInt32 lzmaProp = p[12];
    UInt32 lzmaDicSize = GetUi32(p + 13);
    if (lzmaProp > 5 * 5 * 9 ||
        lzmaDicSize > ((UInt32)1 << 28) ||
        lzmaPackSize < 5 ||
        lzmaPackSize > ((UInt32)1 << 28))
      return k_IsArc_Res_NO;
  }

  return k_IsArc_Res_YES;
}
}

struct CItem
{
  Byte Buf[kHeaderLzmaSize];
  unsigned HeaderSize;

  UInt32 GetSize() const { return GetUi32(Buf + 4); }
  UInt32 GetLzmaPackSize() const { return GetUi32(Buf + 8); }
  UInt32 GetLzmaDicSize() const { return GetUi32(Buf + 13); }

  bool IsSwf() const { return (Buf[1] == 'W' && Buf[2] == 'S' && Buf[3] < kVerLim); }
  bool IsUncompressed() const { return Buf[0] == SWF_UNCOMPRESSED; }
  bool IsZlib() const { return Buf[0] == SWF_COMPRESSED_ZLIB; }
  bool IsLzma() const { return Buf[0] == SWF_COMPRESSED_LZMA; }

  void MakeUncompressed()
  {
    Buf[0] = SWF_UNCOMPRESSED;
    HeaderSize = kHeaderBaseSize;
  }
  void MakeZlib()
  {
    Buf[0] = SWF_COMPRESSED_ZLIB;
    if (Buf[3] < SWF_MIN_COMPRESSED_ZLIB_VER)
      Buf[3] = SWF_MIN_COMPRESSED_ZLIB_VER;
  }
  void MakeLzma(UInt32 packSize)
  {
    Buf[0] = SWF_COMPRESSED_LZMA;
    if (Buf[3] < SWF_MIN_COMPRESSED_LZMA_VER)
      Buf[3] = SWF_MIN_COMPRESSED_LZMA_VER;
    SetUi32(Buf + 8, packSize)
    HeaderSize = kHeaderLzmaSize;
  }

  HRESULT ReadHeader(ISequentialInStream *stream)
  {
    HeaderSize = kHeaderBaseSize;
    return ReadStream_FALSE(stream, Buf, kHeaderBaseSize);
  }
  HRESULT WriteHeader(ISequentialOutStream *stream)
  {
    return WriteStream(stream, Buf, HeaderSize);
  }
};


Z7_class_CHandler_final:
  public IInArchive,
  public IArchiveOpenSeq,
 #ifdef Z7_SWF_UPDATE
  public IOutArchive,
  public ISetProperties,
 #endif
  public CMyUnknownImp
{
 #ifdef Z7_SWF_UPDATE
  Z7_IFACES_IMP_UNK_4(IInArchive, IArchiveOpenSeq, IOutArchive, ISetProperties)
 #else
  Z7_IFACES_IMP_UNK_2(IInArchive, IArchiveOpenSeq)
 #endif

  CItem _item;
  UInt64 _packSize;
  bool _packSizeDefined;
  CMyComPtr<ISequentialInStream> _seqStream;
  CMyComPtr<IInStream> _stream;

#ifdef Z7_SWF_UPDATE
  CSingleMethodProps _props;
  bool _lzmaMode;
#endif

public:
 #ifdef Z7_SWF_UPDATE
  CHandler(): _lzmaMode(false) {}
 #endif
};

static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize,
  kpidMethod
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: if (_packSizeDefined) prop = _item.HeaderSize + _packSize; break;
    case kpidIsNotArcType: prop = true; break;
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1;
  return S_OK;
}

static void DicSizeToString(char *s, UInt32 val)
{
  char c = 0;
  unsigned i;
  for (i = 0; i < 32; i++)
    if (((UInt32)1 << i) == val)
    {
      val = i;
      break;
    }
  if (i == 32)
  {
    c = 'b';
    if      ((val & ((1 << 20) - 1)) == 0) { val >>= 20; c = 'm'; }
    else if ((val & ((1 << 10) - 1)) == 0) { val >>= 10; c = 'k'; }
  }
  ::ConvertUInt32ToString(val, s);
  unsigned pos = MyStringLen(s);
  s[pos++] = c;
  s[pos] = 0;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidSize: prop = (UInt64)_item.GetSize(); break;
    case kpidPackSize: if (_packSizeDefined) prop = _item.HeaderSize + _packSize; break;
    case kpidMethod:
    {
      char s[32];
      if (_item.IsZlib())
        MyStringCopy(s, "zlib");
      else
      {
        MyStringCopy(s, "LZMA:");
        DicSizeToString(s + 5, _item.GetLzmaDicSize());
      }
      prop = s;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *))
{
  RINOK(OpenSeq(stream))
  _stream = stream;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  Close();
  RINOK(_item.ReadHeader(stream))
  if (!_item.IsSwf())
    return S_FALSE;
  if (_item.IsLzma())
  {
    RINOK(ReadStream_FALSE(stream, _item.Buf + kHeaderBaseSize, kHeaderLzmaSize - kHeaderBaseSize))
    _item.HeaderSize = kHeaderLzmaSize;
    _packSize = _item.GetLzmaPackSize();
    _packSizeDefined = true;
  }
  else if (!_item.IsZlib())
    return S_FALSE;
  if (_item.GetSize() < _item.HeaderSize)
    return S_FALSE;
  _seqStream = stream;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Close())
{
  _packSize = 0;
  _packSizeDefined = false;
  _seqStream.Release();
  _stream.Release();
  return S_OK;
}

Z7_CLASS_IMP_COM_1(
  CCompressProgressInfoImp,
  ICompressProgressInfo
)
  CMyComPtr<IArchiveOpenCallback> Callback;
public:
  UInt64 Offset;
  void Init(IArchiveOpenCallback *callback) { Callback = callback; }
};

Z7_COM7F_IMF(CCompressProgressInfoImp::SetRatioInfo(const UInt64 *inSize, const UInt64 * /* outSize */))
{
  if (Callback)
  {
    const UInt64 files = 0;
    const UInt64 value = Offset + *inSize;
    return Callback->SetCompleted(&files, &value);
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  RINOK(extractCallback->SetTotal(_item.GetSize()))
  Int32 opRes;
{
  CMyComPtr<ISequentialOutStream> realOutStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
  if (!testMode && !realOutStream)
    return S_OK;

  RINOK(extractCallback->PrepareOperation(askMode))

  CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> outStream;
  outStream->SetStream(realOutStream);
  outStream->Init();
  // realOutStream.Release();

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);

  lps->InSize = _item.HeaderSize;
  lps->OutSize = outStream->GetSize();
  RINOK(lps->SetCur())
  
  CItem item = _item;
  item.MakeUncompressed();
  if (_stream)
    RINOK(InStream_SeekSet(_stream, _item.HeaderSize))
  NCompress::NZlib::CDecoder *_decoderZlibSpec = NULL;
  NCompress::NLzma::CDecoder *_decoderLzmaSpec = NULL;
  CMyComPtr<ICompressCoder> _decoder;

  CMyComPtr<ISequentialInStream> inStream2;

  const UInt64 unpackSize = _item.GetSize() - (UInt32)8;
  if (_item.IsZlib())
  {
    _decoderZlibSpec = new NCompress::NZlib::CDecoder;
    _decoder = _decoderZlibSpec;
    inStream2 = _seqStream;
  }
  else
  {
    /* Some .swf files with lzma contain additional 8 bytes at the end
       in uncompressed stream.
       What does that data mean ???
       We don't decompress these additional 8 bytes */
    
    // unpackSize = _item.GetSize();
    // SetUi32(item.Buf + 4, (UInt32)(unpackSize + 8));
    CLimitedSequentialInStream *limitedStreamSpec = new CLimitedSequentialInStream;
    inStream2 = limitedStreamSpec;
    limitedStreamSpec->SetStream(_seqStream);
    limitedStreamSpec->Init(_item.GetLzmaPackSize());

    _decoderLzmaSpec = new NCompress::NLzma::CDecoder;
    _decoder = _decoderLzmaSpec;
    // _decoderLzmaSpec->FinishStream = true;

    Byte props[5];
    memcpy(props, _item.Buf + 12, 5);
    UInt32 dicSize = _item.GetLzmaDicSize();
    if (dicSize > (UInt32)unpackSize)
    {
      dicSize = (UInt32)unpackSize;
      SetUi32(props + 1, dicSize)
    }
    RINOK(_decoderLzmaSpec->SetDecoderProperties2(props, 5))
  }
  RINOK(item.WriteHeader(outStream))
  const HRESULT result = _decoder->Code(inStream2, outStream, NULL, &unpackSize, lps);
  opRes = NExtract::NOperationResult::kDataError;
  if (result == S_OK)
  {
    if (item.GetSize() == outStream->GetSize())
    {
      if (_item.IsZlib())
      {
        _packSizeDefined = true;
        _packSize = _decoderZlibSpec->GetInputProcessedSize();
        opRes = NExtract::NOperationResult::kOK;
      }
      else
      {
        // if (_decoderLzmaSpec->GetInputProcessedSize() == _packSize)
          opRes = NExtract::NOperationResult::kOK;
      }
    }
  }
  else if (result != S_FALSE)
    return result;

  // outStream.Release();
 }
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}


#ifdef Z7_SWF_UPDATE

static HRESULT UpdateArchive(ISequentialOutStream *outStream, UInt64 size,
    bool lzmaMode, const CSingleMethodProps &props,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = 0;
  RINOK(updateCallback->SetTotal(size))
  RINOK(updateCallback->SetCompleted(&complexity))

  CMyComPtr<ISequentialInStream> fileInStream;
  RINOK(updateCallback->GetStream(0, &fileInStream))

  /*
  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  realOutStream.Release();
  */

  CItem item;
  const HRESULT res = item.ReadHeader(fileInStream);
  if (res == S_FALSE)
    return E_INVALIDARG;
  RINOK(res)
  if (!item.IsSwf() || !item.IsUncompressed() || size != item.GetSize())
    return E_INVALIDARG;

  NCompress::NZlib::CEncoder *encoderZlibSpec = NULL;
  NCompress::NLzma::CEncoder *encoderLzmaSpec = NULL;
  CMyComPtr<ICompressCoder> encoder;
  CMyComPtr<IOutStream> outSeekStream;
  if (lzmaMode)
  {
    outStream->QueryInterface(IID_IOutStream, (void **)&outSeekStream);
    if (!outSeekStream)
      return E_NOTIMPL;
    encoderLzmaSpec = new NCompress::NLzma::CEncoder;
    encoder = encoderLzmaSpec;
    RINOK(props.SetCoderProps(encoderLzmaSpec, &size))
    item.MakeLzma((UInt32)0xFFFFFFFF);
    CBufPtrSeqOutStream *propStreamSpec = new CBufPtrSeqOutStream;
    CMyComPtr<ISequentialOutStream> propStream = propStreamSpec;
    propStreamSpec->Init(item.Buf + 12, 5);
    RINOK(encoderLzmaSpec->WriteCoderProperties(propStream))
  }
  else
  {
    encoderZlibSpec = new NCompress::NZlib::CEncoder;
    encoder = encoderZlibSpec;
    encoderZlibSpec->Create();
    RINOK(props.SetCoderProps(encoderZlibSpec->DeflateEncoderSpec, NULL))
    item.MakeZlib();
  }
  RINOK(item.WriteHeader(outStream))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);
  
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, lps))
  UInt64 inputProcessed;
  if (lzmaMode)
  {
    UInt64 curPos = 0;
    RINOK(outSeekStream->Seek(0, STREAM_SEEK_CUR, &curPos))
    const UInt64 packSize = curPos - kHeaderLzmaSize;
    if (packSize > (UInt32)0xFFFFFFFF)
      return E_INVALIDARG;
    item.MakeLzma((UInt32)packSize);
    RINOK(outSeekStream->Seek(0, STREAM_SEEK_SET, NULL))
    RINOK(item.WriteHeader(outStream))
    inputProcessed = encoderLzmaSpec->GetInputProcessedSize();
  }
  else
  {
    inputProcessed = encoderZlibSpec->GetInputProcessedSize();
  }
  if (inputProcessed + kHeaderBaseSize != size)
    return E_INVALIDARG;
  return updateCallback->SetOperationResult(NUpdate::NOperationResult::kOK);
}

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *timeType))
{
  *timeType = NFileTimeType::kUnix;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback))
{
  if (numItems != 1)
    return E_INVALIDARG;

  Int32 newData, newProps;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive))

  if (IntToBool(newProps))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop))
      if (prop.vt == VT_BOOL)
      {
        if (prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
  }

  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop))
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }
    return UpdateArchive(outStream, size, _lzmaMode, _props, updateCallback);
  }
    
  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (!_seqStream)
    return E_NOTIMPL;

  if (_stream)
  {
    RINOK(InStream_SeekToBegin(_stream))
  }
  else
    _item.WriteHeader(outStream);
  return NCompress::CopyStream(_seqStream, outStream, NULL);
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  _lzmaMode = false;
  RINOK(_props.SetProperties(names, values, numProps))
  const AString &m = _props.MethodName;
  if (m.IsEqualTo_Ascii_NoCase("lzma"))
  {
    return E_NOTIMPL;
    // _lzmaMode = true;
  }
  else if (m.IsEqualTo_Ascii_NoCase("Deflate") || m.IsEmpty())
    _lzmaMode = false;
  else
    return E_INVALIDARG;
  return S_OK;
}

#endif


static const Byte k_Signature[] = {
    3, 'C', 'W', 'S',
    3, 'Z', 'W', 'S' };

REGISTER_ARC_I(
  "SWFc", "swf", "~.swf", 0xD8,
  k_Signature,
  0,
  NArcInfoFlags::kMultiSignature,
  IsArc_Swfc)

}

namespace NSwf {

static const unsigned kNumTagsMax = 1 << 23;

struct CTag
{
  UInt32 Type;
  CByteBuffer Buf;
};


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IArchiveOpenSeq
)
  CObjectVector<CTag> _tags;
  NSwfc::CItem _item;
  UInt64 _phySize;

  HRESULT OpenSeq3(ISequentialInStream *stream, IArchiveOpenCallback *callback);
  HRESULT OpenSeq2(ISequentialInStream *stream, IArchiveOpenCallback *callback);
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidComment,
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: prop = _phySize; break;
    case kpidIsNotArcType: prop = true; break;
  }
  prop.Detach(value);
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _tags.Size();
  return S_OK;
}

static const char * const g_TagDesc[92] =
{
    "End"
  , "ShowFrame"
  , "DefineShape"
  , NULL
  , "PlaceObject"
  , "RemoveObject"
  , "DefineBits"
  , "DefineButton"
  , "JPEGTables"
  , "SetBackgroundColor"
  , "DefineFont"
  , "DefineText"
  , "DoAction"
  , "DefineFontInfo"
  , "DefineSound"
  , "StartSound"
  , NULL
  , "DefineButtonSound"
  , "SoundStreamHead"
  , "SoundStreamBlock"
  , "DefineBitsLossless"
  , "DefineBitsJPEG2"
  , "DefineShape2"
  , "DefineButtonCxform"
  , "Protect"
  , NULL
  , "PlaceObject2"
  , NULL
  , "RemoveObject2"
  , NULL
  , NULL
  , NULL
  , "DefineShape3"
  , "DefineText2"
  , "DefineButton2"
  , "DefineBitsJPEG3"
  , "DefineBitsLossless2"
  , "DefineEditText"
  , NULL
  , "DefineSprite"
  , NULL
  , "41"
  , NULL
  , "FrameLabel"
  , NULL
  , "SoundStreamHead2"
  , "DefineMorphShape"
  , NULL
  , "DefineFont2"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "ExportAssets"
  , "ImportAssets"
  , "EnableDebugger"
  , "DoInitAction"
  , "DefineVideoStream"
  , "VideoFrame"
  , "DefineFontInfo2"
  , NULL
  , "EnableDebugger2"
  , "ScriptLimits"
  , "SetTabIndex"
  , NULL
  , NULL
  , "FileAttributes"
  , "PlaceObject3"
  , "ImportAssets2"
  , NULL
  , "DefineFontAlignZones"
  , "CSMTextSettings"
  , "DefineFont3"
  , "SymbolClass"
  , "Metadata"
  , "DefineScalingGrid"
  , NULL
  , NULL
  , NULL
  , "DoABC"
  , "DefineShape4"
  , "DefineMorphShape2"
  , NULL
  , "DefineSceneAndFrameLabelData"
  , "DefineBinaryData"
  , "DefineFontName"
  , "StartSound2"
  , "DefineBitsJPEG4"
  , "DefineFont4"
};

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  const CTag &tag = _tags[index];
  switch (propID)
  {
    case kpidPath:
    {
      char s[32];
      ConvertUInt32ToString(index, s);
      size_t i = strlen(s);
      s[i++] = '.';
      ConvertUInt32ToString(tag.Type, s + i);
      prop = s;
      break;
    }
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)tag.Buf.Size(); break;
    case kpidComment:
      TYPE_TO_PROP(g_TagDesc, tag.Type, prop);
      break;
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  return OpenSeq2(stream, callback);
}

static UInt16 Read16(CInBuffer &stream)
{
  UInt32 res = 0;
  for (unsigned i = 0; i < 2; i++)
  {
    Byte b;
    if (!stream.ReadByte(b))
      throw 1;
    res |= (UInt32)b << (i * 8);
  }
  return (UInt16)res;
}

static UInt32 Read32(CInBuffer &stream)
{
  UInt32 res = 0;
  for (unsigned i = 0; i < 4; i++)
  {
    Byte b;
    if (!stream.ReadByte(b))
      throw 1;
    res |= (UInt32)b << (i * 8);
  }
  return res;
}

struct CBitReader
{
  CInBuffer *stream;
  unsigned NumBits;
  Byte Val;

  CBitReader(): NumBits(0), Val(0) {}

  UInt32 ReadBits(unsigned numBits);
};

UInt32 CBitReader::ReadBits(unsigned numBits)
{
  UInt32 res = 0;
  while (numBits > 0)
  {
    if (NumBits == 0)
    {
      Val = stream->ReadByte();
      NumBits = 8;
    }
    if (numBits <= NumBits)
    {
      res <<= numBits;
      NumBits -= numBits;
      res |= (Val >> NumBits);
      Val = (Byte)(Val & (((unsigned)1 << NumBits) - 1));
      break;
    }
    else
    {
      res <<= NumBits;
      res |= Val;
      numBits -= NumBits;
      NumBits = 0;
    }
  }
  return res;
}

HRESULT CHandler::OpenSeq3(ISequentialInStream *stream, IArchiveOpenCallback *callback)
{
  RINOK(_item.ReadHeader(stream))
  if (!_item.IsSwf() || !_item.IsUncompressed())
    return S_FALSE;
  const UInt32 uncompressedSize = _item.GetSize();
  if (uncompressedSize > kFileSizeMax)
    return S_FALSE;

  
  CInBuffer s;
  if (!s.Create(1 << 20))
    return E_OUTOFMEMORY;
  s.SetStream(stream);
  s.Init();
  {
    CBitReader br;
    br.stream = &s;
    const unsigned numBits = br.ReadBits(5);
    /* UInt32 xMin = */ br.ReadBits(numBits);
    /* UInt32 xMax = */ br.ReadBits(numBits);
    /* UInt32 yMin = */ br.ReadBits(numBits);
    /* UInt32 yMax = */ br.ReadBits(numBits);
  }
  /* UInt32 frameDelay = */ Read16(s);
  /* UInt32 numFrames =  */ Read16(s);

  _tags.Clear();
  UInt64 offsetPrev = 0;
  for (;;)
  {
    const UInt32 pair = Read16(s);
    const UInt32 type = pair >> 6;
    UInt32 length = pair & 0x3F;
    if (length == 0x3F)
      length = Read32(s);
    if (type == 0)
      break;
    const UInt64 offset = s.GetProcessedSize() + NSwfc::kHeaderBaseSize + length;
    if (offset > uncompressedSize || _tags.Size() >= kNumTagsMax)
      return S_FALSE;
    CTag &tag = _tags.AddNew();
    tag.Type = type;
    tag.Buf.Alloc(length);
    if (s.ReadBytes(tag.Buf, length) != length)
      return S_FALSE;
    if (callback && offset >= offsetPrev + (1 << 20))
    {
      const UInt64 numItems = _tags.Size();
      RINOK(callback->SetCompleted(&numItems, &offset))
      offsetPrev = offset;
    }
  }
  _phySize = s.GetProcessedSize() + NSwfc::kHeaderBaseSize;
  if (_phySize != uncompressedSize)
  {
    // do we need to support files extracted from SFW-LZMA with additional 8 bytes?
    return S_FALSE;
  }
  return S_OK;
}

HRESULT CHandler::OpenSeq2(ISequentialInStream *stream, IArchiveOpenCallback *callback)
{
  HRESULT res;
  try { res = OpenSeq3(stream, callback); }
  catch(...) { res = S_FALSE; }
  return res;
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  return OpenSeq2(stream, NULL);
}

Z7_COM7F_IMF(CHandler::Close())
{
  _phySize = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _tags.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _tags[allFilesMode ? i : indices[i]].Buf.Size();
  RINOK(extractCallback->SetTotal(totalSize))

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  totalSize = 0;

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CByteBuffer &buf = _tags[index].Buf;
    totalSize += buf.Size();

    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode))
    if (!testMode && !outStream)
      continue;
      
    RINOK(extractCallback->PrepareOperation(askMode))
    if (outStream)
      RINOK(WriteStream(outStream, buf, buf.Size()))
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
  }
  return S_OK;
  COM_TRY_END
}

static const Byte k_Signature[] = { 'F', 'W', 'S' };

REGISTER_ARC_I(
  "SWF", "swf", NULL, 0xD7,
  k_Signature,
  0,
  NArcInfoFlags::kKeepName,
  NSwfc::IsArc_Swf)

}}

/* ================ unit: CPP/7zip/Archive/UefiHandler.cpp ================ */
// UefiHandler.cpp

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
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

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)
#define Get24(p) (Get32(p) & 0xFFFFFF)

namespace NArchive {
namespace NUefi {

static const size_t kBufTotalSizeMax = 1 << 29;
static const unsigned kNumFilesMax = 1 << 18;
static const unsigned kLevelMax = 64;

static const Byte k_IntelMeSignature[] =
{
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x5A, 0xA5, 0xF0, 0x0F
};

static bool IsIntelMe(const Byte *p)
{
  return memcmp(p, k_IntelMeSignature, sizeof(k_IntelMeSignature)) == 0;
}

static const unsigned kFvHeaderSize = 0x38;

static const unsigned kGuidSize = 16;

#define CAPSULE_SIGNATURE   0xBD,0x86,0x66,0x3B,0x76,0x0D,0x30,0x40,0xB7,0x0E,0xB5,0x51,0x9E,0x2F,0xC5,0xA0
#define CAPSULE2_SIGNATURE  0x8B,0xA6,0x3C,0x4A,0x23,0x77,0xFB,0x48,0x80,0x3D,0x57,0x8C,0xC1,0xFE,0xC4,0x4D
#define CAPSULE_UEFI_SIGNATURE  0xB9,0x82,0x91,0x53,0xB5,0xAB,0x91,0x43,0xB6,0x9A,0xE3,0xA9,0x43,0xF7,0x2F,0xCC
/*
  6dcbd5ed-e82d-4c44-bda1-7194199ad92a : Firmware Management `
*/

static const Byte k_Guids_Capsules[][kGuidSize] =
{
  { CAPSULE_SIGNATURE },
  { CAPSULE2_SIGNATURE },
  { CAPSULE_UEFI_SIGNATURE }
};


static const unsigned kFfsGuidOffset = 16;

#define FFS1_SIGNATURE  0xD9,0x54,0x93,0x7A,0x68,0x04,0x4A,0x44,0x81,0xCE,0x0B,0xF6,0x17,0xD8,0x90,0xDF
#define FFS2_SIGNATURE  0x78,0xE5,0x8C,0x8C,0x3D,0x8A,0x1C,0x4F,0x99,0x35,0x89,0x61,0x85,0xC3,0x2D,0xD3
#define MACFS_SIGNATURE 0xAD,0xEE,0xAD,0x04,0xFF,0x61,0x31,0x4D,0xB6,0xBA,0x64,0xF8,0xBF,0x90,0x1F,0x5A
// APPLE_BOOT
/*
  "FFS3":        "5473c07a-3dcb-4dca-bd6f-1e9689e7349a",
  "NVRAM_EVSA":  "fff12b8d-7696-4c8b-a985-2747075b4f50",
  "NVRAM_NVAR":  "cef5b9a3-476d-497f-9fdc-e98143e0422c",
  "NVRAM_EVSA2": "00504624-8a59-4eeb-bd0f-6b36e96128e0",
static const Byte k_NVRAM_NVAR_Guid[kGuidSize] =
  { 0xA3,0xB9,0xF5,0xCE,0x6D,0x47,0x7F,0x49,0x9F,0xDC,0xE9,0x81,0x43,0xE0,0x42,0x2C };
*/

static const Byte k_Guids_FS[][kGuidSize] =
{
  { FFS1_SIGNATURE },
  { FFS2_SIGNATURE },
  { MACFS_SIGNATURE }
};


static const UInt32 kFvSignature = 0x4856465F; // "_FVH"

static const Byte kGuids[][kGuidSize] =
{
  { 0xB0,0xCD,0x1B,0xFC,0x31,0x7D,0xAA,0x49,0x93,0x6A,0xA4,0x60,0x0D,0x9D,0xD0,0x83 },
  { 0x2E,0x06,0xA0,0x1B,0x79,0xC7,0x82,0x45,0x85,0x66,0x33,0x6A,0xE8,0xF7,0x8F,0x09 },
  { 0x25,0x4E,0x37,0x7E,0x01,0x8E,0xEE,0x4F,0x87,0xf2,0x39,0x0C,0x23,0xC6,0x06,0xCD },
  { 0x97,0xE5,0x1B,0x16,0xC5,0xE9,0xDB,0x49,0xAE,0x50,0xC4,0x62,0xAB,0x54,0xEE,0xDA },
  { 0xDB,0x7F,0xAD,0x77,0x2A,0xDF,0x02,0x43,0x88,0x98,0xC7,0x2E,0x4C,0xDB,0xD0,0xF4 },
  { 0xAB,0x71,0xCF,0xF5,0x4B,0xB0,0x7E,0x4B,0x98,0x8A,0xD8,0xA0,0xD4,0x98,0xE6,0x92 },
  { 0x91,0x45,0x53,0x7A,0xCE,0x37,0x81,0x48,0xB3,0xC9,0x71,0x38,0x14,0xF4,0x5D,0x6B },
  { 0x84,0xE6,0x7A,0x36,0x5D,0x33,0x71,0x46,0xA1,0x6D,0x89,0x9D,0xBF,0xEA,0x6B,0x88 },
  { 0x98,0x07,0x40,0x24,0x07,0x38,0x42,0x4A,0xB4,0x13,0xA1,0xEC,0xEE,0x20,0x5D,0xD8 },
  { 0xEE,0xA2,0x3F,0x28,0x2C,0x53,0x4D,0x48,0x93,0x83,0x9F,0x93,0xB3,0x6F,0x0B,0x7E },
  { 0x9B,0xD5,0xB8,0x98,0xBA,0xE8,0xEE,0x48,0x98,0xDD,0xC2,0x95,0x39,0x2F,0x1E,0xDB },
  { 0x09,0x6D,0xE3,0xC3,0x94,0x82,0x97,0x4B,0xA8,0x57,0xD5,0x28,0x8F,0xE3,0x3E,0x28 },
  { 0x18,0x88,0x53,0x4A,0xE0,0x5A,0xB2,0x4E,0xB2,0xEB,0x48,0x8B,0x23,0x65,0x70,0x22 }
};

static const Byte k_Guid_LZMA_COMPRESSED[kGuidSize] =
  { 0x98,0x58,0x4E,0xEE,0x14,0x39,0x59,0x42,0x9D,0x6E,0xDC,0x7B,0xD7,0x94,0x03,0xCF };

static const char * const kGuidNames[] =
{
    "CRC"
  , "VolumeTopFile"
  , "ACPI"
  , "ACPI2"
  , "Main"
  , "Intel32"
  , "Intel64"
  , "Intel32c"
  , "Intel64c"
  , "MacVolume"
  , "MacUpdate.txt"
  , "MacName"
  , "Insyde"
};

enum
{
  kGuidIndex_CRC = 0
};

struct CSigExtPair
{
  const char *ext;
  unsigned sigSize;
  Byte sig[16];
};

static const CSigExtPair g_Sigs[] =
{
  { "bmp",  2, { 'B','M' } },
  { "riff", 4, { 'R','I','F','F' } },
  { "pe",   2, { 'M','Z'} },
  { "gif",  6, { 'G','I','F','8','9', 'a' } },
  { "png",  8, { 0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A } },
  { "jpg", 10, { 0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46 } },
  { "rom",  2, { 0x55,0xAA } }
};

enum
{
  kSig_BMP,
  kSig_RIFF,
  kSig_PE
};

static const char *FindExt(const Byte *p, size_t size)
{
  unsigned i;
  for (i = 0; i < Z7_ARRAY_SIZE(g_Sigs); i++)
  {
    const CSigExtPair &pair = g_Sigs[i];
    if (size >= pair.sigSize)
      if (memcmp(p, pair.sig, pair.sigSize) == 0)
        break;
  }
  if (i == Z7_ARRAY_SIZE(g_Sigs))
    return NULL;
  switch (i)
  {
    case kSig_BMP:
      if (GetUi32(p + 2) > size || GetUi32(p + 0xA) > size)
        return NULL;
      break;
    case kSig_RIFF:
      if (GetUi32(p + 8) == 0x45564157 || GetUi32(p + 0xC) == 0x20746D66 )
        return "wav";
      break;
    case kSig_PE:
    {
      if (size < 512)
        return NULL;
      UInt32 peOffset = GetUi32(p + 0x3C);
      if (peOffset >= 0x1000 || peOffset + 512 > size || (peOffset & 7) != 0)
        return NULL;
      if (GetUi32(p + peOffset) != 0x00004550)
        return NULL;
      break;
    }
  }
  return g_Sigs[i].ext;
}

static bool AreGuidsEq(const Byte *p1, const Byte *p2)
{
  return memcmp(p1, p2, kGuidSize) == 0;
}

static int FindGuid(const Byte *p)
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(kGuids); i++)
    if (AreGuidsEq(p, kGuids[i]))
      return (int)i;
  return -1;
}
 
static bool IsFfs(const Byte *p)
{
  if (Get32(p + 0x28) != kFvSignature)
    return false;
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_Guids_FS); i++)
    if (AreGuidsEq(p + kFfsGuidOffset, k_Guids_FS[i]))
      return true;
  return false;
}

#define FVB_ERASE_POLARITY  (1 << 11)

/*
static const CUInt32PCharPair g_FV_Attribs[] =
{
  {  0, "ReadDisabledCap" },
  {  1, "ReadEnabledCap" },
  {  2, "ReadEnabled" },
  {  3, "WriteDisabledCap" },
  {  4, "WriteEnabledCap" },
  {  5, "WriteEnabled" },
  {  6, "LockCap" },
  {  7, "Locked" },

  {  9, "StickyWrite" },
  { 10, "MemoryMapped" },
  { 11, "ErasePolarity" },
  
  { 12, "ReadLockCap" },
  { 13, "WriteLockCap" },
  { 14, "WriteLockCap" }
};
*/

enum
{
  FV_FILETYPE_ALL,
  FV_FILETYPE_RAW,
  FV_FILETYPE_FREEFORM,
  FV_FILETYPE_SECURITY_CORE,
  FV_FILETYPE_PEI_CORE,
  FV_FILETYPE_DXE_CORE,
  FV_FILETYPE_PEIM,
  FV_FILETYPE_DRIVER,
  FV_FILETYPE_COMBINED_PEIM_DRIVER,
  FV_FILETYPE_APPLICATION,
  // The value 0x0A is reserved and should not be used
  FV_FILETYPE_FIRMWARE_VOLUME_IMAGE = 0x0B,
  // types 0xF0 - 0xFF are FFS file types
  FV_FILETYPE_FFS_PAD = 0xF0
};

static const char * const g_FileTypes[] =
{
    "ALL"
  , "RAW"
  , "FREEFORM"
  , "SECURITY_CORE"
  , "PEI_CORE"
  , "DXE_CORE"
  , "PEIM"
  , "DRIVER"
  , "COMBINED_PEIM_DRIVER"
  , "APPLICATION"
  , "0xA"
  , "VOLUME"
};

// typedef Byte FFS_FILE_ATTRIBUTES;
// FFS File Attributes
#define FFS_ATTRIB_TAIL_PRESENT 0x01
// #define FFS_ATTRIB_RECOVERY 0x02
// #define FFS_ATTRIB_HEADER_EXTENSION 0x04
// #define FFS_ATTRIB_DATA_ALIGNMENT 0x38
#define FFS_ATTRIB_CHECKSUM 0x40

static const CUInt32PCharPair g_FFS_FILE_ATTRIBUTES[] =
{
  { 0, "" /* "TAIL" */ },
  { 1, "RECOVERY" },
  // { 2, "HEADER_EXTENSION" }, // reserved for future
  { 6, "" /* "CHECKSUM" */ }
};

// static const Byte g_Allignment[8] = { 3, 4, 7, 9, 10, 12, 15, 16 };

// typedef Byte FFS_FILE_STATE;

// Look also FVB_ERASE_POLARITY.
// Lower-order State bits are superceded by higher-order State bits.

// #define FILE_HEADER_CONSTRUCTION  0x01
// #define FILE_HEADER_VALID         0x02
#define FILE_DATA_VALID           0x04
// #define FILE_MARKED_FOR_UPDATE    0x08
// #define FILE_DELETED              0x10
// #define FILE_HEADER_INVALID       0x20

// SECTION_TYPE

// #define SECTION_ALL 0x00

#define SECTION_COMPRESSION  0x01
#define SECTION_GUID_DEFINED 0x02

// Leaf section Type values
// #define SECTION_PE32      0x10
// #define SECTION_PIC       0x11
// #define SECTION_TE        0x12
#define SECTION_DXE_DEPEX 0x13
#define SECTION_VERSION   0x14
#define SECTION_USER_INTERFACE 0x15
// #define SECTION_COMPATIBILITY16 0x16
#define SECTION_FIRMWARE_VOLUME_IMAGE 0x17
#define SECTION_FREEFORM_SUBTYPE_GUID 0x18
#define SECTION_RAW       0x19
#define SECTION_PEI_DEPEX 0x1B


// #define GUIDED_SECTION_PROCESSING_REQUIRED 0x01
// #define GUIDED_SECTION_AUTH_STATUS_VALID 0x02

static const CUInt32PCharPair g_GUIDED_SECTION_ATTRIBUTES[] =
{
  { 0, "PROCESSING_REQUIRED" },
  { 1, "AUTH" }
};

static const CUInt32PCharPair g_SECTION_TYPE[] =
{
  { 0x01, "COMPRESSION" },
  { 0x02, "GUID" },
  { 0x10, "efi" },
  { 0x11, "PIC" },
  { 0x12, "te" },
  { 0x13, "DXE_DEPEX" },
  { 0x14, "VERSION" },
  { 0x15, "USER_INTERFACE" },
  { 0x16, "COMPATIBILITY16" },
  { 0x17, "VOLUME" },
  { 0x18, "FREEFORM_SUBTYPE_GUID" },
  { 0x19, "raw" },
  { 0x1B, "PEI_DEPEX" }
};

#define COMPRESSION_TYPE_NONE 0
#define COMPRESSION_TYPE_LZH  1
#define COMPRESSION_TYPE_LZMA 2

static const char * const g_Methods[] =
{
    "COPY"
  , "LZH"
  , "LZMA"
};


static void AddGuid(AString &dest, const Byte *p, bool full)
{
  char s[64];
  ::RawLeGuidToString(p, s);
  // MyStringUpper_Ascii(s);
  if (!full)
    s[8] = 0;
  dest += s;
}

static const char * const kExpressionCommands[] =
{
  "BEFORE", "AFTER", "PUSH", "AND", "OR", "NOT", "TRUE", "FALSE", "END", "SOR"
};

static bool ParseDepedencyExpression(const Byte *p, UInt32 size, AString &res)
{
  res.Empty();
  for (UInt32 i = 0; i < size;)
  {
    const unsigned command = p[i++];
    if (command >= Z7_ARRAY_SIZE(kExpressionCommands))
      return false;
    res += kExpressionCommands[command];
    if (command < 3)
    {
      if (i + kGuidSize > size)
        return false;
      res.Add_Space();
      AddGuid(res, p + i, false);
      i += kGuidSize;
    }
    res += "; ";
  }
  return true;
}

static bool ParseUtf16zString(const Byte *p, UInt32 size, UString &res)
{
  if ((size & 1) != 0)
    return false;
  res.Empty();
  UInt32 i;
  for (i = 0; i < size; i += 2)
  {
    wchar_t c = Get16(p + i);
    if (c == 0)
      break;
    res += c;
  }
  return (i == size - 2);
}

static bool ParseUtf16zString2(const Byte *p, UInt32 size, AString &res)
{
  UString s;
  if (!ParseUtf16zString(p, size, s))
    return false;
  res = UnicodeStringToMultiByte(s);
  return true;
}

#define FLAGS_TO_STRING(pairs, value) FlagsToString(pairs, Z7_ARRAY_SIZE(pairs), value)
#define TYPE_TO_STRING(table, value) TypeToString(table, Z7_ARRAY_SIZE(table), value)
#define TYPE_PAIR_TO_STRING(table, value) TypePairToString(table, Z7_ARRAY_SIZE(table), value)

static const UInt32 kFileHeaderSize = 24;

static void AddSpaceAndString(AString &res, const AString &newString)
{
  if (!newString.IsEmpty())
  {
    res.Add_Space_if_NotEmpty();
    res += newString;
  }
}

class CFfsFileHeader
{
PRF(public:)
  Byte CheckHeader;
  Byte CheckFile;
  Byte Attrib;
  Byte State;

  UInt16 GetTailReference() const { return (UInt16)(CheckHeader | ((UInt16)CheckFile << 8)); }
  UInt32 GetTailSize() const { return IsThereTail() ? 2 : 0; }
  bool IsThereFileChecksum() const { return (Attrib & FFS_ATTRIB_CHECKSUM) != 0; }
  bool IsThereTail() const { return (Attrib & FFS_ATTRIB_TAIL_PRESENT) != 0; }
public:
  Byte GuidName[kGuidSize];
  Byte Type;
  UInt32 Size;
  
  bool Parse(const Byte *p)
  {
    unsigned i;
    for (i = 0; i < kFileHeaderSize; i++)
      if (p[i] != 0xFF)
        break;
    if (i == kFileHeaderSize)
      return false;
    memcpy(GuidName, p, kGuidSize);
    CheckHeader = p[0x10];
    CheckFile = p[0x11];
    Type = p[0x12];
    Attrib = p[0x13];
    Size = Get24(p + 0x14);
    State = p[0x17];
    return true;
  }

  UInt32 GetDataSize() const { return Size - kFileHeaderSize - GetTailSize(); }
  UInt32 GetDataSize2(UInt32 rem) const { return rem - kFileHeaderSize - GetTailSize(); }

  bool Check(const Byte *p, UInt32 size)
  {
    if (Size > size)
      return false;
    UInt32 tailSize = GetTailSize();
    if (Size < kFileHeaderSize + tailSize)
      return false;
    
    {
      unsigned checkSum = 0;
      for (UInt32 i = 0; i < kFileHeaderSize; i++)
        checkSum += p[i];
      checkSum -= p[0x17];
      checkSum -= p[0x11];
      if ((Byte)checkSum != 0)
        return false;
    }
    
    if (IsThereFileChecksum())
    {
      unsigned checkSum = 0;
      UInt32 checkSize = Size - tailSize;
      for (UInt32 i = 0; i < checkSize; i++)
        checkSum += p[i];
      checkSum -= p[0x17];
      if ((Byte)checkSum != 0)
        return false;
    }
    
    if (IsThereTail())
      if (GetTailReference() != (UInt16)~Get16(p + Size - 2))
        return false;

    int polarity = 0;
    int i;
    for (i = 5; i >= 0; i--)
      if (((State >> i) & 1) == polarity)
      {
        // AddSpaceAndString(s, g_FFS_FILE_STATE_Flags[i]);
        if ((1 << i) != FILE_DATA_VALID)
          return false;
        break;
      }
    if (i < 0)
      return false;

    return true;
  }

  AString GetCharacts() const
  {
    AString s;
    if (Type == FV_FILETYPE_FFS_PAD)
      s += "PAD";
    else
      s += TYPE_TO_STRING(g_FileTypes, Type);
    AddSpaceAndString(s, FLAGS_TO_STRING(g_FFS_FILE_ATTRIBUTES, Attrib & 0xC7));
    /*
    int align = (Attrib >> 3) & 7;
    if (align != 0)
    {
      s += " Align:";
      s.Add_UInt32((UInt32)1 << g_Allignment[align]);
    }
    */
    return s;
  }
};

#define G32(_offs_, dest) dest = Get32(p + (_offs_))
#define G16(_offs_, dest) dest = Get16(p + (_offs_))

struct CCapsuleHeader
{
  UInt32 HeaderSize;
  UInt32 Flags;
  UInt32 CapsuleImageSize;
  UInt32 SequenceNumber;
  // Guid InstanceId;
  UInt32 OffsetToSplitInformation;
  UInt32 OffsetToCapsuleBody;
  UInt32 OffsetToOemDefinedHeader;
  UInt32 OffsetToAuthorInformation;
  UInt32 OffsetToRevisionInformation;
  UInt32 OffsetToShortDescription;
  UInt32 OffsetToLongDescription;
  UInt32 OffsetToApplicableDevices;

  void Clear() { memset(this, 0, sizeof(*this)); }

  bool Parse(const Byte *p)
  {
    Clear();
    G32(0x10, HeaderSize);
    G32(0x14, Flags);
    G32(0x18, CapsuleImageSize);
    if (HeaderSize < 0x1C)
      return false;
    if (AreGuidsEq(p, k_Guids_Capsules[0]))
    {
      const unsigned kHeaderSize = 80;
      if (HeaderSize != kHeaderSize)
        return false;
      G32(0x1C, SequenceNumber);
      G32(0x30, OffsetToSplitInformation);
      G32(0x34, OffsetToCapsuleBody);
      G32(0x38, OffsetToOemDefinedHeader);
      G32(0x3C, OffsetToAuthorInformation);
      G32(0x40, OffsetToRevisionInformation);
      G32(0x44, OffsetToShortDescription);
      G32(0x48, OffsetToLongDescription);
      G32(0x4C, OffsetToApplicableDevices);
      return true;
    }
    else if (AreGuidsEq(p, k_Guids_Capsules[1]))
    {
      // capsule v2
      G16(0x1C, OffsetToCapsuleBody);
      G16(0x1E, OffsetToOemDefinedHeader);
      return true;
    }
    else if (AreGuidsEq(p, k_Guids_Capsules[2]))
    {
      OffsetToCapsuleBody = HeaderSize;
      return true;
    }
    else
    {
      // here we must check for another capsule types
      return false;
    }
  }
};


struct CItem
{
  AString Name;
  AString Characts;
  int Parent;
  int Method;
  int NameIndex;
  unsigned NumChilds;
  bool IsDir;
  bool Skip;
  bool ThereAreSubDirs;
  bool ThereIsUniqueName;
  bool KeepName;

  unsigned BufIndex;
  UInt32 Offset;
  UInt32 Size;

  CItem(): Parent(-1), Method(-1), NameIndex(-1), NumChilds(0),
      IsDir(false), Skip(false), ThereAreSubDirs(false), ThereIsUniqueName(false), KeepName(true) {}
  void SetGuid(const Byte *guidName, bool full = false);
  AString GetName(int numChildsInParent) const;
};

void CItem::SetGuid(const Byte *guidName, bool full)
{
  ThereIsUniqueName = true;
  int index = FindGuid(guidName);
  if (index >= 0)
    Name = kGuidNames[(unsigned)index];
  else
  {
    Name.Empty();
    AddGuid(Name, guidName, full);
  }
}

AString CItem::GetName(int numChildsInParent) const
{
  if (numChildsInParent <= 1 || NameIndex < 0)
    return Name;
  char sz[32];
  char sz2[32];
  ConvertUInt32ToString((unsigned)NameIndex, sz);
  ConvertUInt32ToString((unsigned)numChildsInParent - 1, sz2);
  const int numZeros = (int)strlen(sz2) - (int)strlen(sz);
  AString res;
  for (int i = 0; i < numZeros; i++)
    res.Add_Char('0');
  res += sz;
  res.Add_Dot();
  res += Name;
  return res;
}

struct CItem2
{
  AString Name;
  AString Characts;
  unsigned MainIndex;
  int Parent;

  CItem2(): Parent(-1) {}
};


Z7_CLASS_IMP_CHandler_IInArchive_1(
    IInArchiveGetStream
)
  CObjectVector<CItem> _items;
  CObjectVector<CItem2> _items2;
  CObjectVector<CByteBuffer> _bufs;
  UString _comment;
  UInt32 _methodsMask;
  bool _capsuleMode;
  bool _headersError;

  size_t _totalBufsSize;
  CCapsuleHeader _h;
  UInt64 _phySize;

  void AddCommentString(const char *name, UInt32 pos);
  unsigned AddItem(const CItem &item);
  unsigned AddFileItemWithIndex(CItem &item);
  unsigned AddDirItem(CItem &item);
  unsigned AddBuf(size_t size);

  HRESULT DecodeLzma(const Byte *data, size_t inputSize);

  HRESULT ParseSections(unsigned bufIndex, UInt32 pos,
      UInt32 size,
      int parent, int method, unsigned level,
      bool &error);
  
  HRESULT ParseIntelMe(unsigned bufIndex, UInt32 posBase,
      UInt32 exactSize, UInt32 limitSize,
      int parent, int method, unsigned level);

  HRESULT ParseVolume(unsigned bufIndex, UInt32 posBase,
      UInt32 exactSize, UInt32 limitSize,
      int parent, int method, unsigned level);

  HRESULT OpenCapsule(IInStream *stream);
  HRESULT OpenFv(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback);
  HRESULT Open2(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback);
public:
  CHandler(bool capsuleMode): _capsuleMode(capsuleMode) {}
};


static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  // kpidOffset,
  kpidMethod,
  kpidCharacts
};

static const Byte kArcProps[] =
{
  kpidComment,
  kpidMethod,
  kpidCharacts
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem2 &item2 = _items2[index];
  const CItem &item = _items[item2.MainIndex];
  switch (propID)
  {
    case kpidPath:
    {
      AString path (item2.Name);
      int cur = item2.Parent;
      while (cur >= 0)
      {
        const CItem2 &item3 = _items2[cur];
        path.InsertAtFront(CHAR_PATH_SEPARATOR);
        path.Insert(0, item3.Name);
        cur = item3.Parent;
      }
      prop = path;
      break;
    }
    case kpidIsDir: prop = item.IsDir; break;
    case kpidMethod: if (item.Method >= 0) prop = g_Methods[(unsigned)item.Method]; break;
    case kpidCharacts: if (!item2.Characts.IsEmpty()) prop = item2.Characts; break;
    case kpidSize: if (!item.IsDir) prop = (UInt64)item.Size; break;
    // case kpidOffset: if (!item.IsDir) prop = item.Offset; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

void CHandler::AddCommentString(const char *name, UInt32 pos)
{
  UString s;
  if (pos < _h.HeaderSize)
    return;
  if (pos >= _h.OffsetToCapsuleBody)
    return;
  UInt32 limit = (_h.OffsetToCapsuleBody - pos) & ~(UInt32)1;
  const Byte *buf = _bufs[0] + pos;
  for (UInt32 i = 0;;)
  {
    if (s.Len() > (1 << 16) || i >= limit)
      return;
    wchar_t c = Get16(buf + i);
    i += 2;
    if (c == 0)
    {
      if (i >= limit)
        return;
      c = Get16(buf + i);
      i += 2;
      if (c == 0)
        break;
      s.Add_LF();
    }
    s += c;
  }
  if (s.IsEmpty())
    return;
  _comment.Add_LF();
  _comment += name;
  _comment += ": ";
  _comment += s;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMethod:
    {
      AString s;
      for (unsigned i = 0; i < 32; i++)
        if ((_methodsMask & ((UInt32)1 << i)) != 0)
          AddSpaceAndString(s, (AString)g_Methods[i]);
      if (!s.IsEmpty())
        prop = s;
      break;
    }
    case kpidComment: if (!_comment.IsEmpty()) prop = _comment; break;
    case kpidPhySize: prop = (UInt64)_phySize; break;

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (_headersError) v |= kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

#ifdef SHOW_DEBUG_INFO
static void PrintLevel(unsigned level)
{
  PRF(printf("\n"));
  for (unsigned i = 0; i < level; i++)
    PRF(printf("  "));
}
static void MyPrint(UInt32 posBase, UInt32 size, unsigned level, const char *name)
{
  PrintLevel(level);
  PRF(printf("%s, pos = %6x, size = %6x", name, posBase, size));
}
#else
#define PrintLevel(level)
#define MyPrint(posBase, size, level, name)
#endif



unsigned CHandler::AddItem(const CItem &item)
{
  if (_items.Size() >= kNumFilesMax)
    throw 2;
  return _items.Add(item);
}

unsigned CHandler::AddFileItemWithIndex(CItem &item)
{
  unsigned nameIndex = _items.Size();
  if (item.Parent >= 0)
    nameIndex = _items[item.Parent].NumChilds++;
  item.NameIndex = (int)nameIndex;
  return AddItem(item);
}

unsigned CHandler::AddDirItem(CItem &item)
{
  if (item.Parent >= 0)
    _items[item.Parent].ThereAreSubDirs = true;
  item.IsDir = true;
  item.Size = 0;
  return AddItem(item);
}

unsigned CHandler::AddBuf(size_t size)
{
  if (size > kBufTotalSizeMax - _totalBufsSize)
    throw 1;
  _totalBufsSize += size;
  unsigned index = _bufs.Size();
  _bufs.AddNew().Alloc(size);
  return index;
}


HRESULT CHandler::DecodeLzma(const Byte *data, size_t inputSize)
{
  if (inputSize < 5 + 8)
    return S_FALSE;
  const UInt64 unpackSize = Get64(data + 5);
  if (unpackSize > ((UInt32)1 << 30))
    return S_FALSE;
  SizeT destLen = (SizeT)unpackSize;
  const unsigned newBufIndex = AddBuf((size_t)unpackSize);
  CByteBuffer &buf = _bufs[newBufIndex];
  ELzmaStatus status;
  SizeT srcLen = inputSize - (5 + 8);
  const SizeT srcLen2 = srcLen;
  SRes res = LzmaDecode(buf, &destLen, data + 13, &srcLen,
      data, 5, LZMA_FINISH_END, &status, &g_Alloc);
  if (res != 0)
    return S_FALSE;
  if (srcLen != srcLen2 || destLen != unpackSize || (
      status != LZMA_STATUS_FINISHED_WITH_MARK &&
      status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK))
    return S_FALSE;
  return S_OK;
}


HRESULT CHandler::ParseSections(unsigned bufIndex, UInt32 posBase, UInt32 size, int parent, int method, unsigned level, bool &error)
{
  error = false;

  if (level > kLevelMax)
    return S_FALSE;
  MyPrint(posBase, size, level, "Sections");
  level++;
  const Byte *bufData = _bufs[bufIndex];
  UInt32 pos = 0;
  for (;;)
  {
    if (size == pos)
      return S_OK;
    PrintLevel(level);
    PRF(printf("%s, abs = %6x, relat = %6x", "Sect", posBase + pos, pos));
    pos = (pos + 3) & ~(UInt32)3;
    if (pos > size)
      return S_FALSE;
    const UInt32 rem = size - pos;
    if (rem == 0)
      return S_OK;
    if (rem < 4)
      return S_FALSE;
    
    const Byte *p = bufData + posBase + pos;
    
    const UInt32 sectSize = Get24(p);
    const Byte type = p[3];
    
    // PrintLevel(level);
    PRF(printf(" type = %2x, sectSize = %6x", type, sectSize));

    if (sectSize > rem || sectSize < 4)
    {
      _headersError = true;
      error = true;
      return S_OK;
      // return S_FALSE;
    }

    CItem item;
    item.Method = method;
    item.BufIndex = bufIndex;
    item.Parent = parent;
    item.Offset = posBase + pos + 4;
    const UInt32 sectDataSize = sectSize - 4;
    item.Size = sectDataSize;
    item.Name = TYPE_PAIR_TO_STRING(g_SECTION_TYPE, type);

    if (type == SECTION_COMPRESSION)
    {
      if (sectSize < 4 + 5)
        return S_FALSE;
      const UInt32 uncompressedSize = Get32(p + 4);
      const Byte compressionType = p[8];

      UInt32 newSectSize = sectSize - 9;
      UInt32 newOffset = posBase + pos + 9;
      const Byte *pStart = p + 9;

      item.KeepName = false;
      if (compressionType > 2)
      {
        // AddFileItemWithIndex(item);
        return S_FALSE;
      }
      else
      {
        item.Name = g_Methods[compressionType];
        // int parent = AddDirItem(item);
        if (compressionType == COMPRESSION_TYPE_NONE)
        {
          bool error2;
          RINOK(ParseSections(bufIndex, newOffset, newSectSize, parent, method, level, error2))
        }
        else if (compressionType == COMPRESSION_TYPE_LZH)
        {
          const unsigned newBufIndex = AddBuf(uncompressedSize);
          CByteBuffer &buf = _bufs[newBufIndex];
          CMyUniquePtr<NCompress::NLzh::NDecoder::CCoder> lzhDecoder;
          lzhDecoder.Create_if_Empty();
          {
            const Byte *src = pStart;
            if (newSectSize < 8)
              return S_FALSE;
            UInt32 packSize = Get32(src);
            const UInt32 unpackSize = Get32(src + 4);

            PRF(printf(" LZH packSize = %6x, unpackSize = %6x", packSize, unpackSize));

            if (uncompressedSize != unpackSize || newSectSize - 8 != packSize)
              return S_FALSE;
            if (packSize < 1)
              return S_FALSE;
            packSize--;
            src += 8;
            if (src[packSize] != 0)
              return S_FALSE;

            CMyComPtr2_Create<IInStream, CBufInStream> inStream;
            CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> outStream;
            // const UInt64 uncompressedSize64 = uncompressedSize;
            // lzhDecoder->FinishMode = true;
            /*
              EFI 1.1 probably used LZH with small dictionary and (pbit = 4). It was named "Efi compression".
              New version of compression code (named Tiano) uses LZH with (1 << 19) dictionary.
              But maybe LZH decoder in UEFI decoder supports larger than (1 << 19) dictionary.
              We check both LZH versions: Tiano and then Efi.
            */
            HRESULT res = S_FALSE;
            for (unsigned m = 0 ; m < 2; m++)
            {
              inStream->Init(src, packSize);
              outStream->Init(buf, uncompressedSize);
              lzhDecoder->SetDictSize(m == 0 ? ((UInt32)1 << 19) : ((UInt32)1 << 14));
              res = lzhDecoder->Code(inStream, outStream, uncompressedSize, NULL);
              if (res == S_OK)
                break;
            }
            RINOK(res)
          }
          bool error2;
          RINOK(ParseSections(newBufIndex, 0, uncompressedSize, parent, compressionType, level, error2))
        }
        else
        {
          if (newSectSize < 4 + 5 + 8)
            return S_FALSE;
          unsigned addSize = 4;
          if (pStart[0] == 0x5d && pStart[1] == 0 && pStart[2] == 0 && pStart[3] == 0x80 && pStart[4] == 0)
          {
            addSize = 0;
            // some archives have such header
          }
          else
          {
            // normal BIOS contains uncompressed size here
            // UInt32 uncompressedSize2 = Get24(pStart);
            // Byte firstSectType = p[9 + 3];
            // firstSectType can be 0 in some archives
          }
          pStart += addSize;

          RINOK(DecodeLzma(pStart, newSectSize - addSize))
          const size_t lzmaUncompressedSize = _bufs.Back().Size();
          // if (lzmaUncompressedSize != uncompressedSize)
          if (lzmaUncompressedSize < uncompressedSize)
            return S_FALSE;
          bool error2;
          RINOK(ParseSections(_bufs.Size() - 1, 0, (UInt32)lzmaUncompressedSize, parent, compressionType, level, error2))
        }
        _methodsMask |= (1 << compressionType);
      }
    }
    else if (type == SECTION_GUID_DEFINED)
    {
      const unsigned kHeaderSize = 4 + kGuidSize + 4;
      if (sectSize < kHeaderSize)
        return S_FALSE;
      item.SetGuid(p + 4);
      const UInt32 dataOffset = Get16(p + 4 + kGuidSize);
      const UInt32 attrib = Get16(p + 4 + kGuidSize + 2);
      if (dataOffset > sectSize || dataOffset < kHeaderSize)
        return S_FALSE;
      UInt32 newSectSize = sectSize - dataOffset;
      item.Size = newSectSize;
      UInt32 newOffset = posBase + pos + dataOffset;
      item.Offset = newOffset;
      const UInt32 propsSize = dataOffset - kHeaderSize;
      AddSpaceAndString(item.Characts, FLAGS_TO_STRING(g_GUIDED_SECTION_ATTRIBUTES, attrib));

      bool needDir = true;
      unsigned newBufIndex = bufIndex;
      int newMethod = method;
  
      if (AreGuidsEq(p + 0x4, k_Guid_LZMA_COMPRESSED))
      {
        // item.Name = "guid.lzma";
        // AddItem(item);
        const Byte *pStart = bufData + newOffset;
        // do we need correct pStart here for lzma steram offset?
        RINOK(DecodeLzma(pStart, newSectSize))
        _methodsMask |= (1 << COMPRESSION_TYPE_LZMA);
        newBufIndex = _bufs.Size() - 1;
        newOffset = 0;
        newSectSize = (UInt32)_bufs.Back().Size();
        newMethod = COMPRESSION_TYPE_LZMA;
      }
      else if (AreGuidsEq(p + 0x4, kGuids[kGuidIndex_CRC]) && propsSize == 4)
      {
        needDir = false;
        item.KeepName = false;
        if (CrcCalc(bufData + newOffset, newSectSize) != Get32(p + kHeaderSize))
          return S_FALSE;
      }
      else
      {
        if (propsSize != 0)
        {
          CItem item2 = item;
          item2.Name += ".prop";
          item2.Size = propsSize;
          item2.Offset = posBase + pos + kHeaderSize;
          AddItem(item2);
        }
      }

      int newParent = parent;
      if (needDir)
        newParent = (int)AddDirItem(item);
      bool error2;
      RINOK(ParseSections(newBufIndex, newOffset, newSectSize, newParent, newMethod, level, error2))
    }
    else if (type == SECTION_FIRMWARE_VOLUME_IMAGE)
    {
      item.KeepName = false;
      const int newParent = (int)AddDirItem(item);
      RINOK(ParseVolume(bufIndex, posBase + pos + 4,
          sectSize - 4,
          sectSize - 4,
          newParent, method, level))
    }
    else
    {
      bool needAdd = true;
      switch (type)
      {
        case SECTION_RAW:
        {
          const UInt32 kInsydeOffset = 12;
          if (sectDataSize >= kFvHeaderSize + kInsydeOffset)
          {
            if (IsFfs(p + 4 + kInsydeOffset) &&
                sectDataSize - kInsydeOffset == Get64(p + 4 + kInsydeOffset + 0x20))
            {
              needAdd = false;
              item.Name = "vol";
              const unsigned newParent = AddDirItem(item);
              RINOK(ParseVolume(bufIndex, posBase + pos + 4 + kInsydeOffset,
                  sectDataSize - kInsydeOffset,
                  sectDataSize - kInsydeOffset,
                  (int)newParent, method, level))
            }
            
            if (needAdd)
            {
              const char *ext = FindExt(p + 4, sectDataSize);
              if (ext)
                item.Name = ext;
            }
          }
          break;
        }
        case SECTION_DXE_DEPEX:
        case SECTION_PEI_DEPEX:
        {
          AString s;
          if (ParseDepedencyExpression(p + 4, sectDataSize, s))
          {
            if (s.Len() < (1 << 9))
            {
              s.InsertAtFront('[');
              s.Add_Char(']');
              AddSpaceAndString(_items[item.Parent].Characts, s);
              needAdd = false;
            }
            else
            {
              item.BufIndex = AddBuf(s.Len());
              CByteBuffer &buf0 = _bufs[item.BufIndex];
              if (s.Len() != 0)
                memcpy(buf0, s, s.Len());
              item.Offset = 0;
              item.Size = s.Len();
            }
          }
          break;
        }
        case SECTION_VERSION:
        {
          if (sectDataSize > 2)
          {
            AString s;
            if (ParseUtf16zString2(p + 6, sectDataSize - 2, s))
            {
              AString s2 ("ver:");
              s2.Add_UInt32(Get16(p + 4));
              s2.Add_Space();
              s2 += s;
              AddSpaceAndString(_items[item.Parent].Characts, s2);
              needAdd = false;
            }
          }
          break;
        }
        case SECTION_USER_INTERFACE:
        {
          AString s;
          if (ParseUtf16zString2(p + 4, sectDataSize, s))
          {
            _items[parent].Name = s;
            needAdd = false;
          }
          break;
        }
        case SECTION_FREEFORM_SUBTYPE_GUID:
        {
          if (sectDataSize >= kGuidSize)
          {
            item.SetGuid(p + 4);
            item.Size = sectDataSize - kGuidSize;
            item.Offset = posBase + pos + 4 + kGuidSize;
          }
          break;
        }
      }
    
      if (needAdd)
        AddFileItemWithIndex(item);
    }

    pos += sectSize;
  }
}

static UInt32 Count_FF_Bytes(const Byte *p, UInt32 size)
{
  UInt32 i;
  for (i = 0; i < size && p[i] == 0xFF; i++);
  return i;
}

static bool Is_FF_Stream(const Byte *p, UInt32 size)
{
  return (Count_FF_Bytes(p, size) == size);
}

struct CVolFfsHeader
{
  UInt32 HeaderLen;
  UInt64 VolSize;
  
  bool Parse(const Byte *p);
};

bool CVolFfsHeader::Parse(const Byte *p)
{
  if (Get32(p + 0x28) != kFvSignature)
    return false;

  UInt32 attribs = Get32(p + 0x2C);
  if ((attribs & FVB_ERASE_POLARITY) == 0)
    return false;
  VolSize = Get64(p + 0x20);
  HeaderLen = Get16(p + 0x30);
  if (HeaderLen < kFvHeaderSize || (HeaderLen & 0x7) != 0 || VolSize < HeaderLen)
    return false;
  return true;
}


HRESULT CHandler::ParseVolume(
    unsigned bufIndex, UInt32 posBase,
    UInt32 exactSize, UInt32 limitSize,
    int parent, int method, unsigned level)
{
  if (level > kLevelMax)
    return S_FALSE;
  MyPrint(posBase, exactSize, level, "Volume");
  level++;
  if (exactSize < kFvHeaderSize)
    return S_FALSE;
  const Byte *p = _bufs[bufIndex] + posBase;
  // first 16 bytes must be zeros, but they are not zeros sometimes.
  if (!IsFfs(p))
  {
    CItem item;
    item.Method = method;
    item.BufIndex = bufIndex;
    item.Parent = parent;
    item.Offset = posBase;
    item.Size = exactSize;
    if (!Is_FF_Stream(p + kFfsGuidOffset, 16))
      item.SetGuid(p + kFfsGuidOffset);
    // if (item.Name.IsEmpty())
    item.Name += "[VOL]";
    AddItem(item);
    return S_OK;
  }
  
  CVolFfsHeader ffsHeader;
  if (!ffsHeader.Parse(p))
    return S_FALSE;
  // if (parent >= 0) AddSpaceAndString(_items[parent].Characts, FLAGS_TO_STRING(g_FV_Attribs, attribs));
  
  // VolSize > exactSize (fh.Size) for some UEFI archives (is it correct UEFI?)
  // so we check VolSize for limitSize instead.

  if (ffsHeader.HeaderLen > limitSize || ffsHeader.VolSize > limitSize)
    return S_FALSE;
  
  {
    UInt32 checkCalc = 0;
    for (UInt32 i = 0; i < ffsHeader.HeaderLen; i += 2)
      checkCalc += Get16(p + i);
    if ((checkCalc & 0xFFFF) != 0)
      return S_FALSE;
  }
  
  // 3 reserved bytes are not zeros sometimes.
  // UInt16 ExtHeaderOffset; // in new SPECIFICATION?
  // Byte revision = p[0x37];
  
  UInt32 pos = kFvHeaderSize;
  for (;;)
  {
    if (pos >= ffsHeader.HeaderLen)
      return S_FALSE;
    UInt32 numBlocks = Get32(p + pos);
    UInt32 length = Get32(p + pos + 4);
    pos += 8;
    if (numBlocks == 0 && length == 0)
      break;
  }
  if (pos != ffsHeader.HeaderLen)
    return S_FALSE;
  
  CRecordVector<UInt32> guidsVector;
  
  for (;;)
  {
    UInt32 rem = (UInt32)ffsHeader.VolSize - pos;
    if (rem < kFileHeaderSize)
      break;
    pos = (pos + 7) & ~7u;
    rem = (UInt32)ffsHeader.VolSize - pos;
    if (rem < kFileHeaderSize)
      break;

    CItem item;
    item.Method = method;
    item.BufIndex = bufIndex;
    item.Parent = parent;

    const Byte *pFile = p + pos;
    CFfsFileHeader fh;
    if (!fh.Parse(pFile))
    {
      UInt32 num_FF_bytes = Count_FF_Bytes(pFile, rem);
      if (num_FF_bytes != rem)
      {
        item.Name = "[junk]";
        item.Offset = posBase + pos + num_FF_bytes;
        item.Size = rem - num_FF_bytes;
        AddItem(item);
      }
      PrintLevel(level); PRF(printf("== FF FF reminder"));

      break;
    }

    PrintLevel(level); PRF(printf("%s, type = %3d,  pos = %7x, size = %7x", "FILE", fh.Type, posBase + pos, fh.Size));
    
    if (!fh.Check(pFile, rem))
      return S_FALSE;
    
    UInt32 offset = posBase + pos + kFileHeaderSize;
    UInt32 sectSize = fh.GetDataSize();
    item.Offset = offset;
    item.Size = sectSize;

    pos += fh.Size;

    if (fh.Type == FV_FILETYPE_FFS_PAD)
      if (Is_FF_Stream(pFile + kFileHeaderSize, sectSize))
        continue;
    
    UInt32 guid32 = Get32(fh.GuidName);
    bool full = true;
    if (guidsVector.FindInSorted(guid32) < 0)
    {
      guidsVector.AddToUniqueSorted(guid32);
      full = false;
    }
    item.SetGuid(fh.GuidName, full);
    
    item.Characts = fh.GetCharacts();
    // PrintLevel(level);
    PRF(printf(" : %s", item.Characts.Ptr()));

    {
      PRF(printf(" attrib = %2u State = %3u ", (unsigned)fh.Attrib, (unsigned)fh.State));
      PRF(char s[64]);
      PRF(RawLeGuidToString(fh.GuidName, s));
      PRF(printf(" : %s ", s));
    }

    if (fh.Type == FV_FILETYPE_FFS_PAD ||
        fh.Type == FV_FILETYPE_RAW)
    {
      bool isVolume = false;
      if (fh.Type == FV_FILETYPE_RAW)
      {
        if (sectSize >= kFvHeaderSize)
          if (IsFfs(pFile + kFileHeaderSize))
            isVolume = true;
      }
      if (isVolume)
      {
        const unsigned newParent = AddDirItem(item);
        const UInt32 limSize = fh.GetDataSize2(rem);
        // volume.VolSize > fh.Size for some UEFI archives (is it correct UEFI?)
        // so we will check VolSize for limitSize instead.
        RINOK(ParseVolume(bufIndex, offset, sectSize, limSize, (int)newParent, method, level))
      }
      else
        AddItem(item);
    }
    else
    {
      /*
      if (fh.Type == FV_FILETYPE_FREEFORM)
      {
        // in intel bio example: one FV_FILETYPE_FREEFORM file is wav file (not sections)
        // AddItem(item);
      }
      else
      */
      {
        const unsigned newParent = AddDirItem(item);
        bool error2;
        RINOK(ParseSections(bufIndex, offset, sectSize, (int)newParent, method, level + 1, error2))
        if (error2)
        {
          // in intel bio example: one FV_FILETYPE_FREEFORM file is wav file (not sections)
          item.IsDir = false;
          item.Size = sectSize;
          item.Name.Insert(0, "[ERROR]");
          AddItem(item);
        }
      }
    }
  }
  
  return S_OK;
}


static const char * const kRegionName[] =
{
    "Descriptor"
  , "BIOS"
  , "ME"
  , "GbE"
  , "PDR"
  , "Region5"
  , "Region6"
  , "Region7"
};


HRESULT CHandler::ParseIntelMe(
    unsigned bufIndex, UInt32 posBase,
    UInt32 exactSize, UInt32 limitSize,
    int parent, int method, unsigned /* level */)
{
  UNUSED_VAR(limitSize)
  // level++;

  const Byte *p = _bufs[bufIndex] + posBase;
  if (exactSize < 16 + 16)
    return S_FALSE;
  if (!IsIntelMe(p))
    return S_FALSE;

  UInt32 v0 = GetUi32(p + 20);
  // UInt32 numRegions = (v0 >> 24) & 0x7;
  UInt32 regAddr = (v0 >> 12) & 0xFF0;
  // UInt32 numComps  = (v0 >> 8) & 0x3;
  // UInt32 fcba = (v0 <<  4) & 0xFF0;
  
  // (numRegions == 0) in header in some new images.
  // So we don't use the value from header
  UInt32 numRegions = 7;

  for (unsigned i = 0; i <= numRegions; i++)
  {
    UInt32 offset = regAddr + i * 4;
    if (offset + 4 > exactSize)
      break;
    UInt32 val = GetUi32(p + offset);

    // only 12 bits probably are OK.
    // How does it work for files larger than 16 MB?
    const UInt32 kMask = 0xFFF;
    // const UInt32 kMask = 0xFFFF; // 16-bit is more logical
    const UInt32 lim  = (val >> 16) & kMask;
    const UInt32 base = (val & kMask);

    /*
      strange combinations:
        PDR:   base = 0x1FFF lim = 0;
        empty: base = 0xFFFF lim = 0xFFFF;

        PDR:   base = 0x7FFF lim = 0;
        empty: base = 0x7FFF lim = 0;
    */
    if (base == kMask && lim == 0)
      continue; // unused

    if (lim < base)
      continue; // unused

    CItem item;
    item.Name = kRegionName[i];
    item.Method = method;
    item.BufIndex = bufIndex;
    item.Parent = parent;
    item.Offset = posBase + (base << 12);
    if (item.Offset > exactSize)
      continue;
    item.Size = (lim + 1 - base) << 12;
    // item.SetGuid(p + kFfsGuidOffset);
    // item.Name += " [VOLUME]";
    AddItem(item);
  }
  return S_OK;
}


HRESULT CHandler::OpenCapsule(IInStream *stream)
{
  const unsigned kHeaderSize = 80;
  Byte buf[kHeaderSize];
  RINOK(ReadStream_FALSE(stream, buf, kHeaderSize))
  if (!_h.Parse(buf))
    return S_FALSE;
  if (_h.CapsuleImageSize < kHeaderSize
       || _h.CapsuleImageSize < _h.HeaderSize
       || _h.OffsetToCapsuleBody < _h.HeaderSize
       || _h.OffsetToCapsuleBody > _h.CapsuleImageSize
       || _h.CapsuleImageSize > (1u << 30) // to reduce false detection
       || _h.HeaderSize > (1u << 28) // to reduce false detection
      )
    return S_FALSE;
  _phySize = _h.CapsuleImageSize;

  if (_h.SequenceNumber != 0 ||
      _h.OffsetToSplitInformation != 0 )
    return E_NOTIMPL;

  const unsigned bufIndex = AddBuf(_h.CapsuleImageSize);
  CByteBuffer &buf0 = _bufs[bufIndex];
  memcpy(buf0, buf, kHeaderSize);
  RINOK(ReadStream_FALSE(stream, buf0 + kHeaderSize, _h.CapsuleImageSize - kHeaderSize))

  AddCommentString("Author", _h.OffsetToAuthorInformation);
  AddCommentString("Revision", _h.OffsetToRevisionInformation);
  AddCommentString("Short Description", _h.OffsetToShortDescription);
  AddCommentString("Long Description", _h.OffsetToLongDescription);


  const UInt32 size = _h.CapsuleImageSize - _h.OffsetToCapsuleBody;

  if (size >= 32 && IsIntelMe(buf0 + _h.OffsetToCapsuleBody))
    return ParseIntelMe(bufIndex, _h.OffsetToCapsuleBody, size, size, -1, -1, 0);

  return ParseVolume(bufIndex, _h.OffsetToCapsuleBody, size, size, -1, -1, 0);
}


HRESULT CHandler::OpenFv(IInStream *stream, const UInt64 * /* maxCheckStartPosition */, IArchiveOpenCallback * /* callback */)
{
  Byte buf[kFvHeaderSize];
  RINOK(ReadStream_FALSE(stream, buf, kFvHeaderSize))
  if (!IsFfs(buf))
    return S_FALSE;
  CVolFfsHeader ffsHeader;
  if (!ffsHeader.Parse(buf))
    return S_FALSE;
  if (ffsHeader.VolSize > ((UInt32)1 << 30))
    return S_FALSE;
  _phySize = ffsHeader.VolSize;
  RINOK(InStream_SeekToBegin(stream))
  UInt32 fvSize32 = (UInt32)ffsHeader.VolSize;
  unsigned bufIndex = AddBuf(fvSize32);
  RINOK(ReadStream_FALSE(stream, _bufs[bufIndex], fvSize32))
  return ParseVolume(bufIndex, 0, fvSize32, fvSize32, -1, -1, 0);
}


HRESULT CHandler::Open2(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback)
{
  if (_capsuleMode)
  {
    RINOK(OpenCapsule(stream))
  }
  else
  {
    RINOK(OpenFv(stream, maxCheckStartPosition, callback))
  }

  const unsigned num = _items.Size();
  CIntArr numChilds(num);
  
  unsigned i;
  
  for (i = 0; i < num; i++)
    numChilds[i] = 0;
  
  for (i = 0; i < num; i++)
  {
    const int parent = _items[i].Parent;
    if (parent >= 0)
      numChilds[(unsigned)parent]++;
  }

  for (i = 0; i < num; i++)
  {
    const CItem &item = _items[i];
    const int parent = item.Parent;
    if (parent >= 0)
    {
      CItem &parentItem = _items[(unsigned)parent];
      if (numChilds[(unsigned)parent] == 1)
        if (!item.ThereIsUniqueName || !parentItem.ThereIsUniqueName || !parentItem.ThereAreSubDirs)
          parentItem.Skip = true;
    }
  }

  CUIntVector mainToReduced;
  
  for (i = 0; i < _items.Size(); i++)
  {
    mainToReduced.Add(_items2.Size());
    const CItem &item = _items[i];
    if (item.Skip)
      continue;
    AString name;
    int numItems = -1;
    int parent = item.Parent;
    if (parent >= 0)
      numItems = numChilds[(unsigned)parent];
    AString name2 (item.GetName(numItems));
    AString characts2 (item.Characts);
    if (item.KeepName)
      name = name2;
    
    while (parent >= 0)
    {
      const CItem &item3 = _items[(unsigned)parent];
      if (!item3.Skip)
        break;
      if (item3.KeepName)
      {
        AString name3 (item3.GetName(-1));
        if (name.IsEmpty())
          name = name3;
        else
          name = name3 + '.' + name;
      }
      AddSpaceAndString(characts2, item3.Characts);
      parent = item3.Parent;
    }
    
    if (name.IsEmpty())
      name = name2;
    
    CItem2 item2;
    item2.MainIndex = i;
    item2.Name = name;
    item2.Characts = characts2;
    if (parent >= 0)
      item2.Parent = (int)mainToReduced[(unsigned)parent];
    _items2.Add(item2);
    /*
    CItem2 item2;
    item2.MainIndex = i;
    item2.Name = item.Name;
    item2.Parent = item.Parent;
    _items2.Add(item2);
    */
  }
  
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();
  {
    HRESULT res = Open2(inStream, maxCheckStartPosition, callback);
    if (res == E_NOTIMPL)
      res = S_FALSE;
    return res;
  }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _phySize = 0;
  _totalBufsSize = 0;
  _methodsMask = 0;
  _items.Clear();
  _items2.Clear();
  _bufs.Clear();
  _comment.Empty();
  _headersError = false;
  _h.Clear();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items2.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _items2.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _items[_items2[allFilesMode ? i : indices[i]].MainIndex].Size;
  RINOK(extractCallback->SetTotal(totalSize))
  totalSize = 0;
  
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

  for (i = 0;; i++)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;
    Int32 opRes;
    {
      CMyComPtr<ISequentialOutStream> realOutStream;
      const Int32 askMode = testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
      const UInt32 index = allFilesMode ? i : indices[i];
      const CItem &item = _items[_items2[index].MainIndex];
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
      totalSize += item.Size;
      if (!testMode && !realOutStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode))
      if (testMode || item.IsDir)
        opRes = NExtract::NOperationResult::kOK;
      else
      {
        opRes = NExtract::NOperationResult::kDataError;
        CMyComPtr<ISequentialInStream> inStream;
        GetStream(index, &inStream);
        if (inStream)
        {
          RINOK(copyCoder.Interface()->Code(inStream, realOutStream, NULL, NULL, lps))
          if (copyCoder->TotalSize == item.Size)
            opRes = NExtract::NOperationResult::kOK;
        }
      }
    }
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  const CItem &item = _items[_items2[index].MainIndex];
  if (item.IsDir)
    return S_FALSE;
  CBufInStream *streamSpec = new CBufInStream;
  CMyComPtr<IInStream> streamTemp = streamSpec;
  const CByteBuffer &buf = _bufs[item.BufIndex];
  if (item.Offset > buf.Size())
    return S_FALSE;
  size_t size = buf.Size() - item.Offset;
  if (size > item.Size)
    size = item.Size;
  streamSpec->Init(buf + item.Offset, size, (IInArchive *)this);
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}


namespace UEFIc {
  
static const Byte k_Capsule_Signatures[] =
{
   16, CAPSULE_SIGNATURE,
   16, CAPSULE2_SIGNATURE,
   16, CAPSULE_UEFI_SIGNATURE
};

REGISTER_ARC_I_CLS(
  CHandler(true),
  "UEFIc", "scap", NULL, 0xD0,
  k_Capsule_Signatures,
  0,
  NArcInfoFlags::kMultiSignature |
  NArcInfoFlags::kFindSignature,
  NULL)
  
}

namespace UEFIf {
  
static const Byte k_FFS_Signatures[] =
{
   16, FFS1_SIGNATURE,
   16, FFS2_SIGNATURE
};


REGISTER_ARC_I_CLS(
  CHandler(false),
  "UEFIf", "uefif", NULL, 0xD1,
  k_FFS_Signatures,
  kFfsGuidOffset,
  NArcInfoFlags::kMultiSignature |
  NArcInfoFlags::kFindSignature,
  NULL)

}

}}

/* ================ unit: CPP/7zip/Archive/VdiHandler.cpp ================ */
// VdiHandler.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

using namespace NWindows;

namespace NArchive {
namespace NVdi {

static const Byte k_Signature[] = { 0x7F, 0x10, 0xDA, 0xBE };

static const unsigned k_ClusterBits = 20;
static const UInt32 k_ClusterSize = (UInt32)1 << k_ClusterBits;


/*
VDI_IMAGE_BLOCK_FREE = (~0) // returns any random data
VDI_IMAGE_BLOCK_ZERO = (~1) // returns zeros
*/

// static const UInt32 k_ClusterType_Free  = 0xffffffff;
static const UInt32 k_ClusterType_Zero  = 0xfffffffe;

#define IS_CLUSTER_ALLOCATED(v) ((UInt32)(v) < k_ClusterType_Zero)


// static const UInt32 kDiskType_Dynamic = 1;
// static const UInt32 kDiskType_Static = 2;

static const char * const kDiskTypes[] =
{
    "0"
  , "Dynamic"
  , "Static"
  , "Undo"
  , "Diff"
};


enum EGuidType
{
  k_GuidType_Creat,
  k_GuidType_Modif,
  k_GuidType_Link,
  k_GuidType_PModif
};

static const unsigned kNumGuids = 4;
static const char * const kGuidNames[kNumGuids] =
{
    "Creat "
  , "Modif "
  , "Link  "
  , "PModif"
};

static bool IsEmptyGuid(const Byte *data)
{
  for (unsigned i = 0; i < 16; i++)
    if (data[i] != 0)
      return false;
  return true;
}



Z7_class_CHandler_final: public CHandlerImg
{
  UInt32 _dataOffset;
  CByteBuffer _table;
  UInt64 _phySize;
  UInt32 _imageType;
  bool _isArc;
  bool _unsupported;

  Byte Guids[kNumGuids][16];

  HRESULT Seek2(UInt64 offset)
  {
    _posInArc = offset;
    return InStream_SeekSet(Stream, offset);
  }

  HRESULT InitAndSeek()
  {
    _virtPos = 0;
    return Seek2(0);
  }

  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openCallback) Z7_override;

public:
  Z7_IFACE_COM7_IMP(IInArchive_Img)

  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
};


Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
    return S_OK;
  {
    UInt64 rem = _size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size == 0)
      return S_OK;
  }
 
  {
    UInt64 cluster = _virtPos >> k_ClusterBits;
    UInt32 lowBits = (UInt32)_virtPos & (k_ClusterSize - 1);
    {
      UInt32 rem = k_ClusterSize - lowBits;
      if (size > rem)
        size = rem;
    }

    cluster <<= 2;
    if (cluster < _table.Size())
    {
      const Byte *p = (const Byte *)_table + (size_t)cluster;
      const UInt32 v = Get32(p);
      if (IS_CLUSTER_ALLOCATED(v))
      {
        UInt64 offset = _dataOffset + ((UInt64)v << k_ClusterBits);
        offset += lowBits;
        if (offset != _posInArc)
        {
          RINOK(Seek2(offset))
        }
        HRESULT res = Stream->Read(data, size, &size);
        _posInArc += size;
        _virtPos += size;
        if (processedSize)
          *processedSize = size;
        return res;
      }
    }
    
    memset(data, 0, size);
    _virtPos += size;
    if (processedSize)
      *processedSize = size;
    return S_OK;
  }
}


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidHeadersSize,
  kpidMethod,
  kpidComment,
  kpidName
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidPhySize: if (_phySize != 0) prop = _phySize; break;
    case kpidHeadersSize: prop = _dataOffset; break;
    
    case kpidMethod:
    {
      TYPE_TO_PROP(kDiskTypes, _imageType, prop);
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
      // if (_headerError) v |= kpv_ErrorFlags_HeadersError;
      if (!Stream && v == 0 && _isArc)
        v = kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }

    case kpidComment:
    {
      AString s;
      for (unsigned i = 0; i < kNumGuids; i++)
      {
        const Byte *guid = Guids[i];
        if (!IsEmptyGuid(guid))
        {
          s.Add_LF();
          s += kGuidNames[i];
          s += " : ";
          char temp[64];
          RawLeGuidToString_Braced(guid, temp);
          MyStringLower_Ascii(temp);
          s += temp;
        }
      }
      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidName:
    {
      const Byte *guid = Guids[k_GuidType_Creat];
      if (!IsEmptyGuid(guid))
      {
        char temp[64];
        RawLeGuidToString_Braced(guid, temp);
        MyStringLower_Ascii(temp);
        MyStringCat(temp, ".vdi");
        prop = temp;
      }
      break;
    }
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidSize: prop = _size; break;
    case kpidPackSize: prop = _phySize - _dataOffset; break;
    case kpidExtension: prop = (_imgExt ? _imgExt : "img"); break;
  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback * /* openCallback */)
{
  const unsigned kHeaderSize = 512;
  Byte buf[kHeaderSize];
  RINOK(ReadStream_FALSE(stream, buf, kHeaderSize))

  if (memcmp(buf + 0x40, k_Signature, sizeof(k_Signature)) != 0)
    return S_FALSE;

  const UInt32 version = Get32(buf + 0x44);
  if (version >= 0x20000)
    return S_FALSE;
  if (version < 0x10000)
  {
    _unsupported = true;
    return S_FALSE;
  }
  
  const unsigned kHeaderOffset = 0x48;
  const unsigned kGuidsOffsets = 0x188;
  const UInt32 headerSize = Get32(buf + kHeaderOffset);
  if (headerSize < kGuidsOffsets - kHeaderOffset || headerSize > 0x200 - kHeaderOffset)
    return S_FALSE;

  _imageType = Get32(buf + 0x4C);
  // Int32 flags = Get32(buf + 0x50);
  // Byte Comment[0x100]

  const UInt32 tableOffset = Get32(buf + 0x154);
  if (tableOffset < 0x200)
    return S_FALSE;

  _dataOffset = Get32(buf + 0x158);

  // UInt32 geometry[3];
  
  const UInt32 sectorSize = Get32(buf + 0x168);
  if (sectorSize != 0x200)
    return S_FALSE;

  _size = Get64(buf + 0x170);
  const UInt32 blockSize = Get32(buf + 0x178);
  const UInt32 totalBlocks = Get32(buf + 0x180);
  const UInt32 numAllocatedBlocks = Get32(buf + 0x184);
  
  _isArc = true;

  if (_dataOffset < tableOffset)
    return S_FALSE;

  if (_imageType > 4)
    _unsupported = true;

  if (blockSize != k_ClusterSize)
  {
    _unsupported = true;
    return S_FALSE;
  }

  if (headerSize >= kGuidsOffsets + kNumGuids * 16 - kHeaderOffset)
  {
    for (unsigned i = 0; i < kNumGuids; i++)
      memcpy(Guids[i], buf + kGuidsOffsets + 16 * i, 16);

    if (!IsEmptyGuid(Guids[k_GuidType_Link]) ||
        !IsEmptyGuid(Guids[k_GuidType_PModif]))
      _unsupported = true;
  }

  {
    UInt64 size2 = (UInt64)totalBlocks << k_ClusterBits;
    if (size2 < _size)
    {
      _unsupported = true;
      return S_FALSE;
    }
    /*
    if (size2 > _size)
      _size = size2;
    */
  }

  {
    UInt32 tableReserved = _dataOffset - tableOffset;
    if ((tableReserved >> 2) < totalBlocks)
      return S_FALSE;
  }

  _phySize = _dataOffset + ((UInt64)numAllocatedBlocks << k_ClusterBits);

  const size_t numBytes = (size_t)totalBlocks * 4;
  if ((numBytes >> 2) != totalBlocks)
  {
    _unsupported = true;
    return E_OUTOFMEMORY;
  }

  _table.Alloc(numBytes);
  RINOK(InStream_SeekSet(stream, tableOffset))
  RINOK(ReadStream_FALSE(stream, _table, numBytes))
    
  const Byte *data = _table;
  for (UInt32 i = 0; i < totalBlocks; i++)
  {
    const UInt32 v = Get32(data + (size_t)i * 4);
    if (!IS_CLUSTER_ALLOCATED(v))
      continue;
    if (v >= numAllocatedBlocks)
    {
      _unsupported = true;
      return S_FALSE;
    }
  }
  
  Stream = stream;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Close())
{
  _table.Free();
  _phySize = 0;
  _isArc = false;
  _unsupported = false;

  for (unsigned i = 0; i < kNumGuids; i++)
    memset(Guids[i], 0, 16);

  // CHandlerImg:
  Clear_HandlerImg_Vars();
  Stream.Release();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  if (_unsupported)
    return S_FALSE;
  CMyComPtr<ISequentialInStream> streamTemp = this;
  RINOK(InitAndSeek())
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}


REGISTER_ARC_I(
  "VDI", "vdi", NULL, 0xC9,
  k_Signature,
  0x40,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/VhdHandler.cpp ================ */
// VhdHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)
#define Get64(p) GetBe64(p)

#define G32(_offs_, dest) dest = Get32(p + (_offs_))
#define G64(_offs_, dest) dest = Get64(p + (_offs_))

using namespace NWindows;

namespace NArchive {
namespace NVhd {

static const unsigned kSignatureSize = 10;
static const Byte kSignature[kSignatureSize] =
  { 'c', 'o', 'n', 'e', 'c', 't', 'i', 'x', 0, 0 };

static const UInt32 kUnusedBlock = 0xFFFFFFFF;

static const UInt32 kDiskType_Fixed = 2;
static const UInt32 kDiskType_Dynamic = 3;
static const UInt32 kDiskType_Diff = 4;

static const char * const kDiskTypes[] =
{
    "0"
  , "1"
  , "Fixed"
  , "Dynamic"
  , "Differencing"
};

struct CFooter
{
  // UInt32 Features;
  // UInt32 FormatVersion;
  UInt64 DataOffset;
  UInt32 CTime;
  UInt32 CreatorApp;
  UInt32 CreatorVersion;
  UInt32 CreatorHostOS;
  // UInt64 OriginalSize;
  UInt64 CurrentSize;
  UInt32 DiskGeometry;
  UInt32 Type;
  Byte Id[16];
  Byte SavedState;

  bool IsFixed() const { return Type == kDiskType_Fixed; }
  bool ThereIsDynamic() const { return Type == kDiskType_Dynamic || Type == kDiskType_Diff; }
  // bool IsSupported() const { return Type == kDiskType_Fixed || Type == kDiskType_Dynamic || Type == kDiskType_Diff; }
  UInt32 NumCyls() const { return DiskGeometry >> 16; }
  UInt32 NumHeads() const { return (DiskGeometry >> 8) & 0xFF; }
  UInt32 NumSectorsPerTrack() const { return DiskGeometry & 0xFF; }
  void AddTypeString(AString &s) const;
  bool Parse(const Byte *p);
};

void CFooter::AddTypeString(AString &s) const
{
  if (Type < Z7_ARRAY_SIZE(kDiskTypes))
    s += kDiskTypes[Type];
  else
    s.Add_UInt32(Type);
}

static bool CheckBlock(const Byte *p, unsigned size, unsigned checkSumOffset, unsigned zeroOffset)
{
  UInt32 sum = 0;
  unsigned i;
  for (i = 0; i < checkSumOffset; i++)
    sum += p[i];
  for (i = checkSumOffset + 4; i < size; i++)
    sum += p[i];
  if (~sum != Get32(p + checkSumOffset))
    return false;
  for (i = zeroOffset; i < size; i++)
    if (p[i] != 0)
      return false;
  return true;
}

static const unsigned kSectorSize_Log = 9;
static const unsigned kSectorSize = 1 << kSectorSize_Log;
static const unsigned kHeaderSize = 512;

bool CFooter::Parse(const Byte *p)
{
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return false;
  // G32(0x08, Features);
  // G32(0x0C, FormatVersion);
  G64(0x10, DataOffset);
  G32(0x18, CTime);
  G32(0x1C, CreatorApp);
  G32(0x20, CreatorVersion);
  G32(0x24, CreatorHostOS);
  // G64(0x28, OriginalSize);
  G64(0x30, CurrentSize);
  G32(0x38, DiskGeometry);
  G32(0x3C, Type);
  if (Type < kDiskType_Fixed ||
      Type > kDiskType_Diff)
    return false;
  memcpy(Id, p + 0x44, 16);
  SavedState = p[0x54];
  // if (DataOffset > ((UInt64)1 << 62)) return false;
  // if (CurrentSize > ((UInt64)1 << 62)) return false;
  return CheckBlock(p, kHeaderSize, 0x40, 0x55);
}

struct CParentLocatorEntry
{
  UInt32 Code;
  UInt32 DataSpace;
  UInt32 DataLen;
  UInt64 DataOffset;

  bool Parse(const Byte *p)
  {
    G32(0x00, Code);
    G32(0x04, DataSpace);
    G32(0x08, DataLen);
    G64(0x10, DataOffset);
    return Get32(p + 0x0C) == 0; // Reserved
  }
};

struct CDynHeader
{
  // UInt64 DataOffset;
  UInt64 TableOffset;
  // UInt32 HeaderVersion;
  UInt32 NumBlocks;
  unsigned BlockSizeLog;
  UInt32 ParentTime;
  Byte ParentId[16];
  bool RelativeNameWasUsed;
  UString ParentName;
  UString RelativeParentNameFromLocator;
  CParentLocatorEntry ParentLocators[8];

  bool Parse(const Byte *p);
  UInt32 NumBitMapSectors() const
  {
    UInt32 numSectorsInBlock = (1 << (BlockSizeLog - kSectorSize_Log));
    return (numSectorsInBlock + kSectorSize * 8 - 1) / (kSectorSize * 8);
  }
  void Clear()
  {
    RelativeNameWasUsed = false;
    ParentName.Empty();
    RelativeParentNameFromLocator.Empty();
  }
};

bool CDynHeader::Parse(const Byte *p)
{
  if (memcmp(p, "cxsparse", 8) != 0)
    return false;
  // G64(0x08, DataOffset);
  G64(0x10, TableOffset);
  // G32(0x18, HeaderVersion);
  G32(0x1C, NumBlocks);
  {
    UInt32 blockSize = Get32(p + 0x20);
    unsigned i;
    for (i = kSectorSize_Log;; i++)
    {
      if (i > 31)
        return false;
      if (((UInt32)1 << i) == blockSize)
        break;
    }
    BlockSizeLog = i;
  }
  G32(0x38, ParentTime);
  if (Get32(p + 0x3C) != 0) // reserved
    return false;
  memcpy(ParentId, p + 0x28, 16);
  {
    const unsigned kNameLen = 256;
    wchar_t *s = ParentName.GetBuf(kNameLen);
    unsigned i;
    for (i = 0; i < kNameLen; i++)
    {
      wchar_t c = Get16(p + 0x40 + i * 2);
      if (c == 0)
        break;
      s[i] = c;
    }
    s[i] = 0;
    ParentName.ReleaseBuf_SetLen(i);
  }
  for (unsigned i = 0; i < 8; i++)
    if (!ParentLocators[i].Parse(p + 0x240 + i * 24))
      return false;
  return CheckBlock(p, 1024, 0x24, 0x240 + 8 * 24);
}

Z7_class_CHandler_final: public CHandlerImg
{
  UInt64 _posInArcLimit;
  UInt64 _startOffset;
  UInt64 _phySize;

  CFooter Footer;
  CDynHeader Dyn;
  CRecordVector<UInt32> Bat;
  CByteBuffer BitMap;
  UInt32 BitMapTag;
  UInt32 NumUsedBlocks;
  CMyComPtr<IInStream> ParentStream;
  CHandler *Parent;
  UInt64 NumLevels;
  UString _errorMessage;
  // bool _unexpectedEnd;

  void AddErrorMessage(const char *message, const wchar_t *name = NULL)
  {
    if (!_errorMessage.IsEmpty())
      _errorMessage.Add_LF();
    _errorMessage += message;
    if (name)
      _errorMessage += name;
  }

  void UpdatePhySize(UInt64 value)
  {
    if (_phySize < value)
      _phySize = value;
  }

  HRESULT Seek2(UInt64 offset);
  HRESULT InitAndSeek();
  HRESULT ReadPhy(UInt64 offset, void *data, UInt32 size);

  bool NeedParent() const { return Footer.Type == kDiskType_Diff; }
  UInt64 GetPackSize() const
    { return Footer.ThereIsDynamic() ? ((UInt64)NumUsedBlocks << Dyn.BlockSizeLog) : Footer.CurrentSize; }

  UString GetParentSequence() const
  {
    const CHandler *p = this;
    UString res;
    while (p && p->NeedParent())
    {
      if (!res.IsEmpty())
        res += " -> ";
      UString mainName;
      UString anotherName;
      if (Dyn.RelativeNameWasUsed)
      {
        mainName = p->Dyn.RelativeParentNameFromLocator;
        anotherName = p->Dyn.ParentName;
      }
      else
      {
        mainName = p->Dyn.ParentName;
        anotherName = p->Dyn.RelativeParentNameFromLocator;
      }
      res += mainName;
      if (mainName != anotherName && !anotherName.IsEmpty())
      {
        res.Add_Space();
        res.Add_Char('(');
        res += anotherName;
        res.Add_Char(')');
      }
      p = p->Parent;
    }
    return res;
  }

  bool AreParentsOK() const
  {
    const CHandler *p = this;
    while (p->NeedParent())
    {
      p = p->Parent;
      if (!p)
        return false;
    }
    return true;
  }

  HRESULT Open3();
  HRESULT Open2(IInStream *stream, CHandler *child, IArchiveOpenCallback *openArchiveCallback, unsigned level);
  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openArchiveCallback) Z7_override
  {
    return Open2(stream, NULL, openArchiveCallback, 0);
  }
  void CloseAtError() Z7_override;

public:
  Z7_IFACE_COM7_IMP(IInArchive_Img)

  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
};

HRESULT CHandler::Seek2(UInt64 offset) { return InStream_SeekSet(Stream, _startOffset + offset); }

HRESULT CHandler::InitAndSeek()
{
  if (ParentStream)
  {
    RINOK(Parent->InitAndSeek())
  }
  _virtPos = _posInArc = 0;
  BitMapTag = kUnusedBlock;
  BitMap.Alloc(Dyn.NumBitMapSectors() << kSectorSize_Log);
  return Seek2(0);
}

HRESULT CHandler::ReadPhy(UInt64 offset, void *data, UInt32 size)
{
  if (offset + size > _posInArcLimit)
    return S_FALSE;
  if (offset != _posInArc)
  {
    _posInArc = offset;
    RINOK(Seek2(offset))
  }
  HRESULT res = ReadStream_FALSE(Stream, data, size);
  if (res == S_OK)
    _posInArc += size;
  else
    Reset_PosInArc();
  return res;
}

HRESULT CHandler::Open3()
{
  // Fixed archive uses only footer

  UInt64 startPos;
  RINOK(InStream_GetPos(Stream, startPos))
  _startOffset = startPos;
  Byte header[kHeaderSize];
  RINOK(ReadStream_FALSE(Stream, header, kHeaderSize))
  bool headerIsOK = Footer.Parse(header);
  _size = Footer.CurrentSize;

  if (headerIsOK && !Footer.ThereIsDynamic())
  {
    // fixed archive
    if (startPos < Footer.CurrentSize)
      return S_FALSE;
    _posInArcLimit = Footer.CurrentSize;
    _phySize = Footer.CurrentSize + kHeaderSize;
    _startOffset = startPos - Footer.CurrentSize;
    _posInArc = _phySize;
    return S_OK;
  }

  UInt64 fileSize;
  RINOK(InStream_GetSize_SeekToEnd(Stream, fileSize))
  if (fileSize < kHeaderSize)
    return S_FALSE;

  const UInt32 kDynSize = 1024;
  Byte buf[kDynSize];

  RINOK(InStream_SeekSet(Stream, fileSize - kHeaderSize))
  RINOK(ReadStream_FALSE(Stream, buf, kHeaderSize))

  if (!headerIsOK)
  {
    if (!Footer.Parse(buf))
      return S_FALSE;
    _size = Footer.CurrentSize;
    if (Footer.ThereIsDynamic())
      return S_FALSE; // we can't open Dynamic Archive backward.
    // fixed archive
    _posInArcLimit = Footer.CurrentSize;
    _phySize = Footer.CurrentSize + kHeaderSize;
    _startOffset = fileSize - kHeaderSize - Footer.CurrentSize;
    _posInArc = _phySize;
    return S_OK;
  }

  _phySize = kHeaderSize;
  _posInArc = fileSize - startPos;
  _posInArcLimit = _posInArc - kHeaderSize;

  bool headerAndFooterAreEqual = false;
  if (memcmp(header, buf, kHeaderSize) == 0)
  {
    headerAndFooterAreEqual = true;
    _phySize = fileSize - _startOffset;
  }

  RINOK(ReadPhy(Footer.DataOffset, buf, kDynSize))
  if (!Dyn.Parse(buf))
    return S_FALSE;

  UpdatePhySize(Footer.DataOffset + kDynSize);

  for (int i = 0; i < 8; i++)
  {
    const CParentLocatorEntry &locator = Dyn.ParentLocators[i];
    const UInt32 kNameBufSizeMax = 1024;
    if (locator.DataLen < kNameBufSizeMax &&
        locator.DataOffset < _posInArcLimit &&
        locator.DataOffset + locator.DataLen <= _posInArcLimit)
    {
      if (locator.Code == 0x57327275 && (locator.DataLen & 1) == 0)
      {
        // "W2ru" locator
        // Path is encoded as little-endian UTF-16
        Byte nameBuf[kNameBufSizeMax];
        UString tempString;
        unsigned len = (locator.DataLen >> 1);
        {
          wchar_t *s = tempString.GetBuf(len);
          RINOK(ReadPhy(locator.DataOffset, nameBuf, locator.DataLen))
          unsigned j;
          for (j = 0; j < len; j++)
          {
            wchar_t c = GetUi16(nameBuf + j * 2);
            if (c == 0)
              break;
            s[j] = c;
          }
          s[j] = 0;
          tempString.ReleaseBuf_SetLen(j);
        }
        if (tempString[0] == L'.' && tempString[1] == L'\\')
          tempString.DeleteFrontal(2);
        Dyn.RelativeParentNameFromLocator = tempString;
      }
    }
    if (locator.DataLen != 0)
      UpdatePhySize(locator.DataOffset + locator.DataLen);
  }
  
  if (Dyn.NumBlocks >= (UInt32)1 << 31)
    return S_FALSE;
  if (Footer.CurrentSize == 0)
  {
    if (Dyn.NumBlocks != 0)
      return S_FALSE;
  }
  else if (((Footer.CurrentSize - 1) >> Dyn.BlockSizeLog) + 1 != Dyn.NumBlocks)
    return S_FALSE;

  Bat.ClearAndReserve(Dyn.NumBlocks);

  UInt32 bitmapSize = Dyn.NumBitMapSectors() << kSectorSize_Log;

  while ((UInt32)Bat.Size() < Dyn.NumBlocks)
  {
    RINOK(ReadPhy(Dyn.TableOffset + (UInt64)Bat.Size() * 4, buf, kSectorSize))
    UpdatePhySize(Dyn.TableOffset + kSectorSize);
    for (UInt32 j = 0; j < kSectorSize; j += 4)
    {
      UInt32 v = Get32(buf + j);
      if (v != kUnusedBlock)
      {
        UInt32 blockSize = (UInt32)1 << Dyn.BlockSizeLog;
        UpdatePhySize(((UInt64)v << kSectorSize_Log) + bitmapSize + blockSize);
        NumUsedBlocks++;
      }
      Bat.AddInReserved(v);
      if ((UInt32)Bat.Size() >= Dyn.NumBlocks)
        break;
    }
  }

  if (headerAndFooterAreEqual)
    return S_OK;

  if (_startOffset + _phySize + kHeaderSize > fileSize)
  {
    // _unexpectedEnd = true;
    _posInArcLimit = _phySize;
    _phySize += kHeaderSize;
    return S_OK;
  }

  RINOK(ReadPhy(_phySize, buf, kHeaderSize))
  if (memcmp(header, buf, kHeaderSize) == 0)
  {
    _posInArcLimit = _phySize;
    _phySize += kHeaderSize;
    return S_OK;
  }

  if (_phySize == 0x800)
  {
    /* WHY does empty archive contain additional empty sector?
       We skip that sector and check footer again. */
    unsigned i;
    for (i = 0; i < kSectorSize && buf[i] == 0; i++);
    if (i == kSectorSize)
    {
      RINOK(ReadPhy(_phySize + kSectorSize, buf, kHeaderSize))
      if (memcmp(header, buf, kHeaderSize) == 0)
      {
        _phySize += kSectorSize;
        _posInArcLimit = _phySize;
        _phySize += kHeaderSize;
        return S_OK;
      }
    }
  }
  _posInArcLimit = _phySize;
  _phySize += kHeaderSize;
  AddErrorMessage("Can't find footer");
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= Footer.CurrentSize)
    return S_OK;
  {
    const UInt64 rem = Footer.CurrentSize - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }
  if (size == 0)
    return S_OK;

  if (Footer.IsFixed())
  {
    if (_virtPos > _posInArcLimit)
      return S_FALSE;
    {
      const UInt64 rem = _posInArcLimit - _virtPos;
      if (size > rem)
        size = (UInt32)rem;
    }
    HRESULT res = S_OK;
    if (_virtPos != _posInArc)
    {
      _posInArc = _virtPos;
      res = Seek2(_virtPos);
    }
    if (res == S_OK)
    {
      UInt32 processedSize2 = 0;
      res = Stream->Read(data, size, &processedSize2);
      if (processedSize)
        *processedSize = processedSize2;
      _posInArc += processedSize2;
    }
    if (res != S_OK)
      Reset_PosInArc();
    return res;
  }

  const UInt32 blockIndex = (UInt32)(_virtPos >> Dyn.BlockSizeLog);
  if (blockIndex >= Bat.Size())
    return E_FAIL; // it's some unexpected case
  const UInt32 blockSectIndex = Bat[blockIndex];
  const UInt32 blockSize = (UInt32)1 << Dyn.BlockSizeLog;
  UInt32 offsetInBlock = (UInt32)_virtPos & (blockSize - 1);
  size = MyMin(blockSize - offsetInBlock, size);

  HRESULT res = S_OK;
  if (blockSectIndex == kUnusedBlock)
  {
    if (ParentStream)
    {
      RINOK(InStream_SeekSet(ParentStream, _virtPos))
      res = ParentStream->Read(data, size, &size);
    }
    else
      memset(data, 0, size);
  }
  else
  {
    const UInt64 newPos = (UInt64)blockSectIndex << kSectorSize_Log;
    if (BitMapTag != blockIndex)
    {
      RINOK(ReadPhy(newPos, BitMap, (UInt32)BitMap.Size()))
      BitMapTag = blockIndex;
    }
    RINOK(ReadPhy(newPos + BitMap.Size() + offsetInBlock, data, size))
    for (UInt32 cur = 0; cur < size;)
    {
      const UInt32 rem = MyMin(0x200 - (offsetInBlock & 0x1FF), size - cur);
      const UInt32 bmi = offsetInBlock >> kSectorSize_Log;
      if (((BitMap[bmi >> 3] >> (7 - (bmi & 7))) & 1) == 0)
      {
        if (ParentStream)
        {
          RINOK(InStream_SeekSet(ParentStream, _virtPos + cur))
          RINOK(ReadStream_FALSE(ParentStream, (Byte *)data + cur, rem))
        }
        else
        {
          const Byte *p = (const Byte *)data + cur;
          for (UInt32 i = 0; i < rem; i++)
            if (p[i] != 0)
              return S_FALSE;
        }
      }
      offsetInBlock += rem;
      cur += rem;
    }
  }
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  return res;
}


enum
{
  kpidParent = kpidUserDefined,
  kpidSavedState
};

static const CStatProp kArcProps[] =
{
  { NULL, kpidOffset, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidClusterSize, VT_UI8},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidNumVolumes, VT_UI4},
  { NULL, kpidTotalPhySize, VT_UI8},
  { "Parent", kpidParent, VT_BSTR},
  { NULL, kpidCreatorApp, VT_BSTR},
  { NULL, kpidHostOS, VT_BSTR},
  { "Saved State", kpidSavedState, VT_BOOL},
  { NULL, kpidId, VT_BSTR}
 };

static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize,
  kpidCTime
  
  /*
  { kpidNumCyls, VT_UI4},
  { kpidNumHeads, VT_UI4},
  { kpidSectorsPerTrack, VT_UI4}
  */
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

// VHD start time: 2000-01-01
static const UInt64 kVhdTimeStartValue = (UInt64)3600 * 24 * (399 * 365 + 24 * 4);

static void VhdTimeToFileTime(UInt32 vhdTime, NCOM::CPropVariant &prop)
{
  FILETIME ft, utc;
  UInt64 v = (kVhdTimeStartValue + vhdTime) * 10000000;
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
  // specification says that it's UTC time, but Virtual PC 6 writes local time. Why?
  LocalFileTimeToFileTime(&ft, &utc);
  prop = utc;
}

static void StringToAString(char *dest, UInt32 val)
{
  for (int i = 24; i >= 0; i -= 8)
  {
    const Byte b = (Byte)((val >> i) & 0xFF);
    if (b < 0x20 || b > 0x7F)
      break;
    *dest++ = (char)b;
  }
  *dest = 0;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidCTime: VhdTimeToFileTime(Footer.CTime, prop); break;
    case kpidClusterSize: if (Footer.ThereIsDynamic()) prop = (UInt32)1 << Dyn.BlockSizeLog; break;
    case kpidShortComment:
    case kpidMethod:
    {
      AString s;
      Footer.AddTypeString(s);
      if (NeedParent())
      {
        s += " -> ";
        const CHandler *p = this;
        while (p && p->NeedParent())
          p = p->Parent;
        if (!p)
          s += '?';
        else
          p->Footer.AddTypeString(s);
      }
      prop = s;
      break;
    }
    case kpidCreatorApp:
    {
      char s[16];
      StringToAString(s, Footer.CreatorApp);
      AString res (s);
      res.Trim();
      res.Add_Space();
      res.Add_UInt32(Footer.CreatorVersion >> 16);
      res.Add_Dot();
      res.Add_UInt32(Footer.CreatorVersion & 0xFFFF);
      prop = res;
      break;
    }
    case kpidHostOS:
    {
      if (Footer.CreatorHostOS == 0x5769326B)
        prop = "Windows";
      else
      {
        char s[16];
        StringToAString(s, Footer.CreatorHostOS);
        prop = s;
      }
      break;
    }
    case kpidId:
    {
      char s[sizeof(Footer.Id) * 2 + 2];
      ConvertDataToHex_Upper(s, Footer.Id, sizeof(Footer.Id));
      prop = s;
      break;
    }
    case kpidSavedState: prop = Footer.SavedState ? true : false; break;
    case kpidParent: if (NeedParent()) prop = GetParentSequence(); break;
    case kpidOffset: prop = _startOffset; break;
    case kpidPhySize: prop = _phySize; break;
    case kpidTotalPhySize:
    {
      const CHandler *p = this;
      UInt64 sum = 0;
      do
      {
        sum += p->_phySize;
        p = p->Parent;
      }
      while (p);
      prop = sum;
      break;
    }
    case kpidNumVolumes: if (NumLevels != 1) prop = (UInt32)NumLevels; break;

    /*
    case kpidErrorFlags:
    {
      UInt32 flags = 0;
      if (_unexpectedEnd)
        flags |= kpv_ErrorFlags_UnexpectedEndOfArc;
      if (flags != 0)
        prop = flags;
      break;
    }
    */
    case kpidError: if (!_errorMessage.IsEmpty()) prop = _errorMessage; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


HRESULT CHandler::Open2(IInStream *stream, CHandler *child, IArchiveOpenCallback *openArchiveCallback, unsigned level)
{
  Close();
  Stream = stream;
  if (level > (1 << 12)) // Maybe we need to increase that limit
    return S_FALSE;
  
  RINOK(Open3())
  
  NumLevels = 1;
  if (child && memcmp(child->Dyn.ParentId, Footer.Id, 16) != 0)
    return S_FALSE;
  if (Footer.Type != kDiskType_Diff)
    return S_OK;

  bool useRelative;
  UString name;
  
  if (!Dyn.RelativeParentNameFromLocator.IsEmpty())
  {
    useRelative = true;
    name = Dyn.RelativeParentNameFromLocator;
  }
  else
  {
    useRelative = false;
    name = Dyn.ParentName;
  }
  
  Dyn.RelativeNameWasUsed = useRelative;

  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveOpenVolumeCallback,
      openVolumeCallback, openArchiveCallback)
  
  if (openVolumeCallback)
  {
    CMyComPtr<IInStream> nextStream;
    HRESULT res = openVolumeCallback->GetStream(name, &nextStream);
    
    if (res == S_FALSE)
    {
      if (useRelative && Dyn.ParentName != Dyn.RelativeParentNameFromLocator)
      {
        res = openVolumeCallback->GetStream(Dyn.ParentName, &nextStream);
        if (res == S_OK)
          Dyn.RelativeNameWasUsed = false;
      }
    }

    if (res != S_OK && res != S_FALSE)
      return res;
    
    if (res == S_FALSE || !nextStream)
    {
      AddErrorMessage("Missing volume : ", name);
      return S_OK;
    }
    
    Parent = new CHandler;
    ParentStream = Parent;
    
    res = Parent->Open2(nextStream, this, openArchiveCallback, level + 1);

    if (res != S_OK)
    {
      Parent = NULL;
      ParentStream.Release();
      if (res == E_ABORT)
        return res;
      if (res != S_FALSE)
      {
        // we must show that error code
      }
    }
    if (res == S_OK)
    {
      NumLevels = Parent->NumLevels + 1;
    }
  }
  {
    const CHandler *p = this;
    while (p->NeedParent())
    {
      p = p->Parent;
      if (!p)
      {
        AddErrorMessage("Can't open parent VHD file : ", Dyn.ParentName);
        break;
      }
    }
  }
  return S_OK;
}


void CHandler::CloseAtError()
{
  // CHandlerImg:
  Stream.Release();
  Clear_HandlerImg_Vars();

  _phySize = 0;
  NumLevels = 0;
  Bat.Clear();
  NumUsedBlocks = 0;
  Parent = NULL;
  ParentStream.Release();
  Dyn.Clear();
  _errorMessage.Empty();
  // _unexpectedEnd = false;
}

Z7_COM7F_IMF(CHandler::Close())
{
  CloseAtError();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidSize: prop = Footer.CurrentSize; break;
    case kpidPackSize: prop = GetPackSize(); break;
    case kpidCTime: VhdTimeToFileTime(Footer.CTime, prop); break;
    case kpidExtension: prop = (_imgExt ? _imgExt : "img"); break;

    /*
    case kpidNumCyls: prop = Footer.NumCyls(); break;
    case kpidNumHeads: prop = Footer.NumHeads(); break;
    case kpidSectorsPerTrack: prop = Footer.NumSectorsPerTrack(); break;
    */
  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  if (Footer.IsFixed())
  {
    CMyComPtr2<ISequentialInStream, CLimitedInStream> streamSpec;
    streamSpec.Create_if_Empty();
    streamSpec->SetStream(Stream);
    // fixme : check (startOffset = 0)
    streamSpec->InitAndSeek(_startOffset, Footer.CurrentSize);
    RINOK(streamSpec->SeekToStart())
    *stream = streamSpec.Detach();
    return S_OK;
  }
  if (!Footer.ThereIsDynamic() || !AreParentsOK())
    return S_FALSE;
  CMyComPtr<ISequentialInStream> streamTemp = this;
  RINOK(InitAndSeek())
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

REGISTER_ARC_I(
  "VHD", "vhd", NULL, 0xDC,
  kSignature,
  0,
  NArcInfoFlags::kUseGlobalOffset,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/VhdxHandler.cpp ================ */
// VhdxHandler.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

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

#define G32(_offs_, dest) dest = Get32(p + (_offs_))
#define G64(_offs_, dest) dest = Get64(p + (_offs_))

using namespace NWindows;


EXTERN_C_BEGIN

// CRC-32C (Castagnoli) : reversed for poly 0x1EDC6F41
#define k_Crc32c_Poly 0x82f63b78

MY_ALIGN(64)
static UInt32 g_Crc32c_Table[256];

static void Z7_FASTCALL Crc32c_GenerateTable()
{
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
    UInt32 r = i;
    unsigned j;
    for (j = 0; j < 8; j++)
      r = (r >> 1) ^ (k_Crc32c_Poly & ((UInt32)0 - (r & 1)));
    g_Crc32c_Table[i] = r;
  }
}


#define CRC32C_INIT_VAL 0xFFFFFFFF

#define CRC_UPDATE_BYTE_2(crc, b) (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

// UInt32 Z7_FASTCALL CrcUpdateT1(UInt32 v, const void *data, size_t size, const UInt32 *table);
static UInt32 Z7_FASTCALL CrcUpdateT1_vhdx(UInt32 v, const void *data, size_t size, const UInt32 *table)
{
  const Byte *p = (const Byte *)data;
  const Byte *pEnd = p + size;
  for (; p != pEnd; p++)
    v = CRC_UPDATE_BYTE_2(v, *p);
  return v;
}

static UInt32 Z7_FASTCALL Crc32c_Calc(const void *data, size_t size)
{
  return CrcUpdateT1_vhdx(CRC32C_INIT_VAL, data, size, g_Crc32c_Table) ^ CRC32C_INIT_VAL;
}

EXTERN_C_END


namespace NArchive {
namespace NVhdx {

static struct C_CRC32c_TableInit { C_CRC32c_TableInit() { Crc32c_GenerateTable(); } } g_CRC32c_TableInit;

static const unsigned kSignatureSize = 8;
static const Byte kSignature[kSignatureSize] =
  { 'v', 'h', 'd', 'x', 'f', 'i', 'l', 'e' };

static const unsigned kBitmapSize_Log = 20;
static const size_t kBitmapSize = (size_t)1 << kBitmapSize_Log;


static bool IsZeroArr(const Byte *p, size_t size)
{
  for (size_t i = 0; i < size; i++)
    if (p[i] != 0)
      return false;
  return true;
}



Z7_FORCE_INLINE
static int DecodeFrom2HexChars(const wchar_t *s)
{
  unsigned v0 = (unsigned)s[0];  Z7_PARSE_HEX_DIGIT(v0, return -1;)
  unsigned v1 = (unsigned)s[1];  Z7_PARSE_HEX_DIGIT(v1, return -1;)
  return (int)((v0 << 4) | v1);
}


struct CGuid
{
  Byte Data[16];

  bool IsZero() const { return IsZeroArr(Data, 16); }
  bool IsEqualTo(const Byte *a) const { return memcmp(Data, a, 16) == 0; }
  bool IsEqualTo(const CGuid &g) const { return IsEqualTo(g.Data); }
  void AddHexToString(UString &s) const;

  void SetFrom(const Byte *p) { memcpy(Data, p, 16); }

  bool ParseFromFormatedHexString(const UString &s)
  {
    const unsigned kLen = 16 * 2 + 4 + 2;
    if (s.Len() != kLen || s[0] != '{' || s[kLen - 1] != '}')
      return false;
    unsigned pos = 0;
    for (unsigned i = 1; i < kLen - 1;)
    {
      if (i == 9 || i == 14 || i == 19 || i == 24)
      {
        if (s[i] != '-')
          return false;
        i++;
        continue;
      }
      const int v = DecodeFrom2HexChars(s.Ptr(i));
      if (v < 0)
        return false;
      unsigned pos2 = pos;
      if (pos < 8)
        pos2 ^= (pos < 4 ? 3 : 1);
      Data[pos2] = (Byte)v;
      pos++;
      i += 2;
    }
    return true; // pos == 16;
  }
};

void CGuid::AddHexToString(UString &s) const
{
  char temp[sizeof(Data) * 2 + 2];
  ConvertDataToHex_Lower(temp, Data, sizeof(Data));
  s += temp;
}


#define IS_NON_ALIGNED(v) (((v) & 0xFFFFF) != 0)

static const unsigned kHeader_GUID_Index_FileWriteGuid = 0;
static const unsigned kHeader_GUID_Index_DataWriteGuid = 1;
static const unsigned kHeader_GUID_Index_LogGuid = 2;

struct CHeader
{
  UInt64 SequenceNumber;
  // UInt16 LogVersion;
  // UInt16 Version;
  UInt32 LogLength;
  UInt64 LogOffset;
  CGuid Guids[3];

  bool IsEqualTo(const CHeader &h) const
  {
    if (SequenceNumber != h.SequenceNumber)
      return false;
    if (LogLength != h.LogLength)
      return false;
    if (LogOffset != h.LogOffset)
      return false;
    for (unsigned i = 0; i < 3; i++)
      if (!Guids[i].IsEqualTo(h.Guids[i]))
        return false;
    return true;
  }

  bool Parse(Byte *p);
};

static const unsigned kHeader2Size = 1 << 12;

bool CHeader::Parse(Byte *p)
{
  if (Get32(p) != 0x64616568) // "head"
    return false;
  const UInt32 crc = Get32(p + 4);
  SetUi32(p + 4, 0)
  if (Crc32c_Calc(p, kHeader2Size) != crc)
    return false;
  G64(8, SequenceNumber);
  for (unsigned i = 0; i < 3; i++)
    Guids[i].SetFrom(p + 0x10 + 0x10 * i);
  // LogVersion = Get16(p + 0x40);
  /* LogVersion MUST be set to zero, for known log format
     but we don't parse log so we ignore it */
  G32(0x44, LogLength);
  G64(0x48, LogOffset);
  if (Get16(p + 0x42) != 1) // Header format Version
    return false;
  if (IS_NON_ALIGNED(LogLength))
    return false;
  if (IS_NON_ALIGNED(LogOffset))
    return false;
  return true;
  // return IsZeroArr(p + 0x50, kHeader2Size - 0x50);
}



static const Byte kBat[16] =
  { 0x66,0x77,0xC2,0x2D,0x23,0xF6,0x00,0x42,0x9D,0x64,0x11,0x5E,0x9B,0xFD,0x4A,0x08 };
static const Byte kMetadataRegion[16] =
  { 0x06,0xA2,0x7C,0x8B,0x90,0x47,0x9A,0x4B,0xB8,0xFE,0x57,0x5F,0x05,0x0F,0x88,0x6E };

struct CRegionEntry
{
  // CGuid Guid;
  UInt64 Offset;
  UInt32 Len;
  UInt32 Required;

  UInt64 GetEndPos() const { return Offset + Len; }
  bool Parse(const Byte *p);
};

bool CRegionEntry::Parse(const Byte *p)
{
  // Guid.SetFrom(p);
  G64(0x10, Offset);
  G32(0x18, Len);
  G32(0x1c, Required);
  if (IS_NON_ALIGNED(Offset))
    return false;
  if (IS_NON_ALIGNED(Len))
    return false;
  if (Offset + Len < Offset)
    return false;
  return true;
}
  

struct CRegion
{
  bool Bat_Defined;
  bool Meta_Defined;
  UInt64 EndPos;
  UInt64 DataSize;

  CRegionEntry BatEntry;
  CRegionEntry MetaEntry;

  bool Parse(Byte *p);
};


static const size_t kRegionSize = 1 << 16;
static const unsigned kNumRegionEntriesMax = (1 << 11) - 1;

bool CRegion::Parse(Byte *p)
{
  Bat_Defined = false;
  Meta_Defined = false;
  EndPos = 0;
  DataSize = 0;

  if (Get32(p) != 0x69676572) // "regi"
    return false;
  const UInt32 crc = Get32(p + 4);
  SetUi32(p + 4, 0)
  const UInt32 crc_calced = Crc32c_Calc(p, kRegionSize);
  if (crc_calced != crc)
    return false;
  
  const UInt32 EntryCount = Get32(p + 8);
  if (Get32(p + 12) != 0) // reserved field must be set to 0.
    return false;
  if (EntryCount > kNumRegionEntriesMax)
    return false;
  for (UInt32 i = 0; i < EntryCount; i++)
  {
    CRegionEntry e;
    const Byte *p2 = p + 0x10 + 0x20 * (size_t)i;
    if (!e.Parse(p2))
      return false;
    DataSize += e.Len;
    const UInt64 endPos = e.GetEndPos();
    if (EndPos < endPos)
      EndPos = endPos;
    CGuid Guid;
    Guid.SetFrom(p2);
    if (Guid.IsEqualTo(kBat))
    {
      if (Bat_Defined)
        return false;
      BatEntry = e;
      Bat_Defined = true;
    }
    else if (Guid.IsEqualTo(kMetadataRegion))
    {
      if (Meta_Defined)
        return false;
      MetaEntry = e;
      Meta_Defined = true;
    }
    else
    {
      if (e.Required != 0)
        return false;
      // it's allowed to ignore unknown non-required region entries
    }
  }
  /*
  const size_t k = 0x10 + 0x20 * EntryCount;
  return IsZeroArr(p + k, kRegionSize - k);
  */
  return true;
}




struct CMetaEntry
{
  CGuid Guid;
  UInt32 Offset;
  UInt32 Len;
  UInt32 Flags0;
  // UInt32 Flags1;

  bool IsUser()        const { return (Flags0 & 1) != 0; }
  bool IsVirtualDisk() const { return (Flags0 & 2) != 0; }
  bool IsRequired()    const { return (Flags0 & 4) != 0; }

  bool CheckLimit(size_t regionSize) const
  {
    return Offset <= regionSize && Len <= regionSize - Offset;
  }

  bool Parse(const Byte *p);
};


bool CMetaEntry::Parse(const Byte *p)
{
  Guid.SetFrom(p);
  
  G32(0x10, Offset);
  G32(0x14, Len);
  G32(0x18, Flags0);
  UInt32 Flags1;
  G32(0x1C, Flags1);

  if (Offset != 0 && Offset < (1 << 16))
    return false;
  if (Len > (1 << 20))
    return false;
  if (Len == 0 && Offset != 0)
    return false;
  if ((Flags0 >> 3) != 0) // Reserved
    return false;
  if ((Flags1 & 3) != 0) // Reserved2
    return false;
  return true;
}
  

struct CParentPair
{
  UString Key;
  UString Value;
};


struct CMetaHeader
{
  // UInt16 EntryCount;
  bool Guid_Defined;
  bool VirtualDiskSize_Defined;
  bool Locator_Defined;

  unsigned BlockSize_Log;
  unsigned LogicalSectorSize_Log;
  unsigned PhysicalSectorSize_Log;

  UInt32 Flags;
  UInt64 VirtualDiskSize;
  CGuid Guid;
  // CGuid LocatorType;

  CObjectVector<CParentPair> ParentPairs;

  int FindParentKey(const char *name) const
  {
    FOR_VECTOR (i, ParentPairs)
    {
      const CParentPair &pair = ParentPairs[i];
      if (pair.Key.IsEqualTo(name))
        return (int)i;
    }
    return -1;
  }

  bool Is_LeaveBlockAllocated() const { return (Flags & 1) != 0; }
  bool Is_HasParent() const { return (Flags & 2) != 0; }

  void Clear()
  {
    Guid_Defined = false;
    VirtualDiskSize_Defined = false;
    Locator_Defined = false;
    BlockSize_Log = 0;
    LogicalSectorSize_Log = 0;
    PhysicalSectorSize_Log = 0;
    Flags = 0;
    VirtualDiskSize = 0;
    ParentPairs.Clear();
  }

  bool Parse(const Byte *p, size_t size);
};


static unsigned GetLogSize(UInt32 size)
{
  unsigned k;
  for (k = 0; k < 32; k++)
    if (((UInt32)1 << k) == size)
      return k;
  return k;
}

 
static const unsigned kMetadataSize = 8;
static const Byte kMetadata[kMetadataSize] =
  { 'm','e','t','a','d','a','t','a' };

static const unsigned k_Num_MetaEntries_Max = (1 << 11) - 1;

static const Byte kFileParameters[16] =
  { 0x37,0x67,0xa1,0xca,0x36,0xfa,0x43,0x4d,0xb3,0xb6,0x33,0xf0,0xaa,0x44,0xe7,0x6b };
static const Byte kVirtualDiskSize[16] =
  { 0x24,0x42,0xa5,0x2f,0x1b,0xcd,0x76,0x48,0xb2,0x11,0x5d,0xbe,0xd8,0x3b,0xf4,0xb8 };
static const Byte kVirtualDiskID[16] =
  { 0xab,0x12,0xca,0xbe,0xe6,0xb2,0x23,0x45,0x93,0xef,0xc3,0x09,0xe0,0x00,0xc7,0x46 };
static const Byte kLogicalSectorSize[16] =
  { 0x1d,0xbf,0x41,0x81,0x6f,0xa9,0x09,0x47,0xba,0x47,0xf2,0x33,0xa8,0xfa,0xab,0x5f };
static const Byte kPhysicalSectorSize[16] =
  { 0xc7,0x48,0xa3,0xcd,0x5d,0x44,0x71,0x44,0x9c,0xc9,0xe9,0x88,0x52,0x51,0xc5,0x56 };
static const Byte kParentLocator[16] =
  { 0x2d,0x5f,0xd3,0xa8,0x0b,0xb3,0x4d,0x45,0xab,0xf7,0xd3,0xd8,0x48,0x34,0xab,0x0c };

static bool GetString16(UString &s, const Byte *p, size_t size)
{
  s.Empty();
  if (size & 1)
    return false;
  for (size_t i = 0; i < size; i += 2)
  {
    const wchar_t c = Get16(p + i);
    if (c == 0)
      return false;
    s += c;
  }
  return true;
}


bool CMetaHeader::Parse(const Byte *p, size_t size)
{
  if (memcmp(p, kMetadata, kMetadataSize) != 0)
    return false;
  if (Get16(p + 8) != 0) // Reserved
    return false;
  const UInt32 EntryCount = Get16(p + 10);
  if (EntryCount > k_Num_MetaEntries_Max)
    return false;
  if (!IsZeroArr(p + 12, 20)) // Reserved
    return false;

  for (unsigned i = 0; i < EntryCount; i++)
  {
    CMetaEntry e;
    if (!e.Parse(p + 32 + 32 * (size_t)i))
      return false;
    if (!e.CheckLimit(size))
      return false;
    const Byte *p2 = p + e.Offset;
    
    if (e.Guid.IsEqualTo(kFileParameters))
    {
      if (BlockSize_Log != 0)
        return false;
      if (e.Len != 8)
        return false;
      const UInt32 v = Get32(p2);
      Flags = Get32(p2 + 4);
      BlockSize_Log = GetLogSize(v);
      if (BlockSize_Log < 20 || BlockSize_Log > 28) // specification from 1 MB to 256 MB
        return false;
      if ((Flags >> 2) != 0) // reserved
        return false;
    }
    else if (e.Guid.IsEqualTo(kVirtualDiskSize))
    {
      if (VirtualDiskSize_Defined)
        return false;
      if (e.Len != 8)
        return false;
      VirtualDiskSize = Get64(p2);
      VirtualDiskSize_Defined = true;
    }
    else if (e.Guid.IsEqualTo(kVirtualDiskID))
    {
      if (e.Len != 16)
        return false;
      Guid.SetFrom(p2);
      Guid_Defined = true;
    }
    else if (e.Guid.IsEqualTo(kLogicalSectorSize))
    {
      if (LogicalSectorSize_Log != 0)
        return false;
      if (e.Len != 4)
        return false;
      const UInt32 v = Get32(p2);
      LogicalSectorSize_Log = GetLogSize(v);
      if (LogicalSectorSize_Log != 9 && LogicalSectorSize_Log != 12)
        return false;
    }
    else if (e.Guid.IsEqualTo(kPhysicalSectorSize))
    {
      if (PhysicalSectorSize_Log != 0)
        return false;
      if (e.Len != 4)
        return false;
      const UInt32 v = Get32(p2);
      PhysicalSectorSize_Log = GetLogSize(v);
      if (PhysicalSectorSize_Log != 9 && PhysicalSectorSize_Log != 12)
        return false;
    }
    else if (e.Guid.IsEqualTo(kParentLocator))
    {
      if (Locator_Defined)
        return false;
      if (e.Len < 20)
        return false;
      // LocatorType.SetFrom(p2);
      /* Specifies the type of the parent virtual disk.
         is different for each type: VHDX, VHD or iSCSI.
         only "B04AEFB7-D19E-4A81-B789-25B8E9445913" (for VHDX) is supported now
      */
      Locator_Defined = true;
      if (Get16(p2 + 16) != 0) // reserved
        return false;
      const UInt32 KeyValueCount = Get16(p2 + 18);
      if (20 + (UInt32)KeyValueCount * 12 > e.Len)
        return false;
      for (unsigned k = 0; k < KeyValueCount; k++)
      {
        const Byte *p3 = p2 + 20 + (size_t)k * 12;
        const UInt32 KeyOffset   = Get32(p3);
        const UInt32 ValueOffset = Get32(p3 + 4);
        const UInt32 KeyLength   = Get16(p3 + 8);
        const UInt32 ValueLength = Get16(p3 + 10);
        if (KeyOffset > e.Len || KeyLength > e.Len - KeyOffset)
          return false;
        if (ValueOffset > e.Len || ValueLength > e.Len - ValueOffset)
          return false;
        CParentPair pair;
        if (!GetString16(pair.Key, p2 + KeyOffset, KeyLength))
          return false;
        if (!GetString16(pair.Value, p2 + ValueOffset, ValueLength))
          return false;
        ParentPairs.Add(pair);
      }
    }
    else
    {
      if (e.IsRequired())
        return false;
      // return false; // unknown metadata;
    }
  }

  // some properties are required for correct processing

  if (BlockSize_Log == 0)
    return false;
  if (LogicalSectorSize_Log == 0)
    return false;
  if (!VirtualDiskSize_Defined)
    return false;
  if (((UInt32)VirtualDiskSize & ((UInt32)1 << LogicalSectorSize_Log)) != 0)
    return false;

  // vhdx specification sets limit for 64 TB.
  // do we need to check over same limit ?
  const UInt64 kVirtualDiskSize_Max = (UInt64)1 << 46;
  if (VirtualDiskSize > kVirtualDiskSize_Max)
    return false;

  return true;
}



struct CBat
{
  CByteBuffer Data;

  void Clear() { Data.Free(); }
  UInt64 GetItem(size_t n) const
  {
    return Get64(Data + n * 8);
  }
};



Z7_class_CHandler_final: public CHandlerImg
{
  UInt64 _phySize;

  CBat Bat;
  CObjectVector<CByteBuffer> BitMaps;

  unsigned ChunkRatio_Log;
  size_t ChunkRatio;
  size_t TotalBatEntries;

  CMetaHeader Meta;
  CHeader Header;

  UInt32 NumUsedBlocks;
  UInt32 NumUsedBitMaps;
  UInt64 HeadersSize;

  UInt32 NumLevels;
  UInt64 PackSize_Total;

  /*
  UInt64 NumUsed_1MB_Blocks; // data and bitmaps
  bool NumUsed_1MB_Blocks_Defined;
  */

  CMyComPtr<IInStream> ParentStream;
  CHandler *Parent;
  UString _errorMessage;
  UString _creator;

  bool _nonEmptyLog;
  bool _isDataContiguous;
  // bool _batOverlap;

  CGuid _parentGuid;
  bool _parentGuid_IsDefined;
  UStringVector ParentNames;
  UString ParentName_Used;

  const CHandler *_child;
  unsigned _level;
  bool _isCyclic;
  bool _isCyclic_or_CyclicParent;

  void AddErrorMessage(const char *message);
  void AddErrorMessage(const char *message, const wchar_t *name);

  void UpdatePhySize(UInt64 value)
  {
    if (_phySize < value)
      _phySize = value;
  }

  HRESULT Seek2(UInt64 offset);
  HRESULT Read_FALSE(Byte *data, size_t size)
  {
    return ReadStream_FALSE(Stream, data, size);
  }
  HRESULT ReadToBuf_FALSE(CByteBuffer &buf, size_t size)
  {
    buf.Alloc(size);
    return ReadStream_FALSE(Stream, buf, size);
  }
  
  void InitSeekPositions();
  HRESULT ReadPhy(UInt64 offset, void *data, UInt32 size, UInt32 &processed);

  bool IsDiff() const
  {
    // here we suppose that only HasParent() flag is mandatory for Diff archive type
    return Meta.Is_HasParent();
    // return _parentGuid_IsDefined;
  }

  void AddTypeString(AString &s) const
  {
    if (IsDiff())
      s += "Differencing";
    else
    {
      if (Meta.Is_LeaveBlockAllocated())
        s +=  _isDataContiguous ? "fixed" : "fixed-non-cont";
      else
        s += "dynamic";
    }
  }

  void AddComment(UString &s) const;

  UInt64 GetPackSize() const
  {
    return (UInt64)NumUsedBlocks << Meta.BlockSize_Log;
  }

  UString GetParentSequence() const
  {
    const CHandler *p = this;
    UString res;
    while (p && p->IsDiff())
    {
      if (!res.IsEmpty())
        res += " -> ";
      res += ParentName_Used;
      p = p->Parent;
    }
    return res;
  }

  bool AreParentsOK() const
  {
    if (_isCyclic_or_CyclicParent)
      return false;
    const CHandler *p = this;
    while (p->IsDiff())
    {
      p = p->Parent;
      if (!p)
        return false;
    }
    return true;
  }

  // bool ParseLog(CByteBuffer &log);
  bool ParseBat();
  bool CheckBat();

  HRESULT Open3();
  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openArchiveCallback) Z7_override;
  HRESULT OpenParent(IArchiveOpenCallback *openArchiveCallback, bool &_parentFileWasOpen);
  virtual void CloseAtError() Z7_override;

public:
  Z7_IFACE_COM7_IMP(IInArchive_Img)

  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)

  CHandler():
    _child(NULL),
    _level(0),
    _isCyclic(false),
    _isCyclic_or_CyclicParent(false)
    {}
};


HRESULT CHandler::Seek2(UInt64 offset)
{
  return InStream_SeekSet(Stream, offset);
}


void CHandler::InitSeekPositions()
{
  /* (_virtPos) and (_posInArc) is used only in Read() (that calls ReadPhy()).
     So we must reset these variables before first call of Read() */
  Reset_VirtPos();
  Reset_PosInArc();
  if (ParentStream)
    Parent->InitSeekPositions();
}


HRESULT CHandler::ReadPhy(UInt64 offset, void *data, UInt32 size, UInt32 &processed)
{
  processed = 0;
  if (offset > _phySize
      || offset + size > _phySize)
  {
    // we don't expect these cases, if (_phySize) was set correctly.
    return S_FALSE;
  }
  if (offset != _posInArc)
  {
    const HRESULT res = Seek2(offset);
    if (res != S_OK)
    {
      Reset_PosInArc(); // we don't trust seek_pos in case of error
      return res;
    }
    _posInArc = offset;
  }
  {
    size_t size2 = size;
    const HRESULT res = ReadStream(Stream, data, &size2);
    processed = (UInt32)size2;
    _posInArc += size2;
    if (res != S_OK)
      Reset_PosInArc(); // we don't trust seek_pos in case of reading error
    return res;
  }
}


#define PAYLOAD_BLOCK_NOT_PRESENT   0
#define PAYLOAD_BLOCK_UNDEFINED     1
#define PAYLOAD_BLOCK_ZERO          2
#define PAYLOAD_BLOCK_UNMAPPED      3
#define PAYLOAD_BLOCK_FULLY_PRESENT 6
#define PAYLOAD_BLOCK_PARTIALLY_PRESENT 7

#define SB_BLOCK_NOT_PRESENT 0
#define SB_BLOCK_PRESENT     6

#define BAT_GET_OFFSET(v) ((v) & ~(UInt64)0xFFFFF)
#define BAT_GET_STATE(v)  ((UInt32)(v) & 7)

/* The log contains only   updates to metadata, bat and region tables
   The log doesn't contain updates to start header, and 2 headers (first 192 KB of file).
   The log is array of 4 KB blocks and each block has 4-byte signature.
   So it's possible to scan whole log to find the latest entry sequence (and header for replay).
*/

/*
struct CLogEntry
{
  UInt32 EntryLength;
  UInt32 Tail;
  UInt64 SequenceNumber;
  CGuid LogGuid;
  UInt32 DescriptorCount;
  UInt64 FlushedFileOffset;
  UInt64 LastFileOffset;
  
  bool Parse(const Byte *p);
};

bool CLogEntry::Parse(const Byte *p)
{
  G32 (8, EntryLength);
  G32 (12,Tail);
  G64 (16, SequenceNumber);
  G32 (24, DescriptorCount); // it's 32-bit, but specification says 64-bit
  if (Get32(p + 28) != 0) // reserved
    return false;
  LogGuid.SetFrom(p + 32);
  G64 (48, FlushedFileOffset);
  G64 (56, LastFileOffset);

  if (SequenceNumber == 0)
    return false;
  if ((Tail & 0xfff) != 0)
    return false;
  if (IS_NON_ALIGNED(FlushedFileOffset))
    return false;
  if (IS_NON_ALIGNED(LastFileOffset))
    return false;
  return true;
}


bool CHandler::ParseLog(CByteBuffer &log)
{
  CLogEntry lastEntry;
  lastEntry.SequenceNumber = 0;
  bool lastEntry_found = false;
  size_t lastEntry_Offset = 0;
  for (size_t i = 0; i < log.Size(); i += 1 << 12)
  {
    Byte *p = (Byte *)(log + i);

    if (Get32(p) != 0x65676F6C) // "loge"
      continue;
    const UInt32 crc = Get32(p + 4);

    CLogEntry e;
    if (!e.Parse(p))
    {
      return false;
      continue;
    }
    const UInt32 entryLength = Get32(p + 8);
    if (e.EntryLength > log.Size() || (e.EntryLength & 0xFFF) != 0 || e.EntryLength == 0)
    {
      return false;
      continue;
    }
    SetUi32(p + 4, 0);
    const UInt32 crc_calced = Crc32c_Calc(p, entryLength);
    SetUi32(p + 4, crc); // we must restore crc if we want same data in log
    if (crc_calced != crc)
      continue;
    if (!lastEntry_found || lastEntry.SequenceNumber < e.SequenceNumber)
    {
      lastEntry = e;
      lastEntry_found = true;
      lastEntry_Offset = i;
    }
  }

  return true;
}
*/


bool CHandler::ParseBat()
{
  ChunkRatio_Log = kBitmapSize_Log + 3 + Meta.LogicalSectorSize_Log - Meta.BlockSize_Log;
  ChunkRatio = (size_t)1 << (ChunkRatio_Log);
  
  UInt64 totalBatEntries64;
  const bool isDiff = IsDiff();
  const UInt32 blockSize = (UInt32)1 << Meta.BlockSize_Log;
  {
    const UInt64 up = Meta.VirtualDiskSize + blockSize - 1;
    if (up < Meta.VirtualDiskSize)
      return false;
    const UInt64 numDataBlocks = up >> Meta.BlockSize_Log;
    
    if (isDiff)
    {
      // differencing table must be finished with bitmap entry
      const UInt64 numBitmaps = (numDataBlocks + ChunkRatio - 1) >> ChunkRatio_Log;
      totalBatEntries64 = numBitmaps * (ChunkRatio + 1);
    }
    else
    {
      // we don't need last Bitmap entry
      totalBatEntries64 = numDataBlocks + ((numDataBlocks - 1) >> ChunkRatio_Log);
    }
  }
  
  if (totalBatEntries64 > Bat.Data.Size() / 8)
    return false;

  const size_t totalBatEntries = (size_t)totalBatEntries64;
  TotalBatEntries = totalBatEntries;
  
  bool isCont = (!isDiff && Meta.Is_LeaveBlockAllocated());
  UInt64 prevBlockOffset = 0;
  UInt64 maxBlockOffset = 0;

  size_t remEntries = ChunkRatio + 1;
  
  size_t i;
  for (i = 0; i < totalBatEntries; i++)
  {
    const UInt64 v = Bat.GetItem(i);
    if ((v & 0xFFFF8) != 0)
      return false;
    const UInt64 offset = BAT_GET_OFFSET(v);
    const unsigned state = BAT_GET_STATE(v);
    
    /*
    UInt64 index64 = v >> 20;
    printf("\n%7d", i);
    printf("%10d, ", (unsigned)index64);
    printf("%4x, ", (unsigned)state);
    */
    
    remEntries--;
    if (remEntries == 0)
    {
      // printf(" ========");
      // printf("\n");
      remEntries = ChunkRatio + 1;
      if (state == SB_BLOCK_PRESENT)
      {
        isCont = false;
        if (!isDiff)
          return false;
        if (offset == 0)
          return false;
        const UInt64 lim = offset + kBitmapSize;
        if (lim < offset)
          return false;
        if (_phySize < lim)
          _phySize = lim;
        NumUsedBitMaps++;
      }
      else if (state != SB_BLOCK_NOT_PRESENT)
        return false;
    }
    else
    {
      if (state == PAYLOAD_BLOCK_FULLY_PRESENT
          || state == PAYLOAD_BLOCK_PARTIALLY_PRESENT)
      {
        if (offset == 0)
          return false;
        if (maxBlockOffset < offset)
          maxBlockOffset = offset;

        if (state == PAYLOAD_BLOCK_PARTIALLY_PRESENT)
        {
          isCont = false;
          if (!isDiff)
            return false;
        }
        else if (isCont)
        {
          if (prevBlockOffset != 0 && prevBlockOffset + blockSize != offset)
            isCont = false;
          else
            prevBlockOffset = offset;
        }

        NumUsedBlocks++;
      }
      else if (state == PAYLOAD_BLOCK_UNMAPPED)
      {
        isCont = false;
        // non-empty (offset) is allowed
      }
      else if (state == PAYLOAD_BLOCK_NOT_PRESENT
          || state == PAYLOAD_BLOCK_UNDEFINED
          || state == PAYLOAD_BLOCK_ZERO)
      {
        isCont = false;
        /* (offset) is reserved and (offset == 0) is expected here,
           but we ignore (offset) here */
        // if (offset != 0) return false;
      }
      else
        return false;
    }
  }

  _isDataContiguous = isCont;

  if (maxBlockOffset != 0)
  {
    const UInt64 lim = maxBlockOffset + blockSize;
    if (lim < maxBlockOffset)
      return false;
    if (_phySize < lim)
      _phySize = lim;
    const UInt64 kPhyLimit = (UInt64)1 << 62;
    if (maxBlockOffset >= kPhyLimit)
      return false;
  }
  return true;
}
    

bool CHandler::CheckBat()
{
  const UInt64 upSize = _phySize + kBitmapSize * 8 - 1;
  if (upSize < _phySize)
    return false;
  const UInt64 useMapSize64 = upSize >> (kBitmapSize_Log + 3);
  const size_t useMapSize = (size_t)useMapSize64;
  
  const UInt32 blockSizeMB = (UInt32)1 << (Meta.BlockSize_Log - kBitmapSize_Log);

  // we don't check useMap, if it's too big.
  if (useMapSize != useMapSize64)
    return true;
  if (useMapSize == 0 || useMapSize > ((size_t)1 << 28))
    return true;

  CByteArr useMap;
  useMap.Alloc(useMapSize);
  memset(useMap, 0, useMapSize);
  // useMap[0] = (Byte)(1 << 0); // first 1 MB is used by headers
  // we can also update useMap for log, and region data.
  
  const size_t totalBatEntries = TotalBatEntries;
  size_t remEntries = ChunkRatio + 1;
  
  size_t i;
  for (i = 0; i < totalBatEntries; i++)
  {
    const UInt64 v = Bat.GetItem(i);
    const UInt64 offset = BAT_GET_OFFSET(v);
    const unsigned state = BAT_GET_STATE(v);
    const UInt64 index = offset >> kBitmapSize_Log;
    UInt32 numBlocks = 1;
    remEntries--;
    if (remEntries == 0)
    {
      remEntries = ChunkRatio + 1;
      if (state != SB_BLOCK_PRESENT)
        continue;
    }
    else
    {
      if (state != PAYLOAD_BLOCK_FULLY_PRESENT &&
          state != PAYLOAD_BLOCK_PARTIALLY_PRESENT)
        continue;
      numBlocks = blockSizeMB;
    }
    
    for (unsigned k = 0; k < numBlocks; k++)
    {
      const UInt64 index2 = index + k;
      const unsigned flag = (unsigned)1 << ((unsigned)index2 & 7);
      const size_t byteIndex = (size_t)(index2 >> 3);
      if (byteIndex >= useMapSize)
        return false;
      const unsigned m = useMap[byteIndex];
      if (m & flag)
        return false;
      useMap[byteIndex] = (Byte)(m | flag);
    }
  }

  /*
  UInt64 num = 0;
  for (i = 0; i < useMapSize; i++)
  {
    Byte b = useMap[i];
    unsigned t = 0;
    t += (b & 1);  b >>= 1;
    t += (b & 1);  b >>= 1;
    t += (b & 1);  b >>= 1;
    t += (b & 1);  b >>= 1;
    t += (b & 1);  b >>= 1;
    t += (b & 1);  b >>= 1;
    t += (b & 1);  b >>= 1;
    t += (b & 1);
    num += t;
  }
  NumUsed_1MB_Blocks = num;
  NumUsed_1MB_Blocks_Defined = true;
  */
  
  return true;
}



HRESULT CHandler::Open3()
{
  {
    const unsigned kHeaderSize = 512; // + 8
    Byte header[kHeaderSize];
    
    RINOK(Read_FALSE(header, kHeaderSize))

    if (memcmp(header, kSignature, kSignatureSize) != 0)
      return S_FALSE;

    const Byte *p = &header[0];
    for (unsigned i = kSignatureSize; i < kHeaderSize; i += 2)
    {
      const wchar_t c = Get16(p + i);
      if (c < 0x20 || c > 0x7F)
        break;
      _creator += c;
    }
  }

  HeadersSize = (UInt32)1 << 20;
  CHeader headers[2];
  {
    Byte header[kHeader2Size];
    for (unsigned i = 0; i < 2; i++)
    {
      RINOK(Seek2((1 << 16) * (1 + i)))
      RINOK(Read_FALSE(header, kHeader2Size))
      bool headerIsOK = headers[i].Parse(header);
      if (!headerIsOK)
        return S_FALSE;
    }
  }
  unsigned mainIndex;
       if (headers[0].SequenceNumber > headers[1].SequenceNumber) mainIndex = 0;
  else if (headers[0].SequenceNumber < headers[1].SequenceNumber) mainIndex = 1;
  else
  {
    /* Disk2vhd v2.02 can create image with 2 full copies of headers.
       It's violation of VHDX specification:
          "A header is current if it is the only valid header
           or if it is valid and its SequenceNumber field is
           greater than the other header's SequenceNumber".
       but we support such Disk2vhd archives. */
    if (!headers[0].IsEqualTo(headers[1]))
      return S_FALSE;
    mainIndex = 0;
  }

  const CHeader &h = headers[mainIndex];
  Header = h;
  if (h.LogLength != 0)
  {
    HeadersSize += h.LogLength;
    UpdatePhySize(h.LogOffset + h.LogLength);
    if (!h.Guids[kHeader_GUID_Index_LogGuid].IsZero())
    {
      _nonEmptyLog = true;
      AddErrorMessage("non-empty LOG was not replayed");
      /*
      if (h.LogVersion != 0)
        AddErrorMessage("unknown LogVresion");
      else
      {
        CByteBuffer log;
        RINOK(Seek2(h.LogOffset));
        RINOK(ReadToBuf_FALSE(log, h.LogLength));
        if (!ParseLog(log))
        {
          return S_FALSE;
        }
      }
      */
    }
  }
  CRegion regions[2];
  int correctRegionIndex = -1;

  {
    CByteBuffer temp;
    temp.Alloc(kRegionSize * 2);
    RINOK(Seek2((1 << 16) * 3))
    RINOK(Read_FALSE(temp, kRegionSize * 2))
    unsigned numTables = 1;
    if (memcmp(temp, temp + kRegionSize, kRegionSize) != 0)
    {
      AddErrorMessage("Region tables mismatch");
      numTables = 2;
    }

    for (unsigned i = 0; i < numTables; i++)
    {
      // RINOK(Seek2((1 << 16) * (3 + i)));
      // RINOK(Read_FALSE(temp, kRegionSize));
      if (regions[i].Parse(temp))
      {
        if (correctRegionIndex < 0)
          correctRegionIndex = (int)i;
      }
      else
      {
        AddErrorMessage("Incorrect region table");
      }
    }
    if (correctRegionIndex < 0)
      return S_FALSE;
    /*
    if (!regions[0].IsEqualTo(regions[1]))
      return S_FALSE;
    */
  }
  
  // UpdatePhySize((1 << 16) * 5);
  UpdatePhySize(1 << 20);
  
  {
    const CRegion &region = regions[correctRegionIndex];
    HeadersSize += region.DataSize;
    UpdatePhySize(region.EndPos);
    {
      if (!region.Meta_Defined)
        return S_FALSE;
      const CRegionEntry &e = region.MetaEntry;
      if (e.Len == 0)
        return S_FALSE;
      {
        // static const kMetaTableSize = 1 << 16;
        CByteBuffer temp;
        {
          RINOK(Seek2(e.Offset))
          RINOK(ReadToBuf_FALSE(temp, e.Len))
        }
        if (!Meta.Parse(temp, temp.Size()))
          return S_FALSE;
      }
      // UpdatePhySize(e.GetEndPos());
    }
    {
      if (!region.Bat_Defined)
        return S_FALSE;
      const CRegionEntry &e = region.BatEntry;
      if (e.Len == 0)
        return S_FALSE;
      // UpdatePhySize(e.GetEndPos());
      {
        RINOK(Seek2(e.Offset))
        RINOK(ReadToBuf_FALSE(Bat.Data, e.Len))
      }
      if (!ParseBat())
        return S_FALSE;
      if (!CheckBat())
      {
        AddErrorMessage("BAT overlap");
        // _batOverlap = true;
        // return S_FALSE;
      }
    }
  }

  {
    // do we need to check "parent_linkage2" also?
    FOR_VECTOR (i, Meta.ParentPairs)
    {
      const CParentPair &pair = Meta.ParentPairs[i];
      if (pair.Key.IsEqualTo("parent_linkage"))
      {
        _parentGuid_IsDefined = _parentGuid.ParseFromFormatedHexString(pair.Value);
        break;
      }
    }
  }

  {
    // absolute paths for parent stream can be rejected later in client callback
    // the order of check by specification:
    const char * const g_ParentKeys[] =
    {
        "relative_path"       // "..\..\path2\sub3\parent.vhdx"
      , "volume_path"         // "\\?\Volume{26A21BDA-A627-11D7-9931-806E6F6E6963}\path2\sub3\parent.vhdx")
      , "absolute_win32_path" // "d:\path2\sub3\parent.vhdx"
    };
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_ParentKeys); i++)
    {
      const int index = Meta.FindParentKey(g_ParentKeys[i]);
      if (index < 0)
        continue;
      ParentNames.Add(Meta.ParentPairs[index].Value);
    }
  }

  if (Meta.Is_HasParent())
  {
    if (!Meta.Locator_Defined)
      AddErrorMessage("Parent locator is not defined");
    else
    {
      if (!_parentGuid_IsDefined)
        AddErrorMessage("Parent GUID is not defined");
      if (ParentNames.IsEmpty())
        AddErrorMessage("Parent VHDX file name is not defined");
    }
  }
  else
  {
    if (Meta.Locator_Defined)
      AddErrorMessage("Unexpected parent locator");
  }

  // here we suppose that and locator can be used only with HasParent flag
    
  // return S_FALSE;

  _size = Meta.VirtualDiskSize; // CHandlerImg

  // _posInArc = 0;
  // Reset_PosInArc();
  // RINOK(InStream_SeekToBegin(Stream))

  return S_OK;
}


/*
static UInt32 g_NumCalls = 0;
static UInt32 g_NumCalls2 = 0;
static struct CCounter { ~CCounter()
{
  printf("\nNumCalls = %10u\n", g_NumCalls);
  printf("NumCalls2 = %10u\n", g_NumCalls2);
} } g_Counter;
*/

Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  // g_NumCalls++;
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= Meta.VirtualDiskSize)
    return S_OK;
  {
    const UInt64 rem = Meta.VirtualDiskSize - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }
  if (size == 0)
    return S_OK;
  const size_t blockIndex = (size_t)(_virtPos >> Meta.BlockSize_Log);
  const size_t chunkIndex = blockIndex >> ChunkRatio_Log;
  const size_t chunkRatio = (size_t)1 << ChunkRatio_Log;
  const size_t blockIndex2 = chunkIndex * (chunkRatio + 1) + (blockIndex & (chunkRatio - 1));
  const UInt64 blockSectVal = Bat.GetItem(blockIndex2);
  const UInt64 blockOffset = BAT_GET_OFFSET(blockSectVal);
  const UInt32 blockState = BAT_GET_STATE(blockSectVal);

  const UInt32 blockSize = (UInt32)1 << Meta.BlockSize_Log;
  const UInt32 offsetInBlock = (UInt32)_virtPos & (blockSize - 1);
  size = MyMin(blockSize - offsetInBlock, size);

  bool needParent = false;
  bool needRead = false;
  
  if (blockState == PAYLOAD_BLOCK_FULLY_PRESENT)
    needRead = true;
  else if (blockState == PAYLOAD_BLOCK_NOT_PRESENT)
  {
    /* for a differencing VHDX: parent virtual disk SHOULD be
         inspected to determine the associated contents (SPECIFICATION).
         we suppose that we should not check BitMap.
       for fixed or dynamic VHDX files: the block contents are undefined and
         can contain arbitrary data (SPECIFICATION). NTFS::pagefile.sys can use such state. */
    if (IsDiff())
      needParent = true;
  }
  else if (blockState == PAYLOAD_BLOCK_PARTIALLY_PRESENT)
  {
    // only allowed for differencing VHDX files.
    // associated sector bitmap block MUST be valid
    if (chunkIndex >= BitMaps.Size())
      return S_FALSE;
    // else
    {
      const CByteBuffer &bitmap = BitMaps[(unsigned)chunkIndex];
      const Byte *p = (const Byte *)bitmap;
      if (!p)
        return S_FALSE;
      // else
      {
        // g_NumCalls2++;
        const UInt64 sectorIndex = _virtPos >> Meta.LogicalSectorSize_Log;

        #define BIT_MAP_UNIT_LOG  3 // it's for small block (4 KB)
        // #define BIT_MAP_UNIT_LOG  5 // speed optimization for large blocks (16 KB)

        const size_t offs = (size_t)(sectorIndex >> 3) &
            (
              (kBitmapSize - 1)
              & ~(((UInt32)1 << (BIT_MAP_UNIT_LOG - 3)) - 1)
            );

        unsigned sector2 = (unsigned)sectorIndex & ((1 << BIT_MAP_UNIT_LOG) - 1);
      #if BIT_MAP_UNIT_LOG == 5
        UInt32 v = GetUi32(p + offs) >> sector2;
      #else
        unsigned v = (unsigned)p[offs] >> sector2;
      #endif
        // UInt32 v = GetUi32(p + offs) >> sector2;
        const UInt32 sectorSize = (UInt32)1 << Meta.LogicalSectorSize_Log;
        const UInt32 offsetInSector = (UInt32)_virtPos & (sectorSize - 1);
        const unsigned bit = (unsigned)(v & 1);
        if (bit)
          needRead = true;
        else
          needParent = true; // zero - from the parent VHDX file
        UInt32 rem = sectorSize - offsetInSector;
        for (sector2++; sector2 < (1 << BIT_MAP_UNIT_LOG); sector2++)
        {
          v >>= 1;
          if (bit != (v & 1))
            break;
          rem += sectorSize;
        }
        if (size > rem)
          size = rem;
      }
    }
  }

  bool needZero = true;

  HRESULT res = S_OK;

  if (needParent)
  {
    if (!ParentStream)
      return S_FALSE;
    // if (ParentStream)
    {
      RINOK(InStream_SeekSet(ParentStream, _virtPos))
      size_t processed = size;
      res = ReadStream(ParentStream, (Byte *)data, &processed);
      size = (UInt32)processed;
      needZero = false;
    }
  }
  else if (needRead)
  {
    UInt32 processed = 0;
    res = ReadPhy(blockOffset + offsetInBlock, data, size, processed);
    size = processed;
    needZero = false;
  }

  if (needZero)
    memset(data, 0, size);

  if (processedSize)
    *processedSize = size;

  _virtPos += size;
  return res;
}


enum
{
  kpidParent = kpidUserDefined
};

static const CStatProp kArcProps[] =
{
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidSectorSize, VT_UI4},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidNumVolumes, VT_UI4},
  { NULL, kpidTotalPhySize, VT_UI8},
  { "Parent", kpidParent, VT_BSTR},
  { NULL, kpidCreatorApp, VT_BSTR},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidId, VT_BSTR}
 };

static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME


void CHandler::AddErrorMessage(const char *message)
{
  if (!_errorMessage.IsEmpty())
    _errorMessage.Add_LF();
  _errorMessage += message;
}

void CHandler::AddErrorMessage(const char *message, const wchar_t *name)
{
  AddErrorMessage(message);
  _errorMessage += name;
}


static void AddComment_Name(UString &s, const char *name)
{
  s += name;
  s += ": ";
}

static void AddComment_Bool(UString &s, const char *name, bool val)
{
  AddComment_Name(s, name);
  s.Add_Char(val ? '+' : '-');
  s.Add_LF();
}

static void AddComment_UInt64(UString &s, const char *name, UInt64 v, bool showMB = false)
{
  AddComment_Name(s, name);
  s.Add_UInt64(v);
  if (showMB)
  {
    s += " (";
    s.Add_UInt64(v >> 20);
    s += " MiB)";
  }
  s.Add_LF();
}

static void AddComment_BlockSize(UString &s, const char *name, unsigned logSize)
{
  if (logSize != 0)
    AddComment_UInt64(s, name, ((UInt64)1 << logSize));
}


void CHandler::AddComment(UString &s) const
{
  AddComment_UInt64(s, "VirtualDiskSize", Meta.VirtualDiskSize);
  AddComment_UInt64(s, "PhysicalSize", _phySize);

  if (!_errorMessage.IsEmpty())
  {
    AddComment_Name(s, "Error");
    s += _errorMessage;
    s.Add_LF();
  }

  if (Meta.Guid_Defined)
  {
    AddComment_Name(s, "Id");
    Meta.Guid.AddHexToString(s);
    s.Add_LF();
  }
  
  AddComment_UInt64(s, "SequenceNumber", Header.SequenceNumber);
  AddComment_UInt64(s, "LogLength", Header.LogLength, true);
  
  for (unsigned i = 0; i < 3; i++)
  {
    const CGuid &g = Header.Guids[i];
    if (g.IsZero())
      continue;
    if (i == 0)
      s += "FileWrite";
    else if (i == 1)
      s += "DataWrite";
    else
      s += "Log";
    AddComment_Name(s, "Guid");
    g.AddHexToString(s);
    s.Add_LF();
  }

  AddComment_Bool(s, "HasParent", Meta.Is_HasParent());
  AddComment_Bool(s, "Fixed", Meta.Is_LeaveBlockAllocated());
  if (Meta.Is_LeaveBlockAllocated())
    AddComment_Bool(s, "DataContiguous", _isDataContiguous);
  
  AddComment_BlockSize(s, "BlockSize", Meta.BlockSize_Log);
  AddComment_BlockSize(s, "LogicalSectorSize", Meta.LogicalSectorSize_Log);
  AddComment_BlockSize(s, "PhysicalSectorSize", Meta.PhysicalSectorSize_Log);

  {
    const UInt64 packSize = GetPackSize();
    AddComment_UInt64(s, "PackSize", packSize, true);
    const UInt64 headersSize = HeadersSize + ((UInt64)NumUsedBitMaps << kBitmapSize_Log);
    AddComment_UInt64(s, "HeadersSize", headersSize, true);
    AddComment_UInt64(s, "FreeSpace", _phySize - packSize - headersSize, true);
    /*
    if (NumUsed_1MB_Blocks_Defined)
      AddComment_UInt64(s, "used2", (NumUsed_1MB_Blocks << 20));
    */
  }

  if (Meta.ParentPairs.Size() != 0)
  {
    s += "Parent:";
    s.Add_LF();
    FOR_VECTOR(i, Meta.ParentPairs)
    {
      const CParentPair &pair = Meta.ParentPairs[i];
      s += "  ";
      s += pair.Key;
      s += ": ";
      s += pair.Value;
      s.Add_LF();
    }
    s.Add_LF();
  }
}



Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidClusterSize: prop = (UInt32)1 << Meta.BlockSize_Log; break;
    case kpidSectorSize: prop = (UInt32)1 << Meta.LogicalSectorSize_Log; break;
    case kpidShortComment:
    case kpidMethod:
    {
      AString s;
      AddTypeString(s);
      if (IsDiff())
      {
        s += " -> ";
        const CHandler *p = this;
        while (p && p->IsDiff())
          p = p->Parent;
        if (!p)
          s += '?';
        else
          p->AddTypeString(s);
      }
      prop = s;
      break;
    }
    case kpidComment:
    {
      UString s;
      {
        if (NumLevels > 1)
        {
          AddComment_UInt64(s, "NumVolumeLevels", NumLevels);
          AddComment_UInt64(s, "PackSizeTotal", PackSize_Total, true);
          s += "----";
          s.Add_LF();
        }

        const CHandler *p = this;
        for (;;)
        {
          if (p->_level != 0 || p->Parent)
            AddComment_UInt64(s, "VolumeLevel", p->_level + 1);
          p->AddComment(s);
          if (!p->Parent)
            break;
          s += "----";
          s.Add_LF();
          {
            s.Add_LF();
            if (!p->ParentName_Used.IsEmpty())
            {
              AddComment_Name(s, "Name");
              s += p->ParentName_Used;
              s.Add_LF();
            }
          }
          p = p->Parent;
        }
      }
      prop = s;
      break;
    }
    case kpidCreatorApp:
    {
      if (!_creator.IsEmpty())
        prop = _creator;
      break;
    }
    case kpidId:
    {
      if (Meta.Guid_Defined)
      {
        UString s;
        Meta.Guid.AddHexToString(s);
        prop = s;
      }
      break;
    }
    case kpidName:
    {
      if (Meta.Guid_Defined)
      {
        UString s;
        Meta.Guid.AddHexToString(s);
        s += ".vhdx";
        prop = s;
      }
      break;
    }
    case kpidParent: if (IsDiff()) prop = GetParentSequence(); break;
    case kpidPhySize: prop = _phySize; break;
    case kpidTotalPhySize:
    {
      const CHandler *p = this;
      UInt64 sum = 0;
      do
      {
        sum += p->_phySize;
        p = p->Parent;
      }
      while (p);
      prop = sum;
      break;
    }
    case kpidNumVolumes: if (NumLevels != 1) prop = (UInt32)NumLevels; break;
    case kpidError:
    {
      UString s;
      const CHandler *p = this;
      do
      {
        if (!p->_errorMessage.IsEmpty())
        {
          if (!s.IsEmpty())
            s.Add_LF();
          s += p->_errorMessage;
        }
        p = p->Parent;
      }
      while (p);
      if (!s.IsEmpty())
        prop = s;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openArchiveCallback)
{
  Stream = stream;
  if (_level >= (1 << 20))
    return S_FALSE;

  RINOK(Open3())

  NumLevels = 1;
  PackSize_Total = GetPackSize();

  if (_child)
  {
    if (!_child->_parentGuid.IsEqualTo(Header.Guids[kHeader_GUID_Index_DataWriteGuid]))
      return S_FALSE;
    const CHandler *child = _child;
    do
    {
      /* We suppose that only FileWriteGuid is unique.
         Another IDs must be identical in in difference and parent archives. */
      if (Header.Guids[kHeader_GUID_Index_FileWriteGuid].IsEqualTo(
              child->Header.Guids[kHeader_GUID_Index_FileWriteGuid])
          && _phySize == child->_phySize)
      {
        _isCyclic = true;
        _isCyclic_or_CyclicParent = true;
        AddErrorMessage("Cyclic parent archive was blocked");
        return S_OK;
      }
      child = child->_child;
    }
    while (child);
  }

  if (!Meta.Is_HasParent())
    return S_OK;

  if (!Meta.Locator_Defined
      || !_parentGuid_IsDefined
      || ParentNames.IsEmpty())
  {
    return S_OK;
  }

  ParentName_Used = ParentNames.Front();

  HRESULT res;
  const unsigned kNumLevelsMax = (1 << 8);  // Maybe we need to increase that limit
  if (_level >= kNumLevelsMax - 1)
  {
    AddErrorMessage("Too many parent levels");
    return S_OK;
  }
  
  bool _parentFileWasOpen = false;
  
  if (!openArchiveCallback)
    res = S_FALSE;
  else
    res = OpenParent(openArchiveCallback, _parentFileWasOpen);
  
  if (res != S_OK)
  {
    if (res != S_FALSE)
      return res;
    
    if (_parentFileWasOpen)
      AddErrorMessage("Can't parse parent VHDX file : ", ParentName_Used);
    else
      AddErrorMessage("Missing parent VHDX file : ", ParentName_Used);
  }


  return S_OK;
}


HRESULT CHandler::OpenParent(IArchiveOpenCallback *openArchiveCallback, bool &_parentFileWasOpen)
{
  _parentFileWasOpen = false;
  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveOpenVolumeCallback,
      openVolumeCallback, openArchiveCallback)
  if (!openVolumeCallback)
    return S_FALSE;
  {
    CMyComPtr<IInStream> nextStream;
    HRESULT res = S_FALSE;
    UString name;
    
    FOR_VECTOR (i, ParentNames)
    {
      name = ParentNames[i];
      
      // we remove prefix ".\\', but client already can support any variant
      if (name[0] == L'.' && name[1] == L'\\')
        name.DeleteFrontal(2);

      res = openVolumeCallback->GetStream(name, &nextStream);

      if (res == S_OK && nextStream)
        break;
         
      if (res != S_OK && res != S_FALSE)
        return res;
    }
    
    if (res == S_FALSE || !nextStream)
      return S_FALSE;
    
    ParentName_Used = name;
    _parentFileWasOpen = true;

    Parent = new CHandler;
    ParentStream = Parent;

    try
    {
      Parent->_level = _level + 1;
      Parent->_child = this;
      /* we could call CHandlerImg::Open() here.
         but we don't need (_imgExt) in (Parent). So we call Open2() here */
      Parent->Close();
      res = Parent->Open2(nextStream, openArchiveCallback);
    }
    catch(...)
    {
      Parent = NULL;
      ParentStream.Release();
      res = S_FALSE;
      throw;
    }

    if (res != S_OK)
    {
      Parent = NULL;
      ParentStream.Release();
      if (res == E_ABORT)
        return res;
      if (res != S_FALSE)
      {
        // we must show that error code
      }
    }

    if (res == S_OK)
    {
      if (Parent->_isCyclic_or_CyclicParent)
        _isCyclic_or_CyclicParent = true;

      NumLevels = Parent->NumLevels + 1;
      PackSize_Total += Parent->GetPackSize();

      // we read BitMaps only if Parent was open

      UInt64 numBytes = (UInt64)NumUsedBitMaps << kBitmapSize_Log;
      if (openArchiveCallback && numBytes != 0)
      {
        RINOK(openArchiveCallback->SetTotal(NULL, &numBytes))
      }
      numBytes = 0;
      for (size_t i = ChunkRatio; i < TotalBatEntries; i += ChunkRatio + 1)
      {
        const UInt64 v = Bat.GetItem(i);
        const UInt64 offset = BAT_GET_OFFSET(v);
        const unsigned state = BAT_GET_STATE(v);

        CByteBuffer &buf = BitMaps.AddNew();
        if (state == SB_BLOCK_PRESENT)
        {
          if (openArchiveCallback)
          {
            RINOK(openArchiveCallback->SetCompleted(NULL, &numBytes))
          }
          numBytes += kBitmapSize;
          buf.Alloc(kBitmapSize);
          RINOK(Seek2(offset))
          RINOK(Read_FALSE(buf, kBitmapSize))
          /*
          for (unsigned i = 0; i < (1 << 20); i+=4)
          {
            UInt32 v = GetUi32(buf + i);
            if (v != 0 && v != (UInt32)(Int32)-1)
              printf("\n%7d %8x", i, v);
          }
          */
        }
      }
    }
  }

  return S_OK;
}


void CHandler::CloseAtError()
{
  // CHandlerImg
  Clear_HandlerImg_Vars();
  Stream.Release();

  _phySize = 0;
  Bat.Clear();
  BitMaps.Clear();
  NumUsedBlocks = 0;
  NumUsedBitMaps = 0;
  HeadersSize = 0;
  /*
  NumUsed_1MB_Blocks = 0;
  NumUsed_1MB_Blocks_Defined = false;
  */

  Parent = NULL;
  ParentStream.Release();
  _errorMessage.Empty();
  _creator.Empty();
  _nonEmptyLog = false;
  _parentGuid_IsDefined = false;
  _isDataContiguous = false;
  // _batOverlap = false;

  ParentNames.Clear();
  ParentName_Used.Empty();

  Meta.Clear();

  ChunkRatio_Log = 0;
  ChunkRatio = 0;
  TotalBatEntries = 0;
  NumLevels = 0;
  PackSize_Total = 0;

  _isCyclic = false;
  _isCyclic_or_CyclicParent = false;
}

Z7_COM7F_IMF(CHandler::Close())
{
  CloseAtError();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidSize: prop = Meta.VirtualDiskSize; break;
    case kpidPackSize: prop = PackSize_Total; break;
    case kpidExtension: prop = (_imgExt ? _imgExt : "img"); break;
  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  // if some prarent is not OK, we don't create stream
  if (!AreParentsOK())
    return S_FALSE;
  InitSeekPositions();
  CMyComPtr<ISequentialInStream> streamTemp = this;
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

REGISTER_ARC_I(
  "VHDX", "vhdx avhdx", NULL, 0xc4,
  kSignature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/VmdkHandler.cpp ================ */
// VmdkHandler.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

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
namespace NVmdk {

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define LE_16(offs, dest) dest = Get16(p + (offs))
#define LE_32(offs, dest) dest = Get32(p + (offs))
#define LE_64(offs, dest) dest = Get64(p + (offs))


static const Byte k_Signature[] = { 'K', 'D', 'M', 'V' };

static const UInt32 k_Flags_NL         = (UInt32)1 << 0;
// static const UInt32 k_Flags_RGD        = (UInt32)1 << 1;
static const UInt32 k_Flags_ZeroGrain  = (UInt32)1 << 2;
static const UInt32 k_Flags_Compressed = (UInt32)1 << 16;
static const UInt32 k_Flags_Marker     = (UInt32)1 << 17;

static const unsigned k_NumMidBits = 9; // num bits for index in Grain Table

struct CHeader
{
  UInt32 flags;
  UInt32 version;

  UInt64 capacity;
  UInt64 grainSize;
  UInt64 descriptorOffset;
  UInt64 descriptorSize;

  UInt32 numGTEsPerGT;
  UInt16 algo;
  // Byte uncleanShutdown;
  // UInt64 rgdOffset;
  UInt64 gdOffset;
  UInt64 overHead;

  bool Is_NL()         const { return (flags & k_Flags_NL) != 0; }
  bool Is_ZeroGrain()  const { return (flags & k_Flags_ZeroGrain) != 0; }
  bool Is_Compressed() const { return (flags & k_Flags_Compressed) != 0; }
  bool Is_Marker()     const { return (flags & k_Flags_Marker) != 0; }

  bool Parse(const Byte *p);

  bool IsSameImageFor(const CHeader &h) const
  {
    return flags == h.flags
        && version == h.version
        && capacity == h.capacity
        && grainSize == h.grainSize
        && algo == h.algo;
  }
};

bool CHeader::Parse(const Byte *p)
{
  if (memcmp(p, k_Signature, sizeof(k_Signature)) != 0)
    return false;

  LE_32 (0x04, version);
  LE_32 (0x08, flags);
  LE_64 (0x0C, capacity);
  LE_64 (0x14, grainSize);
  LE_64 (0x1C, descriptorOffset);
  LE_64 (0x24, descriptorSize);
  LE_32 (0x2C, numGTEsPerGT);
  // LE_64 (0x30, rgdOffset);
  LE_64 (0x38, gdOffset);
  LE_64 (0x40, overHead);
  // uncleanShutdown = buf[0x48];
  LE_16(0x4D, algo);

  if (Is_NL() && Get32(p + 0x49) != 0x0A0D200A) // do we need Is_NL() check here?
    return false;

  return (numGTEsPerGT == (1 << k_NumMidBits)) && (version <= 3);
}


enum
{
  k_Marker_END_OF_STREAM = 0,
  k_Marker_GRAIN_TABLE   = 1,
  k_Marker_GRAIN_DIR     = 2,
  k_Marker_FOOTER        = 3
};

struct CMarker
{
  UInt64 NumSectors;
  UInt32 SpecSize; // = 0 for metadata sectors
  UInt32 Type;

  void Parse(const Byte *p)
  {
    LE_64 (0, NumSectors);
    LE_32 (8, SpecSize);
    LE_32 (12, Type);
  }
};


static bool Str_to_ValName(const AString &s, AString &name, AString &val)
{
  name.Empty();
  val.Empty();
  int qu = s.Find('"');
  int eq = s.Find('=');
  if (eq < 0 || (qu >= 0 && eq > qu))
    return false;
  name.SetFrom(s.Ptr(), eq);
  name.Trim();
  val = s.Ptr(eq + 1);
  val.Trim();
  return true;
}

static inline bool IsSpaceChar(char c)
{
  return (c == ' ' || c == '\t');
}

static const char *SkipSpaces(const char *s)
{
  for (;; s++)
  {
    char c = *s;
    if (c == 0 || !IsSpaceChar(c))
      return s;
  }
}

#define SKIP_SPACES(s) s = SkipSpaces(s);

static const char *GetNextWord(const char *s, AString &dest)
{
  dest.Empty();
  SKIP_SPACES(s)
  const char *start = s;
  for (;; s++)
  {
    char c = *s;
    if (c == 0 || IsSpaceChar(c))
    {
      dest.SetFrom(start, (unsigned)(s - start));
      return s;
    }
  }
}

static const char *GetNextNumber(const char *s, UInt64 &val)
{
  SKIP_SPACES(s)
  if (*s == 0)
    return s;
  const char *end;
  val = ConvertStringToUInt64(s, &end);
  char c = *end;
  if (c != 0 && !IsSpaceChar(c))
    return NULL;
  return end;
}


struct CExtentInfo
{
  AString Access;    // RW, RDONLY, or NOACCESS
  UInt64 NumSectors; // 512 bytes sectors
  AString Type;      // FLAT, SPARSE, ZERO, VMFS, VMFSSPARSE, VMFSRDM, VMFSRAW
  AString FileName;
  UInt64 StartSector; // used for FLAT

  // for VMWare Player 9:
  // PartitionUUID
  // DeviceIdentifier

  bool IsType_ZERO() const { return Type.IsEqualTo("ZERO"); }
  // bool IsType_FLAT() const { return Type.IsEqualTo("FLAT"); }
  bool IsType_Flat() const
    { return Type.IsEqualTo("FLAT")
          || Type.IsEqualTo("VMFS")
          || Type.IsEqualTo("VMFSRAW"); }
  
  bool Parse(const char *s);
};

bool CExtentInfo::Parse(const char *s)
{
  NumSectors = 0;
  StartSector = 0;
  Access.Empty();
  Type.Empty();
  FileName.Empty();

  s = GetNextWord(s, Access);
  s = GetNextNumber(s, NumSectors);
  if (!s)
    return false;
  s = GetNextWord(s, Type);

  if (Type.IsEmpty())
    return false;

  SKIP_SPACES(s)

  if (IsType_ZERO())
    return (*s == 0);

  if (*s != '\"')
    return false;
  s++;
  {
    const char *s2 = strchr(s, '\"');
    if (!s2)
      return false;
    FileName.SetFrom(s, (unsigned)(s2 - s));
    s = s2 + 1;
  }
  SKIP_SPACES(s)
  if (*s == 0)
    return true;

  s = GetNextNumber(s, StartSector);
  if (!s)
    return false;
  return true;
  // SKIP_SPACES(s);
  // return (*s == 0);
}


struct CDescriptor
{
  AString CID;
  AString parentCID;
  AString createType;
  // AString encoding; // UTF-8, windows-1252 - default is UTF-8

  CObjectVector<CExtentInfo> Extents;

  static void GetUnicodeName(const AString &s, UString &res)
  {
    if (!ConvertUTF8ToUnicode(s, res))
      MultiByteToUnicodeString2(res, s);
  }

  void Clear()
  {
    CID.Empty();
    parentCID.Empty();
    createType.Empty();
    Extents.Clear();
  }

  bool IsThere_Parent() const
  {
    return !parentCID.IsEmpty() && !parentCID.IsEqualTo_Ascii_NoCase("ffffffff");
  }

  bool Parse(const Byte *p, size_t size);
};


bool CDescriptor::Parse(const Byte *p, size_t size)
{
  Clear();

  AString s;
  AString name;
  AString val;
  
  for (;;)
  {
    Byte c = 0;
    if (size != 0)
    {
      size--;
      c = *p++;
    }
    if (c == 0 || c == 0xA || c == 0xD)
    {
      if (!s.IsEmpty() && s[0] != '#')
      {
        if (Str_to_ValName(s, name, val))
        {
          if (name.IsEqualTo_Ascii_NoCase("CID"))
            CID = val;
          else if (name.IsEqualTo_Ascii_NoCase("parentCID"))
            parentCID = val;
          else if (name.IsEqualTo_Ascii_NoCase("createType"))
            createType = val;
        }
        else
        {
          CExtentInfo ei;
          if (!ei.Parse(s))
            return false;
          Extents.Add(ei);
        }
      }
      
      s.Empty();
      if (c == 0)
        return true;
    }
    else
      s += (char)c;
  }
}


struct CExtent
{
  bool IsOK;
  bool IsArc;
  bool NeedDeflate;
  bool Unsupported;
  bool IsZero;
  bool IsFlat;
  bool DescriptorOK;
  bool HeadersError;

  unsigned ClusterBits;
  UInt32 ZeroSector;

  CObjectVector<CByteBuffer> Tables;

  CMyComPtr<IInStream> Stream;
  UInt64 PosInArc;
  
  UInt64 PhySize;
  UInt64 VirtSize;     // from vmdk header of volume

  UInt64 StartOffset;  // virtual offset of this extent
  UInt64 NumBytes;     // from main descriptor, if multi-vol
  UInt64 FlatOffset;   // in Stream

  CByteBuffer DescriptorBuf;
  CDescriptor Descriptor;

  CHeader h;

  UInt64 GetEndOffset() const { return StartOffset + NumBytes; }
  
  bool IsVmdk() const { return !IsZero && !IsFlat; }
  // if (IsOK && IsVmdk()), then VMDK header of this extent was read
  
  CExtent():
      IsOK(false),
      IsArc(false),
      NeedDeflate(false),
      Unsupported(false),
      IsZero(false),
      IsFlat(false),
      DescriptorOK(false),
      HeadersError(false),

      ClusterBits(0),
      ZeroSector(0),
      
      PosInArc(0),
      
      PhySize(0),
      VirtSize(0),

      StartOffset(0),
      NumBytes(0),
      FlatOffset(0)
        {}
    

  HRESULT ReadForHeader(IInStream *stream, UInt64 sector, void *data, size_t numSectors);
  HRESULT Open3(IInStream *stream, IArchiveOpenCallback *openCallback,
        unsigned numVols, unsigned volIndex, UInt64 &complexity);

  HRESULT Seek(UInt64 offset)
  {
    PosInArc = offset;
    return InStream_SeekSet(Stream, offset);
  }

  HRESULT InitAndSeek()
  {
    if (Stream)
      return Seek(0);
    return S_OK;
  }

  HRESULT Read(void *data, size_t *size)
  {
    HRESULT res = ReadStream(Stream, data, size);
    PosInArc += *size;
    return res;
  }
};
  

Z7_class_CHandler_final: public CHandlerImg
{
  bool _isArc;
  bool _unsupported;
  bool _unsupportedSome;
  bool _headerError;
  bool _missingVol;
  bool _isMultiVol;
  bool _needDeflate;

  UInt64 _cacheCluster;
  unsigned _cacheExtent;
  CByteBuffer _cache;
  CByteBuffer _cacheCompressed;
  
  unsigned _clusterBitsMax;
  UInt64 _phySize;

  CObjectVector<CExtent> _extents;

  CBufInStream *_bufInStreamSpec;
  CMyComPtr<ISequentialInStream> _bufInStream;

  CBufPtrSeqOutStream *_bufOutStreamSpec;
  CMyComPtr<ISequentialOutStream> _bufOutStream;

  NCompress::NZlib::CDecoder *_zlibDecoderSpec;
  CMyComPtr<ICompressCoder> _zlibDecoder;

  CByteBuffer _descriptorBuf;
  CDescriptor _descriptor;

  UString _missingVolName;
  
  void InitAndSeekMain()
  {
    _virtPos = 0;
  }

  virtual HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openCallback) Z7_override;
  virtual void CloseAtError() Z7_override;
public:
  Z7_IFACE_COM7_IMP(IInArchive_Img)

  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
};


Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
    return S_OK;
  {
    UInt64 rem = _size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size == 0)
      return S_OK;
  }

  unsigned extentIndex;
  {
    unsigned left = 0, right = _extents.Size();
    for (;;)
    {
      unsigned mid = (left + right) / 2;
      if (mid == left)
        break;
      if (_virtPos < _extents[mid].StartOffset)
        right = mid;
      else
        left = mid;
    }
    extentIndex = left;
  }
  
  CExtent &extent = _extents[extentIndex];

  {
    const UInt64 vir = _virtPos - extent.StartOffset;
    if (vir >= extent.NumBytes)
    {
      return E_FAIL;
      /*
      if (vir > extent.NumBytes)
        _stream_dataError = true;
      memset(data, 0, size);
      _virtPos += size;
      if (processedSize)
        *processedSize = size;
      return S_OK;
      */
    }

    {
      const UInt64 rem = extent.NumBytes - vir;
      if (size > rem)
        size = (UInt32)rem;
    }

    if (vir >= extent.VirtSize)
    {
      // if vmdk's VirtSize is smaller than VirtSize from main multi-volume descriptor
      _stream_dataError = true;
      return S_FALSE;
      /*
      memset(data, 0, size);
      _virtPos += size;
      if (processedSize)
        *processedSize = size;
      return S_OK;
      */
    }

    {
      const UInt64 rem = extent.VirtSize - vir;
      if (size > rem)
        size = (UInt32)rem;
    }

    if (extent.IsZero || !extent.IsOK || !extent.Stream || extent.Unsupported)
    {
      if (extent.Unsupported)
      {
        _stream_unsupportedMethod = true;
        return S_FALSE;
      }
      if (!extent.IsOK || !extent.Stream)
      {
        _stream_unavailData = true;
        return S_FALSE;
      }
      memset(data, 0, size);
      _virtPos += size;
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }

    if (extent.IsFlat)
    {
      UInt64 offset = extent.FlatOffset + vir;
      if (offset != extent.PosInArc)
      {
        RINOK(extent.Seek(offset))
      }
      UInt32 size2 = 0;
      HRESULT res = extent.Stream->Read(data, size, &size2);
      if (res == S_OK && size2 == 0)
      {
        _stream_unavailData = true;
        /*
        memset(data, 0, size);
        _virtPos += size;
        if (processedSize)
          *processedSize = size;
        return S_OK;
        */
      }
      // _stream_PackSize += size2;
      extent.PosInArc += size2;
      _virtPos += size2;
      if (processedSize)
        *processedSize = size2;
      return res;
    }
  }

  
  for (;;)
  {
    const UInt64 vir = _virtPos - extent.StartOffset;
    const unsigned clusterBits = extent.ClusterBits;
    const UInt64 cluster = vir >> clusterBits;
    const size_t clusterSize = (size_t)1 << clusterBits;
    const size_t lowBits = (size_t)vir & (clusterSize - 1);
    {
      size_t rem = clusterSize - lowBits;
      if (size > rem)
        size = (UInt32)rem;
    }

    if (extentIndex == _cacheExtent && cluster == _cacheCluster)
    {
      memcpy(data, _cache + lowBits, size);
      _virtPos += size;
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }
    
    const UInt64 high = cluster >> k_NumMidBits;
 
    if (high < extent.Tables.Size())
    {
      const CByteBuffer &table = extent.Tables[(unsigned)high];
    
      if (table.Size() != 0)
      {
        const size_t midBits = (size_t)cluster & ((1 << k_NumMidBits) - 1);
        const Byte *p = (const Byte *)table + (midBits << 2);
        const UInt32 v = Get32(p);
        
        if (v != 0 && v != extent.ZeroSector)
        {
          UInt64 offset = (UInt64)v << 9;
          if (extent.NeedDeflate)
          {
            if (offset != extent.PosInArc)
            {
              // printf("\n%12x %12x\n", (unsigned)offset, (unsigned)(offset - extent.PosInArc));
              RINOK(extent.Seek(offset))
            }
            
            const size_t kStartSize = 1 << 9;
            {
              size_t curSize = kStartSize;
              RINOK(extent.Read(_cacheCompressed, &curSize))
              // _stream_PackSize += curSize;
              if (curSize != kStartSize)
                return S_FALSE;
            }

            if (Get64(_cacheCompressed) != (cluster << (clusterBits - 9)))
              return S_FALSE;

            UInt32 dataSize = Get32(_cacheCompressed + 8);
            if (dataSize > ((UInt32)1 << 31))
              return S_FALSE;

            size_t dataSize2 = (size_t)dataSize + 12;
            
            if (dataSize2 > kStartSize)
            {
              dataSize2 = (dataSize2 + 511) & ~(size_t)511;
              if (dataSize2 > _cacheCompressed.Size())
                return S_FALSE;
              size_t curSize = dataSize2 - kStartSize;
              const size_t curSize2 = curSize;
              RINOK(extent.Read(_cacheCompressed + kStartSize, &curSize))
              // _stream_PackSize += curSize;
              if (curSize != curSize2)
                return S_FALSE;
            }

            _bufInStreamSpec->Init(_cacheCompressed + 12, dataSize);
            
            _cacheCluster = (UInt64)(Int64)-1;
            _cacheExtent = (unsigned)(int)-1;

            if (_cache.Size() < clusterSize)
              return E_FAIL;
            _bufOutStreamSpec->Init(_cache, clusterSize);
            
            // Do we need to use smaller block than clusterSize for last cluster?
            const UInt64 blockSize64 = clusterSize;
            HRESULT res = _zlibDecoder->Code(_bufInStream, _bufOutStream, NULL, &blockSize64, NULL);

            /*
            if (_bufOutStreamSpec->GetPos() != clusterSize)
            {
              _stream_dataError = true;
              memset(_cache + _bufOutStreamSpec->GetPos(), 0, clusterSize - _bufOutStreamSpec->GetPos());
            }
            */

              if (_bufOutStreamSpec->GetPos() != clusterSize
                  || _zlibDecoderSpec->GetInputProcessedSize() != dataSize)
              {
                _stream_dataError = true;
                if (res == S_OK)
                  res = S_FALSE;
              }

            RINOK(res)
            
            _cacheCluster = cluster;
            _cacheExtent = extentIndex;
            
            continue;
            /*
            memcpy(data, _cache + lowBits, size);
            _virtPos += size;
            if (processedSize)
              *processedSize = size;
            return S_OK;
            */
          }
          {
            offset += lowBits;
            if (offset != extent.PosInArc)
            {
              // printf("\n%12x %12x\n", (unsigned)offset, (unsigned)(offset - extent.PosInArc));
              RINOK(extent.Seek(offset))
            }
            UInt32 size2 = 0;
            HRESULT res = extent.Stream->Read(data, size, &size2);
            if (res == S_OK && size2 == 0)
            {
              _stream_unavailData = true;
              /*
              memset(data, 0, size);
              _virtPos += size;
              if (processedSize)
                *processedSize = size;
              return S_OK;
              */
            }
            extent.PosInArc += size2;
            // _stream_PackSize += size2;
            _virtPos += size2;
            if (processedSize)
              *processedSize = size2;
            return res;
          }
        }
      }
    }
    
    memset(data, 0, size);
    _virtPos += size;
    if (processedSize)
      *processedSize = size;
    return S_OK;
  }
}


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidNumVolumes,
  kpidTotalPhySize,
  kpidMethod,
  kpidClusterSize,
  kpidHeadersSize,
  kpidId,
  kpidName,
  kpidComment
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps


Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  
  const CExtent *e = NULL;
  const CDescriptor *desc = NULL;
  
  if (_isMultiVol)
    desc = &_descriptor;
  else if (_extents.Size() == 1)
  {
    e = &_extents[0];
    desc = &e->Descriptor;
  }

  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidPhySize: if (_phySize != 0) prop = _phySize; break;
    case kpidTotalPhySize:
    {
      UInt64 sum = _phySize;
      if (_isMultiVol)
      {
        FOR_VECTOR (i, _extents)
          sum += _extents[i].PhySize;
      }
      prop = sum;
      break;
    }
    case kpidClusterSize: prop = (UInt32)((UInt32)1 << _clusterBitsMax); break;
    case kpidHeadersSize: if (e) prop = (e->h.overHead << 9); break;
    case kpidMethod:
    {
      AString s;

      if (desc && !desc->createType.IsEmpty())
        s = desc->createType;

      bool zlib = false;
      bool marker = false;
      Int32 algo = -1;

      FOR_VECTOR (i, _extents)
      {
        const CExtent &extent = _extents[i];
        if (!extent.IsOK || !extent.IsVmdk())
          continue;

        const CHeader &h = extent.h;

        if (h.algo != 0)
        {
          if (h.algo == 1)
            zlib = true;
          else if (algo != h.algo)
          {
            s.Add_Space_if_NotEmpty();
            s.Add_UInt32(h.algo);
            algo = h.algo;
          }
        }
        
        if (h.Is_Marker())
          marker = true;
      }

      if (zlib)
        s.Add_OptSpaced("zlib");

      if (marker)
        s.Add_OptSpaced("Marker");
      
      if (!s.IsEmpty())
        prop = s;
      break;
    }
    
    case kpidComment:
    {
      if (e && e->DescriptorBuf.Size() != 0)
      {
        AString s;
        s.SetFrom_CalcLen((const char *)(const Byte *)e->DescriptorBuf, (unsigned)e->DescriptorBuf.Size());
        if (!s.IsEmpty() && s.Len() <= (1 << 16))
          prop = s;
      }
      break;
    }
    
    case kpidId:
    {
      if (desc && !desc->CID.IsEmpty())
      {
        prop = desc->CID;
      }
      break;
    }

    case kpidName:
    {
      if (!_isMultiVol && desc && desc->Extents.Size() == 1)
      {
        const CExtentInfo &ei = desc->Extents[0];
        if (!ei.FileName.IsEmpty())
        {
          UString u;
          CDescriptor::GetUnicodeName(ei.FileName, u);
          if (!u.IsEmpty())
            prop = u;
        }
      }
      break;
    }

    case kpidNumVolumes: if (_isMultiVol) prop = (UInt32)_extents.Size(); break;

    case kpidError:
    {
      if (_missingVol || !_missingVolName.IsEmpty())
      {
        UString s ("Missing volume : ");
        if (!_missingVolName.IsEmpty())
          s += _missingVolName;
        prop = s;
      }
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
      if (_unsupportedSome) v |= kpv_ErrorFlags_UnsupportedMethod;
      if (_headerError) v |= kpv_ErrorFlags_HeadersError;
      // if (_missingVol)  v |= kpv_ErrorFlags_UnexpectedEnd;
      if (v != 0)
        prop = v;
      break;
    }
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidSize: prop = _size; break;
    case kpidPackSize:
    {
      UInt64 packSize = 0;
      FOR_VECTOR (i, _extents)
      {
        const CExtent &e = _extents[i];
        if (!e.IsOK)
          continue;
        if (e.IsVmdk() && !_isMultiVol)
        {
          UInt64 ov = (e.h.overHead << 9);
          if (e.PhySize >= ov)
            packSize += e.PhySize - ov;
        }
        else
          packSize += e.PhySize;
      }
      prop = packSize;
      break;
    }
    case kpidExtension: prop = (_imgExt ? _imgExt : "img"); break;
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


static int inline GetLog(UInt64 num)
{
  for (int i = 0; i < 64; i++)
    if (((UInt64)1 << i) == num)
      return i;
  return -1;
}


HRESULT CExtent::ReadForHeader(IInStream *stream, UInt64 sector, void *data, size_t numSectors)
{
  sector <<= 9;
  RINOK(InStream_SeekSet(stream, sector))
  size_t size = numSectors << 9;
  RINOK(ReadStream_FALSE(stream, data, size))
  UInt64 end = sector + size;
  if (PhySize < end)
    PhySize = end;
  return S_OK;
}


void CHandler::CloseAtError()
{
  _extents.Clear();
  CHandlerImg::CloseAtError();
}


static const char * const kSignature_Descriptor = "# Disk DescriptorFile";


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openCallback)
{
  const unsigned kSectoreSize = 512;
  Byte buf[kSectoreSize];
  size_t headerSize = kSectoreSize;
  RINOK(ReadStream(stream, buf, &headerSize))

  if (headerSize < sizeof(k_Signature))
    return S_FALSE;

  CMyComPtr<IArchiveOpenVolumeCallback> volumeCallback;

  if (memcmp(buf, k_Signature, sizeof(k_Signature)) != 0)
  {
    const size_t k_SigDesc_Size = strlen(kSignature_Descriptor);
    if (headerSize < k_SigDesc_Size)
      return S_FALSE;
    if (memcmp(buf, kSignature_Descriptor, k_SigDesc_Size) != 0)
      return S_FALSE;

    UInt64 endPos;
    RINOK(InStream_GetSize_SeekToEnd(stream, endPos))
    if (endPos > (1 << 20))
      return S_FALSE;
    const size_t numBytes = (size_t)endPos;
    _descriptorBuf.Alloc(numBytes);
    RINOK(InStream_SeekToBegin(stream))
    RINOK(ReadStream_FALSE(stream, _descriptorBuf, numBytes))

    if (!_descriptor.Parse(_descriptorBuf, _descriptorBuf.Size()))
      return S_FALSE;
    _isMultiVol = true;
    _isArc = true;
    _phySize = numBytes;
    if (_descriptor.IsThere_Parent())
      _unsupported = true;

    if (openCallback)
    {
      openCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volumeCallback);
    }
    if (!volumeCallback)
    {
      _unsupported = true;
      return E_NOTIMPL;
    }

    /*
    UInt64 totalVirtSize = 0;
    FOR_VECTOR (i, _descriptor.Extents)
    {
      const CExtentInfo &ei = _descriptor.Extents[i];
      if (ei.NumSectors >= ((UInt64)1 << (63 - 9)))
        return S_FALSE;
      totalVirtSize += ei.NumSectors;
      if (totalVirtSize >= ((UInt64)1 << (63 - 9)))
        return S_FALSE;
    }
    totalVirtSize <<= 9;
    */

    if (_descriptor.Extents.Size() > 1)
    {
      const UInt64 numFiles = _descriptor.Extents.Size();
      RINOK(openCallback->SetTotal(&numFiles, NULL))
    }
  }

  UInt64 complexity = 0;

  for (;;)
  {
    CExtent *e = NULL;
    CMyComPtr<IInStream> nextStream;
    
    if (_isMultiVol)
    {
      const unsigned extentIndex = _extents.Size();
      if (extentIndex >= _descriptor.Extents.Size())
        break;
      const CExtentInfo &ei = _descriptor.Extents[extentIndex];
      e = &_extents.AddNew();
      e->StartOffset = 0;
      if (ei.NumSectors >= ((UInt64)1 << (62 - 9)) ||
          ei.StartSector >= ((UInt64)1 << (62 - 9)))
        return S_FALSE;
      e->NumBytes = ei.NumSectors << 9;
      e->IsZero = ei.IsType_ZERO();
      if (extentIndex != 0)
        e->StartOffset = _extents[extentIndex - 1].GetEndOffset();
      if (e->GetEndOffset() < e->StartOffset)
        return S_FALSE;

      e->VirtSize = e->NumBytes;
      if (e->IsZero)
      {
        e->IsOK = true;
        continue;
      }

      e->IsFlat = ei.IsType_Flat();
      e->FlatOffset = ei.StartSector << 9;

      UString u;
      CDescriptor::GetUnicodeName(ei.FileName, u);
      if (u.IsEmpty())
      {
        _missingVol = true;
        continue;
      }

      HRESULT result = volumeCallback->GetStream(u, &nextStream);

      if (result != S_OK && result != S_FALSE)
        return result;
      
      if (!nextStream || result != S_OK)
      {
        if (_missingVolName.IsEmpty())
          _missingVolName = u;
        _missingVol = true;
        continue;
      }

      if (e->IsFlat)
      {
        e->IsOK = true;
        e->Stream = nextStream;
        e->PhySize = e->NumBytes;
        continue;
      }

      stream = nextStream;

      headerSize = kSectoreSize;
      RINOK(ReadStream(stream, buf, &headerSize))

      if (headerSize != kSectoreSize)
        continue;
      if (memcmp(buf, k_Signature, sizeof(k_Signature)) != 0)
        continue;
    }
    else
    {
      if (headerSize != kSectoreSize)
        return S_FALSE;
      e = &_extents.AddNew();
      e->StartOffset = 0;
    }

    HRESULT res = S_FALSE;
    if (e->h.Parse(buf))
      res = e->Open3(stream, openCallback, _isMultiVol ? _descriptor.Extents.Size() : 1, _extents.Size() - 1, complexity);
    
    if (!_isMultiVol)
    {
      _isArc = e->IsArc;
      _phySize = e->PhySize;
      _unsupported = e->Unsupported;
    }

    if (e->Unsupported)
      _unsupportedSome = true;
    if (e->HeadersError)
      _headerError = true;

    if (res != S_OK)
    {
      if (res != S_FALSE)
        return res;
      if (!_isMultiVol)
        return res;
      continue;
    }

    e->Stream = stream;
    e->IsOK = true;
    
    if (!_isMultiVol)
    {
      e->NumBytes = e->VirtSize;
      break;
    }

    if (e->NumBytes != e->VirtSize)
      _headerError = true;
  }

  if (!_extents.IsEmpty())
    _size = _extents.Back().GetEndOffset();

  _needDeflate = false;
  _clusterBitsMax = 0;
  
  // unsigned numOKs = 0;
  unsigned numUnsupported = 0;

  FOR_VECTOR (i, _extents)
  {
    const CExtent &e = _extents[i];
    if (e.Unsupported)
      numUnsupported++;
    if (!e.IsOK)
      continue;
    // numOKs++;
    if (e.IsVmdk())
    {
      if (e.NeedDeflate)
        _needDeflate = true;
      if (_clusterBitsMax < e.ClusterBits)
        _clusterBitsMax = e.ClusterBits;
    }
  }

  if (numUnsupported != 0 && numUnsupported == _extents.Size())
    _unsupported = true;

  return S_OK;
}


HRESULT CExtent::Open3(IInStream *stream, IArchiveOpenCallback *openCallback,
    unsigned numVols, unsigned volIndex, UInt64 &complexity)
{
  if (h.descriptorSize != 0)
  {
    if (h.descriptorOffset == 0 ||
        h.descriptorSize > (1 << 10))
      return S_FALSE;
    DescriptorBuf.Alloc((size_t)h.descriptorSize << 9);
    RINOK(ReadForHeader(stream, h.descriptorOffset, DescriptorBuf, (size_t)h.descriptorSize))
    if (h.descriptorOffset == 1 && h.Is_Marker() && Get64(DescriptorBuf) == 0)
    {
      // We check data as end marker.
      // and if probably it's footer's copy of header, we don't want to open it.
      return S_FALSE;
    }

    DescriptorOK = Descriptor.Parse(DescriptorBuf, DescriptorBuf.Size());
    if (!DescriptorOK)
      HeadersError = true;
    if (Descriptor.IsThere_Parent())
      Unsupported = true;
  }

  if (h.gdOffset == (UInt64)(Int64)-1)
  {
    // Grain Dir is at end of file
    UInt64 endPos;
    RINOK(InStream_GetSize_SeekToEnd(stream, endPos))
    if ((endPos & 511) != 0)
      return S_FALSE;

    const size_t kEndSize = 512 * 3;
    Byte buf2[kEndSize];
    if (endPos < kEndSize)
      return S_FALSE;
    RINOK(InStream_SeekSet(stream, endPos - kEndSize))
    RINOK(ReadStream_FALSE(stream, buf2, kEndSize))
    
    CHeader h2;
    if (!h2.Parse(buf2 + 512))
      return S_FALSE;
    if (!h.IsSameImageFor(h2))
      return S_FALSE;

    h = h2;

    CMarker m;
    m.Parse(buf2);
    if (m.NumSectors != 1 || m.SpecSize != 0 || m.Type != k_Marker_FOOTER)
      return S_FALSE;
    m.Parse(buf2 + 512 * 2);
    if (m.NumSectors != 0 || m.SpecSize != 0 || m.Type != k_Marker_END_OF_STREAM)
      return S_FALSE;
    PhySize = endPos;
  }

  const int grainSize_Log = GetLog(h.grainSize);
  if (grainSize_Log < 3 || grainSize_Log > 30 - 9) // grain size must be >= 4 KB
    return S_FALSE;
  if (h.capacity >= ((UInt64)1 << (63 - 9)))
    return S_FALSE;
  if (h.overHead >= ((UInt64)1 << (63 - 9)))
    return S_FALSE;

  IsArc = true;
  ClusterBits = (9 + (unsigned)grainSize_Log);
  VirtSize = h.capacity << 9;
  NeedDeflate = (h.algo >= 1);

  if (h.Is_Compressed() ? (h.algo > 1 || !h.Is_Marker()) : (h.algo != 0))
  {
    Unsupported = true;
    PhySize = 0;
    return S_FALSE;
  }

  {
    const UInt64 overHeadBytes = h.overHead << 9;
    if (PhySize < overHeadBytes)
      PhySize = overHeadBytes;
  }

  ZeroSector = 0;
  if (h.Is_ZeroGrain())
    ZeroSector = 1;

  const UInt64 numSectorsPerGde = (UInt64)1 << ((unsigned)grainSize_Log + k_NumMidBits);
  const UInt64 numGdeEntries = (h.capacity + numSectorsPerGde - 1) >> ((unsigned)grainSize_Log + k_NumMidBits);
  CByteBuffer table;
  
  if (numGdeEntries != 0)
  {
    if (h.gdOffset == 0)
      return S_FALSE;

    size_t numSectors = (size_t)((numGdeEntries + ((1 << (9 - 2)) - 1)) >> (9 - 2));
    size_t t1SizeBytes = numSectors << 9;
    if ((t1SizeBytes >> 2) < numGdeEntries)
      return S_FALSE;
    table.Alloc(t1SizeBytes);

    if (h.Is_Marker())
    {
      Byte buf2[1 << 9];
      if (ReadForHeader(stream, h.gdOffset - 1, buf2, 1) != S_OK)
        return S_FALSE;
      {
        CMarker m;
        m.Parse(buf2);
        if (m.Type != k_Marker_GRAIN_DIR
            || m.NumSectors != numSectors
            || m.SpecSize != 0)
          return S_FALSE;
      }
    }

    RINOK(ReadForHeader(stream, h.gdOffset, table, numSectors))
  }

  const size_t clusterSize = (size_t)1 << ClusterBits;

  const UInt64 complexityStart = complexity;

  if (openCallback)
  {
    complexity += (UInt64)numGdeEntries << (k_NumMidBits + 2);
    {
      const UInt64 numVols2 = numVols;
      RINOK(openCallback->SetTotal((numVols == 1) ? NULL : &numVols2, &complexity))
    }
    if (numVols != 1)
    {
      const UInt64 volIndex2 = volIndex;
      RINOK(openCallback->SetCompleted(numVols == 1 ? NULL : &volIndex2, &complexityStart))
    }
  }

  UInt64 lastSector = 0;
  UInt64 lastVirtCluster = 0;
  size_t numProcessed_Prev = 0;

  for (size_t i = 0; i < numGdeEntries; i++)
  {
    const size_t k_NumSectors = (size_t)1 << (k_NumMidBits - 9 + 2);
    const size_t k_NumMidItems = (size_t)1 << k_NumMidBits;

    CByteBuffer &buf = Tables.AddNew();

    {
      const UInt32 v = Get32((const Byte *)table + (size_t)i * 4);
      if (v == 0 || v == ZeroSector)
        continue;
      if (openCallback && (i - numProcessed_Prev) >= 1024)
      {
        const UInt64 comp = complexityStart + ((UInt64)i << (k_NumMidBits + 2));
        const UInt64 volIndex2 = volIndex;
        RINOK(openCallback->SetCompleted(numVols == 1 ? NULL : &volIndex2, &comp))
        numProcessed_Prev = i;
      }
      
      if (h.Is_Marker())
      {
        Byte buf2[1 << 9];
        if (ReadForHeader(stream, v - 1, buf2, 1) != S_OK)
          return S_FALSE;
        {
          CMarker m;
          m.Parse(buf2);
          if (m.Type != k_Marker_GRAIN_TABLE
            || m.NumSectors != k_NumSectors
            || m.SpecSize != 0)
            return S_FALSE;
        }
      }
      
      buf.Alloc(k_NumMidItems * 4);
      RINOK(ReadForHeader(stream, v, buf, k_NumSectors))
    }

    for (size_t k = 0; k < k_NumMidItems; k++)
    {
      const UInt32 v = Get32((const Byte *)buf + (size_t)k * 4);
      if (v == 0 || v == ZeroSector)
        continue;
      if (v < h.overHead)
        return S_FALSE;
      if (lastSector < v)
      {
        lastSector = v;
        if (NeedDeflate)
          lastVirtCluster = ((UInt64)i << k_NumMidBits) + k;
      }
    }
  }

  if (!NeedDeflate)
  {
    UInt64 end = ((UInt64)lastSector << 9) + clusterSize;
    if (PhySize < end)
      PhySize = end;
  }
  else if (lastSector != 0)
  {
    Byte buf[1 << 9];
    if (ReadForHeader(stream, lastSector, buf, 1) == S_OK)
    {
      UInt64 lba = Get64(buf);
      if (lba == (lastVirtCluster << (ClusterBits - 9)))
      {
        UInt32 dataSize = Get32(buf + 8);
        size_t dataSize2 = (size_t)dataSize + 12;
        dataSize2 = (dataSize2 + 511) & ~(size_t)511;
        UInt64 end = ((UInt64)lastSector << 9) + dataSize2;
        if (PhySize < end)
          PhySize = end;
      }
    }
  }

  return S_OK;
}


Z7_COM7F_IMF(CHandler::Close())
{
  _phySize = 0;
  
  _cacheCluster = (UInt64)(Int64)-1;
  _cacheExtent = (unsigned)(int)-1;

  _clusterBitsMax = 0;

  _isArc = false;
  _unsupported = false;
  _unsupportedSome = false;
  _headerError = false;
  _missingVol = false;
  _isMultiVol = false;
  _needDeflate = false;

  _missingVolName.Empty();

  _descriptorBuf.Free();
  _descriptor.Clear();

  // CHandlerImg:
  Clear_HandlerImg_Vars();
  Stream.Release();

  _extents.Clear();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;

  if (_unsupported)
    return S_FALSE;

  ClearStreamVars();
  // _stream_UsePackSize = true;

  if (_needDeflate)
  {
    if (!_bufInStream)
    {
      _bufInStreamSpec = new CBufInStream;
      _bufInStream = _bufInStreamSpec;
    }
    
    if (!_bufOutStream)
    {
      _bufOutStreamSpec = new CBufPtrSeqOutStream();
      _bufOutStream = _bufOutStreamSpec;
    }

    if (!_zlibDecoder)
    {
      _zlibDecoderSpec = new NCompress::NZlib::CDecoder;
      _zlibDecoder = _zlibDecoderSpec;
    }
    
    const size_t clusterSize = (size_t)1 << _clusterBitsMax;
    _cache.AllocAtLeast(clusterSize);
    _cacheCompressed.AllocAtLeast(clusterSize * 2);
  }

  FOR_VECTOR (i, _extents)
  {
    RINOK(_extents[i].InitAndSeek())
  }

  CMyComPtr<ISequentialInStream> streamTemp = this;
  InitAndSeekMain();
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}


REGISTER_ARC_I(
  "VMDK", "vmdk", NULL, 0xC8,
  k_Signature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/XarHandler.cpp ================ */
// XarHandler.cpp

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
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

#define XAR_SHOW_RAW

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)
#define Get64(p) GetBe64(p)

namespace NArchive {
namespace NXar {

Z7_CLASS_IMP_NOQIB_1(
  CInStreamWithSha256
  , ISequentialInStream
)
  bool _sha512Mode;
  CMyComPtr<ISequentialInStream> _stream;
  CAlignedBuffer1 _sha256;
  CAlignedBuffer1 _sha512;
  UInt64 _size;

  CSha256 *Sha256() { return (CSha256 *)(void *)(Byte *)_sha256; }
  CSha512 *Sha512() { return (CSha512 *)(void *)(Byte *)_sha512; }
public:
  CInStreamWithSha256():
      _sha256(sizeof(CSha256)),
      _sha512(sizeof(CSha512))
      {}
  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init(bool sha512Mode)
  {
    _sha512Mode = sha512Mode;
    _size = 0;
    if (sha512Mode)
      Sha512_Init(Sha512(), SHA512_DIGEST_SIZE);
    else
      Sha256_Init(Sha256());
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  void Final256(Byte *digest) { Sha256_Final(Sha256(), digest); }
  void Final512(Byte *digest) { Sha512_Final(Sha512(), digest, SHA512_DIGEST_SIZE); }
};

Z7_COM7F_IMF(CInStreamWithSha256::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize;
  const HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (_sha512Mode)
    Sha512_Update(Sha512(), (const Byte *)data, realProcessedSize);
  else
    Sha256_Update(Sha256(), (const Byte *)data, realProcessedSize);
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}


Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithSha256
  , ISequentialOutStream
)
  bool _sha512Mode;
  CMyComPtr<ISequentialOutStream> _stream;
  CAlignedBuffer1 _sha256;
  CAlignedBuffer1 _sha512;
  UInt64 _size;

  CSha256 *Sha256() { return (CSha256 *)(void *)(Byte *)_sha256; }
  CSha512 *Sha512() { return (CSha512 *)(void *)(Byte *)_sha512; }
public:
  COutStreamWithSha256():
      _sha256(sizeof(CSha256)),
      _sha512(sizeof(CSha512))
      {}
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(bool sha512Mode)
  {
    _sha512Mode = sha512Mode;
    _size = 0;
    if (sha512Mode)
      Sha512_Init(Sha512(), SHA512_DIGEST_SIZE);
    else
      Sha256_Init(Sha256());
  }
  UInt64 GetSize() const { return _size; }
  void Final256(Byte *digest) { Sha256_Final(Sha256(), digest); }
  void Final512(Byte *digest) { Sha512_Final(Sha512(), digest, SHA512_DIGEST_SIZE); }
};

Z7_COM7F_IMF(COutStreamWithSha256::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Write(data, size, &size);
  // if (_calculate)
  if (_sha512Mode)
    Sha512_Update(Sha512(), (const Byte *)data, size);
  else
    Sha256_Update(Sha256(), (const Byte *)data, size);
  _size += size;
  if (processedSize)
    *processedSize = size;
  return result;
}

// we limit supported xml sizes:
// ((size_t)1 << (sizeof(size_t) / 2 + 28)) - (1u << 14);
static const size_t kXmlSizeMax = ((size_t)1 << 30) - (1u << 14);
static const size_t kXmlPackSizeMax = kXmlSizeMax;

#define XAR_CKSUM_NONE    0
#define XAR_CKSUM_SHA1    1
#define XAR_CKSUM_MD5     2
#define XAR_CKSUM_SHA256  3
#define XAR_CKSUM_SHA512  4
// #define XAR_CKSUM_OTHER   3
// fork version of xar can use (3) as special case,
// where name of hash is stored as string at the end of header
// we do not support such hash still.

static const char * const k_ChecksumNames[] =
{
    "NONE"
  , "SHA1"
  , "MD5"
  , "SHA256"
  , "SHA512"
};

static unsigned GetHashSize(int algo)
{
  if (algo <= XAR_CKSUM_NONE || algo > XAR_CKSUM_SHA512)
    return 0;
  if (algo == XAR_CKSUM_SHA1)
    return SHA1_DIGEST_SIZE;
  return (16u >> XAR_CKSUM_MD5) << algo;
}

#define METHOD_NAME_ZLIB  "zlib"

static int Find_ChecksumId_for_Name(const AString &style)
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_ChecksumNames); i++)
  {
    // old xars used upper case in "style"
    // new xars use  lower case in "style"
    if (style.IsEqualTo_Ascii_NoCase(k_ChecksumNames[i]))
      return (int)i;
  }
  return -1;
}


struct CCheckSum
{
  int AlgoNumber;
  bool Error;
  CByteBuffer Data;
  AString Style;
  
  CCheckSum(): AlgoNumber(-1), Error(false) {}
  void AddNameToString(AString &s) const;
};

void CCheckSum::AddNameToString(AString &s) const
{
  if (Style.IsEmpty())
    s.Add_OptSpaced("NO-CHECKSUM");
  else
  {
    s.Add_OptSpaced(Style);
    if (Error)
      s += "-ERROR";
  }
}


struct CFile
{
  bool IsDir;
  bool Is_SymLink;
  bool HasData;
  bool Mode_Defined;
  bool INode_Defined;
  bool UserId_Defined;
  bool GroupId_Defined;
  // bool Device_Defined;
  bool Id_Defined;

  int Parent;
  UInt32 Mode;

  UInt64 Size;
  UInt64 PackSize;
  UInt64 Offset;
  UInt64 MTime;
  UInt64 CTime;
  UInt64 ATime;
  UInt64 INode;
  UInt64 UserId;
  UInt64 GroupId;
  // UInt64 Device;

  AString Name;
  AString Method;
  AString User;
  AString Group;
  // AString Id;
  AString Type;
  AString Link;
  // AString LinkType;
  // AString LinkFrom;
  
  UInt64 Id;
  CCheckSum extracted_checksum;
  CCheckSum archived_checksum;

  CFile(int parent):
      IsDir(false),
      Is_SymLink(false),
      HasData(false),
      Mode_Defined(false),
      INode_Defined(false),
      UserId_Defined(false),
      GroupId_Defined(false),
      // Device_Defined(false),
      Id_Defined(false),
      Parent(parent),
      Mode(0),
      Size(0), PackSize(0), Offset(0),
      MTime(0), CTime(0), ATime(0),
      INode(0)
      {}

  bool IsCopyMethod() const
  {
    return Method.IsEmpty() || Method.IsEqualTo("octet-stream");
  }

  void UpdateTotalPackSize(UInt64 &totalSize) const
  {
    const UInt64 t = Offset + PackSize;
    if (t >= Offset)
    if (totalSize < t)
        totalSize = t;
  }
};


Z7_CLASS_IMP_CHandler_IInArchive_2(
    IArchiveGetRawProps,
    IInArchiveGetStream
)
  bool _is_pkg;
  bool _toc_CrcError;
  CObjectVector<CFile> _files;
  CMyComPtr<IInStream> _inStream;
  UInt64 _dataStartPos;
  UInt64 _phySize;
  CAlignedBuffer _xmlBuf;
  size_t _xmlLen;
  // UInt64 CreationTime;
  AString CreationTime_String;
  UInt32 _checkSumAlgo;
  Int32 _mainSubfile;

  HRESULT Open2(IInStream *stream);
};


static const Byte kArcProps[] =
{
  kpidSubType,
  // kpidHeadersSize,
  kpidMethod,
  kpidCTime
};

// #define kpidLinkType 250
// #define kpidLinkFrom 251

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidCTime,
  kpidATime,
  kpidPosixAttrib,
  kpidType,
  kpidUser,
  kpidGroup,
  kpidUserId,
  kpidGroupId,
  kpidINode,
  // kpidDeviceMajor,
  // kpidDeviceMinor,
  kpidSymLink,
  // kpidLinkType,
  // kpidLinkFrom,
  kpidMethod,
  kpidId,
  kpidOffset
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static bool ParseUInt64(const CXmlItem &item, const char *name, UInt64 &res)
{
  const AString s (item.GetSubStringForTag(name));
  if (s.IsEmpty())
    return false;
  const char *end;
  res = ConvertStringToUInt64(s, &end);
  return *end == 0;
}


#define PARSE_NUM(_num_, _dest_) \
    { const char *end; _dest_ = ConvertStringToUInt32(p, &end); \
    if ((unsigned)(end - p) != _num_) return 0; \
    p += _num_ + 1; }

static UInt64 ParseTime(const CXmlItem &item, const char *name /* , bool z_isRequired */ )
{
  const AString s (item.GetSubStringForTag(name));
  if (s.Len() < 20 /* (z_isRequired ? 20u : 19u) */)
    return 0;
  const char *p = s;
  if (p[ 4] != '-' ||
      p[ 7] != '-' ||
      p[10] != 'T' ||
      p[13] != ':' ||
      p[16] != ':')
    return 0;
  // if (z_isRequired)
  if (p[19] != 'Z')
    return 0;
  UInt32 year, month, day, hour, min, sec;
  PARSE_NUM(4, year)
  PARSE_NUM(2, month)
  PARSE_NUM(2, day)
  PARSE_NUM(2, hour)
  PARSE_NUM(2, min)
  PARSE_NUM(2, sec)
  UInt64 numSecs;
  if (!NTime::GetSecondsSince1601(year, month, day, hour, min, sec, numSecs))
    return 0;
  return numSecs * 10000000;
}


static void ParseChecksum(const CXmlItem &item, const char *name, CCheckSum &checksum)
{
  const CXmlItem *checkItem = item.FindSubTag_GetPtr(name);
  if (!checkItem)
    return; // false;
  checksum.Style = checkItem->GetPropVal("style");
  const AString s (checkItem->GetSubString());
  if ((s.Len() & 1) == 0 && s.Len() <= (2u << 7)) // 1024-bit max
  {
    const size_t size = s.Len() / 2;
    CByteBuffer temp(size);
    if ((size_t)(ParseHexString(s, temp) - temp) == size)
    {
      checksum.Data = temp;
      const int index = Find_ChecksumId_for_Name(checksum.Style);
      if (index >= 0 && checksum.Data.Size() == GetHashSize(index))
      {
        checksum.AlgoNumber = index;
        return;
      }
    }
  }
  checksum.Error = true;
}


static bool AddItem(const CXmlItem &item, CObjectVector<CFile> &files, int parent, int level)
{
  if (!item.IsTag)
    return true;
  if (level >= 1024)
    return false;
  if (item.Name.IsEqualTo("file"))
  {
    CFile file(parent);
    parent = (int)files.Size();
    {
      const AString id = item.GetPropVal("id");
      const char *end;
      file.Id = ConvertStringToUInt64(id, &end);
      if (*end == 0)
        file.Id_Defined = true;
    }
    file.Name = item.GetSubStringForTag("name");
    z7_xml_DecodeString(file.Name);
    {
      const CXmlItem *typeItem = item.FindSubTag_GetPtr("type");
      if (typeItem)
      {
        file.Type = typeItem->GetSubString();
        // file.LinkFrom = typeItem->GetPropVal("link");
        if (file.Type.IsEqualTo("directory"))
          file.IsDir = true;
        else
        {
          // file.IsDir = false;
          /*
          else if (file.Type.IsEqualTo("file"))
          {}
          else if (file.Type.IsEqualTo("hardlink"))
          {}
          else
          */
          if (file.Type.IsEqualTo("symlink"))
            file.Is_SymLink = true;
          // file.IsDir = false;
        }
      }
    }
    {
      const CXmlItem *linkItem = item.FindSubTag_GetPtr("link");
      if (linkItem)
      {
        // file.LinkType = linkItem->GetPropVal("type");
        file.Link = linkItem->GetSubString();
        z7_xml_DecodeString(file.Link);
      }
    }

    const CXmlItem *dataItem = item.FindSubTag_GetPtr("data");
    if (dataItem && !file.IsDir)
    {
      file.HasData = true;
      if (!ParseUInt64(*dataItem, "size", file.Size))
        return false;
      if (!ParseUInt64(*dataItem, "length", file.PackSize))
        return false;
      if (!ParseUInt64(*dataItem, "offset", file.Offset))
        return false;
      ParseChecksum(*dataItem, "extracted-checksum", file.extracted_checksum);
      ParseChecksum(*dataItem, "archived-checksum",  file.archived_checksum);
      const CXmlItem *encodingItem = dataItem->FindSubTag_GetPtr("encoding");
      if (encodingItem)
      {
        AString s (encodingItem->GetPropVal("style"));
        if (!s.IsEmpty())
        {
          const AString appl ("application/");
          if (s.IsPrefixedBy(appl))
          {
            s.DeleteFrontal(appl.Len());
            const AString xx ("x-");
            if (s.IsPrefixedBy(xx))
            {
              s.DeleteFrontal(xx.Len());
              if (s.IsEqualTo("gzip"))
                s = METHOD_NAME_ZLIB;
            }
          }
          file.Method = s;
        }
      }
    }

    file.INode_Defined = ParseUInt64(item, "inode", file.INode);
    file.UserId_Defined = ParseUInt64(item, "uid", file.UserId);
    file.GroupId_Defined = ParseUInt64(item, "gid", file.GroupId);
    // file.Device_Defined = ParseUInt64(item, "deviceno", file.Device);
    file.MTime = ParseTime(item, "mtime"); // z_IsRequied = true
    file.CTime = ParseTime(item, "ctime");
    file.ATime = ParseTime(item, "atime");
    {
      const AString s (item.GetSubStringForTag("mode"));
      if (s[0] == '0')
      {
        const char *end;
        file.Mode = ConvertOctStringToUInt32(s, &end);
        file.Mode_Defined = (*end == 0);
      }
    }
    file.User = item.GetSubStringForTag("user");
    file.Group = item.GetSubStringForTag("group");

    files.Add(file);
  }

  FOR_VECTOR (i, item.SubItems)
    if (!AddItem(item.SubItems[i], files, parent, level + 1))
      return false;
  return true;
}



struct CInStreamWithHash
{
  CMyComPtr2_Create<ISequentialInStream, CInStreamWithSha1> inStreamSha1;
  CMyComPtr2_Create<ISequentialInStream, CInStreamWithSha256> inStreamSha256;
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStreamLim;

  void SetStreamAndInit(ISequentialInStream *stream, int algo);
  bool CheckHash(int algo, const Byte *digest_from_arc) const;
};


void CInStreamWithHash::SetStreamAndInit(ISequentialInStream *stream, int algo)
{
  if (algo == XAR_CKSUM_SHA1)
  {
    inStreamSha1->SetStream(stream);
    inStreamSha1->Init();
    stream = inStreamSha1;
  }
  else if (algo == XAR_CKSUM_SHA256
        || algo == XAR_CKSUM_SHA512)
  {
    inStreamSha256->SetStream(stream);
    inStreamSha256->Init(algo == XAR_CKSUM_SHA512);
    stream = inStreamSha256;
  }
  inStreamLim->SetStream(stream);
}

bool CInStreamWithHash::CheckHash(int algo, const Byte *digest_from_arc) const
{
  if (algo == XAR_CKSUM_SHA1)
  {
    Byte digest[SHA1_DIGEST_SIZE];
    inStreamSha1->Final(digest);
    if (memcmp(digest, digest_from_arc, sizeof(digest)) != 0)
      return false;
  }
  else if (algo == XAR_CKSUM_SHA256)
  {
    Byte digest[SHA256_DIGEST_SIZE];
    inStreamSha256->Final256(digest);
    if (memcmp(digest, digest_from_arc, sizeof(digest)) != 0)
      return false;
  }
  else if (algo == XAR_CKSUM_SHA512)
  {
    Byte digest[SHA512_DIGEST_SIZE];
    inStreamSha256->Final512(digest);
    if (memcmp(digest, digest_from_arc, sizeof(digest)) != 0)
      return false;
  }
  return true;
}


HRESULT CHandler::Open2(IInStream *stream)
{
  const unsigned kHeaderSize = 28;
  UInt32 buf32[kHeaderSize / sizeof(UInt32)];
  RINOK(ReadStream_FALSE(stream, buf32, kHeaderSize))
  const unsigned headerSize = Get16((const Byte *)(const void *)buf32 + 4);
  // xar library now writes 1 to version field.
  // some old xars could have version == 0 ?
  // specification allows (headerSize != 28),
  // but we don't expect big value in (headerSize).
  // so we restrict (headerSize) with 64 bytes to reduce false open.
  const unsigned kHeaderSize_MAX = 64;
  if (Get32(buf32) != 0x78617221  // signature: "xar!"
      || headerSize < kHeaderSize
      || headerSize > kHeaderSize_MAX
      || Get16((const Byte *)(const void *)buf32 + 6) > 1 // version
      )
    return S_FALSE;
  _checkSumAlgo = Get32(buf32 + 6);
  const UInt64 packSize = Get64(buf32 + 2);
  const UInt64 unpackSize = Get64(buf32 + 4);
  if (packSize >= kXmlPackSizeMax ||
      unpackSize >= kXmlSizeMax)
    return S_FALSE;
  /* some xar archives can have padding bytes at offset 28,
     or checksum algorithm name at offset 28 (in xar fork, if cksum_alg==3)
     But we didn't see such xar archives.
  */
  if (headerSize != kHeaderSize)
  {
    RINOK(InStream_SeekSet(stream, headerSize))
  }
  _dataStartPos = headerSize + packSize;
  _phySize = _dataStartPos;

  _xmlBuf.Alloc((size_t)unpackSize + 1);
  if (!_xmlBuf.IsAllocated())
    return E_OUTOFMEMORY;
  _xmlLen = (size_t)unpackSize;
  
  CInStreamWithHash hashStream;
  {
    CMyComPtr2_Create<ICompressCoder, NCompress::NZlib::CDecoder> zlibCoder;
    hashStream.SetStreamAndInit(stream, (int)(unsigned)_checkSumAlgo);
    hashStream.inStreamLim->Init(packSize);
    CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> outStreamLim;
    outStreamLim->Init(_xmlBuf, (size_t)unpackSize);
    RINOK(zlibCoder.Interface()->Code(hashStream.inStreamLim, outStreamLim, NULL, &unpackSize, NULL))
    if (outStreamLim->GetPos() != (size_t)unpackSize)
      return S_FALSE;
  }
  _xmlBuf[(size_t)unpackSize] = 0;
  if (strlen((const char *)(const Byte *)_xmlBuf) != (size_t)unpackSize)
    return S_FALSE;
  CXml xml;
  if (!xml.Parse((const char *)(const Byte *)_xmlBuf))
    return S_FALSE;
  
  if (!xml.Root.IsTagged("xar") || xml.Root.SubItems.Size() != 1)
    return S_FALSE;
  const CXmlItem &toc = xml.Root.SubItems[0];
  if (!toc.IsTagged("toc"))
    return S_FALSE;

  // CreationTime = ParseTime(toc, "creation-time", false); // z_IsRequied
  CreationTime_String = toc.GetSubStringForTag("creation-time");
  {
    // we suppose that offset of checksum is always 0;
    // but [TOC].xml contains exact offset value in <checksum> block.
    const UInt64 offset = 0;
    const unsigned hashSize = GetHashSize((int)(unsigned)_checkSumAlgo);
    if (hashSize)
    {
      /*
      const CXmlItem *csItem = toc.FindSubTag_GetPtr("checksum");
      if (csItem)
      {
        const int checkSumAlgo2 = Find_ChecksumId_for_Name(csItem->GetPropVal("style"));
        UInt64 csSize, csOffset;
        if (ParseUInt64(*csItem, "size", csSize) &&
            ParseUInt64(*csItem, "offset", csOffset)  &&
            csSize == hashSize &&
            (unsigned)checkSumAlgo2 == _checkSumAlgo)
          offset = csOffset;
      }
      */
      CByteBuffer digest_from_arc(hashSize);
      RINOK(InStream_SeekSet(stream, _dataStartPos + offset))
      RINOK(ReadStream_FALSE(stream, digest_from_arc, hashSize))
      if (!hashStream.CheckHash((int)(unsigned)_checkSumAlgo, digest_from_arc))
        _toc_CrcError = true;
    }
  }
    
  if (!AddItem(toc, _files,
      -1, // parent
      0)) // level
    return S_FALSE;

  UInt64 totalPackSize = 0;
  unsigned numMainFiles = 0;
  
  FOR_VECTOR (i, _files)
  {
    const CFile &file = _files[i];
    file.UpdateTotalPackSize(totalPackSize);
    if (file.Parent == -1)
    {
      if (file.Name.IsEqualTo("Payload") ||
          file.Name.IsEqualTo("Content"))
      {
        _mainSubfile = (Int32)(int)i;
        numMainFiles++;
      }
      else if (file.Name.IsEqualTo("PackageInfo"))
        _is_pkg = true;
    }
  }

  if (numMainFiles > 1)
    _mainSubfile = -1;

  const UInt64 k_PhySizeLim = (UInt64)1 << 62;
  _phySize = (totalPackSize > k_PhySizeLim - _dataStartPos) ?
      k_PhySizeLim :
      _dataStartPos + totalPackSize;

  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  {
    Close();
    RINOK(Open2(stream))
    _inStream = stream;
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _phySize = 0;
  _dataStartPos = 0;
  _inStream.Release();
  _files.Clear();
  _xmlLen = 0;
  _xmlBuf.Free();
  _mainSubfile = -1;
  _is_pkg = false;
  _toc_CrcError = false;
  _checkSumAlgo = 0;
  // CreationTime = 0;
  CreationTime_String.Empty();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _files.Size()
#ifdef XAR_SHOW_RAW
    + 1
#endif
  ;
  return S_OK;
}

static void TimeToProp(UInt64 t, NCOM::CPropVariant &prop)
{
  if (t != 0)
  {
    FILETIME ft;
    ft.dwLowDateTime = (UInt32)(t);
    ft.dwHighDateTime = (UInt32)(t >> 32);
    prop = ft;
  }
}

static void Utf8StringToProp(const AString &s, NCOM::CPropVariant &prop)
{
  if (!s.IsEmpty())
  {
    UString us;
    ConvertUTF8ToUnicode(s, us);
    prop = us;
  }
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    // case kpidHeadersSize: prop = _dataStartPos; break;
    case kpidPhySize: prop = _phySize; break;
    case kpidMainSubfile: if (_mainSubfile >= 0) prop = (UInt32)_mainSubfile; break;
    case kpidSubType: if (_is_pkg) prop = "pkg"; break;
    case kpidExtension: prop = _is_pkg ? "pkg" : "xar"; break;
    case kpidCTime:
    {
      // it's local time. We can transfer it to UTC time, if we use FILETIME.
      // TimeToProp(CreationTime, prop); break;
      if (!CreationTime_String.IsEmpty())
        prop = CreationTime_String;
      break;
    }
    case kpidMethod:
    {
      AString s;
      if (_checkSumAlgo < Z7_ARRAY_SIZE(k_ChecksumNames))
        s = k_ChecksumNames[_checkSumAlgo];
      else
      {
        s += "Checksum";
        s.Add_UInt32(_checkSumAlgo);
      }
      prop = s;
      break;
    }
    case kpidWarningFlags:
    {
      UInt32 v = 0;
      if (_toc_CrcError) v |= kpv_ErrorFlags_CrcError;
      prop = v;
      break;
    }
    case kpidINode: prop = true; break;
    case kpidIsTree: prop = true; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


/*
inline UInt32 MY_dev_major(UInt64 dev)
{
  return ((UInt32)(dev >> 8) & (UInt32)0xfff) | ((UInt32)(dev >> 32) & ~(UInt32)0xfff);
}
inline UInt32 MY_dev_minor(UInt64 dev)
{
  return ((UInt32)(dev) & 0xff) | ((UInt32)(dev >> 12) & ~(UInt32)0xff);
}
*/

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  
#ifdef XAR_SHOW_RAW
  if (index >= _files.Size())
  {
    switch (propID)
    {
      case kpidName:
      case kpidPath:
        prop = "[TOC].xml"; break;
      case kpidSize:
      case kpidPackSize: prop = (UInt64)_xmlLen; break;
    }
  }
  else
#endif
  {
    const CFile &item = _files[index];
    switch (propID)
    {
      case kpidPath:
      {
        AString path;
        unsigned cur = index;
        for (;;)
        {
          const CFile &item2 = _files[cur];
          if (!path.IsEmpty())
            path.InsertAtFront(CHAR_PATH_SEPARATOR);
// #define XAR_EMPTY_NAME_REPLACEMENT "[]"
          if (item2.Name.IsEmpty())
          {
            AString s('[');
            s.Add_UInt32(cur);
            s.Add_Char(']');
            path.Insert(0, s);
          }
          else
            path.Insert(0, item2.Name);
          if (item2.Parent < 0)
            break;
          cur = (unsigned)item2.Parent;
        }
        Utf8StringToProp(path, prop);
        break;
      }

      case kpidName:
      {
        if (item.Name.IsEmpty())
        {
          AString s('[');
          s.Add_UInt32(index);
          s.Add_Char(']');
          prop = s;
        }
        else
          Utf8StringToProp(item.Name, prop);
        break;
      }

      case kpidIsDir: prop = item.IsDir; break;
      
      case kpidSize:     if (item.HasData && !item.IsDir) prop = item.Size; break;
      case kpidPackSize: if (item.HasData && !item.IsDir) prop = item.PackSize; break;

      case kpidMethod:
      {
        if (item.HasData)
        {
          AString s = item.Method;
          item.extracted_checksum.AddNameToString(s);
          item.archived_checksum.AddNameToString(s);
          Utf8StringToProp(s, prop);
        }
        break;
      }
        
      case kpidMTime: TimeToProp(item.MTime, prop); break;
      case kpidCTime: TimeToProp(item.CTime, prop); break;
      case kpidATime: TimeToProp(item.ATime, prop); break;
      
      case kpidPosixAttrib:
        if (item.Mode_Defined)
        {
          UInt32 mode = item.Mode;
          if ((mode & MY_LIN_S_IFMT) == 0)
            mode |= (
                item.Is_SymLink ? MY_LIN_S_IFLNK :
                item.IsDir      ? MY_LIN_S_IFDIR :
                                  MY_LIN_S_IFREG);
          prop = mode;
        }
        break;
      
      case kpidType:  Utf8StringToProp(item.Type, prop); break;
      case kpidUser:  Utf8StringToProp(item.User, prop); break;
      case kpidGroup: Utf8StringToProp(item.Group, prop); break;
      case kpidSymLink: if (item.Is_SymLink) Utf8StringToProp(item.Link, prop); break;
      
      case kpidUserId:  if (item.UserId_Defined)  prop = item.UserId;   break;
      case kpidGroupId: if (item.GroupId_Defined) prop = item.GroupId;  break;
      case kpidINode:   if (item.INode_Defined)   prop = item.INode;    break;
      case kpidId:      if (item.Id_Defined)      prop = item.Id;       break;
      // Utf8StringToProp(item.Id, prop);
      /*
      case kpidDeviceMajor: if (item.Device_Defined) prop = (UInt32)MY_dev_major(item.Device);  break;
      case kpidDeviceMinor: if (item.Device_Defined) prop = (UInt32)MY_dev_minor(item.Device);  break;
      case kpidLinkType:
        if (!item.LinkType.IsEmpty())
          Utf8StringToProp(item.LinkType, prop);
        break;
      case kpidLinkFrom:
        if (!item.LinkFrom.IsEmpty())
          Utf8StringToProp(item.LinkFrom, prop);
        break;
      */
      case kpidOffset:
        if (item.HasData)
          prop = _dataStartPos + item.Offset;
        break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


// for debug:
// #define Z7_XAR_SHOW_CHECKSUM_PACK

#ifdef Z7_XAR_SHOW_CHECKSUM_PACK
enum
{
  kpidChecksumPack = kpidUserDefined
};
#endif

static const Byte kRawProps[] =
{
  kpidChecksum
#ifdef Z7_XAR_SHOW_CHECKSUM_PACK
  , kpidCRC // instead of kpidUserDefined / kpidCRC
#endif
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

#ifdef Z7_XAR_SHOW_CHECKSUM_PACK
  if (index != 0)
  {
    *propID = kpidChecksumPack;
    *name = NWindows::NCOM::AllocBstrFromAscii("archived-checksum");
  }
#endif
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType))
{
  *parentType = NParentType::kDir;
  *parent = (UInt32)(Int32)-1;
#ifdef XAR_SHOW_RAW
  if (index >= _files.Size())
    return S_OK;
#endif
  {
    const CFile &item = _files[index];
    *parent = (UInt32)(Int32)item.Parent;
  }
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  // COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  if (propID == kpidChecksum)
  {
#ifdef XAR_SHOW_RAW
    if (index >= _files.Size())
    {
      // case kpidPath: prop = "[TOC].xml"; break;
    }
    else
#endif
    {
      const CFile &item = _files[index];
      const size_t size = item.extracted_checksum.Data.Size();
      if (size != 0)
      {
        *dataSize = (UInt32)size;
        *propType = NPropDataType::kRaw;
        *data = item.extracted_checksum.Data;
      }
    }
  }

#ifdef Z7_XAR_SHOW_CHECKSUM_PACK
  if (propID == kpidChecksumPack)
  {
#ifdef XAR_SHOW_RAW
    if (index >= _files.Size())
    {
      // we can show digest check sum here
    }
    else
#endif
    {
      const CFile &item = _files[index];
      const size_t size = (UInt32)item.archived_checksum.Data.Size();
      if (size != 0)
      {
        *dataSize = (UInt32)size;
        *propType = NPropDataType::kRaw;
        *data = item.archived_checksum.Data;
      }
    }
  }
#endif
  return S_OK;
}




Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _files.Size()
#ifdef XAR_SHOW_RAW
    + 1
#endif
    ;
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const UInt32 index = allFilesMode ? i : indices[i];
#ifdef XAR_SHOW_RAW
    if (index >= _files.Size())
      totalSize += _xmlLen;
    else
#endif
      totalSize += _files[index].Size;
  }
  RINOK(extractCallback->SetTotal(totalSize))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CInStreamWithHash inHashStream;
  CMyComPtr2_Create<ISequentialOutStream, COutStreamWithSha1> outStreamSha1;
  CMyComPtr2_Create<ISequentialOutStream, COutStreamWithSha256> outStreamSha256;
  CMyComPtr2_Create<ISequentialOutStream, CLimitedSequentialOutStream> outStreamLim;
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ICompressCoder, NCompress::NZlib::CDecoder> zlibCoder;
  CMyComPtr2_Create<ICompressCoder, NCompress::NBZip2::CDecoder> bzip2Coder;
  bzip2Coder->FinishMode = true;
  
  UInt64 cur_PackSize, cur_UnpSize;

  for (i = 0;; i++,
      lps->InSize += cur_PackSize,
      lps->OutSize += cur_UnpSize)
  {
    cur_PackSize = 0;
    cur_UnpSize = 0;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;

    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
    
    if (index < _files.Size())
    {
      const CFile &item = _files[index];
      if (item.IsDir)
      {
        RINOK(extractCallback->PrepareOperation(askMode))
        realOutStream.Release();
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
        continue;
      }
    }

    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))

    Int32 opRes = NExtract::NOperationResult::kOK;

#ifdef XAR_SHOW_RAW
    if (index >= _files.Size())
    {
      cur_PackSize = cur_UnpSize = _xmlLen;
      if (realOutStream)
        RINOK(WriteStream(realOutStream, _xmlBuf, _xmlLen))
      realOutStream.Release();
    }
    else
#endif
    {
      const CFile &item = _files[index];
      if (!item.HasData)
        realOutStream.Release();
      else
      {
        cur_PackSize = item.PackSize;
        cur_UnpSize = item.Size;
        
        RINOK(InStream_SeekSet(_inStream, _dataStartPos + item.Offset))

        inHashStream.SetStreamAndInit(_inStream, item.archived_checksum.AlgoNumber);
        inHashStream.inStreamLim->Init(item.PackSize);

        const int checksum_method = item.extracted_checksum.AlgoNumber;
        if (checksum_method == XAR_CKSUM_SHA1)
        {
          outStreamLim->SetStream(outStreamSha1);
          outStreamSha1->SetStream(realOutStream);
          outStreamSha1->Init();
        }
        else if (checksum_method == XAR_CKSUM_SHA256
              || checksum_method == XAR_CKSUM_SHA512)
        {
          outStreamLim->SetStream(outStreamSha256);
          outStreamSha256->SetStream(realOutStream);
          outStreamSha256->Init(checksum_method == XAR_CKSUM_SHA512);
        }
        else
          outStreamLim->SetStream(realOutStream);

        realOutStream.Release();

        // outStreamSha1->Init(item.Sha1IsDefined);

        outStreamLim->Init(item.Size);
        HRESULT res = S_OK;
        
        ICompressCoder *coder = NULL;
        if (item.IsCopyMethod())
        {
          if (item.PackSize == item.Size)
            coder = copyCoder;
          else
            opRes = NExtract::NOperationResult::kUnsupportedMethod;
        }
        else if (item.Method.IsEqualTo(METHOD_NAME_ZLIB))
          coder = zlibCoder;
        else if (item.Method.IsEqualTo("bzip2"))
          coder = bzip2Coder;
        else
          opRes = NExtract::NOperationResult::kUnsupportedMethod;
        
        if (coder)
          res = coder->Code(inHashStream.inStreamLim, outStreamLim, NULL, &item.Size, lps);
        
        if (res != S_OK)
        {
          if (!outStreamLim->IsFinishedOK())
            opRes = NExtract::NOperationResult::kDataError;
          else if (res != S_FALSE)
            return res;
          if (opRes == NExtract::NOperationResult::kOK)
            opRes = NExtract::NOperationResult::kDataError;
        }

        if (opRes == NExtract::NOperationResult::kOK)
        {
          if (outStreamLim->IsFinishedOK())
          {
            if (checksum_method == XAR_CKSUM_SHA1)
            {
              Byte digest[SHA1_DIGEST_SIZE];
              outStreamSha1->Final(digest);
              if (memcmp(digest, item.extracted_checksum.Data, SHA1_DIGEST_SIZE) != 0)
                opRes = NExtract::NOperationResult::kCRCError;
            }
            else if (checksum_method == XAR_CKSUM_SHA256)
            {
              Byte digest[SHA256_DIGEST_SIZE];
              outStreamSha256->Final256(digest);
              if (memcmp(digest, item.extracted_checksum.Data, sizeof(digest)) != 0)
                opRes = NExtract::NOperationResult::kCRCError;
            }
            else if (checksum_method == XAR_CKSUM_SHA512)
            {
              Byte digest[SHA512_DIGEST_SIZE];
              outStreamSha256->Final512(digest);
              if (memcmp(digest, item.extracted_checksum.Data, sizeof(digest)) != 0)
                opRes = NExtract::NOperationResult::kCRCError;
            }
            if (opRes == NExtract::NOperationResult::kOK)
              if (!inHashStream.CheckHash(
                    item.archived_checksum.AlgoNumber,
                    item.archived_checksum.Data))
                opRes = NExtract::NOperationResult::kCRCError;
          }
          else
            opRes = NExtract::NOperationResult::kDataError;
        }
        if (checksum_method == XAR_CKSUM_SHA1)
          outStreamSha1->ReleaseStream();
        else if (checksum_method == XAR_CKSUM_SHA256)
          outStreamSha256->ReleaseStream();
      }
      outStreamLim->ReleaseStream();
    }
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  *stream = NULL;
  COM_TRY_BEGIN
#ifdef XAR_SHOW_RAW
  if (index >= _files.Size())
  {
    Create_BufInStream_WithNewBuffer(_xmlBuf, _xmlLen, stream);
    return S_OK;
  }
  else
#endif
  {
    const CFile &item = _files[index];
    if (item.HasData && item.IsCopyMethod() && item.PackSize == item.Size)
      return CreateLimitedInStream(_inStream, _dataStartPos + item.Offset, item.Size, stream);
  }
  return S_FALSE;
  COM_TRY_END
}

// 0x1c == 28 is expected header size value for most archives.
// but we want to support another (rare case) headers sizes.
// so we must reduce signature to 4 or 5 bytes.
static const Byte k_Signature[] =
//  { 'x', 'a', 'r', '!', 0, 0x1C, 0 };
    { 'x', 'a', 'r', '!', 0 };

REGISTER_ARC_I(
  "Xar", "xar pkg xip", NULL, 0xE1,
  k_Signature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/XzHandler.cpp ================ */
// XzHandler.cpp

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

// amalgamation: header emitted in prologue

using namespace NWindows;

namespace NArchive {
namespace NXz {

#define k_LZMA2_Name "LZMA2"


struct CBlockInfo
{
  unsigned StreamFlags;
  UInt64 PackPos;
  UInt64 PackSize; // pure value from Index record, it doesn't include pad zeros
  UInt64 UnpackPos;
};


Z7_class_CHandler_final:
  public IInArchive,
  public IArchiveOpenSeq,
  public IInArchiveGetStream,
  public ISetProperties,
 #ifndef Z7_EXTRACT_ONLY
  public IOutArchive,
 #endif
  public CMyUnknownImp,
 #ifndef Z7_EXTRACT_ONLY
  public CMultiMethodProps
 #else
  public CCommonMethodProps
 #endif
{
  Z7_COM_QI_BEGIN2(IInArchive)
  Z7_COM_QI_ENTRY(IArchiveOpenSeq)
  Z7_COM_QI_ENTRY(IInArchiveGetStream)
  Z7_COM_QI_ENTRY(ISetProperties)
 #ifndef Z7_EXTRACT_ONLY
  Z7_COM_QI_ENTRY(IOutArchive)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(IInArchive)
  Z7_IFACE_COM7_IMP(IArchiveOpenSeq)
  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISetProperties)
 #ifndef Z7_EXTRACT_ONLY
  Z7_IFACE_COM7_IMP(IOutArchive)
 #endif

  bool _stat_defined;
  bool _stat2_defined;
  bool _isArc;
  bool _needSeekToStart;
  bool _firstBlockWasRead;
  SRes _stat2_decode_SRes;

  CXzStatInfo _stat;    // it's stat from backward parsing
  CXzStatInfo _stat2;   // it's data from forward parsing, if the decoder was called

  const CXzStatInfo *GetStat() const
  {
    if (_stat_defined) return &_stat;
    if (_stat2_defined) return &_stat2;
    return NULL;
  }
  
  AString _methodsString;


  #ifndef Z7_EXTRACT_ONLY

  UInt32 _filterId;
  UInt64 _numSolidBytes;

  void InitXz()
  {
    _filterId = 0;
    _numSolidBytes = XZ_PROPS_BLOCK_SIZE_AUTO;
  }

  #endif


  void Init()
  {
    #ifndef Z7_EXTRACT_ONLY
      InitXz();
      CMultiMethodProps::Init();
    #else
      CCommonMethodProps::InitCommon();
    #endif
  }
  
  HRESULT SetProperty(const wchar_t *name, const PROPVARIANT &value);

  HRESULT Open2(IInStream *inStream, /* UInt32 flags, */ IArchiveOpenCallback *callback);

  HRESULT Decode(NCompress::NXz::CDecoder &decoder,
      ISequentialInStream *seqInStream,
      ISequentialOutStream *outStream,
      ICompressProgressInfo *progress)
  {
    #ifndef Z7_ST
    decoder._numThreads = _numThreads;
    #endif
    decoder._memUsage = _memUsage_Decompress;

    const HRESULT hres = decoder.Decode(seqInStream, outStream,
        NULL, // *outSizeLimit
        true, // finishStream
        progress);
    
    if (decoder.MainDecodeSRes_wasUsed
        && decoder.MainDecodeSRes != SZ_ERROR_MEM
        && decoder.MainDecodeSRes != SZ_ERROR_UNSUPPORTED)
    {
      // if (!_stat2_defined)
      {
        _stat2_decode_SRes = decoder.MainDecodeSRes;
        _stat2 = decoder.Stat;
        _stat2_defined = true;
      }
    }

    if (hres == S_OK && progress)
    {
      // RINOK(
      progress->SetRatioInfo(&decoder.Stat.InSize, &decoder.Stat.OutSize);
    }
    return hres;
  }

public:
  CBlockInfo *_blocks;
  size_t _blocksArraySize;
  UInt64 _maxBlocksSize;
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;

  CXzBlock _firstBlock;

  CHandler();
  ~CHandler();

  HRESULT SeekToPackPos(UInt64 pos)
  {
    return InStream_SeekSet(_stream, pos);
  }
};


CHandler::CHandler():
    _blocks(NULL),
    _blocksArraySize(0)
{
  #ifndef Z7_EXTRACT_ONLY
  InitXz();
  #endif
}

CHandler::~CHandler()
{
  MyFree(_blocks);
}


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize,
  kpidMethod
};

static const Byte kArcProps[] =
{
  kpidMethod,
  kpidNumStreams,
  kpidNumBlocks,
  kpidClusterSize,
  kpidCharacts
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static void Lzma2PropToString(AString &s, unsigned prop)
{
  char c = 0;
  UInt32 size;
  if ((prop & 1) == 0)
    size = prop / 2 + 12;
  else
  {
    c = 'k';
    size = (UInt32)(2 | (prop & 1)) << (prop / 2 + 1);
    if (prop > 17)
    {
      size >>= 10;
      c = 'm';
    }
  }
  s.Add_UInt32(size);
  if (c != 0)
    s.Add_Char(c);
}

struct CMethodNamePair
{
  UInt32 Id;
  const char *Name;
};

static const CMethodNamePair g_NamePairs[] =
{
  { XZ_ID_Subblock, "SB" },
  { XZ_ID_Delta, "Delta" },
  { XZ_ID_X86, "BCJ" },
  { XZ_ID_PPC, "PPC" },
  { XZ_ID_IA64, "IA64" },
  { XZ_ID_ARM, "ARM" },
  { XZ_ID_ARMT, "ARMT" },
  { XZ_ID_SPARC, "SPARC" },
  { XZ_ID_ARM64, "ARM64" },
  { XZ_ID_RISCV, "RISCV" },
  { XZ_ID_LZMA2, "LZMA2" }
};

static void AddMethodString(AString &s, const CXzFilter &f)
{
  const char *p = NULL;
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_NamePairs); i++)
    if (g_NamePairs[i].Id == f.id)
    {
      p = g_NamePairs[i].Name;
      break;
    }
  char temp[32];
  if (!p)
  {
    ::ConvertUInt64ToString(f.id, temp);
    p = temp;
  }

  s += p;

  if (f.propsSize > 0)
  {
    s.Add_Colon();
    if (f.id == XZ_ID_LZMA2 && f.propsSize == 1)
      Lzma2PropToString(s, f.props[0]);
    else if (f.id == XZ_ID_Delta && f.propsSize == 1)
      s.Add_UInt32((UInt32)f.props[0] + 1);
    else if (f.id == XZ_ID_ARM64 && f.propsSize == 1)
      s.Add_UInt32((UInt32)f.props[0] + 16 + 2);
    else
    {
      s.Add_Char('[');
      for (UInt32 bi = 0; bi < f.propsSize; bi++)
      {
        const unsigned v = f.props[bi];
        s.Add_Char(GET_HEX_CHAR_UPPER(v >> 4));
        s.Add_Char(GET_HEX_CHAR_UPPER(v & 15));
      }
      s.Add_Char(']');
    }
  }
}

static const char * const kChecks[] =
{
    "NoCheck"
  , "CRC32"
  , NULL
  , NULL
  , "CRC64"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "SHA256"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
};

static void AddCheckString(AString &s, const CXzs &xzs)
{
  size_t i;
  UInt32 mask = 0;
  for (i = 0; i < xzs.num; i++)
    mask |= ((UInt32)1 << XzFlags_GetCheckType(xzs.streams[i].flags));
  for (i = 0; i <= XZ_CHECK_MASK; i++)
    if (((mask >> i) & 1) != 0)
    {
      s.Add_Space_if_NotEmpty();
      if (kChecks[i])
        s += kChecks[i];
      else
      {
        s += "Check-";
        s.Add_UInt32((UInt32)i);
      }
    }
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  const CXzStatInfo *stat = GetStat();
  
  switch (propID)
  {
    case kpidPhySize: if (stat) prop = stat->InSize; break;
    case kpidNumStreams: if (stat && stat->NumStreams_Defined) prop = stat->NumStreams; break;
    case kpidNumBlocks: if (stat && stat->NumBlocks_Defined) prop = stat->NumBlocks; break;
    case kpidUnpackSize: if (stat && stat->UnpackSize_Defined) prop = stat->OutSize; break;
    case kpidClusterSize: if (_stat_defined && _stat.NumBlocks_Defined && stat->NumBlocks > 1) prop = _maxBlocksSize; break;
    case kpidCharacts:
      if (_firstBlockWasRead)
      {
        AString s;
        if (XzBlock_HasPackSize(&_firstBlock))
          s.Add_OptSpaced("BlockPackSize");
        if (XzBlock_HasUnpackSize(&_firstBlock))
          s.Add_OptSpaced("BlockUnpackSize");
        if (!s.IsEmpty())
          prop = s;
      }
      break;
        

    case kpidMethod: if (!_methodsString.IsEmpty()) prop = _methodsString; break;
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      SRes sres = _stat2_decode_SRes;
      if (!_isArc)                      v |= kpv_ErrorFlags_IsNotArc;
      if (sres == SZ_ERROR_INPUT_EOF)   v |= kpv_ErrorFlags_UnexpectedEnd;
      if (_stat2_defined && _stat2.DataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
      if (sres == SZ_ERROR_ARCHIVE)     v |= kpv_ErrorFlags_HeadersError;
      if (sres == SZ_ERROR_UNSUPPORTED) v |= kpv_ErrorFlags_UnsupportedMethod;
      if (sres == SZ_ERROR_DATA)        v |= kpv_ErrorFlags_DataError;
      if (sres == SZ_ERROR_CRC)         v |= kpv_ErrorFlags_CrcError;
      if (v != 0)
        prop = v;
      break;
    }

    case kpidMainSubfile:
    {
      // debug only, comment it:
      // if (_blocks) prop = (UInt32)0;
      break;
    }
    default: break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  const CXzStatInfo *stat = GetStat();
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidSize: if (stat && stat->UnpackSize_Defined) prop = stat->OutSize; break;
    case kpidPackSize: if (stat) prop = stat->InSize; break;
    case kpidMethod: if (!_methodsString.IsEmpty()) prop = _methodsString; break;
    default: break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


struct COpenCallbackWrap
{
  ICompressProgress vt;
  IArchiveOpenCallback *OpenCallback;
  HRESULT Res;
  
  // new clang shows "non-POD" warning for offsetof(), if we use constructor instead of Init()
  void Init(IArchiveOpenCallback *progress);
};

static SRes OpenCallbackProgress(ICompressProgressPtr pp, UInt64 inSize, UInt64 /* outSize */)
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(COpenCallbackWrap)
  if (p->OpenCallback)
    p->Res = p->OpenCallback->SetCompleted(NULL, &inSize);
  return HRESULT_To_SRes(p->Res, SZ_ERROR_PROGRESS);
}

void COpenCallbackWrap::Init(IArchiveOpenCallback *callback)
{
  vt.Progress = OpenCallbackProgress;
  OpenCallback = callback;
  Res = SZ_OK;
}


struct CXzsCPP
{
  CXzs p;
  CXzsCPP() { Xzs_CONSTRUCT(&p) }
  ~CXzsCPP() { Xzs_Free(&p, &g_Alloc); }
};

#define kInputBufSize ((size_t)1 << 10)

struct CLookToRead2_CPP: public CLookToRead2
{
  CLookToRead2_CPP()
  {
    buf = NULL;
    LookToRead2_CreateVTable(this,
        True // Lookahead ?
        );
  }
  void Alloc(size_t allocSize)
  {
    buf = (Byte *)MyAlloc(allocSize);
    if (buf)
      this->bufSize = allocSize;
  }
  ~CLookToRead2_CPP()
  {
    MyFree(buf);
  }
};


static HRESULT SRes_to_Open_HRESULT(SRes res)
{
  switch (res)
  {
    case SZ_OK: return S_OK;
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_PROGRESS: return E_ABORT;
    /*
    case SZ_ERROR_UNSUPPORTED:
    case SZ_ERROR_CRC:
    case SZ_ERROR_DATA:
    case SZ_ERROR_ARCHIVE:
    case SZ_ERROR_NO_ARCHIVE:
      return S_FALSE;
    */
    default: break;
  }
  return S_FALSE;
}



HRESULT CHandler::Open2(IInStream *inStream, /* UInt32 flags, */ IArchiveOpenCallback *callback)
{
  _needSeekToStart = true;

  {
    CXzStreamFlags st;
    CSeqInStreamWrap inStreamWrap;
    
    inStreamWrap.Init(inStream);

    SRes res = Xz_ReadHeader(&st, &inStreamWrap.vt);
    
    if (inStreamWrap.Res != S_OK)
      return inStreamWrap.Res;
    if (res != SZ_OK)
      return SRes_to_Open_HRESULT(res);

    {
      CXzBlock block;
      BoolInt isIndex;
      UInt32 headerSizeRes;
    
      SRes res2 = XzBlock_ReadHeader(&block, &inStreamWrap.vt, &isIndex, &headerSizeRes);
      
      if (inStreamWrap.Res != S_OK)
        return inStreamWrap.Res;
      
      if (res2 != SZ_OK)
      {
        if (res2 == SZ_ERROR_INPUT_EOF)
        {
          _stat2_decode_SRes = res2;
          _stream = inStream;
          _seqStream = inStream;
          _isArc = true;
          return S_OK;
        }

        if (res2 == SZ_ERROR_ARCHIVE)
          return S_FALSE;
        // what codes are possible here ?
        // ?? res2 == SZ_ERROR_MEM           : is possible here
        // ?? res2 == SZ_ERROR_UNSUPPORTED   : is possible here
      }
      else if (!isIndex)
      {
        _firstBlockWasRead = true;
        _firstBlock = block;

        unsigned numFilters = XzBlock_GetNumFilters(&block);
        for (unsigned i = 0; i < numFilters; i++)
        {
          _methodsString.Add_Space_if_NotEmpty();
          AddMethodString(_methodsString, block.filters[i]);
        }
      }
    }
  }

  RINOK(InStream_GetSize_SeekToEnd(inStream, _stat.InSize))
  if (callback)
  {
    RINOK(callback->SetTotal(NULL, &_stat.InSize))
  }

  CSeekInStreamWrap inStreamImp;
  
  inStreamImp.Init(inStream);

  CLookToRead2_CPP lookStream;

  lookStream.Alloc(kInputBufSize);
  
  if (!lookStream.buf)
    return E_OUTOFMEMORY;

  lookStream.realStream = &inStreamImp.vt;
  LookToRead2_INIT(&lookStream)

  COpenCallbackWrap openWrap;
  openWrap.Init(callback);

  CXzsCPP xzs;
  Int64 startPosition;
  SRes res = Xzs_ReadBackward(&xzs.p, &lookStream.vt, &startPosition, &openWrap.vt, &g_Alloc);
  if (res == SZ_ERROR_PROGRESS)
    return (openWrap.Res == S_OK) ? E_FAIL : openWrap.Res;
  /*
  if (res == SZ_ERROR_NO_ARCHIVE && xzs.p.num > 0)
    res = SZ_OK;
  */
  if (res == SZ_OK && startPosition == 0)
  {
    _stat_defined = true;

    _stat.OutSize = Xzs_GetUnpackSize(&xzs.p);
    _stat.UnpackSize_Defined = true;

    _stat.NumStreams = xzs.p.num;
    _stat.NumStreams_Defined = true;
    
    _stat.NumBlocks = Xzs_GetNumBlocks(&xzs.p);
    _stat.NumBlocks_Defined = true;

    AddCheckString(_methodsString, xzs.p);

    const size_t numBlocks = (size_t)_stat.NumBlocks + 1;
    const size_t bytesAlloc = numBlocks * sizeof(CBlockInfo);
    
    if (bytesAlloc / sizeof(CBlockInfo) == _stat.NumBlocks + 1)
    {
      _blocks = (CBlockInfo *)MyAlloc(bytesAlloc);
      if (_blocks)
      {
        unsigned blockIndex = 0;
        UInt64 unpackPos = 0;
        
        for (size_t si = xzs.p.num; si != 0;)
        {
          si--;
          const CXzStream &str = xzs.p.streams[si];
          UInt64 packPos = str.startOffset + XZ_STREAM_HEADER_SIZE;
          
          for (size_t bi = 0; bi < str.numBlocks; bi++)
          {
            const CXzBlockSizes &bs = str.blocks[bi];
            const UInt64 packSizeAligned = bs.totalSize + ((0 - (unsigned)bs.totalSize) & 3);
            
            if (bs.unpackSize != 0)
            {
              if (blockIndex >= _stat.NumBlocks)
                return E_FAIL;

              CBlockInfo &block = _blocks[blockIndex++];
              block.StreamFlags = str.flags;
              block.PackSize = bs.totalSize; // packSizeAligned;
              block.PackPos = packPos;
              block.UnpackPos = unpackPos;
            }
            packPos += packSizeAligned;
            unpackPos += bs.unpackSize;
            if (_maxBlocksSize < bs.unpackSize)
              _maxBlocksSize = bs.unpackSize;
          }
        }
    
        /*
        if (blockIndex != _stat.NumBlocks)
        {
          // there are Empty blocks;
        }
        */
        if (_stat.OutSize != unpackPos)
          return E_FAIL;
        CBlockInfo &block = _blocks[blockIndex++];
        block.StreamFlags = 0;
        block.PackSize = 0;
        block.PackPos = 0;
        block.UnpackPos = unpackPos;
        _blocksArraySize = blockIndex;
      }
    }
  }
  else
  {
    res = SZ_OK;
  }

  RINOK(SRes_to_Open_HRESULT(res))

  _stream = inStream;
  _seqStream = inStream;
  _isArc = true;
  return S_OK;
}



Z7_COM7F_IMF(CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  {
    Close();
    return Open2(inStream, callback);
  }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  Close();
  _seqStream = stream;
  _isArc = true;
  _needSeekToStart = false;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Close())
{
  XzStatInfo_Clear(&_stat);
  XzStatInfo_Clear(&_stat2);
  _stat_defined = false;
  _stat2_defined = false;
  _stat2_decode_SRes = SZ_OK;

  _isArc = false;
  _needSeekToStart = false;
  _firstBlockWasRead = false;

   _methodsString.Empty();
  _stream.Release();
  _seqStream.Release();

  MyFree(_blocks);
  _blocks = NULL;
  _blocksArraySize = 0;
  _maxBlocksSize = 0;

  return S_OK;
}


struct CXzUnpackerCPP2
{
  Byte *InBuf;
  // Byte *OutBuf;
  CXzUnpacker p;
  
  CXzUnpackerCPP2();
  ~CXzUnpackerCPP2();
};

CXzUnpackerCPP2::CXzUnpackerCPP2(): InBuf(NULL)
  // , OutBuf(NULL)
{
  XzUnpacker_Construct(&p, &g_Alloc);
}

CXzUnpackerCPP2::~CXzUnpackerCPP2()
{
  XzUnpacker_Free(&p);
  MidFree(InBuf);
  // MidFree(OutBuf);
}


Z7_CLASS_IMP_IInStream(
  CInStream
)

  UInt64 _virtPos;
public:
  UInt64 Size;
  UInt64 _cacheStartPos;
  size_t _cacheSize;
  CByteBuffer _cache;
  // UInt64 _startPos;
  CXzUnpackerCPP2 xz;

  void InitAndSeek()
  {
    _virtPos = 0;
    _cacheStartPos = 0;
    _cacheSize = 0;
    // _startPos = startPos;
  }

  CMyComPtr2<IInArchive, CHandler> _handlerSpec;
  // ~CInStream();
};

/*
CInStream::~CInStream()
{
  // _cache.Free();
}
*/

static size_t FindBlock(const CBlockInfo *blocks, size_t numBlocks, UInt64 pos)
{
  size_t left = 0, right = numBlocks;
  for (;;)
  {
    size_t mid = (left + right) / 2;
    if (mid == left)
      return left;
    if (pos < blocks[mid].UnpackPos)
      right = mid;
    else
      left = mid;
  }
}



static HRESULT DecodeBlock(CXzUnpackerCPP2 &xzu,
    ISequentialInStream *seqInStream,
    unsigned streamFlags,
    UInt64 packSize, // pure size from Index record, it doesn't include pad zeros
    size_t unpackSize, Byte *dest
    // , ICompressProgressInfo *progress
    )
{
  const size_t kInBufSize = (size_t)1 << 16;

  XzUnpacker_Init(&xzu.p);

  if (!xzu.InBuf)
  {
    xzu.InBuf = (Byte *)MidAlloc(kInBufSize);
    if (!xzu.InBuf)
      return E_OUTOFMEMORY;
  }
  
  xzu.p.streamFlags = (UInt16)streamFlags;
  XzUnpacker_PrepareToRandomBlockDecoding(&xzu.p);

  XzUnpacker_SetOutBuf(&xzu.p, dest, unpackSize);

  const UInt64 packSizeAligned = packSize + ((0 - (unsigned)packSize) & 3);
  UInt64 packRem = packSizeAligned;

  UInt32 inSize = 0;
  SizeT inPos = 0;
  SizeT outPos = 0;

  HRESULT readRes = S_OK;

  for (;;)
  {
    if (inPos == inSize && readRes == S_OK)
    {
      inPos = 0;
      inSize = 0;
      UInt32 rem = kInBufSize;
      if (rem > packRem)
        rem = (UInt32)packRem;
      if (rem != 0)
        readRes = seqInStream->Read(xzu.InBuf, rem, &inSize);
    }

    SizeT inLen = inSize - inPos;
    SizeT outLen = unpackSize - outPos;
    
    ECoderStatus status;

    const SRes res = XzUnpacker_Code(&xzu.p,
        // dest + outPos,
        NULL,
        &outLen,
        xzu.InBuf + inPos, &inLen,
        (inLen == 0), // srcFinished
        CODER_FINISH_END, &status);

    // return E_OUTOFMEMORY;
    // res = SZ_ERROR_CRC;

    if (res != SZ_OK)
    {
      if (res == SZ_ERROR_CRC)
        return S_FALSE;
      return SResToHRESULT(res);
    }

    inPos += inLen;
    outPos += outLen;

    packRem -= inLen;
  
    const BoolInt blockFinished = XzUnpacker_IsBlockFinished(&xzu.p);

    if ((inLen == 0 && outLen == 0) || blockFinished)
    {
      if (packRem != 0 || !blockFinished || unpackSize != outPos)
        return S_FALSE;
      if (XzUnpacker_GetPackSizeForIndex(&xzu.p) != packSize)
        return S_FALSE;
      return S_OK;
    }
  }
}


Z7_COM7F_IMF(CInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  COM_TRY_BEGIN

  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;

  {
    if (_virtPos >= Size)
      return S_OK; // (Size == _virtPos) ? S_OK: E_FAIL;
    {
      UInt64 rem = Size - _virtPos;
      if (size > rem)
        size = (UInt32)rem;
    }
  }

  if (size == 0)
    return S_OK;

  if (_virtPos < _cacheStartPos || _virtPos >= _cacheStartPos + _cacheSize)
  {
    const size_t bi = FindBlock(_handlerSpec->_blocks, _handlerSpec->_blocksArraySize, _virtPos);
    const CBlockInfo &block = _handlerSpec->_blocks[bi];
    const UInt64 unpackSize = _handlerSpec->_blocks[bi + 1].UnpackPos - block.UnpackPos;
    if (_cache.Size() < unpackSize)
      return E_FAIL;

    _cacheSize = 0;

    RINOK(_handlerSpec->SeekToPackPos(block.PackPos))
    RINOK(DecodeBlock(xz, _handlerSpec->_seqStream, block.StreamFlags, block.PackSize,
        (size_t)unpackSize, _cache))
    _cacheStartPos = block.UnpackPos;
    _cacheSize = (size_t)unpackSize;
  }

  {
    const size_t offset = (size_t)(_virtPos - _cacheStartPos);
    const size_t rem = _cacheSize - offset;
    if (size > rem)
      size = (UInt32)rem;
    memcpy(data, _cache.ConstData() + offset, size);
    _virtPos += size;
    if (processedSize)
      *processedSize = size;
    return S_OK;
  }

  COM_TRY_END
}
 

Z7_COM7F_IMF(CInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += Size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}



static const UInt64 kMaxBlockSize_for_GetStream = (UInt64)1 << 40;

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN

  *stream = NULL;

  if (index != 0)
    return E_INVALIDARG;

  if (!_stat.UnpackSize_Defined
      || _maxBlocksSize == 0 // 18.02
      || _maxBlocksSize > kMaxBlockSize_for_GetStream
      || _maxBlocksSize != (size_t)_maxBlocksSize)
    return S_FALSE;

  size_t memSize;
  if (!NSystem::GetRamSize(memSize))
    memSize = (size_t)sizeof(size_t) << 28;
  {
    if (_maxBlocksSize > memSize / 4)
      return S_FALSE;
  }

  CMyComPtr2<ISequentialInStream, CInStream> spec;
  spec.Create_if_Empty();
  spec->_cache.Alloc((size_t)_maxBlocksSize);
  spec->_handlerSpec.SetFromCls(this);
  // spec->_handler = (IInArchive *)this;
  spec->Size = _stat.OutSize;
  spec->InitAndSeek();

  *stream = spec.Detach();
  return S_OK;
  
  COM_TRY_END
}


static Int32 Get_Extract_OperationResult(const NCompress::NXz::CDecoder &decoder)
{
  Int32 opRes;
  SRes sres = decoder.MainDecodeSRes;
  if (sres == SZ_ERROR_NO_ARCHIVE) // (!IsArc)
    opRes = NExtract::NOperationResult::kIsNotArc;
  else if (sres == SZ_ERROR_INPUT_EOF) // (UnexpectedEnd)
    opRes = NExtract::NOperationResult::kUnexpectedEnd;
  else if (decoder.Stat.DataAfterEnd)
    opRes = NExtract::NOperationResult::kDataAfterEnd;
  else if (sres == SZ_ERROR_CRC) // (CrcError)
    opRes = NExtract::NOperationResult::kCRCError;
  else if (sres == SZ_ERROR_UNSUPPORTED) // (Unsupported)
    opRes = NExtract::NOperationResult::kUnsupportedMethod;
  else if (sres == SZ_ERROR_ARCHIVE) //  (HeadersError)
    opRes = NExtract::NOperationResult::kDataError;
  else if (sres == SZ_ERROR_DATA)  // (DataError)
    opRes = NExtract::NOperationResult::kDataError;
  else if (sres != SZ_OK)
    opRes = NExtract::NOperationResult::kDataError;
  else
    opRes = NExtract::NOperationResult::kOK;
  return opRes;
}




Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  const CXzStatInfo *stat = GetStat();

  if (stat)
    RINOK(extractCallback->SetTotal(stat->InSize))

  UInt64 currentTotalPacked = 0;
  RINOK(extractCallback->SetCompleted(&currentTotalPacked))
  Int32 opRes;
 {
  CMyComPtr<ISequentialOutStream> realOutStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
  
  if (!testMode && !realOutStream)
    return S_OK;

  RINOK(extractCallback->PrepareOperation(askMode))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, true);

  if (_needSeekToStart)
  {
    if (!_stream)
      return E_FAIL;
    RINOK(InStream_SeekToBegin(_stream))
  }
  else
    _needSeekToStart = true;


  NCompress::NXz::CDecoder decoder;

  const HRESULT hres = Decode(decoder, _seqStream, realOutStream, lps);

  if (!decoder.MainDecodeSRes_wasUsed)
    return hres == S_OK ? E_FAIL : hres;

  opRes = Get_Extract_OperationResult(decoder);
  if (opRes == NExtract::NOperationResult::kOK
      && hres != S_OK)
    opRes = NExtract::NOperationResult::kDataError;

  // realOutStream.Release();
 }
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}



#ifndef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *timeType))
{
  *timeType = GET_FileTimeType_NotDefined_for_GetFileTimeType;
  // *timeType = NFileTimeType::kUnix;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback))
{
  COM_TRY_BEGIN

  if (numItems == 0)
  {
    CSeqOutStreamWrap seqOutStream;
    seqOutStream.Init(outStream);
    SRes res = Xz_EncodeEmpty(&seqOutStream.vt);
    return SResToHRESULT(res);
  }
  
  if (numItems != 1)
    return E_INVALIDARG;

  {
    Z7_DECL_CMyComPtr_QI_FROM(
        IStreamSetRestriction,
        setRestriction, outStream)
    if (setRestriction)
      RINOK(setRestriction->SetRestriction(0, 0))
  }

  Int32 newData, newProps;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive))

  if (IntToBool(newProps))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop))
      if (prop.vt != VT_EMPTY)
        if (prop.vt != VT_BOOL || prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
    }
  }

  if (IntToBool(newData))
  {
    UInt64 dataSize;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop))
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      dataSize = prop.uhVal.QuadPart;
    }

    CMyComPtr2_Create<ICompressCoder, NCompress::NXz::CEncoder> encoder;

    CXzProps &xzProps = encoder->xzProps;
    CLzma2EncProps &lzma2Props = xzProps.lzma2Props;

    lzma2Props.lzmaProps.level = GetLevel();

    xzProps.reduceSize = dataSize;
    /*
    {
      NCOM::CPropVariant prop = (UInt64)dataSize;
      RINOK(encoder->SetCoderProp(NCoderPropID::kReduceSize, prop))
    }
    */

    #ifndef Z7_ST

#ifdef _WIN32
    // we don't use chunk multithreading inside lzma2 stream.
    // so we don't set xzProps.lzma2Props.numThreadGroups.
    if (_numThreadGroups > 1)
      xzProps.numThreadGroups = _numThreadGroups;
#endif
    
    UInt32 numThreads = _numThreads;

    const UInt32 kNumThreads_Max = 1024;
    if (numThreads > kNumThreads_Max)
      numThreads = kNumThreads_Max;

    if (!_numThreads_WasForced
        && _numThreads >= 1
        && _memUsage_WasSet)
    {
      COneMethodInfo oneMethodInfo;
      if (!_methods.IsEmpty())
        oneMethodInfo = _methods[0];

      SetGlobalLevelTo(oneMethodInfo);

      const bool numThreads_WasSpecifiedInMethod = (oneMethodInfo.Get_NumThreads() >= 0);
      if (!numThreads_WasSpecifiedInMethod)
      {
        // here we set the (NCoderPropID::kNumThreads) property in each method, only if there is no such property already
        CMultiMethodProps::SetMethodThreadsTo_IfNotFinded(oneMethodInfo, numThreads);
      }

      // printf("\n====== GetProcessGroupAffinity : \n");

      UInt64 cs = _numSolidBytes;
      if (cs != XZ_PROPS_BLOCK_SIZE_AUTO)
        oneMethodInfo.AddProp_BlockSize2(cs);
      cs = oneMethodInfo.Get_Xz_BlockSize();

      if (cs != XZ_PROPS_BLOCK_SIZE_AUTO &&
          cs != XZ_PROPS_BLOCK_SIZE_SOLID)
      {
        const UInt32 lzmaThreads = oneMethodInfo.Get_Lzma_NumThreads();
        const UInt32 numBlockThreads_Original = numThreads / lzmaThreads;

        if (numBlockThreads_Original > 1)
        {
          UInt32 numBlockThreads = numBlockThreads_Original;
          {
            const UInt64 lzmaMemUsage = oneMethodInfo.Get_Lzma_MemUsage(false);
            for (; numBlockThreads > 1; numBlockThreads--)
            {
              UInt64 size = numBlockThreads * (lzmaMemUsage + cs);
              UInt32 numPackChunks = numBlockThreads + (numBlockThreads / 8) + 1;
              if (cs < ((UInt32)1 << 26)) numPackChunks++;
              if (cs < ((UInt32)1 << 24)) numPackChunks++;
              if (cs < ((UInt32)1 << 22)) numPackChunks++;
              size += numPackChunks * cs;
              // printf("\nnumBlockThreads = %d, size = %d\n", (unsigned)(numBlockThreads), (unsigned)(size >> 20));
              if (size <= _memUsage_Compress)
                break;
            }
          }
          if (numBlockThreads == 0)
            numBlockThreads = 1;
          if (numBlockThreads != numBlockThreads_Original)
            numThreads = numBlockThreads * lzmaThreads;
        }
      }
    }
    xzProps.numTotalThreads = (int)numThreads;

    #endif // Z7_ST


    xzProps.blockSize = _numSolidBytes;
    if (_numSolidBytes == XZ_PROPS_BLOCK_SIZE_SOLID)
    {
      xzProps.lzma2Props.blockSize = LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID;
    }

    RINOK(encoder->SetCheckSize(_crcSize))

    {
      CXzFilterProps &filter = xzProps.filterProps;
      
      if (_filterId == XZ_ID_Delta)
      {
        bool deltaDefined = false;
        FOR_VECTOR (j, _filterMethod.Props)
        {
          const CProp &prop = _filterMethod.Props[j];
          if (prop.Id == NCoderPropID::kDefaultProp && prop.Value.vt == VT_UI4)
          {
            UInt32 delta = (UInt32)prop.Value.ulVal;
            if (delta < 1 || delta > 256)
              return E_INVALIDARG;
            filter.delta = delta;
            deltaDefined = true;
          }
          else
            return E_INVALIDARG;
        }
        if (!deltaDefined)
          return E_INVALIDARG;
      }
      filter.id = _filterId;
    }

    FOR_VECTOR (i, _methods)
    {
      COneMethodInfo &m = _methods[i];

      FOR_VECTOR (j, m.Props)
      {
        const CProp &prop = m.Props[j];
        RINOK(encoder->SetCoderProp(prop.Id, prop.Value))
      }
    }

    {
      CMyComPtr<ISequentialInStream> fileInStream;
      RINOK(updateCallback->GetStream(0, &fileInStream))
      if (!fileInStream)
        return S_FALSE;
      {
        CMyComPtr<IStreamGetSize> streamGetSize;
        fileInStream.QueryInterface(IID_IStreamGetSize, &streamGetSize);
        if (streamGetSize)
        {
          UInt64 size;
          if (streamGetSize->GetSize(&size) == S_OK)
            dataSize = size;
        }
      }
      RINOK(updateCallback->SetTotal(dataSize))
      CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
      lps->Init(updateCallback, true);
      RINOK(encoder.Interface()->Code(fileInStream, outStream, NULL, NULL, lps))
    }
      
    return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
  }

  if (indexInArchive != 0)
    return E_INVALIDARG;

  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveUpdateCallbackFile,
      opCallback, updateCallback)
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(NEventIndexType::kInArcIndex, 0, NUpdateNotifyOp::kReplicate))
  }

  if (_stream)
  {
    const CXzStatInfo *stat = GetStat();
    if (stat)
    {
      RINOK(updateCallback->SetTotal(stat->InSize))
    }
    RINOK(InStream_SeekToBegin(_stream))
  }

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);

  return NCompress::CopyStream(_stream, outStream, lps);

  COM_TRY_END
}

#endif


HRESULT CHandler::SetProperty(const wchar_t *nameSpec, const PROPVARIANT &value)
{
  UString name = nameSpec;
  name.MakeLower_Ascii();
  if (name.IsEmpty())
    return E_INVALIDARG;
  
  #ifndef Z7_EXTRACT_ONLY

  if (name[0] == L's')
  {
    const wchar_t *s = name.Ptr(1);
    if (*s == 0)
    {
      bool useStr = false;
      bool isSolid;
      switch (value.vt)
      {
        case VT_EMPTY: isSolid = true; break;
        case VT_BOOL: isSolid = (value.boolVal != VARIANT_FALSE); break;
        case VT_BSTR:
          if (!StringToBool(value.bstrVal, isSolid))
            useStr = true;
          break;
        default: return E_INVALIDARG;
      }
      if (!useStr)
      {
        _numSolidBytes = (isSolid ? XZ_PROPS_BLOCK_SIZE_SOLID : XZ_PROPS_BLOCK_SIZE_AUTO);
        return S_OK;
      }
    }
    return ParseSizeString(s, value,
        0, // percentsBase
        _numSolidBytes) ? S_OK: E_INVALIDARG;
  }

  return CMultiMethodProps::SetProperty(name, value);

  #else

  {
    HRESULT hres;
    if (SetCommonProperty(name, value, hres))
      return hres;
  }

  return E_INVALIDARG;
  
  #endif
}



Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  COM_TRY_BEGIN

  Init();

  for (UInt32 i = 0; i < numProps; i++)
  {
    RINOK(SetProperty(names[i], values[i]))
  }

  #ifndef Z7_EXTRACT_ONLY

  if (!_filterMethod.MethodName.IsEmpty())
  {
    unsigned k;
    for (k = 0; k < Z7_ARRAY_SIZE(g_NamePairs); k++)
    {
      const CMethodNamePair &pair = g_NamePairs[k];
      if (StringsAreEqualNoCase_Ascii(_filterMethod.MethodName, pair.Name))
      {
        _filterId = pair.Id;
        break;
      }
    }
    if (k == Z7_ARRAY_SIZE(g_NamePairs))
      return E_INVALIDARG;
  }

  _methods.DeleteFrontal(GetNumEmptyMethods());
  if (_methods.Size() > 1)
    return E_INVALIDARG;
  if (_methods.Size() == 1)
  {
    AString &methodName = _methods[0].MethodName;
    if (methodName.IsEmpty())
      methodName = k_LZMA2_Name;
    else if (
        !methodName.IsEqualTo_Ascii_NoCase(k_LZMA2_Name)
        && !methodName.IsEqualTo_Ascii_NoCase("xz"))
      return E_INVALIDARG;
  }
  
  #endif

  return S_OK;

  COM_TRY_END
}


REGISTER_ARC_IO(
  "xz", "xz txz", "* .tar", 0xC,
  XZ_SIG, 0
  , NArcInfoFlags::kKeepName
  , 0
  , NULL)

}}

/* ================ unit: CPP/7zip/Archive/ZHandler.cpp ================ */
// ZHandler.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZ {

Z7_CLASS_IMP_CHandler_IInArchive_0

  CMyComPtr<IInStream> _stream;
  UInt64 _packSize;
  // UInt64 _unpackSize;
  // bool _unpackSize_Defined;
};

static const Byte kProps[] =
{
  kpidPackSize
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySizeCantBeDetected: prop = true; break;
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    // case kpidSize: if (_unpackSize_Defined) prop = _unpackSize; break;
    case kpidPackSize: prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}

/*
Z7_CLASS_IMP_COM_1(
  CCompressProgressInfoImp
  , ICompressProgressInfo
)
  CMyComPtr<IArchiveOpenCallback> Callback;
public:
  void Init(IArchiveOpenCallback *callback) { Callback = callback; }
};

Z7_COM7F_IMF(CCompressProgressInfoImp::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  outSize = outSize;
  if (Callback)
  {
    const UInt64 files = 1;
    return Callback->SetCompleted(&files, inSize);
  }
  return S_OK;
}
*/

API_FUNC_static_IsArc IsArc_Z(const Byte *p, size_t size)
{
  if (size < 3)
    return k_IsArc_Res_NEED_MORE;
  if (size > NCompress::NZ::kRecommendedCheckSize)
      size = NCompress::NZ::kRecommendedCheckSize;
  if (!NCompress::NZ::CheckStream(p, size))
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openCallback */))
{
  COM_TRY_BEGIN
  {
    // RINOK(InStream_GetPos(stream, _streamStartPosition));
    Byte buffer[NCompress::NZ::kRecommendedCheckSize];
    // Byte buffer[1500];
    size_t size = NCompress::NZ::kRecommendedCheckSize;
    // size = 700;
    RINOK(ReadStream(stream, buffer, &size))
    if (!NCompress::NZ::CheckStream(buffer, size))
      return S_FALSE;

    UInt64 endPos;
    RINOK(InStream_GetSize_SeekToEnd(stream, endPos))
    _packSize = endPos;
  
    /*
    bool fullCheck = false;
    if (fullCheck)
    {
      CCompressProgressInfoImp *compressProgressSpec = new CCompressProgressInfoImp;
      CMyComPtr<ICompressProgressInfo> compressProgress = compressProgressSpec;
      compressProgressSpec->Init(openCallback);

      NCompress::NZ::CDecoder *decoderSpec = new NCompress::NZ::CDecoder;
      CMyComPtr<ICompressCoder> decoder = decoderSpec;

      CDummyOutStream *outStreamSpec = new CDummyOutStream;
      CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->SetStream(NULL);
      outStreamSpec->Init();
      decoderSpec->SetProp(_prop);
      if (openCallback)
      {
        UInt64 files = 1;
        RINOK(openCallback->SetTotal(&files, &endPos));
      }
      RINOK(InStream_SeekSet(stream, _streamStartPosition + kSignatureSize))
      HRESULT res = decoder->Code(stream, outStream, NULL, NULL, openCallback ? compressProgress : NULL);
      if (res != S_OK)
        return S_FALSE;
      _packSize = decoderSpec->PackSize;
    }
    */
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _packSize = 0;
  // _unpackSize_Defined = false;
  _stream.Release();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;
  RINOK(extractCallback->SetTotal(_packSize))
  UInt64 currentTotalPacked = 0;
  RINOK(extractCallback->SetCompleted(&currentTotalPacked))
  
  int opRes;
  {
  CMyComPtr<ISequentialOutStream> realOutStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
    
  if (!testMode && !realOutStream)
    return S_OK;

  RINOK(extractCallback->PrepareOperation(askMode))

  CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> outStream;
  outStream->SetStream(realOutStream);
  outStream->Init();
  // realOutStream.Release();

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, true);
  
  RINOK(InStream_SeekToBegin(_stream))

  NCompress::NZ::CDecoder decoder;
  {
    const HRESULT hres = decoder.Code(_stream, outStream, lps);
    if (hres == S_FALSE)
      opRes = NExtract::NOperationResult::kDataError;
    else
    {
      RINOK(hres)
      opRes = NExtract::NOperationResult::kOK;
    }
  }
  // _unpackSize = outStreamSpec->GetSize();
  // _unpackSize_Defined = true;
  // outStream.Release();
  }
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

static const Byte k_Signature[] = { 0x1F, 0x9D };

REGISTER_ARC_I(
  "Z", "z taz", "* .tar", 5,
  k_Signature,
  0,
  0,
  IsArc_Z)

}}

/* ================ unit: CPP/7zip/Archive/ZstdHandler.cpp ================ */
// ZstdHandler.cpp

// amalgamation: header emitted in prologue

// #define Z7_USE_ZSTD_ORIG_DECODER
// #define Z7_USE_ZSTD_COMPRESSION

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#ifdef Z7_USE_ZSTD_ORIG_DECODER
#include "../Compress/Zstd2Decoder.h"
#endif

#ifdef Z7_USE_ZSTD_COMPRESSION
#include "../Compress/ZstdEncoder.h"
#include "../Compress/ZstdEncoderProps.h"
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

namespace NArchive {
namespace NZstd {

#define DESCRIPTOR_Get_DictionaryId_Flag(d)   ((d) & 3)
#define DESCRIPTOR_FLAG_CHECKSUM              (1 << 2)
#define DESCRIPTOR_FLAG_RESERVED              (1 << 3)
#define DESCRIPTOR_FLAG_UNUSED                (1 << 4)
#define DESCRIPTOR_FLAG_SINGLE                (1 << 5)
#define DESCRIPTOR_Get_ContentSize_Flag3(d)   ((d) >> 5)
#define DESCRIPTOR_Is_ContentSize_Defined(d)  (((d) & 0xe0) != 0)

struct CFrameHeader
{
  Byte Descriptor;
  Byte WindowDescriptor;
  UInt32 DictionaryId;
  UInt64 ContentSize;

  /* by zstd specification:
      the decoder must check that (Is_Reserved() == false)
      the decoder must ignore Unused_bit */
  bool Is_Reserved() const                { return (Descriptor & DESCRIPTOR_FLAG_RESERVED) != 0; }
  bool Is_Checksum() const                { return (Descriptor & DESCRIPTOR_FLAG_CHECKSUM) != 0; }
  bool Is_SingleSegment() const           { return (Descriptor & DESCRIPTOR_FLAG_SINGLE) != 0; }
  bool Is_ContentSize_Defined() const     { return DESCRIPTOR_Is_ContentSize_Defined(Descriptor); }
  unsigned Get_DictionaryId_Flag() const  { return DESCRIPTOR_Get_DictionaryId_Flag(Descriptor); }
  unsigned Get_ContentSize_Flag3() const  { return DESCRIPTOR_Get_ContentSize_Flag3(Descriptor); }
  
  const Byte *Parse(const Byte *p, int size)
  {
    if ((unsigned)size < 2)
      return NULL;
    Descriptor = *p++;
    size--;
    {
      Byte w = 0;
      if (!Is_SingleSegment())
      {
        w = *p++;
        size--;
      }
      WindowDescriptor = w;
    }
    {
      unsigned n = Get_DictionaryId_Flag();
      UInt32 d = 0;
      if (n)
      {
        n = (unsigned)1 << (n - 1);
        if ((size -= (int)n) < 0)
          return NULL;
        d = GetUi32(p) & ((UInt32)(Int32)-1 >> (32 - 8u * n));
        p += n;
      }
      DictionaryId = d;
    }
    {
      unsigned n = Get_ContentSize_Flag3();
      UInt64 v = 0;
      if (n)
      {
        n >>= 1;
        if (n == 1)
          v = 256;
        n = (unsigned)1 << n;
        if ((size -= (int)n) < 0)
          return NULL;
        v += GetUi64(p) & ((UInt64)(Int64)-1 >> (64 - 8u * n));
        p += n;
      }
      ContentSize = v;
    }
    return p;
  }
};



class CHandler Z7_final:
  public IInArchive,
  public IArchiveOpenSeq,
  public ISetProperties,
#ifdef Z7_USE_ZSTD_COMPRESSION
  public IOutArchive,
#endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(IInArchive)
  Z7_COM_QI_ENTRY(IArchiveOpenSeq)
  Z7_COM_QI_ENTRY(ISetProperties)
#ifdef Z7_USE_ZSTD_COMPRESSION
  Z7_COM_QI_ENTRY(IOutArchive)
#endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
  Z7_IFACE_COM7_IMP(IInArchive)
  Z7_IFACE_COM7_IMP(IArchiveOpenSeq)
  Z7_IFACE_COM7_IMP(ISetProperties)
#ifdef Z7_USE_ZSTD_COMPRESSION
  Z7_IFACE_COM7_IMP(IOutArchive)
#endif

  bool _isArc;
  bool _needSeekToStart;
  // bool _dataAfterEnd;
  // bool _needMoreInput;
  bool _unsupportedBlock;

  bool _wasParsed;
  bool _phySize_Decoded_Defined;
  bool _unpackSize_Defined; // decoded
  bool _decoded_Info_Defined;
  
  bool _parseMode;
  bool _disableHash;
  // bool _smallMode;

  UInt64 _phySize;
  UInt64 _phySize_Decoded;
  UInt64 _unpackSize;

  CZstdDecInfo _parsed_Info;
  CZstdDecInfo _decoded_Info;

  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;

#ifdef Z7_USE_ZSTD_COMPRESSION
  CSingleMethodProps _props;
#endif

public:
  CHandler():
    _parseMode(false),
    _disableHash(false)
    // _smallMode(false)
    {}
};


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidNumStreams,
  kpidNumBlocks,
  kpidMethod,
  // kpidChecksum
  kpidCRC
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps


// static const unsigned kBlockType_Raw = 0;
static const unsigned kBlockType_RLE = 1;
// static const unsigned kBlockType_Compressed = 2;
static const unsigned kBlockType_Reserved = 3;
/*
static const char * const kNames[] =
{
    "RAW"
  , "RLE"
  , "Compressed"
  , "Reserved"
};
*/

static void Add_UInt64(AString &s, const char *name, UInt64 v)
{
  s.Add_OptSpaced(name);
  s.Add_Colon();
  s.Add_UInt64(v);
}


static void PrintSize(AString &s, UInt64 w)
{
  char c = 0;
       if ((w & ((1 << 30) - 1)) == 0)  { c = 'G'; w >>= 30; }
  else if ((w & ((1 << 20) - 1)) == 0)  { c = 'M'; w >>= 20; }
  else if ((w & ((1 << 10) - 1)) == 0)  { c = 'K'; w >>= 10; }
  s.Add_UInt64(w);
  if (c)
  {
    s.Add_Char(c);
    s += "iB";
  }
}


Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;

  CZstdDecInfo *p = NULL;
  if (_wasParsed || !_decoded_Info_Defined)
    p = &_parsed_Info;
  else if (_decoded_Info_Defined)
    p = &_decoded_Info;

  switch (propID)
  {
    case kpidPhySize:
      if (_wasParsed)
        prop = _phySize;
      else if (_phySize_Decoded_Defined)
        prop = _phySize_Decoded;
      break;

    case kpidUnpackSize:
      if (_unpackSize_Defined)
        prop = _unpackSize;
      break;
    
    case kpidNumStreams:
      if (p)
      if (_wasParsed || _decoded_Info_Defined)
        prop = p->num_DataFrames;
      break;
     
    case kpidNumBlocks:
      if (p)
      if (_wasParsed || _decoded_Info_Defined)
        prop = p->num_Blocks;
      break;
    
    // case kpidChecksum:
    case kpidCRC:
      if (p)
        if (p->checksum_Defined && p->num_DataFrames == 1)
          prop = p->checksum; // it's checksum from last frame
      break;

    case kpidMethod:
    {
      AString s;
      s.Add_OptSpaced(p == &_decoded_Info ?
          "decoded:" : _wasParsed ?
          "parsed:" :
          "header-open-only:");
      
      if (p->dictionaryId != 0)
      {
        if (p->are_DictionaryId_Different)
          s.Add_OptSpaced("different-dictionary-IDs");
        s.Add_OptSpaced("dictionary-ID:");
        s.Add_UInt32(p->dictionaryId);
      }
      /*
      if (ContentSize_Defined)
      {
        s.Add_OptSpaced("ContentSize=");
        s.Add_UInt64(ContentSize_Total);
      }
      */
      // if (p->are_Checksums)
      if (p->descriptor_OR & DESCRIPTOR_FLAG_CHECKSUM)
        s.Add_OptSpaced("XXH64");
      if (p->descriptor_NOT_OR & DESCRIPTOR_FLAG_CHECKSUM)
        s.Add_OptSpaced("NO-XXH64");

      if (p->descriptor_OR & DESCRIPTOR_FLAG_UNUSED)
        s.Add_OptSpaced("unused_bit");

      if (p->descriptor_OR & DESCRIPTOR_FLAG_SINGLE)
        s.Add_OptSpaced("single-segments");

      if (p->descriptor_NOT_OR & DESCRIPTOR_FLAG_SINGLE)
      {
        // Add_UInt64(s, "wnd-descriptors", p->num_WindowDescriptors);
        s.Add_OptSpaced("wnd-desc-log-MAX:");
        // WindowDescriptor_MAX = 16 << 3; // for debug
        const unsigned e = p->windowDescriptor_MAX >> 3;
        s.Add_UInt32(e + 10);
        const unsigned m = p->windowDescriptor_MAX & 7;
        if (m != 0)
        {
          s.Add_Dot();
          s.Add_UInt32(m);
        }
      }
      
      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor_OR) ||
          (p->descriptor_NOT_OR & DESCRIPTOR_FLAG_SINGLE))
      /*
      if (p->are_ContentSize_Known ||
          p->are_WindowDescriptors)
      */
      {
        s.Add_OptSpaced("wnd-MAX:");
        PrintSize(s, p->windowSize_MAX);
        if (p->windowSize_MAX != p->windowSize_Allocate_MAX)
        {
          s.Add_OptSpaced("wnd-use-MAX:");
          PrintSize(s, p->windowSize_Allocate_MAX);
        }
      }

      if (p->num_DataFrames != 1)
        Add_UInt64(s, "data-frames", p->num_DataFrames);
      if (p->num_SkipFrames != 0)
      {
        Add_UInt64(s, "skip-frames", p->num_SkipFrames);
        Add_UInt64(s, "skip-frames-size-total", p->skipFrames_Size);
      }

      if (p->are_ContentSize_Unknown)
        s.Add_OptSpaced("unknown-content-size");

      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor_OR))
      {
        Add_UInt64(s, "content-size-frame-max", p->contentSize_MAX);
        Add_UInt64(s, "content-size-total", p->contentSize_Total);
      }
     
      /*
      for (unsigned i = 0; i < 4; i++)
      {
        const UInt64 n = p->num_Blocks_forType[i];
        if (n)
        {
          s.Add_OptSpaced(kNames[i]);
          s += "-blocks:";
          s.Add_UInt64(n);

          s.Add_OptSpaced(kNames[i]);
          s += "-block-bytes:";
          s.Add_UInt64(p->num_BlockBytes_forType[i]);
        }
      }
      */
      prop = s;
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc)            v |= kpv_ErrorFlags_IsNotArc;
      // if (_needMoreInput)     v |= kpv_ErrorFlags_UnexpectedEnd;
      // if (_dataAfterEnd)      v |= kpv_ErrorFlags_DataAfterEnd;
      if (_unsupportedBlock)  v |= kpv_ErrorFlags_UnsupportedMethod;
      /*
      if (_parsed_Info.numBlocks_forType[kBlockType_Reserved])
        v |= kpv_ErrorFlags_UnsupportedMethod;
      */
      prop = v;
      break;
    }

    default: break;
  }
  prop.Detach(value);
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPackSize:
      if (_wasParsed)
        prop = _phySize;
      else if (_phySize_Decoded_Defined)
        prop = _phySize_Decoded;
      break;

    case kpidSize:
      if (_wasParsed && !_parsed_Info.are_ContentSize_Unknown)
        prop = _parsed_Info.contentSize_Total;
      else if (_unpackSize_Defined)
        prop = _unpackSize;
      break;

    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

static const unsigned kSignatureSize = 4;
static const Byte k_Signature[kSignatureSize] = { 0x28, 0xb5, 0x2f, 0xfd } ;

static const UInt32 kDataFrameSignature32    = 0xfd2fb528;
static const UInt32 kSkipFrameSignature      = 0x184d2a50;
static const UInt32 kSkipFrameSignature_Mask = 0xfffffff0;

/*
API_FUNC_static_IsArc IsArc_Zstd(const Byte *p, size_t size)
{
  if (size < kSignatureSize)
    return k_IsArc_Res_NEED_MORE;
  if (memcmp(p, k_Signature, kSignatureSize) != 0)
  {
    const UInt32 v = GetUi32(p);
    if ((v & kSkipFrameSignature_Mask) != kSkipFrameSignature)
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES;
  }
  p += 4;
  // return k_IsArc_Res_YES;
}
}
*/

// kBufSize must be >= (ZSTD_FRAMEHEADERSIZE_MAX = 18)
// we use big buffer for fast parsing of worst case small blocks.
static const unsigned kBufSize =
     1 << 9;
     // 1 << 14; // fastest in real file

struct CStreamBuffer
{
  unsigned pos;
  unsigned lim;
  IInStream *Stream;
  UInt64 StreamOffset;
  Byte buf[kBufSize];

  CStreamBuffer():
      pos(0),
      lim(0),
      StreamOffset(0)
      {}
  unsigned Avail() const { return lim - pos; }
  const Byte *GetPtr() const { return &buf[pos]; }
  UInt64 GetCurOffset() const { return StreamOffset - Avail(); }
  void SkipInBuf(UInt32 size) { pos += size; }
  HRESULT Skip(UInt32 size);
  HRESULT Read(unsigned num);
};

HRESULT CStreamBuffer::Skip(UInt32 size)
{
  unsigned rem = lim - pos;
  if (rem != 0)
  {
    if (rem > size)
      rem = size;
    pos += rem;
    size -= rem;
    if (pos != lim)
      return S_OK;
  }
  if (size == 0)
    return S_OK;
  return Stream->Seek(size, STREAM_SEEK_CUR, &StreamOffset);
}

HRESULT CStreamBuffer::Read(unsigned num)
{
  if (lim - pos >= num)
    return S_OK;
  if (pos != 0)
  {
    lim -= pos;
    memmove(buf, buf + pos, lim);
    pos = 0;
  }
  size_t processed = kBufSize - ((unsigned)StreamOffset & (kBufSize - 1));
  const unsigned avail = kBufSize - lim;
  num -= lim;
  if (avail < processed || processed < num)
    processed = avail;
  const HRESULT res = ReadStream(Stream, buf + lim, &processed);
  StreamOffset += processed;
  lim += (unsigned)processed;
  return res;
}


static const unsigned k_ZSTD_FRAMEHEADERSIZE_MAX = 4 + 14;

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();

  CZstdDecInfo *p = &_parsed_Info;
  // p->are_ContentSize_Unknown = False;
  CStreamBuffer sb;
  sb.Stream = stream;
  
  for (;;)
  {
    RINOK(sb.Read(k_ZSTD_FRAMEHEADERSIZE_MAX))
    if (sb.Avail() < kSignatureSize)
      break;

    if (callback && (ZstdDecInfo_GET_NUM_FRAMES(p) & 0xFFF) == 2)
    {
      const UInt64 numBytes = sb.GetCurOffset();
      RINOK(callback->SetCompleted(NULL, &numBytes))
    }

    const UInt32 v = GetUi32(sb.GetPtr());
    if (v != kDataFrameSignature32)
    {
      if ((v & kSkipFrameSignature_Mask) != kSkipFrameSignature)
        break;
      _phySize = sb.GetCurOffset() + 8;
      p->num_SkipFrames++;
      sb.SkipInBuf(4);
      if (sb.Avail() < 4)
        break;
      const UInt32 size = GetUi32(sb.GetPtr());
      p->skipFrames_Size += size;
      sb.SkipInBuf(4);
      _phySize = sb.GetCurOffset() + size;
      RINOK(sb.Skip(size))
      continue;
    }

    p->num_DataFrames++;
    // _numStreams_Defined = true;
    sb.SkipInBuf(4);
    CFrameHeader fh;
    {
      const Byte *data = fh.Parse(sb.GetPtr(), (int)sb.Avail());
      if (!data)
      {
        // _needMoreInput = true;
        // we set size for one byte more to show that stream was truncated
        _phySize = sb.StreamOffset + 1;
        break;
      }
      if (fh.Is_Reserved())
      {
        // we don't want false detection
        if (ZstdDecInfo_GET_NUM_FRAMES(p) == 1)
          return S_FALSE;
        // _phySize = sb.GetCurOffset();
        break;
      }
      sb.SkipInBuf((unsigned)(data - sb.GetPtr()));
    }

    p->descriptor_OR     = (Byte)(p->descriptor_OR     |  fh.Descriptor);
    p->descriptor_NOT_OR = (Byte)(p->descriptor_NOT_OR | ~fh.Descriptor);

    // _numBlocks_Defined = true;
    // if (fh.Get_DictionaryId_Flag())
    // p->dictionaryId_Cur = fh.DictionaryId;
    if (fh.DictionaryId != 0)
    {
      if (p->dictionaryId == 0)
        p->dictionaryId = fh.DictionaryId;
      else if (p->dictionaryId != fh.DictionaryId)
        p->are_DictionaryId_Different = True;
    }

    UInt32 blockSizeAllowedMax = (UInt32)1 << 17;
    {
      UInt64 winSize = fh.ContentSize;
      UInt64 winSize_forAllocate = fh.ContentSize;
      if (!fh.Is_SingleSegment())
      {
        if (p->windowDescriptor_MAX < fh.WindowDescriptor)
            p->windowDescriptor_MAX = fh.WindowDescriptor;
        const unsigned e = (fh.WindowDescriptor >> 3);
        const unsigned m = (fh.WindowDescriptor & 7);
        winSize = (UInt64)(8 + m) << (e + 10 - 3);
        if (!fh.Is_ContentSize_Defined()
            || fh.DictionaryId != 0
            || winSize_forAllocate > winSize)
          winSize_forAllocate = winSize;
        // p->are_WindowDescriptors = true;
      }
      else
      {
        // p->are_SingleSegments = True;
      }
      if (blockSizeAllowedMax > winSize)
          blockSizeAllowedMax = (UInt32)winSize;
      if (p->windowSize_MAX < winSize)
          p->windowSize_MAX = winSize;
      if (p->windowSize_Allocate_MAX < winSize_forAllocate)
          p->windowSize_Allocate_MAX = winSize_forAllocate;
    }

    if (fh.Is_ContentSize_Defined())
    {
      // p->are_ContentSize_Known = True;
      p->contentSize_Total += fh.ContentSize;
      if (p->contentSize_MAX < fh.ContentSize)
          p->contentSize_MAX = fh.ContentSize;
    }
    else
    {
      p->are_ContentSize_Unknown = True;
    }

    p->checksum_Defined = false;

    // p->numBlocks_forType[3] += 99; // for debug

    if (!_parseMode)
    {
      if (ZstdDecInfo_GET_NUM_FRAMES(p) == 1)
        break;
    }

    _wasParsed = true;
     
    bool blocksWereParsed = false;

    for (;;)
    {
      if (callback && (p->num_Blocks & 0xFFF) == 2)
      {
        // Sleep(10);
        const UInt64 numBytes = sb.GetCurOffset();
        RINOK(callback->SetCompleted(NULL, &numBytes))
      }
      _phySize = sb.GetCurOffset() + 3;
      RINOK(sb.Read(3))
      if (sb.Avail() < 3)
      {
        // _needMoreInput = true;
        // return S_FALSE;
        break; // change it
      }
      const unsigned pos = sb.pos;
      sb.pos = pos + 3;
      UInt32 b = 0;
      b += (UInt32)sb.buf[pos];
      b += (UInt32)sb.buf[pos + 1] << (8 * 1);
      b += (UInt32)sb.buf[pos + 2] << (8 * 2);
      p->num_Blocks++;
      const unsigned blockType = (b >> 1) & 3;
      UInt32 size = b >> 3;
      // p->num_Blocks_forType[blockType]++;
      // p->num_BlockBytes_forType[blockType] += size;
      if (size > blockSizeAllowedMax
          || blockType == kBlockType_Reserved)
      {
        _unsupportedBlock = true;
        if (ZstdDecInfo_GET_NUM_FRAMES(p) == 1 && p->num_Blocks == 1)
          return S_FALSE;
        break;
      }
      if (blockType == kBlockType_RLE)
        size = 1;
      _phySize = sb.GetCurOffset() + size;
      RINOK(sb.Skip(size))
      if (b & 1)
      {
        // it's last block
        blocksWereParsed = true;
        break;
      }
    }

    if (!blocksWereParsed)
      break;

    if (fh.Is_Checksum())
    {
      _phySize = sb.GetCurOffset() + 4;
      RINOK(sb.Read(4))
      if (sb.Avail() < 4)
        break;
      p->checksum_Defined = true;
      // if (p->num_DataFrames == 1)
      p->checksum = GetUi32(sb.GetPtr());
      sb.SkipInBuf(4);
    }
  }
  
  if (ZstdDecInfo_GET_NUM_FRAMES(p) == 0)
    return S_FALSE;

  _needSeekToStart = true;
  // } // _parseMode
  _isArc = true;
  _stream = stream;
  _seqStream = stream;

  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  Close();
  _isArc = true;
  _seqStream = stream;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Close())
{
  _isArc = false;
  _needSeekToStart = false;
  // _dataAfterEnd = false;
  // _needMoreInput = false;
  _unsupportedBlock = false;

  _wasParsed = false;
  _phySize_Decoded_Defined = false;
  _unpackSize_Defined = false;
  _decoded_Info_Defined = false;

  ZstdDecInfo_CLEAR(&_parsed_Info)
  ZstdDecInfo_CLEAR(&_decoded_Info)

  _phySize = 0;
  _phySize_Decoded = 0;
  _unpackSize = 0;

  _seqStream.Release();
  _stream.Release();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
  Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;
  if (_wasParsed)
  {
    RINOK(extractCallback->SetTotal(_phySize))
  }

  Int32 opRes;
  {
    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
    if (!testMode && !realOutStream)
      return S_OK;
      
    extractCallback->PrepareOperation(askMode);
    
    if (_needSeekToStart)
    {
      if (!_stream)
        return E_FAIL;
      RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL))
    }
    else
      _needSeekToStart = true;
    
    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
    lps->Init(extractCallback, true);
    
#ifdef Z7_USE_ZSTD_ORIG_DECODER
    CMyComPtr2_Create<ICompressCoder, NCompress::NZstd2::CDecoder> decoder;
#else
    CMyComPtr2_Create<ICompressCoder, NCompress::NZstd::CDecoder> decoder;
#endif

    CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> outStreamSpec;
    outStreamSpec->SetStream(realOutStream);
    outStreamSpec->Init();
    // realOutStream.Release();
    
    decoder->FinishMode = true;
#ifndef Z7_USE_ZSTD_ORIG_DECODER
    decoder->DisableHash = _disableHash;
#endif
    
    // _dataAfterEnd = false;
    // _needMoreInput = false;
    const HRESULT hres = decoder.Interface()->Code(_seqStream, outStreamSpec, NULL, NULL, lps);
    /*
    {
      UInt64 t1 = decoder->GetInputProcessedSize();
      // for debug
      const UInt32 kTempSize = 64;
      Byte buf[kTempSize];
      UInt32 processedSize = 0;
      RINOK(decoder->ReadUnusedFromInBuf(buf, kTempSize, &processedSize))
      processedSize -= processedSize;
      UInt64 t2 = decoder->GetInputProcessedSize();
      t2 = t2;
      t1 = t1;
    }
    */
    const UInt64 outSize = outStreamSpec->GetSize();
  // }

    // if (hres == E_ABORT) return hres;
    opRes = NExtract::NOperationResult::kDataError;
    
    if (hres == E_OUTOFMEMORY)
    {
      return hres;
      // opRes = NExtract::NOperationResult::kMemError;
    }
    else if (hres == S_OK || hres == S_FALSE)
    {
#ifndef Z7_USE_ZSTD_ORIG_DECODER
      _decoded_Info_Defined = true;
      _decoded_Info = decoder->_state.info;
      // NumDataFrames_Decoded = decoder->_state.info.num_DataFrames;
      // NumSkipFrames_Decoded = decoder->_state.info.num_SkipFrames;
      const UInt64 inSize = decoder->_inProcessed;
#else
      const UInt64 inSize = decoder->GetInputProcessedSize();
#endif
      _phySize_Decoded = inSize;
      _phySize_Decoded_Defined = true;
      
      _unpackSize_Defined = true;
      _unpackSize = outSize;

      // RINOK(
      lps.Interface()->SetRatioInfo(&inSize, &outSize);
      
#ifdef Z7_USE_ZSTD_ORIG_DECODER
      if (hres == S_OK)
        opRes = NExtract::NOperationResult::kOK;
#else
      if (decoder->ResInfo.decode_SRes == SZ_ERROR_CRC)
      {
        opRes = NExtract::NOperationResult::kCRCError;
      }
      else if (decoder->ResInfo.decode_SRes == SZ_ERROR_NO_ARCHIVE)
      {
        _isArc = false;
        opRes = NExtract::NOperationResult::kIsNotArc;
      }
      else if (decoder->ResInfo.decode_SRes == SZ_ERROR_INPUT_EOF)
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
      else
      {
        if (hres == S_OK && decoder->ResInfo.decode_SRes == SZ_OK)
          opRes = NExtract::NOperationResult::kOK;
        if (decoder->ResInfo.extraSize)
        {
          // if (inSize == 0) _isArc = false;
          opRes = NExtract::NOperationResult::kDataAfterEnd;
        }
        /*
        if (decoder->ResInfo.unexpededEnd)
          opRes = NExtract::NOperationResult::kUnexpectedEnd;
        */
      }
#endif
    }
    else if (hres == E_NOTIMPL)
    {
      opRes = NExtract::NOperationResult::kUnsupportedMethod;
    }
    else
      return hres;
  }

  return extractCallback->SetOperationResult(opRes);

  COM_TRY_END
}



Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  // return _props.SetProperties(names, values, numProps);
  // _smallMode = false;
  _disableHash = false;
  _parseMode = false;
  // _parseMode = true; // for debug
#ifdef Z7_USE_ZSTD_COMPRESSION
  _props.Init();
#endif

  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    const PROPVARIANT &value = values[i];
    
    if (name.IsEqualTo("parse"))
    {
      bool parseMode = true;
      RINOK(PROPVARIANT_to_bool(value, parseMode))
        _parseMode = parseMode;
      continue;
    }
    if (name.IsPrefixedBy_Ascii_NoCase("crc"))
    {
      name.Delete(0, 3);
      UInt32 crcSize = 4;
      RINOK(ParsePropToUInt32(name, value, crcSize))
      if (crcSize == 0)
        _disableHash = true;
      else if (crcSize == 4)
        _disableHash = false;
      else
        return E_INVALIDARG;
      continue;
    }
#ifdef Z7_USE_ZSTD_COMPRESSION
    /*
    if (name.IsEqualTo("small"))
    {
      bool smallMode = true;
      RINOK(PROPVARIANT_to_bool(value, smallMode))
      _smallMode = smallMode;
      continue;
    }
    */
    RINOK(_props.SetProperty(names[i], value))
#endif
  }
  return S_OK;
}




#ifdef Z7_USE_ZSTD_COMPRESSION

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *timeType))
{
  *timeType = GET_FileTimeType_NotDefined_for_GetFileTimeType;
  // *timeType = NFileTimeType::kUnix;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback))
{
  COM_TRY_BEGIN

  if (numItems != 1)
    return E_INVALIDARG;
  {
    CMyComPtr<IStreamSetRestriction> setRestriction;
    outStream->QueryInterface(IID_IStreamSetRestriction, (void **)&setRestriction);
    if (setRestriction)
      RINOK(setRestriction->SetRestriction(0, 0))
  }
  Int32 newData, newProps;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive))
 
  if (IntToBool(newProps))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop))
      if (prop.vt != VT_EMPTY)
        if (prop.vt != VT_BOOL || prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
    }
  }

  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop))
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }

    if (!_props.MethodName.IsEmpty()
        && !_props.MethodName.IsEqualTo_Ascii_NoCase("zstd"))
      return E_INVALIDARG;

    {
      CMyComPtr<ISequentialInStream> fileInStream;
      RINOK(updateCallback->GetStream(0, &fileInStream))
      if (!fileInStream)
        return S_FALSE;
      {
        CMyComPtr<IStreamGetSize> streamGetSize;
        fileInStream.QueryInterface(IID_IStreamGetSize, &streamGetSize);
        if (streamGetSize)
        {
          UInt64 size2;
          if (streamGetSize->GetSize(&size2) == S_OK)
            size = size2;
        }
      }
      RINOK(updateCallback->SetTotal(size))

      CMethodProps props2 = _props;

#ifndef Z7_ST
      /*
      CSingleMethodProps (_props)
      derives from
      CMethodProps (props2)
      So we transfer additional variable (num Threads) to CMethodProps list of properties
      */

      UInt32 numThreads = _props._numThreads;

      if (numThreads > Z7_ZSTDMT_NBWORKERS_MAX)
          numThreads = Z7_ZSTDMT_NBWORKERS_MAX;

      if (_props.FindProp(NCoderPropID::kNumThreads) < 0)
      {
        if (!_props._numThreads_WasForced
            && numThreads >= 1
            && _props._memUsage_WasSet)
        {
          NCompress::NZstd::CEncoderProps zstdProps;
          RINOK(zstdProps.SetFromMethodProps(_props))
          ZstdEncProps_NormalizeFull(&zstdProps.EncProps);
          numThreads = ZstdEncProps_GetNumThreads_for_MemUsageLimit(
              &zstdProps.EncProps, _props._memUsage_Compress, numThreads);
        }
        props2.AddProp_NumThreads(numThreads);
      }

#endif // Z7_ST

      CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
      lps->Init(updateCallback, true);
      {
        CMyComPtr2_Create<ICompressCoder, NCompress::NZstd::CEncoder> encoder;
        // size = 1 << 24; // for debug
        RINOK(props2.SetCoderProps(encoder.ClsPtr(), size != (UInt64)(Int64)-1 ? &size : NULL))
        // encoderSpec->_props.SmallFileOpt = _smallMode;
        // we must set kExpectedDataSize just before Code().
        encoder->SrcSizeHint64 = size;
        /*
        CMyComPtr<ICompressSetCoderPropertiesOpt> optProps;
        _compressEncoder->QueryInterface(IID_ICompressSetCoderPropertiesOpt, (void**)&optProps);
        if (optProps)
        {
          PROPID propID = NCoderPropID::kExpectedDataSize;
          NWindows::NCOM::CPropVariant prop = (UInt64)size;
          // RINOK(optProps->SetCoderPropertiesOpt(&propID, &prop, 1))
          RINOK(encoderSpec->SetCoderPropertiesOpt(&propID, &prop, 1))
        }
        */
        RINOK(encoder.Interface()->Code(fileInStream, outStream, NULL, NULL, lps))
      }
    }
    return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
  }

  if (indexInArchive != 0)
    return E_INVALIDARG;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);

  CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(
        NEventIndexType::kInArcIndex, 0,
        NUpdateNotifyOp::kReplicate))
  }

  if (_stream)
    RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL))

  return NCompress::CopyStream(_stream, outStream, lps);

  COM_TRY_END
}
#endif



#ifndef Z7_USE_ZSTD_COMPRESSION
#undef  IMP_CreateArcOut
#define IMP_CreateArcOut
#undef  CreateArcOut
#define CreateArcOut NULL
#endif

#ifdef Z7_USE_ZSTD_COMPRESSION
REGISTER_ARC_IO(
  "zstd2", "zst tzst", "* .tar", 0xe + 1,
  k_Signature, 0
  , NArcInfoFlags::kKeepName
  , 0
  , NULL)
#else
REGISTER_ARC_IO(
  "zstd", "zst tzst", "* .tar", 0xe,
  k_Signature, 0
  , NArcInfoFlags::kKeepName
  , 0
  , NULL)
#endif

}}
